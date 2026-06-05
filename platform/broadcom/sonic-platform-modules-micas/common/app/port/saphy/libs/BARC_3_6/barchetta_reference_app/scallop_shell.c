/*
 *
 * $Id:  $
 *
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

/*---------------------------------------------------------------------------------*\
 CLI for invoking PHY driver commands for debug or reference purposes

 This program provides a command line shell to let users issue CLI commands.
 Currently, only REGISTER_READ and TimeSync (IEEE-1588) commands are supported.

 This program may not work in some environments.
\*---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef  GNU_READLINE_SUPPORT
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define  BCM_DEBUG(_f, _a...)
#ifndef  TRUE
#define  TRUE                       1
#endif
#ifndef  FALSE
#define  FALSE                      0
#endif

#if   defined(PLP_EVORA_SUPPORT) || defined(PLP_EUROPA_SUPPORT)
  /* for Evora/Europa in 10G MACSEC bypass mode */
  #include "evora_common.h"
  #define  LANE_MAX                 4
  #define  LANEMAP_MIN              0x01
  #define  LANEMAP_MAX              0x10     /* Max 4 lanes (0x1/2/4/8)   */
  #define  LANE_WIDTH               1

#elif defined(PLP_MIURA_SUPPORT)
  /* for Miura in 10G MACSEC bypass mode */
  #include "miura_common.h"
  #define  LANE_MAX                 4
  #define  LANEMAP_MIN              0x01
  #define  LANEMAP_MAX              0x10     /* Max 4 lanes (0x1/2/4/8)   */
  #define  LANE_WIDTH               1

#elif defined(PLP_APERTA_SUPPORT)
  #include "aperta_common.h"
  #if defined(PLP_APERTA_10G_NRZ)
  /* for Aperta in  10G NRZ (8x10G) mode with lanemaps 0x01/02/04/08/10/20/40/80 */
  #define  LANE_MAX                 8
  #define  LANEMAP_MIN              0x01
  #define  LANEMAP_MAX              0x100    /* Max 2 QLane (0xF0 / 0x0F) */
  #define  LANE_WIDTH               1
  #elif defined(PLP_APERTA_25G_50G_FOV)
  /* for Aperta in  25/50G Failover mode with lanemaps 0x01/0x04          */
  #define  LANE_MAX                 4
  #define  LANEMAP_MIN              0x01
  #define  LANEMAP_MAX              0x08     /* Max 2 QLane (0xF0 / 0x0F) */
  #define  LANE_WIDTH               2
  #else
  /* for Aperta in 100G NRZ (4x25G) mode with lanemaps 0x0F and 0xF0      */
  #define  LANE_MAX                 8
  #define  LANEMAP_MIN              0x0F
  #define  LANEMAP_MAX              0x100    /* Max 2 QLane (0xF0 / 0x0F) */
  #define  LANE_WIDTH               4
  #endif

#elif defined(PLP_QUADRA28_SUPPORT)
  #include "quadra28_common.h"
  #define  LANE_MAX                 1        /* 10G speed, one lane per port */
  #define  LANEMAP_MIN              0x1
  #define  LANEMAP_MAX              0x2
  #define  LANE_WIDTH               1

#elif defined(PLP_BARCHETTA_SUPPORT)
  #include "barchetta_common.h"
  #include "barchetta_config.h"
  #define  LANE_MAX                 8        /* 10G/25G speed, one lane per port */
  #define  LANEMAP_MIN              0x01
  #define  LANEMAP_MAX              0x100
  #define  LANE_WIDTH               1

#else
  #error "Scallop Shell is not supported for this chip yet"
#endif

/* iterate each single lanemap, from high bit to low bit */
#define  FOREACH_LANE_HI2LO(_ln)     \
                      for ( _ln = (LANEMAP_MAX>>1); _ln > 0; _ln >>= 1 )
/* iterate each single lanemap, from low bit to hight bit */
#define  FOREACH_LANE(_ln)     \
                      for ( _ln = 0x1; _ln < LANEMAP_MAX; _ln <<= 1 )
/* iterate each aggregated lanemap, such as 0x3, 0xC, 0x0F, 0xF0, ... */
#define  FOREACH_LANEMAP(_ln)  \
                      for ( _ln = LANEMAP_MIN; _ln < LANEMAP_MAX; _ln <<= LANE_WIDTH )
/* iterate each PhyID */
#define  FOREACH_PHYID(_ad, _list, _i)     for ( _i = 0; (_ad = _list[_i]) >= 0; _i++ )

/* iterate each aggregated lanemap per PhyID */
#define  FOREACH_PHYID_LANEMAP(_ad, _ln, _list, _i)  \
                      FOREACH_PHYID(_ad, _list, _i)   FOREACH_LANEMAP(_ln)
/* iterate each single lanemap per PhyID */
#define  FOREACH_PHYID_LANE(_ad, _ln, _list, _i)     \
                      FOREACH_PHYID(_ad, _list, _i)   FOREACH_LANE(_ln)
/* iterate each interface side (line side and systerm side) */
#define  FOREACH_SIDE(_s)     \
                      for ( (_s) = LINE_SIDE; (_s) <= SYS_SIDE; (_s)++ )

/* symbols for CLI shell to process inputted commands */
#define  EXIT_SHELL                 (-9999)
#define  EMPTY_LINE                 (1688)
#define  CMDLINE_MAX                256
#define  DELIMITERS                 "; =\t\n\r\0"
#define  CMD_DELIMITERS             ";\n\r\0"
#define  PHY_INVALID_ARG            0x80000000
#define  IS_STRMATCH(_s1,_s2)       if ( ! strcmp(_s1,_s2) )
#define  IS_STRMATCH2(_s1,_s2,_s3)  if ( (! strcmp(_s1,_s2)) || (! strcmp(_s1,_s3)) )
#define  NOT_EMPTYSTR(_s)           ( '\0' != *_s )
#define  TS_ARG_MAX                 4

#define  PLP_ACCESS_INFO            ((bcm_plp_access_t) {&p_ctxt, PHY_ID0, LINE_SIDE, LANE_MAP})

#ifdef BCM_PLP_TIMESYNC_SUPPORT
int timesync_conf_t1234_tc( char* chipname, char *opt, unsigned int arg[]);
int timesync_verify_tc_dpll(char* chipname, char *opt, int port, unsigned int arg1, unsigned int arg2);
int timesync_config_filter_dump(char *chipname, int port, int rxtx);
int timesync_config_filter(char *chipname, int port, int rxtx, int index,
                           bcm_plp_timesync_inband_filter_ctrl_t filter);
#endif
unsigned int  failover_lanemap_get(char *chipname, bcm_plp_access_t phyinfo);

const char  empty_str[1] =  { '\0' };

extern int   p_ctxt;
extern int   phyid_all[];
int  run_cmdline(char *cline);
int  cli_shell(char*);

/*
 *   MAIN function - parse OS command line options and init the device
 */
int scallop_main(int argc, char *argv[]) {
    int  rv = 0, phy_id = 0, lane = LANEMAP_MAX, nn = 0;
    unsigned int    rev_id, fw = 0, fw_crc = 0;
    unsigned short  api_ver = 0, en_ver = 0;
    char chip_name[16];

    FOREACH_PHYID(phy_id, phyid_all, nn) {
        rv = bcm_plp_firmware_info_get(chipname,
                                 ((bcm_plp_access_t){&p_ctxt, phy_id, 0, lane}), &fw, &fw_crc);
        rv = bcm_plp_rev_id(chipname, ((bcm_plp_access_t){&p_ctxt, phy_id, 0, lane}), &rev_id);
        printf("Firmware version = 0x%x, CRC = 0x%x on phy_id = %d (rev_id=%x)\n",
                                     fw, fw_crc,       phy_id,      rev_id        );
    }
    printf("Firmware Version used is 0x%x \n",fw);
    bcm_plp_driver_version_get(chipname, ((bcm_plp_access_t){&p_ctxt, phyid_all[0], 0, 0x1}),
                               chip_name, &api_ver, &en_ver);

    printf("\nBroadcom PLP driver version %s.%d.%d  (built %s %s)\n\n",
                                  chip_name, api_ver, en_ver, __DATE__, __TIME__);
    rv = cli_shell(chip_name);   /* command shell */

    return rv;
}

/******************************************************************************/

#define  RUNCMD(_cmd)  do { int rv; printf(":) %s\n", _cmd); \
                            rv = run_cmdline(_cmd); printf("[rv=%d]\n", rv); \
                            memset(_cmd, 0, CMDLINE_MAX); \
                       } while(0)

/******************************************************************************\
|*  Helper function to configure the PHY chips                                   *|
\******************************************************************************/

/* reset ports */
int chip_reset(int phy_id_only, int verbose) {
    int  phy_id, nn, rv = 0;

    FOREACH_PHYID(phy_id, phyid_all, nn) {
        if ( (phy_id == phy_id_only) || (PHY_INVALID_ARG == phy_id_only) ) {
            rv |= bcm_plp_reset_set(chipname, ((bcm_plp_access_t){&p_ctxt, phy_id, 0, 0xff}), 0, 0);
            if ( verbose)    printf("PhyID %2d reset rv=%d\n", phy_id, rv);
        }
    }
    return rv;
}

/* show line/system side link status */
int port_status(int phy_id)
{
    bcm_plp_access_t  pinfo = PLP_ACCESS_INFO;
    unsigned int      link_sts;
    int               lane = 0, rv = 0;

    pinfo.phy_addr = phy_id;
    printf("PhyID %2d", phy_id);
    FOREACH_SIDE(pinfo.if_side) {
        printf("   %s:[ ", (SYS_SIDE == pinfo.if_side) ? "sys" : "line");
        FOREACH_LANE_HI2LO(lane) {
            pinfo.lane_map = lane;
            rv = bcm_plp_link_status_get(chipname, pinfo, &link_sts);
            printf("%c", (rv != 0) ? 'x' : (link_sts) ? '^' : '.');
            if ( lane & 0x11111111 )    putchar(' ');    /* insert a space every 4 lanes */
        }
        printf("]");
    }
    printf("\n");

    return rv;
}


/******************************************************************************\
|*  SCALLOP SHELL                                                             *|
\******************************************************************************/

#define  VAR_SHELF_GET      PHY_INVALID_ARG
#define  VAR_SHELF_PUT      1

/* Environment variable support for SCALLOP Shell */
int  var_shelf(char *varname, int val) {
    static int  shelf[128];
    int  idx = (int) *(varname+1);     /* ++varname to eliminate the prefix $ */

    if ( VAR_SHELF_GET == val )
        return shelf[idx];

    shelf[idx] = val;   /* store the value */
    return PHY_INVALID_ARG;
}

/* convert a hexadecimal digit to decimal value */
int hex1digit(char c1)
{
    if ( (c1 >= 'a') && (c1 <= 'z') )    return  (int) (c1 - 'a') + 10;
    if ( (c1 >= 'A') && (c1 <= 'Z') )    return  (int) (c1 - 'A') + 10;
    if ( (c1 >= '0') && (c1 <= '9') )    return  (int) (c1 - '0');
    return  0;
}
/* convert a char string digit to decimal/hexadecimal integer */
int atoix(char *ix) {
    if ( '0' == *ix )    ix++;     /* discard the leading '0' */
    if ( ('x' == *ix) || ('X' == *ix) || ('h' == *ix) || ('H' == *ix) ) {
        int   val;
        /* hex number (with a leading '0x', 'X' or 'H') */
        for ( val = 0, ix++; *ix; ix++ ) {
            val = (val << 4) + hex1digit(*ix);  /* (0x10*val) + hex1digit(*ix) */
        }
        return  val;
    } else {
        /* decimal number */
        return (atoi(ix));
    }
}

/* get the next hexadecimal/decimal number from user input command */
int next_num(void)
{
    char *dstr = strtok(NULL, DELIMITERS);

    if ( NULL == dstr)   return PHY_INVALID_ARG;

    if ( '$' == dstr[0] ) {
        /* retrieve the value of the variable */
        return  var_shelf(dstr, VAR_SHELF_GET);
    } else {
        /* decimal or hexadecimal number */
        return (atoix(dstr));
    }
}

/* get the next text string token from user input command */
char *next_word(char *sentence)
{
    char *dstr = strtok(sentence, DELIMITERS);
    return  (dstr) ? dstr : (char*) empty_str;
}

void getbytes(const char* str, char sep, char* bytes, int maxb, int b) {
    int i = 0;
    for (i = 0; i < maxb; i++) {
        bytes[i] = strtoul(str, NULL, b);
        str = strchr(str, sep);
        if (str == NULL || *str == '\0') {
            break;
        }
        str++;
    }
}

/*
 *  SCALLOP SHELL main function - parse and run user commands
 */
int
scallop(char cmdline[])
{
    int  rv = 0;
    int  lane = LANEMAP_MIN, phy_id = 0, if_side = LINE_SIDE, nn = 0;
    char *cmd;

    cmd = next_word(cmdline);

    /* simple variable handling mechanism  (e.g.) $<var_name> = <var_value> */
    if ( '$' == cmd[0] ) {
        int  value = next_num();

        if ( PHY_INVALID_ARG != value ) {
            var_shelf(cmd, value);
        } else {
            value = var_shelf(cmd, VAR_SHELF_GET);
            printf(" %s = %d = 0x%x\n", cmd, value, value);
        }
    } else

    /* read/write a register for a specific PhyID */
    IS_STRMATCH(cmd, "r") {
        unsigned int  side, regad, val;
        phy_id = next_num();
        if ( PHY_INVALID_ARG != phy_id ) {
            lane  = next_num();    side = next_num();
            regad = next_num();    val  = next_num();
#if defined(BCM_PLP_TIMESYNC_SUPPORT) && defined(_DEBUGGING_IEEE1588_TIMESYNC_)
            /* for easily specifying IEEE1588 registers */
            if ( 0x0 == (regad & 0xfffff000) )      regad |= 0x49007000;
#endif
            if ( PHY_INVALID_ARG != val ) {
                rv = bcm_plp_reg_value_set(chipname,
                                    ((bcm_plp_access_t){&p_ctxt, phy_id, side, lane}),
                                    1, regad,  val);
                printf("rv=%d PhyID=[%02x.0x%02x]%s Reg.0x%08x <== 0x%04x\n", rv,
                        phy_id, lane, (side==SYS_SIDE) ? "SYS " : "Line", regad, val);
            } else {
                rv = bcm_plp_reg_value_get(chipname,
                                    ((bcm_plp_access_t){&p_ctxt, phy_id, side, lane}),
                                    1, regad, &val);
                printf("rv=%d PhyID=[%02x.0x%02x]%s Reg.0x%08x  =  0x%04x\n", rv,
                        phy_id, lane, (side==SYS_SIDE) ? "SYS " : "Line", regad, val);
            }
        } else {
            printf("Usage:  r <PhyID> <lanemap> <if_side> <Reg Addr> [<write value>]\n\n");
        }
    } else

    /* dump firmware version and CRC for all ports*/
    IS_STRMATCH2(cmd, "firmware", "fw") {
        unsigned int  fw_version=0, fw_crc=0;
        FOREACH_PHYID(phy_id, phyid_all, nn) {
            rv = bcm_plp_firmware_info_get(chipname, ((bcm_plp_access_t){&p_ctxt, phy_id, 0, 0xff}),
                                       &fw_version, &fw_crc);
            printf("rv=%d phy_id %2d  firmware ver = 0x%04x  crc = 0x%04x\n",
                    rv, phy_id, fw_version, fw_crc);
        }
    } else

    /* dump line/system side status for all PhyID/lanes */
    IS_STRMATCH(cmd, "ps") {
        phy_id = next_num();

        printf("port Link Status:   ( ^=Up .=Down x=Error )\n");
        if ( PHY_INVALID_ARG != phy_id ) {
            do {
                rv = port_status(phy_id);
                phy_id = next_num();
            } while ( PHY_INVALID_ARG != phy_id );

        } else {
            FOREACH_PHYID(phy_id, phyid_all, nn) {
                rv = port_status(phy_id);
            }
        }
    } else

    /* set line/system side loopback */
    IS_STRMATCH2(cmd, "loopback", "lb") {
        char  lb_status[3][2] = { {'_', '_' }, {'.', 'D' }, {'.', 'R' } };
        unsigned int  dlb = 0, rlb = 0;

        if ( PHY_INVALID_ARG == (phy_id = next_num()) ) {
            /* PhyID not given, show the usage */
            printf("Usage: lb <PhyID> <lanemap>  <if_side>    <Digital_LB> <Remote_LB>\n");
            printf("                           (0=line 1=sys)   ( 0 | 1 )   ( 0 | 1 )\n\n");
        }
        else {
            if ( PHY_INVALID_ARG == (lane    = next_num()) )    lane    = LANEMAP_MIN;
            if ( PHY_INVALID_ARG == (if_side = next_num()) )    if_side = LINE_SIDE;
            BCM_DEBUG("dlb %d %d\n", phy_id, dlb);
            if ( PHY_INVALID_ARG != (dlb = next_num()) ) {
                rv = bcm_plp_loopback_set(chipname,
                                ((bcm_plp_access_t){&p_ctxt, phy_id, if_side, lane}), 1,  dlb);
                if ( PHY_INVALID_ARG != (rlb = next_num()) ) {
                    rv = bcm_plp_loopback_set(chipname,
                                ((bcm_plp_access_t){&p_ctxt, phy_id, if_side, lane}), 2,  rlb);
                } else {
                    rlb = 0;
                }
            } else {
                rv  = bcm_plp_loopback_get(chipname,
                                ((bcm_plp_access_t){&p_ctxt, phy_id, if_side, lane}), 1, &dlb);
                rv |= bcm_plp_loopback_get(chipname,
                                ((bcm_plp_access_t){&p_ctxt, phy_id, if_side, lane}), 2, &rlb);
            }
            sleep(1);
        }

        FOREACH_SIDE(if_side) {      /* display current loopback settings */
            FOREACH_PHYID(phy_id, phyid_all, nn) {
                printf("PhyID %2d %s side Loopback: Digital = [ ", phy_id,
                                                     (SYS_SIDE == if_side) ? " sys" : "line");
                /* Digital PMD Loopback status per PhyID per lane */
                FOREACH_LANE_HI2LO(lane) {
                    rv  = bcm_plp_loopback_get(chipname,
                                    ((bcm_plp_access_t){&p_ctxt, phy_id, if_side, lane}), 1, &dlb);
                    printf("%c", lb_status[1][dlb]);
                    if ( lane & 0x11111111 )    putchar(' ');    /* insert a space every 4 lanes */
                }
                printf("]  Remote = [ ");
                /* Remote PMD Loopback status per PhyID per lane */
                FOREACH_LANE_HI2LO(lane) {
                    rv  = bcm_plp_loopback_get(chipname,
                                    ((bcm_plp_access_t){&p_ctxt, phy_id, if_side, lane}), 2, &rlb);
                    printf("%c", lb_status[2][rlb]);
                    if ( lane & 0x11111111 )    putchar(' ');    /* insert a space every 4 lanes */
                }
                printf("]  rv=%d\n", rv);
            }
        }
    } else

#if defined(PLP_APERTA_25G_50G_FOV)
    /* switch over to failover ports */
    IS_STRMATCH2(cmd, "failover", "fov") {
        int p_ctxt1 = 5;
        bcm_plp_access_t  phy_info = { NULL, 0x0, 0, 0x0, 0x0, 0x0 };
        phy_info.phy_addr = (unsigned int) next_num();
        phy_info.lane_map = (unsigned int) next_num();
        phy_info.if_side  = SYS_SIDE;
        phy_info.platform_ctxt = &p_ctxt1;
        rv = bcm_plp_failover_mode_set(chipname, phy_info, TRUE);
        if ( ! rv ) {
            printf("PhyID 0x%02X  LaneMap 0x%04X ==>> 0x%04X\n", phy_info.phy_addr,
                    phy_info.lane_map, failover_lanemap_get(chipname, phy_info));
        } else {
            printf("!! rv = %d\n", rv);
        }
    } else
#endif

#if defined(BCM_PLP_TIMESYNC_SUPPORT)
    /* IEEE-1588 PTP TimeSync */
    IS_STRMATCH2(cmd, "timesync", "ts") {
        unsigned int  arg[TS_ARG_MAX], ii;
        char         *opt = next_word(NULL);

        for ( ii = 1; ii <= TS_ARG_MAX;  ii++ ) {
            if ( PHY_INVALID_ARG == (arg[ii] = (unsigned int) next_num()) ) {
                arg[ii] = 0xffffffff;
            }
        }
        rv = timesync_conf_t1234_tc(chipname, opt, arg);
        if ( rv )   printf("!! rv = %d\n", rv);
    } else

    /* IEEE-1588 verifications for  TC + DPLL  */
    IS_STRMATCH2(cmd, "tcdpll", "tc") {
        unsigned int  port, arg1, arg2;
        char         *opt = next_word(NULL);
        port = (unsigned int) next_num();
        arg1 = (unsigned int) next_num();
        arg2 = (unsigned int) next_num();
        rv = timesync_verify_tc_dpll(chipname, opt, port, arg1, arg2);
        if ( rv )   printf("!! rv = %d\n", rv);
    } else

    /* IEEE-1588 PTP Inband Filter */
    IS_STRMATCH2(cmd, "tsibf", "ibf") {
        bcm_plp_timesync_inband_filter_ctrl_t  filter;
        unsigned int  rxtx = bcmplpTimesyncTxRx, port = PHY_INVALID_ARG;
        char         *first = next_word(NULL);
        memset(&filter, 0, sizeof(bcm_plp_timesync_inband_filter_ctrl_t));

        IS_STRMATCH2(first, "dump", "d") {
            if ( '\0' != first[1] )    next_word(NULL);
            rxtx = (unsigned int) next_num();   /* Rx/Tx direction        */
            port = (unsigned int) next_num();   /* port number (optional) */
            printf("\t%cx direction :\n", (bcmplpTimesyncRx == rxtx) ? 'R' : 'T');
            rv = timesync_config_filter_dump(chipname, port, rxtx);
        } else

        IS_STRMATCH2(first, "valid", "v") {
            int          ii = 0, jj = 0;
            unsigned int fval = 0, valid = 0, action = 0, macip = 0, dst = 0, src = 0, index = 0;
            char        *fstr = NULL, *addrval = NULL;

            valid = next_num();
            for ( fstr = next_word(NULL); *fstr != '\0'; fstr = next_word(NULL) ) {
                IS_STRMATCH(fstr, "addr") {
                   addrval = next_word(NULL);
                   continue;
                }
                if ( PHY_INVALID_ARG == (fval = next_num()) )    break;    /* end of input line */

                IS_STRMATCH2(fstr, "action", "a") {
                   action = fval;
                } else
                IS_STRMATCH2(fstr, "macip" , "m") {
                    macip = fval;
                } else
                IS_STRMATCH2(fstr, "dst"   , "d") {
                    dst   = fval;
                } else
                IS_STRMATCH2(fstr, "src"   , "s") {
                    src   = fval;
                } else
                IS_STRMATCH2(fstr, "port"  , "p") {
                    port  = fval;
                } else
                IS_STRMATCH2(fstr, "rxtx"  , "x") {
                    rxtx  = fval;
                } else
                IS_STRMATCH2(fstr, "index" , "i") {
                    index = fval;
                }
            }

            filter.valid  = valid;
            filter.action = action;
            /* Don't care PTP Version flag (1 << 2) */
            filter.flags = (macip << 3 | dst << 1 | src) & 0xb;

            if ((filter.flags == 0) || (filter.flags == 0x4)) {
                printf("Set IBF entry: Index[%d] Valid[%d] Action[%d] Flags[0x%x] "
                       "Direction[%d]\n", index, filter.valid, filter.action, filter.flags, rxtx);
            } else if (macip == 1) {
                char mac[6];
                getbytes(addrval, '-', mac, 6, 16);
                for (ii = 0, jj = 5; ii < 6; ii++, jj--) {
                    filter.match_addr.mac_addr[ii] = mac[jj] & 0xFF;
                }
                printf("Set IBF entry: Index[%d] Valid[%d] Action[%d] Flags[0x%x] "
                       "Direction[%d] MAC[%02x-%02x-%02x-%02x-%02x-%02x]\n", index,
                        filter.valid, filter.action, filter.flags, rxtx,
                        filter.match_addr.mac_addr[5], filter.match_addr.mac_addr[4],
                        filter.match_addr.mac_addr[3], filter.match_addr.mac_addr[2],
                        filter.match_addr.mac_addr[1], filter.match_addr.mac_addr[0]);
            } else {
                char ipv4[4];
                getbytes(addrval, '.', ipv4, 4, 10);
                for (ii = 0, jj = 3; ii < 4; ii++, jj--) {
                    filter.match_addr.ip_addr[ii] = ipv4[jj] & 0xFF;
                }
                printf("Set IBF entry: Index[%d] Valid[%d] Action[%d] Flags[0x%x] "
                       "Direction[%d] IPv4[%02x-%02x-%02x-%02x]\n", index,
                        filter.valid, filter.action, filter.flags, rxtx,
                        filter.match_addr.ip_addr[3], filter.match_addr.ip_addr[2],
                        filter.match_addr.ip_addr[1], filter.match_addr.ip_addr[0]);
            }
            rv = timesync_config_filter(chipname, port, rxtx, index, filter);
        }
        else {
            printf("Usage:\n");
            printf("   tsibf dump rxtx <1|2> [<port>]\n");
            printf("   tsibf valid <0|1> action <0-3> addr <1.0.0.0|aa-bb-cc-dd-ee-ff> macip <0|1>\n"
                                           "\t\t\t\t   dst <0|1> src <0|1> rxtx <1|2> index <0-31>\n");
        }

        if ( rv )   printf("!! rv = %d\n", rv);
        printf("\n");
    } else
#endif  /* BCM_PLP_TIMESYNC_SUPPORT */

    /* reset one or all PhyIDs */
    IS_STRMATCH(cmd, "reset") {
        rv = chip_reset(next_num(), TRUE);
    } else

    /* ZZzzz... */
    IS_STRMATCH(cmd, "sleep") {
        int sec = next_num();
        if ( sec < 33 ) {
            sleep(sec);
        } else {
            usleep(sec);
        }
    } else

    /* just enter key, do nothing */
    IS_STRMATCH(cmd, "") {
        return EMPTY_LINE;
    } else

    /* quit the CLI shell */
    IS_STRMATCH2(cmd, "exit", "q" ) {
        char*  opt = next_word(NULL);
        rv = 0;
        if ( 'w' != opt[0] ) {
            rv |= chip_reset(PHY_INVALID_ARG, TRUE);   /* reset all PhyIDs */
        }
        return EXIT_SHELL;

    } else {    /* unknown command */

        printf("\t%s ??\n", cmd);
        rv = 0;
    }

    return rv;
}

/******************************************************************************/

/*
 *  One user input command line can have multiple commands separated by semicolons.
 *  Extract each command and feed them into scallop() to run.
 */
int run_cmdline(char *cline) {
    int    rv = 0;
    char  *cmd, *cmdline_ptr;

    cmdline_ptr = cline;
    cmd = strtok(cmdline_ptr, ";\n");
    while ( cmd ) {
        cmdline_ptr += strlen(cmd);     cmdline_ptr++;
        /* if ( cmd && cmdline_ptr )   printf("[%s](%s)\n", cmd, cmdline_ptr); */
        rv = scallop(cmd);      printf("\n");
        cmd = strtok(cmdline_ptr, ";\n");
    }
    return rv;
}

#ifdef  GNU_READLINE_SUPPORT

static char  input_line[CMDLINE_MAX], *line_read = (char*) NULL;

/* read a string, and return a pointer to it.  Returns NULL on EOF. */
char *readline_gets(char *prompt) {
    if ( line_read ) {
        /* the buffer has already been allocated */
        free (line_read);   /* return the memory to the free pool */
        line_read = (char*) NULL;
    }

    line_read = readline (prompt);      /* get a line from the user. */
    /* if the line has any text in it, save it on the history. */
    if ( line_read && *line_read )      add_history(line_read);

    return  line_read;
}

/* Main loop to receive user input command lines */
int cli_shell(char *chipname) {
    int    rv = 0;
    char   prompt[16];

    sprintf(prompt, "%s:) ", chipname);

    while ( rv != EXIT_SHELL ) {
        memset(input_line, 0, CMDLINE_MAX);
        strcpy(input_line, readline_gets(prompt));
        rv = run_cmdline(input_line);
    }
    return 0;
}

#else      /* without GNU_READLINE_SUPPORT */

/* #define  RUNCMD(_cmd)  do{ int rv=scallop(_cmd); } while(0) */

/* Main loop to receive user input command lines */
int cli_shell(char *chipname) {
    int  rv = 0;
    char cmdline[CMDLINE_MAX], last_cmdline[CMDLINE_MAX] = { '\n', '\0' };

    while ( rv != EXIT_SHELL ) {
        printf("%s:) ", chipname);
        /*  printf("\033[01;34m%s\033[00;33m>\033[00m ", chipname);  */
        if ( rv == EMPTY_LINE ) {
            strcpy(cmdline, last_cmdline);
            puts(cmdline);  /* repeat last command line */
        } else {
            fgets(cmdline, CMDLINE_MAX, stdin);
            if ( cmdline[0] == '\n' ) {
                strcpy(cmdline, last_cmdline);
                /* printf(":) %s", cmdline); */  /* repeat last command line */
            } else {
                strcpy(last_cmdline, cmdline);
            }
        }

#if _ONE_CMD_PER_INPUT_LINE_
        rv = scallop(cmdline);
#else
        {
            char *cmd, *cmdline_ptr = cmdline;
            cmd = strtok(cmdline, CMD_DELIMITERS);
            while ( cmd ) {
                cmdline_ptr += strlen(cmd);     cmdline_ptr++;
                /* if ( cmd && cmdline_ptr )   printf("[%s][%s]\n", cmd, cmdline_ptr); */
                rv = scallop(cmd);      printf("\n");
                cmd = strtok(cmdline_ptr, ";\n");
            }
        }
#endif
    }
    return 0;
}

#endif  /* GNU_READLINE_SUPPORT */

