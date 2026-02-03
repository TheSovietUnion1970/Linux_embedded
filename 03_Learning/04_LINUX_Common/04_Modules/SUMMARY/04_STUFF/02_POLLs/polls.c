/*
 * Simple TCP echo server demonstrating select(), poll(), and epoll
 * Compile with one of these flags:
 *
 *   gcc -o server server.c -DUSE_SELECT
 *   gcc -o server server.c -DUSE_POLL
 *   gcc -o server server.c -DUSE_EPOLL
 *
 * Default: uses epoll if none defined
 *
 * Run: ./server
 * Then connect with: nc localhost 8080
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#ifdef USE_EPOLL
#include <sys/epoll.h>
#elif defined(USE_POLL)
#include <poll.h>
#else
#include <sys/select.h>
#endif

#define PORT            8080
#define MAX_CLIENTS     64
#define BUFFER_SIZE     1024
#define BACKLOG         10

// Set socket to non-blocking mode
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// ==================== SELECT version ====================
#ifdef USE_SELECT

int main_select() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    fd_set readfds, allfds;
    int max_fd;
    int i;

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port %d (using select)...\n", PORT);

    FD_ZERO(&allfds);
    FD_SET(server_fd, &allfds);
    max_fd = server_fd;

    while (1) {
        readfds = allfds;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity == -1) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // New connection?
        if (FD_ISSET(server_fd, &readfds)) {
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                    perror("accept");
                continue;
            }

            set_nonblocking(client_fd);
            FD_SET(client_fd, &allfds);
            if (client_fd > max_fd) max_fd = client_fd;

            char addr_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
            printf("New connection from %s:%d\n", addr_str, ntohs(client_addr.sin_port));
        }

        // Check all clients for data
        for (i = 0; i <= max_fd; i++) {
            if (i == server_fd) continue;
            if (!FD_ISSET(i, &readfds)) continue;

            char buffer[BUFFER_SIZE];
            ssize_t n = read(i, buffer, sizeof(buffer));

            if (n <= 0) {
                // Connection closed or error
                if (n == 0) {
                    printf("Client disconnected\n");
                } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                }
                close(i);
                FD_CLR(i, &allfds);
                if (i == max_fd) max_fd--;  // optional optimization
            } else {
                // Echo back
                write(i, buffer, n);
                buffer[n] = '\0';
                printf("Echoed: %s", buffer);
            }
        }
    }

    close(server_fd);
    return 0;
}

#endif // USE_SELECT

// ==================== POLL version ====================
#ifdef USE_POLL

int main_poll() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;  // server socket

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port %d (using poll)...\n", PORT);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        // Check server socket for new connections
        if (fds[0].revents & POLLIN) {
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                    perror("accept");
                continue;
            }

            set_nonblocking(client_fd);

            if (nfds < MAX_CLIENTS + 1) {
                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;
                nfds++;
                printf("New connection accepted (fd=%d)\n", client_fd);
            } else {
                printf("Too many clients\n");
                close(client_fd);
            }
        }

        // Check all client sockets
        for (int i = 1; i < nfds; i++) {
            if (!(fds[i].revents & POLLIN)) continue;

            char buffer[BUFFER_SIZE];
            ssize_t n = read(fds[i].fd, buffer, sizeof(buffer));

            if (n <= 0) {
                if (n == 0) {
                    printf("Client disconnected (fd=%d)\n", fds[i].fd);
                } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                }
                close(fds[i].fd);
                // Remove from array (simple shift)
                fds[i] = fds[nfds-1];
                nfds--;
                i--;  // re-check this slot
            } else {
                write(fds[i].fd, buffer, n);
                buffer[n] = '\0';
                printf("Echoed: %s", buffer);
            }
        }
    }

    close(server_fd);
    return 0;
}

#endif // USE_POLL

// ==================== EPOLL version ====================
#ifdef USE_EPOLL

int main_epoll() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    int epoll_fd;
    struct epoll_event ev, events[MAX_CLIENTS];

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    printf("Server listening on port %d (using epoll)...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_CLIENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                // New connection
                client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                        perror("accept");
                    continue;
                }

                set_nonblocking(client_fd);

                ev.events = EPOLLIN | EPOLLET;  // edge-triggered
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl: client_fd");
                    close(client_fd);
                    continue;
                }

                printf("New connection accepted (fd=%d)\n", client_fd);
            } else {
                // Client data
                char buffer[BUFFER_SIZE];
                ssize_t n;

                // Edge-triggered: read until no more data
                while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
                    write(fd, buffer, n);
                    buffer[n] = '\0';
                    printf("Echoed: %s", buffer);
                }

                if (n == 0 || (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                    printf("Client disconnected (fd=%d)\n", fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    return 0;
}

#endif // USE_EPOLL

int main() {
#if defined(USE_SELECT)
    return main_select();
#elif defined(USE_POLL)
    return main_poll();
#else
    return main_epoll();
#endif
}