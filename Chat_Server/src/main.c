#include "server.h"
#include "client.h"

int main(void)
{
    signal(SIGPIPE, SIG_IGN);

    client_table_init();

    pthread_t monitor_tid;

    if (pthread_create(
            &monitor_tid,
            NULL,
            heartbeat_monitor_thread,
            NULL
        ) != 0) {
        perror("心跳监控线程创建失败");
        return EXIT_FAILURE;
    }

    pthread_detach(monitor_tid);

    int server_fd =
        create_server_socket(SERVER_PORT);

    if (server_fd < 0) {
        return EXIT_FAILURE;
    }

    printf(
        "服务器启动成功，监听端口: %d\n",
        SERVER_PORT
    );

    server_run(server_fd);

    close(server_fd);

    return EXIT_SUCCESS;
}