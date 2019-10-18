#include "Client.h"

int DEBUG_TEST = 0;
char *buffer;

int main(int argc, char **argv) {
    int udp_fd = -1, tcp_fd = -1, userId = -1;
    char *ip, *port;
    struct addrinfo hints, *resUDP, *resTCP;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    buffer = malloc(sizeof(char) * BUFFER_SIZE);
    buffer[0] = '\0';

    port = strdup(DEFAULT_PORT);
    ip = strdup("127.0.0.1");
    parseArguments(argc, &DEBUG_TEST, argv, &port, &ip);
    printf("Connected to %s:%s\n", ip, port);
    printf("Enter 'help' for available commands\n\n");

    connectToServer(&udp_fd, &tcp_fd, ip, port, hints, &resUDP, &resTCP);
    parseCommands(&userId, udp_fd, tcp_fd, resUDP, resTCP, addrlen, addr);

    free(ip);
    free(port);
    free(buffer);
    close(udp_fd);
    freeaddrinfo(resTCP);
    freeaddrinfo(resUDP);
    return 0;
}

void connectToServer(int *udp_fd, int *tcp_fd, char *ip, char *port, 
    struct addrinfo hints, struct addrinfo **resUDP, struct addrinfo **resTCP) {

    struct timeval tv = { 5, 0 };
    memset(&hints,0, sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_flags=AI_NUMERICSERV;

    /* UDP Connection */
    if (*udp_fd < 0) {
        hints.ai_socktype=SOCK_DGRAM;
        if (getaddrinfo(ip, port, &hints, resUDP)) exit(1);

        if ((*udp_fd = socket(
            (*resUDP)->ai_family, 
            (*resUDP)->ai_socktype, 
            (*resUDP)->ai_protocol
        )) == -1) exit(1); 

        /* Setting UDP socket timeout value */
        if (setsockopt(
            *udp_fd, SOL_SOCKET, SO_RCVTIMEO, 
            (struct timeval *)&tv, sizeof(struct timeval))) {
            
            printf("Failed to set a timeout for the UDP socket\n");
            close(*udp_fd);
            exit(2);
        }
    }

    /* TCP Connection */
    if (*tcp_fd < 0) {
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(ip, port, &hints, resTCP)) exit(1);
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
        if (n == -1) {
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) 
                exit(1);
            printf("Communication error\n");
            break;
        }
    }

    return buffer;
}

void SendMessageTCP(char *message, int *fd, struct addrinfo **res) {
    struct timeval tv = { 5, 0 };

    *fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (*fd == -1) exit(1);

    /* Setting TCP socket timeout value */
    if (setsockopt(
        *fd, SOL_SOCKET, SO_RCVTIMEO,
        (struct timeval *)&tv, sizeof(struct timeval))) {

        printf("Failed to set a timeout for the TCP socket\n");
        close(*fd);
        exit(2);
    }

    if ((connect(*fd, (*res)->ai_addr, (*res)->ai_addrlen)) == -1) exit(EXIT_FAILURE);
    if ((write(*fd, message, strlen(message))) == -1) exit(EXIT_FAILURE);
}

char* receiveMessageTCP(int fd) {
    buffer[0] = '\0';

    if ((read(fd, buffer, BUFFER_SIZE)) == -1) {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
            exit(1);
        printf("Communication error\n");
    }

    return buffer;
}

void parseCommands(int *userId, int udp_fd, int tcp_fd, struct addrinfo *resUDP, 
    struct addrinfo *resTCP, socklen_t addrlen, struct sockaddr_in addr) {
    
    int numTopics = -1, numQuestions = -1;
    int command_type, topicChosenNum, question, ans;
    char *text_file = NULL, *img_file = NULL, *line = NULL;
    char *response, *command, *answerPath, *answerImg, *arg, msg[21];
    char **topics, **questions, *topicChosen = NULL, *questionChosen = NULL;
    size_t size = 0;

    topics = arrayInit(NUM_TOPICS);
    questions = arrayInit(NUM_QUESTIONS);

    while(1) {
        memset(buffer, 0, sizeof(*buffer));
        if ((getline(&line, &size, stdin)) == -1) {
            printf("Error reading user input, please try again.\n");
            continue;
        }
        command = strtok(line, " ");

        if (!strcmp(command, "register") || !strcmp(command, "reg")) {
            if ((command = strtok(NULL, " ")) && strlen(command) == ID_SIZE + 1) {
                *userId = atoi(strtok(command, "\n"));
                
                if (strtok(NULL, " ") != NULL  || *userId == 0)
                    printf("Invalid command\n");
                else  if (!(ans = registerNewUser(
                    *userId, udp_fd, resUDP, addrlen, addr))) 
                    *userId = -1;
            }
            else
                printf("Invalid command\n");
        }

        else if (!strcmp(line, "help\n")) {
            printf("\n");
            printf("reg (id)\t- sign in\n");
            printf("tl\t\t- list topics\n");
            printf("tp (topic_name)\t- propose topic\n");
            printf("ts (topic_id)\t- select topic by id\n");
            printf("ql\t\t- list questions of select topic\n");
            printf("exit\t\t- exit program\n");
        }

        else if (!strcmp(line, "exit\n")) {
            freeQuestions(numQuestions, questions);
            freeTopics(numTopics, topics);
            free(topicChosen);
            free(questions);
            free(line);
            return;
        }

        /* Register is yet to be made, can't go past this point then... */
        else if (*userId <= 0) {
            printf("You need to register first before performing any commands\n");
            continue;
        }

        else if (!strcmp(command, "tl\n") || !strcmp(command, "topic_list\n")) {
            topicList(udp_fd, resUDP, addrlen, addr, topics, &numTopics);
        }

        else if (!strcmp(command, "ts") || !strcmp(command, "topic_select")) {
            command_type = !strcmp(command, "ts") ? 0 : 1;

            if (topicChosen != NULL)
                free(topicChosen);

            if ((command = strtok(NULL, " "))) {
                arg = strtok(command, "\n");
                if (strtok(NULL, " ") != NULL || arg == NULL)
                    printf("Invalid command\n");
                else if (command_type) {
                    topicChosen = topicSelectName(numTopics, topics, arg);
                }
                else {
                    topicChosenNum = atoi(arg);
                    topicChosen = topicSelectNum(numTopics, topics, topicChosenNum);
                }
            }
        }

        else if (!strcmp(command, "tp") || !strcmp(command, "topic_propose")) {
            char *topic = strtok(NULL, "\n");
            sprintf(msg, "PTP %d %s\n", *userId, topic);
            SendMessageUDP(msg, udp_fd, resUDP);

            response = receiveMessageUDP(udp_fd, addrlen, addr);

            if (!strcmp(response, "PTR OK")) {
                printf("Topic accepted!\n");
                topicChosen = strdup(topic);
            }
            else if (!strcmp(response, "\0"))
                printf("Communication error\n");
            else if (!strcmp(response, "PTR DUP"))
                printf("Could not register topic, topic already exists\n");
            else if (!strcmp(response, "PTR FUL"))
                printf("Could not register topic, topic list is already full\n");
            else if (!strcmp(response, "PTR NOK"))
                printf("Failed to process your request\n");
        }

        else if (!strcmp(command, "ql\n") || !strcmp(command, "question_list\n")) {
            freeQuestions(numQuestions, questions);
            numQuestions = getQuestionList(udp_fd, resUDP, addrlen, addr, 
                topicChosen, questions);
        }

        else if (!strcmp(command, "qg") || !strcmp(command, "question_get")) {
            command_type = !strcmp(command, "qg") ? 0 : 1;

            if ((command = strtok(NULL, " "))) {
                arg = strtok(command, "\n");
                
                if (strtok(NULL, " ") != NULL || arg == NULL)
                    printf("Invalid command\n");
                else {
                    if (command_type)
                        questionChosen = questionSelectName(
                            arg, numQuestions, questions);
                    else {
                        question = atoi(arg);
                        questionChosen = questionSelectNum(
                            question, numQuestions, questions);
                    }   
                    
                    if (questionChosen) {
                        sprintf(msg, "GQU %s %s\n", topicChosen, questionChosen);
                        SendMessageTCP(msg, &tcp_fd, &resTCP);
                        questionGet(topicChosen, questionChosen, tcp_fd);
                        close(tcp_fd);
                    }
                }
            }
        }

        else if (!strcmp(command, "qs") || !strcmp(command, "question_submit")) {
            command = strtok(NULL, "\n");
            questionChosen = strtok(command, " ");
            text_file = strtok(NULL, " ");
            img_file = strtok(NULL, "\n");
           
            if (questionChosen == NULL || text_file == NULL)
                printf("Invalid arguments\n");
            else submitQuestion(tcp_fd, &resTCP, *userId, topicChosen, 
                questionChosen, text_file, img_file);
        }

        else if ((!strcmp(command, "as") || !strcmp(command, "answer_submit"))) {
            answerPath = strtok(NULL, " ");
            answerImg = strtok(NULL, " ");
            answerSubmit(tcp_fd, &resTCP, *userId, topicChosen, questionChosen, 
                strtok(answerPath, "\n"), answerImg);
        }

        else 
            printf("Invalid command\n");
        printf("\n");
    }
}

int registerNewUser(int id, int fd, struct addrinfo *res, socklen_t addrlen, 
    struct sockaddr_in addr) {

    char *response, *message = malloc(sizeof(char) * REGISTER_SIZE);
    snprintf(message, REGISTER_SIZE, "REG %d\n", id);
    SendMessageUDP(message, fd, res);
    free(message);

    response = receiveMessageUDP(fd, addrlen, addr);
    if (!strcmp(response, "\0")) {
        /* Communication timed out */
        return 0;
    }

    !strcmp(response, "RGR OK")
    ? printf("User \"%d\" registered\n", id)
    : printf("Could not register user, invalid user ID\n");
    return !strcmp(response, "RGR OK");
}

void topicList(int fd, struct addrinfo *res, socklen_t addrlen, 
    struct sockaddr_in addr, char** topics, int* numTopics) {
    int i = 1, N, offset, user;
    char *iter, *ltr, *name, *sep;

    SendMessageUDP("LTP\n", fd, res);
    ltr = receiveMessageUDP(fd, addrlen, addr);

    if (!strcmp(ltr, "\0") || strcmp(strtok(ltr, " "), "LTR")) {
        /* Communication timed out */
        return;
    }

    N = atoi(strtok(NULL, " "));
    printf("Listing topics:\n");
    while (i <= N) {
        iter = strtok(NULL, " ");

        sep = strstr(iter, ":");
        offset = sep - iter;
        name = malloc(sizeof(char) * (offset + 1));

        memcpy(name, iter, offset);
        name[offset] = '\0';
        user = atoi(iter + offset + 1);

        if (topics[i - 1] != NULL)
            free(topics[i - 1]);
        topics[i - 1] = strdup(iter);
        printf("Topic %2d - %s (proposed by %d)\n", i++, name, user);

        free(name);
    }

    *numTopics = N;
}

void freeTopics(int numTopics, char **topics) {
    for (int i = 0; i < numTopics; i++)
        free(topics[i]);
    free(topics);
}

void freeQuestions(int numQuestions, char** questions) {
    for (int i = 0; i < numQuestions; i++) {
        free(questions[i]);
        questions[i] = NULL;
    }
}

char* topicSelectNum(int numTopics, char** topics, int topicChosen) {
    if (numTopics == -1) {
        printf("Topic list not available\n");
        return NULL;
    }
    else if (topicChosen > numTopics || topicChosen <= 0) {
        printf("Invalid topic number\n");
        return NULL;
    }

    char *topicInfo = strdup(topics[topicChosen - 1]);
    char *topic = strdup(strtok(topicInfo, ":"));
    char *userId = strdup(strtok(NULL, ":"));
    printf("Selecting topic: %s (%s)\n", topic, userId);
    free(topicInfo);
    free(userId);
    return topic;
}

char* topicSelectName(int numTopics, char** topics, char* name) {
    int i;
    char *topic = NULL,  *userId = NULL, *topicInfo, *nextTopic;
    
    if (numTopics == -1) {
        printf("Topic list not available\n");
        return NULL;
    }

    for (i = 0; i < numTopics; i++) {
        topicInfo = strdup(topics[i]);
        nextTopic = strdup(strtok(topicInfo, ":"));
        userId = strdup(strtok(NULL,":"));
        
        free(topicInfo);
        if (!strcmp(nextTopic, name)) {
            topic = nextTopic;
            break;
        }
        free(nextTopic);
        free(userId);
    }

    !topic 
    ? printf("Can't find that topic\n") 
    : printf("Selecting topic: %s (%s)\n", topic, userId);
    
    free(userId);
    return topic;
}

int getQuestionList(int fd, struct addrinfo *res, socklen_t addrlen, 
    struct sockaddr_in addr, char* topicChosen, char** questions) {

    int i = 0, lenMsg = LEN_COMMAND + 1 + LEN_TOPIC + 1 + 1, numQuestions = 0;
    char *iter, *message, *questionList, *response;

    if (!topicChosen) { printf("Select a topic first\n"); return -1; }

    message = malloc(sizeof(char) * lenMsg);
    snprintf(message, lenMsg, "LQU %s\n", topicChosen);
    SendMessageUDP(message, fd, res);
    free(message);

    questionList = receiveMessageUDP(fd, addrlen, addr);
    response = strdup(questionList);

    if (!strcmp(response, "\0")) {
        /* Communication timed out */
        free(response);
        return -1;
    }
    else if (!strcmp(response, "ERR")
        || !strcmp(strtok(response, " "), "LQR ")) {
        printf("Failed to process your request\n");
        free(response);
        return -1;
    }

    numQuestions = atoi(strtok(NULL, " \n"));
    if (!numQuestions) {
        printf("No available questions about %s\n", topicChosen);
        free(response);
        return -1;
    }

    printf("Listing questions for topic %s:\n", topicChosen);

    while ((iter = strtok(NULL, " \n")))
        questions[i++] = strdup(iter);
        
    for (i = 0; i < numQuestions; i++) {
        questions[i] = strtok(questions[i], ":");
        printf("Question %2d - %s\n", i + 1, questions[i]);
    }

    free(response);
    return numQuestions;
}

void submitQuestion(int fd, struct addrinfo **res, int aUserID, 
    char *topicChosen, char* question, char* text_file, char* img_file) {
    int request_len, sizeAux, nRead;
    long qsize, aisize;
    char *qdata, *textPath, *request, *aidata;
    FILE *questionFd, *imageFd;

    if (strlen(question) > 10) {
        printf("Question name is too long\n");
        return;
    }

    if (strlen(text_file) == 0) return;

    /* Get the question and its size */
    textPath = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(textPath, BUFFER_SIZE, "%s.txt", text_file);

    questionFd = fopen(textPath, "r");
    free(textPath);
    if (questionFd == NULL) {
        printf("The specified question file is unavailable\n");
        return;
    }

    if (img_file != NULL) {
        FILE *imageFd = fopen(img_file, "rb");
        if (imageFd == NULL) {
            printf("The specified image file is unavailable\n");
            return;
        }
        fclose(imageFd);
    }

    /* Get size of file */
    fseek(questionFd, 0L, SEEK_END);
    qsize = ftell(questionFd);
    fseek(questionFd, 0L, SEEK_SET);

    /* Send information */
    request_len = 3 + 1 + 5 + 1 + strlen(topicChosen) + 1 + strlen(question) + 1 + lengthInt(qsize) + 2;
    request = malloc(sizeof(char) * request_len);
    snprintf(request, request_len, "QUS %d %s %s %ld ", aUserID, topicChosen, question, qsize);
    DEBUG_PRINT("[QUS] Sending request: %s\n", request);
    SendMessageTCP(request, &fd, res);

    /* Send file data */
    qdata = (char*) malloc(sizeof(char) * BUFFER_SIZE);
    sizeAux = qsize;

    while (sizeAux > 0) {
        nRead = fread(qdata, 1 , BUFFER_SIZE, questionFd);
        if ((write(fd, qdata, nRead)) == -1) {
            printf("Error sending question file.\n");
            return;
        }
        sizeAux = sizeAux - BUFFER_SIZE;
    }

    fclose(questionFd);
    free(qdata);

    /* Get the question's image information */
    if (img_file != NULL) {
        if (!(imageFd = fopen(img_file, "rb"))) {
            printf("Can't find image file\n");
            free(request);
            return;
        }

        /* Get image size */
        fseek(imageFd, 0L, SEEK_END);
        aisize = ftell(imageFd);
        fseek(imageFd, 0L, SEEK_SET);

        char *aiext = strtok(img_file, ".");
        aiext = strtok(NULL, ".");

        /* Send image information */
        snprintf(request, BUFFER_SIZE, " 1 %s %ld ", aiext, aisize);
        if ((write(fd, request, strlen(request))) == -1) {
            printf("Error sending question information.\n");
            return;
        }

        /* Send image data */
        aidata = (char*) malloc(sizeof(char) * BUFFER_SIZE);
        int sizeAux = aisize;

        while (sizeAux > 0){
            int nRead = fread(aidata, 1 , BUFFER_SIZE,imageFd);
            if ((write(fd, aidata, nRead)) == -1) {
                printf("Error sending question information.\n");
                return;
            }
            sizeAux = sizeAux - BUFFER_SIZE;
        }

        if ((write(fd, "\n", strlen("\n"))) == -1) {
            printf("Error sending question information.\n");
            return;
        }
        fclose(imageFd);
        free(aidata);
    }
    else {
        if ((write(fd, " 0\n", 3)) == -1) {
            printf("Error sending question information.\n");
            return;
        }
    }
    free(request);

    char* reply = strtok(receiveMessageTCP(fd), "\n");
    DEBUG_PRINT("[QUS] Received reply: \"%s\"\n", reply);

    if (!strcmp(reply, "QUR OK")) printf("Question submitted with success\n");
    else if (!strcmp(reply, "QUR DUP")) printf("Unable to submit your question: the question already exists\n");
    else if (!strcmp(reply, "QUR FUL")) printf("Unable to submit your question: the list is full\n");
    else if (!strcmp(reply, "QUR NOK")) printf("Unable to submit your question\n");

    close(fd);
}

void answerSubmit(int fd, struct addrinfo **res, int aUserID, char *topicChosen, 
    char* questionChosen, char *text_file, char *img_file) {
    FILE *answerFd, *imageFd;
    char *answerPath = malloc(strlen(text_file) + strlen(".txt") + 1);
    
    sprintf(answerPath, "%s.txt", text_file);
    DEBUG_PRINT("[ANS] Destiny file path: %s\n", answerPath);
    
    /* Ensure specified answer path is valid */
    if (!(answerFd = fopen(answerPath, "r"))) {
        printf("The specified answer file is unavailable\n");
        free(answerPath);
        return;
    }
    free(answerPath);

    /* Ensure specified image path is valid, if applicable */
    if (img_file && !(imageFd = fopen(strtok(img_file, "\n"), "rb"))) {
        printf("The specified image file is unavailable\n");
        fclose(answerFd);
        return;
    }

    fseek(answerFd, 0L, SEEK_END);
    long asize = ftell(answerFd);
    fseek(answerFd, 0L, SEEK_SET);

    char *message = malloc(sizeof(char) * BUFFER_SIZE);
    snprintf(message, BUFFER_SIZE, "ANS %d %s %s %ld ", 
        aUserID, topicChosen, questionChosen, asize);
    SendMessageTCP(message, &fd, res);
    free(message);

    DEBUG_PRINT("[ANS] Answer properties:\n");
    DEBUG_PRINT("      - User ID: \"%d\"\n", aUserID);
    DEBUG_PRINT("      - Topic chosen: \"%s\"\n", topicChosen);
    DEBUG_PRINT("      - Question chosen: \"%s\"\n", questionChosen);
    DEBUG_PRINT("      - Answer size: \"%ld\"\n", asize);

    int nRead, sizeAux = asize;
    long isize = 0;
    char *iext, *idata, *adata = (char*) malloc(sizeof(char) * BUFFER_SIZE);

    while (sizeAux > 0) {
        nRead = fread(adata, 1 , BUFFER_SIZE, answerFd);
        if ((write(fd, adata, nRead)) == -1) {
            printf("Error sending answer information.\n");
            return;
        }
        sizeAux -= BUFFER_SIZE;
    }

    fclose(answerFd);

    if (img_file) {
        DEBUG_PRINT("[ANS] Image properties:\n");
        
        /* Get image size */
        fseek(imageFd, 0L, SEEK_END);
        isize = ftell(imageFd);
        fseek(imageFd, 0L, SEEK_SET);
        DEBUG_PRINT("      - Image size: \"%ld\"\n", isize);

        strtok(img_file, ".");
        iext = strtok(NULL, ".");
        DEBUG_PRINT("      - Image extention: \"%s\"\n", iext);

        /* Send image information */
        snprintf(adata, BUFFER_SIZE, " 1 %s %ld ", iext, isize);
        if ((write(fd, adata, strlen(adata))) == -1) {
            printf("Error sending answer information.\n");
            return;
        }

        /* Send image data */
        sizeAux = isize;
        idata = (char*) malloc(sizeof(char) * BUFFER_SIZE);

        DEBUG_PRINT("[ANS] Preparing to send image data\n");
        DEBUG_PRINT("[ANS] Writing image data (0%% completed)");
        while (sizeAux > 0) {
            nRead = fread(idata, 1, BUFFER_SIZE, imageFd);
            if ((write(fd, idata, nRead)) == -1) {
                printf("Error sending answer information.\n");
                return;
            }
            sizeAux = sizeAux - nRead;

            fflush(stdout);
            DEBUG_PRINT("\r[ANS] Writing image data (%.0f%% completed).", 
                (isize - sizeAux) * 1.0 / isize * 100);
        }
        
        DEBUG_PRINT("\n");
        if ((write(fd, "\n", 1)) == -1) {
            printf("Error sending answer information.\n");
            return;
        }
        fclose(imageFd);
        free(idata);
    }
    else {
        if ((write(fd, " 0\n", 3)) == -1) {
            printf("Error sending answer information.\n");
            return;
        }
    }

    DEBUG_PRINT("[ANS] Finished sending files\n");
    DEBUG_PRINT("[ANS] Preparing to receive server response\n");

    /* Parse reply */
    char* reply = strtok(receiveMessageTCP(fd), "\n");
    DEBUG_PRINT("[ANS] Received reply: \"%s\"\n", reply);

    if (!strcmp(reply, "ANR OK")) printf("Answer submitted with success\n");
    else if (!strcmp(reply, "ANR FUL")) printf("Unable to submit your answer: the list is full\n");
    else if (!strcmp(reply, "ANR NOK")) printf("Unable to submit your answer\n");

    close(fd);
    free(adata);
}

char* questionSelectNum(int question, int num_questions, char ** questions) {
    if (num_questions == -1) {
        printf("Run question list first\n");
        return NULL;
    }
    else if (question > num_questions || question <= 0) {
        printf("Invalid question number\n");
        return NULL;
    }

    return strtok(questions[question - 1], ":");
}

char* questionSelectName(char * name, int num_questions, char ** questions) {
    char * question = NULL;

    if (num_questions == -1){
        printf("Run question list first\n");
        return NULL;
    }

    for (int i = 0; i < num_questions; i++){
        char * nextQuestion = strtok(questions[i],":");
        if (!strcmp(nextQuestion, name)){
            question = nextQuestion;
            break;
        }
    }

    if (!question) printf("Can't find that question\n");
    return question;
}

void questionGet(char *topic, char *questionChosen, int fd) {
    int nMsg, qIMG, pathLen, dirLen, userId, offset = 0, N = 0, aIMG;
    long qsize = 0, qisize, asize, aisize;
    char qiext[3], aiext[3], *path, *directory, *AN, *reply;

    reply = calloc(BUFFER_SIZE + 1, sizeof(char));
    while ((nMsg = read(fd, reply, BUFFER_SIZE)) <= 0);

    if (!strcmp(reply, "QGR EOF\n") || !strcmp(reply, "QGR ERR\n")) {
        printf("Unable to retrieve this question\n");
        free(reply);
        return;
    }

    sscanf(reply, "%*s %*d %ld", &qsize);
    DEBUG_PRINT("[QG] Parsed reply: %s", reply);

    offset = 3 + 1 + 5 + 1 + lengthInt(qsize) + 1;
    pathLen = strlen("client/") + strlen(topic) + strlen("/") + strlen(questionChosen) + strlen(".txt") + 1;
    path = (char*) malloc(pathLen);
    sprintf(path, "client/%s/%s.txt", topic, questionChosen);

    dirLen = strlen("client/") + strlen(topic) + 1;
    directory = (char*) malloc(dirLen);
    sprintf(directory, "client/%s", topic);

    struct stat st = {0};
    if (stat(directory, &st) == -1)
        mkdir(directory, 0700);
    free(directory);

    // Write text file
    if (offset >= nMsg) {
        while ( (nMsg = read(fd, reply, BUFFER_SIZE))<= 0 );
        offset = 0;
    }

    if (recvTCPWriteFile(fd, path, &reply, &nMsg, BUFFER_SIZE, &offset, qsize, DEBUG_TEST) == -1)
        printf("Unable to write question file\n");
    if (offset == 0) offset++;

    if (offset != 0 && offset >= nMsg) {
        while ( (nMsg = read(fd, reply, BUFFER_SIZE))<= 0 );
        offset = 1;
    }

    if (vertifyIfSpace(reply, offset, "qIMG")) return;
    sscanf(reply + offset, "%d", &qIMG);
    if (qIMG != 0 && qIMG != 1) {
        printf("Incorrect format: image flag can only take the values 0 or 1.\n");
        return;
    }
    offset += 2;

    if (qIMG) {
        if (vertifyIfSpace(reply, offset, "qiext")) return;
        sscanf(reply + offset, "%s", qiext);
        offset += strlen(qiext) + 1;

        if (vertifyIfSpace(reply, offset, "qizise")) return;
        sscanf(reply + offset, "%ld", &qisize);

        sprintf(path, "client/%s/%s.%s", topic, questionChosen, qiext);
        offset += lengthInt(qisize) + 1;

        if (recvTCPWriteFile(fd, path, &reply, &nMsg, BUFFER_SIZE, &offset, qisize, DEBUG_TEST) == -1)
            printf("Unable to write question image file\n");
    if (offset == 0) offset++;
        if (offset == 0) offset++;
    }

    if (offset != 0 && offset >= nMsg) {
        while ( (nMsg = read(fd, reply, BUFFER_SIZE))<= 0 );
        offset = 1;
    }

    /* Get Number of Answers */
    if (vertifyIfSpace(reply, offset, "number of answers")) return;
    sscanf(reply + offset, "%d", &N);
    if (N > 10) {
        printf("Can only receive the maximum of 10 answers, not more.\n");
        return;
    }

    if (N > 0) offset += (lengthInt(N) + 1);
    else offset += 2;

    /* Print question */
    sprintf(path, "client/%s/%s.txt", topic, questionChosen);
    FILE * fp = fopen(path, "r");
    if (fp == NULL) {
        printf("Error opening question file.\n");
        return;
    }
    char * question = malloc(qsize + 1);
    if (!fread(question, 1, BUFFER_SIZE, fp)) {
        printf("Error reading question file.\n");
        return;
    }
    question[qsize] = '\0';
    printf("\nQ: %s\n", question);
    
    N
    ? printf("%d answers found for this question:\n", N)
    : printf("No answers found for this question\n");

    fclose(fp);
    free(path);
    free(question);

    /* Answers section */
    AN = (char*) malloc(3);
    for (int i = 0; i < N; i++) {
        if (vertifyIfSpace(reply, offset, "number of the answer")) return;
        sscanf(reply + offset, "%s", AN);
        offset += strlen(AN) + 1;
        if (strlen(AN) != 2) {
            printf("Incorrect format: Number of the answer can only have 2 digits.\n");
            return;
        }

        if (vertifyIfSpace(reply, offset, "user ID")) return;
        sscanf(reply + offset, "%d", &userId);
        offset += lengthInt(userId) + 1;
        if (lengthInt(userId) != 5) {
            printf("Incorrect format: User ID can only have 5 digits.\n");
            return;
        }

        if (vertifyIfSpace(reply, offset, "asize")) return;
        sscanf(reply + offset, "%ld", &asize);
        offset += lengthInt(asize) + 1;

        pathLen = strlen("client/") + strlen(topic) + strlen("/") + strlen(questionChosen) + strlen("_") + strlen(AN) + strlen(".txt") + 1;
        path = (char*) malloc(pathLen);
        sprintf(path, "client/%s/%s_%s.txt", topic, questionChosen, AN);
        
        if (recvTCPWriteFile(fd, path, &reply, &nMsg, BUFFER_SIZE, &offset, asize, DEBUG_TEST) == -1)
            printf("Unable to write answer file\n");
        if (offset == 0) offset++;

        if (offset != 0 && offset >= nMsg) {
            while ((nMsg = read(fd, reply, BUFFER_SIZE)) <= 0);
            offset = 1;
        }

        if (vertifyIfSpace(reply, offset, "aIMG")) return;
        sscanf(reply + offset, "%d", &aIMG);
        if (aIMG != 0 && aIMG != 1) {
            printf("Incorrect format: image flag can only take the values 0 or 1.\n");
            return;
        }
        offset += 2;

        if (aIMG) {
            if (vertifyIfSpace(reply, offset, "aiext")) return;
            sscanf(reply + offset, "%s", aiext);
            offset += strlen(aiext) + 1;

            if (vertifyIfSpace(reply, offset, "aisize")) return;
            sscanf(reply + offset, "%ld", &aisize);
            offset += lengthInt(aisize) + 1;

            DEBUG_PRINT("[ANS] Preparing to receive image (size: %ld, file extention: %s)\n", aisize, aiext);
            sprintf(path, "client/%s/%s_%s.%s", topic, questionChosen, AN, aiext);
            
            if (recvTCPWriteFile(fd, path, &reply, &nMsg, BUFFER_SIZE, &offset, aisize, DEBUG_TEST) == -1)
                printf("Unable to write answer image file\n");
            if (offset == 0) offset++;
        }

        else {
            if (offset != 0 && offset >= nMsg && (i + 1) < N) {
                while ((nMsg = read(fd, reply, BUFFER_SIZE)) <= 0)
                offset = 1;
            }
        }
        free(path);
    }
    free(AN);
    free(reply);
}
