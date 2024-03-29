
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

enum display_modes {ALL_MODE, TRUSS_MODE, DUMP_MODE};
/* Bit enums */
enum truss_modes {
    TRUSS_TID = 1,
};

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

static int 
openLog(const char * logfile)
{
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

static void 
initStats()
{
    for (int i = 0; i < RRLOG_MAX_THREADS; i++) {
	eventsPerThread[i] = 0;
    }
}

static void 
updateStats(RRLogEntry entry)
{
    rreventTable[entry.event].count++;
    eventsPerThread[entry.threadId] += 1;
}

static void
displayStats()
{
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

static int
readEntry(RRLogEntry * entry)
{
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

static inline void
assertData(RRLogEntry * entry)
{
    if (entry->event != RREVENT_DATA) {
	fprintf(stderr, "invalide entry: something went horribly wrong while reading the log.");
	exit(1);
    }
}

void readData(uint8_t * buf, size_t len) 
{
    assert(buf != NULL);
    uint64_t recs = len / RREVENT_DATA_LEN;
    uint64_t rlen = len % RREVENT_DATA_LEN;
    RRLogEntry entry;

    for (unsigned int i = 0; i < recs; i++) {
	readEntry(&entry);
	assertData(&entry);
	uint8_t *src = ((uint8_t *)&entry) + RREVENT_DATA_OFFSET;
	memcpy(buf, src, RREVENT_DATA_LEN);
	buf += RREVENT_DATA_LEN;
    }
    if (rlen) {
	readEntry(&entry);
	assertData(&entry);
	uint8_t *src = ((uint8_t *)&entry) + RREVENT_DATA_OFFSET;
	memcpy(buf, src, rlen);
    }
}

static void
dumpEntry(RRLogEntry entry)
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

static void
prettyPrintEntryWithTrussOption(RRLogEntry entry, uint32_t trussOption)
{
    if (entry.event > RREVENTS_MAX) {
	fprintf(stderr, "invalid event number %u\n", entry.event);
	exit(1);
    }
    if (trussOption & TRUSS_TID) {
	printf("%d", entry.threadId);
    }
    if (trussOption) {
	printf(": ");
    }
    rreventTable[entry.event].pretty_print(entry);
}

static void 
prettyPrintSyscallsWithTrussOption(RRLogEntry entry, uint32_t trussOption)
{
    if (entry.event > RREVENTS_MAX) {
	fprintf(stderr, "invalid event number %u\n", entry.event);
	exit(1);
    }
    if (GENERATED_SYSCALLS_CONTAINS(entry.event) ||
	    BUILTIN_SYSCALLS_CONTAINS(entry.event)) {
        if (trussOption & TRUSS_TID) {
	    printf("%d", entry.threadId);
	}
	if (trussOption) {
	    printf(": ");
	}
	rreventTable[entry.event].pretty_print(entry);
    }
}

static void
usage()
{
    printf("rrlog [options] [logfile]\n");
    printf("  -d            Dump - display raw log records.\n");
    printf("  -t            Truss - truss mode, just display system calls.\n");
    printf("  -a            All -  display all events. (default)\n");
    printf("  -h            Help - display this message.\n");
    printf("  -s            Summary - show summary of total # of events by type and thread.\n");
    printf("  -H            TID - show TID in truss mode.\n");
}


int main(int argc, char * const argv[])
{
    int mode = ALL_MODE;
    int trussMode = 0;
    char ch;
    int showStats = 0;

    while ((ch = getopt(argc, argv, "dtasHh:")) != -1) {
	switch (ch) {
	    case 'd':
		mode = DUMP_MODE;
		break;
	    case 't':
		mode = TRUSS_MODE;
		break;
	    case 'a':
		mode = ALL_MODE;
		break;

	    /* Truss mode arguments */
	    case 'H': // Show TID
		trussMode |= TRUSS_TID;
		break;

	    /* General arguments */
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
    switch (mode) {
	case ALL_MODE:
	    while (readEntry(&entry)) {
		updateStats(entry);
		prettyPrintEntryWithTrussOption(entry, 0);
	    }
	    break;
	case TRUSS_MODE:
	    while (readEntry(&entry)) {
		updateStats(entry);
		prettyPrintSyscallsWithTrussOption(entry, trussMode);
	    }
	    break;
	case DUMP_MODE:
	    printf("%-16s  %-8s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s\n",
		    "Event #", "Thread #", "Event", "Object ID",
		    "Value[0]", "Value[1]", "Value[2]", "Value[3]", "Value[4]");
	    while (readEntry(&entry)) {
		updateStats(entry);
		dumpEntry(entry);
	    }
	    break;
    }

    if (showStats) {
	displayStats();
    }

    return 0;
}

