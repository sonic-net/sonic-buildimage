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

#include <netdb.h>
#include <netinet/in.h>
#include "includes.h"
#include "pacinfra_common.h"
#include "mab_include.h"
#include "radius_attr_parse.h"
#include "packet.h"
#include "osapi.h"
#include "auth_mgr_exports.h"
#include "fpSonicUtils.h"

#define STATUS_COPY(_status)  static void _status##_##copy(char *intf, clientStatusReply_t *reply, char *addr, void *param)
#define STATUS_ENTER(_status, _intf, _reply, _addr, _param)  _status##_##copy(_intf, _reply, _addr,  _param)
#define ETHERNET_PREFIX "Ethernet"

#define STATUS_MALLOC(__status, __data, _reply)  \
  do { \
       reply = (clientStatusReply_t *)malloc(sizeof(*_reply)); \
       memset(_reply, 0, sizeof(*_reply)); \
  } while(0); 


typedef struct status_map_s
{
 char *status;
 unsigned int val;
}status_map_t;

static int mab_data_async_send (char *buf, int bufLen, unsigned char *addr) 
{
  struct sockaddr_in saddr;
  int fd, ret_val;
  struct hostent *local_host;	/* need netdb.h for this */
  int i;
  char *ptr = buf;
  struct linger sl;
  struct sockaddr_in client;
  socklen_t clientlen = sizeof(client);

  printf("buffer: ");  
  for (i =0; i<10; i++)
  {
    printf("0x%x ", ptr[i]);  
  }
  printf("\n");  

	/* Step1: create a TCP socket */
	fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (fd == -1)
	{
		fprintf(stderr, "socket failed [%s]\n", strerror(errno));
		return -1;
	}

     MAB_EVENT_TRACE(
	"Created a socket with fd: %d", fd);

	/* Let us initialize the server address structure */
	saddr.sin_family = AF_INET;         
	saddr.sin_port = htons(3434);     
	local_host = gethostbyname("127.0.0.1");
	saddr.sin_addr = *((struct in_addr *)local_host->h_addr);

	/* Step2: connect to the server socket */
	ret_val = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret_val == -1)
	{
		fprintf(stderr, "connect failed [%s]\n", strerror(errno));
		close(fd);
		return -1;
	}
    getsockname(fd, (struct sockaddr *) &client, &clientlen);

    if (addr)
    {
     MAB_EVENT_TRACE(
       " The Socket is now connected fd %d [%s:%u]"
       " mac (%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x)",
       fd, inet_ntoa(client.sin_addr), ntohs(client.sin_port), 
       addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    }
    else
    {
     MAB_EVENT_TRACE(
       "The Socket is now connected fd %d [%s:%u] ", 
       fd, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    }

    sl.l_onoff = 1;  /* enable linger option */
    sl.l_linger = 30;    /* timeout interval in seconds */
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)))
    {
     MAB_EVENT_TRACE(
      "unable to set SO_LINGER option socket with fd: %d", fd);
    }

	/* Next step: send some data */
	ret_val = send(fd, buf, bufLen, 0);
     MAB_EVENT_TRACE(
	   "fd : %d Successfully sent data (len %d bytes): %s", 
				 fd, bufLen, buf);

	/* Last step: close the socket */
	close(fd);
	return 0;
}

STATUS_COPY(AUTH_SUCCESS)
{
	authmgrClientStatusInfo_t *clientInfo = (authmgrClientStatusInfo_t *)param;
	memset(reply, 0, sizeof(*reply));

	if (addr)
	{
		memcpy(reply->info.authInfo.addr, addr, 6);
	}
	osapiStrncpySafe (reply->method, "mab", sizeof(reply->method)-1);
	reply->status = AUTH_SUCCESS; 
	osapiStrncpySafe (reply->intf, intf, sizeof(reply->intf)-1);

	/* copy user details */
	memcpy(reply->info.authInfo.userName, clientInfo->info.authInfo.authmgrUserName,
                min(sizeof(clientInfo->info.authInfo.authmgrUserName), sizeof(clientInfo->info.authInfo.authmgrUserName)));
	reply->info.authInfo.userNameLength = strlen(clientInfo->info.authInfo.authmgrUserName);

	/* copy bam used for authentication */
	osapiStrncpySafe(reply->info.authInfo.bam_used, "radius", sizeof(reply->info.authInfo.bam_used)-1);

	/* copy all the attributes received from radius/ or backend method */
        memcpy(&reply->info.authInfo.attrInfo, &clientInfo->info.authInfo.attrInfo, sizeof(attrInfo_t));
}



STATUS_COPY(AUTH_FAIL)
{
	authmgrClientStatusInfo_t *clientInfo = (authmgrClientStatusInfo_t *)param;

	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	osapiStrncpySafe (reply->method, "mab", sizeof(reply->method)-1);
	reply->status = AUTH_FAIL; 
	osapiStrncpySafe (reply->intf, intf, sizeof(reply->intf)-1);

	/* copy user details */
	memcpy(reply->info.authInfo.userName, clientInfo->info.authInfo.authmgrUserName,
                min(sizeof(clientInfo->info.authInfo.authmgrUserName), sizeof(clientInfo->info.authInfo.authmgrUserName)));
	reply->info.authInfo.userNameLength = strlen(clientInfo->info.authInfo.authmgrUserName);
}


STATUS_COPY(AUTH_TIMEOUT)
{
	authmgrClientStatusInfo_t *clientInfo = (authmgrClientStatusInfo_t *)param;

	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	osapiStrncpySafe (reply->method, "mab", sizeof(reply->method)-1);
	reply->status = AUTH_TIMEOUT; 
	osapiStrncpySafe (reply->intf, intf, sizeof(reply->intf)-1);

	/* copy user details */
	memcpy(reply->info.authInfo.userName, clientInfo->info.authInfo.authmgrUserName,
                min(sizeof(clientInfo->info.authInfo.authmgrUserName), sizeof(clientInfo->info.authInfo.authmgrUserName)));
	reply->info.authInfo.userNameLength = strlen(clientInfo->info.authInfo.authmgrUserName);
}


STATUS_COPY(AUTH_SERVER_COMM_FAILURE)
{
	authmgrClientStatusInfo_t *clientInfo = (authmgrClientStatusInfo_t *)param;

	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	osapiStrncpySafe (reply->method, "mab", sizeof(reply->method)-1);
	reply->status = AUTH_SERVER_COMM_FAILURE; 
	osapiStrncpySafe (reply->intf, intf, sizeof(reply->intf)-1);

	/* copy user details */
	memcpy(reply->info.authInfo.userName, clientInfo->info.authInfo.authmgrUserName,
                min(sizeof(clientInfo->info.authInfo.authmgrUserName), sizeof(clientInfo->info.authInfo.authmgrUserName)));
	reply->info.authInfo.userNameLength = strlen(clientInfo->info.authInfo.authmgrUserName);
}


STATUS_COPY(METHOD_CHANGE)
{
	 authmgrClientStatusInfo_t *clientInfo = (authmgrClientStatusInfo_t *)param;

	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	osapiStrncpySafe (reply->method, "mab", sizeof(reply->method)-1);
	reply->status = METHOD_CHANGE; 

	osapiStrncpySafe (reply->intf, intf, sizeof(reply->intf)-1);

  if (clientInfo->info.enableStatus)
  {
    osapiStrncpySafe (reply->info.enableStatus, "enable", strlen("enable")+1);
  }
  else
  {
    osapiStrncpySafe (reply->info.enableStatus, "disable", strlen("disable")+1);
  }

}

STATUS_COPY(RADIUS_SERVERS_DEAD)
{
	authmgrClientStatusInfo_t *clientInfo = (authmgrClientStatusInfo_t *)param;

	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	osapiStrncpySafe (reply->method, "mab", sizeof(reply->method)-1);
	reply->status = RADIUS_SERVERS_DEAD; 
	osapiStrncpySafe (reply->intf, intf, sizeof(reply->intf)-1);

	/* copy user details */
	memcpy(reply->info.authInfo.userName, clientInfo->info.authInfo.authmgrUserName,
                min(sizeof(clientInfo->info.authInfo.authmgrUserName), sizeof(clientInfo->info.authInfo.authmgrUserName)));
	reply->info.authInfo.userNameLength = strlen(clientInfo->info.authInfo.authmgrUserName);
}

static int client_resp_val_get(char *in)
{
  unsigned int i = 0;

  static status_map_t status_map[] = {
		{"auth_fail", AUTH_FAIL},
		{"auth_success", AUTH_SUCCESS},
		{"auth_timeout", AUTH_TIMEOUT},
		{"auth_server_comm_failure", AUTH_SERVER_COMM_FAILURE},
		{"method_change", METHOD_CHANGE},
		{"radius_server_dead", RADIUS_SERVERS_DEAD},
	};

	for (i = 0; i < sizeof(status_map)/sizeof(status_map_t); i++)
	{
		if (0 == strcmp(in, status_map[i].status))
		{
			return status_map[i].val;
		}
	}
	return -1;
}

int mabPortClientAuthStatusUpdate(int intIfNum, unsigned char *addr, char *status, void *param)
{
  clientStatusReply_t *reply;
  unsigned int val = 0;
   char8 intf[ NIM_IF_ALIAS_SIZE + 1] = {};

  fpGetHostIntfName(intIfNum, intf);

  val = client_resp_val_get(status);

  STATUS_MALLOC(val, param, reply);

 switch (val)
 {
  case AUTH_FAIL:
  STATUS_ENTER(AUTH_FAIL, intf, reply, addr, param);
  break;

  case AUTH_SUCCESS:
  STATUS_ENTER(AUTH_SUCCESS, intf, reply, addr, param);
  break;

  case AUTH_TIMEOUT:
  STATUS_ENTER(AUTH_TIMEOUT, intf, reply, addr, param);
  break;

  case AUTH_SERVER_COMM_FAILURE:
  STATUS_ENTER(AUTH_SERVER_COMM_FAILURE, intf, reply, addr, param);
  break;

  case METHOD_CHANGE:
  STATUS_ENTER(METHOD_CHANGE, intf, reply, addr, param);
  break;

  case RADIUS_SERVERS_DEAD:
  STATUS_ENTER(RADIUS_SERVERS_DEAD, intf, reply, addr, param);
  break;

  default:
    /* unknown response is received */
    return -1;

 }

  /* send msg */
  mab_data_async_send((char *)reply, sizeof(*reply), addr);

  if (reply)
     free (reply);
  return 0;
}


