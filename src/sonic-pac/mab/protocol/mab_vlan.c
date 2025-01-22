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
#include "auth_mgr_exports.h"
#include "mab_include.h"
#include "mab_client.h"
#include "mab_struct.h"

extern mabBlock_t *mabBlock;


/*********************************************************************
 * @purpose  Processes  Vlan PVID Change Notify event.
 *
 * @param    intIfNum   @b{(input)} internal interface number
 * @param    vlanId     @b{(input)} VlanId (new PVID on the port)
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
*********************************************************************/
RC_t mabVlanPVIDChangeEventProcess(uint32 physPort,uint32 vlanId)
{
  RC_t rc =  FAILURE;
  mabLogicalPortInfo_t *logicalPortInfo;
  uint32 i = 0;
   BOOL valid =  FALSE;

  MAB_EVENT_TRACE(
      "%s:PVID for port -%d changed to Vlan %d\n",
      __FUNCTION__,physPort,vlanId);

  if (mabBlock->mabPortInfo[physPort].portControlMode ==  AUTHMGR_PORT_AUTO)
  {
    /* Nothing to be done as port pvid change is also triggered by a client being authorized
       on the port in other host modes, where mab can aquire the port. 
       The Admin cannot change the operational pvid of the port
       as it is acquired by mab. */

    /* check if the host policy is valid for the current host mode */
    if (( SUCCESS == mabHostIsDynamicNodeAllocCheck(mabBlock->mabPortInfo[physPort].hostMode, &valid)) &&
        ( TRUE == valid))
    {
      i = MAB_LOGICAL_PORT_ITERATE;
      while ((NULLPTR != (logicalPortInfo=mabLogicalPortInfoGetNextNode(physPort,&i)) &&
            (0 != logicalPortInfo->key.keyNum)))
      {
        if ((vlanId == logicalPortInfo->client.vlanId) &&
            ( AUTHMGR_PORT_STATUS_AUTHORIZED == logicalPortInfo->client.logicalPortStatus))
        {
          MAB_EVENT_TRACE(
              "pvid %d is changed. logicalPort %d is authenticated on same vlan. vlan type %s"
              "Cleaning up the client \n",
              vlanId, i, mabVlanTypeStringGet(logicalPortInfo->client.vlanType));


          /* invoke the cleanup */
          rc = mabClientInfoCleanup(logicalPortInfo);

          if ( SUCCESS != rc)
          {
            MAB_EVENT_TRACE(
                "client cleanup for logicalPort %d is NOT successful\n",i);
          }
        }
      }
    }
  }
  return rc;
}

/*********************************************************************
 * @purpose  check if the port participation can be removed for a vlan
 *
 * @param    physPort    @b{(input))  Port
 * @param    vlanId      @b{(input)) vlan Id
 *
 * @returns   SUCCESS/ FAILRE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabVlanPortDeletionValidate(uint32 physPort, uint32 vlanId)
{
  uint32 i = 0;
  mabLogicalPortInfo_t *logicalPortInfo =  NULLPTR;

  if (AUTHMGR_PORT_AUTO == mabBlock->mabPortInfo[physPort].portControlMode)
  {
      i = MAB_LOGICAL_PORT_ITERATE;
      while ((NULLPTR != (logicalPortInfo=mabLogicalPortInfoGetNextNode(physPort,&i)) &&
            (0 != logicalPortInfo->key.keyNum)))
      {
        if (vlanId == logicalPortInfo->client.vlanId)
        {
           MAB_EVENT_TRACE(
            "logicalPort %d is still a member of vlanId %d. \n",
             i, logicalPortInfo->client.vlanId);

          return  FAILURE;
        }
      }
  }
  return  SUCCESS;
}

/*********************************************************************
 * @purpose  check if the port can be aquired by mab 
 *
 * @param    physPort    @b{(input))  Port
 * @param    vlanId      @b{(input)) vlan Id
 *
 * @returns   SUCCESS/ FAILRE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabVlanPortAcquireCheck(uint32 physPort)
{
   BOOL valid =  FALSE;

  if ( AUTHMGR_PORT_AUTO == mabBlock->mabPortInfo[physPort].portControlMode)
  {
    /* check if the host policy is valid for the current host mode */
    if (( SUCCESS == mabHostIsDynamicNodeAllocCheck(mabBlock->mabPortInfo[physPort].hostMode, &valid)) &&
        ( FALSE == valid))
    {
      return  SUCCESS;
    }
  }
  return  FAILURE;
}

/*********************************************************************
 * @purpose  Apply mab vlan assignment to a specific logical interface
 *
 * @param    intIfNum   @b{(input)) internal interface number
 * @param    vlanId     @b{(input)) vlan id
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments This call should happen only in mac-based mode.
 *
 * @end
 *********************************************************************/
RC_t mabVlanPortAddProcess(uint32 intIfNum,uint32 vlanId)
{
  return  SUCCESS;
}

/*********************************************************************
 * @purpose  remove mab vlan assignment to a specific logical interface
 *
 * @param    intIfNum   @b{(input)) internal interface number
 * @param    vlanId     @b{(input)) vlan id
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments This call should happen only in mac-based mode.
 *
 * @end
 *********************************************************************/
RC_t mabVlanDeleteProcess(uint32 vlanId)
{
  RC_t nimRc =  SUCCESS;
  uint32 intIfNum;

  nimRc = mabFirstValidIntfNumber(&intIfNum);
  while (nimRc ==  SUCCESS)
  {
    /* delete the clients on this port for the vlan */
    mabVlanPortDeleteProcess(intIfNum, vlanId);

    nimRc = mabNextValidIntf(intIfNum, &intIfNum);
  }

  return  SUCCESS;
}


/*********************************************
 * @purpose  Enable  mab vlan to a specified interface
 *
 * @param    intIfNum   @b{(input)) internal interface number
 * @param    guestVlanId @b{(input)} guest vlan id
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabVlanPortDeleteProcess(uint32 intIfNum,uint32 vlanId)
{
  RC_t rc =  SUCCESS;
  mabLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 lIntIfNum;
  mabPortCfg_t *pCfg;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
     return  FAILURE;


  if ( AUTHMGR_PORT_AUTO != mabBlock->mabPortInfo[intIfNum].portControlMode)
    return  SUCCESS;


  if(mabBlock->mabPortInfo[intIfNum].portControlMode ==  AUTHMGR_PORT_AUTO)
  {
    lIntIfNum = MAB_LOGICAL_PORT_ITERATE;
    while((logicalPortInfo=mabLogicalPortInfoGetNextNode(intIfNum,&lIntIfNum))!=  NULLPTR)
    {
      if ( AUTHMGR_PORT_STATUS_AUTHORIZED == logicalPortInfo->client.logicalPortStatus)
      {
        /* clean up the client */
        if (vlanId == logicalPortInfo->client.vlanId)
        {
          MAB_EVENT_TRACE(
              "port %d is deleted from vlan %d. logicalPort %d is authenticated on same vlan. vlan type %s"
              "Cleaning up the client \n",
              intIfNum, vlanId, lIntIfNum, mabVlanTypeStringGet(logicalPortInfo->client.vlanType));

          /* invoke the cleanup */
          rc = mabClientSwInfoCleanup(logicalPortInfo);

          if ( SUCCESS != rc)
          {
            MAB_EVENT_TRACE(
                "client cleanup for logicalPort %d is NOT successful\n",lIntIfNum);
          }
        }
      }
    }
  }
  return  SUCCESS;
}


