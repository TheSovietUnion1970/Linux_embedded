#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

// #define MSG "Hi everyone!"
#define MAX_BUF_SIZE 256

char recvbuff[MAX_BUF_SIZE];
char sendbuff[MAX_BUF_SIZE];

struct sockaddr_in svaddr;

void Print_info(struct sockaddr_in sockaddr, char* name, char* data, int len){
    int i = 0;
    char sockaddrStr[INET_ADDRSTRLEN];
    for (i = 0; i < len; i++){
        if (data[i] == '\n') data[i] = 0;
    }
    if (inet_ntop(AF_INET, &svaddr.sin_addr, sockaddrStr, INET_ADDRSTRLEN) == NULL) 
        printf("Couldn't convert client address to string\n"); 
    else 
    {
        printf("*****%s node info:****\n", name);
        printf("Server IP: %s, server port: %d\n", sockaddrStr, ntohs(sockaddr.sin_port));
        printf("Data[%d]: '%s'\n", len, data);
        printf("*************************\n\n");
    }
}

int main(int argc, char *argv[])
{    
  int fd;
  size_t msgLen;
  ssize_t numBytes;
  int portno;

  if (argc < 3) {
      printf("command : ./client <server address> <server port number>\n");
      exit(1);
  }
  portno = atoi(argv[2]);

  fd = socket(AF_INET, SOCK_DGRAM, 0);      
  if (fd == -1)
  return 1;
  memset(&svaddr, 0, sizeof(struct sockaddr_in));
  svaddr.sin_family = AF_INET;
  svaddr.sin_port = htons(portno);
  if(inet_pton(AF_INET, argv[1], &svaddr.sin_addr) <= 0)
  return 1;

  for (;;) {
    memset(sendbuff, '0', MAX_BUF_SIZE);
    memset(recvbuff, '0', MAX_BUF_SIZE);

    printf("Please enter the message : ");
    fgets(sendbuff, MAX_BUF_SIZE, stdin);

    if(sendto(fd, sendbuff, strlen(sendbuff),0,(struct sockaddr *)&svaddr, sizeof(struct sockaddr_in)) != (strlen(sendbuff)))
        return 1;

    memset(recvbuff, '0', MAX_BUF_SIZE);
    int len = sizeof(struct sockaddr_in);
    numBytes = recvfrom(fd, recvbuff, MAX_BUF_SIZE, 0, (struct sockaddr *)&svaddr, &len);
    if (numBytes == -1)
        return 1;
    else
        Print_info(svaddr, "Local", recvbuff, numBytes);
    }
    
    return 0;
}