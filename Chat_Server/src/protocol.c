#include "protocol.h"

void init_recv_buffer(recv_buffer_t *recv_buf)
{
    if (recv_buf == NULL) {
        return;
    }

    memset(
        recv_buf->buffer,
        0,
        sizeof(recv_buf->buffer)
    );

    recv_buf->data_len = 0;
}

int send_all(
    int fd,
    const char *data,
    size_t len
)
{
    size_t total_sent = 0;

    while (total_sent < len) {
        ssize_t n = send(
            fd,
            data + total_sent,
            len - total_sent,
            MSG_NOSIGNAL
        );

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (n == 0) {
            return -1;
        }

        total_sent += (size_t)n;
    }

    return 0;
}

int send_message(
    int fd,
    const char *msg
)
{
    if (fd < 0 || msg == NULL) {
        return -1;
    }

    size_t msg_len = strlen(msg);

    if (msg_len == 0 ||
        msg_len > MAX_PAYLOAD_SIZE) {
        fprintf(stderr, "消息长度不合法\n");
        return -1;
    }

    uint32_t net_len = htonl((uint32_t)msg_len);

    size_t packet_len = sizeof(net_len) + msg_len;

    char *packet = malloc(packet_len);

    if (packet == NULL) {
        return -1;
    }

    memcpy(
        packet,
        &net_len,
        sizeof(net_len)
    );

    memcpy(
        packet + sizeof(net_len),
        msg,
        msg_len
    );

    int result = send_all(
        fd,
        packet,
        packet_len
    );

    free(packet);

    return result;
}

char *extract_message(
    recv_buffer_t *recv_buf
)
{
    if (recv_buf == NULL ||
        recv_buf->data_len < sizeof(uint32_t)) {
        return NULL;
    }

    uint32_t net_len;

    memcpy(
        &net_len,
        recv_buf->buffer,
        sizeof(net_len)
    );

    uint32_t msg_len = ntohl(net_len);

    if (msg_len == 0 ||
        msg_len > MAX_PAYLOAD_SIZE) {
        fprintf(
            stderr,
            "消息长度不合法: %u\n",
            msg_len
        );

        recv_buf->data_len = 0;
        return NULL;
    }

    size_t packet_len =
        sizeof(uint32_t) + (size_t)msg_len;

    if (recv_buf->data_len < packet_len) {
        return NULL;
    }

    char *msg = malloc((size_t)msg_len + 1);

    if (msg == NULL) {
        return NULL;
    }

    memcpy(
        msg,
        recv_buf->buffer + sizeof(uint32_t),
        msg_len
    );

    msg[msg_len] = '\0';

    size_t remaining =
        recv_buf->data_len - packet_len;

    memmove(
        recv_buf->buffer,
        recv_buf->buffer + packet_len,
        remaining
    );

    recv_buf->data_len = remaining;

    return msg;
}