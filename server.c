#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int errono;

const int PORT = 5678;
const char* ADDR = "127.0.0.1";

int serverSock;
struct sockaddr_in serverAddr;
socklen_t serverAddrLen = sizeof(serverAddr);
int option = 1;

bool createSocket()
{
    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        fprintf(stderr, "socket() failed\n");
        exit(EXIT_FAILURE);
    }

    if ((setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))) == -1)
    {
        fprintf(stderr, "setsockopt() failed\n");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_aton(ADDR, &serverAddr.sin_addr) == 0)    // sets serverAddr.sin_addr.s_addr
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(EXIT_FAILURE);
    }

    if ((bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) == -1) 
    {
        fprintf(stderr, "bind() failed\n");
        exit(EXIT_FAILURE);
    }
}
int main()
{   
    createSocket();
    int testSock;
    
    if ((listen(serverSock, 10)) == -1) 
    {
        fprintf(stderr, "listen() failed\n");
        printf("%d\n", errno);
        exit(EXIT_FAILURE);
    }

    if ((testSock = accept(serverSock, (struct sockaddr*)&serverAddr, &serverAddrLen))== -1)
    {
        fprintf(stderr, "accept() failed\n");
        exit(EXIT_FAILURE);
    }

    close(testSock);
    close(serverSock);
    return 0;
}