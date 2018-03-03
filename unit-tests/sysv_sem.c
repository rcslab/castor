
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

#include <sys/sem.h>
#include <sys/wait.h>

#include <castor/rr_debug.h>

int main(int argc, const char *argv[])
{
    key_t key;
    int sema_id;
    pid_t p;
    struct sembuf op_arg;
    int proc_id, val, n_waiting;

    key = ftok("sysv_sem.c", 1);
    if (key == -1) {
        perror("ftok");
        assert(false);
    }

    sema_id = semget(key, 1, IPC_CREAT | 0660);
    if (sema_id == -1) {
        perror("semget");
        assert(false);
    }

    printf("Created semaphore %d.\n", sema_id);

    p = fork();
    assert(p != -1);

    op_arg.sem_num = 0;
    op_arg.sem_op = 1;
    op_arg.sem_flg = 0;

    if (p == 0) { // Child
        printf("Semaphore %d down.\n", sema_id);

        op_arg.sem_op = -1;
        if (semop(sema_id, &op_arg, 1)) {
            semctl(sema_id, 0, IPC_RMID, 0);
            perror("semop");
            assert(false);
        }

        proc_id = semctl(sema_id, 0, GETPID, 0);
        if (proc_id == -1) {
            semctl(sema_id, 0, IPC_RMID, 0);
            perror("semctl");
            assert(false);
        }

        val = semctl(sema_id, 0, GETVAL, 0);
        if (val == -1) {
            semctl(sema_id, 0, IPC_RMID, 0);
            perror("semctl");
            assert(false);
        }

        n_waiting = semctl(sema_id, 0, GETZCNT, 0);
        if (n_waiting == -1) {
            semctl(sema_id, 0, IPC_RMID, 0);
            perror("semctl");
            assert(false);
        }

        printf("Semaphore %d (%d) down'd by proc %d.\n", sema_id, val, proc_id);
        printf("There are %d other threads waiting for it to turn 0.\n", n_waiting);

    } else { // Parent
        printf("Semaphore %d up.\n", sema_id);

        op_arg.sem_op = 1;
        if (semop(sema_id, &op_arg, 1)) {
            semctl(sema_id, 0, IPC_RMID, 0);
            perror("semop");
            assert(false);
        }

        proc_id = semctl(sema_id, 0, GETPID, 0);
        if (proc_id == -1) {
            semctl(sema_id, 0, IPC_RMID, 0);
            perror("semctl");
            assert(false);
        }

        val = semctl(sema_id, 0, GETVAL, 0);
        if (val == -1) {
            semctl(sema_id, 0, IPC_RMID, 0);
            perror("semctl");
            assert(false);
        }

        n_waiting = semctl(sema_id, 0, GETZCNT, 0);
        if (n_waiting == -1) {
            semctl(sema_id, 0, IPC_RMID, 0);
            perror("semctl");
            assert(false);
        }

        printf("Semaphore %d (%d) up'd by proc %d.\n", sema_id, val, proc_id);
        printf("There are %d other threads waiting for it to turn 0.\n", n_waiting);
    }

    if (p != 0) {
        wait(NULL);

        union semun arg;
        arg.val = 5;
        if (semctl(sema_id, 0, SETVAL, arg)) { // Set semaphore value
            perror("semctl");
            assert(false);
        }

        val = semctl(sema_id, 0, GETVAL, 0);
        if (val == -1) {
            perror("semctl");
            assert(false);
        }

        printf("Semaphore %d value changed to %d.\n", sema_id, val);

        if (semctl(sema_id, 0, IPC_RMID, 0)) { // Remove semaphore
            perror("semctl");
            assert(false);
        }

        printf("Removed semaphore %d!\n", sema_id);
    }

    return 0;
}
