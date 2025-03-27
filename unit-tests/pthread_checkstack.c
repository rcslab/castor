#include "stdio.h"

#include "pthread.h"
#include <pthread_np.h>

void *
print_stack(void *arg)
{
    size_t stacksize;
    void *stackaddr = NULL;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_get_np(pthread_self(), &attr);
    pthread_attr_getstack(&attr, &stackaddr, &stacksize);
    printf("[%ld]: stack addr %p size %zu fp0 %p\n", (long)arg, stackaddr, stacksize, __builtin_frame_address(0));
    pthread_attr_destroy(&attr);
    return NULL;
}

int
main()
{
    pthread_t t;

    print_stack((void *)0); 

    pthread_create(&t, NULL, print_stack, (void *)1);
    pthread_join(t, NULL);
}
