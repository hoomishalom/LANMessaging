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

#define TERMINAL_DBG 0

#define stdlog stdout   // logging messages go to "stdlog"
#define stddbg stdout   // debugging messages go to "stdlog"

extern int errono;

const int PORT = 5678;
const char* ADDR = "127.0.0.1";

const char* DATA_DELIMITER = "~";
const char* ARGS_DELIMITER = "|";

fd_set connectedSockets;
fd_set readyToReadSockets;
fd_set readyToWriteSockets;
fd_set errorSockets;

int serverSock;
struct sockaddr_in serverAddr;
socklen_t serverAddrLen = sizeof(serverAddr);

int option = 1;
int BACKLOG = 32;

const int messageBufferLen = 1024;

typedef struct {
    char *cmd;
    char *data;
} readMessageStruct;

typedef struct {
    int destination;
    char *cmd;
    char *data;
} sendMessageStruct;

// functions initialization
void createServerSocket();
readMessageStruct parseReadMessage(char message[messageBufferLen]);
int handleNewSocket();
void handleQuitReqeust();
void handleIncomingRequest(int sock);
void handleDebugTerminalInput(char terminalInputBuffer[2]);

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

readMessageStruct parseReadMessage(char message[messageBufferLen]) {
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
    FD_CLR(sock, &connectedSockets);
    fprintf(stdlog, "Dissconected sock: %d\n", sock);
    close(sock);
}

void handleIncomingRequest(int sock)
{
    char messageBuffer[messageBufferLen];
    char tempBuffer[messageBufferLen];
    char *cmd;
    int readBytes;

    bzero(messageBuffer, messageBufferLen);

    if ((readBytes = read(sock, messageBuffer, messageBufferLen)) == -1)
    {
        fprintf(stderr, "handleIncomingRequest - read() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if (strlen(messageBuffer) == 0)         // handle client crash
    {
        handleQuitReqeust(sock);
    }
}

void handleDebugTerminalInput(char terminalInput[2])
{
    fprintf(stddbg, "\n\nTEST TESTSETSETS \n\n");
    if (strcmp(terminalInput, "d") == 0)
    {   
        int i;
        int counter = 0;

        fprintf(stddbg, "Listing All Connected Sockets:\n");
        for (i = 0; i < FD_SETSIZE; ++i)
        {   
            if (FD_ISSET(i, &connectedSockets) != 0)
            {
                ++counter;
                fprintf(stddbg, "\tsocket num: %d is: %d\n", counter, i);
            }
        }
        fprintf(stddbg, "End Listing\n");
        
    } else if(strcmp(terminalInput,"q") == 0)
    {
        fprintf(stdlog, "quitting\n");
        close(serverSock);
        exit(EXIT_SUCCESS);
    }
    else
    {
        fprintf(stderr, "handleDebugTerminalInput - unknown command [%c]\n", terminalInput);
    }
}

int main()
{
    createServerSocket();

    fd_set terminalInput;
    fd_set readyTerminalInput;

    char terminalInputBuffer;

    FD_ZERO(&terminalInput);
    FD_SET(0, &terminalInput);  // stdin - 0

    FD_ZERO(&connectedSockets);
    FD_SET(serverSock, &connectedSockets);


    while (true)
    {
        readyToReadSockets = connectedSockets;
        readyTerminalInput = terminalInput;
        
        if (TERMINAL_DBG)
        {
            if ((select(1, &readyTerminalInput, NULL, NULL, NULL)) == -1)               // check readinnes of terminal input
            {
                fprintf(stderr, "main - select() [terminal] failed errno: %d\n", errno);
                exit(EXIT_FAILURE);
            } 

            if (FD_ISSET(0, &readyTerminalInput))
            {
                char terminalInputBuffer[2];
                fgets(terminalInputBuffer, sizeof(terminalInputBuffer), stdin);
                handleDebugTerminalInput(terminalInputBuffer);
            }

        }
        
        if ((select(FD_SETSIZE, &readyToReadSockets, NULL, NULL, NULL)) == -1)      // check readinnes of sockets
        {
            fprintf(stderr, "main - select() [sockets] failed errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }

        int i;
        for (i = 0; i < FD_SETSIZE; ++i)
        {
            if (FD_ISSET(i, &readyToReadSockets))
            {
                if (i == serverSock)        // new connection
                {
                    int clientSock = handleNewSocket();
                    FD_SET(clientSock, &connectedSockets);
                } else
                {
                    handleIncomingRequest(i);
                }
            }   // addd write and error handling here
        }
    }

    close(serverSock);
    return 0;
}