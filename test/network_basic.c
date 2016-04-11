
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>

static pthread_mutex_t mtx;
static pthread_cond_t started;

void *
server(void *arg)
{
    int s;
    int fd;
    int status;
    struct sockaddr_in local, remote;

    bzero(&local, sizeof(local));
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(7777);
    local.sin_family = AF_INET;
    local.sin_len = sizeof(local);

    bzero(&remote, sizeof(remote));

    s = socket(PF_INET, SOCK_STREAM, 0);
    assert(s > 0);

    int yes = 1;
    status = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    assert(status >= 0);

    status = bind(s, (struct sockaddr *)&local, sizeof(local));
    assert(status >= 0);

    status = listen(s, 10);
    assert(status >= 0);

    pthread_cond_signal(&started);

    socklen_t sz = sizeof(remote);
    fd = accept(s, (struct sockaddr *)&remote, &sz);
    close(fd);

    return NULL;
}

void *
client(void *arg)
{
    int fd;
    int status;
    struct sockaddr_in local, server;

    bzero(&local, sizeof(local));
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(0);
    local.sin_family = AF_INET;
    local.sin_len = sizeof(local);

    bzero(&server, sizeof(server));
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server.sin_port = htons(7777);
    server.sin_family = AF_INET;
    server.sin_len = sizeof(server);

    fd = socket(PF_INET, SOCK_STREAM, 0);
    assert(fd > 0);

    status = connect(fd, (struct sockaddr *)&server, sizeof(server));
    if (status < 0) {
	perror("connect");
    }
    assert(status >= 0);

    close(fd);

    return NULL;
}

int main(int argc, const char *argv[])
{
    int status;
    pthread_t s;
    pthread_t c;

    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&started, NULL);

    status = pthread_create(&s, NULL, &server, (void *)1);
    assert(status == 0);

    pthread_mutex_lock(&mtx);
    pthread_cond_wait(&started, &mtx);
    pthread_mutex_unlock(&mtx);

    status = pthread_create(&c, NULL, &client, (void *)2);
    assert(status == 0);

    pthread_join(s, NULL);
    pthread_join(c, NULL);
}

