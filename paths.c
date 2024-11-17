#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include "paths.h"
#include "http_message.h"

ServerStats server_stats = {0, 0, 0};

void handle_static(Request* req, int client_fd) {
    char file_path[1024] = "./static";
    strncat(file_path, req->path + 7, sizeof(file_path) - strlen(file_path) - 1); // Skip "/static"

    FILE* file = fopen(file_path, "rb");
    if (!file) {
        dprintf(client_fd, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Send HTTP headers
    dprintf(client_fd, "HTTP/1.1 200 OK\r\n");
    dprintf(client_fd, "Content-Length: %ld\r\n", file_size);
    dprintf(client_fd, "Content-Type: application/octet-stream\r\n\r\n");

    // Send file content
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        write(client_fd, buffer, bytes_read);
        server_stats.bytes_sent += bytes_read;
    }

    fclose(file);
}


void handle_stats(Request* req, int client_fd) {
    char response[1024];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body>"
        "<h1>Server Stats</h1>"
        "<p>Total Requests: %d</p>"
        "<p>Bytes Received: %lld</p>"
        "<p>Bytes Sent: %lld</p>"
        "</body></html>",
        server_stats.total_requests, server_stats.bytes_received, server_stats.bytes_sent);

    write(client_fd, response, strlen(response));
    server_stats.bytes_sent += strlen(response);
}

void handle_calc(Request* req, int client_fd) {
    int a = 0, b = 0;
    sscanf(req->path, "/calc/%d/%d", &a, &b);

    int sum = a + b;

    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Sum: %d\n", sum);

    write(client_fd, response, strlen(response));
    server_stats.bytes_sent += strlen(response);
}