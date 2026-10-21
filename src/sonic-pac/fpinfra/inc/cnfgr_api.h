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

#ifndef  CNFGR_API_H
#define  CNFGR_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pacinfra_common.h"
//#include "async_event_api.h"
//#include "syncdb_api.h"

#define  CNFGR_VER_1      0x1

#define CNFGR_IN   const
#define CNFGR_OUT
#define CNFGR_INOUT

#define  DEFAULT_UNCONFIG_DELAY 5000

/*
=================================================
=================================================
=================================================

   Basic Data Types

=================================================
=================================================
=================================================
*/

/* Enumerated values for configurator commands */
typedef enum  CNFGR_CMD_e
{
     CNFGR_CMD_FIRST      = 0,    /* Range Checking */

     CNFGR_CMD_INITIALIZE,
     CNFGR_CMD_EXECUTE,
     CNFGR_CMD_UNCONFIGURE,
     CNFGR_CMD_TERMINATE,
     CNFGR_CMD_SUSPEND,
     CNFGR_CMD_RESUME,

     CNFGR_CMD_LAST              /* Range Checking */

}  CNFGR_CMD_t;

/* Enumerated values for configurator command types */
typedef enum  CNFGR_CMD_TYPE_e
{
     CNFGR_CMD_TYPE_INVALID = 0,
     CNFGR_RQST = 1,
     CNFGR_EVNT

}  CNFGR_CMD_TYPE_t;


/* Enumerated values for request to the component */
typedef enum  CNFGR_RQST_e
{
     CNFGR_RQST_INVALID = 0,

    /* First request  ---- range checking */
     CNFGR_RQST_FIRST = 30,

    /* Component Initialization Requests. */
     CNFGR_RQST_I_PHASE1_START,
     CNFGR_RQST_I_PHASE2_START,
     CNFGR_RQST_I_PHASE3_START,
     CNFGR_RQST_I_WAIT_MGMT_UNIT,

    /* Component Execute Request          */
     CNFGR_RQST_E_START,

    /* Component Unconfigure Request      */
     CNFGR_RQST_U_PHASE1_START,
     CNFGR_RQST_U_PHASE2_START,

    /* Component Termination Request      */
     CNFGR_RQST_T_START,

    /* Component Suspend Request          */
     CNFGR_RQST_S_START,

    /* Component Resume Request           */
     CNFGR_RQST_R_START,

    /* Last Request --- Range checking    */
     CNFGR_RQST_LAST

}  CNFGR_RQST_t;

/* Enumerated values for external event notifications to the configurator */
typedef enum  CNFGR_EVNT_e
{
    /* First Event -- range checking      */
     CNFGR_EVNT_FIRST = 50,

    /* Component Initialization events    */
     CNFGR_EVNT_I_START,
     CNFGR_EVNT_I_MGMT_UNIT,

    /* Component Unconfigure events       */
     CNFGR_EVNT_U_START,
     CNFGR_EVNT_U_PHASE1_CMPLT,

    /* Component Termination event        */
     CNFGR_EVNT_T_START,

    /* Component Suspend event            */
     CNFGR_EVNT_S_START,

    /* Component Resume event             */
     CNFGR_EVNT_R_START,

    /* Last Event --Range checking        */
     CNFGR_EVNT_LAST

}  CNFGR_EVNT_t;


/* Enumerated values for error reason code      */
typedef enum  CNFGR_ERR_RC_e
{
     CNFGR_ERR_RC_INVALID = 0,
     CNFGR_ERR_RC_FIRST = 80,    /* Range checking */

     CNFGR_ERR_RC_FATAL,
     CNFGR_ERR_RC_LACK_OF_RESOURCES,
     CNFGR_ERR_RC_NOT_AVAILABLE,
     CNFGR_ERR_RC_NOT_FOUND,
     CNFGR_ERR_RC_BUSY,
     CNFGR_ERR_RC_IGNORED,
     CNFGR_ERR_RC_INTERNAL,
     CNFGR_ERR_RC_INVALID_ID,
     CNFGR_ERR_RC_INVALID_DATA,
     CNFGR_ERR_RC_INVALID_RSQT,
     CNFGR_ERR_RC_INVALID_EVNT,
     CNFGR_ERR_RC_INVALID_HANDLE,
     CNFGR_ERR_RC_INVALID_CMD,
     CNFGR_ERR_RC_INVALID_CMD_TYPE,
     CNFGR_ERR_RC_INVALID_PAIR,
     CNFGR_ERR_RC_OUT_OF_SEQUENCE,

     CNFGR_ERR_RC_LAST           /* Range checking */

}  CNFGR_ERR_RC_t;

/* Enumerated values for configurator reponse              */
/* Valid if callback return code is  CNFGR_CB_RC_SUCCESS */
typedef enum CNFGR_RESPONSE_e
{
     CNFGR_RESPONSE_INVALID = 0,
     CNFGR_CMD_COMPLETE = 111,
     CNFGR_INIT_COMPLETE,
     CNFGR_INVALID_RESPONSE

}  CNFGR_RESPONSE_t;

/* Configuration Data definition */
typedef uint32  CNFGR_DATA_t;


/* Callback Handle and correlator definitions */
typedef uint32  CNFGR_CORRELATOR_t,
                   CNFGR_CB_HANDLE_t;

#define  CNFGR_NO_HANDLE  ( CNFGR_CB_HANDLE_t) NULL

/* Normalized component states */
typedef enum  CNGFR_STATE_e
{
     CNFGR_STATE_INVALID = 0,

    /* Range checking           */
     CNFGR_STATE_NULL = 150,

     CNFGR_STATE_IDLE,

    /* Initializing             */
     CNFGR_STATE_P1,
     CNFGR_STATE_P2,
     CNFGR_STATE_P3,
     CNFGR_STATE_WMU,

    /* Executing                */
     CNFGR_STATE_E,

    /* Unconfiguring            */
     CNFGR_STATE_U1,
     CNFGR_STATE_U2,

    /* Terminated               */
     CNFGR_STATE_T,

    /* Suspended                */
     CNFGR_STATE_S,

    /* Resuming                 */
     CNFGR_STATE_R,

    /* Range checking           */
     CNFGR_STATE_LAST

}  CNFGR_STATE_t;

/* Access definitions */

typedef enum  CNFGR_QUERY_RQST_e
{
     CNFGR_QRY_RQST_FIRST  = 180,

     CNFGR_QRY_RQST_STATE,
     CNFGR_QRY_RQST_PRESENT,
     CNFGR_QRY_RQST_ALL_PRESENT,
     CNFGR_QRY_RQST_FEATURE,
     CNFGR_QRY_RQST_MODE,
     CNFGR_QRY_RQST_STATUS,
     CNFGR_QRY_RQST_CMDRQST,
     CNFGR_QRY_RQST_MNEMONIC,
     CNFGR_QRY_RQST_NAME,
     CNFGR_QRY_RQST_HW_APPLY_PHASES,

     CNFGR_QRY_RQST_LAST

}  CNFGR_QUERY_RQST_t;

typedef enum  CNFGR_QUERY_TYPE_e
{
   CNFGR_QRY_TYPE_SINGLE  = 195,
   CNFGR_QRY_TYPE_LIST
}  CNFGR_QUERY_TYPE_t;

typedef enum  CNFGR_COMPONENT_MODE_e
{
     CNFGR_COMPONENT_ENABLE  = 201,
     CNFGR_COMPONENT_DISABLE

}  CNFGR_COMPONENT_MODE_t;

typedef enum  CNFGR_COMPONENT_STATUS_e
{
     CNFGR_COMPONENT_ACTIVE  = 251,
     CNFGR_COMPONENT_INACTIVE

}  CNFGR_COMPONENT_STATUS_t;

/* Definition of the hardware reconciliation phases */
typedef enum  CNFGR_HW_APPLY_e
{
	 CNFGR_HW_APPLY_CONFIG = 1,
	 CNFGR_HW_APPLY_L3 = 2,
	 CNFGR_HW_APPLY_IPMCAST = 4,

  /* Number of reconciliation phases */
   CNFGR_HW_APPLY_NUM_PHASES = 3

}  CNFGR_HW_APPLY_t;

/* Initialization request flags sent to components. 
 * pCmdData->u.rqstData.data */
typedef enum  CNFGR_REQUEST_FLAGS_e
{
  /* When this bit is on a p3 init indication, the restart is warm. When the 
   * bit is not set, the restart is cold. */
   CNFGR_RESTART_WARM = 1,

}  CNFGR_RESTART_TYPE_t;

/*
=================================================
=================================================
=================================================

   CONFIGURATOR INITIALIZATION DATA

=================================================
=================================================
=================================================
*/
typedef uint32  CNFGR_INIT_DATA_t;



/*
=================================================
=================================================
=================================================

   COMMAND FROM/TO THE CONFIGURATOR

=================================================
=================================================
=================================================
*/

/* Request data structure */
typedef struct {

     CNFGR_RQST_t request;
     CNFGR_DATA_t data;

}  CNFGR_RQST_DATA_t;


/* Event data structure */
typedef struct {

     CNFGR_EVNT_t event;
     CNFGR_DATA_t data;

}  CNFGR_EVNT_DATA_t;

/* Command from (output)/to(input) the configurator structure  */
typedef struct {

     CNFGR_CORRELATOR_t  correlator;
     CNFGR_CB_HANDLE_t   cbHandle;
     CNFGR_CMD_t         command;
     CNFGR_CMD_TYPE_t    type;
    union{

         CNFGR_RQST_DATA_t rqstData;       /* output */
         CNFGR_EVNT_DATA_t evntData;       /* input  */

    }u;

}  CNFGR_CMD_DATA_t;


/*
=================================================
=================================================
=================================================

   COMPLETION CALLBACKS

=================================================
=================================================
=================================================
*/

/* Asynchronous Response Structure */
typedef struct {

    RC_t                     rc;
    union {
         CNFGR_ERR_RC_t       reason;
         CNFGR_RESPONSE_t     response;
    }u;

}  CNFGR_ASYNC_RESPONSE_t;

/* Callback data structure        */
typedef struct {

     CNFGR_CORRELATOR_t       correlator;
     CNFGR_ASYNC_RESPONSE_t   asyncResponse;

}  CNFGR_CB_DATA_t;


/*
=================================================
=================================================
=================================================

   CONFIGURATOR QUERY DATA

=================================================
=================================================
=================================================
*/
typedef struct {

     CNFGR_QUERY_TYPE_t type;                 /* input */
    union {
       COMPONENT_IDS_t    cid;                /* input */
      uint32             *cid_list;          /* output */
    } args;
     CNFGR_QUERY_RQST_t request;              /* input */
     CNFGR_ERR_RC_t     reason;               /* ouput */
    union {
        struct {
             CNFGR_STATE_t current;
             CNFGR_STATE_t next;
             CNFGR_STATE_t saved;
        } state;
         BOOL                     present;
         CNFGR_COMPONENT_MODE_t   mode;
         CNFGR_COMPONENT_STATUS_t status;
        struct {
             CNFGR_CMD_t            cmd;
             CNFGR_RQST_t           rqst;
             CNFGR_ASYNC_RESPONSE_t aRsp;
        } cmdRqst;
         char8                    *name;
        uint32                    hwApplyPhases;
    } data;                                     /* output */

}  CNFGR_QUERY_DATA_t;

/*
=================================================
=================================================
=================================================

   CONFIGURATOR INTERFACE

=================================================
=================================================
=================================================
*/

#ifdef __cplusplus
}
#endif

#endif /*  CNFGR_API_H */
