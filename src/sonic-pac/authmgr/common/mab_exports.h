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

/* MAB Component Feature List */
typedef enum
{
   MAB_FEATURE_ID = 0,                   /* general support statement */
   MAB_FEATURE_ID_TOTAL,                /* total number of enum values */
}  MAB_FEATURE_IDS_t;

#define  MAB_MAX_USERS_PER_PORT   FD_AUTHMGR_PORT_MAX_USERS

#define  MAB_USER_NAME_LEN             65
#define  MAB_CHALLENGE_LEN             32

#define  MAB_CHAP_CHALLENGE_LEN        16
#define  MAB_FILTER_NAME_LEN           256

/******************************************************************/
/*************       Start MAB types and defines        *********/
/******************************************************************/

/* Port protocol version */
typedef enum
{
   MAB_PAE_PORT_PROTOCOL_VERSION_1 = 1
}  MAB_PAE_PORT_PROTOCOL_VERSION_t;


/* MAB Request Attribute1 Group Size types */
typedef enum
{
   MAB_REQUEST_ATTRIBUTE1_GROUP_SIZE_INVALID = 0,
   MAB_REQUEST_ATTRIBUTE1_GROUP_SIZE_1  = 1,
   MAB_REQUEST_ATTRIBUTE1_GROUP_SIZE_2  = 2,
   MAB_REQUEST_ATTRIBUTE1_GROUP_SIZE_4  = 4,
   MAB_REQUEST_ATTRIBUTE1_GROUP_SIZE_12 = 12
}  MAB_REQUEST_ATTRIBUTE1_GROUP_SIZE_t;

/* MAB Request Attribute1 Separator types */
typedef enum
{
   MAB_REQUEST_ATTRIBUTE1_SEPARATOR_INVALID = 0,
   MAB_REQUEST_ATTRIBUTE1_SEPARATOR_IETF,     /* '-' is used as a separator */
   MAB_REQUEST_ATTRIBUTE1_SEPARATOR_LEGACY,   /* ':' is used as a separator */
   MAB_REQUEST_ATTRIBUTE1_SEPARATOR_DOT       /* '.' is used as a separator */
}  MAB_REQUEST_ATTRIBUTE1_SEPARATOR_t;

/* MAB Request Attribute1 Case types */
typedef enum
{
   MAB_REQUEST_ATTRIBUTE1_CASE_INVALID = 0,
   MAB_REQUEST_ATTRIBUTE1_CASE_UPPER,
   MAB_REQUEST_ATTRIBUTE1_CASE_LOWER
}  MAB_REQUEST_ATTRIBUTE1_CASE_t;





/******************** conditional Override *****************************/

#ifdef INCLUDE_MAB_EXPORTS_OVERRIDES
#include "mab_exports_overrides.h"
#endif

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* __MAB_EXPORTS_H_*/
