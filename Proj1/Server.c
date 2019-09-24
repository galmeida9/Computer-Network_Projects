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
#define BUFFER_SIZE 128
#define ID_SIZE 5
#define TOPIC_LIST "topics/List_of_Topics.txt"

int nUDP, nTCP, fdUDP, fdTCP, newfd;
socklen_t addrlenUDP, addrlenTCP;
struct addrinfo hintsUDP, hintsTCP, *resUDP, *resTCP;
struct sockaddr_in addrUDP, addrTCP;
char buffer[BUFFER_SIZE];

char* processUDPMessage(char* buffer, int len);
int checkIfStudentCanRegister(int number);
char* registerNewStudent(char* arg1);
char* listOfTopics();
void handleKill(int sig);

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

    printf("Welcome to RC Forum\n");

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
                char *bufferUDP = malloc(sizeof(char) * BUFFER_SIZE);

                nMsg = recvfrom(fdUDP, bufferUDP, BUFFER_SIZE, 0, (struct sockaddr*) &addrUDP, &addrlen);
                if (nMsg == -1) /*error*/ exit(1);

                /*Analyze message*/
                char *response = processUDPMessage(strtok(bufferUDP, "\n"), BUFFER_SIZE);
                //write(1, "received: ", 10); write(1, buffer, nMsg);

                /*Send response*/
                nMsg = sendto(fdUDP, response, strlen(response), 0, (struct sockaddr*) &addrUDP, addrlen);
                if (nMsg == -1) /*error*/ exit(1);

                free(response);
                free(bufferUDP);
            }
            else if (FD_ISSET(fdTCP, &readset)){
                printf("\nTCP\n");

                if ((newfd = accept(fdTCP, (struct sockaddr*) &addrTCP, &addrlenTCP)) == -1) exit(1);

                /*Analyze message*/
                nMsg = read(newfd, buffer, BUFFER_SIZE);
                if (nMsg == -1) exit(1);

                //write(1, "received: ", 10); write(1, buffer, nMsg);

                /*Send response*/
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

char* processUDPMessage(char* buffer, int len){
    char *command, *response;
    size_t size;

    command = strtok(buffer, " ");

    if (strcmp(command, "REG") == 0) {
        command = strtok(NULL, " ");
        response = registerNewStudent(command);
        return response;
    }

    else if (strcmp(command, "LTP") == 0) {
        printf("Entrei\n");
        response = listOfTopics();
        return response;
    }

    else {
        printf("Command not found.\n");
        return NULL;
    }
}

int checkIfStudentCanRegister(int number){
    FILE* fp = fopen("students.txt", "r");
    int currNumber = -1;
    char line[6] = "";
    while (fgets(line, sizeof(line), fp)){
        currNumber = atoi(line);
        if (currNumber == number) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

char* registerNewStudent(char* arg1){
    char *response;
    int stuNumber = atoi(arg1);

    if (stuNumber == 0) {
        printf("Number error.\n");
        response = strdup("RGR NOK\n");
        return response;
    }

    int enabled = checkIfStudentCanRegister(stuNumber);
    if (!enabled) {
        printf("Register %d: refused.\n", stuNumber);    
        response = strdup("NOK\n");
        return response;
    }

    printf("Register %d: accepted.\n", stuNumber);

    /*TODO: Register on file??*/

    response = strdup("RGR OK\n");
    return response;
}

char* listOfTopics() {
    int numberOfTopics = 0;
    char *response = malloc(sizeof(char) * BUFFER_SIZE);
    char *finalResponse = malloc(sizeof(char) * BUFFER_SIZE);
    char numberString[6];
    char *line;
    size_t len = 0;
    ssize_t nread;
    FILE *topicList;

    strcpy(response, " ");
    topicList = fopen(TOPIC_LIST, "r");
    if (topicList == NULL) exit(1);

    while ((nread = getline(&line, &len, topicList)) != -1) {
        char *token;
        char *id;
        numberOfTopics++;

        /*Get topic: in string*/
        token = strtok(line, ":");
        strcat(response, token);
        response[strlen(response)] = '\0';
        strcat(response, ":\0");

        /*Get user ID*/
        token = strtok(NULL, ":");
        id = strtok(token, "\n");
        id[ID_SIZE] = '\0';

        /*Put everything together*/
        strcat(response, id);
        response[strlen(response)] = '\0';
        strcat(response, " \0");
    }

    /*build final response*/
    strcpy(finalResponse, "LTR ");
    sprintf(numberString, "%d", numberOfTopics);
    numberString[strlen(numberString)] = '\0';
    strcat(finalResponse, numberString);
    strcat(finalResponse, response);
    finalResponse[strlen(finalResponse) - 1] = '\n';

    printf("%s", finalResponse);
    fclose(topicList);
    free(response);
    free(line);
    return finalResponse;
}

void handleKill(int sig){
    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);
    _Exit(EXIT_SUCCESS);
}