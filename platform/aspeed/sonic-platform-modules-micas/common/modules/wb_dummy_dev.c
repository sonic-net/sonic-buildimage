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

#include "wb_dummy_dev.h"
#include <wb_bsp_kernel_debug.h>
#include <wb_kernel_io.h>

#define MODULE_NAME                "wb-dummy-dev"

/* Use the wb_bsp_kernel_debug header file must define debug variable */
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int status_cache_ms = 0;
module_param(status_cache_ms, int, S_IRUGO | S_IWUSR);

static DEFINE_SPINLOCK(dev_array_lock);
static struct dummy_dev_info* dummy_dev_arry[MAX_DEV_NUM];

typedef struct dummy_dev_info {
    const char *name;               /* generate dev name */
    const char *alias;              /* generate dev alias */
    const char *logic_dev_name;     /* dependent dev name */
    uint32_t dummy_len;          /* dev data len */
    uint32_t data_bus_width;        /* dev data_bus_width */
    uint32_t offset;
    uint32_t logic_func_mode;       /* 1: i2c, 2: file, 3:pcie, 4:io, 5:spi */
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
    struct miscdevice misc;
    struct kobject kobj;
    struct attribute_group *sysfs_group;
    uint8_t file_cache_rd;
    uint8_t file_cache_wr;
    char cache_file_path[MAX_NAME_SIZE];
    char mask_file_path[MAX_NAME_SIZE];
    struct mutex update_lock;
    wb_bsp_key_device_log_node_t log_node;
    device_status_check_t status_check;
} wb_dummy_dev_t;

static int wb_logic_reg_write(struct dummy_dev_info *dummy_dev, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_write pfunc;

    pfunc = (device_func_write)dummy_dev->write_intf_addr;
    return pfunc(dummy_dev->logic_dev_name, pos, val, size);
}

static int wb_logic_reg_read(struct dummy_dev_info *dummy_dev, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    pfunc = (device_func_read)dummy_dev->read_intf_addr;
    return pfunc(dummy_dev->logic_dev_name, pos, val, size);
}


static int device_read(struct dummy_dev_info *dummy_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int ret;
    u8 val[MAX_RW_LEN + WIDTH_4Byte];
    u32 data_width;
    u32 fix_offset, fix_count;
    u32 tmp_offset;
    u32 tmp, rd_len;
    u32 read_count;

    if (offset >= dummy_dev->dummy_len) {
        DEBUG_VERBOSE("offset: 0x%x, dummy len: 0x%x, count: %zu, EOF.\n",
            offset, dummy_dev->dummy_len, count);
        return 0;
    }

    data_width = dummy_dev->data_bus_width;
    tmp_offset = offset % data_width;
    if (tmp_offset) {
        /* Read from aligned address first, then drop the head bytes. */
        fix_offset = offset - tmp_offset;
        fix_count = count + tmp_offset;
    } else {
        fix_offset = offset;
        fix_count = count;
    }

    DEBUG_VERBOSE("dummy bus width:%u, offset:0x%x, count %zu.\n", data_width, offset, count);
    DEBUG_VERBOSE("dummy bus tmp_offset:0x%x, fix_offset:0x%x, fix_count %u.\n",
        tmp_offset, fix_offset, fix_count);

    if (fix_count > dummy_dev->dummy_len - fix_offset) {
        DEBUG_VERBOSE("read count out of range. fix_count:%u, read len:%u.\n",
            fix_count, dummy_dev->dummy_len - fix_offset);
        fix_count = dummy_dev->dummy_len - fix_offset;
    }

    tmp = (data_width - 1) & fix_count;
    rd_len = (tmp == 0) ? fix_count : fix_count + data_width - tmp;

    if (rd_len > sizeof(val)) {
        DEBUG_ERROR("read len is too long, rd_len:%u, val len:%zu.\n", rd_len, sizeof(val));
        return -EINVAL;
    }

    mem_clear(val, sizeof(val));
    ret = wb_logic_reg_read(dummy_dev, fix_offset + dummy_dev->offset, val, rd_len);
    if (ret < 0) {
        DEBUG_ERROR("read error.read offset = 0x%x\n", fix_offset);
        return -EFAULT;
    }

    if (fix_count <= tmp_offset) {
        return 0;
    }

    read_count = fix_count - tmp_offset;
    memcpy(buf, val + tmp_offset, read_count);

    if (dummy_dev->file_cache_rd) {
        ret = cache_value_read(dummy_dev->mask_file_path, dummy_dev->cache_file_path, offset, buf, read_count);
        if (ret < 0) {
            DEBUG_ERROR("dummy_dev data offset: 0x%x, read_len: %zu, read cache file fail, ret: %d, return act value\n",
                offset, (size_t)read_count, ret);
        } else {
            DEBUG_VERBOSE("dummy_dev data offset: 0x%x, read_len: %zu success, read from cache value\n",
                offset, (size_t)read_count);
        }
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(dummy_dev->name, offset, buf, read_count, true);
    }

    return read_count;
}

static int device_write(struct dummy_dev_info *dummy_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int ret;
    u32 data_width;

    if (offset >= dummy_dev->dummy_len) {
        DEBUG_VERBOSE("offset: 0x%x, dummy len: 0x%x, count: %zu, EOF.\n",
            offset, dummy_dev->dummy_len, count);
        return 0;
    }

    data_width = dummy_dev->data_bus_width;
    if (offset % data_width) {
        DEBUG_ERROR("data bus width:%d, offset:0x%x, read size %zu invalid.\n",
            data_width, offset, count);
        return -EINVAL;
    }

    if (count > (dummy_dev->dummy_len - offset)) {
        DEBUG_VERBOSE("write count out of range. input len:%zu, read len:%u.\n",
            count, dummy_dev->dummy_len - offset);
        count = dummy_dev->dummy_len - offset;
    }

    ret = wb_logic_reg_write(dummy_dev, offset + dummy_dev->offset, buf, count);
    if (ret < 0) {
        DEBUG_ERROR("write error.write offset = %u\n", offset);
        return -EFAULT;
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(dummy_dev->name, offset, buf, count, false);
    }

    return count;
}

static ssize_t dummy_dev_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int ret, read_len;
    struct dummy_dev_info *dummy_dev;

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, read failed.\n");
        return -EINVAL;
    }

    dummy_dev = file->private_data;
    if (dummy_dev == NULL) {
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
    read_len = device_read(dummy_dev, (uint32_t)*offset, val, count);
    if (read_len < 0) {
        DEBUG_ERROR("dummy dev read failed, dev name:%s, offset:0x%x, len:%zu.\n",
            dummy_dev->name, (uint32_t)*offset, count);
        return read_len;
    }

    if (read_len == 0) {
        DEBUG_VERBOSE("dummy dev read EOF, offset: 0x%llx, count: %zu\n", *offset, count);
        return 0;
    }

    DEBUG_VERBOSE("read, buf: %p, offset: %lld, read count %zu.\n", buf, *offset, count);
    memcpy(buf, val, read_len);

    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t dummy_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    DEBUG_VERBOSE("dummy_dev_read_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(to), iocb->ki_pos);
    return wb_iov_iter_read(iocb, to, dummy_dev_read);
}

static ssize_t dummy_dev_write(struct file *file, char *buf,
                   size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int write_len;
    struct dummy_dev_info *dummy_dev;
    char bsp_log_dev_name[BSP_LOG_DEV_NAME_MAX_LEN];
    char bsp_log_file_path[BSP_LOG_DEV_NAME_MAX_LEN];

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, read failed.\n");
        return -EINVAL;
    }

    dummy_dev = file->private_data;
    if (dummy_dev == NULL) {
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

    if (dummy_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Devfs]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_file_path), "%s.%s_bsp_key_reg", BSP_LOG_DIR, dummy_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(dummy_dev->log_node), (uint32_t)*offset, val, count);
    }

    write_len = device_write(dummy_dev, (uint32_t)*offset, val, count);
    if (write_len < 0) {
        DEBUG_ERROR("dummy dev write failed, dev name:%s, offset:0x%llx, len:%zu.\n",
            dummy_dev->name, *offset, count);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t dummy_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    DEBUG_VERBOSE("dummy_dev_write_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(from), iocb->ki_pos);
    return wb_iov_iter_write(iocb, from, dummy_dev_write);
}

static loff_t dummy_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    struct dummy_dev_info *dummy_dev;

    dummy_dev = file->private_data;
    if (dummy_dev == NULL) {
        DEBUG_ERROR("dummy_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            DEBUG_ERROR("SEEK_SET, offset:%lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > dummy_dev->dummy_len) {
            DEBUG_ERROR("SEEK_SET out of range, offset:%lld, i2c_len:0x%x.\n",
                offset, dummy_dev->dummy_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > dummy_dev->dummy_len) || ((file->f_pos + offset) < 0)) {
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

static long dummy_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int minor_to_dev(int minor, struct dummy_dev_info **dummy_dev)
{
    int i;

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (dummy_dev_arry[i] == NULL) {
            continue;
        }
        if (dummy_dev_arry[i]->misc.minor == minor) {
            *dummy_dev = dummy_dev_arry[i];
            return 0;
        }
    }
    return -ENODEV;
}

static int add_dev_to_g_dev_list(struct dummy_dev_info *dummy_dev)
{
    int i;
    unsigned long flags;

    spin_lock_irqsave(&dev_array_lock, flags);
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (dummy_dev_arry[i] == NULL) {
            dummy_dev_arry[i] = dummy_dev;
            spin_unlock_irqrestore(&dev_array_lock, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&dev_array_lock, flags);
    return -EBUSY;
}

static int remove_dev_from_g_dev_list(int minor)
{
    int i;
    unsigned long flags;

    spin_lock_irqsave(&dev_array_lock, flags);
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (dummy_dev_arry[i] == NULL) {
            continue;
        }
        if (dummy_dev_arry[i]->misc.minor == minor) {
            dummy_dev_arry[i] = NULL;
            spin_unlock_irqrestore(&dev_array_lock, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&dev_array_lock, flags);
    return -ENODEV ;
}

static int dummy_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    struct dummy_dev_info *dummy_dev;
    int ret;

    ret = minor_to_dev(minor, &dummy_dev);
    if (ret) {
        return ret;
    }
    file->private_data = dummy_dev;
    return 0;
}

static int dummy_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;

    return 0;
}

static const struct file_operations dummy_dev_fops = {
    .owner          = THIS_MODULE,
    .llseek         = dummy_dev_llseek,
    .read_iter      = dummy_dev_read_iter,
    .write_iter     = dummy_dev_write_iter,
    .unlocked_ioctl = dummy_dev_ioctl,
    .open           = dummy_dev_open,
    .release        = dummy_dev_release,
};

static struct dummy_dev_info *dev_match(const char *path)
{
    struct dummy_dev_info *dummy_dev;
    char dev_name[MAX_NAME_SIZE];
    int i;

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (dummy_dev_arry[i] == NULL) {
            continue;
        }
        dummy_dev = dummy_dev_arry[i];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", dummy_dev->name);
        if (!strcmp(path, dev_name)) {
            DEBUG_VERBOSE("get dev_name = %s, minor = %d\n", dev_name, i);
            return dummy_dev;
        }
    }

    return NULL;
}

int dummy_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct dummy_dev_info *dummy_dev;
    int read_len;

    if (path == NULL) {
        DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    dummy_dev = dev_match(path);
    if (dummy_dev == NULL) {
        DEBUG_ERROR("dummy_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    read_len = device_read(dummy_dev, offset, buf, count);
    if (read_len < 0) {
        DEBUG_ERROR("dummy_dev_read_tmp failed, ret:%d.\n", read_len);
    }
    return read_len;
}
EXPORT_SYMBOL(dummy_device_func_read);

int dummy_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct dummy_dev_info *dummy_dev;
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

    dummy_dev = dev_match(path);
    if (dummy_dev == NULL) {
        DEBUG_ERROR("dummy_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    if (dummy_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Symbol]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_file_path), "%s.%s_bsp_key_reg", BSP_LOG_DIR, dummy_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(dummy_dev->log_node), offset, buf, count);
    }

    write_len = device_write(dummy_dev, offset, buf, count);
    if (write_len < 0) {
        DEBUG_ERROR("dummy_dev_write_tmp failed, ret:%d.\n", write_len);
    }
    return write_len;
}
EXPORT_SYMBOL(dummy_device_func_write);

static ssize_t dummy_dev_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->show) {
        DEBUG_ERROR("dummy dev attr show is null.\n");
        return -ENOSYS;
    }

    return attribute->show(kobj, attribute, buf);
}

static ssize_t dummy_dev_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf,
                   size_t len)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->store) {
        DEBUG_ERROR("dummy dev attr store is null.\n");
        return -ENOSYS;
    }

    return attribute->store(kobj, attribute, buf, len);
}

static const struct sysfs_ops dummy_dev_sysfs_ops = {
    .show = dummy_dev_attr_show,
    .store = dummy_dev_attr_store,
};

static void dummy_dev_obj_release(struct kobject *kobj)
{
    return;
}

static struct kobj_type dummy_dev_ktype = {
    .sysfs_ops = &dummy_dev_sysfs_ops,
    .release = dummy_dev_obj_release,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    .default_attrs = NULL,
#endif
};

static ssize_t alias_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_dummy_dev_t *dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);

    if (!dummy_dev) {
        DEBUG_ERROR("alias show dummy dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", dummy_dev->alias);
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

    wb_dummy_dev_t *dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);
    if (!dummy_dev) {
        DEBUG_ERROR("info show alias_show dummy dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    offset = 0;
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "name: %s\n", dummy_dev->name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "alias: %s\n", dummy_dev->alias);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "dependent logic_dev_name: %s\n", dummy_dev->logic_dev_name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "dummy_len: 0x%x\n", dummy_dev->dummy_len);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "offset: 0x%x\n", dummy_dev->offset);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "logic_func_mode: %u\n", dummy_dev->logic_func_mode);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "data_bus_width: %u\n", dummy_dev->data_bus_width);
    buf_len = strlen(buf);
    return buf_len;
}

static ssize_t status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_dummy_dev_t *dummy_dev;
    uint32_t type, len;
    int ret;

    dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);
    if (!dummy_dev) {
        DEBUG_ERROR("Failed: status show param is NULL.\n");
        return -ENODEV;
    }

    type = dummy_dev->status_check.status_check_type_bmp;
    /* check logic dev support type */
    if (wb_logic_status_type_get_number(type) == 0) {
        DEBUG_ERROR("unsupport dev status check.\n");
        return -EOPNOTSUPP;
    }

    if (time_before(jiffies, dummy_dev->status_check.last_jiffies + msecs_to_jiffies(status_cache_ms))) {
        /* Within the time range of status_cache_ms, directly return the last result */
        DEBUG_VERBOSE("time before last time %d ms return last status: %d\n",
            status_cache_ms, dummy_dev->status_check.dev_status);
        return sprintf(buf, "%u\n", dummy_dev->status_check.dev_status);
    }

    dummy_dev->status_check.last_jiffies = jiffies;

    len = dummy_dev->data_bus_width;
    if (len > MAX_DATA_WIDTH) {
        DEBUG_ERROR("status show rw len:%u beyond max byte.\n", len);
        return -EINVAL;
    }

    mutex_lock(&dummy_dev->update_lock);
    ret = wb_logic_dev_get_status(&dummy_dev->status_check, len, buf, PAGE_SIZE);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_dev_get_status fail. (ret %d)\n", ret);
    }
    mutex_unlock(&dummy_dev->update_lock);

    return ret;
}

static ssize_t status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_dummy_dev_t *dummy_dev;
    uint32_t val;
    int ret;
    uint32_t len;

    dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);
    if (!dummy_dev) {
        DEBUG_ERROR("status_store param is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou32(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }

    len = dummy_dev->data_bus_width;
    if (len > MAX_DATA_WIDTH) {
        DEBUG_ERROR("status store rw len:%u beyond max byte.\n", len);
        return -EINVAL;
    }

    DEBUG_INFO("status store len:%u val:0x%x.\n", len, val);

    mutex_lock(&dummy_dev->update_lock);
    ret = wb_logic_clear_status(&dummy_dev->status_check, len, val);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_clear_status fail. (ret %d)\n", ret);
    } else {
        ret = count;
    }
    mutex_unlock(&dummy_dev->update_lock);

    return ret;
}

static ssize_t status_show_with_type(struct kobject *kobj, struct kobj_attribute *attr, char *buf, uint32_t type)
{
    wb_dummy_dev_t *dummy_dev;
    uint32_t len;
    int ret;

    dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);
    if (!dummy_dev) {
        DEBUG_ERROR("Failed: status show param is NULL.\n");
        return -ENODEV;
    }

    /* Input parameter detection, only one type */
    if (wb_logic_status_type_get_number(type) != 1) {
        DEBUG_ERROR("unsupport dev status check. type 0x%x\n", type);
        return -EOPNOTSUPP;
    }

    ret = wb_logic_get_cache_status_with_type(&dummy_dev->status_check, type, status_cache_ms, buf, PAGE_SIZE);
    if (ret > 0) {
        /* use cache status */
        DEBUG_VERBOSE("use cache status return %d, type %d\n", ret, type);
        return ret;
    } else if (ret == 0) {
        /* do nothing */
        DEBUG_VERBOSE("not use cache status\n");
    } else {
        /* get cache status fail */
        DEBUG_VERBOSE("wb_logic_get_cache_status_with_type fail, ret %d, type %d\n", ret, type);
        return ret;
    }

    len = dummy_dev->data_bus_width;
    if (len > MAX_DATA_WIDTH) {
        DEBUG_ERROR("status show rw len:%u beyond max 4 byte.\n", len);
        return -EINVAL;
    }

    mutex_lock(&dummy_dev->update_lock);
    ret = wb_logic_dev_get_status_with_type(&dummy_dev->status_check, len, buf, PAGE_SIZE, type);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_dev_get_status fail. (ret %d)\n", ret);
    }
    mutex_unlock(&dummy_dev->update_lock);

    return ret;
}

static ssize_t status_seu_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return status_show_with_type(kobj, attr, buf, STATUS_CHECK_SEU);
}

static ssize_t status_selftest_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return status_show_with_type(kobj, attr, buf, STATUS_CHECK_SELFTEST);
}

static ssize_t status_scratch_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return status_show_with_type(kobj, attr, buf, STATUS_CHECK_SCRATCH);
}

static ssize_t status_cram_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return status_show_with_type(kobj, attr, buf, STATUS_CHECK_CRAM);
}

static ssize_t file_cache_rd_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_dummy_dev_t *dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);

    if (!dummy_dev) {
        DEBUG_ERROR("file_cache_rd_show dummy dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%u\n", dummy_dev->file_cache_rd);
}

static ssize_t file_cache_rd_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_dummy_dev_t *dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);
    u8 val;
    int ret;

    if (!dummy_dev) {
        DEBUG_ERROR("file_cache_rd_store dummy dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    dummy_dev->file_cache_rd = val;

    return count;
}

static ssize_t file_cache_wr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_dummy_dev_t *dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);

    if (!dummy_dev) {
        DEBUG_ERROR("file_cache_wr_show dummy dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", dummy_dev->file_cache_wr);
}

static ssize_t file_cache_wr_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_dummy_dev_t *dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);
    u8 val;
    int ret;

    if (!dummy_dev) {
        DEBUG_ERROR("file_cache_wr_store dummy dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    dummy_dev->file_cache_wr = val;

    return count;
}

static ssize_t cache_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_dummy_dev_t *dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);

    if (!dummy_dev) {
        DEBUG_ERROR("cache_file_path_show dummy dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", dummy_dev->cache_file_path);
}

static ssize_t mask_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_dummy_dev_t *dummy_dev = container_of(kobj, wb_dummy_dev_t, kobj);

    if (!dummy_dev) {
        DEBUG_ERROR("mask_file_path_show dummy dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", dummy_dev->mask_file_path);
}

static struct kobj_attribute alias_attribute = __ATTR(alias, S_IRUGO, alias_show, NULL);
static struct kobj_attribute type_attribute = __ATTR(type, S_IRUGO, type_show, NULL);
static struct kobj_attribute info_attribute = __ATTR(info, S_IRUGO, info_show, NULL);
static struct kobj_attribute status_attribute = __ATTR(status, S_IRUGO | S_IWUSR, status_show, status_store);
static struct kobj_attribute seu_status_attribute = __ATTR(seu_status, S_IRUGO, status_seu_show, NULL);
static struct kobj_attribute selftest_status_attribute = __ATTR(selftest_status, S_IRUGO, status_selftest_show, NULL);
static struct kobj_attribute scratch_status_attribute = __ATTR(scratch_status, S_IRUGO, status_scratch_show, NULL);
static struct kobj_attribute cram_status_attribute = __ATTR(cram_status, S_IRUGO, status_cram_show, NULL);
static struct kobj_attribute file_cache_rd_attribute = __ATTR(file_cache_rd, S_IRUGO  | S_IWUSR, file_cache_rd_show, file_cache_rd_store);
static struct kobj_attribute file_cache_wr_attribute = __ATTR(file_cache_wr, S_IRUGO  | S_IWUSR, file_cache_wr_show, file_cache_wr_store);
static struct kobj_attribute cache_file_path_attribute = __ATTR(cache_file_path, S_IRUGO, cache_file_path_show, NULL);
static struct kobj_attribute mask_file_path_attribute = __ATTR(mask_file_path, S_IRUGO, mask_file_path_show, NULL);

static struct attribute *dummy_dev_attrs[] = {
    &alias_attribute.attr,
    &type_attribute.attr,
    &info_attribute.attr,
    &status_attribute.attr,
    &file_cache_rd_attribute.attr,
    &file_cache_wr_attribute.attr,
    &cache_file_path_attribute.attr,
    &mask_file_path_attribute.attr,
    &seu_status_attribute.attr,
    &selftest_status_attribute.attr,
    &scratch_status_attribute.attr,
    &cram_status_attribute.attr,
    NULL,
};

static struct attribute_group dummy_dev_attr_group = {
    .attrs = dummy_dev_attrs,
};

static int of_dummy_dev_config_init(struct platform_device *pdev, struct dummy_dev_info *dummy_dev)
{
    int i, ret;

    ret = 0;
    ret += of_property_read_string(pdev->dev.of_node, "dev_name", &dummy_dev->name);
    ret += of_property_read_string(pdev->dev.of_node, "logic_dev_name", &dummy_dev->logic_dev_name);
    ret += of_property_read_u32(pdev->dev.of_node, "data_bus_width", &dummy_dev->data_bus_width);
    ret += of_property_read_u32(pdev->dev.of_node, "offset", &dummy_dev->offset);
    ret += of_property_read_u32(pdev->dev.of_node, "dummy_len", &dummy_dev->dummy_len);
    ret += of_property_read_u32(pdev->dev.of_node, "logic_func_mode", &dummy_dev->logic_func_mode);
    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to get dts config, ret:%d.\n", ret);
        return -ENXIO;
    }

    if (of_property_read_string(pdev->dev.of_node, "dev_alias", &dummy_dev->alias)) {
        dummy_dev->alias = dummy_dev->name;
    }

    if (dummy_dev->data_bus_width == 0 || dummy_dev->data_bus_width > WIDTH_4Byte) {
        dev_err(&pdev->dev, "Invalid data_bus_width: %u\n", dummy_dev->data_bus_width);
        return -EINVAL;
    }

    ret = of_dev_status_check_config_init(&pdev->dev, &dummy_dev->status_check, dummy_dev->data_bus_width,
                                            dummy_device_func_read, dummy_device_func_write, dummy_dev->name);
    if (ret != 0) {
        dev_err(&pdev->dev, "status_check_config_init fail: %d\n", ret);
        return -EINVAL;
    }

    ret = of_property_read_u32(pdev->dev.of_node, "log_num", &(dummy_dev->log_node.log_num));
    if (ret == 0) {
        if ((dummy_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) || (dummy_dev->log_node.log_num == 0)) {
            dev_err(&pdev->dev, "Invalid log_num: %u\n", dummy_dev->log_node.log_num);
            return -EINVAL;
        }
        ret = of_property_read_u32_array(pdev->dev.of_node, "log_index", dummy_dev->log_node.log_index, dummy_dev->log_node.log_num);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get log_index config, ret: %d\n", ret);
            return -EINVAL;
        }
        for (i = 0; i < dummy_dev->log_node.log_num; i++) {
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, dummy_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
        dummy_dev->log_node.log_num = 0;
    }

    DEBUG_VERBOSE("dev_name: %s, dev_alias: %s, logic_dev_name: %s, dummy_len: 0x%x, data_bus_width: %u, logic_func_mode: %d\n",
        dummy_dev->name, dummy_dev->alias, dummy_dev->logic_dev_name, dummy_dev->dummy_len,
        dummy_dev->data_bus_width, dummy_dev->logic_func_mode);
    DEBUG_VERBOSE("offset: 0x%x\n", dummy_dev->offset);
    return 0;
}

static int dummy_dev_config_init(struct platform_device *pdev, struct dummy_dev_info *dummy_dev)
{
    int i;
    dummy_dev_device_t *dummy_dev_device;
    int ret;

    if (pdev->dev.platform_data == NULL) {
        dev_err(&pdev->dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }

    dummy_dev_device = pdev->dev.platform_data;
    dummy_dev->name = dummy_dev_device->dev_name;
    dummy_dev->logic_dev_name = dummy_dev_device->logic_dev_name;
    dummy_dev->data_bus_width = dummy_dev_device->data_bus_width;
    dummy_dev->offset = dummy_dev_device->offset;
    dummy_dev->dummy_len = dummy_dev_device->dummy_len;
    dummy_dev->logic_func_mode = dummy_dev_device->logic_func_mode;
    if (strlen(dummy_dev_device->dev_alias) == 0) {
        dummy_dev->alias = dummy_dev->name;
    } else {
        dummy_dev->alias = dummy_dev_device->dev_alias;
    }

    if (dummy_dev->data_bus_width == 0 || dummy_dev->data_bus_width > WIDTH_4Byte) {
        dev_err(&pdev->dev, "Invalid data_bus_width: %u\n", dummy_dev->data_bus_width);
        return -EINVAL;
    }

    ret = platform_dev_status_check_config_init(&pdev->dev, &dummy_dev->status_check, dummy_dev->data_bus_width, &dummy_dev_device->status_check,
                                        dummy_device_func_read, dummy_device_func_write, dummy_dev->name);
    if (ret != 0) {
        dev_err(&pdev->dev, "platform status_check_config_init fail: %d\n", ret);
        return -EINVAL;
    }

    dummy_dev->log_node.log_num = dummy_dev_device->log_num;
    if (dummy_dev->log_node.log_num != 0) {
        if (dummy_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) {
            dev_err(&pdev->dev, "Invalid log_num: %u\n", dummy_dev->log_node.log_num);
            return -EINVAL;
        }
        for (i = 0; i < dummy_dev->log_node.log_num; i++) {
            dummy_dev->log_node.log_index[i] = dummy_dev_device->log_index[i];
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, dummy_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
    }

    DEBUG_VERBOSE("dev_name: %s, dev_alias: %s, logic_dev_name: %s, dummy_len: 0x%x, data_bus_width: %u, logic_func_mode: %d\n",
        dummy_dev->name, dummy_dev->alias, dummy_dev->logic_dev_name, dummy_dev->dummy_len,
        dummy_dev->data_bus_width, dummy_dev->logic_func_mode);
    DEBUG_VERBOSE("offset: 0x%x\n", dummy_dev->offset);
    return 0;
}

static int wb_dummy_dev_probe(struct platform_device *pdev)
{
    int ret;
    struct dummy_dev_info *dummy_dev;
    struct miscdevice *misc;

    DEBUG_VERBOSE("wb_dummy_dev_probe\n");

    dummy_dev = devm_kzalloc(&pdev->dev, sizeof(struct dummy_dev_info), GFP_KERNEL);
    if (!dummy_dev) {
        dev_err(&pdev->dev, "devm_kzalloc error.\n");
        return -ENOMEM;
    }

    if (pdev->dev.of_node) {
        ret = of_dummy_dev_config_init(pdev, dummy_dev);
    } else {
        ret = dummy_dev_config_init(pdev, dummy_dev);
    }

    if (ret < 0) {
        return ret;
    }

    ret = find_intf_addr(&dummy_dev->write_intf_addr, &dummy_dev->read_intf_addr, dummy_dev->logic_func_mode);
    if (ret) {
        dev_err(&pdev->dev, "Failed to find_intf_addr func mode %u, ret: %d\n", dummy_dev->logic_func_mode, ret);
        return ret;
    }

    if (!dummy_dev->write_intf_addr || !dummy_dev->read_intf_addr) {
        dev_err(&pdev->dev, "Fail: func mode %u rw symbol undefined\n", dummy_dev->logic_func_mode);
        return -ENOSYS;
    }

    mutex_init(&dummy_dev->update_lock);
    mutex_init(&dummy_dev->log_node.file_lock);

    dummy_dev->file_cache_rd = 0;
    dummy_dev->file_cache_wr = 0;
    snprintf(dummy_dev->cache_file_path, sizeof(dummy_dev->cache_file_path), CACHE_FILE_PATH, dummy_dev->name);
    snprintf(dummy_dev->mask_file_path, sizeof(dummy_dev->mask_file_path), MASK_FILE_PATH, dummy_dev->name);

    /* creat parent dir by dev name in /sys/logic_dev */
    ret = kobject_init_and_add(&dummy_dev->kobj, &dummy_dev_ktype, logic_dev_kobj, "%s", dummy_dev->name);
    if (ret) {
        kobject_put(&dummy_dev->kobj);
        dev_err(&pdev->dev, "Failed to creat parent dir: %s, ret: %d\n", dummy_dev->name, ret);
        return ret;
    }

    dummy_dev->sysfs_group = &dummy_dev_attr_group;
    ret = sysfs_create_group(&dummy_dev->kobj, dummy_dev->sysfs_group);
    if (ret) {
        dev_err(&pdev->dev, "Failed to create %s sysfs group, ret: %d\n", dummy_dev->name, ret);
        goto remove_parent_kobj;
    }

    platform_set_drvdata(pdev, dummy_dev);
    misc = &dummy_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = dummy_dev->name;
    misc->fops = &dummy_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&pdev->dev, "Failed to register %s device\n", misc->name);
        ret = -ENXIO;
        goto remove_sysfs_group;
    }

    ret = add_dev_to_g_dev_list(dummy_dev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to add_dev_to_g_dev_list, ret: %d\n", ret);
        goto deregister_misc;
    }

    dev_info(&pdev->dev, "Register dummy device %s success, logic_dev_name: %s, dummy_len: 0x%x, offset: 0x%x\n",
        dummy_dev->name, dummy_dev->logic_dev_name, dummy_dev->dummy_len, dummy_dev->offset);

    ret = wb_logic_check_status_hw_init(&dummy_dev->status_check, dummy_dev->data_bus_width);
    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to wb_logic_check_status_hw_init, ret: %d\n", ret);
    }

    return 0;
deregister_misc:
    misc_deregister(misc);
remove_sysfs_group:
    sysfs_remove_group(&dummy_dev->kobj, (const struct attribute_group *)dummy_dev->sysfs_group);
remove_parent_kobj:
    kobject_put(&dummy_dev->kobj);
    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
static void wb_dummy_dev_remove(struct platform_device *pdev)
#else
static int wb_dummy_dev_remove(struct platform_device *pdev)
#endif
{
    int minor;
    wb_dummy_dev_t *dummy_dev;

    dummy_dev = platform_get_drvdata(pdev);
    minor = dummy_dev->misc.minor;

    dev_dbg(&pdev->dev, "misc_deregister %s, minor: %d\n", dummy_dev->misc.name, minor);
    misc_deregister(&dummy_dev->misc);
    remove_dev_from_g_dev_list(minor);

    if (dummy_dev->sysfs_group) {
        dev_dbg(&pdev->dev, "Unregister %s dummy_dev sysfs group\n", dummy_dev->name);
        sysfs_remove_group(&dummy_dev->kobj, (const struct attribute_group *)dummy_dev->sysfs_group);
        kobject_put(&dummy_dev->kobj);
    }

    dev_info(&pdev->dev, "Remove %s dummy device success.\n", dummy_dev->name);
    platform_set_drvdata(pdev, NULL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
    return 0;
#endif
}

static const struct of_device_id wb_dummy_dev_driver_of_match[] = {
    { .compatible = "wb-dummy-dev" },
    { },
};

static struct platform_driver wb_dummy_dev_driver = {
    .probe      = wb_dummy_dev_probe,
    .remove     = wb_dummy_dev_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = MODULE_NAME,
        .of_match_table = wb_dummy_dev_driver_of_match,
    },
};

static int __init wb_dummy_dev_init(void)
{
    return platform_driver_register(&wb_dummy_dev_driver);
}

static void __exit wb_dummy_dev_exit(void)
{
    platform_driver_unregister(&wb_dummy_dev_driver);
}

module_init(wb_dummy_dev_init);
module_exit(wb_dummy_dev_exit);
MODULE_DESCRIPTION("dummy device driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
