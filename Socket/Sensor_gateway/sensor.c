#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>     //  Chứa cấu trúc cần thiết cho socket. 
#include <netinet/in.h>     //  Thư viện chứa các hằng số, cấu trúc khi sử dụng địa chỉ trên internet
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <ctype.h>
#include <unistd.h>

#define LISTEN_BACKLOG 50 // for server
#define BUFF_SIZE 256

/* >>>>>>>>>>>>>>>>>>>>>>>>>> MODIFY PORT + IP <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
#define SERVER_PORT 3000 
const char IP_Target[16] = "192.168.100.77";

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

/*  ========================================= Variables ======================================================  */
pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t sensor_id1;
char sendbuff[BUFF_SIZE];
char recvbuff[BUFF_SIZE];

struct sockaddr_in serv_addr, client_addr;

typedef struct gateway_info{
    int fd;
    char ip[16];
    int Isconnected;
} gi;
gi gateway;

struct pollfd fds_from_gateway; // for server handling events (connecting + reading)
int n_fds_from_gateway = 1;

int flag_connected = 0;
int server_fd_temp_for_client;
void *client_func_connect(){
    int portno;
    char ip[16];

    memset(&serv_addr, '0',sizeof(serv_addr));
    portno = 2000;
    strncpy(ip, IP_Target, sizeof(IP_Target));

    /* Scan to check whether IP is available */
    if (gateway.Isconnected == 1){
        printf("sensor already connects\n");
        //flag_connected = 1;
    }

    if (flag_connected != 1){
        /* Khởi tạo địa chỉ server */
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port   = htons(portno);
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) == -1) 
            handle_error("inet_pton()");
        
        /* Tạo socket */
        server_fd_temp_for_client = socket(AF_INET, SOCK_STREAM, 0);
        printf("server_fd_temp_for_client = %d\n", server_fd_temp_for_client);

        /* Kết nối tới server*/
        if (connect(server_fd_temp_for_client, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
            handle_error("connect()");

        fds_from_gateway.fd = server_fd_temp_for_client;
        fds_from_gateway.events = POLLIN;
    }
}

void *client_func2_handle() {
    int tmp_fd = 0;
    int msg_i_gateway = 0;
    while (1) {
        //printf("POLLING here...\n");
        int poll_count = poll(&fds_from_gateway, 1, -1);
        //printf("DONE POLLING ...\n");
        memset(recvbuff, 0, sizeof(recvbuff));
        if (poll_count == -1) {
            handle_error("poll1()");
        }

        /* 1 - only gateway connection */
        for (int i = 0; i < 1; i++) {
            pthread_mutex_lock(&client_lock); // Acquire the lock before processing events
            if (fds_from_gateway.revents & POLLIN) {
                ssize_t bytes_read = -1;
                bytes_read = read(fds_from_gateway.fd, recvbuff, BUFF_SIZE);
                if (bytes_read == -1) {
                    handle_error("read(client)");
                }

                if (bytes_read > 0){
                    if (strncmp(recvbuff, "minus", sizeof("minus")) == 0){
                        bytes_read = 0;
                    }
                }

                if (bytes_read == 0) {
                    // Server has closed the connection
                    printf("* Gateway has shut down\n");
                    close(fds_from_gateway.fd);
                    fds_from_gateway.fd = -1;  // Mark the socket as closed
                    //break;  // Exit the loop since the server is disconnected
                }
            }
            pthread_mutex_unlock(&client_lock);
        }
    }
}



int splitString(const char *input, char *str1, char *str2, char *str3) {
    // Temporary copy of the input string since strtok modifies the string
    char temp[256];
    strncpy(temp, input, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';  // Ensure null termination

    // Tokenize the string
    char *token = strtok(temp, " ");

    // Initialize output strings to empty
    str1[0] = str2[0] = str3[0] = '\0';

    // First token (command)
    if (token != NULL) {
        strncpy(str1, token, 255);
        str1[255] = '\0';  // Ensure null termination
        token = strtok(NULL, " ");
    }
    /* Scan to remove '\n' */
    int k = 0;
    while(1){
        /* Remove newline */
        if (str1[k] == '\n'){
            str1[k] = '\0';
            return 1;
        }

        else if (str1[k] == '\0'){
            break;
        }
        k++;
    }

    // Second token (IP address)
    if (token != NULL) {
        strncpy(str2, token, 255);
        str2[255] = '\0';  // Ensure null termination
        token = strtok(NULL, " ");
    }
    /* Scan to remove '\n' */
    int j = 0;
    while(1){
        /* Remove newline */
        if (str2[j] == '\n'){
            str2[j] = '\0';
            return 2;
        }

        else if (str2[j] == '\0'){
            break;
        }
        j++;
    }
    

    // The rest of the string (message)
    if (token != NULL) {
        // Copy the rest of the string as the third part
        strncpy(str3, token, 255);
        str3[255] = '\0';  // Ensure null termination
        token = strtok(NULL, "");

        if (token != NULL) {
            // Append the rest of the string to str3 if there are more tokens
            strncat(str3, " ", 255 - strlen(str3));
            strncat(str3, token, 255 - strlen(str3));
        }

        /* Scan to remove '\n' */
        int i = 0;
        while(1){
            /* Remove newline */
            if (str3[i] == '\n'){
                str3[i] = '\0';
                return 3;
            }

            else if (str3[i] == '\0'){
                break;
            }
            i++;
        }
    }
}

void Printpollfd(struct pollfd* fds, int s) {
    printf("---- List of fds --------\n");
    for (int i = 0; i < s; i++) {
        printf("(%d) ", fds[i].fd);
        printf("\n");
    }
    printf("-------------------------\n");
}

int main(){

    if (pthread_create(&sensor_id1, NULL, &client_func2_handle, NULL) != 0) {
        handle_error("pthread_create()");
    }

    while(1){
        char cmd[200];
        printf("Enter the command: ");
        fgets(cmd, sizeof(cmd), stdin); // Reads the entire line, including spaces

        char IP_addr[BUFF_SIZE];
        int port, fd_temp = -1;
        char request[BUFF_SIZE];
        int check = 0;
        char msg[256];
        char port_str[256];
        int s_rev;
        float temperature;
        char *endptr; // Pointer to character where conversion stops

        s_rev = splitString(cmd, request, IP_addr, port_str);
        port = atoi(port_str);

        temperature = strtof(IP_addr, &endptr);

        if (strncmp(request, "connect", sizeof("connect")) == 0){
            client_func_connect();

            pthread_detach(sensor_id1);

            if (pthread_create(&sensor_id1, NULL, &client_func2_handle, NULL) != 0) {
                handle_error("pthread_create()");
            }
        }
        else if (strncmp(request, "fd", sizeof("fd")) == 0){
            Printpollfd(&fds_from_gateway, 1);
        }
        else if (strncmp(request, "exit", sizeof("exit")) == 0){
            close(3);
            server_fd_temp_for_client = -1;
        }
        else if (strncmp(request, "send", sizeof("send")) == 0){
            if (-1 == server_fd_temp_for_client){
                printf("Sensor is already disconnected\n");
            }
            else {
                char temp_buffer[256];
                sprintf(temp_buffer, "Avergae temperature: %.2f oC", temperature);
                printf("temp_buffer = %s\n", temp_buffer);
                if (write(server_fd_temp_for_client, temp_buffer, sizeof(temp_buffer)) == -1)
                    handle_error("write()");
            }
        }
    }

}