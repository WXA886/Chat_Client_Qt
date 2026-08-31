#include "protocol.h"

#include <cjson/cJSON.h>

typedef struct {
    int fd;
    pthread_mutex_t send_mutex;
    atomic_int running;
} client_context_t;

static int send_json_locked(
    client_context_t *ctx,
    cJSON *json
)
{
    if (ctx == NULL || json == NULL) {
        return -1;
    }

    char *text = cJSON_PrintUnformatted(json);

    if (text == NULL) {
        return -1;
    }

    int result = send_message(
        ctx->fd,
        text
    );

    free(text);

    return result;
}

static int send_chat_message(
    client_context_t *ctx,
    const char *content
)
{
    if (ctx == NULL ||
        content == NULL ||
        content[0] == '\0') {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();

    if (root == NULL) {
        return -1;
    }

    cJSON_AddStringToObject(
        root,
        "type",
        "chat"
    );

    cJSON_AddStringToObject(
        root,
        "content",
        content
    );

    pthread_mutex_lock(
        &ctx->send_mutex
    );

    int result = send_json_locked(
        ctx,
        root
    );

    pthread_mutex_unlock(
        &ctx->send_mutex
    );

    cJSON_Delete(root);

    return result;
}

static int send_register_message(
    client_context_t *ctx,
    const char *username,
    const char *password
)
{
    if (ctx == NULL ||
        username == NULL ||
        password == NULL) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();

    if (root == NULL) {
        return -1;
    }

    cJSON_AddStringToObject(
        root,
        "type",
        "register"
    );

    cJSON_AddStringToObject(
        root,
        "username",
        username
    );

    cJSON_AddStringToObject(
        root,
        "password",
        password
    );

    pthread_mutex_lock(
        &ctx->send_mutex
    );

    int result = send_json_locked(
        ctx,
        root
    );

    pthread_mutex_unlock(
        &ctx->send_mutex
    );

    cJSON_Delete(root);

    return result;
}

static int send_login_message(
    client_context_t *ctx,
    const char *username,
    const char *password
)
{
    if (ctx == NULL ||
        username == NULL ||
        password == NULL) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();

    if (root == NULL) {
        return -1;
    }

    cJSON_AddStringToObject(
        root,
        "type",
        "login"
    );

    cJSON_AddStringToObject(
        root,
        "username",
        username
    );

    cJSON_AddStringToObject(
        root,
        "password",
        password
    );

    pthread_mutex_lock(
        &ctx->send_mutex
    );

    int result = send_json_locked(
        ctx,
        root
    );

    pthread_mutex_unlock(
        &ctx->send_mutex
    );

    cJSON_Delete(root);

    return result;
}

static int send_private_message(
    client_context_t *ctx,
    const char *username,
    const char *content
)
{
    if (ctx == NULL ||
        username == NULL ||
        content == NULL ||
        username[0] == '\0' ||
        content[0] == '\0') {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();

    if (root == NULL) {
        return -1;
    }

    cJSON_AddStringToObject(
        root,
        "type",
        "private_chat"
    );

    cJSON_AddStringToObject(
        root,
        "to",
        username
    );

    cJSON_AddStringToObject(
        root,
        "content",
        content
    );

    pthread_mutex_lock(
        &ctx->send_mutex
    );

    int result = send_json_locked(
        ctx,
        root
    );

    pthread_mutex_unlock(
        &ctx->send_mutex
    );

    cJSON_Delete(root);

    return result;
}

static int send_simple_message(
    client_context_t *ctx,
    const char *type
)
{
    if (ctx == NULL ||
        type == NULL ||
        type[0] == '\0') {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();

    if (root == NULL) {
        return -1;
    }

    cJSON_AddStringToObject(
        root,
        "type",
        type
    );

    pthread_mutex_lock(
        &ctx->send_mutex
    );

    int result = send_json_locked(
        ctx,
        root
    );

    pthread_mutex_unlock(
        &ctx->send_mutex
    );

    cJSON_Delete(root);

    return result;
}

void *pthread_recv(void *arg)
{
    if (arg == NULL) {
        return NULL;
    }

    client_context_t *ctx =
        (client_context_t *)arg;

    recv_buffer_t recv_buf;

    init_recv_buffer(
        &recv_buf
    );

    char buf[2048];

    while (atomic_load(&ctx->running)) {
        ssize_t recv_len = recv(
            ctx->fd,
            buf,
            sizeof(buf),
            0
        );

        if (recv_len < 0) {
            if (errno == EINTR) {
                continue;
            }

            if (errno != EBADF &&
                errno != ENOTCONN &&
                errno != ECONNRESET) {
                perror("recv失败");
            }

            break;
        }

        if (recv_len == 0) {
            printf(
                "\n与服务器断开连接\n"
            );

            break;
        }

        if (recv_buf.data_len +
                (size_t)recv_len >
            RECV_BUFFER_SIZE) {
            fprintf(
                stderr,
                "\n接收缓冲区溢出\n"
            );

            break;
        }

        memcpy(
            recv_buf.buffer +
                recv_buf.data_len,
            buf,
            (size_t)recv_len
        );

        recv_buf.data_len +=
            (size_t)recv_len;

        char *msg;

        while ((msg = extract_message(
                    &recv_buf
                )) != NULL) {
            cJSON *root =
                cJSON_Parse(msg);

            int is_heartbeat_ack = 0;

            if (root != NULL) {
                cJSON *type_item =
                    cJSON_GetObjectItemCaseSensitive(
                        root,
                        "type"
                    );

                if (cJSON_IsString(type_item) &&
                    type_item->valuestring != NULL &&
                    strcmp(
                        type_item->valuestring,
                        "heartbeat_ack"
                    ) == 0) {
                    is_heartbeat_ack = 1;
                }
            }

            /*
             * 心跳确认只用于维持连接，
             * 不显示到终端，避免不断刷屏。
             */
            if (!is_heartbeat_ack) {
                printf(
                    "\n收到服务器消息: %s\n",
                    msg
                );

                /*
                 * 只有主线程负责打印输入提示。
                 * 接收线程不再打印提示，避免重复显示。
                 */
            }

            if (root != NULL) {
                cJSON_Delete(root);
            }

            free(msg);
        }
    }

    atomic_store(
        &ctx->running,
        0
    );

    return NULL;
}

void *heartbeat_thread(void *arg)
{
    if (arg == NULL) {
        return NULL;
    }

    client_context_t *ctx =
        (client_context_t *)arg;

    const char *heartbeat =
        "{\"type\":\"heartbeat\"}";

    while (atomic_load(&ctx->running)) {
        sleep(HEARTBEAT_INTERVAL);

        if (!atomic_load(&ctx->running)) {
            break;
        }

        pthread_mutex_lock(
            &ctx->send_mutex
        );

        int result = send_message(
            ctx->fd,
            heartbeat
        );

        pthread_mutex_unlock(
            &ctx->send_mutex
        );

        if (result < 0) {
            if (errno != EBADF &&
                errno != ENOTCONN &&
                errno != ECONNRESET) {
                perror("心跳发送失败");
            }

            atomic_store(
                &ctx->running,
                0
            );

            shutdown(
                ctx->fd,
                SHUT_RDWR
            );

            break;
        }

        /*
         * 不在这里打印“已发送心跳”和输入提示，
         * 避免与主线程的 fgets 输出互相覆盖。
         */
    }

    return NULL;
}

int main(void)
{
    signal(
        SIGPIPE,
        SIG_IGN
    );

    int fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (fd < 0) {
        perror("socket创建失败");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;

    memset(
        &addr,
        0,
        sizeof(addr)
    );

    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);

    /*
     * 修改为你的服务器公网 IP。
     */
    if (inet_pton(
            AF_INET,
            "43.138.249.232",
            &addr.sin_addr
        ) <= 0) {
        perror("服务器地址错误");
        close(fd);
        return EXIT_FAILURE;
    }

    if (connect(
            fd,
            (struct sockaddr *)&addr,
            sizeof(addr)
        ) < 0) {
        perror("connect连接失败");
        close(fd);
        return EXIT_FAILURE;
    }

    printf(
        "连接服务器成功\n"
    );

    client_context_t ctx;

    ctx.fd = fd;

    atomic_init(
        &ctx.running,
        1
    );

    if (pthread_mutex_init(
            &ctx.send_mutex,
            NULL
        ) != 0) {
        perror("发送锁初始化失败");
        close(fd);
        return EXIT_FAILURE;
    }

    pthread_t recv_tid;
    pthread_t heartbeat_tid;

    if (pthread_create(
            &recv_tid,
            NULL,
            pthread_recv,
            &ctx
        ) != 0) {
        perror("接收线程创建失败");

        pthread_mutex_destroy(
            &ctx.send_mutex
        );

        close(fd);

        return EXIT_FAILURE;
    }

    if (pthread_create(
            &heartbeat_tid,
            NULL,
            heartbeat_thread,
            &ctx
        ) != 0) {
        perror("心跳线程创建失败");

        atomic_store(
            &ctx.running,
            0
        );

        shutdown(
            fd,
            SHUT_RDWR
        );

        pthread_join(
            recv_tid,
            NULL
        );

        pthread_mutex_destroy(
            &ctx.send_mutex
        );

        close(fd);

        return EXIT_FAILURE;
    }

    printf(
        "可用命令:\n"
        "/register 用户名 密码\n"
        "/login 用户名 密码\n"
        "/logout\n"
        "/to 用户名 消息内容\n"
        "/quit\n"
        "其他内容发送群聊\n"
    );

    char msg[1024];

    while (atomic_load(&ctx.running)) {
        printf(
            "请输入命令: "
        );

        fflush(stdout);

        if (fgets(
                msg,
                sizeof(msg),
                stdin
            ) == NULL) {
            break;
        }

        msg[strcspn(
            msg,
            "\n"
        )] = '\0';

        if (msg[0] == '\0') {
            continue;
        }

        int send_result = 0;

        if (strcmp(msg, "/quit") == 0 ||
            strcmp(msg, "exit") == 0) {
            send_result = send_simple_message(
                &ctx,
                "logout"
            );

            if (send_result < 0) {
                perror("退出消息发送失败");
            }

            break;
        }

        if (strcmp(msg, "/logout") == 0) {
            send_result = send_simple_message(
                &ctx,
                "logout"
            );
        } else if (strncmp(
                       msg,
                       "/register ",
                       10
                   ) == 0) {
            char username[
                MAX_USERNAME_LEN + 1
            ];

            char password[
                MAX_PASSWORD_LEN + 1
            ];

            int parsed = sscanf(
                msg + 10,
                "%63s %127s",
                username,
                password
            );

            if (parsed != 2) {
                printf(
                    "用法: /register 用户名 密码\n"
                );

                continue;
            }

            send_result = send_register_message(
                &ctx,
                username,
                password
            );
        } else if (strncmp(
                       msg,
                       "/login ",
                       7
                   ) == 0) {
            char username[
                MAX_USERNAME_LEN + 1
            ];

            char password[
                MAX_PASSWORD_LEN + 1
            ];

            int parsed = sscanf(
                msg + 7,
                "%63s %127s",
                username,
                password
            );

            if (parsed != 2) {
                printf(
                    "用法: /login 用户名 密码\n"
                );

                continue;
            }

            send_result = send_login_message(
                &ctx,
                username,
                password
            );
        } else if (strncmp(
                       msg,
                       "/to ",
                       4
                   ) == 0) {
            char username[
                MAX_USERNAME_LEN + 1
            ];

            char content[1024];

            int parsed = sscanf(
                msg + 4,
                "%63s %1023[^\n]",
                username,
                content
            );

            if (parsed != 2 ||
                content[0] == '\0') {
                printf(
                    "用法: /to 用户名 消息内容\n"
                );

                continue;
            }

            send_result = send_private_message(
                &ctx,
                username,
                content
            );
        } else {
            send_result = send_chat_message(
                &ctx,
                msg
            );
        }

        if (send_result < 0) {
            perror("消息发送失败");
            break;
        }
    }

    atomic_store(
        &ctx.running,
        0
    );

    shutdown(
        ctx.fd,
        SHUT_RDWR
    );

    pthread_join(
        recv_tid,
        NULL
    );

    pthread_join(
        heartbeat_tid,
        NULL
    );

    pthread_mutex_destroy(
        &ctx.send_mutex
    );

    close(ctx.fd);

    return EXIT_SUCCESS;
}