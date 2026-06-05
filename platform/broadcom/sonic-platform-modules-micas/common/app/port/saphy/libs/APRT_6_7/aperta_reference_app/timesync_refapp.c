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

 For example users can use the shell command 'ts' to configure Timestamps T1/T2/T3/T4 and TC/PTC,
 which are described in the Application Note '1588-Gen2-ANxxx-RDS'.

 This reference program is intented to show how to use BCM APIs to configure PHYs.
 This reference program may not work in some environments.
\*-----------------------------------------------------------------------------------------------*/

/******************************************************************************************\
 * The functions ts_t1(), ts_t2(), ts_t3_t4(), ts_tc(), ts_ptc(), ts_dpll_ptp_clock() and 
 * ts_ext_xtal_ptp_clock() implement the VB scripts introduced by the Section 4 and 6 in
 * the Application Notes: 1588-Gen2-AN100-RDS / 1588-Gen2-AN201-RDS / 5499X-AN100-RDS .
\******************************************************************************************/

#ifdef BCM_PLP_TIMESYNC_SUPPORT
#include "timesync_refapp.h"

/* ingress and egress phy_info data structure */
extern int          p_ctxt;
bcm_plp_access_t    pinf_[] = { {&p_ctxt, 0, LINE_SIDE, 0x01}, {&p_ctxt, 0, LINE_SIDE, 0x02},
                                {&p_ctxt, 0, LINE_SIDE, 0x04}, {&p_ctxt, 0, LINE_SIDE, 0x08},
                                {&p_ctxt, 0, LINE_SIDE, 0x10}, {&p_ctxt, 0, LINE_SIDE, 0x20},
                                {&p_ctxt, 0, LINE_SIDE, 0x40}, {&p_ctxt, 0, LINE_SIDE, 0x80},
                                {&p_ctxt, 1, LINE_SIDE, 0x01}, {&p_ctxt, 1, LINE_SIDE, 0x02},
                                {&p_ctxt, 1, LINE_SIDE, 0x04}, {&p_ctxt, 1, LINE_SIDE, 0x08},
                                {&p_ctxt, 1, LINE_SIDE, 0x10}, {&p_ctxt, 1, LINE_SIDE, 0x20},
                                {&p_ctxt, 1, LINE_SIDE, 0x40}, {&p_ctxt, 1, LINE_SIDE, 0x80} };
#if defined(GEARBOX_MODE)
bcm_plp_access_t    tsigr   = ((bcm_plp_access_t) {&p_ctxt, IGR_PORT, SYS_SIDE, SYS_IGR_LANE});
bcm_plp_access_t    tsegr   = ((bcm_plp_access_t) {&p_ctxt, EGR_PORT, SYS_SIDE, SYS_EGR_LANE});
bcm_plp_access_t    tsigr_l   = ((bcm_plp_access_t) {&p_ctxt, IGR_PORT, LINE_SIDE, IGR_LANE});
bcm_plp_access_t    tsegr_l   = ((bcm_plp_access_t) {&p_ctxt, EGR_PORT, LINE_SIDE, EGR_LANE});

#define  pinfo_b         tsegr     /*  Egress port */
#define  pinfo_a         tsigr     /* Ingress port */
#define  pinfo_b_line    tsegr_l   /*  Egress port */
#define  pinfo_a_line    tsigr_l   /* Ingress port */

bcm_plp_access_t  pinfo[NUM_PORT+1] = { 
#if ( NUM_PORT > 2 )
                                        {NULL, 0, SYS_SIDE, 0, 0, 0}, {NULL, 0, SYS_SIDE, 0, 0, 0},
#if ( NUM_PORT > 4 )
                                        {NULL, 0, SYS_SIDE, 0, 0, 0}, {NULL, 0, SYS_SIDE, 0, 0, 0},
                                        {NULL, 0, SYS_SIDE, 0, 0, 0}, {NULL, 0, SYS_SIDE, 0, 0, 0},
#if ( NUM_PORT > 8 )
                                        {NULL, 0, SYS_SIDE, 0, 0, 0}, {NULL, 0, SYS_SIDE, 0, 0, 0},
                                        {NULL, 0, SYS_SIDE, 0, 0, 0}, {NULL, 0, SYS_SIDE, 0, 0, 0},
                                        {NULL, 0, SYS_SIDE, 0, 0, 0}, {NULL, 0, SYS_SIDE, 0, 0, 0},
                                        {NULL, 0, SYS_SIDE, 0, 0, 0}, {NULL, 0, SYS_SIDE, 0, 0, 0},
#endif
#endif
#endif
                                        {NULL, 0, SYS_SIDE, 0, 0, 0}, {NULL, 0, SYS_SIDE, 0, 0, 0} };

#else
bcm_plp_access_t    tsigr   = ((bcm_plp_access_t) {&p_ctxt, IGR_PORT, LINE_SIDE, IGR_LANE});
bcm_plp_access_t    tsegr   = ((bcm_plp_access_t) {&p_ctxt, EGR_PORT, LINE_SIDE, EGR_LANE});

#define  pinfo_b         tsegr     /*  Egress port */
#define  pinfo_a         tsigr     /* Ingress port */
#define  pinfo_b_line    tsegr     /*  Egress port */
#define  pinfo_a_line    tsigr     /* Ingress port */

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
                                        {NULL, 0, LINE_SIDE, 0, 0, 0}, {NULL, 0, LINE_SIDE, 0, 0, 0} };
#endif /* GEARBOX_MODE */

static unsigned int _simple_log2(unsigned int val) {
    int  logv = -1;

    if ( val ) {
        for ( logv = 0; ! (val & 0x1); val >>= 1 ) {
            logv++;
        }
    }
    return (unsigned int) logv;
}

#ifdef  _DEBUGGING_IEEE1588_TIMESYNC_

/* get IEEE1588 register value */
static int _timesync_1588_reg_get(char *chipname, bcm_plp_access_t pinfo,
                           unsigned int reg1588, unsigned int *data)  {
    int  rv = 0;
  #if defined(BCM_PLP_BASE_T_PHY)
    unsigned int  xgp;      /* 848xx needs to set XGP register table */
    rv |= bcm_plp_reg_value_get(chipname, pinfo, 30, 0x4110, &xgp);
    rv |= bcm_plp_reg_value_set(chipname, pinfo, 30, 0x4110, 0x2004);
  #endif

    rv |= bcm_plp_reg_value_get(chipname, pinfo,  1, (P1588_BASE | reg1588), data);

  #if defined(BCM_PLP_BASE_T_PHY)
    rv |= bcm_plp_reg_value_set(chipname, pinfo, 30, 0x4110,  xgp);
  #endif
    return  rv;
}

#endif

static int _port_phyinfo_retrieve(char *chipname, int port, bcm_plp_access_t *phyinfo) {
    bcm_plp_timesync_config_t  tscfg = { 0 };
    int  rv = 0;

    if ( ! pinfo[port].platform_ctxt ) {
        phyinfo->platform_ctxt = &p_ctxt;
        phyinfo->phy_addr      =  TC1588_PHY_ID( port);
        phyinfo->lane_map      =  TC1588_LANEMAP(port);
        phyinfo->if_side       = pinfo[port].if_side;
        
        pinfo[port].platform_ctxt = &p_ctxt;
        pinfo[port].phy_addr      =  TC1588_PHY_ID( port);
        pinfo[port].lane_map      =  TC1588_LANEMAP(port);
        TIMESYNC_BASE_SETTING(tscfg);
        rv |= bcm_plp_timesync_config_set(chipname, *phyinfo, &tscfg);
    } else {
        memcpy(phyinfo, &(pinfo[port]), sizeof(bcm_plp_access_t));
    }

    return  rv;
}

/*******************************************************************************************************\
 *   SOPmem operations
\*******************************************************************************************************/

/* lookup the SOPmem to read one or multiple entries */
int ts_sopmem(char *chipname, bcm_plp_access_t pinfo, unsigned int flags,
                                     unsigned int id, unsigned long long *tstmp) {
    unsigned int  entry_id, entry_max, seqid;
    int  rv = 0, valid_entry_count = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    if ( flags == TIMESYNC_SOPMEM_DUMP ) {  /* dump all SOPmem entries   */
        seqid     =  UNKNOWN;   /* Seq_ID should not be a lookup key     */
        entry_max =  TIMESYNC_SOPMEM_MAX; /* go through entire SOPmem (32 entries) */
        flags    |=  bcmplpTimesyncSOPmemOpNoSkipInvalid;
    } else {    /* search the SOPmem for the entry with the key (Seq_ID == id)  */
        seqid     =  id;    /* the input parameter 'id' is the lookup-key value */
        entry_max =  1;     /* get the 1st matched entry                        */
        flags     =  bcmplpTimesyncSOPmemOpRegular;
    }

    /* read/lookup the SOPmem and print valid entries */
    for ( entry_id = 0; entry_id < entry_max; entry_id++ ) {
        ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
                rv = ts_sopmem_get(chipname, pinfo, seqid, entry_id, flags, tstmp) );
        if ( 1 == rv ) {            /* a valid entry was found successfully */ 
            valid_entry_count++;    /* increase the counter                 */
        }
    }

    /* print the summary */
    printf("rv=%d  SOPmem[%2d.%02x]  ----  ", rv, pinfo.phy_addr, pinfo.lane_map);
    if ( valid_entry_count > 0 ) {
        printf("%2d valid entries found\n\n", valid_entry_count);
    } else {
        printf("empty or no entry matched\n\n");
    }
    return  0;
}

const char *_sopmem_ip_mac_capture_mode_display(bcm_plp_timesync_inband_parse_t ipmac_mode) {
  const char *ipmac_disp[] = { "SrcPort", "MPLS"   , "MacDA", "MacSA"  ,
                               "DstIPv4", "SrcIPv4", "Resv2", "DstIPv6", "SrcIPv6" };
    return    ipmac_disp[_simple_log2((int) ipmac_mode)];
}

static bcm_plp_timesync_inband_parse_t _sopmem_ip_mac_capture_mode_convert(int ipmac_in) {
  const bcm_plp_timesync_inband_parse_t
        ipmac_tab[] = { bcmplpTimesyncInbandParseSrcPort  , bcmplpTimesyncInbandParseMplsLabel,
                        bcmplpTimesyncInbandParseMacDa    , bcmplpTimesyncInbandParseMacSa    ,
                        bcmplpTimesyncInbandParseIpv4DstIp, bcmplpTimesyncInbandParseIpv4SrcIp,
                        bcmplpTimesyncInbandParseResv2    , bcmplpTimesyncInbandParseIpv6DstIp };
    /* currently, 1G PHYs support capturing MAC_DA/MAC_SA/SrcIPv4/DstIPv4 only */
    if ( (ipmac_in < 0) || (7 < ipmac_in) )    ipmac_in = 4;

    return  ipmac_tab[ipmac_in];
}


/*******************************************************************************************************\
 *   TimeSync 1588 configuration & register dump
\*******************************************************************************************************/

/* show specific TimeSync 1588 settings */
void ts_conf_show(int rv, int port, char *gossip, bcm_plp_timesync_config_t cfg) {
    printf("rv=%d port %2d [%s]  flag=0x%08x gm=%x ib=(0x%x 0x%x) "
           "synout=(0x%x 0x%x 0x%x) tpid=(0x%x 0x%x) "
           "dpll=(0x%x 0x%x 0x%x 0x%x)\n",
           rv, port, gossip,  cfg.flags,
           cfg.gmode, cfg.inband_ctrl.flags, cfg.inband_ctrl.resv0_id,
           cfg.syncout.pulse_1_length, cfg.syncout.pulse_2_length, cfg.syncout.interval,
           cfg.itpid, cfg.otpid,
           cfg.phy_1588_dpll_k1, cfg.phy_1588_dpll_k2, cfg.phy_1588_dpll_k3,
           cfg.phy_1588_dpll_ref_phase_delta);
}

void ts_reg_dump_header(unsigned int regad[], int offset16) {
    int   ii = 0;

    if ( offset16 )  { printf("       "); }
    for ( ii = 0; regad[ii] < PHY_INVALID_ARG; ii++ ) {
        if ( ! offset16 ) {
            unsigned int  disp_addr = (regad[ii] < 0x10000)
                                    ? ((0xffff & P1588_BASE) | regad[ii])
                                    : ( 0xffff & regad[ii]);
            printf("=%04X", disp_addr);
        } else {
            printf("---%X ", regad[ii]);
        }
    }
    printf("==\n");
}

int  ts_reg_dump_group(char *chipname, bcm_plp_access_t *pinfo, unsigned int regad_seg,
                                       unsigned int regad[], unsigned int regvl[]) {
    unsigned int  val = 0, devad = 1;
    int  rv = 0, ii = 0;

    if ( regad_seg > P1588_REG_MAX ) {
        printf("[%04X]", regad_seg & 0x0000ffff);   /* print the hex-segment head when */
        regad_seg &= P1588_REG_MAX;                 /*      dumping all 1588 regisgers */
    }
#if defined(BCM_PLP_BASE_T_PHY)
        unsigned int  xgp;      /* 848xx needs to set XGP register table */
        rv |= bcm_plp_reg_value_get(chipname, *pinfo, 30, 0x4110, &xgp);
        rv |= bcm_plp_reg_value_set(chipname, *pinfo, 30, 0x4110, 0x2004);
#endif
        for ( ii = 0; regad[ii] < PHY_INVALID_ARG; ii++ ) {
            unsigned int  reg_addr = 0x00;
            if ( regad[ii] < 0x10000) {
                devad    =   1;
                reg_addr =  (P1588_BASE | regad[ii]) + regad_seg;
            } else {
                devad    =  (0x00ff0000 & regad[ii]) >> UINT32_HI16_SHIFT;
                reg_addr =   0x0000ffff & regad[ii];
            }
            if ( regvl && (regvl[ii] != PHY_INVALID_ARG) ) {  /* write to  registers */
                rv |= bcm_plp_reg_value_set(chipname, *pinfo,
                                            devad, reg_addr, val = regvl[ii] );
            } else {                 /* read from  registers */
                rv |= bcm_plp_reg_value_get(chipname, *pinfo,
                                            devad, reg_addr, &(val));
            }
            printf(" %04x", val & 0xffff);
        }
#if defined(BCM_PLP_BASE_T_PHY)
        rv |= bcm_plp_reg_value_set(chipname, *pinfo, 30, 0x4110, xgp);
#endif
#if defined(_TIMESYNC_TIMECODE_VERBOSE_)
        {   bcm_plp_timesync_timespec_t   timespec;
            rv |= bcm_plp_timesync_time_code_get(chipname, *pinfo, 0, &timespec);
            printf(" (%llx ns=%x)", timespec.seconds, timespec.nanoseconds);
        }
#endif
        printf(" [p=%02d.%02x]\n", pinfo->phy_addr , pinfo->lane_map);


    return  rv;
}

/* dump key P1588 registers */
int  ts_reg_dump_igr_egr(char *chipname, unsigned int regad[], unsigned int regvl[]) {
    bcm_plp_access_t   *pinfo, *plist[PORT_MAX] = { &tsigr, &tsegr, NULL };
    int  rv = 0, jj = 0;
    ts_reg_dump_header(regad, FALSE);

    for ( jj = 0; (pinfo = plist[jj]) != NULL; jj++ ) {
        rv |= ts_reg_dump_group(chipname, pinfo, 0, regad, regvl);
    }
    return  rv;
}

unsigned int  regad_0[DUMP_MAX] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, PHY_INVALID_ARG };
/* dump all P1588 registers */
int  ts_reg_dump_all(char *chipname, bcm_plp_access_t pinfo) {
    int  rv = 0, jj = 0;
    ts_reg_dump_header(regad_0, TRUE);

    for ( jj = 0; jj < P1588_REG_MAX; jj += 0x10 ) {
        rv |= ts_reg_dump_group(chipname, &pinfo, (P1588_BASE | jj), regad_0, NULL);
    }
    return  rv;
}


unsigned int  regad_1[DUMP_MAX] = { 0x00, 0x07, 0x08, 0x09,
                                    0xb7, 0xb8, 0xb9, 0xba, 0x20, 0x50,
                                    0x18, 0x1a, 0xbc, 0xb6, 0x16, 0xc7, 0xc9, PHY_INVALID_ARG };
unsigned int  regad_2[DUMP_MAX] = { 0xe9, 0xea, 0x05, 0x06, 0x0a, 0x0b,
                                    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x13,
                                    0x14, 0x15, 0x3a, 0x3d, 0xf0, PHY_INVALID_ARG };
unsigned int  regad_3[DUMP_MAX] = { 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                                    0x27, 0x28, 0x30, 0x31, 0x32, PHY_INVALID_ARG };
unsigned int  regad_4[DUMP_MAX] = { 0x29, 0x2a, 0x2b, 0x2c, 0x2e, 0x2f,
                                    0xa8, 0xa9, 0xae, 0xaf, 0xdf, PHY_INVALID_ARG };
#if defined(PLP_QUADRA28_SUPPORT)
unsigned int  regad_5[DUMP_MAX] = { 0x1c8d9, 0x1c8d8, 0x1c804 , 0x10001 , 0x1d0b4, PHY_INVALID_ARG };
#endif
unsigned int  regad_6[DUMP_MAX] = { 0x51, 0x53, 0x52, 0x6f, 0x6e,
                                    0x60, 0x5f, 0x5e, 0x6d, 0x6c, 0x6b, PHY_INVALID_ARG };
unsigned int  regad_7[DUMP_MAX] = { 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, PHY_INVALID_ARG };
unsigned int  regad_8[DUMP_MAX] = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, PHY_INVALID_ARG };

/* dump P1588 registers in group */
int  ts_reg_dump(char *chipname, int nn) {
    int  rv = 0;

    switch ( nn ) {
    case  0 :
        rv |= ts_reg_dump_igr_egr(chipname, regad_1, NULL);
        rv |= ts_reg_dump_igr_egr(chipname, regad_2, NULL);
        break;
    case  1 :
        rv |= ts_reg_dump_igr_egr(chipname, regad_1, NULL);
        break;
    case  2 :
        rv |= ts_reg_dump_igr_egr(chipname, regad_2, NULL);
        break;
    case  3 :   case 4 :
        rv |= ts_reg_dump_igr_egr(chipname, regad_3, NULL);
        rv |= ts_reg_dump_igr_egr(chipname, regad_4, NULL);
        #if defined(PLP_QUADRA28_SUPPORT)
        rv |= ts_reg_dump_igr_egr(chipname, regad_5, NULL);
        #endif
        break;
    case  5 :
        rv |= ts_reg_dump_igr_egr(chipname, regad_4, NULL);
        #if defined(PLP_QUADRA28_SUPPORT)
        rv |= ts_reg_dump_igr_egr(chipname, regad_5, NULL);
        #endif
        break;
    case  6:
        rv |= ts_reg_dump_igr_egr(chipname, regad_6, NULL);
        rv |= ts_reg_dump_igr_egr(chipname, regad_7, NULL);
        rv |= ts_reg_dump_igr_egr(chipname, regad_8, NULL);
        break;
    }

    return  rv;
}


/*******************************************************************************************************\
 *   TimeSync Inband Filter support
\*******************************************************************************************************/

/* configure an Inband Filter entry */
int timesync_config_filter(char *chipname, int port, int rxtx, int index,
                           bcm_plp_timesync_inband_filter_ctrl_t filter) {
    int  rv = 0;
    bcm_plp_access_t  pinfo  = { 0 };
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    if ( (port < 0) || (NUM_PORT <= port) ) {
        pinfo = (rxtx == bcmplpTimesyncRx) ? tsigr : tsegr;
    } else {
        rv |= _port_phyinfo_retrieve(chipname, port, &pinfo);
    }

    /* Don't care PTP Version flag */
    filter.flags &= (bcmplpTimesyncFilterFlagMatchSrc | bcmplpTimesyncFilterFlagMatchDst |
                     bcmplpTimesyncFilterFlagMatchMacIp);
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_inband_filter_set(chipname, pinfo, rxtx, index, &filter) );

    return rv;
}

/* dump all inband filter entries */
int timesync_config_filter_dump(char *chipname, int port, int rxtx) {
    int  rv = 0, ii = 0;
    bcm_plp_access_t  pinfo  = { 0 };
    bcm_plp_timesync_inband_filter_ctrl_t  filter;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    if ( (port < 0) || (NUM_PORT <= port) ) {
        pinfo = (rxtx == bcmplpTimesyncRx) ? tsigr : tsegr;
    } else {
        rv |= _port_phyinfo_retrieve(chipname, port, &pinfo);
    }

    for (ii = 0; ii < 32; ii++) {
        memset(&filter, 0 , sizeof(bcm_plp_timesync_inband_filter_ctrl_t));
        ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_inband_filter_get(chipname, pinfo, rxtx, ii, &filter) );

        if (filter.flags & bcmplpTimesyncFilterFlagMatchMacIp) {
            printf("[%d.0x%x]: entry[%02d] valid[%d] action[%d] flags[0x%x] "
                   "macbytes [%x][%x][%x][%x][%x][%x]\n",  pinfo.phy_addr, pinfo.lane_map,
                   ii, filter.valid, filter.action, filter.flags,
                   filter.match_addr.mac_addr[5], filter.match_addr.mac_addr[4],
                   filter.match_addr.mac_addr[3], filter.match_addr.mac_addr[2],
                   filter.match_addr.mac_addr[1], filter.match_addr.mac_addr[0] );
        } else {
            printf("[%d.0x%x]: entry[%02d] valid[%d] action[%d] flags[0x%x] "
                   "ipbytes [%x][%x][%x][%x]\n",  pinfo.phy_addr, pinfo.lane_map,
                   ii, filter.valid, filter.action, filter.flags,
                   filter.match_addr.ip_addr[3], filter.match_addr.ip_addr[2],
                   filter.match_addr.ip_addr[1], filter.match_addr.ip_addr[0] );
        }
    }

    return rv;
}

/* clear all inband filter entries */
int  ts_clear_inband_filters(char *chipname, bcm_plp_access_t pinfo) {
    int rv = 0;
    int ii;
    bcm_plp_timesync_inband_filter_ctrl_t filter;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    memset(&filter, 0 , sizeof(bcm_plp_timesync_inband_filter_ctrl_t));
    for (ii = 0; ii < 32; ii++) {
        ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_inband_filter_set(chipname, pinfo, bcmplpTimesyncRx, ii, &filter) );
        ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_inband_filter_set(chipname, pinfo, bcmplpTimesyncTx, ii, &filter) );
    }
    return rv;
}


/********************************************************************************************************/

/* magic number to reset TimeSync 1588 when setting TimeStamp T1/T2/T3/T4 via CLI shells */
#define  P1588_RESET_FROM_CLI           9999
/* bitmap for CLI commands to designate IEEE1588 RESET types */
#define  P1588_RESET_ALL                PHY_INVALID_ARG
#define  P1588_RESET_INBAND_FILTER      BIT(5)
#define  P1588_RESET_SOPMEM             BIT(4)
#define  P1588_RESET_SOFT               BIT(2)
#define  P1588_RESET_REG                BIT(1)
#define  P1588_RESET_NSE                BIT(0)
/* composite bitmaps for IEEE1588 RESET */
#define  P1588_RESET_REGULAR            (P1588_RESET_SOFT | P1588_RESET_REG | P1588_RESET_NSE)
#define  P1588_RESET_WHOLE              (P1588_RESET_INBAND_FILTER | \
                                         P1588_RESET_SOPMEM | P1588_RESET_REGULAR)
#define  P1588_RESET_TC                 P1588_RESET_REGULAR

/* reset TimeSync 1588 engine */
int  ts_reset(char *chipname, bcm_plp_access_t pinfo, int rst_type, bcm_plp_timesync_config_t *cfg) {
    unsigned int  val = 0;
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    if ( rst_type & P1588_RESET_ALL )     rst_type |= (P1588_RESET_SOPMEM | P1588_RESET_REGULAR);
    if ( rst_type & P1588_RESET_REGULAR ) {
        /* IEEE1588 Enable Control Register bit[15:13]  (reset options) */
        rv     |= bcm_plp_timesync_enable_get(chipname, pinfo, bcmplpTimesyncEnCtrlFlag, &val);
        val    |= (0x7 & rst_type) << 13;  /* bit[15]=datapath/FSM [14]=registers [13]=NSE_timer */
        ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, bcmplpTimesyncEnCtrlFlag,  val) );
    }
    printf("rv=%d [p=%d.%02x] RESET (type=0x%x)\n", rv, pinfo.phy_addr , pinfo.lane_map,  val);

  #if defined(PLP_QUADRA28_SUPPORT)
    {   unsigned int  reg16;
        rv |= bcm_plp_reg_value_get(chipname, pinfo, 1, 0x0000, &reg16);
        rv |= bcm_plp_reg_value_set(chipname, pinfo, 1, 0x0000,  reg16 | BIT(15));
        /* Merlin SerDes retimer settings */
        rv |= bcm_plp_reg_value_get(    chipname, pinfo, 1, 0xC8D9, &reg16);
        if ( 0 == (reg16 & BIT(4)) ) {
            rv |= bcm_plp_reg_value_set(chipname, pinfo, 1, 0xC8D9,  reg16 | BIT(4));
        }
        rv |= bcm_plp_reg_value_set(chipname, pinfo, 1, 0xC8D8, 0x8802);
        rv |= bcm_plp_reg_value_set(chipname, pinfo, 1, 0xC8D8, 0x8882);
        /* write 1.0xD0B4 twice to make sure it is set properly */
        rv |= bcm_plp_reg_value_set(chipname, pinfo, 1, 0xD0B4, 0x0420);
        rv |= bcm_plp_reg_value_set(chipname, pinfo, 1, 0xD0B4, 0x0420);
    }
  #endif

    if ( rst_type & P1588_RESET_SOPMEM ) {
        rv |= ts_sopmem_clear(chipname, tsigr, TIMESYNC_SOPMEM_MAX);
        rv |= ts_sopmem_clear(chipname, tsegr, TIMESYNC_SOPMEM_MAX);
    }
    if ( rst_type & P1588_RESET_INBAND_FILTER) {
        rv |= ts_clear_inband_filters(chipname, pinfo);
    }

    if ( cfg ) {    /* set the base configuration for TimeSync */
        ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );
    }
    return  rv;
}

/*
 *  MPLS
 */
int ts_mpls(char *chipname, bcm_plp_access_t pinfo, int rxtx, int count, int enable) {
    int  rv = 0, ii = 0;
    bcm_plp_timesync_mpls_ctrl_t  mpls;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    if ( 0 == count ) {
        memset(&mpls, 0, sizeof(bcm_plp_timesync_mpls_ctrl_t));
        mpls.size = 10;     /* clear all 10 labels */
    } else {
        /*  the MPLS label values here are for Broadcom's unit tests  *\
        \*                      User should prepare their own labels  */
        mpls.labels[0].value = 0xbe111;  mpls.labels[0].mask = 0xfffff;  mpls.labels[0].flags = 0x0;
        mpls.labels[1].value = 0x11111;  mpls.labels[1].mask = 0xfffff;  mpls.labels[1].flags = 0x0;
        mpls.labels[2].value = 0x22222;  mpls.labels[2].mask = 0xfffff;  mpls.labels[2].flags = 0x0;
        mpls.labels[3].value = 0x33333;  mpls.labels[3].mask = 0xfffff;  mpls.labels[3].flags = 0x0;
        mpls.labels[4].value = 0x44444;  mpls.labels[4].mask = 0xfffff;  mpls.labels[4].flags = 0x0;
        mpls.labels[5].value = 0x55555;  mpls.labels[5].mask = 0xfffff;  mpls.labels[5].flags = 0x0;
        mpls.labels[6].value = 0x66666;  mpls.labels[6].mask = 0xfffff;  mpls.labels[6].flags = 0x0;
        mpls.labels[7].value = 0x77777;  mpls.labels[7].mask = 0xfffff;  mpls.labels[7].flags = 0x0;
        mpls.labels[8].value = 0x88888;  mpls.labels[8].mask = 0xfffff;  mpls.labels[8].flags = 0x0;
        mpls.labels[9].value = 0x99999;  mpls.labels[9].mask = 0xfffff;  mpls.labels[9].flags = 0x0;
        mpls.special_label   = 0x12345;
        mpls.size  = count;
        mpls.flags = enable & ( bcmplpTimesyncMplsEnable  | bcmplpTimesyncMplsEntropy |
                                bcmplpTimesyncMplsSpecial | bcmplpTimesyncMplsCtrlWord );
    }
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_mpls_set(chipname, pinfo, rxtx,  enable, &mpls) );
    printf("rv=%d port %d MPLS rxtx=%d count=%d en=%d\n", rv, pinfo.phy_addr, rxtx, count, enable);
    if ( count > 0 ) {
        /* get the setting back for checking */
        memset(&mpls, 0, sizeof(bcm_plp_timesync_mpls_ctrl_t));
            rv |= bcm_plp_timesync_mpls_get(chipname, pinfo, rxtx, &enable, &mpls);
        printf("en=%d f=0x%x L=", enable, mpls.flags);
        for ( ii = 0; ii < count; ii++ ) {
            printf(" 0x%05X(%x)", mpls.labels[ii].value, mpls.labels[ii].flags);
        }
        printf("\n");
    }

    return  rv;
}

/*
 *  PortMacro Timestamping support
 */
#define  P1588_PORT_MACRO       BIT(0)

#if  defined(PLP_PM_TIMESTAMPING_SUPPORT)

#if defined (GEARBOX_MODE)
bcm_plp_access_t      ts_p0   = ((bcm_plp_access_t) {&p_ctxt, SYS_P0__PORT, SYS_SIDE, SYS_P0__LANE});
bcm_plp_access_t      ts_p0_l = ((bcm_plp_access_t) {&p_ctxt, P0__PORT, LINE_SIDE, P0__LANE});
#define  pinfo_0      ts_p0     /* port 0 (source of PortMacro (PM) TimeStamp) */
#define  pinfo_0_line ts_p0_l   /* port 0 (source of PortMacro (PM) TimeStamp) */
#else
bcm_plp_access_t      ts_p0   = ((bcm_plp_access_t) {&p_ctxt, P0__PORT, LINE_SIDE, P0__LANE});
#define  pinfo_0      ts_p0     /* port 0 (source of PortMacro (PM) TimeStamp) */
#define  pinfo_0_line ts_p0     /* port 0 (source of PortMacro (PM) TimeStamp) */
#endif

/* MAC Accessing data structure for calling 'bcm_plp_pm_timesync_enable_set()' */
bcm_plp_mac_access_t  macigr = { {&p_ctxt, IGR_PORT, LINE_SIDE, IGR_LANE} };
bcm_plp_mac_access_t  macegr = { {&p_ctxt, EGR_PORT, LINE_SIDE, EGR_LANE} };
bcm_plp_mac_access_t  mac_p0 = { {&p_ctxt, P0__PORT, LINE_SIDE, P0__LANE} };

/*
 *  TimeSync 1588 config sequence example -- PortMacro (PM) timestamping.
 */
int ts_pm_example(char *chipname, unsigned int arg1, unsigned int arg2 ) {
    bcm_plp_aperta_device_aux_modes_t  aux_mode = { 0 };
    int  rv = 0, speed = 0, intf_type = 0, r_clk = 0, if_mode = 0;
    unsigned int  igr1step = 0, egr1step = 0, igr2step = 0, egr2step = 0;

    if ( (0x1 == arg1) || (0x2 == arg1) ) {
        /* set PM (PortMacro) timestamping.  arg1 = one/two-step , arg2 = enable/disable */
        rv |= bcm_plp_pm_timesync_enable_set(chipname, macigr, arg1, (arg2) ? TRUE : FALSE);
        rv |= bcm_plp_pm_timesync_enable_set(chipname, macegr, arg1, (arg2) ? TRUE : FALSE);
        FORBOTH_LINE_SYS_SIDE(tsigr) {
            rv |= bcm_plp_mode_config_get(chipname, tsigr,  &speed, &intf_type,
                                          &r_clk, &if_mode, (void*) &aux_mode);
            aux_mode.ts_config = arg2;   /* 0=Off / 1=NSE / 2=NSE-PM / 3=PM / 4=PM-NSE */
            rv |= bcm_plp_mode_config_set(chipname, tsigr,   speed,  intf_type,
                                           r_clk,  if_mode, (void*) &aux_mode);
        }
        FORBOTH_LINE_SYS_SIDE(tsegr) {
            rv |= bcm_plp_mode_config_get(chipname, tsegr,  &speed, &intf_type,
                                          &r_clk, &if_mode, (void*) &aux_mode);
            aux_mode.ts_config = arg2;   /* 0=Off / 1=NSE / 2=NSE-PM / 3=PM / 4=PM-NSE */
            rv |= bcm_plp_mode_config_set(chipname, tsegr,   speed,  intf_type,
                                           r_clk,  if_mode, (void*) &aux_mode);
        }
        printf("- - - -\nrv=%d PM TimeSync Ports[%2d.%02x][%2d.%02x] set: %d-step enable=%d\n",
                    rv, macigr.phy_info.phy_addr, macigr.phy_info.lane_map,
                        macegr.phy_info.phy_addr, macegr.phy_info.lane_map, arg1, arg2);
    } else {
        printf("Usage:  ts pm  <1/2-step> <0=Off/1=NSE/2=NSE-PM/3=PM/4=PM-NSE>\n\n");
    }
    rv = 0;    tsigr.if_side = tsegr.if_side =  LINE_SIDE;

    /* display current PM timestamping setting */
    rv |= bcm_plp_pm_timesync_enable_get(chipname, macigr, TIMESYNC_T1_ONE_STEP, &igr1step);
    rv |= bcm_plp_pm_timesync_enable_get(chipname, macigr, TIMESYNC_T1_TWO_STEP, &igr2step);
    rv |= bcm_plp_pm_timesync_enable_get(chipname, macegr, TIMESYNC_T1_ONE_STEP, &egr1step);
    rv |= bcm_plp_pm_timesync_enable_get(chipname, macegr, TIMESYNC_T1_TWO_STEP, &egr2step);
    printf("rv=%d PM TimeSync  Rx1step=0x%x Rx2step=0x%x Tx1step=0x%x Tx2step=0x%x\n",
                                          rv, igr1step, igr2step, egr1step, egr2step);
    printf("TimeSync NSE/PM mode: Port[%d.%02x]=(", tsigr.phy_addr, tsigr.lane_map);
    FORBOTH_LINE_SYS_SIDE(tsigr) {
        aux_mode.ts_config = -1;
        rv  = bcm_plp_mode_config_get(chipname, tsigr,  &speed, &intf_type,
                                      &r_clk, &if_mode, (void*) &aux_mode);
        printf("%d ", aux_mode.ts_config);
    }
    printf(")  Port[%d.%02x]=(", tsegr.phy_addr, tsegr.lane_map);
    FORBOTH_LINE_SYS_SIDE(tsegr) {
        aux_mode.ts_config = -1;
        rv |= bcm_plp_mode_config_get(chipname, tsegr,  &speed, &intf_type,
                                                &r_clk, &if_mode, (void*) &aux_mode);
        printf("%d ", aux_mode.ts_config);
    }
    printf(")  rv=%d\n", rv);

    return  rv;
}


/* disable PortMacro (PM) timestamping */
int ts_pm_off(char *chipname, bcm_plp_access_t pinfo) {
    int  rv = 0;
    bcm_plp_mac_access_t  macinfo;

    memcpy(&(macinfo.phy_info), &pinfo, sizeof(bcm_plp_access_t));
    rv |= bcm_plp_pm_timesync_enable_set(chipname, macinfo, 0x5, FALSE);
    rv |= bcm_plp_pm_timesync_enable_set(chipname, macinfo, 0x1, FALSE);
    return  rv;
}

#endif  /* PLP_PM_TIMESTAMPING_SUPPORT */

/* disable IEEE1588 TimeSync */
int ts_off(char *chipname, bcm_plp_access_t pinfo, int rst_type) {
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    rv |= ts_reset(chipname, pinfo, rst_type, NULL);
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, TIMESYNC_DIRECTION_TXRX,
                                                               TIMESYNC_DISABLE) );
  #if  defined(PLP_PM_TIMESTAMPING_SUPPORT)
    rv |= ts_pm_off(chipname, pinfo);
  #endif
    printf("Disable TimeSync: p=[%d.0x%02x]\n", pinfo.phy_addr, pinfo.lane_map);
    return  rv;
}


int tc_pm_timesync(char *chipname, bcm_plp_access_t pinfo, unsigned int pm1588_en) {
    int  rv = 0;

  #if  defined(PLP_PM_TIMESTAMPING_SUPPORT)
    {
        bcm_plp_mac_access_t  macinfo;
        /* enable PortMacro (PM) timestamping for PM TC/t12 modes */
        memcpy(&(macinfo.phy_info), &pinfo, sizeof(bcm_plp_access_t));
        rv |= bcm_plp_pm_timesync_enable_set(chipname, macinfo, 0x5, (pm1588_en) ? TRUE : FALSE);
        printf("  PM=0x%x (rv=%d) for port [%2d.%02x]", pm1588_en, rv,
                                               pinfo.phy_addr , pinfo.lane_map);
    }
  #endif
    printf("\n");
    return rv;
}

int ts_tod_set(char *chipname, bcm_plp_access_t pinfo, unsigned long long timeofday) {
    bcm_plp_timesync_time_value_t  timevalue;
    int           rv = 0;

    timevalue.type       = bcmplpTimesyncTimerTypeTimeOfDay48;
    timevalue.time_value = timeofday;
    rv = bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue);
    printf("rv=%d Setting Time of Day = 0x%llX (ToD) for port [%2d.%02x]\n", rv, timeofday,
                                                      pinfo.phy_addr , pinfo.lane_map);
    return rv;
}

int fsync_shadow_enable(char *chipname, bcm_plp_access_t pinfo, unsigned int shadow_load1,
                                                                unsigned int shadow_load2,
                                                                unsigned int framesync)  {
    bcm_plp_timesync_framesync_t  synmode;
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* [ enable shadow_cnt_ld phy,2 ]  (1588reg.14/15=0x04bf) shadow load control */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_load_ctrl_set(chipname, pinfo, 0, shadow_load1, shadow_load2) );

    if ( 0 != shadow_load1 ) {
        /* [ enable gsm3_fsync_so phy ]  (1588reg.3a=0xF008/F004)  */
        rv |= bcm_plp_timesync_framesync_mode_get(chipname, pinfo, 0, &synmode);
        synmode.mode         =  framesync;                            /* FrameSync mode */
        synmode.syncout_mode =  bcmplpTimesyncSyncoutModeDisable;     /*   SyncOut mode */
        synmode.gmode        =  bcmplpTimesyncGLobalModeCpu;          /*    global mode */
        synmode.flags        =  bcmplpTimesyncSyncmodeCtrlTsCapture |
                                bcmplpTimesyncSyncmodeCtrlNseInit;
        ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_framesync_mode_set(chipname, pinfo, 0, &synmode) );
    }
    printf("rv=%d Setting shadow load (0x%x 0x%x) for port [%2d.%02x]  FrameSync=%s\n",
            rv, shadow_load1, shadow_load2, pinfo.phy_addr, pinfo.lane_map,
            ( (synmode.mode == bcmplpTimesyncFramsyncModeSyncIn0) ? "Sync0" :
              (synmode.mode == bcmplpTimesyncFramsyncModeSyncIn1) ? "Sync1" :
              (synmode.mode == bcmplpTimesyncFramsyncModeCpu    ) ? "CPU"   : "INVALID" ) );
    return rv;
}

int fsync_shadow_set(char *chipname, bcm_plp_access_t *pinfo, unsigned int shadow_load1,
                                                              unsigned int shadow_load2,
                                                              unsigned int framesync)  {
    int  rv = 0;
    unsigned int  load1 = 0x0, load2 = 0x0;
    load1 = (shadow_load1 == 0) ? 0     /* to clear shadow registers */
          : (shadow_load1 == 1) ? TIMESYNC_SHADOW_LOAD_DEFAULT
          :  shadow_load1 & 0xFFFF;     /* value given by users      */
    load2 = (shadow_load2 == 0) ? 0     /* to clear shadow registers */
          : (shadow_load2 == 1) ? TIMESYNC_SHADOW_LOAD_DEFAULT
          : (shadow_load2 > 0xFFFF) ? load1
          :  shadow_load2 & 0xFFFF;     /* value given by users      */

    if ( pinfo != NULL ) {  /* Transparent Clock (TC) mode           */
        rv |= fsync_shadow_enable(chipname, *pinfo , load1, load2, framesync);
    }
    else {                  /* Grand Master or Slave Clock modes     */
      #if  defined(PLP_PM_TIMESTAMPING_SUPPORT)
        rv |= fsync_shadow_enable(chipname, pinfo_0, load1, load2, framesync);
      #endif
        rv |= fsync_shadow_enable(chipname, pinfo_a, load1, load2, framesync);
        rv |= fsync_shadow_enable(chipname, pinfo_b, load1, load2, framesync);
    }

    return  rv;
}

/*
 * TimeSync 1588 config sequence for setting
 * External PTP_XTAL for PTP Clock  ( Section 6 in 1588-Gen2-ANxxx-RDS Application Note )
 * Imitate the VB script introduced in section "Synch Using DPLL & FrameSync"
 * of the app note :     P03_ptpclk_set_SI_FSync.vbs
 *
 * ( see more details in Section 5.2 of IEEE1588 TimeSync Software User Guide )
 */
int ts_ext_xtal_ptp_clock(char *chipname, bcm_plp_access_t pinfo, int txrx, bcm_plp_timesync_config_t *cfg) {
    bcm_plp_timesync_config_t      cfg_;
    bcm_plp_timesync_timespec_t    timespec;
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* [ enable 1588_rxtx phy,0 ]  enable NSE Timer Clock  1588reg.00[2] */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo,
                                      bcmplpTimesyncEnCtrlNseTimerClkEn, TIMESYNC_ENABLE) );

    /* [ ptp_set_clock_config phy,1 ]  enable 1588 Tx/Rx  1588reg.00[3:2]=0x2*/
    /* [ ptp_ext_sync_l phy,1 ]  1588reg.81[8:6]=0x1*/
    cfg->nco_sync0_pulse = 0x1;

    /* [ set int_threshold phy,1 ]  threshold value to trigger the packet timestamp interrupt */
    cfg->fifo_level_intr_thold = 0x1;       /* 1588reg.b6 */
    /* [ enable hb_capture phy,1 ]  (1588reg.20[13:12])  heartbeat capture mode */
    cfg->hb_capture_mode = bcmplpTimesyncHeartbeatCaptureTc;
    /* SOPmem capturing MAC/IP/SrcPortID setting */
    cfg->rx_inband_prop.compare_sopmem = addr_capture_rx;
    cfg->rx_inband_prop.write_sopmem   = addr_capture_rx;
    cfg->tx_inband_prop.compare_sopmem = addr_capture_tx;
    cfg->tx_inband_prop.write_sopmem   = addr_capture_tx;

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );
            rv |= bcm_plp_timesync_config_get(chipname, pinfo, &cfg_);
    TS_COMPARE(pinfo, cfg, &cfg_, bcm_plp_timesync_config_t);

    /* [ enable interrupt phy ]  (1588reg.16) enable all interrupts */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_intr_enable_set( chipname, pinfo,
                                (bcmplpTimesyncIntrIpg | bcmplpTimesyncIntrSyncIn |
                                 bcmplpTimesyncIntrSop | bcmplpTimesyncIntrFsync  ),
                                TIMESYNC_ENABLE) );

    /* [ set local_time phy,0 ]  (1588reg.30-32) 48-bit local timer */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_local_time_set(chipname, pinfo, 0, 0) );
    /* [ set time_code phy,0 ]  (1588reg.0c-10) tgime code */
    timespec.seconds = 0xffffffffLL;    timespec.nanoseconds = 0xffffffffL ;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_time_code_set(chipname, pinfo, 0, &timespec) );
    return rv;
}


/* Timing value functions verification */

#define  TS_SNOOP_TIMEVALUE(_p, _tv)  \
                printf("p=%2d  [%04x] = 0x%016llx\n", _p, _tv.type, _tv.time_value)

int ts_timing_value_check(char *chipname, bcm_plp_access_t pinfo) {
    bcm_plp_timesync_time_value_t  timevalue;
    int           rv = 0;

    /* [ 1588reg.a8-a9) ]  NTP Frequency control word */
    timevalue.type       = bcmplpTimesyncTimerTypeNtpFreqCtrl;
    timevalue.time_value = 0x1234567890abc111LL;
    rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue);
    rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue);
        TS_SNOOP_TIMEVALUE(pinfo.phy_addr, timevalue);
    /* [ 1588reg.ae-af) ]  NTP Down Counter control word */
    timevalue.type       = bcmplpTimesyncTimerTypeNTPDownCntr;
    timevalue.time_value = 0x1234567890abc222LL;
    rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue);
    rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue);
        TS_SNOOP_TIMEVALUE(pinfo.phy_addr, timevalue);
    /* [ 1588reg.2e-2f) ]  NCO Frequency Stepping control */
    timevalue.type       = bcmplpTimesyncTimerTypeNcoFreqStep;
    timevalue.time_value = 0x1234567890abc333LL;
    rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue);
    rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue);
        TS_SNOOP_TIMEVALUE(pinfo.phy_addr, timevalue);
    /* [ 1588reg.29-2c) ]  DPLL initial loopfilter */
    timevalue.type       = bcmplpTimesyncTimerTypeDpllInitLpFlt;
    timevalue.time_value = 0x1234567890abc444LL;
    rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue);
    rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue);
        TS_SNOOP_TIMEVALUE(pinfo.phy_addr, timevalue);
    /* [ set dpll_init_ref_phase phy,0) ]  1588reg.21-23 NSE DPLL Reference Phase */
    timevalue.type       = bcmplpTimesyncTimerTypeDpllRefPhase;
    timevalue.time_value = 0x1234567890abc555LL;
    rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue);
    rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue);
            TS_SNOOP_TIMEVALUE(pinfo.phy_addr, timevalue);
    /* [ set dpll_init_ref_phase_delta phy,0) ]  1588reg.24-25 NSE DPLL Reference Delta Phase */
    timevalue.type       = bcmplpTimesyncTimerTypeDpllRefDelta;
    timevalue.time_value = 0x1234567890abc666LL;
    rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue);
    rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue);
            TS_SNOOP_TIMEVALUE(pinfo.phy_addr, timevalue);
    /* [ set dpll_k1_k3 phy,0) ]  1588reg.26-28 NSE DPLL K3/K2/K1 */
    timevalue.type       = bcmplpTimesyncTimerTypeDpllK3K2K1;
    timevalue.time_value = 0x1234abcdef88LL;
    rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue);
    rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue);
            TS_SNOOP_TIMEVALUE(pinfo.phy_addr, timevalue);
    return rv;
}

/* CPU software emulates SyncIn0 to trigger Framesync to make HeartBeat advances */
int ts_heartbeat_cpu_tirgger(char *chipname, bcm_plp_access_t pinfo) {
    int           rv  = 0;
    bcm_plp_timesync_framesync_mode_t  fs_mode = bcmplpTimesyncFramsyncModeNone;
    bcm_plp_timesync_framesync_t  framesync = { .mode = bcmplpTimesyncFramsyncModeNone };
  #ifdef  _DEBUGGING_IEEE1588_TIMESYNC_
    unsigned int  val = 0U;
  #endif

    rv |= bcm_plp_timesync_framesync_mode_get(chipname, pinfo, 0, &framesync);
    fs_mode         =  framesync.mode;
    framesync.mode  =  bcmplpTimesyncFramsyncModeCpu;    /* CPU trigger heartbeat */

  #ifdef  _DEBUGGING_IEEE1588_TIMESYNC_
    rv |= _timesync_1588_reg_get(chipname, pinfo, (P1588_BASE | 0xF0),  &val);
    printf("-- Port [%2d.%02x]  Reg.NSE5 = 0x%04X", pinfo.phy_addr, pinfo.lane_map, val);
  #endif

    rv |= bcm_plp_timesync_framesync_mode_set(chipname, pinfo, 0, &framesync);
  #ifdef  _DEBUGGING_IEEE1588_TIMESYNC_
    rv |= _timesync_1588_reg_get(chipname, pinfo, (P1588_BASE | 0xF0),  &val);
    printf(" --> 0x%04X", val);
  #endif
    usleep(100000);

    /* toggle or resume the original FrameSync mode */
    framesync.mode  = (bcmplpTimesyncFramsyncModeCpu == fs_mode)
                    ?  bcmplpTimesyncFramsyncModeNone : fs_mode;
    rv |= bcm_plp_timesync_framesync_mode_set(chipname, pinfo, 0, &framesync);
  #ifdef  _DEBUGGING_IEEE1588_TIMESYNC_
    rv |= _timesync_1588_reg_get(chipname, pinfo, (P1588_BASE | 0xF0),  &val);
    printf(" --> 0x%04X\n", val);
  #endif

    return rv;
}

/* display the timestamp heartbeat */
int ts_heartbeat_show(char *chipname, bcm_plp_access_t pinfo,
                      int size, unsigned long long *heartbeat) {
    bcm_plp_timesync_timespec_t   hb80 = { 0U, 0ULL, 0U };
    int  rv = 0;
    size = (size < 80) ? 48 : 80;

    if ( bcmplpTimesyncFramsyncModeCpu == (*heartbeat) ) {
        /* CPU to emulates SyncIn0 to trigger HeartBeat advances */
        rv |= ts_heartbeat_cpu_tirgger(chipname, pinfo);
    }

    if ( size == 80 ) {
        rv |= bcm_plp_timesync_time_code_get(chipname, pinfo,
                                             bcmplpTimesyncUnivHeartbeat80bit, &hb80);
        *heartbeat = (((hb80.seconds) & 0xFFFF) << 32) | (hb80.nanoseconds);
    } else {
        rv |= bcm_plp_timesync_heartbeat_timestamp_get(chipname, pinfo, 0x0, heartbeat);
    }

    printf("rv=%d port [%2d.%02x] HB%d = ", rv, pinfo.phy_addr, pinfo.lane_map, size);
    if ( size == 80 ) {
        printf("%04x_%04x_" , (unsigned int) (((hb80.seconds) >> 32) & 0xFFFF),
                              (unsigned int) (((hb80.seconds) >> 16) & 0xFFFF) );
    }
    printf("%04x_%04x_%04x \n", (unsigned int) (((*heartbeat) >> 32) & 0xFFFF),
                              (unsigned int) (((*heartbeat) >> 16) & 0xFFFF),
                              (unsigned int) (((*heartbeat) >>  0) & 0xFFFF) );
  #ifdef __DUMP_80BIT_HEARTBEAT_REGISTERS__
    {
        unsigned int  val = 0;
        int           ii = 0;
        printf("\n\t\t\t 0x490074xx = 0x0000");
        for ( ii = 0; ii < 3; ii++ ) {
            rv |= bcm_plp_reg_value_get(chipname, pinfo, 1, P1588_BASE | (0x445 + ii), &val);
            printf("%04x", val);
        }
        printf("\n\t\tHeartbeat80 = 0x");
        for ( ii = 5; ii >= 0; ii-- ) {
            rv |= bcm_plp_reg_value_get(chipname, pinfo, 1, P1588_BASE | (0x0F4 + ii), &val);
            printf("%04x", val);
        }
    }
  #endif
    return rv;
}

/* display Local Time and Time Code */
int ts_local_time_code_show(char *chipname, bcm_plp_access_t pinfo) {
    unsigned long long           lt64     =      0LL;
    bcm_plp_timesync_timespec_t  timespec = { 0, 0LL, 0 };
    int                          rv = 0;

    rv |= bcm_plp_timesync_time_code_get( chipname, pinfo, 0, &timespec);
    rv |= bcm_plp_timesync_local_time_get(chipname, pinfo, 0, &lt64);
    printf(  "rv=%d port [%2d.%02x]  localTime = 0x%llx\ttimecode = ( 0x%llx%x )\n", rv,
              pinfo.phy_addr, pinfo.lane_map, lt64, timespec.seconds, timespec.nanoseconds);
    return rv;
}

/*--------------------------------------------------------------------------------------------*/


/*************************************************************************************************\
 *  TimeSync 1588 Unit Test configurations
\*************************************************************************************************/

#define  cfg_a         icfg
#define  cfg_b         ecfg

int timesync_conf_t1234_tc(char *chipname,  char *opt, unsigned int arg[]) {
    static bcm_plp_timesync_framesync_mode_t  framesync_trigger = bcmplpTimesyncFramsyncModeNone;
    int        rv = 0, flags = 0, do_base_conf = TRUE;
    long long  link_delay = 0LL, ts_offset_rx = 0LL, ts_offset_tx = 0LL;
    bcm_plp_timesync_config_t  icfg;
    bcm_plp_timesync_config_t  ecfg;

    TIMESYNC_BASE_SETTING(icfg);
    TIMESYNC_BASE_SETTING(ecfg);

    IS_STRMATCH2(opt, "reset", "rst") {   /* reset TimeSync 1588 setting */
        /* reset type: 0x1=NSE, 0x2=registers, 0x4=soft_reset_1588,  others=ALL */
        if ( (arg[2] < 0x1) || (0x7 < arg[2]) )    arg[2] = P1588_RESET_ALL;  /* reset ALL by default */

        /* reset Rx, Tx or both directions */
        if ( (arg[1] < 0x1) || (0x3 < arg[1]) )    arg[1] = 0x3;    /* both Rx & Tx */
        else                                   do_base_conf = FALSE;

        if ( arg[1] & 0x1 ) {   /* Rx direction */
            rv |= ts_reset(chipname, tsigr, arg[2], (do_base_conf) ? &icfg : NULL);
        }
        if ( arg[1] & 0x2 ) {   /* Tx direction */
            rv |= ts_reset(chipname, tsegr, arg[2], (do_base_conf) ? &ecfg : NULL);
        }
        return  rv;
    } else

#if  defined(PLP_PM_TIMESTAMPING_SUPPORT)
    IS_STRMATCH(opt, "pmt1") {    /* PortMacro timestamping for 2-step T1 */
        rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        rv |= ts_pm_t12_tc(chipname, tsigr, PM1588_MODE_T12, arg[1], &icfg);
        rv |= ts_reg_dump(chipname, 0);
        rv |= ts_reg_dump(chipname, 3);
        return  rv;
    } else

    IS_STRMATCH(opt, "pmtc") {    /* PortMacro timestamping for 1/2-step TC */
        rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        rv |= ts_pm_t12_tc(chipname, tsigr, PM1588_MODE_TC, arg[1], &icfg);
        rv |= ts_pm_t12_tc(chipname, tsegr, PM1588_MODE_TC, arg[1], &ecfg);
        rv |= ts_reg_dump(chipname, 0);
        rv |= ts_reg_dump(chipname, 3);
        return  rv;
    } else

    IS_STRMATCH(opt, "nsetc") {    /* PortMacro timestamping for 1/2-step TC */
        rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        rv |= ts_pm_t12_tc(chipname, tsigr, NSE1588_MODE_TC, arg[1], &icfg);
        rv |= ts_pm_t12_tc(chipname, tsegr, NSE1588_MODE_TC, arg[1], &ecfg);
        rv |= ts_reg_dump(chipname, 0);
        rv |= ts_reg_dump(chipname, 3);
        return  rv;
    } else

    IS_STRMATCH2(opt, "portmacro", "pm0") {    /* PortMacro timestamping */
        rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        rv |= ts_pm_example(chipname, arg[1], arg[2]);
        rv |= ts_reg_dump(chipname, 0);
        rv |= ts_reg_dump(chipname, 3);
    } else
#endif /* PLP_PM_TIMESTAMPING_SUPPORT */

    IS_STRMATCH2( opt, "sopmemcap", "sopc") {
        bcm_plp_timesync_inband_parse_t  ipmac_rx = 0, ipmac_tx = 0;
        /* arg[1]/[2] can be 0-7 to select capturing                                *\
        \*          SrcPort/MacDA/MacSA/DstIP/SrcIP/DstIPv6/MPLSlabel into SOPmem   */
        if (arg[1] <= 7 ) {
            arg[1] = _sopmem_ip_mac_capture_mode_convert(arg[1]);
            if ( arg[2] <= 7 ) {
                 arg[2] = _sopmem_ip_mac_capture_mode_convert(arg[2]);
            } else {
                 arg[2] = arg[1];
            }
            addr_capture_rx = arg[1];   addr_capture_tx = arg[2];
            rv |= ts_sopmem_ip_mac_capture_set(chipname, pinfo_a, arg[1], arg[2]);
            rv |= ts_sopmem_ip_mac_capture_set(chipname, pinfo_b, arg[1], arg[2]);
        } else {
            printf("SOPmem IP/MAC capture mode  "
                   "(0:SrcPort 2:MacDA 3:MacSA 4:DstIP 5:SrcIP 7:DstIPv6)\n"
                   "Usage:  ts sopc  <0|2|3|4|5|7>  [<0|2|3|4|5|7>]\n"
                               "\t\t  Capture_RX      Capture_TX\n"         );
        }
        rv = ts_sopmem_ip_mac_capture_get(chipname, pinfo_a, &ipmac_rx, &ipmac_tx);
        printf("  current mode:  (RxCap=%s   TxCap=%s)\n\n",
                                            _sopmem_ip_mac_capture_mode_display(ipmac_rx),
                                            _sopmem_ip_mac_capture_mode_display(ipmac_tx));
        return  rv;
    } else

    IS_STRMATCH2(opt, "sopmemcap_", "sopc_") {   /* to capture Src/Dst MAC or IP */
        const  char  *cap_text[] = { "SrcPort", "MPLS" , "MacDA", "MacSA",
                                     "DstIP"  , "SrcIP", "Resv2", "DstIPv6" };
        const  bcm_plp_timesync_inband_parse_t
               cap_select[] = { bcmplpTimesyncInbandParseSrcPort  , bcmplpTimesyncInbandParseMplsLabel,
                                bcmplpTimesyncInbandParseMacDa    , bcmplpTimesyncInbandParseMacSa    ,
                                bcmplpTimesyncInbandParseIpv4DstIp, bcmplpTimesyncInbandParseIpv4SrcIp,
                                bcmplpTimesyncInbandParseResv2    , bcmplpTimesyncInbandParseIpv6DstIp };
        if ( arg[1] < 8 ) {
            addr_capture_rx = cap_select[arg[1]];
        } else {
            printf("SOPmem IP/MAC capture mode "
                   "(0=SrcPort 2=MacDA 3=MacSA 4=DstIP 5=SrcIP 7=DstIPv6)\n"
                   "Usage:  ts sopc  <0-7>   [<0-7>]\n\t\t(RxCap)  (TxCap)\n"      );
        }
        addr_capture_tx = (arg[2] < 8) ?  cap_select[arg[2]]  :  addr_capture_rx;
        printf("    Current capture mode: RxCap=%s TxCap=%s\n\n",
                cap_text[_simple_log2(addr_capture_rx)], cap_text[_simple_log2(addr_capture_tx)]);
        return  rv;
    } else

    IS_STRMATCH2(opt, "sm", "ti") {   /* lookup/dump/clear SOPmem/SOP_FIFO & TS TxInfo */
        unsigned long long  tstmp;
        if ( arg[1] == TIMESYNC_SOPMEM_CLEAR ) {            /*  delete the entry indexed by arg[2] */
            rv |= ts_sopmem_clear(chipname, tsigr, arg[2]);  /* or clear all SOPmem entries */
            rv |= ts_sopmem_clear(chipname, tsegr, arg[2]);  /*      if arg[2] is not given */
        } else {
            if ( arg[1] == ANDBEYOND ) {   /* arg[1] is not given, show command usage */
                printf("Usage:  ts sm\t\t\t: get the 1st valid entry in SOPmem\n");
                printf("\tts sm <Seq_ID>\t\t: get the 1st entry with matched Seq_ID\n");
                printf("\tts sm 0xf1f0a11\t\t: dump all entries in SOPmem\n");
                printf("\tts sm 0xf1f0de1 [<eid>] : delete the entry #eid "
                                         "(clear all if eid not given)\n\n");
            } else
            if ( arg[1] != TIMESYNC_SOPMEM_DUMP ) {   /* arg[1] is the Seq_ID as a */
                 arg[2]  = arg[1];                    /*      key to lookup SOPmem */
                 arg[1]  = bcmplpTimesyncSOPmemOpRegular;  /*  using linear search */
            } else {
                /* dump all valid SOPmem entries */
            }

            rv |= ts_sopmem(chipname, tsigr, arg[1], arg[2], &tstmp);
            rv |= ts_sopmem(chipname, tsegr, arg[1], arg[2], &tstmp);
        }

      #if  defined(PLP_PM_TIMESTAMPING_SUPPORT)
        {   bcm_plp_access_t        *phyinfo[] = { &tsigr, &tsegr, &pinfo_0 };
            bcm_plp_mac_access_t     macinfo;
            bcm_plp_pm_ts_tx_info_t  ts_tx_info;
            int                      ii;

            for ( ii = 0; ii < 3; ii++ ) {
                memcpy(&(macinfo.phy_info), phyinfo[ii], sizeof(bcm_plp_access_t));
                memset(&ts_tx_info, 0, sizeof(bcm_plp_pm_ts_tx_info_t));
                printf("TS_TxInfo[%2d.%02x] ", macinfo.phy_info.phy_addr, macinfo.phy_info.lane_map);
                rv |= bcm_plp_pm_timesync_tx_info_get(chipname, macinfo, &ts_tx_info);
                if ( 0 == rv ) {
                    printf("seqID=0x%x SUBns=0x%x timestamp=0x%04x_%04x_%04x_%04x (delta=0x%x)\n",
                            ts_tx_info.ts_seq_id          , ts_tx_info.ts_sub_nanosec,
                            ts_tx_info.ts_in_fifo_hi >> 16, ts_tx_info.ts_in_fifo_hi & 0xffff,
                            ts_tx_info.ts_in_fifo_lo >> 16, ts_tx_info.ts_in_fifo_lo & 0xffff,
                            ts_tx_info.ts_in_fifo_lo - (unsigned int) (tstmp & 0xffffffff) );
                } else if ( -8 == rv ) {
                    printf(" < empty >\n");
                    rv = 0;     /* reset the return value */
                } else {
                    printf(" ERROR !!\n");
                }
            }
        }
      #endif  /* PLP_PM_TIMESTAMPING_SUPPORT */
        return  rv;
    } else

    IS_STRMATCH2(opt, "freqstep", "fsc") {    /* set NCO Frequency Stepping control   */
        unsigned int  fsc_a = 0U,  fsc_b = 0U;
        if ( ANDBEYOND == arg[1] )   arg[1] = 0x80000000;   /* 8ns frequency stepping */
        if ( ANDBEYOND == arg[2] )   arg[2] = arg[1]    ;
        rv |= bcm_plp_timesync_nco_addend_set(chipname, pinfo_a, 0, arg[1]);
        rv |= bcm_plp_timesync_nco_addend_set(chipname, pinfo_b, 0, arg[2]);
        rv |= bcm_plp_timesync_nco_addend_get(chipname, pinfo_a, 0, &fsc_a);
        rv |= bcm_plp_timesync_nco_addend_get(chipname, pinfo_b, 0, &fsc_b);

        printf("   Set Frequency Stepping: 0x%08X for [%2d.%02x] , 0x%08X for [%2d.%02x]\n",
                                                 fsc_a, pinfo_a.phy_addr, pinfo_a.lane_map,
                                                 fsc_b, pinfo_b.phy_addr, pinfo_b.lane_map );
        return rv;
    } else

    IS_STRMATCH2(opt, "refclk", "rc") {    /* set external RTP_REFCLK clock source */
        unsigned long long  local_time_code =
                            (((unsigned long long) arg[1]) << 32) | arg[2];  /* ToD Time of Date */

        framesync_trigger = (arg[3] == TIMESYNC_HEARTBEAT_SYNC0) ? bcmplpTimesyncFramsyncModeSyncIn0 :
                            (arg[3] == TIMESYNC_HEARTBEAT_SYNC1) ? bcmplpTimesyncFramsyncModeSyncIn1 :
                            (arg[3] == TIMESYNC_HEARTBEAT_CPU  ) ? bcmplpTimesyncFramsyncModeCpu     :
                                                                   bcmplpTimesyncFramsyncModeNone    ;
        rv |= ts_ext_ptp_ref_clock(chipname, pinfo_a, bcmplpTimesyncFlagClockSrcExtMode,
                                                      framesync_trigger, local_time_code);
        rv |= ts_ext_ptp_ref_clock(chipname, pinfo_b, bcmplpTimesyncFlagClockSrcExtMode,
                                                      framesync_trigger, local_time_code);
        ts_reg_dump(chipname, 0);

        return rv;
    } else

    IS_STRMATCH(opt, "dpll") {    /* set internal DPLL clock source */
        /* arg1 = SyncIn frequency (1/4/8 kHz) ,  arg2 = flags (reset_all/PM1588) */
        if ( P1588_RESET_ALL & arg[2] ) {    /* reset 1588 datapath/registers/timers */
            memset(&cfg_a, 0, sizeof(bcm_plp_timesync_config_t));
            memset(&cfg_b, 0, sizeof(bcm_plp_timesync_config_t));
            rv |= bcm_plp_timesync_config_get(chipname, pinfo_a, &cfg_a);
            rv |= bcm_plp_timesync_config_get(chipname, pinfo_b, &cfg_b);
        }
        arg[2] = (P1588_PORT_MACRO & arg[2]) ? TRUE : FALSE;    /* PortMacro PM1588 enable/disable */
      #if  defined(PLP_PM_TIMESTAMPING_SUPPORT)
        rv |= ts_dpll_ptp_clock(chipname, pinfo_0, (bcmplpTimesyncTx | bcmplpTimesyncRx),
                                arg[1], TIMESYNC_LOCAL_TIME_SEC, TIMESYNC_LOCAL_TIME_NS, arg[2]);
        if (arg[2] == 1) {
            rv |= tc_pm_timesync(chipname, pinfo_0_line, arg[2]);
        }
      #endif
        rv |= ts_dpll_ptp_clock(chipname, pinfo_a, (bcmplpTimesyncTx | bcmplpTimesyncRx),
                                arg[1], TIMESYNC_LOCAL_TIME_SEC, TIMESYNC_LOCAL_TIME_NS, arg[2]);
        if (arg[2] == 1) {
            rv |= tc_pm_timesync(chipname, pinfo_a_line, arg[2]);
        }

        rv |= ts_dpll_ptp_clock(chipname, pinfo_b, (bcmplpTimesyncTx | bcmplpTimesyncRx),
                                arg[1], TIMESYNC_LOCAL_TIME_SEC, TIMESYNC_LOCAL_TIME_NS, arg[2]);
        if (arg[2] == 1) {
            rv |= tc_pm_timesync(chipname, pinfo_b_line, arg[2]);
        }
        rv |= ts_reg_dump(chipname, 3);
        return  rv;
    } else

    IS_STRMATCH2(opt, "shadow" , "sha" ) {    /* set shadow load reg. & use default  FrameSync */
        return  fsync_shadow_set(chipname, NULL, arg[1], arg[2], framesync_trigger);
    } else

    IS_STRMATCH2(opt, "shadow1", "sha1") {    /* set shadow load reg. & use Sync1 as FrameSync */
        return  fsync_shadow_set(chipname, NULL, arg[1], arg[2], bcmplpTimesyncFramsyncModeSyncIn1);
    } else

    IS_STRMATCH2(opt, "shadow0", "sha0") {    /* set shadow load reg. & use Sync0 as FrameSync */
        return  fsync_shadow_set(chipname, NULL, arg[1], arg[2], bcmplpTimesyncFramsyncModeSyncIn0);
    } else

    IS_STRMATCH(opt, "tod") {    /* set Time of Day (ToD) */
        unsigned long long  tod = 0;
        if ( IS_PHY_INVALID_ARG(arg[1]) )    arg[1] = 0;
        if ( IS_PHY_INVALID_ARG(arg[2]) )    arg[2] = 0;
        tod = (((unsigned long long) arg[1]) << 32) | arg[2];
        rv |= ts_tod_set(chipname, pinfo_a, tod);
        rv |= ts_tod_set(chipname, pinfo_b, tod);
        return  rv;
    } else

    IS_STRMATCH2(opt, "heartbeat", "hb") {    /* get local Heartbeat */
        if ( arg[3] != 80 )    arg[3] = 48;   /* 48-bit or 80-bit heartbeat */
        if ( (0 < arg[2]) && (arg[2] < 0xffff) ) {
            /* check the heartbeat increment on one individual port */
            static  unsigned long long  hb_prev    = 0LL;
            static  long long           delta_prev = 0LL;

            rv |= ts_heartbeat_show(chipname, (arg[2] & 1) ? pinfo_a : pinfo_b, arg[3], &hb_prev);
            printf("\n");
            for ( arg[1] &= 0x3F; arg[1]; arg[1]-- ) {    /* at most 64 iterations for the FOR loop */
                unsigned long long  hb    = (unsigned long long) framesync_trigger;
                long long           delta = 0LL;
                int                 accel = 0;
                if ( rv )    printf("\tERROR !!\n");

                rv |= ts_heartbeat_show(chipname, (arg[2] & 1) ? pinfo_a : pinfo_b, arg[3], &hb);
                delta = hb - hb_prev;     accel = delta - delta_prev;
                printf("   DELTA=0x%09llx (%11lld)  Accel=%10d\n",
                                                   delta & 0xFFFFFFFFFLL, delta, accel);
                hb_prev = hb;    delta_prev = delta;
                usleep(HEARTBEAT_FETCH_INTERVAL);     /* sleep approximately 1 second */
            }
        } else {
            /* compare the heartbeat of Rx & Tx ports */
            unsigned long long  hba = 0LL, hbb = 0LL;
            if ( (99 == arg[1]) || (9090 == arg[1]) )  {
                /* manually trigger heartbeat by CPU */
                if (  0x1 == (arg[2] & 0x1) ) {
                    rv |= ts_heartbeat_cpu_tirgger(chipname, pinfo_a);
                }
                if ( (0x2 == (arg[2] & 0x2)) || (0 == arg[2]) ) {
                    rv |= ts_heartbeat_cpu_tirgger(chipname, pinfo_b);
                }
                return  rv;
            }
            else if ( 0 == arg[1] )  {
                arg[1] = 10;      /* 10 iteration by default */
            }
            else { };

            for ( arg[1] &= 0x3F; arg[1]; arg[1]-- ) {    /* at most 64 iterations for the FOR loop */
                rv |= ts_heartbeat_show(chipname, pinfo_a, arg[3], &hba);    printf("\n");
                rv |= ts_heartbeat_show(chipname, pinfo_b, arg[3], &hbb);
                printf("\tHeartbeat diff = %d\n\n", (int) (hbb - hba));
                usleep(HEARTBEAT_FETCH_INTERVAL);     /* sleep approximately 1 second */
            }
        }
        return  rv;
    } else

    IS_STRMATCH2(opt, "localtime", "lt") {    /* get the Local Time and Time Code */
        unsigned long long  hb0 = 0LL, hba = 0LL, hbb = 0LL, lt64 = 0LL;
        bcm_plp_timesync_timespec_t  timespec = { 0, 0LL, 0 };
        if ( arg[3] != 80 )    arg[3] = 48;   /* 48-bit or 80-bit heartbeat */

        if ( ! IS_PHY_INVALID_ARG(arg[1]) ) {
            if ( IS_PHY_INVALID_ARG(arg[2]) )    arg[2] = 0;
            lt64 = timespec.seconds = (((unsigned long long) arg[1]) << 32) | arg[2];
            rv |= bcm_plp_timesync_time_code_set( chipname, pinfo_a, 0, &timespec);
            rv |= bcm_plp_timesync_local_time_set(chipname, pinfo_a, 0, lt64     );
            rv |= bcm_plp_timesync_time_code_set( chipname, pinfo_b, 0, &timespec);
            rv |= bcm_plp_timesync_local_time_set(chipname, pinfo_b, 0, lt64     );
            printf("rv=%d port [%2d.%02x]  Set localTime/TimeCode = 0x%llx\n", rv,
                                         pinfo_a.phy_addr, pinfo_a.lane_map, lt64);
        }
      #if  defined(PLP_PM_TIMESTAMPING_SUPPORT)
        rv |= ts_local_time_code_show(chipname, pinfo_0);
        rv |= ts_heartbeat_show(chipname, pinfo_0, arg[3], &hb0);
        printf("\t<--- Port 0\n");
      #endif
        rv |= ts_local_time_code_show(chipname, pinfo_a);
        rv |= ts_heartbeat_show(chipname, pinfo_a, arg[3], &hba);
        printf("\tHeartbeat diff = %d\n"  , (int) (hba - hb0));
        rv |= ts_local_time_code_show(chipname, pinfo_b);
        rv |= ts_heartbeat_show(chipname, pinfo_b, arg[3], &hbb);
        printf("\tHeartbeat diff = %d (%d)\n\n", (int) (hbb - hb0), (int) (hbb - hba));

        return  rv;
    } else

    IS_STRMATCH(opt, "mpls") {    /* set MPLS parameters */
        rv |= ts_mpls(chipname, pinfo_a, TIMESYNC_DIRECTION_RXTX, arg[2], arg[1]);
        rv |= ts_mpls(chipname, pinfo_b, TIMESYNC_DIRECTION_RXTX, arg[2], arg[1]);
        rv |= ts_reg_dump(chipname, 6);
        return  rv;
    } else

    IS_STRMATCH2(opt, "igregr", "ie") {    /* specify ingress & egress ports */
        /* set the ingress & egress ports of IEEE1588 PTP event messages */
        tsigr.phy_addr = UINT32_HI16(arg[1]);   tsigr.lane_map = UINT32_LO16(arg[1]);
        tsegr.phy_addr = UINT32_HI16(arg[2]);   tsegr.lane_map = UINT32_LO16(arg[2]);
        return  rv;
    } else

    IS_STRMATCH2(opt, "framesync", "fs") {   /* manually trigger a one-time FrameSync */
        rv |= bcm_plp_timesync_do_sync(chipname, tsigr);
        rv |= bcm_plp_timesync_do_sync(chipname, tsegr);
        printf("rv=%d Trigger one-time FrameSync on port %d & %d\n",
                                          rv, tsigr.phy_addr, tsegr.phy_addr);
        return  rv;
    } else

  #ifdef BCM_PLP_TIMESYNC_V2_1_SUPPORT
    IS_STRMATCH2(opt, "action", "act") {   /* PTP lookup-action mode */
        bcm_plp_access_t                    pinfo = {&p_ctxt, arg[1], LINE_SIDE, IGR_LANE};
        bcm_plp_timesync_user_action_t      userdef;
        bcm_plp_timesync_txrx_t             dir = (bcm_plp_timesync_txrx_t) arg[2];
        bcm_plp_timesync_ptp_action_mode_t  act[3] = { (bcm_plp_timesync_ptp_action_mode_t) arg[3] };

        if ( arg[1] >= PHY_INVALID_ARG ) {
            printf("Usage:  ts action  <phy_ID>  <1=Rx 2=Tx 3=RxTx>  <action_mode>\n\n");
            return  rv;
        }

        if ( dir >= TIMESYNC_DIRECTION_RXTX ) {
             dir  = TIMESYNC_DIRECTION_RXTX;
        }
        if ( act[0] < bcmplpTimesyncPtpActionModeSetActionOnly ) {
            rv |= ts_lookup_action_set(chipname, pinfo, dir, act[0]);
            printf("rv=%d  port %d SET  %s_action=0x%x\n", rv, pinfo.phy_addr,
                   (TIMESYNC_DIRECTION_RXTX == dir) ? "RxTx" :
                   (TIMESYNC_DIRECTION_RX   == dir) ? "Rx"   : "TX" ,  act[0]);
        }

        for ( dir = bcmplpTimesyncTx; dir > 0; dir >>= 1 ) {
            rv |= ts_lookup_action_get(chipname, pinfo, dir, &act[dir], &userdef);
        }
        printf("rv=%d  port %d Lookup Action :  TX_action=0x%x\tRX_action=0x%x",
                rv, pinfo.phy_addr, act[bcmplpTimesyncTx], act[bcmplpTimesyncRx]);
      #if defined(__LOOKUP_ACTION_USeR_DEFINED__)
        printf("=== dir=0x%x act=[0x%x 0x%x 0x%x]\n", dir, act[0], act[1], act[2]);
        if ( bcmplpTimesyncPtpActionModeUserDefine == act ) {
            printf("  user_def=[%x %x %x %x  %x %x %x %x  %x]",
                    userdef.sync_1step, userdef.sync_2step, userdef.delay_request, userdef.sync_followup,
                    userdef.pdelay_request, userdef.pdelay_response_1step, userdef.pdelay_response_2step,
                    userdef.pdelay_response_followup, userdef.delay_response);
        }
      #endif
        printf("\n\n");
        return  rv;
    } else
  #endif  /* BCM_PLP_TIMESYNC_V2_1_SUPPORT */

    IS_STRMATCH2(opt, "disable", "off") {  /* disable TimeSync 1588 */
        if ( (arg[1] < 0x1) || (0x3 < arg[1]) )    arg[1] = 0x3;    /* both Rx & Tx */
        if ( arg[1] & 0x1 ) {   /* Rx direction */
            rv |= ts_off(chipname, tsigr, P1588_RESET_ALL);
        }
        if ( arg[1] & 0x2 ) {   /* Tx direction */
            rv |= ts_off(chipname, tsegr, P1588_RESET_ALL);
        }
        return  rv;
    } else

    IS_STRMATCH(opt, "dump") {   /* dump TimeSync 1588 setting */
        rv |= ts_reg_dump_all(chipname, tsigr);
        rv |= ts_reg_dump_all(chipname, tsegr);
        return  rv;
    } else

    IS_STRMATCH2(opt, "regdump", "rd") {   /* dump TimeSync 1588 registers */
        rv |= ts_reg_dump(chipname, (arg[1] > 5) ? 5 : arg[1]);
        return  rv;
    }

    /*
     *  dispatching Timestamp T1/T2/T3/T4, TC/PTC and other operations
     */
    switch ( atoi(opt) ) {

    case 11 :       /* Timestamp T1 -- one step inband */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_t1(chipname, tsegr, TIMESYNC_DIRECTION_TX,
                              TIMESYNC_T1_ONE_STEP, TIMESYNC_INBAND, /* inband      */
                              bcmplpTimesyncTimerMode80Bit, &ecfg);  /* one-step T1 */
        rv |= ts_reg_dump(chipname, 0);
        break;
    case 12 :       /* Timestamp T1 -- two step inband */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_t1(chipname, tsegr, TIMESYNC_DIRECTION_TX,
                              TIMESYNC_T1_TWO_STEP, TIMESYNC_INBAND, /* inband      */
                              bcmplpTimesyncTimerMode80Bit, &ecfg);  /* two-step T1 */
        rv |= ts_reg_dump(chipname, 0);
        break;
    case 15 :       /* Timestamp T1 -- one step out-of-band */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_t1(chipname, tsegr, TIMESYNC_DIRECTION_TX,
                              TIMESYNC_T1_ONE_STEP, TIMESYNC_OOBAND, /* out-of-band */
                              bcmplpTimesyncTimerMode80Bit, &ecfg);  /* one-step T1 */
        rv |= ts_reg_dump(chipname, 0);
        break;
    case 16 :       /* Timestamp T1 -- two step out-of-band */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_t1(chipname, tsegr, TIMESYNC_DIRECTION_TX,
                              TIMESYNC_T1_TWO_STEP, TIMESYNC_OOBAND, /* out-of-band */
                              bcmplpTimesyncTimerMode80Bit, &ecfg);  /* two-step T1 */
        rv |= ts_reg_dump(chipname, 0);
        break;

    case 21 :       /* Timestamp T2 inband */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        ts_t2(chipname, tsigr, TIMESYNC_DIRECTION_RX, TIMESYNC_INBAND,
                               bcmplpTimesyncTimerMode80Bit, &icfg);
        rv |= ts_reg_dump(chipname, 0);
        break;
    case 25 :       /* Timestamp T2 out-of-band */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        ts_t2(chipname, tsigr, TIMESYNC_DIRECTION_RX, TIMESYNC_OOBAND,
                               bcmplpTimesyncTimerMode80Bit, &icfg);
        rv |= ts_reg_dump(chipname, 0);
        break;

    case 31 :       /* Timestamp T3 inband */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset( chipname, tsegr, P1588_RESET_ALL, &ecfg);
        /* if (arg1 == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg); */
        rv |= ts_t3_t4( chipname, tsegr, TIMESYNC_DIRECTION_TX,
                                        bcmplpTimesyncTimerMode80Bit, &ecfg);
        /* rv |= ts_t3_t4(chipname, tsigr, TIMESYNC_DIRECTION_RX, &icfg); */
        rv |= ts_reg_dump(chipname, 0);
        break;
    case 35 :       /* Timestamp T3 out-of-band */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset( chipname, tsegr, P1588_RESET_ALL, &ecfg);
        /* if (arg1 == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg); */
        rv |= ts_t3_oob(chipname, tsegr, TIMESYNC_DIRECTION_TX,
                                         bcmplpTimesyncTimerMode80Bit, &ecfg);
        /* rv |= ts_t3_oob(chipname, tsigr, TIMESYNC_DIRECTION_RX, &icfg); */
        rv |= ts_reg_dump(chipname, 0);
        break;

    case 41 :       /* Timestamp T4 inband */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset( chipname, tsigr, P1588_RESET_ALL, &icfg);
        rv |= ts_t3_t4( chipname, tsigr, TIMESYNC_DIRECTION_RX,
                                        bcmplpTimesyncTimerMode80Bit, &icfg);
        rv |= ts_reg_dump(chipname, 0);
        break;
    case 45 :       /* Timestamp T4 out-of-band */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset( chipname, tsigr, P1588_RESET_ALL, &icfg);
        rv |= ts_t4_oob(chipname, tsigr, TIMESYNC_DIRECTION_RX,
                                         bcmplpTimesyncTimerMode80Bit, &icfg);
        rv |= ts_reg_dump(chipname, 0);
        break;

    case 90 :       /* Regular Transparent Clock */
        link_delay    = (arg[1] < PHY_INVALID_ARG) ? arg[1] : TIMESYNC_LINK_DELAY;
        ts_offset_rx  = (arg[2] < PHY_INVALID_ARG) ? (arg[2] & 0xFFFF) : TIMESYNC_TS_OFFSET_RX;
        ts_offset_tx  = (arg[2] < PHY_INVALID_ARG) ? (arg[2] >> 16   ) : TIMESYNC_TS_OFFSET_TX;

        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_tc(chipname, tsigr, TIMESYNC_DIRECTION_TXRX, bcmplpTimesyncTimerMode48Bit,
                                     link_delay, ts_offset_rx, ts_offset_tx,
                                     TIMESYNC_LOCAL_TIME_SEC, TIMESYNC_LOCAL_TIME_NS, &icfg);
        rv |= ts_tc(chipname, tsegr, TIMESYNC_DIRECTION_TXRX, bcmplpTimesyncTimerMode48Bit,
                                     link_delay, ts_offset_rx, ts_offset_tx,
                                     TIMESYNC_LOCAL_TIME_SEC, TIMESYNC_LOCAL_TIME_NS, &ecfg);
        rv |= ts_reg_dump(chipname, 0);
        break;
    case 92 :       /* Partial Transparent Clock -- Inband 32-bit format */
        flags |= TIMESYNC_INBAND32_FORMAT;
    case 91 :       /* Partial Transparent Clock -- Inband 36-bit format */
        flags |= TIMESYNC_DIRECTION_RXTX;
        link_delay    = (arg[1] < PHY_INVALID_ARG) ? arg[1] : TIMESYNC_LINK_DELAY;
        ts_offset_rx  = (arg[2] < PHY_INVALID_ARG) ? (arg[2] & 0xFFFF) : TIMESYNC_TS_OFFSET_RX;
        ts_offset_tx  = (arg[2] < PHY_INVALID_ARG) ? (arg[2] >> 16   ) : TIMESYNC_TS_OFFSET_TX;

        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_ptc(chipname, tsigr, flags, bcmplpTimesyncTimerMode80Bit,
                                      link_delay, ts_offset_rx, ts_offset_tx, &icfg);
        rv |= ts_ptc(chipname, tsegr, flags, bcmplpTimesyncTimerMode80Bit,
                                      link_delay, ts_offset_rx, ts_offset_tx, &ecfg);
        rv |= ts_reg_dump(chipname, 0);
        break;

    case 61 :       /* DPLL for PTP Clock */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_dpll_ptp_clock(chipname, pinfo_a, (bcmplpTimesyncTx | bcmplpTimesyncRx),
                                arg[1], TIMESYNC_LOCAL_TIME_SEC, TIMESYNC_LOCAL_TIME_NS, arg[2]);
        rv |= ts_dpll_ptp_clock(chipname, pinfo_b, (bcmplpTimesyncTx | bcmplpTimesyncRx),
                                arg[1], TIMESYNC_LOCAL_TIME_SEC, TIMESYNC_LOCAL_TIME_NS, arg[2]);
        rv |= ts_reg_dump(chipname, 3);
        break;
    case 62 :       /* External PTP_XTAL for PTP Clock */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_ext_xtal_ptp_clock(chipname, tsigr, TIMESYNC_DIRECTION_TXRX, &icfg);
        rv |= ts_ext_xtal_ptp_clock(chipname, tsegr, TIMESYNC_DIRECTION_TXRX, &ecfg);
        rv |= ts_reg_dump(chipname, 3);
        break;

    case 66 :    {          /* peek at timestamp and heartbeat */
        unsigned long long  data64out2, data64out1;
        rv |= bcm_plp_timesync_capture_timestamp_get(chipname, tsigr, 0, &data64out1);
        rv |= bcm_plp_timesync_capture_timestamp_get(chipname, tsegr, 0, &data64out2);
        printf("rv=%d bcm_timesync_capture_timestamp   : out=0x%llx\n", rv, data64out1);
        printf("rv=%d bcm_timesync_capture_timestamp   : out=0x%llx\n", rv, data64out2);

        /* [ rd_time_code ]  read Heartbeat Timestamp */
        rv |= bcm_plp_timesync_heartbeat_timestamp_get(chipname, tsigr, 0, &data64out1);
        printf("rv=%d p=%d bcm_timesync_heartbeat_timestamp : 0x%llx\n", rv, tsigr.phy_addr, data64out1);
        rv |= bcm_plp_timesync_heartbeat_timestamp_get(chipname, tsegr, 0, &data64out2);
        printf("rv=%d p=%d bcm_timesync_heartbeat_timestamp : 0x%llx\n", rv, tsegr.phy_addr, data64out2);
        }
        break;

    case 108 :      /* Timing value functions verification */
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsigr, P1588_RESET_ALL, &icfg);
        if (arg[1] == P1588_RESET_FROM_CLI)  rv |= ts_reset(chipname, tsegr, P1588_RESET_ALL, &ecfg);
        rv |= ts_timing_value_check(chipname, tsigr);
        rv |= ts_timing_value_check(chipname, tsegr);
        break;
    case 109 :    {          /* TimeSync Control items */
        bcm_plp_timesync_framesync_t  framesync;
        unsigned int        data32out1, data32out2, data32in1, data32in2;
        unsigned long long  data64out, data64in = 0x1234567890abcdefLL;

        memset(&framesync, 0, sizeof(bcm_plp_timesync_framesync_t));

        if ( (data32in1 = (arg[1] & 0x7)) > bcmplpTimesyncFramsyncModeCpu) {
            data32in1 = bcmplpTimesyncFramsyncModeCpu;
        }
        framesync.mode = data32in1;
        rv |= bcm_plp_timesync_framesync_mode_set(chipname, tsigr, 0, &framesync);
        rv |= bcm_plp_timesync_framesync_mode_get(chipname, tsigr, 0, &framesync);
        data32out1 = framesync.mode;
        printf("rv=%d bcm_timesync_framesync_mode : in=0x%x out=0x%x\n", rv, data32in1, data32out1);

        data32in1 = data32in2 = arg[1];
        rv |= bcm_plp_timesync_nco_addend_set(chipname, tsigr, 0,  data32in1);
        rv |= bcm_plp_timesync_nco_addend_get(chipname, tsigr, 0, &data32out1);
        printf("rv=%d bcm_timesync_nco_addend : in=0x%x out=0x%x\n", rv, data32in1, data32out1);

        rv |= bcm_plp_timesync_local_time_set(chipname, tsigr, 0,  data64in);
        rv |= bcm_plp_timesync_local_time_get(chipname, tsigr, 0, &data64out);
        printf("rv=%d bcm_timesync_local_time : in=0x%llx out=0x%llx\n", rv, data64in, data64out);

        rv |= bcm_plp_timesync_load_ctrl_set(chipname, tsigr, 0, data32in1, data32in2);
        rv |= bcm_plp_timesync_load_ctrl_get(chipname, tsigr, 0, &data32out1, &data32out2);
        printf("rv=%d bcm_timesync_load_ctrl  : once=(0x%x 0x%x) always=(0x%x 0x%x)\n",
                        rv, data32in1, data32out1, data32in2, data32out2);

        rv |= bcm_plp_intr_enable_set(chipname, tsegr, arg[1] & 0x1f, TIMESYNC_ENABLE );
        rv |= bcm_plp_intr_enable_get(chipname, tsegr, arg[1] & 0x1f, &data32out1);
        printf("rv=%d bcm_plp_intr_enable : in=0x%x out=0x%x\n", rv, arg[1] & 0x1f, data32out1);
        }
        break;

    default :
        rv  = ts_reg_dump(chipname, 0);
        rv  = ts_reg_dump(chipname, 3);
        memset(&icfg, 0, sizeof(bcm_plp_timesync_config_t));
        memset(&ecfg, 0, sizeof(bcm_plp_timesync_config_t));
        rv |= bcm_plp_timesync_config_get(chipname, tsigr, &icfg);
        rv |= bcm_plp_timesync_config_get(chipname, tsegr, &ecfg);
    }

    return rv;
}


int _port_phyinfo_retrieve_pm(char *chipname, int port, bcm_plp_access_t *phyinfo) {
    int  rv = 0;

    phyinfo->platform_ctxt = &p_ctxt;

  #ifdef GEARBOX_MODE
    phyinfo->phy_addr      =  TC1588_PHY_ID_PM( port);
    phyinfo->lane_map      =  TC1588_LANEMAP_PM(port);
  #else
    phyinfo->phy_addr      =  TC1588_PHY_ID( port);
    phyinfo->lane_map      =  TC1588_LANEMAP(port);
  #endif
    phyinfo->if_side       =  LINE_SIDE;

    return  rv;
}

int timesync_verify_tc_dpll(char *chipname, char *cmd, int port, unsigned int arg[]) 
{
    int  rv = 0;
    bcm_plp_access_t           pi  = { 0 };
    bcm_plp_timesync_config_t  cfg = { 0 };

    if ( (port < 0) || (NUM_PORT <= port) ) {
        port =  NUM_PORT;
    } else {
        rv  |=  _port_phyinfo_retrieve(chipname, port, &pi);
    }

    IS_STRMATCH(cmd, "dpll") {    /* set DPLL Synchronization coefficients */
        printf(" Port %2d : ", port);
        /* arg[1] = SyncIn frequency (1/4/8 kHz) ,  arg[2] = flags (reset_all/PM1588_RxTx)  */
        if ( P1588_RESET_ALL & arg[2] ) {    /* reset 1588 datapath/registers/timers */
            memset(&cfg, 0, sizeof(bcm_plp_timesync_config_t));
            rv |= bcm_plp_timesync_config_get(chipname, pi, &cfg);
            printf("!! RESET !! (arg[2]=0x%x) ", arg[2]);
        }
        rv |= ts_dpll_ptp_clock(chipname, pi,
                                (bcmplpTimesyncTx | bcmplpTimesyncRx), arg[1] /* 1/4/8KHz */,
                                TIMESYNC_LOCAL_TIME_SEC, TIMESYNC_LOCAL_TIME_NS, arg[2] /* PM_enable */ );
        if (arg[2] == 1) {
            rv |= _port_phyinfo_retrieve_pm(chipname, port, &pi);
            rv |= tc_pm_timesync(chipname, pi, arg[2] /* PM_enable */ );
        }

        return  rv;
    } else

    IS_STRMATCH2(cmd, "shadow" , "sha" ) {   /* set shadow load reg. & use Sync1 as FrameSync */
       return  fsync_shadow_set(chipname, &pi, arg[1], arg[2], bcmplpTimesyncFramsyncModeSyncIn1);
    } else

    IS_STRMATCH2(cmd, "shadow0", "sha0") {   /* set shadow load reg. & use Sync0 as FrameSync */
       return  fsync_shadow_set(chipname, &pi, arg[1], arg[2], bcmplpTimesyncFramsyncModeSyncIn0);
    } else

    IS_STRMATCH(cmd, "tod") {    /* set Time of Day (ToD) */
        unsigned long long  tod = 0;

        if ( IS_PHY_INVALID_ARG(arg[1]) ) {   /* get the heartbeat clock from port 0 */
            rv |= bcm_plp_timesync_heartbeat_timestamp_get(chipname, pinfo[0], 0x0, &tod);
            tod += 1000000000LL;   /* advance 1 second */
        } else {                     /* ToD value from user inputs          */
            if ( IS_PHY_INVALID_ARG(arg[2]) )     arg[2] = 0;
            tod = (((unsigned long long) arg[1]) << 32) | arg[2];
        }
        printf(" Port %2d (0x%x 0x%x): ", port, arg[1], arg[2]);
        rv |= ts_tod_set(chipname, pi, tod);
        return  rv;
    } else

    IS_STRMATCH2(cmd, "heartbeat", "hb") {    /* get local Heartbeat */
        if ( (port < 0) || (NUM_PORT <= port) ) {
            int  ii = 0, active = 0;
            unsigned long long  hb0 = 0LL, hba = 0LL, hbb = 0LL;
            for ( ii = 0; ii < NUM_PORT; ii++ ) {
                if ( pinfo[ii].platform_ctxt ) {
                    rv |= ts_heartbeat_show(chipname, pinfo[ii], 48, &hbb);
                    if ( ii == 0 ) {
                        hb0 = hbb;
                        printf("\t<--- Port 0\n");
                    } else if ( active == 1 ) {
                        hba = hbb;
                        printf("\tHeartbeat diff = %lld\n"  , (hba - hb0));
                    } else {
                        printf("\tHeartbeat diff = %lld (%lld)\n", (hbb - hb0), (hbb - hba));
                    }
                    active++;
                }
            }
            printf("Party size: %2d\n", active);
        }
        else {
            static  unsigned long long  hb_prev = 0LL;
            static  long long           delta_prev = 0LL;

            rv |= ts_heartbeat_show(chipname, pi, 48, &hb_prev);
            printf("\n");
            for ( arg[1] &= 0x3F; arg[1]; arg[1]-- ) {    /* at most 64 iterations for the FOR loop */
                unsigned long long  hb = 0LL;
                long long           delta = 0LL;
                if ( rv )    printf("\tERROR !!\n");

                rv |= ts_heartbeat_show(chipname, pi, 48, &hb);
                delta = hb - hb_prev;
                printf("   DELTA=0x%09llx (%lld)  D-Delta=%lld\n", delta, delta, delta - delta_prev);
                hb_prev = hb;    delta_prev = delta;
                usleep(720000);     /* sleep 0.72 second */
            }
        }
        return  rv;
    } else

    IS_STRMATCH2(cmd, "localtime", "lt") {    /* get the Local Time and Time Code */
        int  ii = 0, active = 0;
        unsigned long long  hb0 = 0LL, hba = 0LL, hbb = 0LL;
        for ( ii = 0; ii < NUM_PORT; ii++ ) {
            if ( pinfo[ii].platform_ctxt ) {
                rv |= ts_local_time_code_show(chipname, pinfo[ii]);
                rv |= ts_heartbeat_show(chipname, pinfo[ii], 48, &hbb);
                if ( ii == 0 ) {
                    hb0 = hbb;
                    printf("\t<--- Port 0\n");
                } else if ( active == 1 ) {
                    hba = hbb;
                    printf("\tHeartbeat diff = %d\n"  , (int) (hba - hb0));
                } else {
                    printf("\tHeartbeat diff = %d (%d)\n", (int) (hbb - hb0), (int) (hbb - hba));
                }
                active++;
            }
        }
        printf("Party size: %2d\n", active);

        return  rv;
    } else

    IS_STRMATCH2(cmd, "add", "tc") {   /* add a port to join Transpraent Clock */
        long long  link_delay = 0LL, ts_offset_rx = 0LL, ts_offset_tx = 0LL;
        link_delay    = (arg[1] < PHY_INVALID_ARG) ? arg[1] : TIMESYNC_LINK_DELAY;
        ts_offset_rx  = (arg[2] < PHY_INVALID_ARG) ? (arg[2] & 0xFFFF) : TIMESYNC_TS_OFFSET_RX;
        ts_offset_tx  = (arg[2] < PHY_INVALID_ARG) ? (arg[2] >> 16   ) : TIMESYNC_TS_OFFSET_TX;

        rv |= ts_tc(chipname, pi, TIMESYNC_DIRECTION_TXRX, bcmplpTimesyncTimerMode48Bit,
                                  link_delay, ts_offset_rx, ts_offset_tx,
                                  TIMESYNC_LOCAL_TIME_SEC, TIMESYNC_LOCAL_TIME_NS, &cfg);
        ts_reg_dump_header(regad_1, FALSE);
        ts_reg_dump_group(chipname, &pi, 0, regad_1, NULL);
        ts_reg_dump_header(regad_2, FALSE);
        ts_reg_dump_group(chipname, &pi, 0, regad_2, NULL);
        printf(" Port %2d  has joint TC\n", port);
    } else

    IS_STRMATCH2(cmd, "delete", "bye") {  /* disable TimeSync 1588 for ports */
        if      ( (port < 0) || (NUM_PORT <= port) )  { arg[1] = NUM_PORT-1 ; port = 0; }  /* do all ports */
        else if ( (arg[1] < 0) || (NUM_PORT <= arg[1]) )  { arg[1] = port; }               /* do one port only */
        else  { }

        printf(" Deleting (%d - %d) ...\n", port, arg[1]);
        for ( ; port <= arg[1]; port++ ) {
            if ( pinfo[port].platform_ctxt ) {
                 rv |= ts_off(chipname, pinfo[port], P1588_RESET_TC);
                 pinfo[port].platform_ctxt = NULL;
            }
        }
        printf("\n");
    } else

    IS_STRMATCH2(cmd, "reset", "rst") {   /* reset TimeSync 1588 setting */
        /* reset type: 0=ALL, 0x1=NSE, 0x2=registers, 0x4=soft_reset_1588, (none)=TC */
        if ( 0        == arg[1] )    arg[1] = P1588_RESET_ALL;   /* reset ALL            */
        if ( PHY_INVALID_ARG == arg[1] )    arg[1] = P1588_RESET_TC ;   /* reset TC by default  */
        /* if arg[2] != 0, do 1588 base configuration for this port */
        rv |= ts_reset(chipname, pi, arg[1], (arg[2] != 0) ? &cfg : NULL);
        printf(" Port %2d  reset\n", port);
    } else

    IS_STRMATCH(cmd, "dump") {   /* dump TimeSync 1588 setting */
        if ( (port < 0) || (NUM_PORT <= port) )    port = 0;
        printf("Dump 1588 register for Port %2d :\n", port);
        rv = ts_reg_dump_all(chipname, pinfo[port]);

    } else {
        int  ii = 0;
        for ( ii = 0; ii < NUM_PORT; ii++ ) {
            if ( pinfo[ii].platform_ctxt ) {
                ts_reg_dump_header(regad_1, FALSE);
                ts_reg_dump_group(chipname, &pi, 0, regad_1, NULL);
                ts_reg_dump_header(regad_2, FALSE);
                ts_reg_dump_group(chipname, &pi, 0, regad_2, NULL);
            }
        }
    }

    return rv;
}

#endif /* BCM_PLP_TIMESYNC_SUPPORT */

