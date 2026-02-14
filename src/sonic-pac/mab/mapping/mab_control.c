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

#define  MAC_EAPOL_PDU

#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>

#include "osapi.h"
#include "mab_include.h"
#include "auth_mgr_exports.h"
#include "radius_attr_parse.h"
#include "utils_api.h"
#include "osapi_sem.h"
#include "sysapi.h"

#include "mab_include.h"
#include "mab_client.h"
#include "mab_auth.h"
#include "mab_timer.h"
#include "mab_struct.h"
#include "mab_socket.h"


static  enetMacAddr_t  EAPOL_PDU_MAC_ADDR =
{{0x01, 0x80, 0xC2, 0x00, 0x00, 0x03}};

extern mabBlock_t *mabBlock;
BOOL mabInitializationState = FALSE;

#define MAX_CLIENTS 1024 

/*********************************************************************
 * @purpose  Initialize mab tasks and data
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments none
 *
 * @end
 *********************************************************************/
RC_t mabStartTasks()
{
  /* semaphore creation for task protection over the common data */
  /* Read write lock protection */
  if (osapiRWLockCreate(&mabBlock->mabRWLock,
        OSAPI_RWLOCK_Q_PRIORITY) ==  FAILURE)
  {
     LOGF( LOG_SEVERITY_INFO,"Error creating mabRWlock semaphore \n");
    return  FAILURE;
  }

  /* fd on mab side  for  MAB- radius client communication */
  if (mabBlock->send_fd > 0)
  {
     close(mabBlock->send_fd);
     mabBlock->send_fd = -1;
  }

  /* fd on radius side  for  MAB- radius client communication */
  if (mabBlock->recv_fd > 0)
  {
     close(mabBlock->recv_fd);
     mabBlock->recv_fd = -1;
  }

  mab_radius_client_alloc(&mabBlock->rad_cxt);
  /* init mab - eloop sockets */

  if (-1 == mab_radius_init_send_socket(&mabBlock->send_fd))
  {
     LOGF( LOG_SEVERITY_INFO,
        "Failed to create mab send_fd.\n");
    return  FAILURE;
  }

  if (-1 == mab_radius_init_recv_socket(&mabBlock->recv_fd))
  {
     LOGF( LOG_SEVERITY_INFO,
        "Failed to create mab recv_fd.\n");
    return  FAILURE;
  }

  /* create dotxTask - to service mab message queue */
  mabBlock->mabTaskId = osapiTaskCreate("mabTask", (void *)mabTask, 0, 0,
      2 * mabSidDefaultStackSize(),
      mabSidDefaultTaskPriority(),
      mabSidDefaultTaskSlice());

  if (mabBlock->mabTaskId == 0)
  {
     LOGF( LOG_SEVERITY_INFO,
        "Failed to create mab task.\n");
    return  FAILURE;
  }

  if (osapiWaitForTaskInit( MAB_TASK_SYNC,  WAIT_FOREVER) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_INFO,
        "Unable to initialize mab task.\n");
    return  FAILURE;
  }

    /* create mabSrvrTask - to service mab message queue */
  mabBlock->mabSrvrTaskId =
    osapiTaskCreate ("mabSrvrTask", (void *) mabSrvrTask, 0, 0,
                     2 * mabSidDefaultStackSize(),
                     mabSidDefaultTaskPriority(),
                     mabSidDefaultTaskSlice());

  if (mabBlock->mabSrvrTaskId == 0)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Failed to create mab task.\n");
    return  FAILURE;
  }

  if (osapiWaitForTaskInit ( MAB_SRVR_TASK_SYNC,  WAIT_FOREVER) !=
                             SUCCESS)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Unable to initialize mab srvr task.\n");
    return  FAILURE;
  }
    /* create mabSrvrTask - to service mab message queue */
  mabBlock->mabEloopTaskId =
    osapiTaskCreate ("mabEloopTask", (void *) mabEloopTask, 0, 0,
                     2 * mabSidDefaultStackSize(),
                     mabSidDefaultTaskPriority(),
                     mabSidDefaultTaskSlice());

  if (mabBlock->mabEloopTaskId == 0)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Failed to create mab eloop task.\n");
    return  FAILURE;
  }

  if (osapiWaitForTaskInit ( MAB_ELOOP_TASK_SYNC,  WAIT_FOREVER) !=
                             SUCCESS)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Unable to initialize mab eloop task.\n");
    return  FAILURE;
  }

  return  SUCCESS;
}

/*********************************************************************
 * @purpose  mab task which serves the request queue
 *
 * @param    none
 *
 * @returns  void
 *
 * @comments User-interface writes and PAE PDUs are serviced off
 *           of the mabQueue
 *
 * @end
 *********************************************************************/
void mabTask()
{
  mabMsg_t msg;

  (void)osapiTaskInitDone( MAB_TASK_SYNC);

  /* allocate the required data structures */
  mabInitPhase1Process();
  
  /* do inter component registration */
  mabInitPhase2Process();

  mabInitPhase3Process( FALSE);

  mabInitializationState = TRUE;

  for (;;)
  {
    if (osapiSemaTake(mabBlock->mabTaskSyncSema,  WAIT_FOREVER) !=  SUCCESS)
    {
       LOGF( LOG_SEVERITY_ERROR,
          "Unable to acquire MAB message queue semaphore.");
      continue;
    }

    if (osapiMessageReceive(mabBlock->mabQueue, (void*)&msg, (uint32)sizeof(mabMsg_t),
           WAIT_FOREVER) ==  SUCCESS)
    {
      (void)mabDispatchCmd(&msg);
    }
    else
    {
       LOGF( LOG_SEVERITY_ERROR, "mabTask: Failed to receive message on mabQueue");
    }
  }
}

/*********************************************************************
* @purpose  mab srvr task which serves the request queue
*
* @param    none
*
* @returns  void
*
* @comments external applications are serviced off
*           of the mabQueue
*
* @end
*********************************************************************/
void mabEloopTask ()
{
  MAB_EVENT_TRACE(
    "%s:%d\r\n", __FUNCTION__, __LINE__);

  (void) osapiTaskInitDone ( MAB_ELOOP_TASK_SYNC);

  mab_eloop_register(mabBlock->recv_fd, mabBlock->rad_cxt);
  return;
}


/*********************************************************************
* @purpose  mab srvr task which serves the request queue
*
* @param    none
*
* @returns  void
*
* @comments external applications are serviced off
*           of the mabQueue
*
* @end
*********************************************************************/
void mabSrvrTask ()
{
  MAB_EVENT_TRACE(
    "%s:%d\r\n", __FUNCTION__, __LINE__);

  (void) osapiTaskInitDone ( MAB_SRVR_TASK_SYNC);

  mabBlock->conn_list = (connection_list_t *)osapiMalloc( MAB_COMPONENT_ID, 
                                                         MAX_CLIENTS * sizeof(connection_list_t));

  mab_socket_server_handle(&mabBlock->mabServerSock);
  return;
}

/*********************************************************************
 * @purpose  Save the data in a message to a shared memory
 *
 * @param    event   @b{(input)} event type
 * @param    *data   @b{(input)} pointer to data
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments Once the message is serviced, this variable size data will
 *           be retrieved
 *
 * @end
 *********************************************************************/
RC_t mabFillMsg(void *data, mabMsg_t *msg)
{
  switch (msg->event)
  {
    /* events originating from UI */
    case mabMgmtPortMABEnableSet:
    case mabMgmtPortMABDisableSet:
    case mabMgmtPortInitializeSet:
    case mabMgmtPortControlModeSet:
    case mabMgmtPortHostModeSet:
      /* add to queue uint32 size */
      memcpy(&msg->data.msgParm, data, sizeof(uint32));
      break;

    case mabVlanDeleteEvent:
    case mabVlanAddEvent:
    case mabVlanAddPortEvent:
    case mabVlanDeletePortEvent:
    case mabVlanPvidChangeEvent:
      memcpy(&msg->data.vlanData, data, sizeof(dot1qNotifyData_t));
      break;

    case mabIntfChange:
      /* add to queue a NIM correlator */
      memcpy(&msg->data.mabIntfChangeParms, data, sizeof(mabIntfChangeParms_t));
      break;

    case mabAuthMgrEvent:
      memcpy(&msg->data.mabAuthmgrMsg, data, sizeof(mabAuthmgrMsg_t));
      break;

    case mabIntfStartup:
      /* add to queue a NIM correlator */
      memcpy(&msg->data.startupPhase, data, sizeof(NIM_STARTUP_PHASE_t));
      break;

    case mabAaaInfoReceived:
      /* add to queue a char pointer */
      memcpy(&msg->data.mabAaaMsg, data, sizeof(mabAaaMsg_t));
      break;

    case mabRadiusConfigUpdate:
      /* add to queue a char pointer */
      memcpy(&msg->data.mabRadiusCfgMsg, data, sizeof(mabRadiusServer_t));
      break;

    case mabAddMacInMacDB:
    case mabTimeTick:
      break; /* NULL data, proceed */

    default:
      /* unmatched event */
      return  FAILURE;

  } /* switch */

  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Send a command to mab queue
 *
 * @param    event      @b{(input)} event type
 * @param    intIfNum   @b{(input)} internal interface number
 * @param    *data      @b{(input)} pointer to data
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments Command is queued for service
 *
 * @end
 *********************************************************************/
RC_t mabIssueCmd(uint32 event, uint32 intIfNum, void *data)
{
  mabMsg_t msg;
  RC_t rc;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  nimGetIntfName(intIfNum,  ALIASNAME, ifName);

  /* copy event, intIfNum and data ptr to msg struct */
  msg.event = event;
  msg.intf = intIfNum;

  if (data !=  NULLPTR)
    (void)mabFillMsg(data, &msg);

  /* send message */
  rc = osapiMessageSend(mabBlock->mabQueue, &msg, (uint32)sizeof(mabMsg_t),  NO_WAIT,  MSG_PRIORITY_NORM);
  if (rc !=  SUCCESS)
  {
    MAB_ERROR_SEVERE("Failed to send to mabQueue! Event: %u, interface: %s\n", event, ifName);
  }

  rc = osapiSemaGive(mabBlock->mabTaskSyncSema);

  return rc;
}

/*********************************************************************
 * @purpose  Route the event to a handling function and grab the parms
 *
 * @param    msg   @b{(input)} message containing event and interface number
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabDispatchCmd(mabMsg_t *msg)
{
  RC_t rc =  FAILURE;

  (void)osapiWriteLockTake(mabBlock->mabRWLock,  WAIT_FOREVER);
  switch (msg->event)
  {
    case mabIntfChange:
      rc = mabIhProcessIntfChange(msg->intf,
          msg->data.mabIntfChangeParms.intfEvent,
          msg->data.mabIntfChangeParms.nimCorrelator);
      break;

    case mabIntfStartup:
      rc = mabIhProcessIntfStartup(msg->data.startupPhase);
      break;

    case mabTimeTick:
      rc = mabTimerAction();
      break;

    case mabMgmtPortInitializeSet:
      rc = mabCtlPortInitializeSet(msg->intf, msg->data.msgParm);
      break;

    case mabMgmtPortControlModeSet:
      rc = mabCtlPortControlModeSet(msg->intf, msg->data.msgParm);
      break;

    case mabMgmtPortHostModeSet:
      rc = mabPortCtrlHostModeSet(msg->intf, msg->data.msgParm);
      break;

    case mabMgmtPortStatsClear:
      rc =  mabCtlPortStatsClear(msg->intf);
      break;

    case mabAaaInfoReceived:
      rc = mabRadiusResponseProcess(msg->intf, msg->data.mabAaaMsg.resp);
      break;

    case mabRadiusConfigUpdate:
      rc = mabRadiusChangeHandle(&msg->data.mabRadiusCfgMsg);
      break;

    case mabMgmtApplyConfigData:
      rc = mabCtlApplyConfigData();
      break;

    case mabVlanDeleteEvent:
    case mabVlanAddEvent:
    case mabVlanAddPortEvent:
    case mabVlanDeletePortEvent:
    case mabVlanPvidChangeEvent:
      mabVlanChangeProcess(msg->event,msg->intf,&(msg->data.vlanData));
      rc =  SUCCESS;
      break;

    case mabMgmtPortMABEnableSet:
      rc = mabCtlPortMABEnableSet(msg->intf);
      break;

    case mabMgmtPortMABDisableSet:
      rc = mabCtlPortMABDisableSet(msg->intf);
      break;

    case mabAddMacInMacDB:      
      rc = mabAddMac(msg->intf);
      break;  

    case mabAuthMgrEvent:
      rc = mabAuthmgrEventProcess(msg->intf, &msg->data.mabAuthmgrMsg);
      break;

    default:
      rc =  FAILURE;
  }

  (void)osapiWriteLockGive(mabBlock->mabRWLock);
  return rc;
}

/*********************************************************************
 * @purpose  Add supplicant MAC in MAC database
 *
 * @param    lIntIfNum   @b{(input)} logical interface number that this PDU was received on
 *
 * @returns   SUCCESS or  FAILURE
 * 
 * @end
 *********************************************************************/
RC_t mabAddMac(uint32 lIntIfNum)
{
  mabLogicalPortInfo_t  *entry =  NULLPTR;
  RC_t rc =  FAILURE;

  entry = mabLogicalPortInfoGet(lIntIfNum);
  if (entry !=  NULLPTR) {
     rc = mabMacAddrInfoAdd(&(entry->client.suppMacAddr), lIntIfNum);    
  } 
  return rc;
}

/*********************************************************************
 * @purpose  Check if pdu is to be processed considering logical port
 *           use and availability
 *
 * @param    intIfNum     @b{(input)} physical interface
 * @param    *srcMac      @b{(input)} supplicant mac address
 * @param    logicalPort  @b{(input)} logical port
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments Command is queued for service
 *
 * @end
 *********************************************************************/
RC_t mabDynamicUserPduMapCheck(uint32 intIfNum,  char8 *srcMac, uint32 *lIntIfNum,  BOOL *existing_node)
{
  uint32 lIndex;
  mabPortCfg_t *pCfg;
  mabLogicalPortInfo_t *logicalPortInfo;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  nimGetIntfName(intIfNum,  ALIASNAME, ifName);

  MAB_EVENT_TRACE("%s:%d: source Mac: %02X:%02X:%02X:%02X:%02X:%02X  Interface:%d ",
      __FUNCTION__,__LINE__,
      ( uchar8)srcMac[0],
      ( uchar8)srcMac[1],
      ( uchar8)srcMac[2],
      ( uchar8)srcMac[3],
      ( uchar8)srcMac[4],
      ( uchar8)srcMac[5],
      intIfNum);

  *existing_node =  FALSE;

  /* Get the port mode */
  if ( TRUE != mabIntfIsConfigurable(intIfNum, &pCfg))
    return  FAILURE;

  if ( AUTHMGR_PORT_AUTO == mabBlock->mabPortInfo[intIfNum].portControlMode)
  {
    /* loop based on the intIfNum */
    lIndex = MAB_LOGICAL_PORT_ITERATE;
    while( NULLPTR != (logicalPortInfo=mabLogicalPortInfoGetNextNode(intIfNum,&lIndex)))
    {
      if (0 == memcmp(srcMac, logicalPortInfo->client.suppMacAddr.addr, MAC_ADDR_LEN))
      {
        *lIntIfNum = lIndex;
        *existing_node =  TRUE;
        return  SUCCESS;
      }
    }

    /* allocate a new logical port for this supplicant */
    logicalPortInfo = mabLogicalPortInfoAlloc(intIfNum);

    if(logicalPortInfo !=  NULLPTR)
    {
      /* add mac address to Mac Addr Database*/
      if (mabIssueCmd(mabAddMacInMacDB, logicalPortInfo->key.keyNum,  NULLPTR) !=  SUCCESS)
      {
         LOGF( LOG_SEVERITY_ERROR,"Failed to add MAC entry %02x:%02x:%02x:%02x:%02x:%02x"
            " in MAC database for interface %s (intIfNum %d, logical port %d). Reason: Failed to send event mabAddMacInMacDB\n",
            srcMac[0],srcMac[1],srcMac[2],srcMac[3],srcMac[4],srcMac[5],
            ifName,intIfNum,logicalPortInfo->key.keyNum);
        mabLogicalPortInfoDeAlloc(logicalPortInfo);
        return  FAILURE;
      }

      mabLogicalPortInfoInit(logicalPortInfo->key.keyNum);

      memcpy(logicalPortInfo->client.suppMacAddr.addr, srcMac,  MAC_ADDR_LEN);
      *existing_node =  FALSE;
      *lIntIfNum = logicalPortInfo->key.keyNum;
      mabBlock->mabPortInfo[intIfNum].numUsers++;

      return  SUCCESS;
    }
  }
    return  FAILURE;
}

/*********************************************************************
 * @purpose  Check if pdu is to be processed considering logical port
 *           use and availability
 *
 * @param    intIfNum     @b{(input)} physical interface
 * @param    *srcMac      @b{(input)} supplicant mac address
 * @param    logicalPort  @b{(input)} logical port
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments Command is queued for service
 *
 * @end
 *********************************************************************/
RC_t mabCheckMapPdu(uint32 intIfNum,  char8 *srcMac, uint32 *lIntIfNum,  BOOL *existingNode)
{
  mabPortCfg_t *pCfg;

  MAB_EVENT_TRACE("%s:%d: source Mac: %02X:%02X:%02X:%02X:%02X:%02X  Interface:%d ",
      __FUNCTION__,__LINE__,
      ( uchar8)srcMac[0],
      ( uchar8)srcMac[1],
      ( uchar8)srcMac[2],
      ( uchar8)srcMac[3],
      ( uchar8)srcMac[4],
      ( uchar8)srcMac[5],
      intIfNum);

  *existingNode =  FALSE;

  /* Get the port mode */
  if ( TRUE != mabIntfIsConfigurable(intIfNum, &pCfg))
    return  FAILURE;

  /* logical nodes are dynamically allocated */
  return mabDynamicUserPduMapCheck(intIfNum, srcMac, lIntIfNum, existingNode);
}

/*********************************************************************
 * @purpose  This routine decrements all the timer counters for all ports
 *
 * @param    none
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments The events generated are directly sent to state machine classifier.
 *
 * @end
 *********************************************************************/
RC_t mabTimerAction()
{
  if (!MAB_IS_READY)
  {
    return  SUCCESS;
  }

  appTimerProcess(mabBlock->mabTimerCB);

  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Generate Response to Radius Challenge
 *
 * @param    lIntIfNum   @b{(input)) logical interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabCtlLogicalPortMABGenRequest(uint32 lIntIfNum, netBufHandle bufHandle)
{
   uchar8 *data;
   enetHeader_t *enetHdr;
   enet_encaps_t *encap;
   eapolPacket_t *eapolPkt;
   authmgrEapPacket_t *eapPkt;
   eapRrPacket_t *eapRrPkt;
  uint32 length=0;
  mabLogicalPortInfo_t *logicalPortInfo;
   uchar8 *userData;
  uint32 physPort = 0;

  MAB_EVENT_TRACE(
      "%s:%d:  In mabCtlLogicalPortMABGeneratePDU intf %d\n",
      __FUNCTION__,__LINE__,lIntIfNum);

  if((logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum))== NULLPTR)
  {
    MAB_EVENT_TRACE(
        "%s:%d:  Could not get logical Interface structure for %d \n",
        __FUNCTION__,__LINE__,lIntIfNum);
    return  FAILURE;
  }

  if (bufHandle ==  NULL)
  {
    return  FAILURE;
  }

  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  MAB_EVENT_TRACE(
      "%s:%d:  Generating packet for interface[%d]  \n",
      __FUNCTION__,__LINE__,lIntIfNum);

  SYSAPI_NET_MBUF_GET_DATASTART(bufHandle, data);
  /* Set ethernet header */
  enetHdr = ( enetHeader_t *)(data);

  /* Set dest and source MAC in ethernet header */
  memcpy(enetHdr->dest.addr,  EAPOL_PDU_MAC_ADDR.addr,  MAC_ADDR_LEN);
  memcpy(enetHdr->src.addr,logicalPortInfo->client.suppMacAddr.addr,  MAC_ADDR_LEN);

  /* Set ethernet type */
  encap = ( enet_encaps_t *)(( uchar8 *)enetHdr +  ENET_HDR_SIZE);
  encap->type = osapiHtons( ETYPE_EAPOL);

  /* Set EAPOL header */
  eapolPkt = ( eapolPacket_t *)(( uchar8 *)encap +  ENET_ENCAPS_HDR_SIZE);
  eapolPkt->protocolVersion =  MAB_PAE_PORT_PROTOCOL_VERSION_1;
  eapolPkt->packetType = EAPOL_EAPPKT;
  /*eapolPkt->packetBodyLength = osapiHtons( ( ushort16)(sizeof( authmgrEapPacket_t) + sizeof( eapRrPacket_t)) );*/

  /* Set EAP header */
  eapPkt = ( authmgrEapPacket_t *)(( uchar8 *)eapolPkt + sizeof( eapolPacket_t));
  eapPkt->code = EAP_RESPONSE;
  eapPkt->id = logicalPortInfo->client.currentIdL;

  /* Set EAP Request/Response header */
  eapRrPkt = ( eapRrPacket_t *)(( uchar8 *)eapPkt + sizeof( authmgrEapPacket_t));

  eapRrPkt->type = EAP_RRIDENTITY;
  userData = (( uchar8 *)eapRrPkt) + sizeof( eapRrPacket_t);

  eapolPkt->packetBodyLength = ( ( ushort16)(sizeof( authmgrEapPacket_t) + sizeof( eapRrPacket_t)+
        strlen(logicalPortInfo->client.mabUserName)) );
  eapPkt->length = eapolPkt->packetBodyLength;

  memset (userData,'\0', strlen(logicalPortInfo->client.mabUserName) +1);
  memcpy (userData, logicalPortInfo->client.mabUserName, strlen(logicalPortInfo->client.mabUserName));

  length = (uint32)(  ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE + sizeof( eapolPacket_t) +
      sizeof( authmgrEapPacket_t) + sizeof( eapRrPacket_t) + strlen(logicalPortInfo->client.mabUserName) );

  SYSAPI_NET_MBUF_SET_DATALENGTH(bufHandle, length);

  return  SUCCESS;
}

/*********************************************************************
 * @purpose Enable MAB operationally on a physical port
 *
 * @param    intIfNum   @b{(input)) internal interface number
 * @param
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabCtlPortMABEnableSet(uint32 intIfNum)
{
  mabPortCfg_t *pCfg;

  /* Get the port mode */
  if ( TRUE != mabIntfIsConfigurable(intIfNum, &pCfg))
  {
    return  FAILURE;
  }

  /* Register for time ticks with appTimer */
  if ( NULLPTR == mabBlock->mabTimerCB)
  {
    mabBlock->mabTimerCB =  appTimerInit( MAB_COMPONENT_ID, mabTimerExpiryHdlr,
         NULLPTR,  APP_TMR_1SEC, mabBlock->mabAppTimerBufferPoolId);
  }

  (void)mabCtlApplyPortConfigData(intIfNum);

  return  SUCCESS;
}

/*********************************************************************
 * @purpose Disable MAB on a physical port
 *
 * @param    intIfNum   @b{(input)) internal interface number
 * @param
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabCtlPortMABDisableSet(uint32 intIfNum)
{
  mabPortCfg_t *pCfg;

  /* Get the port mode */
  if ( TRUE != mabIntfIsConfigurable(intIfNum, &pCfg))
  {
    return  FAILURE;
  }

  (void)mabPortInfoCleanup(intIfNum);

  (void)mabCtlApplyPortConfigData(intIfNum);

  mabAppTimerDeInitCheck();

  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Set initialize value for a port
 *
 * @param    intIfNum   @b{(input)) internal interface number
 * @param    initialize @b{(input)) initialize value
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments This value is set to  TRUE by management in order to force
 *           initialization of a port.  It is re-set to  FALSE after
 *           initialization has completed.
 *
 * @end
 *********************************************************************/
RC_t mabCtlPortInitializeSet(uint32 intIfNum,  BOOL initialize)
{
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  
  nimGetIntfName(intIfNum,  ALIASNAME, ifName);
  MAB_EVENT_TRACE("%s:Intialize physical port-%s \n",
      __FUNCTION__,ifName);

  mabPortInfoCleanup(intIfNum);
  return mabCtlApplyPortConfigData(intIfNum);
}

/*********************************************************************
 * @purpose  Clear mab stats for specified port
 *
 * @param    intIfNum     @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabCtlPortStatsClear(uint32 intIfNum)
{
  memset(( char8 *)&mabBlock->mabPortStats[intIfNum],  NULL, sizeof(mabPortStats_t));
  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Apply mab config data to specified interface
 *
 * @param    intIfNum     @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabCtlApplyPortConfigData(uint32 intIfNum)
{
  mabPortCfg_t *pCfg;
  authmgrClientStatusInfo_t clientStatus;
   uchar8 ifNamel[ NIM_IF_ALIAS_SIZE + 1];

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  (void)mabPortInfoInitialize(intIfNum,  TRUE);

  /* mab is not enabled */
  if (pCfg->mabEnabled ==  FALSE )
  {
    /* do nothing */
  }
  else
  {
    /* set the port control mode and host control mode */
    mabPortCtrlModeSet(intIfNum, mabBlock->mabPortInfo[intIfNum].portControlMode);
  }

  if ( ENABLE == mabBlock->mabPortInfo[intIfNum].portEnabled)
  {
    memset(&clientStatus, 0, sizeof(authmgrClientStatusInfo_t));
    clientStatus.info.enableStatus = mabBlock->mabPortInfo[intIfNum].mabEnabled;

    nimGetIntfName (intIfNum,  ALIASNAME, ifNamel);

    MAB_EVENT_TRACE(
    "%s %d sending method change %d to authmgr for interface %s", __func__, __LINE__, mabBlock->mabPortInfo[intIfNum].mabEnabled, ifNamel);

    mabPortClientAuthStatusUpdate(intIfNum,  NULLPTR, "method_change", (void*) &clientStatus);
  }

  return  SUCCESS;
}

void mabVlanChangeProcess(uint32 event, uint32 intIfNum, dot1qNotifyData_t *vlanData)
{
  uint32      i = 0, vlanId = 0, numVlans = 0;
  RC_t rc;

  for (i = 1; i<= VLAN_MAX_MASK_BIT; i++)
  {
    {
      vlanId = vlanData->data.vlanId;
      /* For any continue, we will break out */
      i =  VLAN_MAX_MASK_BIT + 1;
    }

    switch (event)
    {
      case mabVlanDeleteEvent:
        mabVlanDeleteProcess(vlanId);
        break;

      case mabVlanDeletePortEvent:
        mabVlanPortDeleteProcess(intIfNum,vlanId);
        break;

      case mabVlanAddEvent:
        break;

      case mabVlanAddPortEvent:
        rc = mabVlanPortAddProcess(intIfNum,vlanId);
        break;

      case mabVlanPvidChangeEvent:
        rc = mabVlanPVIDChangeEventProcess(intIfNum,vlanId);
        break;
    }
    MAB_EVENT_TRACE(
        "mabEvent %d port %d \n", event, intIfNum);
    numVlans++;
  }
  return;
}

/*********************************************************************
* @purpose  Set values of the Logical Dot1x Port Structure
*           with Default Values of port it belongs to
*
* @param    logicalPortInfo  @b{(input))  Logical port Info
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t mabLogicalPortInfoInit(uint32 lIntIfNum)
{
  mabLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0, lPort = 0, type = 0, vlanId = 0;

  logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);
  MAB_EVENT_TRACE(
                        "%s:Resetting information for linterface = %d . \n",
                        __FUNCTION__, lPort);

  if ( AUTHMGR_PORT_STATUS_AUTHORIZED == logicalPortInfo->client.logicalPortStatus)
  {
    /* Clean up the client hw info */
    MAB_EVENT_TRACE(
        "\n%s:Cleaning up hw info for linterface = %d\n",
        __FUNCTION__,lIntIfNum);
  }

  memset(&logicalPortInfo->client, 0, sizeof(mabClientInfo_t));
  /* FSM state Holders */
  logicalPortInfo->client.currentIdL         = mabBlock->mabPortInfo[physPort].currentId;

  logicalPortInfo->client.vlanId         = vlanId;
  logicalPortInfo->client.vlanType       = AUTHMGR_VLAN_DEFAULT;

  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);
  memset(&logicalPortInfo->protocol, 0, sizeof(mabProtocolInfo_t));

  mabUnAuthenticatedAction(logicalPortInfo);

  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Generate Response to Radius Challenge
 *
 * @param    lIntIfNum   @b{(input)) logical interface number
 * @param    generateNak @b{(input)} Bool value to generate NAK
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @notes    generateNak flag is used if MAB needs to responce with NAK 
 *           for unsupported EAP type
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabCtlLogicalPortMABGenResp(uint32 lIntIfNum,   BOOL generateNak)
{
   netBufHandle bufHandle;
   uchar8 *data;
   enetHeader_t *enetHdr;
   enet_encaps_t *encap;
   eapolPacket_t *eapolPkt;
   authmgrEapPacket_t *eapPkt;
   eapRrPacket_t *eapRrPkt;
  uint32 length=0;
  mabLogicalPortInfo_t *logicalPortInfo;
   uchar8  md5ChkSum[MAB_MD5_LEN+1], responseData[ PASSWORD_SIZE + MAB_CHALLENGE_LEN + 2];
  uint32  responseDataLen;
   uchar8  password[ PASSWORD_SIZE];
   uchar8 *userData;
  uint32 challengelen, physPort = 0;
  RC_t rc =  SUCCESS;

  MAB_EVENT_TRACE(
      "%s:%d:  In mabCtlLogicalPortMABGenResp intf %d  \n",
      __FUNCTION__,__LINE__,lIntIfNum);

  if((logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum))== NULLPTR)
  {
    MAB_EVENT_TRACE(
        "%s:%d:  Could not get logical Interface structure for %d \n",
        __FUNCTION__,__LINE__,lIntIfNum);
    return  FAILURE;
  }

  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  /*generate the EAPOL PDU*/
  SYSAPI_NET_MBUF_GET(bufHandle);
  if (bufHandle ==  NULL)
  {
     LOGF( LOG_SEVERITY_INFO,
        "Out of system buffers.\n");
    MAB_EVENT_TRACE(
        "%s:%d out of system buffers\n",
        __FUNCTION__,__LINE__);
    return  FAILURE;
  }

  MAB_EVENT_TRACE(
      "%s:%d:  Generating packet for interface[%d]  \n",
      __FUNCTION__,__LINE__,lIntIfNum);

  SYSAPI_NET_MBUF_GET_DATASTART(bufHandle, data);
  /* Set ethernet header */
  enetHdr = ( enetHeader_t *)(data);

  /* Set dest and source MAC in ethernet header */
  memcpy(enetHdr->dest.addr,  EAPOL_PDU_MAC_ADDR.addr,  MAC_ADDR_LEN);
  memcpy(enetHdr->src.addr,logicalPortInfo->client.suppMacAddr.addr,  MAC_ADDR_LEN);

  /* Set ethernet type */
  encap = ( enet_encaps_t *)(( uchar8 *)enetHdr +  ENET_HDR_SIZE);
  encap->type = osapiHtons( ETYPE_EAPOL);

  /* Set EAPOL header */
  eapolPkt = ( eapolPacket_t *)(( uchar8 *)encap +  ENET_ENCAPS_HDR_SIZE);
  eapolPkt->protocolVersion =  MAB_PAE_PORT_PROTOCOL_VERSION_1;
  eapolPkt->packetType = EAPOL_EAPPKT;
  eapolPkt->packetBodyLength = osapiHtons( ( ushort16)(sizeof( authmgrEapPacket_t) + sizeof( eapRrPacket_t)) );

  /* Set EAP header */
  eapPkt = ( authmgrEapPacket_t *)(( uchar8 *)eapolPkt + sizeof( eapolPacket_t));
  eapPkt->code = EAP_RESPONSE;
  eapPkt->id = logicalPortInfo->client.currentIdL;

  /* Set EAP Request/Response header */
  eapRrPkt = ( eapRrPacket_t *)(( uchar8 *)eapPkt + sizeof( authmgrEapPacket_t));

  if (generateNak ==  FALSE)
  {
    eapolPkt->packetBodyLength = (( ushort16)(sizeof( authmgrEapPacket_t) +
          sizeof( eapRrPacket_t)+MAB_MD5_LEN+1));
  }
  else 
  {
    eapolPkt->packetBodyLength = (( ushort16)(sizeof( authmgrEapPacket_t) +
          sizeof( eapRrPacket_t) + 1));
  }

  eapPkt->length =  eapolPkt->packetBodyLength ;
  if (generateNak ==  FALSE)
  {
    eapRrPkt->type = EAP_RRMD5;
  }
  else 
  {
    eapRrPkt->type = EAP_RRNAK;
  }
  userData = (( uchar8 *)eapRrPkt) + sizeof( eapRrPacket_t);

  if (generateNak ==  FALSE)
  {
    /* generate password */
    memset(password,0,sizeof(password));
    memcpy(password, logicalPortInfo->client.mabUserName, sizeof(password));

    /*generate challenge*/
    challengelen= logicalPortInfo->client.mabChallengelen;

    responseDataLen = 1 + strlen(( char8 *)password) + challengelen;


    memset(responseData,0,sizeof(responseData));
   

    responseData[0] = logicalPortInfo->client.currentIdL;

    memcpy(&responseData[1],password, strlen(( char8 *)password));
    memcpy(&responseData[1+strlen(( char8 *)password)],logicalPortInfo->client.mabChallenge,challengelen);

    mabLocalMd5Calc(responseData, responseDataLen, md5ChkSum);

    *userData = MAB_MD5_LEN;
    userData++;
    memcpy (userData, md5ChkSum,MAB_MD5_LEN);
    length = (uint32)(  ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE + sizeof( eapolPacket_t) +
        sizeof( authmgrEapPacket_t) + sizeof( eapRrPacket_t) +MAB_MD5_LEN+1);
  }
  else
  {
    *userData = EAP_RRMD5;
    length = (uint32)(  ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE + sizeof( eapolPacket_t) +
        sizeof( authmgrEapPacket_t) + sizeof( eapRrPacket_t) + 1);

  }

  SYSAPI_NET_MBUF_SET_DATALENGTH(bufHandle, length);

  MAB_EVENT_TRACE("%s:%d:  Generated PDU :%s \n",
      __FUNCTION__,__LINE__,data);

  rc = mabClientResponseAction(logicalPortInfo, bufHandle);

  if ( NULL != bufHandle)
  {
    SYSAPI_NET_MBUF_FREE(bufHandle);
  }

  return rc;
}

/*********************************************************************
 * @purpose  Processes Dot1x-related event initiated by Dot1Q
 *
 * @param (in)    vlanId    Virtual LAN Id
 * @param (in)    intIfNum  Interface Number
 * @param (in)    event
 *
 * @returns   SUCCESS  or  FAILURE
 *
 * @end
 *********************************************************************/
RC_t mabVlanChangeCallback(dot1qNotifyData_t *vlanData, uint32 intIfNum, uint32 event)
{
  /* Vlan Change callbacks can be called during unconfig phase when dot1q is trying
     to restore the vlan config. */
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
   INTF_TYPES_t intfType;

  memset(ifName, 0x00, sizeof(ifName));

  if (!MAB_IS_READY)
  {
    LOGF(LOG_SEVERITY_INFO,
         "Received an VLAN change callback while MAB is not ready to receive it.");
    return  FAILURE;
  }

  /* before performing any operations with interfaces,
     check if NIM is ready to handle requests */
  if ((nimPhaseStatusCheck() ==  TRUE) && (nimGetIntfType(intIfNum, &intfType) ==  SUCCESS))
  {
    if (mabIsValidIntfType(intfType) !=  TRUE)
    {
      /* if MAB is not interested in this interface,
       * inform event issuer that we have completed processing.
       */
      nimGetIntfName(intIfNum,  ALIASNAME, ifName);
      MAB_EVENT_TRACE("Interface %s is not MAB configurable\r\n", ifName);
      return  SUCCESS;
    }
  }

  MAB_EVENT_TRACE(
      "mabVlanChangeCallback:Received Vlan event %d for interface %d \n",
      event,intIfNum);

  if ((intIfNum!=0) && (mabBlock->mabPortInfo[intIfNum].portEnabled !=  TRUE) && (event != VLAN_DELETE_PORT_NOTIFY))
  {
    return  SUCCESS;
  }

  switch (event)
  {
    case VLAN_DELETE_PENDING_NOTIFY:
      MAB_EVENT_TRACE(
          "Received Vlan Delete Notify \n");
      mabIssueCmd(mabVlanDeleteEvent,intIfNum,vlanData);
      break;

    case VLAN_ADD_NOTIFY:
      MAB_EVENT_TRACE(
          "Received Vlan Add Notify \n");
      mabIssueCmd(mabVlanAddEvent,intIfNum,vlanData);
      break;

    case VLAN_ADD_PORT_NOTIFY:
      {
        MAB_EVENT_TRACE(
            "Received Vlan Add Port Notify for Port %d \n",
            intIfNum);
        mabIssueCmd(mabVlanAddPortEvent,intIfNum,vlanData);
      }
      break;

    case VLAN_DELETE_PORT_NOTIFY:
      MAB_EVENT_TRACE(
          "Received Vlan Delete Port Notify for Port %d\n",
          intIfNum);
      mabIssueCmd(mabVlanDeletePortEvent,intIfNum,vlanData);
      break;

    case VLAN_PVID_CHANGE_NOTIFY:
      {
        MAB_EVENT_TRACE(
            "Received Vlan PVID Change Notify for Port %d \n",
            intIfNum);
        mabIssueCmd(mabVlanPvidChangeEvent,intIfNum,vlanData);
      }
    default:
      break;
  }
  return  SUCCESS;
}

RC_t mabHostModeMapInfoGet( AUTHMGR_HOST_CONTROL_t type, mabHostModeMap_t *elem)
{
  uint32 i = 0;
  static mabHostModeMap_t mabHostModeHandlerTable[] =
  { 
    { AUTHMGR_SINGLE_AUTH_MODE, mabControlSingleAuthActionSet},
    { AUTHMGR_MULTI_HOST_MODE, mabControlMultiHostActionSet},
    { AUTHMGR_MULTI_AUTH_MODE, mabControlMultAuthActionSet},
  };


  for (i = 0; i < (sizeof (mabHostModeHandlerTable)/sizeof(mabHostModeMap_t)); i++)
  {
    if (type == mabHostModeHandlerTable[i].hostMode)
    {
      *elem = mabHostModeHandlerTable[i];
      return  SUCCESS;
    }
  }
  return  FAILURE;
}


/*********************************************************************
 * @purpose control mode function to set the port control mode to auto 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabPortCtrlHostModeSet(uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t hostMode)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  mabHostModeMap_t entry;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  if (hostMode == mabBlock->mabPortInfo[intIfNum].hostMode)
  {
    return  SUCCESS;
  }

  mabPortInfoCleanup(intIfNum);
  mabPortInfoInitialize(intIfNum, TRUE);

  /* check the configured host mode
     and set the port accordingly */
  memset(&entry, 0, sizeof(mabHostModeMap_t));
  if ( SUCCESS != mabHostModeMapInfoGet(hostMode, &entry))
  {
    /* failed to get the handler for the host mode */
    return  FAILURE;
  }

  rc = entry.hostModeFn(intIfNum);

  return rc;
}

/*********************************************************************
 * @purpose control function to set the host mode to multi host 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabControlMultiHostActionSet(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  mabLogicalPortInfo_t *logicalPortInfo;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  /* Set the operating host mode */
  mabBlock->mabPortInfo[intIfNum].hostMode =  AUTHMGR_MULTI_HOST_MODE;

  if ( NULLPTR == (logicalPortInfo = mabLogicalPortInfoAlloc(intIfNum)))
  {
    MAB_EVENT_TRACE(
      "%s, %d, Failed to allocate logicalInterface \n", 
                    __func__, __LINE__);

    return  FAILURE;
  }

  mabLogicalPortInfoInit(logicalPortInfo->key.keyNum);

   MAB_EVENT_TRACE(
     "%s, %d, triggering event  mabAuthenticationStart for logicalInterface %d \n", __func__, __LINE__, logicalPortInfo->key.keyNum);

  return rc;
}


/*********************************************************************
 * @purpose control function to set the host mode to single host mode 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabControlSingleAuthActionSet(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  /* Set the operating host mode */
  mabBlock->mabPortInfo[intIfNum].hostMode =  AUTHMGR_SINGLE_AUTH_MODE;

  return rc;
}


/*********************************************************************
 * @purpose control function to set the host mode to multi auth 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabControlMultAuthActionSet(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  
  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  /* Set the operating host mode */
  mabBlock->mabPortInfo[intIfNum].hostMode =  AUTHMGR_MULTI_AUTH_MODE;

  return rc;
}

/*********************************************************************
 * @purpose function to clean up mab port oper info
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabPortInfoCleanup(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  uint32 lIntIfNum;
  mabPortCfg_t *pCfg;
  mabLogicalPortInfo_t *logicalPortInfo;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

   /* reset all the clients associated with the port */
   lIntIfNum = MAB_LOGICAL_PORT_ITERATE;
   while ( NULLPTR != (logicalPortInfo=mabLogicalPortInfoGetNextNode(intIfNum,&lIntIfNum)))
   {
     if (0 != logicalPortInfo->key.keyNum)
     {
       if( SUCCESS != mabClientSwInfoCleanup(logicalPortInfo))
       {
          MAB_EVENT_TRACE(
                         "%s:%d Failed to clean up mab info %d  \n",
                        __FUNCTION__, __LINE__, logicalPortInfo->key.keyNum);
       }
     }
   }

   return rc;
}

/*********************************************************************
 * @purpose  Set port control mode
 *
 * @param    intIfNum    @b{(input)) internal interface number
 * @param    portControl @b{(input)) port control mode
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabCtlPortControlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  if (portControl == mabBlock->mabPortInfo[intIfNum].portControlMode)
  {
    return rc;
  }

  /* clean up previous info */
  mabPortInfoCleanup(intIfNum);
  mabPortInfoInitialize(intIfNum, TRUE);

  return mabPortCtrlModeSet(intIfNum, portControl);
}

/*********************************************************************
 * @purpose  Set port control mode
 *
 * @param    intIfNum    @b{(input)) internal interface number
 * @param    portControl @b{(input)) port control mode
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabPortCtrlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  switch (portControl)
  {
    case  AUTHMGR_PORT_FORCE_UNAUTHORIZED:
      rc = mabPortControlForceUnAuthActionSet(intIfNum);
      mabBlock->mabPortInfo[intIfNum].portControlMode = portControl;
      break;

    case  AUTHMGR_PORT_FORCE_AUTHORIZED:
      rc = mabPortControlForceAuthActionSet(intIfNum);
      mabBlock->mabPortInfo[intIfNum].portControlMode = portControl;
      break;

    case  AUTHMGR_PORT_AUTO:
      rc = mabPortControlAutoActionSet(intIfNum);
      mabBlock->mabPortInfo[intIfNum].portControlMode = portControl;
      break;

    default:
      rc =  FAILURE;
  }

  return rc;
}

/*********************************************************************
 * @purpose control function to set the to force un-authorized 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabPortControlForceUnAuthActionSet(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  mabLogicalPortInfo_t *logicalPortInfo;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  /* Set the operating host mode */
  mabBlock->mabPortInfo[intIfNum].hostMode =  AUTHMGR_INVALID_HOST_MODE;

  if ( NULLPTR == (logicalPortInfo = mabLogicalPortInfoAlloc(intIfNum)))
  {
    MAB_EVENT_TRACE(
                    "%s:%d:  Unable to allocate logical port \n",
                    __FUNCTION__,__LINE__);
    return  FAILURE;
  }

  /* Call the api to set the port to authorized */
  mabClientStatusSet(logicalPortInfo,  AUTHMGR_PORT_STATUS_UNAUTHORIZED);

  return rc;
}

/*********************************************************************
 * @purpose control function to set the to force authorized 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabPortControlForceAuthActionSet(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  mabLogicalPortInfo_t *logicalPortInfo;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  /* Set the operating host mode */
  mabBlock->mabPortInfo[intIfNum].hostMode =  AUTHMGR_INVALID_HOST_MODE;

  if ( NULLPTR == (logicalPortInfo = mabLogicalPortInfoAlloc(intIfNum)))
  {
    MAB_EVENT_TRACE(
                    "%s:%d:  Unable to allocate logical port \n",
                    __FUNCTION__,__LINE__);
    return  FAILURE;
  }

  /* Call the api to set the port to authorized */
  mabClientStatusSet(logicalPortInfo,  AUTHMGR_PORT_STATUS_AUTHORIZED);

  return rc;
}

/*********************************************************************
 * @purpose control mode function to set the port control mode to auto 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabPortControlAutoActionSet(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  mabHostModeMap_t entry;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  /* check the configured host mode
     and set the port accordingly */

  memset(&entry, 0, sizeof(mabHostModeMap_t));
  if ( SUCCESS != mabHostModeMapInfoGet(mabBlock->mabPortInfo[intIfNum].hostMode, &entry))
  {
    /* failed to get the handler for the host mode */
    return  FAILURE;
  }

  rc = entry.hostModeFn(intIfNum);

  return rc;
}


/*********************************************************************
* @purpose  Handle Auth Manager event
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    event       @b{(input)} event
* @param    macAddr     @b{(input)} client mac address
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t mabClientEventUpdate(uint32 intIfNum, uint32 event,  enetMacAddr_t *macAddr)
{
  mabPortCfg_t *pCfg;
  mabAuthmgrMsg_t authmgrMsg;

  if (mabIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  memset(&authmgrMsg, 0, sizeof(mabAuthmgrMsg_t));

  memcpy(&authmgrMsg.clientMacAddr, macAddr,  ENET_MAC_ADDR_LEN);
  authmgrMsg.event = event; 

  return mabIssueCmd(mabAuthMgrEvent, intIfNum, &authmgrMsg);
}

/*********************************************************************
 * @purpose  Apply mab config data
 *
 * @param    void
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabCtlApplyConfigData(void)
{
  uint32 intIfNum;
  RC_t nimRc;
  mabPortCfg_t *pCfg;

  nimRc = mabFirstValidIntfNumber(&intIfNum);
  while (nimRc ==  SUCCESS)
  {
    if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
      return  FAILURE;

    (void)mabCtlApplyPortConfigData(intIfNum);
    nimRc = mabNextValidIntf(intIfNum, &intIfNum);
  }

  return  SUCCESS;
}

RC_t mabAuthmgrEventProcess(uint32 intIfNum, mabAuthmgrMsg_t *authmgrParams)
{
  mabAuthmgrEventFnMap_t entry;
  RC_t rc =  SUCCESS;

  memset(&entry, 0, sizeof(mabAuthmgrEventFnMap_t));

  MAB_IF_NULLPTR_RETURN_LOG(authmgrParams);

  if ( SUCCESS != mabAuthmgrEventMapFnGet(authmgrParams->event, &entry))
  {
    return  FAILURE;
  }

  if ( NULLPTR != entry.eventMapFn)
  {
    rc = entry.eventMapFn(intIfNum, authmgrParams->clientMacAddr);
  }

  return rc;
}

RC_t mabAuthenticationInitiate(uint32 intIfNum,  enetMacAddr_t suppMacAddr)
{
  mabLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0;
  uint32 lIntIfNum = 0;
  uint32 temp = 0;
   BOOL exists =  FALSE;

  /* search the Mac address in the list of clients on the port */
  if (mabCheckMapPdu(intIfNum, suppMacAddr.addr, &lIntIfNum, &exists)!= SUCCESS)
  {
    MAB_EVENT_TRACE(
        "Failed to Initiate Authentication \n");
    return  SUCCESS;
  }

  logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);

  if ( NULLPTR == logicalPortInfo)
  {
    MAB_EVENT_TRACE(
                    "%s:%d:  Unable to find logical port \n",
                    __FUNCTION__,__LINE__);
    return  FAILURE;
  }

  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  MAB_EVENT_TRACE(
    "%s: receoved event to start authentication for logicalInterface %d \n",
    __func__, logicalPortInfo->key.keyNum);

  if( (mabBlock->mabPortInfo[intIfNum].portControlMode ==  AUTHMGR_PORT_FORCE_UNAUTHORIZED) ||
      (mabBlock->mabPortInfo[intIfNum].portControlMode ==  AUTHMGR_PORT_FORCE_AUTHORIZED))
  {
    return  SUCCESS;
  }
  else if (mabBlock->mabPortInfo[intIfNum].portControlMode ==  AUTHMGR_PORT_AUTO)
  {
    temp = MAB_LOGICAL_PORT_ITERATE;
    if ( SUCCESS == mabMacAddrInfoFind(&logicalPortInfo->client.suppMacAddr,&temp))
    {
      if (temp != lIntIfNum)
      {
        MAB_EVENT_TRACE(
            "%s, %d, Received client is already present on logical Interface %d.\n"
            "Not triggering mabAuthenticationStart for logicalInterface %d \n", __func__, __LINE__, temp, lIntIfNum);
        temp = MAB_LOGICAL_PORT_ITERATE;
      }
    }
    else
    {
      (void)mabMacAddrInfoAdd(&(logicalPortInfo->client.suppMacAddr),logicalPortInfo->key.keyNum);
    }
  }
  else
  {
    return  FAILURE;
  }

  return mabAuthenticatingAction(logicalPortInfo);

}

RC_t mabCtrlClientReAuthenticate(uint32 intIfNum,  enetMacAddr_t suppMacAddr)
{
  uint32 physPort;
  mabLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 lIntIfNum = 0;

  if ( SUCCESS != mabMacAddrInfoFind(&suppMacAddr, &lIntIfNum))
  {
    MAB_EVENT_TRACE(
                    "%s:%d:  Unable to client mac in db \n",
                    __FUNCTION__,__LINE__);
    return  FAILURE;
  }

  logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  /* generate the event to trigger the req id tx */
  MAB_EVENT_TRACE(
      "%s, %d, triggering event  to reauthenticate logicalInterface %d \n",
      __func__, __LINE__, lIntIfNum);

  logicalPortInfo->client.reAuthenticate =  TRUE;

  mabUnAuthenticatedAction(logicalPortInfo);
  /* This api is called in mabAuthenticationInitiate when authetication action is triggered,
     hence commenting duplicate call.
     mabAuthenticatingAction(logicalPortInfo);
  */
  return  SUCCESS;
}

RC_t mabClientInfoPurge(uint32 intIfNum,  enetMacAddr_t suppMacAddr)
{
  uint32 physPort;
  mabLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 lIntIfNum = 0;

  (void)mabRadiusClearRadiusMsgsSend(suppMacAddr);
  if ( SUCCESS != mabMacAddrInfoFind(&suppMacAddr,&lIntIfNum))
  {
    MAB_EVENT_TRACE(
                    "%s:%d:  Unable to find client mac in db \n",
                    __FUNCTION__,__LINE__);
    return  FAILURE;
  }

  logicalPortInfo = mabLogicalPortInfoGet(lIntIfNum);

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  /* generate the event to trigger the req id tx */
  MAB_EVENT_TRACE(
      "%s, %d, triggering event  to cleanup logicalInterface %d \n",
      __func__, __LINE__, lIntIfNum);

  mabClientInfoCleanup(logicalPortInfo);
  return  SUCCESS;
}


RC_t mabAuthmgrEventMapFnGet(uint32 event, mabAuthmgrEventFnMap_t *elem)
{
  uint32 i = 0;
  static mabAuthmgrEventFnMap_t mabAuthmgrEventFnMap[] =
  { 
    {authmgrClientReAuthenticate, mabCtrlClientReAuthenticate},
    {authmgrClientAuthStart, mabAuthenticationInitiate},
    {authmgrClientDisconnect, mabClientInfoPurge}
  };


  for (i = 0; i < (sizeof (mabAuthmgrEventFnMap)/sizeof(mabAuthmgrEventFnMap_t)); i++)
  {
    if (event == mabAuthmgrEventFnMap[i].event)
    {
      *elem = mabAuthmgrEventFnMap[i];
      return  SUCCESS;
    }
  }
  return  FAILURE;
}



/*********************************************************************
 * @purpose  Actions to be performed in the APM state AUTHENTICATING
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabAuthenticatingAction(mabLogicalPortInfo_t *logicalPortInfo)
{
  uint32 physPort = 0;
   uchar8 username[MAB_USER_NAME_LEN];
  uint32 usernamelen=0;
   netBufHandle bufHandle= NULL;
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg =  NULLPTR;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);
  
  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  if ((mabIntfIsConfigurable(physPort, &pCfg) !=  TRUE) ||
      ( NULLPTR == pCfg))
  {
    return  FAILURE;
  }

  mabBlock->mabPortStats[physPort].authEntersAuthenticating++;

  logicalPortInfo->protocol.authSuccess =  FALSE;
  logicalPortInfo->protocol.authFail =  FALSE;

  logicalPortInfo->client.mabAuthType = pCfg->mabAuthType;

  /* Construct username form supplicant Mac address and store it*/
  usernamelen=MAB_USER_NAME_LEN;
  memset(username,0,MAB_USER_NAME_LEN);
  osapiSnprintf(username,usernamelen,"%02X%02X%02X%02X%02X%02X",
                 logicalPortInfo->client.suppMacAddr.addr[0],
                 logicalPortInfo->client.suppMacAddr.addr[1],
                 logicalPortInfo->client.suppMacAddr.addr[2],
                 logicalPortInfo->client.suppMacAddr.addr[3],
                 logicalPortInfo->client.suppMacAddr.addr[4],
                 logicalPortInfo->client.suppMacAddr.addr[5]);

  osapiStrncat(username,"\0", 1);
  usernamelen = osapiStrnlen(username, sizeof(username));
  MAB_EVENT_TRACE(
      "%s:%d:Username :%s Length:%d  \n",
      __FUNCTION__,__LINE__,username,usernamelen);

  memset(logicalPortInfo->client.mabUserName,0,MAB_USER_NAME_LEN);
  memcpy(logicalPortInfo->client.mabUserName, username, usernamelen);
  memset(&logicalPortInfo->client.attrInfo, 0, sizeof(logicalPortInfo->client.attrInfo));

  logicalPortInfo->client.mabUserNameLength = usernamelen;

  /*generate the EAPOL PDU*/
  SYSAPI_NET_MBUF_GET(bufHandle);
  if (bufHandle ==  NULL)
  {
     LOGF( LOG_SEVERITY_INFO,
        "Out of system buffers.\n");
    MAB_EVENT_TRACE(
                    "%s:%d out of system buffers\n",
                    __FUNCTION__,__LINE__);
    return  FAILURE;
  }

  mabCtlLogicalPortMABGenRequest(logicalPortInfo->key.keyNum, bufHandle);

  logicalPortInfo->protocol.mabAuthState = MAB_AUTHENTICATING;

  MAB_EVENT_TRACE(
    "logicalInterface %d moved to state %d\n", logicalPortInfo->key.keyNum,
    logicalPortInfo->protocol.mabAuthState);

  /* authenticate the client */
  rc = mabClientResponseAction(logicalPortInfo, bufHandle);

  if ( NULL != bufHandle)
  {
    SYSAPI_NET_MBUF_FREE(bufHandle);
  }
  return rc;
}

/*********************************************************************
 * @purpose  Actions to be performed in the APM state AUTHENTICATED
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabAuthenticatedAction(mabLogicalPortInfo_t *logicalPortInfo)
{
  uint32  physPort = 0;
  authmgrClientStatusInfo_t   clientStatus;

  memset(&clientStatus, 0, sizeof(authmgrClientStatusInfo_t));

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);

  mabBlock->mabPortStats[physPort].authAuthSuccessWhileAuthenticating++;

  /* call the api to add the client */
  if ( AUTHMGR_PORT_STATUS_AUTHORIZED != logicalPortInfo->client.logicalPortStatus)
  {
    mabClientStatusSet(logicalPortInfo,  AUTHMGR_PORT_STATUS_AUTHORIZED);
  }

  logicalPortInfo->protocol.mabAuthState = MAB_AUTHENTICATED;
  logicalPortInfo->protocol.authSuccess =  TRUE;

  memset(&clientStatus, 0, sizeof(authmgrClientStatusInfo_t));
  memcpy(&clientStatus.info.authInfo.macAddr, &logicalPortInfo->client.suppMacAddr,  ENET_MAC_ADDR_LEN);
  memcpy(&clientStatus.info.authInfo.attrInfo,  &logicalPortInfo->client.attrInfo, sizeof(authmgrAuthAttributeInfo_t));

  clientStatus.info.authInfo.authMethod = logicalPortInfo->client.authMethod;

  osapiStrncpySafe(clientStatus.info.authInfo.authmgrUserName, logicalPortInfo->client.mabUserName, 
                   strlen(logicalPortInfo->client.mabUserName)+1);
  clientStatus.info.authInfo.authmgrUserNameLength = strlen(logicalPortInfo->client.mabUserName)+1;

  mabPortClientAuthStatusUpdate(physPort, logicalPortInfo->client.suppMacAddr.addr, "auth_success", (void*) &clientStatus);

  memset(&logicalPortInfo->client.attrInfo, 0, sizeof(logicalPortInfo->client.attrInfo));

  MAB_EVENT_TRACE(
    "logicalInterface %d moved to state %d\n", logicalPortInfo->key.keyNum,
    logicalPortInfo->protocol.mabAuthState);
  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Actions to be performed in the APM state DISCONNECTED
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabUnAuthenticatedAction(mabLogicalPortInfo_t *logicalPortInfo)
{
  uint32 physPort = 0;
  authmgrClientStatusInfo_t clientStatus;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_PORT_GET(physPort, logicalPortInfo->key.keyNum);


    if ( TRUE == logicalPortInfo->protocol.authFail)
    {
      memset(&clientStatus, 0, sizeof(authmgrClientStatusInfo_t));
      memcpy(&clientStatus.info.authInfo.macAddr, &logicalPortInfo->client.suppMacAddr,  ENET_MAC_ADDR_LEN);

      osapiStrncpySafe(clientStatus.info.authInfo.authmgrUserName, logicalPortInfo->client.mabUserName,
                       strlen(logicalPortInfo->client.mabUserName)+1);
      clientStatus.info.authInfo.authmgrUserNameLength = strlen(logicalPortInfo->client.mabUserName)+1;

      MAB_EVENT_TRACE(
          "MAB Auth failure for logicalInterface %d\n", 
                      logicalPortInfo->key.keyNum);

      mabPortClientAuthStatusUpdate(physPort, logicalPortInfo->client.suppMacAddr.addr, 
                                    "auth_fail", (void*) &clientStatus);
    }

  memset(&logicalPortInfo->client.attrInfo, 0, sizeof(logicalPortInfo->client.attrInfo));

  if (!(( TRUE == logicalPortInfo->client.reAuthenticate) &&
        ( AUTHMGR_PORT_STATUS_AUTHORIZED == logicalPortInfo->client.logicalPortStatus)))
  {
    
    mabClientStatusSet(logicalPortInfo,  AUTHMGR_PORT_STATUS_UNAUTHORIZED);


    if ( SUCCESS == mabClientDisconnectAction(logicalPortInfo))
    {
      return  SUCCESS;
    } 
  }

  logicalPortInfo->protocol.mabAuthState = MAB_UNAUTHENTICATED;

  MAB_EVENT_TRACE(
      "logicalInterface %d moved to state %d\n", logicalPortInfo->key.keyNum,
      logicalPortInfo->protocol.mabAuthState);
  return  SUCCESS;
}

/*********************************************************************
 * @purpose API to check and clear appTimer Deinit 
 *
 * @param   none 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabAppTimerDeInitCheck(void)
{
  RC_t nimRc =  SUCCESS;
  uint32 phyIntf = 0;

  nimRc = mabFirstValidIntfNumber(&phyIntf);
  while (nimRc ==  SUCCESS)
  {
    /* check if mab still enabled */
    if ( ENABLE == mabBlock->mabPortInfo[phyIntf].mabEnabled)
    {
      return  SUCCESS;
    }
    nimRc = mabNextValidIntf(phyIntf, &phyIntf);
  }

  /* No interface is enabled with MAB.stop the timer */
  if ( NULLPTR != mabBlock->mabTimerCB)
  {
    (void)appTimerDeInit(mabBlock->mabTimerCB);
    mabBlock->mabTimerCB =  NULLPTR;
  }

  return  SUCCESS;
}

RC_t mabRadiusChangeHandle(mabRadiusServer_t *info)
{
  mab_radius_cmd_msg_t req;
  mabIpAaddr_t nas_ip;

  if ( NULLPTR == info)
    return  FAILURE;

    MAB_EVENT_TRACE(
        "%s:Processing cfg update for server %s cmd %d",
        __FUNCTION__, info->cmd_data.server.serv_addr, info->cmd);

  memset(&req, 0, sizeof(req));
 
  switch(info->cmd)
  {
    case RADIUS_MAB_SERVER_ADD:
    case RADIUS_MAB_SERVER_MODIFY:
      if (RADIUS_MAB_SERVER_MODIFY == info->cmd)
      {
        osapiStrncpySafe(req.cmd, "server-delete", strlen("server-delete")+1);
        req.data = mabBlock->rad_cxt;
        radius_mab_cmd_req_send(mabBlock->send_fd, (char *)&req, sizeof(req));
      }

      memset(&req, 0, sizeof(req));

      osapiStrncpySafe(req.cmd, "server-add", strlen("server-add")+1);
      req.data = mabBlock->rad_cxt;
      memcpy(&req.cmd_data.server, &info->cmd_data.server, sizeof(mab_radius_server_t));
      radius_mab_cmd_req_send(mabBlock->send_fd,(char *)&req, sizeof(req)); 
      break;

    case RADIUS_MAB_SERVER_DELETE:
      osapiStrncpySafe(req.cmd, "server-delete", strlen("server-delete")+1);
      req.data = mabBlock->rad_cxt;
      memcpy(&req.cmd_data.server, &info->cmd_data.server, sizeof(mab_radius_server_t));
      radius_mab_cmd_req_send(mabBlock->send_fd, (char *)&req, sizeof(req)); 
      break;

    case RADIUS_MAB_GLOBAL_CFG:
      memset(&nas_ip, 0, sizeof(nas_ip));

      if (inet_pton(AF_INET6, info->cmd_data.globalCfg.nas_ip, &nas_ip.u.v6) <= 0) 
      {
        if (inet_pton(AF_INET, info->cmd_data.globalCfg.nas_ip, &nas_ip.u.v4) <= 0) 
        {
          MAB_EVENT_TRACE(
              "%s:Invalid nas ip %s",
              __FUNCTION__, info->cmd_data.globalCfg.nas_ip);
        }
        else
        {
          nas_ip.af = AF_INET;
        }
      }
      else
      {
        nas_ip.af = AF_INET6;
      }

      if (nas_ip.af)
      {
        memset(&mabBlock->nas_ip, 0, sizeof(mabBlock->nas_ip));
        memcpy(&mabBlock->nas_ip, &nas_ip, sizeof(mabBlock->nas_ip));
      }

      memcpy(mabBlock->nas_id, info->cmd_data.globalCfg.nas_id, min(sizeof(mabBlock->nas_id), sizeof(info->cmd_data.globalCfg.nas_id)));

      break;

    case RADIUS_MAB_SERVERS_RELOAD:
      osapiStrncpySafe(req.cmd, "server-reload", strlen("server-reload")+1);
      req.data = mabBlock->rad_cxt;
      radius_mab_cmd_req_send(mabBlock->send_fd, (char *)&req, sizeof(req)); 
      break;

    default:
      break;

  }

  return  SUCCESS;
}
