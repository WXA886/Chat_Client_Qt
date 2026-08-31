#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char buffer[RECV_BUFFER_SIZE];
    size_t data_len;
} recv_buffer_t;

void init_recv_buffer(recv_buffer_t *recv_buf);

int send_all(int fd, const char *data, size_t len);

int send_message(int fd, const char *msg);

char *extract_message(recv_buffer_t *recv_buf);

#ifdef __cplusplus
}
#endif

#endif
