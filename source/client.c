#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

extern int errono;

const int PORT = 5678;
const char* ADDR = "127.0.0.1";

int clientSock;
struct sockaddr_in serverAddr;

// functions initialization
int createAndConnectSocket();

int createAndConnectSocket()
{
    int clientSock;

    if ((clientSock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "createAndConnectSocket - socket() failed errno: %d\n", errno);
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
}

int main(int argc, char const* argv[])
{  
    clientSock = createAndConnectSocket();

    printf("connected to server\n");
    while(true){}
    close(clientSock);
}