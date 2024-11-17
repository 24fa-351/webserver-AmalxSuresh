#ifndef HTTP_PATH_H
#define HTTP_PATH_H
#include "http_message.h"

//extern ServerStats server_stats;

typedef struct {
    int total_requests;     // Total number of requests
    long long bytes_received;  // Total bytes received
    long long bytes_sent;      // Total bytes sent
} ServerStats;


void handle_calc(Request* req, int client_fd);
void handle_stats(Request* req, int client_fd);
void handle_static(Request* req, int client_fd);

#endif