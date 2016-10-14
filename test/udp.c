#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>

#include <sys/select.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

//read/write example derived from BSD ipc tutorial.

#define DATA "The sea is calm tonight, the tide is full . . ."

int create_sock()
{
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
        exit(1);
    }
    return sock;
}

int bind_sock(int sock)
{
    struct sockaddr_in name;

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = 5555;
    if (bind(sock, (struct sockaddr *)&name, sizeof(name))) {
        perror("binding datagram socket");
        exit(1);
    }

    return sock;
}

void * read_recvmsg(void *arg)
{
    puts("read_recvmsg");
    int sock = create_sock();
    bind_sock(sock);

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
	abort();
    }
    printf("recieved:-->%s\n", buffer);
    close(sock);
    return NULL;
}

void * read_select_recvfrom(void * arg)
{
    puts("read_select_recvfrom");
    int sock = create_sock();
    bind_sock(sock);
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    int result = select(sock+1, &readfds, NULL, NULL, NULL);
    if (result < 0) {
	perror("select");
	abort();
    }

    char buffer[128];
    result = recvfrom(sock, buffer, 128, 0, NULL, NULL);
    if (result < 0) {
	perror("recvfrom");
	abort();
    }

    printf("recieved:-->%s\n", buffer);
    close(sock);
    return NULL;
}

void *
write_sendto(void *arg)
{
    puts("write_sendto");
    int sock = create_sock();
    struct sockaddr_in name;
    struct hostent *hp;
    hp = gethostbyname("localhost");
    bcopy(hp->h_addr, (struct sockaddr *)&name.sin_addr, (size_t)hp->h_length);
    name.sin_family = AF_INET;
    name.sin_port = 5555;
    int result = sendto(sock, DATA, sizeof(DATA), 0, (struct sockaddr *) &name,
	    sizeof(name));

    if (result < 0) {
        perror("sending datagram message");
	exit(1);
    }
    close(sock);
    return NULL;
}

void *
write_sendmsg(void *arg)
{
    puts("write_sendmsg");
    int sock = create_sock();

    struct sockaddr_in dst_addr;
    struct hostent *hp;
    hp = gethostbyname("localhost");
    bcopy(hp->h_addr, (struct sockaddr *)&dst_addr.sin_addr, (size_t)hp->h_length);
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = 5555;

    char buffer[128];
    strlcpy(buffer, DATA, sizeof(DATA));

    struct iovec iov[1];
    iov[0].iov_base=buffer;
    iov[0].iov_len=strlen(buffer);

    struct msghdr msg;
    msg.msg_name=&dst_addr;
    msg.msg_namelen=sizeof(dst_addr);
    msg.msg_iov=iov;
    msg.msg_iovlen=1;
    msg.msg_control=0;
    msg.msg_controllen=0;

    int result = sendmsg(sock, &msg, 0);

    if (result < 0) {
        perror("sending datagram message");
	exit(1);
    }
    close(sock);
    return NULL;
}


int 
main(void) 
{
    pthread_t read_thread, write_thread;
    pthread_create(&read_thread, NULL, read_recvmsg, NULL);
    pthread_create(&write_thread, NULL, write_sendto, NULL);
    pthread_join(read_thread, NULL);
    pthread_join(write_thread, NULL);
    pthread_create(&read_thread, NULL, read_select_recvfrom, NULL);
    pthread_create(&write_thread, NULL, write_sendmsg, NULL);
    pthread_join(read_thread, NULL);
    pthread_join(write_thread, NULL);
    return 0;
}
