#include <fcntl.h>
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

#define REQ_QUEUE_LEN 30
#define MAX_REQUEST_SIZE 1024
#define POLL_TIMEOUT 1000
#define PORT 4444

bool setSocketBlocking(int fd, bool blocking); 
void sigActHandler(int s); 

int main() {
   int sockfd;
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
       perror("socket");
       exit(1);
   }
    struct sockaddr_in localAddr = {0};
    localAddr.sin_port = htons(PORT);
    localAddr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &localAddr.sin_addr);
    if((bind(sockfd, (struct sockaddr*)&localAddr, sizeof(localAddr))) == -1 ) {
        perror("bind");
        exit(1);
    }
   
    if((listen(sockfd, REQ_QUEUE_LEN)) == -1) {
        perror("listen");
        exit(1);
    }

    struct sigaction sigAct;
    sigAct.sa_handler = sigActHandler;
    sigemptyset(&sigAct.sa_mask);
    sigAct.sa_flags = SA_RESTART;

    // set sigaction
    if(sigaction(SIGCHLD, &sigAct, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    // prepare polling struct

    struct pollfd pollObj = {
        .events = POLLIN
    };
    int pollResult;

    int incomingSockfd;
    struct sockaddr_in incomingAddr;
    socklen_t incomingAddrSize = (socklen_t)sizeof(incomingAddr);
    char* ipString = malloc(INET_ADDRSTRLEN * sizeof(char));
    char* response = "HTTP/1.0 200 \n\nHello World!";
    printf("listening on port %d\n", PORT);
    while(true) {
        incomingSockfd = accept(sockfd,(struct sockaddr*)&incomingAddr, &incomingAddrSize);
        if(incomingSockfd == -1) {
            perror("accept");
            continue;
        }

        if(inet_ntop(incomingAddr.sin_family, &incomingAddr.sin_addr, ipString, INET_ADDRSTRLEN) == NULL) {
            perror("inet_ntop");
            close(sockfd);
            exit(1);
        }           
        printf("Connection from %s\n", ipString);
        int pid = fork();
        if(pid == 0) {
            close(sockfd);
            // child process
            pollObj.fd = incomingSockfd;
            pollResult = poll(&pollObj, 1, POLL_TIMEOUT);
            if(pollResult < 0) {
                perror("poll");
                close(incomingSockfd);
                exit(1);
            } else if(pollResult == 0) {
                printf("request timed out\n");
                close(incomingSockfd);
                exit(1);
            }

            printf("preparing to read incoming bytes...\n");
            char* incomingBuffer = (char*) malloc(MAX_REQUEST_SIZE * sizeof(char) + 1);
            int incomingBytesRead;
            printf("allocated space for buffer...\n");
           
            // read up to max length request
            incomingBytesRead = read(incomingSockfd, incomingBuffer, MAX_REQUEST_SIZE);
            if(incomingBytesRead < 0) {
                // error
                perror("read");
                free(incomingBuffer);
                close(incomingSockfd);
                exit(1);
            }  
            
            printf("read request\n");

            incomingBuffer[incomingBytesRead] = '\0';
            printf("Request: \n%s\n", incomingBuffer);

            if((send(incomingSockfd, response, strlen(response), 0)) == -1) {
                free(incomingBuffer);
                perror("send");
                close(incomingSockfd);
                exit(1);
            }
            printf("successfully sent response\n");
            free(incomingBuffer);
            close(incomingSockfd);
            exit(0);
        } else if(pid < 0) {
            printf("could not fork process\n");
        } 

        
        close(incomingSockfd);

    }

    close(sockfd);
    return 0;

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
