/**
 @file dal_kernel_io.h

 @author  Copyright (C) 2012 Centec Networks Inc.  All rights reserved.

 @date 2012-4-9

 @version v2.0

*/
#ifndef _DAL_KERNEL_H_
#define _DAL_KERNEL_H_
#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_RESOURCES_64BIT) || defined(CONFIG_PHYS_ADDR_T_64BIT)
#define PHYS_ADDR_IS_64BIT
#endif

#ifndef SDK_IN_USERMODE
#ifdef PHYS_ADDR_IS_64BIT
typedef  long long  intptr;
typedef  unsigned long long uintptr;
#else
typedef  int  intptr;
typedef  unsigned int uintptr;
#endif
#endif

#ifndef STATIC
#define STATIC static
#endif
#define DAL_PCI_READ_ADDR  0x0
#define DAL_PCI_READ_DATA  0xc
#define DAL_PCI_WRITE_ADDR 0x8
#define DAL_PCI_WRITE_DATA 0x4
#define DAL_PCI_STATUS     0x10

#define DAL_PCI_STATUS_IN_PROCESS      31
#define DAL_PCI_STATUS_BAD_PARITY      5
#define DAL_PCI_STATUS_CPU_ACCESS_ERR  4
#define DAL_PCI_STATUS_READ_CMD        3
#define DAL_PCI_STATUS_REGISTER_ERR    1
#define DAL_PCI_STATUS_REGISTER_ACK    0

#define DAL_PCI_ACCESS_TIMEOUT 0x64

#define DAL_NAME          "linux_dal"  /* "linux_dal" */

#define DAL_DEV_MAJOR     198

#define DAL_DEV_INTR_MAJOR_BASE     200

#define DAL_DEV_NAME      "/dev/" DAL_NAME
#define DAL_ONE_KB 1024
#define DAL_ONE_MB (1024*1024)
#define CTC_MAX_INTR_NUM 8
struct dal_chip_parm_s
{
    unsigned int lchip;     /*tmp should be uint8*/
    unsigned int fpga_id;     /*tmp add*/
    unsigned int reg_addr;
    unsigned int value;
};
typedef struct dal_chip_parm_s dal_chip_parm_t;

struct dal_intr_parm_s
{
    unsigned int irq;
    unsigned int enable;
};
typedef struct dal_intr_parm_s dal_intr_parm_t;

struct dal_irq_mapping_s
{
    unsigned int hw_irq;
    unsigned int sw_irq;
};
typedef struct dal_irq_mapping_s dal_irq_mapping_t;

struct dal_user_dev_s
{
    unsigned int chip_num;   /*output: local chip number*/
    unsigned int lchip;       /*input: local chip id*/
    unsigned int phy_base0; /* low 32bits physical base address */
    unsigned int phy_base1; /* high 32bits physical base address */
    unsigned int dma_phy_base0; /* low 32bits physical base address */
    unsigned int dma_phy_base1; /* high 32bits physical base address */
    unsigned int bus_no;
    unsigned int dev_no;
    unsigned int fun_no;
    unsigned int soc_active;
    void* virt_base[2];        /* !!!!warning!!!!Virtual base address; pointer void* must be last member */
};
typedef  struct dal_user_dev_s dal_user_dev_t;

struct dal_pci_cfg_ioctl_s
{
    unsigned int lchip;                      /* Device ID */
    unsigned int offset;
    unsigned int value;
};
typedef struct dal_pci_cfg_ioctl_s  dal_pci_cfg_ioctl_t;

enum dal_msi_type_e
{
    DAL_MSI_TYPE_MSI,
    DAL_MSI_TYPE_MSIX,
    DAL_MSI_TYPE_MAX
};
typedef enum dal_msi_type_e dal_msi_type_t;

struct dal_msi_info_s
{
    unsigned int lchip;
    unsigned int irq_base[CTC_MAX_INTR_NUM];
    unsigned int irq_num;
    unsigned int msi_type;
};
typedef struct dal_msi_info_s dal_msi_info_t;

struct dal_intr_info_s
{
    unsigned int irq;
    unsigned int irq_idx;
};
typedef struct dal_intr_info_s dal_intr_info_t;

struct dal_dma_cache_info_s
{
    unsigned long ptr;
    unsigned int length;
};
typedef struct dal_dma_cache_info_s dal_dma_cache_info_t;

#define CMD_MAGIC 'C'
#define CMD_WRITE_CHIP              _IO(CMD_MAGIC, 0) /* for humber ioctrol*/
#define CMD_READ_CHIP               _IO(CMD_MAGIC, 1) /* for humber ioctrol*/
#define CMD_GET_DEVICES             _IO(CMD_MAGIC, 2)
#define CMD_GET_DAL_VERSION         _IO(CMD_MAGIC, 3)
#define CMD_PCI_CONFIG_WRITE        _IO(CMD_MAGIC, 4)
#define CMD_PCI_CONFIG_READ         _IO(CMD_MAGIC, 5)
#define CMD_GET_DMA_INFO            _IO(CMD_MAGIC, 6)
#define CMD_REG_INTERRUPTS          _IO(CMD_MAGIC, 7)
#define CMD_UNREG_INTERRUPTS        _IO(CMD_MAGIC, 8)
#define CMD_EN_INTERRUPTS           _IO(CMD_MAGIC, 9)
#define CMD_I2C_READ                _IO(CMD_MAGIC, 10)
#define CMD_I2C_WRITE               _IO(CMD_MAGIC, 11)
#define CMD_GET_MSI_INFO            _IO(CMD_MAGIC, 12)
#define CMD_SET_MSI_CAP             _IO(CMD_MAGIC, 13)
#define CMD_IRQ_MAPPING             _IO(CMD_MAGIC, 14)
#define CMD_GET_INTR_INFO           _IO(CMD_MAGIC, 15)
#define CMD_CACHE_INVAL             _IO(CMD_MAGIC, 16)
#define CMD_CACHE_FLUSH             _IO(CMD_MAGIC, 17)
#define CMD_GET_KNET_VERSION      _IO(CMD_MAGIC, 18)
#define CMD_CONNECT_INTERRUPTS        _IO(CMD_MAGIC, 19)
#define CMD_DISCONNECT_INTERRUPTS   _IO(CMD_MAGIC, 20)
#define CMD_SET_DMA_INFO             _IO(CMD_MAGIC, 21)
#define CMD_REG_DMA_CHAN             _IO(CMD_MAGIC, 22)
#define CMD_HANDLE_NETIF             _IO(CMD_MAGIC, 23)
#define CMD_GET_WB_INFO              _IO(CMD_MAGIC, 24)

enum dal_version_e
{
    VERSION_MIN,
    VERSION_1DOT0,
    VERSION_1DOT1,
    VERSION_1DOT2,
    VERSION_1DOT3,
    VERSION_1DOT4,

    VERSION_MAX
};
typedef enum dal_version_e dal_version_t;

struct dal_ops_s {
    int     (*interrupt_connect)(unsigned int irq, int prio, void (*)(void*), void *data);
    int     (*interrupt_disconnect)(unsigned int irq);
};
typedef struct dal_ops_s dal_ops_t;

/* We try to assemble a contiguous segment from chunks of this size */
#define DMA_BLOCK_SIZE (512 * DAL_ONE_KB)

extern int dal_get_dal_ops(dal_ops_t **dal_ops);
extern int dal_cache_inval(unsigned long ptr, unsigned int length);
extern int dal_cache_flush(unsigned long ptr, unsigned int length);
extern int dal_dma_direct_read(unsigned char lchip, unsigned int offset, unsigned int* value);
extern int dal_dma_direct_write(unsigned char lchip, unsigned int offset, unsigned int value);
#ifdef __cplusplus
}
#endif

#endif

