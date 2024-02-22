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

#define maxCmdLen 256
#define maxDataLen 2048
#define maxMessageLen maxCmdLen + maxDataLen

#define maxMessageQueued 256

extern int errono;

const int PORT = 5678;
const char* ADDR = "127.0.0.1";

const char* DATA_DELIMITER = "~";
const char* ARGS_DELIMITER = "|";

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
    int destination;
    char cmd[maxCmdLen];
    char data[maxDataLen];
} sendMessageStruct;

sendMessageStruct messagesToSend[maxMessageQueued];

// functions initialization
void createServerSocket();
readMessageStruct parseReadMessage(char message[maxMessageLen]);
sendMessageStruct encodeSendMessage(int destination, char *cmd, char *data);
int handleNewSocket();
void handleQuitReqeust();
void handleIncomingRequest(int sock);
void clearDebugTerminalInput();
void handleDebugTerminalInput(char terminalInputBuffer[2]);
void sendMessageToClient(int sock, char *cmd, char *data);

void createServerSocket()
{
    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "createServerSocket - socket() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if ((setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))) == -1)
    {
        fprintf(stderr, "createServerSocket - setsockopt() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_aton(ADDR, &serverAddr.sin_addr) == 0)    // sets serverAddr.sin_addr.s_addr
    {
        fprintf(stderr, "createServerSocket - inet_aton() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if ((bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) == -1)
    {
        fprintf(stderr, "createServerSocket - bind() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if ((listen(serverSock, BACKLOG)) == -1)
    {
        fprintf(stderr, "createServerSocket - listen() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

readMessageStruct parseReadMessage(char message[maxMessageLen]) {
    readMessageStruct messageObj;

    strcpy(messageObj.cmd, strtok(message, ARGS_DELIMITER));
    strcpy(messageObj.data, strtok(NULL, ARGS_DELIMITER));

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

int handleNewSocket() {
    int clientSock;

    if ((clientSock = accept(serverSock, (struct sockaddr*)&serverAddr, &serverAddrLen)) == -1)
    {
        fprintf(stderr, "handleNewSocket - accept() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    fprintf(stdlog, "New connection from {ip: %s, port: %d, sock: %d}\n", inet_ntoa(serverAddr.sin_addr), serverAddr.sin_port, clientSock);

    return clientSock;
}

void handleQuitReqeust(int sock)
{
    FD_CLR(sock, &connectedFileDescriptors);
    fprintf(stdlog, "Dissconected sock: %d\n", sock);
    close(sock);
}

void handleIncomingRequest(int sock)
{
    char messageBuffer[maxMessageLen];
    char tempBuffer[maxMessageLen];
    char *cmd;
    int readBytes;

    readMessageStruct messageObj;

    bzero(messageBuffer, maxMessageLen);

    if ((readBytes = read(sock, messageBuffer, maxMessageLen)) == -1)
    {
        fprintf(stderr, "handleIncomingRequest - read() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if (strlen(messageBuffer) == 0)         // handle client crash
    {
        handleQuitReqeust(sock);
        return;
    }

    messageObj = parseReadMessage(messageBuffer);
    
    if (strcmp(messageObj.cmd, "quit") == 0) {
        handleQuitReqeust(sock);
    } else if (strcmp(messageObj.cmd, "test") == 0) {
        printf("%s\n", messageObj.data);
    } else
    {
        fprintf(stdlog, "handleIncomingRequest - messageObj.cmd: %s, isn't known\n", messageObj.cmd);
    }

}

void clearDebugTerminalInput()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void handleDebugTerminalInput(char terminalInput[2])
{
    if (strcmp(terminalInput, "d") == 0)
    {   
        int i;
        int counter = 0;

        printf("\n");
        for (i = 0; i < FD_SETSIZE; ++i)
        {   
            if (FD_ISSET(i, &connectedFileDescriptors) != 0)
            {
                ++counter;
                fprintf(stddbg, "\tsocket num: %d is: %d\n", counter, i);
            }
        }
        printf("\n");
        
    } else if(strcmp(terminalInput,"q") == 0)
    {
        fprintf(stdlog, "quitting\n");
        close(serverSock);
        exit(EXIT_SUCCESS);
    }
    else
    {   
        fprintf(stderr, "handleDebugTerminalInput - unknown command [%s]\n", terminalInput);
    }
    clearDebugTerminalInput();
}

void sendMessageToClient(int sock, char *cmd, char *data)
{

}

int main()
{
    createServerSocket();

    fd_set terminalInput;
    fd_set readyTerminalInput;

    char terminalInputBuffer;

    FD_ZERO(&connectedFileDescriptors);
    FD_SET(serverSock, &connectedFileDescriptors);
    FD_SET(0, &connectedFileDescriptors);  // stdin - 0


    while (true)
    {
        readyToReadFileDescriptors = connectedFileDescriptors;

        if ((select(FD_SETSIZE, &readyToReadFileDescriptors, NULL, NULL, NULL)) == -1)      // check readinnes of sockets
        {
            fprintf(stderr, "main - select() [sockets] failed errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }

        if (TERMINAL_DBG && FD_ISSET(0, &readyToReadFileDescriptors))   // debugg
        {   
            char terminalInputBuffer[2];
            fgets(terminalInputBuffer, sizeof(terminalInputBuffer), stdin);
            handleDebugTerminalInput(terminalInputBuffer);
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
            }   // add write and error handling here
        }
    }

    close(serverSock);
    return 0;
}