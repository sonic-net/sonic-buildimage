/*
 * An wb_pcie_dev driver for pcie device function
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/uio.h>

#include "wb_pcie_dev.h"

#define PROXY_NAME "wb-pci-dev"
#define MAX_NAME_SIZE            (20)
#define MAX_PCIE_NUM             (256)
#define PCI_RDWR_MAX_LEN         (256)
#define PCIE_BUS_WIDTH_1         (1)
#define PCIE_BUS_WIDTH_2         (2)
#define PCIE_BUS_WIDTH_4         (4)

#define KERNEL_SPACE         (0)
#define USER_SPACE           (1)

#define SEARCH_DEV_DEFAULT       (0)
#define SEARCH_DEV_BY_BRIDGE     (1)

#define SECBUS                   (0x19)
#define SUBBUS                   (0x1a)

static int g_pcie_dev_debug = 0;
static int g_pcie_dev_error = 0;

module_param(g_pcie_dev_debug, int, S_IRUGO | S_IWUSR);
module_param(g_pcie_dev_error, int, S_IRUGO | S_IWUSR);

#define PCIE_DEV_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_pcie_dev_debug) { \
        printk(KERN_INFO "[PCIE_DEV][VER][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PCIE_DEV_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_pcie_dev_error) { \
        printk(KERN_ERR "[PCIE_DEV][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

typedef struct firmware_upg_s {
    int upg_ctrl_base;
    int upg_flash_base;
} firmware_upg_t;

typedef struct wb_pci_dev_s {
    const char *name;
    uint32_t domain;
    uint32_t bus;
    uint32_t slot;
    uint32_t fn;
    uint32_t bar;
    void __iomem *pci_mem_base;
    uint32_t pci_io_base;
    uint32_t bar_len;
    uint32_t bar_flag;
    uint32_t bus_width;
    uint32_t check_pci_id;
    uint32_t pci_id;
    uint32_t search_mode;
    uint32_t bridge_bus;
    uint32_t bridge_slot;
    uint32_t bridge_fn;
    struct miscdevice misc;
    void (*setreg)(struct wb_pci_dev_s *wb_pci_dev, int reg, u32 value);
    u32 (*getreg)(struct wb_pci_dev_s *wb_pci_dev, int reg);
    firmware_upg_t firmware_upg;
} wb_pci_dev_t;

static wb_pci_dev_t* pcie_dev_arry[MAX_PCIE_NUM];

static void pci_dev_setreg_8(wb_pci_dev_t *wb_pci_dev, int reg, u32 value)
{
    u8 w_value;

    w_value = (u8)(value & 0xff);
    if (wb_pci_dev->bar_flag == IORESOURCE_MEM) {
        writeb(w_value, wb_pci_dev->pci_mem_base + reg);
    } else {
        outb(w_value, wb_pci_dev->pci_io_base + reg);
    }
    return;
}

static void pci_dev_setreg_16(wb_pci_dev_t *wb_pci_dev, int reg, u32 value)
{
    u16 w_value;

    w_value = (u16)(value & 0xffff);
    if (wb_pci_dev->bar_flag == IORESOURCE_MEM) {
        writew(w_value, wb_pci_dev->pci_mem_base + reg);
    } else {
        outw(w_value, wb_pci_dev->pci_io_base + reg);
    }

    return;
}

static void pci_dev_setreg_32(wb_pci_dev_t *wb_pci_dev, int reg, u32 value)
{

    if (wb_pci_dev->bar_flag == IORESOURCE_MEM) {
        writel(value, wb_pci_dev->pci_mem_base + reg);
    } else {
        outl(value, wb_pci_dev->pci_io_base + reg);
    }
    return;
}

static inline u32 pci_dev_getreg_8(wb_pci_dev_t *wb_pci_dev, int reg)
{
    u32 value;

    if (wb_pci_dev->bar_flag == IORESOURCE_MEM) {
        value = readb(wb_pci_dev->pci_mem_base + reg);
    } else {
        value = inb(wb_pci_dev->pci_io_base + reg);
    }

    return value;
}

static inline u32 pci_dev_getreg_16(wb_pci_dev_t *wb_pci_dev, int reg)
{
    u32 value;

    if (wb_pci_dev->bar_flag == IORESOURCE_MEM) {
        value = readw(wb_pci_dev->pci_mem_base + reg);
    } else {
        value = inw(wb_pci_dev->pci_io_base + reg);
    }

    return value;
}

static inline u32 pci_dev_getreg_32(wb_pci_dev_t *wb_pci_dev, int reg)
{
    u32 value;

    if (wb_pci_dev->bar_flag == IORESOURCE_MEM) {
        value = readl(wb_pci_dev->pci_mem_base + reg);
    } else {
        value = inl(wb_pci_dev->pci_io_base + reg);
    }

    return value;
}

static inline void pci_dev_setreg(wb_pci_dev_t *wb_pci_dev, int reg, u32 value)
{
    wb_pci_dev->setreg(wb_pci_dev, reg, value);
}

static inline u32 pci_dev_getreg(wb_pci_dev_t *wb_pci_dev, int reg)
{
    return wb_pci_dev->getreg(wb_pci_dev, reg);
}

static int pci_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    wb_pci_dev_t *wb_pci_dev;

    PCIE_DEV_DEBUG_VERBOSE("inode: %p, file: %p, minor: %u", inode, file, minor);

    if (minor >= MAX_PCIE_NUM) {
        PCIE_DEV_DEBUG_ERROR("minor out of range, minor = %d.\n", minor);
        return -ENODEV;
    }

    wb_pci_dev = pcie_dev_arry[minor];
    if (wb_pci_dev == NULL) {
        PCIE_DEV_DEBUG_ERROR("wb_pci_dev is NULL, open failed, minor = %d\n", minor);
        return -ENODEV;
    }

    file->private_data = wb_pci_dev;
    return 0;
}

static int pci_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}

static int pci_dev_read_tmp(wb_pci_dev_t *wb_pci_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int width, i, j;
    u32 val;

    if (offset > wb_pci_dev->bar_len) {
        PCIE_DEV_DEBUG_VERBOSE("offset:0x%x, bar len:0x%x, EOF.\n", offset, wb_pci_dev->bar_len);
        return 0;
    }

    width = wb_pci_dev->bus_width;

    if (offset % width) {
        PCIE_DEV_DEBUG_ERROR("pci bus width:%d, offset:0x%x, read size %lu invalid.\n",
            width, offset, count);
        return -EINVAL;
    }

    if (count > wb_pci_dev->bar_len - offset) {
        PCIE_DEV_DEBUG_VERBOSE("read count out of range. input len:%lu, read len:%u.\n",
            count, wb_pci_dev->bar_len - offset);
        count = wb_pci_dev->bar_len - offset;
    }

    for (i = 0; i < count; i += width) {
        val = pci_dev_getreg(wb_pci_dev, offset + i);
        for (j = 0; (j < width) && (i + j < count); j++) {
            buf[i + j] = (val >> (8 * j)) & 0xff;
        }
    }
    return count;
}

static ssize_t pci_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset, int flag)
{
    wb_pci_dev_t *wb_pci_dev;
    int ret, read_len;
    u8 buf_tmp[PCI_RDWR_MAX_LEN];

    wb_pci_dev = file->private_data;
    if (wb_pci_dev == NULL) {
        PCIE_DEV_DEBUG_ERROR("wb_pci_dev is NULL, read failed.\n");
        return -EINVAL;
    }

    if (count == 0) {
        PCIE_DEV_DEBUG_ERROR("Invalid params, read count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(buf_tmp)) {
        PCIE_DEV_DEBUG_VERBOSE("read count %lu exceed max %lu.\n", count, sizeof(buf_tmp));
        count = sizeof(buf_tmp);
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));
    read_len = pci_dev_read_tmp(wb_pci_dev, *offset, buf_tmp, count);
    if (read_len < 0) {
        PCIE_DEV_DEBUG_ERROR("pci_dev_read_tmp failed, ret:%d.\n", read_len);
        return read_len;
    }
    /* check flag is user spase or kernel spase */
    if (flag == USER_SPACE) {
        PCIE_DEV_DEBUG_VERBOSE("user space read, buf: %p, offset: %lld, read count %lu.\n",
            buf, *offset, count);
        if (copy_to_user(buf, buf_tmp, read_len)) {
            PCIE_DEV_DEBUG_ERROR("copy_to_user failed.\n");
            return -EFAULT;
        }
    } else {
        PCIE_DEV_DEBUG_VERBOSE("kernel space read, buf: %p, offset: %lld, read count %lu.\n",
            buf, *offset, count);
        memcpy(buf, buf_tmp, read_len);
    }
    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t pci_dev_read_user(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int ret;

    PCIE_DEV_DEBUG_VERBOSE("pci_dev_read_user, file: %p, count: %lu, offset: %lld\n",
        file, count, *offset);
    ret = pci_dev_read(file, buf, count, offset, USER_SPACE);
    return ret;
}

static ssize_t pci_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    int ret;

    PCIE_DEV_DEBUG_VERBOSE("pci_dev_read_iter, file: %p, count: %lu, offset: %lld\n",
        iocb->ki_filp, to->count, iocb->ki_pos);
    ret = pci_dev_read(iocb->ki_filp, to->kvec->iov_base, to->count, &iocb->ki_pos, KERNEL_SPACE);
    return ret;
}

static int pci_dev_write_tmp(wb_pci_dev_t *wb_pci_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int width, i, j;
    u32 val;

    if (offset > wb_pci_dev->bar_len) {
        PCIE_DEV_DEBUG_VERBOSE("offset:0x%x, bar len:0x%x, EOF.\n", offset, wb_pci_dev->bar_len);
        return 0;
    }

    width = wb_pci_dev->bus_width;

    if (offset % width) {
        PCIE_DEV_DEBUG_ERROR("pci bus width:%d, offset:0x%x, read size %lu invalid.\n",
            width, offset, count);
        return -EINVAL;
    }

    if (count > wb_pci_dev->bar_len - offset) {
        PCIE_DEV_DEBUG_VERBOSE("write count out of range. input len:%lu, write len:%u.\n",
            count, wb_pci_dev->bar_len - offset);
        count = wb_pci_dev->bar_len - offset;
    }

    for (i = 0; i < count; i += width) {
        val = 0;
        for (j = 0; (j < width) && (i + j < count); j++) {
            val |= buf[i + j] << (8 * j);
        }
        pci_dev_setreg(wb_pci_dev, i + offset, val);
    }

    return count;
}

static ssize_t pci_dev_write(struct file *file, const char __user *buf, size_t count,
                   loff_t *offset, int flag)
{
    wb_pci_dev_t *wb_pci_dev;
    u8 buf_tmp[PCI_RDWR_MAX_LEN];
    int write_len;

    wb_pci_dev = file->private_data;
    if (wb_pci_dev == NULL) {
        PCIE_DEV_DEBUG_ERROR("wb_pci_dev is NULL, write failed.\n");
        return -EINVAL;
    }

    if (count == 0) {
        PCIE_DEV_DEBUG_ERROR("Invalid params, write count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(buf_tmp)) {
        PCIE_DEV_DEBUG_VERBOSE("write count %lu exceed max %lu.\n", count, sizeof(buf_tmp));
        count = sizeof(buf_tmp);
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));
    /* check flag is user spase or kernel spase */
    if (flag == USER_SPACE) {
        PCIE_DEV_DEBUG_VERBOSE("user space write, buf: %p, offset: %lld, write count %lu.\n",
            buf, *offset, count);
        if (copy_from_user(buf_tmp, buf, count)) {
            PCIE_DEV_DEBUG_ERROR("copy_from_user failed.\n");
            return -EFAULT;
        }
    } else {
        PCIE_DEV_DEBUG_VERBOSE("kernel space write, buf: %p, offset: %lld, write count %lu.\n",
            buf, *offset, count);
        memcpy(buf_tmp, buf, count);
    }

    write_len = pci_dev_write_tmp(wb_pci_dev, *offset, buf_tmp, count);
    if (write_len < 0) {
        PCIE_DEV_DEBUG_ERROR("pci_dev_write_tmp failed, ret:%d.\n", write_len);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t pci_dev_write_user(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    int ret;

    PCIE_DEV_DEBUG_VERBOSE("pci_dev_write_user, file: %p, count: %lu, offset: %lld\n",
        file, count, *offset);
    ret = pci_dev_write(file, buf, count, offset, USER_SPACE);
    return ret;
}

static ssize_t pci_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    int ret;

    PCIE_DEV_DEBUG_VERBOSE("pci_dev_write_iter, file: %p, count: %lu, offset: %lld\n",
        iocb->ki_filp, from->count, iocb->ki_pos);
    ret = pci_dev_write(iocb->ki_filp, from->kvec->iov_base, from->count, &iocb->ki_pos, KERNEL_SPACE);
    return ret;
}

static loff_t pci_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    wb_pci_dev_t *wb_pci_dev;

    wb_pci_dev = file->private_data;
    if (wb_pci_dev == NULL) {
        PCIE_DEV_DEBUG_ERROR("wb_pci_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            PCIE_DEV_DEBUG_ERROR("SEEK_SET, offset:%lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > wb_pci_dev->bar_len) {
            PCIE_DEV_DEBUG_ERROR("SEEK_SET out of range, offset:%lld, bar len:0x%x.\n",
                offset, wb_pci_dev->bar_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > wb_pci_dev->bar_len) || ((file->f_pos + offset) < 0)) {
            PCIE_DEV_DEBUG_ERROR("SEEK_CUR out of range, f_ops:%lld, offset:%lld, bar len:0x%x.\n",
                file->f_pos, offset, wb_pci_dev->bar_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        PCIE_DEV_DEBUG_ERROR("unsupport llseek type:%d.\n", origin);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static long pci_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    wb_pci_dev_t *wb_pci_dev;
    void __user *argp;
    firmware_upg_t *firmware_upg;
    int upg_ctrl_base;
    int upg_flash_base;

    PCIE_DEV_DEBUG_VERBOSE("ioctl, cmd=0x%02x, arg=0x%02lx\n",cmd, arg);

    wb_pci_dev = file->private_data;
    if (wb_pci_dev == NULL) {
        PCIE_DEV_DEBUG_ERROR("wb_pci_dev is NULL, ioctl failed.\n");
        return -EINVAL;
    }

    firmware_upg = &wb_pci_dev->firmware_upg;

    argp = (void __user *)arg;

    switch (cmd) {
    case GET_FPGA_UPG_CTL_BASE:
        if (firmware_upg->upg_ctrl_base < 0) {
            PCIE_DEV_DEBUG_ERROR("dts not adaptive upg_ctrl_base\n");
            return -EFAULT;
        } else {
            upg_ctrl_base = firmware_upg->upg_ctrl_base;
            if (copy_to_user(argp, &upg_ctrl_base, sizeof(upg_ctrl_base))) {
                PCIE_DEV_DEBUG_ERROR("upg_ctrl_base copy_from_user failed\n");
                return -EFAULT;
            }
        }
        break;
    case GET_FPGA_UPG_FLASH_BASE:
        if (firmware_upg->upg_flash_base < 0) {
            PCIE_DEV_DEBUG_ERROR("dts not adaptive upg_flash_base\n");
            return -EFAULT;
        } else {
            upg_flash_base = firmware_upg->upg_flash_base;
            if (copy_to_user(argp, &upg_flash_base, sizeof(upg_flash_base))) {
                PCIE_DEV_DEBUG_ERROR("upg_flash_base copy_from_user failed\n");
                return -EFAULT;
            }
        }
        break;
    default:
        PCIE_DEV_DEBUG_ERROR("command unsupported \n");
        return -ENOTTY;
    }

    return 0;
}

static const struct file_operations pcie_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = pci_dev_llseek,
    .read       = pci_dev_read_user,
    .write      = pci_dev_write_user,
    .read_iter  = pci_dev_read_iter,
    .write_iter = pci_dev_write_iter,
    .unlocked_ioctl = pci_dev_ioctl,
    .open       = pci_dev_open,
    .release    = pci_dev_release,
};

static wb_pci_dev_t *dev_match(const char *path)
{
    wb_pci_dev_t *wb_pci_dev;
    char dev_name[MAX_NAME_SIZE];
    int i;

    for (i = 0; i < MAX_PCIE_NUM; i++) {
        if (pcie_dev_arry[i] == NULL) {
            continue;
        }
        wb_pci_dev = pcie_dev_arry[i];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", wb_pci_dev->name);
        if (!strcmp(path, dev_name)) {
            PCIE_DEV_DEBUG_VERBOSE("get dev_name = %s, minor = %d\n", dev_name, i);
            return wb_pci_dev;
        }
    }

    return NULL;
}

int pcie_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    wb_pci_dev_t *wb_pci_dev;
    int read_len;

    if (path == NULL) {
        PCIE_DEV_DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        PCIE_DEV_DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    wb_pci_dev = dev_match(path);
    if (wb_pci_dev == NULL) {
        PCIE_DEV_DEBUG_ERROR("i2c_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    read_len = pci_dev_read_tmp(wb_pci_dev, offset, buf, count);
    if (read_len < 0) {
        PCIE_DEV_DEBUG_ERROR("pci_dev_read_tmp failed, ret:%d.\n", read_len);
    }
    return read_len;
}
EXPORT_SYMBOL(pcie_device_func_read);

int pcie_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    wb_pci_dev_t *wb_pci_dev;
    int write_len;

    if (path == NULL) {
        PCIE_DEV_DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        PCIE_DEV_DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    wb_pci_dev = dev_match(path);
    if (wb_pci_dev == NULL) {
        PCIE_DEV_DEBUG_ERROR("i2c_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    write_len = pci_dev_write_tmp(wb_pci_dev, offset, buf, count);
    if (write_len < 0) {
        PCIE_DEV_DEBUG_ERROR("pci_dev_write_tmp failed, ret:%d.\n", write_len);
    }
    return write_len;
}
EXPORT_SYMBOL(pcie_device_func_write);

static int pci_setup_bars(wb_pci_dev_t *wb_pci_dev, struct pci_dev *dev)
{
    int ret;
    uint32_t addr, len, flags;

    ret = 0;
    addr = pci_resource_start(dev, wb_pci_dev->bar);
    len = pci_resource_len(dev, wb_pci_dev->bar);
    if (addr == 0 || len == 0) {
        PCIE_DEV_DEBUG_ERROR("get bar addr failed. bar:%d, addr:0x%x, len:0x%x.\n",
            wb_pci_dev->bar, addr, len);
        return -EFAULT;
    }
    wb_pci_dev->bar_len = len;

    flags = pci_resource_flags(dev, wb_pci_dev->bar);
    PCIE_DEV_DEBUG_VERBOSE("bar:%d, flag:0x%08x, phys addr:0x%x, len:0x%x\n",
        wb_pci_dev->bar, flags, addr, len);
    if (flags & IORESOURCE_MEM) {
        wb_pci_dev->bar_flag = IORESOURCE_MEM;
        wb_pci_dev->pci_mem_base = ioremap(addr, len);
        PCIE_DEV_DEBUG_VERBOSE("pci mem base:%p.\n", wb_pci_dev->pci_mem_base);
    } else if (flags & IORESOURCE_IO) {
        wb_pci_dev->bar_flag = IORESOURCE_IO;
        wb_pci_dev->pci_io_base = addr;
        PCIE_DEV_DEBUG_VERBOSE("pci io base:0x%x.\n", wb_pci_dev->pci_io_base);
    } else {
        PCIE_DEV_DEBUG_ERROR("unknow pci bar flag:0x%08x.\n", flags);
        ret = -EINVAL;
    }

    return ret;
}

static int pci_dev_probe(struct platform_device *pdev)
{
    int ret, devfn;
    uint32_t pci_id;
    wb_pci_dev_t *wb_pci_dev;
    struct pci_dev *pci_dev, *pci_bridge_dev;
    struct miscdevice *misc;
    firmware_upg_t *firmware_upg;
    pci_dev_device_t *pci_dev_device;
    u8 secbus_val, subbus_val;

    wb_pci_dev = devm_kzalloc(&pdev->dev, sizeof(wb_pci_dev_t), GFP_KERNEL);
    if (!wb_pci_dev) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        ret = -ENOMEM;
        return ret;
    }

    firmware_upg = &wb_pci_dev->firmware_upg;

    if (pdev->dev.of_node) {
        ret = 0;
        ret += of_property_read_string(pdev->dev.of_node, "pci_dev_name", &wb_pci_dev->name);
        ret += of_property_read_u32(pdev->dev.of_node, "pci_domain", &wb_pci_dev->domain);
        ret += of_property_read_u32(pdev->dev.of_node, "pci_slot", &wb_pci_dev->slot);
        ret += of_property_read_u32(pdev->dev.of_node, "pci_fn", &wb_pci_dev->fn);
        ret += of_property_read_u32(pdev->dev.of_node, "pci_bar", &wb_pci_dev->bar);
        ret += of_property_read_u32(pdev->dev.of_node, "bus_width", &wb_pci_dev->bus_width);

        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get dts config, ret:%d.\n", ret);
            return -ENXIO;
        }

        wb_pci_dev->search_mode = SEARCH_DEV_DEFAULT;
        of_property_read_u32(pdev->dev.of_node, "search_mode", &wb_pci_dev->search_mode);
        ret = 0;
        if (wb_pci_dev->search_mode == SEARCH_DEV_BY_BRIDGE) {
            ret += of_property_read_u32(pdev->dev.of_node, "bridge_bus", &wb_pci_dev->bridge_bus);
            ret += of_property_read_u32(pdev->dev.of_node, "bridge_slot", &wb_pci_dev->bridge_slot);
            ret += of_property_read_u32(pdev->dev.of_node, "bridge_fn", &wb_pci_dev->bridge_fn);
            if (ret != 0) {
                PCIE_DEV_DEBUG_VERBOSE("get pci bridge config fail, ret:%d.\n", ret);
                return -ENXIO;
            } else {
                PCIE_DEV_DEBUG_VERBOSE("bridge_bus:0x%02x, bridge_slot:0x%02x, bridge_fn:0x%02x.\n",
                    wb_pci_dev->bridge_bus, wb_pci_dev->bridge_slot, wb_pci_dev->bridge_fn);
            }
        } else {
            ret += of_property_read_u32(pdev->dev.of_node, "pci_bus", &wb_pci_dev->bus);
            if (ret != 0) {
                PCIE_DEV_DEBUG_VERBOSE("get pci bus config fail, ret:%d.\n", ret);
                return -ENXIO;
            } else {
                PCIE_DEV_DEBUG_VERBOSE("get pci_bus:0x%02x.\n", wb_pci_dev->bus);
            }
        }

        ret = 0;
        ret += of_property_read_u32(pdev->dev.of_node, "upg_ctrl_base", &firmware_upg->upg_ctrl_base);
        ret += of_property_read_u32(pdev->dev.of_node, "upg_flash_base", &firmware_upg->upg_flash_base);
        if (ret != 0) {
            PCIE_DEV_DEBUG_VERBOSE("dts don't adaptive fpga upg related, ret:%d.\n", ret);
            firmware_upg->upg_ctrl_base = -1;
            firmware_upg->upg_flash_base = -1;
        } else {
            PCIE_DEV_DEBUG_VERBOSE("upg_ctrl_base:0x%04x, upg_flash_base:0x%02x.\n",
                firmware_upg->upg_ctrl_base, firmware_upg->upg_flash_base);
        }
        ret = of_property_read_u32(pdev->dev.of_node, "check_pci_id", &wb_pci_dev->check_pci_id);
        if (ret == 0) {
            ret = of_property_read_u32(pdev->dev.of_node, "pci_id", &wb_pci_dev->pci_id);
            if (ret != 0) {
                dev_err(&pdev->dev, "Failed to get pci_id, ret:%d.\n", ret);
                return -ENXIO;
            }
        }
    } else {
        if (pdev->dev.platform_data == NULL) {
            dev_err(&pdev->dev, "Failed to get platform data config.\n");
            return -ENXIO;
        }
        pci_dev_device = pdev->dev.platform_data;
        wb_pci_dev->name = pci_dev_device->pci_dev_name;
        wb_pci_dev->domain = pci_dev_device->pci_domain;
        wb_pci_dev->slot = pci_dev_device->pci_slot;
        wb_pci_dev->fn = pci_dev_device->pci_fn;
        wb_pci_dev->bar = pci_dev_device->pci_bar;
        wb_pci_dev->bus_width = pci_dev_device->bus_width;
        wb_pci_dev->check_pci_id = pci_dev_device->check_pci_id;
        wb_pci_dev->search_mode = pci_dev_device->search_mode;
        firmware_upg->upg_ctrl_base = pci_dev_device->upg_ctrl_base;
        firmware_upg->upg_flash_base = pci_dev_device->upg_flash_base;
        if (wb_pci_dev->search_mode == SEARCH_DEV_BY_BRIDGE) {
            PCIE_DEV_DEBUG_VERBOSE("bridge_bus:0x%02x, bridge_slot:0x%02x, bridge_fn:0x%02x.\n",
                    wb_pci_dev->bridge_bus, wb_pci_dev->bridge_slot, wb_pci_dev->bridge_fn);
        }
        PCIE_DEV_DEBUG_VERBOSE("upg_ctrl_base:0x%04x, upg_flash_base:0x%02x.\n",
            firmware_upg->upg_ctrl_base, firmware_upg->upg_flash_base);
    }

    PCIE_DEV_DEBUG_VERBOSE("name:%s, domain:0x%04x, bus:0x%02x, slot:0x%02x, fn:%u, bar:%u, bus_width:%d, search_mode:%d \n",
        wb_pci_dev->name, wb_pci_dev->domain, wb_pci_dev->bus, wb_pci_dev->slot, wb_pci_dev->fn,
        wb_pci_dev->bar, wb_pci_dev->bus_width, wb_pci_dev->search_mode);

    if (wb_pci_dev->search_mode != SEARCH_DEV_DEFAULT && wb_pci_dev->search_mode != SEARCH_DEV_BY_BRIDGE) {
        dev_err(&pdev->dev, "Error: unsupported search_mode (%d).\n", wb_pci_dev->search_mode);
        return -EINVAL;
    }

    if (wb_pci_dev->search_mode == SEARCH_DEV_DEFAULT) {
        wb_pci_dev->bus = pci_dev_device->pci_bus;
        devfn = PCI_DEVFN(wb_pci_dev->slot, wb_pci_dev->fn);
        pci_dev = pci_get_domain_bus_and_slot(wb_pci_dev->domain, wb_pci_dev->bus, devfn);
        if (pci_dev == NULL) {
            dev_err(&pdev->dev, "Failed to find pci_dev, domain:0x%04x, bus:0x%02x, devfn:0x%x\n",
                wb_pci_dev->domain, wb_pci_dev->bus, devfn);
            return -ENXIO;
        }
    } else { /* search_mode = SEARCH_DEV_BY_BRIDGE */
        wb_pci_dev->bridge_bus = pci_dev_device->bridge_bus;
        wb_pci_dev->bridge_slot = pci_dev_device->bridge_slot;
        wb_pci_dev->bridge_fn = pci_dev_device->bridge_fn;
        devfn = PCI_DEVFN(wb_pci_dev->bridge_slot, wb_pci_dev->bridge_fn);
        pci_bridge_dev = pci_get_domain_bus_and_slot(wb_pci_dev->domain, wb_pci_dev->bridge_bus, devfn);
        if (pci_bridge_dev == NULL) {
            dev_err(&pdev->dev, "Failed to find pci_bridge_dev, domain:0x%04x, bus:0x%02x, devfn:0x%x\n",
                wb_pci_dev->domain, wb_pci_dev->bridge_bus, devfn);
            return -ENXIO;
        }

        ret = pci_read_config_byte(pci_bridge_dev, SECBUS, &secbus_val);
        if (ret) {
            PCIE_DEV_DEBUG_ERROR("pci_read_config_dword failed reg:%02x ret %d.\n", SECBUS, ret);
            return -EIO;
        }
        ret = pci_read_config_byte(pci_bridge_dev, SUBBUS, &subbus_val);
        if (ret) {
            PCIE_DEV_DEBUG_ERROR("pci_read_config_dword failed reg:%02x ret %d.\n", SUBBUS, ret);
            return -EIO;
        }
        if (secbus_val != subbus_val) {
            /* If the SECBUS register value is different from the SUBBUS register value, a multistage PCIE bridge is available*/
            PCIE_DEV_DEBUG_ERROR("not support ,secbus_val not equal subbus_val  secbus_val:%02x secbus_val:%02x.\n", secbus_val, subbus_val);
            return -EIO;
        }
        wb_pci_dev->bus = secbus_val;
        devfn = PCI_DEVFN(wb_pci_dev->slot, wb_pci_dev->fn);
        pci_dev = pci_get_domain_bus_and_slot(wb_pci_dev->domain, wb_pci_dev->bus, devfn);
        if (pci_dev == NULL) {
            dev_err(&pdev->dev, "Failed to find pci_dev, domain:0x%04x, bus:0x%02x, devfn:0x%x\n",
                wb_pci_dev->domain, wb_pci_dev->bus, devfn);
            return -ENXIO;
        }
    }

    if (wb_pci_dev->check_pci_id == 1) {
        wb_pci_dev->pci_id = pci_dev_device->pci_id;
        pci_id = (pci_dev->vendor << 16) | pci_dev->device;
        if (wb_pci_dev->pci_id != pci_id) {
            dev_err(&pdev->dev, "Failed to check pci id, except: 0x%x, really: 0x%x\n",
                wb_pci_dev->pci_id, pci_id);
            return -ENXIO;
        }
        PCIE_DEV_DEBUG_VERBOSE("pci id check ok, pci_id: 0x%x", pci_id);
    }
    ret = pci_setup_bars(wb_pci_dev, pci_dev);
    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to get pci bar address.\n");
        return ret;
    }

    if (!wb_pci_dev->setreg || !wb_pci_dev->getreg) {
        switch (wb_pci_dev->bus_width) {
        case 1:
            wb_pci_dev->setreg = pci_dev_setreg_8;
            wb_pci_dev->getreg = pci_dev_getreg_8;
            break;

        case 2:
            wb_pci_dev->setreg = pci_dev_setreg_16;
            wb_pci_dev->getreg = pci_dev_getreg_16;
            break;

        case 4:
            wb_pci_dev->setreg = pci_dev_setreg_32;
            wb_pci_dev->getreg = pci_dev_getreg_32;
            break;
        default:
            dev_err(&pdev->dev, "Error: unsupported I/O width (%d).\n", wb_pci_dev->bus_width);
            ret = -EINVAL;
            goto io_unmap;
        }
    }

    misc = &wb_pci_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = wb_pci_dev->name;
    misc->fops = &pcie_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&pdev->dev, "Failed to register %s device.\n", misc->name);
        ret = -ENXIO;
        goto io_unmap;
    }
    if (misc->minor >= MAX_PCIE_NUM) {
        dev_err(&pdev->dev, "Error: device minor[%d] more than max pcie num[%d].\n",
            misc->minor, MAX_PCIE_NUM);
        misc_deregister(misc);
        ret = -EINVAL;
        goto io_unmap;
    }
    pcie_dev_arry[misc->minor] = wb_pci_dev;
    dev_info(&pdev->dev, "%04x:%02x:%02x.%d[bar%d: %s]: register %s device with minor:%d success.\n",
        wb_pci_dev->domain, wb_pci_dev->bus, wb_pci_dev->slot, wb_pci_dev->fn, wb_pci_dev->bar,
        wb_pci_dev->bar_flag == IORESOURCE_MEM ? "IORESOURCE_MEM" : "IORESOURCE_IO",
        misc->name, misc->minor );
    return 0;

io_unmap:
    if (wb_pci_dev->pci_mem_base) {
        iounmap(wb_pci_dev->pci_mem_base);
    }
    return ret;
}

static int pci_dev_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < MAX_PCIE_NUM ; i++) {
        if (pcie_dev_arry[i] != NULL) {
            if (pcie_dev_arry[i]->pci_mem_base) {
                iounmap(pcie_dev_arry[i]->pci_mem_base);
            }
            misc_deregister(&pcie_dev_arry[i]->misc);
            pcie_dev_arry[i] = NULL;
        }
    }

    return 0;
}

static struct of_device_id pci_dev_match[] = {
    {
        .compatible = "wb-pci-dev",
    },
    {},
};
MODULE_DEVICE_TABLE(of, pci_dev_match);

static struct platform_driver wb_pci_dev_driver = {
    .probe      = pci_dev_probe,
    .remove     = pci_dev_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = PROXY_NAME,
        .of_match_table = pci_dev_match,
    },
};

static int __init wb_pci_dev_init(void)
{
    return platform_driver_register(&wb_pci_dev_driver);
}

static void __exit wb_pci_dev_exit(void)
{
    platform_driver_unregister(&wb_pci_dev_driver);
}

module_init(wb_pci_dev_init);
module_exit(wb_pci_dev_exit);
MODULE_DESCRIPTION("pcie device driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
