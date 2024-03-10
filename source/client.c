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
#define maxMessageLen maxCmdLen + maxDataLen - 16 // 16 - buffer for delimiters

#define maxLoggingMessageLen 1024
#define maxErrorMessageLen 1024

#define maxMessageQueued 256

#define maxNameLen 32
#define maxDescriptionLen 256

extern int errono;

int clientSock; 

const int PORT = 5678;
const char* ADDR = "127.0.0.1";

const char* DATA_DELIMITER = "~";
const char* ARGS_DELIMITER = "|";

char loggingMessage[maxLoggingMessageLen];
char errorMessage[maxErrorMessageLen];

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
} messageStruct;

char username[maxNameLen] = "test";
char description[maxDescriptionLen] = "testDescription";

messageStruct messagesQueue[maxMessageQueued];
int messageQueueCount = 0;

// queue intialization
void enqueue(messageStruct queue[maxMessageQueued], messageStruct *msg);
messageStruct *dequeue(messageStruct queue[maxMessageQueued]);
messageStruct *head(messageStruct queue[maxMessageQueued]);
messageStruct tail(messageStruct queue[maxMessageQueued]);

// functions initialization
void loggerPrint(char *data);
void errorPrint(char *data);
messageStruct parseReadMessage(char message[maxMessageLen]);
int createAndConnectSocket();
void disconnect(int sock);
void sendMessage(int sock, messageStruct *input);
void sendLoginMessage();
void handleTerminalInput(char terminalInput[2]);

// queue functions
void enqueue(messageStruct queue[maxMessageQueued], messageStruct *msg)
{
    if (messageQueueCount < maxMessageQueued) {
        strcpy(queue[messageQueueCount].cmd, msg->cmd);
        strcpy(queue[messageQueueCount].data, msg->data);

        ++messageQueueCount;
    } else {
        errorPrint("Messages Are Not Being Sent To Server, Closing Client Connection");
    }
}

messageStruct *dequeue(messageStruct queue[maxMessageQueued])
{
    messageStruct *message;
    if (messageQueueCount > 0)
    {
        message = &queue[messageQueueCount - 1];
        --messageQueueCount;
    }

    return message;
}

messageStruct *head(messageStruct queue[maxMessageQueued])
{
    return &queue[0];
}

messageStruct tail(messageStruct queue[maxMessageQueued])
{
    return queue[messageQueueCount - 1];
}

// functions 
void loggerPrint(char *data)
{
    char message[maxLoggingMessageLen];
    char timestamp[32];

    memset(message, 0, maxLoggingMessageLen);

    snprintf(timestamp, sizeof(timestamp), "[%d] (log) ", time(NULL));
    
    
    strcat(message, timestamp);
    strcat(message, data);

    if (message[strlen(message) - 1] == '\n') // removes trailing \n
    {
        message[strlen(message) - 1] = '\0';
    }

    fprintf(stdlog, "%s\n", message);

    memset(&data, 0, sizeof(data));
}

void errorPrint(char *data)
{
    char message[maxLoggingMessageLen];
    char timestamp[32];

    memset(message, 0, maxLoggingMessageLen);

    snprintf(timestamp, sizeof(timestamp), "[%d] (err) ", time(NULL));

    strcat(message, timestamp);
    strcat(message, data);

    if (message[strlen(message) - 1] == '\n') // removes trailing \n
    {
        message[strlen(message) - 1] = '\0';
    }

    fprintf(stderr, "%s\n", message);

    memset(&data, 0, sizeof(data));
}

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

messageStruct parseReadMessage(char message[maxMessageLen])
{
    messageStruct messageObj;

    strcpy(messageObj.cmd, strtok(message, ARGS_DELIMITER));
    strcpy(messageObj.data, strtok(message, ARGS_DELIMITER));

    return messageObj;
}

int createAndConnectSocket()
{
    if ((clientSock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        snprintf(errorMessage, sizeof(errorMessage), "createAndConnectSocket - socket() failed errno: %d\n", errno);
        errorPrint(errorMessage);
        exit(EXIT_FAILURE);
    }

    if ((setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))) == -1)
    {
        snprintf(errorMessage, sizeof(errorMessage), "createAndConnectSocket - setsockopt() failed errno: %d\n", errno);
        errorPrint(errorMessage);
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if ((inet_pton(AF_INET, ADDR, &serverAddr.sin_addr)) <= 0)
    {
        snprintf(errorMessage, sizeof(errorMessage), "createAndConnectSocket - inet_pton() failed errno: %d\n", errno);
        errorPrint(errorMessage);
        exit(EXIT_FAILURE);
    }

    if ((connect(clientSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) == -1)
    {
        snprintf(errorMessage, sizeof(errorMessage), "createAndConnectSocket - connect() failed errno: %d\n", errno);
        errorPrint(errorMessage);
        exit(EXIT_FAILURE);
    }

    loggerPrint("Socket Connected\n");
}

void disconnect(int sock)
{
    close(sock);
    loggerPrint("Socket Disconnected\n");
    exit(EXIT_SUCCESS);
}

void sendMessage(int sock, messageStruct *input)
{
    char message[maxMessageLen];
    memset(message, 0, maxMessageLen);

    printf("%s\n", input->cmd);

    strcat(message, input->cmd);
    strcat(message, ARGS_DELIMITER);
    strcat(message, input->data);

    send(sock, message, sizeof(message), 0);

    snprintf(loggingMessage, sizeof(loggingMessage), "sendMessage - Message Sent To Server: %s", message);
    loggerPrint(loggingMessage);
}

void sendLoginMessage()
{
    char cmd[maxCmdLen] = "login";
    char data[maxDataLen] = "";
    messageStruct msg;

    strcat(data, username);
    strcat(data, DATA_DELIMITER);
    strcat(data, description);

    strcpy(msg.cmd, cmd);
    strcpy(msg.data, data);

    enqueue(messagesQueue, &msg);
}

void handleTerminalInput(char terminalInput[2])
{
    if(strcmp(terminalInput, "t") == 0){
        char cmd[maxCmdLen] = "test";
        char data[maxDataLen] = "This Is A Test";
        messageStruct msg;
        strcpy(msg.cmd, cmd);
        strcpy(msg.data, data);

        sendMessage(clientSock, &msg);
    } else if(strcmp(terminalInput, "q") == 0){
        disconnect(clientSock);
    } else if(strcmp(terminalInput, "s") == 0){
        char cmd[maxCmdLen] = "send";
        char data[maxDataLen];
        messageStruct message;

        printf("message: ");
        fgets(data, 5, stdin); // makes sure the program waits for user input instead of just continuing

        fgets(data, sizeof(data), stdin);
        if (strlen(data) != 0)
        {
            enqueue(messagesQueue, &message);
        }
    }
}

void runClient()
{

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
            snprintf(errorMessage, sizeof(errorMessage), "main - select() [sockets] failed errno: %d\n", errno);
            errorPrint(errorMessage);
            exit(EXIT_FAILURE);
        }


        if (FD_ISSET(0, &readyToReadFileDescriptors))   // userInput
        {   
            char terminalInputBuffer[2];
            fgets(terminalInputBuffer, sizeof(terminalInputBuffer), stdin);
            handleTerminalInput(terminalInputBuffer);
        }

        if (FD_ISSET(clientSock, &readyToWriteFileDescriptors)) // check writeinnes of sockets (server)
        {
            if (messageQueueCount > 0)
            {
                sendMessage(clientSock, dequeue(messagesQueue));
            }
        }
    }
}

int main(int argc, char const* argv[])
{
    createAndConnectSocket();
    sendLoginMessage();
    runClient();
    close(clientSock);
    return 0;
}