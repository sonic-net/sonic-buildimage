/********************************************************************************
 * Copyright(C) 2020 Micas Network. All rights reserved.
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期        | 作者     |  修改记录
**===============================================================================
**| 2025/4/10  | zhoutenghui  |  创建该文件
**
*********************************************************************************/
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sap_socket.h"

#define SAPMSG_BUF      "/tmp/sapcmd_output"
#define SAPCMD_SOCKET_PATH "/tmp/sapcmd_socket"
#define MAX_DEBUG_STR_LEN 256
#define MAX_DEBUG_ARG_NUM 16
#define MAX_STRING 256

#define TIME_INTERNAL (100000)
#define GET_TIMEOUT_SECS(timeout) (timeout*(1000000/TIME_INTERNAL))
#define DEFAULT_CLI_TIMEOUT (30)
#define DEFAULT_CON_TIMEOUT (10)

static NET_COMM_CONN_STATUS_E client_connected = NET_COMM_CONN_STATUS_PREPARING;
static int server_process_done = 0;

static void *client_message_handle(NET_CONN net_comm, char *data, uint32_t data_len);
static void *client_connect_handle(NET_CONN net_comm, NET_COMM_CONN_STATUS_E status);
static SAP_LOG_LEVEL_E g_debug_level = SAP_LOG_LEVEL_DISABLE;

#define MSG_PRINTF(fmt, args...) do { \
    if (SAP_LOG_LEVEL_ERROR <= g_debug_level) { \
         printf("[msg][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
    } while (0)

static void show_output()
{
    char ch;
    int t;
    FILE *fp;
    system("sync");
    fp = fopen(SAPMSG_BUF, "r");
    if (fp == NULL) {
        MSG_PRINTF("message buffer file open fail!");
        return;
    }
    // printf("\n");
    while (!feof(fp)) {
        if ((ch = fgetc(fp)) != EOF) {
            putchar(ch);
        }
    }
    fclose(fp);
    /* 清空文件内容 */
    fp = fopen(SAPMSG_BUF, "w");
    fclose(fp);
}

static void *client_connect_handle(NET_CONN net_comm, NET_COMM_CONN_STATUS_E status)
{
    switch (status) {
    case NET_COMM_CONN_STATUS_CONNECTED: {
        client_connected = NET_COMM_CONN_STATUS_CONNECTED;
        break;
    }
    case NET_COMM_CONN_STATUS_CONNECT_LOST: {
        MSG_PRINTF("%s: %d", "clinet connect to server lost!", net_comm);
        client_connected = NET_COMM_CONN_STATUS_CONNECT_LOST;
        rl_deprep_terminal();
        exit(0);
        break;
    }
    case NET_COMM_CONN_STATUS_CONNECT_FAILED: {
        MSG_PRINTF("%s: %d", "clinet connect to server failed!", net_comm);
        client_connected = NET_COMM_CONN_STATUS_CONNECT_FAILED;
        break;
    }
    case NET_COMM_CONN_STATUS_MDNS_QUERY_TIMEOUT: {
        MSG_PRINTF("%s: %d", "clinet connect to server timeout!", net_comm);
        client_connected = NET_COMM_CONN_STATUS_MDNS_QUERY_TIMEOUT;
        break;
    }
    default:
        break;
    }
    return NULL;
}

static void *client_message_handle(NET_CONN net_comm, char *data, uint32_t data_len)
{
    MSG_PRINTF("recv message[%d]: %s", data_len, data);
    if (memcmp(data, "done", strlen("done")) == 0) {
        server_process_done = 1;
    }
    return NULL;
}

void run_shell(NET_CONN client_comm)
{
    int wait_conn_count = 0;
    while (client_connected == NET_COMM_CONN_STATUS_PREPARING)
    {
        usleep(TIME_INTERNAL);
        wait_conn_count ++;
        if (wait_conn_count > GET_TIMEOUT_SECS(DEFAULT_CON_TIMEOUT)) {
            MSG_PRINTF("wait service connect timeout!");
            break;
        }
    }
    int wait_process_count = 0;
    while (client_connected == NET_COMM_CONN_STATUS_CONNECTED)
    {
        char *buffer;
        buffer = readline("drivshell>");
        if (strlen(buffer) == 0) {
            continue;
        }
        if (!strcmp(buffer, "exit")) {
            break;
        }
        add_history(buffer);
        net_comm_message_send(client_comm, buffer, strlen(buffer) + 1);
        /* 等待服务端返回结果 */
        while (server_process_done != 1)
        {
            usleep(TIME_INTERNAL);
            wait_process_count++;
            if (wait_process_count > GET_TIMEOUT_SECS(DEFAULT_CLI_TIMEOUT)) {
                MSG_PRINTF("wait cli process timeout!");
                break;
            }
        }
        free(buffer);
        if (server_process_done == 1) {
            show_output();
        }
        wait_process_count = 0;
    }
    if (client_connected == NET_COMM_CONN_STATUS_PREPARING) {
        MSG_PRINTF("client connect timeout!");
    } else if (client_connected == NET_COMM_CONN_STATUS_CONNECT_LOST) {
        MSG_PRINTF("client connect lost!");
    } else if (client_connected == NET_COMM_CONN_STATUS_CONNECT_FAILED) {
        MSG_PRINTF("client connect failed!");
    } else if (client_connected == NET_COMM_CONN_STATUS_CONNECTED) {
        net_comm_disconnect(client_comm);
    }
}

void run_cmd(NET_CONN client_comm, char *cmd_buffer, int timeout)
{
    int wait_conn_count = 0;
    while (client_connected == NET_COMM_CONN_STATUS_PREPARING)
    {
        usleep(TIME_INTERNAL);
        wait_conn_count ++;
        if (wait_conn_count > GET_TIMEOUT_SECS(DEFAULT_CON_TIMEOUT)) {
            MSG_PRINTF("wait service connect timeout!");
            break;
        }
    }
    int wait_process_count = 0;
    while (client_connected == NET_COMM_CONN_STATUS_CONNECTED)
    {
        if (strlen(cmd_buffer) == 0) {
            break;
        }
        if (!strcmp(cmd_buffer, "exit")) {
            break;
        }
        net_comm_message_send(client_comm, cmd_buffer, strlen(cmd_buffer) + 1);
        /* 等待服务端返回结果 */
        while (server_process_done != 1)
        {
            usleep(TIME_INTERNAL);
            wait_process_count++;
            if (wait_process_count > GET_TIMEOUT_SECS(timeout)) {
                MSG_PRINTF("wait cli process timeout!");
                break;
            }
        }
        if (server_process_done == 1) {
            show_output();
        }
        wait_process_count = 0;
        break;
    }
    if (client_connected == NET_COMM_CONN_STATUS_PREPARING) {
        MSG_PRINTF("client connect timeout!");
    } else if (client_connected == NET_COMM_CONN_STATUS_CONNECT_LOST) {
        MSG_PRINTF("client connect lost!");
    } else if (client_connected == NET_COMM_CONN_STATUS_CONNECT_FAILED) {
        MSG_PRINTF("client connect failed!");
    } else if (client_connected == NET_COMM_CONN_STATUS_CONNECTED) {
        net_comm_disconnect(client_comm);
    }
}

int main(int argc, char *argv[])
{
    char cmd_buffer[256] = {0};
    int timeout;
    int n;
    net_comm_init();
    NET_COMM_SERVICE_INFO_T service_info = {
        .host_name    = SAPCMD_SOCKET_PATH,
        .service_name = "plpcmd_service",
        .text_key     = NULL,
        .text_value   = NULL,
        .ipv4_addr    = NULL,
        .port         = 0
    };
    NET_CONN client_comm = net_comm_connect(&service_info, client_message_handle, client_connect_handle);
    timeout = DEFAULT_CLI_TIMEOUT;
    if (argc == 1) {
		printf("usage:\n");
		printf("\t-t: timeout in seconds, default 30, rang [5-300]\n");
		return 0;
	} else if (argc == 2) {
		if (strcmp(argv[1], "-shmod") == 0)  {
            run_shell(client_comm);
        } else {
			if (strlen(argv[1]) >= MAX_STRING) {
			    printf("command line is too long\n");
				return 0;
			}
            run_cmd(client_comm, argv[1], timeout);
        }
	} else {
		for (n = 1; n < argc; n++) {
		    if (strcmp(argv[n], "-t") == 0) {
			    timeout = atoi(argv[n + 1]);
				if ((timeout > 300) || (timeout < 5)) {
					printf("invalid timeout param\n");
					return 0;
				}
			} else if (strcmp(argv[n], "-d") == 0) {
			    g_debug_level = SAP_LOG_LEVEL_ALL;
                net_comm_set_debug_level(SAP_LOG_LEVEL_ALL);
			}
		}
		if (strlen(argv[argc - 1]) >= MAX_STRING) {
			printf("command line is too long\n");
			return 0;
		}
        run_cmd(client_comm, argv[argc - 1], timeout);
	}
    return 0;
}
