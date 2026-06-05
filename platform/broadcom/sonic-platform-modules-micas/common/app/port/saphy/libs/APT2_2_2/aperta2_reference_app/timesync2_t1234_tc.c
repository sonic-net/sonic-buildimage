/*
 *
 * $Id:  $
 *
 * $Copyright: (c) 2023 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

/*---------------------------------------------------------------------------------*\
 In this program reference application(s) provided to demonstrate the usage of 
 IEEE-1588 TimeSync APIs for applicable BRCM PHY chip families using 
 Command Line Interface

 This reference program is intented to show how to use BCM APIs to configure PHYs.
 This reference program may not work in some environments.
\*---------------------------------------------------------------------------------*/

#ifdef BCM_PLP_TIMESYNC_SUPPORT

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

#define  PHY_INVALID_ARG            0x80000000
#define  ANDBEYOND                  0xFFFFFFFF
#define  BIT(_b)                    (1U << (_b))
#define  UINT32_HI16_SHIFT          16
#define  UINT32_HI16_MASK           0xFFFF0000
#define  UINT32_LO16_MASK           0x0000FFFF
#define  UINT32_LO16(_u32)          ( (_u32) & UINT32_LO16_MASK)
#define  UINT32_HI16(_u32)          (((_u32) & UINT32_HI16_MASK) >> UINT32_HI16_SHIFT)
#define  UINT64_HI32_SHIFT          32
#define  UINT64_LO32_MASK           0x00000000FFFFFFFFLL
#define  UINT64_LO48_MASK           0x0000FFFFFFFFFFFFLL
#define  UINT64_SET(_hi,_lo)        ( (((_hi) & UINT64_LO32_MASK)  \
                                             << UINT64_HI32_SHIFT) | (_lo) )
#define  IS_STRMATCH(_s1,_s2)       if ( ! strcmp(_s1,_s2) )
#define  IS_STRMATCH2(_s1,_s2,_s3)  if ( (! strcmp(_s1,_s2)) || (! strcmp(_s1,_s3)) )
#define  IS_PHY_INVALID_ARG(_u)     ((_u == PHY_INVALID_ARG) || (ANDBEYOND == _u))

/* (IGR_PORT.IGR_LANE) specifies where PTP messages are received *\
|* (EGR_PORT.EGR_LANE) specifies where to send PTP messages out  *|
|*     the IGE/EGR ports & lanes setting are strictly            *|
\*      hardware-specific and configuration-specific             */
#if defined(PLP_APERTA2_SUPPORT)
  #include "epdm_sec.h"
  #define  PLP_PM_TIMESTAMPING_SUPPORT

  #if defined(PLP_APERTA2_SINGLE_LANE_PORTS)
    #define  IGR_PORT                   0
    #define  EGR_PORT                   0
    #define  SYS_IGR_LANE               0x0001
    #define  SYS_EGR_LANE               0x0100
    #define  LINE_IGR_LANE              0x0001
    #define  LINE_EGR_LANE              0x0100
  #elif defined(PLP_APERTA2_DUAL_LANE_PORTS)
    #define  IGR_PORT                   0
    #define  EGR_PORT                   0
    #define  SYS_IGR_LANE               0x0003
    #define  SYS_EGR_LANE               0x0300
    #define  LINE_IGR_LANE              0x0003
    #define  LINE_EGR_LANE              0x0300
  #elif defined(PLP_APERTA2_QUAD_LANE_PORTS)
    #define  IGR_PORT                   0
    #define  EGR_PORT                   0
    #define  SYS_IGR_LANE               0x000F
    #define  SYS_EGR_LANE               0x0F00
    #define  LINE_IGR_LANE              0x000F
    #define  LINE_EGR_LANE              0x0F00
  #elif defined(PLP_APERTA2_OCTAL_LANE_PORTS)
    #define  IGR_PORT                   0
    #define  EGR_PORT                   0
    #define  SYS_IGR_LANE               0x00FF
    #define  SYS_EGR_LANE               0xFF00
    #define  LINE_IGR_LANE              0x00FF
    #define  LINE_EGR_LANE              0xFF00
  #elif defined(PLP_APERTA2_RGB24_LANE_PORTS)
    #define  IGR_PORT                   0
    #define  EGR_PORT                   0
    #define  SYS_IGR_LANE               0x0003
    #define  SYS_EGR_LANE               0x0300
    #define  LINE_IGR_LANE              0x000F
    #define  LINE_EGR_LANE              0x0F00
  #elif defined(PLP_APERTA2_FGB42_LANE_PORTS)
    #define  IGR_PORT                   0
    #define  EGR_PORT                   0
    #define  SYS_IGR_LANE               0x000F
    #define  SYS_EGR_LANE               0x0F00
    #define  LINE_IGR_LANE              0x0003
    #define  LINE_EGR_LANE              0x0300
  #else
    #define  IGR_PORT                   0
    #define  EGR_PORT                   0
    #define  SYS_IGR_LANE               0x0001
    #define  SYS_EGR_LANE               0x0100
    #define  LINE_IGR_LANE              0x0001
    #define  LINE_EGR_LANE              0x0100
  #endif
#endif

/*                                          *\
|* useful symbols for TimeSync operations   *|
\*                                          */
#define  SYS_SIDE                   1
#define  LINE_SIDE                  0
#define  DUMP_MAX                   33

#define  TIMESYNC_DISABLE           0
#define  TIMESYNC_ENABLE            1

#define  TIMESYNC_DIRECTION_RX      bcmplpTrafficRx     /* 0x1 */
#define  TIMESYNC_DIRECTION_TX      bcmplpTrafficTx     /* 0x2 */
#define  TIMESYNC_DIRECTION_RXTX    (TIMESYNC_DIRECTION_RX | TIMESYNC_DIRECTION_TX)
#define  TIMESYNC_MESSAGE_TYPE_ALL  (bcmplpPktTypePtpPdelayResp | bcmplpPktTypePtpPdelayReq |  \
                                     bcmplpPktTypePtpDelayReq   | bcmplpPktTypePtpSync)

#define  TIMESYNC_TS_OFFSET_RX      0x0LL          /* user defined */
#define  TIMESYNC_TS_OFFSET_TX      0x0LL          /* user defined */

#if defined(BCM_PLP_EXTERNAL_PTP_REFCLK)
    #define  HEARTBEAT_FETCH_INTERVAL   1000000     /* 1.00 second */
#else
    #define  HEARTBEAT_FETCH_INTERVAL   720000      /* 0.72 second */
#endif

/* Register Defines */
#define  P1588_BASE                  0xAA000000
#define  P1588_OFFSET                0x00000100
#define  P1588_COMMON_BASE           0x00005000
#define  P1588_COMMON_MAX            0x00005012
#define  P1588_EGR_PARSER_BASE       0x00006000
#define  P1588_EGR_PARSER_MAX        0x00006037
#define  P1588_ING_PARSER_BASE       0x00006080
#define  P1588_ING_PARSER_MAX        0x000060b7
#define  P1588_EGR_TS_BASE           0x00007000
#define  P1588_EGR_TS_MAX            0x0000700C
#define  P1588_ING_TS_BASE           0x00007080
#define  P1588_ING_TS_MAX            0x0000709b
#define  P1588_TS_BASE               0x00007800
#define  P1588_TS_MAX                0x00007805

/* TimeSync 1588 base configuration */
#define TIMESYNC_BASE_SETTING(tscfg) do {\
            memset(&tscfg, 0, sizeof(bcm_plp_timesync_config_t));\
            tscfg.flags           = 0x78;\
            tscfg.itpid           = 0x8100;\
            tscfg.otpid           = 0x88a8;\
            tscfg.otpid1          = 0x9100;\
            tscfg.otpid2          = 0x9200;\
            tscfg.itpid_igr       = 0x8100;\
            tscfg.otpid_igr       = 0x88a8;\
            tscfg.otpid1_igr      = 0x9100;\
            tscfg.otpid2_igr      = 0x9200;\
            tscfg.tx_inband_clear = 1;\
            tscfg.tx_partial_tc   = 0;\
            tscfg.tx_mode_tc      = 1;\
            tscfg.rx_partial_tc   = 0;\
            tscfg.rx_mode_tc      = 1;\
            tscfg.ip4udp_chksum_disable = 0;\
            tscfg.vxlan_outer_chksum_disable = 0;\
            tscfg.sopmem_intr_enable = 1;\
            tscfg.sopmem_wr_on_err = 0;\
        } while(0)


#define  FOR_LINE_SYS_SIDE(_rv, _pi, _fn)   do {           \
             (_pi).if_side = SYS_SIDE ;       _rv |= _fn;  \
             printf("------ rv=%d side=%d\n", _rv, (_pi).if_side); \
             (_pi).if_side = LINE_SIDE;       _rv |= _fn;  \
             printf("------ rv=%d side=%d\n", _rv, (_pi).if_side); \
         } while ( 0 )

#define  FORBOTH_LINE_SYS_SIDE(_pi)       \
         for ( (_pi).if_side = SYS_SIDE;  \
               (signed) ((_pi).if_side) >= LINE_SIDE; ((_pi).if_side)-- )


unsigned int failover_lanemap_get(char *chipname, bcm_plp_access_t phyinfo) {
    int speed = 0, iftype = 0, refclk = 0, ifmode = 0;
    bcm_plp_aperta_device_aux_modes_t  auxmode;
    phyinfo.if_side  = SYS_SIDE;

    bcm_plp_mode_config_get(chipname, phyinfo, &speed, &iftype,
                                      &refclk, &ifmode, (void*) &auxmode);
    printf("%s:%d p=[0x%x 0x%x + 0x%x]\n",__func__,__LINE__, phyinfo.phy_addr,
                           phyinfo.lane_map, auxmode.failover_config.lane_map);
    return  auxmode.failover_config.lane_map;
}

#if defined(PLP_APERTA2_FAILOVER)
#define  PROLOGUE_FOV(_c, _pi, _olm, _flm)  \
         unsigned int _olm = (_pi).lane_map, _flm = failover_lanemap_get(_c,_pi)
#define  ALSO_FOV_LANEMAP(_pi, _olm, _flm, _func)   do {          \
                             if ( _flm && (!(_flm & _olm) ) ) {   \
                                 (_pi).lane_map = _flm;  (_func); \
                                 (_pi).lane_map = _olm;           \
                             }                                    \
                             (_func);                             \
                         } while (0)
#else

#define  PROLOGUE_FOV(_c, _pi, _olm, _flm)
#define  ALSO_FOV_LANEMAP(_pi, _olm, _flm, _func)        (_func)
#endif  /* PLP_APERTA2_FAILOVER */

/* ingress and egress phy_info data structure */
extern int p_ctxt;
bcm_plp_access_t    pinfo_igr        = ((bcm_plp_access_t) {&p_ctxt, IGR_PORT, SYS_SIDE,  SYS_IGR_LANE});
bcm_plp_access_t    pinfo_egr        = ((bcm_plp_access_t) {&p_ctxt, EGR_PORT, SYS_SIDE,  SYS_EGR_LANE});
bcm_plp_access_t    pinfo_igr_line   = ((bcm_plp_access_t) {&p_ctxt, IGR_PORT, LINE_SIDE, LINE_IGR_LANE});
bcm_plp_access_t    pinfo_egr_line   = ((bcm_plp_access_t) {&p_ctxt, EGR_PORT, LINE_SIDE, LINE_EGR_LANE});

void ts_reg_dump_header(unsigned int regad[], int offset16) {
    int ii = 0;

    if ( offset16 )  { printf("       "); }
    for ( ii = 0; regad[ii] < PHY_INVALID_ARG; ii++ ) {
        if ( ! offset16 ) {
            unsigned int disp_addr = (regad[ii] < 0x10000)
                                    ? ((0xffff & P1588_BASE) | regad[ii])
                                    : ( 0xffff & regad[ii]);
            printf("=%04X", disp_addr);
        } else {
            printf("---%X ", regad[ii]);
        }
    }
    printf("==\n");
}

int ts_reg_dump_group(char *chipname, bcm_plp_access_t *pinfo, unsigned int regad_seg,
                                       unsigned int regad[], unsigned int regvl[]) {
    unsigned int val = 0, devad = 1;
    int rv = 0, ii = 0;

    if ( regad_seg != P1588_BASE ) {
        printf("[%04X]", regad_seg & 0x0000ffff);   /* print the hex-segment head when */
                                                    /* dumping all 1588 registers      */
    }
    for ( ii = 0; regad[ii] < PHY_INVALID_ARG; ii++ ) {
        unsigned int reg_addr = 0x00;
        reg_addr = regad[ii] + regad_seg;
        rv |= bcm_plp_reg_value_get(chipname, *pinfo, devad, reg_addr, &val);
        printf(" %04x", val & 0xffff);
    }
    printf(" [p=%02d.%02x]\n", pinfo->phy_addr , pinfo->lane_map);

    return  rv;
}

/* dump key P1588 registers */
int ts_reg_dump_igr_egr(char *chipname, bcm_plp_access_t pinfo, unsigned int regad[], unsigned int regvl[]) 
{
    int rv = 0;
    ts_reg_dump_header(regad, FALSE);

    rv |= ts_reg_dump_group(chipname, &pinfo, P1588_BASE, regad, regvl);
    return  rv;
}

unsigned int regad_0[DUMP_MAX] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                   0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, PHY_INVALID_ARG };

/* dump all P1588 registers */
int ts_reg_dump_all(char *chipname, bcm_plp_access_t pinfo) {
    int rv = 0, jj = 0;
    ts_reg_dump_header(regad_0, TRUE);

    for ( jj = P1588_COMMON_BASE; jj < P1588_COMMON_MAX; jj += 0x10 ) {
        rv |= ts_reg_dump_group(chipname, &pinfo, (P1588_BASE | jj), regad_0, NULL);
    }
    for ( jj = P1588_EGR_PARSER_BASE; jj < P1588_EGR_PARSER_MAX; jj += 0x10 ) {
        rv |= ts_reg_dump_group(chipname, &pinfo, (P1588_BASE | jj), regad_0, NULL);
    }
    for ( jj = P1588_ING_PARSER_BASE; jj < P1588_ING_PARSER_MAX; jj += 0x10 ) {
        rv |= ts_reg_dump_group(chipname, &pinfo, (P1588_BASE | jj), regad_0, NULL);
    }
    for ( jj = P1588_EGR_TS_BASE; jj < P1588_EGR_TS_MAX; jj += 0x10 ) {
        rv |= ts_reg_dump_group(chipname, &pinfo, (P1588_BASE | jj), regad_0, NULL);
    }
    for ( jj = P1588_ING_TS_BASE; jj < P1588_ING_TS_MAX; jj += 0x10 ) {
        rv |= ts_reg_dump_group(chipname, &pinfo, (P1588_BASE | jj), regad_0, NULL);
    }
    for ( jj = P1588_TS_BASE; jj < P1588_TS_MAX; jj += 0x10 ) {
        rv |= ts_reg_dump_group(chipname, &pinfo, (P1588_BASE | jj), regad_0, NULL);
    }
    return  rv;
}

unsigned int regad_common[DUMP_MAX]       = { 0x5000, 0x5001, 0x5002, 0x5003, 0x5004, 0x5005, 
                                               0x5006, 0x5007, 0x5008, 0x500d, 0x500e, PHY_INVALID_ARG };
unsigned int regad_egr_ts[DUMP_MAX]       = { 0x7000, 0x7001, 0x7002, PHY_INVALID_ARG };
unsigned int regad_ing_ts[DUMP_MAX]       = { 0x7080, 0x7081, 0x7082, PHY_INVALID_ARG };
unsigned int regad_egr_parser[DUMP_MAX]   = { 0x6000, 0x6001, PHY_INVALID_ARG };
unsigned int regad_ing_parser[DUMP_MAX]   = { 0x6080, 0x6081, PHY_INVALID_ARG };
unsigned int regad_ts[DUMP_MAX]           = { 0x7800, 0x7801, 0x7802, 0x7803, 0x7805, 0x7805, PHY_INVALID_ARG };
unsigned int regad_egr_counters[DUMP_MAX] = { 0x6031, 0x6032, 0x6033, 0x6034, 0x6035, 0x6036, PHY_INVALID_ARG };
unsigned int regad_ing_counters[DUMP_MAX] = { 0x60b1, 0x60b2, 0x60b3, 0x60b4, 0x60b5, 0x60b6, PHY_INVALID_ARG };
unsigned int regad_mpls[DUMP_MAX]         = { 0x600c, 0x608c, PHY_INVALID_ARG };

/* dump P1588 registers in group */
int ts_reg_dump(char *chipname, bcm_plp_access_t pinfo, int nn) 
{
    int rv = 0;

    switch ( nn ) {
    case 0 :
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_common, NULL);
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_egr_ts, NULL);
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_ing_ts, NULL);
        break;
    case 1 :
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_common, NULL);
        break;
    case 2 :
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_egr_ts, NULL);
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_ing_ts, NULL);
        break;
    case 3 :
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_egr_parser, NULL);
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_ing_parser, NULL);
        break;
    case 4 :
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_ts, NULL);
        break;
    case 5 :
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_egr_counters, NULL);
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_ing_counters, NULL);
        break;
    case 6 :
        rv |= ts_reg_dump_igr_egr(chipname, pinfo, regad_mpls, NULL);
        break;
    }

    return  rv;
}

/* magic number to reset TimeSync 1588 when setting TimeStamp T1/T2/T3/T4 via CLI shells */
#define  P1588_RESET_FROM_CLI           9999

/* bitmap for CLI commands to designate IEEE1588 RESET types */
#define  P1588_RESET_SOFT               bcmplpTimesyncEnCtrlSwRstb
#define  P1588_RESET_REG                bcmplpTimesyncEnCtrlSwRstbReg
#define  P1588_RESET_NSE                bcmplpTimesyncEnCtrlSwRstbNse
#define  P1588_RESET_ALL                (P1588_RESET_SOFT | P1588_RESET_REG | P1588_RESET_NSE)

/* reset TimeSync 1588 engine */
int ts_reset(char *chipname, bcm_plp_access_t phyinfo, int rst_type) {
    unsigned int val = 1;
    int rv = 0;
    PROLOGUE_FOV(chipname, phyinfo, orilm, fovlm);

    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
        rv |= bcm_plp_timesync_enable_set(chipname, phyinfo, rst_type,  val) );
    printf("rv=%d [p=%d.%02x] RESET (type=0x%x)\n", rv, phyinfo.phy_addr, phyinfo.lane_map, rst_type);
#if defined(PLP_PM_TIMESTAMPING_SUPPORT)
    bcm_plp_mac_access_t macinfo;
    unsigned int enable = FALSE;
    int step = 1;
    /* Enable PortMacro (PM) timestamping for PM TC/t12 modes */
    memcpy(&(macinfo.phy_info), &phyinfo, sizeof(bcm_plp_access_t));
    macinfo.phy_info.if_side = LINE_SIDE;
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_pm_timesync_enable_set(chipname, macinfo, (2 == step) ? 0x1 : 0x5, enable) );
    printf("PortMacro TimeSync set: p=[%d.0x%02x] %d-step enable:%d\n",
                                phyinfo.phy_addr, phyinfo.lane_map, step, enable);

    enable = 0;
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_pm_timesync_enable_get(chipname, macinfo, (2 == step) ? 0x1 : 0x5, &enable) );
    printf("PortMacro TimeSync get: p=[%d.0x%02x] %d-step enable:%d\n",
                                phyinfo.phy_addr, phyinfo.lane_map, step, enable);
#endif  /* PLP_PM_TIMESTAMPING_SUPPORT */

    return  rv;
}

/*
 *  MPLS
 */
int ts_mpls(char *chipname, bcm_plp_access_t phyinfo, int rxtx, int count, int enable, int mpls_flags) {
    int rv = 0, ii = 0;
    bcm_plp_timesync_mpls_ctrl_t  mpls;
    PROLOGUE_FOV(chipname, phyinfo, orilm, fovlm);

    if ( 0 == count ) {
        memset(&mpls, 0, sizeof(bcm_plp_timesync_mpls_ctrl_t));
        mpls.size = 10;     /* clear all 10 labels */
    } else {
        /*  the MPLS label values here are for Broadcom's unit tests  *\
        \*                      User should prepare their own labels  */
        mpls.labels[0].value = 0xaaaaa;  mpls.labels[0].mask = 0xfffff;  mpls.labels[0].flags = mpls_flags;
        mpls.labels[1].value = 0x11111;  mpls.labels[1].mask = 0xfffff;  mpls.labels[1].flags = mpls_flags;
        mpls.labels[2].value = 0x22222;  mpls.labels[2].mask = 0xfffff;  mpls.labels[2].flags = mpls_flags;
        mpls.labels[3].value = 0x33333;  mpls.labels[3].mask = 0xfffff;  mpls.labels[3].flags = mpls_flags;
        mpls.labels[4].value = 0x44444;  mpls.labels[4].mask = 0xfffff;  mpls.labels[4].flags = mpls_flags;
        mpls.labels[5].value = 0x55555;  mpls.labels[5].mask = 0xfffff;  mpls.labels[5].flags = mpls_flags;
        mpls.labels[6].value = 0x66666;  mpls.labels[6].mask = 0xfffff;  mpls.labels[6].flags = mpls_flags;
        mpls.labels[7].value = 0x77777;  mpls.labels[7].mask = 0xfffff;  mpls.labels[7].flags = mpls_flags;
        mpls.labels[8].value = 0x88888;  mpls.labels[8].mask = 0xfffff;  mpls.labels[8].flags = mpls_flags;
        mpls.labels[9].value = 0x99999;  mpls.labels[9].mask = 0xfffff;  mpls.labels[9].flags = mpls_flags;
        mpls.size  = count;
        if (enable == 1)
            mpls.flags = bcmplpTimesyncMplsEnable | bcmplpTimesyncMplsCtrlWord;
        else
            mpls.flags = bcmplpTimesyncMplsNone;
    }
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_mpls_set(chipname, phyinfo, rxtx,  enable, &mpls) );
    printf("mpls_set => rv=%d port[%d.0x%04x] MPLS rxtx=%d count=%d en=%d\n", rv, phyinfo.phy_addr, phyinfo.lane_map, rxtx, count, enable);
    if ( count > 0 ) {
        /* get the setting back for checking */
        memset(&mpls, 0, sizeof(bcm_plp_timesync_mpls_ctrl_t));
            rv |= bcm_plp_timesync_mpls_get(chipname, phyinfo, rxtx, &enable, &mpls);
        printf("mpls_get => rv=%d port[%d.0x%04x] MPLS ", rv, phyinfo.phy_addr, phyinfo.lane_map);
        printf("enable=%d flags=0x%x Labels=", enable, mpls.flags);
        for ( ii = 0; ii < count; ii++ ) {
            printf(" 0x%05X(%x)", mpls.labels[ii].value, mpls.labels[ii].flags);
        }
        printf("\n");
    }

    return  rv;
}

/* clear SOPmem  */
int _ts_sopmem_individual_clear(char *chipname, bcm_plp_access_t phyinfo) {
    /* bcm_plp_timesync_sopmem_get() does not accept NULL parameters */
    bcm_plp_timesync_sopmem_t sopmem[16];
    int  rv = 0;
    int  ii = 0;
    int index = 0;
    unsigned long long tstmp;
    PROLOGUE_FOV(chipname, phyinfo, orilm, fovlm);
    memset(&sopmem, 0, sizeof(bcm_plp_timesync_sopmem_t)*16);

    for (index=0;index<16;index++)
    {
        sopmem[index].valid = 1;
        sopmem[index].seq_id = index;
    }

    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_sopmem_get(chipname, phyinfo, 0, 0, sopmem));

    for (index=0;index<16;index++)
    {
        printf("sopmem[%d].valid:%d    sopmem[%d].seq_id:%d    \n", index, sopmem[index].valid, index, sopmem[index].seq_id);
        if (sopmem[index].valid == 1) {
            printf("v=%x dir=%d type=%d seqId=0x%x dn=0x%x vid=0x%x ad=0x%x.%x.%x.%x.%x.%x.%x.%x TS=0x",
                sopmem[index].valid, sopmem[index].direction, sopmem[index].msg_type, sopmem[index].seq_id, sopmem[index].domain_num, sopmem[index].vlan_id,
                                 sopmem[index].classified_data[7], sopmem[index].classified_data[6], sopmem[index].classified_data[5],
                                 sopmem[index].classified_data[4], sopmem[index].classified_data[3], sopmem[index].classified_data[2],
                                 sopmem[index].classified_data[1], sopmem[index].classified_data[0]);
            for ( ii = 9; ii >= 0; ii-- ) {
                    tstmp <<= 8;
                    tstmp  |= (unsigned int) sopmem[index].timestamp[ii];
                printf("%02x", sopmem[index].timestamp[ii]);
                if ( ii && (0x0 == (ii & 0x1)) )   printf("_");  /* separators at every 4th nibble */
            }
        }
    }
    return  rv;
}

int ts_sopmem_clear(char *chipname) {
    int  rv = 0;
    rv |= _ts_sopmem_individual_clear(chipname, pinfo_igr);
    rv |= _ts_sopmem_individual_clear(chipname, pinfo_egr);

    return  rv;
}

/*
 *  SOPMEM
 */
/* lookup SOPmem */
int __ts_sopmem(char *chipname, bcm_plp_access_t phyinfo, unsigned int seqid, unsigned int eseqid, unsigned int direction, unsigned long long *tstmp, unsigned long long *ingsrcport) 
{
    bcm_plp_timesync_sopmem_t  sopmem[16];

    int ii, rv = 0;
    int index = 0;

    memset(&sopmem, 0, sizeof(bcm_plp_timesync_sopmem_t)*16);
    sopmem[0].seq_id = seqid;
    sopmem[0].seq_id_encrypt = eseqid;
    sopmem[0].direction = direction;

    rv |= bcm_plp_timesync_sopmem_get(chipname, phyinfo, 0, 0, sopmem);   /* lookup SOPmem  */
    printf("SOPmem[%2d.%02x]\n", phyinfo.phy_addr, phyinfo.lane_map);
    if ( 0 == rv ) {
        for (index=0;index<16;index++)
        {
            if ((sopmem[index].valid == 1) || (sopmem[index].valid == 3))
            {
                printf("Index:%d Valid:%d Direction:%d SeqId:0x%x ESeqId:0x%x Timestamp:0x", index, sopmem[index].valid,
                                      sopmem[index].direction, sopmem[index].seq_id, sopmem[index].seq_id_encrypt);
                for ( ii = 9; ii >= 0; ii-- ) {
                    if ( NULL != tstmp ) {
                        *tstmp <<= 8;
                        *tstmp  |= (unsigned int) sopmem[index].timestamp[ii];
                    }
                    printf("%02x", sopmem[index].timestamp[ii]);
                    if ( ii && (0x0 == (ii & 0x1)) )   printf("_");  /* separators at every 4th nibble */
                }
                    printf(" SrcPort:0x");
                for ( ii = 9; ii >= 0; ii-- ) {
                    if ( NULL != ingsrcport ) {
                        *ingsrcport <<= 8;
                        *ingsrcport  |= (unsigned int) sopmem[index].src_port_id[ii];
                    }
                    printf("%02x", sopmem[index].src_port_id[ii]);
                    if ( ii && (0x0 == (ii & 0x1)) )   printf("_");  /* separators at every 4th nibble */
                }
                printf("\n");
            }
        }
    } else if ( -8 == rv ) {
        printf(" empty or no entry matched\n");
        rv = 0;     /* reset the return value */
    } else {
        printf(" ERROR !!\n");
    }

    return  rv;
}

int ts_sopmem(char *chipname, bcm_plp_access_t phyinfo, unsigned int seqid, unsigned int eseqid, unsigned int direction, unsigned long long *tstmp, unsigned long long *ingsrcport) {
    int rv = 0;
    PROLOGUE_FOV(chipname, phyinfo, orilm, fovlm);

    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv = __ts_sopmem(chipname, phyinfo, seqid, eseqid, direction, tstmp, ingsrcport) );
    return  rv;
}

/* disable IEEE1588 TimeSync */
int ts_off(char *chipname, bcm_plp_access_t phyinfo, int rst_type) {
    int rv = 0;
    PROLOGUE_FOV(chipname, phyinfo, orilm, fovlm);

    rv |= ts_reset(chipname, phyinfo, rst_type);
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, phyinfo, TIMESYNC_DIRECTION_RXTX,
                                                               TIMESYNC_DISABLE) );
    printf("Disable TimeSync: p=[%d.0x%04x]\n", phyinfo.phy_addr, phyinfo.lane_map);
    return  rv;
}

/*
 * TimeSync 1588 config sequence
 */
int ts_t1_t2_t3_t4(char *chipname, bcm_plp_access_t phyinfo, bcm_plp_access_t phyinfo_line, int txrx, int timesync_ptp_mode, int encryption, int ieee_brcm_mode,
                                                                 int mode_tc, int onestep_twostep) 
{
    int rv = 0;
    bcm_plp_timesync_config_t cfg;
    bcm_plp_timesync_framesync_t framesync;

    PROLOGUE_FOV(chipname, phyinfo, orilm, fovlm);

    TIMESYNC_BASE_SETTING(cfg);

    printf("txrx:%d timesync_ptp_mode:%d ieee_brcm_mode:%d mode_tc:%d\n", txrx, timesync_ptp_mode, ieee_brcm_mode, mode_tc);

    /* Enable TimeSync */
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, phyinfo, txrx, TIMESYNC_ENABLE) );
    if (rv != 0) {
        printf("Timesync Enable Set failed :%d\n", rv);
        return rv;
    }

    if (timesync_ptp_mode != bcmplpTimesyncNseEnabled) {
        bcm_plp_mac_access_t macinfo;
        unsigned int enable = TRUE;
        /* Enable PortMacro (PM) timestamping for PM TC/t12 modes */
        memcpy(&(macinfo.phy_info), &phyinfo_line, sizeof(bcm_plp_access_t));
        macinfo.phy_info.if_side = LINE_SIDE;
        ALSO_FOV_LANEMAP( phyinfo_line, orilm, fovlm,
                rv |= bcm_plp_pm_timesync_enable_set(chipname, macinfo, (2 == onestep_twostep) ? 0x11 : 0x15, enable) );
        printf("PortMacro TimeSync set: p=[%d.0x%02x] %d-step enable:%d rv:%d\n",
                                    phyinfo_line.phy_addr, phyinfo_line.lane_map, onestep_twostep, enable, rv);

        enable = 0;
        ALSO_FOV_LANEMAP( phyinfo_line, orilm, fovlm,
                rv |= bcm_plp_pm_timesync_enable_get(chipname, macinfo, (2 == onestep_twostep) ? 0x11 : 0x15, &enable) );
        printf("PortMacro TimeSync get: p=[%d.0x%02x] %d-step enable:%d rv:%d\n",
                                    phyinfo_line.phy_addr, phyinfo_line.lane_map, onestep_twostep, enable, rv);
    }

    /* Configure PPS Enable & TOD Load option */
    memset(&framesync, 0, sizeof(bcm_plp_timesync_framesync_t));
    framesync.flags = bcmplpTimesyncSyncmodeCtrlPPSEnable | bcmplpTimesyncSyncmodeCtrlTODLoadOnce;
    printf("Framesync.flags set : 0x%x\n", framesync.flags);
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_framesync_mode_set(chipname, phyinfo, 0, &framesync) );
    memset(&framesync, 0, sizeof(bcm_plp_timesync_framesync_t));
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_framesync_mode_get(chipname, phyinfo, 0, &framesync) );
    printf("Framesync.flags get : 0x%x\n", framesync.flags);

    /* L2/IPv4/IPv6 Flags */
    cfg.flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;
    /* PTP Mode */
    cfg.flags |= timesync_ptp_mode;

    if (encryption == 1) {
        /* Enable Encryption for PTP packets */
        printf("Encryption enabled\n");
        cfg.flags |= bcmplpTimesyncFlag1588EncryptedMode;
    }

    cfg.tx_inband_clear = 1;              /* To clear Reserved Field */
    cfg.tx_partial_tc =   ieee_brcm_mode; /* IEEE TS Mode */
    cfg.tx_mode_tc =      mode_tc;        /* 0->BC Mode, 1->TC Mode */
    cfg.rx_partial_tc =   ieee_brcm_mode; /* IEEE TS Mode */
    cfg.rx_mode_tc =      mode_tc;        /* 0->BC Mode, 1->TC Mode */

    cfg.ip4udp_chksum_disable = 0; /* Disable ipv4 UDP checksum calculation- applicable only for EGR */
    cfg.vxlan_outer_chksum_disable = 0;

    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, phyinfo, &cfg) );

    return rv;
}

/*
 * TimeSync 1588 config sequence for IEEE or BRCM TC Mode
 */
int ts_tc_ieee_brcm(char *chipname, bcm_plp_access_t phyinfo, int txrx, int timesync_ptp_mode, int encryption, 
                                long long ts_offset_rx, long long ts_offset_tx, int ieee_brcm_mode, int onestep_twostep)
{
    int rv = 0;
    bcm_plp_timesync_time_value_t  timevalue;
    bcm_plp_timesync_config_t cfg;
    bcm_plp_timesync_framesync_t framesync;

    PROLOGUE_FOV(chipname, phyinfo, orilm, fovlm);

    TIMESYNC_BASE_SETTING(cfg);
    memset(&framesync, 0, sizeof(bcm_plp_timesync_framesync_t));

    /* Enable TimeSync */
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, phyinfo, txrx, TIMESYNC_ENABLE) );

    /* Set Rx timestamp offset */
    timevalue.direction  =  bcmplpTrafficRx;
    timevalue.op         =  (ts_offset_rx >= 0) ? bcmplpMathOperatorNone
                                                : bcmplpMathOperatorSubtract;
    timevalue.type       =  TIMESYNC_MESSAGE_TYPE_ALL;
    timevalue.time_value =  llabs(ts_offset_rx);
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timestamp_offset_set(chipname, phyinfo, 0, &timevalue) );

    /* Set Tx timestamp offset */
    timevalue.direction  =  bcmplpTrafficTx;
    timevalue.op         =  (ts_offset_tx >= 0) ? bcmplpMathOperatorNone
                                                : bcmplpMathOperatorSubtract;
    timevalue.type       =  TIMESYNC_MESSAGE_TYPE_ALL;
    timevalue.time_value =  llabs(ts_offset_tx);
    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timestamp_offset_set(chipname, phyinfo, 0, &timevalue) );

    /* Configure PPS Enable & TOD Load option */
    framesync.flags = bcmplpTimesyncSyncmodeCtrlPPSEnable | bcmplpTimesyncSyncmodeCtrlTODLoadOnce;
    rv |= bcm_plp_timesync_framesync_mode_set(chipname, phyinfo, 0, &framesync);
    memset(&framesync, 0, sizeof(bcm_plp_timesync_framesync_t));
    rv |= bcm_plp_timesync_framesync_mode_get(chipname, phyinfo, 0, &framesync);
    printf("Framesync.flags : 0x%x\n", framesync.flags);

    /* L2/IPv4/IPv6 Flags */
    cfg.flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;

    /* PTP Mode */
    cfg.flags |= timesync_ptp_mode;

    if (encryption == 1) {
        /* Enable Encryption for PTP packets */
        cfg.flags |= bcmplpTimesyncFlag1588EncryptedMode;
    }

    cfg.tx_inband_clear = 1;              /* To clear Reserved Field */
    cfg.tx_partial_tc =   ieee_brcm_mode; /* IEEE TS Mode */
    cfg.tx_mode_tc =      1;              /* TC Mode      */
    cfg.rx_partial_tc =   ieee_brcm_mode; /* IEEE TS Mode */
    cfg.rx_mode_tc =      1;              /* TC Mode      */

    cfg.ip4udp_chksum_disable = 0;        /* Disable ipv4 UDP checksum calculation- applicable only for EGR */
    cfg.vxlan_outer_chksum_disable = 0;

    ALSO_FOV_LANEMAP( phyinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, phyinfo, &cfg) );
    return rv;
}

int ts_tod_set(char *chipname, bcm_plp_access_t phyinfo, unsigned int flags, unsigned long long seconds, unsigned int nanoseconds, unsigned long long syncrefdelay)
{
    int rv = 0;
    bcm_plp_timesync_timespec_t timesync;
    unsigned long long nanoseconds64;

    timesync.isnegative = 0;                 /* Not used, Sign identifier */
    timesync.seconds = seconds;              /* Seconds absolute value     */
    timesync.nanoseconds = nanoseconds;      /* Nanoseconds absolute value */
    timesync.nanoseconds64 = 0;              /* Not used, calculated Internally */

    timesync.syncref_delay.enable = 1;
    timesync.syncref_delay.value = syncrefdelay;

    rv = bcm_plp_timesync_time_code_set(chipname, phyinfo, flags, &timesync);
    if (flags == bcmplpTimesyncTimeCode48bit) {
        nanoseconds64 = timesync.seconds * (u_int64_t)1000000000 + (u_int64_t)timesync.nanoseconds ;
        printf("rv=%d Setting Time of Day(48nanoseconds) = 0x%llx (ToD) for port [%2d.%02x]\n", rv, 
                                                     nanoseconds64, phyinfo.phy_addr , phyinfo.lane_map);
    } else if (flags == bcmplpTimesyncTimeCode80bit) {
        printf("rv=%d Setting Time of Day(48seconds.32nanoseconds) = 0x%llx.0x%x (ToD) for port [%2d.%02x]\n", rv, 
                                                   seconds, nanoseconds, phyinfo.phy_addr , phyinfo.lane_map);
    }

    sleep(1);
    timesync.seconds = 0;
    timesync.nanoseconds = 0;
    timesync.nanoseconds64 = 0;

    rv = bcm_plp_timesync_time_code_get(chipname, phyinfo, flags, &timesync);
    if (flags == bcmplpTimesyncTimeCode48bit) {
        printf("rv=%d Getting Time of Day(48nanoseconds) = 0x%llx (ToD) for port [%2d.%02x]\n", rv, 
                                                 timesync.nanoseconds64, phyinfo.phy_addr , phyinfo.lane_map);
    } else if (flags == bcmplpTimesyncTimeCode80bit) {
        printf("rv=%d Getting Time of Day(48seconds.32nanoseconds) = 0x%llx.0x%x (ToD) for port [%2d.%02x]\n", rv, 
                                 timesync.seconds, timesync.nanoseconds, phyinfo.phy_addr , phyinfo.lane_map);
    }
    return rv;
}

int ts_tod_get(char *chipname, bcm_plp_access_t phyinfo, unsigned int flags, unsigned long long *seconds, unsigned int *nanoseconds, unsigned long long *syncrefdelay)
{
    int rv = 0;
    bcm_plp_timesync_timespec_t timesync;
    memset(&timesync, 0, sizeof(bcm_plp_timesync_timespec_t));

    rv = bcm_plp_timesync_time_code_get(chipname, phyinfo, flags, &timesync);
    if (flags == bcmplpTimesyncTimeCode48bit) {
        printf("rv=%d Getting Time of Day(48nanoseconds) = 0x%llx (ToD) for port [%2d.%02x]\n", rv, 
                                                 timesync.nanoseconds64, phyinfo.phy_addr , phyinfo.lane_map);
        *nanoseconds = timesync.nanoseconds64;
        *syncrefdelay = timesync.syncref_delay.value;
    } else if (flags == bcmplpTimesyncTimeCode80bit) {
        printf("rv=%d Getting Time of Day(48seconds.32nanoseconds) = 0x%llx.0x%x (ToD) for port [%2d.%02x]\n", rv, 
                                 timesync.seconds, timesync.nanoseconds, phyinfo.phy_addr , phyinfo.lane_map);
        *seconds = timesync.seconds;
        *nanoseconds = timesync.nanoseconds64;
        *syncrefdelay = timesync.syncref_delay.value;
    }
    return rv;
}

int ts_heartbeat_show(char *chipname, bcm_plp_access_t phyinfo,
                      int flags, unsigned long long *heartbeat) 
{
    int rv = 0;
    bcm_plp_timesync_timespec_t timespec;
    memset(&timespec, 0, sizeof(bcm_plp_timesync_timespec_t));

    if ((flags == bcmplpTimesyncHeartBeat48bit) || (flags == bcmplpTimesyncHeartBeat48bitIeee)) {
        rv |= bcm_plp_timesync_time_code_get(chipname, phyinfo, flags, &timespec);
        printf("rv=%d port[%2d.%02x] HeartBeat = ", rv, phyinfo.phy_addr, phyinfo.lane_map);
        if ( flags == bcmplpTimesyncHeartBeat48bit ) {
            *heartbeat = timespec.nanoseconds64;
        } else if ( flags == bcmplpTimesyncHeartBeat48bitIeee ) {
            *heartbeat = ((timespec.seconds & 0xFFFFFF) << 32) | timespec.nanoseconds;
        } else if ( flags == bcmplpTimesyncHeartBeat80bit ) {
        }
        printf("%04x_%04x_%04x\n", (unsigned int) (((*heartbeat) >> 32) & 0xFFFF),
                                    (unsigned int) (((*heartbeat) >> 16) & 0xFFFF),
                                    (unsigned int) (((*heartbeat) >>  0) & 0xFFFF));
    }

#ifdef DUMP_PTP_REGISTERS
    rv |= ts_reg_dump(chipname, phyinfo, 1);
#endif
    return rv;
}

int ts_local_time_code_show(char *chipname, bcm_plp_access_t phyinfo, unsigned int flags) 
{
    int rv = 0;
    bcm_plp_timesync_timespec_t timespec;
    memset(&timespec, 0, sizeof(bcm_plp_timesync_timespec_t));

    rv |= bcm_plp_timesync_time_code_get( chipname, phyinfo, flags, &timespec);
    if ( flags == bcmplpTimesyncTimeCode48bit ) {
        printf("rv=%d port [%2d.%02x] timecode=(0x%llx)", rv, phyinfo.phy_addr, 
                                           phyinfo.lane_map, timespec.nanoseconds64);
    } else if ( flags == bcmplpTimesyncTimeCode80bit ) {
        printf("rv=%d port [%2d.%02x] timecode=( 0x%llx.0x%x )", rv, phyinfo.phy_addr, 
                           phyinfo.lane_map, timespec.seconds, timespec.nanoseconds);
    }

    return rv;
}

/*
 *  TimeSync 1588 Unit Test configurations
 */
int timesync_conf_t1234_tc(char *chipname,  char *opt, unsigned int arg[]) 
{
    int rv = 0, flags = 0;
    long long ts_offset_rx = 0LL, ts_offset_tx = 0LL;
    bcm_plp_timesync_config_t icfg;
    bcm_plp_timesync_config_t ecfg;
    int ieee_brcm_mode = 0;
    int mode_tc = 0;
    int onestep_twostep = 1;
    int timesync_ptp_mode = 0;
    int ptp_encrypt = 0;

    TIMESYNC_BASE_SETTING(icfg);
    TIMESYNC_BASE_SETTING(ecfg);

    IS_STRMATCH2(opt, "reset", "rst") {      /* reset TimeSync 1588 setting */
        int reset_type = 0;
        if ((arg[0] < 0x1) || (0x7 < arg[0]))
            reset_type |= P1588_RESET_ALL;                /* reset ALL by default */

        if ((arg[0] & 0x1)== 0x1)
            reset_type |= bcmplpTimesyncEnCtrlSwRstbNse;  /* reset NSE 1588 block only */

        if ((arg[0] & 0x2) == 0x2)
            reset_type |= bcmplpTimesyncEnCtrlSwRstbReg;  /* reset 1588 register block only */

        if ((arg[0] & 0x4) == 0x4)
            reset_type |= bcmplpTimesyncEnCtrlSwRstb;     /* reset 1588 block, Datapath & State Machine */

        rv |= ts_reset(chipname, pinfo_igr, reset_type);
        rv |= ts_reset(chipname, pinfo_egr, reset_type);
        return  rv;
    } else

    IS_STRMATCH(opt, "tod") {                /* set Time of Day (ToD) */
        unsigned long long syncrefdelay = 0;
        unsigned long long seconds = 0;
        unsigned int nanoseconds = 0;
        unsigned int flags = 0;

        if ( IS_PHY_INVALID_ARG(arg[0]) )    arg[0] = 0;
        if ( IS_PHY_INVALID_ARG(arg[1]) )    arg[1] = 0;
        if ( IS_PHY_INVALID_ARG(arg[2]) )    arg[2] = 0;
        if ( IS_PHY_INVALID_ARG(arg[3]) )    arg[3] = 0;
        if ( IS_PHY_INVALID_ARG(arg[4]) )    arg[4] = 0;

        if (arg[0] == 0)
            flags = bcmplpTimesyncTimeCode48bit;
        if (arg[0] == 1)
            flags = bcmplpTimesyncTimeCode80bit;

        seconds     = (unsigned long long)arg[1]<<32 | arg[2];
        nanoseconds = arg[3];
        syncrefdelay = arg[4];
        rv |= ts_tod_set(chipname, pinfo_igr, flags, seconds, nanoseconds, syncrefdelay);
        rv |= ts_tod_set(chipname, pinfo_egr, flags, seconds, nanoseconds, syncrefdelay);

        return  rv;
    } else

    IS_STRMATCH2(opt, "heartbeat", "hb") {   /* get the Heartbeat */
        unsigned int flags = 0;

        if ( IS_PHY_INVALID_ARG(arg[0]) )    arg[0] = 0;
        if ( IS_PHY_INVALID_ARG(arg[1]) )    arg[1] = 10;

        if (arg[0] == 0)
            flags = bcmplpTimesyncHeartBeat48bit;
        if (arg[0] == 1)
            flags = bcmplpTimesyncHeartBeat48bitIeee;

        int count = 0;
        unsigned long long hba = 0LL, hbb = 0LL;
        for ( count = 0; count < arg[1]; count++ ) {
            rv |= ts_heartbeat_show(chipname, pinfo_igr, flags, &hba);
            rv |= ts_heartbeat_show(chipname, pinfo_egr, flags, &hbb);
            if (flags == bcmplpTimesyncHeartBeat48bit) {
                printf("\tHeartbeat diff = %lld nanoseconds\n"  , (hbb - hba));
            } else if (flags == bcmplpTimesyncHeartBeat48bitIeee) {
                printf("\tHeartbeat diff = %lld seconds, %lld nanoseconds\n"  , ((hbb - hba)>>32 & 0xFFFF), ((hbb - hba) & 0xFFFFFFFF));
            }
            usleep(HEARTBEAT_FETCH_INTERVAL);/* sleep approximately 1 second */
        }

        return  rv;
    } else

    IS_STRMATCH2(opt, "localtime", "lt") {   /* get the Local Time */
        bcm_plp_timesync_timespec_t timesync;
        memset(&timesync, 0, sizeof(bcm_plp_timesync_timespec_t));

        if ( IS_PHY_INVALID_ARG(arg[0]) )    arg[0] = 0;
        if (arg[0] == 0)
            flags = bcmplpTimesyncTimeCode48bit;
        if (arg[0] == 1)
            flags = bcmplpTimesyncTimeCode80bit;

        rv |= bcm_plp_timesync_time_code_get( chipname, pinfo_igr, flags, &timesync);
        printf("rv=%d port [%2d.%02x] LocalTime Seconds=%lld  NanoSeconds=%d \n", rv,
                pinfo_igr.phy_addr, pinfo_igr.lane_map, timesync.seconds, timesync.nanoseconds);
        rv |= bcm_plp_timesync_time_code_get( chipname, pinfo_egr, flags, &timesync);
        printf("rv=%d port [%2d.%02x] LocalTime Seconds=%lld  NanoSeconds=%d \n", rv,
                pinfo_egr.phy_addr, pinfo_egr.lane_map, timesync.seconds, timesync.nanoseconds);
        return  rv;
    } else

    IS_STRMATCH2(opt, "tx", "txinfo") {   /* lookup TS TxInfo */
#if defined(PLP_PM_TIMESTAMPING_SUPPORT)
        bcm_plp_access_t        *phyinfo[] = { &pinfo_igr_line, &pinfo_egr_line };
        bcm_plp_mac_access_t     macinfo;
        bcm_plp_pm_ts_tx_info_t  ts_tx_info;
        int                      ii;

        for ( ii = 0; ii < 2; ii++ ) {
            memcpy(&(macinfo.phy_info), phyinfo[ii], sizeof(bcm_plp_access_t));
            macinfo.phy_info.if_side = LINE_SIDE;
            memset(&ts_tx_info, 0, sizeof(bcm_plp_pm_ts_tx_info_t));
            printf("TS_TxInfo[%2d.%02x] ", macinfo.phy_info.phy_addr, macinfo.phy_info.lane_map);
            rv = bcm_plp_pm_timesync_tx_info_get(chipname, macinfo, &ts_tx_info);
            if ( 0 == rv ) {
                printf("seqID=0x%x(%d) timestamp=0x%08x_%08x\n",
                        ts_tx_info.ts_seq_id ,ts_tx_info.ts_seq_id,
                        ts_tx_info.ts_in_fifo_hi, ts_tx_info.ts_in_fifo_lo);
            } else if ( -8 == rv ) {
                printf(" < empty >\n");
                rv = 0;     /* reset the return value */
            } else {
                printf(" ERROR!!! rv:%d\n", rv);
            }
        }
#endif  /* PLP_PM_TIMESTAMPING_SUPPORT */
        return  rv;
    }

    IS_STRMATCH2(opt, "sm", "sopmem") {   /* lookup/clear SOPmem & TS TxInfo */
        unsigned long long tstmp;
        unsigned long long ingsrcport;
        if ( IS_PHY_INVALID_ARG(arg[0]) )    arg[0] = 0;
        if ( IS_PHY_INVALID_ARG(arg[1]) )    arg[1] = 0;
        if ( IS_PHY_INVALID_ARG(arg[2]) )    arg[2] = 0;

        if ( arg[0] == 99 ) {
            rv |= ts_sopmem_clear(chipname);
        } else {
            rv |= ts_sopmem(chipname, pinfo_igr, arg[0], arg[1], arg[2], &tstmp, &ingsrcport);
            rv |= ts_sopmem(chipname, pinfo_egr, arg[0], arg[1], arg[2], &tstmp, &ingsrcport);
        }
        return  rv;
    } else

    IS_STRMATCH(opt, "mpls") {               /* set MPLS parameters */
        rv |= ts_mpls(chipname, pinfo_igr, TIMESYNC_DIRECTION_RXTX, arg[0], arg[1], arg[2]);
        rv |= ts_mpls(chipname, pinfo_egr, TIMESYNC_DIRECTION_RXTX, arg[0], arg[1], arg[2]);
        rv |= ts_reg_dump(chipname, pinfo_igr, 6);
        rv |= ts_reg_dump(chipname, pinfo_egr, 6);
        return  rv;
    } else

    IS_STRMATCH2(opt, "igregr", "ie") {      /* specify ingress & egress ports */
        /* set the ingress & egress ports of IEEE1588 PTP event messages */
        pinfo_igr.phy_addr = UINT32_HI16(arg[0]);   
        pinfo_igr.lane_map = UINT32_LO16(arg[0]);
        pinfo_egr.phy_addr = UINT32_HI16(arg[1]);   
        pinfo_egr.lane_map = UINT32_LO16(arg[1]);
        return  rv;
    } else

    IS_STRMATCH2(opt, "disable", "off") {    /* disable TimeSync 1588 */
        if ( (arg[0] < 0x1) || (0x3 < arg[0]) )    
            arg[0] = 0x3;                    /* both Rx & Tx */
        if ( arg[0] & 0x1 ) {                /* Rx direction */
            rv |= ts_off(chipname, pinfo_igr, P1588_RESET_ALL);
        }
        if ( arg[0] & 0x2 ) {                /* Tx direction */
            rv |= ts_off(chipname, pinfo_egr, P1588_RESET_ALL);
        }
        return  rv;
    } else

    IS_STRMATCH(opt, "dump") {               /* dump all TimeSync 1588 Registers */
        rv |= ts_reg_dump_all(chipname, pinfo_igr);
        rv |= ts_reg_dump_all(chipname, pinfo_egr);
        return  rv;
    } else

    IS_STRMATCH2(opt, "regdump", "rd") {     /* dump TimeSync 1588 registers */
        rv |= ts_reg_dump(chipname, pinfo_igr, (arg[0] > 5) ? 5 : arg[0]);
        rv |= ts_reg_dump(chipname, pinfo_egr, (arg[0] > 5) ? 5 : arg[0]);
        return  rv;
    }

    /*
     *  dispatching Timestamp T1/T2/T3/T4, IEEE/BRCM TC and other operations
     */
    switch ( atoi(opt) ) {

    case 11 :                                /* Timestamp T1 */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset(chipname, pinfo_egr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 1;                     /* 1-step */
        mode_tc = 0;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_egr, pinfo_egr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                                        ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;
    case 12 :                                /* Timestamp T1 */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset(chipname, pinfo_egr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 1;                     /* 1-step */
        mode_tc = 0;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_egr, pinfo_egr_line, TIMESYNC_DIRECTION_RX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;
    case 13 :                                /* Timestamp T1 */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset(chipname, pinfo_egr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 2;                     /* 2-step */
        mode_tc = 0;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_egr, pinfo_egr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                                        ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;
    case 14 :                                /* Timestamp T1 */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset(chipname, pinfo_egr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 2;                     /* 2-step */
        mode_tc = 0;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_egr, pinfo_egr_line, TIMESYNC_DIRECTION_RX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;

    case 21 :                                /* Timestamp T2 */
        if (arg[0] == P1588_RESET_FROM_CLI)
            rv |= ts_reset(chipname, pinfo_igr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 1;                     /* 1-step */
        mode_tc = 0;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_igr, pinfo_igr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        break;
    case 22 :                                /* Timestamp T2 */
        if (arg[0] == P1588_RESET_FROM_CLI)
            rv |= ts_reset(chipname, pinfo_igr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 2;                     /* 1-step */
        mode_tc = 0;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_igr, pinfo_igr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        break;
    case 23 :                                /* Timestamp T2 */
        if (arg[0] == P1588_RESET_FROM_CLI)
            rv |= ts_reset(chipname, pinfo_igr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 1;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 1;                     /* 2-step */
        mode_tc = 0;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_igr, pinfo_igr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        break;
    case 24 :                                /* Timestamp T2 */
        if (arg[0] == P1588_RESET_FROM_CLI)
            rv |= ts_reset(chipname, pinfo_igr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 1;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 2;                     /* 2-step */
        mode_tc = 0;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_igr, pinfo_igr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        break;

    case 31 :                                /* Timestamp T3  */
        if (arg[0] == P1588_RESET_FROM_CLI)
            rv |= ts_reset( chipname, pinfo_egr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 1;                     /* 1-step */
        mode_tc = 1;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_egr, pinfo_egr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;
    case 32 :                                /* Timestamp T3  */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset( chipname, pinfo_egr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 1;                     /* 1-step */
        mode_tc = 1;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_egr, pinfo_egr_line, TIMESYNC_DIRECTION_RX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;
    case 33 :                                /* Timestamp T3  */
        if (arg[0] == P1588_RESET_FROM_CLI)
            rv |= ts_reset( chipname, pinfo_egr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 2;                     /* 2-step */
        mode_tc = 1;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_egr, pinfo_egr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;
    case 34 :                                /* Timestamp T3  */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset( chipname, pinfo_egr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 2;                     /* 2-step */
        mode_tc = 1;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_egr, pinfo_egr_line, TIMESYNC_DIRECTION_RX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;

    case 41 :                                /* Timestamp T4 */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset( chipname, pinfo_igr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 1;                     /* 1-step */
        mode_tc = 1;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_igr, pinfo_igr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        break;
    case 42 :                                /* Timestamp T4 */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset( chipname, pinfo_igr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 1;                     /* 1-step */
        mode_tc = 1;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_igr, pinfo_igr_line, TIMESYNC_DIRECTION_RX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        break;
    case 43 :                                /* Timestamp T4 */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset( chipname, pinfo_igr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 2;                     /* 2-step */
        mode_tc = 1;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_igr, pinfo_igr_line, TIMESYNC_DIRECTION_TX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        break;
    case 44 :                                /* Timestamp T4 */
        if (arg[0] == P1588_RESET_FROM_CLI)  
            rv |= ts_reset( chipname, pinfo_igr, P1588_RESET_ALL);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 2;                     /* 2-step */
        mode_tc = 1;                         /* 0-> BC Mode, 1->TC Mode */
        rv |= ts_t1_t2_t3_t4(chipname, pinfo_igr, pinfo_igr_line, TIMESYNC_DIRECTION_RX, timesync_ptp_mode, ptp_encrypt, 
                       ieee_brcm_mode, mode_tc, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        break;

    case 90 :                                /* BRCM Transparent Clock Mode */
        ts_offset_rx  = (arg[0] < PHY_INVALID_ARG) ? (arg[0] & 0xFFFFFFFF) : TIMESYNC_TS_OFFSET_RX;
        ts_offset_tx  = (arg[1] < PHY_INVALID_ARG) ? (arg[1] & 0xFFFFFFFF) : TIMESYNC_TS_OFFSET_TX;

        if (arg[2] == P1588_RESET_FROM_CLI) {
            rv |= ts_reset(chipname, pinfo_igr, P1588_RESET_ALL);
            rv |= ts_reset(chipname, pinfo_egr, P1588_RESET_ALL);
        }
        ieee_brcm_mode = 1;                  /* BRCM Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        rv |= ts_tc_ieee_brcm(chipname, pinfo_igr, TIMESYNC_DIRECTION_RXTX, timesync_ptp_mode,
                                     ts_offset_rx, ts_offset_tx, ptp_encrypt, ieee_brcm_mode, onestep_twostep);
        ieee_brcm_mode = 1;                  /* BRCM Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        rv |= ts_tc_ieee_brcm(chipname, pinfo_egr, TIMESYNC_DIRECTION_RXTX, timesync_ptp_mode,
                                     ts_offset_rx, ts_offset_tx, ptp_encrypt, ieee_brcm_mode, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;
    case 91 :                                /* IEEE Transparent Clock Mode */
        ts_offset_rx  = (arg[0] < PHY_INVALID_ARG) ? (arg[0] & 0xFFFFFFFF) : TIMESYNC_TS_OFFSET_RX;
        ts_offset_tx  = (arg[1] < PHY_INVALID_ARG) ? (arg[1] & 0xFFFFFFFF) : TIMESYNC_TS_OFFSET_TX;

        if (arg[2] == P1588_RESET_FROM_CLI) {
            rv |= ts_reset(chipname, pinfo_igr, P1588_RESET_ALL);
            rv |= ts_reset(chipname, pinfo_egr, P1588_RESET_ALL);
        }
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        rv |= ts_tc_ieee_brcm(chipname, pinfo_igr, TIMESYNC_DIRECTION_RXTX, timesync_ptp_mode,
                                     ts_offset_rx, ts_offset_tx, ptp_encrypt, ieee_brcm_mode, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_igr, 0);
        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        rv |= ts_tc_ieee_brcm(chipname, pinfo_egr, TIMESYNC_DIRECTION_RXTX, timesync_ptp_mode,
                                     ts_offset_rx, ts_offset_tx, ptp_encrypt, ieee_brcm_mode, onestep_twostep);
        rv |= ts_reg_dump(chipname, pinfo_egr, 0);
        break;

    case 61 :    {                           /* peek at timestamp and heartbeat */
        bcm_plp_timesync_timespec_t timespec;
        memset(&timespec, 0, sizeof(bcm_plp_timesync_timespec_t));

        if ( IS_PHY_INVALID_ARG(arg[0]) )    arg[0] = 0;
        if (arg[0] == 0)
            flags = bcmplpTimesyncTimeCode48bit;
        if (arg[0] == 1)
            flags = bcmplpTimesyncTimeCode80bit;

        rv |= bcm_plp_timesync_time_code_get( chipname, pinfo_igr, flags, &timespec);
        printf("rv=%d port [%2d.%02x] LocalTime Seconds=%lld  NanoSeconds=%d \n", rv,
          pinfo_igr.phy_addr, pinfo_igr.lane_map, timespec.seconds, timespec.nanoseconds);
        rv |= bcm_plp_timesync_time_code_get( chipname, pinfo_egr, flags, &timespec);
        printf("rv=%d port [%2d.%02x] LocalTime Seconds=%lld  NanoSeconds=%d \n", rv,
          pinfo_egr.phy_addr, pinfo_egr.lane_map, timespec.seconds, timespec.nanoseconds);

        if (arg[0] == 0)
            flags = bcmplpTimesyncHeartBeat48bit;
        if (arg[0] == 1)
            flags = bcmplpTimesyncHeartBeat48bitIeee;

        unsigned long long hba = 0LL, hbb = 0LL;
        rv |= ts_heartbeat_show(chipname, pinfo_igr, flags, &hba);
        usleep(HEARTBEAT_FETCH_INTERVAL);    /* sleep approximately 1 second */
        rv |= ts_heartbeat_show(chipname, pinfo_igr, flags, &hbb);
        printf("port [%2d.%02x] Heartbeat diff = %lld\n", 
           pinfo_igr.phy_addr, pinfo_igr.lane_map, hba - hbb);
        rv |= ts_heartbeat_show(chipname, pinfo_egr, flags, &hba);
        usleep(HEARTBEAT_FETCH_INTERVAL);    /* sleep approximately 1 second */
        rv |= ts_heartbeat_show(chipname, pinfo_egr, flags, &hbb);
        printf("port [%2d.%02x] Heartbeat diff = %lld\n", 
           pinfo_egr.phy_addr, pinfo_egr.lane_map, hba - hbb);
        }
        break;

    default :
        rv  = ts_reg_dump(chipname, pinfo_igr, 0);
        rv  = ts_reg_dump(chipname, pinfo_egr, 3);
        memset(&icfg, 0, sizeof(bcm_plp_timesync_config_t));
        memset(&ecfg, 0, sizeof(bcm_plp_timesync_config_t));
        rv |= bcm_plp_timesync_config_get(chipname, pinfo_igr, &icfg);
        rv |= bcm_plp_timesync_config_get(chipname, pinfo_egr, &ecfg);
    }

    return rv;
}

#if defined(PLP_APERTA2_SINGLE_LANE_PORTS)
  #define  NUM_PORT             16
  #define  TC1588_PHY_ID(_p)    ((_p) >> 4)               /* octal_0: ports 0-7 ,  octal_1: ports 8-15 */
  #define  TC1588_LANEMAP(_p)   (0x1U << ((_p)&0xF))      /* 8 ports per octal ( 10G-NRZ/25G-NRZ/50G-PAM4/100G-PAM4 modes )  */
#elif defined(PLP_APERTA2_DUAL_LANE_PORTS)
  #define  NUM_PORT             8
  #define  TC1588_PHY_ID(_p)    ((_p) >> 3)               /* octal_0: ports 0-3 ,  octal_1: ports 4-7  */
  #define  TC1588_LANEMAP(_p)   (0x3U << ((_p&0xF)<<1))   /* 4 ports per octal ( 100G-NRZ/200G-PAM4/50G-NRZ modes ) */
#elif defined(PLP_APERTA2_QUAD_LANE_PORTS)
  #define  NUM_PORT             4
  #define  TC1588_PHY_ID(_p)    ((_p) >> 2)               /* octal_0: ports 0-1 ,  octal_1: ports 2-3  */
  #define  TC1588_LANEMAP(_p)   (0x0FU << ((_p&0x3)<<2))  /* 2 ports per octal ( 100G-NRZ/400G-PAM4 modes ) */
#elif defined(PLP_APERTA2_OCTAL_LANE_PORTS)
  #define  NUM_PORT             2
  #define  TC1588_PHY_ID(_p)    ((_p) >> 2)               /* octal_0: port  0   ,  octal_1: port  1    */
  #define  TC1588_LANEMAP(_p)   (0xFFU << ((_p&0x3)<<3))  /* 1 port per ocatl  ( 800G-PAM4)  */
#elif defined(PLP_APERTA2_RGB24_LANE_PORTS)
  #define  NUM_PORT             8
  #define  TC1588_PHY_ID(_p)    ((_p) >> 3)               /* octal_0: ports 0-3 ,  octal_1: ports 4-7  */
  #define  TC1588_LANEMAP(_p)   (0x3U << ((_p&0xF)<<1))   /* 4 ports per octal ( 100G-NRZ/200G-PAM4/50G-NRZ modes ) */
#elif defined(PLP_APERTA2_FGB42_LANE_PORTS)
  #define  NUM_PORT             4
  #define  TC1588_PHY_ID(_p)    ((_p) >> 2)               /* octal_0: ports 0-1 ,  octal_1: ports 2-3  */
  #define  TC1588_LANEMAP(_p)   (0x0FU << ((_p&0x3)<<2))  /* 2 ports per octal ( 100G-NRZ/400G-PAM4 modes ) */
#endif

bcm_plp_access_t  pinfo[NUM_PORT+1] = { 
#if ( NUM_PORT > 2 )
                                        {NULL, 0, LINE_SIDE, 0, 0, 0}, {NULL, 0, LINE_SIDE, 0, 0, 0},
#if ( NUM_PORT > 4 )
                                        {NULL, 0, LINE_SIDE, 0, 0, 0}, {NULL, 0, LINE_SIDE, 0, 0, 0},
                                        {NULL, 0, LINE_SIDE, 0, 0, 0}, {NULL, 0, LINE_SIDE, 0, 0, 0},
#if ( NUM_PORT > 8 )
                                        {NULL, 0, LINE_SIDE, 0, 0, 0}, {NULL, 0, LINE_SIDE, 0, 0, 0},
                                        {NULL, 0, LINE_SIDE, 0, 0, 0}, {NULL, 0, LINE_SIDE, 0, 0, 0},
                                        {NULL, 0, LINE_SIDE, 0, 0, 0}, {NULL, 0, LINE_SIDE, 0, 0, 0},
                                        {NULL, 0, LINE_SIDE, 0, 0, 0}, {NULL, 0, LINE_SIDE, 0, 0, 0},
#endif
#endif
#endif
                                        {NULL, 0, LINE_SIDE, 0, 0, 0}, {NULL, 0, LINE_SIDE, 0, 0, 0} 
                                         };

int _port_phyinfo_retrieve(char *chipname, int port, bcm_plp_access_t *phyinfo) 
{
    int rv = 0;

    if ( ! pinfo[port].platform_ctxt ) {
        phyinfo->platform_ctxt = &p_ctxt;
        phyinfo->phy_addr      =  IGR_PORT;
        phyinfo->lane_map      =  TC1588_LANEMAP(port);
        phyinfo->if_side       = pinfo[port].if_side;
        
        pinfo[port].platform_ctxt = &p_ctxt;
        pinfo[port].phy_addr      =  IGR_PORT;
        pinfo[port].lane_map      =  TC1588_LANEMAP(port);
    } else {
        memcpy(phyinfo, &(pinfo[port]), sizeof(bcm_plp_access_t));
    }

    return  rv;
}

int timesync_verify_tc_dpll(char *chipname, char *cmd, int port, unsigned int arg[]) 
{
    int rv = 0;
    bcm_plp_access_t pi;
    int ieee_brcm_mode = 0;
    int ptp_encrypt = 0;
    int timesync_ptp_mode = 0;
    int onestep_twostep = 1;

    if ( (port < 0) || (port >= NUM_PORT) ) {
        printf("Port number is not in range, Will use default port 0\n");
        port =  0;
    }
    rv  |=  _port_phyinfo_retrieve(chipname, port, &pi);

    IS_STRMATCH(cmd, "tod") {                /* set Time of Day (ToD) */
        unsigned long long syncrefdelay = 0;
        unsigned long long seconds = 0;
        unsigned int nanoseconds = 0;
        unsigned int flags = 0;

        if ( IS_PHY_INVALID_ARG(arg[0]) )    arg[0] = 0;
        if ( IS_PHY_INVALID_ARG(arg[1]) )    arg[1] = 0;
        if ( IS_PHY_INVALID_ARG(arg[2]) )    arg[2] = 0;

        if (arg[0] == 0)
            flags = bcmplpTimesyncTimeCode48bit;
        if (arg[0] == 1)
            flags = bcmplpTimesyncTimeCode80bit;
        seconds     = (unsigned long long)arg[1]<<32 | arg[2];
        nanoseconds = arg[3];
        syncrefdelay = arg[4];
        rv |= ts_tod_set(chipname, pi, flags, seconds, nanoseconds, syncrefdelay);

        return  rv;
    } else

    IS_STRMATCH2(cmd, "heartbeat", "hb") {   /* get the Heartbeat */
        unsigned int flags = 0;
        int ii = 0;
        unsigned long long  hb0 = 0LL, hba = 0LL, hbb = 0LL;

        if ( IS_PHY_INVALID_ARG(arg[0]) )    arg[0] = 0;
        if ( IS_PHY_INVALID_ARG(arg[1]) )    arg[1] = 10;

        if (arg[0] == 0)
            flags = bcmplpTimesyncHeartBeat48bit;
        if (arg[0] == 1)
            flags = bcmplpTimesyncHeartBeat48bitIeee;

        for ( ii = 0; ii < arg[1]; ii++ ) {
            rv |= ts_heartbeat_show(chipname, pi, flags, &hb0);
            if ( ii == 0 ) {
                hba = hb0;
                printf("\t<--- Port 0\n");
            } else {
                hbb = hb0;
                if (flags == bcmplpTimesyncHeartBeat48bit) {
                    printf("\tHeartbeat diff = %lld nanoseconds\n"  , (hbb - hba));
                } else if (flags == bcmplpTimesyncHeartBeat48bitIeee) {
                    printf("\tHeartbeat diff = %lld seconds, %lld nanoseconds\n"  , (((hbb>>32) - (hba>>32))& 0xFFFFFF), (hbb&0xFFFF) - (hba&0xFFFF) );
                }
                hba = hbb;
            }
            usleep(HEARTBEAT_FETCH_INTERVAL);     /* sleep approximately 1 second */
        }

        return  rv;
    } else

    IS_STRMATCH2(cmd, "localtime", "lt") {   /* get the Local Time */
        unsigned int flags = 0;
        if ( IS_PHY_INVALID_ARG(arg[0]) )    arg[0] = 0;

        if (arg[0] == 0)
            flags = bcmplpTimesyncTimeCode48bit;
        if (arg[0] == 1)
            flags = bcmplpTimesyncTimeCode80bit;

        rv |= ts_local_time_code_show(chipname, pi, flags);
        return  rv;
    } else

    IS_STRMATCH2(cmd, "add", "tc") {         /* add a port to join Transparent Clock */
        long long ts_offset_rx = 0LL, ts_offset_tx = 0LL;
        ts_offset_rx  = (arg[0] < PHY_INVALID_ARG) ? (arg[0] & 0xFFFF) : TIMESYNC_TS_OFFSET_RX;
        ts_offset_tx  = (arg[1] < PHY_INVALID_ARG) ? (arg[1] & 0xFFFF) : TIMESYNC_TS_OFFSET_TX;

        ieee_brcm_mode = 0;                  /* IEEE Mode */
        ptp_encrypt = 0;                     /* Encryption disabled for PTP packets */
        timesync_ptp_mode = bcmplpTimesyncHybPmEnabled; /* NSE Mode */
        onestep_twostep = 1;                 /* 1-step */
        rv |= ts_tc_ieee_brcm(chipname, pi, TIMESYNC_DIRECTION_RXTX, timesync_ptp_mode,
                                     ts_offset_rx, ts_offset_tx, ptp_encrypt, ieee_brcm_mode, onestep_twostep);
        ts_reg_dump(chipname, pi, 0);
        printf("Port %2d  has joint TC\n", port);
    } else

    IS_STRMATCH2(cmd, "tx", "txinfo") {   /* lookup TS TxInfo */
#if defined(PLP_PM_TIMESTAMPING_SUPPORT)
        bcm_plp_mac_access_t     macinfo;
        bcm_plp_pm_ts_tx_info_t  ts_tx_info;

        memcpy(&(macinfo.phy_info), &pi, sizeof(bcm_plp_access_t));
        macinfo.phy_info.if_side = LINE_SIDE;
        memset(&ts_tx_info, 0, sizeof(bcm_plp_pm_ts_tx_info_t));
        printf("TS_TxInfo[%2d.%02x] ", macinfo.phy_info.phy_addr, macinfo.phy_info.lane_map);
        rv = bcm_plp_pm_timesync_tx_info_get(chipname, macinfo, &ts_tx_info);
        if ( 0 == rv ) {
            printf("seqID=0x%x(%d) timestamp=0x%08x_%08x\n",
                    ts_tx_info.ts_seq_id , ts_tx_info.ts_seq_id ,
                    ts_tx_info.ts_in_fifo_hi, ts_tx_info.ts_in_fifo_lo);
        } else if ( -8 == rv ) {
            printf(" < empty >\n");
            rv = 0;     /* reset the return value */
        } else {
            printf(" ERROR!!! rv:%d\n", rv);
        }
#endif  /* PLP_PM_TIMESTAMPING_SUPPORT */
    } else

    IS_STRMATCH2(cmd, "sm", "sopmem") {   /* lookup/clear SOPmem */
        unsigned long long tstmp;
        unsigned long long ingsrcport;
        if ( IS_PHY_INVALID_ARG(arg[0]) )    arg[0] = 0;
        if ( IS_PHY_INVALID_ARG(arg[1]) )    arg[1] = 0;
        if ( IS_PHY_INVALID_ARG(arg[2]) )    arg[2] = 0;

        if ( arg[0] == 99 ) {
            rv |= ts_sopmem_clear(chipname);
        } else {
            rv |= ts_sopmem(chipname, pi, arg[0], arg[1], arg[2], &tstmp, &ingsrcport);
        }
    } else


    IS_STRMATCH2(cmd, "delete", "bye") {     /* disable TimeSync 1588 for ports */
        printf("Deleting Port %d ...\n", port);
        rv |= ts_off(chipname, pi, P1588_RESET_ALL);
        pi.platform_ctxt = NULL;
    } else

    IS_STRMATCH2(cmd, "reset", "rst") {      /* reset TimeSync 1588 setting */
        int reset_type = 0;
        /* reset type: 0x1=NSE, 0x2=registers, 0x4=soft reset,  others=ALL */
        if ((arg[0] < 0x1) || (0x7 < arg[0]))
            reset_type |= P1588_RESET_ALL;                /* reset ALL by default */

        if ((arg[0] & 0x1)== 0x1)
            reset_type |= bcmplpTimesyncEnCtrlSwRstbNse;  /* reset NSE 1588 block only */

        if ((arg[0] & 0x2) == 0x2)
            reset_type |= bcmplpTimesyncEnCtrlSwRstbReg;  /* reset 1588 register block only */

        if ((arg[0] & 0x4) == 0x4)
            reset_type |= bcmplpTimesyncEnCtrlSwRstb;     /* reset 1588 block, Datapath & State Machine */

        rv |= ts_reset(chipname, pi, reset_type);

        printf("Port %2d reset\n", port);
    } else

    IS_STRMATCH(cmd, "dump") {               /* dump TimeSync 1588 registers */
        printf("Dump 1588 register for Port : %2d\n", port);
        rv = ts_reg_dump_all(chipname, pi);
    }

    return rv;
}

int timesync_config_filter(char *chipname, int port, int rxtx, int index,
                           bcm_plp_timesync_inband_filter_ctrl_t filter) {
    int rv = 0;
    bcm_plp_access_t  pinfo  = { 0 };
    int flags = filter.flags;

    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    if ( (port < 0) || (port >= NUM_PORT) ) {
        printf("Port number is not in range, Will use default port 0\n");
        port =  0;
    }
    rv  |=  _port_phyinfo_retrieve(chipname, port, &pinfo);

    filter.flags = 0;
    if (flags & 0x8)
        filter.flags |= bcmplpTimesyncFilterFlagMatchMacIp;   /* Match with MAC address */
    else if (flags & 0x10)
        filter.flags |= bcmplpTimesyncFilterFlagMatchIpv6;    /* Match with IPV6 address */

    if (flags & 0x1)
        filter.flags |= bcmplpTimesyncFilterFlagMatchSrc;     /* Match with Src address */
    else if (flags & 0x2)
        filter.flags |= bcmplpTimesyncFilterFlagMatchDst;     /* Match with Dst address */

    printf("valid[%d] action[%d] flags[0x%02x]\n", filter.valid, filter.action, filter.flags);
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_inband_filter_set(chipname, pinfo, rxtx, index, &filter) );

    return rv;
}

int timesync_config_filter_dump(char *chipname, int port, int rxtx) {
    int rv = 0, ii = 0;
    bcm_plp_access_t  pinfo  = { 0 };
    bcm_plp_timesync_inband_filter_ctrl_t  filter;

    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    if ( (port < 0) || (port >= NUM_PORT) ) {
        printf("Port number is not in range, Will use default port 0\n");
        port =  0;
    }
    rv  |=  _port_phyinfo_retrieve(chipname, port, &pinfo);

    for (ii = 0; ii < 16; ii++) {
        memset(&filter, 0 , sizeof(bcm_plp_timesync_inband_filter_ctrl_t));
        ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_inband_filter_get(chipname, pinfo, rxtx, ii, &filter) );

        if (filter.flags & bcmplpTimesyncFilterFlagMatchMacIp) {
            printf("[%d.0x%x]: entry[%02d] valid[%d] action[%d] flags[0x%02x] "
                   "MAC  [%02x:%02x:%02x:%02x:%02x:%02x]\n",  pinfo.phy_addr, pinfo.lane_map,
                   ii, filter.valid, filter.action, filter.flags,
                   filter.match_addr.mac_addr[5], filter.match_addr.mac_addr[4],
                   filter.match_addr.mac_addr[3], filter.match_addr.mac_addr[2],
                   filter.match_addr.mac_addr[1], filter.match_addr.mac_addr[0] );
        } else if (filter.flags & bcmplpTimesyncFilterFlagMatchIpv6) {
            printf("[%d.0x%x]: entry[%02d] valid[%d] action[%d] flags[0x%02x] "
                   "IPv6 [%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X]\n",  
                   pinfo.phy_addr, pinfo.lane_map, ii, filter.valid, filter.action, filter.flags,
                   filter.match_addr.ip6_addr[15], filter.match_addr.ip6_addr[14],
                   filter.match_addr.ip6_addr[13], filter.match_addr.ip6_addr[12],
                   filter.match_addr.ip6_addr[11], filter.match_addr.ip6_addr[10],
                   filter.match_addr.ip6_addr[9], filter.match_addr.ip6_addr[8],
                   filter.match_addr.ip6_addr[7], filter.match_addr.ip6_addr[6],
                   filter.match_addr.ip6_addr[5], filter.match_addr.ip6_addr[4],
                   filter.match_addr.ip6_addr[3], filter.match_addr.ip6_addr[2],
                   filter.match_addr.ip6_addr[1], filter.match_addr.ip6_addr[0] );
        } else {
            printf("[%d.0x%x]: entry[%02d] valid[%d] action[%d] flags[0x%02x] "
                   "IPv4 [%d.%d.%d.%d]\n",  pinfo.phy_addr, pinfo.lane_map,
                   ii, filter.valid, filter.action, filter.flags,
                   filter.match_addr.ip_addr[3], filter.match_addr.ip_addr[2],
                   filter.match_addr.ip_addr[1], filter.match_addr.ip_addr[0] );
        }
    }

    return rv;
}

#endif /* BCM_PLP_TIMESYNC_SUPPORT */
