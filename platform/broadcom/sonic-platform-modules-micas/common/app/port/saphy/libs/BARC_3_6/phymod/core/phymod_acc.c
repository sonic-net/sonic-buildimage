/*
 *         
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

/*
 * $Id: phymod_acc.c,v 1.1.2.6 Broadcom SDK $
 * $Copyright:.$
 */

#include <phymod/phymod_acc.h>

int
plp_barchetta_phymod_acc_check(const plp_barchetta_phymod_access_t *pa)
{
    if (pa == 0 ||
        PHYMOD_ACC_BUS(pa) == 0 ||
        PHYMOD_ACC_BUS(pa)->read == 0 ||
        PHYMOD_ACC_BUS(pa)->write == 0) {
        return -1;
    }
    return 0;
}

int
plp_barchetta_phymod_bus_read(const plp_barchetta_phymod_access_t *pa, uint32_t reg, uint32_t *data)
{
    int ret = 0;
    /* Read raw PHY data */
    ret = PHYMOD_ACC_BUS(pa)->read(PHYMOD_ACC_USER_ACC(pa),
                                    PHYMOD_ACC_BUS_ADDR(pa),
                                    reg, data);
    PHYMOD_DEBUG_INFO(("READ:PHYID:0x%x Address:0x%08x Data:0x%08x\n",
                           pa->addr, reg, *data));
    return ret;
}

int
plp_barchetta_phymod_bus_write(const plp_barchetta_phymod_access_t *pa, uint32_t reg, uint32_t data)
{


    PHYMOD_DEBUG_INFO(("WRITE:PHYID:0x%x Address :0x%08x Data:0x%08x\n",
                           pa->addr, reg, data));

    /* Write raw PHY data */
    return PHYMOD_ACC_BUS(pa)->write(PHYMOD_ACC_USER_ACC(pa),
                                     PHYMOD_ACC_BUS_ADDR(pa),
                                     reg, data);
}

int
plp_barchetta_phymod_is_write_disabled(const plp_barchetta_phymod_access_t *pa, uint32_t *data)
{
    if((PHYMOD_ACC_BUS(pa)->is_write_disabled) == NULL) { *data = 0; } else {
        return PHYMOD_ACC_BUS(pa)->is_write_disabled(PHYMOD_ACC_USER_ACC(pa),
                                     data);
    }
    return 0;
}
