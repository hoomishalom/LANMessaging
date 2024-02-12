#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

int main(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    return 0;
}