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

#define SERVER_PORT 2000 
const char IP_APP[16] = "192.168.1.4";
/*
#define SERVER_PORT 2000 
const char IP_APP[16] = "192.168.1.4";

Each app should has unique port and IP address ==> define port and IP address above
*/

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* ============= Debug =============*/
#define debug 0

/*  ========================================= Structures ======================================================  */
typedef enum{
    NONE,
    FROM_ACTIVE_CLIENT,
    FROM_PASSIVE_SERVER
} IP_STATE;

typedef struct ip_port{
    char ip[16];
    unsigned int port;
}ip;
ip IP_port__App[10]; // 10 apps maximum
int IP_port__App_index = 0;

typedef struct {
    int fd;
    char ip[INET_ADDRSTRLEN];
} Info;

Info IP_fd__Client_Active[100];
int IP_fd__Client_Active_index = 1;

Info IP_fd__Server_Passive[100];
int IP_fd__Server_Passive_index = 1;

typedef struct {
    int fd;
    char ip[INET_ADDRSTRLEN];
    IP_STATE state_IP;
} Info_App;
Info_App IP_fd__Apps[100];
int IP_fd__Apps_index = 1;

struct sockaddr_in serv_addr, client_addr;
struct sockaddr_in serv_addr_for_client;

int msg_i_client, msg_i_server;
/*  ========================================= Print function ======================================================  */
void PrintArr(Info_App* Apps, int s) {
#if (debug == 1)
    printf("---- List of Ips -----\n");
    if (s >= 1){
        for (int i = 0; i < s; i++) {
            printf("[%d]->[%s] ", Apps[i].fd, Apps[i].ip);
            printf("\n");
        }
    }
    else {
        printf("No available IPs\n");
    }
    printf("-------------------------\n");
#else
    printf("\n*******************************************\n");
    printf("ID |    IP Address      \n");
    for (int i = 0; i < s; i++) {
        printf("%d  |    %s\n", i, Apps[i].ip);
    }
    printf("*******************************************\n");
#endif
}

void Printpollfd(struct pollfd* fds, int s) {
    printf("---- List of fds --------\n");
    for (int i = 0; i < s; i++) {
        printf("(%d) ", fds[i].fd);
        printf("\n");
    }
    printf("-------------------------\n");
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
/*  ========================================= Variables ======================================================  */
struct pollfd fds[100]; // for server handling events (connecting + reading)
int nfds = 1;

struct pollfd fds_r[100]; // for server handling events (reading)
int nfds_r = 1;
int nfds_r_max = 10;

pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t client_id1, client_id2;
pthread_t server_id1, server_id2;
pthread_t read_id1;

char sendbuff[BUFF_SIZE];
char recvbuff[BUFF_SIZE];

int flag_check_IP;

/* ========================================= Client part ====================================================== */

int server_fd_temp_for_client;
/* handle multiple servers */
void *client_func2_handle() {
    while (1) {
        int poll_count = poll(fds_r, nfds_r, -1);
        memset(recvbuff, 0, sizeof(recvbuff));
        if (poll_count == -1) {
            handle_error("poll()");
        }

        for (int i = 0; i < nfds_r; i++) {
            if (fds_r[i].fd == -1) {
                continue; // Skip invalid (removed) file descriptors
            }

            if (fds_r[i].revents & POLLIN) {
                //pthread_mutex_lock(&client_lock);
#if (debug == 1)
                printf("\nCLIENT handling, i = %d\n", i);
#endif               
                ssize_t bytes_read = read(IP_fd__Apps[i].fd, recvbuff, BUFF_SIZE);
                if (bytes_read == -1) {
                    handle_error("read(client)");
                }
#if (debug == 1)
                    // Process the data
                    printf("Done to read\n");
#endif
                //pthread_mutex_lock(&client_lock);

                recvbuff[bytes_read] = '\0'; // Null-terminate the received message
#if (debug == 1)
                printf("(Client) ==> Msg[%d] = '%s'\n", msg_i_client, recvbuff);
                msg_i_client++;
#else
                printf("\n*******************************************\n");
                printf("* Messge from: %s\n", IP_fd__Apps[i].ip);
                printf("* Messge     : '%s'\n", recvbuff);
                printf("*******************************************\n");
#endif

                if (strcmp(recvbuff, "exit") == 0) {
#if (debug == 1)
                    printf("Deleting IP: %s\n", IP_fd__Apps[i].ip);
#endif
                    // Close the socket and mark it for removal
                    close(fds_r[i].fd);

                    // Remove the deleted IP and shift the remaining entries
                    for (int x = i; x < IP_fd__Apps_index - 1; x++) {
                        IP_fd__Apps[x] = IP_fd__Apps[x + 1];
                        // IP_fd__Server_Passive[x] = IP_fd__Server_Passive[x + 1];
                        fds_r[x] = fds_r[x + 1];
                    }
                    
                    IP_fd__Apps_index--;
                    nfds_r--;

                    // Mark the last entry as invalid after shifting
                    fds_r[nfds_r].fd = -1;
                }

                //pthread_mutex_unlock(&client_lock);
            }
        }
    }
}

/* 
IP is used for storing in IP_fd__Client_Active
Port is used for connecting server
*/
void *client_func_connect(void *arg){
    ip *IP_used = (ip*)arg;
/* ------------------------- Client part ------------------------- */
    int portno;
    char ip[16];
    
	memset(&serv_addr_for_client, '0',sizeof(serv_addr_for_client));


    portno = IP_used->port;
    strncpy(ip, IP_used->ip, sizeof(IP_used->ip));

    printf("ip = %s\n", ip);

    /* Scan to check whether IP is available */
    for (int i = 0; i < IP_fd__Apps_index; i++){
        if (strcmp(ip, IP_fd__Apps[i].ip) == 0){
            printf("IP avaiable already or IP of this app\n");
            flag_check_IP = 1;
        }
    }

    // if IP is not available
    if (flag_check_IP != 1){
        /* Khởi tạo địa chỉ server */
        serv_addr_for_client.sin_family = AF_INET;
        serv_addr_for_client.sin_port   = htons(portno);
        if (inet_pton(AF_INET, IP_used->ip, &serv_addr_for_client.sin_addr) == -1) 
            handle_error("inet_pton()");
        
        /* Tạo socket */
        server_fd_temp_for_client = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_temp_for_client == -1)
            handle_error("socket()");

        /* SAVE Active Clients into arrays */
        IP_fd__Server_Passive[IP_fd__Server_Passive_index].fd = server_fd_temp_for_client;
        strcpy(IP_fd__Server_Passive[IP_fd__Server_Passive_index].ip, ip);
        printf("IP_fd__Server_Passive_index = %d\n", IP_fd__Server_Passive_index);
        /* SAVE common apps into arrays */
        IP_fd__Apps[IP_fd__Apps_index].fd = server_fd_temp_for_client;
        IP_fd__Apps[IP_fd__Apps_index].state_IP = FROM_PASSIVE_SERVER;
        strcpy(IP_fd__Apps[IP_fd__Apps_index].ip, ip);
        
        /* Kết nối tới server*/
        if (connect(server_fd_temp_for_client, (struct sockaddr *)&serv_addr_for_client, sizeof(serv_addr_for_client)) == -1)
            handle_error("connect()");
        printf("Done connecting!!\n");

        /* SAVE poll fds into arrays of CLIENTS + enable events*/
        fds_r[nfds_r].fd = server_fd_temp_for_client;
        fds_r[nfds_r].events = POLLIN;

        IP_fd__Server_Passive_index++; // increase index of ser ip index (passive server)
        IP_fd__Apps_index++; // increase index of all ip index (active client and passive server)
        // nfds++;
        nfds_r++; // increatse poll fd events
    }

    // reset flag
    flag_check_IP = 0;
/* --------------------------------------------------------------- */   
}
/* ========================================= Server part ====================================================== */
int len;
int server_fd_temp;
void *server_func1_listen(void *arg){
    ip *IP_used = (ip*)arg;

    int portno;
    char ip[16];
    int opt = 1;
    

    portno = IP_used->port;

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    
    /* Tạo socket */
    server_fd_temp = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_temp == -1)
        handle_error("socket()");
    // fprintf(stderr, "ERROR on socket() : %s\n", strerror(errno));

    /* Ngăn lỗi : “address already in use” */
    if (setsockopt(server_fd_temp, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        handle_error("setsockopt()");  

    /* Khởi tạo địa chỉ cho server */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr =  INADDR_ANY; //inet_addr("192.168.5.128"); //INADDR_ANY

    /* Gắn socket với địa chỉ server */
    if (bind(server_fd_temp, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        handle_error("bind()");

    /* Nghe tối đa 5 kết nối trong hàng đợi */
    if (listen(server_fd_temp, LISTEN_BACKLOG) == -1)
        handle_error("listen()");

    /* Dùng để lấy thông tin client */
	len = sizeof(client_addr);

    /* Poll for server */
    fds[0].fd = server_fd_temp;
    fds[0].events = POLLIN;

    /* Save this server into  IP_fd__Server_Passive */
    IP_fd__Server_Passive[0].fd = server_fd_temp;
    strcpy(IP_fd__Server_Passive[0].ip, IP_APP);
    /* Save this server into IP_fd__Apps*/
    IP_fd__Apps[0].fd = server_fd_temp;
    strcpy(IP_fd__Apps[0].ip, IP_APP);

    printf("Server is listening at port: %d\n....\n", SERVER_PORT);
}

void *server_func2_handle() {
    int new_socket_fd_temp = 0;
    while (1) {
        int poll_count = poll(fds, nfds, -1);
        memset(recvbuff, 0, sizeof(recvbuff));
        if (poll_count == -1) {
            handle_error("poll()");
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {

                //pthread_mutex_lock(&client_lock);

                if (fds[i].fd == IP_fd__Apps[0].fd) { // Server socket

                    //pthread_mutex_lock(&client_lock);

                    new_socket_fd_temp = accept(IP_fd__Apps[0].fd, (struct sockaddr*)&client_addr, &len);
                    if (new_socket_fd_temp == -1) {
                        handle_error("accept()");
                    }

                    //pthread_mutex_lock(&client_lock);
                    fds[nfds].fd = new_socket_fd_temp;
                    fds[nfds].events = POLLIN;

                    inet_ntop(AF_INET, &client_addr.sin_addr, IP_fd__Client_Active[IP_fd__Client_Active_index].ip, INET_ADDRSTRLEN);
                    IP_fd__Client_Active[IP_fd__Client_Active_index].fd = new_socket_fd_temp;

                    inet_ntop(AF_INET, &client_addr.sin_addr, IP_fd__Apps[IP_fd__Apps_index].ip, INET_ADDRSTRLEN);
                    IP_fd__Apps[IP_fd__Apps_index].fd = new_socket_fd_temp;
                    IP_fd__Apps[IP_fd__Apps_index].state_IP = FROM_ACTIVE_CLIENT;

                    nfds++;
                    IP_fd__Client_Active_index++;
                    IP_fd__Apps_index++;
                    //pthread_mutex_unlock(&client_lock);
#if (debug == 1)
                    printf("Accepted an active client connection, IP = %s\n", IP_fd__Client_Active[IP_fd__Client_Active_index - 1].ip);
                    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
#else
                    printf("\n*******************************************\n");
                    printf("Accepted a new connection from IP = %s\n", IP_fd__Client_Active[IP_fd__Client_Active_index - 1].ip);
                    printf("*******************************************\n\n");
#endif

                    //pthread_mutex_unlock(&client_lock);

                } else { // Handle client communication

                    //pthread_mutex_lock(&client_lock);

                    //char buffer[1024];
#if (debug == 1)
                    // Process the data
                    printf("Prepare to read\n");
#endif
                    ssize_t bytes_read = read(fds[i].fd, recvbuff, sizeof(recvbuff));
                    if (fds[i].fd == -1) {
                        handle_error("read (server)");
                    }
#if (debug == 1)
                    // Process the data
                    printf("Done to read\n");
#endif
                    //pthread_mutex_lock(&client_lock);
                    if (bytes_read > 0) {
#if (debug == 1)
                        // Process the data
                        printf("(Server) ==> Msg[%d] = '%s'\n", msg_i_server, recvbuff);
                        msg_i_server++;
#else
                        printf("\n*******************************************\n");
                        printf("* Messge from: %s\n", IP_fd__Apps[i].ip);
                        printf("* Messge     : '%s'\n", recvbuff);
                        printf("*******************************************\n");    
#endif
                        /* Exit command */
                        if (strcmp(recvbuff, "exit") == 0){
#if (debug == 1)
                            printf("delete IP: %s\n", IP_fd__Apps[i].ip);
#endif
                            close(IP_fd__Apps[i].fd);

                            // remove the deleted IP
                            for (int x = i; x < IP_fd__Apps_index; x++){
                                IP_fd__Apps[x] = IP_fd__Apps[x+1];
                            }
                            fds[i].fd = -1; // set the fd of deleted IP -1
                            
                            IP_fd__Apps_index--;
                            nfds--;
                        }

                    } else if (bytes_read == 0) {
#if (debug == 1)
                        // Connection closed by client
                        printf("Client disconnected, fd = %d\n", fds[i].fd);
#endif
                        close(fds[i].fd);
                        //pthread_mutex_lock(&client_lock);
                        for (int j = i; j < nfds - 1; j++) {
                            fds[j] = fds[j + 1];
                        }
                        nfds--;
                        //pthread_mutex_unlock(&client_lock);
                    } else {
#if (debug == 1)
                        printf("error\n");
#endif
                    }
#if (debug == 1)
                    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
#endif

                    //pthread_mutex_unlock(&client_lock);
                }

                //pthread_mutex_unlock(&client_lock);
            }
        }
    }
}

void DisplayOption(){
    printf("******************************* Chat application ******************************************\n");
    printf("Use the command below:\n");
    printf("1. help                                   : display user interface\n");
    printf("2. myip                                   : display IP of the host\n");
    printf("3. myport                                 : display the listening port of the host\n");
    printf("4. connect <IP address> <port No>         : connect to the app of another host\n");
    printf("5. send <IP address> <msg>                : send msg to the app of another host\n");
    printf("6. list                                   : list all avaiable IPs\n");
    printf("7. terminate <IP address>                 : terminate connections from an avaiable IP\n");
    printf("8. exit                                   : terminate connections from all avaiable IPs\n");
    printf("*******************************************************************************************\n");
}
/* ========================================= Main part ====================================================== */
int main(){
    char cmd[200];

    /* The info of server of app1 */
    strncpy(IP_port__App[0].ip, IP_APP, sizeof(IP_APP));
    IP_port__App[0].port = SERVER_PORT;

    /* Set up server part (not accepting)*/
    server_func1_listen(IP_port__App);

    /* parralel server handling client connecting */
    if (pthread_create(&server_id1, NULL, &server_func2_handle, NULL) != 0) {
        handle_error("pthread_create()");
    }

    /* parralel client handling server connecting */
    if (pthread_create(&client_id1, NULL, &client_func2_handle, NULL) != 0) {
        handle_error("pthread_create()");
    }

    DisplayOption();

    while(1){
        printf("Enter the command: ");

        fgets(cmd, sizeof(cmd), stdin); // Reads the entire line, including spaces
#if (debug == 1)
        printf("cmd = '%s'\n" ,cmd);
#endif
        char IP_addr[BUFF_SIZE];
        int port, fd_temp = -1;
        char request[BUFF_SIZE];
        int check = 0;
        char msg[256];
        char port_str[256];
        int s_rev;

        //sscanf(cmd, "%s %s %d",request, IP_addr, port_str);
        s_rev = splitString(cmd, request, IP_addr, port_str);
        port = atoi(port_str);

        if (strncmp(request, "connect", sizeof("connect")) == 0){
#if (debug == 1)
            printf(">>>>> Connect command\n");
#endif
            /* SAVE this active client (on this app) into IP_port__App */
            strncpy(IP_port__App[IP_port__App_index].ip, IP_addr, 16); 
            IP_port__App[IP_port__App_index].port = port;

            client_func_connect(IP_port__App + IP_port__App_index);

            pthread_detach(client_id1);

            /* parralel client handling server connecting */
            /* detach -> create -> ... to update fds_r */
            if (pthread_create(&client_id1, NULL, &client_func2_handle, NULL) != 0) {
                handle_error("pthread_create()");
            }

        }
        else if (strncmp(request, "list", sizeof("list")) == 0){
#if (debug == 1)
            printf(">>>>> List command\n");
#endif
            PrintArr(IP_fd__Apps, IP_fd__Apps_index);
        }
        else if (strncmp(request, "send", sizeof("send")) == 0){
#if (debug == 1)
            printf(">>>>> Send command\n");
#endif
            //pthread_mutex_lock(&client_lock);
            /* Scan to get msg */
            // sscanf(cmd, "%s %s %s",request, IP_addr, msg);
            s_rev = splitString(cmd, request, IP_addr, msg);

            /* scan IP to get fd */
            for (int i = 0; i < IP_fd__Apps_index; i++){
                 if (strcmp(IP_addr, IP_fd__Apps[i].ip) == 0){
                    fd_temp = IP_fd__Apps[i].fd;
                 }
            }

            /* check send Ip of this app */
            if (strcmp(IP_APP, IP_addr) == 0){
                fd_temp = -1;
            }

            if (fd_temp != -1){
                if (write(fd_temp, msg, sizeof(msg)) == -1)
                    handle_error("write()");
#if (debug == 1)
                printf("*** wtite to fd_temp: %d ***\n", fd_temp);
#endif
            }
            else {
                printf("\n*******************************************\n");
                printf("* Send invalid IP or not connecting\n");
                printf("*******************************************\n");
            }
            //pthread_mutex_unlock(&client_lock);
        }
        else if (strncmp(request, "exit", sizeof("exit")) == 0){
#if (debug == 1)
            printf(">>>>> Exit command\n");
#endif
            /* Check no available Ips */
            if (IP_fd__Apps_index == 1){
                printf("\n*******************************************\n");
                printf("* App already exited\n");
                printf("*******************************************\n");
            }
            else {
                for (int i = 1; i < IP_fd__Apps_index; i++){
                    if (write(IP_fd__Apps[i].fd, "exit", sizeof("exit")) == -1)
                        handle_error("write()");

                    IP_fd__Apps[i].fd = -1;
                    memset(IP_fd__Apps[i].ip, 0, sizeof(IP_fd__Apps[i].ip));

                    nfds_r--;
                    // Mark the last entry as invalid after shifting
                    fds_r[1].fd = -1;
                }
            }

            IP_fd__Apps_index = 1; // only contain of IP of this app


        }
        else if (strncmp(request, "terminate", sizeof("terminate")) == 0){
#if (debug == 1)
            printf(">>>>> Terminate command\n");
#endif
            int i_captured = 0;

            /* Scan to get IP */
            //sscanf(cmd, "%s %s",request, IP_addr);
            s_rev = splitString(cmd, request, IP_addr, port_str); // port_str = NULL

            /* Check no available Ips */
            if (IP_fd__Apps_index == 1){
                printf("\n*******************************************\n");
                printf("App already exited\n");
                printf("*******************************************\n");
            }
            else {
                /* Scan IPs to delete */
                for (int i = 1; i < IP_fd__Apps_index; i++){
                    if (strcmp(IP_fd__Apps[i].ip, IP_addr) == 0){
                        i_captured = i;
                        if (write(IP_fd__Apps[i].fd, "exit", sizeof("exit")) == -1)
                            handle_error("write()");

                        IP_fd__Apps[i].fd = -1;
                        memset(IP_fd__Apps[i].ip, 0, sizeof(IP_fd__Apps[i].ip));

                        if (IP_fd__Apps[i].state_IP == FROM_ACTIVE_CLIENT){
                            //printf("FROM_ACTIVE_CLIENT\n");
                            nfds--;
                            //nfds_r--;
                        }
                        else if (IP_fd__Apps[i].state_IP == FROM_PASSIVE_SERVER){
                            //printf("FROM_PASSIVE_SERVER\n");
                            nfds_r--;
                            //nfds--;
                        }

                        /* manage leftover IPs */
                        for (int i = i_captured; i < IP_fd__Apps_index; i++){
                            IP_fd__Apps[i] = IP_fd__Apps[i+1];
                        }
                        //printf("state IP = %d\n", IP_fd__Apps[i].state_IP);

                        // if (IP_fd__Apps[i].state_IP == FROM_ACTIVE_CLIENT){
                        //     //printf("FROM_ACTIVE_CLIENT\n");
                        //     nfds--;
                        //     //nfds_r--;
                        // }
                        // else if (IP_fd__Apps[i].state_IP == FROM_PASSIVE_SERVER){
                        //     //printf("FROM_PASSIVE_SERVER\n");
                        //     nfds_r--;
                        //     //nfds--;
                        // }

                        /* remove the size */
                        IP_fd__Apps_index--;
                        IP_fd__Apps[IP_fd__Apps_index].fd = -1;
                        IP_fd__Apps[IP_fd__Apps_index].state_IP = NONE;
                        memset(IP_fd__Apps[IP_fd__Apps_index].ip, 0, sizeof(IP_fd__Apps[IP_fd__Apps_index].ip));

                        
                        // nfds_r--;
                        // // Mark the last entry as invalid after shifting
                        // fds_r[1].fd = -1;
                        break;
                    }

                    /* If no IPs */
                    else {
                        printf("\n*******************************************\n");
                        printf("No available IPs in the list\n");
                        printf("*******************************************\n");
                    }
                }

            }
        }
        else if (strncmp(request, "help", sizeof("help")) == 0){
            DisplayOption();
        }
        else if (strncmp(request, "myip", sizeof("myip")) == 0){
            printf("\n*******************************************\n");
            printf("* IP address: %s\n", IP_fd__Apps[0].ip);
            printf("*******************************************\n");
        }
        else if (strncmp(request, "myport", sizeof("myport")) == 0){
            printf("\n*******************************************\n");
            printf("* Port: %d\n", SERVER_PORT);
            printf("*******************************************\n");
        }
        else {
            printf("Invalid input!\n");
            DisplayOption();
        }
#if (debug == 1)
        printf("Done command <<<<<<\n");
#endif
    }

}