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
#include <wb_bsp_kernel_debug.h>
#include <wb_kernel_io.h>

#define PROXY_NAME "wb-io-dev"
#define IO_INDIRECT_ADDR_H(addr)           ((addr >> 8) & 0xff)
#define IO_INDIRECT_ADDR_L(addr)           ((addr) & 0xff)
#define IO_INDIRECT_OP_WRITE               (0x2)
#define IO_INDIRECT_OP_READ                (0X3)

/* Use the wb_bsp_kernel_debug header file must define debug variable */
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int status_cache_ms = 0;
module_param(status_cache_ms, int, S_IRUGO | S_IWUSR);

typedef struct wb_io_dev_s {
    const char *name;
    const char *alias;
    void __iomem * vir_base;
    uint8_t io_type;                                    /* 0:ioport 1:iomem*/
    uint32_t io_base;
    uint32_t io_len;
    uint32_t indirect_addr;
    uint32_t wr_data;
    uint32_t wr_data_width;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t rd_data;
    uint32_t rd_data_width;
    uint32_t opt_ctl;
    spinlock_t io_dev_lock;
    struct miscdevice misc;
    struct kobject kobj;
    struct attribute_group *sysfs_group;
    uint8_t file_cache_rd;
    uint8_t file_cache_wr;
    char cache_file_path[MAX_NAME_SIZE];
    char mask_file_path[MAX_NAME_SIZE];
    uint32_t status_check_type;          /* 0: Not supported 1: Readback verification, 2: Readback anti-verification */
    uint32_t test_reg_num;               /* The number of test registers is 8 at most */
    uint32_t test_reg[TEST_REG_MAX_NUM]; /* Test register address */
    uint8_t test_data[WIDTH_4Byte];      /* Data written into the test register, supports up to 4 bytes */
    unsigned long last_jiffies;          /* The number of jiffies when the device status was last obtained */
    bool dev_status;                     /* Device status flag */
    struct mutex update_lock;
    wb_bsp_key_device_log_node_t log_node;
} wb_io_dev_t;

static wb_io_dev_t* io_dev_arry[MAX_DEV_NUM];

static uint8_t io_dev_readb(wb_io_dev_t *wb_io_dev, uint32_t offset)
{
    uint8_t value;

    switch (wb_io_dev->io_type) {
    case IO_DEV_USE_IOPORT:
        value = inb(wb_io_dev->io_base + offset);
        break;
    case IO_DEV_USE_IOMEM:
        value = readb(wb_io_dev->vir_base + offset);
        break;
    default:
        value = 0;
        DEBUG_ERROR("io type %d don't support.\n", wb_io_dev->io_type);
        break;
    }

    return value;
}

static void io_dev_writeb(wb_io_dev_t *wb_io_dev, uint32_t offset, uint8_t value)
{
    switch (wb_io_dev->io_type) {
    case IO_DEV_USE_IOPORT:
        outb(value, wb_io_dev->io_base + offset);
        break;
    case IO_DEV_USE_IOMEM:
        writeb(value, wb_io_dev->vir_base + offset);
        break;
    default:
        DEBUG_ERROR("io type %d don't support.\n", wb_io_dev->io_type);
        break;
    }

    return;
}

static u32 io_dev_data_read(wb_io_dev_t *wb_io_dev, uint32_t offset)
{
    u32 value;

    switch (IO_DATA_MODE(wb_io_dev->io_type, wb_io_dev->rd_data_width)) {
    case IO_DATA_MODE(IO_DEV_USE_IOPORT, WIDTH_2Byte):
        value = inw(wb_io_dev->io_base + offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOPORT, WIDTH_4Byte):
        value = inl(wb_io_dev->io_base + offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOPORT, WIDTH_1Byte):
        value = inb(wb_io_dev->io_base + offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOMEM, WIDTH_2Byte):
        value = readw(wb_io_dev->vir_base + offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOMEM, WIDTH_4Byte):
        value = readl(wb_io_dev->vir_base + offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOMEM, WIDTH_1Byte):
        value = readb(wb_io_dev->vir_base + offset);
        break;
    default:
        /* default 1 byte mode */
        value = io_dev_readb(wb_io_dev, offset);
        break;
    }

    return value;
}

static void io_dev_data_write(wb_io_dev_t *wb_io_dev, uint32_t offset, uint32_t value)
{
    switch (IO_DATA_MODE(wb_io_dev->io_type, wb_io_dev->wr_data_width)) {
    case IO_DATA_MODE(IO_DEV_USE_IOPORT, WIDTH_2Byte):
        outw(value & 0xFFFF, wb_io_dev->io_base + offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOPORT, WIDTH_4Byte):
        outl(value, wb_io_dev->io_base + offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOPORT, WIDTH_1Byte):
        outb(value & 0xFF, wb_io_dev->io_base + offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOMEM, WIDTH_2Byte):
        writew(value & 0xFFFF, wb_io_dev->vir_base + offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOMEM, WIDTH_4Byte):
        writel(value, wb_io_dev->vir_base +offset);
        break;
    case IO_DATA_MODE(IO_DEV_USE_IOMEM, WIDTH_1Byte):
        writeb(value & 0xFF, wb_io_dev->vir_base + offset);
        break;
    default:
        /* default 1 byte mode */
        io_dev_writeb(wb_io_dev, offset, value & 0xFF);
        break;
    }

    return;
}

static int io_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    wb_io_dev_t *wb_io_dev;

    if (minor >= MAX_DEV_NUM) {
        DEBUG_ERROR("minor out of range, minor = %d.\n", minor);
        return -ENODEV;
    }

    wb_io_dev = io_dev_arry[minor];
    if (wb_io_dev == NULL) {
        DEBUG_ERROR("wb_io_dev is NULL, open failed, minor = %d\n", minor);
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

u32 io_indirect_addressing_read(wb_io_dev_t *wb_io_dev, uint32_t address)
{
    uint8_t addr_l, addr_h;
    unsigned long flags;
    u32 value;

    addr_h = IO_INDIRECT_ADDR_H(address);
    addr_l = IO_INDIRECT_ADDR_L(address);

    spin_lock_irqsave(&wb_io_dev->io_dev_lock, flags);

    io_dev_writeb(wb_io_dev, wb_io_dev->addr_low, addr_l);

    io_dev_writeb(wb_io_dev, wb_io_dev->addr_high, addr_h);

    io_dev_writeb(wb_io_dev, wb_io_dev->opt_ctl, IO_INDIRECT_OP_READ);

    value = io_dev_data_read(wb_io_dev, wb_io_dev->rd_data);

    spin_unlock_irqrestore(&wb_io_dev->io_dev_lock, flags);

    DEBUG_VERBOSE("read one count, addr = 0x%x, value = 0x%x\n", address, value);

    return value;
}

static int io_dev_read_tmp(wb_io_dev_t *wb_io_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int width, i, j, ret;
    u32 val;

    if (offset >= wb_io_dev->io_len) {
        DEBUG_VERBOSE("offset: 0x%x, io len:0x%x, EOF.\n", offset, wb_io_dev->io_len);
        return 0;
    }

    if (count > wb_io_dev->io_len - offset) {
        DEBUG_VERBOSE("read count out of range. input len:%lu, read len:%u.\n",
            count, wb_io_dev->io_len - offset);
        count = wb_io_dev->io_len - offset;
    }

    if (wb_io_dev->indirect_addr) {
        width = wb_io_dev->rd_data_width;
        if (offset % width) {
            DEBUG_VERBOSE("rd_data_width: %d, offset: 0x%x, size %lu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }

        for (i = 0; i < count; i += width) {
            val = io_indirect_addressing_read(wb_io_dev, offset + i);
            for (j = 0; (j < width) && (i + j < count); j++) {
                buf[i + j] = (val >> (8 * j)) & 0xff;
            }
        }
    } else {
        for (i = 0; i < count; i++) {
            buf[i] = io_dev_readb(wb_io_dev, offset + i);
        }
        if (wb_io_dev->file_cache_rd) {
            ret = cache_value_read(wb_io_dev->mask_file_path, wb_io_dev->cache_file_path, offset, buf, count);
            if (ret < 0) {
                DEBUG_ERROR("io data offset: 0x%x, read_len: %lu, read cache file fail, ret: %d, return act value\n",
                    offset, count, ret);
            } else {
                DEBUG_VERBOSE("io data offset: 0x%x, read_len: %lu success, read from cache value\n",
                    offset, count);
            }
        }
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(wb_io_dev->name, offset, buf, count, true);
    }

    return count;
}

static ssize_t io_dev_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
    wb_io_dev_t *wb_io_dev;
    int ret, read_len;
    u8 buf_tmp[MAX_RW_LEN];

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, read failed.\n");
        return -EINVAL;
    }

    wb_io_dev = file->private_data;
    if (wb_io_dev == NULL) {
        DEBUG_ERROR("wb_io_dev is NULL, read failed.\n");
        return -EINVAL;
    }

    if (count == 0) {
        DEBUG_ERROR("Invalid params, read count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(buf_tmp)) {
        DEBUG_VERBOSE("read count %lu exceed max %lu.\n", count, sizeof(buf_tmp));
        count = sizeof(buf_tmp);
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));
    read_len = io_dev_read_tmp(wb_io_dev, *offset, buf_tmp, count);
    if (read_len < 0) {
        DEBUG_ERROR("io_dev_read_tmp failed, offset: 0x%llx, count: %lu, ret: %d\n",
            *offset, count, read_len);
        return read_len;
    }

    if (read_len == 0) {
        DEBUG_VERBOSE("io_dev_read_tmp EOF, offset: 0x%llx, count: %lu\n", *offset, count);
        return 0;
    }

    DEBUG_VERBOSE("read, buf: %p, offset: %llx, read count %lu.\n", buf, *offset, count);
    memcpy(buf, buf_tmp, read_len);

    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t io_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    DEBUG_VERBOSE("io_dev_read_iter, file: %p, count: %lu, offset: 0x%llx\n",
        iocb->ki_filp, iov_iter_count(to), iocb->ki_pos);
    return wb_iov_iter_read(iocb, to, io_dev_read);
}

void io_indirect_addressing_write(wb_io_dev_t *wb_io_dev, uint32_t address, u32 reg_val)
{
    uint8_t addr_l, addr_h;
    unsigned long flags;

    addr_h = IO_INDIRECT_ADDR_H(address);
    addr_l = IO_INDIRECT_ADDR_L(address);
    DEBUG_VERBOSE("write one count, addr = 0x%x, val = 0x%x\n", address, reg_val);

    spin_lock_irqsave(&wb_io_dev->io_dev_lock, flags);

    io_dev_data_write(wb_io_dev, wb_io_dev->wr_data, reg_val);

    io_dev_writeb(wb_io_dev, wb_io_dev->addr_low, addr_l);

    io_dev_writeb(wb_io_dev, wb_io_dev->addr_high, addr_h);

    io_dev_writeb(wb_io_dev, wb_io_dev->opt_ctl, IO_INDIRECT_OP_WRITE);

    spin_unlock_irqrestore(&wb_io_dev->io_dev_lock, flags);

    return;
}

static int io_dev_write_tmp(wb_io_dev_t *wb_io_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int width, i, j;
    u32 val;

    if (offset >= wb_io_dev->io_len) {
        DEBUG_VERBOSE("offset: 0x%x, io len: 0x%x, EOF.\n", offset, wb_io_dev->io_len);
        return 0;
    }

    if (count > wb_io_dev->io_len - offset) {
        DEBUG_VERBOSE("write count out of range. input len: %lu, write len: %u.\n",
            count, wb_io_dev->io_len - offset);
        count = wb_io_dev->io_len - offset;
    }

    if (wb_io_dev->indirect_addr) {
        width = wb_io_dev->wr_data_width;
        if (offset % width) {
            DEBUG_VERBOSE("wr_data_width:%d, offset:0x%x, size %lu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }
        for (i = 0; i < count; i += width) {
            val = 0;
            for (j = 0; (j < width) && (i + j < count); j++) {
                val |= buf[i + j] << (8 * j);
            }
            io_indirect_addressing_write(wb_io_dev, i + offset, val);
        }
    } else {
        for (i = 0; i < count; i++) {
            io_dev_writeb(wb_io_dev, offset + i, buf[i]);
        }
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(wb_io_dev->name, offset, buf, count, false);
    }

    return count;
}

static ssize_t io_dev_write(struct file *file, char *buf, size_t count, loff_t *offset)
{
    wb_io_dev_t *wb_io_dev;
    int write_len;
    u8 buf_tmp[MAX_RW_LEN];
    char bsp_log_dev_name[BSP_LOG_DEV_NAME_MAX_LEN];
    char bsp_log_file_path[BSP_LOG_DEV_NAME_MAX_LEN];

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, write failed.\n");
        return -EINVAL;
    }

    wb_io_dev = file->private_data;
    if (wb_io_dev == NULL) {
        DEBUG_ERROR("wb_io_dev is NULL, write failed.\n");
        return -EINVAL;
    }

    if (count == 0) {
        DEBUG_ERROR("Invalid params, write count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(buf_tmp)) {
        DEBUG_VERBOSE("write count %lu exceed max %lu.\n", count, sizeof(buf_tmp));
        count = sizeof(buf_tmp);
    }

    mem_clear(buf_tmp, sizeof(buf_tmp));

    DEBUG_VERBOSE("write, buf: %p, offset: 0x%llx, write count %lu.\n", buf, *offset, count);
    memcpy(buf_tmp, buf, count);

    if (wb_io_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Devfs]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_file_path), "%s.%s_bsp_key_reg", BSP_LOG_DIR, wb_io_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(wb_io_dev->log_node), (uint32_t)*offset, buf_tmp, count);
    }

    write_len = io_dev_write_tmp(wb_io_dev, *offset, buf_tmp, count);
    if (write_len < 0) {
        DEBUG_ERROR("io_dev_write_tmp failed, offset: 0x%llx, count: %lu, ret: %d.\n",
            *offset, count, write_len);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t io_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    DEBUG_VERBOSE("io_dev_write_iter, file: %p, count: %lu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(from), iocb->ki_pos);
    return wb_iov_iter_write(iocb, from, io_dev_write);
}

static loff_t io_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    wb_io_dev_t *wb_io_dev;

    wb_io_dev = file->private_data;
    if (wb_io_dev == NULL) {
        DEBUG_ERROR("wb_io_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            DEBUG_ERROR("SEEK_SET, offset: %lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > wb_io_dev->io_len) {
            DEBUG_ERROR("SEEK_SET out of range, offset: %lld, io_len: 0x%x.\n",
                offset, wb_io_dev->io_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > wb_io_dev->io_len) || ((file->f_pos + offset) < 0)) {
            DEBUG_ERROR("SEEK_CUR out of range, f_ops: %lld, offset: %lld, io_len: 0x%x.\n",
                file->f_pos, offset, wb_io_dev->io_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        DEBUG_ERROR("unsupport llseek type:%d.\n", origin);
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

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (io_dev_arry[i] == NULL) {
            continue;
        }
        wb_io_dev = io_dev_arry[i];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", wb_io_dev->name);
        if (!strcmp(path, dev_name)) {
            DEBUG_VERBOSE("get dev_name = %s, minor = %d\n", dev_name, i);
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
        DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    wb_io_dev = dev_match(path);
    if (wb_io_dev == NULL) {
        DEBUG_ERROR("io_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    read_len = io_dev_read_tmp(wb_io_dev, offset, buf, count);
    if (read_len < 0) {
        DEBUG_ERROR("io_dev_read_tmp failed, offset: 0x%x, count: %lu, ret: %d\n",
            offset, count, read_len);
    }

    return read_len;
}
EXPORT_SYMBOL(io_device_func_read);

int io_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    wb_io_dev_t *wb_io_dev;
    int write_len;
    char bsp_log_dev_name[BSP_LOG_DEV_NAME_MAX_LEN];
    char bsp_log_file_path[BSP_LOG_DEV_NAME_MAX_LEN];

    if (path == NULL) {
        DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    wb_io_dev = dev_match(path);
    if (wb_io_dev == NULL) {
        DEBUG_ERROR("io_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    if (wb_io_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Symbol]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_file_path), "%s.%s_bsp_key_reg", BSP_LOG_DIR, wb_io_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(wb_io_dev->log_node), offset, buf, count);
    }

    write_len = io_dev_write_tmp(wb_io_dev, offset, buf, count);
    if (write_len < 0) {
        DEBUG_ERROR("io_dev_write_tmp failed, offset: 0x%x, count: %lu, ret: %d.\n",
            offset, count, write_len);
    }
    return write_len;
}
EXPORT_SYMBOL(io_device_func_write);

static ssize_t io_dev_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->show) {
        DEBUG_ERROR("io dev attr show is null.\n");
        return -ENOSYS;
    }

    return attribute->show(kobj, attribute, buf);
}

static ssize_t io_dev_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf,
                   size_t len)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->store) {
        DEBUG_ERROR("io dev attr store is null.\n");
        return -ENOSYS;
    }

    return attribute->store(kobj, attribute, buf, len);
}

static const struct sysfs_ops io_dev_sysfs_ops = {
    .show = io_dev_attr_show,
    .store = io_dev_attr_store,
};

static void io_dev_obj_release(struct kobject *kobj)
{
    return;
}

static struct kobj_type io_dev_ktype = {
    .sysfs_ops = &io_dev_sysfs_ops,
    .release = io_dev_obj_release,
};

static ssize_t alias_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_io_dev_t *io_dev = container_of(kobj, wb_io_dev_t, kobj);

    if (!io_dev) {
        DEBUG_ERROR("alias show io dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", io_dev->alias);
}

static ssize_t type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", PROXY_NAME);
}

static ssize_t info_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int offset;
    ssize_t buf_len;

    wb_io_dev_t *io_dev = container_of(kobj, wb_io_dev_t, kobj);
    if (!io_dev) {
        DEBUG_ERROR("info show io dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    offset = 0;
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "name: %s\n", io_dev->name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "alias: %s\n", io_dev->alias);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "io_base: 0x%x\n", io_dev->io_base);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "io_len: 0x%x\n", io_dev->io_len);
    buf_len = strlen(buf);
    return buf_len;
}

static int rw_status_check_one_time(wb_io_dev_t *io_dev, uint32_t offset, uint32_t len, uint32_t type)
{
    uint8_t wr_buf[WIDTH_4Byte];
    uint8_t rd_buf[WIDTH_4Byte];
    int ret, i;

    if (len > sizeof(wr_buf) || len > sizeof(rd_buf)) {
        DEBUG_ERROR("input param error: len: %d out of range.\n", len);
        return -EINVAL;
    }

    for (i = 0; i < len; i++) {
        wr_buf[i] = io_dev->test_data[i];
    }

    ret = io_dev_write_tmp(io_dev, offset, wr_buf, len);
    if ((ret < 0) || (ret != len)) {
        DEBUG_ERROR("STATUS_NOT_OK dev status wr offset: 0x%x, len: %u failed, ret: %d.\n",
            offset, len, ret);
        return -EIO;
    }

    mem_clear(rd_buf, sizeof(rd_buf));
    ret = io_dev_read_tmp(io_dev, offset, rd_buf, len);
    if (ret < 0 || (ret != len)) {
        DEBUG_ERROR("STATUS_NOT_OK dev status rd offset: 0x%x, len: %u failed, ret: %d.\n",
            offset, len, ret);
        return -EIO;
    }

    DEBUG_VERBOSE("offset :0x%x, len: %u, check_type: %d\n", offset, len, type);
    for (i = 0; i < len; i++) {
        DEBUG_VERBOSE("rd_buf[%d]: 0x%x, wr_buf[%d]: 0x%x\n", i, rd_buf[i], i, wr_buf[i]);
    }

    ret = dev_rw_check(rd_buf, wr_buf, len, type);
    if (ret < 0) {
        DEBUG_ERROR("STATUS_NOT_OK result check failed, ret:%d.\n", ret);
        return ret;
    }

    DEBUG_VERBOSE("STATUS_OK rw one time result check success.");
    return 0;
}

static int rw_status_check(wb_io_dev_t *io_dev, uint32_t offset, uint32_t len, uint32_t type)
{
    int ret, i;

    ret = 0;
    for (i = 0; i < LOGIC_DEV_RETRY_TIME; i++) {
        ret = rw_status_check_one_time(io_dev, offset, len, type);
        if (ret < 0) {
            DEBUG_ERROR("STATUS_NOT_OK time: %d, dev status wr offset: 0x%x failed, ret: %d\n",
                i, offset, ret);
            msleep(LOGIC_DEV_RETRY_DELAY_MS);
            continue;
        }
        break;
    }

    return ret;
}

static ssize_t status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_io_dev_t *io_dev;
    uint32_t offset, type, len;
    int ret, i;

    io_dev = container_of(kobj, wb_io_dev_t, kobj);
    if (!io_dev) {
        DEBUG_ERROR("Failed: status show param is NULL.\n");
        return -ENODEV;
    }

    type = io_dev->status_check_type;

    if (type == NOT_SUPPORT_CHECK_STATUS) {
        DEBUG_ERROR("unsupport dev status check.\n");
        return -EOPNOTSUPP;
    }

    if (time_before(jiffies, io_dev->last_jiffies + msecs_to_jiffies(status_cache_ms))) {
        /* Within the time range of status_cache_ms, directly return the last result */
        DEBUG_VERBOSE("time before last time %d ms return last status: %d\n",
            status_cache_ms, io_dev->dev_status);
        return sprintf(buf, "%u\n", io_dev->dev_status);
    }

    io_dev->last_jiffies = jiffies;

    len = WIDTH_1Byte;

    mutex_lock(&io_dev->update_lock);

    for (i = 0; i < len; i++) {
        /* reverse to ensure that different data is read and written each time the verification is performed.*/
        io_dev->test_data[i] = ~io_dev->test_data[i];
    }
    for (i = 0; i < io_dev->test_reg_num; i++) {
        offset = io_dev->test_reg[i];
        ret = rw_status_check(io_dev, offset, len, type);
        if (ret < 0) {
            io_dev->dev_status = LOGIC_DEV_STATUS_NOT_OK;
            DEBUG_ERROR("STATUS_NOT_OK result check all retry failed.\n");
            mutex_unlock(&io_dev->update_lock);
            return sprintf(buf, "%u\n", io_dev->dev_status);
        }
    }

    io_dev->dev_status = LOGIC_DEV_STATUS_OK;
    mutex_unlock(&io_dev->update_lock);
    return sprintf(buf, "%u\n", io_dev->dev_status);
}

static ssize_t file_cache_rd_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_io_dev_t *io_dev = container_of(kobj, wb_io_dev_t, kobj);

    if (!io_dev) {
        DEBUG_ERROR("file_cache_rd_show io dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%u\n", io_dev->file_cache_rd);
}

static ssize_t file_cache_rd_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_io_dev_t *io_dev = container_of(kobj, wb_io_dev_t, kobj);
    u8 val;
    int ret;

    if (!io_dev) {
        DEBUG_ERROR("file_cache_rd_store io dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    io_dev->file_cache_rd = val;

    return count;
}

static ssize_t file_cache_wr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_io_dev_t *io_dev = container_of(kobj, wb_io_dev_t, kobj);

    if (!io_dev) {
        DEBUG_ERROR("file_cache_wr_show io dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%u\n", io_dev->file_cache_wr);
}

static ssize_t file_cache_wr_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_io_dev_t *io_dev = container_of(kobj, wb_io_dev_t, kobj);
    u8 val;
    int ret;

    if (!io_dev) {
        DEBUG_ERROR("file_cache_wr_store io dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    io_dev->file_cache_wr = val;

    return count;
}

static ssize_t cache_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_io_dev_t *io_dev = container_of(kobj, wb_io_dev_t, kobj);

    if (!io_dev) {
        DEBUG_ERROR("cache_file_path_show io dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", io_dev->cache_file_path);
}

static ssize_t mask_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_io_dev_t *io_dev = container_of(kobj, wb_io_dev_t, kobj);

    if (!io_dev) {
        DEBUG_ERROR("mask_file_path_show io dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", io_dev->mask_file_path);
}

static struct kobj_attribute alias_attribute = __ATTR(alias, S_IRUGO, alias_show, NULL);
static struct kobj_attribute type_attribute = __ATTR(type, S_IRUGO, type_show, NULL);
static struct kobj_attribute info_attribute = __ATTR(info, S_IRUGO, info_show, NULL);
static struct kobj_attribute status_attribute = __ATTR(status, S_IRUGO, status_show, NULL);
static struct kobj_attribute file_cache_rd_attribute = __ATTR(file_cache_rd, S_IRUGO  | S_IWUSR, file_cache_rd_show, file_cache_rd_store);
static struct kobj_attribute file_cache_wr_attribute = __ATTR(file_cache_wr, S_IRUGO  | S_IWUSR, file_cache_wr_show, file_cache_wr_store);
static struct kobj_attribute cache_file_path_attribute = __ATTR(cache_file_path, S_IRUGO, cache_file_path_show, NULL);
static struct kobj_attribute mask_file_path_attribute = __ATTR(mask_file_path, S_IRUGO, mask_file_path_show, NULL);

static struct attribute *io_dev_attrs[] = {
    &alias_attribute.attr,
    &type_attribute.attr,
    &info_attribute.attr,
    &status_attribute.attr,
    &file_cache_rd_attribute.attr,
    &file_cache_wr_attribute.attr,
    &cache_file_path_attribute.attr,
    &mask_file_path_attribute.attr,
    NULL,
};

static struct attribute_group io_dev_attr_group = {
    .attrs = io_dev_attrs,
};

static int of_io_dev_config_init(struct platform_device *pdev, wb_io_dev_t *wb_io_dev)
{
    int i, ret;
    uint32_t type;

    ret = 0;
    if (of_property_read_u8(pdev->dev.of_node, "io_type", &wb_io_dev->io_type)) {
        /* dts have no io_type,set default 0 */
        wb_io_dev->io_type = IO_DEV_USE_IOPORT;
    }

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

        DEBUG_VERBOSE("Indirect addressing: wr_data: 0x%x, rd_data: 0x%x, addr_low: 0x%x, addr_high: 0x%x, opt_ctl: 0x%x\n",
            wb_io_dev->wr_data, wb_io_dev->rd_data, wb_io_dev->addr_low, wb_io_dev->addr_high, wb_io_dev->opt_ctl);

        if (of_property_read_u32(pdev->dev.of_node, "wr_data_width", &wb_io_dev->wr_data_width)) {
            /* dts have no wr_data_width,set default 1 */
            wb_io_dev->wr_data_width = WIDTH_1Byte;
        }
        if (of_property_read_u32(pdev->dev.of_node, "rd_data_width", &wb_io_dev->rd_data_width)) {
            /* dts have no rd_data_width,set default 1 */
            wb_io_dev->rd_data_width = WIDTH_1Byte;
        }
        DEBUG_VERBOSE("Indirect addressing: wr_data_width: %d, rd_data_width: %d\n",
            wb_io_dev->wr_data_width, wb_io_dev->rd_data_width);
    } else {
        wb_io_dev->indirect_addr = 0;
    }
    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to get dts config, ret:%d.\n", ret);
        return -ENXIO;
    }

    if (of_property_read_string(pdev->dev.of_node, "io_dev_alias", &wb_io_dev->alias)) {
        wb_io_dev->alias = wb_io_dev->name;
    }

    ret = of_property_read_u32(pdev->dev.of_node, "status_check_type", &wb_io_dev->status_check_type);
    if (ret == 0) {
        type = wb_io_dev->status_check_type;
        if (type != READ_BACK_CHECK && type != READ_BACK_NAGATIVE_CHECK) {
            dev_err(&pdev->dev, "Invalid status_check_type: %u\n", type);
            return -EINVAL;
        }
        DEBUG_VERBOSE("status_check_type: %d, \n", wb_io_dev->status_check_type);

        ret = of_property_read_u32(pdev->dev.of_node, "test_reg_num", &wb_io_dev->test_reg_num);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get test_reg_num config, ret: %d\n", ret);
            return -ENXIO;
        }
        if (wb_io_dev->test_reg_num == 0 || wb_io_dev->test_reg_num > TEST_REG_MAX_NUM) {
            dev_err(&pdev->dev, "Invalid test_reg_num: %u\n", wb_io_dev->test_reg_num);
            return -EINVAL;
        }

        ret = of_property_read_u32_array(pdev->dev.of_node, "test_reg", wb_io_dev->test_reg, wb_io_dev->test_reg_num);
        if(ret != 0) {
            dev_err(&pdev->dev, "Failed to get test_reg config, ret: %d\n", ret);
            return -ENXIO;
        }

        wb_io_dev->last_jiffies = jiffies;
        wb_io_dev->dev_status = LOGIC_DEV_STATUS_OK;

        for (i = 0; i < WIDTH_4Byte; i++) {
            wb_io_dev->test_data[i] = INIT_TEST_DATA;
        }

        for (i = 0; i < wb_io_dev->test_reg_num; i++) {
            DEBUG_VERBOSE("test_reg[%d] = 0x%x.\n", i, wb_io_dev->test_reg[i]);
        }
    } else {
        wb_io_dev->status_check_type = NOT_SUPPORT_CHECK_STATUS;
        DEBUG_VERBOSE("not support dev status check sysfs.\n");
    }

    ret = of_property_read_u32(pdev->dev.of_node, "log_num", &(wb_io_dev->log_node.log_num));
    if (ret == 0) {
        if ((wb_io_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) || (wb_io_dev->log_node.log_num == 0)) {
            dev_err(&pdev->dev, "Invalid log_num: %u\n", wb_io_dev->log_node.log_num);
            return -EINVAL;
        }
        ret = of_property_read_u32_array(pdev->dev.of_node, "log_index", wb_io_dev->log_node.log_index, wb_io_dev->log_node.log_num);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get log_index config, ret: %d\n", ret);
            return -EINVAL;
        }
        for (i = 0; i < wb_io_dev->log_node.log_num; i++) {
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, wb_io_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
        wb_io_dev->log_node.log_num = 0;
    }

    DEBUG_VERBOSE("name: %s, alias: %s, io type: %d, io base: 0x%x, io len: 0x%x, addressing type: %s.\n",
        wb_io_dev->name, wb_io_dev->alias, wb_io_dev->io_type, wb_io_dev->io_base, wb_io_dev->io_len,
        wb_io_dev->indirect_addr ? "indirect" : "direct");

    return 0;
}

static int io_dev_config_init(struct platform_device *pdev, wb_io_dev_t *wb_io_dev)
{
    int i;
    uint32_t type;
    io_dev_device_t *io_dev_device;

    if (pdev->dev.platform_data == NULL) {
        dev_err(&pdev->dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }

    io_dev_device = pdev->dev.platform_data;
    wb_io_dev->io_type = io_dev_device->io_type;
    wb_io_dev->name = io_dev_device->io_dev_name;
    wb_io_dev->io_base = io_dev_device->io_base;
    wb_io_dev->io_len = io_dev_device->io_len;
    wb_io_dev->indirect_addr = io_dev_device->indirect_addr;
    if (wb_io_dev->indirect_addr == 1) {
        wb_io_dev->wr_data = io_dev_device->wr_data;
        wb_io_dev->wr_data_width = io_dev_device->wr_data_width;
        wb_io_dev->addr_low = io_dev_device->addr_low;
        wb_io_dev->addr_high = io_dev_device->addr_high;
        wb_io_dev->rd_data = io_dev_device->rd_data;
        wb_io_dev->rd_data_width = io_dev_device->rd_data_width;
        wb_io_dev->opt_ctl = io_dev_device->opt_ctl;
        DEBUG_VERBOSE("Indirect addressing: wr_data: 0x%x, rd_data: 0x%x, addr_low: 0x%x, addr_high: 0x%x, opt_ctl: 0x%x\n",
            wb_io_dev->wr_data, wb_io_dev->rd_data, wb_io_dev->addr_low, wb_io_dev->addr_high, wb_io_dev->opt_ctl);
        if (wb_io_dev->wr_data_width == 0) {
            wb_io_dev->wr_data_width = WIDTH_1Byte;
        }
        if (wb_io_dev->rd_data_width == 0) {
            wb_io_dev->rd_data_width = WIDTH_1Byte;
        }
        DEBUG_VERBOSE("Indirect addressing: wr_data_width: %d, rd_data_width: %d\n",
            wb_io_dev->wr_data_width, wb_io_dev->rd_data_width);
    }
    if (strlen(io_dev_device->io_dev_alias) == 0) {
        wb_io_dev->alias = wb_io_dev->name;
    } else {
        wb_io_dev->alias = io_dev_device->io_dev_alias;
    }

    wb_io_dev->status_check_type = io_dev_device->status_check_type;
    if (wb_io_dev->status_check_type != NOT_SUPPORT_CHECK_STATUS) {
        type = wb_io_dev->status_check_type;
        if ((type != READ_BACK_CHECK) && (type != READ_BACK_NAGATIVE_CHECK)) {
            dev_err(&pdev->dev, "Invalid status_check_type: %u\n", type);
            return -EINVAL;
        }
        DEBUG_VERBOSE("status_check_type: %d\n", wb_io_dev->status_check_type);

        wb_io_dev->test_reg_num = io_dev_device->test_reg_num;
        if ((wb_io_dev->test_reg_num == 0) || (wb_io_dev->test_reg_num > TEST_REG_MAX_NUM)) {
            dev_err(&pdev->dev, "Invalid test_reg_num: %u\n", wb_io_dev->test_reg_num);
            return -EINVAL;
        }
        for (i = 0; i < wb_io_dev->test_reg_num; i++) {
            wb_io_dev->test_reg[i] = io_dev_device->test_reg[i];
            DEBUG_VERBOSE("test_reg[%d] address = 0x%x.\n", i, wb_io_dev->test_reg[i]);
        }

        wb_io_dev->last_jiffies = jiffies;
        wb_io_dev->dev_status = LOGIC_DEV_STATUS_OK;
        for (i = 0; i < WIDTH_4Byte; i++) {
            wb_io_dev->test_data[i] = INIT_TEST_DATA;
        }
    } else {
        DEBUG_VERBOSE("not support dev status check sysfs.\n");
    }

    wb_io_dev->log_node.log_num = io_dev_device->log_num;
    if (wb_io_dev->log_node.log_num != 0) {
        if (wb_io_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) {
            dev_err(&pdev->dev, "Invalid log_num: %u\n", wb_io_dev->log_node.log_num);
            return -EINVAL;
        }
        for (i = 0; i < wb_io_dev->log_node.log_num; i++) {
            wb_io_dev->log_node.log_index[i] = io_dev_device->log_index[i];
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, wb_io_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
    }

    DEBUG_VERBOSE("name: %s, alias: %s, io type: %d, io base: 0x%x, io len: 0x%x, addressing type: %s.\n",
        wb_io_dev->name, wb_io_dev->alias, wb_io_dev->io_type, wb_io_dev->io_base, wb_io_dev->io_len,
        wb_io_dev->indirect_addr ? "indirect" : "direct");

    return 0;
}

static int io_dev_probe(struct platform_device *pdev)
{
    int ret;
    wb_io_dev_t *wb_io_dev;
    struct miscdevice *misc;

    wb_io_dev = devm_kzalloc(&pdev->dev, sizeof(wb_io_dev_t), GFP_KERNEL);
    if (!wb_io_dev) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        ret = -ENOMEM;
        return ret;
    }
    spin_lock_init(&wb_io_dev->io_dev_lock);

    if (pdev->dev.of_node) {
        ret = of_io_dev_config_init(pdev, wb_io_dev);
    } else {
        ret = io_dev_config_init(pdev, wb_io_dev);
    }

    if (ret < 0) {
        return ret;
    }

    switch (wb_io_dev->io_type) {
    case IO_DEV_USE_IOPORT:
        break;
    case IO_DEV_USE_IOMEM:
        wb_io_dev->vir_base = ioremap(wb_io_dev->io_base, wb_io_dev->io_len);
        break;
    default:
        dev_err(&pdev->dev, "Invalid io_type: %u\n", wb_io_dev->io_type);
        return -EINVAL;
    }

    mutex_init(&wb_io_dev->update_lock);
    mutex_init(&wb_io_dev->log_node.file_lock);

    wb_io_dev->file_cache_rd = 0;
    wb_io_dev->file_cache_wr = 0;
    snprintf(wb_io_dev->cache_file_path, sizeof(wb_io_dev->cache_file_path), CACHE_FILE_PATH, wb_io_dev->name);
    snprintf(wb_io_dev->mask_file_path, sizeof(wb_io_dev->mask_file_path), MASK_FILE_PATH, wb_io_dev->name);

    /* creat parent dir by dev name in /sys/logic_dev */
    ret = kobject_init_and_add(&wb_io_dev->kobj, &io_dev_ktype, logic_dev_kobj, "%s", wb_io_dev->name);
    if (ret) {
        kobject_put(&wb_io_dev->kobj);
        dev_err(&pdev->dev, "Failed to creat parent dir: %s, ret: %d\n", wb_io_dev->name, ret);
        return ret;
    }

    wb_io_dev->sysfs_group = &io_dev_attr_group;
    ret = sysfs_create_group(&wb_io_dev->kobj, wb_io_dev->sysfs_group);
    if (ret) {
        dev_err(&pdev->dev, "Failed to create %s sysfs group, ret: %d\n", wb_io_dev->name, ret);
        goto remove_parent_kobj;
    }

    platform_set_drvdata(pdev, wb_io_dev);
    misc = &wb_io_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = wb_io_dev->name;
    misc->fops = &io_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&pdev->dev, "Failed to register %s device\n", misc->name);
        ret = -ENXIO;
        goto remove_sysfs_group;
    }
    if (misc->minor >= MAX_DEV_NUM) {
        dev_err(&pdev->dev, "Error: device minor[%d] more than max device num[%d].\n",
            misc->minor, MAX_DEV_NUM);
        ret = -EINVAL;
        goto deregister_misc;
    }
    io_dev_arry[misc->minor] = wb_io_dev;
    dev_info(&pdev->dev, "register %s device [0x%x][0x%x] with minor %d using %s addressing success.\n",
        misc->name, wb_io_dev->io_base, wb_io_dev->io_len, misc->minor,
        wb_io_dev->indirect_addr ? "indirect" : "direct");

    return 0;

deregister_misc:
    misc_deregister(misc);
remove_sysfs_group:
    sysfs_remove_group(&wb_io_dev->kobj, (const struct attribute_group *)wb_io_dev->sysfs_group);
remove_parent_kobj:
    kobject_put(&wb_io_dev->kobj);
    return ret;
}

static int io_dev_remove(struct platform_device *pdev)
{
    int minor;
    wb_io_dev_t *wb_io_dev;

    wb_io_dev = platform_get_drvdata(pdev);
    minor = wb_io_dev->misc.minor;
    if (minor < MAX_DEV_NUM && (io_dev_arry[minor] != NULL)) {
        dev_dbg(&pdev->dev, "misc_deregister %s, minor: %d\n", wb_io_dev->misc.name, minor);
        misc_deregister(&io_dev_arry[minor]->misc);
        io_dev_arry[minor] = NULL;
    }

    if (wb_io_dev->sysfs_group) {
        dev_dbg(&pdev->dev, "Unregister %s io_dev sysfs group\n", wb_io_dev->name);
        sysfs_remove_group(&wb_io_dev->kobj, (const struct attribute_group *)wb_io_dev->sysfs_group);
        kobject_put(&wb_io_dev->kobj);
    }

    dev_info(&pdev->dev, "Remove %s io device success.\n", wb_io_dev->name);
    platform_set_drvdata(pdev, NULL);
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
