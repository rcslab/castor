Most pthread gets passed through, these calls are listed in PASSTHROUGH.

Of the remaining calls, about half (pthread_init_*, pthread_destroy_*, pthread_try[rdw]*lock), signal, broadcast
just wait for result and return (are trivial).

Of the remaining calls:

pthread_create - does book keeping to track calls.

unlock/lock calls - spin_lock|mutex_lock|rwlock_rdlock, rwlock_rwlock:
    synchronize on per-object locks using LEnter/LAlloc/Lappend
    do the actual operation on replay e.g. call spinlock - read cycle counter.

time_lock - Same thing as locked call, but if succeeded on record and failed on
replay, loops until it gets the lock. if it failed on record, just returns the
return value.

detach, join, kill - do call and wait for event on replay- read cycle counter.
timed_join - similar to timed lock in that it loops if saw success on record.

cond_wait, barried_wait - use two events to capture entry and exit order so all threads on a common
variable enter and leave in the appropriate order. cond wait acquires a mutex in keeping with
the post-condition of pthread_cond_wait.

Question: how/when in cycle counter needed.
