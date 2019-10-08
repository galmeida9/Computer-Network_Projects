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
#include <sys/stat.h>

#define PORT "58013"
#define BUFFER_SIZE 2048
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
char* processTCPMessage(char* buffer, int len, int fd);
int recvTCPWriteFile(int fd, char* filePath, char** buffer, int bufferSize, int* offset, int size);
int checkIfStudentCanRegister(int number);
char* registerNewStudent(char* arg1);
char* listOfTopics();
char* topicPropose(char *input);
void updateListWithTopics();
int isTopicInList(char *topic);
void addToTopicList(char* topic, char *usedId);
void freeTopicInList();
char* questionGet(char *input, int fd);
char* questionGetReadFiles(char* path, char* question, int qUserId, int numberOfAnswers, int qIMG, char *qixt, int fd);
char* getAnswerInformation(char *path, char *question, char *numb, int fd);
char * listOfQuestions(char * topic);
char* submitAnswer(char* input, int sizeInput, int fd);
char* questionSubmit(char *input);

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
                char *response = processTCPMessage(bufferTCP, nMsg, newfd);

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
        free(bufferBackup);
        return NULL;
    }

    else if (strcmp(command, "LQU") == 0) {
        command = strtok(NULL, " ");
        if (command == NULL) {
            free(bufferBackup);
            return strdup("ERR\n");
        }

        printf("%s\n", command);
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

char* processTCPMessage(char* buffer, int len, int fd){
    char *command, *response, *bufferBackup = (char*) malloc(sizeof(char)*BUFFER_SIZE);
    size_t size;

    //bufferBackup = strdup(buffer);
    memcpy(bufferBackup, buffer, len);
    bufferBackup[len] = '\0';
    
    command = strtok(buffer, " ");

    if (!strcmp(command, "GQU"))
        response = questionGet(bufferBackup, fd);

    else if (!strcmp(command, "QUS"))
        response = questionSubmit(bufferBackup);

    else if (!strcmp(command, "ANS"))
        response = submitAnswer(bufferBackup, len, fd);

    else {
        printf("Command not found.\n");
        response = strdup("ERR\n");
    }

    free(bufferBackup);
    return response;
}

int recvTCPWriteFile(int fd, char* filePath, char** bufferAux, int bufferSize, int* offset, int size){
    char *buffer = (char*) malloc(sizeof(char)*bufferSize);
    //Open file
    FILE* fp = fopen(filePath, "wb");
    if (fp == NULL) return -1;

    int toWrite = size;

    if (toWrite <= (bufferSize - *offset)) {
        fwrite(*bufferAux+*offset, sizeof(char), toWrite, fp);
        *offset = *offset + toWrite + 1;
        toWrite = 0;
    }
    else if (*offset < (bufferSize)){
        fwrite(*bufferAux+*offset, sizeof(char), bufferSize-*offset, fp);
        toWrite = toWrite - (bufferSize-*offset);
    }

    //Receive message if needed
    ssize_t nMsg = 0;
    while (toWrite > 0 && (nMsg = read(fd, buffer, bufferSize))>0){
        int sizeAux = toWrite > nMsg? nMsg : toWrite;
        fwrite(buffer, 1, sizeAux, fp);
        toWrite = toWrite - sizeAux;
        if (toWrite <= 0) {
            *offset = *offset + sizeAux + 1;
            if (*offset >= bufferSize) {
                read(fd, buffer, bufferSize);
                *offset = *offset - bufferSize;
            }
            break;
        }
        memset(buffer, 0, sizeof(buffer));
        *offset = 0;
    }
    
    //Close file and return
    fclose(fp);
    memcpy(*bufferAux, buffer, nMsg);
    free(buffer);
    return 0;
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

        // Create folder for the new topic
        int lenDirectoryPath = strlen(TOPIC_FOLDER) + strlen(topic) + 1;
        char * directory = malloc(sizeof(char) * (lenDirectoryPath));
        snprintf(directory, lenDirectoryPath, "%s%s", TOPIC_FOLDER, topic);

        struct stat st = {0};
        if (stat(directory, &st) == -1) {
            mkdir(directory, 0700);
        }

        free(directory);
        int lenQuestionPath = strlen(TOPIC_FOLDER) + strlen(topic) + strlen(QUESTIONS_LIST) + 1;
        char * questionPath = malloc(sizeof(char) * (lenQuestionPath));
        snprintf(questionPath, lenQuestionPath, "%s%s%s", TOPIC_FOLDER, topic, QUESTIONS_LIST);

        FILE *topicFd = fopen(questionPath, "w");
        if (topicFd == NULL) printf("Failed to create file for the new topic.\n");
        fclose(topicFd);
        free(questionPath);
        
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

char* questionGet(char *input, int fd) {
    strtok(input, " ");
    char *response;
    char *topic = strtok(NULL, " ");
    char *question = strtok(NULL, "\n");

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
        free(line);
        return response;
    }

    response = questionGetReadFiles(topicFolderPath, question, qUserId, numberOfAnswers, qIMG, qiext, fd);
    free(topicFolderPath);
    free(line);
    return response;
}

char* questionSubmit(char * input) {
	int qUserId, qIMG, found, NQ = 0;
	long qsize, offset, isize;
	char * topic, * question, * qdata, * data, format[BUFFER_SIZE];
	char iext[3], * idata, * line = NULL, * questionAux, * response;
	size_t len;
	
	// Discard request type
	strtok(input, " ");

	qUserId = atoi(strtok(NULL, " "));
	topic = strtok(NULL, " ");
	question = strtok(NULL, " ");
	qsize = atol(strtok(NULL, " "));
	data = strtok(NULL, "\n");

	sprintf(format, "%%%ldc %%d", qsize);
	qdata = malloc(sizeof(char) * qsize);
	sscanf(data, format, qdata, &qIMG);

	if (qIMG) {
		offset = qsize + 3;
		sscanf(data + offset, "%s %ld", iext, &isize);
        idata = malloc(sizeof(char) * isize);

        // TODO proper image transmission
        sscanf(data + offset, "%*s %*d %s", idata);
	}
	free(qdata);

	// Check if topic exists
    found = isTopicInList(topic);
    if (!found) { return strdup("QUR NOK\n"); }

    // Check if question already exists
    int lenQuestionPath = strlen(TOPIC_FOLDER) + strlen(topic) + strlen(QUESTIONS_LIST)+1;
    char * questionPath = malloc(sizeof(char) * (lenQuestionPath));
    snprintf(questionPath, lenQuestionPath, "%s%s%s", TOPIC_FOLDER, topic, QUESTIONS_LIST);

	FILE * fd = fopen(questionPath, "a+");
    if (!fd) {
        free(questionPath);
        return strdup("QUR NOK\n");
    }
    
    found = 0;
    rewind(fd);
    while (getline(&line, &len, fd) != -1) {
        
        // Text file format:  question:qUserID:NA
        questionAux = strtok(line, ":");
        if (!strcmp(question, questionAux)) { found = 1; break; }
        NQ++;
    }
    free(line);
    free(questionPath);

    if (NQ >= 99)
    	response = strdup("QUR FUL\n");
    else if (!found && qIMG) {
    	fprintf(fd, "%s:%d:00:1:%s:\n", question, qUserId, iext);
    	response = strdup("QUR OK\n");
    }
    else if (!found) {
    	fprintf(fd, "%s:%d:00:0:\n", question, qUserId);
    	response = strdup("QUR OK\n");
    }
    else
    	response = strdup("QUR DUP\n");

    fclose(fd);
	return response;
}

char* questionGetReadFiles(char* path, char* question, int qUserId, int numberOfAnswers, int qIMG, char *qiext, int fd) {
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
    char *response = malloc(sizeof(char) * BUFFER_SIZE);
    if (qIMG) {
        char *imgPath = malloc(sizeof(char) * BUFFER_SIZE);
        FILE *imageFd;
        snprintf(imgPath, BUFFER_SIZE, "%s/%s.%s", path, question, qiext);
        imageFd = fopen(imgPath, "r");
        if (imageFd == NULL) exit(1);

        fseek(imageFd, 0L, SEEK_END);
        qisize  = ftell(imageFd);
        fseek(imageFd, 0L, SEEK_SET);

        snprintf(response, BUFFER_SIZE, "QGR %d %ld %s 1 %s %ld ", qUserId, qsize, qdata, qiext, qisize);
        write(fd, response, strlen(response));

        int sizeAux = qisize;
        char *qidata = malloc(sizeof(char) * (BUFFER_SIZE));

        while (sizeAux > 0){
            int nRead = fread(qidata, 1 , BUFFER_SIZE,imageFd);
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

    /*Get the answers information*/
    for (int i = 1; (i <= numberOfAnswers) && (i <= DISPLAY_ANSWERS); i++) {
        char *questionNumber = malloc(sizeof(char) * AN_SIZE);
        i < 10 ? snprintf(questionNumber, AN_SIZE, "0%d", i) : snprintf(question, AN_SIZE, "%d", i);
        getAnswerInformation(path, question, questionNumber, fd);
        free(questionNumber);
    }

    write(fd, "\n", strlen("\n"));
    free(qdata);
    return response;
}

char* getAnswerInformation(char *path, char *question, char *numb, int fd) {
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

    /*Get the answer in the file and its size*/
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

    //adata = (char*) malloc(sizeof(char) * (asize + 1));
    adata = (char*) calloc(asize + 1, sizeof(char));
    
    //strcpy(adata, ""); ????
    fread(adata,asize,sizeof(unsigned char),answerFd);

    fclose(answerFd);
    free(answerPath);

    /*Get the answer's image information*/
    long aisize;
    char *aidata;
    char *response = malloc(sizeof(char) * BUFFER_SIZE);
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

        snprintf(response, BUFFER_SIZE, " %s %d %ld %s %d %s %ld ", numb, aUserID, asize, adata, aIMG, aiext, aisize);
        write(fd, response, strlen(response));

        //Get image data
        int sizeAux = aisize;
        char *aidata = malloc(sizeof(char) * (BUFFER_SIZE));

        while (sizeAux > 0){
            int nRead = fread(aidata, 1 , BUFFER_SIZE, imageFd);
            write(fd, aidata, nRead);
            sizeAux = sizeAux - BUFFER_SIZE;
        }

        fclose(imageFd);
        free(imgPath);
        free(aidata);
    }

    else {
        snprintf(response, BUFFER_SIZE, " %s %d %ld %s 0", numb, aUserID, asize, adata);
        write(fd, response, strlen(response));
    }

    free(adata);
    free(line);
    return response;
}

char* listOfQuestions(char * topic) {
    int N = 0;
    char path[33] = TOPIC_FOLDER;
    char * response, * line = NULL, * question, * userID, * NA;
    size_t len = 0;

    strcat(path, topic);
    strcat(path, QUESTIONS_LIST);
    response = malloc(sizeof(char) * BUFFER_SIZE);
    
    FILE *fp = fopen (path, "r");
    if (!fp) {
        printf ("There are no questions available.\n");
        strcpy(response, "LQR 0\n");
        return response;
    }

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
    printf("/%s/\n", response);
    return response;
}

char* submitAnswer(char* input, int sizeInput, int fd){
    char *userID, *topic, *question, *asize, *inputAux, *aIMG, *iext = NULL, *isize = NULL, *idata = NULL;
    //printf("/%s/%d\n", input, sizeInput);

    if (strcmp(strtok(input, " "), "ANS")) return strdup("ERR\n"); //Check if command is ANS
    
    //Get arguments
    userID = strdup(strtok(NULL, " ")); topic = strdup(strtok(NULL, " ")); question = strdup(strtok(NULL, " "));
    asize = strtok(NULL, " ");

    int offset = 3 + 1 + strlen(userID) + 1 + strlen(topic) + 1 + strlen(question) + 1 + strlen(asize) + 1;

    int asizeInt = atoi(asize);

    //Check if topic exists
    int found = isTopicInList(topic);
    if (!found) {
        free(userID); free(topic); free(question);
        return strdup("ANR NOK\n");
    }

    //Check if question exists
    int lenQuestionPath = strlen(TOPIC_FOLDER) + strlen(topic) + strlen(QUESTIONS_LIST)+1;
    char *questionPath = (char*) malloc(sizeof(char)* (lenQuestionPath) );
    snprintf(questionPath, lenQuestionPath, "%s%s%s", TOPIC_FOLDER, topic, QUESTIONS_LIST);

    FILE *questionListFP = fopen(questionPath, "r+");
    if (questionListFP == NULL){
        free(questionPath);
        free(userID); free(topic); free(question);
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
        free(questionPath);
        free(line);
        free(userID); free(topic); free(question);
        return strdup("ANR NOK\n");
    }

    //Check if answer list is full
    if (numOfAnswers == MAX_ANSWERS){
        fclose(questionListFP);
        free(questionPath);
        free(line);
        free(userID); free(topic); free(question);
        return strdup("ANR FUL\n");
    }

    //Prepare file pathname
    numOfAnswers++;
    int lenAnswerPath = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + 2 + 4 + 1; //Example: question_56.txt\0
    char *answerPath = (char*) malloc(sizeof(char)*lenAnswerPath);
    snprintf(answerPath, lenAnswerPath, "%s%s/%s_%02d.txt", TOPIC_FOLDER, topic, question, numOfAnswers);

    //Receive and write text file
    if (recvTCPWriteFile(fd, answerPath, &input, BUFFER_SIZE, &offset, asizeInt) == -1) printf("erro\n");
    free(answerPath);

    //Check if input has argument of aIMG
    if ((BUFFER_SIZE - offset) < (2)){
        read(fd, input, BUFFER_SIZE);
        offset = BUFFER_SIZE - offset;
    }

    //Prepare for image
    aIMG = strtok(input+offset, " "); 
    int aIMGInt = 0;

    if (strcmp(strtok(aIMG, "\n"), "1") == 0){
        iext = strtok(input+offset+1+strlen(aIMG), " "); 
        isize = strtok(input+offset+1+strlen(aIMG)+1+strlen(iext), " ");
        aIMGInt = 1;
    }

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
        free(userID); free(topic); free(question);
        return strdup("ANR NOK\n");
    }

    if (aIMGInt == 1) fprintf(answerDescFP, "%s:1:%s:", userID, iext);
    else fprintf(answerDescFP, "%s:0:", userID);
    fclose(answerDescFP);
    free(answerDescPath);
    
    //Check if there is an image
    if (aIMGInt == 1){

        if (iext == NULL || isize == NULL){
            fclose(questionListFP);
            free(questionPath);
            free(line);
            free(userID); free(topic); free(question);
            return strdup("ANR NOK\n");
        }

        //Prepare image pathname
        int isizeInt = atoi(isize);
        int lenAnswerImgPath = strlen(TOPIC_FOLDER) + strlen(topic) + 1 + strlen(question) + 1 + 2 + 1 + strlen(iext) + 1; //Example: question_56.jpg\0
        char *answerImgPath = (char*) malloc(sizeof(char)*lenAnswerImgPath);
        snprintf(answerImgPath, lenAnswerImgPath, "%s%s/%s_%02d.%s", TOPIC_FOLDER, topic, question, numOfAnswers, iext);

        //Receive and write image
        offset = offset + strlen(aIMG) + 1 + strlen(iext) + 1 + strlen(isize) + 1;
        if (recvTCPWriteFile(fd, answerImgPath, &input, BUFFER_SIZE, &offset, isizeInt) == -1) printf("erro\n");

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
    free(userID); free(topic); free(question);

    return strdup("ANR OK\n");
}
