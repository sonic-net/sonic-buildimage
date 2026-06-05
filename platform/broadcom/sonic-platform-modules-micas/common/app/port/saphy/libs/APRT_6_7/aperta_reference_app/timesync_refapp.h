/*
 * $Id:  $
 *
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */
/*-----------------------------------------------------------------------------------------------*\
 In this program reference application(s) provided to demonstrate the usage of 
 IEEE-1588 TimeSync APIs for applicable BRCM PHY chip families using Command Line Interface

 This reference program is intented to show how to use BCM APIs to configure PHYs.
 This reference program may not work in some environments.
\*-----------------------------------------------------------------------------------------------*/
#ifndef __TIMESYNC_REFAPP_H__
#define __TIMESYNC_REFAPP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "epdm.h"

#ifndef  TRUE
#define  TRUE                       1
#endif
#ifndef  FALSE
#define  FALSE                      0
#endif
#define  UNKNOWN                    (-1)
#define  TBD32                      UNKNOWN
#define  TBD16                      UNKNOWN
#define  TBD8                       BCMPLP_TIMESYNC_SOPMEM_TO_BE_READ_BYTE

#define  PHY_INVALID_ARG            0x80000000
#define  ANDBEYOND                  0xFFFFFFFF
#define  BIT(_b)                    (1U << (_b))
#define  UINT32_HI16_SHIFT          16
#define  UINT32_HI16_MASK           0xFFFF0000
#define  UINT32_LO16_MASK           0x0000FFFF
#define  UINT32_LO16(_u32)          ( (_u32) & UINT32_LO16_MASK)
#define  UINT32_HI16(_u32)          (((_u32) & UINT32_HI16_MASK) >> UINT32_HI16_SHIFT)
#define  UINT64_HI32_SHIFT          32
#define  UINT64_HI32_MASK           0xFFFFFFFF00000000LL
#define  UINT64_LO16_MASK           0x000000000000FFFFLL
#define  UINT64_LO32_MASK           0x00000000FFFFFFFFLL
#define  UINT64_LO48_MASK           0x0000FFFFFFFFFFFFLL
#define  UINT64_SET(_hi,_lo)        ( (((_hi) & UINT64_LO48_MASK) * 1000000000) + _lo)
#define  IS_STRMATCH(_s1,_s2)       if ( ! strcmp(_s1,_s2) )
#define  IS_STRMATCH2(_s1,_s2,_s3)  if ( (! strcmp(_s1,_s2)) || (! strcmp(_s1,_s3)) )
#define  IS_PHY_INVALID_ARG(_u)     ((_u == PHY_INVALID_ARG) || (ANDBEYOND == _u))

#define  BCM_PORT_TYPE_RETIMER_REPEATER_MODE      0
#define  BCM_PORT_TYPE_FORWARD_GEARBOX_MODE       1
#define  BCM_PORT_TYPE_REVERSE_GEARBOX_MODE       2

#define  PORT_TYPE                  BCM_PORT_TYPE_RETIMER_REPEATER_MODE

/* (IGR_PORT.IGR_LANE) specifies where PTP messages are received *\
|* (EGR_PORT.EGR_LANE) specifies where to send PTP messages out  *|
|*     the IGE/EGR ports & lanes setting are strictly            *|
\*      hardware-specific and configuration-specific             */
#if   defined(PLP_APERTA_SUPPORT)
    #include "epdm_sec.h"
    #define  PLP_PM_TIMESTAMPING_SUPPORT        /* Aperta has PortMacro Timestamping */
    #define  NSE_TIMER_FREQ             125

  #if defined(PLP_APERTA_10G_NRZ)
    #define  PHY_DUAL_8Ln_CORE_1Ln_PER_PORT     /* Aperta 10G/25G NRZ mode      */
    #define  P0__PORT                   0       /*      PHY_ID  of Port_0       */
    #define  P0__LANE                   0x01    /*     Lane_Map of Port_0       */
    #define  IGR_PORT                   0
    #define  EGR_PORT                   0
    #define  IGR_LANE                   0x04
    #define  EGR_LANE                   0x02
  #elif defined(PLP_APERTA_25G_50G_FOV)
    #define  PHY_DUAL_8Ln_CORE_1Ln_PER_PORT_FOV /* Aperta 25G/50G Failover mode */
    #define  P0__PORT                   0       /*      PHY_ID  of Port_0       */
    #define  P0__LANE                   0x01    /*     Lane_Map of Port_0       */
    #define  IGR_PORT                   1
    #define  EGR_PORT                   0
    #define  IGR_LANE                   0x01
    #define  EGR_LANE                   0x04
  #elif defined(PLP_APERTA_100G_RGB_FOV)
    #define  PHY_DUAL_8Ln_CORE_2x4Ln_PER_PORT   /* Aperta 100G RGB(2x53G -> 4x25G) mode */
    #define  P0__PORT                   0       /*      PHY_ID  of Port_0       */
    #define  P0__LANE                   0x0F    /*     Lane_Map of Port_0       */
    #define  SYS_P0__PORT               0       /*      PHY_ID  of Port_0       */
    #define  SYS_P0__LANE               0x03    /*     Lane_Map of Port_0       */
    #define  IGR_PORT                   1
    #define  EGR_PORT                   0
    #define  IGR_LANE                   0x0F
    #define  EGR_LANE                   0x0F
    #define  SYS_IGR_LANE               0x03
    #define  SYS_EGR_LANE               0x03
    #define  GEARBOX_MODE
  #elif defined(PLP_APERTA_100G_NRZ)
    #define  PHY_DUAL_8Ln_CORE_4Ln_PER_PORT     /* Aperta 100G NRZ (4x25G) mode */
    #define  P0__PORT                   0       /*      PHY_ID  of Port_0       */
    #define  P0__LANE                   0x0F    /*     Lane_Map of Port_0       */
    #define  IGR_PORT                   1
    #define  EGR_PORT                   0
    #define  IGR_LANE                   0x0F
    #define  EGR_LANE                   0x0F
  #elif defined(PLP_APERTA_100G_PAM4)
    #define  PHY_DUAL_8Ln_CORE_2Ln_PER_PORT     /* Aperta 100G PAM4 (2x53G) mode */
    #define  P0__PORT                   0       /*      PHY_ID  of Port_0       */
    #define  P0__LANE                   0x03    /*     Lane_Map of Port_0       */
    #define  IGR_PORT                   1
    #define  EGR_PORT                   0
    #define  IGR_LANE                   0x03
    #define  EGR_LANE                   0x03
  #else
    #define  PHY_DUAL_8Ln_CORE_8Ln_PER_PORT     /* Aperta 400G PAM4 (8x53G) mode */
    #define  P0__PORT                   0       /*      PHY_ID  of Port_0       */
    #define  P0__LANE                   0xFF    /*     Lane_Map of Port_0       */
    #define  IGR_PORT                   1
    #define  EGR_PORT                   0
    #define  IGR_LANE                   0xFF
    #define  EGR_LANE                   0xFF
  #endif
#elif defined(PLP_QUADRA28_SUPPORT)
    #define  IGR_PORT                   2
    #define  EGR_PORT                   7
    #define  IGR_LANE                   BIT(0)
    #define  EGR_LANE                   BIT(0)
    #define  NSE_TIMER_FREQ             128
#elif defined(BCM_PLP_BASE_T_PHY)
  /* for 10GBase-T copper PHYs */
    #define  BCM_PLP_EXTERNAL_PTP_REFCLK
  #if   defined(PLP_WHITETIP_SUPPORT)
    #define  IGR_PORT                   2
    #define  EGR_PORT                   3
  #elif defined(PLP_BROADFIN_SUPPORT)
    #define  IGR_PORT                   9
    #define  EGR_PORT                   10
  #elif defined(PLP_KAUAI_SUPPORT)
    #define  IGR_PORT                   1
    #define  EGR_PORT                   0
  #elif defined(PLP_LANAI_SUPPORT)
    #define  IGR_PORT                   31
    #define  EGR_PORT                   31
  #else  /* Shortfin / Longfin / Blackfin */
    #define  IGR_PORT                   1
    #define  EGR_PORT                   2
  #endif
    #define  IGR_LANE                   BIT(0)
    #define  EGR_LANE                   BIT(0)
    #define  NSE_TIMER_FREQ             125
#elif defined(PLP_MGAUTO_SUPPORT)
    #define  IGR_PORT                   0
    #define  EGR_PORT                   0
    #define  IGR_LANE                   BIT(0)
    #define  EGR_LANE                   BIT(0)
    #define  NSE_TIMER_FREQ             125
#else
    /* for Evora/Europa/Miura in 10G/25G MACSEC bypass mode */
    #define  PHY_DUAL_4Ln_CORE_1Ln_PER_PORT   /* 10G/25G NRZ mode, 4 lanes per core */
    #define  IGR_PORT                   1
    #define  EGR_PORT                   0
    #define  IGR_LANE                   BIT(0)
    #define  EGR_LANE                   BIT(1)
    #define  NSE_TIMER_FREQ             136
#endif

/*                                          *\
|* useful symbols for TimeSync operations   *|
\*                                          */
#define  SYS_SIDE                   1
#define  LINE_SIDE                  0
#define  DUMP_MAX                   33
#define  PORT_MAX                   33
#define  TIMESYNC_DISABLE           0
#define  TIMESYNC_ENABLE            1
#define  TIMESYNC_HEARTBEAT_SYNC0   0
#define  TIMESYNC_HEARTBEAT_SYNC1   1
#define  TIMESYNC_HEARTBEAT_SYNCOUT 2
#define  TIMESYNC_HEARTBEAT_CPU     4
#define  TIMESYNC_DIRECTION_TXRX    bcmplpTimesyncTxRx   /* 0x0 */
#define  TIMESYNC_DIRECTION_RX      bcmplpTimesyncRx     /* 0x1 */
#define  TIMESYNC_DIRECTION_TX      bcmplpTimesyncTx     /* 0x2 */
#define  TIMESYNC_DIRECTION_RXTX    (TIMESYNC_DIRECTION_RX | TIMESYNC_DIRECTION_TX)
#define  TIMESYNC_MESSAGE_TYPE_ALL  (bcmplpPktTypePtpPdelayResp | bcmplpPktTypePtpPdelayReq |  \
                                     bcmplpPktTypePtpDelayReq   | bcmplpPktTypePtpSync)
#define  TIMESYNC_INBAND32_FORMAT   (1U << 4)
#define  TIMESYNC_INBAND            1
#define  TIMESYNC_OUT_OF_BAND       2
#define  TIMESYNC_OOBAND            TIMESYNC_OUT_OF_BAND
#define  TIMESYNC_T1_ONE_STEP       1
#define  TIMESYNC_T1_TWO_STEP       2
#define  TIMESYNC_SEQID_MAX         0xFFFF
#define  TIMESYNC_SEQID_DEFAULT     0x168
#define  TIMESYNC_SOPMEM_MAX        32              /* SOPmem contains 32 entries      */
#define  TIMESYNC_SOPMEM_DUMP       0xf1f0a11       /* dump all SOPmem entries         */
#define  TIMESYNC_SOPMEM_CLEAR      0xf1f0de1       /* clear one or all SOPmem entries */

#define  TIMESYNC_LINK_DELAY        0x00LL          /* user defined */
#define  TIMESYNC_TS_OFFSET_RX      0x0LL           /* user defined */
#define  TIMESYNC_TS_OFFSET_TX      0x0LL           /* user defined */
#define  TIMESYNC_LOCAL_TIME_SEC    0x00LL          /* user defined */
#define  TIMESYNC_LOCAL_TIME_NS     0x00LL          /* user defined */
#define  TIMESYNC_SHADOW_LOAD_DEFAULT  \
                       ( bcmplpTimesyncLoadCtrlTimeCode       | bcmplpTimesyncLoadCtrlLocalTime    |  \
                         bcmplpTimesyncLoadCtrlDpllLoopFilter | bcmplpTimesyncLoadCtrlDpllRefPhase |  \
                         bcmplpTimesyncLoadCtrlDpllRefDelta   | bcmplpTimesyncLoadCtrlDpllK3       |  \
                         bcmplpTimesyncLoadCtrlDpllK2         | bcmplpTimesyncLoadCtrlDpllK1       )
#if defined(BCM_PLP_EXTERNAL_PTP_REFCLK)
    #define  HEARTBEAT_FETCH_INTERVAL   1000000     /* 1.00 second */
#else
    #define  HEARTBEAT_FETCH_INTERVAL   720000      /* 0.72 second */
#endif

#if   defined(BCM_PLP_BASE_T_PHY) || defined(PLP_QUADRA28_SUPPORT)
  #define  P1588_BASE     0xC600        /* Orca, XxxxFINs/Whitetip/Kauai, Quadra28 */
#else
  #define  P1588_BASE     0x49007000    /* Evora, Miura, Europa, Aperta */
#endif
  #define  P1588_REG_MAX  0x00FF


#define  FOR_LINE_SYS_SIDE(_rv, _pi, _fn)   do {           \
             (_pi).if_side = SYS_SIDE ;       _rv |= _fn;  \
             printf("------ rv=%d side=%d\n", _rv, (_pi).if_side); \
             (_pi).if_side = LINE_SIDE;       _rv |= _fn;  \
             printf("------ rv=%d side=%d\n", _rv, (_pi).if_side); \
         } while ( 0 )

#define  FORBOTH_LINE_SYS_SIDE(_pi)       \
         for ( (_pi).if_side = SYS_SIDE;  \
               (signed) ((_pi).if_side) >= LINE_SIDE; ((_pi).if_side)-- )

#if defined(PLP_APERTA_25G_50G_FOV) ||  defined(PLP_APERTA_100G_RGB_FOV)

unsigned int  failover_lanemap_get(char *chipname, bcm_plp_access_t phyinfo);
    int  speed = 0, iftype = 0, refclk = 0, ifmode = 0;
    bcm_plp_aperta_device_aux_modes_t  auxmode;
    phyinfo.if_side  = SYS_SIDE;

    bcm_plp_mode_config_get(chipname, phyinfo, &speed, &iftype,
                                      &refclk, &ifmode, (void*) &auxmode);
#if defined(_DEBUGGING_IEEE1588_TIMESYNC_)
    printf("%s:%d p=[0x%x 0x%x + 0x%x]\n",__func__,__LINE__, phyinfo.phy_addr,
                           phyinfo.lane_map, auxmode.failover_config.lane_map);
#endif
    return  auxmode.failover_config.lane_map;
}

#define  PROLOGUE_FOV(_c, _pi, _olm, _flm)  \
         unsigned int _olm = (_pi).lane_map, _flm = failover_lanemap_get(_c,_pi)
#define  ALSO_FOV_LANEMAP(_pi, _olm, _flm, _func)   do {          \
                             if ( _flm && (!(_flm & _olm) ) );   \
                                 (_pi).lane_map = _flm;  (_func); \
                                 (_pi).lane_map = _olm;           \
                             }                                    \
                             (_func);                             \
                         } while (0)
#else

#define  PROLOGUE_FOV(_c, _pi, _olm, _flm)
#define  ALSO_FOV_LANEMAP(_pi, _olm, _flm, _func)        (_func)
#endif  /* PLP_APERTA_25G_50G_FOV */


#if   defined(PHY_DUAL_4Ln_CORE_1Ln_PER_PORT)
  #define  NUM_PORT             8
  #define  TC1588_PHY_ID(_p)    ((_p) >> 2)           /* die_0: ports 0-3 ,  die_1: ports 4-7  */
  #define  TC1588_LANEMAP(_p)   (0x1U << ((_p) & 0x3))      /* 4 lanes per die  ( 10/25G mdoe) */
#elif defined(PHY_DUAL_8Ln_CORE_1Ln_PER_PORT)
  #define  NUM_PORT             16
  #define  TC1588_PHY_ID(_p)    ((_p) >> 3)           /* die_0: ports 0-7 ,  die_1: ports 8-15 */
  #define  TC1588_LANEMAP(_p)   (0x1U << ((_p) & 0x7))      /* 8 lanes per die  ( 10/25G mdoe) */
#elif defined(PHY_DUAL_8Ln_CORE_1Ln_PER_PORT_FOV)
  #define  NUM_PORT             8                     /* 25/50G failover)                      */
  #define  TC1588_PHY_ID(_p)    ((_p) >> 2)           /* die_0: ports 0-3 ,  die_1: ports 4-7  */
  #define  TC1588_LANEMAP(_p)   (0x1U << (((_p)&0x3)<<1))   /* lanemap 0x01/0x04/0x10/0x40     */
#elif defined(PHY_DUAL_8Ln_CORE_4Ln_PER_PORT)
  #define  NUM_PORT             4
  #define  TC1588_PHY_ID(_p)    ((_p) >> 1)           /* die_0: ports 0-1 ,  die_1: ports 2-3  */
  #define  TC1588_LANEMAP(_p)   (0xFU << (((_p)&0x1)<<2))   /* 4 lanes per port (100G m  */
#elif defined(PHY_DUAL_8Ln_CORE_2Ln_PER_PORT)
  #define  NUM_PORT             8
  #define  TC1588_PHY_ID(_p)    ((_p) >> 2)           /* die_0: ports 0-1 ,  die_1: ports 2-3  */
  #define  TC1588_LANEMAP(_p)   (0x3U << (((_p)&0x3)<<1))   /* 4 lanes per port (100G m  */
#elif defined(PHY_DUAL_8Ln_CORE_2x4Ln_PER_PORT)
  #define  NUM_PORT             4
  #define  TC1588_PHY_ID(_p)     ((_p) >> 1)           /* die_0: ports 0-1 ,  die_1: ports 2-3  */
  #define  TC1588_LANEMAP(_p)    (0x3U << (((_p)&0x1)<<2))   /* 2 lanes per port (100G m  */
#if defined (GEARBOX_MODE)
  #define  TC1588_PHY_ID_PM(_p)  ((_p) >> 1)           /* die_0: ports 0-1 ,  die_1: ports 2-3  */
  #define  TC1588_LANEMAP_PM(_p) (0xFU << (((_p)&0x1)<<2))   /* 4 lanes per port (100G m  */
#endif
#elif defined(PHY_DUAL_8Ln_CORE_8Ln_PER_PORT)
  #define  NUM_PORT             2
  #define  TC1588_PHY_ID(_p)    ((_p))                 /* die_0: port 0 ,  die_1: port 1  */
  #define  TC1588_LANEMAP(_p)   (0xFF)                 /* 8 lanes per port (400G)   */
#else    /* 1G/10G PHYs */
  #define  NUM_PORT             4
  #define  TC1588_PHY_ID(_p)    (_p)         /* 1G/10G PHYs           */
  #define  TC1588_LANEMAP(_p)   (0x1U)       /*   do not have lanemap */
#endif

#if  defined(PLP_PM_TIMESTAMPING_SUPPORT)
  #define  PM1588_MODE_TC       9
  #define  NSE1588_MODE_TC      8
  #define  PM1588_MODE_T12      1

  int ts_pm_t12_tc(char *chipname, bcm_plp_access_t pinfo, int mode, int step,
                                                      bcm_plp_timesync_config_t *cfg);
#endif   /*  PLP_PM_TIMESTAMPING_SUPPORT  */

#define  TS_COMPARE(_p, _v1, _v2, _t)   \
            do {    \
                if ( memcmp(_v1, _v2, sizeof(_t)) )  \
                      printf("%s(%d) p=[%x.0x%02x]  TS_COMPARE differ !!!\n", \
                              __func__,__LINE__, _p.phy_addr, _p.lane_map);   \
                else  printf("Timesync_config GET same values as SET for port [%x.0x%02x]\n", \
                                                 _p.phy_addr, _p.lane_map);   \
            } while ( 0 )

/* TimeSync 1588 base configuration */
#define TIMESYNC_BASE_SETTING(tscfg) do {\
            memset(&tscfg, 0, sizeof(bcm_plp_timesync_config_t));\
            tscfg.flags = 0x77;\
            tscfg.itpid = 0x8100;\
            tscfg.otpid = 0x9100;\
            tscfg.otpid2 = 0x9100;\
            tscfg.inband_ctrl.resv0_id = 0xa;\
            tscfg.inband_ctrl.timer_mode = 0x4;\
            tscfg.gmode = 0x2;\
            tscfg.tx_inband_prop.write_sopmem   = 0x80;\
            tscfg.tx_inband_prop.compare_sopmem = 0x80;\
            tscfg.tx_inband_prop.cmp_field_sel  = TRUE;\
            tscfg.tx_pkt_count_sel = 0x100;\
            tscfg.rx_inband_prop.write_sopmem   = 0x80;\
            tscfg.rx_inband_prop.compare_sopmem = 0x80;\
            tscfg.rx_inband_prop.cmp_field_sel  = TRUE;\
            tscfg.rx_pkt_count_sel = 0x100;\
            tscfg.fifo_level_intr_thold = 0x1;\
            tscfg.hb_capture_mode = 0x1;\
            tscfg.nco_sync0_pulse = 0x1;\
        } while(0)

#ifdef __TIMESYNC_HELPER_C__
  bcm_plp_timesync_inband_parse_t  addr_capture_tx = bcmplpTimesyncInbandParseIpv4DstIp;
  bcm_plp_timesync_inband_parse_t  addr_capture_rx = bcmplpTimesyncInbandParseIpv4SrcIp;
#else
  extern bcm_plp_timesync_inband_parse_t  addr_capture_tx;
  extern bcm_plp_timesync_inband_parse_t  addr_capture_rx;
#endif

/* helper function to set the PTP lookup-action mode */
int ts_lookup_action_set(char *chipname, bcm_plp_access_t  phy_info,
                                         bcm_plp_timesync_txrx_t  rxtx,
                                         bcm_plp_timesync_ptp_action_mode_t  mode);
/* helper function to get the PTP lookup-action mode */
int ts_lookup_action_get(char *chipname, bcm_plp_access_t  phy_info,
                                         bcm_plp_timesync_txrx_t  rxtx,
                                         bcm_plp_timesync_ptp_action_mode_t *mode,
                                         bcm_plp_timesync_user_action_t *user_def);
/* read/lookup the SOPmem and print valid entries */
int ts_sopmem_get(char *chipname, bcm_plp_access_t pinfo, unsigned int seqid,
                  unsigned int entry_id, unsigned int flags, unsigned long long *tstmp);
/* clear SOPmem  */
int ts_sopmem_clear(char *chipname, bcm_plp_access_t pinfo, unsigned int index);
/* select what to be captured to SOPmem: SrcMAC/DstMAC/SrcIP/DstIP/SourcePortIdentity */
int ts_sopmem_ip_mac_capture_set(char *chipname, bcm_plp_access_t   phy_info,
                                   bcm_plp_timesync_inband_parse_t  ipmac_rx,
                                   bcm_plp_timesync_inband_parse_t  ipmac_tx);
/* get the setting of what to be captured to SOPmem */
int ts_sopmem_ip_mac_capture_get(char *chipname, bcm_plp_access_t   phy_info,
                                   bcm_plp_timesync_inband_parse_t *ipmac_rx,
                                   bcm_plp_timesync_inband_parse_t *ipmac_tx);
/* TimeSync 1588 config sequence for setting internal DPLL for PTP Clock */
int ts_dpll_ptp_clock(char *chipname, bcm_plp_access_t pinfo, int txrx, int syncin_freq,
                            unsigned long long localtime_sec,  unsigned int localtime_ns,
                                                               unsigned int pm1588_en);
/* TimeSync 1588 config sequence for setting External Reference Clocks :    *\
\*          to use [PTP_CLK+/-] or [PHY REF_CLK] to run NCO counter         */
int ts_ext_ptp_ref_clock(char *chipname, bcm_plp_access_t phy_info,
                                                      int ext_ptp_clk_src,
                        bcm_plp_timesync_framesync_mode_t framsync_src   ,
                                       unsigned long long local_time);
/* TimeSync 1588 config sequence for TIMESTAMP T1 */
int ts_t1(char *chipname, bcm_plp_access_t pinfo, int txrx, int step, int band,
                          unsigned int timer_precision, bcm_plp_timesync_config_t *cfg);
/* TimeSync 1588 config sequence for TIMESTAMP T2 */
int ts_t2(char *chipname, bcm_plp_access_t pinfo, int txrx, int band,
                          unsigned int timer_precision, bcm_plp_timesync_config_t *cfg);
/* TimeSync 1588 config sequence for TIMESTAMP T3/T4 Inband mode */
int ts_t3_t4(char *chipname, bcm_plp_access_t pinfo, int txrx,
                          unsigned int timer_precision, bcm_plp_timesync_config_t *cfg);
/* TimeSync 1588 config sequence for TIMESTAMP T3 Out-of-Band mode */
int ts_t3_oob(char *chipname, bcm_plp_access_t pinfo, int txrx,
                          unsigned int timer_precision, bcm_plp_timesync_config_t *cfg);
/* TimeSync 1588 config sequence for TIMESTAMP T4 Out-of-Band mode */
int ts_t4_oob(char *chipname, bcm_plp_access_t pinfo, int txrx,
                          unsigned int timer_precision, bcm_plp_timesync_config_t *cfg);
/* TimeSync 1588 config sequence for Regular TC Mode */
int ts_tc(char *chipname, bcm_plp_access_t pinfo, int txrx,
                unsigned int timer_precision    , long long link_delay,
                long long    ts_offset_rx       , long long ts_offset_tx ,
                unsigned long long localtime_sec, unsigned int  localtime_ns,
                 bcm_plp_timesync_config_t *cfg);
/* TimeSync 1588 config sequence for Partial TC Mode */
int ts_ptc(char *chipname, bcm_plp_access_t pinfo, unsigned int flags,
                     unsigned int timer_precision, long long link_delay,
                     long long    ts_offset_rx   , long long ts_offset_tx ,
                     bcm_plp_timesync_config_t *cfg);

#endif /* __TIMESYNC_REFAPP_H__ */

