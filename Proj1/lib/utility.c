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

    int sizeAux;
    float percentage = 0.0;
    char *buffer;
    ssize_t nMsg = 0;
    FILE* fp;

    if (!(fp = fopen(filePath, "wb"))) return -1;
    buffer = (char*) malloc(sizeof(char) * bufferSize);

    DEBUG_PRINT("\n");
    DEBUG_PRINT("[RCVTCP] Message size: \"%d\"\n", *sizeMsg);
    DEBUG_PRINT("[RCVTCP] Offset: \"%d\"\n", *offset);
    DEBUG_PRINT("[RCVTCP] Size: \"%d\"\n", size);
    
    int toWrite = size;
    if (toWrite <= (*sizeMsg - *offset)) {
        /* Case #1: data completely fit the buffer. */
        fwrite(*bufferAux+*offset, sizeof(char), toWrite, fp);
        printf("Copying file to %s (%d%% completed)", 
            filePath, toWrite / size * 100);
        *offset = *offset + toWrite + 1;
        toWrite = 0;
    }
    else if (*offset < *sizeMsg) {
        /* Case #2: the buffer didn't accommodate the full data,
         * -------  there's still data to be read. */
        fwrite(*bufferAux+*offset, sizeof(char), *sizeMsg-*offset, fp);
        printf("Copying file to %s (%d%% completed)", 
            filePath, (*sizeMsg-*offset) / size * 100);
        toWrite = toWrite - (*sizeMsg-*offset);
    }

    /* Receive the remaining portion of the data, if needed. */
    while (toWrite > 0 && (nMsg = read(fd, buffer, 1)) > 0) {
        fflush(stdout);
        
        sizeAux = toWrite > nMsg? nMsg : toWrite;
        fwrite(buffer, 1, sizeAux, fp);
        percentage = (size - toWrite) * 1.0 / size * 100;

        printf("\rCopying file to %s (%.0f%% completed)", filePath, percentage);
        
        toWrite = toWrite - sizeAux;
        if (toWrite <= 0) {
            nMsg = read(fd, buffer, bufferSize);
            *offset = 0;
            *sizeMsg = nMsg;
            break;
        }
        memset(buffer, 0, sizeof(*buffer));
        *offset = 0;
    }
    printf("\n");

    /* Close file and return */
    fclose(fp);
    memcpy(*bufferAux, buffer, nMsg);
    free(buffer);
    return 0;
}

int lengthInt(int x){
    return floor(log10(abs(x))) + 1;
}