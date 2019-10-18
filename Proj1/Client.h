#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "lib/commandlinereader.h"
#include "lib/utility.h"

#define LEN_COMMAND 3
#define LEN_TOPIC 10
#define NUM_QUESTIONS 99
#define NUM_TOPICS 99
#define REGISTER_SIZE 12

void connectToServer(int *udp_fd, int *tcp_fd, char *ip, char *port, struct addrinfo hints, struct addrinfo **resUDP, struct addrinfo **resTCP);
void SendMessageUDP(char *message, int fd, struct addrinfo *res);
char* receiveMessageUDP(int fd, socklen_t addrlen, struct sockaddr_in addr);
void SendMessageTCP(char *message, int *fd, struct addrinfo **res);
char* receiveMessageTCP(int fd);
void parseCommands(int *userId, int udp_fd, int tcp_fd, struct addrinfo *resUDP, struct addrinfo *resTCP, socklen_t addrlen, struct sockaddr_in addr);
int registerNewUser(int id, int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr);
void topicList(int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr, char **topics, int *numTopics);
void freeTopics(int numTopics, char **topics);
char* topicSelectNum(int numTopics, char **topics, int topicChosen);
char* topicSelectName(int numTopics, char **topics, char *name);
int getQuestionList(int fd, struct addrinfo *res, socklen_t addrlen, struct sockaddr_in addr, char *topicChosen, char **questions);
void freeQuestions(int numQuestions, char **questions);
void answerSubmit(int fd, struct addrinfo **res, int aUserID, char *topicChosen, char *questionChosen, char *text_file, char *img_file);
char* questionSelectNum(int question, int num_questions, char **questions);
void questionGet(char *topic, char *questionChosen, int fd);
char* questionSelectName(char *name, int num_questions, char **questions);
void submitQuestion(int fd, struct addrinfo **res, int aUserID, char *topicChosen, char *question, char *text_file, char *img_file);
void handleTimeout(int sig);

#endif /* CLIENT_H */