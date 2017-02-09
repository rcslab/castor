
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <atomic>

#include <castor/archconfig.h>
#include <castor/rrshared.h>
#include <castor/events.h>

#include <map>
#include <stack>
#include <string>
#include <cmath>

using namespace std;

static int logfd;

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

struct ThreadInfo
{
    stack<uint64_t>	wstart; // Wait
    stack<uint64_t>	hstart; // Held
};

struct MutexInfo
{
    uint64_t		wsum_x; // Wait
    uint64_t		wsum_x2;
    uint64_t		hsum_x; // Held
    uint64_t		hsum_x2;
    uint64_t		count;
};

map<uint64_t, ThreadInfo> tinfo;
map<uint64_t, MutexInfo> minfo;

void
initWait()
{
    int i;

    for (i = 0; i < RRLOG_MAX_THREADS; i++) {
	tinfo[(uint64_t)i] = { {} };
	eventsPerThread[i] = 0;
    }
}

void
processWait(RRLogEntry *entry)
{
    switch (entry->event) {
	case RREVENT_LOCKED_EVENT:
	    tinfo[entry->threadId].wstart.push(entry->value[4]);
	    break;
	case RREVENT_MUTEX_LOCK: {
	    uint64_t s = tinfo[entry->threadId].wstart.top();
	    tinfo[entry->threadId].wstart.pop();
	    tinfo[entry->threadId].hstart.push(entry->value[4]);
	    s = entry->value[4] - s;
	    eventsPerThread[entry->threadId] += 1;
	    minfo[entry->objectId].wsum_x += s;
	    minfo[entry->objectId].wsum_x2 += s*s;
	    minfo[entry->objectId].count++;
	    break;
	}
	case RREVENT_MUTEX_UNLOCK: {
	    uint64_t s = tinfo[entry->threadId].hstart.top();
	    tinfo[entry->threadId].hstart.pop();
	    s = entry->value[4] - s;
	    minfo[entry->objectId].hsum_x += s;
	    minfo[entry->objectId].hsum_x2 += s*s;
	    break;
	}
	case RREVENT_MUTEX_TRYLOCK:
	case RREVENT_ATOMIC_LOAD:
	case RREVENT_ATOMIC_STORE:
	case RREVENT_ATOMIC_RMW:
	case RREVENT_ATOMIC_XCHG:
	case RREVENT_INLINE_ASM:
	    NOT_IMPLEMENTED();
	    break;
	default:
	    break;
    }
}

void
printWait()
{
    int i;

    printf("\n%-8s      %s\n", "Thread", "# Lock Events");
    for (i = 0; i < RRLOG_MAX_THREADS; i++) {
	if (eventsPerThread[i] > 0) {
	    printf("%-8d      %lu\n", i, eventsPerThread[i]);
	}
    }

    printf("\n");

    printf("%-14s %12s %12s %12s %12s %12s\n",
	   "Mutex", "Avg Wait", "StdDev", "Avg Held", "StdDev", "Count");
    for (auto &x : minfo) {
	uint64_t wavg = x.second.wsum_x / x.second.count;
	uint64_t wx2avg = x.second.wsum_x2 / x.second.count;
	double wstddev = sqrt((double)(wx2avg - wavg*wavg));
	uint64_t havg = x.second.hsum_x / x.second.count;
	uint64_t hx2avg = x.second.hsum_x2 / x.second.count;
	double hstddev = sqrt((double)(hx2avg - havg*havg));
	printf("%014lx %12lu  %12.2f  %12lu %12.2f %12lu\n",
	       x.first, wavg, wstddev, havg, hstddev, x.second.count);
    }
}

void
initStats()
{
    int i;

    for (i = 0; i < RRLOG_MAX_THREADS; i++) {
	eventsPerThread[i] = 0;
    }

    for (i = 0; rreventTable[i].str != 0; i++) {
	rreventTable[i].count = 0;
    }
}

void
processStats(RRLogEntry *entry)
{
    int i;
    const char *evtStr;

    for (i = 0; rreventTable[i].str != 0; i++) {
	if (rreventTable[i].evtid == entry->event) {
	    evtStr = rreventTable[i].str;
	    rreventTable[i].count++;
	    break;
	}
    }

    eventsPerThread[entry->threadId] += 1;
}

void
printStats()
{
    int i;

    printf("%-16s      %s\n", "Event", "Count");
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
}

int main(int argc, const char *argv[])
{
    int flags = O_RDWR;
    void (*init)();
    void (*process)(RRLogEntry *);
    void (*print)();
    RRLogEntry entry;

    if (argc != 2 && argc != 3) {
	fprintf(stderr, "Usage: %s TOOL [LOGFILE]\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "Tools:\n");
	fprintf(stderr, "    rank    Lock ranking\n");
	fprintf(stderr, "    wait    Wait times for locks\n");
	fprintf(stderr, "    stats   Event statistics\n");
	return 1;
    }

    if (strcmp(argv[1], "rank") == 0) {
    } else if (strcmp(argv[1], "wait") == 0) {
	init = &initWait;
	process = &processWait;
	print = &printWait;
    } else if (strcmp(argv[1], "stats") == 0) {
	init = &initStats;
	process = &processStats;
	print = &printStats;
    } else {
	fprintf(stderr, "Unknown tool!\n");
	return 1;
    }

    logfd = open((argc == 3) ? argv[2] : "default.rr", flags, 0600);
    if (logfd < 0) {
	perror("open");
	return 1;
    }

    initStats();

    while (1) {
	int result = read(logfd, (void *)&entry, sizeof(entry));
	if (result < 0) {
	    perror("read");
	    return 1;
	}
	if (result == 0) {
	    break;
	}
	if (result != sizeof(entry)) {
	    fprintf(stderr, "Read a fragment of an entry (%d of %d bytes)\n",
		    result, (int)sizeof(entry));
	    break;
	}

	process(&entry);
    }

    print();

    return 0;
}

