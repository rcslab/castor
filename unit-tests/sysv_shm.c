
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, const char *argv[])
{
    pid_t pid;
    int shmid;
    key_t shmkey;
    void *region = NULL;

    shmkey = ftok("sysv_shm.c", 0);
    if (shmkey == -1) {
	perror("ftok");
	assert(false);
    }
    printf("shmkey: %lx\n", shmkey);

    pid = fork();

    shmid = shmget(shmkey, 4096, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (shmid  == -1) {
	perror("shmget");
	assert(false);
    }
    printf("shmid: %x\n", shmid);

    region = shmat(shmid, NULL, 0);
    if (region == MAP_FAILED) {
	shmctl(shmid, IPC_RMID, NULL);
	perror("shmat");
	assert(false);
    }
    if (shmdt(region) == -1) {
	perror("shmdt");
	assert(false);
    }

    if (pid != 0) {
	wait(NULL);
	shmctl(shmid, IPC_RMID, NULL);
    }
}

