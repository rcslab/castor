
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <err.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/Common/ft.h>

#define RRFT_PORT	8051

int rrsock = -1;
extern bool ftMode;

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
	PERROR("socket");
    }

    if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    {   
	PERROR("setsockopt");
    }

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = INADDR_ANY;
    srvaddr.sin_port = htons(RRFT_PORT);

    if (bind(lsock, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0) {
	PERROR("bind");
    }

    if (listen(lsock, 5) < 0) {
	PERROR("listen");
    }

    fprintf(stderr, "Waiting for slave ...\n");

    clilen = sizeof(cliaddr);
    rrsock = accept(lsock, (struct sockaddr *)&cliaddr, &clilen);
    if (rrsock < 0) {
	PERROR("accept");
    }

    // Handshake
    char handshake[8];

    memcpy(&handshake, "RRFTSRVR", 8);
    uint64_t start = __builtin_readcyclecounter();
    write(rrsock, &handshake, 8);
    read(rrsock, &handshake, 8);
    uint64_t stop = __builtin_readcyclecounter();
    if (memcmp(&handshake, "RRFTCLNT", 8) != 0) {
	fprintf(stderr, "Handshake failed!\n");
	abort();
    }

    // Report RTT
    fprintf(stderr, "RTT = %lu\n", (stop-start));

    ftMode = true;
}

void
RRFT_InitSlave(const char *hostname)
{
    int status;
    struct addrinfo hints, *res, *res0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    status = getaddrinfo(hostname, "8051", &hints, &res0);
    if (status) {
	errx(1, "%s", gai_strerror(status));
    }

    rrsock = -1;
    for (res = res0; res; res = res->ai_next) {
	rrsock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (rrsock < 0) {
	    PERROR("socket");
	}

	if (connect(rrsock, res->ai_addr, res->ai_addrlen) < 0) {
	    PERROR("connect");
	}

	break;
    }
    freeaddrinfo(res0);

    if (rrsock == -1) {
	fprintf(stderr, "Couldn't connect to host!\n");
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

    ftMode = true;
}

void
RRFT_Send(uint64_t count, RRLogEntry *evt)
{
    int result;

    result = write(rrsock, evt, count * sizeof(RRLogEntry));
    if (result < 0) {
	PERROR("write");
    }
}

uint64_t
RRFT_Recv(uint64_t count, RRLogEntry *evt)
{
    int result;

    result = read(rrsock, evt, count * sizeof(RRLogEntry));
    if (result < 0) {
	PERROR("read");
    }

    return (uint64_t)result / sizeof(RRLogEntry);
}

