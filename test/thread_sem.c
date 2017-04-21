
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

#include <sys/sem.h>
#include <sys/wait.h>

#include <castor/rr_debug.h>

int main(int argc, const char *argv[])
{
    key_t key = ftok("../sysroot-src/sys/sys/sem.h", 1);
    int sema_id = semget(key, 1, IPC_CREAT | 0660);
    printf("Created semaphore %d.\n", sema_id);

    printf("Forking.\n");

    // FIXME: Based on a comment from mmap_shared_shm, there seems to be an uncovered fork issue?
    pid_t p = fork();
    assert(p != -1);

    if (p == 0) { // Child
        printf("Semaphore %d down.\n", sema_id);

        struct sembuf b = {0, -1, 0};
        semop(sema_id, &b, 1);

        printf("Semaphore %d down'd'.\n", sema_id);
    } else { // Parent
        printf("Semaphore %d up.\n", sema_id);

        struct sembuf b = {0, 1, 0};
        semop(sema_id, &b, 1);

        printf("Semaphore %d up'd.\n", sema_id);
    }

    if (p != 0) {
        wait(NULL);
        // printf("Complete!\n");

        semctl(sema_id, 0, IPC_RMID, 0);
        printf("Removed semaphore %d!\n", sema_id);
    }

    semctl(sema_id, 0, IPC_RMID, 0);
    printf("Removed semaphore %d!\n", sema_id);

    return 0;
}

