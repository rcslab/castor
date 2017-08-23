#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h> 
#include <stdlib.h>

int create_sock()
{
    int sock;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    return sock;
}

int accept_sock(int sock)
{
    struct sockaddr_in name;

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = 5555;
    if (bind(sock, (struct sockaddr *)&name, sizeof(name))) {
        perror("bind");
        exit(1);
    }
    if (listen(sock, 128)) {
        perror("listen");
        exit(1);
    }
    socklen_t len = sizeof(name);
    int newsock;
    newsock = accept(sock, (struct sockaddr *)&name, &len);
    if (newsock == -1) {
        perror("accept");
        exit(1);
    }
    close(sock);
    return newsock;
}

void * read_recvmsg(void *arg)
{
    puts("read_recvmsg");
    int sock = create_sock();
    sock = accept_sock(sock);

    //recvmsg dance.
    char buffer[128];
    struct sockaddr_storage src_addr;

    struct iovec iov[1];
    iov[0].iov_base=buffer;
    iov[0].iov_len=sizeof(buffer);

    struct msghdr msg;
    msg.msg_name=&src_addr;
    msg.msg_namelen=sizeof(src_addr);
    msg.msg_iov=iov;
    msg.msg_iovlen=1;
    msg.msg_control=0;
    msg.msg_controllen=0;

    puts("waiting");
    int result = recvmsg(sock, &msg, 0);
    if (result < 0) {
        perror("recvmsg");
        exit(1);
    }
    printf("received:-->%s\n", buffer);
    close(sock);
    return NULL;
}

void *
write_sendfile(void *arg)
{
    puts("write_sendfile");
    int sock = create_sock();

    struct sockaddr_in dst_addr;
    struct hostent *hp;
    hp = gethostbyname("localhost");
    bcopy(hp->h_addr, (struct sockaddr *)&dst_addr.sin_addr, (size_t)hp->h_length);
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = 5555;
    connect(sock, (const struct sockaddr *)&dst_addr, sizeof(dst_addr));
    
    int fd = open("read.c", O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    
    int result = sendfile(fd, sock, 0, 0, NULL, NULL, 0);
    if (result != 0) {
        perror("write_sendfile");
        exit(1);
    }
    
    close(sock);
    return NULL;
}

int
main()
{
    pthread_t read_thread, write_thread;
    pthread_create(&read_thread, NULL, read_recvmsg, NULL);
    pthread_create(&write_thread, NULL, write_sendfile, NULL);
    pthread_join(read_thread, NULL);
    pthread_join(write_thread, NULL);
    return 0;
}
    