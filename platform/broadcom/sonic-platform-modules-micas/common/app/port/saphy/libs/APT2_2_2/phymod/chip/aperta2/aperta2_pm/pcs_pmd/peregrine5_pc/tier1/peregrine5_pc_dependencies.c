/*
 *  $Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.$
 *  $Id$
*/

#include "peregrine5_pc_dependencies.h"
#include "common/srds_api_err_code.h"
#include "peregrine5_pc_common.h"
#include <phymod/phymod.h>
#include <phymod/phymod_system.h>
#include "peregrine5_pc_fields.h"
#include "peregrine5_pc_field_access.h"
#include "peregrine5_pc_interface.h"
#include "peregrine5_pc_functions.h"
#include <tier1/aperta2_reg_access.h>


err_code_t plp_aperta2_peregrine5_pc_pmd_mwr_reg(srds_access_t *sa__, uint16_t addr, uint16_t mask, uint8_t lsb, uint16_t val) {
    plp_aperta2_phymod_phy_access_t sa_copy;
    uint16_t tmp = 0;
    uint32_t i;
    uint32_t error_code, addr_32=0, reg_val = 0;

    
    val = val << lsb;
    val = val & mask ;

    addr_32 = APERTA2_PM_TSC_PERIGRINE_BASE | addr;

    error_code=0;
    PHYMOD_MEMCPY(&sa_copy, sa__, sizeof(srds_access_t));
    for(i=1; i <= 0x8000; i = i << 1) {
        if ( i & sa__->access.lane_mask ) {
            sa_copy.access.lane_mask = i;
            error_code += plp_aperta2_reg32_read(&sa_copy,  addr_32, &reg_val);
            tmp = (uint16_t) (reg_val & 0xffff);
            tmp &= ~(mask);
            tmp |= val;
            error_code+=plp_aperta2_reg32_write(&sa_copy, addr_32, tmp);
        }
    }
    if(error_code) {
        return  ERR_CODE_DATA_NOTAVAIL;
    }
    return  ERR_CODE_NONE;
}

uint8_t plp_aperta2_peregrine5_pc_get_lane(srds_access_t *sa__) {
    if (sa__->access.lane_mask == 0x1) {
        return ( 0 );
    } else if (sa__->access.lane_mask == 0x2) {
        return ( 1 );
    } else if (sa__->access.lane_mask == 0x4) {
        return ( 2 );
    } else if (sa__->access.lane_mask == 0x8) {
        return ( 3 );
    } else if (sa__->access.lane_mask == 0x10) {
        return ( 4 );
    } else if (sa__->access.lane_mask == 0x20) {
        return ( 5 );
    } else if (sa__->access.lane_mask == 0x40) {
        return ( 6 );
    } else if (sa__->access.lane_mask == 0x80) {
        return ( 7 );
    } else if (sa__->access.lane_mask == 0x100) {
        return ( 0 );
    } else if (sa__->access.lane_mask == 0x200) {
        return ( 1 );
    } else if (sa__->access.lane_mask == 0x400) {
        return ( 2 );
    } else if (sa__->access.lane_mask == 0x800) {
        return ( 3 );
    } else if (sa__->access.lane_mask == 0x1000) {
        return ( 4 );
    } else if (sa__->access.lane_mask == 0x2000) {
        return ( 5 );
    } else if (sa__->access.lane_mask == 0x4000) {
        return ( 6 );
    } else if (sa__->access.lane_mask == 0x8000) {
        return ( 7 );
    }else {
        return ( 0 );
    }
}

err_code_t plp_aperta2_peregrine5_pc_delay_ns(srds_access_t *sa__, uint32_t delay_ns) {
    uint32_t delay;
    delay = delay_ns / 1000;
    if (!delay ) {
        delay = 1;
    }
    PHYMOD_USLEEP(delay);
    return ( 0 );
}

err_code_t plp_aperta2_peregrine5_pc_delay_ms(srds_access_t *sa__, uint32_t delay_ms) {

    uint32_t delay;
    delay = delay_ms * 1000;
    if (!delay ) {
        delay = 1;
    }
    PHYMOD_USLEEP(delay);
    return ( 0 );
}

err_code_t plp_aperta2_peregrine5_pc_pmd_wr_reg(srds_access_t *sa__, uint16_t address, uint16_t val){
    uint32_t data = 0xffff & val;
    uint32_t error_code;
    error_code = plp_aperta2_reg32_write(sa__, (APERTA2_PM_TSC_PERIGRINE_BASE | (uint32_t) address), data);
    if(error_code) {
        return  ERR_CODE_DATA_NOTAVAIL;
    }
    return  ERR_CODE_NONE;
}

err_code_t plp_aperta2_peregrine5_pc_delay_us(srds_access_t *sa__, uint32_t delay_us){
    PHYMOD_USLEEP(delay_us);
    return ( 0 );
}

err_code_t plp_aperta2_peregrine5_pc_pmd_rdt_reg(srds_access_t *pa, uint16_t address, uint16_t *val) {
    uint32_t data;
    uint32_t error_code;

    error_code = plp_aperta2_reg32_read(pa, (APERTA2_PM_TSC_PERIGRINE_BASE | (uint32_t) address), &data);
    data = data & 0xffff;
    *val = (uint16_t)data;
    if(error_code)
        return  ERR_CODE_DATA_NOTAVAIL;
    return ( 0 );
}

uint8_t plp_aperta2_peregrine5_pc_get_core(srds_access_t *sa__) {
    return (sa__->access.lane_mask & 0xFF) ? 0 : 1;
}

err_code_t plp_aperta2_peregrine5_pc_pmd_wr_pram(srds_access_t *sa__, uint8_t val){

    return ERR_CODE_NONE;

}

err_code_t plp_aperta2_peregrine5_pc_set_lane(srds_access_t *sa__, uint8_t lane_index)
{
    if (sa__->access.lane_mask & 0xFF) {
        sa__->access.lane_mask = 1 << lane_index;
    } else {
        sa__->access.lane_mask = 1 << (lane_index+8);
    }
    return ERR_CODE_NONE;

}

uint8_t plp_aperta2_peregrine5_pc_get_pll_idx(srds_access_t *sa__)
{
    uint32_t data;
    data = sa__->access.pll_idx;
    return (data & 0x1);
}

err_code_t plp_aperta2_peregrine5_pc_set_pll_idx(srds_access_t *sa__, uint8_t pll_index)
{
    sa__->access.pll_idx = pll_index;
    return ERR_CODE_NONE;
}

uint8_t plp_aperta2_peregrine5_pc_get_micro_idx(srds_access_t *sa__)
{
    uint32_t data;
    data = sa__->access.pll_idx;
    return (data & 0x3);
}

err_code_t plp_aperta2_peregrine5_pc_set_micro_idx(srds_access_t *sa__, uint8_t micro_index)
{
    sa__->access.pll_idx = micro_index;
    return ERR_CODE_NONE;

}

srds_info_t *plp_aperta2_peregrine5_pc_get_info_table_address(srds_access_t *sa__)
{
#if defined(SERDES_EXTERNAL_INFO_TABLE_EN)
    return (srds_info_t *) (sa__->access.pmd_info_ptr);
#else
    return 0;
#endif
}
