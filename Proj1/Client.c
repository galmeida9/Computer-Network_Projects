#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define  DEFAULT_PORT "58013"
#define BUFFER_SIZE 128
#define ID_SIZE 5

void parseArgs(int number, char** arguments, char **port, char **ip);
void connectToServer(int *udp_fd, int *tcp_fd, char *ip, char *port, struct addrinfo hints, struct addrinfo **resUDP, struct addrinfo **resTCP);
void SendMessageUDP(char *message, int fd, struct addrinfo *res);
void receiveMessageUDP(int fd, socklen_t addrlen, struct sockaddr_in addr);
void SendMessageTCP(char *message, int fd, struct addrinfo *res);
void receiveMessageTCP(int fd);
void parseCommands(int *userId, int udp_fd, int tcp_fd, struct addrinfo *resUDP, struct addrinfo *resTCP);
void registerNewUser(int id, int fd, struct addrinfo *res);

int main(int argc, char** argv) {
    int *udp_fd = malloc(sizeof(int));
    int *tcp_fd = malloc(sizeof(int));
    char* ip, *port;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *resUDP, *resTCP;
    struct sockaddr_in addr;
    char buffer[128];

    port = DEFAULT_PORT;
    ip = "127.0.0.1";
    parseArgs(argc, argv, &port, &ip);
    printf("ip: %s\nport: %s\n", ip, port);

    *udp_fd = -1; *tcp_fd = -1;
    connectToServer(udp_fd, tcp_fd, ip, port, hints, &resUDP, &resTCP);

    char msg[] = "mensagem muita grande para ver se dá para enviar os tópicos todos ou não, para ver se está tudo fixe ou quê, um gajo tem de duvidar de tudo\n",
        msg2[] = "ABC 1",
        msg3[] = "REG 54321";
    /*SendMessageUDP(msg3, *udp_fd, resUDP);*/

    int userId;
    parseCommands(&userId, *udp_fd, *tcp_fd, resUDP, resTCP);

    return 0;
}

void parseArgs(int number, char** arguments, char **port, char **ip) {
    int opt; 
       
    while((opt = getopt(number, arguments, "n:p:")) != -1) {  
        switch(opt) {   
            case 'n':  
                if (optarg == NULL || strcmp(optarg, "-p") == 0) {
                    printf("The IP is missing.\n");
                    exit(1);
                }
                *ip = strdup(optarg);
                break;  
            case 'p':  
                if (optarg == NULL || strcmp(optarg, "-n") == 0) {
                    printf("The port is missing.\n");
                    exit(1);
                }
                *port = strdup(optarg);
                break;  
        }  
    }
}

void connectToServer(int *udp_fd, int *tcp_fd, char *ip, char *port, struct addrinfo hints, struct addrinfo **resUDP, struct addrinfo **resTCP) {
    ssize_t n;
    memset(&hints,0, sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_flags=AI_NUMERICSERV;

    /*UDP Connection */
    if (*udp_fd < 0) {
        hints.ai_socktype=SOCK_DGRAM;
        n = getaddrinfo(ip, port, &hints, resUDP);
        if (n != 0) exit(1);

        *udp_fd = socket((*resUDP)->ai_family, (*resUDP)->ai_socktype, (*resUDP)->ai_protocol);
        if (*udp_fd == -1) exit(1);
    }

    /*TCP Connection */
    if (*tcp_fd < 0){
        hints.ai_socktype=SOCK_STREAM;
        n = getaddrinfo(ip, port, &hints, resTCP);
        if (n != 0) exit(1);

        *tcp_fd = socket((*resTCP)->ai_family, (*resTCP)->ai_socktype, (*resTCP)->ai_protocol);
        if (*tcp_fd == -1) exit(1);
    }
}

void SendMessageUDP(char *message, int fd, struct addrinfo *res) {
    ssize_t n;

    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);
    printf("enviei\n");
}

void receiveMessageUDP(int fd, socklen_t addrlen, struct sockaddr_in addr) {
    ssize_t n;  
    char buffer[128];
    addrlen = sizeof(addr);

    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) exit(1);
    printf("%s\n", buffer);
}

void SendMessageTCP(char *message, int fd, struct addrinfo *res) {
    ssize_t n;

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);

    n = write(fd, message, strlen(message));
    if (n == -1) exit(1);
    printf("enviei\n");
}

void receiveMessageTCP(int fd) {
    ssize_t n;  
    char buffer[128];

    n = read(fd, buffer, 128);
    if (n == -1) exit(1);
    printf("%s\n", buffer);
}

void parseCommands(int *userId, int udp_fd, int tcp_fd, struct addrinfo *resUDP, struct addrinfo *resTCP) {
    char *line = malloc(sizeof(char) * BUFFER_SIZE), *command;
    size_t size;

    while(1) {
        getline(&line, &size, stdin);
        command = strtok(line, " ");

        if (strcmp(command, "register") == 0 || strcmp(command, "reg") == 0) {
            command = strtok(NULL, " ");
            if (command != NULL && strlen(command) == ID_SIZE + 1) {
                *userId = atoi(strtok(command, "\n"));
                strtok(NULL, " ") != NULL ? printf("Invalid command.\n") : registerNewUser(*userId, udp_fd, resUDP);
                
            }
            else {
                printf("Invalid command.\n");
            }
        }
        else if (strcmp(line, "exit\n") == 0) {
            return;
        }
    }
}

void registerNewUser(int id, int fd, struct addrinfo *res) {
    printf("New User: %d\n", id);
    SendMessageUDP("REG " + id, fd, res);
    receiveMessageTCP(fd);
}