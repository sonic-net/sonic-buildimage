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

/* FILE NAME:  osal_mdc.h
 * PURPOSE:
 * 1. Provide device operate from AML interface
 * NOTES:
 *
 */

#ifndef OSAL_MDC_H
#define OSAL_MDC_H

/* INCLUDE FILE DECLARATIONS */
#ifndef CLX_LINUX_KERNEL_MODE
#ifndef __KERNEL__
#include <cmlib/cmlib_list.h>
#endif
#endif
#include <clx_types.h>
#include <aml/aml.h>

#define OSAL_MDC_DRIVER_NAME           "clx_dev"
#define OSAL_MDC_DRIVER_MISC_MAJOR_NUM (10)
#define OSAL_MDC_DRIVER_MISC_MINOR_NUM (250)
#define OSAL_MDC_PCI_BUS_WIDTH         (4)

#define OSAL_MDC_DMA_LIST_SZ_UNLIMITED (0)
#define OSAL_MDC_DMA_LIST_NAME         "RSRV_DMA"
#define OSAL_MDC_DMA_SEMAPHORE_NAME    "DMALIST"
#define OSAL_MDC_DEV_FILE_PATH         "/dev/" OSAL_MDC_DRIVER_NAME

#define OSAL_MDC_MAX_CHIPS_PER_SYSTEM (16)
#define OSAL_MDC_ISR_MAX_NUM          (32)
#define OSAL_MDC_INTR_VALID_CODE      (0xABCD900D)
#define OSAL_MDC_INTR_INVALID_MSI     (0xDEADDEAD)

/* This flag value will be specified when user inserts kernel module. */
#define OSAL_DBG_CRIT       (0x1UL << 0)
#define OSAL_DBG_ERR        (0x1UL << 1)
#define OSAL_DBG_WARN       (0x1UL << 2)
#define OSAL_DBG_INFO       (0x1UL << 3)
#define OSAL_DBG_DEBUG      (0x1UL << 4)
#define OSAL_DBG_TX         (0x1UL << 5)
#define OSAL_DBG_RX         (0x1UL << 6)
#define OSAL_DBG_TX_PAYLOAD (0x1UL << 7)
#define OSAL_DBG_RX_PAYLOAD (0x1UL << 8)
#define OSAL_DBG_INTF       (0x1UL << 9)
#define OSAL_DBG_PROFILE    (0x1UL << 10)
#define OSAL_DBG_COMMON     (0x1UL << 11)
#define OSAL_DBG_NETLINK    (0x1UL << 12)
#define OSAL_DBG_INTR       (0x1UL << 13)

extern UI32_T verbosity;
#ifdef __KERNEL__
#define OSAL_PRINT(__flag__, fmt, ...)                                             \
    do {                                                                           \
        if (0 != ((__flag__) & (verbosity))) {                                     \
            printk("CLX_KERN %s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                          \
    } while (0)
#else
#define OSAL_PRINT(__flag__, fmt, ...)                                    \
    do {                                                                  \
        if (0 != ((__flag__) & (verbosity))) {                            \
            printf("%s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                 \
    } while (0)
#endif

#define OSAL_CHECK_PTR(__ptr__)                                          \
    do {                                                                 \
        if (NULL == (__ptr__)) {                                         \
            OSAL_PRINT(OSAL_DBG_CRIT, "%s is null pointer\n", #__ptr__); \
            return (CLX_E_BAD_PARAMETER);                                \
        }                                                                \
    } while (0)
/* NAMING CONSTANT DECLARATIONS
 */
/* linked list node */
#if defined(CLX_LINUX_KERNEL_MODE)

typedef struct OSAL_MDC_LIST_NODE_S {
    void *ptr_data;                        /* node data                   */
    struct OSAL_MDC_LIST_NODE_S *ptr_next; /* point to next link node     */
    struct OSAL_MDC_LIST_NODE_S *ptr_prev; /* point to previous link node */
} OSAL_MDC_LIST_NODE_T;

/* linked list head */
typedef struct OSAL_MDC_LIST_S {
    OSAL_MDC_LIST_NODE_T *ptr_head_node; /* linked list head node   */
    OSAL_MDC_LIST_NODE_T *ptr_tail_node; /* linked list tail node   */
    UI32_T capacity;                     /* max count of nodes in list
                                          * size=0: the capacity is unlimited.
                                          * size>0: the capacity is limited.
                                          */
    UI32_T node_cnt;                     /* the count of nodes in the list */
} OSAL_MDC_LIST_T;

#endif /* End of defined(CLX_LINUX_KERNEL_MODE) */

typedef struct {
    CLX_ADDR_T phy_addr;
    void *ptr_virt_addr;
    CLX_ADDR_T size;
    CLX_ADDR_T bus_addr;

#if defined(CLX_EN_DMA_RESERVED)
    BOOL_T available;
#endif

} OSAL_MDC_DMA_NODE_T;

typedef struct {
#if defined(CLX_EN_DMA_RESERVED)
    void *ptr_rsrv_virt_addr;
    CLX_ADDR_T rsrv_phy_addr;
    CLX_ADDR_T rsrv_size;
#else
    struct device *ptr_dma_dev; /* for allocate/free system memory */
#endif

#if defined(CLX_LINUX_KERNEL_MODE)
    OSAL_MDC_LIST_T *ptr_dma_list;
#else
#ifndef __KERNEL__
    CMLIB_LIST_T *ptr_dma_list;
#else
    void *ptr_dma_list;
#endif
#endif

    CLX_SEMAPHORE_ID_T sema;

} OSAL_MDC_DMA_INFO_T;

#if defined(CLX_LINUX_USER_MODE)

/* Data type of IOCTL argument for DMA management */
typedef struct {
#if defined(CLX_EN_DMA_RESERVED)
    CLX_ADDR_T rsrv_dma_phy_addr; /* information of reserved memory */
    CLX_ADDR_T rsrv_dma_size;
#else
    CLX_ADDR_T phy_addr; /* information of system memory */
    CLX_ADDR_T size;
    CLX_ADDR_T bus_addr;
#endif
} OSAL_MDC_IOCTL_DMA_DATA_T;

/* Data type of IOCTL argument for device initialization */
#pragma pack(push, 1)
typedef struct {
    AML_DEV_ID_T id[OSAL_MDC_MAX_CHIPS_PER_SYSTEM];
    CLX_ADDR_T pci_mmio_phy_start[OSAL_MDC_MAX_CHIPS_PER_SYSTEM];
    CLX_ADDR_T pci_mmio_size[OSAL_MDC_MAX_CHIPS_PER_SYSTEM];
    UI32_T dev_num;
} OSAL_MDC_IOCTL_DEV_DATA_T;
#pragma pack(pop)

typedef enum {
    OSAL_MDC_IOCTL_TYPE_MDC_INIT_DEV = 0,
    OSAL_MDC_IOCTL_TYPE_MDC_DEINIT_DEV,
    OSAL_MDC_IOCTL_TYPE_MDC_INIT_RSRV_DMA_MEM,
    OSAL_MDC_IOCTL_TYPE_MDC_DEINIT_RSRV_DMA_MEM,
    OSAL_MDC_IOCTL_TYPE_MDC_ALLOC_SYS_DMA_MEM,
    OSAL_MDC_IOCTL_TYPE_MDC_FREE_SYS_DMA_MEM,
    OSAL_MDC_IOCTL_TYPE_MDC_CONNECT_ISR,
    OSAL_MDC_IOCTL_TYPE_MDC_DISCONNECT_ISR,
    OSAL_MDC_IOCTL_TYPE_MDC_SAVE_PCI_CONFIG,
    OSAL_MDC_IOCTL_TYPE_MDC_RESTORE_PCI_CONFIG,
    /* network interface */
    OSAL_MDC_IOCTL_TYPE_NETIF_CREATE_INTF,
    OSAL_MDC_IOCTL_TYPE_NETIF_DESTROY_INTF,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_INTF,
    OSAL_MDC_IOCTL_TYPE_NETIF_SET_INTF,
    OSAL_MDC_IOCTL_TYPE_NETIF_CREATE_PROFILE,
    OSAL_MDC_IOCTL_TYPE_NETIF_DESTROY_PROFILE,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_PROFILE,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_INTF_CNT,
    OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_INTF_CNT,
    /* driver */
    OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_RX_FREE,
    OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_TX_FREE, /* waitTxFree(ASYNC) */
    OSAL_MDC_IOCTL_TYPE_NETIF_SET_RX_CFG,   /* setRxConfig       */
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CFG,   /* getRxConfig       */
    OSAL_MDC_IOCTL_TYPE_NETIF_DEINIT_TASK,  /* deinitTask        */
    OSAL_MDC_IOCTL_TYPE_NETIF_DEINIT_DRV,   /* deinitDrv         */
    OSAL_MDC_IOCTL_TYPE_NETIF_INIT_TASK,    /* initTask          */
    OSAL_MDC_IOCTL_TYPE_NETIF_INIT_DRV,     /* initDrv           */
    /* counter */
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_TX_CNT,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CNT,
    OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_TX_CNT,
    OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_RX_CNT,
    /* port attribute */
    OSAL_MDC_IOCTL_TYPE_NETIF_SET_PORT_ATTR,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_PORT_ATTR,
    /* netlink */
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_SET_INTF_PROPERTY,
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_GET_INTF_PROPERTY,
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_CREATE_NETLINK,
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_DESTROY_NETLINK,
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_GET_NETLINK,

    OSAL_MDC_IOCTL_TYPE_NETIF_DEV_TX,

    OSAL_MDC_IOCTL_TYPE_PKT_MOD,

    OSAL_MDC_IOCTL_TYPE_LAST

} OSAL_MDC_IOCTL_TYPE_T;
typedef union {
    UI32_T value;
#if defined(CLX_EN_BIG_ENDIAN)
    struct {
        UI32_T rsvd : 16;
        UI32_T type : 10; /* Maximum 1024 IOCTL types         */
        UI32_T unit : 6;  /* Maximum unit number is 64.       */
    } field;
#elif defined(CLX_EN_LITTLE_ENDIAN)
    struct {
        UI32_T unit : 6;  /* Maximum unit number is 64.       */
        UI32_T type : 10; /* Maximum 1024 IOCTL types         */
        UI32_T rsvd : 16;
    } field;
#else
#error "Host GPD endian is not defined!!\n"
#endif
} OSAL_MDC_IOCTL_CMD_T;

typedef CLX_ERROR_NO_T (*OSAL_MDC_IOCTL_CALLBACK_FUNC_T)(const UI32_T unit, void *ptr_data);

CLX_ERROR_NO_T
_osal_mdc_registerIoctlCallback(const UI32_T unit,
                                const OSAL_MDC_IOCTL_TYPE_T type,
                                const OSAL_MDC_IOCTL_CALLBACK_FUNC_T func);

typedef struct {
    OSAL_MDC_IOCTL_CALLBACK_FUNC_T callback[OSAL_MDC_IOCTL_TYPE_LAST];

} OSAL_MDC_IOCTL_CB_T;

CLX_ERROR_NO_T
osal_mdc_ioctl(const UI32_T unit, const OSAL_MDC_IOCTL_TYPE_T type, void *ptr_data);
#endif /* End of CLX_LINUX_USER_MODE */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
CLX_ERROR_NO_T
osal_mdc_readPciReg(const UI32_T unit, const UI32_T offset, UI32_T *ptr_data, const UI32_T len);

CLX_ERROR_NO_T
osal_mdc_writePciReg(const UI32_T unit,
                     const UI32_T offset,
                     const UI32_T *ptr_data,
                     const UI32_T len);

CLX_ERROR_NO_T
osal_mdc_initDevice(AML_DEV_T *ptr_dev_list, UI32_T *ptr_dev_num);

CLX_ERROR_NO_T
osal_mdc_deinitDevice(void);

CLX_ERROR_NO_T
osal_mdc_initDmaMem(void);

CLX_ERROR_NO_T
osal_mdc_deinitDmaMem(void);

void *
osal_mdc_mmapDmaMem(const CLX_ADDR_T dma_phy_addr, const UI32_T size);

CLX_ERROR_NO_T
osal_mdc_munmapDmaMem(void *ptr_virt_addr);

void *
osal_mdc_allocDmaMem(const UI32_T size);

CLX_ERROR_NO_T
osal_mdc_freeDmaMem(void *ptr_virt_addr);

CLX_ERROR_NO_T
osal_mdc_convertVirtToPhy(void *ptr_virt_addr, CLX_ADDR_T *ptr_phy_addr);

CLX_ERROR_NO_T
osal_mdc_convertPhyToVirt(const CLX_ADDR_T phy_addr, void **pptr_virt_addr);

CLX_ERROR_NO_T
osal_mdc_registerIsr(const UI32_T unit, AML_DEV_ISR_FUNC_T handler, void *ptr_cookie);

CLX_ERROR_NO_T
osal_mdc_connectIsr(const UI32_T unit, AML_DEV_ISR_FUNC_T handler, AML_DEV_ISR_DATA_T *ptr_cookie);

CLX_ERROR_NO_T
osal_mdc_disconnectIsr(const UI32_T unit);

CLX_ERROR_NO_T
osal_mdc_synchronize_irq(const UI32_T unit);

CLX_ERROR_NO_T
osal_mdc_flushCache(void *ptr_virt_addr, const UI32_T size);

CLX_ERROR_NO_T
osal_mdc_invalidateCache(void *ptr_virt_addr, const UI32_T size);

CLX_ERROR_NO_T
osal_mdc_savePciConfig(const UI32_T unit);

CLX_ERROR_NO_T
osal_mdc_restorePciConfig(const UI32_T unit);

/* IO */
int
osal_io_copyToUser(void *ptr_usr_buf, void *ptr_knl_buf, unsigned int size);

int
osal_io_copyFromUser(void *ptr_knl_buf, void *ptr_usr_buf, unsigned int size);

#endif /* OSAL_MDC_H */
