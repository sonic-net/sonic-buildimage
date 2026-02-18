#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include "wb_ssd_power.h"
#include <wb_logic_dev_common.h>
#include <wb_bsp_kernel_debug.h>

#define PROXY_NAME "wb-ssd-power"

#define SSD_POWER_MAX_NUM       (8)

typedef struct  {
    uint32_t addr;
    uint32_t en_val;
    uint32_t dis_val;
    uint32_t mask;
    uint32_t delay;
} ssd_power_wr_pro_t;

typedef struct {
    uint32_t port_id;
    const char *file_name;
    uint32_t addr;
    uint32_t power_on;
    uint32_t power_off;
    uint32_t power_mask;
    uint32_t power_on_delay;
    uint32_t power_off_delay;
    uint32_t reg_access_mode;
    struct device *dev;
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
    bool need_wr_pro;
    ssd_power_wr_pro_t wr_pro_info;
} ssd_power_node_t;

static ssd_power_node_t *g_ssd_power_n[SSD_POWER_MAX_NUM];

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

typedef int (*ata_power_reset_func_t)(unsigned int);
typedef int (*p_reg_ata_ssd_power_reset)(ata_power_reset_func_t power_reset_handle);
typedef int (*p_unreg_ata_ssd_power_reset)(void);
p_reg_ata_ssd_power_reset g_p_reg_ata_ssd_power_reset = NULL;
p_unreg_ata_ssd_power_reset g_p_unreg_ata_ssd_power_reset = NULL;

static int wb_logic_reg_write(ssd_power_node_t *node, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_write pfunc;

    pfunc = (device_func_write)node->write_intf_addr;
    return pfunc(node->file_name, pos, val, size);
}

static int wb_logic_reg_read(ssd_power_node_t *node, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    pfunc = (device_func_read)node->read_intf_addr;
    return pfunc(node->file_name, pos, val, size);
}

static int wb_ssd_power_set_wr_pro(ssd_power_node_t *node, ssd_power_wr_pro_t *wr_pro_info, int on)
{
    uint8_t wr_pro_ops;
    int ret;

    if (!node->need_wr_pro) {
        DEBUG_VERBOSE("do not need set wr pro.\n");
        return 0;
    }

    /* read origin register value */
    wr_pro_ops = 0;
    ret = wb_logic_reg_read(node, wr_pro_info->addr, &wr_pro_ops, 1);
    if (ret < 0) {
        DEBUG_ERROR("read %s, addr: 0x%x failed, ret: %d\n", node->file_name,
            wr_pro_info->addr, ret);
        return -EIO;
    }
    DEBUG_VERBOSE("read %s addr 0x%x success, value: 0x%x\n",
        node->file_name, wr_pro_info->addr, wr_pro_ops);

    /* modify register value to ssd power control */
    wr_pro_ops &= ~(wr_pro_info->mask);
    if (on == 0) {
        wr_pro_ops |= wr_pro_info->dis_val;
    } else {
        wr_pro_ops |= wr_pro_info->en_val;
    }

    /* write back to register */
    ret = wb_logic_reg_write(node, wr_pro_info->addr, &wr_pro_ops, 1);
    if (ret < 0) {
        DEBUG_ERROR("write %s, addr: 0x%x failed, ret: %d write value: 0x%x\n",
            node->file_name, wr_pro_info->addr, ret, wr_pro_ops);
        return -EIO;
    }
    DEBUG_VERBOSE("write %s addr 0x%x value: 0x%x success\n",
        node->file_name, wr_pro_info->addr, wr_pro_ops);

    if (wr_pro_info->delay > 0) {
        msleep(wr_pro_info->delay);
    }
    return 0;
}

static int wb_ssd_power_ctrl(ssd_power_node_t *node, int on)
{
    uint8_t power_ops;
    int ret;

    ret = wb_ssd_power_set_wr_pro(node, &node->wr_pro_info, 0);
    if (ret != 0) {
        dev_err(node->dev, "set wr pro failed, ret: %d\n", ret);
        return ret;
    }

    /* read origin register value */
    power_ops = 0;
    ret = wb_logic_reg_read(node, node->addr, &power_ops, 1);
    if (ret < 0) {
        DEBUG_ERROR("read %s, addr: 0x%x failed, ret: %d\n", node->file_name,
            node->addr, ret);
        return -EIO;
    }
    DEBUG_VERBOSE("read %s addr 0x%x success, value: 0x%x\n",
        node->file_name, node->addr, power_ops);

    /* modify register value to ssd power control */
    power_ops &= ~(node->power_mask);
    if (on == 0) {
        power_ops |= node->power_off;
    } else {
        power_ops |= node->power_on;
    }

    /* write back to register */
    ret = wb_logic_reg_write(node, node->addr, &power_ops, 1);
    if (ret < 0) {
        DEBUG_ERROR("write %s, addr: 0x%x failed, ret: %d write value: 0x%x\n",
            node->file_name, node->addr, ret, power_ops);
        return -EIO;
    }
    DEBUG_VERBOSE("write %s addr 0x%x value: 0x%x success\n",
        node->file_name, node->addr, power_ops);
    if ((on == 0) && (node->power_off_delay != 0)) {
        msleep(node->power_off_delay);
    } else if ((on != 0) && (node->power_on_delay != 0)) {
        msleep(node->power_on_delay);
    }

    return 0;
}

static int wb_ssd_power_reset(unsigned int port_id)
{
    ssd_power_node_t *node;
    int ret;

    if (port_id >= SSD_POWER_MAX_NUM) {
        printk(KERN_ERR "Invalid param, port_id[%u] greater than the maximum value %d, power reset failed.\n",
            port_id, SSD_POWER_MAX_NUM);
        return -EINVAL;
    }

    if (g_ssd_power_n[port_id] == NULL) {
        printk(KERN_ERR "SSD%u power reset node is NULL, power reset failed.\n", port_id);
        return -EINVAL;
    }

    node = g_ssd_power_n[port_id];

    dev_dbg(node->dev, "SSD%u power reset start\n", port_id);
    ret = wb_ssd_power_ctrl(node, 0);
    if (ret != 0) {
        dev_err(node->dev, "SSD%u power off failed, ret: %d\n", port_id, ret);
        return ret;
    }

    ret = wb_ssd_power_ctrl(node, 1);
    if (ret != 0) {
        dev_err(node->dev, "SSD%u power on failed, ret: %d\n", port_id, ret);
        return ret;
    }
    dev_info(node->dev, "SSD%u power reset success\n", port_id);
    return 0;
}

static void wb_ssd_power_node_destroy(void)
{
    int i;

    for (i = 0; i < SSD_POWER_MAX_NUM; i++) {
        if (g_ssd_power_n[i] != NULL) {
            g_ssd_power_n[i] = NULL;
        }
    }
    return;
}

static int wb_ssd_power_config_init(ssd_power_node_t *node)
{
    int ret;
    struct device *dev;
    ssd_power_device_t *ssd_power_device;
    ssd_power_wr_pro_t *ssd_power_wr_pro;
    ssd_power_wr_pro_device_t *ssd_power_wr_pro_device;

    dev = node->dev;
    if (dev->of_node) {
        ret = 0;
        ret += of_property_read_u32(node->dev->of_node, "port_id", &node->port_id);
        ret += of_property_read_u32(node->dev->of_node, "addr", &node->addr);
        ret += of_property_read_u32(node->dev->of_node, "power_on", &node->power_on);
        ret += of_property_read_u32(node->dev->of_node, "power_off", &node->power_off);
        ret += of_property_read_u32(node->dev->of_node, "power_mask", &node->power_mask);
        ret += of_property_read_u32(node->dev->of_node, "power_on_delay", &node->power_on_delay);
        ret += of_property_read_u32(node->dev->of_node, "power_off_delay", &node->power_off_delay);
        ret += of_property_read_string(node->dev->of_node, "file_name", &node->file_name);
        ret += of_property_read_u32(node->dev->of_node, "reg_access_mode", &node->reg_access_mode);
        if (ret != 0) {
            DEBUG_ERROR("get ssd power control dts config error, ret: %d\n", ret);
            return -ENXIO;
        }
    } else {
        if (dev->platform_data == NULL) {
            DEBUG_ERROR("Failed to priv platform data \n");
            return -ENXIO;
        }
        ssd_power_wr_pro = &node->wr_pro_info;
        ssd_power_device = node->dev->platform_data;
        ssd_power_wr_pro_device = &ssd_power_device->wr_pro_info;
        node->port_id = ssd_power_device->port_id;
        node->addr = ssd_power_device->addr;
        node->power_on = ssd_power_device->power_on;
        node->power_off = ssd_power_device->power_off;
        node->power_mask = ssd_power_device->power_mask;
        node->power_on_delay = ssd_power_device->power_on_delay;
        node->power_off_delay = ssd_power_device->power_off_delay;
        node->file_name = ssd_power_device->file_name;
        node->reg_access_mode = ssd_power_device->reg_access_mode;
        node->need_wr_pro = ssd_power_device->need_wr_pro;
        if (node->need_wr_pro) {
            ssd_power_wr_pro->addr = ssd_power_wr_pro_device->addr;
            ssd_power_wr_pro->en_val = ssd_power_wr_pro_device->en_val;
            ssd_power_wr_pro->dis_val = ssd_power_wr_pro_device->dis_val;
            ssd_power_wr_pro->mask = ssd_power_wr_pro_device->mask;
            ssd_power_wr_pro->delay = ssd_power_wr_pro_device->delay;
        }
    }

    ret = find_intf_addr(&node->write_intf_addr, &node->read_intf_addr, node->reg_access_mode);
    if (ret) {
        dev_err(dev, "find_intf_addr func mode %d fail, ret: %d.\n", node->reg_access_mode, ret);
        return ret;
    }

    if (!node->write_intf_addr || !node->read_intf_addr) {
        DEBUG_ERROR("Fail: func mode %u rw symbol undefined.\n", node->reg_access_mode);
        return -ENOSYS;
    }

    DEBUG_VERBOSE("wb_ssd_power_config_init success");
    return ret;
}

static int wb_ssd_power_probe(struct platform_device *pdev)
{
    ssd_power_node_t *ssd_power;
    int ret;
    uint32_t port_id;

    DEBUG_VERBOSE("wb_ssd_power_init\n");

    ssd_power = devm_kzalloc(&pdev->dev, sizeof(ssd_power_node_t), GFP_KERNEL);
    if (!ssd_power) {
        dev_err(&pdev->dev, "devm_kzalloc error.\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, ssd_power);
    ssd_power->dev = &pdev->dev;

    ret = wb_ssd_power_config_init(ssd_power);
    if (ret !=0) {
        dev_err(ssd_power->dev, "Failed to get ssd power control dts config, ret: %d\n", ret);
        return -ENXIO;
    }

    port_id = ssd_power->port_id;
    if (port_id >= SSD_POWER_MAX_NUM) {
        dev_err(ssd_power->dev, "Port id %u, beyond the limit.\n", ssd_power->port_id);
        return -EINVAL;
    }
    g_ssd_power_n[port_id] = ssd_power;

    dev_info(ssd_power->dev, "Register ssd%u power control using mode %u with %s offset address 0x%x power off delay %u ms, power on delay %u ms success.\n",
        ssd_power->port_id, ssd_power->reg_access_mode, ssd_power->file_name, ssd_power->addr, ssd_power->power_off_delay, ssd_power->power_on_delay);
    return 0;
}

static int wb_ssd_power_remove(struct platform_device *pdev)
{
    ssd_power_node_t *ssd_power;
    uint32_t port_id;

    ssd_power = platform_get_drvdata(pdev);
    port_id = ssd_power->port_id;
    g_ssd_power_n[port_id] = NULL;
    dev_info(ssd_power->dev, "Remove ssd%u power control success.\n", ssd_power->port_id);
    platform_set_drvdata(pdev, NULL);
    return 0;
}

static const struct of_device_id wb_ssd_power_driver_of_match[] = {
    { .compatible = "wb,wb-ssd-power" },
    { },
};
MODULE_DEVICE_TABLE(of, wb_ssd_power_driver_of_match);

static struct platform_driver wb_ssd_power_driver = {
    .probe      = wb_ssd_power_probe,
    .remove     = wb_ssd_power_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = PROXY_NAME,
        .of_match_table = wb_ssd_power_driver_of_match,
    },
};

static int libata_reset_ksyms_look_up(void)
{
    unsigned long sym_reg_ata_ssd_power_reset_addr;
    unsigned long sym_unreg_ata_ssd_power_reset_addr;

    sym_reg_ata_ssd_power_reset_addr = (unsigned long)kallsyms_lookup_name_fun("reg_ata_ssd_power_reset");
    if (!sym_reg_ata_ssd_power_reset_addr) {
        printk(KERN_ERR "Failed to find symbol reg_ata_ssd_power_reset.\n");
        return -ENOSYS;
    }

    sym_unreg_ata_ssd_power_reset_addr = (unsigned long)kallsyms_lookup_name_fun("unreg_ata_ssd_power_reset");
    if (!sym_unreg_ata_ssd_power_reset_addr) {
        printk(KERN_ERR "Failed to find symbol unreg_ata_ssd_power_reset.\n");
        return -ENOSYS;
    }

    g_p_reg_ata_ssd_power_reset = (p_reg_ata_ssd_power_reset)sym_reg_ata_ssd_power_reset_addr;
    g_p_unreg_ata_ssd_power_reset = (p_unreg_ata_ssd_power_reset)sym_unreg_ata_ssd_power_reset_addr;
    return 0;
}

static int __init wb_ssd_power_init(void)
{
    int i, ret;

    ret = libata_reset_ksyms_look_up();
    if (ret < 0) {
        printk(KERN_ERR "wb_ssd_power_init failed.\n");
        return ret;
    }

    for (i = 0; i < SSD_POWER_MAX_NUM; i++) {
        g_ssd_power_n[i] = NULL;
    }

    ret = g_p_reg_ata_ssd_power_reset(wb_ssd_power_reset);
    if (unlikely(ret != 0)) {
        printk(KERN_ERR "Failed to register ssd power control hook, ret: %d\n", ret);
        return ret;
    }

    return platform_driver_register(&wb_ssd_power_driver);
}

static void __exit wb_ssd_power_exit(void)
{
    wb_ssd_power_node_destroy();
    platform_driver_unregister(&wb_ssd_power_driver);
    (void)g_p_unreg_ata_ssd_power_reset();
    g_p_reg_ata_ssd_power_reset = NULL;
    g_p_unreg_ata_ssd_power_reset = NULL;
    printk(KERN_INFO "SSD power conrtol driver exit success.\n");
}

module_init(wb_ssd_power_init);
module_exit(wb_ssd_power_exit);

MODULE_DESCRIPTION("ssd power control driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
