#ifndef SERVER_H
#define SERVER_H

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
#define DEBUG_TEST 0
#define DISPLAY_ANSWERS 10
#define MAX_ANSWERS 99
#define MAX_TOPICS 99
#define QUESTIONS_DESC "_desc"
#define QUESTIONS_LIST "/_questions.txt"
#define TOPIC_FOLDER "topics/"
#define TOPIC_LIST "topics/List_of_Topics.txt"
#define TOPICNAME_SIZE 10

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

#endif /* SERVER_H */