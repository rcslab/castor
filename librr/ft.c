
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "rrlog.h"
#include "ft.h"

#define RRFT_PORT	8051

int rrsock = -1;

void
RRFT_InitMaster()
{
    int lsock;
    socklen_t clilen;
    int yes = 1;
    struct sockaddr_in srvaddr;
    struct sockaddr_in cliaddr;

    lsock = socket(AF_INET, SOCK_STREAM, 0);
    if (lsock < 0) {
	perror("socket");
	abort();
    }

    if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    {   
	perror("setsockopt");
    }

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = INADDR_ANY;
    srvaddr.sin_port = htons(RRFT_PORT);

    if (bind(lsock, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0) {
	perror("bind");
	abort();
    }

    if (listen(lsock, 5) < 0) {
	perror("listen");
	abort();
    }

    fprintf(stderr, "Waiting for slave ...\n");

    clilen = sizeof(cliaddr);
    rrsock = accept(lsock, (struct sockaddr *)&cliaddr, &clilen);
    if (rrsock < 0) {
	perror("accept");
	abort();
    }

    // Handshake
    char handshake[8];

    memcpy(&handshake, "RRFTSRVR", 8);
    write(rrsock, &handshake, 8);
    read(rrsock, &handshake, 8);
    if (memcmp(&handshake, "RRFTCLNT", 8) != 0) {
	fprintf(stderr, "Handshake failed!\n");
	abort();
    }

    // XXX: Report RTT
}

void
RRFT_InitSlave(const char *hostname)
{
    int lsock;
    int clilen;
    struct sockaddr_in srvaddr;
    struct sockaddr_in cliaddr;

    rrsock = socket(AF_INET, SOCK_STREAM, 0);
    if (rrsock < 0) {
	perror("socket");
	abort();
    }

    // XXX: gethostbyname

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = INADDR_ANY;
    srvaddr.sin_port = htons(RRFT_PORT);

    if (connect(rrsock, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0) {
	perror("connect");
	abort();
    }

    // Handshake
    char handshake[8];

    read(rrsock, &handshake, 8);
    if (memcmp(&handshake, "RRFTSRVR", 8) != 0) {
	fprintf(stderr, "Handshake failed!\n");
	abort();
    }
    memcpy(&handshake, "RRFTCLNT", 8);
    write(rrsock, &handshake, 8);

    // XXX: Report RTT
}

void
RRFT_Send(int count, RRLogEntry *evt)
{
    int result;

    result = write(rrsock, evt, count * sizeof(RRLogEntry));
    if (result < 0) {
	perror("write");
	abort();
    }
}

int
RRFT_Recv(int count, RRLogEntry *evt)
{
    int result;

    result = read(rrsock, evt, count * sizeof(RRLogEntry));
    if (result < 0) {
	perror("read");
	abort();
    }

    return result / sizeof(RRLogEntry);
}

