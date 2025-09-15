/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2025
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

#ifndef __CLX_CLX_TYPES_H__
#define __CLX_CLX_TYPES_H__

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

typedef uint32_t clx_port_t;
typedef uint8_t clx_mac_t[6];

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN) || \
    defined(CLX_EN_64BIT_ADDR)
typedef unsigned long long int clx_addr_t;
typedef unsigned long long int clx_huge_t;

#define clx_addr_64_hi(__addr__)  ((uint32_t)((__addr__) >> 32))
#define clx_addr_64_low(__addr__) ((uint32_t)((__addr__) & 0xFFFFFFFF))
#define clx_addr_32_to_64(__hi32__, __low32__) \
    (((unsigned long long int)(__low32__)) | (((unsigned long long int)(__hi32__)) << 32))
#else
typedef unsigned int clx_addr_t;
typedef unsigned int clx_huge_t;

#define clx_addr_64_hi(__addr__)               (0)
#define clx_addr_64_low(__addr__)              (__addr__)
#define clx_addr_32_to_64(__hi32__, __low32__) (__low32__)
#endif

// NOTICE: This is a enum for ioctrl error code same as clx_error_no_t
//        it should be updated after ioctrl is updated
typedef enum clx_ioctl_error_no_e {
    CLX_IOCTL_E_OK = 0,          /* Ok and no error */
    CLX_IOCTL_E_BAD_PARAMETER,   /* Parameter is wrong */
    CLX_IOCTL_E_NO_MEMORY,       /* No memory is available */
    CLX_IOCTL_E_TABLE_FULL,      /* Table is full */
    CLX_IOCTL_E_ENTRY_NOT_FOUND, /* Entry is not found */
    CLX_IOCTL_E_ENTRY_EXISTS,    /* Entry already exists */
    CLX_IOCTL_E_NOT_SUPPORT,     /* Feature is not supported */
    CLX_IOCTL_E_ALREADY_INITED,  /* Module is reinitialized */
    CLX_IOCTL_E_NOT_INITED,      /* Module is not initialized */
    CLX_IOCTL_E_OTHERS,          /* Other errors */
    CLX_IOCTL_E_ENTRY_IN_USE,    /* Entry is in use */
    CLX_IOCTL_E_TIMEOUT,         /* Time out error */
    CLX_IOCTL_E_OP_INVALID,      /* Operation is invalid */
    CLX_IOCTL_E_OP_STOPPED,      /* Operation is stopped by user callback */
    CLX_IOCTL_E_OP_INCOMPLETE,   /* Operation is incomplete */
    CLX_IOCTL_E_TRY_AGAIN,       /* Opertion not exec, try again */
    CLX_IOCTL_E_BAD_CASE,        /* Switch case err */
    CLX_IOCTL_E_LAST
} clx_ioctl_error_no_t;

#endif // __CLX_CLX_TYPES_H__