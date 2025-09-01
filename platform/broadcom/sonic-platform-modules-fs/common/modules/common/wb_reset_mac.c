/*
 * Copyright(C) 2022 Ruijie Network. All rights reserved.
 * wb_reset_mac.c
 * Original Author: sonic_rd@ruijie.com.cn 2023-02-07
 *
 * This driver is used to reset MAC.
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@ruijie.com.cn         2023-02-07          Initial version
 */
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/hwmon-sysfs.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/kdebug.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/preempt.h>

#include "wb_reset_mac.h"
#include <wb_bsp_kernel_debug.h>
#include <wb_logic_dev_common.h>

#define MODULE_NAME "wb_reset_mac"

#define NO_DISABLE_LINK           (0)
#define IO_DISABLE_LINK           (1)
#define PCI_DISABLE_LINK          (2)
#define RESET_MAC_INFO_MAX        (16)

#define RESET_MAC_RETRY_TIME                (5)
#define RESET_MAC_RETRY_SLEEP_TIME          (10)     /* 10ms */ 

#define RESET_IN_PANIC                      (0x1)   
#define RESET_IN_OOPS                       (0x2)

/* Use the wb_bsp_kernel_debug header file must define debug variable */
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

/* 逻辑器件复位mac */
typedef struct {
    const char  *dev_name;              /* reset_mac逻辑器件名称 */
    uint32_t    reset_mac_reg;          /* reset_mac寄存器地址 */
    uint32_t    reset_mac_val;  
    uint32_t    reset_mac_mask;    
    uint8_t     logic_func_mode;        /* 1:i2c, 2:pcie, 3:io, 4:设备文件 */    
    uint32_t    reset_mac_delay;        /* reset_mac写寄存器前等待时间(ms) */
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
} logic_reset_mac_info_t;

/* io操作 */
typedef struct {
    uint32_t    addr;         
    uint32_t    mask;  
    uint32_t    en_value;       /* 开启时写入的值，暂时不用 */
    uint32_t    disable_value;  /* 关闭时写入的值 */           
} io_info_t;

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
    struct pci_dev *pci_dev;  /* 对应的pci设备 */
} pci_info_t;


typedef struct {
    logic_reset_mac_info_t logic_reset_mac_info[RESET_MAC_INFO_MAX];
    uint8_t down_pcie_mode;
    io_info_t io_info;
    pci_info_t pci_info;
    uint32_t    logic_info_num;             /* reset mac reg Number */
    uint8_t     support_in_interrupt;       /* 是否支持在中断上下文调用 */
    uint8_t     reset_env_type;             /* 支持的环境上下文 */
    struct device *dev;
} reset_mac_t;

static reset_mac_t *g_reset_mac;

extern int tcs_reboot_callback_register(struct notifier_block *nb);
extern int tcs_reboot_callback_unregister(struct notifier_block *nb);

static int wb_reset_mac(struct notifier_block *self, unsigned long event, void *unused);

static int wb_logic_reg_write(logic_reset_mac_info_t *node, uint8_t *val, size_t size)
{
    device_func_write pfunc;

    pfunc = (device_func_write)node->write_intf_addr;
    return pfunc(node->dev_name, node->reset_mac_reg, val, size);
}

static int wb_logic_reg_read(logic_reset_mac_info_t *node, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    pfunc = (device_func_read)node->read_intf_addr;
    return pfunc(node->dev_name, node->reset_mac_reg, val, size);
}

static int io_config_mode_init(reset_mac_t *reset_mac_info)
{
    struct device *dev;
    io_info_t *io_info;
    int ret;

    dev = reset_mac_info->dev;
    io_info = &reset_mac_info->io_info;

    ret = 0;
    ret += of_property_read_u32(dev->of_node, "io_addr", &io_info->addr);
    ret += of_property_read_u32(dev->of_node, "io_mask", &io_info->mask);
    ret += of_property_read_u32(dev->of_node, "io_en_val", &io_info->en_value);
    ret += of_property_read_u32(dev->of_node, "io_disable_val", &io_info->disable_value);
    
    if (ret != 0) {
        dev_err(dev, "Failed to io_info dts.\n");
        return -ENXIO;
    }

    return 0;
}

static int pci_config_mode_init(reset_mac_t *reset_mac_info)
{
    struct device *dev;
    pci_info_t *pci_info;
    int ret, devfn;
    reset_mac_device_t *reset_mac_device;
    pci_info_device_t *pci_info_device;

    dev = reset_mac_info->dev;
    pci_info = &reset_mac_info->pci_info;
    if (dev->of_node) {
        ret = 0;
        ret += of_property_read_u32(dev->of_node, "pci_domain", &pci_info->domain);
        ret += of_property_read_u32(dev->of_node, "pci_bus", &pci_info->bus);
        ret += of_property_read_u32(dev->of_node, "pci_slot", &pci_info->slot);
        ret += of_property_read_u32(dev->of_node, "pci_fn", &pci_info->fn);
        ret += of_property_read_u32(dev->of_node, "pci_addr", &pci_info->addr);
        ret += of_property_read_u32(dev->of_node, "pci_mask", &pci_info->mask);
        ret += of_property_read_u32(dev->of_node, "pci_en_val", &pci_info->en_value);
        ret += of_property_read_u32(dev->of_node, "pci_disable_val", &pci_info->disable_value);
        
        if (ret != 0) {
            dev_err(dev, "Failed to pci_info dts.\n");
            return -ENXIO;
        }
    } else {
        if (dev->platform_data == NULL) {
            DEBUG_ERROR("Failed to priv pci_info platform data \n");
            return -ENXIO;
        }
        reset_mac_device = dev->platform_data;
        pci_info_device = &reset_mac_device->en_type.pci;
        pci_info->domain = pci_info_device->domain;
        pci_info->bus = pci_info_device->bus;
        pci_info->slot = pci_info_device->slot;
        pci_info->fn = pci_info_device->fn;
        pci_info->addr = pci_info_device->addr;
        pci_info->mask = pci_info_device->mask;
        pci_info->en_value = pci_info_device->en_value;
        pci_info->disable_value = pci_info_device->disable_value;
    }

    /* 根据pci配置信息获取pci设备 */
    devfn = PCI_DEVFN(pci_info->slot, pci_info->fn);
    pci_info->pci_dev = pci_get_domain_bus_and_slot(pci_info->domain, pci_info->bus, devfn);
    if (pci_info->pci_dev == NULL) {
        DEBUG_ERROR("Failed to find pci_dev, domain:0x%04x, bus:0x%02x, devfn:0x%x\n",
            pci_info->domain, pci_info->bus, devfn);
        return -ENXIO;
    }
    DEBUG_VERBOSE("success find pci_dev, domain:0x%04x, bus:0x%02x, devfn:0x%x\n",
            pci_info->domain, pci_info->bus, devfn);

    DEBUG_VERBOSE("reset_mac_info: domain:0x%x, bus:0x%x, slot:0x%x, fn:0x%x \n", pci_info->domain, pci_info->bus, pci_info->slot, pci_info->fn);
    DEBUG_VERBOSE("reset_mac_info: addr:0x%x, mask:0x%x, en_value:0x%x, disable_value:0x%x \n", pci_info->addr, pci_info->mask, pci_info->en_value, pci_info->disable_value);

    return 0;
}

static int reset_mac_config_init(reset_mac_t *reset_mac_info)
{
    int ret;
    int i;
    char buf[64];
    struct device *dev;
    reset_mac_device_t *reset_mac_device;

    logic_reset_mac_info_t *logic_reset_mac_info_point;

    DEBUG_VERBOSE("Enter reset_mac_config_init\r\n");

    dev = reset_mac_info->dev;

    if (dev->of_node) {
        ret = 0;
        ret += of_property_read_u8(dev->of_node, "down_pcie_mode", &reset_mac_info->down_pcie_mode);
#if 0
        ret += of_property_read_u8(dev->of_node, "reset_env_type", &reset_mac_info->reset_env_type);
#endif
        if (ret != 0) {
            DEBUG_ERROR("Failed to priv dts \n");
            return -ENXIO;
        }

#if 0
        /* 默认支持中断模式下调用 */
        reset_mac_info->support_in_interrupt = 1;
        of_property_read_u8(dev->of_node, "support_in_interrupt", &reset_mac_info->support_in_interrupt);
#endif

        switch (reset_mac_info->down_pcie_mode) {
        case NO_DISABLE_LINK:
            break;
        case IO_DISABLE_LINK:
            ret = io_config_mode_init(reset_mac_info);
            if (ret < 0) {
                dev_err(dev, "init reset_mac func mode failed.\n");
                return -ENXIO;
            }
            break;

        case PCI_DISABLE_LINK:
            ret = pci_config_mode_init(reset_mac_info);
            if (ret < 0) {
                dev_err(dev, "init reset_mac func mode failed.\n");
                return -ENXIO;
            }
            break;

        default:
            dev_err(dev, "unsupport reset_mac func mode: %d.\n", reset_mac_info->down_pcie_mode);
            return -ENXIO;
        }
        
        /* 循环解析配置树中的reset_mac寄存器信息 */
        reset_mac_info->logic_info_num = 0;
        for (i = 0; i < RESET_MAC_INFO_MAX; i++) {
            logic_reset_mac_info_point = &reset_mac_info->logic_reset_mac_info[i];
            mem_clear(buf, sizeof(buf));
            snprintf(buf, sizeof(buf) - 1, "logic_dev_%d", i);
            ret = 0;
            ret += of_property_read_string(dev->of_node, buf, &logic_reset_mac_info_point->dev_name); 
            if(ret != 0) {
                /* Failure to resolve to LOGIC_DEV means no logical device is enabled. No failure is returned */
                ret = 0;
                break;
            }

            mem_clear(buf, sizeof(buf));
            snprintf(buf, sizeof(buf) - 1, "reset_mac_reg_%d", i);
            ret = of_property_read_u32(dev->of_node, buf, &logic_reset_mac_info_point->reset_mac_reg);
            if (ret != 0) {
                DEBUG_ERROR("Failed to config reset_mac_reg_%d ret =%d.\n", i, ret);
                break;
            }

            mem_clear(buf, sizeof(buf));
            snprintf(buf, sizeof(buf) - 1, "reset_mac_val_%d", i);
            ret = of_property_read_u32(dev->of_node, buf, &logic_reset_mac_info_point->reset_mac_val);
            if (ret != 0) {
                DEBUG_ERROR("Failed to config reset_mac_val_%d ret =%d.\n", i, ret);
                break;
            }

            mem_clear(buf, sizeof(buf));
            snprintf(buf, sizeof(buf) - 1, "reset_mac_mask_%d", i);
            ret = of_property_read_u32(dev->of_node, buf, &logic_reset_mac_info_point->reset_mac_mask);
            if (ret != 0) {
                DEBUG_ERROR("Failed to config reset_mac_mask_%d ret =%d.\n", i, ret);
                break;
            }

            mem_clear(buf, sizeof(buf));
            snprintf(buf, sizeof(buf) - 1, "logic_func_mode_%d", i);
            ret = of_property_read_u8(dev->of_node, buf, &logic_reset_mac_info_point->logic_func_mode);
            if (ret != 0) {
                DEBUG_ERROR("Failed to config logic_func_mode_%d ret =%d.\n", i, ret);
                break;
            }

            ret = find_intf_addr(&logic_reset_mac_info_point->write_intf_addr, &logic_reset_mac_info_point->read_intf_addr, logic_reset_mac_info_point->logic_func_mode);
            if (ret) {
                dev_err(dev, "find_intf_addr func mode %d fail, ret: %d.\n", logic_reset_mac_info_point->logic_func_mode, ret);
                return ret;
            }

            /* 不配置delay默认为0 */
            mem_clear(buf, sizeof(buf));
            snprintf(buf, sizeof(buf) - 1, "reset_mac_delay_%d", i);
            logic_reset_mac_info_point->reset_mac_delay = 0;
            of_property_read_u32(dev->of_node, buf, &logic_reset_mac_info_point->reset_mac_delay);
            
            reset_mac_info->logic_info_num++;
        }
    } else {
        if (dev->platform_data == NULL) {
            DEBUG_ERROR("Failed to priv platform data \n");
            return -ENXIO;
        }
        reset_mac_device = dev->platform_data;
        reset_mac_info->down_pcie_mode = reset_mac_device->down_pcie_mode;
#if 0
        reset_mac_info->reset_env_type = reset_mac_device->reset_env_type;
        reset_mac_info->support_in_interrupt = reset_mac_device->support_in_interrupt;
#endif
        switch (reset_mac_device->down_pcie_mode) {
        case NO_DISABLE_LINK:
            break;
        case PCI_DISABLE_LINK:
            ret = pci_config_mode_init(reset_mac_info);
            if (ret < 0) {
                dev_err(dev, "init reset_mac func mode failed.\n");
                return -ENXIO;
            }
            break;
        default:
            dev_err(dev, "unsupport reset_mac func mode: %d.\n", reset_mac_device->down_pcie_mode);
            return -ENXIO;
        }

        /* 循环解析配置树中的reset_mac寄存器信息 */
        reset_mac_info->logic_info_num = 0;
        for (i = 0; i < reset_mac_device->en_number; i++) {
            logic_reset_mac_info_point = &reset_mac_info->logic_reset_mac_info[i];
            logic_reset_mac_info_point->dev_name = reset_mac_device->logic_dev[i];
            logic_reset_mac_info_point->reset_mac_reg = reset_mac_device->reset_mac_reg[i];
            logic_reset_mac_info_point->reset_mac_val = reset_mac_device->reset_mac_val[i];
            logic_reset_mac_info_point->reset_mac_mask = reset_mac_device->reset_mac_mask[i];
            logic_reset_mac_info_point->logic_func_mode = reset_mac_device->logic_func_mode[i];
            logic_reset_mac_info_point->reset_mac_delay = reset_mac_device->reset_mac_delay[i];
            reset_mac_info->logic_info_num++;
            ret = find_intf_addr(&logic_reset_mac_info_point->write_intf_addr, &logic_reset_mac_info_point->read_intf_addr, logic_reset_mac_info_point->logic_func_mode);
            if (ret) {
                dev_err(dev, "find_intf_addr func mode %d fail, ret: %d.\n", logic_reset_mac_info_point->logic_func_mode, ret);
                return ret;
            }
        }
    }

    DEBUG_VERBOSE("logic_info_num:%u", reset_mac_info->logic_info_num);
    DEBUG_VERBOSE("reset_mac_config_init success");
    return 0;
}

static int set_link_by_io(io_info_t *io_info, int on)
{
    uint8_t value;

    value = inb(io_info->addr);

    value &= ~(io_info->mask);
    if (on == 1) {
        value |= io_info->en_value;
    } else {
        value |= io_info->disable_value;
    }

    outb(value, io_info->addr);
    return 0;
}

static int set_link_by_pci(pci_info_t *pci_info, int on)
{
    int ret;
    u32 bar_val;

    DEBUG_VERBOSE("enter set link by pci.\n");

    /* 先读然后做mask和或操作，最后写入 */
    ret = pci_read_config_dword(pci_info->pci_dev, pci_info->addr, &bar_val);
    if (ret) {
        DEBUG_ERROR("pci_read_config_dword failed ret %d.\n", ret);
        return -EIO;
    }

    DEBUG_VERBOSE("pci read config dword value:0x%x.\n", bar_val);

    bar_val &= ~(pci_info->mask);
    if (on == 1) {
        bar_val |= pci_info->en_value;
    } else {
        bar_val |= pci_info->disable_value;
    }
    ret = pci_write_config_dword(pci_info->pci_dev, pci_info->addr, bar_val);
    if (ret) {
        DEBUG_ERROR("pci_write_config_dword bar_val[0x%x], failed ret %d.\n", bar_val, ret);
        return -EIO;
    }
    DEBUG_VERBOSE("pci write config dword info, value:0x%x, addr:0x%x.\n", bar_val, pci_info->addr);
    DEBUG_VERBOSE("set link by pci success.\n");
    return 0;
}

static int wb_reset_mac_ctrl(reset_mac_t *node)
{
    uint8_t reset_ops;
    int ret, i;

    /* 先根据配置断链 */
    switch (node->down_pcie_mode) {
    case IO_DISABLE_LINK:
        ret = set_link_by_io(&node->io_info, 0);
        if (ret < 0) {
            DEBUG_ERROR("set link by io fail.\n");
            return -EPIPE;
        }
        break;

    case PCI_DISABLE_LINK:
        ret = set_link_by_pci(&node->pci_info, 0);
        if (ret < 0) {
            DEBUG_ERROR("set link by pci fail.\n");
            return -EPIPE;
        }
        break;

    default:
        break;
    }

    /* 循环写寄存器复位mac */
    for (i = 0; i < node->logic_info_num; i++) {
        /* read origin register value */
        DEBUG_VERBOSE("reset_mac_info: log_dev_name:%s, reset_mac_reg:0x%x, reset_mac_val:0x%x, reset_mac_mask:0x%x", node->logic_reset_mac_info[i].dev_name, 
            node->logic_reset_mac_info[i].reset_mac_reg, node->logic_reset_mac_info[i].reset_mac_val, node->logic_reset_mac_info[i].reset_mac_mask);

        if (node->logic_reset_mac_info[i].reset_mac_delay != 0) {
            mdelay(node->logic_reset_mac_info[i].reset_mac_delay);
        }

        ret = wb_logic_reg_read(&node->logic_reset_mac_info[i], &reset_ops, 1);
        if (ret < 0) {
            DEBUG_ERROR("read %s, addr: 0x%x failed, ret: %d\n", node->logic_reset_mac_info[i].dev_name,
                node->logic_reset_mac_info[i].reset_mac_reg, ret);
            return -EIO;
        }
        DEBUG_VERBOSE("read %s addr 0x%x success, value: 0x%x\n",
            node->logic_reset_mac_info[i].dev_name, node->logic_reset_mac_info[i].reset_mac_reg, reset_ops);

        reset_ops &= ~(node->logic_reset_mac_info[i].reset_mac_mask);
        reset_ops |= node->logic_reset_mac_info[i].reset_mac_val;

        ret = wb_logic_reg_write(&node->logic_reset_mac_info[i], &reset_ops, 1);
        if (ret < 0) {
            DEBUG_ERROR("write %s, addr: 0x%x failed, ret: %d\n", node->logic_reset_mac_info[i].dev_name,
                node->logic_reset_mac_info[i].reset_mac_reg, ret);
            return -EIO;
        }
        DEBUG_VERBOSE("write %s addr 0x%x success, value: 0x%x\n",
            node->logic_reset_mac_info[i].dev_name, node->logic_reset_mac_info[i].reset_mac_reg, reset_ops);

    }
    return 0;
}

int do_wb_reset_mac(void)
{
    int ret, i;

    if (g_reset_mac == NULL) {
        DEBUG_ERROR("mac reset info is NULL, power reset failed.\n");
        return -ENXIO;
    }

    dev_info(g_reset_mac->dev, "bsp mac reset!!!\n");

    DEBUG_VERBOSE("mac reset start\n");
    for (i = 0; i < RESET_MAC_RETRY_TIME; i++) {
        ret = wb_reset_mac_ctrl(g_reset_mac);
        if (ret >= 0) {
            dev_info(g_reset_mac->dev, "mac reset success!!!\n");
            return 0;
        }
        DEBUG_VERBOSE("try to reset mac fail,time:%d\n",i);
        if (i < (RESET_MAC_RETRY_TIME - 1) ) {
            mdelay(RESET_MAC_RETRY_SLEEP_TIME);
        }
    }

    dev_err(g_reset_mac->dev, "mac reset failed, ret: %d\n", ret);
    return ret;
}

EXPORT_SYMBOL(do_wb_reset_mac);

static struct notifier_block mac_reset_nb = {
        .notifier_call = wb_reset_mac,
};

static int wb_reset_mac(struct notifier_block *self,
            unsigned long event, void *unused)
{
    return do_wb_reset_mac();
}

static int wb_reset_mac_probe(struct platform_device *pdev)
{
    reset_mac_t *reset_mac;
    int ret;

    DEBUG_VERBOSE("wb_reset_mac_init\n");

    if (g_reset_mac != NULL) {
        DEBUG_ERROR("g_reset_mac is not null, exit\n");
        return -EBUSY;
    } 

    reset_mac = devm_kzalloc(&pdev->dev, sizeof(reset_mac_t), GFP_KERNEL);
    if (!reset_mac) {
        dev_err(&pdev->dev, "devm_kzalloc error.\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, reset_mac);
    reset_mac->dev = &pdev->dev;

    ret = reset_mac_config_init(reset_mac);
    if (ret != 0) {
        dev_err(reset_mac->dev, "Failed to get reset mac config, ret: %d\n", ret);
        return -ENXIO;
    }

    /* 赋值给全局变量 */
    g_reset_mac = reset_mac;

    return 0;
}

static int wb_reset_mac_remove(struct platform_device *pdev)
{
    reset_mac_t *reset_mac;

    reset_mac = platform_get_drvdata(pdev);

    g_reset_mac = NULL;
    dev_info(reset_mac->dev, "Remove reset mac control success.\n");
    platform_set_drvdata(pdev, NULL);
    
    return 0;
}

static const struct of_device_id wb_reset_mac_driver_of_match[] = {
    { .compatible = "wb_reset_mac" },
    { },
};
MODULE_DEVICE_TABLE(of, wb_reset_mac_driver_of_match);

static struct platform_driver wb_reset_mac_driver = {
    .probe      = wb_reset_mac_probe,
    .remove     = wb_reset_mac_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = MODULE_NAME,
        .of_match_table = wb_reset_mac_driver_of_match,
    },
};


static int __init wb_reset_mac_init(void)
{
    int ret = 0;

    platform_driver_register(&wb_reset_mac_driver);

    // ret = tcs_reboot_callback_register(&mac_reset_nb);
    // if (ret != 0)
    // {
    //     DEBUG_ERROR("reset_mac_init tcs_reboot_callback_register err! ret=%d \n", ret);
    // }

    return ret;
}
static void __exit wb_reset_mac_exit(void)
{
    // tcs_reboot_callback_unregister(&mac_reset_nb);
    platform_driver_unregister(&wb_reset_mac_driver);
    DEBUG_INFO("module bsp_mac_reset uninstalled !\n");
}

module_init(wb_reset_mac_init);
module_exit(wb_reset_mac_exit);

MODULE_AUTHOR("sonic_rd <sonic_rd@ruijie.com.cn>");
MODULE_DESCRIPTION("system mac_reset driver");
MODULE_LICENSE("GPL");
