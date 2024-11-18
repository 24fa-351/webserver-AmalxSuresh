#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "http_message.h"
#include "paths.h"

#define DEFAULT_PORT 62626
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024



void server_dispatch_request(Request* req, int fd) {

    printf("Dispatching request: %s\n", req->path);

    // Route based on the path
    if (strncmp(req -> path, "/calc", 5) == 0) {
        handle_calc(req, fd);
    } else if (strncmp(req -> path, "/static", 7) == 0) {
        handle_static(req, fd);
    } else if (strcmp(req -> path, "/stats") == 0) {
        handle_stats(req, fd);
    } else {
        dprintf(fd, "HTTP/1.1 404 Not Found\r\n\r\n");
    }
}

void handle_connection(int* socket_fd_ptr) {
    int socket_fd = *socket_fd_ptr;
    free(socket_fd_ptr);
    printf("handling connection on %d\n", socket_fd);
    while(1) {
        //take user input in server
        Request* req = request_read_from_fd(socket_fd);
        if(req == NULL) {
            break;
        }

        request_print(req);

        server_dispatch_request(req, socket_fd);
        request_free(req);
    }
    printf("Connection closed for %d.\n", socket_fd);
    close(socket_fd);
}

int main(int argc, char* argv[]) {
    int return_value, socket_fd, client_fd, port;

    port = DEFAULT_PORT;

    //custom port number
    if (argc == 3 && strcmp(argv[1], "-p") == 0) {
        port = atoi(argv[2]);
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in socket_address;
    memset(&socket_address, '\0', sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_address.sin_port = htons(port);

    return_value = bind(socket_fd, (struct sockaddr*)&socket_address, sizeof(socket_address));
    if (return_value == -1) {
        perror("Failed to bind socket");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    return_value = listen(socket_fd, LISTEN_BACKLOG);
    if (return_value == -1) {
        perror("Failed to listen on socket");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", port);

    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    while (1) {
        pthread_t thread;

        int* client_fd_buffer = malloc(sizeof(int));
        if (client_fd_buffer == NULL) {
            perror("Failed to allocate memory for client socket");
            continue;
        }

        *client_fd_buffer = accept(socket_fd, (struct sockaddr*)&client_address, &client_address_len);
        if (*client_fd_buffer == -1) {
            perror("Failed to accept connection");
            free(client_fd_buffer);
            continue;
        }

        printf("Accepted connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        if (pthread_create(&thread, NULL, (void*(*)(void*))handle_connection, (void*)client_fd_buffer) != 0) {
            perror("Failed to create thread");
            free(client_fd_buffer);
            close(*client_fd_buffer);
        }
    }

    close(socket_fd);
    return 0;
}
