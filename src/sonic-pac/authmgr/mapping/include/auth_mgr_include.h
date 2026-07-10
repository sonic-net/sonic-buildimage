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

#ifndef INCLUDE_AUTHMGR_INCLUDE_H
#define INCLUDE_AUTHMGR_INCLUDE_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif
/*
***********************************************************************
*                           COMMON INCLUDES
***********************************************************************
*/
#include "pacinfra_common.h"
#include "osapi.h" 
#include "nimapi.h"
#include "log.h"
#include <stdbool.h>
#include "datatypes.h"
#include "auth_mgr_api.h"

/*
**********************************************************************
*                           STANDARD LIBRARIES
**********************************************************************
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
**********************************************************************
*                   AUTHMGR HEADER FILES
**********************************************************************
*/
#include "auth_mgr_exports.h"
#include "auth_mgr_sid.h"
#include "auth_mgr_sm.h"
#include "auth_mgr_db.h"
#include "auth_mgr_cfg.h"
#include "auth_mgr_control.h"
#include "auth_mgr_ih.h"
#include "auth_mgr_txrx.h"
#include "auth_mgr_debug.h"
#include "auth_mgr_mac_db.h"
#include "auth_mgr_client.h"
#include "auth_mgr_radius.h"
#include "auth_mgr_vlan.h"
#include "auth_mgr_util.h"

#ifdef __cplusplus
}
#endif
#endif  /* INCLUDE_AUTHMGR_INCLUDE_H */
