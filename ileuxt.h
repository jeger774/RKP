#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <omp.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#ifndef _ileuxt_h
#define _ileuxt_h

#define CYAN_TEXT(x) "\033[36;1m" x "\033[0m"
#define RED_TEXT(x) "\033[31;1m" x "\033[0m"
#define GREEN_TEXT(x) "\033[32;1m" x "\033[0m"

#define BUFSIZE 1024                 // Max length of buffer
#define PORT_NO 80                  // The port on which the server is listening
#define ADDRESS "193.6.135.162"    // Target address
int Post(char* neptunID, char* message, int numCh)
{
    int s;                            // socket ID
    int flag;                         // transmission flag
    int bytes;                        // received/sent bytes
    int err;                          // error code
    unsigned int server_size;         // length of the sockaddr_in server
    char on;                          // sockopt option
    char buffer[BUFSIZE];             // datagram buffer area
    struct sockaddr_in server;        // address of server
    /********************** Initialization **********************/
    on   = 1;
    flag = 0;
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = inet_addr(ADDRESS);
    server.sin_port        = htons(PORT_NO);
    server_size = sizeof server;
    /********************** Creating socket **********************/
    s = socket(AF_INET, SOCK_STREAM, 0 );
    if ( s < 0 ) {
        fprintf(stderr, RED_TEXT(" %s: Socket creation error.\n"), ADDRESS);
        exit(2);
    }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);
    /********************** Connecting ***************************/
    err = connect( s, (struct sockaddr *) &server, server_size);
    if ( err < 0 ) {
        fprintf(stderr, RED_TEXT(" %s: Connecting error.\n"), ADDRESS);
        exit(3);
    }
    /********************** Sending data *************************/
    char body[27 + numCh + 1];
    sprintf(body, "NeptunID=%s&PostedText=%s", neptunID, message);
    int contentLength = strlen(body);

    char header[1024] = {0};

    sprintf(header, "POST /~vargai/post.php HTTP/1.1\r\nHost: irh.inf.unideb.hu\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n", contentLength);
    int headerSize = strlen(header);

    int requestSize = headerSize + contentLength;
    char* request = (char*)malloc(requestSize * sizeof(char));
    strcpy(request, header);
    strcpy(request + headerSize, body);

    bytes = send(s, request, requestSize, flag);
    free(request);

    if ( bytes <= 0 ) {
        fprintf(stderr, " %s: Sending error.\n", ADDRESS);
        exit(4);
    }
    printf (CYAN_TEXT(" %i bytes have been sent to server.\n"), bytes-1);
    /************************ Receive data *************************/
    bytes = recv(s, buffer, BUFSIZE, flag);
    if ( bytes < 0 ) {
        fprintf(stderr, RED_TEXT(" %s: Receiving error.\n"), ADDRESS);
        exit(5);
    }
    const char* ack = "The message has been received.";
    /************************ Closing *****************************/
    close(s);
    if (strstr(buffer, ack) == NULL){
        return 1;
    }
    return 0;
}

void WhatToDo(int sig){
    switch(sig) {
        case SIGALRM:
            fprintf(stderr, RED_TEXT("\nTimed out.\n"));
            exit(1);
        case SIGINT:
            if(fork()==0) {
                fprintf(stderr,RED_TEXT("\nCtrl+C is not available while unwrapping is in progress.\n"));
                kill(getpid(),SIGKILL);
            }
            break;
        default:
            break;
    }
}

char* Unwrap(char *Pbuff, int numCh) {
    signal(SIGINT,WhatToDo);
    unsigned char *out = (unsigned char *) malloc(numCh);
    if (!out) {
        free(out);
        fprintf(stderr, RED_TEXT("Memory allocation failed.\n"));
        exit(1);
    }

#pragma omp parallel for schedule(static)
    for (int i = 0; i < numCh * 3; i += 3) {
        unsigned char first = (Pbuff[i] & 3) << 6;
        unsigned char second = (Pbuff[i + 1] & 7) << 3;
        unsigned char third = (Pbuff[i + 2] & 7);

        unsigned char final = first | second | third;

#pragma omp critical
        out[i/3] = final;
    }
    free(Pbuff);
    return out;
}

char* ReadPixels (int f, int* numCh) {
    unsigned char fSizeArray[4];
    lseek(f, 3L, SEEK_SET);
    read(f, fSizeArray, 4);
    uint32_t fileSize = (fSizeArray[3] << 24) | (fSizeArray[2] << 16) | (fSizeArray[1] << 8) | (fSizeArray[0]);

    unsigned char toReadArray[4];
    lseek(f, 6L, SEEK_SET);
    read(f, toReadArray, 4);
    uint32_t toRead = (toReadArray[3] << 24) | (toReadArray[2] << 16) | (toReadArray[1] << 8) | (toReadArray[0]);
    *numCh = (int)toRead;

    unsigned char startArray[4];
    read(f, startArray, 4);
    uint32_t start = (startArray[3] << 24) | (startArray[2] << 16) | (startArray[1] << 8) | (startArray[0]);

    int arraySize = fileSize - start;
    char* buffer = (char*)malloc(arraySize);
    if (!buffer) {
        free(buffer);
        fprintf(stderr, RED_TEXT("Memory allocation failed.\n"));
        exit(1);
    }

    lseek(f, (long int)start, SEEK_SET);
    read(f, buffer, toRead*3);

    close(f);
    return buffer;
}

int BrowseForOpen() {
    DIR* d;
    struct dirent *dir;
    char input[256];
    chdir(getenv("HOME"));

    while (1) {
        d = opendir(".");
        if (d) {
            while((dir = readdir(d)) != NULL) {
                printf("%s\n", dir->d_name);
            }
        }

        printf(GREEN_TEXT("Choose one:"));
        scanf("%s", input);

        d = opendir(".");
        if (d) {
            int isValid=0;
            while((dir = readdir(d)) != NULL) {
                if (strcmp(dir->d_name, input) == 0) {
                    if (dir->d_type == 4) {
                        chdir(dir->d_name);
                        isValid=1;
                    } else if (dir->d_type == 8) {
                        return open(dir->d_name, O_RDONLY);
                    }
                    break;
                }
            }
            if (!isValid){
                fprintf(stderr,RED_TEXT("No such file or directory.\n"));
            }
            closedir(d);
        }
    }
}

void Decode(int fd) {
    int size;
    char* pixels = ReadPixels(fd, &size);
    char* codedData = Unwrap(pixels, size);
    alarm(0);
    signal(SIGINT,SIG_DFL);
    Post("rkp", codedData, size);
    free(codedData);
}

#endif