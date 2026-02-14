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

#ifndef INCLUDE_MAB_STRUCT_H
#define INCLUDE_MAB_STRUCT_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "pacinfra_common.h"
#include "osapi.h"
#include "avl_api.h"
#include "apptimer_api.h"
#include "mab_db.h"
#include "mab_cfg.h"
#include "mab_api.h"
#include "mab_debug.h"
#include "mab_include.h"
#include "portevent_mask.h"
#include "tree_api.h"

typedef struct connection_list_e
{
  int socket;
  pthread_t tid;
}connection_list_t;


typedef struct mabBlock_s
{
  void * mabTaskId;
  void * mabSrvrTaskId;
  void * mabEloopTaskId;
  int  mabServerSock;
  int  send_fd;
  int  recv_fd;
  connection_list_t *conn_list;
  void *rad_cxt;

  mabCfg_t *mabCfg;
  mabInfo_t      mabInfo;
  mabPortInfo_t  *mabPortInfo;
  mabPortStats_t *mabPortStats;
  uint32        *mabMapTbl;

  /* App timer related data */
   APP_TMR_CTRL_BLK_t   mabTimerCB;
  uint32              mabAppTimerBufferPoolId;

   BOOL warmRestart;
   BOOL mabSwitchoverInProgress;

  void *mabQueue;      /* reference to the mab message queue */
  void *mabTaskSyncSema;  

  void *mabRadiusSrvrTaskSyncSema;  

   /* Global parameters */
   avlTree_t                mabLogicalPortTreeDb;
   avlTreeTables_t          *mabLogicalPortTreeHeap;
   mabLogicalPortInfo_t   *mabLogicalPortDataHeap;

   uint32     mabMacAddrBufferPoolId;
    sll_t      mabMacAddrSLL;
   osapiRWLock_t mabMacAddrDBRWLock;

   osapiRWLock_t       mabRWLock;

   mabIpAaddr_t  nas_ip;
   unsigned char nas_id[64];

}mabBlock_t;

/* USE C Declarations */
#ifdef __cplusplus
}
#endif


#endif /* INCLUDE_MAB_STRUCT_H */
