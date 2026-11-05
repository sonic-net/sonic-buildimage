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


#define  MAC_ENET_BCAST

#include "datatypes.h"
#include "commdefs.h"
#include "cnfgr_api.h"
#include "log.h"
#include "osapi.h"
#include "osapi_sem.h"
#include "sysapi_hpc.h"
#include "portevent_mask.h"
#include "nim_ifindex.h"
#include "nim_util.h"
#include "nim_data.h"
#include "nim_events.h"
#include "nimapi.h"
#include "nim_exports.h"
#include "nim_config.h"
#include "nim_outcalls.h"
#include "nim_startup.h"
#include "nim_trace.h"
#include "system_exports.h"
#include "defaultconfig.h"
#include "utils_api.h"
#include "platform_config.h"

static nimCorrelatorData_t  correlatorTable = { 0};
static NIM_CORRELATOR_t     *correlatorInUse = 0;

/* The NIM timeout is 600 seconds. The timeout needs to be long
** because when routing is enabled an interface event can trigger a long operation,
** such as clearing the ARP cache.
*/
#define NIM_EVENT_TIMEOUT 600
#define MASK_STRING_LENGTH  ((( LAST_COMPONENT_ID/32)+1)*15)
#define FD_NIM_DEFAULT_NETWORK_INTERFACE_TYPE                          NW_INTF_TYPE_SWITCHPORT

/* semaphore for the event transaction creation */
static void *nimEventSema =  NULLPTR;
static void *nimCorrelatorSema =  NULLPTR;

static uint32 maxCorrelators = 0;
static uint32 maxCorrelatorsInUse = 0;
static uint32 lastCorrelatorTaken = 0;

/* data and structures for status queue */
static void *pNimEventStatusQueue;

/*added to handle async behavior of sonic*/
static int pending_startup_cb ;

typedef struct {
  RC_t                 response;
  NIM_CORRELATOR_t        correlator;
   PORT_EVENTS_t        event;
  uint32               intIfNum;
} NIM_EVENT_STATUS_MSG_t;

/* Local prototypes */
void nimEventAttachPostProcess(NIM_EVENT_NOTIFY_INFO_t eventInfo);
extern  char8 *nimDebugCompStringGet( COMPONENT_IDS_t cid);

static uint32 default_nim_timeout = NIM_EVENT_TIMEOUT*1000;

/* Funtions to handle the case where NIM callback registration happens after the PortInitDone*/
void nimStartupCallbackPendingSet()
{
  pending_startup_cb = 1 ;
}

int nimStartupCallbackPendingGet()
{
  return pending_startup_cb;
}

void nimStartupCallbackPendingClear()
{
  pending_startup_cb = 0;
}

/*********************************************************************
* @purpose  go through registered users and notify them of interface changes.
*
* @param    correlator    The correlator to match with the request
* @param    eventInfo     The event, intf, component, callback func,
*                         additional event specific data.
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
void nimDoNotify(NIM_CORRELATOR_t correlator, NIM_EVENT_NOTIFY_INFO_t eventInfo)
{
  RC_t   rc;
   int32 i;
   PORT_EVENTS_t event  = eventInfo.event;
  uint32 intIfNum      = eventInfo.intIfNum;
  uint32 now;
  NIM_NOTIFY_CB_INFO_t cbData;
   uchar8 maskString[MASK_STRING_LENGTH]; /* number of chars per mask index */
  NIM_EVENT_STATUS_MSG_t msg;
   uchar8 ifName[ NIM_IFNAME_SIZE + 1];
   int32  intIndex, bitIndex;

  memset(ifName, 0, sizeof(ifName));

  /* Set to  TRUE when one or more clients want to receive this event. */
   BOOL sendEvent =  FALSE;

  now = osapiUpTimeRaw();
  if (nimPhaseStatusCheck() !=  TRUE) return;

  /* create the transaction */
  osapiSemaTake(nimEventSema, WAIT_FOREVER);

  /* start with no responses received */
  memset(correlatorTable.remainingMask,0,sizeof(uint32) * (( LAST_COMPONENT_ID/32) +1));
  memset(correlatorTable.failedMask,0,sizeof(uint32) * (( LAST_COMPONENT_ID/32) +1));
  memset(maskString,0,sizeof(maskString));

  for (i = 0; i <  LAST_COMPONENT_ID; i++)
  {
    if ((nimCtlBlk_g->nimNotifyList[i].registrar_ID) &&
      (PORTEVENT_ISMASKBITSET(nimCtlBlk_g->nimNotifyList[i].notify_mask,event)))
    {
      correlatorTable.remainingMask[i/32] |= (1 << i % 32);
      sendEvent =  TRUE;
    }
  }

  cbData.handle = correlator;

  if (sendEvent)
  {
      correlatorTable.time = now + default_nim_timeout;
      correlatorTable.correlator = correlator;
      correlatorTable.inUse =  TRUE;
      memcpy(&correlatorTable.requestData,&eventInfo,sizeof(NIM_EVENT_NOTIFY_INFO_t));

      /* assume success */
      correlatorTable.response =  SUCCESS;

      osapiSemaGive(nimEventSema);
      /* notify the components */
      for (i = 0; i <  LAST_COMPONENT_ID; i++)
      {
        if ((nimCtlBlk_g->nimNotifyList[i].registrar_ID) &&
          (PORTEVENT_ISMASKBITSET(nimCtlBlk_g->nimNotifyList[i].notify_mask,event)))
        {
          nimTracePortEventComp(i,event,intIfNum, TRUE,correlator);
          nimProfilePortEventComp(i,event,intIfNum, TRUE);
          rc = (*nimCtlBlk_g->nimNotifyList[i].notify_intf_change)(intIfNum, event, correlator,
                                                                   eventInfo.eventData);
        }
      }

      /* wait for message with a timeout */
      rc = osapiMessageReceive(pNimEventStatusQueue, &msg, sizeof(NIM_EVENT_STATUS_MSG_t),/*default_nim_timeout needs to work on it*/  WAIT_FOREVER);

      osapiSemaTake(nimEventSema, WAIT_FOREVER);

      if (rc !=  SUCCESS)
      {
         int32 failedComp = 0;

        cbData.response.rc      =  FAILURE;
        cbData.response.reason  = NIM_ERR_RC_TIMEOUT;

        for (i=( LAST_COMPONENT_ID / 32); i >= 0 ;i--)
        {
          if ((MASK_STRING_LENGTH - strlen(maskString)) >= 15)
          {
            sprintf(maskString,"%s 0x%.8x ",maskString,correlatorTable.remainingMask[i]);
          }
        }

        for (i = 0; i <  LAST_COMPONENT_ID; i++)
        {
          intIndex = i/32;
          bitIndex = i%32;
          if (correlatorTable.remainingMask[intIndex] & (1 << bitIndex))
          {
            failedComp = i;
            nimLogErrorMsg ( TRUE,__FILE__, __LINE__, 
                            "NIM:%s component not responding. Internal timeout may indicate "
                            "system instability. Recommend checking interface in next message.\n", nimDebugCompStringGet(i));

          }
        }

        nimGetIntfName(correlatorTable.requestData.intIfNum,  ALIASNAME, ifName);

        LOG_ERROR_OPT_RESET( LOG_SEVERITY_ERROR,
                              NIM_COMPONENT_ID<<24 | failedComp, 
                             "NIM: Notification is timedout. "
                             "The system may be in inconsistent state. "
                             "Recommend rebooting the system now.");
        cbData.response.rc =  SUCCESS;
      }
      else
      {
        /* tally complete */
        cbData.response.rc  = correlatorTable.response;
      }
  }
  else
  {
      cbData.response.rc =  SUCCESS;
  }

  /* delete the correlator for the next event */
  if (nimEventCorrelatorDelete(correlator) !=  SUCCESS)
  {
    NIM_LOG_ERROR("NIM: Error deleting the event correlator(%d)\n", correlatorTable.correlator);
  }

  correlatorTable.inUse =  FALSE;

  (void)osapiSemaGive(nimEventSema);
  /* change the state of intf if necessary, do any processing, and callback the generator */
  nimEventPostProcessor(eventInfo,cbData);

}

/*********************************************************************
* @purpose  Send message to nim to Notifies registered routines of interface changes.
*
* @param    intIfNum    internal interface number
* @param    event       all port events,
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
RC_t nimNotifyIntfChange(uint32 intIfNum, uint32 event,
                            NIM_EVENT_SPECIFIC_DATA_t eventData)
{
  RC_t     returnVal =   FAILURE;
  NIM_HANDLE_t           handle;
  NIM_EVENT_NOTIFY_INFO_t eventInfo;
  RC_t rc;
  uint32 ifIndex;
   char8 buf[128];

  if (nimPhaseStatusCheck() !=  TRUE)
  {
      return( ERROR);
  }

  eventInfo.component     =  NIM_COMPONENT_ID;
  eventInfo.event         = event;
  eventInfo.intIfNum      = intIfNum;
  eventInfo.pCbFunc       =  NULLPTR;
  memcpy(&eventInfo.eventData, &eventData, sizeof(NIM_EVENT_SPECIFIC_DATA_t));

  /* don't need to keep the handle around */
  if ((returnVal = nimEventIntfNotify(eventInfo,&handle)) !=  SUCCESS)
  {
    NIM_LOG_MSG("NIM: Failed to send PORT EVENT on NIM_QUEUE\n");
  }
  else
  {
    rc = nimGetIntfIfIndex(intIfNum, &ifIndex);
    sprintf(buf, "IfIndex: %d", ifIndex);
    //EM_LOG((SYSEVENT_EVENT|event), buf);
    returnVal =  SUCCESS;
  }

  return(returnVal);
}

void nimEventCmgrDebugCallback(NIM_NOTIFY_CB_INFO_t retVal)
{
  if (retVal.response.rc !=  SUCCESS)
  {
    if (retVal.response.reason == NIM_ERR_RC_TIMEOUT)
    {
      NIM_LOG_MSG("NIM: Timeout Cmgr event occured for handle(%d)\n",retVal.handle);
    }
    else
    {
      NIM_LOG_MSG("NIM: Failed Cmgr event notify for handle(%d)\n",retVal.handle);
    }
  }
}


/*********************************************************************
* @purpose  Register a routine to be called when a link state changes.
*
* @param    registrar_ID   routine registrar id  (See  COMPONENT_ID_t)
* @param    *notify        pointer to a routine to be invoked for link state
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
RC_t nimRegisterIntfChange( COMPONENT_IDS_t registrar_ID,
                              RC_t (*notify)(uint32 intIfNum, uint32 event,
                                                NIM_CORRELATOR_t correlator,
                                                NIM_EVENT_SPECIFIC_DATA_t eventData),
                              StartupNotifyFcn startupFcn,
                              uint32 priority)
{
  RC_t rc =  FAILURE;

  if (registrar_ID >=  LAST_COMPONENT_ID)
  {
    NIM_LOG_MSG("NIM registrar_ID %ud greater then NIM_USER_LAST\n", registrar_ID);
  }
  else if (nimCtlBlk_g ==  NULLPTR)
  {
    NIM_LOG_ERROR("NIM: nimCtlBlk_g uninitialized\n");

  }
  else if (nimCtlBlk_g->nimNotifyList ==  NULLPTR)
  {
    NIM_LOG_ERROR("NIM: nimNotifyList not initialized\n");
  }
  else
  {
    nimCtlBlk_g->nimNotifyList[registrar_ID].registrar_ID = registrar_ID;
    nimCtlBlk_g->nimNotifyList[registrar_ID].notify_intf_change = notify;

    if (startupFcn !=  NULLPTR)
    {
      nimStartUpCreate(registrar_ID, priority, startupFcn);
      /* Handle the case where callback registration happens after PortInitDone */
      if(nimStartupCallbackPendingGet())
      {
         nimStartupCallbackPendingClear();
         nimStartupCallbackInvoke(NIM_INTERFACE_CREATE_STARTUP);
         nimStartupCallbackInvoke(NIM_INTERFACE_ACTIVATE_STARTUP);
      }
    }
    else {
      LOG_ERROR(registrar_ID);
    }
    rc =  SUCCESS;
  }

  return(rc);

}

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
RC_t nimRegisterIntfEvents( COMPONENT_IDS_t registrar_ID,
                              PORTEVENT_MASK_t   registeredEvents)
{
  RC_t rc =  FAILURE;

  if (registrar_ID >=  LAST_COMPONENT_ID)
  {
    NIM_LOG_MSG("NIM registrar_ID %ud greater then NIM_USER_LAST\n", registrar_ID);
  }
  else if (nimCtlBlk_g ==  NULLPTR)
  {
    NIM_LOG_ERROR("NIM: nimCtlBlk_g uninitialized\n");

  }
  else if (nimCtlBlk_g->nimNotifyList ==  NULLPTR)
  {
    NIM_LOG_ERROR("NIM: nimNotifyList not initialized\n");
  }
  else
  {
    osapiSemaTake(nimEventSema, WAIT_FOREVER);
    nimCtlBlk_g->nimNotifyList[registrar_ID].notify_mask = registeredEvents;
    osapiSemaGive(nimEventSema);
    rc =  SUCCESS;
  }

  return(rc);
}

/*********************************************************************
* @purpose  Notify all recepients of nim notifications of the link up event
*
* @param    none
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    This routine is called only once when the system initialization is complete.
*           This event was suppressed when the system comes up and is only
*           propagated when all the components have initialized.
*
* @end
*
*********************************************************************/
RC_t nimNotifyLinkUp()
{
  RC_t rc =  SUCCESS;
  uint32 result;
  uint32 i;
  NIM_EVENT_SPECIFIC_DATA_t eventData;

  memset (&eventData, 0, sizeof(NIM_EVENT_SPECIFIC_DATA_t));

  if (nimPhaseStatusCheck() !=  TRUE) return( ERROR);
  for (i = 1; i <= platIntfTotalMaxCountGet() ; i++)
  {
    result = NIM_INTF_ISMASKBITSET(nimCtlBlk_g->linkStateMask, i);
    if ((result !=  NIM_UNUSED_PARAMETER) && (nimCtlBlk_g->nimPorts[i].sysIntfType !=  CPU_INTF))
    {
      rc = nimNotifyIntfChange(i,  UP, eventData);
    }
  }
  return(rc);
}

/*********************************************************************
* @purpose  Callback routine for DTL to notify NIM of Interface Events
*
*
* @param    usp           internal interface number
* @param    event         all port events,
* @param    dapiIntMgmt   data sent with the callback
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
void nimDtlIntfChangeCallback(nimUSP_t *usp, uint32 event, void * dapiIntmgmt)
{
  nimUSP_t      nimUsp;
  uint32     intIfNum = 0;
  NIM_HANDLE_t           handle;
  NIM_EVENT_NOTIFY_INFO_t eventInfo;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    return;
  }

  /* fill the nim config usp */
  nimUsp.unit = usp->unit;
  nimUsp.slot = usp->slot;
  nimUsp.port = usp->port;

  if (nimGetIntIfNumFromUSP(&nimUsp, &intIfNum) !=  SUCCESS)
  {
    /* interface does not exist */
    NIM_LOG_MSG("NIM: Failed to find interface at unit %d slot %d port %hu for event(%d)\n",
                usp->unit,usp->slot,usp->port,event);
    return;
  }

  NIM_CRIT_SEC_WRITE_ENTER();

  switch (event)
  {
    case  UP:
      NIM_INTF_SETMASKBIT(nimCtlBlk_g->linkStateMask, nimCtlBlk_g->nimPorts[intIfNum].runTimeMaskId);
      break;
    case  DOWN:
      NIM_INTF_CLRMASKBIT(nimCtlBlk_g->linkStateMask, nimCtlBlk_g->nimPorts[intIfNum].runTimeMaskId);
      break;
    default:
      break;
  }

  NIM_CRIT_SEC_WRITE_EXIT();

  eventInfo.component     =  DTL_COMPONENT_ID;
  eventInfo.event         = event;
  eventInfo.intIfNum      = intIfNum;
  eventInfo.pCbFunc       =  NULLPTR;

  /* don't need to keep the handle around */
  if (nimEventIntfNotify(eventInfo,&handle) !=  SUCCESS)
  {
    NIM_LOG_MSG("NIM: Failed to send CMGR PORT EVENT on NIM_QUEUE\n");
  }

  return;

}


/*********************************************************************
* @purpose  Notifies registered routines of interface changes.
*
* @param    correlator    The correlator to match with the request
* @param    eventInfo     The event, intf, component, callback func
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimNotifyUserOfIntfChange(NIM_CORRELATOR_t correlator, NIM_EVENT_NOTIFY_INFO_t eventInfo)
{
  uint32   result;
  uint32   macroPort;
  nimUSP_t    usp;
   INTF_STATES_t  state, prevState;
  NIM_NOTIFY_CB_INFO_t status;
  RC_t     rc =  SUCCESS;
  uint32   intIfNum = eventInfo.intIfNum;
   PORT_EVENTS_t  event = eventInfo.event;
   BOOL performCallback =  FALSE;
  uint32     negoCapabilities;
   uchar8     ifName[ NIM_IFNAME_SIZE + 1];
  uint32     ifModeStatus =  TRUE;
   portmode_t ifMode;
   INTF_TYPES_t ifType;
  uint32 switchPortType = FD_NIM_DEFAULT_NETWORK_INTERFACE_TYPE;
  NIM_EVENT_SPECIFIC_DATA_t eventData;

  memset (&eventData, 0, sizeof(NIM_EVENT_SPECIFIC_DATA_t));

  memset(ifName, 0, sizeof(ifName));
  state = nimUtilIntfStateGet(eventInfo.intIfNum);

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  ( ERROR);
    performCallback =  TRUE;
    nimGetIntfName(intIfNum,  ALIASNAME, ifName);
    NIM_LOG_MSG("Component %s generated interface event %s (%d) for interface %s (%u). "
                "Interface manager not ready to receive interface events.",
                nimDebugCompStringGet(eventInfo.component), nimGetIntfEvent(event),
                event, ifName, intIfNum);
  }
  else if (state ==  INTF_UNINITIALIZED)
  {
    rc =  SUCCESS;
    performCallback =  TRUE;

    if ((event !=  DOWN) &&
        (event !=  AUTHMGR_PORT_UNAUTHORIZED) &&
        (event !=  PORT_DISABLE) &&
        (event !=  NOT_FORWARDING) &&
        /* Setting interface type as switchport as part of  DETACH event processing, 
         * after notifying registered components is resulting in below message being logged
         * as an error. Adding conditional check for switchport interface type. 
         */
        (event !=  ETHERNET_SWITCHPORT) &&
        (event !=  INACTIVE))
    {
      /* Not a INTF teardown event, LOG it */
      nimGetIntfName(intIfNum,  ALIASNAME, ifName);
      NIM_LOG_MSG("Component %s generated interface event %s (%d) for interface %s (%u). "
                  "This interface is uninitialized.",
                  nimDebugCompStringGet(eventInfo.component), nimGetIntfEvent(event),
                  event, ifName, intIfNum);
    }
    else
    {
      /* A teardown event, trace it (This can happen during normal operation */ 
    }
  }
  else if ((rc = nimGetUnitSlotPort(intIfNum,&usp)) !=  SUCCESS)
  {
    rc =  ERROR;
    performCallback =  TRUE;
  }  
  else
  {
    nimGetIntfName(intIfNum,  ALIASNAME, ifName);
    IS_INTIFNUM_PRESENT(intIfNum,rc);    
    if (rc ==  SUCCESS)
    {
      if (event <  LAST_PORT_EVENT)
      {
        switch (event)
        {
          case  ATTACH:
            if ((prevState = nimUtilIntfStateGet(eventInfo.intIfNum)) !=  INTF_CREATED)
            {
              rc = nimIntfPortModeGet(intIfNum, &ifMode);
              if ((( SUCCESS == rc) && 
                   (  PORT_PORTMODE_NONE == ifMode)) || ( FAILURE == rc))
              {
                rc = nimGetIntfType(intIfNum, &ifType);
                if (((rc ==  SUCCESS) && (ifType !=  LOGICAL_VLAN_INTF)) && 
                    ((prevState !=  INTF_ATTACHED) && (prevState !=  INTF_ATTACHING)))
                {
                  NIM_LOG_MSG("NIM:  ATTACH out of order for Interface %s\n", ifName);
                }
              }
              performCallback =  TRUE;
            }
            else 
            {              
              /*dtlIntfNameSet(eventInfo.intIfNum, 
                             nimCtlBlk_g->nimPorts[eventInfo.intIfNum].operInfo.ifName);*/
              nimUtilIntfStateSet(eventInfo.intIfNum,  INTF_ATTACHING);
              if (nimIntfConfigApply(eventInfo.intIfNum) !=  SUCCESS)
              { 
                nimUtilIntfStateSet(eventInfo.intIfNum, prevState);
                NIM_LOG_MSG("NIM: Failed to ATTACH Interface %s\n", ifName);
                rc =  FAILURE;
                performCallback =  TRUE;
              }
              else
              {
                /* If the new status is false, skip ATTACH */
                if (nimIntfPortModeEnableStatusGet(intIfNum, &ifModeStatus) !=  SUCCESS)
                {                  
                  rc =  ERROR;
                  performCallback =  TRUE;
                }
                else
                {
                  if ( FALSE == ifModeStatus)
                  {
                    /* Set the state back to created since Attach did not happen */
                    nimUtilIntfStateSet(eventInfo.intIfNum,  INTF_CREATED);
                    /* Multple Attaches can occur for Exp Ports. No need to log */
                    //NIM_EXP_PORT_DBG(" attach skipped for interface %d\n", eventInfo.intIfNum);
                    rc =  SUCCESS;
                    /* This releases the Sema */
                    performCallback =  TRUE;                
                  }
                  else
                  { 
                    nimDoNotify(correlator,eventInfo);
                  }
                }
              }
            }            
            break;
          case  DETACH:
            if ( PHY_CAP_EXPANDABLE_PORT_CHECK(nimCtlBlk_g->nimPorts[eventInfo.intIfNum].operInfo.phyCapability))
            {
              if ((nimConfigPhaseGet() ==  CNFGR_STATE_U1) &&
                  (nimCtlBlk_g->nimPorts[eventInfo.intIfNum].defaultCfg.portModeFlags !=
                   nimCtlBlk_g->nimPorts[eventInfo.intIfNum].configPort.cfgInfo.portModeFlags))
              {
                /*Reset config back to default for interface */
              }
            }          

            if (nimIntfPortModeEnableStatusGet(intIfNum, &ifModeStatus) !=  SUCCESS)
            {
              rc =  ERROR;
              performCallback =  TRUE;
            }
            else
            {
              if ((( FALSE == ifModeStatus) || 
                   (! PHY_CAP_EXPANDABLE_PORT_CHECK(nimCtlBlk_g->nimPorts[eventInfo.intIfNum].operInfo.phyCapability)))
                  && (nimUtilIntfStateGet(eventInfo.intIfNum) ==  INTF_CREATED))
              {
                /* Skip detach if interface was never attached */
                rc =  SUCCESS;
                performCallback =  TRUE;
              }
              else if (nimUtilIntfStateGet(eventInfo.intIfNum) !=  INTF_ATTACHED)              
              {   
                rc = nimIntfPortModeGet(intIfNum, &ifMode);
                if ((( SUCCESS == rc) && 
                     (  PORT_PORTMODE_NONE == ifMode)) || ( FAILURE == rc))
                {                
                  NIM_LOG_MSG("NIM:  DETACH out of order for Interface %s\n", ifName);
                }
                performCallback =  TRUE;              
              }
              else
              {                              
                /*dtlIntfNameSet(eventInfo.intIfNum, nimCtlBlk_g->nimPorts[eventInfo.intIfNum].operInfo.ifName);*/
                rc = nimUtilIntfStateSet(eventInfo.intIfNum, INTF_DETACHING);
                
                NIM_CRIT_SEC_WRITE_ENTER();
                /* Set dynamicCap off */
                nimCtlBlk_g->nimPorts[intIfNum].dynamicCap =  FALSE;    
                           
                nimCtlBlk_g->nimPorts[intIfNum].linkChangeTime = osapiUpTimeRaw();
                
                NIM_INTF_CLRMASKBIT(nimCtlBlk_g->linkStateMask, nimCtlBlk_g->nimPorts[intIfNum].runTimeMaskId);
                NIM_INTF_CLRMASKBIT(nimCtlBlk_g->forwardStateMask, nimCtlBlk_g->nimPorts[intIfNum].runTimeMaskId);
                
                NIM_CRIT_SEC_WRITE_EXIT();
                
                nimDoNotify(correlator,eventInfo);;

                /* Reset network interface type of physical and LAG interfaces */
                if((nimCtlBlk_g->nimPorts[eventInfo.intIfNum].sysIntfType ==  PHYSICAL_INTF)
                   || (nimCtlBlk_g->nimPorts[eventInfo.intIfNum].sysIntfType ==  LAG_INTF))
                {
                  if( FAILURE == nimNetworkIntfTypeSet(eventInfo.intIfNum, FD_NIM_DEFAULT_NETWORK_INTERFACE_TYPE))
                  {
                    nimGetIntfName(eventInfo.intIfNum,  ALIASNAME, ifName);
                    NIM_LOG_MSG("Unable to apply Network Interface Type config to %s", ifName);
                  }
                }
              }
            }
            break;
          case  DELETE:
            if (nimUtilIntfStateGet(eventInfo.intIfNum) !=  INTF_CREATED)
            {
              NIM_LOG_MSG("NIM:  DELETE out of order for Interface %s\n", ifName);
              rc =  FAILURE;
              performCallback =  TRUE;
            }
            else
            {
              rc = nimUtilIntfStateSet(eventInfo.intIfNum, INTF_DELETING);
              nimDoNotify(correlator,eventInfo);;
            }
            break;

          case   UP:

            NIM_CRIT_SEC_WRITE_ENTER();

            nimCtlBlk_g->nimPorts[intIfNum].linkChangeTime = osapiUpTimeRaw();
            NIM_INTF_SETMASKBIT(nimCtlBlk_g->linkStateMask, nimCtlBlk_g->nimPorts[intIfNum].runTimeMaskId);
            NIM_CRIT_SEC_WRITE_EXIT();

            if (! nimCtlBlk_g->nimPorts[intIfNum].dynamicCap)
            {
              if( PHY_CAP_DUAL_MODE_SUPPORT_CHECK(nimCtlBlk_g->nimPorts[intIfNum].operInfo.phyCapability))
              {
                NIM_CRIT_SEC_WRITE_ENTER();
                nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.negoCapabilities = negoCapabilities;
                NIM_CRIT_SEC_WRITE_EXIT();
              }
            }

            nimDoNotify(correlator,eventInfo);;

            if (nimIsMacroPort(intIfNum))
            {
              if (nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.adminState ==  ENABLE)
              {

                /* Hooks for interactions with other components */
                nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
                LOGF( LOG_SEVERITY_NOTICE,
                   "Link up on interface %s. ", ifName);
              }
            }
            else
            {
              macroPort = nimCtlBlk_g->nimPorts[intIfNum].operInfo.macroPort.macroPort;

              if (nimCtlBlk_g->nimPorts[macroPort].configPort.cfgInfo.adminState ==  ENABLE)
              {
                  LOGF( LOG_SEVERITY_NOTICE,
                 "Link up on interface %s. ", ifName);

              }
            }
            break;

          case  DOWN:
            NIM_CRIT_SEC_WRITE_ENTER();

            nimCtlBlk_g->nimPorts[intIfNum].linkChangeTime = osapiUpTimeRaw();
            /* check if the port is in FWD state */
            result = NIM_INTF_ISMASKBITSET(nimCtlBlk_g->forwardStateMask, nimCtlBlk_g->nimPorts[intIfNum].runTimeMaskId);
            NIM_INTF_CLRMASKBIT(nimCtlBlk_g->linkStateMask, nimCtlBlk_g->nimPorts[intIfNum].runTimeMaskId);
            NIM_CRIT_SEC_WRITE_EXIT();
            if( PHY_CAP_DUAL_MODE_SUPPORT_CHECK(nimCtlBlk_g->nimPorts[intIfNum].operInfo.phyCapability))
            {
                NIM_CRIT_SEC_WRITE_ENTER();
                nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.negoCapabilities = negoCapabilities;
                NIM_CRIT_SEC_WRITE_EXIT();
            }



            nimDoNotify(correlator,eventInfo);
             LOGF( LOG_SEVERITY_NOTICE,
               "Link down on interface %s. ", ifName);

            /* if the Macro port and in FWD state */
            if ((nimIsMacroPort(intIfNum)) &&
                ((result) ||
                 (nimCtlBlk_g->nimPorts[intIfNum].sysIntfType ==  LAG_INTF)
#ifdef  PORT_AGGREGATOR_PACKAGE
                 ||
                 (( TRUE == simSimpleModeActiveGet()) &&
                  ( TRUE == portAggregatorIsValidIntf(intIfNum)) &&
                  ( TRUE == portAggregatorIsIntfAutoLagMember(intIfNum)))
#endif
                ))
            {
              /* Hooks for interactions with other components */
              nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
            }
            break;


          case  FORWARDING:
            NIM_CRIT_SEC_WRITE_ENTER();
            NIM_INTF_SETMASKBIT(nimCtlBlk_g->forwardStateMask, nimCtlBlk_g->nimPorts[intIfNum].runTimeMaskId);
            NIM_CRIT_SEC_WRITE_EXIT();
            nimDoNotify(correlator,eventInfo);

            /* Hooks for interactions with other components */
            nimNotifyUserOfIntfChangeOutcall(intIfNum, event);

            break;

          case  NOT_FORWARDING:
            NIM_CRIT_SEC_WRITE_ENTER();
            NIM_INTF_CLRMASKBIT(nimCtlBlk_g->forwardStateMask, nimCtlBlk_g->nimPorts[intIfNum].runTimeMaskId);
            NIM_CRIT_SEC_WRITE_EXIT();
            nimDoNotify(correlator,eventInfo);

            /* Hooks for interactions with other components */
            nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
            break;

          case  PORT_DISABLE:
            nimDoNotify(correlator,eventInfo);
            if (nimIsMacroPort(intIfNum))
            {
              result = NIM_INTF_ISMASKBITSET(nimCtlBlk_g->linkStateMask, intIfNum);

              if (result)
              {
                /* Hooks for interactions with other components */
                nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
              }
            }
            break;

          case  PORT_ENABLE:
            nimDoNotify(correlator,eventInfo);

            if (nimIsMacroPort(intIfNum))
            {
              /* only send a trap if it's already up and being enabled */
              result = NIM_INTF_ISMASKBITSET(nimCtlBlk_g->linkStateMask, intIfNum);

              if (result)
              {
                /* Hooks for interactions with other components */
                nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
              }
            }
            break;


          case  LAG_ACQUIRE:
            nimDoNotify(correlator,eventInfo);

            /* Hooks for interactions with other components */
            nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
            break;

          case  LAG_RELEASE:
            nimDoNotify(correlator,eventInfo);

            /* only send a trap if it's already up and being enabled */
            result = NIM_INTF_ISMASKBITSET(nimCtlBlk_g->linkStateMask, intIfNum);

            /* release implies port is now a macro port */
            if (result
#ifdef  PORT_AGGREGATOR_PACKAGE
                ||
                ((simSimpleModeActiveGet() ==  TRUE) &&
                 (portAggregatorIsValidIntf(intIfNum) ==  TRUE) &&
                 (portAggregatorIsIntfAutoLagMember(intIfNum) ==  TRUE))
#endif
               )
            {
              //trapMgrLinkUpLogTrap(intIfNum);
            }

            /* Hooks for interactions with other components */
            nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
            break;

          case  PORT_ROUTING_ENABLED:
          case  PORT_ROUTING_DISABLED:
          case  TRILL_TRUNK_ENABLED:
          case  TRILL_TRUNK_DISABLED:
          case  DELETE_PENDING:
          case  SPEED_CHANGE:
          case  LAG_CFG_CREATE:
          case  LAG_CFG_MEMBER_CHANGE:
          case  LAG_CFG_REMOVE:
          case  LAG_CFG_END:
          case  PORT_STATS_RESET:
            nimDoNotify(correlator,eventInfo);
            break;

          case  AUTHMGR_PORT_AUTHORIZED:
              if(NIM_INTF_ISMASKBITSET(nimCtlBlk_g->authorizedStateMask, intIfNum)==  NIM_UNUSED_PARAMETER)
              {
                NIM_CRIT_SEC_WRITE_ENTER();
                NIM_INTF_SETMASKBIT(nimCtlBlk_g->authorizedStateMask, intIfNum);
                NIM_CRIT_SEC_WRITE_EXIT();
              }
              nimDoNotify(correlator,eventInfo);
              nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
              break;

        case  AUTHMGR_PORT_UNAUTHORIZED:
              if(NIM_INTF_ISMASKBITSET(nimCtlBlk_g->authorizedStateMask, intIfNum) !=  NIM_UNUSED_PARAMETER)
              {
                NIM_CRIT_SEC_WRITE_ENTER();
                NIM_INTF_CLRMASKBIT(nimCtlBlk_g->authorizedStateMask, intIfNum);
                NIM_CRIT_SEC_WRITE_EXIT();
              }
              nimDoNotify(correlator,eventInfo);
              nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
            break;

          case  AUTHMGR_ACQUIRE:
            nimDoNotify(correlator,eventInfo);
            break;

          case  AUTHMGR_RELEASE:
            nimDoNotify(correlator,eventInfo);
            break;

#ifdef  PORT_AGGREGATOR_PACKAGE
          case  PORT_AGG_UP:
          case  PORT_AGG_DOWN:
            nimDoNotify(correlator,eventInfo);
            nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
            break;
#endif

          case  PORT_PFC_ACTIVE:
            if(!NIM_INTF_ISMASKBITSET(nimCtlBlk_g->pfcActiveMask, intIfNum))
            {
              NIM_CRIT_SEC_WRITE_ENTER();
              NIM_INTF_SETMASKBIT(nimCtlBlk_g->pfcActiveMask, intIfNum);
              NIM_CRIT_SEC_WRITE_EXIT();
            }
            nimDoNotify(correlator, eventInfo);
            nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
            break;

          case  PORT_PFC_INACTIVE:
            if(NIM_INTF_ISMASKBITSET(nimCtlBlk_g->pfcActiveMask, intIfNum))
            {
              NIM_CRIT_SEC_WRITE_ENTER();
              NIM_INTF_CLRMASKBIT(nimCtlBlk_g->pfcActiveMask, intIfNum);
              NIM_CRIT_SEC_WRITE_EXIT();
            }
            nimDoNotify(correlator, eventInfo);
            nimNotifyUserOfIntfChangeOutcall(intIfNum, event);
            break;

          case  ETHERNET_SWITCHPORT:
            nimDoNotify(correlator,eventInfo);
            break;
  
          case  NETWORK_INTF_TYPE_NNI:
            nimDoNotify(correlator,eventInfo);
            break;
  
          case  NETWORK_INTF_TYPE_UNI_C:
            nimDoNotify(correlator,eventInfo);
            break;
  
          case  NETWORK_INTF_TYPE_UNI_S:
            nimDoNotify(correlator,eventInfo);
            break;

          default:
            nimDoNotify(correlator,eventInfo);
            break;
        }
      }
      else
      {
        rc = ( FAILURE);
      }
    }
  }

  nimProfilePortEvent(eventInfo.component,eventInfo.event,eventInfo.intIfNum, FALSE);
  if (performCallback ==  TRUE)
  {
    /* delete the correlator for the next event */
    if (nimEventCorrelatorDelete(correlator) !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM: Error deleting the event correlator(%d)\n", correlator);
    }

    nimTracePortEvent(eventInfo.component,eventInfo.event,eventInfo.intIfNum, FALSE,correlator);

    if (eventInfo.pCbFunc !=  NULLPTR)
    {
      status.event    = eventInfo.event;
      status.intIfNum = eventInfo.intIfNum;
      status.response.rc = rc;
      status.handle   = correlator;

      eventInfo.pCbFunc(status);
    }
  }
  return(rc);
}




/*********************************************************************
* @purpose  NIM Task
*
* @param    none
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
void nimTask()
{
  nimPdu_t nmpdu;
  RC_t rc;
   CNFGR_CMD_DATA_t pCmdData;
  
  if (osapiTaskInitDone( NIM_TASK_SYNC) !=  SUCCESS)
  {
    NIM_LOG_ERROR("NIM: Task failed to int\n");
  }

  do
  {
    if (nimCtlBlk_g->nimMsgQueue ==  NULLPTR)
      continue;

    /* initialize the message to zero */
    memset(&nmpdu,0x00,sizeof(nimPdu_t));

    rc = osapiMessageReceive(nimCtlBlk_g->nimMsgQueue, &nmpdu,
                             sizeof(nimPdu_t),  WAIT_FOREVER);
    
    if (rc ==  SUCCESS)
    {

      switch (nmpdu.msgType)
      {
        case CNFGR_MSG:

         // pCmdData = nmpdu.data.pCmdData;
         // nimRecvCnfgrCommand( &pCmdData );
          break;

        case NIM_MSG:
          if (nimNotifyUserOfIntfChange(nmpdu.data.message.correlator,nmpdu.data.message.eventInfo) !=  SUCCESS)
          {
            NIM_LOG_MSG("NIM: Failed to notify users of interface change\n");
          }
          break;

        case START_MSG:

          (nmpdu.data.nimStartMsg.startupFunction)(nmpdu.data.nimStartMsg.phase);
          /* Wait until startup has completed */
          nimStartupEventWait();
          break;
        case TIMER_MSG:
          #if  FEAT_DIAG_DISABLED_AUTO_RECOVERY
             nimDDisableResoreInterfaceProcess();
          #endif
          break;
        case ISSU_RESTORE_MSG:
          break;
        default:
        {         
          NIM_LOG_MSG("nimTask(): invalid message type:%d. %s:%d\n",
                      nmpdu.msgType, __FILE__, __LINE__);
          break;
        }
      }
    }
  } while (1);
  return;
}


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
void nimEventStatusCallback_track( char8 *file_name, uint32 line_number,
                                  NIM_EVENT_COMPLETE_INFO_t status)
{
  RC_t   rc =  SUCCESS;
   BOOL   done =  FALSE;
  NIM_EVENT_STATUS_MSG_t msg = { 0 };

  osapiSemaTake(nimEventSema, WAIT_FOREVER);

  nimTracePortEventComp(status.component,status.event,status.intIfNum, FALSE,status.correlator);
  nimProfilePortEventComp(status.component,status.event,status.intIfNum, FALSE);

  utilsFilenameStrip(( char8 **)&file_name);

  /* take the correlator semaphore and tally the response */
  if ((rc = nimEventTally(file_name, line_number, status, &done)) !=  SUCCESS)
  {
    NIM_LOG_MSG("NIM: Error in the tally routine\n");
  }
  else if (done ==  TRUE)
  {
    /* Send a message when the api is ready */
    correlatorTable.inUse =  FALSE;

    msg.correlator = status.correlator;
    msg.event      = status.event;
    msg.intIfNum   = status.intIfNum;
    msg.response   = correlatorTable.response;


    rc = osapiMessageSend(pNimEventStatusQueue,
                          &msg,
                          sizeof(msg),
                           NO_WAIT,
                           MSG_PRIORITY_NORM);

    if (rc !=  SUCCESS)
    {
      NIM_LOG_MSG("failed to put status on queue");
    }

  }
  else
  {
    /* no work to do */
  }

  osapiSemaGive(nimEventSema);

  return;
}

/*********************************************************************
*
* @purpose  Notify all interested components of an Interface Change event
*
* @param    eventInfo     @b{(input)}   The event information
* @param    pHandle       @b{(output)}  A handle that identifies this request
*
* @returns   SUCCESS    Event was accepted
* @returns   FAILURE    Event was not accepted
*
* @notes    If the caller is interested in being notified at when the event is
*           completed, they must put a callback function in the cbInfo.pCbFunc
*
* @end
*********************************************************************/
RC_t nimEventIntfNotify(NIM_EVENT_NOTIFY_INFO_t eventInfo, NIM_HANDLE_t *pHandle)
{
  RC_t rc =  SUCCESS;
  nimPdu_t  pdu;
  NIM_CORRELATOR_t *correlator;
  correlator = pHandle;


  /* validate the data */
  if (pHandle ==  NULL)
  {
    rc =  FAILURE;
  }
  else if (eventInfo.component >=  LAST_COMPONENT_ID)
  {
    rc =  FAILURE;
    NIM_LOG_MSG("NIM: Component(%d) out of range in nimEventIntfNotify\n",eventInfo.component);
  }
  else if (eventInfo.event >=  LAST_PORT_EVENT)
  {
    rc =  FAILURE;
    NIM_LOG_MSG("NIM: Event(%d) out of range in nimEventIntfNotify\n",eventInfo.event);
  }
  else if ((rc = nimEventCorrelatorCreate(correlator)) !=  SUCCESS)
  {
    NIM_LOG_MSG("NIM: Failed to get a correlator in nimNotify\n");
    rc =  FAILURE;
  }
  else
  {
    /* turn off the port */
    if (eventInfo.event ==  DETACH)
    {
    if (nimIsMacroPort(eventInfo.intIfNum))
    {
          /*rc = dtlIntfAdminStateSet(eventInfo.intIfNum,  FALSE);*/
    }
    }

    /* pack the message */
    pdu.msgType = NIM_MSG;
    pdu.data.message.correlator = *correlator;
    memcpy((void*)&pdu.data.message.eventInfo,&eventInfo,sizeof(NIM_EVENT_NOTIFY_INFO_t));

    /* send the message to NIM_QUEUE */
    if (osapiMessageSend( nimCtlBlk_g->nimMsgQueue, (void *)&pdu,
                          (uint32)sizeof(nimPdu_t),  WAIT_FOREVER,  MSG_PRIORITY_NORM ) ==  ERROR)
    {
      NIM_LOG_MSG("NIM: failed to send message to NIM message Queue.\n");
      /*nimTraceEventError(eventInfo.component,eventInfo.event,eventInfo.intIfNum,NIM_ERR_LAST);*/
      rc =  FAILURE;
    }
    else
    {
      nimTracePortEvent(eventInfo.component,eventInfo.event,eventInfo.intIfNum, TRUE,*pHandle);
      nimProfilePortEvent(eventInfo.component,eventInfo.event,eventInfo.intIfNum, TRUE);
      rc =  SUCCESS;
    }
  }

  return(rc);
}

/*********************************************************************
*
* @purpose  Create a correlator for the event
*
* @param    correlator        @b{(output)}  A returned correlator
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*********************************************************************/
RC_t nimEventCorrelatorCreate(NIM_CORRELATOR_t *correlator)
{
  RC_t rc =  SUCCESS;
  uint32 index;


  if (correlator ==  NULL)
  {
    rc =  FAILURE;
  }
  else
  {
    osapiSemaTake(nimCorrelatorSema, WAIT_FOREVER);

    index = lastCorrelatorTaken;
    index++; /* start at the next correlator */

    for (; index < maxCorrelators; index++)
    {
      if (correlatorInUse[index] ==  FALSE )
      {
        correlatorInUse[index] =  TRUE;
        break;
      }
    }

    if (index == maxCorrelators)
    {
      /*
       * We must not have found a correlator
       * start at the beginning
       */
      index = 1;

      for (; index < lastCorrelatorTaken ; index++)
      {
        if (correlatorInUse[index] ==  FALSE )
        {
          correlatorInUse[index] =  TRUE;
          break;
        }
      }
    } /* index == maxCorrelators */

    if ((index == lastCorrelatorTaken) || (index == maxCorrelators))
    {
      /* No correlator available at this time */
      *correlator = 0;
      rc =  FAILURE;
    }
    else
    {
      rc =  SUCCESS;
      *correlator = index;
      lastCorrelatorTaken = index;
    }

    osapiSemaGive(nimCorrelatorSema);
  }

  return(rc);
}

/*********************************************************************
*
* @purpose  Delete a correlator for the event
*
* @param    correlator        @b{(input)}  The correlator to delete
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*********************************************************************/
RC_t nimEventCorrelatorDelete(NIM_CORRELATOR_t correlator)
{
  RC_t rc =  SUCCESS;

  osapiSemaTake(nimCorrelatorSema, WAIT_FOREVER);

  if (correlator < maxCorrelators)
  {
    correlatorInUse[correlator] =  FALSE;
  }
  else
  {
    rc =  FAILURE;
  }

  osapiSemaGive(nimCorrelatorSema);

  return(rc);
}

/*********************************************************************
* @purpose  Initialize the Event Handler Resources
*
* @param    none
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimEventHdlrInit()
{
  RC_t rc =  SUCCESS;

  do
  {
    maxCorrelators = /*nimSidMsgCountGet()*/ 16000 * 2;

    if ((nimEventSema = osapiSemaMCreate(OSAPI_SEM_Q_FIFO)) ==  NULLPTR)
    {
      NIM_LOG_ERROR("NIM: failed to create the event semaphore\n");
      rc =  FAILURE;
      break;
    }

    if ((nimCorrelatorSema = osapiSemaMCreate(OSAPI_SEM_Q_FIFO)) ==  NULLPTR)
    {
      NIM_LOG_ERROR("NIM: failed to create the correlator semaphore\n");
      rc =  FAILURE;
      break;
    }

    correlatorInUse = osapiMalloc ( NIM_COMPONENT_ID, maxCorrelators * sizeof (NIM_CORRELATOR_t));

    (void)memset((void*)correlatorInUse,0,sizeof(NIM_CORRELATOR_t)*maxCorrelators);

    (void)memset((void*)&correlatorTable,0,sizeof(nimCorrelatorData_t));

    correlatorTable.remainingMask = osapiMalloc( NIM_COMPONENT_ID, sizeof(uint32) * (( LAST_COMPONENT_ID/32) +1));

    if (correlatorTable.remainingMask ==  NULL)
    {
      NIM_LOG_ERROR("NIM: unable to alloc memory for correlator table.\n");
      rc =  FAILURE;
    }
    else
    {
      memset((void*)correlatorTable.remainingMask,0,sizeof(uint32) * (( LAST_COMPONENT_ID/32) +1));
    }

    correlatorTable.failedMask = osapiMalloc( NIM_COMPONENT_ID, sizeof(uint32) * (( LAST_COMPONENT_ID/32) +1));

    if (correlatorTable.failedMask ==  NULL)
    {
      NIM_LOG_ERROR("NIM: unable to alloc memory for correlator table.\n");
      rc =  FAILURE;
    }
    else
    {
      memset((void*)correlatorTable.failedMask,0,sizeof(uint32) * (( LAST_COMPONENT_ID/32) +1));
    }

    maxCorrelatorsInUse = 0;

    lastCorrelatorTaken = 0;

    /* create queue for receiving responses from sync messages */
    pNimEventStatusQueue = osapiMsgQueueCreate("NIM EVENT RESPONSE QUEUE", 1, sizeof(NIM_EVENT_STATUS_MSG_t));

    if (pNimEventStatusQueue ==  NULLPTR)
    {
      NIM_LOG_ERROR("NIM: unable to create the status quueue.\n");
      rc =  ERROR;
      break;
    }

  } while ( 0 );

  return rc;
}


/*********************************************************************
* @purpose  Remove the Event Handler Resources
*
* @param    file_name   @b{(input)}   File name from where this function is called
* @param    line_number @b{(input)}   Line number from where this function is called
* @param    status      @b{(input)}   The returned value from the caller
* @param    complete    @b{(output)}  Boolean value indicating whether event is complete
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimEventTally( char8 *file_name, uint32 line_number,
                      NIM_EVENT_COMPLETE_INFO_t status,
                       BOOL *complete)
{
  RC_t rc =  SUCCESS;
  uint32 mask = 0;
  uint32 intIndex,bitIndex;
  uint32 index;
  uint32 maxComps = 0;
   uchar8 ifName[ NIM_IFNAME_SIZE + 1];

  intIndex = status.component/32;
  bitIndex = status.component%32;
  maxComps = ( LAST_COMPONENT_ID / 32) + 1;

  if (status.correlator != correlatorTable.correlator)
  {
    NIM_LOG_ERROR("NIM: Unexpected status callback on correlator(%d), event(%d), intIf(%d).\
                  \r\nnimEventTally: Failed by task %p in file %s line %d.\n",
                  status.correlator,status.event,status.intIfNum,
                  osapiTaskIdSelf(), file_name, line_number);
    rc =  FAILURE;
  }
  else
  {
    correlatorTable.remainingMask[intIndex] &= ~(1 << bitIndex);

    for (index = 0;index < maxComps;index++)
    {
      mask |= correlatorTable.remainingMask[index];
    }

    *complete = (mask == 0)? TRUE: FALSE;

    if (status.response.rc !=  SUCCESS)
    {
      correlatorTable.response = status.response.rc;
      correlatorTable.failedMask[intIndex] |= (1 << bitIndex);
      memset(ifName, 0, sizeof(ifName));
      nimGetIntfName(status.intIfNum,  ALIASNAME, ifName);
      NIM_LOG_ERROR("NIM: Component(%s) failed on event(%s) for interface(%s).\
                     \r\nnimEventTally: Failed by task %p in file %s line %d.\n",
                     nimDebugCompStringGet(status.component),nimGetIntfEvent(status.event),
                     ifName, osapiTaskIdSelf(), file_name, line_number);
    }
  }

  return(rc);
}


/*********************************************************************
* @purpose  Post Processor for Events
*
* @param    eventInfo    @b{(input)}   The event information as assigned by the generator
* @param    status       @b{(output)}  The status of the event by either Tally or Timeout
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
void nimEventPostProcessor(NIM_EVENT_NOTIFY_INFO_t eventInfo, NIM_NOTIFY_CB_INFO_t status)
{
   INTF_STATES_t currState, nextState;

  NIM_CRIT_SEC_WRITE_ENTER();

  currState = nimUtilIntfStateGet(eventInfo.intIfNum);

  if (status.response.rc ==  SUCCESS)
  {
    switch (eventInfo.event)
    {
      case  CREATE:
        if (nimUtilIntfNextStateGet(currState, CREATE_COMPLETE,&nextState) ==  SUCCESS)
        {
          nimUtilIntfStateSet(eventInfo.intIfNum,nextState);
        }

        NIM_INTF_SETMASKBIT(nimCtlBlk_g->createdMask,eventInfo.intIfNum);

        /* set the appropriate bit interface specific mask */
        switch (nimCtlBlk_g->nimPorts[eventInfo.intIfNum].sysIntfType)
        {
          case  PHYSICAL_INTF:
            NIM_INTF_SETMASKBIT(nimCtlBlk_g->physicalIntfMask,eventInfo.intIfNum);
            break;
          case  CPU_INTF:
            NIM_INTF_SETMASKBIT(nimCtlBlk_g->cpuIntfMask,eventInfo.intIfNum);
            break;
          case  LAG_INTF:
            NIM_INTF_SETMASKBIT(nimCtlBlk_g->lagIntfMask,eventInfo.intIfNum);
            break;
          case  LOGICAL_VLAN_INTF:
            NIM_INTF_SETMASKBIT(nimCtlBlk_g->vlanIntfMask,eventInfo.intIfNum);
            break;
          case  SERVICE_PORT_INTF:
            NIM_INTF_SETMASKBIT(nimCtlBlk_g->servicePortIntfMask,eventInfo.intIfNum);
            break;
          default:
            break;
        }

        /* shouldn't increment for pseudo interfaces like tunnels... */
        nimCtlBlk_g->ifNumber++;

        NIM_CRIT_SEC_WRITE_EXIT();
        break;

      case  DETACH:
        if (nimUtilIntfNextStateGet(currState, DETACH_COMPLETE,&nextState) ==  SUCCESS)
        {
          nimUtilIntfStateSet(eventInfo.intIfNum,nextState);
        }

        NIM_INTF_CLRMASKBIT(nimCtlBlk_g->presentMask,eventInfo.intIfNum);

        NIM_CRIT_SEC_WRITE_EXIT();
        break;

      case  ATTACH:
        if (nimUtilIntfNextStateGet(currState, ATTACH_COMPLETE,&nextState) ==  SUCCESS)
        {
          nimUtilIntfStateSet(eventInfo.intIfNum,nextState);
        }

        NIM_INTF_SETMASKBIT(nimCtlBlk_g->presentMask,eventInfo.intIfNum);

        NIM_CRIT_SEC_WRITE_EXIT();

        /* More work needed for the ATTACH process */
        nimEventAttachPostProcess(eventInfo);

        break;
      case  DELETE:
        if (nimUtilIntfNextStateGet(currState, DELETE_COMPLETE,&nextState) ==  SUCCESS)
        {
          nimUtilIntfStateSet(eventInfo.intIfNum,nextState);
        }


        NIM_INTF_CLRMASKBIT(nimCtlBlk_g->createdMask,eventInfo.intIfNum);

        /* set the appropriate bit interface specific mask */
        switch (nimCtlBlk_g->nimPorts[eventInfo.intIfNum].sysIntfType)
        {
          case  PHYSICAL_INTF:
            NIM_INTF_CLRMASKBIT(nimCtlBlk_g->physicalIntfMask,eventInfo.intIfNum);
            break;
          case  CPU_INTF:
            NIM_INTF_CLRMASKBIT(nimCtlBlk_g->cpuIntfMask,eventInfo.intIfNum);
            break;
          case  LAG_INTF:
            NIM_INTF_CLRMASKBIT(nimCtlBlk_g->lagIntfMask,eventInfo.intIfNum);
            break;
          case  LOGICAL_VLAN_INTF:
            NIM_INTF_CLRMASKBIT(nimCtlBlk_g->vlanIntfMask,eventInfo.intIfNum);
            break;
          case  SERVICE_PORT_INTF:
            NIM_INTF_CLRMASKBIT(nimCtlBlk_g->servicePortIntfMask,eventInfo.intIfNum);
            break;
          default:
            break;
        }

        /* shouldn't decrement for pseudo interfaces like tunnels... */
        nimCtlBlk_g->ifNumber--;

        NIM_CRIT_SEC_WRITE_EXIT();

        /* Now we can successfully delete the interface */
        nimDeleteInterface(eventInfo.intIfNum);

        break;

      default:
        NIM_CRIT_SEC_WRITE_EXIT();
        break;
    }
  }
  else
  {
    NIM_CRIT_SEC_WRITE_EXIT();

    NIM_LOG_ERROR("NIM: Failed event(%d), intIfNum(%d)\n",
                  eventInfo.event, eventInfo.intIfNum);
  }

  /* notify the event generator if there was a callback assigned */
  if (eventInfo.pCbFunc !=  NULLPTR)
  {
    status.event    = eventInfo.event;
    status.intIfNum = eventInfo.intIfNum;
    eventInfo.pCbFunc(status);
  }

  nimTracePortEvent(eventInfo.component,eventInfo.event,eventInfo.intIfNum, FALSE,status.handle);
}

/*********************************************************************
*
* @purpose  Define the system ifName,ifNameLong and ifDescr for the specified interface
*
* @param    configId        @b{(input)} NIM configID for the interface
* @param    configId        @b{(input)} NIM configID for the interface
* @param    *ifName         @b{(output)}  Ptr to buffer to contain name
* @param    *ifNameDescr    @b{(output)}  Ptr to buffer to contain description
* @param    *ifNameLong     @b{(output)}  Ptr to buffer to contain long format name
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    Buffer must be of size  NIM_IFNAME_SIZE
*           and  NIM_INTF_DESCR_SIZE, respectively
*
* @notes    "This routine provides a hook for customization of the ifName and ifDescr 
*            of a specific interface type. A valid ifName and ifDescr are expected to 
*            be passed in by the component.If nimIfDescrInfoSet() is overridden by a 
*            replacement routine,the replacement routine must be used to modify these
*            parameters for interfaces which must be customized for the product."
*            This function defines the parameters in FP naming style
*
*         
*
* @end
*********************************************************************/
void nimIfDescrInfoSet(nimConfigID_t *configId,  IANA_INTF_TYPE_t ianaType,
                           uchar8 *ifName, uchar8 *ifDescr, uchar8 *ifNameLong)
{
  return;
}


/*********************************************************************
* @purpose  Request to create an interface
*
* @param    *pRequest       @b{(input)}   pointer to nimIntfCreateRequest_t structure
* @param    *pOutput         @b{(output)}  The returned data for this create request
*
*
* @returns  one of RC_t enum values
*
* @notes  This routine is used to request the creation of an interface.
*         All interfaces must be created using this mechanism.
*
*
*         *pIntfIdInfo - This should contain sufficient information to uniquely
*                       identify the interface in the system.
*
*         *pIntfDescr  - This is a pointer to static information that describes this
*                       interface. This information MUST be completely specified
*                       with the create request.
*                       based on FD_NIM_DEFAULT* values.
*
*         *pDefaultCfg - This is a pointer to the information that would be stored
*                       in this particular port's configuration file if configuration
*                       were reset to default values.
*
*                       If this pointer is null, NIM presumes default configuration
*                       based on FD_NIM_DEFAULT* values.
*
*                       Note that all ports of the same type should have the same
*                       default configuration, although this methodology gives a
*                       system integrator flexibility on different devices.
*
*                       The default configuration will be applied on a subsequent
*                        ATTACH command if no non-volatile configuration exists for
*                       this interface.
*
*
*
* @end
*********************************************************************/
RC_t   nimIntfCreate(nimIntfCreateRequest_t *pRequest, nimIntfCreateOutput_t *pOutput)
{
  nimUSP_t  usp;
  RC_t   rc =  SUCCESS;
  uint32 *intIfNum;
  uint32 port;
  nimConfigIdTreeData_t  configIdInfo;
  nimConfigID_t          *intfIdInfo;  /* unique interface specification info */
  nimIntfDescr_t         *intfDescr;   /* unique interface descriptor  info */
  uint32 defaultPortModeEnable=0;   /* Default state read from card structure */
  uint32 runtimeEnable=0;           /* State read from HPC file if present */
  uint32 defaultExpand=0;           /* Does this port expand Immediately from 40G to 10G and vice versa */
  
  if ((pRequest ==  NULLPTR) || (pOutput ==  NULLPTR) ||
      (pRequest->pIntfDescr ==  NULLPTR) ||
      (pRequest->pIntfIdInfo ==  NULLPTR))
  {
    NIM_LOG_ERROR("NIM: Null data in call to nimIntfCreate\n");
    return  FAILURE; 
  }
  
  intIfNum = pOutput->intIfNum;
  intfIdInfo = pRequest->pIntfIdInfo;
  intfDescr = pRequest->pIntfDescr;
  *intIfNum = 0;
  
  NIM_CRIT_SEC_WRITE_ENTER();

  do
  {
    /* No interfaces can be created until we are in Execute */
    if ( nimPhaseStatusCheck() !=  TRUE)
    {
      rc =  FAILURE;
      break;
    }
    else
    {
      rc =  SUCCESS;
    }

    memcpy((void *)&usp,&pRequest->pIntfIdInfo->configSpecifier.usp,sizeof(nimUSP_t));

    switch (pRequest->pIntfIdInfo->type)
    {
      case  PHYSICAL_INTF:
      case  CPU_INTF:
      case  STACK_INTF:
        break;

      case  LAG_INTF:
        /*
         * get the u and s from cmgr, port is the id of the interface
         * The passed in intfId must be 1 based and it's range needs to be
         * determined as 1 - platIntfLagIntfMaxCountGet()
         */

        /*
         * Need to perform a LAG create for DTL
         * the LAG U and S need to be determined via CMGR
         */
        usp.unit = ( uchar8) LOGICAL_UNIT;
        usp.slot = ( uchar8)platSlotLagSlotNumGet ();
        usp.port = pRequest->pIntfIdInfo->configSpecifier.dot3adIntf;
        break;

      case  LOGICAL_VLAN_INTF:
        /*
         * get the u and s from cmgr, port is the id of the interface
         * The passed in intfId must be 1 based and it's range needs to be
         * determined as 1 - platIntfVlanIntfMaxCountGet()
         */

        usp.unit = ( uchar8) LOGICAL_UNIT;
        usp.slot = ( uchar8)platSlotVlanSlotNumGet ();

        if (nimPortInstanceNumGet(*pRequest->pIntfIdInfo, &port) ==  SUCCESS)
          usp.port = port;
        else
          usp.port = platIntfMaxCountGet() + 1;  /* Set to an invalid value */
        break;

      case  LOOPBACK_INTF:
        /*
         * get the u and s from cmgr, port is the id of the interface
         * The passed in loopbackId is 0 based and it's range needs to be
         * determined as 1 - platIntfLoopbackIntfMaxCountGet()
         */

        usp.unit = ( uchar8) LOGICAL_UNIT;
        usp.slot = ( uchar8)platSlotLoopbackSlotNumGet ();
        usp.port = pRequest->pIntfIdInfo->configSpecifier.loopbackId + 1;
        break;

      case  TUNNEL_INTF:
        /*
         * get the u and s from cmgr, port is the id of the interface
         * The passed in tunnelId is 0 based and it's range needs to be
         * determined as 1 - platIntfTunnelIntfMaxCountGet()
         */

        usp.unit = ( uchar8) LOGICAL_UNIT;
        usp.slot = ( uchar8)platSlotTunnelSlotNumGet ();
        usp.port = pRequest->pIntfIdInfo->configSpecifier.tunnelId + 1;
        break;

      case  SERVICE_PORT_INTF:
        /*
         * get the u and s from cmgr, port is the id of the interface
         * The passed in servicePortId is 0 based and it's range needs to be
         * determined as 1 - platIntfServicePortIntfMaxCountGet()
         */

        usp.unit = ( uchar8) LOGICAL_UNIT;
        usp.slot = ( uchar8)platSlotServicePortSlotNumGet ();
        usp.port = pRequest->pIntfIdInfo->configSpecifier.servicePortId + 1;
        break;


      default:
        NIM_LOG_MSG("NIM: Unknown interface type in nimIntfCreate\n");
        rc =  FAILURE;
        break;
    }

    /* if we have not gotten here successfully, return */
    if (rc !=  SUCCESS) break;

     NIM_IFDESCRINFO_SET(intfIdInfo, intfDescr->ianaType,
                                intfDescr->ifName,
                                intfDescr->ifDescr,
                                intfDescr->ifLongName);

    NIM_CRIT_SEC_WRITE_EXIT();

    /* quick check to see if the interface is created */
    if ((rc = nimGetIntIfNumFromUSP(&usp,intIfNum)) !=  ERROR)
    {
      NIM_LOG_MSG("NIM: Interface already created, %d.%d.%d\n",usp.unit,usp.slot,usp.port);
      rc =  ERROR;
    }
    else if (pRequest->pIntfIdInfo->type >=  MAX_INTF_TYPE_VALUE)
    {
      NIM_LOG_MSG("Invalid Interface Type in create request\n");
      rc =  FAILURE;
    }
    else if (nimNumberOfInterfaceExceeded(pRequest->pIntfIdInfo->type) ==  TRUE)
    {
      NIM_LOG_MSG("NIM: Number of interface of type(%d) exceeded during create\n",pRequest->pIntfIdInfo->type);
      rc =  FAILURE;
    }
    else
    {
      rc =  SUCCESS;
    }

    NIM_CRIT_SEC_WRITE_ENTER();

    if (rc !=  SUCCESS)
    {
        /* Note that we must break out of the loop while holding the write lock.
        */
        break;
    }

    if ((nimIntIfNumCreate(*pRequest->pIntfIdInfo,intIfNum) !=  SUCCESS) || (*intIfNum ==  0))
    {
      NIM_LOG_MSG("NIM: Failed to create the internal interface number\n");
      rc =  FAILURE;
      break;
    }
    else
    {
      /* reset all the information for this interface */
      memset(( void * )&nimCtlBlk_g->nimPorts[*intIfNum],0, sizeof( nimIntf_t )); 
    }

    /* set the fast lookup for USP to intIfNum */
    if (nimUnitSlotPortToIntfNumSet(&usp,*intIfNum) !=  SUCCESS)
    {
      NIM_LOG_MSG("NIM: Failed to set the mapping of USP to intIfNum fast lookup\n");
      rc =  FAILURE;

      break;
    }
    else
    {
      nimCtlBlk_g->nimPorts[*intIfNum].resetTime = osapiUpTimeRaw();

      /* set the intIfNums of the interface */
      nimCtlBlk_g->nimPorts[*intIfNum].intfNo = *intIfNum;

      nimCtlBlk_g->nimPorts[*intIfNum].runTimeMaskId = *intIfNum;

      NIM_CONFIG_ID_COPY(&nimCtlBlk_g->nimPorts[*intIfNum].configInterfaceId,pRequest->pIntfIdInfo);

      nimCtlBlk_g->nimPorts[*intIfNum].sysIntfType = pRequest->pIntfIdInfo->type;

      nimIfIndexCreate(usp,pRequest->pIntfIdInfo->type,&nimCtlBlk_g->nimPorts[*intIfNum].ifIndex,*intIfNum);

      nimCtlBlk_g->nimPorts[*intIfNum].usp = usp;

      /* copy the interface characteristics from the caller's request */
      memcpy(&nimCtlBlk_g->nimPorts[*intIfNum].operInfo,pRequest->pIntfDescr,sizeof(nimIntfDescr_t));

      /* Expandable Ports and Port Mode related changes */
     
      /* Enabling all ports by default */
      NIM_EXP_PORT_MODE_STATUS_ENABLE(nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.portModeFlags);
      NIM_EXP_PORT_MODE_SET_NONE(nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.portModeFlags);

      
      /* remaining items in the interface are present, resetTime, linkChangeTime, currentLoopbackState, intfState */
      if (pRequest->pIntfIdInfo->type ==
           PHYSICAL_INTF)
      {
        /* nimCtlBlk_g->nimPorts[*intIfNum].operInfo.activeMedium = FD_NIM_DEFAULT_ACTIVE_MEDIUM; */
      }
      /* Get the default config for the interface */
      if (pRequest->pDefaultCfg ==  NULLPTR)
      {
        /* no defaultConfig was supplied by the caller; therefore, use NIMs default for the type */
        nimConfigDefaultGet(pRequest->pIntfDescr,&nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg);

#if 0
        /* For SFP/SFP+/Non-Combo capable ports, set default AutoNeg to 0 */
        if (nimCtlBlk_g->nimPorts[*intIfNum].operInfo.connectorType !=  RJ45)
        {
          if (nimCtlBlk_g->nimPorts[*intIfNum].operInfo.ianaType ==  IANA_10G_ETHERNET)
          {
            nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.negoCapabilities = 0;
            nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.cfgNegoCapabilities = 0;
            nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.ifSpeed = FD_NIM_NO_NEG_10G_ENET_SPEED;
          }
          nimCtlBlk_g->nimPorts[*intIfNum].operInfo.activeMedium = PORT_MEDIUM_FIBER;
        }
#endif
        if ( PHY_CAP_EXPANDABLE_PORT_CHECK(nimCtlBlk_g->nimPorts[*intIfNum].operInfo.phyCapability))
        {
        NIM_EXP_PORT_DBG("For intf %d defaults, flag 0x%x got port mode %d enable %d Immediate Mode %d\n", 
                         *intIfNum,
                         nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.portModeFlags,         
                         NIM_EXP_PORT_MODE_GET(nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.portModeFlags),
                         NIM_EXP_PORT_MODE_STATUS_GET(nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.portModeFlags),
                         nimCtlBlk_g->nimPorts[*intIfNum].operInfo.ifImmediateExpand);
        }
        if ( PHY_CAP_FEC_SUPPORT_CHECK(nimCtlBlk_g->nimPorts[*intIfNum].operInfo.phyCapability))
        {
          if (nimCtlBlk_g->nimPorts[*intIfNum].operInfo.fecCapability >  CAP_FEC_ENABLE)
          {
            nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.fecMode =
              nimCtlBlk_g->nimPorts[*intIfNum].operInfo.defaultFEC;
          }
        }
      }
      else
      {
        /* use the callers supplied default config */
        memcpy(&nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg,pRequest->pDefaultCfg,sizeof(nimIntfConfig_t));
        /* This field not set by caller */
        nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.mgmtAdminState = FD_NIM_ADMIN_STATE;
        nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.softShutdownState = FD_NIM_SOFT_SHUT_STATE;
      }

      NIM_INTF_CLRMASKBIT(nimCtlBlk_g->linkStateMask, nimCtlBlk_g->nimPorts[*intIfNum].runTimeMaskId);

      if (nimCtlBlk_g->nimPorts[*intIfNum].capabilityCfg.medium != 0 )
      {
        nimCtlBlk_g->nimPorts[*intIfNum].operInfo.activeMedium = 
          nimCtlBlk_g->nimPorts[*intIfNum].capabilityCfg.medium;
      }

      nimCtlBlk_g->nimPorts[*intIfNum].present =  TRUE;

      /* check to see if we have a saved config for the interface */
      if ((rc = nimConfigSet(&nimCtlBlk_g->nimPorts[*intIfNum],NIM_CFG_VER_CURRENT)) !=  SUCCESS)
      {
        NIM_LOG_MSG("NIM: Failed to set the config for interface\n");
        rc =  FAILURE;
      }
      else if ((rc = nimUtilIntfStateSet(*intIfNum, INTF_CREATING)) !=  SUCCESS)
      {
        NIM_LOG_MSG("NIM: Failed to set intf state to  INTF_CREATING\n");
        rc =  FAILURE;
      }
      else
      {        
        rc =  SUCCESS;
#ifndef  FEATURE_EXPANDABLE_PORTS_NO_HPC 
        if ( FALSE ==
            NIM_EXP_IS_PORT_MODE_NONE(nimCtlBlk_g->nimPorts[*intIfNum].defaultCfg.portModeFlags))
        {
          if ( TRUE ==  nimCtlBlk_g->nimPorts[*intIfNum].operInfo.ifImmediateExpand)
          {
            /* Set whatever was read from HPC as the new Configured state.  */
            NIM_EXP_PORT_MODE_STATUS_SET(nimCtlBlk_g->nimPorts[*intIfNum].configPort.cfgInfo.portModeFlags, 
                                         ( BOOL)runtimeEnable);        
            if ( PHY_CAP_EXPANDABLE_PORT_CHECK(pRequest->pIntfDescr->phyCapability))
            {
              if ( TRUE == runtimeEnable)
              {
                NIM_EXP_PORT_MODE_SET_QUAD_40G(nimCtlBlk_g->nimPorts[*intIfNum].configPort.cfgInfo.portModeFlags);
              }
              else
              {
                NIM_EXP_PORT_MODE_SET_SINGLE_10G(nimCtlBlk_g->nimPorts[*intIfNum].configPort.cfgInfo.portModeFlags);
              }
            }              
          }          
        }
#endif
        if ( PHY_CAP_EXPANDABLE_PORT_CHECK(pRequest->pIntfDescr->phyCapability))
        {
          NIM_EXP_PORT_DBG("For intf %d after startup apply got port mode %d enable %d Immediate Mode %d\n", 
               *intIfNum,
                           NIM_EXP_PORT_MODE_GET(nimCtlBlk_g->nimPorts[*intIfNum].configPort.cfgInfo.portModeFlags) ,
                           NIM_EXP_PORT_MODE_STATUS_GET(nimCtlBlk_g->nimPorts[*intIfNum].configPort.cfgInfo.portModeFlags),
                           nimCtlBlk_g->nimPorts[*intIfNum].operInfo.ifImmediateExpand);
        }
      }
    }

  } while ( 0 );

  if (rc !=  SUCCESS)
  {
    if ((*intIfNum != 0) && (rc !=  ERROR))
    {
      /* rewind the data for the create */
      (void)nimUtilIntfStateSet(*intIfNum, INTF_UNINITIALIZED);
      nimIntIfNumDelete(*intIfNum);
      nimUnitSlotPortToIntfNumSet(&usp,0);
      memset((void*)&nimCtlBlk_g->nimPorts[*intIfNum],0,sizeof(nimIntf_t));


      memset(&configIdInfo, 0 , sizeof( configIdInfo));
      NIM_CONFIG_ID_COPY(&configIdInfo.configId, pRequest->pIntfIdInfo);
      configIdInfo.intIfNum = *intIfNum;
      (void)nimConfigIdTreeEntryDelete( &configIdInfo);
    }
    NIM_CRIT_SEC_WRITE_EXIT();
  }
  else
  {
    if (nimCtlBlk_g->nimHighestIntfNumber < *intIfNum)
    {
      nimCtlBlk_g->nimHighestIntfNumber = *intIfNum;
    }
    /* give back the sema before calling out to the other components */
    NIM_CRIT_SEC_WRITE_EXIT();

    if (rc !=  SUCCESS)
    {
      NIM_LOG_MSG("NIM: Failed to notify the components of  CREATE event\n");

      NIM_CRIT_SEC_WRITE_ENTER();

      /* rewind the data for the create */
      (void)nimUtilIntfStateSet(*intIfNum, INTF_UNINITIALIZED);
      nimIntIfNumDelete(*intIfNum);
      nimUnitSlotPortToIntfNumSet(&usp,0);
      memset((void*)&nimCtlBlk_g->nimPorts[*intIfNum],0,sizeof(nimIntf_t));


      memset(&configIdInfo, 0 , sizeof( configIdInfo));
      NIM_CONFIG_ID_COPY(&configIdInfo.configId, pRequest->pIntfIdInfo);
      configIdInfo.intIfNum = *intIfNum;
      (void)nimConfigIdTreeEntryDelete( &configIdInfo);

      NIM_CRIT_SEC_WRITE_EXIT();
    }
    else
    {

      NIM_CRIT_SEC_WRITE_ENTER();

      nimCtlBlk_g->numberOfInterfacesByType[pRequest->pIntfIdInfo->type]++;

      nimCtlBlk_g->nimNumberOfPortsPerUnit[(uint32)usp.unit]++;

      NIM_CRIT_SEC_WRITE_EXIT();
    }
  }

  return rc;
}


/*********************************************************************
* @purpose  Notify the system application layer of changes in interfaces
*
* @param    unit        unit number (stack unit)
* @param    slot        slot number
* @param    port        port number
* @param    cardType    a card descriptor as given to Card Manager from HPC
* @param    event       an event designator such as  ENABLE
* @param    interfaceType   the type of interface such as PHYSICAL
*
* @returns  void
*
* @notes
*
* @end
*********************************************************************/
RC_t   nimCmgrNewIntfChangeCallback(uint32 unit, uint32 slot, uint32 port,
                                       uint32 cardType, PORT_EVENTS_t event,
                                       SYSAPI_HPC_PORT_DESCRIPTOR_t *portData,
                                                     enetMacAddr_t *macAddr)
{
  RC_t                rc =  FAILURE;
  nimIntfCreateRequest_t pRequest;
  uint32   intIfNum;
  NIM_HANDLE_t  handle;
  NIM_INTF_CREATE_INFO_t eventInfo;
  NIM_EVENT_NOTIFY_INFO_t  notifyEventInfo;
  const  char8*        pIfaceSpeed = NULL;

  /* pIfaceSpeedDescr: "<port speed>" */
   char8               pIfaceSpeedDescr[ NIM_INTF_SPEED_DESCR_SIZE]; 
  
  /* workBuffer: "Unit: <unit> Slot: <slot> Port: <port>" */
   char8               workBuffer[ NIM_INTF_DESCR_SIZE]; 

  /* workBuffer1: "<unit>/<slot>/<port>" */
   char8               workBuffer1[ NIM_USP_DESCR_SIZE]; 

  nimConfigID_t          pIntfIdInfo;  /* unique interface specification info */
  nimIntfDescr_t         pIntfDescr;   /* unique interface descriptor  info */
  nimIntfCreateOutput_t  output;

  /* Initialize data structures */

  memset((void *)&pIntfIdInfo,0,sizeof(nimConfigID_t));
  memset((void *)&eventInfo,0,sizeof(NIM_INTF_CREATE_INFO_t));
  memset((void *)&pIntfDescr,0,sizeof(nimIntfDescr_t));
  memset((void *)&pRequest,0,sizeof(nimIntfCreateRequest_t));

  output.handle =   &handle;
  output.intIfNum = &intIfNum;

  eventInfo.component =  CARDMGR_COMPONENT_ID;
  eventInfo.pCbFunc   = nimEventCmgrDebugCallback;

  /* setup the config ID */
  pIntfIdInfo.configSpecifier.usp.unit = unit;
  pIntfIdInfo.configSpecifier.usp.slot = slot;
  pIntfIdInfo.configSpecifier.usp.port = port;

  /* setup the request block pointers */
  pRequest.pDefaultCfg  =  NULLPTR;
  pRequest.pIntfDescr   = &pIntfDescr;
  pRequest.pIntfIdInfo  = &pIntfIdInfo;
  pRequest.pCreateInfo  = &eventInfo;

  memset(workBuffer, 0, sizeof(workBuffer));
  memset(workBuffer1, 0, sizeof(workBuffer1));
  memset(pIfaceSpeedDescr, 0, sizeof(pIfaceSpeedDescr));
  
  switch (portData->type)
  {
    case   IANA_OTHER_CPU:
      pIntfIdInfo.type =  CPU_INTF;
      snprintf (pIntfDescr.ifDescr, sizeof(pIntfDescr.ifDescr),
                "%s %s %d %s %d",
                " CPU Interface for", "Slot:", slot, "Port:", port);

      snprintf (pIntfDescr.ifName, sizeof(pIntfDescr.ifName),
                "%s %d/%d",
                "CPU Interface: ", slot, port);
      break;

    case  IANA_LAG:
      pIntfIdInfo.type =  LAG_INTF;
      snprintf (pIntfDescr.ifName, sizeof(pIntfDescr.ifName),
                "%d/%d", slot, port);
      break;

    case   IANA_ETHERNET:
    case   IANA_FAST_ETHERNET:
    case   IANA_FAST_ETHERNET_FX:
    case   IANA_GIGABIT_ETHERNET:
    case   IANA_2P5G_ETHERNET:
    case   IANA_5G_ETHERNET:
    case   IANA_10G_ETHERNET:
    case   IANA_20G_ETHERNET:
    case   IANA_25G_ETHERNET:
    case   IANA_40G_ETHERNET:
    case   IANA_50G_ETHERNET:
    case   IANA_100G_ETHERNET:
    case   IANA_200G_ETHERNET:
    case   IANA_400G_ETHERNET:

      switch (portData->type)
      {
      case   IANA_GIGABIT_ETHERNET:
        pIfaceSpeed = IANA_GIGABIT_ETHERNET_DESC;
        osapiStrncpySafe(pIfaceSpeedDescr, GIGA_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;
        
      case   IANA_2P5G_ETHERNET:
        pIfaceSpeed = IANA_2P5G_ETHERNET_DESC;     
        osapiStrncpySafe(pIfaceSpeedDescr, TWOPOINTFIVEGIG_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;

      case   IANA_5G_ETHERNET:
        pIfaceSpeed = IANA_5G_ETHERNET_DESC;     
        osapiStrncpySafe(pIfaceSpeedDescr, FIVE_GIGA_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;

      case   IANA_10G_ETHERNET:
        pIfaceSpeed = IANA_10G_ETHERNET_DESC;     
        osapiStrncpySafe(pIfaceSpeedDescr, TENGIG_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;
        
      case   IANA_20G_ETHERNET:
        pIfaceSpeed = IANA_20G_ETHERNET_DESC;     
        osapiStrncpySafe(pIfaceSpeedDescr, TWENTYGIG_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;
        
      case   IANA_25G_ETHERNET:
        pIfaceSpeed = IANA_25G_ETHERNET_DESC;     
        osapiStrncpySafe(pIfaceSpeedDescr, TWENTYFIVEGIG_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;
        
      case   IANA_40G_ETHERNET:
        pIfaceSpeed = IANA_40G_ETHERNET_DESC;     
        osapiStrncpySafe(pIfaceSpeedDescr, FORTYGIG_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;
        
      case   IANA_50G_ETHERNET:
        pIfaceSpeed = IANA_50G_ETHERNET_DESC;
        osapiStrncpySafe(pIfaceSpeedDescr, FIFTYGIG_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;
        
      case   IANA_100G_ETHERNET:
        pIfaceSpeed = IANA_100G_ETHERNET_DESC;     
        osapiStrncpySafe(pIfaceSpeedDescr, HUNDREDGIG_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;
        
      case   IANA_200G_ETHERNET:
        pIfaceSpeed = IANA_200G_ETHERNET_DESC;     
        osapiStrncpySafe(pIfaceSpeedDescr, TWOHUNDREDGIG_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;

      case   IANA_400G_ETHERNET:
        pIfaceSpeed = IANA_400G_ETHERNET_DESC;     
        osapiStrncpySafe(pIfaceSpeedDescr, FOURHUNDREDGIG_ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;

      default:
        pIfaceSpeed = IANA_FAST_ETHERNET_DESC;    
        osapiStrncpySafe(pIfaceSpeedDescr, ETH_INTF_IFNAME_LONG_PREFIX, sizeof(pIfaceSpeedDescr));
        break;
      }

      pIntfIdInfo.type =  PHYSICAL_INTF;
      snprintf (workBuffer, sizeof(workBuffer), "%s %d %s %d",
                "Slot:", slot, "Port:", port);

      snprintf (pIntfDescr.ifName, sizeof(pIntfDescr.ifName), "%d/%d",
                slot, port);

      osapiSnprintf (workBuffer1, sizeof(workBuffer1), "%d/%d",
                     slot, port);

      snprintf (pIntfDescr.ifDescr, sizeof(pIntfDescr.ifDescr), "%s %s",
                workBuffer, pIfaceSpeed);

      osapiSnprintf (pIntfDescr.ifLongName, sizeof(pIntfDescr.ifLongName), "%s %s",
                     pIfaceSpeedDescr, workBuffer1);
      break;

    case  IANA_L2_VLAN:
      pIntfIdInfo.type =  LOGICAL_VLAN_INTF;
      break;

    case  IANA_OTHER_SERV_PORT:
      pIntfIdInfo.type =  SERVICE_PORT_INTF;
      pIntfIdInfo.configSpecifier.servicePortId = 0;

      snprintf (pIntfDescr.ifDescr, sizeof(pIntfDescr.ifDescr),
                "%s %s %d %s %d",
                " Service Port Interface for", "Slot:", slot, "Port:", port);

      snprintf (pIntfDescr.ifName, sizeof(pIntfDescr.ifName),
                "%s %d/%d",
                "Service Port Interface: ", slot, port);
      break;

    default:
      NIM_LOG_MSG("NIM: Unknown interface type\n");
      return( FAILURE);
  }


  memcpy(&pIntfDescr.macAddr, macAddr,  ENET_MAC_ADDR_LEN);
  memcpy(&pIntfDescr.l3MacAddr.addr, macAddr,  ENET_MAC_ADDR_LEN);

  pIntfDescr.configurable   =  TRUE;
  pIntfDescr.settableParms  =  INTF_PARM_LINKTRAP;
  
  if (pIntfIdInfo.type !=  SERVICE_PORT_INTF)
  {
    pIntfDescr.settableParms |=  INTF_PARM_ADMINSTATE |  INTF_PARM_MTU |
                                 INTF_PARM_MACADDR | 
                                 INTF_PARM_LOOPBACKMODE |
                                 INTF_PARM_MACROPORT |  INTF_PARM_ENCAPTYPE |
                                 INTF_PARM_NW_INTF_TYPE;
  }
  switch (pIntfIdInfo.type)
  {
    case  PHYSICAL_INTF:
      pIntfDescr.settableParms |=  INTF_PARM_AUTONEG |  INTF_PARM_SPEED |
         INTF_PARM_FRAMESIZE| INTF_PARM_DEBOUNCETIME;

      if (! PHY_CAP_PORTSPEED_AUTO_NEG_CHECK(portData->phyCapabilities))
      {
        pIntfDescr.settableParms &= ~ INTF_PARM_AUTONEG;
      }

      if ( PHY_CAP_AUTO_CONFIG_CHECK(portData->phyCapabilities))
      {
        pIntfDescr.settableParms &= ~( INTF_PARM_AUTONEG |  INTF_PARM_SPEED);
      }
      
      if ((portData->type &  IANA_40G_ETHERNET) && 
           PHY_CAP_EXPANDABLE_PORT_CHECK(portData->phyCapabilities))
      {
        pIntfDescr.settableParms |=  INTF_PARM_PORTMODE;
        if ( PHY_CAP_PORTSPEED_FULL_10_CHECK(portData->phyCapabilities))
        {
          pIntfDescr.settableParms |=  INTF_PARM_SPEED;
        }
        NIM_EXP_PORT_DBG("For usp %d/%d/%d, setting parm portmode on\n", unit, slot,port);
      }
      
      break;

    case  LAG_INTF:
      pIntfDescr.settableParms |=  INTF_PARM_FRAMESIZE;
      break;

    default:
      /* do nothing */
      break;
  }

  /*pIntfDescr.connectorType  = portData->connectorType;*/
  pIntfDescr.defaultSpeed   =  portData->defaultSpeed;
  pIntfDescr.frameSize.largestFrameSize = 1500;
  pIntfDescr.ianaType       = portData->type;
  pIntfDescr.internal       =  FALSE;
  pIntfDescr.phyCapability  = portData->phyCapabilities;
  memset((void*)&pIntfDescr.macroPort,0,sizeof(nimMacroPort_t));
#ifdef  MAC_ENET_BCAST
  memcpy (&pIntfDescr.bcastMacAddr,  & ENET_BCAST_MAC_ADDR, 6);
#endif


  if (nimPhaseStatusCheck() !=  TRUE)
  {
    NIM_LOG_MSG("NIM: Attempted event (%d), on USP %d.%d.%d before phase 3\n",event,unit,slot,port);
    return(rc);
  }

  if ((unit < 1) || (slot > nimCtlBlk_g->maxNumOfSlotsPerUnit) || (port < 1))
  {
    NIM_LOG_MSG("NIM: attempted event (%d) with invalid USP, %d.%d.%d\n",event,unit,slot,port);
    return(rc);
  }

  if (event ==  CREATE)
  {
    if (nimIntfCreate(&pRequest,&output) ==  SUCCESS)
    {
      NIM_LOG_MSG("Success in create\n");
      notifyEventInfo.component =  CARDMGR_COMPONENT_ID;
      notifyEventInfo.pCbFunc   = NULL;
      notifyEventInfo.event =  CREATE;
      notifyEventInfo.intIfNum = intIfNum;
      rc = nimEventIntfNotify(notifyEventInfo,&handle);
      if (rc !=  SUCCESS)
      {
        LOG_ERROR (rc);
      }
    }
    else
    {
      NIM_LOG_MSG("Failed in create\n");
    }
  }
  else
  {
    NIM_LOG_MSG("NIM: BAD event for my test\n");
  }

  return rc;
}



/*********************************************************************
* @purpose  Post Processor for ATTACH events
*
* @param    eventInfo    @b{(input)}   The event information as assigned by the generator
*
* @returns  none
*
* @notes    none
*
* @end
*
*********************************************************************/
void nimEventAttachPostProcess(NIM_EVENT_NOTIFY_INFO_t eventInfo)
{
   BOOL                 isLinkUp;
  NIM_EVENT_NOTIFY_INFO_t attachEventInfo;
  NIM_HANDLE_t            handle;
  RC_t                 rc =  SUCCESS;

  if ( SUCCESS) /* TODO */
  {
    NIM_CRIT_SEC_WRITE_ENTER();

    if (isLinkUp ==  TRUE)
    {
      NIM_INTF_SETMASKBIT(nimCtlBlk_g->linkStateMask, nimCtlBlk_g->nimPorts[eventInfo.intIfNum].runTimeMaskId);

      NIM_CRIT_SEC_WRITE_EXIT();

      /*
       * don't need to keep the handle around
       * need to notify the rest of the system of the link state
       */
      attachEventInfo.event         =  UP;
      attachEventInfo.component     =  NIM_COMPONENT_ID;
      attachEventInfo.intIfNum      = eventInfo.intIfNum;
      attachEventInfo.pCbFunc       =  NULLPTR;

      if (nimEventIntfNotify(attachEventInfo,&handle) !=  SUCCESS)
      {
        NIM_LOG_MSG("NIM: Failed to send LINK UP on queue\n");
        rc =  FAILURE;
      }
    }
    else
    {
      NIM_INTF_CLRMASKBIT(nimCtlBlk_g->linkStateMask, nimCtlBlk_g->nimPorts[eventInfo.intIfNum].runTimeMaskId);

      NIM_CRIT_SEC_WRITE_EXIT();
    }

  }
  else
    rc =  FAILURE;

}





