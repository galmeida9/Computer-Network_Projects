#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>

#define  DEFAULT_PORT "58013"
#define BUFFER_SIZE 2048
#define ID_SIZE 5
#define REGISTER_SIZE 12
#define NUM_TOPICS 99

void parseArgs(int number, char** arguments, char **port, char **ip);
void connectToServer(int *udp_fd, int *tcp_fd, char *ip, char *port, struct addrinfo hints, struct addrinfo **resUDP, struct addrinfo **resTCP);
void SendMessageUDP(char *message, int fd, struct addrinfo *res);
char* receiveMessageUDP(int fd, socklen_t addrlen, struct sockaddr_in addr);
void SendMessageTCP(char *message, int fd, struct addrinfo *res);
char* receiveMessageTCP(int fd);
void parseCommands(int *userId, int udp_fd, int tcp_fd, struct addrinfo *resUDP, struct addrinfo *resTCP, socklen_t addrlen, struct sockaddr_in addr);
void registerNewUser(int id, int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr);
void requestLTP(int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr, char** topics, int* numTopics);
void freeTopics(int numTopics, char** topics);
char* topicSelectNum(int numTopics, char** topics, int topicChosen);
char* topicSelectName(int numTopics, char** topics, char* name);

char *buffer;
int debug = 0;

int main(int argc, char** argv) {
    int *udp_fd = malloc(sizeof(int));
    int *tcp_fd = malloc(sizeof(int));
    char* ip, *port;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *resUDP, *resTCP;
    struct sockaddr_in addr;
    buffer = malloc(sizeof(char) * BUFFER_SIZE);
    buffer[0] = '\0';

    port = DEFAULT_PORT;
    ip = "127.0.0.1";
    parseArgs(argc, argv, &port, &ip);
    printf("Connected to %s:%s\n", ip, port);

    *udp_fd = -1; *tcp_fd = -1;
    connectToServer(udp_fd, tcp_fd, ip, port, hints, &resUDP, &resTCP);

    int userId = -1;
    parseCommands(&userId, *udp_fd, *tcp_fd, resUDP, resTCP, addrlen, addr);

    free(buffer);
    freeaddrinfo(resTCP);
    freeaddrinfo(resUDP);
    close(*udp_fd);
    close(*tcp_fd);
    free(udp_fd);
    free(tcp_fd);
    return 0;
}

void parseArgs(int number, char** arguments, char **port, char **ip) {
    int opt; 
       
    while((opt = getopt(number, arguments, "n:p:d")) != -1) {  
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
            case 'd':
                debug = 1;
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
}

char* receiveMessageUDP(int fd, socklen_t addrlen, struct sockaddr_in addr) {
    ssize_t n;  
    addrlen = sizeof(addr);
    buffer[0] = '\0';

    while (strtok(buffer, "\n") == NULL) {
        n = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
        if (n == -1) exit(1);
    }

    if (debug == 1) printf("Received: |%s|\n", buffer);

    return buffer;
}

void SendMessageTCP(char *message, int fd, struct addrinfo *res) {
    ssize_t n;

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);

    n = write(fd, message, strlen(message));
    if (n == -1) exit(1);
    printf("enviei\n");
}

char* receiveMessageTCP(int fd) {
    ssize_t n;  

    n = read(fd, buffer, BUFFER_SIZE);
    if (n == -1) exit(1);
    printf("%s\n", buffer);

    return buffer;
}

void parseCommands(int *userId, int udp_fd, int tcp_fd, struct addrinfo *resUDP, struct addrinfo *resTCP, socklen_t addrlen, struct sockaddr_in addr) {
    char *line = NULL, *command, **topics = malloc(sizeof(char*)*NUM_TOPICS), *topicChosen = NULL;
    size_t size = 0;
    int numTopics = -1;

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        getline(&line, &size, stdin);
        command = strtok(line, " ");

        if (strcmp(command, "register") == 0 || strcmp(command, "reg") == 0) {
            command = strtok(NULL, " ");
            if (command != NULL && strlen(command) == ID_SIZE + 1) {
                *userId = atoi(strtok(command, "\n"));
                strtok(NULL, " ") != NULL  || *userId == 0? printf("Invalid command.\n") : registerNewUser(*userId, udp_fd, resUDP, addrlen, addr);
            }
            else {
                printf("Invalid command.\n");
            }
        }

        else if ((strcmp(command, "topic_list\n") == 0 || strcmp(command, "tl\n") == 0) && *userId != -1)
            requestLTP(udp_fd, resUDP, addrlen, addr, topics, &numTopics);

        else if ((strcmp(command, "ts") == 0) && *userId != -1){
            int topicChosenNum;
            command = strtok(NULL, " ");
            if (command != NULL) {
                topicChosenNum = atoi(strtok(command, "\n"));
                if (strtok(NULL, " ") != NULL) printf("Invalid command.\n");
                else {
                    topicChosen = topicSelectNum(numTopics, topics, topicChosenNum);
                    if (topicChosen != NULL) printf("Topic chosen: %s\n", topicChosen);
                }
            }
        }

        else if ((strcmp(command, "topic_select") == 0) && *userId != -1){
            char *arg;
            command = strtok(NULL, " ");
            if (command != NULL){
                arg = strtok(command, "\n");
                if (strtok(NULL, " ") != NULL) printf("Invalid command.\n");
                else {
                    topicChosen = topicSelectName(numTopics, topics, arg);
                    if (topicChosen != NULL) printf("Topic chosen: %s\n", topicChosen);
                }
            }

        }

        else if (strcmp(command, "tp\n") == 0) {
            SendMessageUDP("PTP 12345 Minecraft\n", udp_fd, resUDP);
            receiveMessageUDP(udp_fd, addrlen, addr);
        }

        else if (strcmp(line, "exit\n") == 0) {
            freeTopics(numTopics, topics);
            free(line);
            return;
        }
        else *userId == -1 ? printf("You need to register first before performing any commands.\n") : printf("Invalid command.\n");
        
        printf("\n");
    }
}

void registerNewUser(int id, int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr) {
    char *message = malloc(sizeof(char) * REGISTER_SIZE);

    snprintf(message, REGISTER_SIZE, "REG %d\n", id);
    SendMessageUDP(message, fd, res);
    char* status = receiveMessageUDP(fd, addrlen, addr);
    strcmp(status, "RGR OK") ==  0 ? printf("Registration Complete!\n") : printf("Could not register user, invalid user ID.\n");
    free(message);
}

void requestLTP(int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr, char** topics, int* numTopics) {
    int i = 1, N;
    char * iter, * ltr;

    SendMessageUDP("LTP\n", fd, res);
    ltr = receiveMessageUDP(fd, addrlen, addr);

    assert(!strcmp(strtok(ltr, " "), "LTR"));
    N = atoi(strtok(NULL, " "));

    while (i <= N) {
        iter = strtok(NULL, " ");
        topics[i-1] = strdup(iter);
        printf("%d: %s\n", i++, topics[i-1]);
    }

    *numTopics = N;
}

void freeTopics(int numTopics, char** topics){
    int i;
    for (i=0; i<numTopics; i++) free(topics[i]);
    free(topics);
}

char* topicSelectNum(int numTopics, char** topics, int topicChosen){
    char* topic;
    
    if (numTopics == -1){
        printf("Run tl first.\n");
        return NULL;
    }

    if (topicChosen > numTopics){
        printf("Invalid topic number.\n");
        return NULL;
    }

    topic = strtok(topics[topicChosen-1], ":");
    return topic;
}

char* topicSelectName(int numTopics, char** topics, char* name){
    char* topic = NULL;
    int i;
    
    if (numTopics == -1){
        printf("Run tl first.\n");
        return NULL;
    }

    for (i=0; i<numTopics; i++){
        char *nextTopic = strtok(topics[i],":");
        if (!strcmp(nextTopic, name)){
            topic = nextTopic;
            break;
        }
    }

    if (topic == NULL) printf("Can't find that topic.\n");

    return topic;
}