#include <sys/un.h>
#include <sys/socket.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h> // EXIT_FAILURE, unlink

#define MAX_BUF_SIZE 256 
#define SOCK_PATH "./sock_stream"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

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
    int server_fd, new_client_fd, j, opt;
    ssize_t numBytes;
    socklen_t len;
    char sendbuff[MAX_BUF_SIZE]={0};
    char recvbuff[MAX_BUF_SIZE]={0};

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        handle_error("socket()");
    }

    /* Ngăn lỗi : “address already in use” */
    unlink(SOCK_PATH);

    memset(&svaddr, 0, sizeof(struct sockaddr_un)); 
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SOCK_PATH, sizeof(svaddr.sun_path)-1);

    if (bind(server_fd, (struct sockaddr *) &svaddr, sizeof(struct sockaddr_un)) == -1) 
        handle_error("bind()");  
 
    if ((listen(server_fd, 5)) == -1)
	    handle_error("listen()");  
    else
	    printf("start listening on server\n");
	
    if((new_client_fd = accept(server_fd, (struct sockaddr*)&svaddr, &len)) == -1)
	    handle_error("accept()");  
    else
	    printf("accept connect\n");
	
    while(1) {
        if(read(new_client_fd, recvbuff, MAX_BUF_SIZE) > 0)
		{
			// printf("Recv: '%s'\n",recvbuff);
            Print_info("Local", recvbuff, strlen(recvbuff));

            printf("Please enter the message : ");
            fgets(sendbuff, MAX_BUF_SIZE, stdin);
            write(new_client_fd,sendbuff,MAX_BUF_SIZE-1);
		}
    }
	close(server_fd);
	remove(SOCK_PATH);
}