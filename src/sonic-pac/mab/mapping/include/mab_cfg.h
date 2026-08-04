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

#ifndef INCLUDE_MAB_CFG_H
#define INCLUDE_MAB_CFG_H

#include "nim_data.h"
/* USE C Declarations */
#ifdef __cplusplus
  extern "C" {
#endif
#define  PASSWORD_SIZE      MAB_USER_NAME_LEN

extern BOOL mabInitializationState;
#define MAB_IS_READY (mabInitializationState)

typedef struct mabPortCfg_s
{
  nimConfigID_t configId;     /* NIM config ID for this interface*/
  uint32 maxUsers;        /*Maximum no. users in Mac-Based Authentication */
  uint32 mabEnabled;           /*enabled if MAB is enabled for the port*/
  AUTHMGR_PORT_MAB_AUTH_TYPE_t mabAuthType; /* Authentication type to be used by MAB */
} mabPortCfg_t;

typedef struct mabCfg_s
{
  mabPortCfg_t        mabPortCfg[ MAB_INTF_MAX_COUNT]; /* Per-port config info */
} mabCfg_t;

extern mabCfg_t *mabCfg;

extern uint32 mabPhysPortGet(uint32 lIntIfNum);
extern void   mabBuildDefaultConfigData();
extern void   mabBuildDefaultIntfConfigData(nimConfigID_t *configId, mabPortCfg_t *pCfg);

extern RC_t mabApplyConfigData(void);
extern RC_t mabPortInfoInitialize(uint32 intIfNum, BOOL flag);
extern RC_t mabPortReset(uint32 intIfNum);

extern  BOOL mabIsRestartTypeWarm();
extern RC_t mabInit(void);
extern  void mabInitUndo();
extern RC_t mabInitPhase1Process(void);
extern RC_t mabInitPhase2Process(void);
extern RC_t mabInitPhase3Process( BOOL warmRestart);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_MAB_CFG_H */
