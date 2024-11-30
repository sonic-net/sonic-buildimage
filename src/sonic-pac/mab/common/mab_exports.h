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

#ifndef __MAB_EXPORTS_H_
#define __MAB_EXPORTS_H_

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#define  MAB_MAX_USERS_PER_PORT        FD_AUTHMGR_PORT_MAX_USERS

#define  MAB_USER_NAME_LEN             65
#define  MAB_CHALLENGE_LEN             32

#define  MAB_CHAP_CHALLENGE_LEN        16
#define  MAB_FILTER_NAME_LEN           256

/* Port protocol version */
typedef enum
{
   MAB_PAE_PORT_PROTOCOL_VERSION_1 = 1
}  MAB_PAE_PORT_PROTOCOL_VERSION_t;

/******************** conditional Override *****************************/

#ifdef INCLUDE_MAB_EXPORTS_OVERRIDES
#include "mab_exports_overrides.h"
#endif

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* __MAB_EXPORTS_H_*/
