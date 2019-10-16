
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "lib/utility.h"

#define AN_SIZE 3
#define BUFFER_SIZE 2048
#define ID_SIZE 5
#define DISPLAY_ANSWERS 10
#define MAX_ANSWERS 99
#define MAX_TOPICS 99
#define PORT "58013"
#define QUESTIONS_DESC "_desc"
#define QUESTIONS_LIST "/_questions.txt"
#define TOPIC_FOLDER "topics/"
#define TOPIC_LIST "topics/List_of_Topics.txt"
#define TOPICNAME_SIZE 10
#define DEBUG_TEST 0

#define DEBUG_PRINT(fmt, args...) \
    do { if (DEBUG_TEST) fprintf(stderr, fmt, ##args); } while (0)

int nUDP, nTCP, fdUDP, fdTCP, newfd, numberOfTopics = 0;
char buffer[BUFFER_SIZE], **listWithTopics;
socklen_t addrlenUDP, addrlenTCP;
struct addrinfo hintsUDP, hintsTCP, *resUDP, *resTCP;
struct sockaddr_in addrUDP, addrTCP;

void waitRequest();
void handleKill();
char* processUDPMessage(char *buffer);
char* processTCPMessage(char *buffer, int len, int fd);
int checkIfStudentCanRegister(int number);
char* registerNewStudent(char *arg1);
char* listOfTopics();
char* topicPropose(char *input);
void updateListWithTopics();
int isTopicInList(char *topic);
void addToTopicList(char *topic, char *usedId);
void freeTopicInList();
char* questionGet(char *input, int fd);
void questionGetReadFiles(char *path, char *question, int qUserId, int numberOfAnswers, int qIMG, char *qixt, int fd);
void getAnswerInformation(char *path, char *question, char *numb, int fd);
char* listOfQuestions(char *topic);
char* questionSubmit(char *input, int fd, int nMsg);
char* submitAnswer(char* input, int fd, int nMsg);

int main(int argc, char **argv) {
    int opt;
    char port[6];
    struct sigaction handle_kill;
    struct timeval tv = { 5, 0 };
    sigset_t int_set;
    sigemptyset(&int_set);
    sigaddset(&int_set, SIGINT);
    handle_kill.sa_handler = handleKill;
    handle_kill.sa_flags = SA_RESTART;
    sigemptyset(&handle_kill.sa_mask);
    sigaction(SIGINT, &handle_kill, NULL);

    strcpy(port, PORT);
    printf("Welcome to RC Forum!\n");

    /*Get port from arguments*/
    if (argc == 2) {
        printf("The port is missing.\n");
        exit(EXIT_FAILURE);
    }

    while((opt = getopt(argc, argv, "p:")) != -1) {
        switch(opt) {
            case 'p':
                strcpy(port, optarg);
                break;
        }
    }

    printf("Port: %s\n", port);

    /* UDP Server */
    memset(&hintsUDP,0, sizeof(hintsUDP));
    hintsUDP.ai_family=AF_INET;
    hintsUDP.ai_socktype=SOCK_DGRAM;
    hintsUDP.ai_flags=AI_PASSIVE|AI_NUMERICSERV;

    nUDP = getaddrinfo(NULL, port, &hintsUDP, &resUDP);
    if (nUDP != 0) /*error*/ exit(EXIT_FAILURE);

    fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol);
    if (fdUDP == -1) /*error*/ exit(EXIT_FAILURE);

    nUDP = bind(fdUDP, resUDP->ai_addr, resUDP->ai_addrlen);
    if (nUDP == -1) /*error*/ exit(EXIT_FAILURE);

    printf("Created UDP Server\n");

    /* TCP Server */
    memset(&hintsTCP, 0, sizeof(hintsTCP));
    hintsTCP.ai_family = AF_INET;
    hintsTCP.ai_socktype = SOCK_STREAM;
    hintsTCP.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    nTCP = getaddrinfo(NULL, port, &hintsTCP, &resTCP);
    if (nTCP != 0) exit(EXIT_FAILURE);

    fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol);
    if (fdTCP == -1) exit(EXIT_FAILURE);

    /* Setting TCP socket timeout value */
    if (setsockopt(fdTCP, SOL_SOCKET, SO_RCVTIMEO,
        (struct timeval *)&tv, sizeof(struct timeval))) {
        printf("Failed to set a timeout value for the TCP socket\n");
        close(fdTCP);
        exit(EXIT_FAILURE);
    }

    // Setting TCP socket timeout value
    if(setsockopt(fdTCP, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval))) {
        printf("setsockopt TCP failed\n");
        close(fdTCP);
        exit(2);
    }

    nTCP = bind(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
    if (nTCP == -1) exit(EXIT_FAILURE);

    if (listen(fdTCP, 5) == -1) exit(EXIT_FAILURE);

    printf("Created TCP Server\n");

    /* Sockets are now open, waiting for clients to send requests */
    waitRequest();
}

void waitRequest() {
    int maxFd;
    fd_set readset;

    FD_ZERO(&readset);
    maxFd = fdUDP > fdTCP ? fdUDP : fdTCP;

    listWithTopics = (char**) malloc(sizeof(char*) * MAX_TOPICS);
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
                printf("\n[UDP] ");
                socklen_t addrlen = sizeof(addrUDP);
                char *bufferUDP = malloc(sizeof(char) * BUFFER_SIZE);

                nMsg = recvfrom(fdUDP, bufferUDP, BUFFER_SIZE, 0, (struct sockaddr*) &addrUDP, &addrlen);
                if (nMsg == -1) /*error*/ exit(EXIT_FAILURE);

                /*Analyze message*/
                char *response = processUDPMessage(strtok(bufferUDP, "\n"));

                /*Send response*/
                nMsg = sendto(fdUDP, response, strlen(response), 0, (struct sockaddr*) &addrUDP, addrlen);
                if (nMsg == -1) /*error*/ exit(EXIT_FAILURE);
                free(response);
                free(bufferUDP);
            }
            else if (FD_ISSET(fdTCP, &readset)){
                printf("\n[TCP] ");
                char *bufferTCP = malloc(sizeof(char) * BUFFER_SIZE);

                if ((newfd = accept(fdTCP, (struct sockaddr*) &addrTCP, &addrlenTCP)) == -1) exit(EXIT_FAILURE);

                nMsg = read(newfd, bufferTCP, BUFFER_SIZE);
                if (nMsg == -1) exit(EXIT_FAILURE);

                /*Analyze message*/
                char *response = processTCPMessage(bufferTCP, nMsg, newfd);

                /*Send response*/
                if (response != NULL) {
                    nMsg = write(newfd, response, strlen(response));
                    if (nMsg == -1) exit(EXIT_FAILURE);
                }

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

void handleKill() {
    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);
    freeTopicInList();
    free(listWithTopics);
    _Exit(EXIT_SUCCESS);
}

/**
UDP message handling
- parameter buffer: buffer containing the request
*/
char* processUDPMessage(char *buffer) {
    char *command, *response, *bufferBackup;

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
        printf("Sent list of topics.\n");
        return response;
    }

    else if (strcmp(command, "PTP") == 0) {
        response = topicPropose(bufferBackup);
        free(bufferBackup);
        return response;
    }

    else if (strcmp(command, "LQU") == 0) {
        if (!(command = strtok(NULL, " "))) {
            free(bufferBackup);
            return strdup("ERR\n");
        }

        response = listOfQuestions(command);
        free(bufferBackup);
        return response;
    }

    else {
        printf("Command not found.\n");
        free(bufferBackup);
        response = strdup("ERR\n");
        return response;
    }
}

/**
TCP message handling
- parameter buffer: buffer containing the request
- parameter len: buffer length
- parameter fd:
*/
char* processTCPMessage(char *buffer, int len, int fd) {
    char *command, *response, *bufferBackup;

    bufferBackup = (char*) malloc(sizeof(char) * (BUFFER_SIZE + 1));
    memcpy(bufferBackup, buffer, len);
    bufferBackup[len] = '\0';

    command = strtok(buffer, " ");

    if (!strcmp(command, "GQU"))
        response = questionGet(bufferBackup, fd);

    else if (!strcmp(command, "QUS"))
        response = questionSubmit(bufferBackup, fd, len);

    else if (!strcmp(command, "ANS"))
        response = submitAnswer(bufferBackup, fd, len);

    else {
        printf("Command not found.\n");
        response = strdup("ERR\n");
    }

    free(bufferBackup);
    return response;
}

/**
Check if student is allowed to register
- parameter number: number to verify 
*/
int checkIfStudentCanRegister(int number) {
    int found = 0 ,currNumber = -1;
    char line[6] = "";
    FILE *fp;

    if (!(fp = fopen("students.txt", "r"))) return 0; // -1 ?
    while (fgets(line, sizeof(line), fp)) {
        currNumber = atoi(line);
        if (currNumber == number) {
            found = 1; break;
        }
    }

    fclose(fp);
    return found;
}

char* registerNewStudent(char *arg1) {
    int stuNumber = atoi(arg1);

    if (stuNumber == 0) {
        printf("Number error.\n");
        return strdup("RGR NOK\n");
    }
    else if (!checkIfStudentCanRegister(stuNumber)) {
        printf("Register %d: refused.\n", stuNumber);
        return strdup("RGR NOK\n");
    }

    printf("Register %d: accepted.\n", stuNumber);
    return strdup("RGR OK\n");
}

char* listOfTopics() {
    int addToList = 0;
    char *response = malloc(sizeof(char) * BUFFER_SIZE);
    char *finalResponse = malloc(sizeof(char) * BUFFER_SIZE);
    char numberString[6], *line = NULL, *token, *id;
    size_t len = 0;
    ssize_t nread;
    FILE *topicList;

    if (numberOfTopics == 0) addToList = 1;

    strcpy(response, " ");
    if (!(topicList = fopen(TOPIC_LIST, "r"))) exit(EXIT_FAILURE);

    while ((nread = getline(&line, &len, topicList)) != -1) {
        /* Get topic */
        token = strtok(line, ":");
        strcat(response, token);
        if (addToList == 1) {
            listWithTopics[numberOfTopics] = strdup(token);
            numberOfTopics++;
        }
        response[strlen(response)] = '\0';
        strcat(response, ":\0");

        /* Get user ID */
        token = strtok(NULL, ":");
        id = strtok(token, "\n");
        id[ID_SIZE] = '\0';

        /* Put everything together */
        strcat(response, id);
        response[strlen(response)] = '\0';
        strcat(response, " \0");
    }

    /* Build final response */
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

// TODO check response
char* topicPropose(char *input) {
    int pathLen;
    char *id, *topic, *directory, *questionPath;
    FILE *topicFd;

    strtok(input, " "); id = strtok(NULL, " "); topic = strtok(NULL, " ");
    if (numberOfTopics == 0) updateListWithTopics();

    /* Check if requirements are met */
    if (numberOfTopics == MAX_TOPICS) return strdup("PTR FUL\n");
    else if (strlen(topic) > TOPICNAME_SIZE) return strdup("PTR NOK\n");
    else if (isTopicInList(topic)) return strdup("PTR DUP\n");

    addToTopicList(topic, id);

    /* Create folder for the new topic */
    pathLen = strlen(TOPIC_FOLDER) + strlen(topic) + 1;
    directory = malloc(sizeof(char) * pathLen);
    snprintf(directory, pathLen, "%s%s", TOPIC_FOLDER, topic);

    struct stat st = {0};
    if (stat(directory, &st) == -1)
        mkdir(directory, 0700);
    free(directory);

    pathLen = strlen(TOPIC_FOLDER) + strlen(topic) + strlen(QUESTIONS_LIST) + 1;
    questionPath = malloc(sizeof(char) * pathLen);
    snprintf(questionPath, pathLen, "%s%s%s", TOPIC_FOLDER, topic, QUESTIONS_LIST);

    if (!(topicFd = fopen(questionPath, "w"))) printf("Failed to create file for the new topic.\n");
    fclose(topicFd);
    free(questionPath);

    return strdup("PTR OK\n");
}

void updateListWithTopics() {
    char *token, *line = NULL;
    size_t len = 0;
    ssize_t nread;
    FILE *topicList;

    if (!(topicList = fopen(TOPIC_LIST, "r"))) exit(EXIT_FAILURE);

    while ((nread = getline(&line, &len, topicList)) != -1) {
        token = strtok(line, ":");
        listWithTopics[numberOfTopics] = strdup(token);
        numberOfTopics++;
    }

    fclose(topicList);
    free(line);
}

int isTopicInList(char *topic) {
    if (numberOfTopics == 0) updateListWithTopics();

    for (int i = 0; i < numberOfTopics; i++)
        if (strcmp(listWithTopics[i], topic) == 0) return 1;
    return 0;
}

void addToTopicList(char *topic, char *usedId) {
    FILE *topicList;

    listWithTopics[numberOfTopics++] = strdup(topic);
    if (!(topicList = fopen(TOPIC_LIST, "a"))) exit(EXIT_FAILURE);

    fprintf(topicList, "%s:%s\n", topic, usedId);
    fclose(topicList);
}

void freeTopicInList() {
    for (int i = 0; i < numberOfTopics; i++) free(listWithTopics[i]);
}

char* questionSubmit(char *input, int fd, int nMsg) {
	int pathLen, qUserId, found, NQ = 0;
	int qsize = 0, offset = 0, qIMG = 0, isize = 0;
	char *topic, *question, *line = NULL, *response, *questionAux, *path, *iext;
	size_t len;
    FILE * questionFd;

    topic = (char*) malloc(sizeof(char)* (TOPICNAME_SIZE+1));
    question = (char*) malloc(sizeof(char)* (TOPICNAME_SIZE+1));

    sscanf(input, "%*s %d %s %s %d", &qUserId, topic, question, &qsize);
    if (qUserId == 0) return strdup("QUR NOK\n");
    offset += 3 + 1 + lengthInt(qUserId) + 1 + strlen(topic) + 1 + strlen(question) + 1 + lengthInt(qsize) + 1;

	/* Check if topic exists */
    if (!(found = isTopicInList(topic))) { return strdup("QUR NOK\n"); }

    /* Check if question already exists */
    pathLen = strlen(TOPIC_FOLDER) + strlen(topic) + strlen(QUESTIONS_LIST)+1;
    path = malloc(sizeof(char) * pathLen);
    snprintf(path, pathLen, "%s%s%s", TOPIC_FOLDER, topic, QUESTIONS_LIST);

    if (!(questionFd = fopen(path, "a+"))) {
        free(path);
        free(topic);
        free(question);
        return strdup("QUR NOK\n");
    }

    found = 0;
    rewind(questionFd);
    while (getline(&line, &len, questionFd) != -1) {
        /* Text file format:  question:qUserID:NA */
        questionAux = strtok(line, ":");
        if (!strcmp(question, questionAux)) { found = 1; break; }
        NQ++;
    }
    free(line);
    free(path);

    /* Check if the question list is full */
    if (NQ >= 99 || found) {
        if (found) {
            printf("Question is a duplicate.\n");
            response = strdup("QUR DUP\n");
        }
        else {
            printf("Question list of %s is full.\n", topic);
            response = strdup("QUR FUL\n");
        }
        free(topic);
        free(question);
        fclose(questionFd);
        return response;
    }

    /* Prepare question file path (example: question.txt\0). */
    pathLen = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + 3 + 1;
    path = (char*) malloc(sizeof(char) * pathLen);
    snprintf(path, pathLen, "%s%s/%s.txt", TOPIC_FOLDER, topic, question);

    /* Receive and write question file data */
    if (recvTCPWriteFile(fd, path, &input, &nMsg, BUFFER_SIZE, &offset, qsize, DEBUG_TEST) == -1)
        printf("Erro a receber e escrever o ficheiro\n");
    if (offset == 0) offset++;
    free(path);

    /* Receive image info */
    sscanf(input + offset, " %d", &qIMG);
    offset += 2;

    if (qIMG) {
        iext = (char*) malloc(sizeof(char)*4); iext[0] = '\0';
        sscanf(input + offset, "%s %d", iext , &isize);
        fprintf(questionFd, "%s:%d:00:1:%s:\n", question, qUserId, iext);

        offset += strlen(iext) + 1 + lengthInt(isize) + 1;

        /* Prepare question image path (example: question.png\0). */
        pathLen = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + strlen(iext) + 1;
        path = (char*) malloc(sizeof(char) * pathLen);
        snprintf(path, pathLen, "%s%s/%s.%s", TOPIC_FOLDER, topic, question, iext);

        /* Receive and write image file */
        if (recvTCPWriteFile(fd, path, &input, &nMsg, BUFFER_SIZE, &offset, isize, DEBUG_TEST) == -1)
            printf("Erro a receber e escrever o ficheiro\n");
        if (offset == 0) offset++;
        free(path);
        free(iext);
    }
    else
    	fprintf(questionFd, "%s:%d:00:0:\n", question, qUserId);
    printf("Question submit on \"%s\" (%d).\n", topic, qUserId);

    free(topic);
    free(question);
    fclose(questionFd);
	return strdup("QUR OK\n");
}

char* questionGet(char *input, int fd) {
    int qUserId, numberOfAnswers, qIMG = 0, foundQuestion = 0;
    char *topic, *question, *path, *topicFolderPath, *qiext = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    FILE *questionsFd;

    strtok(input, " "); topic = strtok(NULL, " "); question = strtok(NULL, "\n");

    /* Check if requirements are met */
    if (topic == NULL) return strdup("QGR ERR\n");
    else if (!isTopicInList(topic)) return strdup("QGR EOF\n");
    else if (question == NULL) return strdup("QGR ERR\n");

    path = (char *) malloc(sizeof(char) * BUFFER_SIZE);

    strcpy(path, TOPIC_FOLDER);
    strcat(path, topic);
    path[strlen(path)] = '\0';
    topicFolderPath = strdup(path);
    strcat(path, QUESTIONS_LIST);

    if (!(questionsFd = fopen(path, "r"))) exit(EXIT_FAILURE);

    while ((nread = getline(&line, &len, questionsFd)) != -1) {
        if (!strcmp(strtok(line, ":"), question)) {
            foundQuestion = 1;
            qUserId = atoi(strtok(NULL, ":"));
            numberOfAnswers = atoi(strtok(NULL, ":"));
            if (!strcmp(strtok(NULL, ":"), "1")) {
                qiext = strtok(NULL, ":");
                qIMG = 1;
            }
            break;
        }
    }

    fclose(questionsFd);
    free(path);

    if (!foundQuestion) {
        free(topicFolderPath);
        free(line);
        return strdup("QGR EOF\n");
    }

    questionGetReadFiles(topicFolderPath, question, qUserId, numberOfAnswers, qIMG, qiext, fd);
    free(topicFolderPath);
    free(line);
    printf("Sent stored files for question \"%s\".\n", question);
    return NULL;
}

void questionGetReadFiles(char* path, char* question, int qUserId,
    int numberOfAnswers, int qIMG, char *qiext, int fd) {
    int sizeAux, nRead;
    char *questionPath, *response, *qdata;
    FILE *questionFd;

    /* Path for the requested question */
    questionPath = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(questionPath, BUFFER_SIZE, "%s/%s.txt", path, question);

    /*Get the question in the file, check if there is a image and what is its extention*/
    if (!(questionFd = fopen(questionPath, "r"))) exit(EXIT_FAILURE);
    fseek(questionFd, 0L, SEEK_END);
    long qsize = ftell(questionFd);
    fseek(questionFd, 0L, SEEK_SET);

    response = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(response, BUFFER_SIZE, "QGR %d %ld ", qUserId, qsize);
    write(fd, response, strlen(response));

    qdata = (char*) malloc(sizeof(char)*BUFFER_SIZE);
    sizeAux = qsize;
    while (sizeAux > 0 ) {
        nRead = fread(qdata, 1 , BUFFER_SIZE, questionFd);
        write(fd, qdata, nRead);
        sizeAux -= BUFFER_SIZE;
    }
    fclose(questionFd);
    free(questionPath);

    /* If there is a image get its size and data */
    long qisize;
    char *qidata, *imgPath, *questionNumber;
    FILE *imageFd;

    if (qIMG) {
        imgPath = malloc(sizeof(char) * BUFFER_SIZE);

        snprintf(imgPath, BUFFER_SIZE, "%s/%s.%s", path, question, qiext);
        if (!(imageFd = fopen(imgPath, "r"))) exit(EXIT_FAILURE);
        fseek(imageFd, 0L, SEEK_END);
        qisize  = ftell(imageFd);
        fseek(imageFd, 0L, SEEK_SET);

        snprintf(response, BUFFER_SIZE, " 1 %s %ld ", qiext, qisize);
        write(fd, response, strlen(response));

        sizeAux = qisize;
        qidata = malloc(sizeof(char) * BUFFER_SIZE);
        while (sizeAux > 0) {
            nRead = fread(qidata, 1 , BUFFER_SIZE, imageFd);
            write(fd, qidata, nRead);
            sizeAux = sizeAux - BUFFER_SIZE;
        }

        free(qidata);
        fclose(imageFd);
        free(imgPath);
        snprintf(response, BUFFER_SIZE, " %d", numberOfAnswers);
        write(fd, response, strlen(response));
    }

    else {
        snprintf(response, BUFFER_SIZE, "QGR %d %ld %s 0 %d", qUserId, qsize, qdata, numberOfAnswers);
        write(fd, response, strlen(response));
    }
    free(qdata);

    /* Get the answers information */
    for (int i = 1; (i <= numberOfAnswers) && (i <= DISPLAY_ANSWERS); i++) {
        questionNumber = malloc(sizeof(char) * AN_SIZE);
        i < 10 ? snprintf(questionNumber, AN_SIZE, "0%d", i) : snprintf(question, AN_SIZE, "%d", i);
        getAnswerInformation(path, question, questionNumber, fd);
        free(questionNumber);
    }

    write(fd, "\n", strlen("\n"));
    free(response);
    return;
}

void getAnswerInformation(char *path, char *question, char *numb, int fd) {
    int sizeAux, aUserID, aIMG = 0;
    char *token, *answerDesc, *aiext  = NULL, *line = NULL;
    size_t len = 0;
    ssize_t nread;
    FILE *answerDescFd, *answerFd, *imageFd;

    /* Get information about the answer */
    answerDesc = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(answerDesc, BUFFER_SIZE, "%s/%s_%s%s.txt", path, question, numb, QUESTIONS_DESC);

    if (!(answerDescFd = fopen(answerDesc, "r"))) exit(EXIT_FAILURE);

    while ((nread = getline(&line, &len, answerDescFd)) != -1) {
        token = strtok(line, ":");
        aUserID  = atoi(token);
        if (strcmp(strtok(NULL, ":"), "1") == 0) {
            aiext  = strtok(NULL, ":");
            aIMG  = 1;
        }
        break;
    }

    fclose(answerDescFd);
    free(answerDesc);

    /* Get the answer in the file and its size */
    int nRead;
    long asize;
    char *answerPath, * response, *adata;

    len = 0;
    answerPath = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(answerPath, BUFFER_SIZE, "%s/%s_%s.txt", path, question, numb);

    if (!(answerFd = fopen(answerPath, "r"))) exit(EXIT_FAILURE);

    /* Get answer size */
    fseek(answerFd, 0L, SEEK_END);
    asize = ftell(answerFd);
    fseek(answerFd, 0L, SEEK_SET);

    response = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(response, BUFFER_SIZE, " %s %d %ld ", numb, aUserID, asize);
    write(fd, response, strlen(response));

    adata = (char*) malloc(sizeof(char)*BUFFER_SIZE);
    sizeAux = asize;
    while (sizeAux > 0 ) {
        nRead = fread(adata, 1 , BUFFER_SIZE, answerFd);
        write(fd, adata, nRead);
        sizeAux -= BUFFER_SIZE;
    }

    fclose(answerFd);
    free(answerPath);

    /* Get the answer's image information */
    long aisize;
    char *aidata, *imgPath;

    if (aIMG) {
        imgPath = malloc(sizeof(char) * BUFFER_SIZE);
        snprintf(imgPath, BUFFER_SIZE, "%s/%s_%s.%s", path, question, numb, aiext);
        if (!(imageFd = fopen(imgPath, "r"))) exit(EXIT_FAILURE);

        /* Get image size */
        fseek(imageFd, 0L, SEEK_END);
        aisize = ftell(imageFd);
        fseek(imageFd, 0L, SEEK_SET);

        snprintf(response, BUFFER_SIZE, " 1 %s %ld ", aiext, aisize);
        write(fd, response, strlen(response));

        /* Get image data */
        sizeAux = aisize;
        aidata = malloc(sizeof(char) * (BUFFER_SIZE));
        while (sizeAux > 0){
            nRead = fread(aidata, 1 , BUFFER_SIZE, imageFd);
            write(fd, aidata, nRead);
            sizeAux -= BUFFER_SIZE;
        }

        fclose(imageFd);
        free(imgPath);
        free(aidata);
    }
    else write(fd, " 0", 2);

    free(adata);
    free(line);
    free(response);
    return;
}

char* listOfQuestions(char *topic) {
    int N = 0;
    char path[33] = TOPIC_FOLDER;
    char * response, * line = NULL, * question, * userID, * NA;
    size_t len = 0;
    FILE *fp;

    strcat(path, topic);
    strcat(path, QUESTIONS_LIST);

    if (!(fp = fopen (path, "r"))) {
        printf ("There are no questions available.\n");
        return strdup("LQR 0\n");
    }

    response = (char *) malloc(sizeof(char) * BUFFER_SIZE);
    while (getline(&line, &len, fp) != -1) N++;
    sprintf(response, "LQR %d", N);

    rewind(fp);
    while (getline(&line, &len, fp) != -1) {
        question = strtok(line, ":"); userID = strtok(NULL, ":"); NA = strtok(NULL, ":");
        sprintf(response,"%s %s:%s:%s", response, question, userID, NA);
    }

    strcat(response,"\n");
    fclose(fp);
    free(line);
    printf("Sent list of questions.\n");
    return response;
}

char* submitAnswer(char* input, int fd, int nMsg) {
    int offset;
    char *topic, *question;
    int userId, asize, aIMG;
    char *iext = NULL;

    topic = (char*) malloc(sizeof(char)* (TOPICNAME_SIZE+1));
    question = (char*) malloc(sizeof(char)* (TOPICNAME_SIZE+1));

    /* Get arguments */
    sscanf(input, "%*s %d %s %s %d", &userId, topic, question, &asize);

    DEBUG_PRINT("[ANS] Parsed argument userID: \"%d\"\n", userId);
    DEBUG_PRINT("[ANS] Parsed argument topic: \"%s\"\n", topic);
    DEBUG_PRINT("[ANS] Parsed argument question: \"%s\"\n", question);
    DEBUG_PRINT("[ANS] Parsed argument asize: \"%d\"\n", asize);

    /* Check if topic exists */
    if (!isTopicInList(topic)) {
        free(topic); free(question);
        return strdup("ANR NOK\n");
    }

    /* Check if question exists */
    int lenQuestionPath, numOfAnswers = -1;
    long questionListOffset = 0;
    char *qUserCreated, *qImg, *qExt, *line = NULL, *numOfAnswersInput = NULL;
    char *questionPath, *questionAux, * response;
    size_t len = 0;
    ssize_t nread;
    FILE *questionListFP;

    lenQuestionPath = strlen(TOPIC_FOLDER) + strlen(topic) + strlen(QUESTIONS_LIST)+1;
    questionPath = (char*) malloc(sizeof(char) * lenQuestionPath);

    snprintf(questionPath, lenQuestionPath, "%s%s%s", TOPIC_FOLDER, topic, QUESTIONS_LIST);

    if (!(questionListFP = fopen(questionPath, "r+"))) {
        free(questionPath); free(topic); free(question);
        return strdup("ANR NOK\n");
    }

    /* Find question and get number of answers */
    while ((nread = getline(&line, &len, questionListFP)) != -1) {
        /* Text file format:  QUESTION:USERID:N_OF_ANS */
        questionAux = strtok(line,":");
        if (!strcmp(question, questionAux)) {
            qUserCreated = strtok(NULL, ":");
            numOfAnswersInput = strtok(NULL, ":");
            qImg = strtok(NULL, ":");
            qExt = strtok(NULL, ":");
            numOfAnswers = atoi(numOfAnswersInput);
            break;
        }
        questionListOffset = ftell(questionListFP);
    }
    /* File closed afterwards */

    /* Question not found or answer list is full*/
    if (numOfAnswers == -1 || numOfAnswers == MAX_ANSWERS) {
        numOfAnswers == -1 ? (response = strdup("ANR NOK\n")) : (response = strdup("ANR FUL\n"));
        fclose(questionListFP);
        free(questionPath);
        free(line);
        free(topic); free(question);
        return response;
    }
    numOfAnswers++;

    /* Prepare file pathname (example: question_56.txt\0). */
    int isize = 0, lenAnswerDescPath, lenAnswerPath, lenAnswerImgPath;
    char *answerPath, *answerDescPath, *answerImgPath;
    FILE *answerDescFP;

    lenAnswerPath = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + 2 + 4 + 1;
    answerPath = (char*) malloc(sizeof(char) * lenAnswerPath);
    snprintf(answerPath, lenAnswerPath, "%s%s/%s_%02d.txt", TOPIC_FOLDER, topic, question, numOfAnswers);

    /* Prepare offset */
    offset = 3 + 1 + lengthInt(userId) + 1 + strlen(topic) + 1;
    offset += strlen(question) + 1 + lengthInt(asize) + 1;

    /* Receive and write text file */
    DEBUG_PRINT("[ANS] Parsed argument answerPath: \"%s\"\n", answerPath);
    DEBUG_PRINT("[ANS] Parsed argument input: \"%s\"\n", input);
    DEBUG_PRINT("[ANS] Parsed argument offset: \"%d\"\n", offset);
    DEBUG_PRINT("[ANS] Parsed argument asize: \"%d\"\n", asize);
    DEBUG_PRINT("[ANS] Writing answer to %s (size = %d)\n", answerPath, asize);
    
    if (recvTCPWriteFile(fd, answerPath, &input, &nMsg, BUFFER_SIZE, &offset, asize, DEBUG_TEST) == -1)
        printf("erro\n");
    if (offset == 0) offset++;
    DEBUG_PRINT("[ANS] Finished writing file.\n\n");

    /* Prepare for image */
    sscanf(input+offset, "%d", &aIMG);
    offset += lengthInt(aIMG) + 1;

    DEBUG_PRINT("[ANS] Has Image? %d\n", aIMG);
    
    if (aIMG) {
        iext = (char*) malloc(sizeof(char)* (3+1)); iext[0] = '\0';
        sscanf(input+offset, "%s %d", iext, &isize);
        offset += strlen(iext) + 1 + lengthInt(isize) + 1;

        DEBUG_PRINT("[ANS] Image ext:%s\n", iext);
        DEBUG_PRINT("[ANS] Image size:%d\n", isize);
    }

    /* Write answer description (example: question_56_desc.txt\0). */
    lenAnswerDescPath = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + 2 + strlen(QUESTIONS_DESC) + 4 + 1;
    answerDescPath = (char*) malloc(sizeof(char)*lenAnswerDescPath);
    snprintf(answerDescPath, lenAnswerDescPath, "%s%s/%s_%02d%s.txt", TOPIC_FOLDER, topic, question, numOfAnswers, QUESTIONS_DESC);

    if (!(answerDescFP = fopen(answerDescPath, "w"))) {
        fclose(questionListFP);
        free(questionPath);
        free(answerDescPath);
        free(line);
        free(topic); free(question);
        return strdup("ANR NOK\n");
    }

    if (aIMG) fprintf(answerDescFP, "%d:1:%s:", userId, iext);
    else fprintf(answerDescFP, "%d:0:", userId);
    fclose(answerDescFP);
    free(answerDescPath);

    /* Check if there is an image */
    if (aIMG) {
        if (!iext || !isize) {
            fclose(questionListFP);
            free(questionPath);
            free(line);
            free(topic); free(question);
            free(iext);
            return strdup("ANR NOK\n");
        }

        /* Prepare image pathname (example: question_56.jpg\0). */
        lenAnswerImgPath = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + 2 + 1 + strlen(iext) + 1;
        answerImgPath = (char*) malloc(sizeof(char) * lenAnswerImgPath);
        snprintf(answerImgPath, lenAnswerImgPath, "%s%s/%s_%02d.%s", TOPIC_FOLDER, topic, question, numOfAnswers, iext);

        /* Receive and write image */
        if (recvTCPWriteFile(fd, answerImgPath, &input, &nMsg, BUFFER_SIZE, &offset, isize, DEBUG_TEST) == -1)
            printf("erro\n");
        if (offset == 0) offset++;

        free(answerImgPath);
        free(iext);
    }

    /* Update number of answers */
    fseek(questionListFP, questionListOffset, SEEK_SET);
    fprintf(questionListFP, "%s:%s:%02d:%s:%s:\n", question, qUserCreated, numOfAnswers, qImg, qExt);
    fclose(questionListFP);
    free(questionPath);
    free(line);

    /* Output to screen */
    printf("New answer (%s) received for %s/%s\n", answerPath, topic, question);
    free(topic); free(question);  free(answerPath);
    return strdup("ANR OK\n");
}
