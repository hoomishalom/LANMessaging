#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int PORT = 5678;
const char* ADDR = "127.0.0.1";

int main()
{   
    int option = 1;
    int serverSock;
    int testSock;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

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

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    if (inet_aton(ADDR, &address.sin_addr) == 0)    // sets address.sin_addr.s_addr
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(EXIT_FAILURE);
    }

    if ((bind(serverSock, (struct sockaddr*)&address, sizeof(address))) == -1) 
    {
        fprintf(stderr, "bind() failed\n");
        exit(EXIT_FAILURE);
    }

    if ((listen(serverSock, 10)) == -1) 
    {
        fprintf(stderr, "listen() failed\n");
        exit(EXIT_FAILURE);
    }

    if ((testSock = accept(serverSock, (struct sockaddr*)&address, &addrlen))== -1)
    {
        fprintf(stderr, "accept() failed\n");
        exit(EXIT_FAILURE);
    }

    close(testSock);
    close(serverSock);
    return 0;
}