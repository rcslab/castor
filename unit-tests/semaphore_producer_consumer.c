#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFF_SIZE 5 /* total number of slots */
#define NP	  3 /* total number of producers */
#define NC	  3 /* total number of consumers */
#define NITERS	  4 /* number of items produced/consumed */

typedef struct {
    int buf[BUFF_SIZE]; /* shared var */
    int in;		/* buf[in%BUFF_SIZE] is the first empty slot */
    int out;		/* buf[out%BUFF_SIZE] is the first full slot */
    sem_t full;		/* keep track of the number of full spots */
    sem_t empty;	/* keep track of the number of empty spots */
    pthread_mutex_t mutex; /* enforce mutual exclusion to shared data */
} sbuf_t;

sbuf_t shared;

void *
Producer(void *arg)
{
    int i, item;
    intptr_t index;

    index = (intptr_t)arg;

    for (i = 0; i < NITERS; i++) {
	/* Produce item */
	item = i;

	/* Prepare to write item to buf */

	/* If there are no empty slots, wait */
	sem_wait(&shared.empty);

	/* If another thread uses the buffer, wait */
	pthread_mutex_lock(&shared.mutex);
	shared.buf[shared.in] = item;
	shared.in = (shared.in + 1) % BUFF_SIZE;
	printf("[P%ld] Producing %d ...\n", index, item);
	fflush(stdout);
	/* Release the buffer */
	pthread_mutex_unlock(&shared.mutex);

	/* Increment the number of full slots */
	sem_post(&shared.full);

	/* Interleave  producer and consumer execution */
	if (i % 2 == 1)
	    sleep(1);
    }
    return NULL;
}

void *
Consumer(void *arg)
{
    int i, item;
    intptr_t index;

    index = (intptr_t)arg;
    for (i = NITERS; i > 0; i--) {
	sem_wait(&shared.full);

	pthread_mutex_lock(&shared.mutex);
	item = shared.buf[shared.out];
	shared.out = (shared.out + 1) % BUFF_SIZE;
	printf("[C%ld] Consuming  %d ...\n", index, item);
	fflush(stdout);
	/* Release the buffer */
	pthread_mutex_unlock(&shared.mutex);

	/* Increment the number of full slots */
	sem_post(&shared.empty);

	/* Interleave  producer and consumer execution */
	if (i % 2 == 1)
	    sleep(1);
    }
    return NULL;
}

int
main()
{
    pthread_t idP[NP], idC[NC];
    int64_t index;

    sem_init(&shared.full, 0, 0);
    sem_init(&shared.empty, 0, BUFF_SIZE);
    pthread_mutex_init(&shared.mutex, NULL);

    for (index = 0; index < NP; index++) {
	/* Create a new producer */
	pthread_create(&idP[index], NULL, Producer, (void *)index);
    }

    for (index = 0; index < NC; index++) {
	/* Create a new consumer */
	pthread_create(&idC[index], NULL, Consumer, (void *)index);
    }

    for (index = 0; index < NP; index++) {
	pthread_join(idP[index], NULL);
    }

    for (index = 0; index < NC; index++) {
	pthread_join(idC[index], NULL);
    }

    return 0;
}
