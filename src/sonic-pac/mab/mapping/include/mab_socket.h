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


#ifndef MAB_SOCKET_H
#define MAB_SOCKET_H

#define MAB_INTF_STR_LEN 128
#define MAB_CMD_STR_LEN  128

typedef struct mab_pac_cmd_s
{
  char intf[MAB_INTF_STR_LEN];
  char cmd[MAB_CMD_STR_LEN];
  unsigned char mac_addr[6];
  unsigned int notif_event; 
}mab_pac_cmd_t;

int mab_radius_init_send_socket(int *sock);
int mab_radius_init_recv_socket(int *sock);
#endif
