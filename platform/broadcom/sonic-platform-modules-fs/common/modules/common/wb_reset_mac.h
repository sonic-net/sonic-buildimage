#ifndef __WB_RESET_MAC_H__
#define __WB_RESET_MAC_H__

#include <linux/kallsyms.h>

#define DEV_NAME_LEN        (64)
#define ENABLE_NUM          (16)

/* io操作 */
typedef struct {
    uint32_t    addr;         
    uint32_t    mask;  
    uint32_t    en_value;       /* 开启时写入的值，暂时不用 */
    uint32_t    disable_value;  /* 关闭时写入的值 */           
} io_info_device_t;

/* PCI操作 */
typedef struct {
    uint32_t domain;          /* LPC PCIe地址总线域 */
    uint32_t bus;             /* LPC PCIe地址总线号 */
    uint32_t slot;            /* LPC PCIe地址设备号 */
    uint32_t fn;              /* LPC PCIe地址功能号 */    
    uint32_t addr; 
    uint32_t mask; 
    uint32_t en_value;        /* 开启时写入的值，暂时不用 */
    uint32_t disable_value;   /* 关闭时写入的值 */
} pci_info_device_t;

typedef struct reset_mac_device_s {
    char  logic_dev[ENABLE_NUM][DEV_NAME_LEN];              /* reset_mac逻辑器件名称 */
    uint32_t    reset_mac_reg[ENABLE_NUM];          /* reset_mac寄存器地址 */
    uint32_t    reset_mac_val[ENABLE_NUM];  
    uint32_t    reset_mac_mask[ENABLE_NUM];    
    uint8_t     logic_func_mode[ENABLE_NUM];        /* 1:i2c, 2:pcie, 3:io, 4:设备文件 */    
    uint32_t    reset_mac_delay[ENABLE_NUM];        /* reset_mac写寄存器前等待时间(ms) */
    uint8_t     down_pcie_mode;
    uint8_t     support_in_interrupt;       /* 是否支持在中断上下文调用 */
    uint8_t     reset_env_type;             /* 支持的环境上下文 */
    union {
        io_info_device_t io;
        pci_info_device_t pci;
    } en_type;
    uint32_t    en_number;
} reset_mac_device_t;


#endif /* __WB_RESET_MAC_H__ */