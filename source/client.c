#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define stdlog stdout   // logging messages go to "stdlog"
#define stddbg stdout   // debugging messages go to "stdlog"

#define maxCmdLen 256
#define maxDataLen 2048
#define maxMessageLen maxCmdLen + maxDataLen

#define maxMessageQueued 256

#define maxNameLen 32
#define maxDescriptionLen 256

extern int errono;

int clientSock; 

const int PORT = 5678;
const char* ADDR = "127.0.0.1";

const char* DATA_DELIMITER = "~";
const char* ARGS_DELIMITER = "|";

fd_set connectedFileDescriptors;
fd_set readyToReadFileDescriptors;
fd_set readyToWriteFileDescriptors;
fd_set errorFileDescriptors;

int option = 1;

int clientSock;
struct sockaddr_in serverAddr;

typedef struct {
    char cmd[maxCmdLen];
    char data[maxDataLen];
} readMessageStruct;

char username[maxNameLen] = "test";
char description[maxDescriptionLen] = "testDescription";

// functions initialization
readMessageStruct parseReadMessage(char message[maxMessageLen]);
int createAndConnectSocket();
void sendMessage(int sock, char cmd[maxCmdLen], char data[maxDataLen]);
void sendLoginMessage();

int isSocketConnected(int sockfd) 
{ 
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

readMessageStruct parseReadMessage(char message[maxMessageLen]) 
{
    readMessageStruct messageObj;

    strcpy(messageObj.cmd, strtok(message, ARGS_DELIMITER));
    strcpy(messageObj.data, strtok(message, ARGS_DELIMITER));

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

    fprintf(stdlog, "Socket Connected\n");
}

void sendMessage(int sock, char cmd[maxCmdLen], char data[maxDataLen])
{
    char message[maxMessageLen];

    strcat(message, cmd);
    strcat(message, ARGS_DELIMITER);
    strcat(message, data);

    send(sock, message, sizeof(message), 0);

    fprintf(stdlog, "sendMessage - Message Sent To Server: %s\n", message);
}

void sendLoginMessage()
{
    char cmd[maxCmdLen] = "login";
    char data[maxDataLen] = "";

    strcat(data, username);
    strcat(data, DATA_DELIMITER);
    strcat(data, description);

    sendMessage(clientSock, cmd, data);
}

int main(int argc, char const* argv[])
{
    createAndConnectSocket();
    sendLoginMessage();

    char terminalInputBuffer;
    
    FD_ZERO(&connectedFileDescriptors);
    FD_SET(clientSock, &connectedFileDescriptors);
    FD_SET(0, &connectedFileDescriptors);  // stdin - 0

    while(true)
    {
        readyToReadFileDescriptors = connectedFileDescriptors;
        readyToWriteFileDescriptors = connectedFileDescriptors;

        if ((select(FD_SETSIZE, &readyToReadFileDescriptors, &readyToWriteFileDescriptors, NULL, NULL)) == -1)      // check readinnes of sockets
        {
            fprintf(stderr, "main - select() [sockets] failed errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }


        // if (FD_ISSET(0, &readyToReadFileDescriptors))   // userInput
        // {   
        //     char terminalInputBuffer[2];
        //     fgets(terminalInputBuffer, sizeof(terminalInputBuffer), stdin);
        //     (terminalInputBuffer);
        // }
    }

    close(clientSock);
}