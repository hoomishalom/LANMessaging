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

#define TERMINAL_DBG 1

#define stdlog stdout   // logging messages go to "stdlog"
#define stddbg stdout   // debugging messages go to "stdlog"

#define maxUsers 16

#define maxCmdLen 256
#define maxDataLen 2048
#define maxMessageLen maxCmdLen + maxDataLen

#define maxLoggingMessageLen 1024
#define maxErrorMessageLen 1024

#define maxMessageQueued 256

#define maxNameLen 32
#define maxDescriptionLen 256

extern int errono;

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

int serverSock;
struct sockaddr_in serverAddr;
socklen_t serverAddrLen = sizeof(serverAddr);

int option = 1;
int BACKLOG = 32;

typedef struct {
    char cmd[maxCmdLen];
    char data[maxDataLen];
} readMessageStruct;

typedef struct {
    char sender[maxNameLen];
    char message[maxMessageLen];
} message;

typedef struct {
    int socket;
    char name[maxNameLen];
    char description[maxDescriptionLen];
    message pendingMessages[maxMessageQueued];
    int pendingMessageCount;
} userInfo;

int usercount = 0;
userInfo *users[maxUsers];

// functions initialization
void loggerPrint(char *data);
void errorPrint(char *data);
void createServerSocket();
readMessageStruct parseMessage(char message[maxMessageLen]);
int locateInfoBySocket(int socket);
int handleNewSocket();
void handleQuitReqeust();
void handleLoginRequest(int sock,readMessageStruct messageObj);
void handleIncomingRequest(int sock);
void handleNewMessage(int sock, char message[maxDataLen]);
void clearDebugTerminalInput();
void handleTerminalInput(char terminalInputBuffer[2]);
void sendMessageToClient(int sock, char *cmd, char *data);
void runServer();

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

void createServerSocket()
{   
    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        snprintf(errorMessage, sizeof(errorMessage), "createServerSocket - socket() failed errno: %d\n", errno);
        errorPrint(errorMessage);
        exit(EXIT_FAILURE);
    }

    if ((setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))) == -1)
    {
        snprintf(errorMessage, sizeof(errorMessage), "createServerSocket - setsocketopt() failed errno: %d\n", errno);
        errorPrint(errorMessage);
        exit(EXIT_FAILURE);
    }
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_aton(ADDR, &serverAddr.sin_addr) == 0)    // sets serverAddr.sin_addr.s_addr
    {   
        snprintf(errorMessage, sizeof(errorMessage), "createServerSocket - inet_aton() failed errno: %d\n", errno);
        errorPrint(errorMessage);
        exit(EXIT_FAILURE);
    }

    if ((bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) == -1)
    {
        snprintf(errorMessage, sizeof(errorMessage), "createServerSocket - bind() failed errno: %d\n", errno);
        errorPrint(errorMessage);
        exit(EXIT_FAILURE);
    }

    if ((listen(serverSock, BACKLOG)) == -1)
    {
        snprintf(errorMessage, sizeof(errorMessage), "createServerSocket - listen() failed errno: %d\n", errno);
        errorPrint(errorMessage);
        exit(EXIT_FAILURE);
    }
}

readMessageStruct parseMessage(char message[maxMessageLen])
{
    readMessageStruct messageObj;

    strcpy(messageObj.cmd, strtok(message, ARGS_DELIMITER));
    strcpy(messageObj.data, strtok(NULL, ARGS_DELIMITER));

    return messageObj;
}

int locateInfoBySocket(int socket)
{
    for (int i = 0; i < usercount; ++i)
    {
        if (users[i]->socket == socket)
        {
            return i;
            break;
        }
    }
    return -1;
}

int handleNewSocket()
{
    int clientSock;

    if ((clientSock = accept(serverSock, (struct sockaddr*)&serverAddr, &serverAddrLen)) == -1)
    {
        snprintf(errorMessage, sizeof(errorMessage), "handleNewSocket - accept() failed errno: %d\n", errno);
        errorPrint(errorMessage);        
        exit(EXIT_FAILURE);
    }

    snprintf(loggingMessage, maxLoggingMessageLen, "New connection from {ip: %s, port: %d, sock: %d}\n", inet_ntoa(serverAddr.sin_addr), serverAddr.sin_port, clientSock);
    loggerPrint(loggingMessage);

    return clientSock;
}

void handleQuitReqeust(int sock)
{
    int index = locateInfoBySocket(sock);

    if(index == -1)
    {
        snprintf(errorMessage, sizeof(errorMessage), "handleQuitReqeust - Socket: %d, Not Found In Users", sock);
        errorPrint(errorMessage);
        return;
    }

    memset(users[index], 0, sizeof(users[index]));  // remove userInfo from users array

    FD_CLR(sock, &connectedFileDescriptors);

    snprintf(loggingMessage, maxLoggingMessageLen, "Dissconected sock: %d", sock);
    loggerPrint(loggingMessage);
    
    close(sock);
}

void handleLoginRequest(int sock,readMessageStruct messageObj)
{
    userInfo *user = (userInfo *)malloc(sizeof(userInfo));
    user->socket = sock;

    strcpy(user->name, strtok(messageObj.data, DATA_DELIMITER));
    strcpy(user->description, strtok(NULL, DATA_DELIMITER));

    users[usercount] = user;
    ++usercount;
}

void handleIncomingRequest(int sock)
{
    char messageBuffer[maxMessageLen];
    char tempBuffer[maxMessageLen];
    char *cmd;
    int readBytes;

    readMessageStruct messageObj;

    memset(messageBuffer, 0, maxMessageLen);

    if ((readBytes = read(sock, messageBuffer, maxMessageLen)) == -1)
    {
        snprintf(errorMessage, maxErrorMessageLen, "handleIncomingRequest - read() failed errno: %d", errno);
        errorPrint(errorMessage);

        exit(EXIT_FAILURE);
    }

    if (strlen(messageBuffer) == 0)         // handle client crash
    {
        handleQuitReqeust(sock);
        return;
    }

    messageObj = parseMessage(messageBuffer);
    
    if (strcmp(messageObj.cmd, "quit") == 0) {
        handleQuitReqeust(sock);
    } else if (strcmp(messageObj.cmd, "login") == 0) {
        handleLoginRequest(sock, messageObj);
    } else if (strcmp(messageObj.cmd, "test") == 0) {
        handleNewMessage(sock, messageObj.data);
    } else if (strcmp(messageObj.cmd, "send") == 0) {
        handleNewMessage(sock, messageObj.data);
    }
    else
    {   
        snprintf(loggingMessage, maxLoggingMessageLen, "handleIncomingRequest - messageObj.cmd: %s, isn't known", messageObj.cmd);
        loggerPrint(loggingMessage);
    }

}

void handleNewMessage(int sock, char message[maxDataLen])
{
    char sender[maxNameLen];

    strcpy(sender, users[locateInfoBySocket(sock)]->name);
    for (int i = 0; i < usercount; ++i)
    {
        strcpy(users[i]->pendingMessages[users[i]->pendingMessageCount].message, message);
        strcpy(users[i]->pendingMessages[users[i]->pendingMessageCount].sender, sender);
        ++users[i]->pendingMessageCount;
    }
    
    if (message[strlen(message) - 1] == '\n') // removes trailing \n
    {
        message[strlen(message) - 1] = '\0';
    }
    
    snprintf(loggingMessage, maxLoggingMessageLen, "handleNewMessage - Message: {sender: %s, data: %s}", sender, message);
    loggerPrint(loggingMessage);
}

void clearDebugTerminalInput()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void handleTerminalInput(char terminalInput[2])
{   
    if (strcmp(terminalInput, "d") == 0)
    {   
        int i;
        int counter = 0;

        fprintf(stddbg, "\n");
        for (i = 0; i < FD_SETSIZE; ++i)
        {   
            if (FD_ISSET(i, &connectedFileDescriptors) != 0)
            {
                ++counter;
                fprintf(stddbg, "\tSocket Num: %d Is: %d\n", counter, i);
            }
        }
        fprintf(stddbg, "\n");
        
    } else if (strcmp(terminalInput, "u") == 0) {
        fprintf(stddbg, "\n\tUser Count: %d\n", usercount);
        for (int i = 0; i < usercount; ++i)
        {
            fprintf(stddbg, "\tUser Num: %d {Socket: %d, Name: %s, Description: %s, Messages Count: %d}\n", i + 1, users[i]->socket, users[i]->name, users[i]->description, users[i]->pendingMessageCount);
        }
        fprintf(stddbg, "\n");
    }
    else if (strcmp(terminalInput, "q") == 0)
    {
        loggerPrint("Quitting\n");

        for (int i = 0; i < FD_SETSIZE; ++i)
        {
            if (FD_ISSET(i, &connectedFileDescriptors))
            {
                close(i);
            }
        }
        exit(EXIT_SUCCESS);
    }
    else
    {   
        snprintf(errorMessage, maxErrorMessageLen, "handleIncomingRequest - read() failed errno: %d\n", errno);
        errorPrint(errorMessage);        
    }
    clearDebugTerminalInput();
}

void sendMessageToClient(int sock, char *cmd, char *data)
{

}

void runServer()
{
    char terminalInputBuffer;

    FD_ZERO(&connectedFileDescriptors);
    FD_SET(serverSock, &connectedFileDescriptors);
    FD_SET(0, &connectedFileDescriptors);  // stdin - 0


    while (true)
    {
        readyToReadFileDescriptors = connectedFileDescriptors;
        readyToWriteFileDescriptors = connectedFileDescriptors;

        if ((select(FD_SETSIZE, &readyToReadFileDescriptors, NULL, NULL, NULL)) == -1)      // check readinnes of sockets
        {
        snprintf(errorMessage, sizeof(errorMessage), "main - select() [sockets] failed errno: %d\n", errno);
        errorPrint(errorMessage);            
        exit(EXIT_FAILURE);
        }

        if (TERMINAL_DBG && FD_ISSET(0, &readyToReadFileDescriptors))   // debugg
        {   
            char terminalInputBuffer[2];
            fgets(terminalInputBuffer, sizeof(terminalInputBuffer), stdin);
            handleTerminalInput(terminalInputBuffer);
        }

        for (int i = 1; i < FD_SETSIZE; ++i)
        {   

            if (FD_ISSET(i, &readyToReadFileDescriptors))
            {
                if (i == serverSock)        // new connection
                {
                    int clientSock = handleNewSocket();
                    FD_SET(clientSock, &connectedFileDescriptors);
                } else
                {
                    handleIncomingRequest(i);
                }
            } else if (FD_ISSET(i, &readyToWriteFileDescriptors))
            {
            }
        }
    }
}

int main()
{
    createServerSocket();
    runServer();
    close(serverSock);
    return 0;
}