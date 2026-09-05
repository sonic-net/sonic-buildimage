/**
 @file dal.h

 @author  Copyright (C) 2012 Centec Networks Inc.  All rights reserved.

 @date 2012-4-9

 @version v2.0

*/
#ifndef _DAL_COMMON_H_
#define _DAL_COMMON_H_
#ifdef __cplusplus
extern "C" {
#endif

#define DAL_MAX_CHIP_NUM   4                   /* DAL support max chip num is 4 */
#define DAL_MAX_INTR_NUM    8

#define DAL_NETIF_T_PORT       0
#define DAL_NETIF_T_VLAN       1

#define DAL_MAX_KNET_NETIF       64
#define DAL_MAX_KNET_NAME_LEN        32

#define DAL_PCI_CMD_STATUS  0x0
#define DAL_PCI_ADDR  0x4
#define DAL_PCI_DATA_BUF  0x8

enum dal_operate_code_e
{
    DAL_OP_CREATE,
    DAL_OP_DESTORY,
    DAL_OP_GET,
    DAL_OP_MAX,
};
typedef enum dal_operate_code_e dal_operate_code_t;

struct dal_dma_info_s
{
    unsigned int lchip;
    unsigned int phy_base;
    unsigned int phy_base_hi;
    unsigned int size;
    unsigned int knet_tx_offset;
    unsigned int knet_tx_size;
    unsigned int* virt_base;
};
typedef struct dal_dma_info_s dal_dma_info_t;

struct dal_dma_chan_s
{
    unsigned char lchip;
    unsigned char channel_id;
    unsigned char dmasel;
    unsigned char active;
    unsigned short current_index;
    unsigned short desc_num;
    unsigned short desc_depth;
    unsigned short data_size;
    unsigned long long mem_base;
    void* virt_base;    /**< don't use when register chan*/
    unsigned char* p_desc_used; /**< don't use when register chan*/
};
typedef struct dal_dma_chan_s dal_dma_chan_t;

struct dal_netif_s
{
    unsigned char op_type;
    unsigned char netif_id;
    unsigned char type;
    unsigned char lchip;
    unsigned short vlan;
    unsigned int gport;
    unsigned char mac[6];
    char name[DAL_MAX_KNET_NAME_LEN];
};
typedef struct dal_netif_s dal_netif_t;

/**
 @brief  define dal error type
*/
enum dal_err_e
{
    DAL_E_NONE = 0,                 /**< NO error */
    DAL_E_INVALID_PTR = -1000,      /**< invalid pointer */
    DAL_E_INVALID_FD = -999,        /**< invalid FD */
    DAL_E_TIME_OUT = -998,          /**< time out */
    DAL_E_INVALID_ACCESS = -997, /**< invalid access type*/
    DAL_E_MPOOL_NOT_CREATE = -996, /**< mpool not create*/
    DAL_E_INVALID_IRQ = -995,
    DAL_E_DEV_NOT_FOUND = -994,
    DAL_E_EXCEED_MAX = -993,
    DAL_E_NOT_INIT = -992,
    DAL_E_ENVALID_MSI_PARA = -991,

    DAL_E_ERROR_CODE_END
};

enum dal_access_type_e
{
    DAL_PCI_IO,       /* [HB]humber is access as pci device, using ioctrl */
    DAL_SUPER_IF,   /* [HB]humber is controled by fpga device */
    DAL_PCIE_MM,    /* [GB]Gb is access as pcie device, using mmap */
    DAL_SPECIAL_EMU, /* [GB]special for emulation */
    DAL_MAX_ACCESS_TYPE
};
typedef enum dal_access_type_e dal_access_type_t;

struct dal_pci_dev_s
{
    unsigned int busNo;
    unsigned int devNo;
    unsigned int funNo;
};
typedef struct dal_pci_dev_s dal_pci_dev_t;

#ifndef HOST_IS_LE
#define HOST_IS_LE 1
#endif

#if (HOST_IS_LE == 0)

/* pci cmd struct define */
typedef struct pci_cmd_status_s
{
    unsigned int pcieReqOverlap   : 1;
    unsigned int wrReqState       : 3;
    unsigned int pciePoison       : 1;
    unsigned int rcvregInProc     : 1;
    unsigned int regInProc        : 1;
    unsigned int reqProcAckCnt    : 5;
    unsigned int reqProcAckError  : 1;
    unsigned int reqProcTimeout   : 1;
    unsigned int reqProcError     : 1;
    unsigned int reqProcDone      : 1;
    unsigned int pcieDataError    : 1;
    unsigned int pcieReqError     : 1;
    unsigned int reserved         : 1;
    unsigned int cmdDataLen       : 5;
    unsigned int cmdEntryWords    : 4;
    unsigned int pcieReqCmdChk    : 3;
    unsigned int cmdReadType      : 1;
} pci_cmd_status_t;

#else

typedef struct pci_cmd_status_s
{
    unsigned int cmdReadType      : 1;       /* bit0 */
    unsigned int pcieReqCmdChk    : 3;       /* bit1~3 */
    unsigned int cmdEntryWords    : 4;       /* bit4~7 */
    unsigned int cmdDataLen       : 5;       /* bit8~12 */
    unsigned int reserved : 1;               /* bit13 */
    unsigned int pcieReqError     : 1;
    unsigned int pcieDataError    : 1;
    unsigned int reqProcDone      : 1;
    unsigned int reqProcError     : 1;
    unsigned int reqProcTimeout   : 1;
    unsigned int reqProcAckError  : 1;
    unsigned int reqProcAckCnt    : 5;
    unsigned int regInProc        : 1;
    unsigned int rcvregInProc     : 1;
    unsigned int pciePoison       : 1;
    unsigned int wrReqState       : 3;
    unsigned int pcieReqOverlap   : 1;
} pci_cmd_status_t;
#endif

typedef union pci_cmd_status_u_e
{
    pci_cmd_status_t cmd_status;
    unsigned int val;
} pci_cmd_status_u_t;

#ifdef __cplusplus
}
#endif

#endif

