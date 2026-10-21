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

#ifndef INCLUDE_NIMAPI
#define INCLUDE_NIMAPI

#include "datatypes.h"
#include "commdefs.h"
#include "packet.h"
#include "portevent_mask.h"
#include "resources.h"


#define  NIM_INTF_DESCR_SIZE  60        /* maximum string size for ifdescription */
#define  NIM_IFNAME_SIZE      60        /* maximum string size for ifnames */
#define  NIM_IF_ALIAS_SIZE    64

/* size of a string describing interface speed */
#define  NIM_INTF_SPEED_DESCR_SIZE    42

/* size of a string describing usp in 'u/s/p' format */
#define  NIM_USP_DESCR_SIZE           16

/* NIM startup callback priorities.
 * NSF capable applications use priority 1000 or higher. */
#define NIM_STARTUP_PRIO_AUTHMGR         9900
#define NIM_STARTUP_PRIO_MAB             4100
#define NIM_STARTUP_PRIO_DEFAULT          500
/*
 * Type used for Correlators and Handles for the event notification mechanism
 */
typedef uint32 NIM_CORRELATOR_t, NIM_HANDLE_t;

/****************************************
*
*  NIM Unit, Slot, Port Structure
*
*****************************************/

typedef struct
{
   uchar8     unit;
   uchar8     slot;
   ushort16  port;

} nimUSP_t;

/*
 * Type used to indicate various reasons for a failure on Port Event Notifications
 */
typedef enum
{
  NIM_ERR_RC_UNUSED = 0,
  NIM_ERR_RC_TIMEOUT,
  NIM_ERR_RC_RESOURCE_BUSY,
  NIM_ERR_RC_INVALID_INTF,
  NIM_ERR_RC_INVALID_EVNT,
  NIM_ERR_RC_FATAL,
  NIM_ERR_RC_LACK_OF_RESOURCES,
  NIM_ERR_RC_NOT_AVAILABLE,
  NIM_ERR_RC_NOT_FOUND,
  NIM_ERR_RC_BUSY,
  NIM_ERR_RC_IGNORED,
  NIM_ERR_RC_INTERNAL,
  NIM_ERR_RC_INVALID_ID,
  NIM_ERR_RC_INVALID_DATA,
  NIM_ERR_RC_INVALID_RQST,
  NIM_ERR_RC_INVALID_HANDLE,
  NIM_ERR_RC_INVALID_CMD,
  NIM_ERR_RC_INVALID_CMD_TYPE,
  NIM_ERR_RC_INVALID_PAIR,
  NIM_ERR_RC_OUT_OF_SEQUENCE,
  NIM_ERR_RC_OTHER,
  NIM_ERR_RC_LAST

}  NIM_ERR_RC_t;

typedef enum
{
  NIM_INTERFACE_CREATE_STARTUP,
  NIM_INTERFACE_ACTIVATE_STARTUP
} NIM_STARTUP_PHASE_t;

/*
 * A list of components which can put an interface to D-Disable state
 */    
typedef enum
{
  NIM_DDISABLE_REASON_NONE,
  NIM_DDISABLE_REASON_UDLD,
  NIM_DDISABLE_REASON_MIN = NIM_DDISABLE_REASON_UDLD,
  NIM_DDISABLE_REASON_STP,
  NIM_DDISABLE_REASON_BPDUGUARD,
  NIM_DDISABLE_REASON_BPDUSTORM,
  NIM_DDISABLE_REASON_DHCPSNOOPING,
  NIM_DDISABLE_REASON_ARPINSPECTION,
  NIM_DDISABLE_REASON_STORMCONTROL_BCAST,
  NIM_DDISABLE_REASON_STORMCONTROL_MCAST,
  NIM_DDISABLE_REASON_STORMCONTROL_UCAST,
  NIM_DDISABLE_REASON_XCEIVER,
  NIM_DDISABLE_REASON_XCEIVERPLUS,
  NIM_DDISABLE_REASON_LOOPPROTECT,
  NIM_DDISABLE_REASON_PML,
  NIM_DDISABLE_REASON_DOS,
  NIM_DDISABLE_REASON_LINK_FLAP,
  NIM_DDISABLE_REASON_AUTHMGR_AUTH_VIOLATION,
  NIM_DDISABLE_REASON_COA,
  NIM_DDISABLE_REASON_MAX = NIM_DDISABLE_REASON_COA  
} nimDDisableReason_t;

typedef RC_t (*nimDDisableRecoveryCallback_t)(uint32 intIfNum);

/*
 * Type used to return status status Asynchronously
 */
typedef struct
{

  RC_t               rc;
   NIM_ERR_RC_t       reason;

}  NIM_ASYNC_RESPONSE_t;

/*
 * Type used to return status to the caller of a Port Event Notification
 */
typedef struct NIM_NOTIFY_CB_INFO_s
{

   NIM_ASYNC_RESPONSE_t   response;
  NIM_HANDLE_t              handle;
  uint32                 intIfNum;
   PORT_EVENTS_t          event;

} NIM_NOTIFY_CB_INFO_t;

/*
 * Type used for the Event Notification Status to be returned to the component
 */
typedef void (*NIM_NOTIFY_CB_FUNC_t)(NIM_NOTIFY_CB_INFO_t retVal);

/* Database Type (Union) used to propagate event specific data
 * given by the component which generates the event.
 * Guidelines in the usage/updation of the Union:
 * 1. Each structure within the union is valid for one or more port events but
 *    each port event should only have one valid structure pointed to.
 * 2. The comments of each structure must be updated with the list of events
 *    for which that structure is valid.
 * 3. The event specific data must be crisp for easier communication.
 *    Hence recommend the size of the Union not to exceed beyond 32 bytes.
 */
typedef union NIM_EVENT_SPECIFIC_DATA_u
{
  struct
  {
    uint32 lagIntIfNum; /* Lag interface internal interface number */
  } lagInfo;  /* This structure is valid for  LAG_ACQUIRE and  LAG_RELEASE events. */

} NIM_EVENT_SPECIFIC_DATA_t;


/*
 * Type used to pass data to NIM for event notifications
 */
typedef struct NIM_EVENT_NOTIFY_INFO_s
{

   COMPONENT_IDS_t         component;
   PORT_EVENTS_t           event;
  uint32                  intIfNum;
  NIM_NOTIFY_CB_FUNC_t       pCbFunc;
  NIM_EVENT_SPECIFIC_DATA_t  eventData;  /* Additional event specific data */

} NIM_EVENT_NOTIFY_INFO_t;



/*
 * Type used to pass data to NIM for event notifications
 */
typedef struct NIM_INTF_CREATE_INFO_s
{

   COMPONENT_IDS_t    component;
  NIM_NOTIFY_CB_FUNC_t  pCbFunc;

} NIM_INTF_CREATE_INFO_t;

/*
 * Type used to return status to NIM by the component at the conclusion of an Event
 */
typedef struct
{

   NIM_ASYNC_RESPONSE_t response;
  NIM_CORRELATOR_t        correlator;
   COMPONENT_IDS_t      component;
   PORT_EVENTS_t        event;
  uint32               intIfNum;

} NIM_EVENT_COMPLETE_INFO_t;

/************************************************************************************************
 * Macro Port Information
 *
 * This structure contains information about interfaces which are made up of a composite of
 * other interfaces.
 *
 *
 * The pointer macroInfo  depends upon the type of macroport.  It may be cast to
 * 1. *nimIntf_t, if the macro port is the physical port or the internal Bridge/Router interface.
 * 2. a pointer to a link aggregation structure, if the macroport is a LAG.
 *
 **************************************************************************************************/

typedef struct
{
  uint32   macroPort;      /* Port Number of owning macroport */
  uint32   macroType;      /* Type of owning macroport */
  uint32   macroMtu;       /* (Layer 3) MTU of owning macroport */
  uint32   macroMaxFrame;  /* (Layer 2) Max Frame Size of owning macroport */
  void        *macroInfo;     /* Pointer to macroport information */

} nimMacroPort_t;

/* Interface Structures for creation */
/* Jumbo Frame Size specification */

typedef struct   nimFrameSizeCapability_s
{
  uint32   frameSize;
  uint32   smallestFrameSize;
  uint32   largestFrameSize;

} nimFrameSizeCapability_t;

/*
 * Type used for saving and retrieving Configuration information across boots
 * In order to create an invalid nimConfigID_t, one should use the memset(&dst,0,sizeof(nimConfigID_t)
 * Comparison of the nimConfigID_t should be done by the NIM_CONFIG_ID_IS_EQUAL
 */
/* Type for the configuration interface ID */
typedef struct
{
   INTF_TYPES_t   type;

  union
  {
    nimUSP_t    usp;
    uint32   vlanId;
    uint32   dot3adIntf;
    uint32   loopbackId;
    uint32   tunnelId;
    uint32   servicePortId;
  }configSpecifier;

} nimConfigID_t;

typedef enum /*same as EVC_INTERFACE_TYPE_t*/
{
   NW_INTF_TYPE_UNI_C = 1,
   NW_INTF_TYPE_UNI_S = 2,
   NW_INTF_TYPE_NNI = 3,
   NW_INTF_TYPE_SWITCHPORT = 4, /*ordinary switchport*/
   NW_INTF_TYPE_LAST = 5
} networkIntfType_t;

typedef struct  nimIntfConfig_s
{
   enetMacAddr_t   LAAMacAddr;           /* Locally Administered MAC Address  */

  uint32   addrType;                    /* Using  BIA or  LAA */

   char8    ifAlias[ NIM_IF_ALIAS_SIZE+1]; /* User-Defined Name */

  uint32   nameType;                    /* Using  SYSNAME or  ALIASNAME */

  uint32   ifSpeed;                     /* ifType (MIB II) duplex is included*/

  uint32   negoCapabilities;            /* negotiation capabilities: disabled, 10h, 10f, 100h, 100f, 1000f, all */
  uint32   mgmtAdminState;              /* ifAdminStatus:  ENABLE or  DISABLE
                                              as configured by management entity. */

  uint32   adminState;                  /* ifAdminStatus:  ENABLE or  DISABLE -- operational */

  uint32   trapState;                   /* link Trap status:  ENABLE or  DISABLE */

  uint32   ipMtu;                       /* Maximum size of data portion of a frame on the port.
                                              Derived from cfgMaxFrameSize. Not the IP MTU.
                                              Not configured directly but here for historical
                                              reasons. */

  uint32   encapsType;                  /* Encapsulation Type of frame:  ENCAPSULATION_t*/

  uint32   cfgMaxFrameSize;             /* Maximum configurable frame size on the port*/

   uchar8   forceMedium;                 /* Use  BOOL values.  Set to  TRUE if the
                                            * medium on a combo port is forced */
   uchar8   comboPref;                   /* Use  NIM_COMBO_PREFERENCE_t values.  The
                                            * preferred media on a combo port */
  uint32   softShutdownState;           /* ifsoftShutdownStatus:  ENABLE or  DISABLE
                                              as configured by management entity. */

  uint32   debounceTime;                /* length of time for which link flaps are ignored */

  uint32   cfgNegoCapabilities;         /* Cfg Nego Cap value, not updated by syncing with H/W */

   short16  portModeFlags;               /* 2 byte field combining the Port Mode and Status value for ExP Ports
                                              Look at nim_util.h for bit defns.
                                              Valid for expandable ports (Root + Peers) only
                                           */
   uint8    fecMode;                     /* The state of Forward Error Correction ( ENABLE/ DISABLE)*/
   uint8    ltMode;                      /* The state of Link Training ( ENABLE/ DISABLE)*/

  uint32   nwIntfType;                  /* Network Interface Type: networkIntfType_t */
} nimIntfConfig_t;

typedef struct  nimIntfDescr_s
{

   BOOL          configurable;   /* end user configurable - save configuration across resets */
   BOOL          internal;       /* internal interface only (i.e. CPU, tunnel, or other pseudo-interface */
  uint32        settableParms;  /* settable parameters */
   enetMacAddr_t macAddr;        /* burned-in mac addr */
   enetMacAddr_t bcastMacAddr;      /* bcast addr */
   enetMacAddr_t l3MacAddr;      /* L3 router MAC addr */

  nimFrameSizeCapability_t       frameSize;       /* includes Jumbo Frame specification */

   IANA_INTF_TYPE_t   ianaType;
   PORT_SPEEDS_t      defaultSpeed;

   uint64 phyCapability;
   uint64 remoteCapability;
   fec_mode_t   defaultFEC;
  uint32       fecCapability;

  char           ifDescr[ NIM_INTF_DESCR_SIZE]; /* ifDescr -  One of IANA descriptor values */
  char           ifName[ NIM_IFNAME_SIZE];      /* ifDescr - for short form of interface */

  /* ifLongName size (60) =  NIM_INTF_SPEED_DESCR_SIZE (42) +
                             NIM_USP_DESCR_SIZE (16)        +
                            space (1)                         +
                            Null termination (1) */

  char           ifLongName[ NIM_IFNAME_SIZE];  /* for long form of the interface*/

  nimMacroPort_t macroPort;    /* MacroPort Information, if any.  */

    /* This is populated by the HPC PHY message */
     uchar8 activeMedium;

   BOOL        ifImmediateExpand;  /* If Expandable Port - Root or
                                        Peer, do changes take effect
                                        immediately */
} nimIntfDescr_t;

/* Details interface  */
typedef struct nimIntfCreateRequest_s
{
  nimConfigID_t       *pIntfIdInfo;  /* unique interface specification info */
  nimIntfDescr_t      *pIntfDescr;   /* unique interface descriptor  info */
  nimIntfConfig_t     *pDefaultCfg;  /* default non-volatile interface configuration info */
  NIM_INTF_CREATE_INFO_t *pCreateInfo; /* the component id of the caller and the callback func */

} nimIntfCreateRequest_t;

typedef struct nimIntfCreateOutput_s
{
  uint32       *intIfNum;  /* the internal interface created */
  NIM_HANDLE_t    *handle;    /* used to match the response with this request */
} nimIntfCreateOutput_t;


  /* This struct is used for filling in dynamic Caps provided by
     BSP. It is a subset of IntfConfig_t.*/
typedef struct  nimIntfDynConfig_s
{

    uint32   ifSpeed;                     /* Default speed setting. duplex is included*/
    uint32   ifAutoneg;                   /* Default Autoneg setting */
    uint32   negoCapabilities;            /* negotiation capabilities: disabled, 10h, 10f, 100h, 100f, 1000f, all */
     uchar8   medium;                  /* medium info from port_monitor */
     uint64  dynCapabilities;
} nimIntfDynConfig_t;

/*
 * Macro used to determine if two IDs are equal
 * returns  TRUE if equal or  FALSE if not equal
 */
#define NIM_CONFIG_ID_IS_EQUAL(_a,_b) ((memcmp(_a,_b,sizeof(nimConfigID_t)) == 0)? TRUE: FALSE)

/*
 * Macro used to copy the contents of one config ID to another
 * both parameters are to be pointers to nimConfigID_t
 * This macro should be used the same as an assignment
 * therefore, the caller should insure that both the src and dst pointers are valid
 */
#define NIM_CONFIG_ID_COPY(_dst,_src) (memcpy(_dst,_src,sizeof(nimConfigID_t)))



/*****************************************************************************/
/*********************NIM INTERFACE MASK MACROS & DEFINES*********************/
/*****************************************************************************/

/* Number of bytes in mask */
#define NIM_INTF_INDICES   (( MAX_INTERFACE_COUNT - 1) / (sizeof( uchar8) * 8) + 1)

/* Interface storage */
typedef struct
{
   uchar8 value[NIM_INTF_INDICES];
} NIM_INTF_MASK_t;

/* Least significant bit/rightmost bit is lowest interface # */
/* this is opposite of what SNMP wants */

/* SETMASKBIT turns on bit index # k in mask j. */
#define NIM_INTF_SETMASKBIT(j, k)                                    \
            ((j).value[((k-1)/(8*sizeof( uchar8)))] \
                         |= 1 << ((k-1) % (8*sizeof( uchar8))))



/* CLRMASKBIT turns off bit index # k in mask j. */
#define NIM_INTF_CLRMASKBIT(j, k)                                    \
           ((j).value[((k-1)/(8*sizeof( uchar8)))]   \
                        &= ~(1 << ((k-1) % (8*sizeof( uchar8)))))



/* MASKEQ sets mask j equal to mask k. */
#define NIM_INTF_MASKEQ(j, k) { \
        uint32 x; \
 \
        for (x = 0; x < NIM_INTF_INDICES; x++) { \
                (j).value[x] = (k).value[x]; \
        } \
}

/* ISMASKBITSET returns 0 if the interface k is not set in mask j */
#define NIM_INTF_ISMASKBITSET(j, k)                               \
        ((j).value[((k-1)/(8*sizeof( uchar8)))]  \
                         & ( 1 << ((k-1) % (8*sizeof( char8)))) )



/****************************************
*
*  NIM API Functions
*
*****************************************/

/*********************************************************************
 * @purpose  Returns the descripion for port event
 *
 * @param    event     interface event
 *
 * @returns  description for the event
 *
 * @notes    none
 *
 * @end
 *********************************************************************/
extern  char8 *nimGetIntfEvent( PORT_EVENTS_t event);

/*********************************************************************
* @purpose  Returns the internal interface type
*           associated with the internal interface number
*
* @param    intIfNum    @b{(input)}  internal interface number
* @param    sysIntfType @b{(output)} pointer to internal interface type,
*                       (@b{Returns: Internal Interface Type}).
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimGetIntfType(uint32 intIfNum,  INTF_TYPES_t *sysIntfType);

/*********************************************************************
* @purpose  Returns the Unit-Slot-Port
*           associated with the internal interface number
*
* @param    intIfNum    @b{(input)}  internal interface number
* @param    usp         @b{(output)} pointer to nimUSP_t structure,
*                       (@b{Returns: nimUSP_t structure}).
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimGetUnitSlotPort(uint32 intIfNum, nimUSP_t *usp);

/*********************************************************************
* @purpose  Get phy capability of the specified interface
*
* @param    intIfNum      @b{(input)}  Internal Interface Number
* @param    phyCapability @b{(output)} pointer to phyCapability,
*                       (@b{Returns:  uint64})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimGetIntfPhyCapability(uint32 intIfNum,  uint64 *phyCapability);

/*********************************************************************
* @purpose  Return Internal Interface Number of next valid interface for
*           the specified system interface type.
*
* @param    sysIntfType  @b{(input)}  Internal Interface Type
* @param    intIfNum     @b{(input)}  Internal Interface Number
* @param    nextintIfNum @b{(output)} Internal Interface Number,
*                       (@b{Returns: next valid internal interface number})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
EXT_API RC_t nimNextValidIntfNumberByType( INTF_TYPES_t sysIntfType, uint32 intIfNum, uint32 *nextIntIfNum);

/*********************************************************************
* @purpose  Return Internal Interface Number of the first valid interface for
*           the specified interface type.
*
* @param    sysIntfType  @b{(input)}  Internal Interface Type
* @param    intIfNum     @b{(output)} Internal Interface Number
*                       (@b{Returns: first valid internal interface number})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
EXT_API RC_t nimFirstValidIntfNumberByType( INTF_TYPES_t sysIntfType, uint32 *firstIntIfNum);

/*********************************************************************
* @purpose  Returns the ifIndex associated with the
*           internal interface number
*
* @param    intIfNum    @b{(input)}  internal interface number
* @param    ifIndex     @b{(output)} pointer to ifIndex,
*                       (@b{Returns: ifIndex}).
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
extern RC_t nimGetIntfIfIndex(uint32 intIfNum, uint32 *ifIndex);

/*********************************************************************
* @purpose  Determine whether NIM is in a state ready to process interface requests.
*
* @param     none
*
* @returns   TRUE  - Nim is ready.
* @returns   FALSE - Nim is not ready.
*
* @notes
*
* @end
*********************************************************************/
extern  BOOL nimPhaseStatusCheck(void);

/*********************************************************************
* @purpose  Gets the link state of the specified interface.
*
* @param    intIfNum    @b{(input)}  Internal Interface Number
* @param    linkState   @b{(output)} pointer to Link State,
*                       (@b{Returns:  UP
*                                    or  DOWN})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    A physical port has link up when the PHY has link up.
* @notes    A LAG has link up when at least one of the member ports has link up.
* @notes    A VLAN interface has link up when at least one of the member ports of the VLAN has link up.
*
* @end
*********************************************************************/
EXT_API RC_t nimGetIntfLinkState(uint32 intIfNum, uint32 *linkState);

/*********************************************************************
* @purpose  Gets the admin state of the specified interface.
*
* @param    intIfNum    @b{(input)}  Internal Interface Number
* @param    adminState  @b{(output)} pointer to Admin State,
*                       (@b{Returns:  DISABLE,
*                                     ENABLE
*                                    or  DIAG_DISABLE})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimGetIntfAdminState(uint32 intIfNum, uint32 *adminState);

/*********************************************************************
* @purpose  Determine if this internal interface number is valid
*
* @param    intIfNum      @b{(input)} internal interface number
*
* @returns   SUCCESS     if interface exists
* @returns   ERROR       if interface does not exist
* @returns   FAILURE     if other failure
*
* @notes    none
*
* @end
*********************************************************************/
extern RC_t nimCheckIfNumber(uint32 intIfNum);

/*********************************************************************
*
* @purpose  Get the configuration ID for the given Internal Interface ID
*
* @param    intIfNum  @b{(input)}  Internal Interface number
* @param    configId        @b{(output)} Pointer to the new config ID
*
* @returns   SUCCESS     if interface exists
* @returns   ERROR       if interface does not exist
* @returns   FAILURE     if other failure
*
* @notes    none
*
*
* @end
*********************************************************************/
EXT_API RC_t nimConfigIdGet(uint32 intIfNum,nimConfigID_t *configId);

/*********************************************************************
* @purpose  Return Internal Interface Number of next valid port
*
* @param    intIfNum     @b{(input)}  Internal Interface Number
* @param    nextintIfNum @b{(output)} Internal Interface Number,
*                       (@b{Returns: next valid internal interface number})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
EXT_API RC_t nimNextValidIntfNumber(uint32 intIfNum, uint32 *nextIntIfNum);

/*********************************************************************
* @purpose  Return Internal Interface Number of the first valid port
*
* @param    firstIntIfNum   @b{(output)} Internal Interface Number
*                       (@b{Returns: first valid internal interface number})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
EXT_API RC_t nimFirstValidIntfNumber(uint32 *firstIntIfNum);

/*******************************************************************************
* @purpose  To allow components to register only for port events that it processes
*
* @param    registrar_ID     @b{(input)} routine registrar id  (See  COMPONENT_ID_t)
* @param    registeredEvents @b{(input)} Bit mask of port events that component requests
*                              notification
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if failure
*
* @notes    none
*
* @end
*******************************************************************************/
EXT_API RC_t nimRegisterIntfEvents( COMPONENT_IDS_t  registrar_ID,
                              PORTEVENT_MASK_t    registeredEvents);

/*********************************************************************
* @purpose  Gets the active of the specified interface.
*
* @param    intIfNum    @b{(input)}  Internal Interface Number
*
* @returns  NIM interface state
*
* @notes    This is an API for internal function nimUtilIntfStateGet
*
* @end
*********************************************************************/
EXT_API  INTF_STATES_t nimGetIntfState(uint32 intIfNum);

/*********************************************************************
* @purpose  Register a routine to be called when a link state changes.
*
* @param    registrar_ID   @b{(input)} routine registrar id  (See  COMPONENT_ID_t)
* @param    *notify        @b{(input)} pointer to a routine to be invoked for link state
*                          changes.  Each routine has the following parameters:
*                          (internal interface number, event( UP,  DOWN,
*                           etc.), correlator and the event specific data).
* @param    *startup_notify @b{(input)} pointer to a routine to be invoked at startup.
*                          Each routine has the following parameters:
*                          (startup_phase(NIM_INTERFACE_CREATE_STARTUP,
*                                         NIM_INTERFACE_ACTIVATE_STARTUP)).
* @param    priority       @b{(input)} priority of the startup notification.
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    eventData is additional event specific data that is
*           passed to the registered components.
*           Registered components must interpret it as per the validity of the
*           structure mentioned in comments for each of the structure in
*           NIM_EVENT_SPECIFIC_DATA_t.
*
* @end
*********************************************************************/
typedef  void (*StartupNotifyFcn)(NIM_STARTUP_PHASE_t startup_phase);

/*********************************************************************
* @purpose  Update phy capability of the specified interface from PHY
*
* @param    intIfNum    Internal Interface Number
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimUpdateIntfPhyCapability(uint32 intIfNum);

/*********************************************************************
* @purpose  Determine if this internal interface number is valid
*
* @param    intIfNum      @b{(input)} internal interface number
*
* @returns   SUCCESS     if interface exists
* @returns   ERROR       if interface does not exist
* @returns   FAILURE     if other failure
*
* @notes    none
*
* @end
*********************************************************************/
extern RC_t nimCheckIfNumber(uint32 intIfNum);

/*********************************************************************
*
* @purpose  Get the configuration ID for the given Internal Interface ID
*
* @param    intIfNum  @b{(input)}  Internal Interface number
* @param    configId        @b{(output)} Pointer to the new config ID
*
* @returns   SUCCESS     if interface exists
* @returns   ERROR       if interface does not exist
* @returns   FAILURE     if other failure
*
* @notes    none
*
*
* @end
*********************************************************************/
EXT_API RC_t nimConfigIdGet(uint32 intIfNum,nimConfigID_t *configId);

/*********************************************************************
* @purpose  Return Internal Interface Number of next valid interface for
*           the specified system interface type.
*
* @param    sysIntfType  @b{(input)}  Internal Interface Type
* @param    intIfNum     @b{(input)}  Internal Interface Number
* @param    nextintIfNum @b{(output)} Internal Interface Number,
*                       (@b{Returns: next valid internal interface number})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
extern RC_t nimNextValidIntfNumberByType( INTF_TYPES_t sysIntfType, uint32 intIfNum, uint32 *nextIntIfNum);

/*********************************************************************
*
* @purpose  Status callback from components to NIM for PORT EVENT Notifications
*
* @param    file_name     @b{(input)}   File name from where this function is called
* @param    line_number   @b{(input)}   Line number from where this function is called
* @param    status        @b{(output)}  Status from the component
*
* @returns  void
*
* @notes    At the conclusion of processing a PORT Event, each component must
*           must call this function with the correlator, intf, status, and
*           the component ID
*
* @end
*********************************************************************/
#define nimEventStatusCallback(status) nimEventStatusCallback_track(__FILE__, __LINE__, status)
EXT_API void nimEventStatusCallback_track( char8 *file_name, uint32 line_number,
                                         NIM_EVENT_COMPLETE_INFO_t status);

/*********************************************************************
*
* @purpose  Notify all interested components of an Interface Change event
*
* @param    cbInfo        @b{(output)}  The event information
* @param    pHandle       @b{(output)}  A handle that identifies this request
*
* @returns  void
*
* @notes    If the caller is interested in being notified at when the event is
*           completed, they must put a callback function in the cbInfo.pCbFunc
*
* @notes    Depending on the event, the interface will either be an intIfNum or
*           a nimUSP_t.  Presently, only the  CREATE may have an interface other
*           than the intIfNum
*
* @end
*********************************************************************/
EXT_API RC_t nimEventIntfNotify(NIM_EVENT_NOTIFY_INFO_t cbInfo, NIM_HANDLE_t *pHandle);

/*********************************************************************
* @purpose  Given a usp, get the interface type associated with the slot
*
* @param    usp         @b{(input)}  pointer to nimUSP_t structure
* @param    sysIntfType @b{(output)} pointer to a parm of  INTF_TYPES_t
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimGetIntfTypeFromUSP(nimUSP_t* usp,  INTF_TYPES_t *sysIntfType);

/*********************************************************************
* @purpose  Get valid range of intIfNumers for a given interface type
*
* @param    intfType  @b{(input)}  one of  INTF_TYPES_t
* @param    *min      @b{(output)} pointer to parm to store min value
* @param    *max      @b{(output)} pointer to parm to store max value
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    If the action fails, the intIfNum will be set to zero
*
* @end
*********************************************************************/
extern RC_t nimIntIfNumRangeGet( INTF_TYPES_t intfType, uint32 *min, uint32 *max);

/*********************************************************************
*
* @purpose  Status callback from components to NIM for startup complete
*           Notifications
*
* @param    componentId  @b{(input)} component ID of the caller.
*
* @returns  void
*
* @notes    At the conclusion of processing a startup Event, each component must
*           must call this function.
*
* @end
*********************************************************************/
EXT_API void nimStartupEventDone( COMPONENT_IDS_t componentId);

/*********************************************************************
*
* @purpose  Invoke startup callbacks for registered components
*
* @param    phase     @b{(input)}  startup phase - create or activate
*
* @returns  void
*
* @notes    Startup's are invoked serially, waiting for each one to
*           complete before invoking the next component's startup.
*
* @end
*********************************************************************/
EXT_API void nimStartupCallbackInvoke(NIM_STARTUP_PHASE_t phase);

/*********************************************************************
*
* @purpose  Gets either the system name or alias name
*           of the specified interface, as requested
*
* @param    intIfNum    @b{(input)} Internal Interface Number
* @param    nameType    @b{(input)} name type requested ( SYSNAME,  ALIASNAME or  NULL)
*                        NULL will return currently configured ifName
* @param    ifName      @b{(output)} pointer to Interface Name,
*                       (@b{Returns: MAX_INTF_NAME byte interface name})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimGetIntfName(uint32 intIfNum, uint32 nameType,  uchar8 *ifName);

/*********************************************************************
* @purpose  Sets the administrative state of the specified interface.
*
* @param    intIfNum    @b{(input)} Internal Interface Number
* @param    adminState  @b{(input)} admin state,
*                       (@b{   ENABLE or
*                              DISABLE})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimSetIntfAdminState(uint32 intIfNum, uint32 adminState);

/*********************************************************************
* @purpose  Sets the ifAlias name of the specified interface.
*
* @param    intIfNum    @b{(input)} Internal Interface Number
* @param    ifAlias     @b{(input)} pointer to string containing alias name
*
* @returns   SUCCESS       if success
* @returns   ERROR         if interface does not exist
* @returns   FAILURE       if other failure
* @returns   NOT_SUPPORTED if port-channel is auto-LAG based
*                            port-channel
*
* @notes none
*
* @end
*********************************************************************/
EXT_API RC_t nimSetIntfifAlias(uint32 intIfNum,  uchar8 *ifAlias);

/*********************************************************************
* @purpose  Send message to nim to Notifies registered routines of interface changes.
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    event       @b{(input)} all port events,
*                       (@b{   CREATE,
*                              DELETE,
*                              PORT_DISABLE,
*                              PORT_ENABLE,
*                              UP,
*                              DOWN,
*                              ENABLE,
*                              DISABLE,
*                              DIAG_DISABLE,
*                              FORWARDING,
*                              NOT_FORWARDING,
*                              CREATE,
*                              DELETE,
*                              ACQUIRE,
*                              RELEASE,
*                              SPEED_CHANGE,
*                              LAG_CFG_CREATE,
*                              LAG_CFG_MEMBER_CHANGE,
*                              LAG_CFG_REMOVE,
*                              LAG_CFG_END,
*                              PROBE_SETUP,
*                              PROBE_TEARDOWN,
*                              SET_INTF_SPEED,
*                              SET_MTU_SIZE,
*                              VRRP_TO_MASTER or
*                              VRRP_FROM_MASTER})
* @param    eventData   @b{(input)} additional event specific data
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimNotifyIntfChange(uint32 intIfNum, uint32 event,
                                   NIM_EVENT_SPECIFIC_DATA_t eventData);
/*********************************************************************
* @purpose  Returns the internal interface number
*           associated with the Unit-Slot-Port
*
* @param    usp         @b{(input)}  pointer to nimUSP_t structure
* @param    intIfNum    @b{(output)} pointer to internal interface number,
*                       (@b{Returns: Internal Interface Number}).
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimGetIntIfNumFromUSP(nimUSP_t* usp, uint32 *intIfNum);

/*********************************************************************
* @purpose  Gets the portmode value of the specified interface.
*
* @param    intIfNum       Internal Interface Number
* @param    intfPortMode   pointer to Port Mode,
*                          (@b{Returns:    PORT_PORTMODE_NONE=0,
                                           PORT_PORTMODE_40G_FULL,
*                                          PORT_PORTMODE_10G_FULL,
*                                          PORT_PORTMODE_INVALID})
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimIntfPortModeGet(uint32 intIfNum,  portmode_t *ifMode);

/*********************************************************************
* @purpose  Gets the portmode status value of the specified interface.
*
* @param    intIfNum       Internal Interface Number
* @param    ifStatus       portModeStatus value of Interface
*                          (@b{Returns:   0 or 1
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimIntfPortModeEnableStatusGet(uint32 intIfNum, uint32 *ifStatus);

/*********************************************************************
* @purpose  Sets the Network Interface Type of the interface
*
* @param    intIfNum    @b{(input)} Internal Interface Number
* @param    nwIntfType  @b{(input)} Network Interface Type
*                       (@b{ networkIntfType_t })
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
RC_t nimNetworkIntfTypeSet(uint32 intIfNum, uint32 nwIntfType);

/*********************************************************************
* @purpose  check if the port is a macro port
*
* @param    intIfNum    @b{(input)} internal interface number
*
* @returns   TRUE
* @returns   FALSE
*
* @notes    none
*
* @end
*********************************************************************/
extern  BOOL nimIsMacroPort(uint32 intIfNum);

/*********************************************************************
* @purpose  Gets either the burned-in or locally administered address
*           of the specified interface, as requested
*
* @param    intIfNum    @b{(input)} Internal Interface Number
* @param    addrType    @b{(input)} address type requested ( SYSMAC_BIA,  SYSMAC_LAA,
*                       or  NULL)  NULL will return currently configured
*                       MAC Address
* @param    macAddr     @b{(output)} pointer to MAC Address,
*                       (@b{Returns: 6 byte mac address})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
EXT_API RC_t nimGetIntfAddress(uint32 intIfNum, uint32 addrType,  uchar8 *macAddr);

/*********************************************************************
* @purpose  Gets the address type being used, either the burned-in or
*           locally administered address of the specified interface.
*
* @param    intIfNum    @b{(input)} Internal Interface Number
* @param    addrType    @b{(output)} address type,
*                       (@b{  Returns:  BIA or
*                              LAA})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
extern RC_t nimGetIntfAddrType(uint32 intIfNum, uint32 *addrType);

EXT_API RC_t nimRegisterIntfChange( COMPONENT_IDS_t registrar_ID,
                                     RC_t (*notify)(uint32 intIfNum, uint32 event,
                                                       NIM_CORRELATOR_t correlator,
                                                       NIM_EVENT_SPECIFIC_DATA_t eventData),
                                     StartupNotifyFcn startupFcn,
                                     uint32 priority);
#endif /* INCLUDE_NIMAPI */
