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



#include "pacinfra_common.h"
#include "osapi.h"
#include "defaultconfig.h"
#include "product.h"
#include "nim_data.h"
#include "nim_config.h"
#include "nim_util.h"
#include "platform_config.h"
#include "system_exports.h"
#include "nim_exports.h"

/*********************************************************************
* @purpose  Allocate the memory for the memory copy of the config file
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimConfigPhase2Init(void)
{
  uint32 fileSize = 0;
  RC_t rc  =  SUCCESS;

  fileSize = sizeof(nimConfigData_t) +
             (sizeof(nimCfgPort_t)*(platIntfTotalMaxCountGet()+1)) + sizeof(uint32);

  /* allocate/initialize the memory to store the configuration data
      Not freed during operation */

  nimCtlBlk_g->nimConfigData = (nimConfigData_t *) osapiMalloc( NIM_COMPONENT_ID, fileSize);

  if (nimCtlBlk_g->nimConfigData ==  NULL)
  {
    NIM_LOG_ERROR("NIM:Couldn't allocate memory for the nimConfigData\n");
    return( FAILURE);
  }
  else
  {
    memset((void*)nimCtlBlk_g->nimConfigData, 0,fileSize);
  }

  if ((rc = nimConfigFileOffsetsSet()) ==  SUCCESS)
    rc = nimConfigFileHdrCreate();


  return(rc);
}

/*********************************************************************
* @purpose  initialize the config structures during phase 3 init
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
void nimConfigInit(void)
{

  uint32             headerSize, totalPortsSize,bufferSize;
  RC_t               rc =  FAILURE;

  headerSize = sizeof(nimConfigData_t);
  totalPortsSize = sizeof(nimCfgPort_t) * (platIntfTotalMaxCountGet() + 1);
  bufferSize = headerSize + totalPortsSize + sizeof(uint32);
  /*
   *  allocate memory for the first part of the file to find how
   *  interfaces were last created on this unit
   */

  if (nimCtlBlk_g->nimConfigData !=  NULLPTR)
  {
    /*rc = sysapiCfgFileGet( NIM_COMPONENT_ID, nimCtlBlk_g->nimFileName,
                          ( char8 *)nimCtlBlk_g->nimConfigData, bufferSize,
                          nimCtlBlk_g->nimConfigData->checkSum, NIM_CFG_VER_CURRENT,
                          nimConfigFileDefaultCreate);

    if (rc !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM: Failed to read the config data\n");
    }
    else if ((rc = nimConfigFileOffsetsSet()) !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM: Failed to set the offsets in the config file\n");
    }
    else*/
    {
      /* nothing to do */
      if (nimCtlBlk_g->nimConfigData->numOfInterfaces != platIntfTotalMaxCountGet())
      {
        nimConfigFileDefaultCreate(NIM_CFG_VER_CURRENT);
      }
    }
  }
  else
  {
    NIM_LOG_ERROR("NIM: Memory not created for config file\n");
    /* need an error handler here */
  }

  return;
}

/*********************************************************************
* @purpose  Setup the config file name
*
* @param    void
*
* @returns   SUCCESS  Success or sysapiRegistrGet error code
*
* @notes
*
*
* @end
*
*********************************************************************/
RC_t nimFileSetup(void)
{
  sprintf (( char8 *)nimCtlBlk_g->nimFileName, "%s",
           NIM_CFG_FILE_NAME);

  return( SUCCESS);
}

/*********************************************************************
* @purpose  Saves all nim user config file to NVStore
*
* @param    void
*
* @returns   SUCCESS  if success
* @returns   ERROR    if error from osapiFsWrite
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimSave(void)
{
  uint32 rc= SUCCESS;

  return(rc);
}

/*********************************************************************
* @purpose  Checks if nim user config data has changed
*
* @param    void
*
* @returns   TRUE or  FALSE
*
* @notes    none
*
* @end
*********************************************************************/
 BOOL nimHasDataChanged(void)
{
  if (nimPhaseStatusCheck() !=  TRUE) return( FALSE);

  return(nimCtlBlk_g->nimConfigData->cfgHdr.dataChanged);
}

void nimResetDataChanged(void)
{
  nimCtlBlk_g->nimConfigData->cfgHdr.dataChanged =  FALSE;
  return;
}

/*********************************************************************
* @purpose  Creates an unique interface id
*
* @param    usp         @b{(input)}  Pointer to nimUSP_t structure
* @param    configId    @b{(output)} Pointer to the new config ID
*
* @returns   SUCCESS or  FALIURE
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimConfigIdCreate(nimUSP_t *usp, nimConfigID_t *configId)
{
  if (nimPhaseStatusCheck() !=  TRUE) return( ERROR);

  memset((void*)configId,0,sizeof(nimConfigID_t));

  configId->configSpecifier.usp.unit = usp->unit;
  configId->configSpecifier.usp.slot = usp->slot;
  configId->configSpecifier.usp.port = usp->port;

  return( SUCCESS);
}

/*********************************************************************
* @purpose  Migrate the config data
*
* @param    savedVersion  @b{(input)} version of the present config file
* @param    newVersion    @b{(input)} version to convert to
* @param    buffer        @b{(input)} the buffer of the cfg file
*
* @returns   SUCCESS or  FALIURE
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
void nimConfigConvert(uint32 savedVersion,uint32 newVersion, char8 *buffer)
{

  /*
   *  allocate memory for the first part of the file to find how
   *  interfaces were last created on this unit
   */

  switch (savedVersion)
  {
    case NIM_CFG_VER_1:
    default:
      switch (newVersion)
      {
        case NIM_CFG_VER_2:
        default:

          nimConfigFileDefaultCreate(NIM_CFG_VER_CURRENT);

          break;
      }
      break;
  }

  nimCtlBlk_g->nimConfigData->cfgHdr.dataChanged =  TRUE;


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
RC_t nimConfigSet(nimIntf_t *port, uint32 cfgVersion)
{
  nimCfgPort_t    *localConfigData;
  nimConfigID_t   configInterfaceId;
  uint32        numOfInterfaces;
   BOOL          configDataFound =  FALSE;
  uint32        counter;
  RC_t   rc =  SUCCESS;

  if ((nimCtlBlk_g ==  NULLPTR) || (nimCtlBlk_g->nimConfigData ==  NULLPTR))
  {
    NIM_LOG_ERROR("NIM: Control block or config data not valid\n");
    rc = ( ERROR);
  }
  else
  {

    localConfigData = nimCtlBlk_g->nimConfigData->cfgPort;

    numOfInterfaces = nimCtlBlk_g->nimConfigData->numOfInterfaces;

    NIM_CONFIG_ID_COPY(&configInterfaceId ,&port->configInterfaceId);

    /* see if the config for the interface exists in the file */
    for (counter = 0; counter <= numOfInterfaces; counter++)
    {
      if (NIM_CONFIG_ID_IS_EQUAL(&localConfigData[counter].configInterfaceId,&configInterfaceId))
      {
        NIM_EXP_PORT_DBG("For interface %d port mode in startup cfg %d %d\n", 
                         port->intfNo,             
                         NIM_EXP_PORT_MODE_GET(localConfigData[counter].cfgInfo.portModeFlags),
                         NIM_EXP_PORT_MODE_STATUS_GET(localConfigData[counter].cfgInfo.portModeFlags));
        memcpy(&port->configPort,&localConfigData[counter],sizeof(nimCfgPort_t));
        configDataFound =  TRUE;
        break;
      }
    }

    if (configDataFound ==  FALSE)
    {
      /* use the default config as determined before */
      memcpy(&port->configPort.cfgInfo,&port->defaultCfg,sizeof(nimIntfConfig_t));
      NIM_CONFIG_ID_COPY(&port->configPort.configInterfaceId,&port->configInterfaceId);
      port->configPort.configIdMaskOffset = port->intfNo;
#if  FEAT_DYNAMIC_PORTS
      /* Since we dont replace the defaultCfg values when dynamic caps
         are on, we have to ensure that the static defaultCfg is not
         copied to config values if dynamic caps are present. Another
         approach would be to replace defaultCfg but in that case we
         have to replace the defaults back when dynamic cap is turned
         off. */
      if (port->dynamicCap)
      {
        DYN_PORT_TRACE("Updating config with dynamic defaults because dynamic Caps are present\n");
        port->configPort.cfgInfo.ifSpeed = port->capabilityCfg.ifSpeed;
        port->configPort.cfgInfo.negoCapabilities = port->capabilityCfg.negoCapabilities;
        DYN_PORT_TRACE("New default speed 0x%x, AutoNegCap 0x%x\n",
                       port->configPort.cfgInfo.ifSpeed,
                       port->configPort.cfgInfo.negoCapabilities);
      }
#endif
    }
  }

  return(rc);
}

/*********************************************************************
* @purpose  apply the config to the system
*
* @param    intIfNum    @b{(input)}   The intenal interface to apply
*
* @returns   SUCCESS or  FALIURE
*
* @notes
*
* @end
*********************************************************************/

RC_t nimIntfConfigApply(uint32 intIfNum)
{
  uint32 rc =  SUCCESS;
  return rc;
}

/*********************************************************************
* @purpose  Get the default config for the interface
*
* @param    intfDescr   @b{(input)}   A description of the interface being created
* @param    defaultCfg  @b{(output)}  The created config
*
* @returns   SUCCESS or  FALIURE
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
RC_t nimConfigDefaultGet(nimIntfDescr_t *intfDescr, nimIntfConfig_t *defaultCfg)
{
  RC_t rc =  SUCCESS;

  memset(defaultCfg->LAAMacAddr.addr, 0, 6);

  defaultCfg->addrType    = FD_NIM_MACTYPE;

  osapiStrncpySafe(defaultCfg->ifAlias, FD_NIM_IF_ALIAS, sizeof(defaultCfg->ifAlias));

  defaultCfg->trapState   = FD_NIM_TRAP_STATE;

  defaultCfg->encapsType  = FD_NIM_ENCAPSULATION_TYPE;

  defaultCfg->mgmtAdminState  = FD_NIM_ADMIN_STATE;
  defaultCfg->softShutdownState  = FD_NIM_SOFT_SHUT_STATE;
  defaultCfg->adminState  = FD_NIM_ADMIN_STATE;

  defaultCfg->nameType    = FD_NIM_NAME_TYPE;

  defaultCfg->ipMtu = FD_NIM_DEFAULT_MTU_SIZE;

  defaultCfg->cfgMaxFrameSize = nimCtlBlk_g->nimSystemMtu;

  defaultCfg->fecMode =  DISABLE;

  if ((intfDescr->settableParms &  INTF_PARM_FEC_MODE) ==  INTF_PARM_FEC_MODE)
  {
    defaultCfg->fecMode = FD_NIM_DEFAULT_INTERFACE_FORWARD_ERROR_CORRECTION_MODE;
  }

  defaultCfg->ltMode =  DISABLE;

  if ((intfDescr->settableParms &  INTF_PARM_LT_MODE) ==  INTF_PARM_LT_MODE)
  {
    defaultCfg->ltMode = FD_NIM_DEFAULT_LINK_TRAINING_MODE;
  }

  if (  PHY_CAP_PORTSPEED_AUTO_NEG_CHECK(intfDescr->phyCapability) &&
      ! PHY_CAP_AUTO_CONFIG_CHECK(intfDescr->phyCapability))
  {
#ifdef  NIM_AUTONEG_SUPPORT_DISABLE_DEFAULT
    if ((intfDescr->ianaType ==  IANA_100G_ETHERNET) ||
        (intfDescr->ianaType ==  IANA_50G_ETHERNET))
    {
      defaultCfg->negoCapabilities =  0;
      defaultCfg->cfgNegoCapabilities =  0;
    }
    else
    {
      defaultCfg->negoCapabilities =   PORT_NEGO_CAPABILITY_ALL;
      defaultCfg->cfgNegoCapabilities =   PORT_NEGO_CAPABILITY_ALL;
    }
#else
    defaultCfg->negoCapabilities =   PORT_NEGO_CAPABILITY_ALL;
    defaultCfg->cfgNegoCapabilities =   PORT_NEGO_CAPABILITY_ALL;
#endif
  }
  else
  {
    defaultCfg->negoCapabilities =  0;
    defaultCfg->cfgNegoCapabilities =  0;
  }
  
  if ( PHY_CAP_COMBO_PREFERENCE_CHECK(intfDescr->phyCapability))
  {
    defaultCfg->forceMedium = FD_NIM_DEFAULT_COMBO_FORCE;
    defaultCfg->comboPref   = FD_NIM_DEFAULT_COMBO_PREFER;
  }

  switch (intfDescr->ianaType)
  {
    case  IANA_FAST_ETHERNET:
      defaultCfg->ifSpeed = FD_NIM_FAST_ENET_SPEED;
      break;

    case  IANA_FAST_ETHERNET_FX:
      defaultCfg->ifSpeed = FD_NIM_FAST_ENET_FX_SPEED;
      break;

    case  IANA_GIGABIT_ETHERNET:
      if(!( PHY_CAP_PORTSPEED_FULL_100_CHECK(intfDescr->phyCapability)))
      {
        defaultCfg->ifSpeed = FD_NIM_GIG_ENET_SPEED;
      }
      else
      {
        defaultCfg->ifSpeed = FD_NIM_FAST_ENET_SPEED;
      }
      break;

    case  IANA_2P5G_ETHERNET:
      if ( PHY_CAP_PORTSPEED_AUTO_NEG_CHECK(intfDescr->phyCapability))
      {
        defaultCfg->ifSpeed = FD_NIM_2P5G_ENET_SPEED;
      }
      else
      {
        defaultCfg->ifSpeed =  FD_NIM_NO_NEG_2P5G_ENET_SPEED;
      }
      break;

    case  IANA_5G_ETHERNET:
      if ( PHY_CAP_PORTSPEED_AUTO_NEG_CHECK(intfDescr->phyCapability))
      {
        defaultCfg->ifSpeed = FD_NIM_5G_ENET_SPEED;
      }
      else
      {
        defaultCfg->ifSpeed =  FD_NIM_NO_NEG_5G_ENET_SPEED;
      }
      break;

    case  IANA_10G_ETHERNET:
      if ( PHY_CAP_PORTSPEED_AUTO_NEG_CHECK(intfDescr->phyCapability))
      {
        defaultCfg->ifSpeed = FD_NIM_10G_ENET_SPEED;
      }
      else
      {
        defaultCfg->ifSpeed =  FD_NIM_NO_NEG_10G_ENET_SPEED;
      }
      break;

    case  IANA_20G_ETHERNET:
      defaultCfg->ifSpeed = FD_NIM_20G_ENET_SPEED;
      break;

    case  IANA_25G_ETHERNET:
      defaultCfg->ifSpeed = FD_NIM_25G_ENET_SPEED;
      break;

    case  IANA_40G_ETHERNET:
      defaultCfg->ifSpeed = FD_NIM_40G_ENET_SPEED;
      break;

    case  IANA_50G_ETHERNET:
      defaultCfg->ifSpeed = FD_NIM_50G_ENET_SPEED;
      break;

    case  IANA_100G_ETHERNET:
      defaultCfg->ifSpeed = FD_NIM_100G_ENET_SPEED;
      break;

    case  IANA_200G_ETHERNET:
      defaultCfg->ifSpeed = FD_NIM_200G_ENET_SPEED;
      break;

    case  IANA_400G_ETHERNET:
      defaultCfg->ifSpeed = FD_NIM_400G_ENET_SPEED;
      break;

    case  IANA_LAG:
      defaultCfg->trapState           = FD_DOT3AD_LINK_TRAP_MODE;
      defaultCfg->adminState          = FD_DOT3AD_ADMIN_MODE;
      defaultCfg->ifSpeed             =  PORTCTRL_PORTSPEED_LAG;
      /* Negotiation capabilities not applicable to LAGs. The following also disables auto-negotiation. */
      defaultCfg->negoCapabilities    = 0;
      break;

    case  IANA_L2_VLAN:
      defaultCfg->adminState          =  ENABLE;
      defaultCfg->trapState           =  DISABLE;
      defaultCfg->ifSpeed             = FD_DOT1Q_DEFAULT_VLAN_INTF_SPEED;
      /* Negotiation capabilities not applicable to VLANs. The following also disables auto-negotiation. */
      defaultCfg->negoCapabilities    = 0;
      break;

    case  IANA_OTHER_CPU:
      defaultCfg->ifSpeed = FD_NIM_OTHER_PORT_TYPE_SPEED;
      break;

    case  IANA_OTHER_SERV_PORT:
      defaultCfg->ifSpeed = FD_NIM_OTHER_PORT_TYPE_SPEED;
      break;

    default:
      defaultCfg->ifSpeed = FD_NIM_OTHER_PORT_TYPE_SPEED;
      break;
  }

  defaultCfg->nwIntfType = FD_NIM_DEFAULT_NETWORK_INTERFACE_TYPE;
  return rc;
}

/*********************************************************************
* @purpose  Sets the offsets for ports and crc in the config struct
*
* @param    none
*
* @returns   SUCCESS or  FAILURE
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
RC_t nimConfigFileOffsetsSet()
{
  RC_t rc =  SUCCESS;
  uint32 fileSize = 0;
   uchar8   *nimFile;
  nimConfigData_t *tmpCfg;

  /* sizeof the config structure + the malloc'd data + CRC */
  fileSize = sizeof(nimConfigData_t) +
             (sizeof(nimCfgPort_t)*(platIntfTotalMaxCountGet()+1)) + sizeof(uint32);

  NIM_CRIT_SEC_WRITE_ENTER();

  do
  {
    if (nimCtlBlk_g->nimConfigData ==  NULL)
    {
      NIM_LOG_ERROR("NIM: Config file not allocated during init\n");
      rc =  FAILURE;
      break;
    }

    nimFile = ( uchar8 *)nimCtlBlk_g->nimConfigData;

    tmpCfg = nimCtlBlk_g->nimConfigData;

    /* set the pointer to the cfgPort in the flat array */
    tmpCfg->cfgPort = (nimCfgPort_t *) (nimFile + sizeof(nimConfigData_t));

    /* set the pointer to the checksum in the flat array */
    tmpCfg->checkSum = (uint32 *) (nimFile + fileSize - sizeof(uint32));


  } while ( 0 );

  NIM_CRIT_SEC_WRITE_EXIT();

  return rc;


}


/*********************************************************************
* @purpose  Sets the config file header
*
* @param    none
*
* @returns   SUCCESS or  FALIURE
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
RC_t nimConfigFileHdrCreate()
{
  RC_t rc =  SUCCESS;
   fileHdr_t  *myHdr;

  NIM_CRIT_SEC_WRITE_ENTER();

  do
  {
    if (nimCtlBlk_g ==  NULLPTR)
    {
      rc =  FAILURE;
      NIM_LOG_ERROR("NIM: nimCtlBlk_g not allocated\n");
      break;
    }
    else if (nimCtlBlk_g->nimConfigData ==  NULLPTR)
    {
      rc =  FAILURE;
      NIM_LOG_ERROR("NIM: Config buffer not allocated\n");
      break;
    }

    myHdr = &nimCtlBlk_g->nimConfigData->cfgHdr;

    memset((void *)myHdr,0,sizeof( fileHdr_t));

    myHdr->version = NIM_CFG_VER_CURRENT;

    osapiStrncpySafe(myHdr->filename,NIM_CFG_FILE_NAME, MAX_FILENAME - 1);

    myHdr->componentID =  NIM_COMPONENT_ID;

    /* sizeof the config structure + the malloc'd data + CRC - File Hdr*/
    myHdr->length = sizeof(nimConfigData_t) +
                    (sizeof(nimCfgPort_t)*(platIntfTotalMaxCountGet()+1)) +
                    sizeof(uint32) ;

    nimCtlBlk_g->nimConfigData->numOfInterfaces = platIntfTotalMaxCountGet();


  } while ( 0 );

  NIM_CRIT_SEC_WRITE_EXIT();

  return rc;


}

/*********************************************************************
* @purpose  Get the default config for the interface
*
* @param    version   @b{(input)} The version to create a default for
*
* @returns  void
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
void nimConfigFileDefaultCreate(uint32 version)
{
  uint32 nimHeaderSize,totalPortsSize,crcSize;

  do
  {
    nimHeaderSize = sizeof(nimConfigData_t);
    totalPortsSize = sizeof(nimCfgPort_t) * (platIntfTotalMaxCountGet() + 1);
    crcSize = sizeof(uint32);
    NIM_CRIT_SEC_WRITE_ENTER();
    memset((void*)nimCtlBlk_g->nimConfigData,0,nimHeaderSize + totalPortsSize + crcSize);
    /*nimCtlBlk_g->nimConfigData->nimDDisableAutorecoveryTimeout = FD_NIM_DEFAULT_DDISABLE_AUTORECOVERY_TIMEOUT;
    nimCtlBlk_g->nimConfigData->nimDDisableStormControlAutorecoveryTimeout = FD_NIM_DEFAULT_DDISABLE_STORM_CONTROL_AUTORECOVERY_TIMEOUT;
    nimCtlBlk_g->nimConfigData->nimDDisableAutorecoveryStatus = FD_NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS;*/
    NIM_CRIT_SEC_WRITE_EXIT();
    nimConfigFileOffsetsSet();

    nimConfigFileHdrCreate();
    /* No need to protect the dataChanged as we have set it to 0
     * and the above 2 function do not modify it */
    nimCtlBlk_g->nimConfigData->cfgHdr.dataChanged =  FALSE;

  } while ( 0 );

  return;
}
