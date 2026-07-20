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

#ifndef __NIM_EXPORTS_H_
#define __NIM_EXPORTS_H_


/* NIM Component Feature List */
typedef enum
{
   NIM_FEATURE_ID = 0,            /* general support statement */
   NIM_JUMBOFRAMES_PER_INTERFACE_FEATURE_ID,
   NIM_JUMBOFRAMES_FEATURE_ID,    /* whether jumbo frames (maximum transmission unit) is configurable */
   NIM_INTF_NUMBER_FORMAT_SIMPLE_FEATURE_ID, /* To Enable/Disable interface name format conversion*/
   NIM_INTF_COMBO_PREFERENCE,                /* If combo ports can be configured to prefer a media type */
   NIM_HOLD_STATS_FEATURE_ID, /* To display Head-Of-Line Drops statistics*/
   NIM_SERVICE_PORT_INTF_FEATURE_ID, /* NIM Service Port Interface feature */
   NIM_EXPANDABLE_PORT_FEATURE_ID, /* NIM warpcore expandable port(ex. 40G->4*10G) feature */
   NIM_LINK_DEBOUNCE_PER_INTERFACE_FEATURE_ID, /* To enable/disable link debounce time per port */
   NIM_SHOW_COMBO_PORT_FEATURE_ID,
   NIM_STORM_CONTROL_AUTO_RECOVERY_SEPARATE_TIMER_FEATURE_ID, /* To use separate Autorecovery timer for Storm Control feature*/
   NIM_EXPANDABLE_PORT_100G_FEATURE_ID, /* 100G expandable port support feature*/
   NIM_SHOW_EXPANDABLE_PORT_ENHANCEMENTS_FEATURE_ID, /* Expandable port show command enhancement feature*/
   NIM_DIAG_DISABLED_AUTO_RECOVERY_FEATURE_ID, /* NIM diag disable auto recovery feature */
   NIM_EXPANDABLE_PORTS_OVERLAP_ROOT_PORT_FEATURE_ID, /* NIM expandable overlap port feature */
   NIM_FEATURE_ID_TOTAL           /* total number of enum values */
}  NIM_FEATURE_IDS_t;

/* Bitmask of possible media -- only one can be selected. */
typedef enum
{
   NIM_COMBO_PREF_SFP = 0,
   NIM_COMBO_PREF_RJ45,
   NIM_COMBO_PREF_TOTAL
}  NIM_COMBO_PREFERENCE_t;

#define  NIM_IFDESCRINFO_SET   nimIfDescrInfoSet
#define NIM_AUTONEG_FIXED_SPEED_MASK     ( PORT_CAPABILITY_SPEED_10   | \
                                           PORT_CAPABILITY_SPEED_100  | \
                                           PORT_CAPABILITY_SPEED_1000 | \
                                           PORT_CAPABILITY_SPEED_2P5G | \
                                           PORT_CAPABILITY_SPEED_5G   | \
                                           PORT_CAPABILITY_SPEED_10G  | \
                                           PORT_CAPABILITY_SPEED_20G  | \
                                           PORT_CAPABILITY_SPEED_25G  | \
                                           PORT_CAPABILITY_SPEED_40G  | \
                                           PORT_CAPABILITY_SPEED_50G  | \
                                           PORT_CAPABILITY_SPEED_100G | \
                                           PORT_CAPABILITY_SPEED_200G | \
                                           PORT_CAPABILITY_SPEED_400G | \
                                           PORT_CAPABILITY_SPEED_AUTO)

#define  LAG_NAME_PREFIX            "lag"
#define  LAG_NAME_FORMAT            "%s %d"
#define  CPU_NAME_PREFIX            "CPU"
#define  VLAN_NAME_PREFIX           "vlan"
#define  PROP_VLAN_NAME_PREFIX      "PVlan"
#define  TUNNEL_NAME_PREFIX         "Tunnel"
#define  CAP_TUNNEL_NAME_PREFIX     "CapTun"
#define  SERVICEPORT_NAME_PREFIX    "Service Port"

/*The below macro is useful while creating interface number.
  In systems where pluggable slots are present, the ports may
  be present from slot 1, by default the ports start from slot 0*/
#define  START_SLOT_NUM_WITH_PORT   0

/* Constants used by ISCLI description routine for an interface*/

/* for iscli need to override DOT1Q_VLAN_INTF_IFNAME_PREFIX to use "Vl"*/
#define NIM_VLAN_INTF_IFNAME_LONG_PREFIX "Vlan"

#define ETH_INTF_IFNAME_PREFIX "Fa"
#define ETH_INTF_IFNAME_LONG_PREFIX "Fastethernet"

#define GIGA_ETH_INTF_IFNAME_PREFIX "Gi"
#define GIGA_ETH_INTF_IFNAME_LONG_PREFIX "Gigabitethernet"

#define FIVE_GIGA_ETH_INTF_IFNAME_PREFIX "Fi"
#define FIVE_GIGA_ETH_INTF_IFNAME_LONG_PREFIX "Fivegigabitethernet"

#define TENGIG_ETH_INTF_IFNAME_PREFIX "Te"
#define TENGIG_ETH_INTF_IFNAME_LONG_PREFIX "Tengigabitethernet"

#define FORTYGIG_ETH_INTF_IFNAME_PREFIX "Fo"
#define FORTYGIG_ETH_INTF_IFNAME_LONG_PREFIX "Fortygigabitethernet"

#define TWENTYGIG_ETH_INTF_IFNAME_PREFIX "Tw"
#define TWENTYGIG_ETH_INTF_IFNAME_LONG_PREFIX "Twentygigabitethernet"

#define HUNDREDGIG_ETH_INTF_IFNAME_PREFIX "Hu"
#define HUNDREDGIG_ETH_INTF_IFNAME_LONG_PREFIX "Hundredgigabitethernet"

#define FIFTYGIG_ETH_INTF_IFNAME_PREFIX "Fi"
#define FIFTYGIG_ETH_INTF_IFNAME_LONG_PREFIX "Fiftygigabitethernet"

#define TWOPOINTFIVEGIG_ETH_INTF_IFNAME_LONG_PREFIX "Twopointfivegigabitethernet"
#define TWENTYFIVEGIG_ETH_INTF_IFNAME_LONG_PREFIX "Twentyfivegigabitethernet"
#define TWOHUNDREDGIG_ETH_INTF_IFNAME_LONG_PREFIX "Twohundredgigabitethernet"
#define FOURHUNDREDGIG_ETH_INTF_IFNAME_LONG_PREFIX "Fourhundredgigabitethernet"

/* Debounce time range */
#define  NIM_MIN_DEBOUNCE_TIME           100   /* minimum debounce timer value in msec */
#define  NIM_MAX_DEBOUNCE_TIME           5000  /* maximum debounce timer value in msec */
#define  NIM_DEBOUNCE_TIME_CONFIG_STEP   100   /* debounce timer should be multiple of this */
/* End of  Debounce time range */

#define  NIM_SID_MSG_COUNT           (5 * platVlanTotalMaxEntriesGet() + \
                                       (5 * platIntfTotalMaxCountGet()))

#define NIM_AUTORECOVERY_DHCP_RATE_LIMIT   0x001
#define NIM_AUTORECOVERY_ARP_INSPECTION    0x002
#define NIM_AUTORECOVERY_UDLD              0x004
#define NIM_AUTORECOVERY_BSTORMCONTROL     0x008
#define NIM_AUTORECOVERY_BPDUGUARD         0x010
#define NIM_AUTORECOVERY_BPDU_RATE_LIMIT   0x020
#define NIM_AUTORECOVERY_SFP_MISMATCH      0x040
#define NIM_AUTORECOVERY_MSTORMCONTROL     0x080
#define NIM_AUTORECOVERY_USTORMCONTROL     0x100
#define NIM_AUTORECOVERY_LOOPPROTECT       0x200
#define NIM_AUTORECOVERY_SFPPLUS_MISMATCH  0x400
#define NIM_AUTORECOVERY_PML               0x800
#define NIM_AUTORECOVERY_DOS               0x1000
#define NIM_AUTORECOVERY_LINK_FLAP         0x2000
#define NIM_AUTORECOVERY_AUTHMGR           0x4000
#define NIM_AUTORECOVERY_STP               0x8000
#define NIM_AUTORECOVERY_COA               0x10000
#define NIM_AUTORECOVERY_ALL               0x1FFFF

#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_DHCP_RATE_LIMIT    DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_ARP_INSPECTION     DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_UDLD               DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_BSTORMCONTROL      DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_BPDUGUARD          DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_BPDU_RATE_LIMIT    DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_SFP_MISMATCH       DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_MSTORMCONTROL      DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_USTORMCONTROL      DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_LOOPPROTECT        DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_SFPPLUS_MISMATCH   DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_PML                DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_DOS                DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_LINK_FLAP          DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_AUTHMGR            DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_STP                DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_COA                DISABLE
#define NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS      ( NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_DHCP_RATE_LIMIT << 0 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_ARP_INSPECTION  << 1 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_UDLD << 2 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_BSTORMCONTROL << 3  | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_BPDUGUARD << 4 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_BPDU_RATE_LIMIT << 5 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_SFP_MISMATCH << 6 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_MSTORMCONTROL << 7 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_USTORMCONTROL << 8 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_LOOPPROTECT << 9 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_SFPPLUS_MISMATCH << 10 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_PML << 11 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_DOS << 12 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_LINK_FLAP << 13 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_AUTHMGR << 14  | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_STP << 15 | \
                                                        NIM_DEFAULT_DDISABLE_AUTORECOVERY_STATUS_COA << 16 )

#define  LOG_DDISABLE_SEVERITY            LOG_SEVERITY_NOTICE
#define  LOG_AUTORECOVERY_SEVERITY        LOG_SEVERITY_NOTICE

/******************** conditional Override *****************************/

#ifdef INCLUDE_NIM_EXPORTS_OVERRIDES
#include "nim_exports_overrides.h"
#endif 

#endif /* __NIM_EXPORTS_H_*/
