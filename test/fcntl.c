#include <fcntl.h>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/socket.h>

int main(int argc, const char *argv[])
{
    int fd, fd2, sock;
    int flags = O_NONBLOCK | O_APPEND;
    int result;


    fd = open("fcntl.c", O_RDONLY);
    if (fd == -1) {
        close(fd);
        perror("open");
        assert(false);
    }
    printf("%d = open()\n", fd);


    result = fcntl(fd, F_SETFD, FD_CLOEXEC);
    if (result == -1) {
        close(fd);
        perror("F_SETFD");
        assert(false);
    }
    result = fcntl(fd, F_GETFD);
    if ((result & FD_CLOEXEC) != FD_CLOEXEC) {
        close(fd);
        perror("F_GETFD");
        assert(false);
    }
    printf("FD_CLOEXEC = %d\n", result);


    fd2 = fcntl(fd, F_DUPFD, fd);
    if (fd2 == -1) {
        close(fd);
        close(fd2);
        perror("F_DUPFD");
        assert(false);
    }
    printf("%d = fcntl(%d, F_DUPFD)\n", fd2, fd);
    result = close(fd2);
    assert(result == 0);

    fd2 = fcntl(fd, F_DUPFD_CLOEXEC, fd);
    if (fd2 == -1) {
        close(fd);
        close(fd2);
        perror("F_DUPFD_CLOEXEC");
        assert(false);
    }
    printf("%d = fcntl(%d, F_DUPFD_CLOEXEC)\n", fd2, fd);
    result = fcntl(fd2, F_GETFD);
    if ((result & FD_CLOEXEC) != FD_CLOEXEC) {
        close(fd);
        close(fd2);
        perror("F_DUPFD_CLOEXEC flag not set");
        assert(false);
    }
    printf("FD_CLOEXEC is set for fd %d\n", fd2);
    result = close(fd2);
    assert(result == 0);


    result = fcntl(fd, F_DUP2FD, 8);
    if (result != 8) {
        close(fd);
        close(8);
        perror("F_DUP2FD");
        assert(false);
    }
    printf("%d = fcntl(%d, F_DUP2FD, 8)\n", result, fd);
    result = close(8);
    assert(result == 0);

    result = fcntl(fd, F_DUP2FD_CLOEXEC, 9);
    if (result != 9) {
        close(fd);
        close(9);
        perror("F_DUP2FD_CLOEXEC");
        assert(false);
    }
    printf("%d = fcntl(%d, F_DUP2FD_CLOEXEC, 9)\n", result, fd);
    result = fcntl(result, F_GETFD);
    if ((result & FD_CLOEXEC) != FD_CLOEXEC) {
        close(fd);
        close(9);
        perror("F_DUPFD_CLOEXEC flag not set");
        assert(false);
    }
    printf("FD_CLOEXEC is set for fd %d\n", 9);
    result = close(9);
    assert(result == 0);


    result = fcntl(fd, F_SETFL, flags);
    if (result == -1) {
        close(fd);
        perror("F_SETFL");
        assert(false);
    }
    result = fcntl(fd, F_GETFL, flags);
    if (result != flags) {
        close(fd);
        perror("F_GETFL");
        assert(false);
    }
    printf("flags of %d: %d\n", fd, result);


    sock = socket(PF_INET, SOCK_STREAM, 0);
    result = fcntl(sock, F_SETOWN, getpid());
    if (result == -1) {
        close(fd);
        close(sock);
        perror("F_SETOWN");
        assert(false);
    }
    result = fcntl(sock, F_GETOWN);
    if (result == -1) {
        close(fd);
        close(sock);
        perror("F_GETOWN");
        assert(false);
    }
    printf("sock owner = %d\n", result);
    result = close(sock);
    assert(result == 0);


    result = fcntl(fd, F_READAHEAD, 40);
    if (result == -1) {
        close(fd);
        perror("F_READAHEAD set");
        assert(false);
    }
    result = fcntl(fd, F_READAHEAD);
    if (result == -1) {
        close(fd);
        perror("F_READAHEAD get");
        assert(false);
    }
    result = fcntl(fd, F_READAHEAD, -1);
    assert(result == 0);
    printf("F_READAHEAD seemed to work!\n");

    result = fcntl(fd, F_RDAHEAD, 40);
    if (result == -1) {
        close(fd);
        perror("F_RDAHEAD set");
        assert(false);
    }
    result = fcntl(fd, F_RDAHEAD);
    if (result == -1) {
        close(fd);
        perror("F_RDAHEAD get");
        assert(false);
    }
    result = fcntl(fd, F_RDAHEAD, -1);
    assert(result == 0);
    printf("F_RDAHEAD seemed to work!\n");


    result = close(fd);
    assert(result == 0);
    return 0;
}
