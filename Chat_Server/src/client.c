#include "client.h"
#include "broadcast.h"

#include <cjson/cJSON.h>

client_info_t clients[MAX_CLIENT];
pthread_mutex_t clients_mutex =
    PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int used;
    char username[MAX_USERNAME_LEN + 1];
    char password[MAX_PASSWORD_LEN + 1];
} user_record_t;

static user_record_t users[MAX_USER];

static pthread_mutex_t users_mutex =
    PTHREAD_MUTEX_INITIALIZER;

static int send_json_to_client(
    client_info_t *client,
    cJSON *json
)
{
    if (client == NULL || json == NULL) {
        return -1;
    }

    char *text =
        cJSON_PrintUnformatted(json);

    if (text == NULL) {
        return -1;
    }

    pthread_mutex_lock(
        &client->send_mutex
    );

    int result = send_message(
        client->fd,
        text
    );

    pthread_mutex_unlock(
        &client->send_mutex
    );

    free(text);

    return result;
}

static void send_error(
    client_info_t *client,
    const char *message
)
{
    cJSON *response =
        cJSON_CreateObject();

    if (response == NULL) {
        return;
    }

    cJSON_AddStringToObject(
        response,
        "type",
        "error"
    );

    cJSON_AddBoolToObject(
        response,
        "ok",
        0
    );

    cJSON_AddStringToObject(
        response,
        "message",
        message != NULL ? message : "未知错误"
    );

    send_json_to_client(
        client,
        response
    );

    cJSON_Delete(response);
}

static void send_success(
    client_info_t *client,
    const char *type,
    const char *message
)
{
    cJSON *response =
        cJSON_CreateObject();

    if (response == NULL) {
        return;
    }

    cJSON_AddStringToObject(
        response,
        "type",
        type
    );

    cJSON_AddBoolToObject(
        response,
        "ok",
        1
    );

    cJSON_AddStringToObject(
        response,
        "message",
        message
    );

    /*
     * 不要在这里再次锁 send_mutex。
     * send_json_to_client() 内部已经负责加锁。
     */
    send_json_to_client(
        client,
        response
    );

    cJSON_Delete(response);
}

void client_table_init(void)
{
    memset(
        clients,
        0,
        sizeof(clients)
    );

    memset(
        users,
        0,
        sizeof(users)
    );

    for (int i = 0; i < MAX_CLIENT; i++) {
        clients[i].fd = -1;

        init_recv_buffer(
            &clients[i].recv_buf
        );

        pthread_mutex_init(
            &clients[i].send_mutex,
            NULL
        );
    }
}

int add_client(
    int fd,
    struct sockaddr_in addr
)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (!clients[i].is_active) {
            clients[i].fd = fd;
            clients[i].addr = addr;
            clients[i].is_active = 1;
            clients[i].last_heartbeat =
                time(NULL);

            clients[i].logged_in = 0;
            clients[i].username[0] = '\0';

            if (inet_ntop(
                    AF_INET,
                    &addr.sin_addr,
                    clients[i].ip,
                    sizeof(clients[i].ip)
                ) == NULL) {
                snprintf(
                    clients[i].ip,
                    sizeof(clients[i].ip),
                    "unknown"
                );
            }

            clients[i].port =
                ntohs(addr.sin_port);

            init_recv_buffer(
                &clients[i].recv_buf
            );

            pthread_mutex_unlock(
                &clients_mutex
            );

            return i;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    return -1;
}

void remove_client(int fd)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].is_active &&
            clients[i].fd == fd) {
            clients[i].is_active = 0;
            clients[i].last_heartbeat = 0;
            clients[i].logged_in = 0;
            clients[i].username[0] = '\0';
            clients[i].fd = -1;
            clients[i].port = 0;
            clients[i].ip[0] = '\0';
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

int find_client_index(int fd)
{
    int index = -1;

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].is_active &&
            clients[i].fd == fd) {
            index = i;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    return index;
}

client_info_t *get_client_by_index(int index)
{
    if (index < 0 ||
        index >= MAX_CLIENT) {
        return NULL;
    }

    return &clients[index];
}

static int register_user(
    const char *username,
    const char *password
)
{
    if (username == NULL ||
        password == NULL) {
        return -1;
    }

    if (username[0] == '\0' ||
        password[0] == '\0') {
        return -1;
    }

    if (strlen(username) > MAX_USERNAME_LEN ||
        strlen(password) > MAX_PASSWORD_LEN) {
        return -1;
    }

    pthread_mutex_lock(&users_mutex);

    int free_index = -1;

    for (int i = 0; i < MAX_USER; i++) {
        if (users[i].used &&
            strcmp(
                users[i].username,
                username
            ) == 0) {
            pthread_mutex_unlock(
                &users_mutex
            );

            return -2;
        }

        if (!users[i].used &&
            free_index == -1) {
            free_index = i;
        }
    }

    if (free_index == -1) {
        pthread_mutex_unlock(
            &users_mutex
        );

        return -3;
    }

    users[free_index].used = 1;

    snprintf(
        users[free_index].username,
        sizeof(users[free_index].username),
        "%s",
        username
    );

    snprintf(
        users[free_index].password,
        sizeof(users[free_index].password),
        "%s",
        password
    );

    /*
     * 成功路径也必须解锁。
     */
    pthread_mutex_unlock(&users_mutex);

    return 0;
}

static int check_login(
    const char *username,
    const char *password
)
{
    if (username == NULL ||
        password == NULL) {
        return -1;
    }

    pthread_mutex_lock(&users_mutex);

    for (int i = 0; i < MAX_USER; i++) {
        if (users[i].used &&
            strcmp(
                users[i].username,
                username
            ) == 0 &&
            strcmp(
                users[i].password,
                password
            ) == 0) {
            pthread_mutex_unlock(
                &users_mutex
            );

            return 0;
        }
    }

    pthread_mutex_unlock(&users_mutex);

    return -1;
}

/*
 * 调用者必须已经持有 clients_mutex。
 */
static client_info_t *find_online_user_locked(
    const char *username
)
{
    if (username == NULL) {
        return NULL;
    }

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].is_active &&
            clients[i].logged_in &&
            strcmp(
                clients[i].username,
                username
            ) == 0) {
            return &clients[i];
        }
    }

    return NULL;
}

static void broadcast_leave_message(
    client_info_t *client,
    int client_fd
)
{
    char username[MAX_USERNAME_LEN + 1];
    char ip[INET_ADDRSTRLEN];
    int logged_in = 0;

    pthread_mutex_lock(&clients_mutex);

    logged_in = client->logged_in;

    snprintf(
        username,
        sizeof(username),
        "%s",
        client->username
    );

    snprintf(
        ip,
        sizeof(ip),
        "%s",
        client->ip
    );

    pthread_mutex_unlock(&clients_mutex);

    cJSON *leave =
        cJSON_CreateObject();

    if (leave == NULL) {
        return;
    }

    cJSON_AddStringToObject(
        leave,
        "type",
        "system"
    );

    cJSON_AddStringToObject(
        leave,
        "message",
        "客户端离开聊天室"
    );

    if (logged_in &&
        username[0] != '\0') {
        cJSON_AddStringToObject(
            leave,
            "username",
            username
        );
    } else {
        cJSON_AddStringToObject(
            leave,
            "username",
            ip
        );
    }

    char *leave_text =
        cJSON_PrintUnformatted(leave);

    if (leave_text != NULL) {
        broadcast_message(
            leave_text,
            client_fd
        );

        free(leave_text);
    }

    cJSON_Delete(leave);
}

void *client_thread(void *arg)
{
    if (arg == NULL) {
        return NULL;
    }

    int client_fd = *(int *)arg;
    free(arg);

    int index =
        find_client_index(client_fd);

    if (index < 0) {
        close(client_fd);
        return NULL;
    }

    client_info_t *client =
        get_client_by_index(index);

    if (client == NULL) {
        close(client_fd);
        return NULL;
    }

    printf(
        "客户端连接成功，IP: %s，端口: %d\n",
        client->ip,
        client->port
    );

    init_recv_buffer(
        &client->recv_buf
    );

    char welcome_msg[1024];

    snprintf(
        welcome_msg,
        sizeof(welcome_msg),
        "{\"type\":\"system\","
        "\"message\":\"客户端进入聊天室\","
        "\"ip\":\"%s\","
        "\"port\":%d}",
        client->ip,
        client->port
    );

    broadcast_message(
        welcome_msg,
        client_fd
    );

    char buf[1024];

    while (1) {
        ssize_t recv_len = recv(
            client_fd,
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
                "客户端断开连接，fd=%d\n",
                client_fd
            );

            break;
        }

        if (client->recv_buf.data_len +
                (size_t)recv_len >
            RECV_BUFFER_SIZE) {
            fprintf(
                stderr,
                "接收缓冲区溢出\n"
            );

            break;
        }

        memcpy(
            client->recv_buf.buffer +
                client->recv_buf.data_len,
            buf,
            (size_t)recv_len
        );

        client->recv_buf.data_len +=
            (size_t)recv_len;

        char *msg;

        while ((msg = extract_message(
                    &client->recv_buf
                )) != NULL) {
            cJSON *root =
                cJSON_Parse(msg);

            if (root == NULL) {
                send_error(
                    client,
                    "JSON格式错误"
                );

                free(msg);
                continue;
            }

            cJSON *type_item =
                cJSON_GetObjectItemCaseSensitive(
                    root,
                    "type"
                );

            if (!cJSON_IsString(type_item) ||
                type_item->valuestring == NULL) {
                send_error(
                    client,
                    "缺少有效的type字段"
                );

                cJSON_Delete(root);
                free(msg);
                continue;
            }

            const char *type =
                type_item->valuestring;

            if (strcmp(type, "heartbeat") == 0) {
                pthread_mutex_lock(
                    &clients_mutex
                );

                if (client->is_active) {
                    client->last_heartbeat =
                        time(NULL);
                }

                pthread_mutex_unlock(
                    &clients_mutex
                );

                cJSON *response =
                    cJSON_CreateObject();

                if (response != NULL) {
                    cJSON_AddStringToObject(
                        response,
                        "type",
                        "heartbeat_ack"
                    );

                    send_json_to_client(
                        client,
                        response
                    );

                    cJSON_Delete(response);
                }

                cJSON_Delete(root);
                free(msg);
                continue;
            }

            if (strcmp(type, "register") == 0) {
                cJSON *username_item =
                    cJSON_GetObjectItemCaseSensitive(
                        root,
                        "username"
                    );

                cJSON *password_item =
                    cJSON_GetObjectItemCaseSensitive(
                        root,
                        "password"
                    );

                if (!cJSON_IsString(username_item) ||
                    !cJSON_IsString(password_item) ||
                    username_item->valuestring == NULL ||
                    password_item->valuestring == NULL) {
                    send_error(
                        client,
                        "注册参数不完整"
                    );

                    cJSON_Delete(root);
                    free(msg);
                    continue;
                }

                int result = register_user(
                    username_item->valuestring,
                    password_item->valuestring
                );

                if (result == 0) {
                    send_success(
                        client,
                        "register_result",
                        "注册成功"
                    );
                } else if (result == -2) {
                    send_error(
                        client,
                        "用户名已经存在"
                    );
                } else if (result == -3) {
                    send_error(
                        client,
                        "用户数量已满"
                    );
                } else {
                    send_error(
                        client,
                        "注册参数不合法"
                    );
                }

                cJSON_Delete(root);
                free(msg);
                continue;
            }

            if (strcmp(type, "login") == 0) {
                cJSON *username_item =
                    cJSON_GetObjectItemCaseSensitive(
                        root,
                        "username"
                    );

                cJSON *password_item =
                    cJSON_GetObjectItemCaseSensitive(
                        root,
                        "password"
                    );

                if (!cJSON_IsString(username_item) ||
                    !cJSON_IsString(password_item) ||
                    username_item->valuestring == NULL ||
                    password_item->valuestring == NULL) {
                    send_error(
                        client,
                        "登录参数不完整"
                    );

                    cJSON_Delete(root);
                    free(msg);
                    continue;
                }

                if (check_login(
                        username_item->valuestring,
                        password_item->valuestring
                    ) != 0) {
                    send_error(
                        client,
                        "用户名或密码错误"
                    );

                    cJSON_Delete(root);
                    free(msg);
                    continue;
                }

                int already_online = 0;

                pthread_mutex_lock(
                    &clients_mutex
                );

                for (int i = 0; i < MAX_CLIENT; i++) {
                    if (clients[i].is_active &&
                        clients[i].logged_in &&
                        strcmp(
                            clients[i].username,
                            username_item->valuestring
                        ) == 0) {
                        already_online = 1;
                        break;
                    }
                }

                if (!already_online) {
                    client->logged_in = 1;

                    snprintf(
                        client->username,
                        sizeof(client->username),
                        "%s",
                        username_item->valuestring
                    );
                }

                pthread_mutex_unlock(
                    &clients_mutex
                );

                if (already_online) {
                    send_error(
                        client,
                        "该用户已经在线"
                    );
                } else {
                    send_success(
                        client,
                        "login_result",
                        "登录成功"
                    );
                }

                cJSON_Delete(root);
                free(msg);
                continue;
            }

            if (strcmp(type, "logout") == 0) {
                pthread_mutex_lock(
                    &clients_mutex
                );

                client->logged_in = 0;
                client->username[0] = '\0';

                pthread_mutex_unlock(
                    &clients_mutex
                );

                send_success(
                    client,
                    "logout_result",
                    "退出登录成功"
                );

                cJSON_Delete(root);
                free(msg);
                continue;
            }

            int logged_in = 0;

            pthread_mutex_lock(
                &clients_mutex
            );

            if (client->is_active &&
                client->logged_in) {
                logged_in = 1;
            }

            pthread_mutex_unlock(
                &clients_mutex
            );

            if (!logged_in) {
                send_error(
                    client,
                    "请先登录"
                );

                cJSON_Delete(root);
                free(msg);
                continue;
            }

            if (strcmp(type, "chat") == 0) {
                cJSON *content_item =
                    cJSON_GetObjectItemCaseSensitive(
                        root,
                        "content"
                    );

                if (!cJSON_IsString(content_item) ||
                    content_item->valuestring == NULL ||
                    content_item->valuestring[0] == '\0') {
                    send_error(
                        client,
                        "聊天内容不能为空"
                    );

                    cJSON_Delete(root);
                    free(msg);
                    continue;
                }

                char username[
                    MAX_USERNAME_LEN + 1
                ];

                pthread_mutex_lock(
                    &clients_mutex
                );

                snprintf(
                    username,
                    sizeof(username),
                    "%s",
                    client->username
                );

                pthread_mutex_unlock(
                    &clients_mutex
                );

                cJSON *chat =
                    cJSON_CreateObject();

                if (chat != NULL) {
                    cJSON_AddStringToObject(
                        chat,
                        "type",
                        "chat"
                    );

                    cJSON_AddStringToObject(
                        chat,
                        "from",
                        username
                    );

                    cJSON_AddStringToObject(
                        chat,
                        "content",
                        content_item->valuestring
                    );

                    char *chat_text =
                        cJSON_PrintUnformatted(chat);

                    if (chat_text != NULL) {
                        broadcast_message(
                            chat_text,
                            client_fd
                        );

                        free(chat_text);
                    }

                    cJSON_Delete(chat);
                }

                cJSON_Delete(root);
                free(msg);
                continue;
            }

            if (strcmp(type, "private_chat") == 0) {
                cJSON *to_item =
                    cJSON_GetObjectItemCaseSensitive(
                        root,
                        "to"
                    );

                cJSON *content_item =
                    cJSON_GetObjectItemCaseSensitive(
                        root,
                        "content"
                    );

                if (!cJSON_IsString(to_item) ||
                    !cJSON_IsString(content_item) ||
                    to_item->valuestring == NULL ||
                    content_item->valuestring == NULL ||
                    to_item->valuestring[0] == '\0' ||
                    content_item->valuestring[0] == '\0') {
                    send_error(
                        client,
                        "私聊参数不完整"
                    );

                    cJSON_Delete(root);
                    free(msg);
                    continue;
                }

                char sender_name[
                    MAX_USERNAME_LEN + 1
                ];

                char target_name[
                    MAX_USERNAME_LEN + 1
                ];

                snprintf(
                    sender_name,
                    sizeof(sender_name),
                    "%s",
                    client->username
                );

                pthread_mutex_lock(
                    &clients_mutex
                );

                client_info_t *target =
                    find_online_user_locked(
                        to_item->valuestring
                    );

                if (target == NULL) {
                    pthread_mutex_unlock(
                        &clients_mutex
                    );

                    send_error(
                        client,
                        "目标用户不在线"
                    );

                    cJSON_Delete(root);
                    free(msg);
                    continue;
                }

                snprintf(
                    target_name,
                    sizeof(target_name),
                    "%s",
                    target->username
                );

                cJSON *private_msg =
                    cJSON_CreateObject();

                if (private_msg != NULL) {
                    cJSON_AddStringToObject(
                        private_msg,
                        "type",
                        "private_chat"
                    );

                    cJSON_AddStringToObject(
                        private_msg,
                        "from",
                        sender_name
                    );

                    cJSON_AddStringToObject(
                        private_msg,
                        "to",
                        target_name
                    );

                    cJSON_AddStringToObject(
                        private_msg,
                        "content",
                        content_item->valuestring
                    );

                    /*
                     * 此处仍持有 clients_mutex，
                     * 防止 target 槽位在发送前被复用。
                     * send_json_to_client 只获取 target 的 send_mutex。
                     */
                    send_json_to_client(
                        target,
                        private_msg
                    );

                    cJSON_Delete(private_msg);
                }

                pthread_mutex_unlock(
                    &clients_mutex
                );

                cJSON_Delete(root);
                free(msg);
                continue;
            }

            send_error(
                client,
                "不支持的消息类型"
            );

            cJSON_Delete(root);
            free(msg);
        }
    }

    broadcast_leave_message(
        client,
        client_fd
    );

    remove_client(client_fd);

    close(client_fd);

    printf(
        "客户端线程结束，fd=%d\n",
        client_fd
    );

    return NULL;
}

void *heartbeat_monitor_thread(void *arg)
{
    (void)arg;

    while (1) {
        sleep(HEARTBEAT_CHECK_INTERVAL);

        time_t now = time(NULL);

        pthread_mutex_lock(
            &clients_mutex
        );

        for (int i = 0; i < MAX_CLIENT; i++) {
            if (!clients[i].is_active) {
                continue;
            }

            time_t inactive_time =
                now - clients[i].last_heartbeat;

            if (inactive_time >
                HEARTBEAT_TIMEOUT) {
                printf(
                    "客户端心跳超时，IP:%s，端口号:%d\n",
                    clients[i].ip,
                    clients[i].port
                );

                /*
                 * 不在这里 remove_client。
                 * shutdown 会使 client_thread 的 recv 返回，
                 * 最终由 client_thread 统一清理。
                 */
                shutdown(
                    clients[i].fd,
                    SHUT_RDWR
                );
            }
        }

        pthread_mutex_unlock(
            &clients_mutex
        );
    }

    return NULL;
}