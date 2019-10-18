/* =============================================================================
 *
 * commandlinereader.c
 *
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void parseArguments(int number, int *DEBUG_TEST, char **arguments, char **port, char **ip) {
    int opt;

    while((opt = getopt(number, arguments, "n:p:d")) != -1) {
        switch(opt) {
            case 'n':
                if (optarg == NULL || strcmp(optarg, "-p") == 0) {
                    printf("The IP is missing\n");
                    exit(1);
                }

                free(*ip); /* Discard default ip previously saved */
                *ip = strdup(optarg);
                break;
            
            case 'p':
                if (optarg == NULL || strcmp(optarg, "-n") == 0) {
                    printf("The port is missing\n");
                    exit(1);
                }

                free(*port); /* Discard default port previously saved */
                *port = strdup(optarg);
                break;
            
            case 'd':
                *DEBUG_TEST = 1;
        }
    }
}
