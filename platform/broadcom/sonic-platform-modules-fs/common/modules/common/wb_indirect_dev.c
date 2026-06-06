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

#include "wb_indirect_dev.h"
#include <wb_bsp_kernel_debug.h>
#include <wb_kernel_io.h>

#define MODULE_NAME                "wb-indirect-dev"
#define INDIRECT_ADDR_H(addr)      ((addr >> 8) & 0xff)
#define INDIRECT_ADDR_L(addr)      ((addr) & 0xff)
#define INDIRECT_OP_WRITE          (0x2)
#define INDIRECT_OP_READ           (0x3)

typedef enum {
    WB_DEV_SPIN_LOCK_MODE = 1,
    WB_DEV_MUTEX_MODE = 2,
} lock_mode_t;

/* Use the wb_bsp_kernel_debug header file must define debug variable */
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int status_cache_ms = 0;
module_param(status_cache_ms, int, S_IRUGO | S_IWUSR);

static struct indirect_dev_info* indirect_dev_arry[MAX_DEV_NUM];

typedef struct indirect_dev_info {
    const char *name;               /* generate dev name */
    const char *alias;              /* generate dev alias */
    const char *logic_dev_name;     /* dependent dev name */
    uint32_t indirect_len;          /* dev data len */
    uint32_t data_bus_width;        /* dev data_bus_width */
    uint32_t wr_data;               /* dependent dev wr date reg */
    uint32_t addr_low;              /* dependent dev w/r addr reg low */
    uint32_t addr_high;             /* dependent dev w/r addr reg high */
    uint32_t rd_data;               /* dependent dev rd date reg */
    uint32_t opt_ctl;               /* dependent dev opt code reg */
    uint32_t logic_func_mode;       /* 1: i2c, 2: file, 3:pcie, 4:io, 5:spi */
    uint32_t lock_mode;             /* 1:spin lock, 2:mutex */
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
    spinlock_t indirect_dev_lock;
    struct mutex mutex_lock;
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
} wb_indirect_dev_t;

static void wb_dev_lock_init(struct indirect_dev_info *indirect_dev)
{
    if (indirect_dev->lock_mode == WB_DEV_MUTEX_MODE) {
        mutex_init(&indirect_dev->mutex_lock);
    } else {
        spin_lock_init(&indirect_dev->indirect_dev_lock);
    }

    return;
}

static void wb_dev_lock(struct indirect_dev_info *indirect_dev, unsigned long *flags)
{
    if (indirect_dev->lock_mode == WB_DEV_MUTEX_MODE) {
        mutex_lock(&indirect_dev->mutex_lock);
    } else {
        spin_lock_irqsave(&indirect_dev->indirect_dev_lock, *flags);
    }

    return;
}

static void wb_dev_unlock(struct indirect_dev_info *indirect_dev, unsigned long *flags)
{
    if (indirect_dev->lock_mode == WB_DEV_MUTEX_MODE) {
        mutex_unlock(&indirect_dev->mutex_lock);
    } else {
        spin_unlock_irqrestore(&indirect_dev->indirect_dev_lock, *flags);
    }

    return;
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

    wb_dev_lock(indirect_dev, &flags);

    DEBUG_VERBOSE("write reg addr_low=0x%x, value = 0x%x\n\n",  indirect_dev->addr_low, addr_l);
    ret = wb_logic_reg_write(indirect_dev, indirect_dev->addr_low, &addr_l, WIDTH_1Byte);
    if (ret < 0) {
        DEBUG_ERROR("indirect_read write reg error.offset = 0x%x, value = 0x%x\n", indirect_dev->addr_low, addr_l);
        goto fail;
    }

    DEBUG_VERBOSE("write reg addr_high=0x%x, value = 0x%x\n\n",  indirect_dev->addr_high, addr_h);
    ret = wb_logic_reg_write(indirect_dev, indirect_dev->addr_high, &addr_h, WIDTH_1Byte);
    if (ret < 0) {
        DEBUG_ERROR("indirect_read write reg error.offset = 0x%x, value = 0x%x\n", indirect_dev->addr_high, addr_h);
        goto fail;
    }

    DEBUG_VERBOSE("write reg opt_ctl=0x%x, value = 0x%x\n\n",  indirect_dev->opt_ctl, op_code);
    ret = wb_logic_reg_write(indirect_dev, indirect_dev->opt_ctl, &op_code, WIDTH_1Byte);
    if (ret < 0) {
        DEBUG_ERROR("indirect_read write reg error.offset = 0x%x, value = 0x%x\n", indirect_dev->opt_ctl, INDIRECT_OP_READ);
        goto fail;
    }

    ret = wb_logic_reg_read(indirect_dev, indirect_dev->rd_data, buf, rd_data_width);
    if (ret < 0) {
        DEBUG_ERROR("indirect_read read reg error.read offset = 0x%x\n, ret = %d", indirect_dev->rd_data, ret);
        goto fail;
    }

    DEBUG_VERBOSE("indirect_read success, addr = 0x%x\n", address);
    wb_dev_unlock(indirect_dev, &flags);
    return ret;
fail:
    wb_dev_unlock(indirect_dev, &flags);
    return ret;
}

static int device_read(struct indirect_dev_info *indirect_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, ret;
    u32 data_width;
    u32 tmp;

    if (offset >= indirect_dev->indirect_len) {
        DEBUG_VERBOSE("offset: 0x%x, indirect len: 0x%x, count: %zu, EOF.\n",
            offset, indirect_dev->indirect_len, count);
        return 0;
    }

    data_width = indirect_dev->data_bus_width;
    if (offset % data_width) {
        DEBUG_ERROR("data bus width:%d, offset:0x%x, read size %zu invalid.\n",
            data_width, offset, count);
        return -EINVAL;
    }

    if (count > indirect_dev->indirect_len - offset) {
        DEBUG_VERBOSE("read count out of range. input len:%zu, read len:%u.\n",
            count, indirect_dev->indirect_len - offset);
        count = indirect_dev->indirect_len - offset;
    }
    tmp = count;

    for (i = 0; i < count; i += data_width) {
        ret = indirect_addressing_read(indirect_dev, buf + i, offset + i, (tmp > data_width ? data_width : tmp));
        if (ret < 0) {
            DEBUG_ERROR("read error.read offset = %u\n", (offset + i));
            return -EFAULT;
        }
        tmp -= data_width;
    }

    if (indirect_dev->file_cache_rd) {
        ret = cache_value_read(indirect_dev->mask_file_path, indirect_dev->cache_file_path, offset, buf, count);
        if (ret < 0) {
            DEBUG_ERROR("indirect_dev data offset: 0x%x, read_len: %zu, read cache file fail, ret: %d, return act value\n",
                offset, count, ret);
        } else {
            DEBUG_VERBOSE("indirect_dev data offset: 0x%x, read_len: %zu success, read from cache value\n",
                offset, count);
        }
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(indirect_dev->name, offset, buf, count, true);
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

    wb_dev_lock(indirect_dev, &flags);

    ret = wb_logic_reg_write(indirect_dev, indirect_dev->wr_data, buf, wr_data_width);
    if (ret < 0) {
        DEBUG_ERROR("indirect_write read reg error.read offset = 0x%x\n, ret = %d", indirect_dev->wr_data, ret);
        goto fail;
    }

    DEBUG_VERBOSE("write reg addr_low=0x%x, value = 0x%x\n\n",  indirect_dev->addr_low, addr_l);
    ret = wb_logic_reg_write(indirect_dev, indirect_dev->addr_low, &addr_l, WIDTH_1Byte);
    if (ret < 0) {
        DEBUG_ERROR("indirect_write write reg error.offset = 0x%x, value = 0x%x\n", indirect_dev->addr_low, addr_l);
        goto fail;
    }

    DEBUG_VERBOSE("write reg addr_high=0x%x, value = 0x%x\n\n",  indirect_dev->addr_high, addr_h);
    ret = wb_logic_reg_write(indirect_dev, indirect_dev->addr_high, &addr_h, WIDTH_1Byte);
    if (ret < 0) {
        DEBUG_ERROR("indirect_write write reg error.offset = 0x%x, value = 0x%x\n", indirect_dev->addr_high, addr_h);
        goto fail;
    }

    DEBUG_VERBOSE("write reg opt_ctl=0x%x, value = 0x%x\n\n",  indirect_dev->opt_ctl, op_code);
    ret = wb_logic_reg_write(indirect_dev, indirect_dev->opt_ctl, &op_code, WIDTH_1Byte);
    if (ret < 0) {
        DEBUG_ERROR("indirect_write write reg error.offset = 0x%x, value = 0x%x\n", indirect_dev->opt_ctl, INDIRECT_OP_READ);
        goto fail;
    }

    DEBUG_VERBOSE("indirect_write success, addr = 0x%x\n", address);
    wb_dev_unlock(indirect_dev, &flags);
    return ret;
fail:
    wb_dev_unlock(indirect_dev, &flags);
    return ret;
}

static int device_write(struct indirect_dev_info *indirect_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, ret;
    u32 data_width;
    u32 tmp;

    if (offset >= indirect_dev->indirect_len) {
        DEBUG_VERBOSE("offset: 0x%x, indirect len: 0x%x, count: %zu, EOF.\n",
            offset, indirect_dev->indirect_len, count);
        return 0;
    }

    data_width = indirect_dev->data_bus_width;
    if (offset % data_width) {
        DEBUG_ERROR("data bus width:%d, offset:0x%x, read size %zu invalid.\n",
            data_width, offset, count);
        return -EINVAL;
    }

    if (count > (indirect_dev->indirect_len - offset)) {
        DEBUG_VERBOSE("write count out of range. input len:%zu, read len:%u.\n",
            count, indirect_dev->indirect_len - offset);
        count = indirect_dev->indirect_len - offset;
    }

    tmp = count;
    for (i = 0; i < count; i += data_width) {
        ret = indirect_addressing_write(indirect_dev, buf + i, offset + i, (tmp > data_width ? data_width : tmp));
        if (ret < 0) {
            DEBUG_ERROR("write error.offset = %u\n", (offset + i));
            return -EFAULT;
        }
        tmp -= data_width;
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(indirect_dev->name, offset, buf, count, false);
    }

    return count;
}

static ssize_t indirect_dev_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int ret, read_len;
    struct indirect_dev_info *indirect_dev;

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, read failed.\n");
        return -EINVAL;
    }

    indirect_dev = file->private_data;
    if (indirect_dev == NULL) {
        DEBUG_ERROR("can't get read private_data.\n");
        return -EINVAL;
    }

    if (count == 0) {
        DEBUG_ERROR("Invalid params, read count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(val)) {
        DEBUG_VERBOSE("read count %zu exceed max %zu.\n", count, sizeof(val));
        count = sizeof(val);
    }

    mem_clear(val, sizeof(val));
    read_len = device_read(indirect_dev, (uint32_t)*offset, val, count);
    if (read_len < 0) {
        DEBUG_ERROR("indirect dev read failed, dev name:%s, offset:0x%x, len:%zu.\n",
            indirect_dev->name, (uint32_t)*offset, count);
        return read_len;
    }

    if (read_len == 0) {
        DEBUG_VERBOSE("indirect dev read EOF, offset: 0x%llx, count: %zu\n", *offset, count);
        return 0;
    }

    DEBUG_VERBOSE("read, buf: %p, offset: %lld, read count %zu.\n", buf, *offset, count);
    memcpy(buf, val, read_len);

    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t indirect_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    DEBUG_VERBOSE("indirect_dev_read_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(to), iocb->ki_pos);
    return wb_iov_iter_read(iocb, to, indirect_dev_read);
}

static ssize_t indirect_dev_write(struct file *file, char *buf, size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int write_len;
    struct indirect_dev_info *indirect_dev;
    char bsp_log_dev_name[BSP_LOG_DEV_NAME_MAX_LEN];
    char bsp_log_file_path[BSP_LOG_DEV_NAME_MAX_LEN];

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, read failed.\n");
        return -EINVAL;
    }

    indirect_dev = file->private_data;
    if (indirect_dev == NULL) {
        DEBUG_ERROR("get write private_data error.\n");
        return -EINVAL;
    }

    if (count == 0) {
        DEBUG_ERROR("Invalid params, write count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(val)) {
        DEBUG_VERBOSE("write count %zu exceed max %zu.\n", count, sizeof(val));
        count = sizeof(val);
    }

    mem_clear(val, sizeof(val));

    DEBUG_VERBOSE("write, buf: %p, offset: %lld, write count %zu.\n", buf, *offset, count);
    memcpy(val, buf, count);

    if (indirect_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Devfs]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_file_path), "%s.%s_bsp_key_reg", BSP_LOG_DIR, indirect_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(indirect_dev->log_node), (uint32_t)*offset, val, count);
    }

    write_len = device_write(indirect_dev, (uint32_t)*offset, val, count);
    if (write_len < 0) {
        DEBUG_ERROR("indirect dev write failed, dev name:%s, offset:0x%llx, len:%zu.\n",
            indirect_dev->name, *offset, count);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t indirect_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    DEBUG_VERBOSE("indirect_dev_write_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(from), iocb->ki_pos);
    return wb_iov_iter_write(iocb, from, indirect_dev_write);
}

static loff_t indirect_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    struct indirect_dev_info *indirect_dev;

    indirect_dev = file->private_data;
    if (indirect_dev == NULL) {
        DEBUG_ERROR("indirect_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            DEBUG_ERROR("SEEK_SET, offset:%lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > indirect_dev->indirect_len) {
            DEBUG_ERROR("SEEK_SET out of range, offset:%lld, i2c_len:0x%x.\n",
                offset, indirect_dev->indirect_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > indirect_dev->indirect_len) || ((file->f_pos + offset) < 0)) {
            DEBUG_ERROR("SEEK_CUR out of range, f_ops:%lld, offset:%lld.\n",
                 file->f_pos, offset);
            ret = -EINVAL;
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

static long indirect_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int indirect_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    struct indirect_dev_info *indirect_dev;

    if (minor >= MAX_DEV_NUM) {
        DEBUG_ERROR("minor [%d] is greater than max dev num [%d], open fail\n", minor, MAX_DEV_NUM);
        return -ENODEV;
    }

    indirect_dev = indirect_dev_arry[minor];
    if (indirect_dev == NULL) {
        DEBUG_ERROR("indirect_dev is NULL, open failed, minor = %d\n", minor);
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
    .read_iter      = indirect_dev_read_iter,
    .write_iter     = indirect_dev_write_iter,
    .unlocked_ioctl = indirect_dev_ioctl,
    .open           = indirect_dev_open,
    .release        = indirect_dev_release,
};

static struct indirect_dev_info *dev_match(const char *path)
{
    struct indirect_dev_info *indirect_dev;
    char dev_name[MAX_NAME_SIZE];
    int i;

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (indirect_dev_arry[i] == NULL) {
            continue;
        }
        indirect_dev = indirect_dev_arry[i];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", indirect_dev->name);
        if (!strcmp(path, dev_name)) {
            DEBUG_VERBOSE("get dev_name = %s, minor = %d\n", dev_name, i);
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
        DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    indirect_dev = dev_match(path);
    if (indirect_dev == NULL) {
        DEBUG_ERROR("indirect_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    read_len = device_read(indirect_dev, offset, buf, count);
    if (read_len < 0) {
        DEBUG_ERROR("indirect_dev_read_tmp failed, ret:%d.\n", read_len);
    }
    return read_len;
}
EXPORT_SYMBOL(indirect_device_func_read);

int indirect_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct indirect_dev_info *indirect_dev;
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

    indirect_dev = dev_match(path);
    if (indirect_dev == NULL) {
        DEBUG_ERROR("indirect_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    if (indirect_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Symbol]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_file_path), "%s.%s_bsp_key_reg", BSP_LOG_DIR, indirect_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(indirect_dev->log_node), offset, buf, count);
    }

    write_len = device_write(indirect_dev, offset, buf, count);
    if (write_len < 0) {
        DEBUG_ERROR("indirect_dev_write_tmp failed, ret:%d.\n", write_len);
    }
    return write_len;
}
EXPORT_SYMBOL(indirect_device_func_write);

static ssize_t indirect_dev_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->show) {
        DEBUG_ERROR("indirect dev attr show is null.\n");
        return -ENOSYS;
    }

    return attribute->show(kobj, attribute, buf);
}

static ssize_t indirect_dev_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf,
                   size_t len)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->store) {
        DEBUG_ERROR("indirect dev attr store is null.\n");
        return -ENOSYS;
    }

    return attribute->store(kobj, attribute, buf, len);
}

static const struct sysfs_ops indirect_dev_sysfs_ops = {
    .show = indirect_dev_attr_show,
    .store = indirect_dev_attr_store,
};

static void indirect_dev_obj_release(struct kobject *kobj)
{
    return;
}

static struct kobj_type indirect_dev_ktype = {
    .sysfs_ops = &indirect_dev_sysfs_ops,
    .release = indirect_dev_obj_release,
};

static ssize_t alias_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_indirect_dev_t *indirect_dev = container_of(kobj, wb_indirect_dev_t, kobj);

    if (!indirect_dev) {
        DEBUG_ERROR("alias show indirect dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", indirect_dev->alias);
}

static ssize_t type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", MODULE_NAME);
}

static ssize_t info_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int offset;
    ssize_t buf_len;

    wb_indirect_dev_t *indirect_dev = container_of(kobj, wb_indirect_dev_t, kobj);
    if (!indirect_dev) {
        DEBUG_ERROR("info show alias_show indirect dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    offset = 0;
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "name: %s\n", indirect_dev->name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "alias: %s\n", indirect_dev->alias);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "dependent logic_dev_name: %s\n", indirect_dev->logic_dev_name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "indirect_len: 0x%x\n", indirect_dev->indirect_len);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "wr_data reg: 0x%x\n", indirect_dev->wr_data);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "rd_data reg: 0x%x\n", indirect_dev->rd_data);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "opt_ctl reg: 0x%x\n", indirect_dev->opt_ctl);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "addr_high reg: 0x%x\n", indirect_dev->addr_high);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "addr_low reg: 0x%x\n", indirect_dev->addr_low);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "logic_func_mode: %u\n", indirect_dev->logic_func_mode);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "lock_mode reg: %u\n", indirect_dev->lock_mode);
    buf_len = strlen(buf);
    return buf_len;
}

static int rw_status_check_one_time(wb_indirect_dev_t *indirect_dev, uint32_t offset, uint32_t len, uint32_t type)
{
    uint8_t wr_buf[WIDTH_4Byte];
    uint8_t rd_buf[WIDTH_4Byte];
    int ret, i;

    if (len > sizeof(wr_buf) || len > sizeof(rd_buf)) {
        DEBUG_ERROR("input param error: len:%d out of range.\n", len);
        return -EINVAL;
    }

    for (i = 0; i < len; i++) {
        wr_buf[i] = indirect_dev->test_data[i];
    }

    ret = device_write(indirect_dev, offset, wr_buf, len);
    if ((ret < 0) || (ret != len)) {
        DEBUG_ERROR("STATUS_NOT_OK dev status wr offset: 0x%x, len: %u failed, ret: %d.\n",
            offset, len, ret);
        return -EIO;
    }

    mem_clear(rd_buf, sizeof(rd_buf));
    ret = device_read(indirect_dev, offset, rd_buf, len);
    if ((ret < 0) || (ret != len)) {
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

static int rw_status_check(wb_indirect_dev_t *indirect_dev, uint32_t offset, uint32_t len, uint32_t type)
{
    int ret, i;

    ret = 0;
    for (i = 0; i < LOGIC_DEV_RETRY_TIME; i++) {
        ret = rw_status_check_one_time(indirect_dev, offset, len, type);
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
    wb_indirect_dev_t *indirect_dev;
    uint32_t offset, type, len;
    int ret, i;

    indirect_dev = container_of(kobj, wb_indirect_dev_t, kobj);
    if (!indirect_dev) {
        DEBUG_ERROR("Failed: status show param is NULL.\n");
        return -ENODEV;
    }

    type = indirect_dev->status_check_type;

    if (type == NOT_SUPPORT_CHECK_STATUS) {
        DEBUG_ERROR("unsupport dev status check.\n");
        return -EOPNOTSUPP;
    }

    if (time_before(jiffies, indirect_dev->last_jiffies + msecs_to_jiffies(status_cache_ms))) {
        /* Within the time range of status_cache_ms, directly return the last result */
        DEBUG_VERBOSE("time before last time %d ms return last status: %d\n",
            status_cache_ms, indirect_dev->dev_status);
        return sprintf(buf, "%u\n", indirect_dev->dev_status);
    }

    indirect_dev->last_jiffies = jiffies;

    len = indirect_dev->data_bus_width;
    if (len > WIDTH_4Byte) {
        DEBUG_ERROR("status show rw len:%u beyond max 4 byte.\n", len);
        return -EINVAL;
    }

    mutex_lock(&indirect_dev->update_lock);

    for (i = 0; i < len; i++) {
        /* reverse to ensure that different data is read and written each time the verification is performed.*/
        indirect_dev->test_data[i] = ~indirect_dev->test_data[i];
    }
    for (i = 0; i < indirect_dev->test_reg_num; i++) {
        offset = indirect_dev->test_reg[i];
        ret = rw_status_check(indirect_dev, offset, len, type);
        if (ret < 0) {
            indirect_dev->dev_status = LOGIC_DEV_STATUS_NOT_OK;
            DEBUG_ERROR("STATUS_NOT_OK result check all retry failed.\n");
            mutex_unlock(&indirect_dev->update_lock);
            return sprintf(buf, "%u\n", indirect_dev->dev_status);
        }
    }

    indirect_dev->dev_status = LOGIC_DEV_STATUS_OK;
    mutex_unlock(&indirect_dev->update_lock);
    return sprintf(buf, "%u\n", indirect_dev->dev_status);
}

static ssize_t file_cache_rd_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_indirect_dev_t *indirect_dev = container_of(kobj, wb_indirect_dev_t, kobj);

    if (!indirect_dev) {
        DEBUG_ERROR("file_cache_rd_show indirect dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%u\n", indirect_dev->file_cache_rd);
}

static ssize_t file_cache_rd_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_indirect_dev_t *indirect_dev = container_of(kobj, wb_indirect_dev_t, kobj);
    u8 val;
    int ret;

    if (!indirect_dev) {
        DEBUG_ERROR("file_cache_rd_store indirect dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    indirect_dev->file_cache_rd = val;

    return count;
}

static ssize_t file_cache_wr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_indirect_dev_t *indirect_dev = container_of(kobj, wb_indirect_dev_t, kobj);

    if (!indirect_dev) {
        DEBUG_ERROR("file_cache_wr_show indirect dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", indirect_dev->file_cache_wr);
}

static ssize_t file_cache_wr_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_indirect_dev_t *indirect_dev = container_of(kobj, wb_indirect_dev_t, kobj);
    u8 val;
    int ret;

    if (!indirect_dev) {
        DEBUG_ERROR("file_cache_wr_store indirect dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    indirect_dev->file_cache_wr = val;

    return count;
}

static ssize_t cache_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_indirect_dev_t *indirect_dev = container_of(kobj, wb_indirect_dev_t, kobj);

    if (!indirect_dev) {
        DEBUG_ERROR("cache_file_path_show indirect dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", indirect_dev->cache_file_path);
}

static ssize_t mask_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_indirect_dev_t *indirect_dev = container_of(kobj, wb_indirect_dev_t, kobj);

    if (!indirect_dev) {
        DEBUG_ERROR("mask_file_path_show indirect dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", indirect_dev->mask_file_path);
}

static struct kobj_attribute alias_attribute = __ATTR(alias, S_IRUGO, alias_show, NULL);
static struct kobj_attribute type_attribute = __ATTR(type, S_IRUGO, type_show, NULL);
static struct kobj_attribute info_attribute = __ATTR(info, S_IRUGO, info_show, NULL);
static struct kobj_attribute status_attribute = __ATTR(status, S_IRUGO, status_show, NULL);
static struct kobj_attribute file_cache_rd_attribute = __ATTR(file_cache_rd, S_IRUGO  | S_IWUSR, file_cache_rd_show, file_cache_rd_store);
static struct kobj_attribute file_cache_wr_attribute = __ATTR(file_cache_wr, S_IRUGO  | S_IWUSR, file_cache_wr_show, file_cache_wr_store);
static struct kobj_attribute cache_file_path_attribute = __ATTR(cache_file_path, S_IRUGO, cache_file_path_show, NULL);
static struct kobj_attribute mask_file_path_attribute = __ATTR(mask_file_path, S_IRUGO, mask_file_path_show, NULL);

static struct attribute *indirect_dev_attrs[] = {
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

static struct attribute_group indirect_dev_attr_group = {
    .attrs = indirect_dev_attrs,
};

static int of_indirect_dev_config_init(struct platform_device *pdev, struct indirect_dev_info *indirect_dev)
{
    int i, ret;
    uint32_t type;

    ret = 0;
    ret += of_property_read_string(pdev->dev.of_node, "dev_name", &indirect_dev->name);
    ret += of_property_read_string(pdev->dev.of_node, "logic_dev_name", &indirect_dev->logic_dev_name);
    ret += of_property_read_u32(pdev->dev.of_node, "addr_low", &indirect_dev->addr_low);
    ret += of_property_read_u32(pdev->dev.of_node, "data_bus_width", &indirect_dev->data_bus_width);
    ret += of_property_read_u32(pdev->dev.of_node, "addr_high", &indirect_dev->addr_high);
    ret += of_property_read_u32(pdev->dev.of_node, "wr_data", &indirect_dev->wr_data);
    ret += of_property_read_u32(pdev->dev.of_node, "rd_data", &indirect_dev->rd_data);
    ret += of_property_read_u32(pdev->dev.of_node, "opt_ctl", &indirect_dev->opt_ctl);
    ret += of_property_read_u32(pdev->dev.of_node, "indirect_len", &indirect_dev->indirect_len);
    ret += of_property_read_u32(pdev->dev.of_node, "logic_func_mode", &indirect_dev->logic_func_mode);
    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to get dts config, ret:%d.\n", ret);
        return -ENXIO;
    }

    if (of_property_read_string(pdev->dev.of_node, "dev_alias", &indirect_dev->alias)) {
        indirect_dev->alias = indirect_dev->name;
    }

    if (indirect_dev->data_bus_width == 0) {
        dev_err(&pdev->dev, "Invalid data_bus_width: %u\n", indirect_dev->data_bus_width);
        return -EINVAL;
    }

    if (of_property_read_u32(pdev->dev.of_node, "lock_mode", &indirect_dev->lock_mode)) {
        /* lock_mode can not set, default use spin lock */
        indirect_dev->lock_mode = WB_DEV_SPIN_LOCK_MODE;
    }

    if ((indirect_dev->lock_mode != WB_DEV_SPIN_LOCK_MODE)
        && (indirect_dev->lock_mode != WB_DEV_MUTEX_MODE)) {
        dev_err(&pdev->dev, "Invalid lock_mode: %u\n", indirect_dev->lock_mode);
        return -EINVAL;
    }

    ret = of_property_read_u32(pdev->dev.of_node, "status_check_type", &indirect_dev->status_check_type);
    if (ret == 0) {
        type = indirect_dev->status_check_type;
        if (type != READ_BACK_CHECK && type != READ_BACK_NAGATIVE_CHECK) {
            dev_err(&pdev->dev, "Invalid status_check_type: %u\n", type);
            return -EINVAL;
        }
        DEBUG_VERBOSE("status_check_type: %d, \n", indirect_dev->status_check_type);

        ret = of_property_read_u32(pdev->dev.of_node, "test_reg_num", &indirect_dev->test_reg_num);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get test_reg_num config, ret: %d\n", ret);
            return -ENXIO;
        }
        if (indirect_dev->test_reg_num == 0 || indirect_dev->test_reg_num > TEST_REG_MAX_NUM) {
            dev_err(&pdev->dev, "Invalid test_reg_num: %u\n", indirect_dev->test_reg_num);
            return -EINVAL;
        }

        ret = of_property_read_u32_array(pdev->dev.of_node, "test_reg", indirect_dev->test_reg, indirect_dev->test_reg_num);
        if(ret != 0) {
            dev_err(&pdev->dev, "Failed to get test_reg config, ret: %d\n", ret);
            return -ENXIO;
        }

        indirect_dev->last_jiffies = jiffies;
        indirect_dev->dev_status = LOGIC_DEV_STATUS_OK;

        for (i = 0; i < WIDTH_4Byte; i++) {
            indirect_dev->test_data[i] = INIT_TEST_DATA;
        }

        for (i = 0; i < indirect_dev->test_reg_num; i++) {
            DEBUG_VERBOSE("test_reg[%d] = 0x%x.\n", i, indirect_dev->test_reg[i]);
        }
    } else {
        indirect_dev->status_check_type = NOT_SUPPORT_CHECK_STATUS;
        DEBUG_VERBOSE("not support dev status check sysfs.\n");
    }

    ret = of_property_read_u32(pdev->dev.of_node, "log_num", &(indirect_dev->log_node.log_num));
    if (ret == 0) {
        if ((indirect_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) || (indirect_dev->log_node.log_num == 0)) {
            dev_err(&pdev->dev, "Invalid log_num: %u\n", indirect_dev->log_node.log_num);
            return -EINVAL;
        }
        ret = of_property_read_u32_array(pdev->dev.of_node, "log_index", indirect_dev->log_node.log_index, indirect_dev->log_node.log_num);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get log_index config, ret: %d\n", ret);
            return -EINVAL;
        }
        for (i = 0; i < indirect_dev->log_node.log_num; i++) {
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, indirect_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
        indirect_dev->log_node.log_num = 0;
    }

    DEBUG_VERBOSE("dev_name: %s, dev_alias: %s, logic_dev_name: %s, indirect_len: 0x%x, data_bus_width: %u, logic_func_mode: %d\n",
        indirect_dev->name, indirect_dev->alias, indirect_dev->logic_dev_name, indirect_dev->indirect_len,
        indirect_dev->data_bus_width, indirect_dev->logic_func_mode);
    DEBUG_VERBOSE("wr_data: 0x%x, rd_data: 0x%x, addr_low: 0x%x, addr_high: 0x%x, opt_ctl: 0x%x, lock_mode: %u\n",
        indirect_dev->wr_data, indirect_dev->rd_data, indirect_dev->addr_low,
        indirect_dev->addr_high, indirect_dev->opt_ctl, indirect_dev->lock_mode);
    return 0;
}

static int indirect_dev_config_init(struct platform_device *pdev, struct indirect_dev_info *indirect_dev)
{
    int i;
    uint32_t type;
    indirect_dev_device_t *indirect_dev_device;

    if (pdev->dev.platform_data == NULL) {
        dev_err(&pdev->dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }

    indirect_dev_device = pdev->dev.platform_data;
    indirect_dev->name = indirect_dev_device->dev_name;
    indirect_dev->logic_dev_name = indirect_dev_device->logic_dev_name;
    indirect_dev->data_bus_width = indirect_dev_device->data_bus_width;
    indirect_dev->wr_data = indirect_dev_device->wr_data;
    indirect_dev->addr_low = indirect_dev_device->addr_low;
    indirect_dev->addr_high = indirect_dev_device->addr_high;
    indirect_dev->rd_data = indirect_dev_device->rd_data;
    indirect_dev->opt_ctl = indirect_dev_device->opt_ctl;
    indirect_dev->indirect_len = indirect_dev_device->indirect_len;
    indirect_dev->logic_func_mode = indirect_dev_device->logic_func_mode;
    indirect_dev->lock_mode = indirect_dev_device->lock_mode;
    if (strlen(indirect_dev_device->dev_alias) == 0) {
        indirect_dev->alias = indirect_dev->name;
    } else {
        indirect_dev->alias = indirect_dev_device->dev_alias;
    }

    if (indirect_dev->data_bus_width == 0) {
        dev_err(&pdev->dev, "Invalid data_bus_width: %u\n", indirect_dev->data_bus_width);
        return -EINVAL;
    }

    if (indirect_dev->lock_mode == 0) {
        /* lock_mode can not set, default use spin lock */
        indirect_dev->lock_mode = WB_DEV_SPIN_LOCK_MODE;
    }

    if ((indirect_dev->lock_mode != WB_DEV_SPIN_LOCK_MODE)
        && (indirect_dev->lock_mode != WB_DEV_MUTEX_MODE)) {
        dev_err(&pdev->dev, "Invalid lock_mode: %u\n", indirect_dev->lock_mode);
        return -EINVAL;
    }

    indirect_dev->status_check_type = indirect_dev_device->status_check_type;
    if (indirect_dev->status_check_type != NOT_SUPPORT_CHECK_STATUS) {
        type = indirect_dev->status_check_type;
        if ((type != READ_BACK_CHECK) && (type != READ_BACK_NAGATIVE_CHECK)) {
            dev_err(&pdev->dev, "Invalid status_check_type: %u\n", type);
            return -EINVAL;
        }
        DEBUG_VERBOSE("status_check_type: %d\n", indirect_dev->status_check_type);

        indirect_dev->test_reg_num = indirect_dev_device->test_reg_num;
        if ((indirect_dev->test_reg_num == 0) || (indirect_dev->test_reg_num > TEST_REG_MAX_NUM)) {
            dev_err(&pdev->dev, "Invalid test_reg_num: %u\n", indirect_dev->test_reg_num);
            return -EINVAL;
        }
        for (i = 0; i < indirect_dev->test_reg_num; i++) {
            indirect_dev->test_reg[i] = indirect_dev_device->test_reg[i];
            DEBUG_VERBOSE("test_reg[%d] address = 0x%x.\n", i, indirect_dev->test_reg[i]);
        }

        indirect_dev->last_jiffies = jiffies;
        indirect_dev->dev_status = LOGIC_DEV_STATUS_OK;
        for (i = 0; i < WIDTH_4Byte; i++) {
            indirect_dev->test_data[i] = INIT_TEST_DATA;
        }
    } else {
        DEBUG_VERBOSE("not support dev status check sysfs.\n");
    }

    indirect_dev->log_node.log_num = indirect_dev_device->log_num;
    if (indirect_dev->log_node.log_num != 0) {
        if (indirect_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) {
            dev_err(&pdev->dev, "Invalid log_num: %u\n", indirect_dev->log_node.log_num);
            return -EINVAL;
        }
        for (i = 0; i < indirect_dev->log_node.log_num; i++) {
            indirect_dev->log_node.log_index[i] = indirect_dev_device->log_index[i];
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, indirect_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
    }

    DEBUG_VERBOSE("dev_name: %s, dev_alias: %s, logic_dev_name: %s, indirect_len: 0x%x, data_bus_width: %u, logic_func_mode: %d\n",
        indirect_dev->name, indirect_dev->alias, indirect_dev->logic_dev_name, indirect_dev->indirect_len,
        indirect_dev->data_bus_width, indirect_dev->logic_func_mode);
    DEBUG_VERBOSE("wr_data: 0x%x, rd_data: 0x%x, addr_low: 0x%x, addr_high: 0x%x, opt_ctl: 0x%x, lock_mode: %u\n",
        indirect_dev->wr_data, indirect_dev->rd_data, indirect_dev->addr_low,
        indirect_dev->addr_high, indirect_dev->opt_ctl, indirect_dev->lock_mode);
    return 0;
}

static int wb_indirect_dev_probe(struct platform_device *pdev)
{
    int ret;
    struct indirect_dev_info *indirect_dev;
    struct miscdevice *misc;

    DEBUG_VERBOSE("wb_indirect_dev_probe\n");

    indirect_dev = devm_kzalloc(&pdev->dev, sizeof(struct indirect_dev_info), GFP_KERNEL);
    if (!indirect_dev) {
        dev_err(&pdev->dev, "devm_kzalloc error.\n");
        return -ENOMEM;
    }

    if (pdev->dev.of_node) {
        ret = of_indirect_dev_config_init(pdev, indirect_dev);
    } else {
        ret = indirect_dev_config_init(pdev, indirect_dev);
    }

    if (ret < 0) {
        return ret;
    }

    ret = find_intf_addr(&indirect_dev->write_intf_addr, &indirect_dev->read_intf_addr, indirect_dev->logic_func_mode);
    if (ret) {
        dev_err(&pdev->dev, "Failed to find_intf_addr func mode %u, ret: %d\n", indirect_dev->logic_func_mode, ret);
        return ret;
    }

    if (!indirect_dev->write_intf_addr || !indirect_dev->read_intf_addr) {
        dev_err(&pdev->dev, "Fail: func mode %u rw symbol undefined\n", indirect_dev->logic_func_mode);
        return -ENOSYS;
    }

    mutex_init(&indirect_dev->update_lock);
    mutex_init(&indirect_dev->log_node.file_lock);

    indirect_dev->file_cache_rd = 0;
    indirect_dev->file_cache_wr = 0;
    snprintf(indirect_dev->cache_file_path, sizeof(indirect_dev->cache_file_path), CACHE_FILE_PATH, indirect_dev->name);
    snprintf(indirect_dev->mask_file_path, sizeof(indirect_dev->mask_file_path), MASK_FILE_PATH, indirect_dev->name);

    /* creat parent dir by dev name in /sys/logic_dev */
    ret = kobject_init_and_add(&indirect_dev->kobj, &indirect_dev_ktype, logic_dev_kobj, "%s", indirect_dev->name);
    if (ret) {
        kobject_put(&indirect_dev->kobj);
        dev_err(&pdev->dev, "Failed to creat parent dir: %s, ret: %d\n", indirect_dev->name, ret);
        return ret;
    }

    indirect_dev->sysfs_group = &indirect_dev_attr_group;
    ret = sysfs_create_group(&indirect_dev->kobj, indirect_dev->sysfs_group);
    if (ret) {
        dev_err(&pdev->dev, "Failed to create %s sysfs group, ret: %d\n", indirect_dev->name, ret);
        goto remove_parent_kobj;
    }

    platform_set_drvdata(pdev, indirect_dev);
    misc = &indirect_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = indirect_dev->name;
    misc->fops = &indirect_dev_fops;
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

    wb_dev_lock_init(indirect_dev);
    indirect_dev_arry[misc->minor] = indirect_dev;

    dev_info(&pdev->dev, "Register indirect device %s success, logic_dev_name: %s, indirect_len: 0x%x, data_bus_width: %u, logic_func_mode: %u, lock_mode: %u\n",
        indirect_dev->name, indirect_dev->logic_dev_name, indirect_dev->indirect_len,
        indirect_dev->data_bus_width, indirect_dev->logic_func_mode, indirect_dev->lock_mode);
    return 0;
deregister_misc:
    misc_deregister(misc);
remove_sysfs_group:
    sysfs_remove_group(&indirect_dev->kobj, (const struct attribute_group *)indirect_dev->sysfs_group);
remove_parent_kobj:
    kobject_put(&indirect_dev->kobj);
    return ret;
}

static int wb_indirect_dev_remove(struct platform_device *pdev)
{
    int minor;
    wb_indirect_dev_t *indirect_dev;

    indirect_dev = platform_get_drvdata(pdev);
    minor = indirect_dev->misc.minor;
    if (minor < MAX_DEV_NUM && (indirect_dev_arry[minor] != NULL)) {
        dev_dbg(&pdev->dev, "misc_deregister %s, minor: %d\n", indirect_dev->misc.name, minor);
        misc_deregister(&indirect_dev_arry[minor]->misc);
        indirect_dev_arry[minor] = NULL;
    }

    if (indirect_dev->sysfs_group) {
        dev_dbg(&pdev->dev, "Unregister %s indirect_dev sysfs group\n", indirect_dev->name);
        sysfs_remove_group(&indirect_dev->kobj, (const struct attribute_group *)indirect_dev->sysfs_group);
        kobject_put(&indirect_dev->kobj);
    }

    dev_info(&pdev->dev, "Remove %s indirect device success.\n", indirect_dev->name);
    platform_set_drvdata(pdev, NULL);
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
