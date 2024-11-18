#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include "http_message.h"
#include "paths.h"

ServerStats server_stats = {0, 0, 0};

bool read_request_line(Request* req, int fd);
bool read_headers(Request* req, int fd);
bool read_body(Request* req, int fd);

Request* request_read_from_fd(int fd) {
    printf("reading request\n");
   
    Request* req = malloc(sizeof(Request));
     
    if(read_request_line(req, fd) == false) {
        printf("failed to read request line\n");
        request_free(req);
        return NULL;
    }
    if(read_headers(req, fd) == false) {
        printf("failed to read headers\n");
        request_free(req);
        return NULL;
    }
    if(read_body(req, fd) == false) {
        printf("failed to read body\n");
        request_free(req);
        return NULL;
    }

    return req;
}

//Print all the information that pertains to the request made by user
void request_print(Request* req) {
    printf("printing request\n");
    if(req -> method) {
        printf("Method: %s\n", req -> method);
    }
    if(req -> path) {
        printf("Path: %s\n", req -> path);
    }
    if(req -> http_version) {
        printf("Version: %s\n", req -> http_version);
    }
    for(int ix = 0; ix < req -> header_count; ix++) {
        printf("header %d: %s: %s\n", ix, req -> headers[ix].key, req -> headers[ix].value);
    }
    
    printf("request donezo\n");
}

//free memeory if pointer isn't null
#define FREE_IF_NOT_NULL(ptr) if (ptr) {free(ptr);}
void request_free(Request* req) {
    printf("Freeing request\n");
    FREE_IF_NOT_NULL(req -> method);
    FREE_IF_NOT_NULL(req -> path);
    FREE_IF_NOT_NULL(req -> http_version);

    for(int ix = 0; ix < req -> header_count; ix ++) {
        FREE_IF_NOT_NULL(req -> headers[ix].key);
        FREE_IF_NOT_NULL(req -> headers[ix].value); 
    }
    FREE_IF_NOT_NULL(req -> headers);

    free(req);
}

//read single line from fd
char* read_line(int fd) {
    printf("reading line from fd %d\n", fd);
    char* line = malloc(10000);
    int len_read = 0;
    
    while(1) {
        char ch;
        int number_bytes_read = read(fd, &ch, 1);
        if(number_bytes_read <= 0) {
            return NULL;
        }
        server_stats.bytes_received += number_bytes_read;
        if(ch == '\n') {
            break;
        }
        line[len_read] = ch;
        len_read++;
        line[len_read] = '\0';
    }
    if(len_read > 0 && line[len_read - 1] == '\r') {
        line[len_read - 1] = '\0';
    }
    line = realloc(line, len_read + 1);
    return line;
}

//use read_line function to read the input line and extract the method, path, and HTTP version
bool read_request_line(Request* req, int fd) {
    printf("reading request line\n");
    char* line = read_line(fd);
    if(line == NULL) {
        return false;
    }
    req -> method = malloc(strlen(line) + 1);
    req -> path = malloc(strlen(line) + 1);
    req -> http_version = malloc(strlen(line) + 1);
    int length_parsed;
    int number_parsed;

    number_parsed = sscanf(line, "%s %s %s%n", req -> method, req -> path, req -> http_version, &length_parsed);

    if(number_parsed != 3 || length_parsed != strlen(line)) {
        printf("Failed to parse request line\n");
        free(line);
        return false;
    }

    if(strcmp(req -> method, "GET") != 0 && strcmp(req -> method, "POST") != 0) {
        printf("Invalid method: %s\n", req -> method);
        free(line);
        return false;
    }

    return true;
}


bool read_headers(Request* req, int fd) {
    printf("reading headers\n");
    req -> headers = malloc(sizeof(Header) * 100);
    req -> header_count = 0;
    while(1) {
        char* line = read_line(fd);
        if(line == NULL) {
            return false;
        }

        server_stats.bytes_received += strlen(line) + 1;

        if (strlen(line) == 0) {
            free(line);
            break;
        }
        req -> headers[req -> header_count].key = malloc(10000);
        req -> headers[req -> header_count].value = malloc(10000);

        int length_parsed;
        int number_parsed;

        number_parsed = sscanf(line, "%[^:]: %s%n", req -> headers[req -> header_count].key, req -> headers[req -> header_count].value, &length_parsed);
        if(number_parsed != 2 || length_parsed != strlen(line)) {
            printf("failed to parse header\n");
            free(line);
            return false;
        }

        req -> headers[req -> header_count].key = realloc(req -> headers[req -> header_count].key, strlen(req -> headers[req -> header_count].key) + 1);
        req -> headers[req -> header_count].value = realloc(req -> headers[req -> header_count].value, strlen(req -> headers[req -> header_count].value) + 1);
        req -> header_count++;
        printf("yo %s\n", req -> headers[req -> header_count].key);
        free(line);
    }
    return true;
}

bool read_body(Request* req, int fd) {
    printf("Reading body\n");
    // Find Content-Length header
    int content_length = 0;
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, "Content-Length") == 0) {
            content_length = atoi(req->headers[i].value);
            break;
        }
    }

    if (content_length > 0) {
        req->body = malloc(content_length + 1); // Allocate memory for body
        if (req->body == NULL) {
            printf("Failed to allocate memory for body\n");
            return false;
        }

        int total_read = 0;
        while (total_read < content_length) {
            int bytes_read = read(fd, req->body + total_read, content_length - total_read);
            if (bytes_read <= 0) {
                printf("Failed to read body\n");
                free(req->body);
                req->body = NULL;
                return false;
            }
            total_read += bytes_read;
            server_stats.bytes_received += bytes_read;
        }
        req->body[content_length] = '\0'; // Null-terminate the body
        return true;
    } else {
        printf("No Content-Length or body is empty\n");
        req -> body = NULL; // No body
        return true;
    }
}