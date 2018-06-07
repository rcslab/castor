
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <castor/archconfig.h>
#include <castor/rrshared.h>
#include <castor/events.h>
#include <castor/file_format.h>

#include "events_pretty_printer_gen.h"

enum display_modes {TRUSS_MODE, DUMP_MODE};

static int logfd;

struct {
    uint32_t	evtid;
    const char* str;
    void (*pretty_print)(RRLogEntry entry);
    uint64_t	count;
} rreventTable[] =
{
#define RREVENT(_a, _b) { RREVENT_##_a, #_a, pretty_print_##_a, 0 },
    RREVENT_TABLE
#undef RREVENT
    { 0, 0 }
};

uint64_t eventsPerThread[RRLOG_MAX_THREADS];

int openLog(const char * logfile) {
    castor_magic_t magic;
    castor_version_t version;
    const int flags = O_RDWR;

    int logfd = open(logfile, flags, 0600);

    if (logfd < 0) {
	fprintf(stderr, "Could not open record/replay log '%s'", logfile);
	perror("open");
	exit(1);
    }

    read(logfd, &magic, sizeof(magic));
    read(logfd, &version, sizeof(version));
    if (magic != CASTOR_MAGIC_NUMBER) {
	fprintf(stderr, "OpenLog failed: %s does not appear to be a log file, invalid "\
			"magic number at file start.\n", logfile);
	exit(1);
    }
    if (version != CASTOR_VERSION_NUMBER) {
	fprintf(stderr, "OpenLog failed: %s recorded with version %d,"\
			"current version is %d.\n",
			logfile, version, CASTOR_VERSION_NUMBER);
	exit(1);
    }

    return logfd;
}

void initStats()
{
    for (int i = 0; i < RRLOG_MAX_THREADS; i++) {
	eventsPerThread[i] = 0;
    }
}

void updateStats(RRLogEntry entry) {
    rreventTable[entry.event].count++;
    eventsPerThread[entry.threadId] += 1;
}

void displayStats() {
    printf("\n%-16s      %s\n", "Event", "Count");
    for (int i = 0; rreventTable[i].str != 0; i++) {
	if (rreventTable[i].count != 0) {
	    printf("%-16s      %lu\n", rreventTable[i].str, rreventTable[i].count);
	}
    }

    printf("\n%-8s      %s\n", "Thread", "# Events");
    for (int i = 0; i < RRLOG_MAX_THREADS; i++) {
	if (eventsPerThread[i] > 0) {
	    printf("%-8d      %lu\n", i, eventsPerThread[i]);
	}
    }
}

int readEntry(RRLogEntry * entry){
    int result = read(logfd, (void *)entry, sizeof(RRLogEntry));
    if (result < 0) {
	perror("read");
	return 0;
    }
    if (result != sizeof(*entry)) {
	return 0;
    }
    return 1;
}

void dumpEntry(RRLogEntry entry)
{
    if (entry.event > RREVENTS_MAX) {
	fprintf(stderr, "invalid event number %u\n", entry.event);
	exit(1);
    }
    const char * event_name = rreventTable[entry.event].str;

    printf("%016ld  %08x  %-16s  %016lx  %016lx  %016lx  %016lx  %016lx  %016lx\n",
	    entry.eventId, entry.threadId, event_name, entry.objectId,
	    entry.value[0], entry.value[1], entry.value[2],
	    entry.value[3], entry.value[4]);
}

void prettyPrintEntry(RRLogEntry entry)
{
    if (entry.event > RREVENTS_MAX) {
	fprintf(stderr, "invalid event number %u\n", entry.event);
	exit(1);
    }

    rreventTable[entry.event].pretty_print(entry);
}


void
usage()
{
    printf("rrlog [options] [logfile]\n");
    printf("  -d	    Dump - display raw log records.\n");
    printf("  -h            Help - display this message.\n");
    printf("  -s            Summary - show summary of total # of events by type and thread.\n");
}


int main(int argc, char * const argv[])
{
    int mode = TRUSS_MODE;
    char ch;
    int showStats = 0;

    while ((ch = getopt(argc, argv, "dsh:")) != -1) {
	switch (ch) {
	    case 'd':
		mode = DUMP_MODE;
		break;
	    case 's':
		showStats = 1;
		break;
	    case 'h':
		usage();
		exit(0);
		break;
	    default:
		usage();
		exit(1);
	}
    }
    argc -= optind;
    argv += optind;

    if (argc == 0) {
	usage();
	exit(1);
    }

    logfd = openLog(argv[0]);
    initStats();

    RRLogEntry entry;
    if (mode == DUMP_MODE) {
	printf("%-16s  %-8s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s\n",
		"Event #", "Thread #", "Event", "Object ID",
		"Value[0]", "Value[1]", "Value[2]", "Value[3]", "Value[4]");

	while (readEntry(&entry)) {
	    updateStats(entry);
	    dumpEntry(entry);
	}
    } else if (mode == TRUSS_MODE) {
	while (readEntry(&entry)) {
	    updateStats(entry);
	    prettyPrintEntry(entry);
	}
    }

    if (showStats) {
	displayStats();
    }

    return 0;
}

