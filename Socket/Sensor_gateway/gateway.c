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
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <mysql/mysql.h>
#include <stdbool.h>
#include <signal.h>

#define LISTEN_BACKLOG 50 // for server
#define BUFF_SIZE 256

/* >>>>>>>>>>>>>>>>>>>>>>>>>> MODIFY PORT + IP <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
#define SERVER_PORT 2000 
const char IP_APP[16] = "192.168.100.77";

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

/*  ========================================= Bug SQL fail 2 times and suceed next times ======================================================  */
#define test_SQL_bug1 0

/*  ========================================= Bug SQL fail 3 times ======================================================  */
#define test_SQL_bug2 1

/*  ========================================= Debug ======================================================  */
#define debug 0

/*  ========================================= Variables ======================================================  */
struct pollfd fds_from_sensors[100];
int n_fds_from_sensors = 1;

typedef struct {
    int fd;
    char ip[INET_ADDRSTRLEN];
    int port;
    int ID;
} Info;
Info IP_fd__Sensor[100];
int IP_fd__Sensor_index = 1;

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int ID_known;
    float pre_avg_temperature;
    bool intoSQL;
} ID_kn;
ID_kn ID_known[100];
int ID_known_index = 1;

struct sockaddr_in serv_addr, client_addr;
pthread_t ConnectionManager_id, DataManager_id, StorageManager_id;

char sendbuff[BUFF_SIZE];
char recvbuff[BUFF_SIZE];

pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_ConnectionManager = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_DataManager = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_StorageManager = PTHREAD_COND_INITIALIZER;

float temperature;

typedef struct temperature_info{
    float avg_temperature;
    float pre_avg_temperature;
    int index;
    int *logFifo_fd;
}ti;

int index_sensor_used;
int ID_known_index_captured;

/* Database part */
const char database_name[] = "GATEWAY_INFO" ;
const char table_name[] = "Sensor_temperature"; 
#define FIFO_PATH "./logFifo"

/* Global MYSQL */
MYSQL *con;
const char user[] = "vinh";
const char hostname[] = "localhost";
char password[] = "1234";

/* SQL handling error */
#define MAX_TRY 3
int count_try;

/*  ========================================= Sub functions ======================================================  */
void finish_with_error(MYSQL *con) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
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

void cleanup(int signum) {
    // Unlink (remove) the FIFO
    unlink(FIFO_PATH);

#if (debug == 1)
    printf("FIFO unlinked and program terminated\n");
#endif
    exit(EXIT_SUCCESS);
}
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
#if (debug == 1)
    printf("server_fd_temp = %d\n", server_fd_temp);
#endif
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

    printf("Gateway is listening at port: %d\n....\n", SERVER_PORT);
}

void *ConnectionManager(void *arg){
    int new_socket_fd_temp = 0;
    int recvbuff_i = 0;
    ti* internal_temp = (ti*)arg;
    while (1){
        int poll_count = poll(fds_from_sensors, n_fds_from_sensors, -1);
#if (debug == 1)
        printf("CLIENT HANDLING...\n");
#endif
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

                    /* Check ID */
                    int flag_ID_known = 0;
                    for (int v = 0; v < ID_known_index; v++){
                        if (strcmp(IP_fd__Sensor[IP_fd__Sensor_index].ip, ID_known[v].ip) == 0){
                            printf("The sensor node already with ID = %d\n", ID_known[v].ID_known);
                            IP_fd__Sensor[IP_fd__Sensor_index].ID = ID_known[v].ID_known;
                            flag_ID_known = 1;
                            break;
                        }
                    }
                    if (0 == flag_ID_known){
                        printf("The sensor node with new ID = %d\n", ID_known_index);
                        /* Save new ID */
                        strcpy(ID_known[ID_known_index].ip, IP_fd__Sensor[IP_fd__Sensor_index].ip);
                        ID_known[ID_known_index].ID_known = ID_known_index;
                        ID_known[ID_known_index].intoSQL = false;
                        /* Save new ID into IP_fd__Sensor*/
                        IP_fd__Sensor[IP_fd__Sensor_index].ID = ID_known_index;

                        ID_known_index++; // update ID
                    }

                    char buffer[256];
                    sprintf(buffer, "ConnectionManager ==> A sensor node with ID %d has opened a new connection\n", IP_fd__Sensor[IP_fd__Sensor_index].ID);
                    int buffer_len = strlen(buffer);
#if (debug == 1)
                    printf("buffer_len = %d\n", buffer_len);
#endif
                    /* WRITE */
                    int write_bytes;
                    //printf("*(internal_temp->logFifo_fd) = %d\n", *(internal_temp->logFifo_fd));
                    write_bytes = write(*(internal_temp->logFifo_fd), buffer, buffer_len);
                    if (-1 == write_bytes){
                        perror("error writing\n");
                    }

                    n_fds_from_sensors++;
                    IP_fd__Sensor_index++;

                    printf("Accepted a connection from sensor[%s]\n", IP_fd__Sensor[IP_fd__Sensor_index-1].ip);
                }

                else {
                    // printf("Wait to read...\n");
                    ssize_t bytes_read = read(fds_from_sensors[i].fd, recvbuff, sizeof(recvbuff));
                    if (bytes_read == -1) {
                        handle_error("read (server)");
                    }

                    if (bytes_read > 0){
                        //printf("Msg[%d] from sensor[%s]: '%s'\n", recvbuff_i, IP_fd__Sensor[i].ip, recvbuff);
                        //printf("Received buffer: %s\n", recvbuff);

                        sscanf(recvbuff, "Avergae temperature: %f oC", &temperature);
                        printf(" ==> temperature: %.2f\n", temperature);
                        
                        for (int v = 1; v < ID_known_index; v++){
                            if (strcmp(ID_known[v].ip, IP_fd__Sensor[i].ip) == 0){
                                ID_known_index_captured = v;
                            }
                        }
#if (debug == 1)
                        printf("ID_known_index_captured = %d\n", ID_known_index_captured);
#endif
                        if (temperature <= 0){
                            /* Do nothing */
                        }
                        else {
                            if (false == ID_known[ID_known_index_captured].intoSQL){
                                internal_temp->avg_temperature = temperature;
                            }
                            else {
                                internal_temp->avg_temperature = (temperature + ID_known[ID_known_index_captured].pre_avg_temperature)/2.0;
                            }

                            ID_known[ID_known_index_captured].pre_avg_temperature = temperature;
                        }

                        printf("\nConnectionManager ==> Average temperature: %.2f\n", internal_temp->avg_temperature);
                        internal_temp->pre_avg_temperature = temperature;
                        internal_temp->index++;

                        recvbuff_i++;

                        /* Get sensor index */
                        index_sensor_used = i;

                        pthread_cond_signal(&cond_DataManager);  // Signal thread DataManager to run
                    }
                    else if (bytes_read == 0) {

                        char buffer[256];
                        sprintf(buffer, "A sensor node with ID %d has closed the connection\n", IP_fd__Sensor[i].ID);
                        /* WRITE */
                        int write_bytes;
                        //printf("*(internal_temp->logFifo_fd) = %d\n", *(internal_temp->logFifo_fd));
                        write_bytes = write(*(internal_temp->logFifo_fd), buffer, sizeof(buffer));
                        if (-1 == write_bytes){
                            perror("error writing\n");
                        }

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
                //fds_from_sensors[i].revents = 0;
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
        

        char buffer[256];
        int write_bytes;

        if (temperature <= 0){
            sprintf(buffer, "DataManager ==> Received sensor data with invalid sensor node ID %d\n", IP_fd__Sensor[index_sensor_used].ID);
        
            int buffer_len = strlen(buffer);
            write_bytes = write(*(internal_temp->logFifo_fd), buffer, buffer_len);
            //printf("write_bytes = %d\n", write_bytes);
            if (-1 == write_bytes){
                perror("error writing\n");
            }
        }
        else {
            sprintf(buffer, "DataManager ==> The sensor node with ID %d reports it’s too cold (running avg temperature = %.2f)\n", IP_fd__Sensor[index_sensor_used].ID, internal_temp->avg_temperature);
            // Process the data after being signaled
            printf("DataManager ==> Average temperature: %.2f\n", internal_temp->avg_temperature);

            int buffer_len = strlen(buffer);
            write_bytes = write(*(internal_temp->logFifo_fd), buffer, buffer_len);
            //printf("write_bytes = %d\n", write_bytes);
            if (-1 == write_bytes){
                perror("error writing\n");
            }

            /* Signal storage manager */
            pthread_cond_signal(&cond_StorageManager); 
        }

        pthread_mutex_unlock(&client_lock);  // Unlock mutex after processing
    }

    return NULL;
}

void *StorageManager(void *arg){

    pthread_mutex_lock(&client_lock);  // Lock mutex before waiting

    pthread_cond_wait(&cond_StorageManager, &client_lock);

    ti* internal_temp = (ti*)arg;

    con = mysql_init(NULL);
    char temp_buffer[256];

    char buffer_log[256];
    int write_bytes;
    int buffer_len;

    int check = 0;

    if (con == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

#if (test_SQL_bug2|test_SQL_bug1 == 1)
    /* Hostname incorrect */
    strcpy(password, "9999");
#endif

    int flag_success = 0;
    for (int try = 0; try < MAX_TRY; try++){
        /* Successful */
        if (mysql_real_connect(con, hostname, user, password, NULL, 0, NULL, 0) != NULL){
            flag_success = 1;
            break;
        }
        if (flag_success == 0){
            printf(" *Attemp %d to log into SQL\n", try);
            sleep(1); /* wait 1 second to log into SQL */

#if (test_SQL_bug1 == 1)
            /* Hostname incorrect */
            if (try == 1) strcpy(password, "1234");
#endif
        }
    }


    /* if log into SQL successfully */
    if (1 == flag_success){
        /* WRITE to Log */
        sprintf(buffer_log, "StorageManager ==> Connection to SQL server established\n");
        buffer_len = strlen(buffer_log);
        write_bytes = write(*(internal_temp->logFifo_fd), buffer_log, buffer_len);
        if (-1 == write_bytes){
            perror("error writing\n");
        }

        // Create the database
        memset(temp_buffer, 0, sizeof(temp_buffer));
        sprintf(temp_buffer, "CREATE DATABASE IF NOT EXISTS %s", database_name);
        if (mysql_query(con, temp_buffer)) {
            finish_with_error(con);
        }

        // Select the database
        memset(temp_buffer, 0, sizeof(temp_buffer));
        sprintf(temp_buffer, "USE %s", database_name);
        if (mysql_query(con, temp_buffer)) {
            finish_with_error(con);
        }

        // Drop the table if it exists
        memset(temp_buffer, 0, sizeof(temp_buffer));
        sprintf(temp_buffer, "DROP TABLE IF EXISTS %s", table_name);
        if (mysql_query(con, temp_buffer)) {
            finish_with_error(con);
        }

        // Create a new table
        memset(temp_buffer, 0, sizeof(temp_buffer));
        sprintf(temp_buffer, "CREATE TABLE %s(Id INT PRIMARY KEY AUTO_INCREMENT, Temperature FLOAT)", table_name);
        if (mysql_query(con, temp_buffer)) {
            finish_with_error(con);
        }
        /* WRITE */
        sprintf(buffer_log, "StorageManager ==> New table %s created\n", table_name);
        buffer_len = strlen(buffer_log);
        write_bytes = write(*(internal_temp->logFifo_fd), buffer_log, buffer_len);
        if (-1 == write_bytes){
            perror("error writing\n");
        }

        pthread_mutex_unlock(&client_lock);  

        while (1) {
            pthread_mutex_lock(&client_lock);  // Lock mutex before waiting

            // Keep waiting until signaled, and use a loop to recheck the condition
            if (check != 0) pthread_cond_wait(&cond_StorageManager, &client_lock);  // Wait for the signal from DataManager
            check = 1;
            
            // Process the data after being signaled
            printf("StorageManager ==> Store temperature: %.2f\n", internal_temp->avg_temperature);

            /* **** MYSQL Part **** */
            if (ID_known[ID_known_index_captured].intoSQL == false){
                // Insert data into the table
                memset(temp_buffer, 0, sizeof(temp_buffer));
                sprintf(temp_buffer, "INSERT INTO %s(Temperature) VALUES(%.2f)", table_name, internal_temp->avg_temperature);
                if (mysql_query(con, temp_buffer)) {
                    finish_with_error(con);
                }
                ID_known[ID_known_index_captured].intoSQL = true;
#if (debug == 1)
                printf("INSERT...\n");
#endif
            }
            else {
                // Insert data into the table
                memset(temp_buffer, 0, sizeof(temp_buffer));
                sprintf(temp_buffer, "UPDATE %s SET Temperature = %.2f WHERE Id = %d", table_name, internal_temp->avg_temperature, ID_known_index_captured);
                if (mysql_query(con, temp_buffer)) {
                    finish_with_error(con);
                }
#if (debug == 1)
                printf("UPDATE...\n");
#endif
            }
            /* **** ********** **** */

            /* WRITE */
            sprintf(buffer_log, "StorageManager ==> The sensor node with ID %d into sql\n", IP_fd__Sensor[index_sensor_used].ID);
            //printf("*(internal_temp->logFifo_fd) = %d\n", *(internal_temp->logFifo_fd));
            int buffer_len = strlen(buffer_log);
            write_bytes = write(*(internal_temp->logFifo_fd), buffer_log, buffer_len);
            //printf("write_bytes = %d\n", write_bytes);
            if (-1 == write_bytes){
                perror("error writing\n");
            }

            pthread_mutex_unlock(&client_lock);  // Unlock mutex after processing
        }
    }
    else {
        printf("Fail to log into SQL\n");

        /* WRITE to Log */
        sprintf(buffer_log, "StorageManager ==> Unable to connect to SQL server\n");
        buffer_len = strlen(buffer_log);
        write_bytes = write(*(internal_temp->logFifo_fd), buffer_log, buffer_len);
        if (-1 == write_bytes){
            perror("error writing\n");
        }
#if (debug == 1)
        printf("n_fds_from_sensors = %d\n", n_fds_from_sensors);
#endif
        for (int i = 1; i < n_fds_from_sensors; i++) {
#if (debug == 1)
            printf("exit -> %d\n",fds_from_sensors[i].fd );
#endif
            if (write(fds_from_sensors[i].fd, "minus", sizeof("minus")) == -1)
                handle_error("write()");
            usleep(50000);
        }

        write_bytes = write(*(internal_temp->logFifo_fd), "Kill", 5);
        //printf("write_bytes = %d\n", write_bytes);
        if (-1 == write_bytes){
            perror("error writing\n");
        }

        /* Wait the log process (last process) delete and unlink FIFO */
        sleep(2);
        exit(0);
    }
    return NULL;   
}

int main(){
    // Set up the signal handler to catch SIGINT (Ctrl+C)
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    /* Forking */
    pid_t Log_process;

    // Create the FIFO (named pipe)
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        if (errno != EEXIST) {  // Ignore the error if the FIFO already exists
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
    int logFifo_fd = open(FIFO_PATH, O_RDWR|O_CREAT);

    ti info_temp;

    /* Set values */
    info_temp.index = 0;
    info_temp.logFifo_fd = &logFifo_fd;

    /* The info of server of app1 */
    strncpy(IP_fd__Sensor[0].ip, IP_APP, sizeof(IP_APP));
    IP_fd__Sensor[0].port = SERVER_PORT;

    /* Set up server part (not accepting)*/
    server_func1_listen(IP_fd__Sensor);   

    /* ConnectionManager handling sensor connecting */
    if (pthread_create(&ConnectionManager_id, NULL, &ConnectionManager, &info_temp) != 0) {
        handle_error("pthread_create()");
    }

    /* DataManager handling after ConnectionManager */
    if (pthread_create(&DataManager_id, NULL, &DataManager, &info_temp) != 0) {
        handle_error("pthread_create()");
    }

    /* StorageManager handling after ConnectionManager */
    if (pthread_create(&StorageManager_id, NULL, &StorageManager, &info_temp) != 0) {
        handle_error("pthread_create()");
    }

    usleep(5000);

    /* Log process */
    if((Log_process = fork()) == 0){
        char logFifo_buffer[256];

        int logFifo_fd = open(FIFO_PATH, O_RDONLY);

        struct pollfd fds_from_logFifo;
        fds_from_logFifo.fd = logFifo_fd;
        fds_from_logFifo.events = POLLIN;

        /* Write to gateway.log */
        int log_fd;
        /* OPEN */
        log_fd = open("./gateway.log", O_RDWR|O_CREAT|O_APPEND, 0644);
        if (-1 == log_fd){
            perror("error opening or creating\n");
        }   

        while(1){
            /* Polling */
            //printf(" >>> fds_from_logFifo.fd = %d\n", fds_from_logFifo.fd);
            int poll_count = poll(&fds_from_logFifo, 1, -1);
            //printf("Handling here ..... <<<\n");

            if (poll_count == -1) {
                perror("poll error");
                exit(EXIT_FAILURE);
            }

            pthread_mutex_lock(&log_lock);
            if (fds_from_logFifo.revents & POLLIN){
                ssize_t bytes_read = read(fds_from_logFifo.fd, logFifo_buffer, sizeof(logFifo_buffer));
                if (bytes_read > 0) {
                    //lseek(fds_from_log->fd, bytes_read, SEEK_SET);  // Move to the end of the last read
                    logFifo_buffer[bytes_read] = '\0';  // Null-terminate the buffer
                    //printf("==> Log_process ==> '%s'\n", logFifo_buffer);

                    if (strcmp(logFifo_buffer, "Kill") == 0){
                        bytes_read = 0;
                    }
                }
                if (bytes_read == 0) {
                    logFifo_buffer[bytes_read] = '\0';  // Null-terminate the buffer
                    //printf("==> Log_process ==> '%s'\n", logFifo_buffer);
                    printf("Log file descriptor closed\n");
                    close(fds_from_logFifo.fd);
                    unlink(FIFO_PATH);
                    exit(0);
                    break;
                }

                /* WRITE */
                int write_bytes;
                int buffer_len = strlen(logFifo_buffer);
                write_bytes = write(log_fd, logFifo_buffer, buffer_len);
                if (-1 == write_bytes){
                    perror("error writing\n");
                }            


            }
            pthread_mutex_unlock(&log_lock);
        }
        exit(0);
    }

    /* For exit manually */
    while(1){
        char cmd[200];
        fgets(cmd, sizeof(cmd), stdin); // Reads the entire line, including spaces
        char request[BUFF_SIZE];
        char temp1[BUFF_SIZE], temp2[BUFF_SIZE];

        (void)splitString(cmd, request, temp1, temp2);

        if (strncmp(request, "exit", sizeof("exit")) == 0){
            for (int i = 1; i < n_fds_from_sensors; i++) {
                if (write(fds_from_sensors[i].fd, "minus", sizeof("minus")) == -1)
                    handle_error("write()");
                usleep(50000);
            }

        }
    }

    /* clean up */
    pthread_mutex_destroy(&client_lock);
    pthread_cond_destroy(&cond_ConnectionManager);
    pthread_cond_destroy(&cond_DataManager);
    pthread_cond_destroy(&cond_StorageManager);

    close(logFifo_fd);
    unlink(FIFO_PATH);
}

// gcc test.c -o t `mysql_config --cflags --libs`