#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "http_message.h"

bool is_complete_http_message(char* buffer) {
    if(strlen(buffer) < 4) {
        return false;
    }
    if(strncmp(buffer, "GET ", 4) != 0) {
        return false;
    }
     if (strstr(buffer, "\r\n\r\n") != NULL || strstr(buffer, "\n\n") != NULL) {
        return true;
    }

    return false;
}

void read_http_client_message(int client_socket, http_client_message_t** msg, http_read_result_t* result) {
    *msg = malloc(sizeof(http_client_message_t));
    char buffer[1024];
    strcpy(buffer, "");
    int total_bytes = 0;

    while(!is_complete_http_message(buffer)) {
        int read_bytes = read(client_socket, buffer + total_bytes, sizeof(buffer) - total_bytes - 1);
        if(read_bytes == 0) {
            *result = CLOSED_CONNECTION;
            free(*msg);
            return;
        }
        if(read_bytes < 0) {
            *result = BAD_REQUEST;
            free(*msg);
            return;
        }

        total_bytes += read_bytes;
        buffer[total_bytes] = '\0';
    }

    (*msg) -> method = strdup("GET");
    (*msg) -> path = strdup(buffer + 4);
    *result = MESSAGE;
}

void http_client_message_free(http_client_message_t* msg) {
    free(msg->method);
    free(msg->path);
    free(msg);

}
