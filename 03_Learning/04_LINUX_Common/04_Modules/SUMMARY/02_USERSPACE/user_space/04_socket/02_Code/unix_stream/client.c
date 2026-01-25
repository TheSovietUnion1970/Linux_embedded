#include <sys/un.h>
#include <sys/socket.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_BUF_SIZE 256 
#define SOCK_PATH "./sock_stream"

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
    int fd;
    size_t msgLen;
    ssize_t numBytes;
    char sendbuff[MAX_BUF_SIZE]={0};
    char recvbuff[MAX_BUF_SIZE]={0};
	
    fd = socket(AF_UNIX, SOCK_STREAM, 0);      
    if (fd == -1)
		return 1;
	
    memset(&svaddr, 0, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SOCK_PATH, sizeof(svaddr.sun_path)-1);
	
	if (connect(fd, (struct sockaddr*)&svaddr, sizeof(struct sockaddr)) != 0) 
	{ 
        printf("connection with the server failed...\n"); 
        return 1; 
    } 
    else
	{
        printf("connected to the server.\n"); 
	}
	
	while(1)
	{      
        memset(sendbuff, '0', MAX_BUF_SIZE);
        memset(recvbuff, '0', MAX_BUF_SIZE);

        printf("Please enter the message : ");
        fgets(sendbuff, MAX_BUF_SIZE, stdin);
        write(fd,sendbuff,MAX_BUF_SIZE-1);

        if(read(fd, recvbuff, sizeof(recvbuff)) > 0) 
			//printf("From Server : %s\n", recvbuff); 
            Print_info("Local", recvbuff, strlen(recvbuff));
    } 
    
    return 0;
}