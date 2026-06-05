/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/*********************************************************************************
 *********************************************************************************
 *  File Name  :  peregrine5_pc_select_defns.h
 *  Created On :  29 Sep 2015
 *  Created By :  Brent Roberts
 *  Description:  Select header files for IP-specific definitions
 *  Revision   :  
 *
 *********************************************************************************
 ********************************************************************************/

 /** @file
 * Select IP files to include for API
 */

#ifndef PEREGRINE5_PC_API_SELECT_DEFNS_H
#define PEREGRINE5_PC_API_SELECT_DEFNS_H

#include "peregrine5_pc_ipconfig.h"
#include "peregrine5_pc_field_access.h"

#   include "peregrine5_api_uc_common.h"


/****************************************************************************
 * @name Defines for Serdes Unified APIs (UAPI)
 *
 * Unified APIs combine multiple IP variants into single UAPI.
 ****************************************************************************/
#define PEREGRINE5_PC_UAPI_INIT                  srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__); \
                                          SRDS_INFO_PTR_NULL_CHECK;
#define PEREGRINE5_PC_UAPI_WR_INIT               srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__); \
                                          SRDS_INFO_PTR_NULL_CHECK;
#define PEREGRINE5_PC_UAPI_RD_INIT               srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__); \
                                          if(peregrine5_pc_info_ptr == NULL) {                                                                     \
                                              *err_code_p = ERR_CODE_INFO_TABLE_ERROR;                                                      \
                                              return 0;                                                                                     \
                                          }
#define PEREGRINE5_PC_UAPI_EX_INIT               srds_info_t const * const peregrine5_pc_info_ptr = plp_aperta2_peregrine5_pc_INTERNAL_get_peregrine5_pc_info_ptr_with_check(sa__); \
                                          if(peregrine5_pc_info_ptr == NULL) {                                                                     \
                                              return 0;                                                                                     \
                                          }
#define PEREGRINE5_PC_UAPI_SWITCH                (peregrine5_pc_info_ptr->silicon_version)
#define PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_1 (0x1)
#define PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_2 (0x2)
#define PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_6 (0x3)
#define PEREGRINE5_PC_UAPI_CASE0                (PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_1)
#define PEREGRINE5_PC_UAPI_CASE1                (PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_2)
#define PEREGRINE5_PC_UAPI_CASE2                (PEREGRINE5_PC_UAPI_A0_AFE_WITH_REVID2_6)
#define PEREGRINE5_PC_UAPI_TERMINATE

/****************************************************************************
 * @name Register Access Macro Inclusions
 *
 * All cores provide access to hardware control/status registers.
 ****************************************************************************/
/**@{*/

/**
 * This build includes register access macros for the PEREGRINE5_PC core.
 */
#include "peregrine5_pc_fields.h"

/**@}*/


/****************************************************************************
 * @name RAM Access Macro Inclusions
 *
 * Some cores also provide access to firmware control/status RAM variables.
 ****************************************************************************/
/**@{*/

/**
 * This build includes macros to access Peregrine5 microcode RAM variables.
 */
#include "peregrine5_api_uc_vars_rdwr_defns.h"
/**@}*/

#endif
