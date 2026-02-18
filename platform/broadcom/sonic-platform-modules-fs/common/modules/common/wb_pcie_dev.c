/*
 * wb_pcie_dev.c
 * ko to read/write pcie iomem and ioports through /dev/XXX device
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
#include <wb_bsp_kernel_debug.h>
#include <wb_kernel_io.h>

#define PROXY_NAME "wb-pci-dev"
#define SEARCH_DEV_DEFAULT       (0)
#define SEARCH_DEV_BY_BRIDGE     (1)

#define SECBUS                   (0x19)
#define SUBBUS                   (0x1a)

/* Use the wb_bsp_kernel_debug header file must define debug variable */
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int status_cache_ms = 0;
module_param(status_cache_ms, int, S_IRUGO | S_IWUSR);

typedef struct wb_pci_dev_s {
    const char *name;
    const char *alias;
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
    struct kobject kobj;
    struct attribute_group *sysfs_group;
    void (*setreg)(struct wb_pci_dev_s *wb_pci_dev, int reg, u32 value);
    u32 (*getreg)(struct wb_pci_dev_s *wb_pci_dev, int reg);
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
} wb_pci_dev_t;

static wb_pci_dev_t* pcie_dev_arry[MAX_DEV_NUM];

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

    DEBUG_VERBOSE("inode: %p, file: %p, minor: %u", inode, file, minor);

    if (minor >= MAX_DEV_NUM) {
        DEBUG_ERROR("minor [%d] is greater than max dev num [%d], open fail\n", minor, MAX_DEV_NUM);
        return -ENODEV;
    }

    wb_pci_dev = pcie_dev_arry[minor];
    if (wb_pci_dev == NULL) {
        DEBUG_ERROR("wb_pci_dev is NULL, open failed, minor = %d\n", minor);
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
    int width, i, j, ret;
    u32 val;

    if (offset >= wb_pci_dev->bar_len) {
        DEBUG_VERBOSE("offset: 0x%x, bar len: 0x%x, EOF.\n", offset, wb_pci_dev->bar_len);
        return 0;
    }

    width = wb_pci_dev->bus_width;

    if (offset % width) {
        DEBUG_ERROR("pci bus width:%d, offset:0x%x, read size %lu invalid.\n",
            width, offset, count);
        return -EINVAL;
    }

    if (count > wb_pci_dev->bar_len - offset) {
        DEBUG_VERBOSE("read count out of range. input len:%lu, read len:%u.\n",
            count, wb_pci_dev->bar_len - offset);
        count = wb_pci_dev->bar_len - offset;
    }

    for (i = 0; i < count; i += width) {
        val = pci_dev_getreg(wb_pci_dev, offset + i);
        for (j = 0; (j < width) && (i + j < count); j++) {
            buf[i + j] = (val >> (8 * j)) & 0xff;
        }
    }

    if (wb_pci_dev->file_cache_rd) {
        ret = cache_value_read(wb_pci_dev->mask_file_path, wb_pci_dev->cache_file_path, offset, buf, count);
        if (ret < 0) {
            DEBUG_ERROR("pci data offset: 0x%x, read_len: %lu, read cache file fail, ret: %d, return act value\n",
                offset, count, ret);
        } else {
            DEBUG_VERBOSE("pci data offset: 0x%x, read_len: %lu success, read from cache value\n",
                offset, count);
        }
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(wb_pci_dev->name, offset, buf, count, true);
    }

    return count;
}

static ssize_t pci_dev_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
    wb_pci_dev_t *wb_pci_dev;
    int ret, read_len;
    u8 buf_tmp[MAX_RW_LEN];

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, read failed.\n");
        return -EINVAL;
    }

    wb_pci_dev = file->private_data;
    if (wb_pci_dev == NULL) {
        DEBUG_ERROR("wb_pci_dev is NULL, read failed.\n");
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
    read_len = pci_dev_read_tmp(wb_pci_dev, *offset, buf_tmp, count);
    if (read_len < 0) {
        DEBUG_ERROR("pci_dev_read_tmp failed, offset: 0x%llx, count: %lu, ret:%d.\n",
            *offset, count, read_len);
        return read_len;
    }

    if (read_len == 0) {
        DEBUG_VERBOSE("pci_dev_read_tmp EOF, offset: 0x%llx, count: %lu\n", *offset, count);
        return 0;
    }

    DEBUG_VERBOSE("read, buf: %p, offset: 0x%llx, read count %lu.\n", buf, *offset, count);
    memcpy(buf, buf_tmp, read_len);
    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t pci_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    DEBUG_VERBOSE("pci_dev_read_iter, file: %p, count: %lu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(to), iocb->ki_pos);
    return wb_iov_iter_read(iocb, to, pci_dev_read);
}

static int pci_dev_write_tmp(wb_pci_dev_t *wb_pci_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int width, i, j;
    u32 val;

    if (offset >= wb_pci_dev->bar_len) {
        DEBUG_VERBOSE("offset: 0x%x, bar len: 0x%x, EOF.\n", offset, wb_pci_dev->bar_len);
        return 0;
    }

    width = wb_pci_dev->bus_width;

    if (offset % width) {
        DEBUG_ERROR("pci bus width:%d, offset:0x%x, read size %lu invalid.\n",
            width, offset, count);
        return -EINVAL;
    }

    if (count > wb_pci_dev->bar_len - offset) {
        DEBUG_VERBOSE("write count out of range. input len:%lu, write len:%u.\n",
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

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(wb_pci_dev->name, offset, buf, count, false);
    }

    return count;
}

static ssize_t pci_dev_write(struct file *file, char *buf, size_t count, loff_t *offset)
{
    wb_pci_dev_t *wb_pci_dev;
    u8 buf_tmp[MAX_RW_LEN];
    int write_len;
    char bsp_log_dev_name[BSP_LOG_DEV_NAME_MAX_LEN];
    char bsp_log_file_path[BSP_LOG_DEV_NAME_MAX_LEN];

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, write failed.\n");
        return -EINVAL;
    }

    wb_pci_dev = file->private_data;
    if (wb_pci_dev == NULL) {
        DEBUG_ERROR("wb_pci_dev is NULL, write failed.\n");
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

    if (wb_pci_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Devfs]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_file_path), "%s.%s_bsp_key_reg", BSP_LOG_DIR, wb_pci_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(wb_pci_dev->log_node), (uint32_t)*offset, buf_tmp, count);
    }

    write_len = pci_dev_write_tmp(wb_pci_dev, *offset, buf_tmp, count);
    if (write_len < 0) {
        DEBUG_ERROR("pci_dev_write_tmp failed, offset: %llx, count: %lu, ret: %d.\n",
            *offset, count, write_len);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t pci_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    DEBUG_VERBOSE("pci_dev_write_iter, file: %p, count: %lu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(from), iocb->ki_pos);
    return wb_iov_iter_write(iocb, from, pci_dev_write);
}

static loff_t pci_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    wb_pci_dev_t *wb_pci_dev;

    wb_pci_dev = file->private_data;
    if (wb_pci_dev == NULL) {
        DEBUG_ERROR("wb_pci_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            DEBUG_ERROR("SEEK_SET, offset: %lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > wb_pci_dev->bar_len) {
            DEBUG_ERROR("SEEK_SET out of range, offset: %lld, bar len: 0x%x.\n",
                offset, wb_pci_dev->bar_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > wb_pci_dev->bar_len) || ((file->f_pos + offset) < 0)) {
            DEBUG_ERROR("SEEK_CUR out of range, f_ops: %lld, offset: %lld, bar len: 0x%x.\n",
                file->f_pos, offset, wb_pci_dev->bar_len);
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

static long pci_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static const struct file_operations pcie_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = pci_dev_llseek,
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

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (pcie_dev_arry[i] == NULL) {
            continue;
        }
        wb_pci_dev = pcie_dev_arry[i];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", wb_pci_dev->name);
        if (!strcmp(path, dev_name)) {
            DEBUG_VERBOSE("get dev_name = %s, minor = %d\n", dev_name, i);
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
        DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if (buf == NULL) {
        DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    wb_pci_dev = dev_match(path);
    if (wb_pci_dev == NULL) {
        DEBUG_ERROR("wb_pci_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    read_len = pci_dev_read_tmp(wb_pci_dev, offset, buf, count);
    if (read_len < 0) {
        DEBUG_ERROR("pci_dev_read_tmp failed, offset: 0x%x, count: %lu, ret: %d.\n",
            offset, count, read_len);
    }
    return read_len;
}
EXPORT_SYMBOL(pcie_device_func_read);

int pcie_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    wb_pci_dev_t *wb_pci_dev;
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

    wb_pci_dev = dev_match(path);
    if (wb_pci_dev == NULL) {
        DEBUG_ERROR("wb_pci_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    if (wb_pci_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Symbol]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_file_path), "%s.%s_bsp_key_reg", BSP_LOG_DIR, wb_pci_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(wb_pci_dev->log_node), offset, buf, count);
    }

    write_len = pci_dev_write_tmp(wb_pci_dev, offset, buf, count);
    if (write_len < 0) {
        DEBUG_ERROR("pci_dev_write_tmp failed, offset: 0x%x, count: %lu, ret: %d.\n",
            offset, count, write_len);
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
        DEBUG_ERROR("get bar addr failed. bar:%d, addr:0x%x, len:0x%x.\n",
            wb_pci_dev->bar, addr, len);
        return -EFAULT;
    }
    wb_pci_dev->bar_len = len;

    flags = pci_resource_flags(dev, wb_pci_dev->bar);
    DEBUG_VERBOSE("bar:%d, flag:0x%08x, phys addr:0x%x, len:0x%x\n",
        wb_pci_dev->bar, flags, addr, len);
    if (flags & IORESOURCE_MEM) {
        wb_pci_dev->bar_flag = IORESOURCE_MEM;
        wb_pci_dev->pci_mem_base = ioremap(addr, len);
        DEBUG_VERBOSE("pci mem base:%p.\n", wb_pci_dev->pci_mem_base);
    } else if (flags & IORESOURCE_IO) {
        wb_pci_dev->bar_flag = IORESOURCE_IO;
        wb_pci_dev->pci_io_base = addr;
        DEBUG_VERBOSE("pci io base:0x%x.\n", wb_pci_dev->pci_io_base);
    } else {
        DEBUG_ERROR("unknow pci bar flag:0x%08x.\n", flags);
        ret = -EINVAL;
    }

    return ret;
}

static ssize_t pci_dev_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->show) {
        DEBUG_ERROR("pci dev attr show is null.\n");
        return -ENOSYS;
    }

    return attribute->show(kobj, attribute, buf);
}

static ssize_t pci_dev_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf,
                   size_t len)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->store) {
        DEBUG_ERROR("pci dev attr store is null.\n");
        return -ENOSYS;
    }

    return attribute->store(kobj, attribute, buf, len);
}

static const struct sysfs_ops pci_dev_sysfs_ops = {
    .show = pci_dev_attr_show,
    .store = pci_dev_attr_store,
};

static void pci_dev_obj_release(struct kobject *kobj)
{
    return;
}

static struct kobj_type pci_dev_ktype = {
    .sysfs_ops = &pci_dev_sysfs_ops,
    .release = pci_dev_obj_release,
};

static ssize_t alias_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_pci_dev_t *pci_dev = container_of(kobj, wb_pci_dev_t, kobj);

    if (!pci_dev) {
        DEBUG_ERROR("alias show pci dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", pci_dev->alias);
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
    wb_pci_dev_t *pci_dev = container_of(kobj, wb_pci_dev_t, kobj);

    mem_clear(buf, PAGE_SIZE);
    offset = 0;
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "name: %s\n", pci_dev->name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "alias: %s\n", pci_dev->alias);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "domain: 0x%04x\n", pci_dev->domain);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "bus: 0x%02x\n", pci_dev->bus);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "slot: 0x%02x\n", pci_dev->slot);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "function: %u\n", pci_dev->fn);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "bar: %u\n", pci_dev->bar);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "bus_width: %d\n", pci_dev->bus_width);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "bar_len: 0x%x\n", pci_dev->bar_len);
    buf_len = strlen(buf);
    return buf_len;
}

static int rw_status_check_one_time(wb_pci_dev_t *pci_dev, uint32_t offset, uint32_t len, uint32_t type)
{
    uint8_t wr_buf[WIDTH_4Byte];
    uint8_t rd_buf[WIDTH_4Byte];
    int ret, i;

    if (len > sizeof(wr_buf) || len > sizeof(rd_buf)) {
        DEBUG_ERROR("input param error: len:%d out of range.\n", len);
        return -EINVAL;
    }

    for (i = 0; i < len; i++) {
        wr_buf[i] = pci_dev->test_data[i];
    }

    ret = pci_dev_write_tmp(pci_dev, offset, wr_buf, len);
    if ((ret < 0) || (ret != len)) {
        DEBUG_ERROR("STATUS_NOT_OK dev status wr offset: 0x%x, len: %u failed, ret: %d.\n",
            offset, len, ret);
        return -EIO;
    }

    mem_clear(rd_buf, sizeof(rd_buf));
    ret = pci_dev_read_tmp(pci_dev, offset, rd_buf, len);
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

static int rw_status_check(wb_pci_dev_t *pci_dev, uint32_t offset, uint32_t len, uint32_t type)
{
    int ret, i;

    ret = 0;
    for (i = 0; i < LOGIC_DEV_RETRY_TIME; i++) {
        ret = rw_status_check_one_time(pci_dev, offset, len, type);
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
    wb_pci_dev_t *pci_dev;
    uint32_t offset, type, len;
    int ret, i;

    pci_dev = container_of(kobj, wb_pci_dev_t, kobj);
    if (!pci_dev) {
        DEBUG_ERROR("Failed: status show param is NULL.\n");
        return -ENODEV;
    }

    type = pci_dev->status_check_type;

    if (type == NOT_SUPPORT_CHECK_STATUS) {
        DEBUG_ERROR("unsupport dev status check.\n");
        return -EOPNOTSUPP;
    }

    if (time_before(jiffies, pci_dev->last_jiffies + msecs_to_jiffies(status_cache_ms))) {
        /* Within the time range of status_cache_ms, directly return the last result */
        DEBUG_VERBOSE("time before last time %d ms return last status: %d\n",
            status_cache_ms, pci_dev->dev_status);
        return sprintf(buf, "%d\n", pci_dev->dev_status);
    }

    pci_dev->last_jiffies = jiffies;

    len = pci_dev->bus_width;
    if (len > WIDTH_4Byte) {
        DEBUG_ERROR("status show rw len:%u beyond max 4 byte.\n", len);
        return -EINVAL;
    }

    mutex_lock(&pci_dev->update_lock);

    for (i = 0; i < len; i++) {
        /* reverse to ensure that different data is read and written each time the verification is performed.*/
        pci_dev->test_data[i] = ~pci_dev->test_data[i];
    }
    for (i = 0; i < pci_dev->test_reg_num; i++) {
        offset = pci_dev->test_reg[i];
        ret = rw_status_check(pci_dev, offset, len, type);
        if (ret < 0) {
            pci_dev->dev_status = LOGIC_DEV_STATUS_NOT_OK;
            DEBUG_ERROR("STATUS_NOT_OK result check all retry failed.\n");
            mutex_unlock(&pci_dev->update_lock);
            return sprintf(buf, "%u\n", pci_dev->dev_status);
        }
    }

    pci_dev->dev_status = LOGIC_DEV_STATUS_OK;
    mutex_unlock(&pci_dev->update_lock);
    return sprintf(buf, "%u\n", pci_dev->dev_status);
}

static ssize_t file_cache_rd_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_pci_dev_t *pci_dev = container_of(kobj, wb_pci_dev_t, kobj);

    if (!pci_dev) {
        DEBUG_ERROR("file_cache_rd_show pci dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%u\n", pci_dev->file_cache_rd);
}

static ssize_t file_cache_rd_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_pci_dev_t *pci_dev = container_of(kobj, wb_pci_dev_t, kobj);
    u8 val;
    int ret;

    if (!pci_dev) {
        DEBUG_ERROR("file_cache_rd_store pci dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    pci_dev->file_cache_rd = val;

    return count;
}

static ssize_t file_cache_wr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_pci_dev_t *pci_dev = container_of(kobj, wb_pci_dev_t, kobj);

    if (!pci_dev) {
        DEBUG_ERROR("file_cache_wr_show pci dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", pci_dev->file_cache_wr);
}

static ssize_t file_cache_wr_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_pci_dev_t *pci_dev = container_of(kobj, wb_pci_dev_t, kobj);
    u8 val;
    int ret;

    if (!pci_dev) {
        DEBUG_ERROR("file_cache_wr_store pci dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    pci_dev->file_cache_wr = val;

    return count;
}

static ssize_t cache_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_pci_dev_t *pci_dev = container_of(kobj, wb_pci_dev_t, kobj);

    if (!pci_dev) {
        DEBUG_ERROR("cache_file_path_show pci dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", pci_dev->cache_file_path);
}

static ssize_t mask_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_pci_dev_t *pci_dev = container_of(kobj, wb_pci_dev_t, kobj);

    if (!pci_dev) {
        DEBUG_ERROR("mask_file_path_show pci dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", pci_dev->mask_file_path);
}

static struct kobj_attribute alias_attribute = __ATTR(alias, S_IRUGO, alias_show, NULL);
static struct kobj_attribute type_attribute = __ATTR(type, S_IRUGO, type_show, NULL);
static struct kobj_attribute info_attribute = __ATTR(info, S_IRUGO, info_show, NULL);
static struct kobj_attribute status_attribute = __ATTR(status, S_IRUGO, status_show, NULL);
static struct kobj_attribute file_cache_rd_attribute = __ATTR(file_cache_rd, S_IRUGO  | S_IWUSR, file_cache_rd_show, file_cache_rd_store);
static struct kobj_attribute file_cache_wr_attribute = __ATTR(file_cache_wr, S_IRUGO  | S_IWUSR, file_cache_wr_show, file_cache_wr_store);
static struct kobj_attribute cache_file_path_attribute = __ATTR(cache_file_path, S_IRUGO, cache_file_path_show, NULL);
static struct kobj_attribute mask_file_path_attribute = __ATTR(mask_file_path, S_IRUGO, mask_file_path_show, NULL);

static struct attribute *pci_dev_attrs[] = {
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

static struct attribute_group pci_dev_attr_group = {
    .attrs = pci_dev_attrs,
};

static int of_pci_dev_config_init(struct platform_device *pdev, wb_pci_dev_t *wb_pci_dev)
{
    int i, ret;
    uint32_t type;

    ret = 0;
    ret += of_property_read_string(pdev->dev.of_node, "pci_dev_name", &wb_pci_dev->name);
    ret += of_property_read_u32(pdev->dev.of_node, "pci_domain", &wb_pci_dev->domain);
    ret += of_property_read_u32(pdev->dev.of_node, "pci_slot", &wb_pci_dev->slot);
    ret += of_property_read_u32(pdev->dev.of_node, "pci_fn", &wb_pci_dev->fn);
    ret += of_property_read_u32(pdev->dev.of_node, "pci_bar", &wb_pci_dev->bar);
    ret += of_property_read_u32(pdev->dev.of_node, "bus_width", &wb_pci_dev->bus_width);
    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to get dts config, ret: %d.\n", ret);
        return -ENXIO;
    }

    if (wb_pci_dev->bus_width == 0) {
        dev_err(&pdev->dev, "Invalid bus_width: %u\n", wb_pci_dev->bus_width);
        return -EINVAL;
    }

    ret = of_property_read_u32(pdev->dev.of_node, "status_check_type", &wb_pci_dev->status_check_type);
    if (ret == 0) {
        type = wb_pci_dev->status_check_type;
        if (type != READ_BACK_CHECK && type != READ_BACK_NAGATIVE_CHECK) {
            dev_err(&pdev->dev, "Invalid status_check_type: %u\n", type);
            return -EINVAL;
        }
        DEBUG_VERBOSE("status_check_type: %d, \n", wb_pci_dev->status_check_type);

        ret = of_property_read_u32(pdev->dev.of_node, "test_reg_num", &wb_pci_dev->test_reg_num);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get test_reg_num config, ret: %d\n", ret);
            return -ENXIO;
        }
        if (wb_pci_dev->test_reg_num == 0 || wb_pci_dev->test_reg_num > TEST_REG_MAX_NUM) {
            dev_err(&pdev->dev, "Invalid test_reg_num: %u\n", wb_pci_dev->test_reg_num);
            return -EINVAL;
        }

        ret = of_property_read_u32_array(pdev->dev.of_node, "test_reg", wb_pci_dev->test_reg, wb_pci_dev->test_reg_num);
        if(ret != 0) {
            dev_err(&pdev->dev, "Failed to get test_reg config, ret: %d\n", ret);
            return -ENXIO;
        }

        wb_pci_dev->last_jiffies = jiffies;
        wb_pci_dev->dev_status = LOGIC_DEV_STATUS_OK;

        for (i = 0; i < WIDTH_4Byte; i++) {
            wb_pci_dev->test_data[i] = INIT_TEST_DATA;
        }

        for (i = 0; i < wb_pci_dev->test_reg_num; i++) {
            DEBUG_VERBOSE("test_reg[%d] = 0x%x.\n", i, wb_pci_dev->test_reg[i]);
        }
    } else {
        wb_pci_dev->status_check_type = NOT_SUPPORT_CHECK_STATUS;
        DEBUG_VERBOSE("not support dev status check sysfs.\n");
    }

    ret = of_property_read_u32(pdev->dev.of_node, "log_num", &(wb_pci_dev->log_node.log_num));
    if (ret == 0) {
        if ((wb_pci_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) || (wb_pci_dev->log_node.log_num == 0)) {
            dev_err(&pdev->dev, "Invalid log_num: %u\n", wb_pci_dev->log_node.log_num);
            return -EINVAL;
        }
        ret = of_property_read_u32_array(pdev->dev.of_node, "log_index", wb_pci_dev->log_node.log_index, wb_pci_dev->log_node.log_num);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get log_index config, ret: %d\n", ret);
            return -EINVAL;
        }
        for (i = 0; i < wb_pci_dev->log_node.log_num; i++) {
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, wb_pci_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
        wb_pci_dev->log_node.log_num = 0;
    }

    if (of_property_read_string(pdev->dev.of_node, "pci_dev_alias", &wb_pci_dev->alias)) {
        wb_pci_dev->alias = wb_pci_dev->name;
    }

    wb_pci_dev->search_mode = SEARCH_DEV_DEFAULT;
    of_property_read_u32(pdev->dev.of_node, "search_mode", &wb_pci_dev->search_mode);
    if (wb_pci_dev->search_mode != SEARCH_DEV_DEFAULT && wb_pci_dev->search_mode != SEARCH_DEV_BY_BRIDGE) {
        dev_err(&pdev->dev, "Invalid pci device search_mode: %d\n", wb_pci_dev->search_mode);
        return -EINVAL;
    }

    ret = 0;
    if (wb_pci_dev->search_mode == SEARCH_DEV_BY_BRIDGE) {
        ret += of_property_read_u32(pdev->dev.of_node, "bridge_bus", &wb_pci_dev->bridge_bus);
        ret += of_property_read_u32(pdev->dev.of_node, "bridge_slot", &wb_pci_dev->bridge_slot);
        ret += of_property_read_u32(pdev->dev.of_node, "bridge_fn", &wb_pci_dev->bridge_fn);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get pci bridge_bus or bridge_slot or bridge_fn config, ret: %d.\n", ret);
            return -ENXIO;
        } else {
            DEBUG_VERBOSE("search_dev_by_bridge, bridge_bus: 0x%02x, bridge_slot: 0x%02x, bridge_fn: 0x%02x.\n",
                wb_pci_dev->bridge_bus, wb_pci_dev->bridge_slot, wb_pci_dev->bridge_fn);
        }
    } else {
        ret += of_property_read_u32(pdev->dev.of_node, "pci_bus", &wb_pci_dev->bus);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get pci_bus config, ret: %d.\n", ret);
            return -ENXIO;
        } else {
            DEBUG_VERBOSE("search_dev_by_pcibus, get pci_bus:0x%02x.\n", wb_pci_dev->bus);
        }
    }

    ret = of_property_read_u32(pdev->dev.of_node, "check_pci_id", &wb_pci_dev->check_pci_id);
    if (ret == 0) {
        ret = of_property_read_u32(pdev->dev.of_node, "pci_id", &wb_pci_dev->pci_id);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get pci_id, ret: %d.\n", ret);
            return -ENXIO;
        }
        DEBUG_VERBOSE("check_pci_id, except pci_id: 0x%x\n", wb_pci_dev->pci_id);
    }

    DEBUG_VERBOSE("name: %s, alias: %s, domain: 0x%04x, slot: 0x%02x, fn: %u, bar: %u, bus_width: %d, search_mode:%d \n",
        wb_pci_dev->name, wb_pci_dev->alias, wb_pci_dev->domain, wb_pci_dev->slot, wb_pci_dev->fn,
        wb_pci_dev->bar, wb_pci_dev->bus_width, wb_pci_dev->search_mode);

    return 0;
}

static int pci_dev_config_init(struct platform_device *pdev, wb_pci_dev_t *wb_pci_dev)
{
    int i;
    uint32_t type;
    pci_dev_device_t *pci_dev_device;

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
    wb_pci_dev->pci_id = pci_dev_device->pci_id;
    wb_pci_dev->search_mode = pci_dev_device->search_mode;
    if (wb_pci_dev->bus_width == 0) {
        dev_err(&pdev->dev, "Invalid bus_width: %u\n", wb_pci_dev->bus_width);
        return -EINVAL;
    }

    if (strlen(pci_dev_device->pci_dev_alias) == 0) {
        wb_pci_dev->alias = wb_pci_dev->name;
    } else {
        wb_pci_dev->alias = pci_dev_device->pci_dev_alias;
    }

    wb_pci_dev->status_check_type = pci_dev_device->status_check_type;
    if (wb_pci_dev->status_check_type != NOT_SUPPORT_CHECK_STATUS) {
        type = wb_pci_dev->status_check_type;
        if ((type != READ_BACK_CHECK) && (type != READ_BACK_NAGATIVE_CHECK)) {
            dev_err(&pdev->dev, "Invalid status_check_type: %u\n", type);
            return -EINVAL;
        }
        DEBUG_VERBOSE("status_check_type: %d\n", wb_pci_dev->status_check_type);

        wb_pci_dev->test_reg_num = pci_dev_device->test_reg_num;
        if ((wb_pci_dev->test_reg_num == 0) || (wb_pci_dev->test_reg_num > TEST_REG_MAX_NUM)) {
            dev_err(&pdev->dev, "Invalid test_reg_num: %u\n", wb_pci_dev->test_reg_num);
            return -EINVAL;
        }
        for (i = 0; i < wb_pci_dev->test_reg_num; i++) {
            wb_pci_dev->test_reg[i] = pci_dev_device->test_reg[i];
            DEBUG_VERBOSE("test_reg[%d] address = 0x%x.\n", i, wb_pci_dev->test_reg[i]);
        }

        wb_pci_dev->last_jiffies = jiffies;
        wb_pci_dev->dev_status = LOGIC_DEV_STATUS_OK;
        for (i = 0; i < WIDTH_4Byte; i++) {
            wb_pci_dev->test_data[i] = INIT_TEST_DATA;
        }
    } else {
        DEBUG_VERBOSE("not support dev status check sysfs.\n");
    }

    wb_pci_dev->log_node.log_num = pci_dev_device->log_num;
    if (wb_pci_dev->log_node.log_num != 0) {
        if (wb_pci_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) {
            dev_err(&pdev->dev, "Invalid log_num: %u\n", wb_pci_dev->log_node.log_num);
            return -EINVAL;
        }
        for (i = 0; i < wb_pci_dev->log_node.log_num; i++) {
            wb_pci_dev->log_node.log_index[i] = pci_dev_device->log_index[i];
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, wb_pci_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
    }

    if (wb_pci_dev->search_mode != SEARCH_DEV_DEFAULT && wb_pci_dev->search_mode != SEARCH_DEV_BY_BRIDGE) {
        dev_err(&pdev->dev, "Invalid pci device search_mode: %d\n", wb_pci_dev->search_mode);
        return -EINVAL;
    }

    if (wb_pci_dev->search_mode == SEARCH_DEV_BY_BRIDGE) {
        wb_pci_dev->bridge_bus = pci_dev_device->bridge_bus;
        wb_pci_dev->bridge_slot = pci_dev_device->bridge_slot;
        wb_pci_dev->bridge_fn = pci_dev_device->bridge_fn;
        DEBUG_VERBOSE("search_dev_by_bridge, bridge_bus: 0x%02x, bridge_slot: 0x%02x, bridge_fn: 0x%02x.\n",
            wb_pci_dev->bridge_bus, wb_pci_dev->bridge_slot, wb_pci_dev->bridge_fn);
    } else {
        wb_pci_dev->bus = pci_dev_device->pci_bus;
        DEBUG_VERBOSE("search_dev_by_pcibus, pci_bus: 0x%02x.\n", wb_pci_dev->bus);
    }

    DEBUG_VERBOSE("name: %s, alias: %s, domain: 0x%04x, slot: 0x%02x, fn: %u, bar: %u, bus_width: %d, search_mode:%d \n",
        wb_pci_dev->name, wb_pci_dev->alias, wb_pci_dev->domain, wb_pci_dev->slot, wb_pci_dev->fn,
        wb_pci_dev->bar, wb_pci_dev->bus_width, wb_pci_dev->search_mode);

    return 0;
}

static int pci_dev_probe(struct platform_device *pdev)
{
    int ret, devfn;
    uint32_t pci_id;
    wb_pci_dev_t *wb_pci_dev;
    struct pci_dev *pci_dev, *pci_bridge_dev;
    struct miscdevice *misc;
    u8 secbus_val, subbus_val;

    wb_pci_dev = devm_kzalloc(&pdev->dev, sizeof(wb_pci_dev_t), GFP_KERNEL);
    if (!wb_pci_dev) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    if (pdev->dev.of_node) {
        ret = of_pci_dev_config_init(pdev, wb_pci_dev);
    } else {
        ret = pci_dev_config_init(pdev, wb_pci_dev);
    }

    if (ret < 0) {
        return ret;
    }

    mutex_init(&wb_pci_dev->update_lock);
    mutex_init(&wb_pci_dev->log_node.file_lock);

    wb_pci_dev->file_cache_rd = 0;
    wb_pci_dev->file_cache_wr = 0;
    snprintf(wb_pci_dev->cache_file_path, sizeof(wb_pci_dev->cache_file_path), CACHE_FILE_PATH, wb_pci_dev->name);
    snprintf(wb_pci_dev->mask_file_path, sizeof(wb_pci_dev->mask_file_path), MASK_FILE_PATH, wb_pci_dev->name);

    if (wb_pci_dev->search_mode == SEARCH_DEV_DEFAULT) {
        devfn = PCI_DEVFN(wb_pci_dev->slot, wb_pci_dev->fn);
        pci_dev = pci_get_domain_bus_and_slot(wb_pci_dev->domain, wb_pci_dev->bus, devfn);
        if (pci_dev == NULL) {
            dev_err(&pdev->dev, "Failed to find pci_dev, domain: 0x%04x, bus: 0x%02x, devfn: 0x%x\n",
                wb_pci_dev->domain, wb_pci_dev->bus, devfn);
            return -ENXIO;
        }
    } else { /* search_mode = SEARCH_DEV_BY_BRIDGE */
        devfn = PCI_DEVFN(wb_pci_dev->bridge_slot, wb_pci_dev->bridge_fn);
        pci_bridge_dev = pci_get_domain_bus_and_slot(wb_pci_dev->domain, wb_pci_dev->bridge_bus, devfn);
        if (pci_bridge_dev == NULL) {
            dev_err(&pdev->dev, "Failed to find pci_bridge_dev, domain: 0x%04x, bus: 0x%02x, devfn: 0x%x\n",
                wb_pci_dev->domain, wb_pci_dev->bridge_bus, devfn);
            return -ENXIO;
        }

        secbus_val = 0;
        ret = pci_read_config_byte(pci_bridge_dev, SECBUS, &secbus_val);
        if (ret) {
            dev_err(&pdev->dev, "Failed to get secbus value, ret: %d\n", ret);
            return -EIO;
        }
        subbus_val = 0;
        ret = pci_read_config_byte(pci_bridge_dev, SUBBUS, &subbus_val);
        if (ret) {
            dev_err(&pdev->dev, "Failed to get subbus value, ret: %d\n", ret);
            return -EIO;
        }
        if (secbus_val != subbus_val) {
            /* If the SECBUS register value is different from the SUBBUS register value, a multistage PCIE bridge is available*/
            dev_err(&pdev->dev, "Failed to get pci bus, secbus: 0x%x not euqal to subbus: 0x%x\n", secbus_val, subbus_val);
            return -EIO;
        }
        wb_pci_dev->bus = secbus_val;
        devfn = PCI_DEVFN(wb_pci_dev->slot, wb_pci_dev->fn);
        pci_dev = pci_get_domain_bus_and_slot(wb_pci_dev->domain, wb_pci_dev->bus, devfn);
        if (pci_dev == NULL) {
            dev_err(&pdev->dev, "Failed to find pci_dev, domain: 0x%04x, bus: 0x%02x, devfn:  0x%x\n",
                wb_pci_dev->domain, wb_pci_dev->bus, devfn);
            return -ENXIO;
        }
    }

    if (wb_pci_dev->check_pci_id == 1) {
        pci_id = (pci_dev->vendor << 16) | pci_dev->device;
        if (wb_pci_dev->pci_id != pci_id) {
            dev_err(&pdev->dev, "Failed to check pci id, except: 0x%x, really: 0x%x\n",
                wb_pci_dev->pci_id, pci_id);
            return -ENXIO;
        }
        DEBUG_VERBOSE("pci id check ok, pci_id: 0x%x", pci_id);
    }
    ret = pci_setup_bars(wb_pci_dev, pci_dev);
    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to get pci bar address.\n");
        return ret;
    }

    if (!wb_pci_dev->setreg || !wb_pci_dev->getreg) {
        switch (wb_pci_dev->bus_width) {
        case WIDTH_1Byte:
            wb_pci_dev->setreg = pci_dev_setreg_8;
            wb_pci_dev->getreg = pci_dev_getreg_8;
            break;

        case WIDTH_2Byte:
            wb_pci_dev->setreg = pci_dev_setreg_16;
            wb_pci_dev->getreg = pci_dev_getreg_16;
            break;

        case WIDTH_4Byte:
            wb_pci_dev->setreg = pci_dev_setreg_32;
            wb_pci_dev->getreg = pci_dev_getreg_32;
            break;
        default:
            dev_err(&pdev->dev, "Error: unsupported I/O width (%d).\n", wb_pci_dev->bus_width);
            ret = -EINVAL;
            goto io_unmap;
        }
    }

    /* creat parent dir by dev name in /sys/logic_dev */
    ret = kobject_init_and_add(&wb_pci_dev->kobj, &pci_dev_ktype, logic_dev_kobj, "%s", wb_pci_dev->name);
    if (ret) {
        kobject_put(&wb_pci_dev->kobj);
        dev_err(&pdev->dev, "Failed to creat parent dir: %s, ret: %d\n", wb_pci_dev->name, ret);
        goto io_unmap;
    }

    wb_pci_dev->sysfs_group = &pci_dev_attr_group;
    ret = sysfs_create_group(&wb_pci_dev->kobj, wb_pci_dev->sysfs_group);
    if (ret) {
        dev_err(&pdev->dev, "Failed to create %s sysfs group, ret: %d\n", wb_pci_dev->name, ret);
        goto remove_parent_kobj;
    }

    platform_set_drvdata(pdev, wb_pci_dev);
    misc = &wb_pci_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = wb_pci_dev->name;
    misc->fops = &pcie_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&pdev->dev, "Failed to register %s device\n", misc->name);
        ret = -ENXIO;
        goto remove_sysfs_group;
    }
    if (misc->minor >= MAX_DEV_NUM) {
        dev_err(&pdev->dev, "Error: device minor[%d] more than max device num[%d]\n",
            misc->minor, MAX_DEV_NUM);
        ret = -EINVAL;
        goto deregister_misc;
    }
    pcie_dev_arry[misc->minor] = wb_pci_dev;
    dev_info(&pdev->dev, "%04x:%02x:%02x.%d[bar%d: %s]: register %s device with minor: %d success.\n",
        wb_pci_dev->domain, wb_pci_dev->bus, wb_pci_dev->slot, wb_pci_dev->fn, wb_pci_dev->bar,
        wb_pci_dev->bar_flag == IORESOURCE_MEM ? "IORESOURCE_MEM" : "IORESOURCE_IO",
        misc->name, misc->minor);
    return 0;
deregister_misc:
    misc_deregister(misc);
remove_sysfs_group:
    sysfs_remove_group(&wb_pci_dev->kobj, (const struct attribute_group *)wb_pci_dev->sysfs_group);
remove_parent_kobj:
    kobject_put(&wb_pci_dev->kobj);
io_unmap:
    if (wb_pci_dev->pci_mem_base) {
        iounmap(wb_pci_dev->pci_mem_base);
    }
    return ret;
}

static int pci_dev_remove(struct platform_device *pdev)
{
    int minor;
    wb_pci_dev_t *wb_pci_dev;

    wb_pci_dev = platform_get_drvdata(pdev);
    minor = wb_pci_dev->misc.minor;
    if (minor < MAX_DEV_NUM && (pcie_dev_arry[minor] != NULL)) {
        dev_dbg(&pdev->dev, "misc_deregister %s, minor: %d\n", wb_pci_dev->misc.name, minor);
        misc_deregister(&pcie_dev_arry[minor]->misc);
        pcie_dev_arry[minor] = NULL;
    }

    if (wb_pci_dev->sysfs_group) {
        dev_dbg(&pdev->dev, "Unregister %s pci_dev sysfs group\n", wb_pci_dev->name);
        sysfs_remove_group(&wb_pci_dev->kobj, (const struct attribute_group *)wb_pci_dev->sysfs_group);
        kobject_put(&wb_pci_dev->kobj);
        wb_pci_dev->sysfs_group = NULL;
    }

    dev_info(&pdev->dev, "Remove %s pci device success.\n", wb_pci_dev->name);
    platform_set_drvdata(pdev, NULL);
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
