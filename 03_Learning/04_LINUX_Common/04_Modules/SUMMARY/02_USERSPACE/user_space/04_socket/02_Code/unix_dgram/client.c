#include <sys/un.h>
#include <sys/socket.h>
#include <stddef.h>
#include <stdio.h>

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
	struct sockaddr_un svaddr;
    int fd,optval;
    size_t msgLen;
    ssize_t numBytes;
    char resp[MAX_BUF_SIZE];
	
    fd = socket(AF_UNIX, SOCK_DGRAM, 0);      
    if (fd == -1)
		return 1;
	
	memset(&svaddr, 0, sizeof(struct sockaddr_un)); 
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SOCK_PATH, sizeof(svaddr.sun_path)-1);
	
	optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval));

    for (;;) {
        memset(sendbuff, '0', MAX_BUF_SIZE);
        memset(recvbuff, '0', MAX_BUF_SIZE);

        // SEND
        printf("Please enter the message : ");
        fgets(sendbuff, MAX_BUF_SIZE, stdin);
        if(sendto(fd, sendbuff, strlen(sendbuff),0,(struct sockaddr *)&svaddr, sizeof(struct sockaddr_un)) != (strlen(sendbuff)))
            return 1;

        // READ
        numBytes = recvfrom(fd, recvbuff, MAX_BUF_SIZE, 0, NULL, NULL);
        if (numBytes == -1)
            return 1;
        else
            Print_info("Local", recvbuff, numBytes);
    }
    
    return 0;
}