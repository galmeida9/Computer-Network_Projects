/* =============================================================================
 *
 * utility.c
 * -- Collection of utility functions with general purpose
 *
 * =============================================================================
 */

#include "utility.h"

 #define BUFFER_SIZE 2048

int recvTCPWriteFile(int fd, char *filePath, char **bufferAux, int *sizeMsg,
    int bufferSize, long *offset, int size) {

    char *buffer = (char*) malloc(sizeof(char)*bufferSize);
    //Open file
    FILE* fp = fopen(filePath, "wb");
    if (fp == NULL) return -1;

    int toWrite = size;

    if (toWrite <= (*sizeMsg - *offset)) {
        fwrite(*bufferAux+*offset, sizeof(char), toWrite, fp);
        printf("%s Copying file %d%%", filePath, toWrite / size * 100);
        *offset = *offset + toWrite + 1;
        toWrite = 0;
    }
    else if (*offset < (*sizeMsg)){
        fwrite(*bufferAux+*offset, sizeof(char), *sizeMsg-*offset, fp);
        printf("%s Copying file %ld%%", filePath, (*sizeMsg-*offset) / size * 100);
        toWrite = toWrite - (*sizeMsg-*offset);
    }

    //Receive message if needed
    ssize_t nMsg = 0;
    int i=0;
    float percentage = 0.0;
    int sizeToRead = toWrite > BUFFER_SIZE ? BUFFER_SIZE : toWrite;

    while (toWrite > 0 && (nMsg = read(fd, buffer, 1))>0){

        fflush(stdout);
        int sizeAux = toWrite > nMsg? nMsg : toWrite;
        fwrite(buffer, 1, sizeAux, fp);
        percentage = (size - toWrite) * 1.0 / size * 100;

        printf("\r%s Copying file %.0f%%", filePath, percentage);
        toWrite = toWrite - sizeAux;
        sizeToRead = toWrite > BUFFER_SIZE ? BUFFER_SIZE : toWrite;
        if (toWrite <= 0) {
            nMsg = read(fd, buffer, bufferSize);
            *offset = 0;
            *sizeMsg = nMsg;
            break;
        }
        memset(buffer, 0, sizeof(buffer));
        *offset = 0;
    }

    //Close file and return
    fclose(fp);
    memcpy(*bufferAux, buffer, nMsg);
    free(buffer);
    printf("\n");
    return 0;
}
