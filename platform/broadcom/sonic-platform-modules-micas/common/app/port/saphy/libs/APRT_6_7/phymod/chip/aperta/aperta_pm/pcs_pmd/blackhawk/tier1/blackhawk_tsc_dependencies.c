/*
 *  $Id$
 * $Copyright: (c) 2020 Broadcom. 
 * Broadcom Proprietary and Confidential. All rights reserved.$
*/

#ifdef LINUX
/* #include <stdint.h> */
#endif

#include "blackhawk_tsc_dependencies.h"
#include "common/srds_api_err_code.h"
#include "blackhawk_tsc_common.h"
#include <phymod/phymod.h>
#include "blackhawk_tsc_fields.h"
#include "blackhawk_tsc_field_access.h"
#include "blackhawk_tsc_interface.h"
#include "blackhawk_tsc_functions.h"
#include <tier1/aperta_reg_access.h>

extern srds_info_t plp_aperta_blackhawk_tsc_info[NUM_SERDES_INFO_TABLES];

err_code_t plp_aperta_blackhawk_tsc_pmd_mwr_reg(srds_access_t *sa__, uint16_t addr, uint16_t mask, uint8_t lsb, uint16_t val) {

    srds_access_t sa_copy;
    uint16_t tmp;
    uint32_t error_code;
    uint32_t reg_val, i;
     uint32_t addr_32;

    val = val << lsb;
    val = val & mask ;

    if (addr > 0x9c) {
        addr_32 = APERTA_PM_TSC_BLACKHAWK_BASEADR | addr;
    } else {
        addr_32 = (APERTA_PM_TSC_BLACKHAWK_BASEADR & ~(0xF000)) | addr;
    }

    error_code=0;
    PHYMOD_MEMCPY(&sa_copy, sa__, sizeof(srds_access_t));
    for(i=1; i <= 0x80; i = i << 1) {
        if ( i & sa__->access.lane_mask ) {
            sa_copy.access.lane_mask = i;
               error_code += plp_aperta_reg32_read(&sa_copy,  addr_32, &reg_val);
            tmp = (uint16_t) (reg_val & 0xffff);
            tmp &= ~(mask);
            tmp |= val;
            error_code+=plp_aperta_reg32_write(&sa_copy, addr_32, tmp);
        }
    }
    if(error_code)
      return  ERR_CODE_DATA_NOTAVAIL;
    return  ERR_CODE_NONE;
}

uint8_t plp_aperta_blackhawk_tsc_get_lane(srds_access_t *sa__) {
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
    } else {
        return ( 0 );
    }
}

uint16_t plp_aperta_blackhawk_tsc_pmd_rd_reg(srds_access_t *sa__, uint16_t address){

    uint32_t data, addr_32 = 0;
    if (address > 0x9c) {
        addr_32 = APERTA_PM_TSC_BLACKHAWK_BASEADR | address;
    } else {
        addr_32 = (APERTA_PM_TSC_BLACKHAWK_BASEADR & ~(0xF000)) | address;
    }

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg32_read(sa__, addr_32, &data));
    data = data & 0xffff;
    return ( (uint16_t)data );
}

err_code_t plp_aperta_blackhawk_tsc_delay_ns(uint16_t delay_ns) {
    uint32_t delay;
    delay = delay_ns / 1000;
    if (!delay ) {
        delay = 1;
    }
    PHYMOD_USLEEP(delay);
    return ( 0 );
}

err_code_t plp_aperta_blackhawk_tsc_delay_ms(uint32_t delay_ms) {

    uint32_t delay;
    delay = delay_ms * 1000;
    if (!delay ) {
        delay = 1;
    }
    PHYMOD_USLEEP(delay);
    return ( 0 );
}


/**
@brief   Falcon PMD Write
@param   phymod access handle to current Falcon Context
@param   address
@param   val
@returns The ERR_CODE_NONE upon successful completion, else returns ERR_CODE_DATA_NOTAVAIL
*/
err_code_t plp_aperta_blackhawk_tsc_pmd_wr_reg(srds_access_t *sa__, uint16_t addr, uint16_t val){
    uint32_t data = 0xffff & val;
    uint32_t error_code, addr_32 = 0;
    if (addr > 0x9c) {
        addr_32 = APERTA_PM_TSC_BLACKHAWK_BASEADR | addr;
    } else {
        addr_32 = (APERTA_PM_TSC_BLACKHAWK_BASEADR & ~(0xF000)) | addr;
    }

    error_code = plp_aperta_reg32_write(sa__, addr_32, data);
    if(error_code)
      return  ERR_CODE_DATA_NOTAVAIL;
    return  ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_delay_us(uint32_t delay_us){
    PHYMOD_USLEEP(delay_us);
    return ( 0 );
}

err_code_t plp_aperta_blackhawk_tsc_pmd_rdt_reg(srds_access_t *pa, uint16_t address, uint16_t *val) {
    uint32_t data = 0, addr_32 = 0;

    if (address > 0x9c) {
        addr_32 = APERTA_PM_TSC_BLACKHAWK_BASEADR | address;
    } else {
        addr_32 = (APERTA_PM_TSC_BLACKHAWK_BASEADR & ~(0xF000)) | address;
    }

    PHYMOD_IF_ERR_RETURN(
       plp_aperta_reg32_read(pa, addr_32, &data));
    data = data & 0xffff;
    *val = (uint16_t)data;
    return ( 0 );
}

uint8_t plp_aperta_blackhawk_tsc_get_core(srds_access_t *sa__) {
    return(0);
}

err_code_t plp_aperta_blackhawk_tsc_uc_lane_idx_to_system_id(srds_access_t *sa__, char string[16], uint8_t uc_lane_idx){
    static char info[16];
   /* Indicates Falcon Core */
    PHYMOD_SPRINTF(info, "%s_%d", "FC_", uc_lane_idx);
   /* PHYMOD_STRNCPY(string,info, PHYMOD_STRLEN(info)+1);*/
    PHYMOD_SPRINTF(string,"%s", info);
    return ERR_CODE_NONE;
}

err_code_t plp_aperta_blackhawk_tsc_pmd_wr_pram(srds_access_t *sa__, uint8_t val){

    return ERR_CODE_NONE;

}

err_code_t plp_aperta_blackhawk_tsc_set_lane(srds_access_t *sa__, uint8_t lane_index)
{
    sa__->access.lane_mask = 1 << lane_index;
    return ERR_CODE_NONE;

}

uint8_t plp_aperta_blackhawk_tsc_get_pll_idx(srds_access_t *sa__)
{
    uint32_t data;
    data = sa__->access.pll_idx;
    return (data & 0x1);
}

err_code_t plp_aperta_blackhawk_tsc_set_pll_idx(srds_access_t *sa__, uint8_t pll_index)
{
    sa__->access.pll_idx = pll_index;
    return ERR_CODE_NONE;
}

uint8_t plp_aperta_blackhawk_tsc_get_micro_idx(srds_access_t *sa__)
{
    /*uint32_t data;
    PHYMOD_IF_ERR_RETURN(
        PHYMOD_BUS_READ(&sa__->access, BCMI_APERTA_D_CTRL_SWGPREG05r, &data));
    return (data & 0xF);*/
    return (sa__->access.pll_idx & 0xF);
}

err_code_t plp_aperta_blackhawk_tsc_set_micro_idx(srds_access_t *sa__, uint8_t micro_index)
{
/*	PHYMOD_IF_ERR_RETURN(
        PHYMOD_BUS_WRITE(&sa__->access, BCMI_APERTA_D_CTRL_SWGPREG05r, micro_index));
    return ERR_CODE_NONE;*/
     sa__->access.pll_idx = micro_index;
    return ERR_CODE_NONE;

}


