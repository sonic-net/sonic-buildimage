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
#include "sysapi_hpc.h"
#include "osapi.h"
#include "nim_data.h"
#include "platform_config.h"
#include "log.h"
#include "utils_api.h"
#include "nim_events.h"
#include "nim_util.h"
#include "nim_startup.h"
#include "nim_debug.h"
//#include "cnfgr_sid.h"

#include <string.h>

static  SYSAPI_HPC_PORT_DESCRIPTOR_t portData =
{
   IANA_GIGABIT_ETHERNET,
   PORTCTRL_PORTSPEED_FULL_10GSX,
   PHY_CAP_PORTSPEED_ALL,
   /* MTRJ,*/
   PORT_FEC_DISABLE,
   CAP_FEC_NONE
};


extern RC_t   nimCmgrNewIntfChangeCallback(uint32 unit, uint32 slot, uint32 port,
                                              uint32 cardType, PORT_EVENTS_t event,
                                              SYSAPI_HPC_PORT_DESCRIPTOR_t *portData,
                                                             enetMacAddr_t *macAddr);

RC_t nimDebugPortCreate(uint32 unit,uint32 slot,uint32 port)
{
  RC_t   rc;
   enetMacAddr_t macAddr;
  macAddr.addr[0] = 01;
  macAddr.addr[0] = 02;
  macAddr.addr[0] = 03;
  macAddr.addr[0] = 04;
  macAddr.addr[0] = 05;
  macAddr.addr[0] = 06;


  rc = nimCmgrNewIntfChangeCallback(unit, slot, port, 0, CREATE,
                                 &portData, &macAddr);
  return(rc);
}

#if 0
 uchar8 *
nimDebugIntfTypeToString(uint32 sysIntfType)
{
   uchar8 *pName = "UNKNOWN";

  switch (sysIntfType)
  {
    case  PHYSICAL_INTF:
      pName = " PHYSICAL_INTF";
      break;
    case  STACK_INTF:
      pName = " STACK_INTF";
      break;
    case  CPU_INTF:
      pName = " CPU_INTF";
      break;
    case  MAX_PHYS_INTF_VALUE:
      pName = " MAX_PHYS_INTF_VALUE";
      break;
    case  LAG_INTF:
      pName = " LAG_INTF";
      break;
    case  UNUSED1_INTF:
      pName = " UNUSED1_INTF";
      break;
    case  LOGICAL_VLAN_INTF:
      pName = " LOGICAL_VLAN_INTF";
      break;
    case  LOOPBACK_INTF:
      pName = " LOOPBACK_INTF";
      break;
    case  TUNNEL_INTF:
      pName = " TUNNEL_INTF";
      break;
    case  SERVICE_PORT_INTF:
      pName = " SERVICE_PORT_INTF";
      break;
    case  MAX_INTF_TYPE_VALUE:
      pName = " MAX_INTF_TYPE_VALUE";
      break;
    default:
      break;
  }

  return pName;
}

void nimDebugConfigIdPrint(nimConfigID_t *pCfgID)
{
  sysapiPrintf("configInterfaceId = type:%s, ",
                nimDebugIntfTypeToString(pCfgID->type));
  switch (pCfgID->type)
  {
    case  LOGICAL_VLAN_INTF:
      sysapiPrintf("vlanId:%u\n", pCfgID->configSpecifier.vlanId);
      break;
    case  LAG_INTF:
      sysapiPrintf("dot3adIntf:%u\n", pCfgID->configSpecifier.dot3adIntf);
      break;
    case  LOOPBACK_INTF:
      sysapiPrintf("loopbackId:%u\n", pCfgID->configSpecifier.loopbackId);
      break;
    case  TUNNEL_INTF:
      sysapiPrintf("tunnelId:%u\n", pCfgID->configSpecifier.tunnelId);
      break;
    case  SERVICE_PORT_INTF:
      sysapiPrintf("servicePortId:%u\n", pCfgID->configSpecifier.servicePortId);
      break;
    default:
      sysapiPrintf("usp:%d.%d.%d\n", pCfgID->configSpecifier.usp.unit,
                   pCfgID->configSpecifier.usp.slot,
                   pCfgID->configSpecifier.usp.port);
      break;
  }
}

void nimDebugDynamicCapPort(uint32 intIfNum)
{
  nimIntfDynConfig_t* dyncap =  &(nimCtlBlk_g->nimPorts[intIfNum].capabilityCfg);
  if ( FALSE == nimCtlBlk_g->nimPorts[intIfNum].dynamicCap) 
  {
    sysapiPrintf("For intf %d, dynamic capabilities are off\n", intIfNum);
    return;
  }
  sysapiPrintf("Last configured AutoNeg Capabilities - 0x%x\n",
               nimCtlBlk_g->nimPorts[intIfNum].configPort.cfgInfo.cfgNegoCapabilities);
  sysapiPrintf("**** Dynamic Capabilities ********\n");
  sysapiPrintf("For intf %d, dynspeed = 0x%x, AutoNegCap = 0x%x, AutoNeg State %d, Phy Cap = 0x%llx\n",
               intIfNum,
               dyncap->ifSpeed, dyncap->negoCapabilities, 
               dyncap->ifAutoneg,
               dyncap->dynCapabilities);
  return;
  
}

void nimDebugCfgPort(nimCfgPort_t *configPort)
{
  uint32 i;
  nimConfigID_t cfgID;
  nimUSP_t usp;
  if (configPort !=  NULL)
  {

    sysapiPrintf("*********** Config Port **************\n");

    NIM_CONFIG_ID_COPY(&cfgID,&configPort->configInterfaceId);
    nimDebugConfigIdPrint(&cfgID);

    memset( (void *)&usp,0, sizeof(nimUSP_t));
    if ( nimUspFromConfigIDGet(&cfgID, &usp) ==  SUCCESS)
    {
      sysapiPrintf("usp = %d.%d.%d\n", usp.unit, usp.slot, usp.port);
    }

    sysapiPrintf("configMaskOffset = %d\n",configPort->configIdMaskOffset);

    sysapiPrintf("LAA = ");
    for (i = 0;i < 5 ;i++)
    {
      sysapiPrintf("%02x:",configPort->cfgInfo.LAAMacAddr.addr[i]);
    }

    sysapiPrintf("%02x\n",configPort->cfgInfo.LAAMacAddr.addr[5]);

    sysapiPrintf("addrType = %s\n",(configPort->cfgInfo.addrType ==  SYSMAC_BIA)?" SYSMAC_BIA":" SYSMAC_LAA");

    sysapiPrintf("ifAlias = %s\n", configPort->cfgInfo.ifAlias);

    sysapiPrintf("nameType = %s\n",(configPort->cfgInfo.nameType ==  ALIASNAME)?" ALIASNAME":" SYSNAME");

    switch (configPort->cfgInfo.ifSpeed)
    {
      case  PORTCTRL_PORTSPEED_AUTO_NEG:
        sysapiPrintf("ifSpeed =  PORTCTRL_PORTSPEED_AUTO_NEG\n");
        break;
      case  PORTCTRL_PORTSPEED_HALF_100TX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_HALF_100TX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_100TX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_100TX\n");
        break;
      case  PORTCTRL_PORTSPEED_HALF_10T:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_HALF_10T\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_10T:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_10T\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_100FX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_100FX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_1000SX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_1000SX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_2P5FX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_2P5FX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_5FX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_5FX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_10GSX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_10GSX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_20GSX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_20GSX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_25GSX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_25GSX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_40GSX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_40GSX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_50GSX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_50GSX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_100GSX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_100GSX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_200GSX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_200GSX\n");
        break;
      case  PORTCTRL_PORTSPEED_FULL_400GSX:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_FULL_400GSX\n");
        break;
      case  PORTCTRL_PORTSPEED_AAL5_155:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_AAL5_155\n");
        break;
      case  PORTCTRL_PORTSPEED_LAG:
        sysapiPrintf("ifSpeed =   PORTCTRL_PORTSPEED_LAG\n");
        break;
      default:
        sysapiPrintf("ifSpeed = 0X%X\n", configPort->cfgInfo.ifSpeed);
        break;
    }

    if (configPort->cfgInfo.negoCapabilities == 0)
    {
      sysapiPrintf("negoCapabilities = DISABLED\n");
    }
    else
    {
      sysapiPrintf("negoCapabilities = ");
      if (configPort->cfgInfo.negoCapabilities &  PORT_NEGO_CAPABILITY_HALF_10)
        sysapiPrintf(" PORT_NEGO_CAPABILITY_HALF_10 |");
      if (configPort->cfgInfo.negoCapabilities &  PORT_NEGO_CAPABILITY_FULL_10)
        sysapiPrintf(" PORT_NEGO_CAPABILITY_FULL_10 |");
      if (configPort->cfgInfo.negoCapabilities &  PORT_NEGO_CAPABILITY_HALF_100)
        sysapiPrintf(" PORT_NEGO_CAPABILITY_HALF_100 |");
      if (configPort->cfgInfo.negoCapabilities &  PORT_NEGO_CAPABILITY_FULL_100)
        sysapiPrintf(" PORT_NEGO_CAPABILITY_FULL_100 |");
      if (configPort->cfgInfo.negoCapabilities &  PORT_NEGO_CAPABILITY_FULL_1000)
        sysapiPrintf(" PORT_NEGO_CAPABILITY_FULL_1000 |");
      if (configPort->cfgInfo.negoCapabilities &  PORT_NEGO_CAPABILITY_FULL_2P5G)
        sysapiPrintf(" PORT_NEGO_CAPABILITY_FULL_2P5G |");
      if (configPort->cfgInfo.negoCapabilities &  PORT_NEGO_CAPABILITY_FULL_5G)
        sysapiPrintf(" PORT_NEGO_CAPABILITY_FULL_5G |");
      if (configPort->cfgInfo.negoCapabilities &  PORT_NEGO_CAPABILITY_FULL_10G)
        sysapiPrintf(" PORT_NEGO_CAPABILITY_FULL_10G | ");
      if (configPort->cfgInfo.negoCapabilities &  PORT_NEGO_CAPABILITY_ALL)
        sysapiPrintf(" PORT_NEGO_CAPABILITY_ALL");

      sysapiPrintf("\n");
    }

    sysapiPrintf("mgmtAdminState = %s\n",(configPort->cfgInfo.mgmtAdminState ==  ENABLE)?"ENABLE":"DISABLED");
    sysapiPrintf("adminState = %s\n",(configPort->cfgInfo.adminState ==  ENABLE)?"ENABLE":"DISABLED");
#if  FEAT_SOFT_SHUTDOWN
    sysapiPrintf("softShutdownState = %s\n",(configPort->cfgInfo.softShutdownState ==  ENABLE)?"ENABLE":"DISABLED");
#endif
    sysapiPrintf("trapState = %s\n",(configPort->cfgInfo.trapState ==  ENABLE)?"ENABLE":"DISABLED");

    sysapiPrintf("MTU = %d\n", configPort->cfgInfo.ipMtu);

    sysapiPrintf("Encapsulation type = %d\n", configPort->cfgInfo.encapsType);
    sysapiPrintf("Ptr to this nimCfgPort_t = %p\n",configPort);   
  }
}

void nimDebugPortIntIfNum(nimIntf_t *port)
{
  uint32 i;
  nimConfigID_t cfgID;


  if (port !=  NULL)
  {
    NIM_CONFIG_ID_COPY(&cfgID,&port->configInterfaceId);
    sysapiPrintf("Present = %d \n",port->present);

    sysapiPrintf("intfNo = %d\n",port->intfNo);

    sysapiPrintf("runTimeMaskId = %d \n",port->runTimeMaskId);

    nimDebugConfigIdPrint(&cfgID);

    sysapiPrintf("sysIntfType = %s\n",
                 nimDebugIntfTypeToString(port->sysIntfType));

    sysapiPrintf("ifIndex = %d \n", port->ifIndex);

    sysapiPrintf("ifDescr = %s \n",port->operInfo.ifDescr);

    switch (port->operInfo.ianaType)
    {
      case  IANA_OTHER:
        sysapiPrintf("ifType =   IANA_OTHER\n");
        break;
      case  IANA_ETHERNET:
        sysapiPrintf("ifType =   IANA_ETHERNET\n");
        break;
      case  IANA_AAL5:
        sysapiPrintf("ifType =   IANA_AAL5\n");
        break;
      case  IANA_PROP_VIRTUAL:
        sysapiPrintf("ifType =   IANA_PROP_VIRTUAL\n");
        break;
      case  IANA_FAST_ETHERNET:
        sysapiPrintf("ifType =   IANA_FAST_ETHERNET\n");
        break;
      case  IANA_FAST_ETHERNET_FX:
        sysapiPrintf("ifType =   IANA_FAST_ETHERNET_FX\n");
        break;
      case  IANA_GIGABIT_ETHERNET:
        sysapiPrintf("ifType =   IANA_GIGABIT_ETHERNET\n");
        break;
      case  IANA_2P5G_ETHERNET:
        sysapiPrintf("ifType =   IANA_2P5G_ETHERNET\n");
        break;
      case  IANA_5G_ETHERNET:
        sysapiPrintf("ifType =   IANA_5G_ETHERNET\n");
        break;
      case  IANA_10G_ETHERNET:
        sysapiPrintf("ifType =   IANA_10G_ETHERNET\n");
        break;
      case  IANA_20G_ETHERNET:
        sysapiPrintf("ifType =   IANA_20G_ETHERNET\n");
        break;
      case  IANA_25G_ETHERNET:
        sysapiPrintf("ifType =   IANA_25G_ETHERNET\n");
        break;
      case  IANA_40G_ETHERNET:
        sysapiPrintf("ifType =   IANA_40G_ETHERNET\n");
        break;
      case  IANA_50G_ETHERNET:
        sysapiPrintf("ifType =   IANA_50G_ETHERNET\n");
        break;
      case  IANA_100G_ETHERNET:
        sysapiPrintf("ifType =   IANA_100G_ETHERNET\n");
        break;
      case  IANA_200G_ETHERNET:
        sysapiPrintf("ifType =   IANA_200G_ETHERNET\n");
        break;
      case  IANA_400G_ETHERNET:
        sysapiPrintf("ifType =   IANA_400G_ETHERNET\n");
        break;
      case  IANA_L2_VLAN:
        sysapiPrintf("ifType =   IANA_L2_VLAN\n");
        break;
      case  IANA_LAG:
        sysapiPrintf("ifType =   IANA_LAG_DESC\n");
        break;
      case  IANA_LOGICAL_DESC:
        sysapiPrintf("ifType =   IANA_LOGICAL_DESC\n");
        break;
      case  IANA_SOFTWARE_LOOPBACK:
        sysapiPrintf("ifType =   IANA_SOFTWARE_LOOPBACK\n");
        break;
      case  IANA_TUNNEL:
        sysapiPrintf("ifType =   IANA_TUNNEL\n");
        break;
      case  IANA_OTHER_SERV_PORT:
        sysapiPrintf("ifType =   IANA_OTHER_SERV_PORT\n");
        break;
      default:
        sysapiPrintf("ifType =  UNKNOWN %d \n", port->operInfo.ianaType);
        break;
    }

    sysapiPrintf("BIA = ");
    for (i = 0; i < ( ENET_MAC_ADDR_LEN - 1); i++)
    {
      sysapiPrintf("%02x:",port->operInfo.macAddr.addr[i]);
    }
    sysapiPrintf("%02x\n",port->operInfo.macAddr.addr[i]);

    sysapiPrintf("L3MAC = ");
    for (i = 0; i < ( ENET_MAC_ADDR_LEN - 1); i++)
    {
      sysapiPrintf("%02x:",port->operInfo.l3MacAddr.addr[i]);
    }
    sysapiPrintf("%02x\n",port->operInfo.l3MacAddr.addr[i]);

    sysapiPrintf("resetTime = %d\n",port->resetTime);

    sysapiPrintf("linkChangeTime = %d\n",port->linkChangeTime);

    sysapiPrintf("ifName = %s\n", port->operInfo.ifName);

    sysapiPrintf("USP = %d.%d.%d\n",port->usp.unit,port->usp.slot,port->usp.port);

    sysapiPrintf("PHY CAP = ");
    if ( PHY_CAP_PORTSPEED_AUTO_NEG_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_AUTO_NEG |\n");
    if ( PHY_CAP_PORTSPEED_HALF_10_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_HALF_10 |\n");
    if ( PHY_CAP_PORTSPEED_FULL_10_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_10 |\n");
    if ( PHY_CAP_PORTSPEED_HALF_100_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_HALF_100 |\n");
    if ( PHY_CAP_PORTSPEED_FULL_100_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_100 |\n");
    if ( PHY_CAP_PORTSPEED_HALF_1000_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_HALF_1000 |\n");
    if ( PHY_CAP_PORTSPEED_FULL_1000_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_1000 |\n");
    if ( PHY_CAP_PORTSPEED_FULL_2P5G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_2P5G |\n");
    if ( PHY_CAP_PORTSPEED_FULL_5G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_5G |\n");
    if ( PHY_CAP_PORTSPEED_FULL_10G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_10G |\n");
    if ( PHY_CAP_PORTSPEED_FULL_20G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_20G |\n");
    if ( PHY_CAP_PORTSPEED_FULL_25G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_25G |\n");
    if ( PHY_CAP_PORTSPEED_FULL_40G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_40G |\n");
    if ( PHY_CAP_PORTSPEED_FULL_50G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_50G |\n");
    if ( PHY_CAP_PORTSPEED_FULL_100G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_100G |\n");
    if ( PHY_CAP_PORTSPEED_FULL_200G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_200G |\n");
    if ( PHY_CAP_PORTSPEED_FULL_400G_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_FULL_400G |\n");
    if ( PHY_CAP_PORTSPEED_SFP_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_SFP |\n");
    if ( PHY_CAP_PORTSPEED_SFP_DETECT_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_PORTSPEED_SFP_DETECT |\n");
    if ( PHY_CAP_POE_PSE_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_POE_PSE |\n");
    if ( PHY_CAP_POE_PD_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_POE_PD |\n");
    if ( PHY_CAP_INTERNAL_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_INTERNAL \n");
    if ( PHY_CAP_NATIVE_EEE_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_NATIVE_EEE \n");
    if ( PHY_CAP_AUTO_EEE_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_AUTO_EEE \n");
    if ( PHY_CAP_DUAL_MODE_SUPPORT_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_DUAL_MODE_SUPPORT \n");
    if ( PHY_CAP_POE_PSE_PLUS_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_POE_PSE_PLUS \n");
    if ( PHY_CAP_POE_PD_PLUS_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_POE_PD_PLUS \n");
    if ( PHY_CAP_ENERGY_DETECT_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_ENERGY_DETECT \n");
    if ( PHY_CAP_AUTO_CONFIG_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_AUTO_CONFIG \n");
    if ( PHY_CAP_UPOE_PSE_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_UPOE_PSE\n");
    if ( PHY_CAP_UPOE_PD_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_UPOE_PD\n");
    if ( PHY_CAP_POE_BT_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_POE_BT\n");
    if (port->operInfo.phyCapability &  PHY_CAP_FIXED_STACK_PORT)
      sysapiPrintf("  PHY_CAP_FIXED_STACK_PORT\n");
    if ( PHY_CAP_DYNAMIC_PORT_CHECK(port->operInfo.phyCapability))
      sysapiPrintf("  PHY_CAP_DYNAMIC_PORT\n");

    if ( PHY_CAP_COMBO_PREFERENCE_CHECK(port->operInfo.phyCapability))
    {
      sysapiPrintf("  PHY_CAP_COMBO_PREFERENCE \n");
      sysapiPrintf("\r\nCombo Preference: %s, %s\r\n",
                    TRUE == port->configPort.cfgInfo.forceMedium ? "forced" : "auto",
                    NIM_COMBO_PREF_SFP == port->configPort.cfgInfo.comboPref ? "sfp" : "rj45");
    }

    switch (port->operInfo.connectorType)
    {
      case  CONNECTOR_NONE:
        sysapiPrintf("connectorType =  CONNECTOR_NONE\n");
        break;
      case  RJ45:
        sysapiPrintf("connectorType =  RJ45\n");
        break;
#if 0
      case  MTRJ:
        sysapiPrintf("connectorType =  MTRJ\n");
        break;
#endif
      case  SCDUP:
        sysapiPrintf("connectorType =  SCDUP\n");
        break;
      case  XAUI:
        sysapiPrintf("connectorType =  XAUI\n");
        break;
      case  CAUI:
        sysapiPrintf("connectorType =  CAUI\n");
        break;
      default:
        sysapiPrintf("connectorType = UNKNOWN\n");
        break;
    }

    sysapiPrintf("maxFrameSize = %d\n",port->operInfo.frameSize.largestFrameSize);

    switch(port->intfState)
    {
      case  INTF_UNINITIALIZED:
      sysapiPrintf("Intf State =  INTF_UNINITIALIZED\n");
      break;
      case  INTF_CREATING:
      sysapiPrintf("Intf State =  INTF_CREATING\n");
      break;
      case  INTF_CREATED:
      sysapiPrintf("Intf State =  INTF_CREATED\n");
      break;
      case  INTF_ATTACHING:
      sysapiPrintf("Intf State =  INTF_ATTACHING\n");
      break;
      case  INTF_ATTACHED:
      sysapiPrintf("Intf State =  INTF_ATTACHED\n");
      break;
      case  INTF_DETACHING:
      sysapiPrintf("Intf State =  INTF_DETACHING\n");
      break;
      case  INTF_DELETING:
      sysapiPrintf("Intf State =  INTF_DELETING\n");
      break;
      default:
        sysapiPrintf("Intf State = Unknown\n");
    }

    sysapiPrintf("Ptr for the nimIntf_t = %p\n",port);
  }

}

void nimDebugPortIntIfShow(uint32 intIfNum)
{
  RC_t rc;
  nimIntf_t port;
   BOOL linkState;
  uint32 intfSpeed;
   char8 *pSpeedStr;

  IS_INTIFNUM_PRESENT(intIfNum,rc);

  if (rc ==  SUCCESS)
  {
    port = nimCtlBlk_g->nimPorts[intIfNum];
    nimDebugPortIntIfNum(&port);
    nimDebugDynamicCapPort(intIfNum);
    nimDebugCfgPort(&port.configPort);
    /* Expandable Port Config Info */
    if ( PHY_CAP_EXPANDABLE_PORT_CHECK(port.operInfo.phyCapability))
    {
      sysapiPrintf("Expandable Port Mode Configured - %d Default - %d\n", 
                   NIM_EXP_PORT_MODE_GET(port.configPort.cfgInfo.portModeFlags),
                   NIM_EXP_PORT_MODE_GET(port.defaultCfg.portModeFlags));
    }
    sysapiPrintf("Default %d and Configured Expandable Port Mode Status %d\n",
                 NIM_EXP_PORT_MODE_STATUS_GET(port.defaultCfg.portModeFlags),
                 NIM_EXP_PORT_MODE_STATUS_GET(port.configPort.cfgInfo.portModeFlags));
    
    sysapiPrintf("Saved config state for expanadable port mode %d - status %d\n",
                 NIM_EXP_PORT_MODE_GET(nimCtlBlk_g->nimConfigData->cfgPort[intIfNum].cfgInfo.portModeFlags), 
                 NIM_EXP_PORT_MODE_STATUS_GET(nimCtlBlk_g->nimConfigData->cfgPort[intIfNum].cfgInfo.portModeFlags)); 

    dtlIntfLinkStateGet(intIfNum,&linkState);
    dtlIntfSpeedGet(intIfNum,&intfSpeed);
    if (linkState)
    {
      switch (intfSpeed)
      {
        case  PORTCTRL_PORTSPEED_AUTO_NEG: pSpeedStr = "Auto?"; break; 
        case  PORTCTRL_PORTSPEED_HALF_100TX: pSpeedStr = "100h"; break; 
        case  PORTCTRL_PORTSPEED_FULL_100TX: pSpeedStr = "100f"; break;
        case  PORTCTRL_PORTSPEED_HALF_10T: pSpeedStr = "10h"; break;
        case  PORTCTRL_PORTSPEED_FULL_10T: pSpeedStr = "10f"; break;
        case  PORTCTRL_PORTSPEED_FULL_100FX: pSpeedStr = "100fx"; break;
        case  PORTCTRL_PORTSPEED_FULL_1000SX: pSpeedStr = "1G"; break;
        case  PORTCTRL_PORTSPEED_FULL_10GSX: pSpeedStr = "10G"; break;
        case  PORTCTRL_PORTSPEED_FULL_20GSX: pSpeedStr = "20G"; break;
        case  PORTCTRL_PORTSPEED_FULL_40GSX: pSpeedStr = "40G"; break;
        case  PORTCTRL_PORTSPEED_FULL_25GSX: pSpeedStr = "25G"; break;
        case  PORTCTRL_PORTSPEED_FULL_50GSX: pSpeedStr = "50G"; break;
        case  PORTCTRL_PORTSPEED_FULL_100GSX: pSpeedStr = "100G"; break;
        case  PORTCTRL_PORTSPEED_FULL_200GSX: pSpeedStr = "200G"; break;
        case  PORTCTRL_PORTSPEED_FULL_400GSX: pSpeedStr = "400G"; break;
        case  PORTCTRL_PORTSPEED_AAL5_155: pSpeedStr = "AAL5_155"; break;
        case  PORTCTRL_PORTSPEED_FULL_2P5FX: pSpeedStr = "2.5G"; break;
        case  PORTCTRL_PORTSPEED_FULL_5FX: pSpeedStr = "5G"; break;
        case  PORTCTRL_PORTSPEED_LAG: pSpeedStr = "LAG"; break;
        case  PORTCTRL_PORTSPEED_UNKNOWN: pSpeedStr = "Unknown"; break;
        default: pSpeedStr = "Indeterminate"; break;
      }
      sysapiPrintf("Driver believes link is up at %s.", pSpeedStr);
    }
    else
    {
      sysapiPrintf("Driver believes link is down");
    }
  }
  return;
}

void nimDebugPortUspShow(uint32 unit,uint32 slot,uint32 port)
{
  nimUSP_t usp;
  uint32 intIfNum;

  usp.unit = unit;
  usp.slot = slot;
  usp.port = port;

  if (nimGetIntIfNumFromUSP(&usp, &intIfNum) ==  SUCCESS)
    nimDebugPortIntIfShow(intIfNum);

}
#endif

void nimDebugEventGenerator(char event[],uint32 intIfNum)
{
  uint32 eventNum =  LAST_PORT_EVENT;
  NIM_EVENT_NOTIFY_INFO_t eventInfo;
  NIM_HANDLE_t pHandle;


  if (strcmp(event," ATTACH") == 0)
  {
    eventNum =  ATTACH;
  }
  else if(strcmp(event," DETACH") == 0)
  {
    eventNum =  DETACH;
  }
  else if(strcmp(event," DELETE") == 0)
  {
    eventNum =  DELETE;
  }

  eventInfo.component =  NIM_COMPONENT_ID;
  eventInfo.intIfNum  = intIfNum;
  eventInfo.pCbFunc   = /*nimEventDtlDebugCallback*/ NULL;
  eventInfo.event     = eventNum;

  /* prefered method of event generation for all events */
  if (nimEventIntfNotify(eventInfo,&pHandle) !=  SUCCESS)
  {
    sysapiPrintf("Error in the call to generate event\n");
  }
}

