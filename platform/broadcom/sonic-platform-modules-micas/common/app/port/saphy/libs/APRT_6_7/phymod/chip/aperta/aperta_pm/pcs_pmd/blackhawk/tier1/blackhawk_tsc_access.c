/***********************************************************************************
 ***********************************************************************************
 *  File Name     :  blackhawk_tsc_access.c                                           *
 *  Created On    :  03 Nov 2015                                                   *
 *  Created By    :  Brent Roberts                                                 *
 *  Description   :  APIs for Serdes IPs                                           *
 *  Revision      :    *
 *                                                                                 *
 * $Copyright: (c) 2020 Broadcom.                                                  *
 * Broadcom Proprietary and Confidential. All rights reserved.$                    *
 *                                                                                 *
 ***********************************************************************************
 ***********************************************************************************/

/** @file blackhawk_tsc_access.c
 * Implementation of API uC access functions
 */

#include "blackhawk_tsc_access.h"
#include "blackhawk_tsc_common.h"
#include "blackhawk_tsc_functions.h"
#include "blackhawk_tsc_config.h"
#include "blackhawk_tsc_internal.h"
#include "blackhawk_tsc_internal_error.h"
#include "blackhawk_tsc_select_defns.h"


static uint16_t _plp_aperta_blackhawk_tsc_get_core_address(srds_access_t *sa__, uint16_t addr, uint8_t lane);

/******************************************************/
/*  Commands through Serdes FW DSC Command Interface  */
/******************************************************/

err_code_t plp_aperta_blackhawk_tsc_pmd_uc_cmd_return_immediate(srds_access_t *sa__, enum plp_aperta_srds_pmd_uc_cmd_enum cmd, uint8_t supp_info) {
  err_code_t err_code;
  uint16_t   cmddata;

  err_code = plp_aperta_blackhawk_tsc_INTERNAL_poll_uc_dsc_ready_for_cmd_equals_1(sa__, 1, cmd); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate blackhawk_tsc ready for command */
  if (err_code) {
    EFUN_PRINTF(("ERROR : DSC ready for command timed out (before cmd) cmd = %d, supp_info = x%02x err=%d !\n", cmd, supp_info, err_code));
    return (err_code);
  }
  /*EFUN(wr_uc_dsc_supp_info(supp_info)); */                   /* Supplement info field */
  /*EFUN(wr_uc_dsc_error_found(0x0)    ); */                   /* Clear error found field */
  /*EFUN(wr_uc_dsc_gp_uc_req(cmd)      ); */                   /* Set command code */
  /*EFUN(wr_uc_dsc_ready_for_cmd(0x0)  ); */                   /* Issue command, by clearing "ready for command" field */
  cmddata = (uint16_t)((((uint16_t)supp_info)<<8) | (uint16_t)cmd);        /* Combine writes to single write instead of 4 RMW */

  EFUN(reg_wr_DSC_A_DSC_UC_CTRL(cmddata));         

  return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_pmd_uc_cmd(srds_access_t *sa__, enum plp_aperta_srds_pmd_uc_cmd_enum cmd, uint8_t supp_info, uint32_t timeout_ms) {

  EFUN(plp_aperta_blackhawk_tsc_pmd_uc_cmd_return_immediate(sa__, cmd, supp_info));    /* Invoke blackhawk_tsc_uc_cmd and return control immediately */

  EFUN(plp_aperta_blackhawk_tsc_INTERNAL_poll_uc_dsc_ready_for_cmd_equals_1(sa__, timeout_ms, cmd)); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate blackhawk_tsc ready for command */
  
  return(ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_pmd_uc_cmd_with_data_return_immediate(srds_access_t *sa__, enum plp_aperta_srds_pmd_uc_cmd_enum cmd, uint8_t supp_info, uint16_t data) {
  uint16_t   cmddata;
  err_code_t err_code;

  err_code = plp_aperta_blackhawk_tsc_INTERNAL_poll_uc_dsc_ready_for_cmd_equals_1(sa__, 1, cmd); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate blackhawk_tsc ready for command */
  if (err_code) {
     EFUN_PRINTF(("ERROR : DSC ready for command timed out (before cmd) cmd = %d, supp_info = x%02x, data = x%04x err=%d !\n", cmd, supp_info, data,err_code));
    return (err_code);
  }

  EFUN(wr_uc_dsc_data(data));                                  /* Write value written to uc_dsc_data field */
  /*EFUN(wr_uc_dsc_supp_info(supp_info)); */                   /* Supplement info field */
  /*EFUN(wr_uc_dsc_error_found(0x0));     */                   /* Clear error found field */
  /*EFUN(wr_uc_dsc_gp_uc_req(cmd));       */                   /* Set command code */
  /*EFUN(wr_uc_dsc_ready_for_cmd(0x0));   */                   /* Issue command, by clearing "ready for command" field */
  cmddata = (uint16_t)((((uint16_t)supp_info)<<8) | (uint16_t)cmd);        /* Combine writes to single write instead of 4 RMW */

  EFUN(reg_wr_DSC_A_DSC_UC_CTRL(cmddata));         

  return(ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_pmd_uc_cmd_with_data(srds_access_t *sa__, enum plp_aperta_srds_pmd_uc_cmd_enum cmd, uint8_t supp_info, uint16_t data, uint32_t timeout_ms) {

    EFUN(plp_aperta_blackhawk_tsc_pmd_uc_cmd_with_data_return_immediate(sa__, cmd, supp_info, data)); /* Invoke blackhawk_tsc_uc_cmd and return control immediately */

    EFUN(plp_aperta_blackhawk_tsc_INTERNAL_poll_uc_dsc_ready_for_cmd_equals_1(sa__, timeout_ms, cmd)); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate blackhawk_tsc ready for command */
    return(ERR_CODE_NONE);
}

err_code_t plp_aperta_blackhawk_tsc_pmd_uc_control_return_immediate(srds_access_t *sa__, enum plp_aperta_srds_pmd_uc_ctrl_cmd_enum control) {
  return(plp_aperta_blackhawk_tsc_pmd_uc_cmd_return_immediate(sa__, CMD_UC_CTRL, (uint8_t) control));
}

err_code_t plp_aperta_blackhawk_tsc_pmd_uc_control(srds_access_t *sa__, enum plp_aperta_srds_pmd_uc_ctrl_cmd_enum control, uint32_t timeout_ms) {
  return(plp_aperta_blackhawk_tsc_pmd_uc_cmd(sa__, CMD_UC_CTRL, (uint8_t) control, timeout_ms));
}

err_code_t plp_aperta_blackhawk_tsc_pmd_uc_diag_cmd(srds_access_t *sa__, enum plp_aperta_srds_pmd_uc_diag_cmd_enum control, uint32_t timeout_ms) {
  return(plp_aperta_blackhawk_tsc_pmd_uc_cmd(sa__, CMD_DIAG_EN, (uint8_t) control, timeout_ms));
}

/************************************************************/
/*      Serdes IP RAM access - Lane RAM Variables           */
/*----------------------------------------------------------*/
/*   - through Micro Register Interface for PMD IPs         */
/*   - through Serdes FW DSC Command Interface for Gallardo */
/************************************************************/

/* This function returns the absolute address of lane RAM variables given the offset address and lane */
uint16_t plp_aperta_blackhawk_tsc_INTERNAL_get_lane_addr(srds_access_t *sa__, uint16_t addr, uint8_t lane) {
    uint16_t lane_var_ram_base=0, lane_var_ram_size=0;
    uint16_t lane_addr_offset = 0;

    uint16_t grp_ram_size=0,lane_count=0;
    srds_info_t * blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__);
#ifdef PHYMOD_SUPPORT
    if ( blackhawk_tsc_info_ptr == NULL ) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    } else if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    }
#endif /* PHYMOD_SUPPORT */
    CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, 0);
    lane_var_ram_base = (uint16_t)(blackhawk_tsc_info_ptr->lane_var_ram_base);
    lane_var_ram_size = (uint16_t)(blackhawk_tsc_info_ptr->lane_var_ram_size);
    grp_ram_size = blackhawk_tsc_info_ptr->grp_ram_size;
    lane_count = blackhawk_tsc_info_ptr->lane_count;
    lane_addr_offset = (uint16_t)(lane_var_ram_base+addr+((lane%lane_count)*lane_var_ram_size) + (grp_ram_size*plp_aperta_blackhawk_tsc_INTERNAL_grp_idx_from_lane(lane)));
    return(lane_addr_offset);
}

/* Micro RAM Lane Byte Read */
uint8_t plp_aperta_blackhawk_tsc_rdbl_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
    uint8_t rddata;
    uint16_t lane_addr_offset = 0;
    uint8_t lane;

    if(!err_code_p) {
        return(0);
    }

    {
        lane = blackhawk_tsc_get_physical_lane(sa__);
        lane_addr_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_lane_addr(sa__, addr,lane);

        EPSTM(rddata = plp_aperta_blackhawk_tsc_rdb_uc_ram(sa__, err_code_p, lane_addr_offset)); /* Use Micro register interface for reading RAM */
        return (rddata);
    }
}

/* Micro RAM Lane Byte Signed Read */
int8_t plp_aperta_blackhawk_tsc_rdbls_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
  return ((int8_t) plp_aperta_blackhawk_tsc_rdbl_uc_var(sa__, err_code_p, addr));
}

/* Micro RAM Lane Word Read */
uint16_t plp_aperta_blackhawk_tsc_rdwl_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
    uint16_t rddata;
    uint16_t lane_addr_offset = 0;
    uint8_t lane;

    if(!err_code_p) {
        return(0);
    }
    if (addr%2 != 0) {                                                                /* Validate even address */
        *err_code_p = ERR_CODE_INVALID_RAM_ADDR;
        return (0);
    }

    {
        lane = blackhawk_tsc_get_physical_lane(sa__);
        lane_addr_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_lane_addr(sa__, addr,lane);

        EPSTM(rddata = plp_aperta_blackhawk_tsc_rdw_uc_ram(sa__, err_code_p, lane_addr_offset)); /* Use Micro register interface for reading RAM */
        return (rddata);
    }
}


/* Micro RAM Lane Word Signed Read */
int16_t plp_aperta_blackhawk_tsc_rdwls_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
  return ((int16_t) plp_aperta_blackhawk_tsc_rdwl_uc_var(sa__, err_code_p, addr));
}

/* Micro RAM Lane Byte Write */
err_code_t plp_aperta_blackhawk_tsc_wrbl_uc_var(srds_access_t *sa__, uint16_t addr, uint8_t wr_val) {
    uint16_t lane_addr_offset = 0;
    uint8_t lane;


    {
        lane = blackhawk_tsc_get_physical_lane(sa__);
        lane_addr_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_lane_addr(sa__, addr,lane);

        return (plp_aperta_blackhawk_tsc_wrb_uc_ram(sa__, lane_addr_offset, wr_val));    /* Use Micro register interface for writing RAM */
    }
}

/* Micro RAM Lane Byte Signed Write */
err_code_t plp_aperta_blackhawk_tsc_wrbls_uc_var(srds_access_t *sa__, uint16_t addr, int8_t wr_val) {
  return (plp_aperta_blackhawk_tsc_wrbl_uc_var(sa__, addr, (uint8_t)wr_val));
}

/* Micro RAM Lane Word Write */
err_code_t plp_aperta_blackhawk_tsc_wrwl_uc_var(srds_access_t *sa__, uint16_t addr, uint16_t wr_val) {
    uint16_t lane_addr_offset = 0;
    uint8_t lane;

    if (addr%2 != 0) {                                                                /* Validate even address */
        return (blackhawk_tsc_error(sa__, ERR_CODE_INVALID_RAM_ADDR));
    }

    {
        lane = blackhawk_tsc_get_physical_lane(sa__);
        lane_addr_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_lane_addr(sa__, addr,lane);

        return (plp_aperta_blackhawk_tsc_wrw_uc_ram(sa__, lane_addr_offset, wr_val));    /* Use Micro register interface for writing RAM */
    }

}

/* Micro RAM Lane Word Signed Write */
err_code_t plp_aperta_blackhawk_tsc_wrwls_uc_var(srds_access_t *sa__, uint16_t addr, int16_t wr_val) {
  return (plp_aperta_blackhawk_tsc_wrwl_uc_var(sa__, addr, (uint16_t)wr_val));
}


/************************************************************/
/*      Serdes IP RAM access - Micro RAM Variables           */
/*----------------------------------------------------------*/
/*   - through Micro Register Interface for PMD IPs         */
/************************************************************/

/* This function returns the absolute address of uc RAM variables given the offset address and lane */
uint16_t plp_aperta_blackhawk_tsc_INTERNAL_get_uc_address(srds_access_t *sa__, uint16_t addr, uint8_t lane) {
    uint16_t micro_var_ram_base=0, grp_ram_size=0;
    uint16_t uc_addr_offset = 0;
    srds_info_t * blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__);
#ifdef PHYMOD_SUPPORT
    if ( blackhawk_tsc_info_ptr == NULL ) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    } else if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    }
#endif /* PHYMOD_SUPPORT */
    CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, 0);

    micro_var_ram_base = (uint16_t)(blackhawk_tsc_info_ptr->micro_var_ram_base);
    grp_ram_size = blackhawk_tsc_info_ptr->grp_ram_size;
    uc_addr_offset = (uint16_t)(micro_var_ram_base + addr + (grp_ram_size*plp_aperta_blackhawk_tsc_INTERNAL_grp_idx_from_lane(lane)));
    return(uc_addr_offset);
}

/* Micro RAM UC Byte Read */
uint8_t plp_aperta_blackhawk_tsc_rdbuc_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
  uint8_t  rddata;
  uint16_t uc_addr_offset = 0;
  uint8_t  lane;

  if(!err_code_p) {
      return(0);
  }

  lane = blackhawk_tsc_get_physical_lane(sa__);
  uc_addr_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_uc_address(sa__, addr,lane);

  EPSTM(rddata = plp_aperta_blackhawk_tsc_rdb_uc_ram(sa__, err_code_p, uc_addr_offset)); /* Use Micro register interface for reading RAM */
  return (rddata);
}

/* Micro RAM UC Byte Signed Read */
int8_t plp_aperta_blackhawk_tsc_rdbucs_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
  return ((int8_t) plp_aperta_blackhawk_tsc_rdbuc_uc_var(sa__, err_code_p, addr));
}

/* Micro RAM UC Word Read */
uint16_t plp_aperta_blackhawk_tsc_rdwuc_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
  uint16_t rddata;
  uint16_t uc_addr_offset = 0;
  uint8_t  lane;

  if(!err_code_p) {
      return(0);
  }
  if (addr%2 != 0) {                                                                /* Validate even address */
      *err_code_p = ERR_CODE_INVALID_RAM_ADDR;
      return (0);
  }

  lane = blackhawk_tsc_get_physical_lane(sa__);
  uc_addr_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_uc_address(sa__, addr,lane);

  EPSTM(rddata = plp_aperta_blackhawk_tsc_rdw_uc_ram(sa__, err_code_p, uc_addr_offset)); /* Use Micro register interface_t for reading RAM */
  return (rddata);
}


/* Micro RAM UC Word Signed Read */
int16_t plp_aperta_blackhawk_tsc_rdwucs_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
  return ((int16_t) plp_aperta_blackhawk_tsc_rdwuc_uc_var(sa__, err_code_p, addr));
}

/* Micro RAM UC Byte Write */
err_code_t plp_aperta_blackhawk_tsc_wrbuc_uc_var(srds_access_t *sa__, uint16_t addr, uint8_t wr_val) {

  uint16_t uc_addr_offset = 0;
  uint8_t  lane;

  lane = blackhawk_tsc_get_physical_lane(sa__);
  uc_addr_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_uc_address(sa__, addr,lane);

  return (plp_aperta_blackhawk_tsc_wrb_uc_ram(sa__, uc_addr_offset, wr_val));    /* Use Micro register interface_t for writing RAM */
}

/* Micro RAM UC Byte Signed Write */
err_code_t plp_aperta_blackhawk_tsc_wrbucs_uc_var(srds_access_t *sa__, uint16_t addr, int8_t wr_val) {
  return (plp_aperta_blackhawk_tsc_wrbuc_uc_var(sa__, addr, (uint8_t)wr_val));
}

/* Micro RAM UC Word Write */
err_code_t plp_aperta_blackhawk_tsc_wrwuc_uc_var(srds_access_t *sa__, uint16_t addr, uint16_t wr_val) {
  uint16_t uc_addr_offset = 0;
  uint8_t  lane;

  if (addr%2 != 0) {                                                                /* Validate even address */
      return (blackhawk_tsc_error(sa__, ERR_CODE_INVALID_RAM_ADDR));
  }
  lane = blackhawk_tsc_get_physical_lane(sa__);
  uc_addr_offset = plp_aperta_blackhawk_tsc_INTERNAL_get_uc_address(sa__, addr,lane);

  return (plp_aperta_blackhawk_tsc_wrw_uc_ram(sa__, uc_addr_offset, wr_val));    /* Use Micro register interface_t for writing RAM */
}

/* Micro RAM UC Word Signed Write */
err_code_t plp_aperta_blackhawk_tsc_wrwucs_uc_var(srds_access_t *sa__, uint16_t addr, int16_t wr_val) {
  return (plp_aperta_blackhawk_tsc_wrwuc_uc_var(sa__, addr,(uint16_t)wr_val));
}



/************************************************************/
/*      Serdes IP RAM access - Core RAM Variables           */
/*----------------------------------------------------------*/
/*   - through Micro Register Interface for PMD IPs         */
/*   - through Serdes FW DSC Command Interface for Gallardo */
/************************************************************/

/* This function returns the absolute address of core RAM variables given the offset address and lane */
static uint16_t _plp_aperta_blackhawk_tsc_get_core_address(srds_access_t *sa__, uint16_t addr, uint8_t lane) {
    uint16_t core_var_ram_base=0;
    uint16_t core_addr_offset = 0;

    srds_info_t * blackhawk_tsc_info_ptr = plp_aperta_blackhawk_tsc_INTERNAL_get_blackhawk_tsc_info_ptr_with_check(sa__);
#ifdef PHYMOD_SUPPORT
    if ( blackhawk_tsc_info_ptr == NULL ) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    } else if (blackhawk_tsc_info_ptr->signature != SRDS_INFO_SIGNATURE) {
        plp_aperta_blackhawk_tsc_init_blackhawk_tsc_info(sa__);
    }
#endif /* PHYMOD_SUPPORT */
    CHECK_AND_RETURN_IF_NULL(blackhawk_tsc_info_ptr, 0);
    core_var_ram_base = (uint16_t)(blackhawk_tsc_info_ptr->core_var_ram_base);
    core_addr_offset = (uint16_t)(core_var_ram_base + addr);

    UNUSED(lane);
    return(core_addr_offset);
}

/* Micro RAM Core Byte Read */
uint8_t plp_aperta_blackhawk_tsc_rdbc_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint8_t addr) {

    uint8_t  rddata;
    uint8_t  lane;
    uint16_t core_addr_offset;
    if(!err_code_p) {
        return(0);
    }

    {
        lane = blackhawk_tsc_get_physical_lane(sa__);
        core_addr_offset = _plp_aperta_blackhawk_tsc_get_core_address(sa__, addr,lane);
        EPSTM(rddata = plp_aperta_blackhawk_tsc_rdb_uc_ram(sa__, err_code_p, core_addr_offset));    /* Use Micro register interface for reading RAM */
        return (rddata);
    }
}

/* Micro RAM Core Byte Signed Read */
int8_t plp_aperta_blackhawk_tsc_rdbcs_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint8_t addr) {
  return ((int8_t) plp_aperta_blackhawk_tsc_rdbc_uc_var(sa__, err_code_p, addr));
}

/* Micro RAM Core Word Read */
uint16_t plp_aperta_blackhawk_tsc_rdwc_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint8_t addr) {
    uint16_t rddata;
    uint8_t  lane;
    uint16_t core_addr_offset;

    if(!err_code_p) {
        return(0);
    }
    if (addr%2 != 0) {                                                                /* Validate even address */
        *err_code_p = ERR_CODE_INVALID_RAM_ADDR;
        return (0);
    }

    {
        lane = blackhawk_tsc_get_physical_lane(sa__);
        core_addr_offset = _plp_aperta_blackhawk_tsc_get_core_address(sa__, addr,lane);

        EPSTM(rddata = plp_aperta_blackhawk_tsc_rdw_uc_ram(sa__, err_code_p, core_addr_offset));    /* Use Micro register interface_t for reading RAM */
        return (rddata);
    }
}

/* Micro RAM Core Word Signed Read */
int16_t plp_aperta_blackhawk_tsc_rdwcs_uc_var(srds_access_t *sa__, err_code_t *err_code_p, uint8_t addr) {
  return ((int16_t) plp_aperta_blackhawk_tsc_rdwc_uc_var(sa__, err_code_p, addr));
}

/* Micro RAM Core Byte Write  */
err_code_t plp_aperta_blackhawk_tsc_wrbc_uc_var(srds_access_t *sa__, uint8_t addr, uint8_t wr_val) {
    uint8_t  lane;
    uint16_t core_addr_offset;

    {
        lane = blackhawk_tsc_get_physical_lane(sa__);
        core_addr_offset = _plp_aperta_blackhawk_tsc_get_core_address(sa__, addr,lane);

        return (plp_aperta_blackhawk_tsc_wrb_uc_ram(sa__, core_addr_offset, wr_val));               /* Use Micro register interface for writing RAM */
    }

}


/* Micro RAM Core Byte Signed Write */
err_code_t plp_aperta_blackhawk_tsc_wrbcs_uc_var(srds_access_t *sa__, uint8_t addr, int8_t wr_val) {
  return (plp_aperta_blackhawk_tsc_wrbc_uc_var(sa__, addr, (uint8_t)wr_val));
}

/* Micro RAM Core Word Write  */
err_code_t plp_aperta_blackhawk_tsc_wrwc_uc_var(srds_access_t *sa__, uint8_t addr, uint16_t wr_val) {
    uint8_t  lane;
    uint16_t core_addr_offset;

    {
        if (addr%2 != 0) {                                                              /* Validate even address */
            return (blackhawk_tsc_error(sa__, ERR_CODE_INVALID_RAM_ADDR));
        }

        lane = blackhawk_tsc_get_physical_lane(sa__);
        core_addr_offset = _plp_aperta_blackhawk_tsc_get_core_address(sa__, addr,lane);
        return (plp_aperta_blackhawk_tsc_wrw_uc_ram(sa__, core_addr_offset, wr_val));             /* Use Micro register interface for writing RAM */
    }
}

/* Micro RAM Core Word Signed Write */
err_code_t plp_aperta_blackhawk_tsc_wrwcs_uc_var(srds_access_t *sa__, uint8_t addr, int16_t wr_val) {
  return(plp_aperta_blackhawk_tsc_wrwc_uc_var(sa__, addr,(uint16_t)wr_val));
}

/*********************************************************/
/*  Program RAM access through Micro Register Interface  */
/*********************************************************/

/* Micro Program Ram Long Read */
uint32_t plp_aperta_blackhawk_tsc_rd_long_uc_prog_ram(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
   uint32_t rddata;
   if(!err_code_p) {
        return(0);
   }
   *err_code_p = ERR_CODE_NONE;
   EPFUN(wrc_micro_autoinc_rdaddr_en(1));
   EPFUN(wrc_micro_ra_rddatasize(0x1));                 /* Select 16bit read datasize */
   EPFUN(wrc_micro_ra_rdaddr_msw(0));                   /* Upper 16bits of ROM address to be read */
   EPFUN(wrc_micro_ra_rdaddr_lsw(addr));                /* Lower 16bits of ROM address to be read */
   EPSTM(rddata = rdc_micro_ra_rddata_lsw());           /* 16bit read data */
   EPSTM(rddata |= (uint32_t)((rdc_micro_ra_rddata_lsw() << 16)));  /* 16bit read data */
   return (rddata);
}

/* Micro Program RAM Block Read */
err_code_t plp_aperta_blackhawk_tsc_rdblk_uc_prog_ram(srds_access_t *sa__, uint8_t *mem, uint16_t addr, uint16_t cnt) {
    blackhawk_tsc_INTERNAL_rdblk_callback_arg_t arg;

    if(!mem) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    arg.mem_ptr = mem;
    arg.sa = sa__;
    return plp_aperta_blackhawk_tsc_INTERNAL_rdblk_uc_generic_ram(sa__, addr, cnt, 0, cnt, &arg, plp_aperta_blackhawk_tsc_INTERNAL_rdblk_callback);
}

/*************************************************/
/*  RAM access through Micro Register Interface  */
/*************************************************/

/* Micro RAM Long Write */
err_code_t plp_aperta_blackhawk_tsc_wr_long_uc_ram(srds_access_t *sa__, uint16_t offset, uint32_t value) {
    EFUN(plp_aperta_blackhawk_tsc_wrw_uc_ram(sa__, (uint16_t)(offset+2), (uint16_t)(value >> 16)));
    EFUN(plp_aperta_blackhawk_tsc_wrw_uc_ram(sa__, offset, value & 0xFFFF));
    return (ERR_CODE_NONE);
}
/* Micro RAM Word Write */
err_code_t plp_aperta_blackhawk_tsc_wrw_uc_ram(srds_access_t *sa__, uint16_t addr, uint16_t wr_val) {

    EFUN(wrc_micro_autoinc_wraddr_en(0));
    EFUN(wrc_micro_ra_wrdatasize(0x1));                 /* Select 16bit write datasize */
    EFUN(wrc_micro_ra_wraddr_msw(0x2000));              /* Upper 16bits of RAM address to be written to */
    EFUN(wrc_micro_ra_wraddr_lsw(addr));                /* Lower 16bits of RAM address to be written to */
    EFUN(wrc_micro_ra_wrdata_lsw(wr_val));              /* uC RAM lower 16bits write data */
    return (ERR_CODE_NONE);
}

/* Micro RAM Byte Write */
err_code_t plp_aperta_blackhawk_tsc_wrb_uc_ram(srds_access_t *sa__, uint16_t addr, uint8_t wr_val) {
    EFUN(wrc_micro_autoinc_wraddr_en(0));
    EFUN(wrc_micro_ra_wrdatasize(0x0));                 /* Select 8bit write datasize */
    EFUN(wrc_micro_ra_wraddr_msw(0x2000));              /* Upper 16bits of RAM address to be written to */
    EFUN(wrc_micro_ra_wraddr_lsw(addr));                /* Lower 16bits of RAM address to be written to */
    EFUN(wrc_micro_ra_wrdata_lsw(wr_val));              /* uC RAM lower 16bits write data */
    return (ERR_CODE_NONE);
}

/* Micro RAM Long Read */
uint32_t plp_aperta_blackhawk_tsc_rd_long_uc_ram(srds_access_t *sa__, err_code_t *err, uint16_t offset) {
    uint32_t result;
    ESTM(result = (uint32_t)(plp_aperta_blackhawk_tsc_rdw_uc_ram(sa__, err, (uint16_t)(offset+2)) << 16));
    ESTM(result |= plp_aperta_blackhawk_tsc_rdw_uc_ram(sa__, err, offset) & 0xFFFF);
    return result;
}
/* Micro RAM Word Read */
uint16_t plp_aperta_blackhawk_tsc_rdw_uc_ram(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
   uint16_t rddata;
   if(!err_code_p) {
        return(0);
   }
   *err_code_p = ERR_CODE_NONE;
   EPFUN(wrc_micro_autoinc_rdaddr_en(0));
   EPFUN(wrc_micro_ra_rddatasize(0x1));                 /* Select 16bit read datasize */
   EPFUN(wrc_micro_ra_rdaddr_msw(0x2000));              /* Upper 16bits of RAM address to be read */
   EPFUN(wrc_micro_ra_rdaddr_lsw(addr));                /* Lower 16bits of RAM address to be read */
   EPSTM(rddata = rdc_micro_ra_rddata_lsw());           /* 16bit read data */
   return (rddata);
}

/* Micro RAM Byte Read */
uint8_t plp_aperta_blackhawk_tsc_rdb_uc_ram(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
    uint8_t rddata;
    if(!err_code_p) {
        return(0);
    }
    *err_code_p = ERR_CODE_NONE;
    EPFUN(wrc_micro_autoinc_rdaddr_en(0));
    EPFUN(wrc_micro_ra_rddatasize(0x0));                 /* Select 8bit read datasize */
    EPFUN(wrc_micro_ra_rdaddr_msw(0x2000));              /* Upper 16bits of RAM address to be read */
    EPFUN(wrc_micro_ra_rdaddr_lsw(addr));                /* Lower 16bits of RAM address to be read */
    EPSTM(rddata = (uint8_t) rdc_micro_ra_rddata_lsw()); /* 16bit read data */
    return (rddata);
}

/* Micro RAM Word Signed Write */
err_code_t plp_aperta_blackhawk_tsc_wrws_uc_ram(srds_access_t *sa__, uint16_t addr, int16_t wr_val) {
    return (plp_aperta_blackhawk_tsc_wrw_uc_ram(sa__, addr, (uint16_t)wr_val));
}

/* Micro RAM Byte Signed Write */
err_code_t plp_aperta_blackhawk_tsc_wrbs_uc_ram(srds_access_t *sa__, uint16_t addr, int8_t wr_val) {
    return (plp_aperta_blackhawk_tsc_wrb_uc_ram(sa__, addr, (uint8_t)wr_val));
}

/* Micro RAM Word Signed Read */
int16_t plp_aperta_blackhawk_tsc_rdws_uc_ram(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
    return ((int16_t)plp_aperta_blackhawk_tsc_rdw_uc_ram(sa__, err_code_p, addr));
}

/* Micro RAM Byte Signed Read */
int8_t plp_aperta_blackhawk_tsc_rdbs_uc_ram(srds_access_t *sa__, err_code_t *err_code_p, uint16_t addr) {
    return ((int8_t)plp_aperta_blackhawk_tsc_rdb_uc_ram(sa__, err_code_p, addr));
}

/* Micro RAM Block Read */
err_code_t plp_aperta_blackhawk_tsc_rdblk_uc_ram(srds_access_t *sa__, uint8_t *mem, uint16_t addr, uint16_t cnt) {
    blackhawk_tsc_INTERNAL_rdblk_callback_arg_t arg;

    if(!mem) {
        return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }

    arg.mem_ptr = mem;
    arg.sa = sa__;
    return plp_aperta_blackhawk_tsc_INTERNAL_rdblk_uc_generic_ram(sa__, (uint32_t)addr | 0x20000000, cnt, 0, cnt, &arg, plp_aperta_blackhawk_tsc_INTERNAL_rdblk_callback);
}

/**************************/
/*  Temperature reading   */
/**************************/

err_code_t plp_aperta_blackhawk_tsc_read_die_temperature (srds_access_t *sa__, int16_t *die_temp) {

    uint16_t die_temp_sensor_reading;

    ESTM(die_temp_sensor_reading=rdc_micro_pvt_tempdata_rmi());
    *die_temp = (int16_t)(_bin_to_degC(die_temp_sensor_reading));

    return(ERR_CODE_NONE);
}


err_code_t plp_aperta_blackhawk_tsc_read_die_temperature_double (srds_access_t *sa__, USR_DOUBLE *die_temp) {
#ifdef SERDES_API_FLOATING_POINT
    uint16_t die_temp_sensor_reading;

    ESTM(die_temp_sensor_reading=rdc_micro_pvt_tempdata_rmi());

    *die_temp = _bin_to_degC_double(die_temp_sensor_reading);

    return(ERR_CODE_NONE);
#else
    *die_temp = 0;
    EFUN_PRINTF((" Function 'plp_aperta_blackhawk_tsc_read_die_temperature_double' needs SERDES_API_FLOATING_POINT defined to operate \n"));
    return(blackhawk_tsc_error(sa__, ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
#endif
}

