#ifndef HTTP_PATH_H
#define HTTP_PATH_H
#include "http_message.h"

typedef struct {
    int total_requests;  
    long long bytes_received; 
    long long bytes_sent;      
} ServerStats;

extern ServerStats server_stats;

void handle_calc(Request* req, int client_fd);
void handle_stats(Request* req, int client_fd);
void handle_static(Request* req, int client_fd);

#endif