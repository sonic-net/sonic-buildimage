/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2023
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

/* FILE NAME:   clx_error.h
 * PURPOSE:
 *      Define the generic error code on CLX SDK.
 * NOTES:
 */

#ifndef CLX_ERROR_H
#define CLX_ERROR_H

/* INCLUDE FILE DECLARATIONS
 */
#include <clx_types.h>

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

typedef enum {
    CLX_E_OK = 0,          /* Ok and no error */
    CLX_E_BAD_PARAMETER,   /* Parameter is wrong */
    CLX_E_NO_MEMORY,       /* No memory is available */
    CLX_E_TABLE_FULL,      /* Table is full */
    CLX_E_ENTRY_NOT_FOUND, /* Entry is not found */
    CLX_E_ENTRY_EXISTS,    /* Entry already exists */
    CLX_E_NOT_SUPPORT,     /* Feature is not supported */
    CLX_E_ALREADY_INITED,  /* Module is reinitialized */
    CLX_E_NOT_INITED,      /* Module is not initialized */
    CLX_E_OTHERS,          /* Other errors */
    CLX_E_ENTRY_IN_USE,    /* Entry is in use */
    CLX_E_TIMEOUT,         /* Time out error */
    CLX_E_OP_INVALID,      /* Operation is invalid */
    CLX_E_OP_STOPPED,      /* Operation is stopped by user callback */
    CLX_E_OP_INCOMPLETE,   /* Operation is incomplete */
    CLX_E_TRY_AGAIN,       /* Opertion not exec, try again */
    CLX_E_LAST
} CLX_ERROR_NO_T;

#define CHECK_ERROR(__rc__)                                                                  \
    do {                                                                                     \
        CLX_ERROR_NO_T __rc = (__rc__);                                                      \
        if (__rc != CLX_E_OK) {                                                              \
            osal_printf("***Error***, %s:%d" #__rc__ "=%d\n", __FUNCTION__, __LINE__, __rc); \
            return __rc;                                                                     \
        }                                                                                    \
    } while (0)

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
/**
 * @brief To obtain the error string of the specified error code
 *
 * support_chip all
 *
 * @param [in]     cause    - The specified error code
 * @return    Pointer to the target error string
 */
C8_T *
clx_error_getString(const CLX_ERROR_NO_T cause);

#endif /* CLX_ERROR_H */
