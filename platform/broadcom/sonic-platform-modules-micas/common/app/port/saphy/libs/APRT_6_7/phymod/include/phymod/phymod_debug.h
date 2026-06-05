/*
 *         
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

/*
 * $Id: phymod_debug.h,v 1.1.2.4 Broadcom SDK $
 * $Copyright:.$
 */

#ifndef __plp_aperta_PHYMOD_DEBUG_H__
#define __plp_aperta_PHYMOD_DEBUG_H__

#include <phymod/phymod.h>

extern uint32_t plp_aperta_phymod_dbg_mask;
extern uint32_t plp_aperta_phymod_dbg_addr;
extern uint32_t plp_aperta_phymod_dbg_lane;

extern int
plp_aperta_phymod_debug_check(uint32_t flags, const plp_aperta_phymod_access_t *pa);

/*
 * The PHYMOD_VDBG macro can be used by PHYMOD driver implementations
 * to print verbose debug message which can be filtered by a mask
 * and a PHY address parameter.
 *
 * Examples of the call syntax:
 *
 *   PHYMOD_VDBG(DBG_SPD, pa, ("link status %d\n", link));
 *   PHYMOD_VDBG(DBG_AN, NULL, ("autoneg status 0x%x\n", an_stat));
 *
 * Please note the use of parentheses.
 *
 * The DBG_SPD and DBG_AN parameters should each be a bit mask for
 * filtering speed-related messages (see #defines below).
 *
 * The 'pa' parameter is a pointer to a plp_aperta_phymod_access_t from which the
 * PHY address is pulled. If this pointer is NULL, the message will be
 * shown for any PHY address.
 */

/*
 * Bits [31:20] are reserved for common debug output flags.  A driver
 * may use bits [19:0] for implementation-specific debug output.
 */
#define DBG_CL72        (1L << 20) /* CL72 */
#define DBG_TOP         (1L << 21) /* Lane swap and polarity */
#define DBG_TST         (1L << 22) /* Testing and PRBS */
#define DBG_SCN         (1L << 23) /* Link scan */
#define DBG_PMD         (1L << 24) /* PMD preemphasis or driver control */
#define DBG_LPK         (1L << 25) /* Local or remote loopback */
#define DBG_AN          (1L << 26) /* Auto-negotiation */
#define DBG_SPD         (1L << 27) /* Speed */
#define DBG_LNK         (1L << 28) /* Link */
#define DBG_CFG         (1L << 29) /* Configuration */
#define DBG_ACC         (1L << 30) /* Register access */
#define DBG_DRV         (1L << 31) /* Driver code outside PHYMOD */


#define PHYMOD_VDBG(flags_, pa_, stuff_) \
    if (plp_aperta_phymod_debug_check(flags_, pa_)) \
        PHYMOD_DEBUG_ERROR(stuff_)

#endif /* _PHYMOD_DEBUG_H__ */
