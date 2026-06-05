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
#ifndef _SAP_SOCKET_H_
#define _SAP_SOCKET_H_

// #include "cJSON.h"
#include "stdint.h"
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // 涉及文件描述符操作
#include "sap_common.h"

typedef int8_t NET_CONN;

typedef enum {
    NET_COMM_CONN_STATUS_PREPARING          = 0,
    NET_COMM_CONN_STATUS_CONNECTED          = 1,
    NET_COMM_CONN_STATUS_MDNS_QUERY_TIMEOUT = 2,
    NET_COMM_CONN_STATUS_CONNECT_FAILED     = 3,
    NET_COMM_CONN_STATUS_CONNECT_LOST       = 4
} NET_COMM_CONN_STATUS_E;

typedef void *(*NET_COMM_MESSAGE_HANDLE)(NET_CONN net_comm, char *data, uint32_t data_len);
typedef void *(*NET_COMM_CONNECT_HANDLE)(NET_CONN net_comm, NET_COMM_CONN_STATUS_E status);

#define NET_COMM_HEADER_LENGTH 8

struct NET_COMM_HEADER {
    uint32_t header; // LEEL
    uint32_t length;
    uint8_t payload;
} __attribute__((packed));

typedef struct NET_COMM_HEADER NET_COMM_HEADER_T;

typedef struct
{
    char *service_name;
    char *host_name;
    char *text_key;
    char *text_value; // cjson格式
    char *ipv4_addr;
    int port;
} NET_COMM_SERVICE_INFO_T;

/*******************************************************************
 ** 函数名:       net_comm_init
 ** 函数描述:     局域网通讯模块初始化函数
 ** 参数:        net_card指定监听哪个网卡的消息
 ** 返回:        
 ** 注意:    必须要在调用其它接口前调用
 ** 记录: 	 2022/09/19,  zhoutenghui创建
 ********************************************************************/
void net_comm_init(void);

/*******************************************************************
 ** 函数名:       net_comm_server_publish
 ** 函数描述:     发布服务
 ** 参数:        service_info指定服务器信息，msg_handle指定消息回调函数，conn_handle指定客户端连接状态变更事件的回调函数
 ** 返回:        
 ** 注意:    
 ** 记录: 	 2022/09/19,  zhoutenghui创建
 ********************************************************************/
NET_CONN net_comm_server_publish(NET_COMM_SERVICE_INFO_T *service_info, NET_COMM_MESSAGE_HANDLE msg_handle, NET_COMM_CONNECT_HANDLE conn_handle);

/*******************************************************************
 ** 函数名:       net_comm_server_terminate
 ** 函数描述:     终止服务
 ** 参数:        net_conn指定要终止的服务
 ** 返回:        
 ** 注意:    
 ** 记录: 	 2022/09/19,  zhoutenghui创建
 ********************************************************************/
int net_comm_server_terminate(NET_CONN net_conn);

/*******************************************************************
 ** 函数名:       net_comm_servcie_info_get
 ** 函数描述:     获取某个连接的服务器信息
 ** 参数:         
 ** 返回:        
 ** 注意:    返回的数据是从堆上分配的空间，需要net_comm_servcie_info_free来释放
 ** 记录: 	 2022/09/19,  zhoutenghui创建
 ********************************************************************/
NET_COMM_SERVICE_INFO_T *net_comm_servcie_info_get(NET_CONN net_conn);

/*******************************************************************
 ** 函数名:       net_comm_servcie_info_free
 ** 函数描述:     释放service_info
 ** 参数:         
 ** 返回:        
 ** 注意:    service_info必须是通过net_comm_servcie_info_get获取的
 ** 记录: 	 2022/09/19,  zhoutenghui创建
 ********************************************************************/
void net_comm_servcie_info_free(NET_COMM_SERVICE_INFO_T *service_info);

/*******************************************************************
 ** 函数名:       net_comm_connect
 ** 函数描述:     客服端发起到服务器的链接
 ** 参数:         service_info指定要连接的服务器，msg_handle指定消息回调函数，conn_handle指定连接状态变更事件回调函数
 ** 返回:        
 ** 注意:    
 ** 记录: 	 2022/09/19,  zhoutenghui创建
 ********************************************************************/
NET_CONN net_comm_connect(NET_COMM_SERVICE_INFO_T *service_info, NET_COMM_MESSAGE_HANDLE msg_handle, NET_COMM_CONNECT_HANDLE conn_handle);

/*******************************************************************
 ** 函数名:       net_comm_disconnect
 ** 函数描述:     断开连接，由服务端或客户端发起均可
 ** 参数:         net_conn指定要断开的连接
 ** 返回:        -1表示连接关闭失败, 1表示连接关闭成功
 ** 注意:    调用该接口主动关闭连接不会触发连接状态变更事件回调函数
 ** 记录: 	 2022/09/19,  zhoutenghui创建
 ********************************************************************/
int net_comm_disconnect(NET_CONN net_conn);

/*******************************************************************
 ** 函数名:       net_comm_message_send
 ** 函数描述:     发送数据
 ** 参数:         net_conn指定通过哪个链路发送，data会被封装到局域网通讯协议的payload中
 ** 返回:        
 ** 注意:    
 ** 记录: 	 2022/09/19,  zhoutenghui创建
 ********************************************************************/
int net_comm_message_send(NET_CONN net_conn, char *data, int data_len);

void net_comm_set_debug_level(SAP_LOG_LEVEL_E _debug_level);
SAP_LOG_LEVEL_E net_comm_get_debug_level();

#endif // _SAP_SOCKET_H_
