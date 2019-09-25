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
#define MAX_TOPICS 99

int nUDP, nTCP, fdUDP, fdTCP, newfd;
socklen_t addrlenUDP, addrlenTCP;
struct addrinfo hintsUDP, hintsTCP, *resUDP, *resTCP;
struct sockaddr_in addrUDP, addrTCP;

char buffer[BUFFER_SIZE];
int numberOfTopics = 0;
char **listWithTopics;

char* processUDPMessage(char* buffer, int len);
char* processTCPMessage(char* buffer, int len);
int checkIfStudentCanRegister(int number);
char* registerNewStudent(char* arg1);
char* listOfTopics();
char* topicPropose(char *input);
void updateListWithTopics();
int isTopicInList(char *topic);
void addToTopicList(char* topic, char *usedId);
void freeTopicInList();
char* questionGet(char *input);
char* questionGetReadFiles(char* path, char* question, int qUserId, int numberOfAnswers);
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
    listWithTopics = (char **) malloc(sizeof(char*) * MAX_TOPICS);
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
                printf("enviou: %s", response);
                //write(1, "received: ", 10); write(1, buffer, nMsg);

                /*Send response*/
                nMsg = sendto(fdUDP, response, strlen(response), 0, (struct sockaddr*) &addrUDP, addrlen);
                if (nMsg == -1) /*error*/ exit(1);
                free(response);
                free(bufferUDP);
            }
            else if (FD_ISSET(fdTCP, &readset)){
                printf("\nTCP\n");
                char *bufferTCP = malloc(sizeof(char) * BUFFER_SIZE);

                if ((newfd = accept(fdTCP, (struct sockaddr*) &addrTCP, &addrlenTCP)) == -1) exit(1);

                /*Analyze message*/
                nMsg = read(newfd, bufferTCP, BUFFER_SIZE);
                if (nMsg == -1) exit(1);

                char *response = processTCPMessage(strtok(bufferTCP, "\n"), BUFFER_SIZE);
                printf("enviou: %s", response);

                //write(1, "received: ", 10); write(1, buffer, nMsg);

                /*Send response*/
                nMsg = write(newfd, response, strlen(response));
                if (nMsg == -1) exit(1);

                free(response);
                free(bufferTCP);
            }
        }
    }

    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);
}

char* processUDPMessage(char* buffer, int len){
    char *command, *response, *bufferBackup;
    size_t size;

    bufferBackup = strdup(buffer);
    command = strtok(buffer, " ");
    
    if (strcmp(command, "REG") == 0) {
        command = strtok(NULL, " ");
        response = registerNewStudent(command);
        free(bufferBackup);
        return response;
    }

    else if (strcmp(command, "LTP") == 0) {
        response = listOfTopics();
        free(bufferBackup);
        return response;
    }

    else if (strcmp(command, "PTP") == 0) {
        response = topicPropose(bufferBackup);
        free(bufferBackup);
        return response;
    }

    else if (strcmp(command, "GQU") == 0) {
        response = questionGet(bufferBackup);
        free(bufferBackup);
        return response;
    }

    else {
        printf("Command not found.\n");
        free(bufferBackup);
        return NULL;
    }
}

char* processTCPMessage(char* buffer, int len){
    char *command, *response, *bufferBackup;
    size_t size;

    bufferBackup = strdup(buffer);
    command = strtok(buffer, " ");

    if (strcmp(command, "GQU") == 0) {
        response = questionGet(bufferBackup);
        free(bufferBackup);
        return response;
    }

    else {
        printf("Command not found.\n");
        free(bufferBackup);
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
        response = strdup("RGR NOK\n");
        return response;
    }

    printf("Register %d: accepted.\n", stuNumber);

    /*TODO: Register on file??*/

    response = strdup("RGR OK\n");
    return response;
}

char* listOfTopics() {
    char *response = malloc(sizeof(char) * BUFFER_SIZE);
    char *finalResponse = malloc(sizeof(char) * BUFFER_SIZE);
    char numberString[6];
    char *line;
    size_t len = 0;
    ssize_t nread;
    FILE *topicList;
    int addToList = 0;

    if (numberOfTopics == 0) addToList = 1;

    strcpy(response, " ");
    topicList = fopen(TOPIC_LIST, "r");
    if (topicList == NULL) exit(1);

    while ((nread = getline(&line, &len, topicList)) != -1) {
        char *token;
        char *id;

        /*Get topic: in string*/
        token = strtok(line, ":");
        strcat(response, token);
        if (addToList == 1) {
            listWithTopics[numberOfTopics] = strdup(token);
            numberOfTopics++;
        }
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

    fclose(topicList);
    free(response);
    free(line);
    return finalResponse;
}

char* topicPropose(char *input) {
    strtok(input, " ");
    char *response;
    char *id = strtok(NULL, " ");
    char *topic = strtok(NULL, " ");

    if (numberOfTopics == 0) updateListWithTopics();
    
    if (numberOfTopics == 99) response = strdup("PTR FUL\n");
    else if (strlen(topic) > 10) response = strdup("PTR NOK\n");
    else if (isTopicInList(topic)) response = strdup("PTR DUP\n");
    else {
        addToTopicList(topic, id);
        response = strdup("PTR OK\n");
    }

    return response;
}

void updateListWithTopics() {
    char *line;
    size_t len = 0;
    ssize_t nread;
    FILE *topicList;

    topicList = fopen(TOPIC_LIST, "r");
    if (topicList == NULL) exit(1);

    while ((nread = getline(&line, &len, topicList)) != -1) {
        char *token;

        token = strtok(line, ":");
        listWithTopics[numberOfTopics] = strdup(token);
        printf("-> %s\n", listWithTopics[numberOfTopics]);
        numberOfTopics++;
    }

    fclose(topicList);
    free(line);
}

int isTopicInList(char *topic) {
    if (numberOfTopics == 0) updateListWithTopics();

    for (int i = 0; i < numberOfTopics; i++) {
        if (strcmp(listWithTopics[i], topic) == 0) return 1;
    }

    return 0;
}

void addToTopicList(char *topic, char *usedId) {
    FILE *topicList;
    
    listWithTopics[numberOfTopics] = strdup(topic);
    numberOfTopics++;

    topicList = fopen(TOPIC_LIST, "a");
    if (topicList == NULL) exit(1);

    fprintf(topicList, "%s:%s\n", topic, usedId);
    fclose(topicList);

}

void freeTopicInList() {
    for (int i = 0; i < numberOfTopics; i++) {
        free(listWithTopics[i]);
    }
}

char* questionGet(char *input) {
    strtok(input, " ");

    char *response;
    char *topic = strtok(NULL, " ");
    char *question = strtok(NULL, " ");
    char *leftover = strtok(NULL, " ");

    if (topic == NULL) {
        response = strdup("QGR ERR\n");
        return response;
    }
    else if (!isTopicInList(topic)) {
        response = strdup("QGR EOF\n");
        return response;
    }

    if (question == NULL) {
        response = strdup("QGR ERR\n");
        return response;
    }

    if (leftover != NULL) {
        response = strdup("QGR ERR\n");
        return response;
    }

    char *path = malloc(sizeof(char) * BUFFER_SIZE);

    strcpy(path, "topics/");
    strcat(path, topic);
    path[strlen(path)] = '\0';
    char *topicFolderPath = strdup(path);
    strcat(path, "/_questions.txt");

    int foundQuestion = 0;
    int qUserId;
    int numberOfAnswers;
    char *line;
    size_t len = 0;
    ssize_t nread;
    FILE *questionsFd;
   
    questionsFd = fopen(path, "r");
    if (questionsFd == NULL) exit(1);

    while ((nread = getline(&line, &len, questionsFd)) != -1) {
        char *token = strtok(line, ":");
        if (strcmp(token, question) == 0) {
            foundQuestion = 1;
            qUserId = atoi(strtok(NULL, ":"));
            numberOfAnswers = atoi(strtok(NULL, ":"));
            break;
        }
    }

    if (!foundQuestion) {
        response = strdup("QGR EOF\n");
        return response;
    }

    response = questionGetReadFiles(topicFolderPath, question, qUserId, numberOfAnswers);
    return response;
}

char* questionGetReadFiles(char* path, char* question, int qUserId, int numberOfAnswers) {
    char *questionPath = malloc(sizeof(char) * BUFFER_SIZE);
    strcpy(questionPath, path);
    questionPath[strlen(questionPath)] = '/';
    questionPath[strlen(questionPath)] = '\0';
    strcat(questionPath, question);
    questionPath[strlen(questionPath)] = '\0';
    strcat(questionPath, ".txt");

    size_t len = 0;
    ssize_t nread;
    FILE *questionFd;
    questionFd = fopen(questionPath, "r");
    if (questionFd == NULL) exit(1);

    char *line;
    char *qdata;
    if ((nread = getline(&line, &len, questionFd)) != -1) {
        getline(&line, &len, questionFd);
        qdata = strtok(line, "\n");
    }

    fseek(questionFd, 0L, SEEK_END);
    long qsize = ftell(questionFd);

    fclose(questionFd);
    char *response = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(response, BUFFER_SIZE, "QGR %d %ld %s 0 %d\n", qUserId, qsize, qdata, numberOfAnswers);
    printf("response: %s\n", response);
    return response;
}

void handleKill(int sig){
    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);
    freeTopicInList();
    free(listWithTopics);
    _Exit(EXIT_SUCCESS);
}