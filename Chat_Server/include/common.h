#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define SERVER_PORT 9527

#define MAX_CLIENT 100
#define MAX_USER 100

#define RECV_BUFFER_SIZE 8192
#define MAX_PAYLOAD_SIZE (RECV_BUFFER_SIZE - sizeof(uint32_t))

#define MAX_USERNAME_LEN 63
#define MAX_PASSWORD_LEN 127

#define HEARTBEAT_INTERVAL 10
#define HEARTBEAT_TIMEOUT 30
#define HEARTBEAT_CHECK_INTERVAL 5

#endif