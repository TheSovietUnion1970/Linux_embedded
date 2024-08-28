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

#define SERVER_PORT 2000 /* >>>>>>>>>>>>>>>>>>>>>>>>>> <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
const char IP_APP[16] = "192.168.0.75";

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* ============= Debug =============*/
#define debug 1

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
struct sockaddr_in serv_addr, client_addr;
struct sockaddr_in serv_addr_for_client;

/*  ========================================= Variables ======================================================  */
struct pollfd fds_from_client[100]; // for server handling events (connecting + reading)
int n_fds_from_client = 1;

struct pollfd fds_from_server[100]; // for server handling events (connecting + reading)
int n_fds_from_server = 0;

pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t client_id1, client_id2;
pthread_t server_id1, server_id2;

char sendbuff[BUFF_SIZE];
char recvbuff[BUFF_SIZE];

int flag_check_IP;
int msg_i_client, msg_i_server;


typedef struct {
    int fd;
    char ip[INET_ADDRSTRLEN];
} Info;

Info IP_fd__Client_Active[100];
int IP_fd__Client_Active_index = 1;

Info IP_fd__Server_Passive[100];
int IP_fd__Server_Passive_index = 0;

typedef struct {
    int fd;
    char ip[INET_ADDRSTRLEN];
    IP_STATE state_IP;
} Info_App;
Info_App IP_fd__Apps[100];
int IP_fd__Apps_index = 1;

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

/*  ========================================= Display ======================================================  */
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
    printf("server_fd_temp = %d\n", server_fd_temp);
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
    fds_from_client[0].fd = server_fd_temp;
    fds_from_client[0].events = POLLIN;

    /* Save this server into  IP_fd__Server_Passive */
    IP_fd__Server_Passive[0].fd = server_fd_temp;
    strcpy(IP_fd__Server_Passive[0].ip, IP_APP);
    /* Save this server into IP_fd__Apps*/
    IP_fd__Apps[0].fd = server_fd_temp;
    strcpy(IP_fd__Apps[0].ip, IP_APP);
    IP_fd__Apps[0].state_IP = FROM_ACTIVE_CLIENT;

    printf("Server is listening at port: %d\n....\n", SERVER_PORT);
}

void *server_func2_handle() {
    int new_socket_fd_temp = 0;
    while (1){
        int poll_count = poll(fds_from_client, n_fds_from_client, -1);
        memset(recvbuff, 0, sizeof(recvbuff));
        if (poll_count == -1) {
            handle_error("poll()");
        }

        for (int i = 0; i <= n_fds_from_client; i++){
            if (fds_from_client[i].revents & POLLIN){

                if (fds_from_client[i].fd == IP_fd__Apps[0].fd){
                    new_socket_fd_temp = accept(IP_fd__Apps[0].fd, (struct sockaddr*)&client_addr, &len);
                    if (new_socket_fd_temp == -1) {
                        handle_error("accept()");
                    }

                    /* fd new */
                    fds_from_client[n_fds_from_client].fd = new_socket_fd_temp;
                    fds_from_client[n_fds_from_client].events = POLLIN;

                    /* client new */
                    inet_ntop(AF_INET, &client_addr.sin_addr, IP_fd__Client_Active[IP_fd__Client_Active_index].ip, INET_ADDRSTRLEN);
                    IP_fd__Client_Active[IP_fd__Client_Active_index].fd = new_socket_fd_temp;

                    /* common new */
                    inet_ntop(AF_INET, &client_addr.sin_addr, IP_fd__Apps[IP_fd__Apps_index].ip, INET_ADDRSTRLEN);
                    IP_fd__Apps[IP_fd__Apps_index].fd = new_socket_fd_temp;
                    IP_fd__Apps[IP_fd__Apps_index].state_IP = FROM_ACTIVE_CLIENT;

                    n_fds_from_client++;
                    IP_fd__Client_Active_index++;
                    IP_fd__Apps_index++;

                    printf("Accepted an active client connection, IP = %s\n", IP_fd__Client_Active[IP_fd__Client_Active_index - 1].ip);
                    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
                }


                else {
#if (debug == 1)
                    // Process the data
                    printf("Prepare to read\n");
#endif
                    ssize_t bytes_read = read(fds_from_client[i].fd, recvbuff, sizeof(recvbuff));
                    if (fds_from_client[i].fd == -1) {
                        handle_error("read (server)");
                    }
#if (debug == 1)
                    // Process the data
                    printf("Done to read\n");

                    if (bytes_read > 0) {
                        printf("(Server - fd[%d].fd = %d) ==> Msg[%d] = '%s'\n", i, fds_from_client[i].fd, msg_i_server, recvbuff);
                        msg_i_server++;

                        /* Exit */
                        if (strcmp(recvbuff, "exit") == 0){
                            printf("Deleting IP (active client): %s\n", IP_fd__Client_Active[i].ip);
                            close(fds_from_client[i].fd);

                            // Remove the deleted IP and shift the remaining entries
                            for (int j = i; j < n_fds_from_client - 1; j++) {
                                fds_from_client[j] = fds_from_client[j + 1];
                                IP_fd__Client_Active[j] = IP_fd__Client_Active[j + 1];
                            }

                            /* scan for common */
                            for (int x = 0; x < IP_fd__Apps_index; x++) {
                                if (strcmp(IP_fd__Client_Active[i].ip, IP_fd__Apps[x].ip) == 0){
                                    for (int v = x; v < IP_fd__Apps_index - 1; v++){
                                        IP_fd__Apps[v] = IP_fd__Apps[v+1];
                                    }
                                    break;
                                }
                            }

                            IP_fd__Client_Active_index--;
                            IP_fd__Apps_index--;
                            n_fds_from_client--;   
                        }
                    }
                    else if (bytes_read == 0) {
                        printf("Client disconnected, fd[%d].fd = %d\n", i, fds_from_client[i].fd);

                        close(fds_from_client[i].fd);

                        // Remove the deleted IP and shift the remaining entries
                        for (int j = i; j < n_fds_from_client - 1; j++) {
                            fds_from_client[j] = fds_from_client[j + 1];
                            IP_fd__Client_Active[j] = IP_fd__Client_Active[j + 1];
                        }

                        /* scan for common */
                        for (int x = 0; x < IP_fd__Apps_index; x++) {
                            if (strcmp(IP_fd__Client_Active[i].ip, IP_fd__Apps[x].ip) == 0){
                                for (int v = x; v < IP_fd__Apps_index - 1; v++){
                                    IP_fd__Apps[v] = IP_fd__Apps[v+1];
                                }
                                break;
                            }
                        }

                        IP_fd__Client_Active_index--;
                        IP_fd__Apps_index--;
                        n_fds_from_client--;
                    }
                }
#endif
                fds_from_client[i].revents = 0;
            }
        }

        PrintArr(IP_fd__Apps, IP_fd__Apps_index);
    }
}

/* ========================================= Client part ====================================================== */
int server_fd_temp_for_client;
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
        printf("server_fd_temp_for_client = %d\n", server_fd_temp_for_client);
        if (server_fd_temp_for_client == -1)
            handle_error("socket()");

        /* SAVE Passive server into arrays */
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
        fds_from_server[n_fds_from_server].fd = server_fd_temp_for_client;
        fds_from_server[n_fds_from_server].events = POLLIN;

        IP_fd__Server_Passive_index++; // increase index of ser ip index (passive server)
        IP_fd__Apps_index++; // increase index of all ip index (active client and passive server)
        // nfds++;
        n_fds_from_server++; // increatse poll fd events
    }

    // reset flag
    flag_check_IP = 0;
}

void *client_func2_handle() {
    while (1) {
        int poll_count = poll(fds_from_server, n_fds_from_server, -1);
        memset(recvbuff, 0, sizeof(recvbuff));
        if (poll_count == -1) {
            handle_error("poll1()");
        }

        for (int i = 0; i <= n_fds_from_server; i++) {
            if (fds_from_server[i].fd == -1) {
                handle_error("poll2()");
            }

            if (fds_from_server[i].revents & POLLIN) {
                //pthread_mutex_lock(&client_lock);
#if (debug == 1)
                printf("\nCLIENT handling, i = %d\n", i);
                printf("IP_fd__Server_Passive[%d].fd = %d\n", i, IP_fd__Server_Passive[i].fd);
#endif               
                ssize_t bytes_read = read(IP_fd__Server_Passive[i].fd, recvbuff, BUFF_SIZE);
                if (bytes_read == -1) {
                    handle_error("read(client)");
                }
#if (debug == 1)
                // Process the data
                printf("Done to read\n");
#endif
                //pthread_mutex_lock(&client_lock);

                if (bytes_read > 0){
                    recvbuff[bytes_read] = '\0'; // Null-terminate the received message
    #if (debug == 1)
                    printf("(Client handle - fd[%d].fd = %d) ==> Msg[%d] = '%s'\n", i, IP_fd__Server_Passive[i].fd, msg_i_client, recvbuff);
                    msg_i_client++;
    #else
                    printf("\n*******************************************\n");
                    printf("* Messge from: %s\n", IP_fd__Apps[i].ip);
                    printf("* Messge     : '%s'\n", recvbuff);
                    printf("*******************************************\n");
    #endif

                    if (strcmp(recvbuff, "exit") == 0) {
    #if (debug == 1)
                        printf("Deleting IP (passive server): %s\n", IP_fd__Server_Passive[i].ip);
    #endif

                        close(fds_from_server[i].fd);
                        // Remove the deleted IP and shift the remaining entries
                        for (int x = i; x < IP_fd__Server_Passive_index - 1; x++) {
                            IP_fd__Server_Passive[x] = IP_fd__Server_Passive[x + 1];
                            fds_from_server[x] = fds_from_server[x + 1];
                        }

                        /* scan for common */
                        for (int x = 0; x < IP_fd__Apps_index; x++) {
                            if (strcmp(IP_fd__Server_Passive[i].ip, IP_fd__Apps[x].ip) == 0){
                                for (int v = x; v < IP_fd__Apps_index - 1; v++){
                                    IP_fd__Apps[v] = IP_fd__Apps[v+1];
                                }
                                break;
                            }
                        }
                        
                        // Close the socket and mark it for removal
                        IP_fd__Apps_index--;
                        IP_fd__Server_Passive_index--;
                        n_fds_from_server--;

    // #if (debug == 1)
    //                     if (nfds_r == -1){
    //                         printf("nfds_r = -1 --> error\n");
    //                     }
    // #endif

                        // Mark the last entry as invalid after shifting
                        //fds_r[nfds_r].fd = -1;
                    }
                }

                else if (bytes_read == 0) {
#if (debug == 1)
                    // Connection closed by client
                    printf("Server disconnected, fd = %d\n", fds_from_server[i].fd);
#endif
                    close(fds_from_server[i].fd);

                    // Remove the deleted IP and shift the remaining entries
                    for (int x = i; x < IP_fd__Server_Passive_index; x++) {
                        IP_fd__Server_Passive[x] = IP_fd__Server_Passive[x + 1];
                        fds_from_server[x] = fds_from_server[x + 1];
                    }

                    /* scan for common */
                    for (int x = 0; x < IP_fd__Apps_index; x++) {
                        if (strcmp(IP_fd__Server_Passive[i].ip, IP_fd__Apps[x].ip) == 0){
                            for (int v = x; v < IP_fd__Apps_index; v++){
                                IP_fd__Apps[v] = IP_fd__Apps[v+1];
                            }
                            break;
                        }
                    }
                    
                    // Close the socket and mark it for removal
                    IP_fd__Apps_index--;
                    IP_fd__Server_Passive_index--;
                    n_fds_from_server--;
                }


                //pthread_mutex_unlock(&client_lock);
                // Clear the revents flag to avoid continuous detection
                fds_from_server[i].revents = 0;
            }
        }

        PrintArr(IP_fd__Apps, IP_fd__Apps_index);
    }
}


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

    while(1){
        printf("Enter the command: ");

        fgets(cmd, sizeof(cmd), stdin); // Reads the entire line, including spaces

        char IP_addr[BUFF_SIZE];
        int port, fd_temp = -1;
        char request[BUFF_SIZE];
        int check = 0;
        char msg[256];
        char port_str[256];
        int s_rev;

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

            PrintArr(IP_fd__Apps, IP_fd__Apps_index);

        }
        else if (strncmp(request, "list", sizeof("list")) == 0){
#if (debug == 1)
            printf(">>>>> List command\n");
#endif
            PrintArr(IP_fd__Apps, IP_fd__Apps_index);
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

                    // if (IP_fd__Apps[i].state_IP == FROM_PASSIVE_SERVER){
                    //     n_fds_from_server--;
                    // }
                    // if (IP_fd__Apps[i].state_IP == FROM_ACTIVE_CLIENT){
                    //     n_fds_from_client--;
                    // }
                    // Mark the last entry as invalid after shifting
                    //fds_from_server[i].fd = -1;
                    //IP_fd__Apps_index--;
                }
            }

            // IP_fd__Apps_index = 1; // only contain of IP of this app

            PrintArr(IP_fd__Apps, IP_fd__Apps_index);
        }
    }
}