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
#include <stdbool.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "osapi.h"
#include "auth_mgr_include.h"
#include "auth_mgr_auth_method.h"
#include "wpa_ctrl.h"
#include "radius_attr_parse.h"
#include "fpSonicUtils.h"

#define MAX_CLIENTS 1024
#define NO_SOCKET -1
#define TID_INIT  -1
#define SERVER_IPV4_ADDR "127.0.0.1"
#define SERVER_LISTEN_PORT 3434
#define MAX_SEND_SIZE 1024

#define AUTH_MGR_COPY(_a) static int _a##_##COPY(void *in, void *out)
#define AUTH_MGR_ENTER(_a, _b, _c, _rc) _rc = _a##_##COPY(_b, _c)

#define AUTH_MGR_STR_COPY(_dst_, _src_) memcpy(_dst_, _src_, min(sizeof(_src_), sizeof(_dst_)))

#define ETH_P_PAE 0x888E

typedef struct connection_list_e
{
  int socket;
  pthread_t tid;
}connection_list_t;

static connection_list_t connection_list[MAX_CLIENTS] = {};

unsigned int extra_detail_logs = 0;

AUTH_MGR_COPY(INTERFACE)
{

	char *intfStr = (char *)in;
	unsigned int intf = 0; 

    fpGetIntIfNumFromHostIfName(intfStr, &intf);
	(*(unsigned int *)out) = intf;

	return 0;
}

AUTH_MGR_COPY(BAM_METHOD)
{
	char *methodStr = ((clientStatusReply_t *)in)->info.authInfo.bam_used;
	int retVal = 0; 

	if (0 == strncmp("radius", methodStr, strlen("radius")))
	{
    ((authmgrClientStatusInfo_t *)out)->info.authInfo.authMethod =  AUTH_METHOD_RADIUS;
	}
	else if (0 == strncmp("local", methodStr, strlen("local")))
	{
    ((authmgrClientStatusInfo_t *)out)->info.authInfo.authMethod =  AUTH_METHOD_LOCAL;
	}
	else
	{
		retVal = -1;
	}

	return retVal;
}


AUTH_MGR_COPY(METHOD_CHANGE)
{
	authmgrClientStatusInfo_t *cInfo = (authmgrClientStatusInfo_t *)out;
	clientStatusReply_t *cReply = (clientStatusReply_t *)in;
        int retVal = 0;

	if (0 == strncmp("enable", cReply->info.enableStatus, strlen("enable")))
	{
          cInfo->info.enableStatus =  ENABLE;
	}
	else if (0 == strncmp("disable", cReply->info.enableStatus, strlen("disable")))
	{
          cInfo->info.enableStatus =  DISABLE;
	}
	else
	{
		retVal = -1;
	}

	return retVal;
}

AUTH_MGR_COPY(METHOD)
{
	char *methodStr = (char *)in;
	int retVal = 0; 

	if (0 == strncmp("802.1X", methodStr, strlen("802.1X")))
	{
		(*(unsigned int *)out) =  AUTHMGR_METHOD_8021X;
	}
	else if (0 == strncmp("mab", methodStr, strlen("mab")))
	{
		(*(unsigned int *)out) =  AUTHMGR_METHOD_MAB;
	}
	else
	{
		retVal = -1;
	}

	return retVal;
}

AUTH_MGR_COPY(COMMON_PARAMS)
{
	authmgrClientStatusInfo_t *cInfo = (authmgrClientStatusInfo_t *)out;
	clientStatusReply_t *cReply = (clientStatusReply_t *)in;

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
      "Received update for user %02X:%02X:%02X:%02X:%02X:%02X ",
      (unsigned int)cReply->info.authInfo.addr[0], (unsigned int)cReply->info.authInfo.addr[1], (unsigned int)cReply->info.authInfo.addr[2],
      (unsigned int)cReply->info.authInfo.addr[3], (unsigned int)cReply->info.authInfo.addr[4], (unsigned int)cReply->info.authInfo.addr[5]);
	/* copy mac address */
	memcpy(cInfo->info.authInfo.macAddr.addr, cReply->info.authInfo.addr, 6); 
	/* copy eapol version */
	cInfo->info.authInfo.eapolVersion = cReply->info.authInfo.eapolVersion;
	return 0;
}

AUTH_MGR_COPY(NEW_CLIENT)
{
	int rc = 0;

	AUTH_MGR_ENTER(COMMON_PARAMS, in, out, rc);
	return rc;
}

AUTH_MGR_COPY(ATTR_INFO)
{

	authmgrClientStatusInfo_t *cInfo = (authmgrClientStatusInfo_t *)out;
	clientStatusReply_t *cReply = (clientStatusReply_t *)in;

    if (cReply->info.authInfo.attrInfo.userNameLen)
    {
      memcpy(cInfo->info.authInfo.attrInfo.userName, cReply->info.authInfo.attrInfo.userName, cReply->info.authInfo.attrInfo.userNameLen);
      cInfo->info.authInfo.attrInfo.userNameLen = cReply->info.authInfo.attrInfo.userNameLen;
    }

    if (cReply->info.authInfo.attrInfo.serverStateLen)
    {
      memcpy(cInfo->info.authInfo.attrInfo.serverState, cReply->info.authInfo.attrInfo.serverState, cReply->info.authInfo.attrInfo.serverStateLen);
      cInfo->info.authInfo.attrInfo.serverStateLen = cReply->info.authInfo.attrInfo.serverStateLen;
    }
    
    if (cReply->info.authInfo.attrInfo.serverClassLen)
    {
      memcpy(cInfo->info.authInfo.attrInfo.serverClass, cReply->info.authInfo.attrInfo.serverClass, cReply->info.authInfo.attrInfo.serverClassLen);
      cInfo->info.authInfo.attrInfo.serverClassLen = cReply->info.authInfo.attrInfo.serverClassLen;
    }
   
    cInfo->info.authInfo.attrInfo.sessionTimeout = cReply->info.authInfo.attrInfo.sessionTimeout;
    cInfo->info.authInfo.attrInfo.terminationAction = cReply->info.authInfo.attrInfo.terminationAction;
    cInfo->info.authInfo.attrInfo.idFromServer = cReply->info.authInfo.attrInfo.idFromServer;
    osapiStrncpySafe(cInfo->info.authInfo.attrInfo.vlanString, cReply->info.authInfo.attrInfo.vlanString, strlen(Reply->info.authInfo.attrInfo.vlanString)+1);
    cInfo->info.authInfo.attrInfo.attrFlags = cReply->info.authInfo.attrInfo.attrFlags;
    cInfo->info.authInfo.attrInfo.vlanAttrFlags = cReply->info.authInfo.attrInfo.vlanAttrFlags;
 
#if 0
  memcpy(&cInfo->info.authInfo.attrInfo, &cReply->info.authInfo.attrInfo,
         sizeof(cInfo->info.authInfo.attrInfo));
#endif
	return 0; 
}


AUTH_MGR_COPY(AUTH_SUCCESS)
{
	int rc = 0;

	authmgrClientStatusInfo_t *cInfo = (authmgrClientStatusInfo_t *)out;
	clientStatusReply_t *cReply = (clientStatusReply_t *)in;
	/* copy the bam if the bam method is one of the supported */
	AUTH_MGR_ENTER(BAM_METHOD, in, out, rc);

  if (0 != rc)
		goto fail;

			AUTH_MGR_ENTER(COMMON_PARAMS, in, out, rc);

   /* copy other auth-success related params */

	/* user name */
	AUTH_MGR_STR_COPY(cInfo->info.authInfo.authmgrUserName, cReply->info.authInfo.userName);
	cInfo->info.authInfo.authmgrUserNameLength = cReply->info.authInfo.userNameLength;

	AUTH_MGR_ENTER(ATTR_INFO, in, out, rc);

	return 0; 

fail:
	return -1;
}



AUTH_MGR_COPY(COMMON_FAIL)
{
	int rc = 0;


	authmgrClientStatusInfo_t *cInfo = (authmgrClientStatusInfo_t *)out;
	clientStatusReply_t *cReply = (clientStatusReply_t *)in;

	/* copy the bam if the bam method is one of the supported */
	AUTH_MGR_ENTER(BAM_METHOD, in, out, rc);

	AUTH_MGR_ENTER(COMMON_PARAMS, in, out, rc);

	/* user name */
	AUTH_MGR_STR_COPY(cInfo->info.authInfo.authmgrUserName, cReply->info.authInfo.userName);
	cInfo->info.authInfo.authmgrUserNameLength = cReply->info.authInfo.userNameLength;

	return 0; 
}


AUTH_MGR_COPY(AUTH_FAIL)
{
	int rc = 0;

	AUTH_MGR_ENTER(COMMON_FAIL, in, out, rc);
	return rc; 
}

AUTH_MGR_COPY(AUTH_TIMEOUT)
{
	int rc = 0;

	AUTH_MGR_ENTER(COMMON_FAIL, in, out, rc);
	return rc; 
}

AUTH_MGR_COPY(AUTH_SERVER_COMM_FAILURE)
{
	int rc = 0;

	AUTH_MGR_ENTER(COMMON_FAIL, in, out, rc);
	return rc; 
}

AUTH_MGR_COPY(RADIUS_SERVERS_DEAD)
{
	int rc = 0;

	AUTH_MGR_ENTER(COMMON_FAIL, in, out, rc);
	return rc; 
}

AUTH_MGR_COPY(CLIENT_DISCONNECTED)
{
	int rc = 0;

	AUTH_MGR_ENTER(COMMON_PARAMS, in, out, rc);
	return rc; 
}


AUTH_MGR_COPY(RADIUS_DACL_INFO)
{
return 0;
}


AUTH_MGR_COPY(RADIUS_FIRST_PASS_DACL_DATA)
{
return 0;
}

AUTH_MGR_COPY(MKA_PEER_TIMEOUT)
{
return 0;
}

int auth_mgr_status_params_copy(authmgrClientStatusInfo_t *clientInfo, clientStatusReply_t *reply)
{
	void *in = (void *)reply;
	void *out = (void *)clientInfo;
	int rc = 0;

	switch (reply->status)
	{
		case NEW_CLIENT:
			AUTH_MGR_ENTER(NEW_CLIENT, in, out, rc);
			break;

		case AUTH_FAIL:
			AUTH_MGR_ENTER(AUTH_FAIL, in, out, rc);
			break;

		case AUTH_SUCCESS:
			AUTH_MGR_ENTER(AUTH_SUCCESS, in, out, rc);
			break;

		case AUTH_TIMEOUT:
			AUTH_MGR_ENTER(AUTH_TIMEOUT, in, out, rc);
			break;

		case AUTH_SERVER_COMM_FAILURE:
			AUTH_MGR_ENTER(AUTH_SERVER_COMM_FAILURE, in, out, rc);
			break;

		case CLIENT_DISCONNECTED:
			AUTH_MGR_ENTER(CLIENT_DISCONNECTED, in, out, rc);
			break;

		case METHOD_CHANGE:
			AUTH_MGR_ENTER(METHOD_CHANGE, in, out, rc);
			break;

		case RADIUS_SERVERS_DEAD:
			AUTH_MGR_ENTER(RADIUS_SERVERS_DEAD, in, out, rc);
			break;

		case RADIUS_FIRST_PASS_DACL_DATA:
			AUTH_MGR_ENTER(RADIUS_FIRST_PASS_DACL_DATA, in, out, rc);
			break;

		case RADIUS_DACL_INFO:
			AUTH_MGR_ENTER(RADIUS_DACL_INFO, in, out, rc);
			break;

		case MKA_PEER_TIMEOUT:
			AUTH_MGR_ENTER(MKA_PEER_TIMEOUT, in, out, rc);
			break;

		default:
			rc = -1;

	}
return rc;
}

int wpa_sync_send(char *ctrl_ifname, char * cmd, char *buf, size_t *len)
{
	static struct wpa_ctrl *ctrl_conn;
	int ret;
	char sock_file[128];

	memset(sock_file, 0, sizeof(sock_file));
	sprintf(sock_file, "/var/run/hostapd/%s", ctrl_ifname);

	ctrl_conn = wpa_ctrl_open(sock_file);

	if (ctrl_conn ==  NULL)
	{
	  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
		  "Not connected to hostapd - command dropped.. retrying..\n");
	  usleep(10 * 1000);

	  ctrl_conn = wpa_ctrl_open(sock_file);

	  if (ctrl_conn ==  NULL)
	  {
		AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
         "Not connected to hostapd - command dropped..\n");
		return -1;
	  }
	}      
      
	*len = sizeof(buf) - 1;
	ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), buf, len,  NULL);
	if (ret == -2) {
		AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
         "'%s' command timed out.\n", cmd);
		return -2;
	} else if (ret < 0) {
		AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
         "'%s' command failed.\n", cmd);
		return -1;
	}
	if (1) {
		buf[*len] = '\0';
		AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"hostapd reply %s", buf);
	}

	wpa_ctrl_close(ctrl_conn);
	return 0;
}               

/* function to receive multiple connections */

/* Start listening socket listen_sock. */
int start_listen_socket(int *listen_sock)
{
	int reuse = 1;
	struct sockaddr_in my_addr;

	// Obtain a file descriptor for our "listening" socket.
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
	my_addr.sin_addr.s_addr = inet_addr(SERVER_IPV4_ADDR);
	my_addr.sin_port = htons(SERVER_LISTEN_PORT);

	if (bind(*listen_sock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) != 0) {
		perror("bind");
		return -1;
	}

	// start accept client connections
	if (listen(*listen_sock, 144) != 0) {
		perror("listen");
		return -1;
	}
	AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
      "Accepting connections on port %d.\n", (int)SERVER_LISTEN_PORT);

	return 0;
}

static void close_connection(int conn_socket)
{
    /* Close it regardless */
	close(conn_socket);
}

void shutdown_properly(int *listen_sock)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; ++i)
		if (connection_list[i].socket != NO_SOCKET)
			close_connection(connection_list[i].socket);

	close(*listen_sock);
	AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"Shutdown server properly.\n");
	return;
}


int build_fd_sets(fd_set *read_fds, fd_set *write_fds, fd_set *except_fds, int listen_sock)
{

	FD_ZERO(read_fds);
	FD_SET(listen_sock, read_fds);
	return 0;
}  

/* Receive message from peer and handle it with message_handler(). */
int read_from_connection(int socket, char *buf, unsigned int buf_len, 
                         unsigned int *bytes_read, bool *more_data)
{
	size_t len_to_receive;
	ssize_t received_count = 0;
	size_t received_total = 0;

	char *buf_ptr = buf;

    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
      "Entering read_from_connection fd %d buf_len %d\n", socket, buf_len);

	do {
		if (received_total >= buf_len)
		{
			/* still more to copy */
			*more_data = true;
	        *bytes_read = received_total;
			AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
             "fd: %d There is more data , Read %zd bytes till now\n", 
             socket, received_count);
			return 0;
		}

		/* Count bytes to receive.*/
		len_to_receive = buf_len;
		if (len_to_receive > MAX_SEND_SIZE)
			len_to_receive = MAX_SEND_SIZE;

		AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0, 
             "fd: %d Let's try to recv() %zd bytes... ", socket, len_to_receive);
		
        received_count = recv(socket, buf_ptr, len_to_receive, NULL);
		
        if (received_count < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
                  "fd %d peer is not ready right now, try again later.\n", socket);
				continue;
			} else {
				perror("recv() from peer error");
				*more_data = false;
				return -1;
			}
		}
		/* If recv() returns 0, it means that peer gracefully shutdown. Shutdown client.*/
		else if (received_count == 0) {
			AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
              "fd %d recv() 0 bytes. Peer gracefully shutdown.\n", socket);
			*more_data = false;
			break;
		}
		else if (received_count > 0) {
			received_total += received_count;
			AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
              "fd %d recv() %zd bytes\n", socket, received_count);

			buf_ptr += received_count;
			len_to_receive -= received_count;
		}
	} while (received_count > 0);

	*bytes_read = received_total;

	AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
       "fd %d Total recv()'ed %zu bytes.\n", socket, received_total);
	return 0;
}


void *handle_connection(void *arg)
{
	int new_socket = *((int *)arg);
	char *recv_buff = NULL;
	char *buf = NULL;
	unsigned int bytes_received;
	bool more_data = true;
    unsigned int buff_step_size = 2048; 
	int rem_len = 4* buff_step_size;
	int buff_size = 4* buff_step_size;
	clientStatusReply_t *clientReply =  NULLPTR;
	authmgrClientStatusInfo_t clientStatus;
	int total_read = 0;
	uint32 intf = 0;
  uint32 method = 0, status = 0;
  void *in = NULL;
  void *out = NULL;
  int rc = -1;
	 char8 paeCapabilities = 0;

  int i;

  recv_buff = (char *)malloc(buff_size);

  if (!recv_buff)
     goto conn_close;

  memset(recv_buff, 0, buff_size);
  buf = recv_buff;

	while(more_data)
	{
		more_data = false;
		if (rem_len <= 0)
		{
			AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
             "fd %d More data to read, but not sufficient buffer !!\n", new_socket);
             buff_size += buff_step_size;
             recv_buff = (char *)realloc(recv_buff, buff_size);
             buf = recv_buff + total_read;
		     rem_len = buff_size - total_read;
		}
		bytes_received = 0;
		if (0 != read_from_connection(new_socket, buf, rem_len, &bytes_received, &more_data))
		{
			break;
		}
		total_read += bytes_received;
        buf = recv_buff + total_read;
		rem_len = buff_size - total_read;
	}

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"fd %d : buffer: total_read  %d", new_socket, total_read);

  close(new_socket);

   if (extra_detail_logs)
   {
     char *ptr = recv_buff;
     for (i =0; i<10; i++)
     {
       AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"0x%x ", ptr[i]);  
     }
     AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"\nstart processing !!\n");
   } 
	/* invoke api to decipher the data 
	 * and post to the appropriate queue */

	memset(&clientStatus, 0, sizeof(clientStatus));

	clientReply = (clientStatusReply_t *)recv_buff;

    if (clientReply->status)
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
      "Read update from socket for user %02X:%02X:%02X:%02X:%02X:%02X  on interface %s"
       "status %s method %s",
      (unsigned int)clientReply->info.authInfo.addr[0],
      (unsigned int)clientReply->info.authInfo.addr[1], 
      (unsigned int)clientReply->info.authInfo.addr[2],
      (unsigned int)clientReply->info.authInfo.addr[3],
      (unsigned int)clientReply->info.authInfo.addr[4],
      (unsigned int)clientReply->info.authInfo.addr[5],
      clientReply->intf, authmgrMethodStatusStringGet(clientReply->status), clientReply->method);

	/* convert char intf to intf number */
       in = (void *)clientReply->intf;
       out = (void *)&intf;

       AUTH_MGR_ENTER(INTERFACE, in, out, rc);

       AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"AUTH_MGR_ENTER INTERFACE !! rc %d \n", rc);

      if (-1 == rc)
          goto conn_close;

  (void)authmgrDot1xPortPaeCapabilitiesGet(intf, &paeCapabilities);
  if ( DOT1X_PAE_PORT_AUTH_CAPABLE != paeCapabilities)
          goto conn_close;

       /* copy the method */
       in = (void *)clientReply->method;
       out = (void *)&method;

       AUTH_MGR_ENTER(METHOD, in, out, rc);
       AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"AUTH_MGR_ENTER METHOD !! rc %d \n", rc);
      if (-1 == rc)
        goto conn_close;

      status = clientReply->status;

      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"client reply Status %d \n", status);
      rc = auth_mgr_status_params_copy(&clientStatus, clientReply);
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"AUTH_MGR_ENTER PARAMS COPY !! rc %d \n", rc);
      if (-1 == rc)
        goto conn_close;

       AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,"AUTH_MGR_ENTER status update !! rc %d \n", rc);
	authmgrPortClientAuthStatusUpdate (intf, method,
			status, (void *) &clientStatus);

conn_close:
    if (recv_buff)
      free(recv_buff);
	free (arg);
    pthread_exit(NULL);
}

int open_new_connection(int new_client_sock)
{
  pthread_attr_t tattr;
  static int cl_count;
  int ret = 0;
  int *new_sock;

  if ((ret = pthread_attr_init(&tattr)) !=0)
  {
	AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, 0,
     "Failed to create thread - pthread_attr_init failed with %d\n", ret);
	close(new_client_sock);
	return -1;
  }

  if ((ret = pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) !=0)
  {
	AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, 0,
     "Failed to create thread - pthread_attr_setdetachstate failed with %d\n", ret);
	close(new_client_sock);
	pthread_attr_destroy(&tattr);
	return -1;
  }

  new_sock = malloc(sizeof(*new_sock));
  *new_sock = new_client_sock;

  connection_list[cl_count].socket = new_client_sock;
  connection_list[cl_count].tid = TID_INIT;

  if ((ret = pthread_create(&connection_list[cl_count].tid, &tattr,
		  handle_connection, (void*)new_sock)) !=0)
  {
	AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, 0,
		"Failed to create thread with %d\n", ret);
	free(new_sock);
	close_connection(new_client_sock);
	pthread_attr_destroy(&tattr);
	return -1;
  }
  cl_count = (cl_count+1)%MAX_CLIENTS;
  pthread_attr_destroy(&tattr);
  return 0;
}


int handle_async_resp_data(int *listen_sock)
{
	int i;
	int new_client_sock = -1;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	char client_ipv4_str[INET_ADDRSTRLEN];
    struct linger sl;


	if (start_listen_socket(listen_sock) != 0) {
		return -1;
	}

	for (i = 0; i < MAX_CLIENTS; ++i) 
	{
		connection_list[i].socket = NO_SOCKET;
		connection_list[i].tid = TID_INIT;
	}

	while (1) 
	{
		new_client_sock = -1;
		memset(&client_addr, 0, sizeof(client_addr));

		new_client_sock = accept(*listen_sock, (struct sockaddr *)&client_addr, &client_len);
		if (new_client_sock < 0) {
            AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, 0, "accept failed");
			continue;
		}

        sl.l_onoff = 1;  /* enable linger option */
        sl.l_linger = 0;    /* timeout interval in seconds */
        if (-1 == setsockopt(new_client_sock, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)))
        {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
            "unable to set SO_LINGER option socket with fd: %d\n", new_client_sock);
        }


		inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);

        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, 0,
                        "received from client fd %d [%s:%u] ",
                       new_client_sock, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		open_new_connection(new_client_sock);
	}
	return 0;
}

int authmgrMabDataSend(mab_pac_cmd_t *req, char *resp, unsigned int *len)
{
  struct sockaddr_in saddr;
  int fd, rc;
  struct hostent *local_host;
  char *ptr = (char *)req;
  struct sockaddr_in client;
  socklen_t clientlen = sizeof(client);

  /* open the socket to MAB server */
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
  {
    fprintf(stderr, "socket failed [%s]\n", strerror(errno));
    return -1;
  }

  saddr.sin_family = AF_INET;  
  saddr.sin_port = htons(3734);     
  local_host = gethostbyname("127.0.0.1");
  saddr.sin_addr = *((struct in_addr *)local_host->h_addr);

  rc = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
  if (-1 == rc)
  {
    fprintf(stderr, "connect failed [%s]\n", strerror(errno));
    goto close_soc;
  }

  getsockname(fd, (struct sockaddr *) &client, &clientlen);

  AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_EVENTS, 0,
      "The Socket is now connected fd %d [%s:%u] ", 
      fd, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

  /* Send the command req */
  rc = send(fd, ptr, sizeof(mab_pac_cmd_t), 0);
  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
    "fd : %d Successfully sent data (len %lu bytes): %s",
	fd,  sizeof(mab_pac_cmd_t), req->cmd);

  /* read the resp */
    rc = recv(fd, resp, *len, NULL);

    *len = 0;
    if (rc)
    {
      *len = rc;
    }

close_soc:
  close(fd);
  return 0;
}




RC_t auth_mgr_eap_socket_create( int32 *fd)
{
  *fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_PAE));
  if (*fd < 0) {
    printf("%s, %d socket[PF_PACKET,SOCK_RAW]: %s",
        __func__, __LINE__, strerror(errno));
    return  FAILURE;
  }
return  SUCCESS;


}
