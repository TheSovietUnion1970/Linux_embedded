#include <sys/un.h>
#include <sys/socket.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h> // EXIT_FAILURE
#include <unistd.h> // unlink

#define MAX_BUF_SIZE 256 
#define SOCK_PATH "./sock_dgram"

char recvbuff[MAX_BUF_SIZE];
char sendbuff[MAX_BUF_SIZE];

void Print_info(char* name, char* data, int len){
    int i = 0;
     
    for (i = 0; i < len; i++){
        if (data[i] == '\n') data[i] = 0;
    }
    printf("*****%s node info:****\n", name);
    printf("Sock path: %s\n", SOCK_PATH);
    printf("Data[%d]: '%s'\n", len - 1, data);
    printf("*************************\n\n");
   
}

int main(int argc, char *argv[])
{
    struct sockaddr_un svaddr, claddr;
    int fd, j;
    ssize_t numBytes;
    socklen_t len;

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd == -1) 
        return 1;

    memset(&svaddr, 0, sizeof(struct sockaddr_un)); 
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SOCK_PATH, sizeof(svaddr.sun_path)-1);

    unlink(SOCK_PATH);
    if (bind(fd, (struct sockaddr *) &svaddr, sizeof(struct sockaddr_un)) == -1) 
        return 1; 
    printf("start listening on server\n");
	
    for (;;) {
        memset(sendbuff, '0', MAX_BUF_SIZE);
        memset(recvbuff, '0', MAX_BUF_SIZE);

        // READ
        len = sizeof(struct sockaddr_un);
        numBytes = recvfrom(fd, recvbuff, MAX_BUF_SIZE, 0, (struct sockaddr *) &claddr, &len);
		if (numBytes == -1)
			return 1;
			
        Print_info("Local", recvbuff, numBytes);

        // SEND
        printf("Please enter the message : ");
        fgets(sendbuff, MAX_BUF_SIZE, stdin);
		if (sendto(fd, sendbuff, strlen(sendbuff), 0, (struct sockaddr *) &claddr, len) != strlen(sendbuff)) 
			printf("sendto error\n");
	}
}