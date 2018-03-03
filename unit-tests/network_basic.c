
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

    int optval;
    socklen_t optlen;
    status = getsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, &optlen);
    assert(status == 0);
    assert(optval);

    status = bind(s, (struct sockaddr *)&local, sizeof(local));
    assert(status >= 0);

    status = listen(s, 10);
    assert(status >= 0);

    pthread_cond_signal(&started);

    socklen_t sz = sizeof(remote);
    fd = accept(s, (struct sockaddr *)&remote, &sz);

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    status = getpeername(fd, (struct sockaddr *)&name, &namelen);
    assert(status == 0);
    char *ip = inet_ntoa(name.sin_addr);
    assert(!strcmp(ip, "127.0.0.1"));
    printf("getpeername() %s:%hu\n", ip, ntohs(name.sin_port));

    status = getsockname(fd, (struct sockaddr *)&name, &namelen);
    assert(status == 0);
    ip = inet_ntoa(name.sin_addr);
    uint16_t port = ntohs(name.sin_port);
    assert(!strcmp(ip, "127.0.0.1"));
    assert(port == 7777);
    printf("getsockname() %s:%hu\n", ip, port);

    char buf[3];
    assert(read(fd, buf, 3) == 3);
    printf("TEST: %s", buf);
    assert(shutdown(fd, SHUT_RD) == 0);
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
    assert(write(fd, "A\n",3) == 3);
    assert(shutdown(fd, SHUT_WR) == 0);
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

