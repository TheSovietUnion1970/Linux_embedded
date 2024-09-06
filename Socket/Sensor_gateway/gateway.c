#include <stdlib.h> //standard stuff
#include <sys/mman.h> //mmap()
#include <stdio.h> //io stuff
#include <unistd.h> //sleep()
#include <semaphore.h> //semaphore()
#include <time.h> //time()
#include <errno.h>
#include <sys/socket.h>     //  Chứa cấu trúc cần thiết cho socket. 
#include <netinet/in.h>     //  Thư viện chứa các hằng số, cấu trúc khi sử dụng địa chỉ trên internet
#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h> //mmap()
#include <bits/mman-linux.h> //MAP_ANONYMOUS

#define LISTEN_BACKLOG 50 // for server
#define BUFF_SIZE 256

/* >>>>>>>>>>>>>>>>>>>>>>>>>> MODIFY PORT + IP <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
#define SERVER_PORT 2000 
const char IP_APP[16] = "192.168.30.61";

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

/*  ========================================= Variables ======================================================  */
struct pollfd fds_from_sensors[100];
int n_fds_from_sensors = 1;

typedef struct {
    int fd;
    char ip[INET_ADDRSTRLEN];
    int port;
} Info;
Info IP_fd__Sensor[100];
int IP_fd__Sensor_index = 1;

struct sockaddr_in serv_addr, client_addr;
pthread_t server_id1;

char sendbuff[BUFF_SIZE];
char recvbuff[BUFF_SIZE];

pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_ConnectionManager = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_DataManager = PTHREAD_COND_INITIALIZER;

float temperature;

typedef struct temperature_info{
    float avg_temperature;
    float pre_avg_temperature;
    int index;
}ti;

/*  ========================================= Server part ======================================================  */
int len;
int server_fd_temp;
void server_func1_listen(void *arg){
    Info* info_sensor = (Info*)arg;

    int portno;
    int opt = 1;

    portno = info_sensor->port;

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
    fds_from_sensors[0].fd = server_fd_temp;
    fds_from_sensors[0].events = POLLIN;

    /* Save this server into  IP_fd__Sensor */
    IP_fd__Sensor[0].fd = server_fd_temp;
    strcpy(IP_fd__Sensor[0].ip, IP_APP);

    printf("Server is listening at port: %d\n....\n", SERVER_PORT);
}

void *ConnectionManager(void *arg){
    int new_socket_fd_temp = 0;
    int recvbuff_i = 0;
    ti* internal_temp = (ti*)arg;
    while (1){
        int poll_count = poll(fds_from_sensors, n_fds_from_sensors, -1);
        memset(recvbuff, 0, sizeof(recvbuff));
        if (poll_count == -1) {
            handle_error("poll()");
        }

        for (int i = 0; i < n_fds_from_sensors; i++){
            pthread_mutex_lock(&client_lock);
            if (fds_from_sensors[i].revents & POLLIN){
                //printf("i = %d\n", i);
                if (fds_from_sensors[i].fd == IP_fd__Sensor[0].fd){
                    new_socket_fd_temp = accept(IP_fd__Sensor[0].fd, (struct sockaddr*)&client_addr, &len);
                    if (new_socket_fd_temp == -1) {
                        handle_error("accept()");
                    }

                    /* store new fd */
                    fds_from_sensors[n_fds_from_sensors].fd = new_socket_fd_temp;
                    fds_from_sensors[n_fds_from_sensors].events = POLLIN;

                    /* store new sensor node */
                    IP_fd__Sensor[IP_fd__Sensor_index].fd = new_socket_fd_temp;
                    inet_ntop(AF_INET, &client_addr.sin_addr, IP_fd__Sensor[IP_fd__Sensor_index].ip, INET_ADDRSTRLEN);

                    n_fds_from_sensors++;
                    IP_fd__Sensor_index++;

                    printf("Accepted a connection from sensor[%s]\n", IP_fd__Sensor[IP_fd__Sensor_index-1].ip);
                }

                else {
                    ssize_t bytes_read = read(fds_from_sensors[i].fd, recvbuff, sizeof(recvbuff));
                    if (fds_from_sensors[i].fd == -1) {
                        handle_error("read (server)");
                    }

                    if (bytes_read > 0){
                        //printf("Msg[%d] from sensor[%s]: '%s'\n", recvbuff_i, IP_fd__Sensor[i].ip, recvbuff);
                        sscanf(recvbuff, "Avergae temperature: %f oC", &temperature);
                        //printf(" ==> temperature: %.2f\n", temperature);
                        
                        if (0 == internal_temp->index){
                            internal_temp->avg_temperature = temperature;
                        }
                        else {
                            internal_temp->avg_temperature = (temperature + internal_temp->pre_avg_temperature)/2;
                        }
                        printf("\nConnectionManager ==> Average temperature: %.2f\n", internal_temp->avg_temperature);
                        internal_temp->pre_avg_temperature = temperature;
                        internal_temp->index++;

                        recvbuff_i++;
                        pthread_cond_signal(&cond_DataManager);  // Signal thread DataManager to run
                    }
                    else if (bytes_read == 0) {
                        printf("Sensor[%s] is disconnected\n", IP_fd__Sensor[i].ip);
                        close(fds_from_sensors[i].fd);

                        /* Moving the remaining sensors to the left */
                        for (int v = i; v < n_fds_from_sensors - 1; v++){
                            IP_fd__Sensor[v] = IP_fd__Sensor[v+1];
                            fds_from_sensors[v] = fds_from_sensors[v+1];
                        }

                        n_fds_from_sensors--;
                        IP_fd__Sensor_index--;
                    }
                }
            fds_from_sensors[i].revents = 0;
            }
            pthread_mutex_unlock(&client_lock);
        }
    }
}

void *DataManager(void *arg) {
    ti* internal_temp = (ti*)arg;

    while (1) {
        pthread_mutex_lock(&client_lock);  // Lock mutex before waiting

        // Keep waiting until signaled, and use a loop to recheck the condition
        pthread_cond_wait(&cond_DataManager, &client_lock);  // Wait for the signal from ConnectionManager
        
        // Process the data after being signaled
        printf("DataManager ==> Average temperature: %.2f\n", internal_temp->avg_temperature);

        pthread_mutex_unlock(&client_lock);  // Unlock mutex after processing
    }

    return NULL;
}

int main(){
    /* Forking */
    pid_t Log_process;
    /* For process locking*/
    sem_t *mutex = (sem_t*)mmap(NULL, sizeof(sem_t*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_t *IsGetting = (sem_t*)mmap(NULL, sizeof(sem_t*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_t *IsNothing = (sem_t*)mmap(NULL, sizeof(sem_t*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(mutex, 1, 1);
    sem_init(IsNothing, 1, 1);
    sem_init(IsGetting, 1, 0);

    /* Shared memory cpy value */
    ti* info_temp = (ti*)mmap(NULL, sizeof(ti*)*256, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    /* Reset temp_index */
    info_temp->index = 0;

    /* The info of server of app1 */
    strncpy(IP_fd__Sensor[0].ip, IP_APP, sizeof(IP_APP));
    IP_fd__Sensor[0].port = SERVER_PORT;

    /* Set up server part (not accepting)*/
    server_func1_listen(IP_fd__Sensor);   

    /* ConnectionManager handling sensor connecting */
    if (pthread_create(&server_id1, NULL, &ConnectionManager, info_temp) != 0) {
        handle_error("pthread_create()");
    }

    /* ConnectionManager handling after ConnectionManager */
    if (pthread_create(&server_id1, NULL, &DataManager, info_temp) != 0) {
        handle_error("pthread_create()");
    }

    // if((Log_process = fork()) == 0){
    //     while(1){
    //         dddd
    //     }
    // }

    while(1);

    /* clean up */
    pthread_mutex_destroy(&client_lock);
    pthread_cond_destroy(&cond_ConnectionManager);
    pthread_cond_destroy(&cond_DataManager);
}