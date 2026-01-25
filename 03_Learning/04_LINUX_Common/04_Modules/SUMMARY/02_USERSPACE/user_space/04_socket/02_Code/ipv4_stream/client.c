#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>     //  Chứa cấu trúc cần thiết cho socket. 
#include <netinet/in.h>     //  Thư viện chứa các hằng số, cấu trúc khi sử dụng địa chỉ trên internet
#include <arpa/inet.h>
#include <unistd.h>

#define BUFF_SIZE 256
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)
struct sockaddr_in server_addr;

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
        printf("Server IP: %s, server port: %d\n", sockaddrStr, ntohs(sockaddr.sin_port));
        if (len) printf("Data[%d]: '%s'\n", len, data);
        printf("*************************\n\n");
    }
}
		
/* Chức năng chat */
void chat_func(int client_fd)
{
    int numb_write, numb_read;
    char recvbuff[BUFF_SIZE];
    char sendbuff[BUFF_SIZE];
    while (1) {
        memset(sendbuff, '0', BUFF_SIZE);
	   				     memset(recvbuff, '0', BUFF_SIZE);
        printf("Please enter the message : ");
        fgets(sendbuff, BUFF_SIZE, stdin);

        /* Gửi thông điệp tới server bằng hàm write */
        numb_write = write(client_fd, sendbuff, sizeof(sendbuff));
        if (numb_write == -1)     
            handle_error("write()");
        if (strncmp("exit", sendbuff, 4) == 0) {
            printf("Client exit ...\n");
            break;
        }
		
        /* Nhận thông điệp từ server bằng hàm read */
        numb_read = read(client_fd, recvbuff, sizeof(recvbuff));
        if (numb_read < 0) 
            handle_error("read()");
        if (strncmp("exit", recvbuff, 4) == 0) {
            printf("Server exit ...\n");
            break;
        }

        Print_info(server_addr, "Local", recvbuff, strlen(recvbuff));
        //printf("\nMessage from Server: %s\n",recvbuff);   
    }
    close(client_fd); /*close*/ 
}

int main(int argc, char *argv[])
{
    int portno;
    int client_fd;
	memset(&server_addr, '0',sizeof(server_addr));
	
    /* Đọc portnumber từ command line */
    if (argc < 3) {
        printf("command : ./client <server address> <server port number>\n");
        exit(1);
    }
    portno = atoi(argv[2]);
	
    /* Khởi tạo địa chỉ server */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(portno);
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) == -1) 
        handle_error("inet_pton()");
	
    /* Tạo socket */
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
        handle_error("socket()");
	
    /* Kết nối tới server*/
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        handle_error("connect()");
	
    chat_func(client_fd);

    return 0;
}