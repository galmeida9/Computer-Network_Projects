#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "58013"

int nUDP, nTCP, fdUDP, fdTCP, newfd;
socklen_t addrlenUDP, addrlenTCP;
struct addrinfo hintsUDP, hintsTCP, *resUDP, *resTCP;
struct sockaddr_in addrUDP, addrTCP;
char buffer[128];

void handleKill(int sig){
    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);
    _Exit(EXIT_SUCCESS);
}

int main(int argc, char** argv){
    char port[6];
    struct sigaction handle_kill;
    sigset_t int_set;
    sigemptyset(&int_set);
    sigaddset(&int_set, SIGINT);
    handle_kill.sa_handler = handleKill;
    handle_kill.sa_flags = SA_RESTART;
    sigemptyset(&handle_kill.sa_mask);
    sigaction(SIGINT, &handle_kill, NULL);

    strcpy(port, PORT);

    printf("Welcome to FS\n");

    /*Get port from arguments*/
    int opt; 

    if (argc == 2){
        printf("The port is missing.\n");
        exit(1);
    }
       
    while((opt = getopt(argc, argv, "p:")) != -1) {  
        switch(opt) {   
            case 'p':
                strcpy(port, optarg);
                break;
        }
    }

    printf("Port: %s\n", port);

    /*UDP Server*/
    memset(&hintsUDP,0, sizeof(hintsUDP));
    hintsUDP.ai_family=AF_INET;
    hintsUDP.ai_socktype=SOCK_DGRAM;
    hintsUDP.ai_flags=AI_PASSIVE|AI_NUMERICSERV;
    
    nUDP = getaddrinfo(NULL, port, &hintsUDP, &resUDP);
    if (nUDP !=0 ) /*error*/ exit(1);

    fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol);
    if (fdUDP == -1) /*error*/ exit(1);

    nUDP = bind(fdUDP, resUDP->ai_addr, resUDP->ai_addrlen);
    if (nUDP == -1) /*error*/ exit(1);
    
    printf("Created UDP Server\n");
    

    /*TCP Server*/
    memset(&hintsTCP, 0, sizeof(hintsTCP));
    hintsTCP.ai_family = AF_INET;
    hintsTCP.ai_socktype = SOCK_STREAM;
    hintsTCP.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    nTCP = getaddrinfo(NULL, port, &hintsTCP, &resTCP);
    if (nTCP != 0) exit(1);

    fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol);
    if (fdTCP == -1) exit(1);

    nTCP = bind(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
    if (nTCP == -1) exit(1);

    if (listen(fdTCP, 5) == -1) exit(1);

    printf("Created TCP Server\n");
    
    /*Wait for communication*/
    int maxFd;
    fd_set readset;

    FD_ZERO(&readset);
    maxFd = fdUDP > fdTCP ? fdUDP : fdTCP;

    printf("\n");
    while (1) {
        int result;
        ssize_t nMsg;

        /*Setup fd in readset*/
        FD_SET(fdUDP, &readset);
        FD_SET(fdTCP, &readset);
        
        /*Select the result selector*/
        result = select(maxFd+1, &readset, NULL, NULL, NULL);
        if (result == -1) continue;
        else {
            if (FD_ISSET(fdUDP, &readset)){
                printf("\nUDP\n");
                int addrlen = sizeof(addrUDP);

                nMsg = recvfrom(fdUDP, buffer, 128, 0, (struct sockaddr*) &addrUDP, &addrlen);
                if (nMsg == -1) /*error*/ exit(1);

                write(1, "received: ", 10); write(1, buffer, nMsg);

                nMsg = sendto(fdUDP, buffer, nMsg, 0, (struct sockaddr*) &addrUDP, addrlen);
                if (nMsg == -1) /*error*/ exit(1);

            }
            else if (FD_ISSET(fdTCP, &readset)){
                printf("\nTCP\n");

                if ((newfd = accept(fdTCP, (struct sockaddr*) &addrTCP, &addrlenTCP)) == -1) exit(1);

                nMsg = read(newfd, buffer, 128);
                if (nMsg == -1) exit(1);
                write(1, "received: ", 10); write(1, buffer, nMsg);

                nMsg = write(newfd, buffer, nMsg);
                if (nMsg == -1) exit(1);
            }
        }
    }

    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);
}