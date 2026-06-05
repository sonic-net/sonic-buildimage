/*********************************************************************************
 *********************************************************************************
 *  File Name  :  blackhawk_tsc_select_defns.h
 *  Created On :  29 Sep 2015
 *  Created By :  Brent Roberts
 *  Description:  Select header files for IP-specific definitions
 *  company private proprietary, and trade secret.
 * $Copyright: (c) 2020 Broadcom.                                               *
 * Broadcom Proprietary and Confidential. All rights reserved.$                 *
 *
 *********************************************************************************
 ********************************************************************************/

 /** @file
 * Select IP files to include for API
 */

#ifndef BLACKHAWK_TSC_API_SELECT_DEFNS_H
#define BLACKHAWK_TSC_API_SELECT_DEFNS_H

#include "blackhawk_tsc_ipconfig.h"
#include "blackhawk_tsc_field_access.h"

#   include "blackhawk_api_uc_common.h"

/****************************************************************************
 * @name Register Access Macro Inclusions
 *
 * All cores provide access to hardware control/status registers.
 ****************************************************************************/
/**@{*/

/**
 * This build includes register access macros for the BLACKHAWK_TSC core.
 */
#include "blackhawk_tsc_fields.h"

/**@}*/


/****************************************************************************
 * @name RAM Access Macro Inclusions
 *
 * Some cores also provide access to firmware control/status RAM variables.
 ****************************************************************************/
/**@{*/

/**
 * This build includes macros to access Blackhawk microcode RAM variables.
 */
#include "blackhawk_api_uc_vars_rdwr_defns.h"
/**@}*/

#endif
