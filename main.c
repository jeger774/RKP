#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "ileuxt.h"

int main(int argc, char* argv[]){
    if (argc >= 2) {
        if (argv[1][0] == '-' && argv[1][1] == '-') {
            if (argv[1][2] == 'v') {
                printf("\nProgram name: %s\nVersion: 1.00\nAuthor:Peter\nCurrent build date:%s %s\n", argv[0], __DATE__, __TIME__);
            } else if (argv[1][2] == 'h' || argv[1][2] == '?') {
                printf("\nPossible arguments:\n--? --help --version filename.xy\n");
            } else {
                printf("\nInvalid argument. Use --help or --? for possible arguments list.\n");
            }
        } else {
            int fd = open(argv[1], O_RDONLY);
            signal(SIGALRM,WhatToDo);
            alarm(1);
            if (fd < 0) {
                fprintf(stderr,RED_TEXT("Failed to open file.\n"));
                return(1);
            }
            Decode(fd);
        }
    } else {
        int fd = BrowseForOpen();
        signal(SIGALRM,WhatToDo);
        alarm(1);
        if (fd < 0) {
            fprintf(stderr,RED_TEXT("Failed to open file.\n"));
            return(1);
        }
        Decode(fd);
    }
    return 0;
}
