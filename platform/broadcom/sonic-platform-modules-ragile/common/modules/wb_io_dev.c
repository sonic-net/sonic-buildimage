/*
 * wb_io_dev.c
 * ko to read/write ioports through /dev/XXX device
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/export.h>
#include <linux/uio.h>

#include "wb_io_dev.h"

#define PROXY_NAME "wb-io-dev"
#define MAX_IO_DEV_NUM                     (256)
#define IO_RDWR_MAX_LEN                    (256)
#define MAX_NAME_SIZE                      (20)
#define IO_INDIRECT_ADDR_H(addr)           ((addr >> 8) & 0xff)
#define IO_INDIRECT_ADDR_L(addr)           ((addr) & 0xff)
#define IO_INDIRECT_OP_WRITE               (0x2)
#define IO_INDIRECT_OP_READ                (0X3)

static int g_io_dev_debug = 0;
static int g_io_dev_error = 0;

module_param(g_io_dev_debug, int, S_IRUGO | S_IWUSR);
module_param(g_io_dev_error, int, S_IRUGO | S_IWUSR);

#define IO_DEV_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_io_dev_debug) { \
        printk(KERN_INFO "[IO_DEV][VER][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define IO_DEV_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_io_dev_error) { \
        printk(KERN_ERR "[IO_DEV][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

typedef struct wb_io_dev_s {
    const char *name;
    uint32_t io_base;
    uint32_t io_len;
    uint32_t indirect_addr;
    uint32_t wr_data;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t rd_data;
    uint32_t opt_ctl;
    spinlock_t io_dev_lock;
    struct miscdevice misc;
} wb_io_dev_t;

static wb_io_dev_t* io_dev_arry[MAX_IO_DEV_NUM];

static int io_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    wb_io_dev_t *wb_io_dev;

    if (minor >= MAX_IO_DEV_NUM) {
        IO_DEV_DEBUG_ERROR("minor out of range, minor = %d.\n", minor);
        return -ENODEV;
    }

    wb_io_dev = io_dev_arry[minor];
    if (wb_io_dev == NULL) {
        IO_DEV_DEBUG_ERROR("wb_io_dev is NULL, open failed, minor = %d\n", minor);
        return -ENODEV;
    }

    file->private_data = wb_io_dev;
    return 0;
}

static int io_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}

uint8_t io_indirect_addressing_read(wb_io_dev_t *wb_io_dev, uint32_t address)
{
    uint8_t addr_l, addr_h, value;
    unsigned long flags;

    addr_h = IO_INDIRECT_ADDR_H(address);
    addr_l = IO_INDIRECT_ADDR_L(address);
    IO_DEV_DEBUG_VERBOSE("read one count, addr = 0x%x\n", address);

    spin_lock_irqsave(&wb_io_dev->io_dev_lock, flags);

    outb(addr_l, wb_io_dev->io_base + wb_io_dev->addr_low);

    outb(addr_h, wb_io_dev->io_base + wb_io_dev->addr_high);

    outb(IO_INDIRECT_OP_READ, wb_io_dev->io_base + wb_io_dev->opt_ctl);

    value = inb(wb_io_dev->io_base + wb_io_dev->rd_data);

    spin_unlock_irqrestore(&wb_io_dev->io_dev_lock, flags);

    return value;
}

static int io_dev_read_tmp(wb_io_dev_t *wb_io_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i;

    if (offset > wb_io_dev->io_len) {
        IO_DEV_DEBUG_VERBOSE("offset:0x%x, io len:0x%x, EOF.\n", offset, wb_io_dev->io_len);
        return 0;
    }

    if (count > wb_io_dev->io_len - offset) {
        IO_DEV_DEBUG_VERBOSE("read count out of range. input len:%lu, read len:%u.\n",
            count, wb_io_dev->io_len - offset);
        count = wb_io_dev->io_len - offset;
    }
    if (wb_io_dev->indirect_addr) {
        for (i = 0; i < count; i++) {
            buf[i] = io_indirect_addressing_read(wb_io_dev, offset + i);
        }
    } else {
        for (i = 0; i < count; i++) {
            buf[i] = inb(wb_io_dev->io_base + offset + i);
        }
    }

    return count;
}

static ssize_t io_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    wb_io_dev_t *wb_io_dev;
    int ret, read_len;
    u8 buf_tmp[IO_RDWR_MAX_LEN];

    wb_io_dev = file->private_data;
    if (wb_io_dev == NULL) {
        IO_DEV_DEBUG_ERROR("wb_io_dev is NULL, read failed.\n");
        return -EINVAL;
    }

    if (count == 0) {
        IO_DEV_DEBUG_ERROR("Invalid params, read count is 0.n");
        return -EINVAL;
    }

    if (count > sizeof(buf_tmp)) {
        IO_DEV_DEBUG_VERBOSE("read conut %lu exceed max %lu.\n", count, sizeof(buf_tmp));
        count = sizeof(buf_tmp);
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));
    read_len = io_dev_read_tmp(wb_io_dev, *offset, buf_tmp, count);
    if (read_len < 0) {
        IO_DEV_DEBUG_ERROR("io_dev_read_tmp failed, ret:%d.\n", read_len);
        return read_len;
    }

    if (access_ok(buf, read_len)) {
        IO_DEV_DEBUG_VERBOSE("user space read, buf: %p, offset: %lld, read conut %lu.\n",
            buf, *offset, count);
        if (copy_to_user(buf, buf_tmp, read_len)) {
            IO_DEV_DEBUG_ERROR("copy_to_user failed.\n");
            return -EFAULT;
        }
    } else {
        IO_DEV_DEBUG_VERBOSE("kernel space read, buf: %p, offset: %lld, read conut %lu.\n",
            buf, *offset, count);
        memcpy(buf, buf_tmp, read_len);
    }
    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t io_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    int ret;

    IO_DEV_DEBUG_VERBOSE("io_dev_read_iter, file: %p, count: %lu, offset: %lld\n",
        iocb->ki_filp, to->count, iocb->ki_pos);
    ret = io_dev_read(iocb->ki_filp, to->kvec->iov_base, to->count, &iocb->ki_pos);
    return ret;
}

void io_indirect_addressing_write(wb_io_dev_t *wb_io_dev, uint32_t address, uint8_t reg_val)
{
    uint8_t addr_l, addr_h;
    unsigned long flags;

    addr_h = IO_INDIRECT_ADDR_H(address);
    addr_l = IO_INDIRECT_ADDR_L(address);
    IO_DEV_DEBUG_VERBOSE("write one count, addr = 0x%x\n", address);

    spin_lock_irqsave(&wb_io_dev->io_dev_lock, flags);

    outb(reg_val, wb_io_dev->io_base + wb_io_dev->wr_data);

    outb(addr_l, wb_io_dev->io_base + wb_io_dev->addr_low);

    outb(addr_h, wb_io_dev->io_base + wb_io_dev->addr_high);

    outb(IO_INDIRECT_OP_WRITE, wb_io_dev->io_base + wb_io_dev->opt_ctl);

    spin_unlock_irqrestore(&wb_io_dev->io_dev_lock, flags);

    return;
}

static int io_dev_write_tmp(wb_io_dev_t *wb_io_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i;

    if (offset > wb_io_dev->io_len) {
        IO_DEV_DEBUG_VERBOSE("offset:0x%x, io len:0x%x, EOF.\n", offset, wb_io_dev->io_len);
        return 0;
    }

    if (count > wb_io_dev->io_len - offset) {
        IO_DEV_DEBUG_VERBOSE("write count out of range. input len:%lu, write len:%u.\n",
            count, wb_io_dev->io_len - offset);
        count = wb_io_dev->io_len - offset;
    }
    if (wb_io_dev->indirect_addr) {
        for (i = 0; i < count; i++) {
            io_indirect_addressing_write(wb_io_dev, offset + i, buf[i]);
        }
    } else {
        for (i = 0; i < count; i++) {
            outb(buf[i], wb_io_dev->io_base + offset + i);
        }
    }

    return count;
}

static ssize_t io_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    wb_io_dev_t *wb_io_dev;
    int write_len;
    u8 buf_tmp[IO_RDWR_MAX_LEN];

    wb_io_dev = file->private_data;
    if (wb_io_dev == NULL) {
        IO_DEV_DEBUG_ERROR("wb_io_dev is NULL, write failed.\n");
        return -EINVAL;
    }

    if (count == 0) {
        IO_DEV_DEBUG_ERROR("Invalid params, write count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(buf_tmp)) {
        IO_DEV_DEBUG_VERBOSE("write conut %lu exceed max %lu.\n", count, sizeof(buf_tmp));
        count = sizeof(buf_tmp);
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));
    if (access_ok(buf, count)) {
        IO_DEV_DEBUG_VERBOSE("user space write, buf: %p, offset: %lld, write conut %lu.\n",
            buf, *offset, count);
        if (copy_from_user(buf_tmp, buf, count)) {
            IO_DEV_DEBUG_ERROR("copy_from_user failed.\n");
            return -EFAULT;
        }
    } else {
        IO_DEV_DEBUG_VERBOSE("kernel space write, buf: %p, offset: %lld, write conut %lu.\n",
            buf, *offset, count);
        memcpy(buf_tmp, buf, count);
    }

    write_len = io_dev_write_tmp(wb_io_dev, *offset, buf_tmp, count);
    if (write_len < 0) {
        IO_DEV_DEBUG_ERROR("io_dev_write_tmp failed, ret:%d.\n", write_len);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t io_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    int ret;

    IO_DEV_DEBUG_VERBOSE("io_dev_write_iter, file: %p, count: %lu, offset: %lld\n",
        iocb->ki_filp, from->count, iocb->ki_pos);
    ret = io_dev_write(iocb->ki_filp, from->kvec->iov_base, from->count, &iocb->ki_pos);
    return ret;
}

static loff_t io_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    wb_io_dev_t *wb_io_dev;

    wb_io_dev = file->private_data;
    if (wb_io_dev == NULL) {
        IO_DEV_DEBUG_ERROR("wb_io_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            IO_DEV_DEBUG_ERROR("SEEK_SET, offset:%lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > wb_io_dev->io_len) {
            IO_DEV_DEBUG_ERROR("SEEK_SET out of range, offset:%lld, io_len:0x%x.\n",
                offset, wb_io_dev->io_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > wb_io_dev->io_len) || ((file->f_pos + offset) < 0)) {
            IO_DEV_DEBUG_ERROR("SEEK_CUR out of range, f_ops:%lld, offset:%lld, io_len:0x%x.\n",
                file->f_pos, offset, wb_io_dev->io_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        IO_DEV_DEBUG_ERROR("unsupport llseek type:%d.\n", origin);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static long io_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static const struct file_operations io_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = io_dev_llseek,
    .read_iter  = io_dev_read_iter,
    .write_iter = io_dev_write_iter,
    .unlocked_ioctl = io_dev_ioctl,
    .open       = io_dev_open,
    .release    = io_dev_release,
};

static wb_io_dev_t *dev_match(const char *path)
{
    wb_io_dev_t *wb_io_dev;
    char dev_name[MAX_NAME_SIZE];
    int i;

    for (i = 0; i < MAX_IO_DEV_NUM; i++) {
        if (io_dev_arry[i] == NULL) {
            continue;
        }
        wb_io_dev = io_dev_arry[i];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", wb_io_dev->name);
        if (!strcmp(path, dev_name)) {
            IO_DEV_DEBUG_VERBOSE("get dev_name = %s, minor = %d\n", dev_name, i);
            return wb_io_dev;
        }
    }

    return NULL;
}

int io_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    wb_io_dev_t *wb_io_dev;
    int read_len;

    if (path == NULL) {
        IO_DEV_DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        IO_DEV_DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    wb_io_dev = dev_match(path);
    if (wb_io_dev == NULL) {
        IO_DEV_DEBUG_ERROR("io_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    read_len = io_dev_read_tmp(wb_io_dev, offset, buf, count);
    if (read_len < 0) {
        IO_DEV_DEBUG_ERROR("io_dev_read_tmp failed, ret:%d.\n", read_len);
    }
    return read_len;
}
EXPORT_SYMBOL(io_device_func_read);

int io_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    wb_io_dev_t *wb_io_dev;
    int write_len;

    if (path == NULL) {
        IO_DEV_DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        IO_DEV_DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    wb_io_dev = dev_match(path);
    if (wb_io_dev == NULL) {
        IO_DEV_DEBUG_ERROR("i2c_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    write_len = io_dev_write_tmp(wb_io_dev, offset, buf, count);
    if (write_len < 0) {
        IO_DEV_DEBUG_ERROR("io_dev_write_tmp failed, ret:%d.\n", write_len);
    }
    return write_len;
}
EXPORT_SYMBOL(io_device_func_write);

static int io_dev_probe(struct platform_device *pdev)
{
    int ret;
    wb_io_dev_t *wb_io_dev;
    struct miscdevice *misc;
    io_dev_device_t *io_dev_device;

    wb_io_dev = devm_kzalloc(&pdev->dev, sizeof(wb_io_dev_t), GFP_KERNEL);
    if (!wb_io_dev) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        ret = -ENOMEM;
        return ret;
    }
    spin_lock_init(&wb_io_dev->io_dev_lock);

    if (pdev->dev.of_node) {
        ret = 0;
        ret += of_property_read_string(pdev->dev.of_node, "io_dev_name", &wb_io_dev->name);
        ret += of_property_read_u32(pdev->dev.of_node, "io_base", &wb_io_dev->io_base);
        ret += of_property_read_u32(pdev->dev.of_node, "io_len", &wb_io_dev->io_len);
        if (of_property_read_bool(pdev->dev.of_node, "indirect_addr")) {

            wb_io_dev->indirect_addr = 1;
            ret += of_property_read_u32(pdev->dev.of_node, "wr_data", &wb_io_dev->wr_data);
            ret += of_property_read_u32(pdev->dev.of_node, "addr_low", &wb_io_dev->addr_low);
            ret += of_property_read_u32(pdev->dev.of_node, "addr_high", &wb_io_dev->addr_high);
            ret += of_property_read_u32(pdev->dev.of_node, "rd_data", &wb_io_dev->rd_data);
            ret += of_property_read_u32(pdev->dev.of_node, "opt_ctl", &wb_io_dev->opt_ctl);
        } else {

            wb_io_dev->indirect_addr = 0;
        }
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get dts config, ret:%d.\n", ret);
            return -ENXIO;
        }
    } else {
        if (pdev->dev.platform_data == NULL) {
            dev_err(&pdev->dev, "Failed to get platform data config.\n");
            return -ENXIO;
        }
        io_dev_device = pdev->dev.platform_data;
        wb_io_dev->name = io_dev_device->io_dev_name;
        wb_io_dev->io_base = io_dev_device->io_base;
        wb_io_dev->io_len = io_dev_device->io_len;
        wb_io_dev->indirect_addr = io_dev_device->indirect_addr;
        if (wb_io_dev->indirect_addr == 1) {
            wb_io_dev->wr_data = io_dev_device->wr_data;
            wb_io_dev->addr_low = io_dev_device->addr_low;
            wb_io_dev->addr_high = io_dev_device->addr_high;
            wb_io_dev->rd_data = io_dev_device->rd_data;
            wb_io_dev->opt_ctl = io_dev_device->opt_ctl;
        }
    }

    IO_DEV_DEBUG_VERBOSE("name:%s, io base:0x%x, io len:0x%x, addressing type:%s.\n",
        wb_io_dev->name, wb_io_dev->io_base, wb_io_dev->io_len,
        wb_io_dev->indirect_addr ? "indirect" : "direct");

    misc = &wb_io_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = wb_io_dev->name;
    misc->fops = &io_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&pdev->dev, "Failed to register %s device.\n", misc->name);
        return -ENXIO;
    }
    if (misc->minor >= MAX_IO_DEV_NUM) {
        dev_err(&pdev->dev, "Error: device minor[%d] more than max io device num[%d].\n",
            misc->minor, MAX_IO_DEV_NUM);
        misc_deregister(misc);
        return -EINVAL;
    }
    io_dev_arry[misc->minor] = wb_io_dev;
    dev_info(&pdev->dev, "register %s device [0x%x][0x%x] with minor %d using %s addressing success.\n",
        misc->name, wb_io_dev->io_base, wb_io_dev->io_len, misc->minor,
        wb_io_dev->indirect_addr ? "indirect" : "direct");

    return 0;
}

static int io_dev_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < MAX_IO_DEV_NUM ; i++) {
        if (io_dev_arry[i] != NULL) {
            misc_deregister(&io_dev_arry[i]->misc);
            io_dev_arry[i] = NULL;
        }
    }

    return 0;
}

static struct of_device_id io_dev_match[] = {
    {
        .compatible = "wb-io-dev",
    },
    {},
};
MODULE_DEVICE_TABLE(of, io_dev_match);

static struct platform_driver wb_io_dev_driver = {
    .probe      = io_dev_probe,
    .remove     = io_dev_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = PROXY_NAME,
        .of_match_table = io_dev_match,
    },
};

static int __init wb_io_dev_init(void)
{
    return platform_driver_register(&wb_io_dev_driver);
}

static void __exit wb_io_dev_exit(void)
{
    platform_driver_unregister(&wb_io_dev_driver);
}

module_init(wb_io_dev_init);
module_exit(wb_io_dev_exit);
MODULE_DESCRIPTION("IO device driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
