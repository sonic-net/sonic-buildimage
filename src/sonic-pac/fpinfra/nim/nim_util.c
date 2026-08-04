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

#include "datatypes.h"
#include "commdefs.h"
#include "cnfgr_api.h"
#include "sysapi_hpc.h"
#include "nim_data.h"
#include "nim_exports.h"
#include "nim_trace.h"
#include "nim_ifindex.h"
#include "nim_util.h"
#include "platform_config.h"
#include "nim_outcalls.h"

static  BOOL nimConfigIdTreePopulatationComplete =  FALSE;


/*********************************************************************
* @purpose  return the configuration phase
*
* @param     none
*
* @returns  the phase number
*
* @notes
*
* @end
*********************************************************************/
 CNFGR_STATE_t nimConfigPhaseGet(void)
{
  if (!nimCtlBlk_g)
  {
    return  CNFGR_STATE_INVALID;
  }
  else
  {
    return(nimCtlBlk_g->nimPhaseStatus );
  }
}

/*********************************************************************
* @purpose  Get the state of an interface
*
* @param    intIfNum    The internal interface number
*
* @returns  Present state of the interface
*
* @notes    none
*
* @end
*********************************************************************/
 INTF_STATES_t nimUtilIntfStateGet(uint32 intIfNum)
{
   INTF_STATES_t state =  INTF_UNINITIALIZED;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    NIM_LOG_MSG("NIM: incorrect CNFGR phase for action\n");
  }
  else if ((intIfNum < 1) || (intIfNum > platIntfTotalMaxCountGet()))
  {
    NIM_LOG_MSG("NIM: intIfNum out of range\n");
  }
  else
  {
    state = nimCtlBlk_g->nimPorts[intIfNum].intfState;
  }

  return state;
}

/*********************************************************************
* @purpose  Determine whether NIM is in a state ready to process
*       interface requests.
*
* @param     none
*
* @returns   TRUE - Nim is ready.
*        FALSE - Nim is not ready.
*
* @notes
*
* @end
*********************************************************************/
 BOOL nimPhaseStatusCheck(void)
{
  return  TRUE;
}

/*********************************************************************
* @purpose  delete a unit slot port mapping to interface number
*
* @param    usp         Pointer to nimUSP_t structure
*
* @returns   SUCCESS or  FALIURE
*
* @notes    This function is not re-entrant, the caller must ensure integrety of data 
*       
* @end
*********************************************************************/
RC_t nimUnitSlotPortToIntfNumClear(nimUSP_t *usp)
{
  RC_t rc =  SUCCESS;
  nimUspIntIfNumTreeData_t data;
  nimUspIntIfNumTreeData_t *pData;

  data.usp = *usp;

  pData = avlDeleteEntry(&nimCtlBlk_g->nimUspTreeData, &data);

  if (pData ==  NULL)
  {
    NIM_LOG_MSG("NIM: %d.%d.%d not found, cannot delete it\n",usp->unit,usp->slot,usp->port);
    rc =  FAILURE;
  }

  return rc;
}

/*********************************************************************
* @purpose  create a unit slot port mapping to interface number
*
* @param    usp         Pointer to nimUSP_t structure
* @param    intIntfNum    interface number
*
* @returns   SUCCESS or  FALIURE
*
* @notes    This function is not re-entrant, the caller must ensure integrety of data 
*       
* @end
*********************************************************************/
RC_t nimUnitSlotPortToIntfNumSet(nimUSP_t *usp, uint32 intIntfNum)
{
  RC_t     rc =  SUCCESS;
  uint32   unit = 0, slot = 0, port = 0;
   INTF_TYPES_t sysIntfType;
  nimUspIntIfNumTreeData_t data;
  nimUspIntIfNumTreeData_t *pData;

  /* check the usp */
  if (usp !=  NULL)
  {
    unit = usp->unit;
    slot = usp->slot;
    port = usp->port;
  }
  else
  {
    rc =  FAILURE;
  }

  if ((rc !=  SUCCESS) || (nimPhaseStatusCheck () !=  TRUE))
  {
    rc =  FAILURE;
  }
  else if ((unit > nimCtlBlk_g->maxNumOfUnits) ||
           (slot > nimCtlBlk_g->maxNumOfSlotsPerUnit) || 
           (port == 0))
  {
    NIM_LOG_MSG("NIM: out of bounds usp used U=%d, S=%d P=%d\n",unit,slot,port); 
    rc =  FAILURE;
  }
  else if ( (nimGetIntfTypeFromUSP(usp, &sysIntfType) !=  SUCCESS) ||
            ( nimNumberOfInterfaceExceeded(sysIntfType) ==  TRUE))

  {
    /* The port number should not exceed the maximum number of interfaces for the type */
    NIM_LOG_MSG("NIM: out of bounds usp used U=%d, S=%d P=%d - port too large\n",unit,slot,port); 
    rc =  FAILURE;
  }

  else
  {
    data.intIfNum = intIntfNum;
    data.usp  = *usp;

    pData = avlInsertEntry(&nimCtlBlk_g->nimUspTreeData, &data);

    if (pData !=  NULLPTR)
    {
      NIM_LOG_MSG("NIM: Usp to intIfNum not added for intIfNum %d\n",intIntfNum);
      rc =  FAILURE;
    }
  } 

  return(rc);
}



/*********************************************************************
* @purpose  Determine the next State to transition to
*          
* @param    currState    @b{(input)}    The current state
* @param    event        @b{(input)}    The event being processed
* @param    nextState    @b{(output)}   The next state to transition to
*
* @returns   SUCCESS    
* @returns   FAILURE    
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimUtilIntfNextStateGet( INTF_STATES_t currState, PORT_EVENTS_t event, INTF_STATES_t *nextState)
{
  RC_t rc =  FAILURE;

  switch (currState)
  {
    case  INTF_UNINITIALIZED:
      if (event ==  CREATE)
      {
        *nextState =  INTF_CREATING;
        rc =  SUCCESS;
      }
      break;
    case  INTF_CREATING:
      if (event ==  CREATE_COMPLETE)
      {
        *nextState =  INTF_CREATED;
        rc =  SUCCESS;
      }
      break;
    case  INTF_CREATED:
      if (event ==  ATTACH)
      {
        *nextState =  INTF_ATTACHING;
        rc =  SUCCESS;
      }
      else if (event ==  DELETE)
      {
        *nextState =  INTF_DELETING;
        rc =  SUCCESS;
      }
      else
      {
        rc =  FAILURE;
      }
      break;
    case  INTF_ATTACHING:
      if (event ==  ATTACH_COMPLETE)
      {
        *nextState =  INTF_ATTACHED;
        rc =  SUCCESS;
      }
      break;
    case  INTF_ATTACHED:
      if (event ==  DETACH)
      {
        *nextState =  INTF_DETACHING;
        rc =  SUCCESS;
      }
      break;
    case  INTF_DETACHING:
      if (event ==  DETACH_COMPLETE)
      {
        *nextState =  INTF_CREATED;
        rc =  SUCCESS;
      }
      break;
    case  INTF_DELETING:
      if (event ==  DELETE_COMPLETE)
      {
        *nextState =  INTF_UNINITIALIZED;
        rc =  SUCCESS;
      }
      break;
    default:
      rc =  FAILURE;
  }
  return rc;
}

/*********************************************************************
* @purpose  Set the state of an interface
*
* @param    intIfNum    The internal interface number 
* @param    state       The state of the interface from the  INTF_STATES_t
*
* @returns   SUCCESS or  FALIURE
*
* @notes    none 
*       
* @end
*********************************************************************/
RC_t nimUtilIntfStateSet(uint32 intIfNum,  INTF_STATES_t state)
{
  RC_t rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    NIM_LOG_MSG("NIM: State set during incorrect CNFGR phase\n");
    rc =  FAILURE;
  }
  else if ((intIfNum < 1) || (intIfNum > platIntfTotalMaxCountGet()))
  {
    NIM_LOG_MSG("NIM: intIfNum out of range\n");
    rc =  FAILURE;
  }
  else
  {
    nimCtlBlk_g->nimPorts[intIfNum].intfState = state;
  }

  return(rc);
}

/*********************************************************************
* @purpose  Deletes the specified interface from the running and cached cfg file
*
* @param    intIfNum Internal Interface Number 
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none 
*       
* @end
*********************************************************************/
RC_t nimDeleteInterface(uint32 intIfNum)
{
  uint32 sysIntfType;
  nimCfgPort_t    *localConfigData;
  nimConfigID_t    configInterfaceId;
  nimConfigIdTreeData_t  configIdInfo;
  uint32        numOfInterfaces = 0;
  nimUSP_t usp;
  RC_t   rc =  SUCCESS;

  NIM_CRIT_SEC_WRITE_ENTER();

  IS_INTIFNUM_PRESENT(intIfNum,rc);

  if (rc ==  SUCCESS)
  {
    /* give back the sema before calling out to the other components */
    NIM_CRIT_SEC_WRITE_EXIT();

    /* delete nim created stats for this interface */
    //nimInterfaceCounters(intIfNum, FALSE);

    NIM_CRIT_SEC_WRITE_ENTER();

    sysIntfType = nimCtlBlk_g->nimPorts[intIfNum].sysIntfType;

    localConfigData   = nimCtlBlk_g->nimConfigData->cfgPort;  
    numOfInterfaces   = nimCtlBlk_g->nimConfigData->numOfInterfaces;
    configInterfaceId = nimCtlBlk_g->nimPorts[intIfNum].configInterfaceId;


    /* Remove the config id info from the avl tree */

    memset(&configIdInfo, 0 , sizeof( configIdInfo));
    NIM_CONFIG_ID_COPY(&configIdInfo.configId, &configInterfaceId);
    configIdInfo.intIfNum = intIfNum;
    (void)nimConfigIdTreeEntryDelete( &configIdInfo);
    

    /* clear the interface to be used in the bitmap*/
    NIM_INTF_CLRMASKBIT(nimCtlBlk_g->nimConfigData->configMaskBitmap, intIfNum);

    /* Delete the interface config in the config data */
    nimConfigInterfaceDelete(configInterfaceId);

    usp.unit = nimCtlBlk_g->nimPorts[intIfNum].usp.unit;
    usp.slot = nimCtlBlk_g->nimPorts[intIfNum].usp.slot;
    usp.port = nimCtlBlk_g->nimPorts[intIfNum].usp.port;

    nimIfIndexDelete(nimCtlBlk_g->nimPorts[intIfNum].ifIndex);

    /* set the entry in the quick map to unused */
    nimUnitSlotPortToIntfNumClear(&usp);   

    /* mark this interface is not in use */
    nimCtlBlk_g->nimPorts[intIfNum].present =  FALSE;

    if (nimCtlBlk_g->nimNumberOfPortsPerUnit[(uint32)usp.unit] > 0 )
    {
      nimCtlBlk_g->nimNumberOfPortsPerUnit[(uint32)usp.unit]--;
      rc =  SUCCESS;
    }

    nimCtlBlk_g->numberOfInterfacesByType[nimCtlBlk_g->nimPorts[intIfNum].sysIntfType]--;
    
    /* reset all the information for this interface */
    memset(( void * )&nimCtlBlk_g->nimPorts[intIfNum],0, sizeof( nimIntf_t )); 

    /* mark the config file as changed */
    //nimCtlBlk_g->nimConfigData->cfgHdr.dataChanged =  TRUE;
  }

  NIM_CRIT_SEC_WRITE_EXIT();

  return(rc);
}

/*********************************************************************
* @purpose  Add an entry to the nim nimConfigIdTreeData AVL tree
*          
* @param    *pConfigIdInfo    @b{(input)} pointer to a nimConfigIdTreeData_t structure
*
* @returns   SUCCESS    
* @returns   FAILURE    
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimConfigIdTreeEntryAdd(  nimConfigIdTreeData_t  *pConfigIdInfo)
{

   void *pData;
   RC_t rc;

   rc =  SUCCESS;

 /* WPJ     NIM_IFINDEX_CRIT_SEC_ENTER(); */
   pData = avlInsertEntry(&nimCtlBlk_g->nimConfigIdTreeData, (void *)pConfigIdInfo);

   if (pData !=  NULLPTR)
   {
     NIM_LOG_MSG("NIM: configId not added to tree for intIfNum %d\n", 
                 pConfigIdInfo->intIfNum);
     rc =  FAILURE;
   }

 /* WPJ     NIM_IFINDEX_CRIT_SEC_EXIT(); */

   return rc;

}

/*********************************************************************
* @purpose  Delete an entry from the nim nimConfigIdTreeData AVL tree
*          
* @param    *pConfigIdInfo    @b{(input)} pointer to a nimConfigIdTreeData_t structure
*
* @returns   SUCCESS    
* @returns   FAILURE    
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimConfigIdTreeEntryDelete(  nimConfigIdTreeData_t  *pConfigIdInfo)
{

   void *pData;
   RC_t rc;

   rc =  SUCCESS;

 /* WPJ     NIM_IFINDEX_CRIT_SEC_ENTER(); */
   pData = avlDeleteEntry(&nimCtlBlk_g->nimConfigIdTreeData, (void *)pConfigIdInfo);

   if (pData ==  NULLPTR)
   {
     NIM_LOG_MSG("NIM: configId could not be deleted from tree for intIfNum %d\n",
                 pConfigIdInfo->intIfNum);
     rc =  FAILURE;
   }


 /* WPJ     NIM_IFINDEX_CRIT_SEC_EXIT(); */

   return rc;

}


/*********************************************************************
* @purpose  Get an entry from the nim nimConfigIdTreeData AVL tree
*          
* @param    *pConfigId    @b{(input)} pointer to a nimConfigID_t structure
* @param    *pConfigIdInfo    @b{(input)} pointer to a nimConfigIdTreeData_t structure
*
* @returns   SUCCESS    
* @returns   FAILURE    
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimConfigIdTreeEntryGet(nimConfigID_t  *pConfigId, 
                                nimConfigIdTreeData_t  *pConfigIdInfo )
{

   void *pData;
   RC_t rc;

   rc =  SUCCESS;

 /* WPJ     NIM_IFINDEX_CRIT_SEC_ENTER(); */
   pData = avlSearch (&nimCtlBlk_g->nimConfigIdTreeData, pConfigId, AVL_EXACT);

   if (pData !=  NULLPTR)
   {
     memcpy( pConfigIdInfo, pData, sizeof(nimConfigIdTreeData_t) );
   }
   else
   {
     rc =  FAILURE;
   }

 /* WPJ     NIM_IFINDEX_CRIT_SEC_EXIT(); */

   return rc;

}
  


/*********************************************************************
* @purpose  Set either the saved config or the default config in the interface
*
* @param    port        @b{(input)}   The interface being manipulated
* @param    cfgVersion  @b{(input)}   version of the config
*
* @returns   SUCCESS or  FALIURE
*
* @notes    
*       
* @end
*********************************************************************/
void nimConfigIdTreePopulate(void)
{
  nimCfgPort_t    *localConfigData;
  nimConfigID_t   configInterfaceId;
  uint32        numOfInterfaces;
  uint32        counter;
  nimConfigIdTreeData_t  configIdInfo;

  if ((nimCtlBlk_g ==  NULLPTR) || (nimCtlBlk_g->nimConfigData ==  NULLPTR))
  {
    NIM_LOG_ERROR("NIM: Control block or config data not valid\n");
  }
  else
  {

    localConfigData = nimCtlBlk_g->nimConfigData->cfgPort;

    numOfInterfaces = nimCtlBlk_g->nimConfigData->numOfInterfaces;

    /* Set a null config ID for comparison */
    memset(&configInterfaceId, 0 , sizeof( configInterfaceId));

    /* see if the config for the interface exists in the file */
    for (counter = 0; counter <= numOfInterfaces; counter++)
    {
      if (NIM_CONFIG_ID_IS_EQUAL(&localConfigData[counter].configInterfaceId,&configInterfaceId))
      {
        /* null config id means empty entry.  Do not populate */
        continue;
      }
      else
      {
          memset(&configIdInfo, 0 , sizeof( configIdInfo));
          configIdInfo.intIfNum = localConfigData[counter].configIdMaskOffset;

          NIM_CONFIG_ID_COPY(&configIdInfo.configId, &localConfigData[counter].configInterfaceId);
          if ( nimConfigIdTreeEntryAdd(&configIdInfo) !=  SUCCESS)
          {
              NIM_LOG_MSG("Failed to add configId to avl tree for intIfNum %d\n",configIdInfo.intIfNum); 
          }
      }
    }

  }

  /* Flag this true if this phase is complete */
  nimConfigIdTreePopulatationComplete =  TRUE;
  return;
}

/*********************************************************************
* @purpose  Determine if nim configuration of config IDs is complete
*
* @param    void
*
* @returns   TRUE or  FALSE
*
* @notes    
*       
* @end
*********************************************************************/
 BOOL nimConfigIdTreeIsPopulated(void)
{
  return (nimConfigIdTreePopulatationComplete);
}

/*********************************************************************
* @purpose  Delete the internal interface number for the interface
*
* @param    configId  @b{(input)}  The configuration ID to be searched
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    If the action fails, the intIfNum will be set to zero 
*       
* @end
*********************************************************************/
RC_t nimConfigInterfaceDelete(nimConfigID_t configId)
{
  RC_t rc =  FAILURE;
  nimCfgPort_t *localConfigData;
  uint32     counter;
  uint32     numOfInterfaces;
  uint32     intIfNum = 0;

  if ((nimCtlBlk_g !=  NULLPTR) && (nimCtlBlk_g->nimConfigData !=  NULLPTR))
  {

    numOfInterfaces = nimCtlBlk_g->nimConfigData->numOfInterfaces;
    localConfigData   = nimCtlBlk_g->nimConfigData->cfgPort;  

    /* see if the config for the interface exists in the file */
    for (counter = 0; counter <= numOfInterfaces; counter++)
    {
      if (NIM_CONFIG_ID_IS_EQUAL(&localConfigData[counter].configInterfaceId,&configId))
      {
        intIfNum = localConfigData[counter].configIdMaskOffset;
        memset(&localConfigData[counter],0x00,sizeof(nimCfgPort_t));
        rc =  SUCCESS;
        break;
      }
    }

    if ((rc ==  SUCCESS) && (nimCtlBlk_g->nimPorts[intIfNum].present ==  TRUE))
    {
      /* clear the interface being deleted */
      NIM_INTF_CLRMASKBIT(nimCtlBlk_g->nimConfigData->configMaskBitmap, intIfNum);
    }
  }

  return(rc);
}

/*********************************************************************
* @purpose  Create the internal interface number for the interface
*
* @param    configId  @b{(input)}  The configuration ID to be searched
* @param    intIfNum  @b{(output)} Internal Interface Number created
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    If the action fails, the intIfNum will be set to zero 
*       
* @end
*********************************************************************/
RC_t nimIntIfNumCreate(nimConfigID_t configId, uint32 *intIfNum)
{
  RC_t rc =  FAILURE;
  uint32    unit,slot,port;
  uint32    maxUnits,maxSlotsPerUnit,maxPhysPortsPerSlot,maxPhysPortsPerUnit;
  uint32    min, max;
  uint32    i;
  uint32    slotOffset;
  nimConfigIdTreeData_t  configIdInfo;


  /*----------------------------------------------------------------------*/
  /* Determine if an intIfNum was assigned from a previous boot and reuse */
  /*----------------------------------------------------------------------*/
   
  if (nimConfigIdTreeEntryGet(&configId, &configIdInfo) ==  SUCCESS)
  {
      *intIfNum = configIdInfo.intIfNum;
      return  SUCCESS;
  }

  /*-------------------------------------------------------*/
  /* An intIfNum was not previously assigned.  Assign one. */
  /*-------------------------------------------------------*/
  
  maxUnits              = platUnitTotalMaxPerStackGet();
  maxSlotsPerUnit       = platSlotMaxPhysicalSlotsPerUnitGet();
  maxPhysPortsPerSlot   = platSlotMaxPhysicalPortsPerSlotGet();
  maxPhysPortsPerUnit   = platUnitMaxPhysicalPortsGet();

  switch (configId.type)
  {
    case  PHYSICAL_INTF:
      unit = (uint32)configId.configSpecifier.usp.unit; 
      slot = (uint32) configId.configSpecifier.usp.slot;  
      port = (uint32) configId.configSpecifier.usp.port;  

      if ((unit > maxUnits) || (slot >= maxSlotsPerUnit) || (port > maxPhysPortsPerSlot))
      {
        rc =  FAILURE;
        NIM_LOG_MSG("NIM: out of range usp (%d.%d.%d)\n",unit, slot,port ); 
      }
      else
      {
        /* calculate the last offset for ports in lower-numbered slots on this unit */
        slotOffset = 0;

        for (i= START_SLOT_NUM_WITH_PORT; i < slot; i++) 
        {
           slotOffset += sysapiHpcPhysPortsInSlotGet(i);
        }

        /* intIfNum determined by intIfNums assigned to lower numbered units and slots */
        *intIfNum = ((unit - 1) * maxPhysPortsPerUnit) + slotOffset + port;
        rc =  SUCCESS;

      }
      break; 


    case  CPU_INTF:
      /* CPU interfaces are directly after the Physical interfaces */
      unit = (uint32)configId.configSpecifier.usp.unit; 
      slot = (uint32) configId.configSpecifier.usp.slot;  
      port = (uint32) configId.configSpecifier.usp.port;  

      if ((slot != platSlotCpuSlotNumGet()) || (unit > maxUnits) ||
          (port > platIntfCpuIntfMaxCountGet()))
      {
        rc =  FAILURE;
        NIM_LOG_MSG("NIM: out of range usp for CPU Intf (%d.%d.%d)\n", unit, slot, port); 
      }
      else
      {

        /* The relative port number used by the owning interface for logical ports is 1-based.
           Decrement this number by 1 so that an intIfNum used by one type of interface does
           not pour over into the range used by another */
        (void) nimIntIfNumRangeGet( CPU_INTF, &min, &max);
        *intIfNum = min+ (port-1);
        rc =  SUCCESS;
      }

      break;

    case  LAG_INTF:
      /* LAG interfaces are directly after the CPU interfaces */
      port = configId.configSpecifier.dot3adIntf;
      if (port > platIntfLagIntfMaxCountGet())
      {
        rc =  FAILURE;
        NIM_LOG_MSG("NIM: out of range for LAG interface (%d)\n",configId.configSpecifier.dot3adIntf);
      }
      else
      {
        /* The relative port number used by the owning interface for logical ports is 1-based.
           Decrement this number by 1 so that an intIfNum used by one type of interface does
           not pour over into the range used by another */
          (void) nimIntIfNumRangeGet( LAG_INTF, &min, &max);
          *intIfNum = min+ (port-1);
          rc =  SUCCESS;
      }
      break;

    case  LOGICAL_VLAN_INTF:
      /* VLAN interfaces are directly after the LAG interfaces */
      if (configId.configSpecifier.vlanId >  PLATFORM_MAX_VLAN_ID)
      {
        rc =  FAILURE;
        NIM_LOG_MSG("NIM: out of range vlan interface (%d)\n",configId.configSpecifier.vlanId);
      }
      else
      {
        if (nimPortInstanceNumGet(configId, &port) ==  SUCCESS) 
        {

            /* The relative port number used by the owning interface for logical ports is 1-based.
               Decrement this number by 1 so that an intIfNum used by one type of interface does
               not pour over into the range used by another */
            (void) nimIntIfNumRangeGet( LOGICAL_VLAN_INTF, &min, &max);
            *intIfNum = min+ (port-1);
            rc =  SUCCESS;
        }
        else
            rc =  FAILURE;
      }
      break;

    case  LOOPBACK_INTF:
      /* Loopback interfaces are directly after the VLAN interfaces */
      if (configId.configSpecifier.loopbackId >=  MAX_NUM_LOOPBACK_INTF)
      {
        rc =  FAILURE;
        NIM_LOG_MSG("NIM: out of range loopback interface (%d)\n",
                    configId.configSpecifier.loopbackId);
      }
      else
      {
        /* loopbackId's are zero-based */
        (void) nimIntIfNumRangeGet( LOOPBACK_INTF, &min, &max);
        *intIfNum = min + configId.configSpecifier.loopbackId;
        rc =  SUCCESS;
      }
      break;

    case  TUNNEL_INTF:
      /* Tunnel interfaces are directly after the Loopback interfaces */
      if (configId.configSpecifier.tunnelId >=  MAX_NUM_TUNNEL_INTF)
      {
        rc =  FAILURE;
        NIM_LOG_MSG("NIM: out of range tunnel interface (%d)\n",
                    configId.configSpecifier.tunnelId);
      }
      else
      {
        /* tunnelId's are zero-based */
        (void) nimIntIfNumRangeGet( TUNNEL_INTF, &min, &max);
        *intIfNum = min + configId.configSpecifier.tunnelId;
        rc =  SUCCESS;
      }
      break;

    case  SERVICE_PORT_INTF:
      /* Service Port interfaces are directly after the Tunnel interfaces */
      if (configId.configSpecifier.servicePortId >=  MAX_NUM_SERVICE_PORT_INTF)
      {
        rc =  FAILURE;
        NIM_LOG_MSG("NIM: out of range service port interface (%d)\n",
                    configId.configSpecifier.servicePortId);
      }
      else
      {
        (void) nimIntIfNumRangeGet( SERVICE_PORT_INTF, &min, &max);
        *intIfNum = min + configId.configSpecifier.servicePortId;
        rc =  SUCCESS;
      }
      break;

    case  STACK_INTF:
      rc =  FAILURE;
      NIM_LOG_MSG("NIM: unsupported type of interface  STACK_INTF\n");
      break;

    default:
      rc =  FAILURE;
      NIM_LOG_MSG("NIM: unsupported type of interface (%d)\n",configId.type);
      break;
  }

  if ((nimCtlBlk_g !=  NULLPTR) && (nimCtlBlk_g->nimConfigData !=  NULLPTR))
  {
    if (rc !=  SUCCESS)
    {
      rc =  FAILURE;
      *intIfNum = 0;
    }
    else
    {
      rc =  SUCCESS;

      /* start with known data in the port */
      /* reset all the information for this interface */
      memset(( void * )&nimCtlBlk_g->nimPorts[*intIfNum],0, sizeof( nimIntf_t )); 

      /* set the interface to be used */
      NIM_INTF_SETMASKBIT(nimCtlBlk_g->nimConfigData->configMaskBitmap, *intIfNum);

      /* Store the configId/intIfNum for quick cross access */

      memset(&configIdInfo, 0 , sizeof( configIdInfo));
      NIM_CONFIG_ID_COPY(&configIdInfo.configId, &configId);
      configIdInfo.intIfNum = *intIfNum;
      (void)nimConfigIdTreeEntryAdd( &configIdInfo);
    }
  }
  else
  {
    rc =  FAILURE;
    *intIfNum = 0;
  }

  return(rc);
}
/*********************************************************************
* @purpose  Create the internal interface number for the interface
*
* @param    configId  @b{(input)}  The configuration ID to be searched
* @param    intIfNum  @b{(output)} Internal Interface Number created
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    If the action fails, the intIfNum will be set to zero 
*       
* @end
*********************************************************************/
RC_t nimIntIfNumDelete(uint32 intIfNum)
{
  RC_t rc =  FAILURE;

  if ((intIfNum > 0) && (intIfNum <= platIntfTotalMaxCountGet()))
  {
    /* start with known data in the port */
    memset(( void * )&nimCtlBlk_g->nimPorts[intIfNum], 0,sizeof(nimIntf_t)); 

    /* set the interface to be used */
    NIM_INTF_CLRMASKBIT(nimCtlBlk_g->nimConfigData->configMaskBitmap, intIfNum);

    rc =  SUCCESS;
  }

  return(rc);
}



/*********************************************************************
* @purpose  Check to see if the number of interfaces per type are exceeded
*
* @param    intfType  @b{(input)} one of  INTF_TYPES_t
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    If the action fails, the intIfNum will be set to zero 
*       
* @end
*********************************************************************/
 BOOL nimNumberOfInterfaceExceeded( INTF_TYPES_t intfType)
{
   BOOL rc =  FALSE;
  switch (intfType)
  {
    case  PHYSICAL_INTF:
      if (nimCtlBlk_g->numberOfInterfacesByType[ PHYSICAL_INTF] >= platIntfPhysicalIntfMaxCountGet())
      {
        rc = ( TRUE);
      }
      break;

    case  STACK_INTF:
      if (nimCtlBlk_g->numberOfInterfacesByType[ STACK_INTF] >= platIntfStackIntfMaxCountGet())
      {
        rc =  TRUE;
      }
      break;

    case  CPU_INTF:
      if (nimCtlBlk_g->numberOfInterfacesByType[ CPU_INTF] >= platIntfCpuIntfMaxCountGet())
      {
        rc =  TRUE;
      }
      break;

    case  LAG_INTF:
      if (nimCtlBlk_g->numberOfInterfacesByType[ LAG_INTF] >= platIntfLagIntfMaxCountGet())
      {
        rc =  TRUE;
      }
      break;

    case  LOGICAL_VLAN_INTF:
      if (nimCtlBlk_g->numberOfInterfacesByType[ LOGICAL_VLAN_INTF] >= platIntfVlanIntfMaxCountGet())
      {
        rc =  TRUE;
      }
      break;

    case  LOOPBACK_INTF:
      if (nimCtlBlk_g->numberOfInterfacesByType[ LOOPBACK_INTF] >= platIntfLoopbackIntfMaxCountGet())
      {
        rc =  TRUE;
      }
      break;

    case  TUNNEL_INTF:
      if (nimCtlBlk_g->numberOfInterfacesByType[ TUNNEL_INTF] >= platIntfTunnelIntfMaxCountGet())
      {
        rc =  TRUE;
      }
      break;

    case  SERVICE_PORT_INTF:
      if (nimCtlBlk_g->numberOfInterfacesByType[ SERVICE_PORT_INTF] >= platIntfServicePortIntfMaxCountGet())
      {
        rc =  TRUE;
      }
      break;

    default:
      rc =  TRUE;
  }
  return(rc);
}


/*********************************************************************
* @purpose  Get the maximum number of interfaces for an interface type
*
* @param    intfType  @b{(input)} one of  INTF_TYPES_t
*
* @returns  the maximum number of interfaces for an interface type, 0 or greater
*
* @notes    Returns zero if an interface type is unrecognized
*       
* @end
*********************************************************************/
 int32 nimMaxIntfForIntfTypeGet( INTF_TYPES_t intfType)
{

  uint32 numIntf;

  switch (intfType)
  {
    case  PHYSICAL_INTF:
       numIntf =  platIntfPhysicalIntfMaxCountGet();
       break;

    case  STACK_INTF:
      numIntf =  platIntfStackIntfMaxCountGet();
      break;

    case  CPU_INTF:
      numIntf =  platIntfCpuIntfMaxCountGet();
      break;

    case  LAG_INTF:
      numIntf =  platIntfLagIntfMaxCountGet();
      break;

    case  LOGICAL_VLAN_INTF:
      numIntf =  platIntfVlanIntfMaxCountGet();
      break;

    case  LOOPBACK_INTF:
      numIntf =  platIntfLoopbackIntfMaxCountGet();
      break;

    case  TUNNEL_INTF:
      numIntf =  platIntfTunnelIntfMaxCountGet();
      break;

    case  SERVICE_PORT_INTF:
      numIntf = platIntfServicePortIntfMaxCountGet();
      break;

    default:
      numIntf =  0;

  }

  return numIntf;
}

/*********************************************************************
* @purpose  Log NIM Message at ERROR Severity
*
* @param     fileName - file
* @param     lineNum - Line
* @param     format - Format of the output.
* @param     ... - Variable output list.
*
* @returns   nothing
*
* @notes    
*       
* @end
*********************************************************************/
void nimLogErrorMsg ( BOOL logError,  char8 * fileName, uint32 lineNum,  char8 * format, ...)
{
   uchar8 nim_log_buf[LOG_MSG_MAX_MSG_SIZE];
  va_list ap;
   int32 rc;

  va_start(ap, format);
  rc = vsnprintf(nim_log_buf, sizeof (nim_log_buf), format, ap);
  va_end(ap);

  nim_log_buf[LOG_MSG_MAX_MSG_SIZE - 1] = 0;
  syslog( LOG_SEVERITY_ERROR, "%s", nim_log_buf); 
}
