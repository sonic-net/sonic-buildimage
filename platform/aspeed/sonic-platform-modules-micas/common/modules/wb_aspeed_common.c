// SPDX-License-Identifier: GPL-2.0-only
/*
 * Aspeed FMC(Firmware SPI Memory Controller)
 *
 * Copyright (C) 2017 Google, Inc.
 * Copyright (C) 2021 Aspeed Technology Inc.
 *
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/hwmon-sysfs.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <linux/types.h>

enum chips {
    AST2500 = 1,
    AST2600 = 2,
    AST2700 = 3,
};
static int g_debug_info = 0;
static int g_debug_err = 0;
static int g_model = AST2500;

#define AST_WDT_MAX_TIMEOUT        (600)   /* 600s */
#define WDT_RESTART_MAGIC          (0x4755)

#define AST2600_FMC_WDT2_CTRL_STATUS   (0x1E620064)
#define AST2600_FMC_WDT2_TIMER_VALUE   (0x1E620068)
#define AST2600_FMC_WDT2_TIMER_REST    (0x1E62006C)
#define AST2600_SYS_WDT1_BASE                 (0x1E785000)
#define AST2600_SYS_WDT2_BASE                 (0x1E785040)
#define AST2600_SYS_WDT_TIMEOUT_STATUS_OFF    (0x10)
#define AST2600_SYS_WDT_CLEAR_STATUS_OFF      (0x14)
#define AST2600_SYS_WDT_SOC_RST_CNT_SHIFT     (8)
#define AST2600_SYS_WDT_SOC_RST_CNT_MASK      (0xFF)
#define AST2600_SYS_WDT_CLEAR_TIMEOUT_VAL     (0x3B)
#define AST2600_SYS_WDT_CLEAR_STATUS_SHIFT    (1)
#define AST2600_SYS_WDT_CLEAR_STATUS_MASK     (0x7F)
#define AST_SYS_WDT_CLEAR_TRIGGER_VAL         (1)

enum ast_sys_wdt_id {
    AST_SYS_WDT_REBOOT = 1,
    AST_SYS_WDT_HANG = 2,
};

#define AST2520_FMC_CE_CTRL               (0x1E620004)
#define AST2520_FMC_WDT2_RESET_MASK       (0x1E78503C)
#define AST2520_FMC_WDT2_COUNTER_RELOAD   (0x1E785024)
#define AST2520_FMC_WDT2_COUNTER_RESTART  (0x1E785028)
#define AST2520_FMC_WDT2_CTRL             (0x1E78502C)
#define AST2520_FMC_WDT2_TIMEOUT_STATUS   (0x1E785030)

#define AST2700_WDTA_COUNTER_RELOAD       (0x14C37404)
#define AST2700_WDTA_COUNTER_RESTART      (0x14C37408)
#define AST2700_WDTA_CTRL                 (0x14C3740C)
#define AST2700_WDTA_SCRATCH_STATUS       (0x14C3744C)

module_param(g_debug_info, int, S_IRUGO | S_IWUSR);
module_param(g_debug_err, int, S_IRUGO | S_IWUSR);

#define WB_ASPEED_COMMON_DEBUG(fmt, args...) do {                                        \
    if (g_debug_info) { \
        printk(KERN_INFO "[wb_aspeed_common][VER][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_ASPEED_COMMON_DEBUG_ERR(fmt, args...) do {                                        \
    if (g_debug_err) { \
        printk(KERN_ERR "[wb_aspeed_common][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static ssize_t wb_aspeed_bmc_view_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint32_t tmp_val;
    char __iomem *addr_after_map;

    switch (g_model) {
    case AST2500:
        addr_after_map = ioremap(AST2520_FMC_WDT2_TIMEOUT_STATUS, 4);
        if (!addr_after_map) {
            WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
                AST2520_FMC_WDT2_TIMEOUT_STATUS);
            return -ENOMEM;
        }
        tmp_val = readl(addr_after_map);
        iounmap(addr_after_map);
        WB_ASPEED_COMMON_DEBUG("get bmc view orign value %d success\n", tmp_val);
        if ((tmp_val & 0x2) != 0) {
            tmp_val = 1;
        } else {
            tmp_val = 0;
        }
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", tmp_val);
    case AST2600:
        addr_after_map = ioremap(AST2600_FMC_WDT2_CTRL_STATUS, 4);
        if (!addr_after_map) {
            WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
                AST2600_FMC_WDT2_CTRL_STATUS);
            return -ENOMEM;
        }
        tmp_val = readl(addr_after_map);
        iounmap(addr_after_map);
        WB_ASPEED_COMMON_DEBUG("get bmc view orign value %d success\n", tmp_val);
        tmp_val = (tmp_val & 0x10) >> 4;
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", tmp_val);
    case AST2700:
        addr_after_map = ioremap(AST2700_WDTA_SCRATCH_STATUS, 4);
        if (!addr_after_map) {
            WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
                AST2700_WDTA_SCRATCH_STATUS);
            return -ENOMEM;
        }
        tmp_val = readl(addr_after_map);
        iounmap(addr_after_map);
        WB_ASPEED_COMMON_DEBUG("get bmc view orign value %d success\n", tmp_val);
        if ((tmp_val & 0x2) != 0) {
            tmp_val = 1;
        } else {
            tmp_val = 0;
        }
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", tmp_val);
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
        return -EINVAL;
    }

    WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
    return -EINVAL;
}

static int wb_ast2500_bmc_switch(int timeout)
{
    uint32_t tmp, timer_value;
    char __iomem *addr_after_map;

    /* set FMC 0 */
    tmp = 0x00;
    addr_after_map = ioremap(AST2520_FMC_CE_CTRL, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2520_FMC_CE_CTRL);
        return -ENOMEM;
    }
    writel(tmp, addr_after_map);
    iounmap(addr_after_map);

    /* reset WDT2 reset mask */
    tmp = 0x033FFFF3;
    addr_after_map = ioremap(AST2520_FMC_WDT2_RESET_MASK, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2520_FMC_WDT2_RESET_MASK);
        return -ENOMEM;
    }
    writel(tmp, addr_after_map);
    iounmap(addr_after_map);

    /* set timer value */
    timer_value = timeout * 1000 * 1000; /* The time unit is 1 us second.*/
    addr_after_map = ioremap(AST2520_FMC_WDT2_COUNTER_RELOAD, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2520_FMC_WDT2_COUNTER_RELOAD);
        return -ENOMEM;
    }
    writel(timer_value, addr_after_map);
    iounmap(addr_after_map);
    WB_ASPEED_COMMON_DEBUG("set timer value 0x%x success\n", timer_value);

    /* load Timer Reload Value */
    tmp = WDT_RESTART_MAGIC;
    addr_after_map = ioremap(AST2520_FMC_WDT2_COUNTER_RESTART, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2520_FMC_WDT2_COUNTER_RESTART);
        return -ENOMEM;
    }
    writel(tmp, addr_after_map);
    iounmap(addr_after_map);

    /* enable fmc wdt2 */
    tmp = 0xB3;
    addr_after_map = ioremap(AST2520_FMC_WDT2_CTRL, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2520_FMC_WDT2_CTRL);
        return -ENOMEM;
    }
    writel(tmp, addr_after_map);
    iounmap(addr_after_map);

    return 0;
}

static int wb_ast2600_bmc_switch(int timeout)
{
    uint32_t tmp, timer_value;
    char __iomem *addr_after_map;

    /* set timer value */
    timer_value = timeout * 10; /* The time unit is 0.1 second.*/
    addr_after_map = ioremap(AST2600_FMC_WDT2_TIMER_VALUE, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2600_FMC_WDT2_TIMER_VALUE);
        return -ENOMEM;
    }
    writel(timer_value, addr_after_map);
    iounmap(addr_after_map);
    WB_ASPEED_COMMON_DEBUG("set timer value 0x%x success\n", timer_value);

    /* load Timer Reload Value */
    tmp = WDT_RESTART_MAGIC;
    addr_after_map = ioremap(AST2600_FMC_WDT2_TIMER_REST, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2600_FMC_WDT2_TIMER_REST);
        return -ENOMEM;
    }
    writel(tmp, addr_after_map);
    iounmap(addr_after_map);

    /* enable fmc wdt2 */
    tmp = 1;
    addr_after_map = ioremap(AST2600_FMC_WDT2_CTRL_STATUS, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2600_FMC_WDT2_CTRL_STATUS);
        return -ENOMEM;
    }
    writel(tmp, addr_after_map);
    iounmap(addr_after_map);

    return 0;
}

static int wb_ast2700_bmc_switch(int timeout)
{
    uint32_t tmp, timer_value;
    char __iomem *addr_after_map;

    /* set timer value */
    timer_value = timeout * 1000 * 1000; /* The time unit is 1 us second.*/
    addr_after_map = ioremap(AST2700_WDTA_COUNTER_RELOAD, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2700_WDTA_COUNTER_RELOAD);
        return -ENOMEM;
    }
    writel(timer_value, addr_after_map);
    iounmap(addr_after_map);
    WB_ASPEED_COMMON_DEBUG("set timer value 0x%x success\n", timer_value);

    /* load Timer Reload Value */
    tmp = WDT_RESTART_MAGIC;
    addr_after_map = ioremap(AST2700_WDTA_COUNTER_RESTART, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2700_WDTA_COUNTER_RESTART);
        return -ENOMEM;
    }
    writel(tmp, addr_after_map);
    iounmap(addr_after_map);

    /* enable wdta */
    tmp = 0x13;
    addr_after_map = ioremap(AST2700_WDTA_CTRL, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            AST2700_WDTA_CTRL);
        return -ENOMEM;
    }
    writel(tmp, addr_after_map);
    iounmap(addr_after_map);

    return 0;
}

static ssize_t wb_aspeed_bmc_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int timeout;
    int ret;

    ret = kstrtoint(buf, 0, &timeout);
    if (ret != 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("Invaild timeout value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if ((timeout <= 0) || (timeout > AST_WDT_MAX_TIMEOUT)) {
        WB_ASPEED_COMMON_DEBUG_ERR("Invaild timeout value: %d, value must in range 0~%d\n",
            timeout, AST_WDT_MAX_TIMEOUT);
        return -EINVAL;
    }

    switch(g_model){
    case AST2500:
        ret = wb_ast2500_bmc_switch(timeout);
        break;
    case AST2600:
        ret = wb_ast2600_bmc_switch(timeout);
        break;
    case AST2700:
        ret = wb_ast2700_bmc_switch(timeout);
        break;
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
        ret = -EINVAL;
        break;
    }

    if (ret < 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("bmc switch failed, ret: %d, model: %d, timeout: %d\n", ret, g_model, timeout);
        return ret;
    }
    return count;
}


static ssize_t wb_aspeed_bmc_dualboot_wdt_en_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint32_t tmp_val, wdt_en_status_addr;
    char __iomem *addr_after_map;

    switch (g_model) {
    case AST2500:
        wdt_en_status_addr = AST2520_FMC_WDT2_CTRL;
        break;
    case AST2600:
        wdt_en_status_addr = AST2600_FMC_WDT2_CTRL_STATUS;
        break;
    case AST2700:
        wdt_en_status_addr = AST2700_WDTA_CTRL;
        break;
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
        return -EINVAL;
    }

    addr_after_map = ioremap(wdt_en_status_addr, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            wdt_en_status_addr);
        return -ENOMEM;
    }
    tmp_val = readl(addr_after_map);
    iounmap(addr_after_map);
    WB_ASPEED_COMMON_DEBUG("get bmc wdt enable status orign value 0x%x success\n", tmp_val);
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", (tmp_val & 0x01));
}

static ssize_t wb_aspeed_bmc_dualboot_wdt_timeout_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    uint32_t tmp_val, val, wdt_timeout_addr;
    char __iomem *addr_after_map;

    switch (g_model) {
    case AST2600:
        wdt_timeout_addr = AST2600_FMC_WDT2_CTRL_STATUS;
        break;
    case AST2500:
    case AST2700:
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
        return -EINVAL;
    }

    addr_after_map = ioremap(wdt_timeout_addr, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            wdt_timeout_addr);
        return -ENOMEM;
    }
    tmp_val = readl(addr_after_map);
    iounmap(addr_after_map);
    val = (tmp_val >> 8) & 0xff;
    WB_ASPEED_COMMON_DEBUG("get bmc dualboot wdt timeout value 0x%x success, val %u\n", tmp_val, val);
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", val);
}


static ssize_t wb_aspeed_bmc_dualboot_wdt_en_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret, value;
    uint32_t wdt_ctrl_addr;
    char __iomem *addr_after_map;

    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("Invaild value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (value != 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("Invaild value: %d, value must 0\n", value);
        return -EINVAL;
    }

    switch (g_model) {
    case AST2500:
        wdt_ctrl_addr = AST2520_FMC_WDT2_CTRL;
        break;
    case AST2600:
        wdt_ctrl_addr = AST2600_FMC_WDT2_CTRL_STATUS;
        break;
    case AST2700:
        wdt_ctrl_addr = AST2700_WDTA_CTRL;
        break;
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
        return -EINVAL;
    }

    addr_after_map = ioremap(wdt_ctrl_addr, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            wdt_ctrl_addr);
        return -ENOMEM;
    }
    writel(value, addr_after_map);
    iounmap(addr_after_map);
    dev_info(dev, "Disable dual boot watchdog.\n");
    return count;
}

static ssize_t wb_aspeed_bmc_dualboot_wdt_timeout_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int ret;
    int value;
    uint32_t tmp_val;
    uint32_t wdt_ctrl_addr;
    char __iomem *addr_after_map;

    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("Invaild clear value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (value != 1) {
        WB_ASPEED_COMMON_DEBUG_ERR("Invaild clear value: %d, value must 1\n", value);
        return -EINVAL;
    }

    switch (g_model) {
    case AST2600:
        wdt_ctrl_addr = AST2600_FMC_WDT2_CTRL_STATUS;
        break;
    case AST2500:
    case AST2700:
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
        return -EINVAL;
    }

    addr_after_map = ioremap(wdt_ctrl_addr, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n", wdt_ctrl_addr);
        return -ENOMEM;
    }

    tmp_val = readl(addr_after_map);
    tmp_val &= ~(0xffU << 24);
    tmp_val |= (0xeaU << 24);
    writel(tmp_val, addr_after_map);
    iounmap(addr_after_map);

    WB_ASPEED_COMMON_DEBUG("clear bmc dualboot wdt timeout, write 0x%x\n", tmp_val);
    return count;
}

static int wb_aspeed_sys_wdt_base_get(enum ast_sys_wdt_id wdt_id, uint32_t *wdt_base)
{
    switch (g_model) {
    case AST2600:
        if (wdt_id == AST_SYS_WDT_REBOOT) {
            *wdt_base = AST2600_SYS_WDT1_BASE;
        } else if (wdt_id == AST_SYS_WDT_HANG) {
            *wdt_base = AST2600_SYS_WDT2_BASE;
        } else {
            WB_ASPEED_COMMON_DEBUG_ERR("invalid wdt id %u\n", wdt_id);
            return -EINVAL;
        }
        break;
    case AST2500:
    case AST2700:
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
        return -EINVAL;
    }

    return 0;
}

static int wb_aspeed_sys_wdt_timeout_counter_get(enum ast_sys_wdt_id wdt_id, uint32_t *counter)
{
    int ret;
    uint32_t wdt_base;
    uint32_t tmp_val;
    uint32_t status_off, shift, mask;
    char __iomem *addr_after_map;

    ret = wb_aspeed_sys_wdt_base_get(wdt_id, &wdt_base);
    if (ret < 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("get sys wdt base failed, ret: %d, model: %d, wdt_id: %u\n",
            ret, g_model, wdt_id);
        return ret;
    }

    switch (g_model) {
    case AST2600:
        status_off = AST2600_SYS_WDT_TIMEOUT_STATUS_OFF;
        shift = AST2600_SYS_WDT_SOC_RST_CNT_SHIFT;
        mask = AST2600_SYS_WDT_SOC_RST_CNT_MASK;
        break;
    case AST2500:
    case AST2700:
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
        return -EINVAL;
    }

    addr_after_map = ioremap(wdt_base + status_off, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            (wdt_base + status_off));
        return -ENOMEM;
    }

    tmp_val = readl(addr_after_map);
    iounmap(addr_after_map);
    *counter = (tmp_val >> shift) & mask;

    WB_ASPEED_COMMON_DEBUG("get sys wdt timeout status 0x%x success, soc_rst_cnt %u\n",
        tmp_val, *counter);
    return 0;
}


static int wb_aspeed_sys_wdt_timeout_counter_clear(enum ast_sys_wdt_id wdt_id)
{
    int ret;
    uint32_t wdt_base;
    uint32_t tmp_val;
    uint32_t clear_off, clear_shift, clear_mask, clear_val;
    char __iomem *addr_after_map;

    ret = wb_aspeed_sys_wdt_base_get(wdt_id, &wdt_base);
    if (ret < 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("get sys wdt base failed, ret: %d, model: %d, wdt_id: %u\n",
            ret, g_model, wdt_id);
        return ret;
    }

    switch (g_model) {
    case AST2600:
        clear_off = AST2600_SYS_WDT_CLEAR_STATUS_OFF;
        clear_shift = AST2600_SYS_WDT_CLEAR_STATUS_SHIFT;
        clear_mask = AST2600_SYS_WDT_CLEAR_STATUS_MASK;
        clear_val = AST2600_SYS_WDT_CLEAR_TIMEOUT_VAL;
        break;
    case AST2500:
    case AST2700:
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("unsupport model %d\n", g_model);
        return -EINVAL;
    }

    addr_after_map = ioremap(wdt_base + clear_off, 4);
    if (!addr_after_map) {
        WB_ASPEED_COMMON_DEBUG_ERR("ioremap failed, addr: 0x%x\n",
            (wdt_base + clear_off));
        return -ENOMEM;
    }

    tmp_val = readl(addr_after_map);
    tmp_val &= ~(clear_mask << clear_shift);
    tmp_val |= (clear_val << clear_shift);
    writel(tmp_val, addr_after_map);
    iounmap(addr_after_map);

    WB_ASPEED_COMMON_DEBUG("clear sys wdt timeout status, write 0x%x\n", tmp_val);
    return 0;
}

static ssize_t wb_aspeed_sys_wdt_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    uint32_t counter;
    struct sensor_device_attribute *wdt_attr;
    enum ast_sys_wdt_id wdt_id;

    wdt_attr = to_sensor_dev_attr(attr);
    wdt_id = (enum ast_sys_wdt_id)wdt_attr->index;

    ret = wb_aspeed_sys_wdt_timeout_counter_get(wdt_id, &counter);
    if (ret < 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("get sys wdt timeout counter failed, ret: %d, model: %d, wdt_id: %u\n",
            ret, g_model, wdt_id);
        return ret;
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", counter);
}

static ssize_t wb_aspeed_sys_wdt_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int ret;
    int value;
    struct sensor_device_attribute *wdt_attr;
    enum ast_sys_wdt_id wdt_id;

    wdt_attr = to_sensor_dev_attr(attr);
    wdt_id = (enum ast_sys_wdt_id)wdt_attr->index;

    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("Invaild clear value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (value != AST_SYS_WDT_CLEAR_TRIGGER_VAL) {
        WB_ASPEED_COMMON_DEBUG_ERR("Invaild clear value: %d, value must 1\n", value);
        return -EINVAL;
    }

    ret = wb_aspeed_sys_wdt_timeout_counter_clear(wdt_id);
    if (ret < 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("clear sys wdt timeout counter failed, ret: %d, model: %d, wdt_id: %u\n",
            ret, g_model, wdt_id);
        return ret;
    }

    return count;
}

static SENSOR_DEVICE_ATTR(sys_wdt_reboot_timeout, S_IRUGO | S_IWUSR, wb_aspeed_sys_wdt_show, wb_aspeed_sys_wdt_store, AST_SYS_WDT_REBOOT);
static SENSOR_DEVICE_ATTR(sys_wdt_hang_timeout, S_IRUGO | S_IWUSR, wb_aspeed_sys_wdt_show, wb_aspeed_sys_wdt_store, AST_SYS_WDT_HANG);

static DEVICE_ATTR(bmc_switch, S_IWUSR, NULL, wb_aspeed_bmc_switch_store);
static DEVICE_ATTR(bmc_view, S_IRUGO, wb_aspeed_bmc_view_show, NULL);
static DEVICE_ATTR(bmc_dualboot_wdt_en, S_IRUGO | S_IWUSR, wb_aspeed_bmc_dualboot_wdt_en_show, wb_aspeed_bmc_dualboot_wdt_en_store);
static DEVICE_ATTR(bmc_dualboot_wdt_timeout, S_IRUGO | S_IWUSR, wb_aspeed_bmc_dualboot_wdt_timeout_show, wb_aspeed_bmc_dualboot_wdt_timeout_store);

static struct attribute * wb_aspeed_attrs[] = {
    &dev_attr_bmc_switch.attr,
    &dev_attr_bmc_view.attr,
    &dev_attr_bmc_dualboot_wdt_en.attr,
    &dev_attr_bmc_dualboot_wdt_timeout.attr,
    &sensor_dev_attr_sys_wdt_reboot_timeout.dev_attr.attr,
    &sensor_dev_attr_sys_wdt_hang_timeout.dev_attr.attr,
    NULL,
};
static struct attribute_group wb_aspeed_attr_group = {
    .attrs = wb_aspeed_attrs,
};

static int wb_aspeed_common_probe(struct platform_device *pdev)
{
    int chip_id, status;

    if (pdev->dev.of_node) {
        chip_id = (enum chips)of_device_get_match_data(&pdev->dev);
    } else {
        WB_ASPEED_COMMON_DEBUG_ERR("currently not support!\n");
        return -ENODEV;
    }

    switch (chip_id) {
    case AST2500:
        g_model = AST2500;
        break;
    case AST2600:
        g_model = AST2600;
        break;
    case AST2700:
        g_model = AST2700;
        break;
    default:
        WB_ASPEED_COMMON_DEBUG_ERR("chip type %d is not supported!\n", chip_id);
        return -ENODEV;
    }

    status = sysfs_create_group(&pdev->dev.kobj, &wb_aspeed_attr_group);
    if (status != 0) {
        WB_ASPEED_COMMON_DEBUG_ERR("create bmc attr error!\n");
        return -EBADRQC;
    }

	return status;
}

static void wb_aspeed_common_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &wb_aspeed_attr_group);
}

static const struct of_device_id wb_aspeed_common_matches[] = {
    { .compatible = "aspeed,ast2500-wb-common", .data = (void *)AST2500},
    { .compatible = "aspeed,ast2600-wb-common", .data = (void *)AST2600},
    { .compatible = "aspeed,ast2700-wb-common", .data = (void *)AST2700},
    {},
};

MODULE_DEVICE_TABLE(of, wb_aspeed_common_matches);

static struct platform_driver wb_aspeed_common_driver = {
	.probe = wb_aspeed_common_probe,
	.driver = {
		.name = "wb_aspeed_common",
		.of_match_table = wb_aspeed_common_matches,
	},
	.remove     = wb_aspeed_common_remove,
};

module_platform_driver(wb_aspeed_common_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("wb aspeed common Driver");
MODULE_LICENSE("GPL");
