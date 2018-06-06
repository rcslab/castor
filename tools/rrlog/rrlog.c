
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

static int logfd;
static RRLogEntry entry;

struct {
    uint32_t	evtid;
    const char* str;
    uint64_t	count;
} rreventTable[] =
{
#define RREVENT(_a, _b) { RREVENT_##_a, #_a, 0 },
    RREVENT_TABLE
#undef RREVENT
    { 0, 0 }
};

uint64_t eventsPerThread[RRLOG_MAX_THREADS];

void dumpEntry()
{
    int i;
    const char *evtStr = "UNKNOWN";

    for (i = 0; rreventTable[i].str != 0; i++) {
	if (rreventTable[i].evtid == entry.event) {
	    evtStr = rreventTable[i].str;
	    rreventTable[i].count++;
	    break;
	}
    }

    eventsPerThread[entry.threadId] += 1;

    printf("%016ld  %08x  %-16s  %016lx  %016lx  %016lx  %016lx  %016lx  %016lx\n",
	    entry.eventId, entry.threadId, evtStr, entry.objectId,
	    entry.value[0], entry.value[1], entry.value[2],
	    entry.value[3], entry.value[4]);
}

int main(int argc, const char *argv[])
{
    int i;
    int flags = O_RDWR;

    if (argc != 2) {
	fprintf(stderr, "Usage: %s [-h] [LOGFILE]\n", argv[0]);
	return 1;
    }

    if (!strcmp(argv[1],"-h")) {
	fprintf(stdout, "Usage: %s [-h] [LOGFILE]\n", argv[0]);
	return 0;
    }

    logfd = open(argv[1], flags, 0600);
    if (logfd < 0) {
	perror("open");
	return 1;
    }

    uint64_t foo;
    read(logfd, (void *)&foo, 8);
    read(logfd, (void *)&foo, 2);

    for (i = 0; i < RRLOG_MAX_THREADS; i++) {
	eventsPerThread[i] = 0;
    }

    printf("%-16s  %-8s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s\n",
	    "Event #", "Thread #", "Event", "Object ID",
	    "Value[0]", "Value[1]", "Value[2]", "Value[3]", "Value[4]");
    while (1) {
	int result = read(logfd, (void *)&entry, sizeof(entry));
	if (result < 0) {
	    perror("read");
	    return 1;
	}
	if (result != sizeof(entry))
	    break;

	dumpEntry();
    }

    printf("\n%-16s      %s\n", "Event", "Count");
    for (i = 0; rreventTable[i].str != 0; i++) {
	if (rreventTable[i].count != 0) {
	    printf("%-16s      %lu\n", rreventTable[i].str, rreventTable[i].count);
	}
    }

    printf("\n%-8s      %s\n", "Thread", "# Events");
    for (i = 0; i < RRLOG_MAX_THREADS; i++) {
	if (eventsPerThread[i] > 0) {
	    printf("%-8d      %lu\n", i, eventsPerThread[i]);
	}
    }

    return 0;
}

