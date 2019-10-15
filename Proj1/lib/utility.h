/* =============================================================================
 *
 * utility.h
 * -- Collection of utility functions with general purpose
 *
 * =============================================================================
 */

#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int recvTCPWriteFile(int fd, char *filePath, char **bufferAux, int *sizeMsg,
    int bufferSize, long *offset, int size);

#endif /* UTILITY_H */
