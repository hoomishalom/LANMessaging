#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

#define maxCmdLen 256
#define maxDataLen 2048
#define maxMessageLen maxCmdLen + maxDataLen

#define maxMessageQueued 256

extern int errono;

int clientSock; 

const int PORT = 5678;
const char* ADDR = "127.0.0.1";

const char* DATA_DELIMITER = "~";
const char* ARGS_DELIMITER = "|";

int option = 1;

int clientSock;
struct sockaddr_in serverAddr;

typedef struct {
    char cmd[maxCmdLen];
    char data[maxDataLen];
} readMessageStruct;

typedef struct {
    int destination;
    char cmd[maxCmdLen];
    char data[maxDataLen];
} sendMessageStruct;

sendMessageStruct messagesToSend[maxMessageQueued];

// functions initialization
readMessageStruct parseReadMessage(char message[maxMessageLen]);
sendMessageStruct encodeSendMessage(int destination, char *cmd, char *data);
int createAndConnectSocket();

int isSocketConnected(int sockfd) { 
    int error; 
    socklen_t len = sizeof(error); 
    int ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len); 
     
    if (ret == 0 && error == 0) { 
        // Socket is connected 
        return 1; 
    } else { 
        // Socket is not connected 
        return 0; 
    } 
} 

readMessageStruct parseReadMessage(char message[maxMessageLen]) {
    readMessageStruct messageObj;

    strcpy(messageObj.cmd, strtok(message, ARGS_DELIMITER));
    strcpy(messageObj.data, strtok(message, ARGS_DELIMITER));

    return messageObj;
}

sendMessageStruct encodeSendMessage(int destination, char *cmd, char *data)
{
    sendMessageStruct messageObj;

    messageObj.destination = destination;
    strcpy(messageObj.cmd, cmd);
    strcpy(messageObj.data, data);

    return messageObj;
}

int createAndConnectSocket()
{
    if ((clientSock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "createAndConnectSocket - socket() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if ((setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))) == -1)
    {
        fprintf(stderr, "createAndConnectSocket - setsockopt() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("%d\n", isSocketConnected(clientSock));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if ((inet_pton(AF_INET, ADDR, &serverAddr.sin_addr)) <= 0)
    {
        fprintf(stderr, "createAndConnectSocket - inet_pton() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if ((connect(clientSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) == -1)
    {
        fprintf(stderr, "createAndConnectSocket - connect() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const* argv[])
{  
    createAndConnectSocket();

    printf("connected to server\n");
    while(true){
        sleep(2);
        char test[maxMessageLen] = "test|this is a test";
        printf("send: %d\n", send(clientSock, test, strlen(test), 0));
    }

    close(clientSock);
}