#ifndef SERVER_H
#define SERVER_H

#include "common.h"

int create_server_socket(int port);

void server_run(int server_fd);

#endif