#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define  PORT "58013"

int fd;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];

int main(){
    memset(&hints, 0, sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_NUMERICSERV;

    n = getaddrinfo("127.0.0.1", PORT, &hints, &res);

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    char msg[] = "vivaaa\n";
    n = sendto(fd, msg, strlen(msg),0, res->ai_addr, res->ai_addrlen);

    addrlen = sizeof(addr);

    n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);

    write(1, "echo: ", 6); write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);
}