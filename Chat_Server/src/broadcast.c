#include "broadcast.h"
#include "client.h"

void broadcast_message(
    const char *msg,
    int sender_fd
)
{
    if (msg == NULL) {
        return;
    }

    /*
     * clients_mutex 保护客户端槽位和 fd 生命周期。
     * send_mutex 保护每个客户端的发送操作。
     *
     * 这里保持 clients_mutex -> send_mutex 的固定加锁顺序。
     */
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (!clients[i].is_active ||
            clients[i].fd == sender_fd) {
            continue;
        }

        pthread_mutex_lock(
            &clients[i].send_mutex
        );

        int result = send_message(
            clients[i].fd,
            msg
        );

        pthread_mutex_unlock(
            &clients[i].send_mutex
        );

        if (result < 0) {
            fprintf(
                stderr,
                "广播发送失败，fd=%d\n",
                clients[i].fd
            );

            /*
             * 由对应的 client_thread 负责后续清理。
             */
            shutdown(
                clients[i].fd,
                SHUT_RDWR
            );
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}