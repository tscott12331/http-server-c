#include <fcntl.h>
#include <dirent.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "directory-parser.h"

#define REQ_QUEUE_LEN 30
#define MAX_REQUEST_SIZE 1024
#define POLL_TIMEOUT 1000
#define PORT 4444
#define IP_ADDR "127.0.0.1"
#define HTTP_HEAD "HTTP/1.0 "
#define RES_SUCCESS "200\n"
#define PNF_LEN 28
#define FNF_LEN 28
#define IR_LEN 36

bool setSocketBlocking(int fd, bool blocking); 
void sigActHandler(int s); 
void pollRead(int reqSockFd);
void readRequest(char* requestDest, int* bytesReadDest, int reqSockFd);
void handleRequest(int reqSockFd, Table* htmlTable);
int prepareSocket(const char* ipAddr, int port);
void serveRequests(int sockfd, Table* htmlTable);

int main(void) {
    Folder enclosingFolder = {
        .page = NULL,
        .item = {0},
    };
    Page* outerPage = initPage(".", &enclosingFolder, NULL);
    enclosingFolder.page = outerPage;

    generatePages(outerPage);
    Table* htmlTable = generateHtmlTable(outerPage);
    
    /*Page* curPage = outerPage;*/
    /*while(curPage != NULL) {*/
    /*    printf("\n== PAGE %s ==\n\n", curPage->name);*/
    /*    for(int i = 0; i < curPage->itemCount; i++) {*/
    /*        printf("%s", curPage->items[i].name);*/
    /*        if(curPage->items[i].type == DI_FOLDER) {*/
    /*            printf(" == FOLDER ==");*/
    /*        }*/
    /*        printf("\n");*/
    /*    }   */
    /*    curPage = curPage->nextPage;*/
    /*}*/
    int sockfd = prepareSocket(IP_ADDR, PORT);

    struct sigaction sigAct;
    sigAct.sa_handler = sigActHandler;
    sigemptyset(&sigAct.sa_mask);
    sigAct.sa_flags = SA_RESTART;

    // set sigaction
    if(sigaction(SIGCHLD, &sigAct, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    printf("listening on port %d\n", PORT);
    serveRequests(sockfd, htmlTable);

    close(sockfd);
    return 0;

}

void serveRequests(int sockfd, Table* htmlTable) {
    int incomingSockfd;
    struct sockaddr_in incomingAddr;
    socklen_t incomingAddrSize = (socklen_t)sizeof(incomingAddr);
    char* ipString = malloc(INET_ADDRSTRLEN * sizeof(char));

    while(true) {
        incomingSockfd = accept(sockfd,(struct sockaddr*)&incomingAddr, &incomingAddrSize);
        if(incomingSockfd == -1) {
            perror("accept");
            continue;
        }

        if(inet_ntop(incomingAddr.sin_family, &incomingAddr.sin_addr, ipString, INET_ADDRSTRLEN) == NULL) {
            free(ipString);
            perror("inet_ntop");
            close(sockfd);
            exit(1);
        }           
        
        printf("Connection from %s\n", ipString);

        int pid = fork();
        if(pid == 0) {
            close(sockfd);
            // child process
            handleRequest(incomingSockfd, htmlTable);
            close(incomingSockfd);
            exit(0);
        } else if(pid < 0) {
            printf("could not fork process\n");
        } 

        
        close(incomingSockfd);

    }
}

int prepareSocket(const char* ipAddr, int port) {
   int sockfd;
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
       perror("socket");
       exit(1);
   }

    struct sockaddr_in localAddr = {0};
    localAddr.sin_port = htons(port);
    localAddr.sin_family = AF_INET;
    inet_aton(ipAddr, &localAddr.sin_addr);

    if((bind(sockfd, (struct sockaddr*)&localAddr, sizeof(localAddr))) == -1 ) {
        perror("bind");
        exit(1);
    }
   
    if((listen(sockfd, REQ_QUEUE_LEN)) == -1) {
        perror("listen");
        exit(1);
    }

    return sockfd;
}

char* getTablePathFromReq(char* path) {
    int len = strlen(path);
    if(len == 1) {
        if(strcmp(path, "/") != 0) {
            return NULL;
        }
        return ".";
    } else if(len > 1) {
        char* p = (char*) malloc((len + 2) * sizeof(char));
        strcpy(p, ".");
        strcat(p, path);
        int i = len;
        while(p[i] == '/') {
            p[i] = '\0';
            i--;
        }
        return p;
    } else {
        return NULL;
    }
}

char* pageNotFound() {
   char* res = (char*) malloc((PNF_LEN + 1) * sizeof(char));
   strcpy(res, "HTTP/1.0 404\n\nPage not found");
   return res;
}

char* fileNotFound() {
   char* res = (char*) malloc((FNF_LEN + 1) * sizeof(char));
   strcpy(res, "HTTP/1.0 404\n\nFile not found");
   return res;
}

char* invalidReqMethod() {
   char* res = (char*) malloc((IR_LEN + 1) * sizeof(char));
   strcpy(res, "HTTP/1.0 400\n\nInvalid request method");
   return res;
}

char* prepareResponse(char* method, char* path, Table* htmlTable) {

    if(!(strcmp(method, "GET") == 0)) {
        return invalidReqMethod(); 
    }

    char* tablePath = getTablePathFromReq(path);
    if(tablePath == NULL) {
        return pageNotFound();
    }

    char* html = tableGet(htmlTable, tablePath);
    if(html == NULL) {
        return pageNotFound(); 
    }
    
    int pathLen = strlen(tablePath);
    
    int htmlLen = (int) strlen(html);
    if(htmlLen == 1 && memcmp(html, "!", 1) == 0) {
        // look for file
        
        FILE* file = fopen(tablePath, "rb");
        if(file == NULL) {
            return fileNotFound();
        }
        fseek(file, 0L, SEEK_END);
        size_t fileSize = ftell(file);
        
        char* buffer = (char*) malloc((fileSize + 1) * sizeof(char));
        
        rewind(file);
        
        fread(buffer, sizeof(char), fileSize, file);
        buffer[fileSize] = '\0';
       
        if(pathLen > 1) {
            free(tablePath);
        }
        return buffer; 
    }

    if(pathLen > 1) {
        free(tablePath);
    }

    char* response = (char*)malloc(sizeof(HTTP_HEAD) + sizeof(RES_SUCCESS) + htmlLen + 1);
    strcpy(response, HTTP_HEAD);
    strcat(response, RES_SUCCESS);
    strcat(response, "\n");
    strcat(response, html);
    return response;
}

void handleRequest(int reqSockFd, Table* htmlTable) {
        pollRead(reqSockFd);
        
        char* incomingBuffer = (char*) malloc(MAX_REQUEST_SIZE * sizeof(char) + 1);
        int incomingBytesRead;
        
        readRequest(incomingBuffer, &incomingBytesRead, reqSockFd);
        
        char* method = strtok(incomingBuffer, " ");
        char* path = strtok(NULL, " ");
        
        if(method == NULL || path == NULL) {
            printf("Invalid request\n");
            free(incomingBuffer);
            return;
        }

        printf("Method: %s\n", method);
        printf("Path: %s\n", path);
        
        char* response = prepareResponse(method, path, htmlTable); 

        if((send(reqSockFd, response, strlen(response), 0)) == -1) {
            free(incomingBuffer);
            free(response);
            perror("send");
            close(reqSockFd);
            exit(1);
        }
        printf("successfully sent response\n");
        free(incomingBuffer);
        free(response);
}

void readRequest(char* requestDest, int* bytesReadDest, int reqSockFd) {
            // read up to max length request
            *bytesReadDest = read(reqSockFd, requestDest, MAX_REQUEST_SIZE);
            if(*bytesReadDest < 0) {
                // error
                perror("read");
                free(requestDest);
                close(reqSockFd);
                exit(1);
            }  
            

            requestDest[*bytesReadDest] = '\0';
}

void pollRead(int reqSockFd) {
            struct pollfd pollObj = {
                .events = POLLIN,
                .fd = reqSockFd
            };
            pollObj.fd = reqSockFd;
            int pollResult = poll(&pollObj, 1, POLL_TIMEOUT);
            if(pollResult < 0) {
                perror("poll");
                close(reqSockFd);
                exit(1);
            } else if(pollResult == 0) {
                printf("request timed out\n");
                close(reqSockFd);
                exit(1);
            }
}

void sigActHandler(int s) {
    // errno may get overriden by waitpid
    int prevErrno = errno;

    // wno hang - return immediately if no child exited
    while(waitpid(-1, NULL, WNOHANG) > 0); 
    errno = prevErrno;
}

bool setSocketBlocking(int fd, bool blocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0);
}
