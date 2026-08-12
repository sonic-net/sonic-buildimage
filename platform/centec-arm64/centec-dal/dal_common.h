/**
 @file dal_common.h

 @author  Copyright (C) 2012 Centec Networks Inc.  All rights reserved.

 @date 2012-4-9

 @version v2.0

*/
#ifndef _DAL_COMMON_H_
#define _DAL_COMMON_H_
#ifdef __cplusplus
extern "C" {
#endif

#define DAL_MAX_INTR_NUM        8

/*pcie device id*/
#define DAL_VENDOR_VID              0xc001
#define DAL_PCIE_VENDOR_ID          0xcb10
#define DAL_PCIE_VENDOR_ID1         0xcb11
#define DAL_GREATBELT_DEVICE_ID     0x03E8
#define DAL_GOLDENGATE_DEVICE_ID    0xC010
#define DAL_GOLDENGATE_DEVICE_ID1   0xC011
#define DAL_DUET2_DEVICE_ID         0x7148
#define DAL_TSINGMA_DEVICE_ID       0x5236
#define DAL_TSINGMA_DX_DEVICE_ID    0x5260 
#define DAL_TSINGMA_BX_DEVICE_ID    0x5220 
#define DAL_TSINGMA_DXL_DEVICE_ID   0x3225
#define DAL_TSINGMA_MX_DEVICE_ID    0x8180
#define DAL_TSINGMA_GX_DEVICE_ID    0x8120
#define DAL_ARCTIC_DEVICE_ID        0x9200
#define DAL_ARCTIC_M3_DEVICE_ID     0x8290
#define DAL_ARCTIC_M3F_DEVICE_ID    0x8291        /* identify by efuse info */
#define DAL_TSINGMA_AX_DEVICE_ID    0x2118
#define DAL_PACIFIC_M12_DEVICE_ID   0x9300
#define DAL_PACIFIC_M12F_DEVICE_ID  0x9361
#define DAL_USW_DEVICE_ID           0x5a5a        /*dal virtual device id*/

#define DAL_AM3_PP_NUM       (2)
#define DAL_NETIF_T_PORT        0
#define DAL_NETIF_T_VLAN        1

#define DAL_MAX_KNET_NETIF      1024
#define DAL_MAX_KNET_NAME_LEN   32

#define DAL_PCI_CMD_STATUS      0x0
#define DAL_PCI_ADDR            0x4
#define DAL_PCI_DATA_BUF        0x8
#define DAL_PCI_ERROR_CODE      0x178

#define DAL_PCI_ERROR_VEC_OFFSET     0x24
#define DAL_PCI_ERROR_ADDR_OFFSET    0x200
#define DAL_PCI_ERROR_INFO_OFFSET    0x200
#define DAL_PCI_CLEAR_ERROR_ADDR     0x10B0
#define DAL_PCI_MASK_ADDR_OFFSET     0x100

#define DAL_PCI_CMD1_STATUS      0x400
#define DAL_PCI_ADDR1            0x404
#define DAL_PCI_DATA1_BUF        0x408
#define DAL_PCI_ERROR_CODE1      0x578

#ifdef EMULATION_ENV
#define PCIE_TIMEOUT 1000000
#else
#define PCIE_TIMEOUT 2000
#endif

#define DAL_MB_SIZE             0x100000
/* We try to assemble a contiguous segment from chunks of this size */
#define DMA_BLOCK_SIZE (512 * DAL_ONE_KB)

enum dal_msi_type_e
{
    DAL_MSI_TYPE_MSI,
    DAL_MSI_TYPE_MSIX,
    DAL_MSI_TYPE_MAX
};
typedef enum dal_msi_type_e dal_msi_type_t;

typedef enum dal_cpu_mode_type_e
{
    DAL_CPU_MODE_TYPE_NONE,
    DAL_CPU_MODE_TYPE_PCIE,      /*use pcie*/
    DAL_CPU_MODE_TYPE_LOCAL,     /*use local bus*/
    DAL_CPU_MODE_TYPE_ASW,
    DAL_CPU_MODE_MAX_TYPE,

} dal_cpu_mode_type_t;

enum dal_operate_code_e
{
    DAL_OP_CREATE,
    DAL_OP_DESTORY,
    DAL_OP_GET,
    DAL_OP_SET_CARRIER_ON,
    DAL_OP_SET_CARRIER_OFF,
    DAL_OP_MAX,
};
typedef enum dal_operate_code_e dal_operate_code_t;

struct dal_dma_info_s
{
    unsigned int ldev;
    unsigned int phy_base;
    unsigned int phy_base_hi;
    unsigned int size;
    unsigned int knet_tx_offset;
    unsigned int knet_tx_size;
    unsigned long long virt_base;
};
typedef struct dal_dma_info_s dal_dma_info_t;

struct dal_dma_chan_s
{
    unsigned char lchip;
    unsigned char channel_id;
    unsigned char wb_keep;
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
    unsigned char rsv;
    unsigned char type;
    unsigned char lchip;
    unsigned short netif_id;
    unsigned short vlan;
    unsigned int gport;
    unsigned char mac[6];
    char name[DAL_MAX_KNET_NAME_LEN];
    unsigned int flag;
};
typedef struct dal_netif_s dal_netif_t;

struct dal_knet_dump_s
{
    unsigned char clear;
    unsigned char chan_id;
    unsigned short start_idx;
    unsigned short end_idx;
    unsigned char ldev;
};
typedef struct dal_knet_dump_s dal_knet_dump_t;


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
    DAL_SMI_IO,       /* [TMA]asw is access as smi device, using ioctrl */
    DAL_SPI_IO,       /* [TMA]asw is access as spi device, using ioctrl */
    DAL_I2C_IO,       /* [TMA]asw is access as i2c device, using ioctrl */
    DAL_MAX_ACCESS_TYPE
};
typedef enum dal_access_type_e dal_access_type_t;

struct dal_pci_dev_s
{
    unsigned int busNo;
    unsigned int devNo;
    unsigned int funNo;
    unsigned int domainNo;
};
typedef struct dal_pci_dev_s dal_pci_dev_t;

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

#ifdef ASW_ACTIVE
#define DAL_SMI_CMD 0x01
#define DAL_SMI_ADDR_L 0x02
#define DAL_SMI_ADDR_H 0x03
#define DAL_SMI_WR_DATA_L 0x04
#define DAL_SMI_WR_DATA_H 0x05
#define DAL_SMI_RD_DATA_L 0x06
#define DAL_SMI_RD_DATA_H 0x07
#define DAL_SMI_STATUS	0x08
#define DAL_SMI_DEBUG_STATUS 0x09
#define DAL_SMI_TIMEOUT_THRD_L 0x0a
#define DAL_SMI_TIMEOUT_THRD_H 0x0b
#define DAL_SMI_SLAVE_ID 0x3
#define DAL_SMI_ACT		0x01
#define DAL_SMI_WRITE	0x02
#define SMI_STATUS_END	0x01
#define SMI_STATUS_TIMEOUT	0x02
#define SMI_STATUS_ERROR	0x04
#define DAL_SMI_TIMEOUT_CNT 0xFFF00
#define DAL_SMI_TIMEOUT 10000
#define DAL_I2C_WR_DATA3 0x00
#define DAL_I2C_WR_DATA2 0x01
#define DAL_I2C_WR_DATA1 0x02
#define DAL_I2C_WR_DATA0 0x03
#define DAL_I2C_ADDR_CMD 0x04
#define DAL_I2C_RD_DATA3 0x05
#define DAL_I2C_RD_DATA2 0x06
#define DAL_I2C_RD_DATA1 0x07
#define DAL_I2C_RD_DATA0 0x08
#define DAL_I2C_STATUS	0x09
#define DAL_I2C_PERF_CTL 0x10
#define CHIPACCESS_CMDWRITE 0x00
#define CHIPACCESS_CMDREAD 0x80
#define CHIPACCESS_WRDATA 0x00
#define CHIPACCESS_ADDRCMD 0x01
#define CHIPACCESS_RDDATE 0x02
#define CHIPACCESS_STATUS 0x03
#define CHIPACCESS_DEVICEID 0x0A
#define I2C_STATUS_DONE 0x80
#define I2C_STATUS_OVERLAP 0x40
#define I2C_STATUS_TIMEOUT 0x20
#define I2C_STATUS_ERROR 0x10
#define I2C_TIMEOUT 250
#define CTC_SPI_CMD_WRITE   0x02
#define CTC_SPI_CMD_READ    0x03
#define CTC_SPI_STATUS_ERR 0xdeadbeef
#define CTC_SPI_STATUS_TIMEOUT 0xdeada000
#define CTC_SPI_STATUS_NODATA 0xdeadb000
#endif
#ifdef __cplusplus
}
#endif

#endif

