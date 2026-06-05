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

#ifndef __TIMESYNC_HELPER_C__
#define __TIMESYNC_HELPER_C__

#ifdef BCM_PLP_TIMESYNC_SUPPORT
#include "timesync_refapp.h"

/*******************************************************************************************************\
 *   PTPv2.1 Action Lookup-based mode
\*******************************************************************************************************/
#ifdef  BCM_PLP_TIMESYNC_V2_1_SUPPORT

/* helper function to set the PTP lookup-action mode */
int ts_lookup_action_set(char *chipname, bcm_plp_access_t  phy_info,
                                         bcm_plp_timesync_txrx_t  rxtx,
                                         bcm_plp_timesync_ptp_action_mode_t  mode) {
    int  rv = 0;
    bcm_plp_timesync_user_action_t  user_def = {1,2,3,4,5,6,7,8,9};

    if ( (mode > bcmplpTimesyncPtpActionModeInvalid) &&
         (mode < bcmplpTimesyncPtpActionModeSetActionOnly) ) {
        rv |= bcm_plp_timesync_enable_set(chipname, phy_info, rxtx, TIMESYNC_ENABLE);
    }
    rv |= bcm_plp_timesync_ptp_action_set(chipname, phy_info, 0, rxtx, mode, &user_def);
    return  rv;
}

/* helper function to get the PTP lookup-action mode */
int ts_lookup_action_get(char *chipname, bcm_plp_access_t  phy_info,
                                         bcm_plp_timesync_txrx_t  rxtx,
                                         bcm_plp_timesync_ptp_action_mode_t *mode,
                                         bcm_plp_timesync_user_action_t *user_def) {
    int  rv = 0;

    rv |= bcm_plp_timesync_ptp_action_get(chipname, phy_info, 0, rxtx, mode, user_def);
    return  rv;
}

#endif  /* BCM_PLP_TIMESYNC_V2_1_SUPPORT */


/*******************************************************************************************************\
 *   SOPmem operations
\*******************************************************************************************************/

/* read/lookup the SOPmem and print valid entries */
int ts_sopmem_get(char *chipname, bcm_plp_access_t pinfo, unsigned int seqid,
                  unsigned int entry_id, unsigned int flags, unsigned long long *tstmp) {
    bcm_plp_timesync_sopmem_t  sop = { .seq_id = seqid,       .valid = TBD8,
                                       .domain_num = UNKNOWN, .direction = UNKNOWN, .msg_type = UNKNOWN,
                                      #ifdef  BCM_PLP_TIMESYNC_V2_1_SUPPORT
                                       .src_port_id     = { TBD8, TBD8 },
                                      #else
                                       .classified_data = { TBD8, TBD8 }, .vlan_id = UNKNOWN,
                                      #endif
                                       .lookup_key_mask =  bcmplpTimesyncSOPmemSeqId          |
                                                           bcmplpTimesyncSOPmemDomainNum      |
                                                        #ifdef  BCM_PLP_TIMESYNC_V2_1_SUPPORT
                                                           /* PTPv2.1 PHY has full SourcePortIdentity */
                                                           bcmplpTimesyncSOPmemSrcPort        |
                                                        #endif
                                                           bcmplpTimesyncSOPmemDirection      |
                                                           bcmplpTimesyncSOPmemMsgType        |
                                                           bcmplpTimesyncSOPmemClassifiedData |
                                                           bcmplpTimesyncSOPmemVlanId         |
                                                           bcmplpTimesyncSOPmemValid          |
                                                           bcmplpTimesyncSOPmemTimestamp };
    /* 'lookup_key_mask' tells what fields to retrieve from SOPmem.                     *\
    |*  If the corresponding field in 'sop' struct is not UNKNOWN, it will become a key *|
    \*  to lookup the SOPmem and the 1st valid entry matched will be returned.          */

    int  ii, rv = 0;
    struct timeval  start, end;     /* need #include <sys/time.h> to measure the elapsed time*/

    gettimeofday(&start, NULL);   /* start the timer */
    rv |= bcm_plp_timesync_sopmem_get(chipname, pinfo, flags, entry_id, &sop);   /* lookup SOPmem  */
    gettimeofday(&end  , NULL);   /*  stop the timer for measuring the elapsed time  */

    if ( (0 == rv) && (0U != sop.valid)) {   /* print the successfully read valid entry */
        printf("SOPmem[%2d.%02x %02d] ", pinfo.phy_addr, pinfo.lane_map, entry_id);
      #ifdef  BCM_PLP_TIMESYNC_V2_1_SUPPORT
        printf("v=%x dir=%d type=%d seqId=0x%x dn=0x%x "
               "SrcPort=0x%02x%02x_%02x%02x_%02x%02x_%02x%02x_%02x%02x TS=0x",
               sop.valid, sop.direction, sop.msg_type, sop.seq_id, sop.domain_num,
                          sop.src_port_id[9], sop.src_port_id[8], sop.src_port_id[7],
                          sop.src_port_id[6], sop.src_port_id[5], sop.src_port_id[4],
                          sop.src_port_id[3], sop.src_port_id[2], sop.src_port_id[1], sop.src_port_id[0]);
      #else
        printf("v=%x dir=%d type=%d seqId=0x%x dn=0x%x vid=0x%x ad=0x%x.%x.%x.%x.%x.%x.%x.%x TS=0x",
               sop.valid, sop.direction, sop.msg_type, sop.seq_id, sop.domain_num, sop.vlan_id,
                          sop.classified_data[7], sop.classified_data[6], sop.classified_data[5],
                          sop.classified_data[4], sop.classified_data[3], sop.classified_data[2],
                          sop.classified_data[1], sop.classified_data[0]);
      #endif
        for ( ii = 9; ii >= 0; ii-- ) {    /* print the 10-octet timestamp */
            if ( NULL != tstmp ) {
                *tstmp <<= 8;
                *tstmp  |= (unsigned int) sop.timestamp[ii];
            }
            printf("%02x", sop.timestamp[ii]);
            if ( ii && (0x0 == (ii & 0x1)) )   printf("_");  /* separators at every 4th nibble */
        }
        printf("  (%ld us)\n", end.tv_usec - start.tv_usec);         /* print the elapsed time */
        rv = 1;   /* report that a valid entry has been found */ 
    }
    return  rv;
}

/* clear SOPmem  */
int ts_sopmem_clear(char *chipname, bcm_plp_access_t pinfo, unsigned int index) {
    /* bcm_plp_timesync_sopmem_get() does not accept NULL parameters */
    bcm_plp_timesync_sopmem_t  dummy = { 0 };
    int  rv = 0;
    unsigned int  flags = (index >= TIMESYNC_SOPMEM_MAX)    /* 32 SOPmem entries in total     */
                          ? bcmplpTimesyncSOPmemOpClearAll  /* delete all entries in SOPmem   */
                          : bcmplpTimesyncSOPmemOpClearOne; /* del the entry indexed by index */
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_sopmem_get(chipname, pinfo, flags, index, &dummy) );
    return  rv;
}

/* select what to be captured to SOPmem: SrcMAC/DstMAC/SrcIP/DstIP/SourcePortIdentity *\
\*                       see the ENUM definition of 'bcm_plp_timesync_inband_parse_t' */
int ts_sopmem_ip_mac_capture_set(char *chipname, bcm_plp_access_t  phy_info,
                                   bcm_plp_timesync_inband_parse_t ipmac_rx,
                                   bcm_plp_timesync_inband_parse_t ipmac_tx) {
    int  rv = 0;
    bcm_plp_timesync_config_t  cfg;

    rv |= bcm_plp_timesync_config_get(chipname, phy_info, &cfg);
    cfg.rx_inband_prop.write_sopmem = cfg.rx_inband_prop.compare_sopmem =  ipmac_rx;
    cfg.tx_inband_prop.write_sopmem = cfg.tx_inband_prop.compare_sopmem =  ipmac_tx;

    rv |= bcm_plp_timesync_config_set(chipname, phy_info, &cfg);
    return  rv;
}

/* get the setting of what to be captured to SOPmem */
int ts_sopmem_ip_mac_capture_get(char *chipname, bcm_plp_access_t  phy_info,
                                  bcm_plp_timesync_inband_parse_t *ipmac_rx,
                                  bcm_plp_timesync_inband_parse_t *ipmac_tx) {
    int  rv = 0;
    bcm_plp_timesync_config_t  cfg;

    rv |= bcm_plp_timesync_config_get(chipname, phy_info, &cfg);
    *ipmac_rx =  cfg.rx_inband_prop.write_sopmem;
    *ipmac_tx =  cfg.tx_inband_prop.write_sopmem;

    return  rv;
}


/*******************************************************************************************************\
 *   Internal (DPLL) / External (PTP_CLK/PHY_REF)  Reference Clocks settings
\*******************************************************************************************************/

/* NSE timer clock setting */
static int  _ts_nse_timer_set(char *chipname, bcm_plp_access_t pinfo, int freq) {
    bcm_plp_timesync_framesync_mode_t  mode_resume;
    bcm_plp_timesync_framesync_t       synmode;
    bcm_plp_timesync_time_value_t      tvalue;
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* NTP Frequency control word  (1588reg.a9-a8)    */
    tvalue.type       = bcmplpTimesyncTimerTypeNtpFreqCtrl;
    tvalue.time_value = (125 == freq) ? 0x00089706LL :
                        (136 == freq) ? 0x0007DA89LL : 0x00085463LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &tvalue) );
    /* NTP Down Counter control word  (1588reg.ae-af) */
    tvalue.type       = bcmplpTimesyncTimerTypeNTPDownCntr;
    tvalue.time_value = 0x001DCD65LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &tvalue) );
    /* NCO Frequency Stepping control  (1588reg.2e-2f) */
    tvalue.type       = bcmplpTimesyncTimerTypeNcoFreqStep;
    tvalue.time_value = (125 == freq) ? 0x80000000LL :
                        (136 == freq) ? 0x75075075LL : 0x7C1F07C2LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &tvalue) );
    /* DPLL initial loopfilter  (1588reg.29-2c)       */
    tvalue.type       = bcmplpTimesyncTimerTypeDpllInitLpFlt;
    tvalue.time_value = (125 == freq) ? 0x8000000000000000LL :
                        (136 == freq) ? 0x7507507507507507LL : 0x7c1F07C1F07C1F08LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &tvalue) );

  #if __P1588_NSE_DPLL_SEQUENCE_OLD__
    /* Soft reset for 1588 timer block */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, bcmplpTimesyncEnCtrlFlag,
                                                               bcmplpTimesyncEnCtrlSwRstbNse) );
  #endif

    /* (1588reg.14/15=0x0020) shadow load control -- DPLL loop filter */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_load_ctrl_set(chipname, pinfo, 0, bcmplpTimesyncLoadCtrlDpllLoopFilter,
                                                                     bcmplpTimesyncLoadCtrlDpllLoopFilter) );
    /* set SyncIn/FrameSync mode to trigger one FrameSync by CPU */
            rv |= bcm_plp_timesync_framesync_mode_get(chipname, pinfo, 0, &synmode);
    mode_resume       =  synmode.mode;
    if ( (mode_resume < bcmplpTimesyncFramsyncModeNone) ||
         (mode_resume > bcmplpTimesyncFramsyncModeCpu) ) {
          mode_resume = bcmplpTimesyncFramsyncModeNone;
    }
    if ( (synmode.syncout_mode < bcmplpTimesyncSyncoutModeDisable) ||
         (synmode.syncout_mode > bcmplpTimesyncSyncoutModePulseTrainSync) ) {
          synmode.syncout_mode = bcmplpTimesyncSyncoutModeDisable;          /* SyncOut mode */
    }
  #if ! defined(PLP_APERTA_SUPPORT)     /* capture TimeStamp on the next FrameSync event    */
    synmode.flags     =  bcmplpTimesyncSyncmodeCtrlTsCapture;
  #else                                 /* Aperta needs to set the "Initial NSE Block" too  */
    synmode.flags     =  bcmplpTimesyncSyncmodeCtrlTsCapture | bcmplpTimesyncSyncmodeCtrlNseInit;
  #endif
    synmode.gmode     =  bcmplpTimesyncGLobalModeCpu;       /* global mode -- CPU involved  */
    synmode.mode      =  bcmplpTimesyncFramsyncModeCpu;     /* enable CPU trigger FrameSync */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_framesync_mode_set(chipname, pinfo, 0, &synmode) );

    /* resume the mode to disable CPU trigger FrameSync */
    synmode.mode      =  mode_resume;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_framesync_mode_set(chipname, pinfo, 0, &synmode) );

    /* (1588reg.14/15=0x0020) shadow load control -- reset all */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_load_ctrl_set(chipname, pinfo, 0, 0, 0) );

    return  rv;
}

/*
 *   DPLL Clock Synchronization setting
 */

#define  TS_CHECK_TIMEVALUE(_rv, _p, _msg, _tv1, _tv2)     \
            do {    \
                if ( _tv1.time_value != _tv2.time_value )  \
                    printf("%s(%d) rv=%d p=[%x.0x%02x] %s W=0x%llx R=0x%llx\n", __func__,__LINE__, \
                           _rv, _p.phy_addr, _p.lane_map, _msg, _tv1.time_value, _tv2.time_value); \
            } while ( 0 )

/* configure the SyncIn for the inernal DPLL reference clock */
static int _dpll_for_syncin(char *chipname, bcm_plp_access_t pinfo,
                            int syncin_freq, unsigned long long dpll_ref_phase) {
    int  rv = 0;
    unsigned long long   dpll_ref_delta = 0LL, dpll_k3k2k1 = 0LL;
    bcm_plp_timesync_time_value_t  timevalue, timevalue_;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* [ set dpll_init_ref_phase phy,0) ]  1588reg.21-23 NSE DPLL Reference Phase */
    timevalue.type       = timevalue_.type  =  bcmplpTimesyncTimerTypeDpllRefPhase;
    timevalue.time_value = dpll_ref_phase & UINT64_LO48_MASK;  /* 48-bit DPLL ref phase */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue ) );
            rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue_  );
    TS_CHECK_TIMEVALUE(rv, pinfo, "DPLL_refPhase", timevalue, timevalue_);

    switch (syncin_freq) {
        case  1 :    /* 1 kHz SyncIn frequency */
            dpll_ref_delta = 0x0000000f4240LL;    dpll_k3k2k1 = 0x00000028002aLL;    break;
        case  4 :    /* 4 kHz SyncIn frequency */
            dpll_ref_delta = 0x00000003D090LL;    dpll_k3k2k1 = 0x00000026002aLL;    break;
        case  8 :    /* 8 kHz SyncIn frequency */
            dpll_ref_delta = 0x00000001E848LL;    dpll_k3k2k1 = 0x00000025002aLL;    break;
        case 19 :    /* 19.2 kHz SyncIn frequency */
            dpll_ref_delta = 0x00000000CB73LL;    dpll_k3k2k1 = 0x00000023002aLL;    break;
        default :  break;
    }

    /* [ set dpll_init_ref_phase_delta phy,0) ]  1588reg.24-25 NSE DPLL Reference Delta Phase */
    timevalue.type       = timevalue_.type =  bcmplpTimesyncTimerTypeDpllRefDelta;
    timevalue.time_value = dpll_ref_delta & UINT64_LO32_MASK;  /* 32-bit DPLL ref delta phase */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue ) );
            rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue_  );
    TS_CHECK_TIMEVALUE(rv, pinfo, "DPLL_refPhaseDELTA", timevalue, timevalue_);

    /* [ set dpll_k1_k3 phy,0) ]  1588reg.26-28 NSE DPLL K3/K2/K1 */
    timevalue.type       = timevalue_.type =  bcmplpTimesyncTimerTypeDpllK3K2K1;
    timevalue.time_value = dpll_k3k2k1 & UINT64_LO48_MASK;  /* 48-bit K3/K2/K1 */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue ) );
            rv |= bcm_plp_timesync_timing_control_get(chipname, pinfo, 0, &timevalue_  );
    TS_CHECK_TIMEVALUE(rv, pinfo, "k3k2k1", timevalue, timevalue_);

    return rv;
}

/*
 * TimeSync 1588 config sequence for setting
 * DPLL for PTP Clock  ( Section 6 in 1588-Gen2-ANxxx-RDS Application Note )
 * Imitate the VB script introduced in section "Synch Using DPLL & FrameSync"
 * of the app note :     P03_1588_SI_Freq_SO_FSync_Rd_HB.vbs
 *
 * ( see more details in Section 5.1 of IEEE1588 TimeSync Software User Guide )
 */
int ts_dpll_ptp_clock(char *chipname, bcm_plp_access_t pinfo, int txrx, int syncin_freq,
                             unsigned long long localtime_sec, unsigned int localtime_ns,
                                                               unsigned int pm1588_en) {
    bcm_plp_timesync_timespec_t  timecode = { FALSE, 0, 0 };
    bcm_plp_timesync_config_t    cfg  = { 0 }, cfg2 = { 0 };
    unsigned long long           dpll_ref_phase = 0LL;
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    rv |= _ts_nse_timer_set(chipname, pinfo, NSE_TIMER_FREQ);

  #if  defined(PLP_APERTA_SUPPORT)
    /* 1588reg.07 TXRX SOP TS CAPTURE ENABLE Register */
    memset(&cfg2, 0 , sizeof(bcm_plp_timesync_config_t));
    rv |= bcm_plp_timesync_config_get(chipname, pinfo, &cfg2);
    cfg2.sop_ts_cap.dp_ts_wclk  = bcmplpTimesyncTx;
    cfg2.sop_ts_cap.err_ecc2pkt = TRUE;
    cfg2.sop_ts_cap.lov         = TRUE;
    cfg2.sop_ts_cap.stamping    = 0;
    if ( pm1588_en ) {
        cfg2.sop_ts_cap.stamping |= (txrx & bcmplpTimesyncTx) ? bcmplpTimesyncStampingPmTx : 0;
        cfg2.sop_ts_cap.stamping |= (txrx & bcmplpTimesyncRx) ? bcmplpTimesyncStampingPmRx : 0;
    }
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, &cfg2) );
  #endif

    /* [ enable 1588_rxtx phy,0 ]  enable NSE Timer Clock  1588reg.00[2] */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo,
                                      bcmplpTimesyncEnCtrlNseTimerClkEn, TIMESYNC_ENABLE) );
    /* local time is just the initial DPLL reference phase */
    dpll_ref_phase = UINT64_SET(localtime_sec, localtime_ns);
    /* determine DPLL settings based on the SyncIn frequency */
    rv |= _dpll_for_syncin(chipname, pinfo, syncin_freq, dpll_ref_phase);

    /* [ set local_time phy,0 ]  (1588reg.30-32) 48-bit local timer */

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_local_time_set(chipname, pinfo, 0,
                                          UINT64_SET(localtime_sec, localtime_ns)) );
    /* [ set time_code phy,1 ]  (1588reg.0c-10) time code */
    timecode.seconds     = localtime_sec;
    timecode.nanoseconds = localtime_ns ;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_time_code_set(chipname, pinfo, 0, &timecode) );

    /* TimeSync configurations */
    rv |= bcm_plp_timesync_config_get(chipname, pinfo, &cfg);
    /* SOPmem capturing MAC/IP/SrcPortID setting */
    cfg.rx_inband_prop.compare_sopmem = addr_capture_rx;
    cfg.rx_inband_prop.write_sopmem   = addr_capture_rx;
    cfg.tx_inband_prop.compare_sopmem = addr_capture_tx;
    cfg.tx_inband_prop.write_sopmem   = addr_capture_tx;
    /* [ enable hb_capture phy,1 ]  (1588reg.20[13:12])  heartbeat capture mode */
    cfg.hb_capture_mode       = bcmplpTimesyncHeartbeatCaptureLc;
    /* [ set int_threshold phy,1 ]  threshold value to trigger the packet timestamp interrupt */
    cfg.fifo_level_intr_thold = 0x1;       /* 1588reg.b6 */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, &cfg ) );
            rv |= bcm_plp_timesync_config_get(chipname, pinfo, &cfg2  );
    TS_COMPARE(pinfo, &cfg, &cfg2, bcm_plp_timesync_config_t);

    /* [ enable interrupt phy ]  (1588reg.16) enable all interrupts */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_intr_enable_set( chipname, pinfo,
                                (bcmplpTimesyncIntrIpg | bcmplpTimesyncIntrSyncIn |
                                 bcmplpTimesyncIntrSop | bcmplpTimesyncIntrFsync  ),
                                TIMESYNC_ENABLE) );
    printf("rv=%d Setting %d kHz DPLL Sync for port [%2d.%02x]", rv, syncin_freq,
                                               pinfo.phy_addr , pinfo.lane_map);

    printf("\n");
    return rv;
}

/*
 * TimeSync 1588 config sequence for setting External Reference Clocks :
 *          to use [PTP_CLK+/-] or [PHY REF_CLK] to run NCO counter
 */
int ts_ext_ptp_ref_clock(char *chipname,  bcm_plp_access_t phy_info,
                                                       int ext_ptp_clk_src,
                         bcm_plp_timesync_framesync_mode_t framsync_src   ,
                                        unsigned long long local_time     ) {
    bcm_plp_timesync_config_t     tscfg;
    bcm_plp_timesync_framesync_t  framesync = { .mode = bcmplpTimesyncFramsyncModeNone };
    bcm_plp_timesync_timespec_t   timespec  = { 0, (local_time >> 32),
                                                   (local_time &  0xffffffff) };
    int  rv = 0;
    /* collect current TimeSync configurations */
    memset(&tscfg, 0, sizeof(bcm_plp_timesync_config_t));
    rv |= bcm_plp_timesync_config_get(    chipname, phy_info, &tscfg  );

    /* enable TimeStamp / Heartbeat / PTP message types / reference clock source     */
    tscfg.gmode      =  bcmplpTimesyncGLobalModeCpu;
    tscfg.flags      =  bcmplpTimesyncFlagCaptureStampEnable   |
                        bcmplpTimesyncFlagHeartbeatStampEnable |
                        bcmplpTimesyncFlag8021asEnable         |
                        bcmplpTimesyncFlagL2Enable             |
                        bcmplpTimesyncFlagIp4Enable            |
                        bcmplpTimesyncFlagIp6Enable            ;
    /* use external PTP clock source PTP_XTAL  (not DPLL via Sync_In) */
    tscfg.flags     |=  bcmplpTimesyncFlagClockSrcExt;

    if ( bcmplpTimesyncFlagClockSrcExtMode == ext_ptp_clk_src ) {
        /* use    PTP_CLK+/-   to run NCO counter  */
        tscfg.flags |=  bcmplpTimesyncFlagClockSrcExtMode;
    } else {
        /* use the PHY REF_CLK to run NCO counter  */
        tscfg.flags &= ~bcmplpTimesyncFlagClockSrcExtMode;
    }
    /* NSE DPLL 1 (1588reg.20[13:12]) heartbeat capture mode */   /* use time code captured */
        tscfg.hb_capture_mode = bcmplpTimesyncHeartbeatCaptureTc; /*     for read back      */

    /* SOPmem capturing MAC/IP/SrcPortID setting */
    tscfg.rx_inband_prop.compare_sopmem = addr_capture_rx;
    tscfg.rx_inband_prop.write_sopmem   = addr_capture_rx;
    tscfg.tx_inband_prop.compare_sopmem = addr_capture_tx;
    tscfg.tx_inband_prop.write_sopmem   = addr_capture_tx;
    /* set TimeSync configuration */
    rv |= bcm_plp_timesync_config_set(    chipname, phy_info, &tscfg);
    /* set ToD Time of Day  (48-bit Local Time and 80-bit Time code) */
    rv |= bcm_plp_timesync_local_time_set(chipname, phy_info, 0, local_time);  /* 48-bit */
    rv |= bcm_plp_timesync_time_code_set( chipname, phy_info, 0, &timespec );  /* 80-bit */
    /* enable TimeSync 1588 / FrameSync  */
    rv |= bcm_plp_timesync_enable_set(    chipname, phy_info, 0, TIMESYNC_ENABLE);

    /* modify FrameSync mode */
    rv |= bcm_plp_timesync_framesync_mode_get(chipname, phy_info, 0, &framesync);
    framesync.mode = framsync_src;     /* select Sync0 or Sync1 to trigger FrameSync  */
    framesync.syncout_mode =  bcmplpTimesyncSyncoutModeDisable;     /*   SyncOut mode */
    framesync.gmode        =  bcmplpTimesyncGLobalModeCpu;          /*    global mode */
    framesync.flags        =  bcmplpTimesyncSyncmodeCtrlTsCapture |
                              bcmplpTimesyncSyncmodeCtrlNseInit;
    rv |= bcm_plp_timesync_framesync_mode_set(chipname, phy_info, 0, &framesync);

    printf("---- Port %02X config: External Reference Clock "
           "(Src=%s  FrameSync=%s) --------\n",  phy_info.phy_addr,
           (     (tscfg.flags & bcmplpTimesyncFlagClockSrcExtMode) ? "PTP_XTAL" : "REF_CLK" ) ,
           ( (framesync.mode == bcmplpTimesyncFramsyncModeSyncIn0) ? "Sync0"    :
             (framesync.mode == bcmplpTimesyncFramsyncModeSyncIn1) ? "Sync1"    :
             (framesync.mode == bcmplpTimesyncFramsyncModeCpu    ) ? "CPU"      : "INVALID" ) );
    return rv;
}


/*******************************************************************************************************\
 *   setting TimeStamp T1/T2/T3/T4 & TC modes
\*******************************************************************************************************/

#if  defined(PLP_PM_TIMESTAMPING_SUPPORT)
/*
 *  TimeSync 1588 config sequence for TIMESTAMP T1/T2 & TC -- use PortMacro (PM) timestamping.
 */
int ts_pm_t12_tc(char *chipname, bcm_plp_access_t pinfo, int mode, int step,
                                             bcm_plp_timesync_config_t *cfg) {
    bcm_plp_timesync_timespec_t    timespec;
    bcm_plp_timesync_time_value_t  timevalue;
    bcm_plp_timesync_framesync_t   synmode;
    bcm_plp_mac_access_t           macinfo;
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);
    if ( step != 2 )    step = 1;       /* 1-step or 2-step operation */

    rv |= _ts_nse_timer_set(chipname, pinfo, NSE_TIMER_FREQ);    /* set NSE timer clock */

    /* (1588reg.14/15=0x0020) shadow load control */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_load_ctrl_set(chipname, pinfo, 0,
                                                 bcmplpTimesyncLoadCtrlDpllLoopFilter,
                                                 bcmplpTimesyncLoadCtrlDpllLoopFilter) );
    /* (1588reg.3a)  */
            rv |= bcm_plp_timesync_framesync_mode_get(chipname, pinfo, 0, &synmode);
    synmode.gmode        =  bcmplpTimesyncGLobalModeCpu;      /* global  mode */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_framesync_mode_set(chipname, pinfo, 0, &synmode) );
    synmode.mode         =  bcmplpTimesyncFramsyncModeCpu;    /* CPU trigger immediate for frame sync */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_framesync_mode_set(chipname, pinfo, 0, &synmode) );
    synmode.gmode        =  bcmplpTimesyncGLobalModeSyncIn;   /* SyncIn0 */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_framesync_mode_set(chipname, pinfo, 0, &synmode) );
    /* (1588reg.14/15=0x0000) shadow load control */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_load_ctrl_set(chipname, pinfo, 0, 0, 0) );
    /* enable TX 1588 1588reg.00[0,2]*/
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, TIMESYNC_DIRECTION_TXRX,
                                                               TIMESYNC_ENABLE) );
    /* 1588reg.08/09[3]  (tx/rx_option[3]) */
    cfg->tx_option.force_ts_to_sopmem = cfg->rx_option.force_ts_to_sopmem = FALSE;
    /* 1588reg.bc[11]=1 used nse48 bit*/
    cfg->inband_ctrl.timer_mode = bcmplpTimesyncTimerMode48Bit;

    /* Tx/Rx Control 1588reg.18/1a[3:0] */
    cfg->flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                 bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;
    /* 1588reg.ba[10,9,8]      = 0x0702  */
    cfg->rx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]  */
    cfg->rx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]   */
    cfg->rx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]   */
    cfg->rx_inband_prop.compare_sopmem = addr_capture_rx;   /* ibctrl2[14:12] */
    cfg->rx_inband_prop.write_sopmem   = addr_capture_rx;   /* ibctrl2[2:0]   */
    /* 1588reg.b8[10,9,8]      = 0x0702  */
    cfg->tx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]  */
    cfg->tx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]   */
    cfg->tx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]   */
    cfg->tx_inband_prop.compare_sopmem = addr_capture_tx;   /* ibctrl2[14:12] */
    cfg->tx_inband_prop.write_sopmem   = addr_capture_tx;   /* ibctrl2[2:0]   */
    if ( PM1588_MODE_T12 == mode ) {
        /* 1588reg.b9[10,9,4] = 0x0610 */
        cfg->rx_inband_prop.mdio_sopmem    = TRUE;  /* ibctrl1[10]  */
        cfg->rx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]   */
        cfg->rx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]   */
        /* 1588reg.b7[13,4] = 0x2010   */
        cfg->tx_inband_prop.clear_rsv012   = TRUE;  /* ibctrl1[13]  */
        cfg->tx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]   */
        /* Tx/Rx Resv0_ID  1588reg.b7/b9 [3:0] */
        cfg->inband_ctrl.resv0_id          = 0x0;   /* ibctrl1[3:0] Resv0_ID = 0x0 */
    } else {
        /* 1588reg.b9[11,7,4] = 0x089B */
        cfg->rx_inband_prop.strict         = TRUE;  /* ibctrl1[11]  */
        cfg->rx_inband_prop.tc_mode        = TRUE;  /* ibctrl1[7]   */
        cfg->rx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]   */
        /* 1588reg.b7[7,4] = 0x009B    */
        cfg->tx_inband_prop.tc_mode        = TRUE;  /* ibctrl1[7]   */
        cfg->tx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]   */
        /* Tx/Rx Resv0_ID  1588reg.b7/b9 [3:0] */
        cfg->inband_ctrl.resv0_id          = 0xB;   /* ibctrl1[3:0] Resv0_ID = 0xB */
    }
    /* threshold value to trigger the packet timestamp interrupt */
    cfg->fifo_level_intr_thold = 0x1;           /* 1588reg.b6   */
    /* 1588reg.50 = 0x00CC ,  count 1588 PTP Sync packets */
    cfg->tx_pkt_count_sel = cfg->rx_pkt_count_sel = bcmplpPktTypePtpSync;

    /* 1588reg.07 TXRX SOP TS CAPTURE ENABLE Register */
    cfg->sop_ts_cap.dp_ts_wclk  = bcmplpTimesyncTx;
    cfg->sop_ts_cap.err_ecc2pkt = TRUE;
    cfg->sop_ts_cap.stamping    = bcmplpTimesyncStampingPmTx;
    cfg->sop_ts_cap.lov         = TRUE;

    /* 1588reg.20[14:11] select nse_reg_ts_captime for read back */
    cfg->hb_capture_mode        = 0;

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );

    /* 1588reg.16 enable all interrupts */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_intr_enable_set( chipname, pinfo,
                                (bcmplpTimesyncIntrIpg | bcmplpTimesyncIntrSyncIn |
                                 bcmplpTimesyncIntrSop | bcmplpTimesyncIntrFsync  ),
                                TIMESYNC_ENABLE) );

    /* link delay  and  timestamp offset */
    timevalue.op         = bcmplpMathOperatorAdd;
    timevalue.type       = TIMESYNC_MESSAGE_TYPE_ALL;
    timevalue.direction  = TIMESYNC_DIRECTION_RX;
    /* 1588reg.05-06 link_delay, for updating CF correction field */
    timevalue.time_value = ( PM1588_MODE_T12 == mode ) ? 0x0aLL : 0x0;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_link_delay_set(      chipname, pinfo, 0, &timevalue) );
    /* 1588reg.0b/7b Rx timestamp offset */
    timevalue.time_value = 0x00LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timestamp_offset_set(chipname, pinfo, 0, &timevalue) );
    /* 1588reg.0a/7b Tx timestamp offset */
    timevalue.direction  = TIMESYNC_DIRECTION_TX;
    timevalue.time_value = 0x00LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timestamp_offset_set(chipname, pinfo, 0, &timevalue) );

    /* NSE DPLL initial reference phase  (1588reg.0x21-23) */
    timevalue.type       = bcmplpTimesyncTimerTypeDpllRefPhase;
    timevalue.time_value = 0x00LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue) );
    /* NSE DPLL initial reference delta phase  (1588reg.0x24-25) */
    timevalue.type       = bcmplpTimesyncTimerTypeDpllRefDelta;
    timevalue.time_value = 0x0001E848LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue) );
    /* NSE DPLL K3 / K2 / K1  (1588reg.0x28-26) */
    timevalue.type       = bcmplpTimesyncTimerTypeDpllK3K2K1;
    timevalue.time_value = 0x00000025002ALL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue) );
    /* NCO sync value of 48-bit local timer  (1588reg.0x30-32) */
    timevalue.type       = bcmplpTimesyncTimerTypeSyncLocalTimer;
    timevalue.time_value = 0x00LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue) );

    /* Original time code value for egress PTP messages  (1588reg.0x0c-10) */
    timespec.seconds     = timespec.nanoseconds = 0x0L ;
    timespec.isnegative  = FALSE;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_time_code_set(chipname, pinfo, 0, &timespec) );

    /* NSE counter offset bit[63-00]  (1588reg.0xf0-ef) */
    timevalue.type       = bcmplpTimesyncTimerTypeNseCtrOffsetHi;
    timevalue.time_value = 0x000D0000LL;
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timing_control_set(chipname, pinfo, 0, &timevalue) );

    /* enable PortMacro (PM) timestamping for PM TC/t12 modes */
    memcpy(&(macinfo.phy_info), &pinfo, sizeof(bcm_plp_access_t));
    rv |= bcm_plp_pm_timesync_enable_set(chipname, macinfo,  (2 == step) ? 0x1 : 0x5,
                                             (NSE1588_MODE_TC == mode) ? FALSE : TRUE);
    printf("PortMacro TimeSync: p=[%d.0x%02x] mode=%d %d-step\n",
                                pinfo.phy_addr, pinfo.lane_map, mode, step);
    if ( rv ) {
        printf(".... enable failed !!  rv=%d\n", rv);
    }

    return rv;
}

#endif   /*  PLP_PM_TIMESTAMPING_SUPPORT  */

/*
 * TimeSync 1588 config sequence for
 * TIMESTAMP T1  ( Section 4: Bring-Up Sequence in 1588-Gen2-ANxxx-RDS Application Note )
 * Imitate the VB scripts introduced in section "Timestamp T1" of the app note :
 *     1-step:    P03_ibts_gm_tx_sync_t1.vbs     &  P03_no_inband_gm_tx_sync_t1.vbs
 *     2-step:    P03_ibts_gm_tx_sync_fu_t1.vbs  &  P03_no_inband_gm_tx_sync_fu_t1.vbs
 *
 * ( see more details in Section 4.1 of IEEE1588 TimeSync Software User Guide )
 */
int ts_t1(char *chipname, bcm_plp_access_t pinfo, int txrx, int step, int band,
                  unsigned int timer_precision, bcm_plp_timesync_config_t *cfg) {
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* [ enable 1588_rxtx phy,2 ]  enable TX 1588 1588reg.00[0,2]*/
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, txrx, TIMESYNC_ENABLE) );

    /* [ enable 1588_rxtx_option phy,0 ]  1588reg.08/09[3] */
    cfg->tx_option.force_ts_to_sopmem = cfg->rx_option.force_ts_to_sopmem = TRUE;
    /* [ enable 1588_rxtx_control phy ] Tx/Rx Control 1588reg.18/1a[3:0] */
    cfg->flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                 bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;
    /* [ enable compute_cf_ts phy, 0 ]  1588reg.bc[11]=0 used nse80 bit*/
    cfg->inband_ctrl.timer_mode = timer_precision;

    /* Inband Tx Control 1/2 registers */
    if ( TIMESYNC_INBAND == band ) {
        /* [ enable ibts_rxtx_gm_sync phy (TX) ]  1588reg.b7[13,11,9,5]  */
        cfg->tx_inband_prop.clear_rsv012   = TRUE;  /* ibctrl1[13] */
        cfg->tx_inband_prop.strict         = TRUE;  /* ibctrl1[11] */
        cfg->tx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
        cfg->tx_inband_prop.check_resv0    = TRUE;  /* ibctrl1[5]  */
    } else {  /* TIMESYNC_OUT_OF_BAND */
        /* [ enable ibts_rxtx_gm_sync phy (TX) ]  1588reg.b7[13,9,4]     */
        cfg->tx_inband_prop.clear_rsv012   = TRUE;  /* ibctrl1[13] */
        /* cfg->tx_inband_prop.mdio_sopmem = TRUE; */  /* ibctrl1[10] */  /* optional */
        cfg->tx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
        cfg->tx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    }
    /* [ enable ibts_rxtx_gm_sync phy (TX) ]      1588reg.b8[10,9,8]  */
    cfg->tx_inband_prop.cmp_field_sel      = TRUE;  /* ibctr12[11]    */
    cfg->tx_inband_prop.cmp_src_port       = TRUE;  /* ibctr12[10]    */
    cfg->tx_inband_prop.cmp_seq_num        = TRUE;  /* ibctrl2[9]     */
    cfg->tx_inband_prop.cmp_domain_num     = TRUE;  /* ibctrl2[8]     */
    cfg->tx_inband_prop.compare_sopmem     = addr_capture_tx;   /* ibctrl2[14:12] */
    cfg->tx_inband_prop.write_sopmem       = addr_capture_tx;   /* ibctrl2[2:0]   */
        /* compare VLAN ID if two-step Sync */       /* ibctrl2[15]    */
    cfg->tx_inband_prop.cmp_vlan_id = (TIMESYNC_T1_TWO_STEP == step) ? TRUE : FALSE;

    /* Inband Rx Control 1/2 registers */
    if ( TIMESYNC_INBAND == band ) {
        /* [ enable ibts_rxtx_gm_sync phy (RX) ]  1588reg.b9[11,9,6,4] */
        cfg->rx_inband_prop.strict         = TRUE;  /* ibctrl1[11] */
        cfg->rx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
        cfg->rx_inband_prop.update_resv0   = TRUE;  /* ibctrl1[6]  */
        cfg->rx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    } else {  /* TIMESYNC_OUT_OF_BAND */
        /* [ enable ibts_rxtx_gm_sync phy (RX) ]  1588reg.b9[11,9,4]   */
        cfg->rx_inband_prop.strict         = TRUE;  /* ibctrl1[11] */
        cfg->rx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
        cfg->rx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    }
    /* [ enable ibts_rxtx_gm_sync phy (RX) ]      1588reg.ba[10,9,8] */
    cfg->rx_inband_prop.cmp_field_sel      = TRUE;  /* ibctr12[11]    */
    cfg->rx_inband_prop.cmp_src_port       = TRUE;  /* ibctr12[10]    */
    cfg->rx_inband_prop.cmp_seq_num        = TRUE;  /* ibctrl2[9]     */
    cfg->rx_inband_prop.cmp_domain_num     = TRUE;  /* ibctrl2[8]     */

    cfg->rx_inband_prop.compare_sopmem     = addr_capture_rx;   /* ibctrl2[14:12] */
    cfg->rx_inband_prop.write_sopmem       = addr_capture_rx;   /* ibctrl2[2:0]   */
        /* compare VLAN ID if two-step Sync */       /* ibctrl2[15]    */
    cfg->rx_inband_prop.cmp_vlan_id = (TIMESYNC_T1_TWO_STEP == step) ? TRUE : FALSE;
        /* Tx/Rx Resv0_ID  1588reg.b7/b9 [3:0] */
    if ( TIMESYNC_INBAND == band ) {
        cfg->inband_ctrl.resv0_id          = 0xb;   /* ibctrl1[3:0] Resv0_ID = 0xb */
    } else {
        cfg->inband_ctrl.resv0_id          = 0x0;
    }
    /* [ set int_threshold phy,1 ]  threshold value to trigger the packet timestamp interrupt */
    cfg->fifo_level_intr_thold = 0x1;       /* 1588reg.b6 */

    /* [ enable txrx_count_packet phy,2]  (1588reg.50) count CRC and PTP Sync packets */
    cfg->tx_pkt_count_sel =
    cfg->rx_pkt_count_sel = bcmplpPktTypePtpSync;

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );

    /* [ enable interrupt phy ]  (1588reg.16) enable all interrupts */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_intr_enable_set( chipname, pinfo,
                                (bcmplpTimesyncIntrIpg | bcmplpTimesyncIntrSyncIn |
                                 bcmplpTimesyncIntrSop | bcmplpTimesyncIntrFsync  ),
                                TIMESYNC_ENABLE) );
    return rv;
}

/*
 * TimeSync 1588 config sequence for
 * TIMESTAMP T2  ( Section 4: Bring-Up Sequence in 1588-Gen2-ANxxx-RDS Application Note )
 * Imitate the VB script introduced in section "Timestamp T2" of the app note :
 *     P03_ibts_sc_rx_sync_t2.vbs  &  P03_no_inband_sc_rx_sync_t2.vbs
 *
 * ( see more details in Section 4.2 of IEEE1588 TimeSync Software User Guide )
 */
int ts_t2(char *chipname, bcm_plp_access_t pinfo, int txrx, int band,
          unsigned int timer_precision, bcm_plp_timesync_config_t *cfg) {
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* [ enable 1588_rxtx phy,1 ]  enable TX 1588 1588reg.00[1,2]*/
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, txrx, TIMESYNC_ENABLE) );

    /* [ enable 1588_rxtx_option phy,0 ] 1588reg.08/09[3]  (tx/rx_option[3]) */
    cfg->tx_option.force_ts_to_sopmem = cfg->rx_option.force_ts_to_sopmem = TRUE;
    /* [ enable 1588_rxtx_control phy ] Tx/Rx Control 1588reg.18/1a[3:0] */
    cfg->flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                 bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;
    /* [ enable compute_cf_ts phy, 0 ] 1588reg.bc[11]=0 used nse80 bit*/
    cfg->inband_ctrl.timer_mode = timer_precision;

    /* [ enable ibts_rx_sc_sync,0 (TX) ]  1588reg.b7[13,12,9,5,4]  1588reg.b8[10,9,8] *\
    \*                              original VB script put "phy,1" is wrong !         */
    /* Inband Tx Control 1/2 registers */
    if ( TIMESYNC_INBAND == band ) {
        /* [ enable ibts_rx_sc_sync (TX) ]  1588reg.b7[13,12,9,5,4] = 0x323B  */
        cfg->tx_inband_prop.clear_rsv012   = TRUE;  /* ibctrl1[13] */
        cfg->tx_inband_prop.ts32_format    = TRUE;  /* ibctrl1[12] */
        cfg->tx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
        cfg->tx_inband_prop.check_resv0    = TRUE;  /* ibctrl1[5]  */
        cfg->tx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    } else {  /* TIMESYNC_OUT_OF_BAND */
        /* [ enable ibts_rx_sc_sync (TX) ]  1588reg.b7[12,10,9,4]   = 0x1610  */
        cfg->tx_inband_prop.ts32_format    = TRUE;  /* ibctrl1[12] */
        cfg->tx_inband_prop.mdio_sopmem    = TRUE;  /* ibctrl1[10] */
        cfg->tx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
        cfg->tx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    }
    /* [ enable ibts_rx_sc_sync (TX) ]      1588reg.b8[10,9,8]      = 0x0702  */
    cfg->tx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]  */
    cfg->tx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]   */
    cfg->tx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]   */
    cfg->tx_inband_prop.compare_sopmem = addr_capture_tx;   /* ibctrl2[14:12] */
    cfg->tx_inband_prop.write_sopmem   = addr_capture_tx;   /* ibctrl2[2:0]   */

    /* [ enable ibts_rxtx phy,0 (RX) ]  1588reg.b9[12,9,6,4]  1588reg.ba[10,9,8]    *\
    \*                              original VB script put "phy,2" is wrong !       */
    /* Inband Rx Control 1/2 registers */
    if ( TIMESYNC_INBAND == band ) {
        /* [ enable ibts_rx_sc_sync (RX) ]  1588reg.b9[2,9,6,4]     = 0x125B  */
        cfg->rx_inband_prop.ts32_format    = TRUE;  /* ibctrl1[12] */
        cfg->rx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
        cfg->rx_inband_prop.update_resv0   = TRUE;  /* ibctrl1[6]  */
        cfg->rx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    } else {  /* TIMESYNC_OUT_OF_BAND */
        /* [ enable ibts_rx_sc_sync (RX) ]  1588reg.b9[12,10,9,4]   = 0x1610  */
        cfg->rx_inband_prop.ts32_format    = TRUE;  /* ibctrl1[12] */
        cfg->rx_inband_prop.mdio_sopmem    = TRUE;  /* ibctrl1[10] */
        cfg->rx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
        cfg->rx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    }
    /* [ enable ibts_rx_sc_sync (RX) ]      1588reg.ba[10,9,8]      = 0x0702  */
    cfg->rx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]  */
    cfg->rx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]   */
    cfg->rx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]   */
    cfg->rx_inband_prop.compare_sopmem = addr_capture_rx;   /* ibctrl2[14:12] */
    cfg->rx_inband_prop.write_sopmem   = addr_capture_rx;   /* ibctrl2[2:0]   */
        /* Tx/Rx Resv0_ID  1588reg.b7/b9 [3:0] */
    cfg->inband_ctrl.resv0_id               = 0xb;   /* ibctrl1[3:0] Resv0_ID = 0xB */

    /* [ set int_threshold phy,1 ]  threshold value to trigger the packet timestamp interrupt */
    cfg->fifo_level_intr_thold = 0x1;       /* 1588reg.b6 */

    /* [ enable txrx_count_packet phy,2]  (1588reg.50) count CRC and PTP Sync packets */
    cfg->tx_pkt_count_sel =
    cfg->rx_pkt_count_sel = bcmplpPktTypePtpSync;

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );

    /* [ enable interrupt phy ]  (1588reg.16) enable all interrupts */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_intr_enable_set( chipname, pinfo,
                                (bcmplpTimesyncIntrIpg | bcmplpTimesyncIntrSyncIn |
                                 bcmplpTimesyncIntrSop | bcmplpTimesyncIntrFsync  ),
                                TIMESYNC_ENABLE) );
    return rv;
}

/*
 * TimeSync 1588 config sequence for Inband operational
 * TIMESTAMP T3/T4  ( Section 4: Bring-Up Sequence in 1588-Gen2-ANxxx-RDS Application Note )
 * Imitate the VB scripts introduced in section "Timestamp T3" & "T4" of the app note :
 *     P03_ibts_sc_dreq_dresp_t3.vbs  &  P03_ibts_gm_rx_dreq_t4.vbs
 *
 * ( see more details in Section 4.3/4.4 of IEEE1588 TimeSync Software User Guide )
 */
int ts_t3_t4(char *chipname, bcm_plp_access_t pinfo, int txrx,
             unsigned int timer_precision, bcm_plp_timesync_config_t *cfg) {
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* [ enable/disable 1588_rxtx phy,1 ]  enable TX 1588 1588reg.00[0,2]*/
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, txrx, TIMESYNC_ENABLE) );

    /* [ enable 1588_rxtx_option phy,0 ] 1588reg.08/09[3]  (tx/rx_option[3]) */
    cfg->tx_option.force_ts_to_sopmem = cfg->rx_option.force_ts_to_sopmem = TRUE;
    /* [ enable 1588_rxtx_control phy ] Tx/Rx Control 1588reg.18/1a[3:0] */
    cfg->flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                 bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;
    /* [ enable compute_cf_ts phy, 0 ] 1588reg.bc[11]=0 used nse80 bit*/
    cfg->inband_ctrl.timer_mode = timer_precision;

    /* [ enable ibts_rxtx phy,1 (TX) ]  1588reg.b7[13,11,9,5,4]  1588reg.b8[10,9,8] */
    cfg->tx_inband_prop.clear_rsv012   = TRUE;  /* ibctrl1[13] */
    cfg->tx_inband_prop.strict         = TRUE;  /* ibctrl1[11] */
    cfg->tx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
    cfg->tx_inband_prop.check_resv0    = TRUE;  /* ibctrl1[5]  */
    cfg->tx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    cfg->tx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]    */
    cfg->tx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]     */
    cfg->tx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]     */
    cfg->tx_inband_prop.compare_sopmem = addr_capture_tx;   /* ibctrl2[14:12] */
    cfg->tx_inband_prop.write_sopmem   = addr_capture_tx;   /* ibctrl2[2:0]   */

    /* [ enable ibts_rxtx phy,1 (RX) ]  1588reg.b9[11,9,6,4]  1588reg.ba[10,9,8] */
    cfg->rx_inband_prop.strict         = TRUE;  /* ibctrl1[11] */
    cfg->rx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
    cfg->rx_inband_prop.update_resv0   = TRUE;  /* ibctrl1[6]  */
    cfg->rx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    cfg->rx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]    */
    cfg->rx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]     */
    cfg->rx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]     */
    cfg->rx_inband_prop.compare_sopmem = addr_capture_rx;   /* ibctrl2[14:12] */
    cfg->rx_inband_prop.write_sopmem   = addr_capture_rx;   /* ibctrl2[2:0]   */

        /* Tx/Rx Resv0_ID  1588reg.b7/b9 [3:0] */
    cfg->inband_ctrl.resv0_id               = 0xb;   /* ibctrl1[3:0] Resv0_ID = 0xB */

    /* [ set int_threshold phy,1 ]  threshold value to trigger the packet timestamp interrupt */
    cfg->fifo_level_intr_thold = 0x1;       /* 1588reg.b6 */

    /* [ enable txrx_count_packet phy,3]  (1588reg.50) count CRC and PTP Delay_Req packets */
    cfg->tx_pkt_count_sel =
    cfg->rx_pkt_count_sel = bcmplpPktTypePtpDelayReq;

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );

    /* [ enable interrupt phy ]  (1588reg.16) enable FrameSync & Packet Timestamp interrupts */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_intr_enable_set( chipname, pinfo,
                                (bcmplpTimesyncIntrIpg | bcmplpTimesyncIntrSyncIn |
                                 bcmplpTimesyncIntrSop | bcmplpTimesyncIntrFsync  ),
                                TIMESYNC_ENABLE) );
    return rv;
}


/*
 * TimeSync 1588 config sequence for Out-of-band operational
 * TIMESTAMP T3  ( Section 4: Bring-Up Sequence in 1588-Gen2-ANxxx-RDS Application Note )
 * Imitate the VB scripts introduced in section "Timestamp T3" & "T4" of the app note :
 *     P03_no_inband_sc_dreq_t3.vbs
 *
 * ( see more details in Section 4.3 of IEEE1588 TimeSync Software User Guide )
 */
int ts_t3_oob(char *chipname, bcm_plp_access_t pinfo, int txrx,
              unsigned int timer_precision, bcm_plp_timesync_config_t *cfg) {
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* [ enable/disable 1588_rxtx phy,1 ]  enable TX 1588 1588reg.00[0,2]*/
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, txrx, TIMESYNC_ENABLE) );

    /* [ enable 1588_rxtx_control phy ] Tx/Rx Control 1588reg.18/1a[3:0] */
    cfg->flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                 bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;

    /* [ enable ibts_tx_ib36 (TX) ]  1588reg.b7[13,9,4]=0x223b  1588reg.b8[13,12,10,9,8,3]=0x3708 */
    cfg->tx_inband_prop.clear_rsv012   = TRUE;  /* ibctrl1[13] */
    cfg->tx_inband_prop.ts_80bits      = TRUE;  /* ibctrl1[9]  */
    cfg->tx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    cfg->tx_inband_prop.compare_sopmem = addr_capture_tx;     /* ibctr12[13:12] */
    cfg->tx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]    */
    cfg->tx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]     */
    cfg->tx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]     */
    cfg->tx_inband_prop.write_sopmem   = addr_capture_tx;     /* ibctrl2[2:0]   */

    /* [ enable ibts_rx_ib36 (RX) ]  1588reg.b9[11,10,4]=0x0a5b  1588reg.ba[13,12,10,9,8,3]=0x3708 */
    cfg->rx_inband_prop.strict         = TRUE;  /* ibctrl1[11] */
    cfg->rx_inband_prop.mdio_sopmem    = TRUE;  /* ibctrl1[10] */
    cfg->rx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    cfg->rx_inband_prop.compare_sopmem = addr_capture_rx;     /* ibctr12[13:12] */
    cfg->rx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]    */
    cfg->rx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]     */
    cfg->rx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]     */
    cfg->rx_inband_prop.write_sopmem   = addr_capture_rx;     /* ibctrl2[2:0]   */

    /* [ enable 1588_tx/rx_option_gen2 ] 1588reg.08/09[3]  (tx/rx_option[3]) */
    cfg->tx_option.force_ts_to_sopmem = cfg->rx_option.force_ts_to_sopmem = TRUE;

    /* [ enable compute_cf_ts80 phy, 0 ]  1588reg.bc[11]=0 used nse80 bit*/
    cfg->inband_ctrl.timer_mode = timer_precision;
    /* [ enable txrx_count_all_1588_DReq ]  (1588reg.50) count CRC and PTP Delay_Req packets */
    cfg->tx_pkt_count_sel =
    cfg->rx_pkt_count_sel = bcmplpPktTypePtpDelayReq;

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );
    return rv;
}

/*
 * TimeSync 1588 config sequence for Out-of-band operational
 * TIMESTAMP T4  ( Section 4: Bring-Up Sequence in 1588-Gen2-ANxxx-RDS Application Note )
 * Imitate the VB scripts introduced in section "Timestamp T3" & "T4" of the app note :
 *     P03_no_inband_gm_rx_dreq_t4.vbs
 *
 * ( see more details in Section 4.4 of IEEE1588 TimeSync Software User Guide )
 */
int ts_t4_oob(char *chipname, bcm_plp_access_t pinfo, int txrx,
              unsigned int timer_precision, bcm_plp_timesync_config_t *cfg) {
    int  rv = 0;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* [ enable/disable 1588_rxtx phy,1 ]  enable TX 1588 1588reg.00[0,2]*/
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo,
                                      txrx, TIMESYNC_ENABLE) );
    /* [ enable 1588_rxtx_control phy ] Tx/Rx Control 1588reg.18/1a[3:0] */
    cfg->flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                 bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;

    /* [ set inband_tx_cntl (TX) ]  1588reg.b7[10,4]=0x047b  1588reg.b8[10,9,8,1]=0x0702 */
    cfg->tx_inband_prop.mdio_sopmem    = TRUE;  /* ibctrl1[10] */
    cfg->tx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    cfg->tx_inband_prop.compare_sopmem = addr_capture_tx;   /* ibctrl2[14:12] */
    cfg->tx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]    */
    cfg->tx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]     */
    cfg->tx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]     */
    cfg->tx_inband_prop.write_sopmem   = addr_capture_tx;   /* ibctrl2[2:0]   */

    /* [ set inband_rx_cntl (RX) ]  1588reg.b9[10,4]=0x047b  1588reg.ba[10,9,8,1]=0x0702 */
    cfg->rx_inband_prop.mdio_sopmem    = TRUE;  /* ibctrl1[10] */
    cfg->rx_inband_prop.inband_on      = TRUE;  /* ibctrl1[4]  */
    cfg->rx_inband_prop.compare_sopmem = addr_capture_rx;   /* ibctrl2[14:12] */
    cfg->rx_inband_prop.cmp_src_port   = TRUE;  /* ibctr12[10]    */
    cfg->rx_inband_prop.cmp_seq_num    = TRUE;  /* ibctrl2[9]     */
    cfg->rx_inband_prop.cmp_domain_num = TRUE;  /* ibctrl2[8]     */
    cfg->rx_inband_prop.write_sopmem   = addr_capture_rx;   /* ibctrl2[2:0]   */

    /* [ enable 1588_tx/rx_option_gen2 ] 1588reg.08/09[3]  (tx/rx_option[3]) */
    cfg->tx_option.force_ts_to_sopmem = cfg->rx_option.force_ts_to_sopmem = TRUE;

    /* [ enable compute_cf_ts80 phy, 0 ]  1588reg.bc[11]=0 used nse80 bit*/
    cfg->inband_ctrl.timer_mode = timer_precision;
    /* [ enable txrx_count_all_1588_DReq ]  (1588reg.50) count CRC and PTP Delay_Req packets */
    cfg->tx_pkt_count_sel =
    cfg->rx_pkt_count_sel = bcmplpPktTypePtpDelayReq;

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );
    return rv;
}

/*
 * TimeSync 1588 config sequence for
 * Regular TC Mode  ( Section 4: Bring-Up Sequence in 1588-Gen2-ANxxx-RDS Application Note )
 * Imitate the VB script introduced in section "Regular TC Mode" of the app note :
 *     P03_ibts_tc_sync.vbs
 *
 * ( see more details in Section 4.5 of IEEE1588 TimeSync Software User Guide )
 */
int ts_tc(char *chipname, bcm_plp_access_t pinfo, int txrx,
                unsigned int timer_precision    , long long link_delay,
                long long    ts_offset_rx       , long long ts_offset_tx ,
                unsigned long long localtime_sec, unsigned int  localtime_ns,
                 bcm_plp_timesync_config_t *cfg) {
    int  rv = 0;
    bcm_plp_timesync_time_value_t  timevalue;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);
    memset (cfg, 0, sizeof(bcm_plp_timesync_config_t));

    /* [ enable 1588_rxtx phy,2 ]  enable TX 1588 1588reg.00[0,2] */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, txrx, TIMESYNC_ENABLE) );
    /* TimeSync configurations */
            rv |= bcm_plp_timesync_config_get(chipname, pinfo, cfg);

    /* [ enable 1588_rxtx_option phy,0 ] 1588reg.08/09[3] */
    cfg->tx_option.force_ts_to_sopmem = cfg->rx_option.force_ts_to_sopmem = TRUE;
    /* [ enable 1588_rxtx_control phy ] Tx/Rx Control 1588reg.18/1a[3:0] */
    cfg->flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                 bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;
    /* [ enable compute_cf_ts phy, 1 ] 1588reg.bc[11]=1 used nse48 bit*/
    cfg->inband_ctrl.timer_mode = timer_precision;

    /* - - subroutines of [ enable ibts_rxtx_tc ] - - - - - - - - - - - - - - - - */
    timevalue.type       =  TIMESYNC_MESSAGE_TYPE_ALL;

    /* [ set link_delay phy,0 ]  (1588reg.05-06) link_delay, for updating CF correction field */
    timevalue.direction  =  txrx;
    timevalue.op         =  (link_delay  >=  0) ? bcmplpMathOperatorAdd
                                                : bcmplpMathOperatorSubtract;
    timevalue.time_value =  llabs(link_delay);
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_link_delay_set(      chipname, pinfo, 0, &timevalue) );
    /* [ set ts_offset_rx  phy,0 ]  (1588reg.0b/7b) Rx timestamp offset */
    timevalue.direction  =  bcmplpTrafficRx;
    timevalue.op         =  (ts_offset_rx >= 0) ? bcmplpMathOperatorAdd
                                                : bcmplpMathOperatorSubtract;
    timevalue.time_value =  llabs(ts_offset_rx);
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timestamp_offset_set(chipname, pinfo, 0, &timevalue) );
    /* [ set ts_offset_tx  phy,0 ]  (1588reg.0a/7b) Tx timestamp offset */
    timevalue.direction  =  bcmplpTrafficTx;
    timevalue.op         =  (ts_offset_tx >= 0) ? bcmplpMathOperatorAdd
                                                : bcmplpMathOperatorSubtract;
    timevalue.time_value =  llabs(ts_offset_tx);
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timestamp_offset_set(chipname, pinfo, 0, &timevalue) );

    /* [ enable ibts_rxtx_tc phy,? (TX) ]  1588reg.b7[7,4]=0x009b     1588reg.b8[10,9,8]=0x0702 */
    cfg->tx_inband_prop.partial_tc_mode = FALSE; /* ibctrl1[8] off */
    cfg->tx_inband_prop.tc_mode         = TRUE;  /* ibctrl1[7]  */
    cfg->tx_inband_prop.inband_on       = TRUE;  /* ibctrl1[4]  */
    cfg->tx_inband_prop.cmp_src_port    = TRUE;  /* ibctr12[10]    */
    cfg->tx_inband_prop.cmp_seq_num     = TRUE;  /* ibctrl2[9]     */
    cfg->tx_inband_prop.cmp_domain_num  = TRUE;  /* ibctrl2[8]     */
    cfg->tx_inband_prop.compare_sopmem  = addr_capture_tx;   /* ibctrl2[14:12] */
    cfg->tx_inband_prop.write_sopmem    = addr_capture_tx;   /* ibctrl2[2:0]   */

    /* [ enable ibts_rxtx_tc phy,? (RX) ]  1588reg.b9[11,7,4]=0x089b  1588reg.ba[10,9,8]=0x0702 */
    cfg->rx_inband_prop.partial_tc_mode = FALSE; /* ibctrl1[8] off */
    cfg->rx_inband_prop.strict          = TRUE;  /* ibctrl1[11] */
    cfg->rx_inband_prop.tc_mode         = TRUE;  /* ibctrl1[7]  */
    cfg->rx_inband_prop.inband_on       = TRUE;  /* ibctrl1[4]  */
    cfg->rx_inband_prop.cmp_src_port    = TRUE;  /* ibctr12[10]    */
    cfg->rx_inband_prop.cmp_seq_num     = TRUE;  /* ibctrl2[9]     */
    cfg->rx_inband_prop.cmp_domain_num  = TRUE;  /* ibctrl2[8]     */
    cfg->rx_inband_prop.compare_sopmem  = addr_capture_rx;   /* ibctrl2[14:12] */
    cfg->rx_inband_prop.write_sopmem    = addr_capture_rx;   /* ibctrl2[2:0]   */

        /* Tx/Rx Resv0_ID  1588reg.b7/b9 [3:0] */
    cfg->inband_ctrl.resv0_id           = 0xb;   /* ibctrl1[3:0] Resv0_ID = 0xb */

    /* [ set int_threshold phy,1 ]  threshold value to trigger the packet timestamp interrupt */
    cfg->fifo_level_intr_thold          = 0x1;       /* 1588reg.b6 */

    /* [ enable txrx_count_packet phy,2]  (1588reg.50) count CRC and PTP Sync packets */
    cfg->tx_pkt_count_sel =
    cfg->rx_pkt_count_sel = bcmplpPktTypePtpSync | bcmplpPktTypeCrc;

    cfg->sop_ts_cap.stamping = 3;

    /* write TimeSync Configurations */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );

    /* [ enable interrupt phy ]  (1588reg.16) enable all interrupts */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_intr_enable_set( chipname, pinfo,
                                (bcmplpTimesyncIntrIpg | bcmplpTimesyncIntrSyncIn |
                                 bcmplpTimesyncIntrSop | bcmplpTimesyncIntrFsync  ),
                                TIMESYNC_ENABLE) );
    return rv;
}

/*
 * TimeSync 1588 config sequence for
 * Partial TC Mode  ( Section 4: Bring-Up Sequence in 1588-Gen2-ANxxx-RDS Application Note )
 * Imitate the VB script introduced in section "Partial TC Mode" of the app note :
 *     P03_ibts_ptc_sync.vbs
 *
 * ( see more details in Section 4.6 of IEEE1588 TimeSync Software User Guide )
 */
int ts_ptc(char *chipname, bcm_plp_access_t pinfo, unsigned int flags,
                     unsigned int timer_precision, long long link_delay,
                     long long    ts_offset_rx   , long long ts_offset_tx ,
                     bcm_plp_timesync_config_t *cfg) {
    int  rv = 0;
    bcm_plp_timesync_time_value_t  timevalue;
    unsigned int  ib32 = (flags & TIMESYNC_INBAND32_FORMAT) ? TRUE : FALSE;
    unsigned int  txrx =  flags & TIMESYNC_DIRECTION_RXTX;
    PROLOGUE_FOV(chipname, pinfo, orilm, fovlm);

    /* [ enable 1588_rxtx phy,1 ]  enable TX 1588 1588reg.00[1,2]*/
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_enable_set(chipname, pinfo, txrx, TIMESYNC_ENABLE) );

    /* [ enable 1588_rxtx_option phy,0 ] 1588reg.08/09[3]  (tx/rx_option[3]) */
    cfg->tx_option.force_ts_to_sopmem = cfg->rx_option.force_ts_to_sopmem = TRUE;
    /* [ enable 1588_rxtx_control phy ] Tx/Rx Control 1588reg.18/1a[3:0] */
    cfg->flags = bcmplpTimesyncFlag8021asEnable | bcmplpTimesyncFlagIp4Enable |
                 bcmplpTimesyncFlagL2Enable     | bcmplpTimesyncFlagIp6Enable ;
    /* [ enable compute_cf_ts phy, 0 ] 1588reg.bc[11]=0 used nse80 bit*/
    cfg->inband_ctrl.timer_mode = timer_precision;

    /* [ enable ibts_rxtx_ptc phy,1 (TX) ]  1588reg.b7[13,11,8,7,4]  1588reg.b8[10,9,8] */
    cfg->tx_inband_prop.clear_rsv012    = TRUE;  /* ibctrl1[13] */
    cfg->tx_inband_prop.ts32_format     = ib32;  /* ibctrl1[12] */
    cfg->tx_inband_prop.strict          = TRUE;  /* ibctrl1[11] */
    cfg->tx_inband_prop.partial_tc_mode = TRUE;  /* ibctrl1[8]  */
    cfg->tx_inband_prop.tc_mode         = TRUE;  /* ibctrl1[7]  */
    cfg->tx_inband_prop.inband_on       = TRUE;  /* ibctrl1[4]  */
    cfg->tx_inband_prop.cmp_src_port    = TRUE;  /* ibctr12[10]    */
    cfg->tx_inband_prop.cmp_seq_num     = TRUE;  /* ibctrl2[9]     */
    cfg->tx_inband_prop.cmp_domain_num  = TRUE;  /* ibctrl2[8]     */
    cfg->tx_inband_prop.compare_sopmem  = addr_capture_tx;   /* ibctrl2[14:12] */
    cfg->tx_inband_prop.write_sopmem    = addr_capture_tx;   /* ibctrl2[2:0]   */

    /* [ enable ibts_rxtx_ptc phy,1 (RX) ]  1588reg.b9[11,8,7,4]  1588reg.ba[10,9,8]    */
    cfg->rx_inband_prop.ts32_format     = ib32;  /* ibctrl1[12] */
    cfg->rx_inband_prop.strict          = TRUE;  /* ibctrl1[11] */
    cfg->rx_inband_prop.partial_tc_mode = TRUE;  /* ibctrl1[8]  */
    cfg->rx_inband_prop.tc_mode         = TRUE;  /* ibctrl1[7]  */
    cfg->rx_inband_prop.inband_on       = TRUE;  /* ibctrl1[4]  */
    cfg->rx_inband_prop.cmp_src_port    = TRUE;  /* ibctr12[10]    */
    cfg->rx_inband_prop.cmp_seq_num     = TRUE;  /* ibctrl2[9]     */
    cfg->rx_inband_prop.cmp_domain_num  = TRUE;  /* ibctrl2[8]     */
    cfg->rx_inband_prop.compare_sopmem  = addr_capture_rx;   /* ibctrl2[14:12] */
    cfg->rx_inband_prop.write_sopmem    = addr_capture_rx;   /* ibctrl2[2:0]   */

        /* Tx/Rx Resv0_ID  1588reg.b7/b9 [3:0] */
    cfg->inband_ctrl.resv0_id           = 0xb;   /* ibctrl1[3:0] Resv0_ID = 0xB */

    /* [ set int_threshold phy,1 ]  threshold value to trigger the packet timestamp interrupt */
    cfg->fifo_level_intr_thold          = 0x1;   /* 1588reg.b6 */

    /* [ enable txrx_count_packet phy,2]  (1588reg.50) count CRC and PTP Sync packets */
    cfg->tx_pkt_count_sel =
    cfg->rx_pkt_count_sel = bcmplpPktTypePtpSync | bcmplpPktTypeCrc;

    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_config_set(chipname, pinfo, cfg) );

    /* [ enable interrupt phy ]  (1588reg.16) enable all interrupts */
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_intr_enable_set( chipname, pinfo,
                                (bcmplpTimesyncIntrIpg | bcmplpTimesyncIntrSyncIn |
                                 bcmplpTimesyncIntrSop | bcmplpTimesyncIntrFsync  ),
                                TIMESYNC_ENABLE) );

    /* Link Delay & Timestamp Offset  for updating CF correction field */
    timevalue.type       =  TIMESYNC_MESSAGE_TYPE_ALL;

    /* [ set link_delay phy,0 ]  (1588reg.05-06) link_delay, for updating CF correction field */
    timevalue.direction  =  txrx;
    timevalue.op         =  (link_delay  >=  0) ? bcmplpMathOperatorAdd
                                                : bcmplpMathOperatorSubtract;
    timevalue.time_value =  llabs(link_delay);
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_link_delay_set(      chipname, pinfo, 0, &timevalue) );
    /* [ set ts_offset_rx  phy,0 ]  (1588reg.0b/7b) Rx timestamp offset */
    timevalue.direction  =  bcmplpTrafficRx;
    timevalue.op         =  (ts_offset_rx >= 0) ? bcmplpMathOperatorAdd
                                                : bcmplpMathOperatorSubtract;
    timevalue.time_value =  llabs(ts_offset_rx);
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timestamp_offset_set(chipname, pinfo, 0, &timevalue) );
    /* [ set ts_offset_tx  phy,0 ]  (1588reg.0a/7b) Tx timestamp offset */
    timevalue.direction  =  bcmplpTrafficTx;
    timevalue.op         =  (ts_offset_tx >= 0) ? bcmplpMathOperatorAdd
                                                : bcmplpMathOperatorSubtract;
    timevalue.time_value =  llabs(ts_offset_tx);
    ALSO_FOV_LANEMAP( pinfo, orilm, fovlm,
            rv |= bcm_plp_timesync_timestamp_offset_set(chipname, pinfo, 0, &timevalue) );

    return rv;
}


#endif /* BCM_PLP_TIMESYNC_SUPPORT */
#endif /*  __TIMESYNC_HELPER_C__   */
