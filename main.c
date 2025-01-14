#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define REQ_QUEUE_LEN 30
#define MAX_REQUEST_SIZE 1024
#define POLL_TIMEOUT 1000
bool setSocketBlocking(int fd, bool blocking); 

int main() {
   int sockfd;
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
       perror("socket");
       exit(1);
   }
    struct sockaddr_in localAddr = {0};
    localAddr.sin_port = htons(4444);
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

    int incomingSockfd;
    struct sockaddr_in incomingAddr;
    socklen_t incomingAddrSize = (socklen_t)sizeof(incomingAddr);
    char* ipString = malloc(INET_ADDRSTRLEN * sizeof(char));
    char* response = "HTTP/1.0 200 \n\nHello World!";
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
            // child process
            if(!setSocketBlocking(incomingSockfd, false)) {
                printf("failed to set socket to nonblocking\n");
                close(sockfd);
                exit(1);
            }  
            
            /*struct pollfd pollObj = {*/
            /*    .fd = incomingSockfd,*/
            /*    .events = POLLIN,*/
            /*};*/
            /*printf("polling socket input ready...\n");  */
            /*if(poll(&pollObj, 1, POLL_TIMEOUT) <= 0) {*/
            /*    printf("cannot read data from socket");*/
            /*    close(sockfd);*/
            /*    exit(0);*/
            /*}*/

            /*printf("socket ready to read from\n");*/

            printf("preparing to read incoming bytes...\n");
            char* incomingBuffer = (char*) malloc(MAX_REQUEST_SIZE * sizeof(char) + 1);
            int incomingBytesRead;
            printf("allocated space for buffer...\n");

            if((incomingBytesRead = read(incomingSockfd, incomingBuffer, MAX_REQUEST_SIZE)) < 0) {
                free(incomingBuffer);
                perror("read");
                exit(1);
            } 
            
            incomingBuffer[incomingBytesRead] = '\0';
            printf("Request: \n%s\n", incomingBuffer);

            if((send(incomingSockfd, response, strlen(response), 0)) == -1) {
                free(incomingBuffer);
                perror("send");
                close(sockfd);
                exit(1);
            }

            free(incomingBuffer);
            close(sockfd);
            exit(0);
        } else if(pid < 0) {
            printf("could not fork process\n");
        } 

        
        close(incomingSockfd);

    }

    close(sockfd);
    return 0;

}

bool setSocketBlocking(int fd, bool blocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0);
}
