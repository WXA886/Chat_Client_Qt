#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "protocol.h"

typedef struct {
    int fd;
    struct sockaddr_in addr;

    int is_active;

    char ip[INET_ADDRSTRLEN];
    int port;

    recv_buffer_t recv_buf;
    time_t last_heartbeat;

    int logged_in;
    char username[MAX_USERNAME_LEN + 1];

    pthread_mutex_t send_mutex;
} client_info_t;

extern client_info_t clients[MAX_CLIENT];
extern pthread_mutex_t clients_mutex;

void client_table_init(void);

int add_client(
    int fd,
    struct sockaddr_in addr
);

void remove_client(int fd);

int find_client_index(int fd);

client_info_t *get_client_by_index(int index);

void *client_thread(void *arg);

void *heartbeat_monitor_thread(void *arg);

#endif