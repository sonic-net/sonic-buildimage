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
#include "system_exports.h"
#include "nim_data.h"
#include "nim_util.h"
#include "log.h"

/*
 * Local macro for checking if a given parameter can be set.
 * Must be used where the intInfNum ("i") has been validated.
 * Used by externalized version nimIntfParmCanSet().
 */
#define NIM_PARM_CANSET(i, p) \
        ((nimCtlBlk_g->nimPorts[(i)].operInfo.settableParms & (p)) == (p))

/*********************************************************************
* @purpose  Sets the administrative state of the specified interface.
*
* @param    intIfNum Internal Interface Number
*
* @param    adminState admin state,
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
RC_t nimSetIntfAdminState(uint32 intIfNum, uint32 adminState)
{
  RC_t  rc =  FAILURE;
  uint32 tmpAdminState = adminState;
  uint32 ldIntfNum = intIfNum;
   uchar8 ifName[ NIM_IFNAME_SIZE + 1];
  uint32 state;
  NIM_EVENT_SPECIFIC_DATA_t eventData;

  memset (&eventData, 0, sizeof(NIM_EVENT_SPECIFIC_DATA_t));

  if (nimPhaseStatusCheck() !=  TRUE)
  {
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for setting admin state - intIfNum %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    /*
    if ( TRUE == dot3adIsLagMember(intIfNum))
    {
      if ( SUCCESS != dot3adWhoisOwnerLag(intIfNum, &ldIntfNum))
      {
        return  FAILURE;
      }
    }
    */

    NIM_CRIT_SEC_WRITE_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      do
      {
        if ((adminState ==  ENABLE) || (adminState ==  DISABLE) || (adminState ==  DIAG_DISABLE))
        {
          if (adminState !=  DIAG_DISABLE)
          {
            if (nimCtlBlk_g->nimPorts[intIfNum].diagDisabled ==  TRUE)
            {
              nimCtlBlk_g->nimPorts[intIfNum].diagDisabled =  FALSE;
              //nimCtlBlk_g->nimPorts[intIfNum].diagDisabledReason = NIM_DDISABLE_REASON_NONE;
            }
            if (nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.adminState != adminState)
            {
              nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.adminState = adminState;
            }
          }
          else
          {
            nimCtlBlk_g->nimPorts[intIfNum].diagDisabled =  TRUE;
          }

          NIM_CRIT_SEC_WRITE_EXIT();
        }
        else
        {
          NIM_CRIT_SEC_WRITE_EXIT();

           LOGF( LOG_SEVERITY_DEBUG,
            "Received invalid admin state %u for interface %s. Set request ignored.",
              adminState, ifName);
          rc =  FAILURE;
          break; /* goto end of while */
        }


        if (rc ==  SUCCESS)
        {
          if (adminState ==  ENABLE)
          {
            rc = (nimNotifyIntfChange(intIfNum,  PORT_ENABLE, eventData));
          }
          else
          {
            /* As far as rest of the apps are concerned there is no difference between
             * a disabled port and a diag disabled port
             */
            rc = nimNotifyIntfChange(intIfNum,  PORT_DISABLE, eventData);
          }
        }

      } while (0);
    }
    else
    {
      NIM_CRIT_SEC_WRITE_EXIT();
    }
  }

  return rc;
}
/*********************************************************************
* @purpose  Returns the internal interface type
*           associated with the internal interface number
*
* @param    intIfNum    internal interface number
* @param    sysIntfType pointer to internal interface type,
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
RC_t nimGetIntfType(uint32 intIfNum,  INTF_TYPES_t *sysIntfType)
{
  RC_t rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for get interface type - ifIndex %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum, rc);

    if (rc ==  SUCCESS)
    {
      *sysIntfType = nimCtlBlk_g->nimPorts[intIfNum].sysIntfType;
    }

    NIM_CRIT_SEC_READ_EXIT();
  }
  return rc;
}

/*********************************************************************
* @purpose  Get phy capability of the specified interface
*
* @param    intIfNum    Internal Interface Number
* @param    macAddr     pointer to phyCapability,
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
RC_t nimGetIntfPhyCapability(uint32 intIfNum,  uint64 *phyCapability)
{
  RC_t rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for getting capabilities - intIfNum %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      if (nimCtlBlk_g->nimPorts[intIfNum].dynamicCap)
      {
        *phyCapability = nimCtlBlk_g->nimPorts[intIfNum].capabilityCfg.dynCapabilities;
        NIM_CRIT_SEC_READ_EXIT();
        return  SUCCESS;
      }
      *phyCapability = nimCtlBlk_g->nimPorts[intIfNum].operInfo.phyCapability;
      if ( PHY_CAP_DUAL_MODE_SUPPORT_CHECK(*phyCapability))
      {
        NIM_CRIT_SEC_READ_EXIT();

        rc = nimUpdateIntfPhyCapability(intIfNum);
        NIM_CRIT_SEC_READ_ENTER();
        *phyCapability = nimCtlBlk_g->nimPorts[intIfNum].operInfo.phyCapability;
      }
    }

    NIM_CRIT_SEC_READ_EXIT();
  }

  return rc;
}

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
RC_t nimUpdateIntfPhyCapability(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  uint32  state;
   uint64 phyCapability;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
            "Incorrect phase %d for updating PHY capabilities - intIfNum %d",
            nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      /* We examine the state here to avoid DTL or NIM config issuing errors
         when configuring interfaces that are pre-configured but are not
         physically present in the system. Returning  SUCCESS gives
         a silent failure mode. When the physical interface is attached,
         this code will be called again. */
      state = nimUtilIntfStateGet(intIfNum);
      if ((state !=  INTF_ATTACHED)
          && (state !=  INTF_ATTACHING)
          && (state !=  INTF_DETACHING))
      {
        return  SUCCESS;
      }

      /* TO DO */
    }
  }

  return rc;
}

/*********************************************************************
* @purpose  Gets the link state of the specified interface.
*
* @param    intIfNum    Internal Interface Number
* @param    linkState   pointer to Link State,
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
* @notes    A CPU interface is always link up.
*
* @end
*********************************************************************/
RC_t nimGetIntfLinkState(uint32 intIfNum, uint32 *linkState)
{
  uint32 result;
  RC_t rc =  FAILURE;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for getting interface link state - intIfNum %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();
    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      switch (nimCtlBlk_g->nimPorts[intIfNum].sysIntfType)
      {
        case  PHYSICAL_INTF:
        case  LAG_INTF:
        case  LOGICAL_VLAN_INTF:
        case  LOOPBACK_INTF:
        case  TUNNEL_INTF:
        case  SERVICE_PORT_INTF:
          result = NIM_INTF_ISMASKBITSET(nimCtlBlk_g->linkStateMask, intIfNum);
          if (result !=  NIM_UNUSED_PARAMETER)
          {
            *linkState =  UP;
          }
          else
          {
            *linkState =  DOWN;
          }
          rc =  SUCCESS;
          break;

        case  CPU_INTF:
          *linkState =  UP;
          break;

        default:
          break;
      }
    } /* interface present */

    NIM_CRIT_SEC_READ_EXIT();
  }

  return rc;
}

/*********************************************************************
* @purpose  Gets the admin state of the specified interface.
*
* @param    intIfNum    Internal Interface Number
* @param    adminState  pointer to Admin State,
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
RC_t nimGetIntfAdminState(uint32 intIfNum, uint32 *adminState)
{
  RC_t rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for getting admin state - intIfNum %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();
    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      if(nimCtlBlk_g->nimPorts[intIfNum].diagDisabled ==  TRUE)
      {
        *adminState =  DIAG_DISABLE;
      }
      else
      {
      *adminState = nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.adminState;
    }
    }
    NIM_CRIT_SEC_READ_EXIT();
  }

  return rc;
}

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
 INTF_STATES_t nimGetIntfState(uint32 intIfNum)
{
  return nimUtilIntfStateGet(intIfNum);
}

/*********************************************************************
*
* @purpose  Gets either the system name or alias name
*           of the specified interface, as requested
*
* @param    intIfNum    Internal Interface Number
* @param    nameType    name type requested ( SYSNAME,  ALIASNAME or  NULL)
*                        NULL will return currently configured ifName
* @param    ifName      pointer to Interface Name,
*                       (@b{Returns: NIM_MAX_INTF_NAME byte interface name})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimGetIntfName(uint32 intIfNum, uint32 nameType,  uchar8 *ifName)
{
  RC_t rc =  SUCCESS;

  if (ifName ==  NULL)
  {
    return  FAILURE;
  }

  /*
    Note that if nameType ==  NULL, then the string passed in must be
    at least  NIM_IF_ALIAS_SIZE + 1 in length, as that could be the
    longest string copied to the passed-in string, and can be longer
    than  NIM_IFNAME_SIZE
  */


  if (nimPhaseStatusCheck() !=  TRUE)
  {
    /* Don't log bad phase. Failure return should be enough for caller to
     * discover the problem. May actually be done intentionally, for example
     * by app in WMU processing checkpoint data. */

    osapiSnprintf(ifName,  NIM_IFNAME_SIZE, "[ifName not yet populated(%d)]", intIfNum);
    rc =  FAILURE;
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      if (nameType ==  0)
      {
        if (nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.nameType ==  SYSNAME)
        {
          memcpy(ifName, nimCtlBlk_g->nimPorts[intIfNum].operInfo.ifName,  NIM_IFNAME_SIZE);
        }
        else if (nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.nameType ==  SYSNAME_LONG)
        {
          memcpy(ifName, nimCtlBlk_g->nimPorts[intIfNum].operInfo.ifLongName,  NIM_IFNAME_SIZE);
        }
        else
        {
          memcpy(ifName, nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.ifAlias,  NIM_IF_ALIAS_SIZE+1);
        }
      }
      else if (nameType ==  SYSNAME)
      {
        memcpy(ifName, nimCtlBlk_g->nimPorts[intIfNum].operInfo.ifName,  NIM_IFNAME_SIZE);
      }
      else if (nameType ==  SYSNAME_LONG)
      {
        memcpy(ifName, nimCtlBlk_g->nimPorts[intIfNum].operInfo.ifLongName,  NIM_IFNAME_SIZE);
      }
      else
      {
        memcpy(ifName, nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.ifAlias,  NIM_IF_ALIAS_SIZE+1);
      }
    }
    else
    {
      osapiSnprintf(ifName,  NIM_IFNAME_SIZE, "[ifName not found(%d)]", intIfNum);
    }

    NIM_CRIT_SEC_READ_EXIT();
  }

  return rc;
}

/*********************************************************************
* @purpose  Gets the OPERATIONAL portmode value of the specified interface.
*
* @param    intIfNum       Internal Interface Number
* @param    intfPortMode   pointer to Port Mode,
*                          (@b{Returns:  Interface Port Mode})
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    The value returned may be different from configured value.
*
* @end
*********************************************************************/
RC_t nimIntfPortModeGet(uint32 intIfNum,  portmode_t *ifMode)
{
   RC_t rc =  SUCCESS;
  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for get interface portMode - ifIndex %d",
        nimConfigPhaseGet(),intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();
    IS_INTIFNUM_PRESENT(intIfNum,rc);    
    if (rc ==  SUCCESS)
    {
      if ( TRUE == nimCtlBlk_g->nimPorts[intIfNum].operInfo.ifImmediateExpand)
      {
        *ifMode = NIM_EXP_PORT_MODE_GET(nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.portModeFlags);      
      }
      else
      {
        *ifMode = NIM_EXP_PORT_MODE_GET(nimCtlBlk_g->nimPorts[intIfNum].defaultCfg.portModeFlags);   
      }
    }
    NIM_CRIT_SEC_READ_EXIT();
  }
  return rc;
}

/*********************************************************************
* @purpose  Gets the portmode enable status value of the specified interface.
*
* @param    intIfNum       Internal Interface Number
* @param    ifStatus       portModeStatus value of Interface
*                          (@b{Returns:   0 or 1
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes This can be obtained for any interface as opposed to portMode
*        which is only valid for 40G ports.
*
* @end
*********************************************************************/
RC_t nimIntfPortModeEnableStatusGet(uint32 intIfNum, uint32 *ifStatus)
{
   RC_t rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for get interface portMode Status - ifIndex %d",
        nimConfigPhaseGet(),intIfNum);
  }
  else
  {    
    NIM_CRIT_SEC_READ_ENTER();
    if ( PHYSICAL_INTF != nimCtlBlk_g->nimPorts[intIfNum].sysIntfType) 
    {
      NIM_CRIT_SEC_READ_EXIT();
      *ifStatus = 1;
      return  SUCCESS;
    }
    IS_INTIFNUM_PRESENT(intIfNum,rc);
    if (rc ==  SUCCESS)
    {   
      if ( TRUE == nimCtlBlk_g->nimPorts[intIfNum].operInfo.ifImmediateExpand)
      {
        *ifStatus = NIM_EXP_PORT_MODE_STATUS_GET(nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.portModeFlags);
      } 
      else
      {
        /* If not immediateActive, get the default */
        *ifStatus = NIM_EXP_PORT_MODE_STATUS_GET(nimCtlBlk_g->nimPorts[intIfNum].defaultCfg.portModeFlags);
      }
    }
    NIM_CRIT_SEC_READ_EXIT();
  }

  return rc;
}

/*********************************************************************
* @purpose  Sets the Network Interface Type of the interface
*
* @param    intIfNum    @b{(input)} Internal Interface Number
* @param    nwIntfType  @b{(input)}Network Interface Type
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
RC_t nimNetworkIntfTypeSet(uint32 intIfNum, uint32 nwIntfType)
{
  RC_t rc =  SUCCESS;
   uchar8 ifName[ NIM_IFNAME_SIZE + 1];
  uint32 event = FD_NIM_DEFAULT_NETWORK_INTERFACE_TYPE;
  NIM_EVENT_SPECIFIC_DATA_t eventData;

  memset (&eventData, 0, sizeof(NIM_EVENT_SPECIFIC_DATA_t));

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for setting network interface type - ifIndex %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_WRITE_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS && !NIM_PARM_CANSET(intIfNum,  INTF_PARM_NW_INTF_TYPE))
    {
      rc =  FAILURE;
    }

    NIM_CRIT_SEC_WRITE_EXIT();

    if (rc ==  SUCCESS)
    {
      switch (nwIntfType)
      {
        case  NW_INTF_TYPE_SWITCHPORT:
          event =  ETHERNET_SWITCHPORT;
          break;
        case  NW_INTF_TYPE_NNI:
          event =  NETWORK_INTF_TYPE_NNI;
          break;
        case  NW_INTF_TYPE_UNI_C:
          event =  NETWORK_INTF_TYPE_UNI_C;
          break;
        case  NW_INTF_TYPE_UNI_S:
          event =  NETWORK_INTF_TYPE_UNI_S;
          break;
        default:
          {
            nimGetIntfName(intIfNum,  ALIASNAME, ifName);
             LOGF( LOG_SEVERITY_DEBUG,
                    "Received invalid network interface type %u for interface %s. Set request ignored.",
                    nwIntfType, ifName);
            rc =  FAILURE;
          }
      }
      if (rc ==  SUCCESS)
      {
        NIM_CRIT_SEC_WRITE_ENTER();

        nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.nwIntfType = nwIntfType;
        //nimCtlBlk_g->nimConfigData->cfgHdr.dataChanged =  TRUE;
        NIM_CRIT_SEC_WRITE_EXIT();
      }
    } 
  }

  return rc;
}

/*********************************************************************
* @purpose  Gets the network interface type for a given interface
*
* @param    intIfNum    @b{(input)} Internal Interface Number
* @param    nwIntfType  @b{(output)} pointer to mtu Size,
*                       (@b{Returns: network interface type})
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimNetworkIntfTypeGet(uint32 intIfNum, uint32 *nwIntfType)
{
  RC_t rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for getting interface type - intIfNum %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      *nwIntfType = nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.nwIntfType;
    }

    NIM_CRIT_SEC_READ_EXIT();
  }

  return rc;
}

/*********************************************************************
* @purpose  Sets the ifAlias name of the specified interface.
*
* @param    intIfNum         Internal Interface Number
* @param    ifAlias          pointer to string containing alias name
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
RC_t nimSetIntfifAlias(uint32 intIfNum,  uchar8 *ifAlias)
{
  RC_t rc =  SUCCESS;
   BOOL         isAutoLag =  FALSE;
   INTF_TYPES_t intfType =  INTF_TYPES_INVALID;
   uchar8       ifName[ NIM_IFNAME_SIZE + 1];

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for setting alias - ifIndex %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum,rc);

    NIM_CRIT_SEC_READ_EXIT();

    if (rc ==  SUCCESS)
    {
        NIM_CRIT_SEC_WRITE_ENTER();

        memset(( void * )nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.ifAlias, 0,  NIM_IF_ALIAS_SIZE+1);

        if (strlen(( uchar8*)ifAlias) <=  NIM_IF_ALIAS_SIZE)
          osapiStrncpySafe( nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.ifAlias, ( uchar8*)ifAlias , strlen(( uchar8*)ifAlias));
        else
          osapiStrncpySafe( nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.ifAlias, ( uchar8*)ifAlias, ( NIM_IF_ALIAS_SIZE) );

        nimCtlBlk_g->nimConfigData->cfgHdr.dataChanged =  TRUE;

        NIM_CRIT_SEC_WRITE_EXIT();
    }
  }

  return rc;
}

/*********************************************************************
* @purpose  Gets either the burned-in or locally administered address
*           of the specified interface, as requested
*
* @param    intIfNum    Internal Interface Number
* @param    addrType    address type requested ( SYSMAC_BIA,  SYSMAC_LAA,
*                       or  NULL)  NULL will return currently configured
*                       MAC Address
* @param    macAddr     pointer to MAC Address,
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
RC_t nimGetIntfAddress(uint32 intIfNum, uint32 addrType,  uchar8 *macAddr)
{
  RC_t rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for getting interface address - intIfNum %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      if (addrType ==  0)
      {
        if (nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.addrType ==  SYSMAC_BIA)
        {
          memcpy(macAddr, nimCtlBlk_g->nimPorts[intIfNum].operInfo.macAddr.addr,  MAC_ADDR_LEN);
        }
        else
        {
          memcpy(macAddr, nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.LAAMacAddr.addr,  MAC_ADDR_LEN);
        }
      }
      else if (addrType ==  SYSMAC_BIA)
      {
        memcpy(macAddr, nimCtlBlk_g->nimPorts[intIfNum].operInfo.macAddr.addr,  MAC_ADDR_LEN);
      }
      else
      {
        memcpy(macAddr, nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.LAAMacAddr.addr,  MAC_ADDR_LEN);
      }
    }

    NIM_CRIT_SEC_READ_EXIT();
  }

  return rc;
}

/*********************************************************************
* @purpose  Gets the address type being used, either the burned-in or
*           locally administered address of the specified interface.
*
* @param    intIfNum    Internal Interface Number
* @param    addrType    address type,
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
RC_t nimGetIntfAddrType(uint32 intIfNum, uint32 *addrType)
{
  RC_t rc =  SUCCESS;

  if (nimPhaseStatusCheck() !=  TRUE)
  {
    rc =  FAILURE;
     LOGF( LOG_SEVERITY_DEBUG,
      "Incorrect phase %d for getting interface address type - intIfNum %d",
        nimConfigPhaseGet(), intIfNum);
  }
  else
  {
    NIM_CRIT_SEC_READ_ENTER();

    IS_INTIFNUM_PRESENT(intIfNum,rc);

    if (rc ==  SUCCESS)
    {
      *addrType = nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.addrType;
    }

    NIM_CRIT_SEC_READ_EXIT();
  }

  return rc;
}

