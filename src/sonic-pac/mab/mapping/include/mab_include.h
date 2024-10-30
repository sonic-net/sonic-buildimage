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



#ifndef INCLUDE_MAB_INCLUDE_H
#define INCLUDE_MAB_INCLUDE_H

/*
***********************************************************************
*                           COMMON INCLUDES
***********************************************************************
*/
  /* USE C Declarations */
#ifdef __cplusplus
  extern "C" {
#endif
  
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
*                   MAB HEADER FILES
**********************************************************************
*/
#include "pacinfra_common.h"
#include "osapi.h" 
#include "nimapi.h"

#include "mab_api.h"
#include "mab_common.h"
#include "mab_db.h"
#include "mab_cfg.h"
#include "mab_control.h"
#include "mab_ih.h"
#include "mab_local.h"
#include "mab_debug.h"
#include "mab_sid.h"
#include "mab_mac_db.h"

/* USE C Declarations */
#ifdef __cplusplus
}
#endif
#endif  /* INCLUDE_MAB_INCLUDE_H */
