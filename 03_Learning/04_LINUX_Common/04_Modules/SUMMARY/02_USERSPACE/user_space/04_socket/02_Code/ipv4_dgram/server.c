#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF_SIZE 256

char recvbuff[MAX_BUF_SIZE];
char sendbuff[MAX_BUF_SIZE];

void Print_info(struct sockaddr_in sockaddr, char* name, char* data, int len){
    int i = 0;
    char sockaddrStr[INET_ADDRSTRLEN];
    for (i = 0; i < len; i++){
        if (data[i] == '\n') data[i] = 0;
    }
    if (inet_ntop(AF_INET, &sockaddr.sin_addr, sockaddrStr, INET_ADDRSTRLEN) == NULL) 
        printf("Couldn't convert client address to string\n"); 
    else 
    {
        printf("*****%s node info:****\n", name);
        printf("Client IP: %s, client port: %d\n", sockaddrStr, ntohs(sockaddr.sin_port));
        printf("Data[%d]: '%s'\n", len, data);
        printf("*************************\n\n");
    }
}

int main(int argc, char *argv[])
{
    struct sockaddr_in svaddr, claddr;
    int fd, j;
    ssize_t numBytes;
    socklen_t len;
    int port_no;
    char claddrStr[INET_ADDRSTRLEN];

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) 
        return 1;

    /* Đọc portnumber trên command line */
    if (argc < 2) {
        printf("No port provided\ncommand: ./server <port number>\n");
        exit(EXIT_FAILURE);
    } else
        port_no = atoi(argv[1]);

    // set local info for binding to socket
    memset(&svaddr, 0, sizeof(struct sockaddr_in)); 
    svaddr.sin_family = AF_INET;
    svaddr.sin_addr.s_addr = INADDR_ANY;
    svaddr.sin_port = htons(port_no);

    if (bind(fd, (struct sockaddr *) &svaddr, sizeof(struct sockaddr_in)) == -1) 
        return 1; 
    printf("start listening on server\n");
	
    for (;;) {
        memset(sendbuff, '0', MAX_BUF_SIZE);
        memset(recvbuff, '0', MAX_BUF_SIZE);

        // printf("Please enter the message : ");
        // fgets(sendbuff, MAX_BUF_SIZE, stdin);

        len = sizeof(struct sockaddr_in);
        numBytes = recvfrom(fd, recvbuff, MAX_BUF_SIZE, 0, (struct sockaddr *) &claddr, &len);
		if (numBytes == -1)
			return 1;
			
        Print_info(claddr, "Other", recvbuff, strlen(recvbuff));
		
        printf("Please enter the message : ");
        fgets(sendbuff, MAX_BUF_SIZE, stdin);
		if (sendto(fd, sendbuff, strlen(sendbuff), 0, (struct sockaddr *) &claddr, len) != strlen(sendbuff)) 
			printf("sendto error\n");
	}
}