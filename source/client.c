#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

extern int errono;

const int PORT = 5678;
const char* ADDR = "127.0.0.1";

int main(int argc, char const* argv[])
{  
    int clientSock;
    int status;
    struct sockaddr_in serverAddr;

    if ((clientSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        fprintf(stderr, "socket() failed\n");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if ((inet_pton(AF_INET, ADDR, &serverAddr.sin_addr)) <= 0)
    {
        fprintf(stderr, "inet_pton() failed\n");
        exit(EXIT_FAILURE);
    }

    if ((status = connect(clientSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) == -1)
    {
        fprintf(stderr, "connect() failed\n");
        exit(EXIT_FAILURE);
    }

    close(clientSock);
}