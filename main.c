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

#define DEFAULT_PORT 62626
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024

int serve_static_file(int socket_fd, const char* file_path) {
    char full_path[BUFFER_SIZE] = "./static";
    strncat(full_path, file_path + strlen("/static"), BUFFER_SIZE - strlen(full_path) - 1);

    FILE* file = fopen(full_path, "rb");
    if (file == NULL) {
        // Send 404 Not Found if the file doesn't exist
        char* not_found_response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        write(socket_fd, not_found_response, strlen(not_found_response));
        return -1;
    }

    // Get file size
    struct stat file_stat;
    if (stat(full_path, &file_stat) != 0) {
        fclose(file);
        return -1;
    }
    int file_size = file_stat.st_size;

    // Send headers
    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", file_size);
    write(socket_fd, header, strlen(header));

    // Send file content
    char buffer[BUFFER_SIZE];
    int read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        write(socket_fd, buffer, read_bytes);
    }

    fclose(file);
    return 0;
}

int respond_to_http_client_message(int socket_fd, http_client_message_t* http_message) {
    if(strncmp(http_message -> path, "/static", 7) == 0) {
        return serve_static_file(socket_fd, http_message -> path);
    } else {
       char* response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        write(socket_fd, response, strlen(response));
        return 0; 
    }
    
}

void handle_connection(int* socket_fd_ptr) {
    int socket_fd = *socket_fd_ptr;
    free(socket_fd_ptr);

    while(1) {
        printf("handling connection on %d\n", socket_fd);
        http_client_message_t* http_message;
        http_read_result_t result;
        // sscanf(BUFFER_SIZE, "%s %s %s", http_message -> method, http_message -> path, http_message -> http_version);
        // if(!)
        read_http_client_message(socket_fd, &http_message, &result);
        if(result == BAD_REQUEST) {
            printf("Bad request\n");
            close(socket_fd);
            return;
        } else if (result == CLOSED_CONNECTION) {
            printf("Closed connection\n");
            close(socket_fd);
            return;
        }

        respond_to_http_client_message(socket_fd, http_message);  
        http_client_message_free(http_message);
    }
    printf("Connection closed for %d.\n", socket_fd);
}

int main(int argc, char* argv[]) {
    int return_value, socket_fd, client_fd, port;

    port = DEFAULT_PORT;

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
