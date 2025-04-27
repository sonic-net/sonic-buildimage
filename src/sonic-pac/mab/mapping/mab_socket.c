/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "mab_api.h"
#include "mab_include.h"
#include "osapi.h"
#include "mab_socket.h"
#include "fpSonicUtils.h"

#define MAX_CLIENTS 1024
#define MAB_NO_SOCKET -1
#define MAB_TID_INIT  -1
#define MAB_SERVER_IPV4_ADDR "127.0.0.1"
#define MAB_SERVER_LISTEN_PORT 3734
#define MAB_MAX_SEND_SIZE 1024

typedef struct connection_list_e
{
  int socket;
  pthread_t tid;
}connection_list_t;

static connection_list_t connection_list[MAX_CLIENTS] = {};

#define MAB_COPY(_a) static int _a##_##COPY(void *in, void *out)
#define MAB_ENTER(_a, _b, _c, _rc) _rc = _a##_##COPY(_b, _c)

int mab_cmd_ping(unsigned int intf, char *resp)
{
  unsigned int status = 0;

  mabPortOperMABEnabledGet(intf, &status);

  if (status)
  {
    osapiStrncpySafe(resp, "PONG", strlen("PONG")+1);
  }
  else
  {
    osapiStrncpySafe(resp, "NO RESP", strlen("NO RESP")+1);
  }

  return 0;
}

int mab_cmd_event_notify(unsigned int intf, unsigned int event , unsigned char *mac, char *resp)
{
  int rc = 0;
   enetMacAddr_t mac_addr;

  memcpy(mac_addr.addr, mac, 6);

  rc = mabClientEventUpdate(intf, event, &mac_addr);

  if (!rc)
  {
    osapiStrncpySafe(resp, "OK", strlen("OK")+1);
  }
  else
  {
    osapiStrncpySafe(resp, "FAIL", strlen("FAIL")+1);
  }

  return 0;
}

MAB_COPY(INTERFACE)
{

	char *intfStr = (char *)in;
	unsigned int intf = 0; 

 fpGetIntIfNumFromHostIfName(intfStr, &intf);
	(*(unsigned int *)out) = intf;

  return 0;
}

MAB_COPY(CMD)
{
  mab_pac_cmd_t *req = (mab_pac_cmd_t *)in;
  char *resp = (char *)out;
  unsigned int intf = 0;
  int rc = 0;

  /* convert char intf to intf number */
  MAB_ENTER(INTERFACE, (void *)req->intf, (void *)&intf, rc);

  printf("MAB ENTER INTERFACE !! rc %d \n", rc);

  if (0 != rc)
    return -1;

  if (0 == strncmp("PING", req->cmd, strlen("PING")))
  {
    rc = mab_cmd_ping(intf, resp);
  }
  else if (0 == strncmp("event-notify", req->cmd, strlen("event-notify")+1))
  {
    rc = mab_cmd_event_notify(intf, req->notif_event, req->mac_addr, resp);
  }
  else
  {
    osapiStrncpySafe(resp, "unknown cmd", strlen("unknown cmd")+1);
  }

  return 0;
}

/* Start listening socket listen_sock. */
int start_listen_socket(int *listen_sock)
{
  int reuse = 1;
  struct sockaddr_in my_addr;

  /* Obtain a file descriptor for our "listening" socket. */
  *listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (*listen_sock < 0)
  {
    perror("socket");
    return -1;
  }

  if (setsockopt(*listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
    perror("setsockopt");
    return -1;
  }

  memset(&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = inet_addr(MAB_SERVER_IPV4_ADDR);
  my_addr.sin_port = htons(MAB_SERVER_LISTEN_PORT);

  if (bind(*listen_sock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) != 0) {
    perror("bind");
    return -1;
  }

  /* start accept client connections */
    if (listen(*listen_sock, 144) != 0) {
      perror("listen");
      return -1;
    }
  printf("Accepting connections on port %d.\n", (int)MAB_SERVER_LISTEN_PORT);

  return 0;
}


void connection_close(int conn_socket)
{
  int i;

  for (i = 0; i < MAX_CLIENTS; ++i) {
    if (connection_list[i].socket == conn_socket) {
      close(connection_list[i].socket);
      connection_list[i].socket = MAB_NO_SOCKET;
    }
  }
}


/* Receive message from peer and handle it with message_handler(). */
int read_from_connection(int socket, char *buf, unsigned int buf_len,
                             unsigned int *bytes_read, bool *more_data)
{
  size_t len_to_receive;
  ssize_t received_count = 0;
  size_t received_total = 0;

  char *buf_ptr = buf;

  MAB_EVENT_TRACE(
    "Entering read_from_connection fd %d buf_len %d\n", socket, buf_len);


  do {
    if (received_total >= buf_len)
    {
      /* still more to copy */
      *more_data = true;
      *bytes_read = received_total;
      MAB_EVENT_TRACE(
          "fd: %d There is more data , Read %zd bytes till now",
          socket, received_count);

      return 0;
    }

    /* Count bytes to receive.*/
    len_to_receive = buf_len;
    if (len_to_receive > MAB_MAX_SEND_SIZE)
      len_to_receive = MAB_MAX_SEND_SIZE;

    MAB_EVENT_TRACE(
             "fd: %d Let's try to recv() %zd bytes... ", socket, len_to_receive);

    received_count = recv(socket, buf_ptr, len_to_receive, NULL);

    if (received_count < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
          MAB_EVENT_TRACE(
          "fd %d peer is not ready right now, try again later.\n", socket);
          continue;
      }
      else {
        perror("recv() from peer error");
        *more_data = false;
        return -1;
      }
    }

    /* If recv() returns 0, it means that peer gracefully shutdown. Shutdown client.*/
    else if (received_count == 0) {
           MAB_EVENT_TRACE(
           "fd %d recv() 0 bytes. Peer gracefully shutdown.\n", socket);
           *more_data = false;
           break;
    }
    else if (received_count > 0) {
      received_total += received_count;
      MAB_EVENT_TRACE(
      "fd %d recv() %zd bytes\n", socket, received_count);

      buf_ptr += received_count;
      len_to_receive -= received_count;
      *more_data = false;
      break;
    }

  } while (received_count > 0);

  *bytes_read = received_total;

  MAB_EVENT_TRACE(
       "fd %d Total recv()'ed %zu bytes.\n", socket, received_total);
  return 0;
}

void close_connection(int conn_socket)
{
  close(conn_socket);
}

void *new_connection_handle(void *arg)
{
  int new_socket = *((int *)arg);
  char recv_buff[2048] = {0};
  char resp_buff[256] = {0};
  char *buf = &recv_buff[0];
  unsigned int bytes_received;
  bool more_data = true;
  int rem_len = sizeof(recv_buff);
  int total_read = 0;
  int rc = -1;

  int i;
  char *ptr = &recv_buff[0];

  while(more_data)
  {
    more_data = false;
    if (rem_len <= 0)
    {
      MAB_EVENT_TRACE(
        "fd: %d More data to read, but not sufficient buffer !!\n", new_socket);
      break;
    }
    bytes_received = 0;
    if (0 != read_from_connection(new_socket, buf, rem_len, &bytes_received, &more_data))
    {
      break;
    }
    buf = recv_buff + bytes_received;
    total_read += bytes_received;
    rem_len = sizeof(recv_buff) - total_read;
  }

  printf("buffer: ");
  for (i =0; i<10; i++)
  {
    printf("0x%x ", ptr[i]);
  }
  printf("\n");

  MAB_EVENT_TRACE(
  "fd :%d start processing the cmd and send resp!!\n", new_socket);

  MAB_ENTER(CMD, (void *)recv_buff, (void *)resp_buff, rc);

  if (strlen(resp_buff))
  {
    /* send the response */
    rc = send(new_socket, resp_buff, sizeof(resp_buff), 0);

    MAB_EVENT_TRACE(
     "fd :%d Successfully sent data (len %lu bytes): %s\n",
           new_socket, sizeof(resp_buff), resp_buff);
  }

  free (arg);
  close_connection(new_socket);
  pthread_exit(NULL); 
}


int new_connection_open(int new_client_sock)
{
  int *new_sock;
  int ret = 0;
  pthread_attr_t tattr;
  static int cl_count;


  if ((ret = pthread_attr_init(&tattr)) !=0)
  {
    MAB_EVENT_TRACE(
      "Failed to create thread - pthread_attr_init failed with %d\n", ret);
    close(new_client_sock);
    return -1;
  }

  if ((ret = pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) !=0)
  {
    MAB_EVENT_TRACE(
      "Failed to create thread - pthread_attr_setdetachstate failed with %d\n", ret);
    close(new_client_sock);
    pthread_attr_destroy(&tattr);
    return -1;
  }

  new_sock = malloc(sizeof(*new_sock));
  *new_sock = new_client_sock;

  connection_list[cl_count].socket = new_client_sock;
  connection_list[cl_count].tid = MAB_TID_INIT;

      if ((ret = pthread_create(&connection_list[cl_count].tid, &tattr, 
               new_connection_handle, (void*)new_sock)) !=0)
      {
        MAB_EVENT_TRACE(
         "Failed to create thread\n");
        free(new_sock);
        connection_close(new_client_sock);
        pthread_attr_destroy(&tattr);
        return -1;
      }
      cl_count = (cl_count+1)%MAX_CLIENTS;
      pthread_attr_destroy(&tattr);
      return 0;
}


int mab_socket_server_handle(int *listen_sock)
{
  int i;
  int new_client_sock = -1;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  char client_ipv4_str[INET_ADDRSTRLEN];

  if (start_listen_socket(listen_sock) != 0) {
    return -1;
  }

  for (i = 0; i < MAX_CLIENTS; ++i)
  {
    connection_list[i].socket = MAB_NO_SOCKET;
    connection_list[i].tid = MAB_TID_INIT;
  }

  while (1)
  {
    new_client_sock = -1;
    printf("calling accept.\n");

    memset(&client_addr, 0, sizeof(client_addr));

    new_client_sock = accept(*listen_sock, (struct sockaddr *)&client_addr, &client_len);
    if (new_client_sock < 0) {
      perror("accept()");
      continue;
    }

    inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);

    MAB_EVENT_TRACE(
        "Incoming connection from client fd %d [%s:%u] ",
        new_client_sock, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    new_connection_open(new_client_sock);
  }
  return 0;
}

int mab_radius_init_recv_socket(int *sock)
{
  struct sockaddr_in s_addr;
  *sock = -1;

  /* create the mab server socket */
	*sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (*sock < 0) {
        MAB_EVENT_TRACE(
		   "socket[AF_INET,SOCK_DGRAM]: %s",
			   strerror(errno));
		return -1;
	}

    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family    = AF_INET; 
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    s_addr.sin_port = htons(9395);

    if (bind(*sock, (const struct sockaddr *)&s_addr, 
            sizeof(s_addr)) < 0 )
    {
        perror("bind failed");
        return -1;
    }
   return 0;
}


int mab_radius_init_send_socket(int *sock)
{
  *sock = -1;

  /* create the mab send socket */
	*sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (*sock < 0) {
        MAB_EVENT_TRACE(
		 "socket[AF_INET,SOCK_DGRAM]: %s",
			   strerror(errno));
		return -1;
	}
   return 0;
}


