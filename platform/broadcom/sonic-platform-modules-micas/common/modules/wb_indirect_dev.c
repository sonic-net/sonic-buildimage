/*
 * An wb_indirect_dev driver for indirect adapter device function
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

#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/slab.h>
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
#include <linux/preempt.h>
#include <linux/miscdevice.h>
#include <linux/uio.h>
#include <linux/kprobes.h>

#include "wb_indirect_dev.h"
#define MODULE_NAME "wb-indirect-dev"

#define SYMBOL_I2C_DEV_MODE       (1)
#define FILE_MODE                 (2)
#define SYMBOL_PCIE_DEV_MODE      (3)
#define SYMBOL_IO_DEV_MODE        (4)
#define SYMBOL_SPI_DEV_MODE       (5)

#define KERNEL_SPACE         (0)
#define USER_SPACE           (1)

#define MAX_INDIRECT_DEV_NUM      (256)
#define INDIRECT_ADDR_H(addr)           ((addr >> 8) & 0xff)
#define INDIRECT_ADDR_L(addr)           ((addr) & 0xff)
#define INDIRECT_OP_WRITE               (0x2)
#define INDIRECT_OP_READ                (0x3)

static int g_indirect_dev_debug = 0;
static int g_indirect_dev_error = 0;

module_param(g_indirect_dev_debug, int, S_IRUGO | S_IWUSR);
module_param(g_indirect_dev_error, int, S_IRUGO | S_IWUSR);

#define INDIRECT_DEV_INFO(fmt, args...) do {                                        \
    printk(KERN_INFO "[INDIRECT_DEV][INFO][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
} while (0)

#define INDIRECT_DEV_DEBUG(fmt, args...) do {                                        \
    if (g_indirect_dev_debug) { \
        printk(KERN_DEBUG "[INDIRECT_DEV][DEBUG][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define INDIRECT_DEV_ERROR(fmt, args...) do {                                        \
    if (g_indirect_dev_error) { \
        printk(KERN_ERR "[INDIRECT_DEV][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static struct indirect_dev_info* indirect_dev_arry[MAX_INDIRECT_DEV_NUM];

static int noop_pre(struct kprobe *p, struct pt_regs *regs) { return 0; }
static struct kprobe kp = {   
	.symbol_name = "kallsyms_lookup_name",  
};
unsigned long (*kallsyms_lookup_name_fun)(const char *name) = NULL;

/* Call kprobe to find the address location of kallsyms_lookup_name */
static int find_kallsyms_lookup_name(void)
{ 
    int ret = -1;

	kp.pre_handler = noop_pre;
	ret = register_kprobe(&kp);
    if (ret < 0) {  
	    INDIRECT_DEV_ERROR("register_kprobe failed, error:%d\n", ret); 
        return ret; 
	}
	INDIRECT_DEV_DEBUG("kallsyms_lookup_name addr: %p\n", kp.addr); 
	kallsyms_lookup_name_fun = (void*)kp.addr; 
	unregister_kprobe(&kp);
    
	return ret;
}

struct indirect_dev_info {
    const char *name;               /* generate dev name */
    const char *logic_dev_name;     /* dependent dev name */
    uint32_t indirect_len;          /* dev data len */
    uint32_t data_bus_width;        /* dev data_bus_width */
    uint32_t addr_bus_width;        /* dev addr_bus_width */
    uint32_t wr_data;               /* dependent dev wr date reg */
    uint32_t wr_data_width;         /* dependent dev wr_data_width */
    uint32_t addr_low;              /* dependent dev w/r addr reg low */
    uint32_t addr_high;             /* dependent dev w/r addr reg high */
    uint32_t rd_data;               /* dependent dev rd date reg */
    uint32_t rd_data_width;         /* dependent dev rd_data_width */
    uint32_t opt_ctl;               /* dependent dev opt code reg */
    uint32_t logic_func_mode;       /* 1: i2c, 2: file, 3:pcie, 4:io, 5:spi */
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
    spinlock_t indirect_dev_lock;
    struct miscdevice misc;
    struct device *dev;
};

static int wb_dev_file_read(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    struct kvec iov = {
        .iov_base = val,
        .iov_len = min_t(size_t, size, MAX_RW_COUNT),
    };
    struct iov_iter iter;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        INDIRECT_DEV_ERROR("read open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }
    tmp_pos = (loff_t)pos;
    iov_iter_kvec(&iter, ITER_DEST, &iov, 1, iov.iov_len);
    ret = vfs_iter_read(filp, &iter, &tmp_pos, 0);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("vfs_iter_read failed, path=%s, addr=0x%x, size=%zu, ret=%d\r\n", path, pos, size, ret);
        goto exit;
    }
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

static int wb_dev_file_write(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    struct kvec iov = {
        .iov_base = val,
        .iov_len = min_t(size_t, size, MAX_RW_COUNT),
    };
    struct iov_iter iter;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        INDIRECT_DEV_ERROR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    iov_iter_kvec(&iter, ITER_SOURCE, &iov, 1, iov.iov_len);
    ret = vfs_iter_write(filp, &iter, &tmp_pos, 0);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("vfs_iter_write failed, path=%s, addr=0x%x, size=%zu, ret=%d\r\n", path, pos, size, ret);
        goto exit;
    }
    
    vfs_fsync(filp, 1);
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

static int wb_logic_reg_write(struct indirect_dev_info *indirect_dev, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_write pfunc;

    pfunc = (device_func_write)indirect_dev->write_intf_addr;
    return pfunc(indirect_dev->logic_dev_name, pos, val, size);
}

static int wb_logic_reg_read(struct indirect_dev_info *indirect_dev, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    pfunc = (device_func_read)indirect_dev->read_intf_addr;
    return pfunc(indirect_dev->logic_dev_name, pos, val, size);
}


static int indirect_addressing_read(struct indirect_dev_info *indirect_dev, uint8_t *buf, uint32_t address, uint32_t rd_data_width)
{
    uint8_t addr_l, addr_h, op_code;
    unsigned long flags;
    int ret = 0;

    addr_h = INDIRECT_ADDR_H(address);
    addr_l = INDIRECT_ADDR_L(address);
    op_code = INDIRECT_OP_READ;

    spin_lock_irqsave(&indirect_dev->indirect_dev_lock, flags);

    ret = wb_logic_reg_write(indirect_dev, indirect_dev->addr_low, &addr_l, WIDTH_1Byte);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("indirect_read write reg error.offset = 0x%x, value = %u\n", indirect_dev->addr_low, addr_l);
        goto fail;
    }

    ret = wb_logic_reg_write(indirect_dev, indirect_dev->addr_high, &addr_h, WIDTH_1Byte);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("indirect_read write reg error.offset = 0x%x, value = %u\n", indirect_dev->addr_high, addr_h);
        goto fail;
    }

    ret = wb_logic_reg_write(indirect_dev, indirect_dev->opt_ctl, &op_code, WIDTH_1Byte);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("indirect_read write reg error.offset = 0x%x, value = %u\n", indirect_dev->opt_ctl, INDIRECT_OP_READ);
        goto fail;
    }

    ret = wb_logic_reg_read(indirect_dev, indirect_dev->rd_data, buf, rd_data_width);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("indirect_read read reg error.read offset = 0x%x\n, ret = %d", indirect_dev->rd_data, ret);
        goto fail;
    }

    INDIRECT_DEV_DEBUG("indirect_read success, addr = 0x%x\n", address);
    spin_unlock_irqrestore(&indirect_dev->indirect_dev_lock, flags);
    return ret;
fail:
    spin_unlock_irqrestore(&indirect_dev->indirect_dev_lock, flags);
    return ret;
}

static int device_read(struct indirect_dev_info *indirect_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, ret;
    u32 data_width;
    u32 tmp;

    data_width = indirect_dev->data_bus_width;

    if (offset % data_width) {
        INDIRECT_DEV_ERROR("data bus width:%d, offset:0x%x, read size %zu invalid.\n",
            data_width, offset, count);
        return -EINVAL;
    }

    if (count > indirect_dev->indirect_len - offset) {
        INDIRECT_DEV_DEBUG("read count out of range. input len:%zu, read len:%u.\n",
            count, indirect_dev->indirect_len  - offset);
        count = indirect_dev->indirect_len  - offset;
    }
    tmp = count;

    for (i = 0; i < count; i += data_width) {
        ret = indirect_addressing_read(indirect_dev, buf + i, offset + i, (tmp > data_width ? data_width : tmp));
        if (ret < 0) {
            INDIRECT_DEV_ERROR("read error.read offset = %u\n", (offset + i));
            return -EFAULT;
        }
        tmp -= data_width;
    }

    return count;
}

static int indirect_addressing_write(struct indirect_dev_info *indirect_dev, uint8_t *buf, uint32_t address, uint32_t wr_data_width)
{
    uint8_t addr_l, addr_h, op_code;
    unsigned long flags;
    int ret = 0;

    addr_h = INDIRECT_ADDR_H(address);
    addr_l = INDIRECT_ADDR_L(address);
    op_code = INDIRECT_OP_WRITE;

    spin_lock_irqsave(&indirect_dev->indirect_dev_lock, flags);

    ret = wb_logic_reg_write(indirect_dev, indirect_dev->wr_data, buf, wr_data_width);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("indirect_write read reg error.read offset = 0x%x\n, ret = %d", indirect_dev->wr_data, ret);
        goto fail;
    }

    ret = wb_logic_reg_write(indirect_dev, indirect_dev->addr_low, &addr_l, WIDTH_1Byte);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("indirect_write write reg error.offset = 0x%x, value = %u\n", indirect_dev->addr_low, addr_l);
        goto fail;
    }

    ret = wb_logic_reg_write(indirect_dev, indirect_dev->addr_high, &addr_h, WIDTH_1Byte);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("indirect_write write reg error.offset = 0x%x, value = %u\n", indirect_dev->addr_high, addr_h);
        goto fail;
    }

    ret = wb_logic_reg_write(indirect_dev, indirect_dev->opt_ctl, &op_code, WIDTH_1Byte);
    if (ret < 0) {
        INDIRECT_DEV_ERROR("indirect_write write reg error.offset = 0x%x, value = %u\n", indirect_dev->opt_ctl, INDIRECT_OP_READ);
        goto fail;
    }

    INDIRECT_DEV_DEBUG("indirect_write success, addr = 0x%x\n", address);
    spin_unlock_irqrestore(&indirect_dev->indirect_dev_lock, flags);
    return ret;
fail:
    spin_unlock_irqrestore(&indirect_dev->indirect_dev_lock, flags);
    return ret;
}

static int device_write(struct indirect_dev_info *indirect_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, ret;
    u32 data_width;
    u32 tmp;

    if (offset > indirect_dev->indirect_len) {
        INDIRECT_DEV_DEBUG("offset: 0x%x, spi len: 0x%x, count: %zu, EOF.\n",
            offset, indirect_dev->indirect_len, count);
        return 0;
    }

    data_width = indirect_dev->data_bus_width;
    if (offset % data_width) {
        INDIRECT_DEV_ERROR("data bus width:%d, offset:0x%x, read size %zu invalid.\n",
            data_width, offset, count);
        return -EINVAL;
    }

    if (count > (indirect_dev->indirect_len - offset)) {
        INDIRECT_DEV_DEBUG("write count out of range. input len:%zu, read len:%u.\n",
            count, indirect_dev->indirect_len - offset);
        count = indirect_dev->indirect_len - offset;
    }

    if (count == 0) {
        INDIRECT_DEV_DEBUG("offset: 0x%x, i2c len: 0x%x, read len: %zu, EOF.\n",
            offset, indirect_dev->indirect_len, count);
        return 0;
    }

    tmp = count;
    for (i = 0; i < count; i += data_width) {
        ret = indirect_addressing_write(indirect_dev, buf + i, offset + i, (tmp > data_width ? data_width : tmp));
        if (ret < 0) {
            INDIRECT_DEV_ERROR("write error.offset = %u\n", (offset + i));
            return -EFAULT;
        }
        tmp -= data_width;
    }
    return count;
}

static ssize_t indirect_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset, int flag)
{
    u8 val[MAX_RW_LEN];
    int ret, read_len;
    struct indirect_dev_info *indirect_dev;

    indirect_dev = file->private_data;
    if (indirect_dev == NULL) {
        INDIRECT_DEV_ERROR("can't get read private_data.\n");
        return -EINVAL;
    }

    if (count == 0) {
        INDIRECT_DEV_ERROR("Invalid params, read count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(val)) {
        INDIRECT_DEV_DEBUG("read count %zu exceed max %zu.\n", count, sizeof(val));
        count = sizeof(val);
    }

    mem_clear(val, sizeof(val));
    read_len = device_read(indirect_dev, (uint32_t)*offset, val, count);
    if (read_len < 0) {
        INDIRECT_DEV_ERROR("indirect dev read failed, dev name:%s, offset:0x%x, len:%zu.\n",
            indirect_dev->name, (uint32_t)*offset, count);
        return read_len;
    }

    /* check flag is user spase or kernel spase */
    if (flag == USER_SPACE) {
        INDIRECT_DEV_DEBUG("user space read, buf: %p, offset: %lld, read count %zu.\n",
            buf, *offset, count);
        if (copy_to_user(buf, val, read_len)) {
            INDIRECT_DEV_ERROR("copy_to_user failed.\n");
            return -EFAULT;
        }
    } else {
        INDIRECT_DEV_DEBUG("kernel space read, buf: %p, offset: %lld, read count %zu.\n",
            buf, *offset, count);
        memcpy(buf, val, read_len);
    }

    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t indirect_dev_read_user(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int ret;

    INDIRECT_DEV_DEBUG("indirect_dev_read_user, file: %p, count: %lu, offset: %lld\n",
        file, count, *offset);
    ret = indirect_dev_read(file, buf, count, offset, USER_SPACE);
    return ret;
}


static ssize_t indirect_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    int ret;

    INDIRECT_DEV_DEBUG("indirect_dev_read_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, to->count, iocb->ki_pos);
    ret = indirect_dev_read(iocb->ki_filp, to->kvec->iov_base, to->count, &iocb->ki_pos, KERNEL_SPACE);
    return ret;
}

static ssize_t indirect_dev_write(struct file *file, const char __user *buf,
                   size_t count, loff_t *offset, int flag)
{
    u8 val[MAX_RW_LEN];
    int write_len;
    struct indirect_dev_info *indirect_dev;

    indirect_dev = file->private_data;
    if (indirect_dev == NULL) {
        INDIRECT_DEV_ERROR("get write private_data error.\n");
        return -EINVAL;
    }

    if (count == 0) {
        INDIRECT_DEV_ERROR("Invalid params, write count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(val)) {
        INDIRECT_DEV_DEBUG("write count %zu exceed max %zu.\n", count, sizeof(val));
        count = sizeof(val);
    }

    mem_clear(val, sizeof(val));
    /* check flag is user spase or kernel spase */
    if (flag == USER_SPACE) {
        INDIRECT_DEV_DEBUG("user space write, buf: %p, offset: %lld, write count %zu.\n",
            buf, *offset, count);
        if (copy_from_user(val, buf, count)) {
            INDIRECT_DEV_ERROR("copy_from_user failed.\n");
            return -EFAULT;
        }
    } else {
        INDIRECT_DEV_DEBUG("kernel space write, buf: %p, offset: %lld, write count %zu.\n",
            buf, *offset, count);
        memcpy(val, buf, count);
    }

    write_len = device_write(indirect_dev, (uint32_t)*offset, val, count);
    if (write_len < 0) {
        INDIRECT_DEV_ERROR("indirect dev write failed, dev name:%s, offset:0x%llx, len:%zu.\n",
            indirect_dev->name, *offset, count);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t indirect_dev_write_user(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    int ret;

    INDIRECT_DEV_DEBUG("indirect_dev_write_user, file: %p, count: %lu, offset: %lld\n",
        file, count, *offset);
    ret = indirect_dev_write(file, buf, count, offset, USER_SPACE);
    return ret;
}

static ssize_t indirect_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    int ret;

    INDIRECT_DEV_DEBUG("indirect_dev_write_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, from->count, iocb->ki_pos);
    ret = indirect_dev_write(iocb->ki_filp, from->kvec->iov_base, from->count, &iocb->ki_pos, KERNEL_SPACE);
    return ret;
}

static loff_t indirect_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    struct indirect_dev_info *indirect_dev;

    indirect_dev = file->private_data;
    if (indirect_dev == NULL) {
        INDIRECT_DEV_ERROR("indirect_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            INDIRECT_DEV_ERROR("SEEK_SET, offset:%lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > indirect_dev->indirect_len) {
            INDIRECT_DEV_ERROR("SEEK_SET out of range, offset:%lld, i2c_len:0x%x.\n",
                offset, indirect_dev->indirect_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > indirect_dev->indirect_len) || ((file->f_pos + offset) < 0)) {
            INDIRECT_DEV_ERROR("SEEK_CUR out of range, f_ops:%lld, offset:%lld.\n",
                 file->f_pos, offset);
        }
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        INDIRECT_DEV_ERROR("unsupport llseek type:%d.\n", origin);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static long indirect_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int indirect_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    struct indirect_dev_info *indirect_dev;

    if (minor >= MAX_INDIRECT_DEV_NUM) {
        INDIRECT_DEV_ERROR("minor out of range, minor = %d.\n", minor);
        return -ENODEV;
    }

    indirect_dev = indirect_dev_arry[minor];
    if (indirect_dev == NULL) {
        INDIRECT_DEV_ERROR("indirect_dev is NULL, open failed, minor = %d\n", minor);
        return -ENODEV;
    }

    file->private_data = indirect_dev;

    return 0;
}

static int indirect_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;

    return 0;
}

static const struct file_operations indirect_dev_fops = {
    .owner          = THIS_MODULE,
    .llseek         = indirect_dev_llseek,
    .read           = indirect_dev_read_user,
    .write          = indirect_dev_write_user,
    .read_iter      = indirect_dev_read_iter,
    .write_iter     = indirect_dev_write_iter,
    .unlocked_ioctl = indirect_dev_ioctl,
    .open           = indirect_dev_open,
    .release        = indirect_dev_release,
};

static struct indirect_dev_info *dev_match(const char *path)
{
    struct indirect_dev_info *indirect_dev;
    char dev_name[DEV_NAME_LEN];
    int i;

    for (i = 0; i < MAX_INDIRECT_DEV_NUM; i++) {
        if (indirect_dev_arry[i] == NULL) {
            continue;
        }
        indirect_dev = indirect_dev_arry[i];
        snprintf(dev_name, DEV_NAME_LEN,"/dev/%s", indirect_dev->name);
        if (!strcmp(path, dev_name)) {
            INDIRECT_DEV_DEBUG("get dev_name = %s, minor = %d\n", dev_name, i);
            return indirect_dev;
        }
    }

    return NULL;
}

int indirect_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct indirect_dev_info *indirect_dev;
    int read_len;

    if (path == NULL) {
        INDIRECT_DEV_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        INDIRECT_DEV_ERROR("buf NULL");
        return -EINVAL;
    }

    indirect_dev = dev_match(path);
    if (indirect_dev == NULL) {
        INDIRECT_DEV_ERROR("indirect_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    read_len = device_read(indirect_dev, offset, buf, count);
    if (read_len < 0) {
        INDIRECT_DEV_ERROR("indirect_dev_read_tmp failed, ret:%d.\n", read_len);
    }
    return read_len;
}
EXPORT_SYMBOL(indirect_device_func_read);

int indirect_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct indirect_dev_info *indirect_dev;
    int write_len;

    if (path == NULL) {
        INDIRECT_DEV_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        INDIRECT_DEV_ERROR("buf NULL");
        return -EINVAL;
    }

    indirect_dev = dev_match(path);
    if (indirect_dev == NULL) {
        INDIRECT_DEV_ERROR("indirect_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    write_len = device_write(indirect_dev, offset, buf, count);
    if (write_len < 0) {
        INDIRECT_DEV_ERROR("indirect_dev_write_tmp failed, ret:%d.\n", write_len);
    }
    return write_len;
}
EXPORT_SYMBOL(indirect_device_func_write);


static int wb_indirect_dev_probe(struct platform_device *pdev)
{
    int ret;
    struct indirect_dev_info *indirect_dev;
    struct miscdevice *misc;
    indirect_dev_device_t *indirect_dev_device;

    INDIRECT_DEV_DEBUG("wb_indirect_dev_probe\n");

    indirect_dev = devm_kzalloc(&pdev->dev, sizeof(struct indirect_dev_info), GFP_KERNEL);
    if (!indirect_dev) {
        dev_err(&pdev->dev, "devm_kzalloc error.\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, indirect_dev);
    indirect_dev->dev = &pdev->dev;

    if (pdev->dev.of_node) {
        ret = 0;
        ret += of_property_read_string(pdev->dev.of_node, "dev_name", &indirect_dev->name);
        ret += of_property_read_string(pdev->dev.of_node, "logic_dev_name", &indirect_dev->logic_dev_name);
        ret += of_property_read_u32(pdev->dev.of_node, "addr_low", &indirect_dev->addr_low);
        ret += of_property_read_u32(pdev->dev.of_node, "data_bus_width", &indirect_dev->data_bus_width);
        ret += of_property_read_u32(pdev->dev.of_node, "addr_bus_width", &indirect_dev->addr_bus_width);
        ret += of_property_read_u32(pdev->dev.of_node, "addr_high", &indirect_dev->addr_high);
        ret += of_property_read_u32(pdev->dev.of_node, "wr_data", &indirect_dev->wr_data);
        ret += of_property_read_u32(pdev->dev.of_node, "rd_data", &indirect_dev->rd_data);
        ret += of_property_read_u32(pdev->dev.of_node, "opt_ctl", &indirect_dev->opt_ctl);
        ret += of_property_read_u32(pdev->dev.of_node, "indirect_len", &indirect_dev->indirect_len);
        ret += of_property_read_u32(pdev->dev.of_node, "logic_func_mode", &indirect_dev->logic_func_mode);

        if (of_property_read_u32(pdev->dev.of_node, "wr_data_width", &indirect_dev->wr_data_width)) {
            /* dts have no wr_data_width,set default 1 */
            indirect_dev->wr_data_width = WIDTH_1Byte;
        }
        if (of_property_read_u32(pdev->dev.of_node, "rd_data_width", &indirect_dev->rd_data_width)) {
            /* dts have no rd_data_width,set default 1 */
            indirect_dev->rd_data_width = WIDTH_1Byte;
        }
        if (ret != 0) {
            dev_err(&pdev->dev, "dts config error.ret:%d.\n", ret);
            return -ENXIO;
        }
    } else {
        if (pdev->dev.platform_data == NULL) {
            dev_err(&pdev->dev, "Failed to get platform data config.\n");
            return -ENXIO;
        }
        indirect_dev_device = pdev->dev.platform_data;
        indirect_dev->name = indirect_dev_device->dev_name;
        indirect_dev->logic_dev_name = indirect_dev_device->logic_dev_name;
        indirect_dev->data_bus_width = indirect_dev_device->data_bus_width;
        indirect_dev->addr_bus_width = indirect_dev_device->addr_bus_width;
        indirect_dev->wr_data = indirect_dev_device->wr_data;
        indirect_dev->wr_data_width = indirect_dev_device->wr_data_width;
        indirect_dev->addr_low = indirect_dev_device->addr_low;
        indirect_dev->addr_high = indirect_dev_device->addr_high;
        indirect_dev->rd_data = indirect_dev_device->rd_data;
        indirect_dev->rd_data_width = indirect_dev_device->rd_data_width;
        indirect_dev->opt_ctl = indirect_dev_device->opt_ctl;
        indirect_dev->indirect_len = indirect_dev_device->indirect_len;
        indirect_dev->logic_func_mode = indirect_dev_device->logic_func_mode;
    }

    switch (indirect_dev->logic_func_mode) {
    case SYMBOL_I2C_DEV_MODE:
        indirect_dev->write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("i2c_device_func_write");
        indirect_dev->read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("i2c_device_func_read");
        break;
    case SYMBOL_SPI_DEV_MODE:
        indirect_dev->write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("spi_device_func_write");
        indirect_dev->read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("spi_device_func_read");
        break;
    case SYMBOL_IO_DEV_MODE:
        indirect_dev->write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("io_device_func_write");
        indirect_dev->read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("io_device_func_read");
        break;
    case SYMBOL_PCIE_DEV_MODE:
        indirect_dev->write_intf_addr = (unsigned long)kallsyms_lookup_name_fun("pcie_device_func_write");
        indirect_dev->read_intf_addr = (unsigned long)kallsyms_lookup_name_fun("pcie_device_func_read");
        break;
    case FILE_MODE:
        indirect_dev->write_intf_addr = (unsigned long)wb_dev_file_write;
        indirect_dev->read_intf_addr = (unsigned long)wb_dev_file_read;
        break;
    default:
        dev_err(&pdev->dev, "func mode %d don't support.\n", indirect_dev->logic_func_mode);
        return -EINVAL;
    }

    if (!indirect_dev->write_intf_addr || !indirect_dev->read_intf_addr) {
        dev_err(&pdev->dev, "Fail: func mode %u rw symbol undefined.\n", indirect_dev->logic_func_mode);
        return -ENOSYS;
    }

    /* TODO: data_bus_width unuse now, need judge in rd or wr */
    dev_info(&pdev->dev, "register indirect device %s success. logic_dev_name: %s, indirect_len: 0x%x, data_bus_width: 0x%x, dependent dev: 0x%x, wr_data: 0x%x, wr_data_width: %d, \
        rd_data: 0x%x, rd_data_width: %d, addr_low: 0x%x, addr_high: 0x%x, opt_ctl: 0x%x, logic_func_mode: %d\n", indirect_dev->name, indirect_dev->logic_dev_name, indirect_dev->indirect_len, indirect_dev->data_bus_width, \
        indirect_dev->addr_bus_width, indirect_dev->wr_data, indirect_dev->wr_data_width, indirect_dev->rd_data, indirect_dev->rd_data_width, indirect_dev->addr_low, indirect_dev->addr_high,\
        indirect_dev->opt_ctl, indirect_dev->logic_func_mode);

    misc = &indirect_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = indirect_dev->name;
    misc->fops = &indirect_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&pdev->dev, "register %s faild.\n", misc->name);
        return -ENXIO;
    }

    if (misc->minor >= MAX_INDIRECT_DEV_NUM) {
        dev_err(&pdev->dev, "minor number beyond the limit! is %d.\n", misc->minor);
        misc_deregister(misc);
        return -ENXIO;
    }
    spin_lock_init(&indirect_dev->indirect_dev_lock);
    indirect_dev_arry[misc->minor] = indirect_dev;

    dev_info(&pdev->dev, "register indirect device %s success.\n", indirect_dev->name);
    return 0;
}

static int wb_indirect_dev_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < MAX_INDIRECT_DEV_NUM; i++) {
        if (indirect_dev_arry[i] != NULL) {
            if (indirect_dev_arry[i]->dev == &pdev->dev) {
                misc_deregister(&indirect_dev_arry[i]->misc);
                indirect_dev_arry[i] = NULL;
                return 0;
            }
        }
    }
    return 0;
}

static const struct of_device_id wb_indirect_dev_driver_of_match[] = {
    { .compatible = "wb-indirect-dev" },
    { },
};

static struct platform_driver wb_indirect_dev_driver = {
    .probe      = wb_indirect_dev_probe,
    .remove     = wb_indirect_dev_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = MODULE_NAME,
        .of_match_table = wb_indirect_dev_driver_of_match,
    },
};

static int __init wb_indirect_dev_init(void)
{
    int ret;

    ret = find_kallsyms_lookup_name();
    if (ret < 0) {
        INDIRECT_DEV_ERROR("find kallsyms_lookup_name failed\n");
        return -ENXIO;
    }
    INDIRECT_DEV_DEBUG("find kallsyms_lookup_name ok\n");
    return platform_driver_register(&wb_indirect_dev_driver);
}

static void __exit wb_indirect_dev_exit(void)
{
    platform_driver_unregister(&wb_indirect_dev_driver);
}

module_init(wb_indirect_dev_init);
module_exit(wb_indirect_dev_exit);
MODULE_DESCRIPTION("indirect device driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
