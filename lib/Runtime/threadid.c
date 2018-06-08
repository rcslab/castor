
#include <stdint.h>

static uint64_t __getThreadId();
static void __setThreadId(uint64_t);

uint64_t (*getThreadId)() = __getThreadId;
void (*setThreadId)(uint64_t) = __setThreadId;

static uint64_t thrId;

static uint64_t
__getThreadId()
{
	return thrId;
}

static void
__setThreadId(uint64_t t)
{
	thrId = t;
}

void
setThreadFunc(uint64_t (*gt)(), void (*st)(uint64_t))
{
	getThreadId = gt;
	setThreadId = st;
}

