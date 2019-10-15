/* =============================================================================
 *
 * utility.c
 * -- Collection of utility functions and macros with general purpose
 *
 * =============================================================================
 */

#include "utility.h"

int recvTCPWriteFile(int fd, char *filePath, char **bufferAux, int *sizeMsg,
    int bufferSize, int *offset, int size, int DEBUG_TEST) {

    int sizeAux, i = 0;
    float percentage = 0.0;
    char *buffer = (char*) malloc(sizeof(char) * bufferSize);
    ssize_t nMsg = 0;
    FILE* fp;

    if (!(fp = fopen(filePath, "wb"))) return -1;

    /* Trace Logs */
    DEBUG_PRINT("[RCVTCP] Message size: \"%d\"\n", *sizeMsg);
    DEBUG_PRINT("[RCVTCP] Offset: \"%d\"\n", *offset);
    DEBUG_PRINT("[RCVTCP] Size: \"%d\"\n", size);
    
    int toWrite = size;
    if (toWrite <= (*sizeMsg - *offset)) {
        fwrite(*bufferAux+*offset, sizeof(char), toWrite, fp);
        printf("Copying file to %s (%d%% completed)", filePath, toWrite / size * 100);
        *offset = *offset + toWrite + 1;
        toWrite = 0;
    }
    else if (*offset < *sizeMsg) {
        fwrite(*bufferAux+*offset, sizeof(char), *sizeMsg-*offset, fp);
        printf("Copying file to %s (%d%% completed)", filePath, (*sizeMsg-*offset) / size * 100);
        toWrite = toWrite - (*sizeMsg-*offset);
    }

    /* Receive message if needed */    
    int sizeToRead = toWrite > BUFFER_SIZE ? BUFFER_SIZE : toWrite;

    while (toWrite > 0 && (nMsg = read(fd, buffer, 1)) > 0) {
        fflush(stdout);
        
        sizeAux = toWrite > nMsg? nMsg : toWrite;
        fwrite(buffer, 1, sizeAux, fp);
        percentage = (size - toWrite) * 1.0 / size * 100;

        printf("\rCopying file to %s (%.0f%% completed)", filePath, percentage);
        
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
