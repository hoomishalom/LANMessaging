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

//functions initialization
void createServerSocket();
int handleNewSocket();
void handleQuitReqeust();
void handleIncomingRequest(int sock);

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

int handleNewSocket() {
    int clientSock;

    if ((clientSock = accept(serverSock, (struct sockaddr*)&serverAddr, &serverAddrLen)) == -1)
    {
        fprintf(stderr, "handleNewSocket - accept() failed errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    return clientSock;
}

void handleQuitReqeust()
{

    return;
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
    
    memcpy(tempBuffer, messageBuffer, messageBufferLen);
    cmd = strtok(tempBuffer, DATA_DELIMITER);

    if (strcmp(cmd, "quit"))
    {
        handleQuitReqeust();
    } else 
    {
        fprintf(stderr, "handleIncomingRequest - unknown command\n");
        exit(EXIT_FAILURE);
    }

    return;
}


int main()
{
    createServerSocket();

    FD_ZERO(&connectedSockets);
    FD_SET(serverSock, &connectedSockets);

    printf("fd_setzize: %d\n", FD_SETSIZE);
    while (true)
    {
        readyToReadSockets = connectedSockets;

        if ((select(FD_SETSIZE, &readyToReadSockets, NULL, NULL, NULL)) == -1)
        {
            fprintf(stderr, "select() failed errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        
        int i;
        for (i = 0; i < FD_SETSIZE; ++i)
        {
            if (FD_ISSET(i, &readyToReadSockets))
            {
                if (i == serverSock)
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