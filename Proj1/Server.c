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
#define BUFFER_SIZE 1024
#define ID_SIZE 5
#define TOPICNAME_SIZE 10
#define TOPIC_LIST "topics/List_of_Topics.txt"
#define TOPIC_FOLDER "topics/"
#define QUESTIONS_LIST "/_questions.txt"
#define QUESTIONS_DESC "_desc"
#define MAX_TOPICS 99
#define MAX_ANSWERS 99
#define AN_SIZE 3
#define DISPLAY_ANSWERS 10

int nUDP, nTCP, fdUDP, fdTCP, newfd;
socklen_t addrlenUDP, addrlenTCP;
struct addrinfo hintsUDP, hintsTCP, *resUDP, *resTCP;
struct sockaddr_in addrUDP, addrTCP;

char buffer[BUFFER_SIZE];
int numberOfTopics = 0;
char **listWithTopics;

void handleKill(int sig);
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
char* questionGetReadFiles(char* path, char* question, int qUserId, int numberOfAnswers, int qIMG, char *qixt);
char* getAnswerInformation(char *path, char *question, char *numb);
char * listOfQuestions(char * topic);
char* submitAnswer(char* input);

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
                printf("\nUDP - ");
                int addrlen = sizeof(addrUDP);
                char *bufferUDP = malloc(sizeof(char) * BUFFER_SIZE);

                nMsg = recvfrom(fdUDP, bufferUDP, BUFFER_SIZE, 0, (struct sockaddr*) &addrUDP, &addrlen);
                if (nMsg == -1) /*error*/ exit(1);

                /*Analyze message*/
                char *response = processUDPMessage(strtok(bufferUDP, "\n"), BUFFER_SIZE);

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

                nMsg = read(newfd, bufferTCP, BUFFER_SIZE);
                if (nMsg == -1) exit(1);

                /*Analyze message*/
                char *response = processTCPMessage(strtok(bufferTCP, "\n"), BUFFER_SIZE);

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

void handleKill(int sig){
    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);
    freeTopicInList();
    free(listWithTopics);
    _Exit(EXIT_SUCCESS);
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
        printf("Sent list of topics.\n");
        return response;
    }

    else if (strcmp(command, "PTP") == 0) {
        response = topicPropose(bufferBackup);
        free(bufferBackup);
        return response;
    }

    else if (strcmp(command, "GQU") == 0) {

    }

    else if (strcmp(command, "LQU") == 0) {
        command = strtok(NULL, " ");
        if (command == NULL) return strdup("ERR\n");

        printf("%s\n", command);
        response = listOfQuestions(command);
        printf("Sent list of questions.\n");
        return response;
    }

    else {
        printf("Command not found.\n");
        free(bufferBackup);
        response = strdup("ERR\n");
        return response;
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

    else if (strcmp(command, "QUS") == 0) {
        response = strdup("QUR OK"); //TODO, just for testing
        free(bufferBackup);
        return response;
    }

    else if (strcmp(command, "ANS") == 0) {
        response = submitAnswer(bufferBackup);
        free(bufferBackup);
        return response;
    }

    else {
        printf("Command not found.\n");
        free(bufferBackup);
        response = strdup("ERR\n");
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

    response = strdup("RGR OK\n");
    return response;
}

char* listOfTopics() {
    char *response = malloc(sizeof(char) * BUFFER_SIZE);
    char *finalResponse = malloc(sizeof(char) * BUFFER_SIZE);
    char numberString[6];
    char *line = NULL;
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

    if (numberOfTopics == MAX_TOPICS) response = strdup("PTR FUL\n");
    else if (strlen(topic) > TOPICNAME_SIZE) response = strdup("PTR NOK\n");
    else if (isTopicInList(topic)) response = strdup("PTR DUP\n");
    else {
        addToTopicList(topic, id);
        response = strdup("PTR OK\n");
    }

    return response;
}

void updateListWithTopics() {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    FILE *topicList;

    topicList = fopen(TOPIC_LIST, "r");
    if (topicList == NULL) exit(1);

    while ((nread = getline(&line, &len, topicList)) != -1) {
        char *token;

        token = strtok(line, ":");
        listWithTopics[numberOfTopics] = strdup(token);
        //printf("-> %s\n", listWithTopics[numberOfTopics]);
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
    for (int i = 0; i < numberOfTopics; i++) free(listWithTopics[i]);
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

    strcpy(path, TOPIC_FOLDER);
    strcat(path, topic);
    path[strlen(path)] = '\0';
    char *topicFolderPath = strdup(path);
    strcat(path, QUESTIONS_LIST);

    int qIMG = 0;
    char *qiext = NULL;
    int foundQuestion = 0;
    int qUserId;
    int numberOfAnswers;
    char *line = NULL;
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
            if (strcmp(strtok(NULL, ":"), "1") == 0) {
                qiext = strtok(NULL, ":");
                qIMG = 1;
            }
            break;
        }
    }

    fclose(questionsFd);
    free(path);

    if (!foundQuestion) {
        response = strdup("QGR EOF\n");
        free(topicFolderPath);
        return response;
    }

    response = questionGetReadFiles(topicFolderPath, question, qUserId, numberOfAnswers, qIMG, qiext);
    free(topicFolderPath);
    free(line);
    return response;
}

char* questionGetReadFiles(char* path, char* question, int qUserId, int numberOfAnswers, int qIMG, char *qiext) {
    /*Path for the requested question*/
    char *questionPath = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(questionPath, BUFFER_SIZE, "%s/%s.txt", path, question);

    /*Get the question in the file, check if there is a image and what is its extention*/
    size_t len = 0;
    ssize_t nread;
    FILE *questionFd;
    questionFd = fopen(questionPath, "r");
    if (questionFd == NULL) exit(1);

    fseek(questionFd, 0L, SEEK_END);
    long qsize = ftell(questionFd);
    fseek(questionFd, 0L, SEEK_SET);
    char *qdata = malloc(sizeof(char) * (qsize + 1));
    fread(qdata,qsize,sizeof(unsigned char),questionFd);

    fclose(questionFd);
    free(questionPath);

    /*If there is a image get its size and data*/
    long qisize;
    char *qidata;
    if (qIMG) {
        char *imgPath = malloc(sizeof(char) * BUFFER_SIZE);
        FILE *imageFd;
        snprintf(imgPath, BUFFER_SIZE, "%s/%s.%s", path, question, qiext);
        imageFd = fopen(imgPath, "r");
        if (imageFd == NULL) exit(1);

        fseek(imageFd, 0L, SEEK_END);
        qisize  = ftell(imageFd);
        fseek(imageFd, 0L, SEEK_SET);

        qidata = (char*) malloc(sizeof(char) * (qisize + 1));
        strcpy(qidata, "");
        fread(qidata,qisize,sizeof(unsigned char),imageFd);

        fclose(imageFd);
        free(imgPath);
    }

    /*Get the answers information*/
    char *answers = malloc(sizeof(char) * BUFFER_SIZE);
    strcpy(answers, "");
    for (int i = 1; (i <= numberOfAnswers) && (i <= DISPLAY_ANSWERS); i++) {
        char *questionNumber = malloc(sizeof(char) * AN_SIZE);
        i < 10 ? snprintf(questionNumber, AN_SIZE, "0%d", i) : snprintf(question, AN_SIZE, "%d", i);
        char *answerInfo = getAnswerInformation(path, question, questionNumber);
        strcat(answers, answerInfo);
        free(questionNumber);
        free(answerInfo);
    }

    char *response = malloc(sizeof(char) * BUFFER_SIZE);
    if (qIMG) {
        snprintf(response, BUFFER_SIZE, "QGR %d %ld %s 1 %s %ld %s %d %s\n", qUserId, qsize, qdata, qiext, qisize, qidata, numberOfAnswers, answers);
        free(qidata);
    }

    else {
        snprintf(response, BUFFER_SIZE, "QGR %d %ld %s 0 %d %s\n", qUserId, qsize, qdata, numberOfAnswers, answers);
    }

    free(answers);
    free(qdata);
    return response;
}

char* getAnswerInformation(char *path, char *question, char *numb) {
    /*get information about the answer*/
    char *answerDesc = malloc(sizeof(char) * BUFFER_SIZE);
    FILE *answerDescFd;
    snprintf(answerDesc, BUFFER_SIZE, "%s/%s_%s%s.txt", path, question, numb, QUESTIONS_DESC);

    answerDescFd = fopen(answerDesc, "r");
    if (answerDescFd == NULL) exit(1);

    int aIMG = 0;
    char *aiext  = NULL;
    int aUserID;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while ((nread = getline(&line, &len, answerDescFd)) != -1) {
        char *token = strtok(line, ":");
        aUserID  = atoi(token);
        if (strcmp(strtok(NULL, ":"), "1") == 0) {
            aiext  = strtok(NULL, ":");
            aIMG  = 1;
        }
        break;
    }

    fclose(answerDescFd);
    free(answerDesc);

    //Get answer data
    char *answerPath = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(answerPath, BUFFER_SIZE, "%s/%s_%s.txt", path, question, numb);
    
    char *adata;
    long asize;
    len = 0;
    FILE *answerFd;
    answerFd = fopen(answerPath, "r");
    if (answerFd == NULL) exit(1);

    //Get answer size
    fseek(answerFd, 0L, SEEK_END);
    asize = ftell(answerFd);
    fseek(answerFd, 0L, SEEK_SET);
    adata = (char*) malloc(sizeof(char) * (asize + 1));
    strcpy(adata, "");
    fread(adata,asize,sizeof(unsigned char),answerFd);

    fclose(answerFd);
    free(answerPath);

    /*Get the answer's image information*/
    long aisize;
    char *aidata;
    char *respose = malloc(sizeof(char) * BUFFER_SIZE);
    if (aIMG) {
        char *imgPath = malloc(sizeof(char) * BUFFER_SIZE);
        FILE *imageFd;
        snprintf(imgPath, BUFFER_SIZE, "%s/%s_%s.%s", path, question, numb, aiext);
        imageFd = fopen(imgPath, "r");
        if (imageFd == NULL) exit(1);

        //Get image size
        fseek(imageFd, 0L, SEEK_END);
        aisize = ftell(imageFd);
        fseek(imageFd, 0L, SEEK_SET);

        //Get image data
        aidata = (char*) malloc(sizeof(char) * (aisize + 1));
        strcpy(aidata, "");
        fread(aidata,aisize,sizeof(unsigned char),imageFd);

        fclose(imageFd);
        free(imgPath);

        snprintf(respose, BUFFER_SIZE, "%d %ld %s %d %s %s %ld %s", aUserID, asize, adata, aIMG, aiext, numb, aisize, aidata);
        free(aidata);
    }

    else snprintf(respose, BUFFER_SIZE, "%d %ld %s 0", aUserID, asize, adata);

    free(adata);
    free(line);
    return respose;
}

char* listOfQuestions(char *topic) {
    int i = 1;
    char path[33] = TOPIC_FOLDER, question[16]; // TODO review size
    char * response, * line;
    size_t len = 0;

    strcat(path, topic);
    strcat(path, QUESTIONS_LIST);
    response = malloc (sizeof(char) * BUFFER_SIZE);
    response = strdup("LQR");
    
    FILE *fp = fopen (path, "r");
    if (!fp) {
        fprintf (stderr, "error: file open failed '%s'.\n", path);
        return response;
    }

    while (getline(&line, &len, fp) != -1) {
        char * token = strtok(line, ":");
        snprintf(question, 16," %s", token);
        strcat(response, question);
    }

    strcat(response, "\n");
    fclose(fp);
    return response;
}

char* submitAnswer(char* input){
    char *userID, *topic, *question, *asize, *adata, *adataAux, *aIMG, *iext = NULL, *isize = NULL, *idata = NULL;
    char *response;

    if (strcmp(strtok(input, " "), "ANS")) return strdup("ERR\n"); //Check if command is ANS
    
    //Get arguments
    userID = strtok(NULL, " "); topic = strtok(NULL, " "); question = strtok(NULL, " ");
    asize = strtok(NULL, " "); adataAux = strtok(NULL, "\n");

    int asizeInt = atoi(asize);
    adata = (char*) malloc(sizeof(char)*(asizeInt+1));
    adata[0] = '\0';
    strncpy(adata, adataAux, atoi(asize));

    //Check if data of file is correct
    if (asizeInt > strlen(adataAux)){
        free(adata);
        return strdup("ANR NOK\n");
    }
    memcpy(input, adataAux + asizeInt+1, strlen(adataAux)-asizeInt);
    aIMG = strtok(input, " "); iext = strtok(NULL, " "); isize = strtok(NULL, " "); idata = strtok(NULL, "\n");

    //Protection against unusual number of arguments
    if (userID == NULL || topic == NULL || question == NULL || asize == NULL || adata == NULL || aIMG == NULL) {
        free(adata);
        return strdup("ANR NOK\n");
    }

    //Check if topic exists
    int found = isTopicInList(topic);
    if (!found) {
        free(adata);
        return strdup("ANR NOK\n");
    }

    //Check if question exists
    int lenQuestionPath = strlen(TOPIC_FOLDER) + strlen(topic) + strlen(QUESTIONS_LIST)+1;
    char *questionPath = (char*) malloc(sizeof(char)* (lenQuestionPath) );
    snprintf(questionPath, lenQuestionPath, "%s%s%s", TOPIC_FOLDER, topic, QUESTIONS_LIST);

    FILE *questionListFP = fopen(questionPath, "r+");
    if (questionListFP == NULL){
        free(adata);
        free(questionPath);
        return strdup("ANR NOK\n");
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    char *numOfAnswersInput = NULL, *qUserCreated, *qImg, *qExt;
    int numOfAnswers = -1;
    long questionListOffset = 0, lineSize = 0;
    //Find question and get number of answers
    while ((nread = getline(&line, &len, questionListFP)) != -1) {
        // Text file format:  QUESTION:USERID:N_OF_ANS
        char *questionAux = strtok(line,":");
        if (!strcmp(question, questionAux)){
            qUserCreated = strtok(NULL, ":"); 
            numOfAnswersInput = strtok(NULL, ":");
            qImg = strtok(NULL, ":");
            qExt = strtok(NULL, ":");
            numOfAnswers = atoi(numOfAnswersInput);
            lineSize = ftell(questionListFP);
            break;
        }
        questionListOffset = ftell(questionListFP);
    }
    //File closed afterwards

    //Question not found
    if ( numOfAnswers == -1){ 
        fclose(questionListFP);
        free(adata);
        free(questionPath);
        free(line);
        return strdup("ANR NOK\n");
    }

    //Check if answer list is full
    if (numOfAnswers == MAX_ANSWERS){
        fclose(questionListFP);
        free(adata);
        free(questionPath);
        free(line);
        return strdup("ANR FUL\n");
    }

    //Write answer to file
    numOfAnswers++;
    int lenAnswerPath = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + 2 + 4 + 1; //Example: question_56.txt\0
    char *answerPath = (char*) malloc(sizeof(char)*lenAnswerPath);
    snprintf(answerPath, lenAnswerPath, "%s%s/%s_%02d.txt", TOPIC_FOLDER, topic, question, numOfAnswers);
    FILE *answerFP = fopen(answerPath, "w");
    if (answerFP == NULL){
        fclose(questionListFP);
        free(adata);
        free(questionPath);
        free(answerPath);
        free(line);
        return strdup("ANR NOK\n");
    }
    fputs(adata, answerFP);
    fclose(answerFP);
    free(adata);
    free(answerPath);

    //Write answer description
    int lenAnswerDescPath = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + 2 + strlen(QUESTIONS_DESC) + 4 + 1; //Example: question_56_desc.txt\0
    char *answerDescPath = (char*) malloc(sizeof(char)*lenAnswerDescPath);
    snprintf(answerDescPath, lenAnswerDescPath, "%s%s/%s_%02d%s.txt", TOPIC_FOLDER, topic, question, numOfAnswers, QUESTIONS_DESC);
    FILE *answerDescFP = fopen(answerDescPath, "w");
    if (answerDescFP == NULL){
        fclose(questionListFP);
        free(questionPath);
        free(answerDescPath);
        free(line);
        return strdup("ANR NOK\n");
    }
    if (!strcmp(aIMG, "1")) fprintf(answerDescFP, "%s:%s:%s:", userID, aIMG, iext);
    else fprintf(answerDescFP, "%s:%s:", userID, aIMG);
    fclose(answerDescFP);
    free(answerDescPath);
    
    //Write image
    if (!strcmp(aIMG, "1")){
        if (iext == NULL || isize == NULL || idata == NULL){
            fclose(questionListFP);
            free(questionPath);
            free(line);
            return strdup("ANR NOK\n");
        }

        int isizeInt = atoi(isize);
        int lenAnswerImgPath = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + 2 + strlen(iext) + 1; //Example: question_56.jpg\0
        char *answerImgPath = (char*) malloc(sizeof(char)*lenAnswerImgPath);
        snprintf(answerPath, lenAnswerPath, "%s%s/%s_%02d.%s", TOPIC_FOLDER, topic, question, numOfAnswers, iext);
        FILE *answerImgFP = fopen(answerPath, "w");
        if (answerImgFP == NULL){
            fclose(questionListFP);
            free(questionPath);
            free(answerImgPath);
            free(line);
            return strdup("ANR NOK\n");
        }
        fputs(idata, answerImgFP);
        fclose(answerImgFP);
        free(answerImgPath);
    }

    //Update number of answers
    fseek(questionListFP, questionListOffset, SEEK_SET);
    fprintf(questionListFP, "%s:%s:%02d:%s:%s:\n", question, qUserCreated, numOfAnswers, qImg, qExt);
    fclose(questionListFP);
    free(questionPath);
    free(line);

    //Output to screen
    printf("New answer received: %s/%s\n", topic, question);

    response = strdup("ANR OK\n");
    return response;
}
