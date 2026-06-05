/*
 * $Id: stat_generic.c,v 1.27 Broadcom SDK $
 *
 *
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 *
 */

#include <soc/drv.h>
#include <soc/counter.h>
#include <soc/debug.h>
#include <shared/bsl.h>
#include <bcm/stat.h>
#include <bcm/error.h>
#include <bcm_int/esw/stat.h>
#include <bcm_int/esw_dispatch.h>
#include <soc/portmod/portmod.h>

#ifdef _ERR_MSG_MODULE_NAME
#error "_ERR_MSG_MODULE_NAME redefined"
#endif
#define _ERR_MSG_MODULE_NAME BSL_LS_SOC_PORT


typedef enum _pm_stat_oper_e
{
   _PORTMOD_PORT_STAT_GET,
   _PORTMOD_PORT_STAT_CLEAR
} pm_stat_oper_t;

typedef enum _pm_stat_reg_oper_e
{
   _PORTMOD_PORT_STAT_REG_ADD,
   _PORTMOD_PORT_STAT_REG_SUB,
   _PORTMOD_PORT_STAT_REG_CLEAR
} pm_stat_reg_oper_t;


int
_portmod_port_stat_reg_op(int unit, soc_port_t port, int reg_op, soc_reg_t ctrl_reg, uint64_t *val)
{
   int rv = SOC_E_NONE;
   uint64_t reg_val;

   switch(reg_op) {
       case _PORTMOD_PORT_STAT_REG_ADD:
            rv = soc_reg_get(unit, ctrl_reg, port, 0, &reg_val);
            if (!rv) COMPILER_64_ADD_64(*val, reg_val);
            break;

       case _PORTMOD_PORT_STAT_REG_SUB:
            rv = soc_reg_get(unit, ctrl_reg, port, 0, &reg_val);
            if (!rv) COMPILER_64_ADD_64(*val, reg_val);
            break;

       case _PORTMOD_PORT_STAT_REG_CLEAR:
            rv = soc_reg_set (unit, ctrl_reg, port, 0, *val);
            break;

       default:
            break;
   }
   return (rv);
}

/*
 * Function:
 *      _portmod_port_stat_get
 * Description:
 *      Get the specified statistic for a port with common counter shared
 *      by both XE and GE MAC.
 * Parameters:
 *      unit - PCI device unit number (driver internal)
 *      port - zero-based port number
 *      type - SNMP statistics type (see stat.h)
 *      val  - (OUT) 64 bit statistic counter value
 * Returns:
 *      BCM_E_NONE - Success.
 *      BCM_E_PARAM - Illegal parameter.
 *      BCM_E_INTERNAL - Chip access failure.
 */
STATIC int
_portmod_port_stat_generic_get_set(int unit, soc_port_t port,
                          int stat_op, portmod_stat_val_t type, uint64_t *val)
{
    uint64_t count;
    int rv;
    int reg_op     = (stat_op == _PORTMOD_PORT_STAT_GET)? _PORTMOD_PORT_STAT_REG_ADD: _PORTMOD_PORT_STAT_REG_CLEAR;
    int reg_sub_op = (stat_op == _PORTMOD_PORT_STAT_GET)? _PORTMOD_PORT_STAT_REG_SUB: _PORTMOD_PORT_STAT_REG_CLEAR;

    COMPILER_REFERENCE(&count);  /* Work around PPC compiler bug */
    COMPILER_64_ZERO(count);

    switch (type) {
        /* *** RFC 1213 *** */
    case portmodIfInOctets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RBYTr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RRBYTr, &count);
        break;
    case portmodIfInUcastPkts:             /* Unicast frames */
        {
            rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPKTr, &count);
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RMCAr, &count); /* - multicast */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RBCAr, &count); /* - broadcast */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RFCSr, &count); /* - bad FCS */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RXCFr, &count); /* - good FCS, all MAC ctrl */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RXPFr, &count); /* - not included in RXCFr */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RJBRr, &count); /* - oversize, bad FCS */
        }
        break;
    case portmodIfInNUcastPkts:             /* Non-unicast frames */
         {
            rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RUCAr, &count); /* + unicast excl */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RMCAr, &count); /* + multicast */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RBCAr, &count); /* + broadcast */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_op, ROVRr, &count); /* + oversize */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RUCr, &count); /* - unicast */
        }
        break;
    case portmodIfInBroadcastPkts:          /* Broadcast frames */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RBCAr, &count);
        break;
    case portmodIfInMulticastPkts:          /* Multicast frames */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RMCAr, &count);
        break;
    case portmodIfInDiscards:           /* Dropped packets including aborted */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RDBGC0r, &count); /* Ingress drop conditions */
        break;
    case portmodIfInErrors: /* RX Errors or Receive packets - non-error frames */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RFCSr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RJBRr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RMTUEr, &count);  /* valid? */
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RRPKTr, &count);
        break;
    case portmodIfOutOctets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TBYTr, &count);
        break;
    case portmodIfOutUcastPkts:             /* Unicast frames */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TUCAr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TOVRr, &count); /* + oversized pkts */
        break;
    case portmodIfOutNUcastPkts:            /* Non-unicast frames */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TMCAr, &count); /* + multicast */
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TBCAr, &count); /* + broadcast */
        break;
    case portmodIfOutBroadcastPkts:         /* Broadcast frames */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TBCAr, &count);
        break;
    case portmodIfOutMulticastPkts:         /* Multicast frames */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TMCAr, &count);
        break;
    case portmodIfOutDiscards:              /* Aged packet counter */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TDBGC3r, &count);
        break;
    case portmodIfOutErrors:   /* Error packets could not be xmitted */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TERRr, &count);
        break;
    case portmodIpInReceives:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPC4r, &count);
        break;
    case portmodIpInHdrErrors:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPD4r, &count);
        break;
    case portmodIpForwDatagrams:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TDBGC4r, &count);
        break;
    case portmodIpInDiscards:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPHE4r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPD4r, &count);
        break;

        /* *** RFC 1493 *** */

    case portmodDot1dBasePortMtuExceededDiscards:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RMTUEr, &count);
        break;
    case portmodDot1dTpPortInFrames:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPKTr, &count);
        break;
    case portmodDot1dTpPortOutFrames:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPKTr, &count);
        break;
    case portmodDot1dPortInDiscards:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RDISCr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPD4r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPD6r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPORTDr, &count);
        break;

        /* *** RFC 1757 *** */

    case portmodEtherStatsDropEvents:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RDISCr, &count);
        break;
    case portmodEtherStatsOctets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RBYTr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TBYTr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RRBYTr, &count); /* Runts bytes */
        break;
    case portmodEtherStatsPkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPKTr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPKTr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RRPKTr, &count); /* Runts */
        break;
    case portmodEtherStatsBroadcastPkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RBCAr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TBCAr, &count);
        break;
    case portmodEtherStatsMulticastPkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RMCAr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TMCAr, &count);
        break;
    case portmodEtherStatsCRCAlignErrors:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RFCSr, &count);
        break;
    case portmodEtherStatsUndersizePkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RUNDr, &count);
        break;
    case portmodEtherStatsOversizePkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, ROVRr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TOVRr, &count);
        break;
    case portmodEtherRxOversizePkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, ROVRr, &count);
        break;
    case portmodEtherTxOversizePkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TOVRr, &count);
        break;
    case portmodEtherStatsFragments:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RFRGr, &count);
        break;
    case portmodEtherStatsJabbers:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RJBRr, &count);
        break;
        /* *** rfc1757 definition counts receive packet only *** */

    case portmodEtherStatsPkts64Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R64r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T64r, &count);
        break;
    case portmodEtherStatsPkts65to127Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R127r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T127r, &count);
        break;
    case portmodEtherStatsPkts128to255Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R255r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T255r, &count);
        break;
    case portmodEtherStatsPkts256to511Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R511r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T511r, &count);
        break;
    case portmodEtherStatsPkts512to1023Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R1023r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T1023r, &count);
        break;
    case portmodEtherStatsPkts1024to1518Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R1518r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T1518r, &count);
        break;
    case portmodBcmEtherStatsPkts1519to1522Octets: /* not in rfc1757 */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RMGVr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TMGVr, &count);
        break;
    case portmodBcmEtherStatsPkts1522to2047Octets: /* not in rfc1757 */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R2047r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RMGVr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T2047r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, TMGVr, &count);
        break;
    case portmodBcmEtherStatsPkts2048to4095Octets: /* not in rfc1757 */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R4095r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T4095r, &count);
        break;
    case portmodBcmEtherStatsPkts4095to9216Octets: /* not in rfc1757 */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R9216r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T9216r, &count);
        break;
    case portmodBcmEtherStatsPkts9217to16383Octets: /* not in rfc1757 */
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R16383r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T16383r, &count);
        break;

    case portmodBcmReceivedPkts64Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R64r, &count);
        break;
    case portmodBcmReceivedPkts65to127Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R127r, &count);
        break;
    case portmodBcmReceivedPkts128to255Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R255r, &count);
        break;
    case portmodBcmReceivedPkts256to511Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R511r, &count);
        break;
    case portmodBcmReceivedPkts512to1023Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R1023r, &count);
        break;
    case portmodBcmReceivedPkts1024to1518Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R1518r, &count);
        break;
    case portmodBcmReceivedPkts1519to2047Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R2047r, &count);
        break;
    case portmodBcmReceivedPkts2048to4095Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R4095r, &count);
        break;
    case portmodBcmReceivedPkts4095to9216Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R9216r, &count);
        break;
    case portmodBcmReceivedPkts9217to16383Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, R16383r, &count);
        break;

    case portmodBcmTransmittedPkts64Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T64r, &count);
        break;
    case portmodBcmTransmittedPkts65to127Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T127r, &count);
        break;
    case portmodBcmTransmittedPkts128to255Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T255r, &count);
        break;
    case portmodBcmTransmittedPkts256to511Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T511r, &count);
        break;
    case portmodBcmTransmittedPkts512to1023Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T1023r, &count);
        break;
    case portmodBcmTransmittedPkts1024to1518Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T1518r, &count);
        break;
    case portmodBcmTransmittedPkts1519to2047Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T2047r, &count);
        break;
    case portmodBcmTransmittedPkts2048to4095Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T4095r, &count);
        break;
    case portmodBcmTransmittedPkts4095to9216Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T9216r, &count);
        break;
    case portmodBcmTransmittedPkts9217to16383Octets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, T16383r, &count);
        break;

    case portmodEtherStatsTXNoErrors:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPOKr, &count);
        break;
    case portmodEtherStatsRXNoErrors:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPOKr, &count);
        break;

        /* *** RFC 2665 *** */

    case portmodDot3StatsFCSErrors:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RFCSr, &count);
        break;
    case portmodDot3StatsInternalMacTransmitErrors:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TUFLr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TERRr, &count);
        break;
    case portmodDot3StatsFrameTooLongs:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RMTUEr, &count);
        break;
    case portmodDot3StatsSymbolErrors:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RERPKTr, &count);
        break;
    case portmodDot3ControlInUnknownOpcodes:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RXUOr, &count);
        break;
    case portmodDot3InPauseFrames:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RXPFr, &count);
        break;
    case portmodDot3OutPauseFrames:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TXPFr, &count);
        break;

        /* *** RFC 2233 high capacity versions of RFC1213 objects *** */

    case portmodIfHCInOctets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RBYTr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RRBYTr, &count);
        break;
    case portmodIfHCInUcastPkts:
         {
            rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPKTr, &count);
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RMCAr, &count);
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RBCAr, &count);
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RFCSr, &count); /* - bad FCS */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RXCFr, &count); /* - good FCS, all MAC ctrl */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RXPFr, &count); /* - not included in RXCFr */
            if (rv) break;
            rv = _portmod_port_stat_reg_op(unit, port,  reg_sub_op, RJBRr, &count); /* - oversize, bad FCS */
        }
        break;
    case portmodIfHCInMulticastPkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RMCAr, &count);
        break;
    case portmodIfHCInBroadcastPkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RBCAr, &count);
        break;
    case portmodIfHCOutOctets:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TBYTr, &count);
        break;
    case portmodIfHCOutUcastPkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TUCAr, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TOVRr, &count);
        break;
    case portmodIfHCOutMulticastPkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TMCAr, &count);
        break;
    case portmodIfHCOutBroadcastPckts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TBCAr, &count);
        break;

        /* *** RFC 2465 *** */

    case portmodIpv6IfStatsInReceives:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPC6r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, IMRP6r, &count);
        break;
    case portmodIpv6IfStatsInHdrErrors:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPD6r, &count);
        break;
    case portmodIpv6IfStatsInAddrErrors:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPHE6r, &count);
        break;
    case portmodIpv6IfStatsInDiscards:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPHE6r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RIPD6r, &count);
        break;
    case portmodIpv6IfStatsOutForwDatagrams:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TDBGC0r, &count);
        break;
    case portmodIpv6IfStatsOutDiscards:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TDBGC1r, &count);
        break;
    case portmodIpv6IfStatsInMcastPkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, IMRP6r, &count);
        break;
    case portmodIpv6IfStatsOutMcastPkts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TDBGC2r, &count);
        break;

        /* *** IEEE 802.1bb *** */
    case portmodIeee8021PfcRequests:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TXPPr, &count);
        break;
    case portmodIeee8021PfcIndications:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RXPPr, &count);
        break;

        /* IPMC counters (broadcom specific) */

    case portmodBcmIPMCBridgedPckts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RDBGC1r, &count);
        break;
    case portmodBcmIPMCRoutedPckts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, IMRP4r, &count);
        if (rv) break;
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, IMRP6r, &count);
        break;
    case portmodBcmIPMCInDroppedPckts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RDBGC2r, &count);
        break;
    case portmodBcmIPMCOutDroppedPckts:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TDBGC5r, &count);
        break;

    /* Vlan Tag Frame Counters */
    case portmodBcmRxVlanTagFrame:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RVLNr, &count);
        break;
    case portmodBcmRxDoubleVlanTagFrame:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RDVLNr, &count);
        break;
    case portmodBcmTxVlanTagFrame:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TVLNr, &count);
        break;
    case portmodBcmTxDoubleVlanTagFrame:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TDVLNr, &count);
        break;

    /* PFC Control Frame Counters */
    case portmodBcmRxPFCControlFrame:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RXPPr, &count);
        break;

    case portmodBcmTxPFCControlFrame:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TXPPr, &count);
        break;

    /* Receive PFC Frame Priority */
    case portmodBcmRxPFCFramePriority0:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPFC0r, &count);
        break;

    case portmodBcmRxPFCFramePriority1:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPFC1r, &count);
        break;

    case portmodBcmRxPFCFramePriority2:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPFC2r, &count);
        break;

    case portmodBcmRxPFCFramePriority3:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPFC3r, &count);
        break;

    case portmodBcmRxPFCFramePriority4:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPFC4r, &count);
        break;

    case portmodBcmRxPFCFramePriority5:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPFC5r, &count);
        break;

    case portmodBcmRxPFCFramePriority6:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPFC6r, &count);
        break;

    case portmodBcmRxPFCFramePriority7:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, RPFC7r, &count);
        break;

    /* Transmit PFC Frame Priority */
    case portmodBcmTxPFCFramePriority0:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPFC0r, &count);
        break;

    case portmodBcmTxPFCFramePriority1:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPFC1r, &count);
        break;

    case portmodBcmTxPFCFramePriority2:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPFC2r, &count);
        break;

    case portmodBcmTxPFCFramePriority3:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPFC3r, &count);
        break;

    case portmodBcmTxPFCFramePriority4:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPFC4r, &count);
        break;

    case portmodBcmTxPFCFramePriority5:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPFC5r, &count);
        break;

    case portmodBcmTxPFCFramePriority6:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPFC6r, &count);
        break;

    case portmodBcmTxPFCFramePriority7:
        rv = _portmod_port_stat_reg_op(unit, port,  reg_op, TPFC7r, &count);
        break;

    default:
        if (type < portmodCount) {
            return BCM_E_UNAVAIL;
        }
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "rv = _portmod_port_stat_get: Statistic not supported: %d\n"), type));
        return BCM_E_PARAM;
    }

    if (stat_op == _PORTMOD_PORT_STAT_GET) {
        *val = count;
    }

    return rv;
}

/*get counter value. */
int portmod_port_stats_read(int unit, int port, portmod_stat_val_t cntr_type, uint64_t* value)
{
    return (_portmod_port_stat_generic_get_set(unit, port, _PORTMOD_PORT_STAT_REG_ADD, cntr_type, value));
}

/*clear counter value. */
int portmod_port_stats_clear(int unit, int port, portmod_stat_val_t cntr_type)
{
    return (_portmod_port_stat_generic_get_set(unit, port, _PORTMOD_PORT_STAT_REG_CLEAR, cntr_type, 0));
}

#undef _ERR_MSG_MODULE_NAME
