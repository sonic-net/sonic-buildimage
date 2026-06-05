/********************************************************************************
 * Copyright(C) 2020 Micas Network. All rights reserved.
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期        | 作者     |  修改记录
**===============================================================================
**| 2024/08/23  | zhoutenghui  |  创建该文件
**
*********************************************************************************/

/*------------------------------- Includes ----------------------------------*/
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <sys/socket.h>  // 基本套接字功能
#include <sys/un.h>      // AF_UNIX 地址结构和相关功能
#include "sap_socket.h"
#include "sap_common.h"
/*------------------- Global Definitions and Declarations -------------------*/
/*----------------------- Variable Declarations -----------------------------*/
/*----------------------- Function Prototype --------------------------------*/

#define MSG_PRINTF(level, fmt, ...) \
    do { \
        if ((level) <= net_comm_get_debug_level()) { \
            printf("[msg][func:%s line:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
        } \
    } while (0)

#define dzlog_debug(fmt, args...)     MSG_PRINTF(SAP_LOG_LEVEL_DEBUG, fmt, ##args)
#define dzlog_info(fmt, args...)     MSG_PRINTF(SAP_LOG_LEVEL_INFO, fmt, ##args)
#define dzlog_notice(fmt, args...)     MSG_PRINTF(SAP_LOG_LEVEL_NOTICE, fmt, ##args)
#define dzlog_warn(fmt, args...)     MSG_PRINTF(SAP_LOG_LEVEL_WARN, fmt, ##args)
#define dzlog_error(fmt, args...)     MSG_PRINTF(SAP_LOG_LEVEL_ERROR, fmt, ##args)

#define MAX_FD_NUM    32
#define MAX_BUFF_SIZE 1024 * 10
static char read_buff[MAX_BUFF_SIZE];
typedef struct
{
    NET_CONN id;
    int sockfd;
    NET_COMM_SERVICE_INFO_T *service_info;
    NET_COMM_MESSAGE_HANDLE msg_handle;
    NET_COMM_CONNECT_HANDLE conn_handle;
} NET_COMM_OBJECT_T;

typedef enum {
    FD_TYPE_UNUSE     = 0,
    FD_TYPE_LISTENING = 1,
    FD_TYPE_PREPARING = 2,
    FD_TYPE_RUNNING   = 3,
    FD_TYPE_CLOSING   = 4,
    FD_TYPE_CLOSED    = 5 // 等待回收
} FD_TYPE_E;

typedef struct
{
    int index;
    NET_COMM_HEADER_T *header_ptr;
} message_send_t;

static int fd_list[MAX_FD_NUM];
static int8_t fd_type_map[MAX_FD_NUM];
static NET_COMM_OBJECT_T *fd_object_map[MAX_FD_NUM];

static pthread_mutex_t mutex_list[MAX_FD_NUM + 1]; // 最后一把锁用于fd分配的竞争

static int server_socket_init(NET_COMM_OBJECT_T *server);
static int client_socket_init(NET_COMM_OBJECT_T *client);

static NET_COMM_SERVICE_INFO_T *service_info_dup(NET_COMM_SERVICE_INFO_T *service_info);
static void service_info_free(NET_COMM_SERVICE_INFO_T *service_info);
static int service_info_cmp(NET_COMM_SERVICE_INFO_T *service_info_a, NET_COMM_SERVICE_INFO_T *service_info_b); // 相同返回0，否则返回非0

static NET_COMM_OBJECT_T *net_comm_object_dup(NET_COMM_OBJECT_T *obj);
static void net_comm_object_free(NET_COMM_OBJECT_T *obj);

/*----------------------- Function Implement --------------------------------*/
// 返回发送成功的数据长度，-1表示发送失败
int net_comm_message_send(NET_CONN net_comm, char *data, int data_len)
{
    if (net_comm < 0 || net_comm >= MAX_FD_NUM) {
        return -1;
    }
    if (NULL == data || data_len <= 0) {
        return -1;
    }
    pthread_mutex_lock(&mutex_list[net_comm]);
    
    if (FD_TYPE_RUNNING != fd_type_map[net_comm] && FD_TYPE_PREPARING != fd_type_map[net_comm]) {
        pthread_mutex_unlock(&mutex_list[net_comm]);
        dzlog_error("error index, this index can't send message!");
        return -1;
    }
    NET_COMM_OBJECT_T *obj = fd_object_map[net_comm];
    char output[MAX_BUFF_SIZE];
    memset(output, 0, sizeof(output));
    output[0] = 0x5f;
    output[1] = 0x54;
    output[2] = 0x48;
    output[3] = 0x5f;
    uint8_t j = 4;
    uint8_t i;
    uint32_t _input_len = htonl(data_len);
    for (i = 4; i > 0; i--) {
        uint8_t len_p  = _input_len >> ((i - 1) * 8);
        output[11 - j] = len_p;
        j++;
    }
    for (i = 0; i < data_len; i++) {
        output[j++] = data[i];
    }
    int res = send(obj->sockfd, output, j, MSG_DONTWAIT); // MSG_DONTWAIT避免缓冲区满导致阻塞
    // dzlog_debug("send res: %d", res);

    pthread_mutex_unlock(&mutex_list[net_comm]);
    if (res - data_len - NET_COMM_HEADER_LENGTH) {
        return -1;
    }
    return res;
}

static void accept_fd_routine(void *arg)
{
    pthread_detach(pthread_self());
    int *index = (int *)arg;

    pthread_mutex_lock(&mutex_list[*index]);
    if (FD_TYPE_PREPARING == fd_type_map[*index]) {
        NET_COMM_OBJECT_T *client = fd_object_map[*index];
        client->conn_handle(*index, NET_COMM_CONN_STATUS_CONNECTED);
        fd_type_map[*index] = FD_TYPE_RUNNING;
    }
    pthread_mutex_unlock(&mutex_list[*index]);

    free(index);
    pthread_exit(0);
}

static void accept_fd(int server_index)
{
    struct sockaddr_in fromaddr;
    socklen_t len = sizeof(fromaddr);
    int clientfd  = accept(fd_list[server_index], (struct sockaddr *)&fromaddr, &len);
    dzlog_info("client connect accepted, addr: %s", inet_ntoa(fromaddr.sin_addr));

    pthread_mutex_lock(&mutex_list[MAX_FD_NUM]);
    int index = 0;
    while (fd_type_map[index] != FD_TYPE_UNUSE) {
        if (++index < MAX_FD_NUM) {
            continue;
        }
        close(clientfd);
        dzlog_error("no idle fd!");
        pthread_mutex_unlock(&mutex_list[MAX_FD_NUM]);
        return;
    }
    fd_type_map[index] = FD_TYPE_PREPARING;
    pthread_mutex_unlock(&mutex_list[MAX_FD_NUM]);

    NET_COMM_OBJECT_T *client = net_comm_object_dup(fd_object_map[server_index]);
    client->id                = index;
    client->sockfd            = clientfd;

    fd_object_map[index] = client;
    fd_list[index]       = clientfd;

    int *client_index = malloc(sizeof(int));
    *client_index     = index;

    pthread_t thread;
    if (pthread_create(&thread, NULL, (void *)&accept_fd_routine, client_index)) {
        close(clientfd);
        free(client_index);
        fd_type_map[index] = FD_TYPE_UNUSE;
        net_comm_object_free(client); // 警告局域网通讯模块初始化失败
    }
}

static void recv_fd_disconnect_routine(void *arg)
{
    pthread_detach(pthread_self());
    int *index = (int *)arg;

    pthread_mutex_lock(&mutex_list[*index]);
    if (FD_TYPE_CLOSING == fd_type_map[*index]) {
        NET_COMM_OBJECT_T *client = fd_object_map[*index];
        client->conn_handle(*index, NET_COMM_CONN_STATUS_CONNECT_LOST);
        fd_type_map[*index] = FD_TYPE_CLOSED;
    }
    pthread_mutex_unlock(&mutex_list[*index]);

    free(index);
    pthread_exit(0);
}
//
static void recv_fd_message_routine(void *arg)
{
    pthread_detach(pthread_self());
    message_send_t *data = (message_send_t *)arg;

    pthread_mutex_lock(&mutex_list[data->index]);
    if (FD_TYPE_RUNNING == fd_type_map[data->index]) {
        NET_COMM_OBJECT_T *client = fd_object_map[data->index];
        client->msg_handle(data->index, (char *)&((data->header_ptr)->payload), (data->header_ptr)->length); // 调用消息回调函数
    }
    pthread_mutex_unlock(&mutex_list[data->index]);

    free(data->header_ptr);
    free(data);
    pthread_exit(0);
}

static void recv_fd(int index)
{
    NET_COMM_OBJECT_T *client = fd_object_map[index];
    int count                 = recv(client->sockfd, read_buff, sizeof(read_buff), 0);
    if (count == 0 && FD_TYPE_RUNNING == fd_type_map[index]) {
        int *client_index = malloc(sizeof(int));
        *client_index     = index;
        pthread_t thread;
        fd_type_map[index] = FD_TYPE_CLOSING; // 防止重复触发
        if (pthread_create(&thread, NULL, (void *)&recv_fd_disconnect_routine, client_index)) {
            free(client_index);
            dzlog_warn("call disconnect handle function failed!");
        }
        dzlog_info("client [%d] connect closed!", index);
        return;
    }
    int i = 0, package_count = 0;
    while (i < count) {
        if ((uint8_t)read_buff[i] != 0x5f) {
            i++;
            continue;
        }
        if ((uint8_t)read_buff[i + 1] != 0x54) {
            i++;
            continue;
        }
        if ((uint8_t)read_buff[i + 2] != 0x48) {
            i++;
            continue;
        }
        if ((uint8_t)read_buff[i + 3] != 0x5f) {
            i++;
            continue;
        }
        uint32_t length = read_buff[i + 4];
        length += read_buff[i + 5] << 8;
        length += read_buff[i + 6] << 16;
        length += read_buff[i + 7] << 24;
        uint32_t _length = ntohl(length);
        _length += NET_COMM_HEADER_LENGTH;
        // dzlog_debug("package_length = %ld", _length);
        package_count++;

        NET_COMM_HEADER_T *header_ptr = malloc(_length);
        memcpy(header_ptr, read_buff + i, _length); // 注意大小，如果大小端错误，length过长，会挂机
        header_ptr->length = _length - NET_COMM_HEADER_LENGTH;

        message_send_t *data = (message_send_t *)malloc(sizeof(message_send_t));
        data->index          = index;
        data->header_ptr     = header_ptr;

        pthread_t thread;
        if (pthread_create(&thread, NULL, (void *)&recv_fd_message_routine, data)) {
            free(header_ptr);
            free(data);
            dzlog_warn("call message handle function failed!");
        }
        i++;
    }
    dzlog_debug("recvfrom [%d], bufflen: %d, package_num: %d", index, count, package_count);
}

static void fd_select_routine(void *arg)
{
    int i, nfds;
    fd_set readfds;
    struct timeval timeout;
    while (1) {
        nfds = 0;
        FD_ZERO(&readfds);
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;
        for (i = 0; i < MAX_FD_NUM; i++) {
            pthread_mutex_lock(&mutex_list[i]);
            if (fd_type_map[i] == FD_TYPE_CLOSED) {
                close(fd_list[i]);
                net_comm_object_free(fd_object_map[i]);
                fd_type_map[i] = FD_TYPE_UNUSE;
                pthread_mutex_unlock(&mutex_list[i]);
                continue;
            }
            if (fd_type_map[i] == FD_TYPE_UNUSE || fd_type_map[i] == FD_TYPE_PREPARING) {
                pthread_mutex_unlock(&mutex_list[i]);
                continue;
            }
            pthread_mutex_unlock(&mutex_list[i]);
            if (fd_list[i] >= nfds) {
                nfds = fd_list[i] + 1;
            }
            FD_SET(fd_list[i], &readfds);
        }
        if (select(nfds, &readfds, NULL, NULL, &timeout) <= 0) {
            continue;
        }
        for (i = 0; i < MAX_FD_NUM; ++i) {
            if (!FD_ISSET(fd_list[i], &readfds)) {
                continue;
            }
            pthread_mutex_lock(&mutex_list[i]);
            switch (fd_type_map[i]) {
            case FD_TYPE_LISTENING:
                accept_fd(i);
                break;
            case FD_TYPE_RUNNING:
                recv_fd(i);
                break;
            case FD_TYPE_CLOSED:
                close(fd_list[i]);
                net_comm_object_free(fd_object_map[i]);
                fd_type_map[i] = FD_TYPE_UNUSE;
                break;
            default:
                break;
            }
            pthread_mutex_unlock(&mutex_list[i]);
        }
    }
}

void net_comm_init(void)
{
    int i;
    pthread_mutexattr_t _attr;
    pthread_mutexattr_init(&_attr);
    pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);
    // mdns_init(net_card);
    for (i = 0; i < MAX_FD_NUM; i++) {
        fd_type_map[i] = FD_TYPE_UNUSE;
        pthread_mutex_init(&mutex_list[i], &_attr);
    }
    pthread_mutex_init(&mutex_list[MAX_FD_NUM], &_attr);
    pthread_t thread;
    if (pthread_create(&thread, NULL, (void *)&fd_select_routine, NULL)) {
        dzlog_error("fd_select_routine thread create failed!");
        return; // 警告局域网通讯模块初始化失败
    }
}

NET_CONN net_comm_server_publish(NET_COMM_SERVICE_INFO_T *service_info, NET_COMM_MESSAGE_HANDLE msg_handle, NET_COMM_CONNECT_HANDLE conn_handle)
{
    if (NULL == service_info || NULL == msg_handle) {
        return -1;
    }
    if (NULL == service_info->service_name || NULL == service_info->host_name || service_info->port <= 0) {
        return -1;
    }
    pthread_mutex_lock(&mutex_list[MAX_FD_NUM]);
    int index = 0;
    while (fd_type_map[index] != FD_TYPE_UNUSE) {
        if (++index < MAX_FD_NUM) {
            continue;
        }
        pthread_mutex_unlock(&mutex_list[MAX_FD_NUM]);
        dzlog_error("no idle fd!");
        return -1;
    }
    fd_type_map[index] = FD_TYPE_PREPARING;
    pthread_mutex_unlock(&mutex_list[MAX_FD_NUM]);
    NET_COMM_OBJECT_T *server = (NET_COMM_OBJECT_T *)malloc(sizeof(NET_COMM_OBJECT_T));
    memset(server, 0, sizeof(NET_COMM_OBJECT_T));

    server->msg_handle   = msg_handle;
    server->conn_handle  = conn_handle;
    server->service_info = service_info_dup(service_info);
    server->id           = index;
    server->sockfd       = server_socket_init(server);

    if (-1 == server->sockfd) {
        fd_type_map[server->id] = FD_TYPE_UNUSE;
        net_comm_object_free(server);
        return -1;
    }
    fd_object_map[server->id] = server;
    fd_list[server->id]       = server->sockfd;
    fd_type_map[server->id]   = FD_TYPE_LISTENING;
    return index;
}

static int server_socket_init(NET_COMM_OBJECT_T *server)
{
    struct sockaddr_un server_addr;
    int sockfd = (int)socket(AF_UNIX, SOCK_STREAM, 0);
    /* 删除旧的套接字文件（如果存在）*/
    unlink(server->service_info->host_name);
    /* 填充服务器地址（sockaddr_un）*/
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, server->service_info->host_name, sizeof(server_addr.sun_path) - 1);
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(sockfd, 100);
    return sockfd;
}

/* 返回sock */
static int client_socket_init(NET_COMM_OBJECT_T *client)
{
    int client_fd;
    struct sockaddr_un server_addr;
    /* 创建客户端套接字 */
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        return client_fd;
    }
    /* 填充服务器地址 */
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, client->service_info->host_name, sizeof(server_addr.sun_path) - 1);
    /* 连接到服务器 */
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        close(client_fd);
        return -1;
    }
    return client_fd;
}

static void client_connect_routine(void *arg)
{
    pthread_detach(pthread_self());
    NET_COMM_OBJECT_T *client = (NET_COMM_OBJECT_T *)arg;
    pthread_mutex_lock(&mutex_list[client->id]);
    client->sockfd            = client_socket_init(client);
    if (-1 == client->sockfd) {
        if (client->conn_handle) {
            client->conn_handle(client->id, NET_COMM_CONN_STATUS_CONNECT_FAILED);
        }
        fd_type_map[client->id] = FD_TYPE_UNUSE;
        net_comm_object_free(client);
    } else {
        fd_list[client->id]     = client->sockfd;
        fd_type_map[client->id] = FD_TYPE_RUNNING;
        sleep(1); // 等待该描述符加入select集合
        if (client->conn_handle) {
            client->conn_handle(client->id, NET_COMM_CONN_STATUS_CONNECTED);
        }
    }
    pthread_mutex_unlock(&mutex_list[client->id]);
    pthread_exit(0);
}

NET_CONN net_comm_connect(NET_COMM_SERVICE_INFO_T *service_info, NET_COMM_MESSAGE_HANDLE msg_handle, NET_COMM_CONNECT_HANDLE conn_handle)
{
    pthread_mutex_lock(&mutex_list[MAX_FD_NUM]);
    int index = 0;
    while (fd_type_map[index] != FD_TYPE_UNUSE) {
        if (++index < MAX_FD_NUM) {
            continue;
        }
        pthread_mutex_unlock(&mutex_list[MAX_FD_NUM]);
        dzlog_error("no idle fd!");
        return -1;
    }
    fd_type_map[index] = FD_TYPE_PREPARING;
    pthread_mutex_unlock(&mutex_list[MAX_FD_NUM]);

    NET_COMM_OBJECT_T *client = (NET_COMM_OBJECT_T *)malloc(sizeof(NET_COMM_OBJECT_T));
    memset(client, 0, sizeof(NET_COMM_OBJECT_T));
    fd_object_map[index] = client;

    client->id           = index;
    client->msg_handle   = msg_handle;
    client->conn_handle  = conn_handle;
    client->service_info = service_info_dup(service_info);

    pthread_t thread;
    pthread_create(&thread, NULL, (void *)&client_connect_routine, (void *)client);
    return index;
}

static NET_COMM_SERVICE_INFO_T *service_info_dup(NET_COMM_SERVICE_INFO_T *service_info)
{
    if (NULL == service_info) {
        return NULL;
    }
    NET_COMM_SERVICE_INFO_T *_service_info = (NET_COMM_SERVICE_INFO_T *)malloc(sizeof(NET_COMM_SERVICE_INFO_T));
    memset(_service_info, 0, sizeof(NET_COMM_SERVICE_INFO_T));
    if (service_info->service_name) {
        _service_info->service_name = strdup(service_info->service_name);
    }
    if (service_info->host_name) {
        _service_info->host_name = strdup(service_info->host_name);
    }
    if (service_info->text_key) {
        _service_info->text_key = strdup(service_info->text_key);
    }
    if (service_info->text_value) {
        _service_info->text_value = strdup(service_info->text_value);
    }
    if (service_info->ipv4_addr) {
        _service_info->ipv4_addr = strdup(service_info->ipv4_addr);
    }
    _service_info->port = service_info->port;
    return _service_info;
}

static void service_info_free(NET_COMM_SERVICE_INFO_T *service_info)
{
    if (NULL == service_info) {
        return;
    }
    if (service_info->service_name) {
        free(service_info->service_name);
    }
    if (service_info->host_name) {
        free(service_info->host_name);
    }
    if (service_info->text_key) {
        free(service_info->text_key);
    }
    if (service_info->text_value) {
        free(service_info->text_value);
    }
    if (service_info->ipv4_addr) {
        free(service_info->ipv4_addr);
    }
    free(service_info);
}

static int service_info_cmp(NET_COMM_SERVICE_INFO_T *service_info_a, NET_COMM_SERVICE_INFO_T *service_info_b)
{
    if (NULL == service_info_a || NULL == service_info_b) {
        return -1;
    }
    if (service_info_a->port != service_info_b->port) {
        return -1;
    }
    if (NULL != service_info_a->service_name && NULL != service_info_b->service_name) {
        if (strcmp(service_info_a->service_name, service_info_b->service_name)) {
            return -1;
        }
    } else if (NULL != service_info_a->service_name || NULL != service_info_b->service_name) {
        return -1;
    }
    if (NULL != service_info_a->host_name && NULL != service_info_b->host_name) {
        if (strcmp(service_info_a->host_name, service_info_b->host_name)) {
            return -1;
        }
    } else if (NULL != service_info_a->host_name || NULL != service_info_b->host_name) {
        return -1;
    }
    if (NULL != service_info_a->text_key && NULL != service_info_b->text_key) {
        if (strcmp(service_info_a->text_key, service_info_b->text_key)) {
            return -1;
        }
    } else if (NULL != service_info_a->text_key || NULL != service_info_b->text_key) {
        return -1;
    }
    if (NULL != service_info_a->text_value && NULL != service_info_b->text_value) {
        if (strcmp(service_info_a->text_value, service_info_b->text_value)) {
            return -1;
        }
    } else if (NULL != service_info_a->text_value || NULL != service_info_b->text_value) {
        return -1;
    }
    if (NULL != service_info_a->ipv4_addr && NULL != service_info_b->ipv4_addr) {
        if (strcmp(service_info_a->ipv4_addr, service_info_b->ipv4_addr)) {
            return -1;
        }
    } else if (NULL != service_info_a->ipv4_addr || NULL != service_info_b->ipv4_addr) {
        return -1;
    }
    return 0;
}

static NET_COMM_OBJECT_T *net_comm_object_dup(NET_COMM_OBJECT_T *obj)
{
    NET_COMM_OBJECT_T *_obj = (NET_COMM_OBJECT_T *)malloc(sizeof(NET_COMM_OBJECT_T));
    memset(_obj, 0, sizeof(NET_COMM_OBJECT_T));
    _obj->conn_handle  = obj->conn_handle;
    _obj->msg_handle   = obj->msg_handle;
    _obj->service_info = service_info_dup(obj->service_info);
    return _obj;
}

static void net_comm_object_free(NET_COMM_OBJECT_T *obj)
{
    if (NULL == obj) {
        return;
    }
    service_info_free(obj->service_info);
    free(obj);
}

NET_COMM_SERVICE_INFO_T *net_comm_servcie_info_get(NET_CONN net_conn)
{
    if (net_conn >= MAX_FD_NUM || net_conn < 0) {
        return NULL;
    }
    pthread_mutex_lock(&mutex_list[net_conn]);
    if (fd_type_map[net_conn] != FD_TYPE_RUNNING && fd_type_map[net_conn] != FD_TYPE_LISTENING) {
        pthread_mutex_unlock(&mutex_list[net_conn]);
        return NULL;
    }
    NET_COMM_SERVICE_INFO_T *service_info = service_info_dup(fd_object_map[net_conn]->service_info);
    pthread_mutex_unlock(&mutex_list[net_conn]);
    return service_info;
}

void net_comm_servcie_info_free(NET_COMM_SERVICE_INFO_T *service_info)
{
    service_info_free(service_info);
}

int net_comm_disconnect(NET_CONN net_conn)
{
    if (net_conn >= MAX_FD_NUM || net_conn < 0) {
        return -1;
    }
    pthread_mutex_lock(&mutex_list[net_conn]);
    if (fd_type_map[net_conn] == FD_TYPE_CLOSED || fd_type_map[net_conn] == FD_TYPE_CLOSING) {
        pthread_mutex_unlock(&mutex_list[net_conn]);
        return 1;
    }
    if (fd_type_map[net_conn] != FD_TYPE_RUNNING) {
        pthread_mutex_unlock(&mutex_list[net_conn]);
        return -1;
    }
    fd_type_map[net_conn] = FD_TYPE_CLOSED;
    pthread_mutex_unlock(&mutex_list[net_conn]);
    return 0;
}

int net_comm_server_terminate(NET_CONN net_conn)
{
    if (net_conn >= MAX_FD_NUM || net_conn < 0) {
        return -1;
    }
    // 关闭连接监听
    pthread_mutex_lock(&mutex_list[net_conn]);
    fd_type_map[net_conn] = FD_TYPE_UNUSE;
    close(fd_list[net_conn]);
    NET_COMM_SERVICE_INFO_T *service_info = service_info_dup(fd_object_map[net_conn]->service_info);
    net_comm_object_free(fd_object_map[net_conn]);
    pthread_mutex_unlock(&mutex_list[net_conn]);
    // 关闭所有客户端连接
    int i;
    for (i = 0; i < MAX_FD_NUM; i++) {
        pthread_mutex_lock(&mutex_list[i]);
        if (FD_TYPE_UNUSE == fd_type_map[i]) {
            pthread_mutex_unlock(&mutex_list[i]);
            continue;
        }
        if (service_info_cmp(service_info, fd_object_map[i]->service_info)) {
            pthread_mutex_unlock(&mutex_list[i]);
            continue;
        }
        net_comm_disconnect(i);
        pthread_mutex_unlock(&mutex_list[i]);
    }
    service_info_free(service_info);
    return 0;
}

static SAP_LOG_LEVEL_E debug_level = SAP_LOG_LEVEL_ERROR;
void net_comm_set_debug_level(SAP_LOG_LEVEL_E _debug_level)
{
    debug_level = _debug_level;
}

SAP_LOG_LEVEL_E net_comm_get_debug_level()
{
    return debug_level;
}
