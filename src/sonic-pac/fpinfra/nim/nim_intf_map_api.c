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
#include "nim_data.h"
#include "nim_util.h"
#include "log.h"

/*********************************************************************
* @purpose  Returns the descripion for port event
*
* @param    event     interface event
*
* @returns  description for the event

* @notes    none
*
* @end
*********************************************************************/
 char8 *nimGetIntfEvent( PORT_EVENTS_t event)
  {
    switch (event)
    {
      case  PORT_DISABLE:    return " PORT_DISABLE";
      case  PORT_ENABLE:    return " PORT_ENABLE";
      case  PORT_INSERT:    return " PORT_INSERT";
      case  DOWN:    return " DOWN";
      case  UP:    return " UP";
      case  INACTIVE:    return " INACTIVE";
      case  ACTIVE:    return " ACTIVE";    break;
      case  FORWARDING:    return " FORWARDING";
      case  NOT_FORWARDING:    return " NOT_FORWARDING";
      case  CREATE:    return " CREATE";
      case  CREATE_COMPLETE:    return " CREATE_COMPLETE";
      case  DELETE_PENDING:    return " DELETE_PENDING";
      case  DELETE:    return " DELETE";
      case  DELETE_COMPLETE:    return " DELETE_COMPLETE";
      case  LAG_ACQUIRE:    return " LAG_ACQUIRE";
      case  LAG_RELEASE:    return " LAG_RELEASE";
      case  SPEED_CHANGE:    return " SPEED_CHANGE";
      case  LAG_CFG_CREATE:    return " LAG_CFG_CREATE";
      case  LAG_CFG_MEMBER_CHANGE:    return " LAG_CFG_MEMBER_CHANGE";
      case  LAG_CFG_REMOVE:    return " LAG_CFG_REMOVE";
      case  LAG_CFG_END:    return " LAG_CFG_END";
      case  PROBE_SETUP:    return " PROBE_SETUP";
      case  PROBE_TEARDOWN:    return " PROBE_TEARDOWN";
      case  SET_INTF_SPEED:    return " SET_INTF_SPEED";
      case  SET_MTU_SIZE:    return " SET_MTU_SIZE";
      case  PORT_ROUTING_ENABLED:    return " PORT_ROUTING_ENABLED";
      case  PORT_ROUTING_DISABLED:    return " PORT_ROUTING_DISABLED";
      case  TRILL_TRUNK_ENABLED:    return " TRILL_TURNK_ENABLED";
      case  TRILL_TRUNK_DISABLED:    return " TRILL_TURNK_DISABLED";
      case  PORT_BRIDGING_ENABLED:    return " PORT_BRIDGING_ENABLED";
      case  PORT_BRIDGING_DISABLED:    return " PORT_BRIDGING_DISABLED";
      case  VRRP_TO_MASTER:    return " VRRP_TO_MASTER";
      case  VRRP_FROM_MASTER:    return " VRRP_FROM_MASTER";
      case  AUTHMGR_PORT_AUTHORIZED:    return " AUTHMGR_PORT_AUTHORIZED";
      case  AUTHMGR_PORT_UNAUTHORIZED:    return " AUTHMGR_PORT_UNAUTHORIZED";
      case  ATTACH:    return " ATTACH";
      case  ATTACH_COMPLETE:    return " ATTACH_COMPLETE";
      case  DETACH:    return " DETACH";
      case  DETACH_COMPLETE:    return " DETACH_COMPLETE";
      case  AUTHMGR_ACQUIRE: return " AUTHMGR_ACQUIRE";
      case  AUTHMGR_RELEASE: return " AUTHMGR_RELEASE";
      case  PORT_STATS_RESET: return " PORT_STATS_RESET";
#ifdef  PORT_AGGREGATOR_PACKAGE
      case  PORT_AGG_UP: return " PORT_AGG_UP";
      case  PORT_AGG_DOWN: return " PORT_AGG_DOWN";
#endif
      case  PORT_PFC_ACTIVE:   return " PORT_PFC_ACTIVE";
      case  PORT_PFC_INACTIVE: return " PORT_PFC_INACTIVE";
      case  ETHERNET_SWITCHPORT: return " ETHERNET_SWITCHPORT";
      case  NETWORK_INTF_TYPE_NNI: return " NETWORK_INTF_TYPE_NNI";
      case  NETWORK_INTF_TYPE_UNI_C: return " NETWORK_INTF_TYPE_UNI_C";
      case  NETWORK_INTF_TYPE_UNI_S: return " NETWORK_INTF_TYPE_UNI_S";
      case  LAG_RELEASE_PENDING: return " LAG_RELEASE_PENDING";
      case  LAG_DOWN_PENDING: return " LAG_DOWN_PENDING";
      case  LAG_ACQUIRE_PENDING: return " LAG_ACQUIRE_PENDING";
   default:
      return "Unknown Port Event";
    }
  }

/*********************************************************************
* @purpose  Returns the Unit-Slot-Port
*           associated with the internal interface number
*
* @param    intIfNum    @b{(input)}  internal interface number
* @param    usp         @b{(output)} pointer to nimUSP_t structure,
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimGetUnitSlotPort(uint32 intIfNum, nimUSP_t *usp)
{
  RC_t   rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  NOT_EXIST;
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: incorrect phase for operation.");
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();
    if ((intIfNum < 1) || (intIfNum > platIntfTotalMaxCountGet()))
    {
        rc =  FAILURE;
    }
    else if (nimCtlBlk_g->nimPorts[intIfNum].present !=  TRUE)
    {
        rc =  ERROR;
    }
    else
    {
        rc =  SUCCESS;
    }

    if (rc ==  SUCCESS)
    {
      memcpy (usp, &nimCtlBlk_g->nimPorts[intIfNum].usp, sizeof(nimUSP_t));
    }
    NIM_CRIT_SEC_READ_EXIT();
  }

  return(rc);
}

/*********************************************************************
* @purpose  Returns the internal interface number
*           associated with the Unit-Slot-Port
*
* @param    usp         @b{(input)}  pointer to nimUSP_t structure
* @param    intIfNum    @b{(output)} pointer to internal interface number,
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimGetIntIfNumFromUSP(nimUSP_t* usp, uint32 *intIfNum)
{

  RC_t     rc =  SUCCESS;
  uint32   unit = 0, slot = 0, port = 0;
   INTF_TYPES_t sysIntfType;
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
    NIM_LOG_MSG("NIM: usp is NULL\n");
    rc =  FAILURE;
  }

  if ((rc !=  SUCCESS) || (nimPhaseStatusCheck() !=  TRUE))
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: incorrect phase for operation.");
  }
  else if ((unit > nimCtlBlk_g->maxNumOfUnits) ||
           (slot > nimCtlBlk_g->maxNumOfSlotsPerUnit) ||
           (port == 0))
  {
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: out of bounds usp used U=%d, S=%d P=%d\n",unit,slot,port);
    rc =  FAILURE;
  }
  else if ( (nimGetIntfTypeFromUSP(usp, &sysIntfType) !=  SUCCESS) ||
            ( port > nimMaxIntfForIntfTypeGet(sysIntfType) ))
  {
    /* The port number should not exceed the maximum number of interfaces for the type */
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: out of bounds usp used U=%d, S=%d P=%d - port too large\n",unit,slot,port);
    rc =  FAILURE;
  }

  else
  {
    NIM_CRIT_SEC_READ_ENTER();

    pData = avlSearch (&nimCtlBlk_g->nimUspTreeData, usp, AVL_EXACT);

    if (pData !=  NULLPTR)
    {
      *intIfNum = pData->intIfNum;
      rc =  SUCCESS;
    }
    else
    {
      rc =  ERROR;
    }

    NIM_CRIT_SEC_READ_EXIT();

  }

  return(rc);
}


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
RC_t nimGetIntfTypeFromUSP(nimUSP_t* usp,  INTF_TYPES_t *sysIntfType)
{
/* WPJ_TBD:  Make this more extensible */

  RC_t     rc =  SUCCESS;
  uint32   unit = 0, slot = 0, port = 0;

  /* check the usp */
  if (usp !=  NULL)
  {
    unit = usp->unit;
    slot = usp->slot;
    port = usp->port;
  }
  else
  {
    NIM_LOG_MSG("NIM: usp is NULL\n");
    rc =  FAILURE;
  }

  if ((rc !=  SUCCESS) || (nimPhaseStatusCheck() !=  TRUE))
  {
    rc =  FAILURE;
    NIM_LOG_MSG("NIM: incorrect phase for operation\n");
  }
  else if ((unit > nimCtlBlk_g->maxNumOfUnits) ||
           (slot > nimCtlBlk_g->maxNumOfSlotsPerUnit) ||
           (port == 0))
  {
     LOGF( LOG_SEVERITY_INFO,
            "NIM: out of bounds usp used U=%d, S=%d P=%d\n",unit,slot,port);
    rc =  FAILURE;
  }

  if (slot == platSlotVlanSlotNumGet() )
    *sysIntfType =  LOGICAL_VLAN_INTF;
  else if (slot == platSlotCpuSlotNumGet() )
    *sysIntfType =  CPU_INTF;
  else if (slot == platSlotLagSlotNumGet() )
    *sysIntfType =  LAG_INTF;
  else if (slot == platSlotLoopbackSlotNumGet() )
    *sysIntfType =  LOOPBACK_INTF;
  else if (slot == platSlotTunnelSlotNumGet() )
    *sysIntfType =  TUNNEL_INTF;
  else if (slot == platSlotServicePortSlotNumGet() )
    *sysIntfType =  SERVICE_PORT_INTF;
  else
    *sysIntfType =  PHYSICAL_INTF;  /* WPJ_TBD:  Assume physical until more automatic mapping is done */
  return rc;
}



/*********************************************************************
* @purpose  Return Internal Interface Number of next valid interface for
*           the specified system interface type.
*
* @param    sysIntfType  @b{(input)}  The type of interface requested
* @param    intIfNum     @b{(input)}  The present Internal Interface Number
* @param    nextintIfNum @b{(output)} The Next Internal Interface Number,
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
RC_t nimNextValidIntfNumberByType( INTF_TYPES_t sysIntfType, uint32 intIfNum, uint32 *nextIntIfNum)
{
  uint32 maxIntf;
  RC_t rc =  SUCCESS;


  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: incorrect phase for operation.");
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();
    /* do not get the same intf number */
    /* look to the next one first */
    intIfNum++;

    maxIntf = platIntfTotalMaxCountGet();

    for (; intIfNum <= maxIntf; intIfNum++)
    {
      if ((nimCtlBlk_g->nimPorts[intIfNum].present ==  TRUE) &&
          (nimCtlBlk_g->nimPorts[intIfNum].sysIntfType == sysIntfType))
      {
        break;
      }
    }

    if (intIfNum <= maxIntf)
    {
      *nextIntIfNum = intIfNum;
    }
    else
    {
      rc =  FAILURE;
    }
    NIM_CRIT_SEC_READ_EXIT();
  }

  return(rc);
}

/*********************************************************************
* @purpose  Return Internal Interface Number of the first valid interface for
*           the specified interface type.
*
* @param    sysIntfType  @b{(input)}  The type of interface requested
* @param    intIfNum     @b{(output)} Internal Interface Number
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
RC_t nimFirstValidIntfNumberByType( INTF_TYPES_t sysIntfType, uint32 *firstIntIfNum)
{
  uint32 maxIntf;
  uint32 intIfNum;
  RC_t   rc =  SUCCESS;


  maxIntf = platIntfTotalMaxCountGet();

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =   ERROR;
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: incorrect phase for operation.");
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();
    /* first avaliable internal interface number for the specified type */
    for (intIfNum = 1; intIfNum <= maxIntf; intIfNum++)
    {
      if ((nimCtlBlk_g->nimPorts[intIfNum].present ==  TRUE) &&
          (nimCtlBlk_g->nimPorts[intIfNum].sysIntfType == sysIntfType))
      {
        break;
      }
    } /* end for */

    if (intIfNum <= maxIntf)
    {
      *firstIntIfNum = intIfNum;
    }
    else
    {
      rc =   ERROR;
    }
    NIM_CRIT_SEC_READ_EXIT();
  }

  return(rc);
}

/*********************************************************************
* @purpose  Returns the ifIndex associated with the
*           internal interface number
*
* @param    intIfNum    @b{(input)}   internal interface number
* @param    ifIndex     @b{(output)}  pointer to ifIndex,
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimGetIntfIfIndex(uint32 intIfNum, uint32 *ifIndex)
{
  RC_t   rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: incorrect phase for operation.");
  }
  else
  {

    NIM_CRIT_SEC_READ_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      *ifIndex = nimCtlBlk_g->nimPorts[intIfNum].ifIndex;
    }

    NIM_CRIT_SEC_READ_EXIT();
  }

  return(rc);
}

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
RC_t nimCheckIfNumber(uint32 intIfNum)
{
  RC_t     rc =  ERROR;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  NOT_EXIST;
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: incorrect phase for operation.");
  }
  else if (intIfNum <= platIntfTotalMaxCountGet())
  {
    NIM_CRIT_SEC_READ_ENTER();

    if (intIfNum < 1)
    {
        rc =  FAILURE;
    }
    else if (nimCtlBlk_g->nimPorts[intIfNum].present !=  TRUE)
    {
        rc =  ERROR;
    }
    else
    {
        rc =  SUCCESS;
    }

    NIM_CRIT_SEC_READ_EXIT();
  }
  else
  {
    NIM_LOG_MSG("nimCheckIfNumber: internal interface number %d out of range\n", (int)intIfNum);
    rc =  FAILURE;
  }

  return(rc);
}

/*********************************************************************
*
* @purpose  Get the configuration ID for the given Internal Interface ID
*
* @param    internalIntfId  @b{(input)}  Internal Interface ID
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
RC_t nimConfigIdGet(uint32 internalIntfId,nimConfigID_t *configId)
{
  RC_t rc =  FAILURE;

  NIM_CRIT_SEC_READ_ENTER();

  do
  {

    if ((nimCtlBlk_g ==  NULLPTR) || (nimCtlBlk_g->nimPorts ==  NULLPTR))
    {
      rc =  FAILURE;
      break;
    }
    else if ((internalIntfId < 1) || (internalIntfId > platIntfTotalMaxCountGet()))
    {
      rc =  FAILURE;
      break;
    }
    else
    {
      IS_INTIFNUM_PRESENT(internalIntfId,rc);

      if (rc ==  SUCCESS)
      {
        NIM_CONFIG_ID_COPY(configId ,&nimCtlBlk_g->nimPorts[internalIntfId].configInterfaceId);
        rc =  SUCCESS;
      }
    }

  } while ( 0 );
  NIM_CRIT_SEC_READ_EXIT();

  return(rc);
}

/*********************************************************************
* @purpose  Return Internal Interface Number of the first valid port
*
* @param    firstIntIfNum    @b{(output)} first valid internal interface number
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
RC_t nimFirstValidIntfNumber(uint32 *firstIntIfNum)
{
  uint32 intIfNum;
  uint32 maxIntf;
  RC_t rc =  SUCCESS;

  maxIntf = platIntfTotalMaxCountGet();

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: incorrect phase for operation.");
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();
    /* first avaliable Physical internal interface number */
    for (intIfNum = 1; intIfNum <= maxIntf ; intIfNum++)
    {
      if (nimCtlBlk_g->nimPorts[intIfNum].present ==  TRUE)
      {
        break;
      }
    }

    if (intIfNum <= maxIntf)
    {
      *firstIntIfNum = intIfNum;
    }
    else
    {
      rc =  ERROR;
    }
    NIM_CRIT_SEC_READ_EXIT();
  }

  return(rc);
}

/*********************************************************************
* @purpose  Return Internal Interface Number of next valid port
*
* @param    intIfNum     @b{(input)}  Internal Interface Number
* @param    nextintIfNum @b{(output)} Internal Interface Number,
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes none
*
* @end
*********************************************************************/
RC_t nimNextValidIntfNumber(uint32 intIfNum, uint32 *nextIntIfNum)
{
  RC_t   rc =  SUCCESS;
  uint32 maxIntf;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
            "NIM: incorrect phase for operation.");
  }
  else
  {

    NIM_CRIT_SEC_READ_ENTER();
    maxIntf = platIntfTotalMaxCountGet();
    /* next avaliable Physical internal interface number */
    for (intIfNum++; intIfNum <= maxIntf; intIfNum++)
    {
      if ((nimCtlBlk_g->nimPorts[intIfNum].present ==  TRUE))
      {
        break;
      }
    }

    if (intIfNum <= maxIntf)
    {
      *nextIntIfNum = intIfNum;
    }
    else
    {
      rc =  FAILURE;
    }
    NIM_CRIT_SEC_READ_EXIT();
  }

  return(rc);
}

/*********************************************************************
* @purpose  check if the port is a macro port
*
* @param    intIfNum    internal interface number
*
* @returns   TRUE
* @returns   FALSE
*
* @notes    none
*
* @end
*********************************************************************/
 BOOL nimIsMacroPort(uint32 intIfNum)
{
   BOOL   returnVal =  FALSE;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    returnVal =   FALSE;
  }
  else if (nimCtlBlk_g->nimPorts[intIfNum].operInfo.macroPort.macroPort == 0)
  {
    returnVal =  TRUE;
  }
  return(returnVal);
}

/*********************************************************************
* @purpose  Get valid range of valid internal inteface numbers
*           for a given interface type
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
RC_t nimIntIfNumRangeGet( INTF_TYPES_t intfType, uint32 *min, uint32 *max)
{

  *min = 0;
  *max = 0;

  if (intfType >=  MAX_INTF_TYPE_VALUE)
      return  FAILURE;

  *min = nimCtlBlk_g->intfTypeData[intfType].minIntIfNumber;
  *max = nimCtlBlk_g->intfTypeData[intfType].maxIntIfNumber;

  return  SUCCESS;
}


