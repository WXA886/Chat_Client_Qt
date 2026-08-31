#include "server.h"
#include "client.h"

int create_server_socket(int port)
{
    int fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (fd < 0) {
        perror("socket创建失败");
        return -1;
    }

    int opt = 1;

    if (setsockopt(
            fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)
        ) < 0) {
        perror("setsockopt失败");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;

    memset(
        &addr,
        0,
        sizeof(addr)
    );

    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(
            fd,
            (struct sockaddr *)&addr,
            sizeof(addr)
        ) < 0) {
        perror("bind失败");
        close(fd);
        return -1;
    }

    if (listen(fd, 10) < 0) {
        perror("listen失败");
        close(fd);
        return -1;
    }

    return fd;
}

void server_run(int server_fd)
{
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len =
            sizeof(client_addr);

        int client_fd = accept(
            server_fd,
            (struct sockaddr *)&client_addr,
            &client_addr_len
        );

        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("accept失败");
            continue;
        }

        if (add_client(
                client_fd,
                client_addr
            ) < 0) {
            const char *full_msg =
                "{\"type\":\"error\","
                "\"ok\":false,"
                "\"message\":\"服务器已满\"}";

            send_message(
                client_fd,
                full_msg
            );

            close(client_fd);
            continue;
        }

        int *arg = malloc(sizeof(*arg));

        if (arg == NULL) {
            remove_client(client_fd);
            close(client_fd);
            continue;
        }

        *arg = client_fd;

        pthread_t tid;

        if (pthread_create(
                &tid,
                NULL,
                client_thread,
                arg
            ) != 0) {
            perror("pthread_create失败");

            free(arg);
            remove_client(client_fd);
            close(client_fd);
            continue;
        }

        pthread_detach(tid);
    }
}