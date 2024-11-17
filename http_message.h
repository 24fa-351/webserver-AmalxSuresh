#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

typedef struct {
    char* key;
    char* value;
} Header;
typedef struct {
    char* method;
    char* path;
    char* http_version;
    int header_count;
    char* body;
    Header* headers;
} Request;

Request* request_read_from_fd(int fd);

void request_print(Request* req);

void request_free(Request* req);

// void read_http_client_message(int client_socket, http_client_message_t** msg, http_read_result_t* result);
// void http_client_message_free(http_client_message_t* msg);
//bool is_complete_http_message(char* buffer);
#endif