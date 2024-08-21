
#include <stdint.h>
#include <threads.h>

static thread_local uint64_t thrId;

uint64_t
getThreadId()
{
	return thrId;
}

void
setThreadId(uint64_t t)
{
	thrId = t;
}


