/* =============================================================================
 *
 * utility.h
 * -- Collection of utility functions with general purpose
 *
 * =============================================================================
 */

#ifndef UTILITY_H
#define UTILITY_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define ID_SIZE 5

#define DEBUG_PRINT(fmt, args...) \
    do { if (DEBUG_TEST) fprintf(stderr, fmt, ##args); } while (0)

int recvTCPWriteFile(int fd, char *filePath, char **bufferAux, int *sizeMsg,
    int bufferSize, int *offset, int size, int DEBUG_TEST);

int lengthInt(int x);

#endif /* UTILITY_H */
