
#include <stdint.h>
#include <threads.h>

/*
 * This file loads late enough to allow thread_local variables to work correctly.
 */

extern void setThreadFunc(uint64_t (*gt)(), void (*st)(uint64_t));

static thread_local uint64_t thrId;

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

__attribute__((constructor))
void
InitialThread()
{
	setThreadFunc(__getThreadId, __setThreadId);
}

