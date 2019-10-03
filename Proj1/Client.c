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
#include <math.h>

#define  DEFAULT_PORT "58013"
#define BUFFER_SIZE 2048
#define ID_SIZE 5
#define REGISTER_SIZE 12
#define NUM_TOPICS 99
#define NUM_QUESTIONS 99
#define LEN_TOPIC 10
#define LEN_COMMAND 3

void parseArgs(int number, char** arguments, char **port, char **ip);
void connectToServer(int *udp_fd, int *tcp_fd, char *ip, char *port, struct addrinfo hints, struct addrinfo **resUDP, struct addrinfo **resTCP);
void SendMessageUDP(char *message, int fd, struct addrinfo *res);
char* receiveMessageUDP(int fd, socklen_t addrlen, struct sockaddr_in addr);
void SendMessageTCP(char *message, int *fd, struct addrinfo **res);
char* receiveMessageTCP(int fd);
int recvTCPWriteFile(int fd, char* filePath, char** bufferAux, int bufferSize, int* offset, int size);
void parseCommands(int *userId, int udp_fd, int tcp_fd, struct addrinfo *resUDP, struct addrinfo *resTCP, socklen_t addrlen, struct sockaddr_in addr);
void registerNewUser(int id, int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr);
void requestLTP(int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr, char** topics, int* numTopics);
void freeTopics(int numTopics, char** topics);
char* topicSelectNum(int numTopics, char** topics, int topicChosen);
char* topicSelectName(int numTopics, char** topics, char* name);
int getQuestionList(int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr, char* topicChosen, char** questions);
void freeQuestions(int numQuestions, char** questions);
void answerSubmit(int fd, struct addrinfo **res, int aUserID, char *topicChosen, char* questionChosen, char *text_file, char *img_file);
char * questionSelectNum(int question, int num_questions, char ** questions);
void questionGet(char * reply, char * topic, char * title);
char * questionSelectName(char * name, int num_questions, char ** questions);
void submitQuestion(int *fd, struct addrinfo **res, int aUserID, char *topicChosen, char* question, char* text_file, char* img_file);

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

void SendMessageTCP(char *message, int *fd, struct addrinfo **res) {
    ssize_t n;

    *fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (*fd == -1) exit(1);
    
    n = connect(*fd, (*res)->ai_addr, (*res)->ai_addrlen);
    if (n == -1) exit(1);

    n = write(*fd, message, strlen(message));
    if (n == -1) exit(1);
}

char* receiveMessageTCP(int fd) {
    ssize_t n;  
    buffer[0] = '\0';

    while (strtok(buffer, "\n") == NULL) {
        n = read(fd, buffer, BUFFER_SIZE);
        if (n == -1) exit(1);
    }
    
    if (debug == 1) printf("Received: |%s|\n", buffer);
    return buffer;
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
    else {
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

void parseCommands(int *userId, int udp_fd, int tcp_fd, struct addrinfo *resUDP, struct addrinfo *resTCP, socklen_t addrlen, struct sockaddr_in addr) {
    int numTopics = -1, numQuestions = -1;
    char * status, msg[21];
    char *line = NULL, *command;
    char ** topics = malloc(sizeof(char*)*NUM_TOPICS), ** questions = malloc(sizeof(char*)*NUM_QUESTIONS);
    char * topicChosen = NULL, * questionChosen = NULL;
    char *answerPath, *answerImg;
    size_t size = 0;

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
            char *arg;
            command = strtok(NULL, " ");
            if (command != NULL) {
                arg = strtok(command, "\n");
                if (strtok(NULL, " ") != NULL || arg == NULL) printf("Invalid command.\n");
                else {
                    topicChosenNum = atoi(arg);
                    topicChosen = topicSelectNum(numTopics, topics, topicChosenNum);
                }
            }
        }

        else if ((strcmp(command, "topic_select") == 0) && *userId != -1){
            char *arg;
            command = strtok(NULL, " ");
            if (command != NULL){
                arg = strtok(command, "\n");
                if (strtok(NULL, " ") != NULL || arg == NULL) printf("Invalid command.\n");
                else {
                    topicChosen = topicSelectName(numTopics, topics, arg);
                }
            }

        }

        else if (!strcmp(command, "tp") || !strcmp(command, "topic_propose")) {            
            sprintf(msg, "PTP %d %s\n", *userId, strtok(NULL, " "));
            SendMessageUDP(msg, udp_fd, resUDP);
            status = receiveMessageUDP(udp_fd, addrlen, addr);

            if (!strcmp(status, "PTR OK"))
                printf("Topic accepted!\n");
            else if (!strcmp(status, "PTR DUP"))
                printf("Could not register topic, topic already exists.\n");
            else if (!strcmp(status, "PTR FUL"))
                printf("Could not register topic, topic list is already full.\n");
            else if (!strcmp(status, "PTR NOK"))
                printf("Could not register topic.\n");

        }

        else if ((strcmp(command, "question_list\n") == 0 || strcmp(command, "ql\n") == 0) && *userId != -1){
            numQuestions = getQuestionList(udp_fd, resUDP, addrlen, addr, topicChosen, questions);
        }

        else if ((strcmp(command, "qg") == 0) && *userId != -1){
            int question;
            char * arg, * reply;
            command = strtok(NULL, " ");

            if (command) {
                arg = strtok(command, "\n");
                if (strtok(NULL, " ") != NULL || arg == NULL)
                    printf("Invalid command.\n");
                else {
                    question = atoi(arg);
                    questionChosen = questionSelectNum(question, numQuestions, questions);

                    if (questionChosen) {
                        sprintf(msg, "GQU %s %s\n", topicChosen, questionChosen);
                        SendMessageTCP(msg, &tcp_fd, &resTCP);
                        questionGet(receiveMessageTCP(tcp_fd), topicChosen, questionChosen);
                        close(tcp_fd);
                    }
                }
            }
        }

        else if ((strcmp(command, "question_get") == 0) && *userId != -1){
            char *arg;
            command = strtok(NULL, " ");

            if (command){
                arg = strtok(command, "\n");
                if (strtok(NULL, " ") != NULL || arg == NULL)
                    printf("Invalid command.\n");
                else {
                    questionChosen = questionSelectName(arg, numQuestions, questions);

                    if (questionChosen) {
                        sprintf(msg, "GQU %s %s\n", topicChosen, questionChosen);
                        SendMessageTCP(msg, &tcp_fd, &resTCP);
                        questionGet(receiveMessageTCP(tcp_fd), topicChosen, questionChosen);
                        close(tcp_fd);
                    }
                }
            }
        }

        else if ((strcmp(command, "question_submit") == 0 || strcmp(command, "qs") == 0) && *userId != -1){
            char *question = NULL, *text_file = NULL, *img_file = NULL;
            command = strtok(NULL, "\n");
            questionChosen = strtok(command, " ");
            text_file = strtok(NULL, " ");
            img_file = strtok(NULL, "\n");
            if (questionChosen == NULL || text_file == NULL) printf("Invalid arguments.\n");
            else submitQuestion(&tcp_fd, &resTCP, *userId, topicChosen, questionChosen, text_file, img_file);
        }

        else if (((strcmp(command, "as") == 0) || (strcmp(command, "answer_submit") == 0)) && *userId != -1) { //TODO
            answerPath = strtok(NULL, " ");
            answerImg = strtok(NULL, " ");
            answerSubmit(tcp_fd, &resTCP, *userId, topicChosen, questionChosen, strtok(answerPath, "\n"), answerImg);
        }

        else if (strcmp(line, "help\n") == 0) {
            printf("\n");
            printf("reg (id)\t- sign in\n");
            printf("tl\t\t- list topics\n");
            printf("tp (topic_name)\t- propose topic\n");
            printf("ts (topic_id)\t- select topic by id\n");
            printf("ql\t\t- list questions of select topic\n");
            printf("exit\t\t- exit program\n");
        }

        else if (strcmp(line, "exit\n") == 0) {
            freeQuestions(numQuestions, questions);
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
    strcmp(status, "RGR OK") ==  0 ? printf("User \"%d\" registered\n", id) : printf("Could not register user, invalid user ID.\n");
    free(message);
}

void requestLTP(int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr, char** topics, int* numTopics) {
    int i = 1, N, offset, user;
    char * iter, * ltr, * name, * sep;

    SendMessageUDP("LTP\n", fd, res);
    ltr = receiveMessageUDP(fd, addrlen, addr);

    assert(!strcmp(strtok(ltr, " "), "LTR"));
    N = atoi(strtok(NULL, " "));

    printf("available topics:\n");
    while (i <= N) {
        iter = strtok(NULL, " ");

        sep = strstr(iter, ":");
        offset = sep - iter;
        name = malloc(sizeof(char) * offset);

        strncpy(name, iter, offset);
        user = atoi(iter + offset + 1);

        topics[i-1] = strdup(iter);
        printf("%d - %s (proposed by %d)\n", i++, name, user);

        free(name);
    }

    *numTopics = N;
}

void freeTopics(int numTopics, char** topics){
    int i;
    for (i=0; i<numTopics; i++) free(topics[i]);
    free(topics);
}

char* topicSelectNum(int numTopics, char** topics, int topicChosen){
    if (numTopics == -1){
        printf("Run tl first.\n");
        return NULL;
    }
    else if (topicChosen > numTopics || topicChosen <= 0){
        printf("Invalid topic number.\n");
        return NULL;
    }

    char *topic = strtok(topics[topicChosen-1], ":");
    char *userId = strtok(NULL, ":");
    printf("selected topic: %s (%s)\n", topic, userId);
    return topic;
}

char* topicSelectName(int numTopics, char** topics, char* name){
    char* topic = NULL;
    char* userId = NULL;
    int i;
    
    if (numTopics == -1){
        printf("Run tl first.\n");
        return NULL;
    }

    for (i = 0; i < numTopics; i++){
        char *nextTopic = strtok(topics[i],":");
        userId = strtok(NULL,":");
        if (!strcmp(nextTopic, name)){
            topic = nextTopic;
            break;
        }
    }

    topic == NULL ? printf("Can't find that topic.\n") : printf("selected topic: %s (%s)\n", topic, userId);

    return topic;
}

int getQuestionList(int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr, char* topicChosen, char** questions){
    int i = 0, lenMsg = LEN_COMMAND + 1 + LEN_TOPIC + 1 + 1, numQuestions = 0;
    char *iter, *message = malloc(sizeof(char) * (lenMsg));

    if (topicChosen == NULL) { printf("Select your topic first.\n"); return -1; }

    snprintf(message, lenMsg, "LQU %s\n", topicChosen);
    SendMessageUDP(message, fd, res);
    char * questionList = receiveMessageUDP(fd, addrlen, addr);

    if (!strcmp(questionList,"ERR") || !strcmp(strtok(questionList, " "), "LQR "))
        printf("ERROR\n");
    
    numQuestions = atoi(strtok(NULL, " \n"));
    if (!numQuestions) {
        printf("no available questions about %s\n", topicChosen);
        return -1;
    }

    printf("available questions about %s:\n", topicChosen);
    while (iter = strtok(NULL, " \n"))
        questions[i++] = strdup(iter);
    
    for (i = 0; i < numQuestions; i++) {
        questions[i] = strdup(strtok(questions[i], ":"));
        printf("%d - %s\n", i + 1, questions[i]);
    }

    free(message);
    return numQuestions;
}

void freeQuestions(int numQuestions, char** questions){
    int i;
    for (i=0; i<numQuestions; i++) free(questions[i]);
    free(questions);
}

void submitQuestion(int *fd, struct addrinfo **res, int aUserID, char *topicChosen, char* question, char* text_file, char* img_file){
    if (strlen(question) > 10){
        printf("Question is too big.\n");
        return;
    }

    if (strlen(text_file) == 0) return;

    /*Get the question and its size*/    
    char *adata;
    long asize;
    FILE *questionFd;

    char *textPath = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(textPath, BUFFER_SIZE, "%s.txt", text_file);

    questionFd = fopen(textPath, "r");
    if (questionFd == NULL) {
        printf("Can't find file.\n");
        free(textPath);
        return;
    }

    //Get size of file
    fseek(questionFd, 0L, SEEK_END);
    asize = ftell(questionFd);
    fseek(questionFd, 0L, SEEK_SET);

    adata = (char*) malloc(sizeof(char) * (asize + 1));
    strcpy(adata, "");
    fread(adata,asize,sizeof(unsigned char),questionFd);

    fclose(questionFd);

    /*Get the question's image information*/
    long aisize;
    char *aidata;
    char *response = malloc(sizeof(char) * BUFFER_SIZE);
    if (img_file != NULL) {
        FILE *imageFd;
        imageFd = fopen(img_file, "rb");
        if (imageFd == NULL) {
            printf("Can't find image file.\n");
            free(textPath);
            free(adata);
            free(response);
            return;
        }

        //Get image size
        fseek(imageFd, 0L, SEEK_END);
        aisize = ftell(imageFd);
        fseek(imageFd, 0L, SEEK_SET);

        char *aiext = strtok(img_file, ".");
        aiext = strtok(NULL, ".");

        snprintf(response, BUFFER_SIZE, "QUS %d %s %s %ld %s 1 %s %ld ", aUserID, topicChosen, question, asize, adata, aiext, aisize);
        SendMessageTCP(response, fd, res);

        aidata = (char*) malloc(sizeof(char) * BUFFER_SIZE);
        int sizeAux = aisize;
        
        while (sizeAux > 0){
            int nRead = fread(aidata, 1 , BUFFER_SIZE,imageFd);
            write(*fd, aidata, nRead);
            sizeAux = sizeAux - BUFFER_SIZE;
        }
        
        write(*fd, "\n", strlen("\n"));

        fclose(imageFd);        
        free(aidata);
    }

    else {
        snprintf(response, BUFFER_SIZE, "QUS %d %s %s %ld %s 0\n", aUserID, topicChosen, question, asize, adata);
        SendMessageTCP(response, fd, res);
    }

    free(adata);
    free(textPath);
    
    free(response);

    char* reply = receiveMessageTCP(*fd);
    if (!strcmp(reply, "QUR OK")) printf("question submitted with success\n");
    else if (!strcmp(reply, "QUR DUP")) printf("question is a duplicate, try again.\n");
    else if (!strcmp(reply, "QUR FUL")) printf("the question list is full.\n");
    else if (!strcmp(reply, "QUR NOK")) printf("an error has occurred, try again.\n");

    close(*fd);
}

void answerSubmit(int fd, struct addrinfo **res, int aUserID, char *topicChosen, char* questionChosen, char *text_file, char *img_file) {
    size_t len = 0;
    ssize_t nread;
    FILE *answerFd;
    answerFd = fopen(text_file, "r");
    if (answerFd == NULL) {
        printf("Can't find answer file.\n");
        return;
    }

    fseek(answerFd, 0L, SEEK_END);
    long asize = ftell(answerFd);
    fseek(answerFd, 0L, SEEK_SET);
    char *adata = malloc(sizeof(char) * (asize + 1));
    fread(adata,asize,sizeof(unsigned char),answerFd);
    fclose(answerFd);

    char *message;
    int aIMG = 0;
    long isize = 0;
    if (img_file != NULL) {
        aIMG = 1;
        len = 0;
        FILE *imgFd;
        imgFd = fopen(strtok(img_file, "\n"), "rb");

        if (imgFd == NULL) {
            printf("Can't find image file.\n");
            free(adata);
            return;
        }

        strtok(img_file, ".");
        char *iext = strtok(NULL, ".");
        if (strlen(iext) > 3) {
            printf("img extention has more than 3 bytes.\n");
            free(adata);
            return;
        }

        fseek(imgFd, 0L, SEEK_END);
        isize = ftell(imgFd);

        message = malloc(sizeof(char) * (BUFFER_SIZE + asize));
        snprintf(message, BUFFER_SIZE + asize + isize, "ANS %d %s %s %ld %s %d %s %ld ", aUserID, topicChosen, questionChosen, asize, adata, aIMG, iext, isize);
        SendMessageTCP(message, &fd, res);

        int sizeAux = isize;
        fseek(imgFd, 0L, SEEK_SET);
        char *idata = malloc(sizeof(char) * (BUFFER_SIZE));

        while (sizeAux > 0){
            int nRead = fread(idata, 1 , BUFFER_SIZE,imgFd);
            write(fd, idata, nRead);
            sizeAux = sizeAux - BUFFER_SIZE;
        }

        write(fd, "\n", strlen("\n"));
    
        fclose(imgFd);
        free(idata);

    }

    else {
        message = malloc(sizeof(char) * (BUFFER_SIZE + asize));
        snprintf(message, BUFFER_SIZE + asize + isize, "ANS %d %s %s %ld %s %d\n", aUserID, topicChosen, questionChosen, asize, adata, aIMG);
        SendMessageTCP(message, &fd, res);
    }

    //Parse reply
    char *reply = receiveMessageTCP(fd);
    if (!strcmp(reply, "ANR OK")) printf("answer submitted!\n");
    else if (!strcmp(reply, "ANR NOK")) printf("error submiting.\n");
    else if (!strcmp(reply, "ANR FUL")) printf("can't submit more answers.\n");

    close(fd);
    free(message);
    free(adata);
}

char * questionSelectNum(int question, int num_questions, char ** questions) {
    if (num_questions == -1) {
        printf("Run question list first.\n");
        return NULL;
    }
    else if (question > num_questions || question <= 0) {
        printf("Invalid question number.\n");
        return NULL;
    }

    return strtok(questions[question - 1], ":");
}

char * questionSelectName(char * name, int num_questions, char ** questions) {
    char * question = NULL;

    if (num_questions == -1){
        printf("Run question list first.\n");
        return NULL;
    }

    for (int i = 0; i < num_questions; i++){
        char * nextQuestion = strtok(questions[i],":");
        if (!strcmp(nextQuestion, name)){
            question = nextQuestion;
            break;
        }
    }

    if (!question) printf("Can't find that question.\n");
    return question;
}

void questionGet(char * reply, char * topic, char * title) {
    int qsize, qisize, qIMG, offset;
    int N, AN, asize, aIMG, aisize;
    char request[3], format[BUFFER_SIZE], * qdata, qiext[3], * qidata;
    char * adata, aiext[3], * aidata;

    sscanf(reply, "%s %*d %d", request, &qsize);
    qdata = malloc(sizeof(char) * qsize);
    offset = 12 + floor(log10(abs(qsize)));

    sprintf(format, "%%%dc %%d", qsize);
    sscanf(reply + offset, format, qdata, &qIMG);
    offset += (qsize + 3);

    if (qIMG) {
        sscanf(reply + offset, "%s %d", qiext, &qisize);
        qidata = malloc(sizeof(char) * qisize);
        sscanf(reply + offset, "%*s %*d %s", qidata);
        offset += (11 + floor(log10(abs(qisize)))); // review
    }

    sscanf(reply + offset, "%d", &N);
    offset += (2 + floor(log10(abs(N))));

    printf("stored files:\n");
    printf("  %s/%s.txt", topic, title);

    for (int i = 0; i < N; i++)
        printf(", %s/%s_%02d.txt", topic, title, i + 1);
    printf("\nQ: %s\n", qdata);

    for (int i = 0; i < N; i++) {
        // Parse relevant asnwer info
        sscanf(reply + offset, "%d %*d %d", &AN, &asize);
        adata = malloc(sizeof(char) * asize);
        offset += (11 + floor(log10(abs(asize))));

        // Read answer 
        sprintf(format, "%%%dc %%d", asize);
        sscanf(reply + offset, format, adata, &aIMG);
        offset += asize + 3;

        if (aIMG) {
            sscanf(reply + offset, "%s %d", aiext, &aisize);
            aidata = malloc(sizeof(char) * aisize);
            sscanf(reply + offset, "%*s %*d %s", aidata);
            offset += (11 + floor(log10(abs(aisize)))); // review
        }

        printf("A%02d: %s\n", i + 1, adata);
    }
}