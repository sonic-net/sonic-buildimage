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


#include "mab_include.h"
#include "mab_client.h"
#include "mab_timer.h"
#include "mab_struct.h"

extern mabBlock_t *mabBlock;

/*********************************************************************
 * @purpose  Set the mab client authorization status
 *
 * @param    lIntIfNum   @b{(input)) internal interface number
 * @param    portStatus @b{(input)) port authorization status setting
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 * @returns   ERROR
 *
 * @comments none
 *
 * @end
 *********************************************************************/
RC_t mabClientStatusSet(mabLogicalPortInfo_t *logicalPortInfo,  AUTHMGR_PORT_STATUS_t portStatus)
{
  uint32 physPort = 0, lPort = 0, type = 0;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);

  MAB_EVENT_TRACE("%s:Setting the Logical port-%d to %s\n",
      __FUNCTION__,  logicalPortInfo->key.keyNum,
      (portStatus== AUTHMGR_PORT_STATUS_AUTHORIZED)?"Authorize":"Unauthorize");

  /* Verify port status parm value */
  if (portStatus !=  AUTHMGR_PORT_STATUS_AUTHORIZED && portStatus !=  AUTHMGR_PORT_STATUS_UNAUTHORIZED)
    return  FAILURE;

  /*If setting to the same value, just return success */
  if (portStatus == logicalPortInfo->client.logicalPortStatus)
  {
    MAB_EVENT_TRACE("%s:%d Status already set \n",__FUNCTION__,__LINE__);
    return  SUCCESS;
  }

  if ((( AUTHMGR_PORT_FORCE_UNAUTHORIZED == mabBlock->mabPortInfo[physPort].portControlMode) &&
        ( AUTHMGR_PORT_STATUS_AUTHORIZED == portStatus)) ||
      (( AUTHMGR_PORT_FORCE_AUTHORIZED == mabBlock->mabPortInfo[physPort].portControlMode) &&
       ( AUTHMGR_PORT_STATUS_UNAUTHORIZED == portStatus)))
  {
    /* this combination is not allowed.. So just a sanity check */
    return  FAILURE;
  }

  /* */

  logicalPortInfo->client.logicalPortStatus = portStatus;

  if ( AUTHMGR_PORT_STATUS_AUTHORIZED == portStatus)
  {
    /* set the port status to authorized */
    mabBlock->mabPortInfo[physPort].authCount++;
  }
  else
  {
    if (mabBlock->mabPortInfo[physPort].authCount > 0)
    {
      mabBlock->mabPortInfo[physPort].authCount--;
    }
  }
  return  SUCCESS;
}

/*********************************************************************
 * @purpose function to  cleanup the client sw info 
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
RC_t mabClientSwInfoCleanup(mabLogicalPortInfo_t *logicalPortInfo)
{
  RC_t rc =  SUCCESS, rc1 =  SUCCESS, rc2 =  SUCCESS, rc3 =  SUCCESS;
  mabPortCfg_t *pCfg;
  uint32 physPort = 0, lPort = 0, type = 0;
   enetMacAddr_t    nullMacAddr;

   memset(&nullMacAddr.addr,0, ENET_MAC_ADDR_LEN);

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);


  if (mabIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  SUCCESS;

  /* delete the timers if any */
  rc =  mabTimerDestroy(mabBlock->mabTimerCB, logicalPortInfo);

  /* set the client to un-authorized */

  rc1 = mabClientStatusSet(logicalPortInfo,  AUTHMGR_PORT_STATUS_UNAUTHORIZED);

  /*remove supplicant mac address from Mac address Database*/
   /*input check*/
  if (0 != memcmp(logicalPortInfo->client.suppMacAddr.addr, nullMacAddr.addr, ENET_MAC_ADDR_LEN))
  {
     rc2 = mabMacAddrInfoRemove(&(logicalPortInfo->client.suppMacAddr));
  }

  if(mabBlock->mabPortInfo[physPort].numUsers > 0)
  {
    mabBlock->mabPortInfo[physPort].numUsers--;
  }

  /* Deallocate memory for Clients */
  rc3 = mabLogicalPortInfoDeAlloc(logicalPortInfo);

  if (( SUCCESS != rc) ||
      ( SUCCESS != rc1) ||
      ( SUCCESS != rc2) ||
      ( SUCCESS != rc3))
  {
    MAB_EVENT_TRACE("%s:%d rc %d, rc1 %d rc2 %d, rc3 %d  \n",
                    __FUNCTION__,__LINE__, rc, rc1, rc2, rc3);
    return  FAILURE;
  }
  else
  {
    return  SUCCESS;
  }
}

/*********************************************************************
 * @purpose function to  check and deAllocate the client 
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
RC_t mabClientDisconnectAction(mabLogicalPortInfo_t *logicalPortInfo)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  uint32 physPort = 0, lPort = 0, type = 0;
   BOOL valid =  FALSE;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (mabIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  SUCCESS;

  /* check if the client can be de-allocated */

  MAB_EVENT_TRACE("checking if logicalInterface %d can be disconnected\n", logicalPortInfo->key.keyNum);

  if (logicalPortInfo->client.reAuthenticate)
  {
    MAB_EVENT_TRACE("%s:%d reAuthenticate  = %d \n",
                    __FUNCTION__,__LINE__, logicalPortInfo->client.reAuthenticate);
    return  FAILURE;
  }

  if (( FALSE == logicalPortInfo->protocol.authSuccess) &&
     ( FALSE == logicalPortInfo->protocol.authFail))
  {
    /* node just allocated */
    MAB_EVENT_TRACE( 
                    "%s:%d node just allocated \n",
                    __FUNCTION__,__LINE__);
    return  FAILURE;
  }

  if ( SUCCESS == mabHostIsDynamicNodeAllocCheck(mabBlock->mabPortInfo[physPort].hostMode, &valid))
  {
    if (valid)
    {
      MAB_EVENT_TRACE(
          "logicalInterface %d is getting disconnected\n", logicalPortInfo->key.keyNum);
      rc = mabClientSwInfoCleanup(logicalPortInfo);
    }
    else
    {
      return  FAILURE;
    }
  }

  if ( SUCCESS != rc) 
  {
    MAB_EVENT_TRACE(
                    "%s:%d Failure in Disconnect Action\n", 
                    __FUNCTION__, __LINE__);
  }

  return rc;
}

/*********************************************************************
 * @purpose function to  cleanup the client 
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
RC_t mabClientInfoCleanup(mabLogicalPortInfo_t *logicalPortInfo)
{
  RC_t rc =  SUCCESS;
  mabPortCfg_t *pCfg;
  uint32 physPort = 0, lPort = 0, type = 0;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);


  if (mabIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  SUCCESS;

  if ( SUCCESS == rc)
   rc = mabClientSwInfoCleanup(logicalPortInfo);

  if ( SUCCESS != rc) 
  {
    MAB_EVENT_TRACE(
                    "%s:%d Failure in client cleanup \n", 
                    __FUNCTION__, __LINE__);
  }

  return rc;
}


