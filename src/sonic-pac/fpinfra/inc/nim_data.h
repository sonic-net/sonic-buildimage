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


#ifndef NIM_DATA_H
#define NIM_DATA_H

#include "product.h"
#include "nimapi.h"
#include "osapi.h"
#include "avl_api.h"
#include "pacinfra_common.h"
#include "cnfgr_api.h"

#define  NIM_UNUSED_PARAMETER   0

/*  fileHdr_t copied from  comm_structs.h*/
#define  MAX_FILENAME 32
#define  FILE_HDR_PAD_LEN     7
typedef struct
{
   uchar8   filename[ MAX_FILENAME];    /* file name                       */
  uint32   version;                      /* software version of file        */
 COMPONENT_IDS_t componentID;
  uint32   type;                         /* type of data-component specific */
  uint32   length;                       /* length of data, including hdr   */
  uint32   dataChanged;                  /* file changed if not NULL        */
  uint32   savePointId;                  /* random number generator */
  uint32   targetDevice;                 /* Identifies the hardware compatible with this file */
  uint32   fileHdrVersion;               /*  fileHdr_t version */
  uint32   pad[ FILE_HDR_PAD_LEN];     /* Space for future enhancements. */
}
 fileHdr_t;

/* Type for the configuration info that will be saved per port in the configuration file */
typedef struct
{
  nimConfigID_t   configInterfaceId;	/* This is the configuration interface ID 64 bit field */

  uint32       configIdMaskOffset;   /* a mask offset to be used for Config files- equals value of intIfNum */
  
  nimIntfConfig_t cfgInfo;              /* provided by the creator of interface or defaults */  

} nimCfgPort_t;

/* Type for the header of the configuration file */
typedef struct
{
   fileHdr_t   	cfgHdr;

  uint32       numberOfInterfaces;	/* Number of created interfaces in the file */

} nimConfigHeaderData_t;

/* type for the format of the configuration file */
typedef struct
{
   fileHdr_t      cfgHdr;
  
  uint32         numOfInterfaces;	/* Number of created interfaces */

  NIM_INTF_MASK_t   configMaskBitmap;  /* a bitmap to determine the used maskoffsets */

  nimCfgPort_t   *cfgPort;		/* an array of the ports configuration */

 
  uint32 nimDDisableAutorecoveryStatus; /* Autorecovery from DDisable feature admin status */

  uint32 nimDDisableAutorecoveryTimeout; /* A time in seconds to automatic recovery from DDisable state */

  uint32 nimDDisableStormControlAutorecoveryTimeout; /* A time in seconds to automatic recovery from D-Disable state for Storm Control reason. */

  uint32      *checkSum;		/* check sum of config file NOTE: must be last entry */

} nimConfigData_t;



/* Type for the interface change callback registration list */
typedef struct
{
   COMPONENT_IDS_t registrar_ID; 

  PORTEVENT_MASK_t   notify_mask;
  RC_t (*notify_intf_change)(uint32 port, uint32 event,
                                NIM_CORRELATOR_t correlator,
                                NIM_EVENT_SPECIFIC_DATA_t eventData);	

} nimNotifyList_t;


/* Type for the NIM specific message format */
typedef struct
{

  NIM_CORRELATOR_t        correlator;
  NIM_EVENT_NOTIFY_INFO_t eventInfo;

} nimMessage_t; 

/* Type for the NIM start message */
typedef struct
{
   COMPONENT_IDS_t  componentId;
  StartupNotifyFcn    startupFunction;
  NIM_STARTUP_PHASE_t phase;

} nimStartMsg_t; 

/* Type for differentiating between either NIM or the CNFGR messages in the nimTask */
typedef enum 
{
  NIM_MSG = 1,
  CNFGR_MSG,
  START_MSG,
  TIMER_MSG,
  PHY_MSG,
  ISSU_RESTORE_MSG
} nimMsgTypes_t;

/* Type for the NIM pdu, which can have a NIM payload or CNFGR payload */
typedef struct
{
  nimMsgTypes_t    msgType;

  union
  {
    nimMessage_t      message;  /* What interface and event to dispatch to registered apps */

     CNFGR_CMD_DATA_t   pCmdData; /* The information passed by the CNFGR to NIM for state transitions */
    nimStartMsg_t     nimStartMsg; 
    //SYSAPI_NIM_CFG_msg_t hpc_msg;

  }data;

} nimPdu_t;

typedef struct
{
   uint64       bitsTxRate;

   uint64       bitsRxRate;

   uint64       bitsTxCount;

   uint64       bitsRxCount;

   uint64       framesTxRate;

   uint64       framesRxRate;

   uint64       framesTxCount;

   uint64       framesRxCount;
} nimRate_t;

/* Type for the runtime information related to an interface, including port mapping information */
typedef struct
{
  uint32      present;                            /* Interface is defined (plugged in or configured */

  uint32      intfNo;                             /* internal port num., 1 to MAX_INTERFACE_COUNT */

  uint32      runTimeMaskId;

  nimConfigID_t  configInterfaceId;                  /* This is the configuration interface ID 64 bit field */

  uint32      sysIntfType;                        /* Intf Type for the system */

  uint32      ifIndex;                            /* ifIndex   */

  uint32      resetTime;                          /* time interface counters were reset */

  uint32      linkChangeTime;                     /* system clock, in seconds, at last link change */

  nimUSP_t       usp;                                /* Unit, Slot, Port */

  uint32      currentLoopbackState;               /*  PHY_LOOPBACK,  MAC_LOOPBACK,  NONE */

  nimCfgPort_t   configPort;                         /* run time config data */

   INTF_STATES_t  intfState;

  nimIntfDescr_t  operInfo;

  nimIntfConfig_t defaultCfg;

   BOOL         defaultCfgSupplied;
  
   BOOL         diagDisabled;
 
  //nimDDisableReason_t diagDisabledReason;

  /* Fields below are for dynamic ports support. They do not need to
     be checkpointed or synced */
   BOOL         dynamicCap;
    /* This means there exists a speed that was configured due to PHY_MSG */
  nimIntfDynConfig_t capabilityCfg;

}nimIntf_t;    

/* Type used to tally the responses to Events */
typedef struct nimCorrelatorData_s
{

   BOOL                 inUse;
  uint32               time;
  NIM_CORRELATOR_t        correlator;
  NIM_EVENT_NOTIFY_INFO_t requestData;
  RC_t                 response;
  uint32               *remainingMask;
  uint32               *failedMask;

}nimCorrelatorData_t;

/* Type for the USP to intIfNum lookup */
typedef struct
{

  nimUSP_t usp;
  uint32 intIfNum;
  void *avlData;

} nimUspIntIfNumTreeData_t; 


/* ConfigId Tree Data structure */
typedef struct
{

  nimConfigID_t  configId;
  uint32     intIfNum;
  void *avlData;

} nimConfigIdTreeData_t; 


/* Intf Type Data structure */
typedef struct
{

  uint32     minIntIfNumber;
  uint32     maxIntIfNumber;
  NIM_INTF_MASK_t intfMask;

} nimIntfTypeData_t; 


#define  NIM_MAX_PORT_COUNT ( MAX_PORT_COUNT +  MAX_NUM_LAG_INTF + 1)
/* An interface table contains time of automatic restored from D-Disable state */
typedef struct
{
  /*uint32 intIfNum;*/       /* Interface number */
  uint32 recoveryTime;       /* When the port should be restored */   
  uint32 ddisableReasonMask; /* Which component put an interface to D-Disable state */
} nimDDisableRecoveryTable_t;


/* Type for the NIM control block */
typedef struct  nimSystemData_s
{
  osapiRWLock_t     rwLock;          /* semaphore for syncronizing NIM */

   CNFGR_STATE_t  nimPhaseStatus;	  /* present configurator state of NIM */

  nimNotifyList_t   *nimNotifyList;		/* an array of the routines registered for events by other components */

  void *         taskId;           /* the task ID for the nimTask */

  void *         hpcTaskId;           /* the task ID for the nimTask */

  uint32   		  ifTableLastChange;/* ifTableLastChange */

  uint32   		  ifNumber;         /* Number of External interfaces created */

  NIM_INTF_MASK_t   linkStateMask;    /* mask of port link up/down */
 
  NIM_INTF_MASK_t   forwardStateMask;	/* mask of port forwarding or not */   /* WPJ: Revisit active mask being removed */

  NIM_INTF_MASK_t   authorizedStateMask;    /* mask of port authorize status */

  NIM_INTF_MASK_t   pfcActiveMask;          /* mask of ports that have PFC active */

  nimIntf_t         *nimPorts;				/* An array of internal interfaces that serves as the primary mapping data */ 

  void 				*nimMsgQueue;			      /* the message queue for CNFGR state changes and port event notifications */
  void              *nimHPCMsgQ;   /* the message queue for HPC messages */

  uint32  		*nimNumberOfPortsPerUnit;	/* an array of number of ports per unit */

  nimUSP_t    		*nimVlanSlotsInfo;	/* an array of the vlan interfaces usp  WPJ_TBD This should be obsolete*/

  uint32   		nimHighestIntfNumber; 	 /* WPJ: Is this useful */   		

  uint32   		maxNumOfUnits;        

  uint32   		maxNumOfSlotsPerUnit; 

  uint32   		maxNumOfPhysicalPortsPerSlot; 

  uint32   		*numberOfInterfacesByType;	/* an array to hold the number of interfaces by type */

  nimIntfTypeData_t *intfTypeData;	/* an array to data specific to an interface type */

   char8    		*nimFileName;	

  //nvStoreFunctionList_t notifyFunctionList;	/* NVRAM storage routines */

  nimConfigData_t  *nimConfigData;  

  nimCorrelatorData_t *pCorrelator_table; 

  NIM_INTF_MASK_t createdMask;

  NIM_INTF_MASK_t physicalIntfMask;

  NIM_INTF_MASK_t lagIntfMask;      /* WPJ: Remove when nimIntfTypeData_t is genericized */

  NIM_INTF_MASK_t cpuIntfMask;

  NIM_INTF_MASK_t vlanIntfMask;
  NIM_INTF_MASK_t l2tnnlIntfMask;

  NIM_INTF_MASK_t servicePortIntfMask;

  NIM_INTF_MASK_t presentMask;

  avlTree_t       nimUspTreeData;
  avlTree_t       nimConfigIdTreeData;       

  uint32       nimPrintLog;
 
 
  uint32 nimDDisableAutorecoveryStatus;
  /* A time in seconds to automatic recovery from DDisable state */
  uint32 nimDDisableAutorecoveryTimeout;
  /* Count of DDisabled ports */
  uint32 dDisabledPortCount;

  /* Table of callbacks to be called to restore an interface from D-Disable */
  /*struct 
  {
    nimDDisableRecoveryCallback_t callback;
     COMPONENT_IDS_t component;
  }
  nimDDisableRecoveryCallbackTable[NIM_DDISABLE_REASON_MAX];*/

  /* An interface table contains time of automatic restored from D-Disable state */
  //nimDDisableRecoveryTable_t nimDDisableRecoveryTable[ NIM_MAX_PORT_COUNT];


  /* system MTU that affects L2 and L3 MTU on the interfaces in the system*/
  uint32       nimSystemMtu;
}nimSystemData_t;

extern nimSystemData_t *nimCtlBlk_g;

#endif /* NIM_DATA_H */
