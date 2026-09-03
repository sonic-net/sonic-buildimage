/*
 * wb_spi_dev.c
 * ko to read/write spi device through /dev/XXX device
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/export.h>
#include <linux/uio.h>

#include "wb_spi_dev.h"
#include <wb_bsp_kernel_debug.h>
#include <wb_kernel_io.h>

#define PROXY_NAME "wb-spi-dev"
#define MAX_ADDR_BUS_WIDTH   (4)
#define TRANSFER_WRITE_BUFF  (1 + MAX_ADDR_BUS_WIDTH + MAX_RW_LEN)

#define OP_READ             (0x3)
#define OP_WRITE            (0x2)

/* Use the wb_bsp_kernel_debug header file must define debug variable */
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int status_cache_ms = 0;
module_param(status_cache_ms, int, S_IRUGO | S_IWUSR);

static DEFINE_SPINLOCK(dev_array_lock);
static struct spi_dev_info* spi_dev_arry[MAX_DEV_NUM];

typedef struct spi_dev_info {
    const char *name;
    const char *alias;
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
    uint32_t spi_len;
    struct miscdevice misc;
    struct spi_device *spi_device;
    struct kobject kobj;
    struct attribute_group *sysfs_group;
    uint8_t file_cache_rd;
    uint8_t file_cache_wr;
    char cache_file_path[MAX_NAME_SIZE];
    char mask_file_path[MAX_NAME_SIZE];
    struct mutex update_lock;
    wb_bsp_key_device_log_node_t log_node;
    uint8_t share_high_addr_cmd_mask;    /* The command word shares the high address mask */
    uint8_t share_high_addr_rd_cmd;      /* The command word shares the high address read command */
    uint8_t share_high_addr_wr_cmd;      /* The command word shares the high address write command */
    uint8_t collect_skip_flag;            /* Collection disable flag */
    device_status_check_t status_check;
} wb_spi_dev_t;

static int transfer_read(struct spi_dev_info *spi_dev, u8 *buf, uint32_t regaddr, size_t count, bool is_atomic)
{
    int i, ret;
    u8 tx_buf[MAX_ADDR_BUS_WIDTH + 1];
    struct spi_message m;
    struct spi_transfer xfer[2];

    i = 0;
    mem_clear(tx_buf, sizeof(tx_buf));
    /**
     * When commands do not share the address,
     * the command word is sent separately,
     * resulting in an additional byte being added to the transmission length
     */
    if (spi_dev->share_high_addr_cmd_mask == 0) {
        tx_buf[i++] = OP_READ;
    }

    switch (spi_dev->addr_bus_width) {
    case WIDTH_4Byte:
        tx_buf[i++] = (regaddr >> 24) & 0xFF;
        tx_buf[i++] = (regaddr >> 16) & 0xFF;
        tx_buf[i++] = (regaddr >> 8) & 0xFF;
        tx_buf[i++] = regaddr & 0xFF;
        break;
    case WIDTH_2Byte:
        tx_buf[i++] = (regaddr >> 8) & 0xFF;
        tx_buf[i++] = regaddr & 0xFF;
        break;
    case WIDTH_1Byte:
        tx_buf[i++] = regaddr & 0xFF;
        break;
    default:
        DEBUG_ERROR("Only support 1,2,4 Byte Width,but set width = %u\n",
            spi_dev->addr_bus_width);
        return -EINVAL;
    }
    /* When commands share an address, update the value of the high address transmission. */
    if (spi_dev->share_high_addr_cmd_mask > 0) {
        tx_buf[0] &= ~spi_dev->share_high_addr_cmd_mask;
        tx_buf[0] |= spi_dev->share_high_addr_rd_cmd;
    }

    mem_clear(xfer, sizeof(xfer));
    spi_message_init(&m);
    xfer[0].tx_buf = tx_buf;
    xfer[0].len = i;
    spi_message_add_tail(&xfer[0], &m);

    xfer[1].rx_buf = buf;
    xfer[1].len = count;
    spi_message_add_tail(&xfer[1], &m);

    if (!is_atomic) {
        ret = spi_sync(spi_dev->spi_device, &m);
    } else {
        ret = spi_async(spi_dev->spi_device, &m);
    }
    if (ret) {
        DEBUG_ERROR("transfer_read failed, is_atomic:%d, reg addr:0x%x, len:%zu, ret:%d.\n",
            is_atomic, regaddr, count, ret);
        return -EIO;
    }
    return 0;
}

static int transfer_write(struct spi_dev_info *spi_dev, u8 *buf, uint32_t regaddr, size_t count, bool is_atomic)
{
    int i, ret;
    u8 tx_buf[TRANSFER_WRITE_BUFF];
    struct spi_message m;
    struct spi_transfer xfer ;

    i = 0;
    mem_clear(tx_buf, sizeof(tx_buf));
    /* When commands do not share an address, the command word is transmitted independently. */
    if (spi_dev->share_high_addr_cmd_mask == 0) {
        tx_buf[i++] = OP_WRITE;
    }
    switch (spi_dev->addr_bus_width) {
    case WIDTH_4Byte:
        tx_buf[i++] = (regaddr >> 24) & 0xFF;
        tx_buf[i++] = (regaddr >> 16) & 0xFF;
        tx_buf[i++] = (regaddr >> 8) & 0xFF;
        tx_buf[i++] = regaddr & 0xFF;
        break;
    case WIDTH_2Byte:
        tx_buf[i++] = (regaddr >> 8) & 0xFF;
        tx_buf[i++] = regaddr & 0xFF;
        break;
    case WIDTH_1Byte:
        tx_buf[i++] = regaddr & 0xFF;
        break;
    default:
        DEBUG_ERROR("Only support 1,2,4 Byte Width, but set width = %u\n",
            spi_dev->addr_bus_width);
        return -EINVAL;
    }
    /* When commands share an address, update the value of the high address transmission. */
    if (spi_dev->share_high_addr_cmd_mask > 0) {
        tx_buf[0] &= ~spi_dev->share_high_addr_cmd_mask;
        tx_buf[0] |= spi_dev->share_high_addr_wr_cmd;
    }
    memcpy(tx_buf + i, buf, count);

    mem_clear(&xfer, sizeof(xfer));
    spi_message_init(&m);
    xfer.tx_buf = tx_buf;
    xfer.len = count + i;
    spi_message_add_tail(&xfer, &m);

    if (!is_atomic) {
        ret = spi_sync(spi_dev->spi_device, &m);
    } else {
        ret = spi_async(spi_dev->spi_device, &m);
    }
    if (ret) {
        DEBUG_ERROR("transfer_write failed, is_atomic:%d, reg addr:0x%x, len:%zu, ret:%d.\n",
            is_atomic, regaddr, count, ret);
        return -EIO;
    }
    return 0;
}

static long spi_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int minor_to_dev(int minor, struct spi_dev_info **spi_dev)
{
    int i;

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (spi_dev_arry[i] == NULL) {
            continue;
        }
        if (spi_dev_arry[i]->misc.minor == minor) {
            *spi_dev = spi_dev_arry[i];
            return 0;
        }
    }
    return -ENODEV;
}

static int add_dev_to_g_dev_list(struct spi_dev_info *spi_dev)
{
    int i;
    unsigned long flags;

    spin_lock_irqsave(&dev_array_lock, flags);
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (spi_dev_arry[i] == NULL) {
            spi_dev_arry[i] = spi_dev;
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
        if (spi_dev_arry[i] == NULL) {
            continue;
        }
        if (spi_dev_arry[i]->misc.minor == minor) {
            spi_dev_arry[i] = NULL;
            spin_unlock_irqrestore(&dev_array_lock, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&dev_array_lock, flags);
    return -ENODEV ;
}

static int spi_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    struct spi_dev_info *spi_dev;
    int ret;

    ret = minor_to_dev(minor, &spi_dev);
    if (ret) {
        return ret;
    }
    file->private_data = spi_dev;
    return 0;
}

static int spi_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;

    return 0;
}

static int device_read(struct spi_dev_info *spi_dev, uint32_t offset, uint8_t *buf, size_t count, bool is_atomic)
{
    int i, j, ret;
    u8 val[MAX_RW_LEN + WIDTH_4Byte];
    u32 data_width, rd_len, per_len, tmp;
    u32 max_per_len;
    u32 fix_offset, fix_count;
    u32 tmp_offset;

    if (offset >= spi_dev->spi_len) {
        DEBUG_VERBOSE("offset: 0x%x, spi len: 0x%x, count: %zu, EOF.\n",
            offset, spi_dev->spi_len, count);
        return 0;
    }

    data_width = spi_dev->data_bus_width;
    tmp_offset = offset % data_width;
    if (tmp_offset) {
        /* The offset is not aligned according to the bit width. */
        fix_offset = offset - tmp_offset;
        fix_count = count + tmp_offset;
    } else {
        /* The offset is aligned according to the bit width. */
        fix_offset = offset;
        fix_count = count;
     }

    DEBUG_VERBOSE("spi bus width:%u, offset:0x%x, count %zu.\n", data_width, offset, count);
    DEBUG_VERBOSE("spi bus tmp_offset:0x%x, fix_offset:0x%x, fix_count %u.\n", tmp_offset, fix_offset, fix_count);

    if (fix_count > spi_dev->spi_len - fix_offset) {
        DEBUG_VERBOSE("read count out of range. fix_count:%u, read len:%u.\n",
            fix_count, spi_dev->spi_len - fix_offset);
        fix_count = spi_dev->spi_len - fix_offset;
    }

    max_per_len = spi_dev->per_rd_len;
    tmp = (data_width - 1) & fix_count;
    rd_len = (tmp == 0) ? fix_count : fix_count + data_width - tmp;
    per_len = (rd_len > max_per_len) ? (max_per_len) : (rd_len);
    
    if (rd_len > sizeof(val)) {
        DEBUG_ERROR("read len is too long, rd_len:%u, val len:%zu.\n", rd_len, sizeof(val));
        return -EINVAL;
    }

    mem_clear(val, sizeof(val));
    for (i = 0; i < rd_len; i += per_len) {
        ret = transfer_read(spi_dev, val + i, fix_offset + i, per_len, is_atomic);
        if (ret < 0) {
            DEBUG_ERROR("read error.read offset = 0x%x\n", (fix_offset + i));
            return -EFAULT;
        }
    }

    if (data_width == WIDTH_1Byte) {
        if (fix_count > sizeof(val)) {
            DEBUG_ERROR("read count out of range. fix_count:%u, read len:%zu.\n",
                fix_count, sizeof(val));
            return -EINVAL;
        }
        memcpy(buf, val, fix_count);
    } else {
        for (i = 0; i < fix_count; i += data_width) {
            for (j = 0; (j < data_width) && (i + j < fix_count); j++) {
                if ((i == 0) && (tmp_offset) && (j < tmp_offset)) {
                    continue;
                }
                buf[i + j - tmp_offset] = val[i + data_width - j - 1];
            }
        }
    }

    if (spi_dev->file_cache_rd) {
        ret = cache_value_read(spi_dev->mask_file_path, spi_dev->cache_file_path, offset, buf, count);
        if (ret < 0) {
            DEBUG_ERROR("spi data offset: 0x%x, read_len: %zu, read cache file fail, ret: %d, return act value\n",
                offset, count, ret);
        } else {
            DEBUG_VERBOSE("spi data offset: 0x%x, read_len: %zu success, read from cache value\n",
                offset, count);
        }
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(spi_dev->name, offset, buf, count, true);
    }

    return count;
}

static int device_write(struct spi_dev_info *spi_dev, uint32_t offset, uint8_t *buf, size_t count, bool is_atomic)
{
    int i, j, ret;
    u32 data_width;
    u8 val[MAX_RW_LEN];
    u32 wr_len, per_len, tmp;
    u32 max_per_len;

    if (offset >= spi_dev->spi_len) {
        DEBUG_VERBOSE("offset: 0x%x, spi len: 0x%x, count: %zu, EOF.\n",
            offset, spi_dev->spi_len, count);
        return 0;
    }

    data_width = spi_dev->data_bus_width;
    if (offset % data_width) {
        DEBUG_ERROR("data bus width: %d, offset: 0x%x, read size %zu invalid.\n",
            data_width, offset, count);
        return -EINVAL;
    }

    if (count > (spi_dev->spi_len - offset)) {
        DEBUG_VERBOSE("read count out of range. input len:%zu, read len:%u.\n",
            count, spi_dev->spi_len - offset);
        count = spi_dev->spi_len - offset;
    }

    mem_clear(val, sizeof(val));

    if (data_width == WIDTH_1Byte) {
        memcpy(val, buf, count);
    } else {
        for (i = 0; i < count; i += data_width) {
            for (j = 0; (j < data_width) && (i + j < count); j++) {
                val[i + data_width - j - 1] = buf[i + j];
            }
        }
    }

    max_per_len = spi_dev->per_wr_len;
    tmp = (data_width - 1) & count;
    wr_len = (tmp == 0) ? count : count + data_width - tmp;
    per_len = (wr_len > max_per_len) ? (max_per_len) : (wr_len);

    for (i = 0; i < wr_len; i += per_len) {
        ret = transfer_write(spi_dev, val + i, offset + i, per_len, is_atomic);
        if (ret < 0) {
            DEBUG_ERROR("write error.offset = %u\n", (offset + i));
            return -EFAULT;
        }
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(spi_dev->name, offset, buf, count, false);
    }

    return count;
}

static ssize_t spi_dev_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int ret, read_len;
    struct spi_dev_info *spi_dev;

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, read failed.\n");
        return -EINVAL;
    }

    spi_dev = file->private_data;
    if (spi_dev == NULL) {
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
    read_len = device_read(spi_dev, (uint32_t)*offset, val, count, false);
    if (read_len < 0) {
        DEBUG_ERROR("spi dev read failed, dev name: %s, offset: 0x%x, len: %zu.\n",
            spi_dev->name, (uint32_t)*offset, count);
        return read_len;
    }

    if (read_len == 0) {
        DEBUG_VERBOSE("spi dev read EOF, offset: 0x%llx, count: %zu\n", *offset, count);
        return 0;
    }

    DEBUG_VERBOSE("read, buf: %p, offset: 0x%llx, read count %zu.\n", buf, *offset, count);
    memcpy(buf, val, read_len);

    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t spi_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    DEBUG_VERBOSE("spi_dev_read_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(to), iocb->ki_pos);
    return wb_iov_iter_read(iocb, to, spi_dev_read);
}

static ssize_t spi_dev_write(struct file *file, char *buf,
                   size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int write_len;
    struct spi_dev_info *spi_dev;
    char bsp_log_dev_name[BSP_LOG_DEV_NAME_MAX_LEN];
    char bsp_log_file_path[BSP_LOG_DEV_NAME_MAX_LEN];

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, read failed.\n");
        return -EINVAL;
    }

    spi_dev = file->private_data;
    if (spi_dev == NULL) {
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
    DEBUG_VERBOSE("write, buf: %p, offset: 0x%llx, write count %zu.\n", buf, *offset, count);
    memcpy(val, buf, count);

    if (spi_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Devfs]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_file_path), "%s.%s_bsp_key_reg", BSP_LOG_DIR, spi_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(spi_dev->log_node), (uint32_t)*offset, val, count);
    }

    write_len = device_write(spi_dev, (uint32_t)*offset, val, count, false);
    if (write_len < 0) {
        DEBUG_ERROR("spi dev write failed, dev name: %s, offset: 0x%llx, len: %zu, ret: %d\n",
            spi_dev->name, *offset, count, write_len);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t spi_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    DEBUG_VERBOSE("spi_dev_write_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(from), iocb->ki_pos);
    return wb_iov_iter_write(iocb, from, spi_dev_write);
}

static loff_t spi_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    struct spi_dev_info *spi_dev;

    spi_dev = file->private_data;
    if (spi_dev == NULL) {
        DEBUG_ERROR("spi_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            DEBUG_ERROR("SEEK_SET, offset: %lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > spi_dev->spi_len) {
            DEBUG_ERROR("SEEK_SET out of range, offset: %lld, spi_len: 0x%x.\n",
                offset, spi_dev->spi_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > spi_dev->spi_len) || ((file->f_pos + offset) < 0)) {
            DEBUG_ERROR("SEEK_CUR out of range, f_ops: %lld, offset: %lld\n",
                 file->f_pos, offset);
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

static const struct file_operations spi_dev_fops = {
    .owner          = THIS_MODULE,
    .llseek         = spi_dev_llseek,
    .read_iter      = spi_dev_read_iter,
    .write_iter     = spi_dev_write_iter,
    .unlocked_ioctl = spi_dev_ioctl,
    .open           = spi_dev_open,
    .release        = spi_dev_release,
};

static struct spi_dev_info *dev_match(const char *path)
{
    struct spi_dev_info * spi_dev;
    char dev_name[MAX_NAME_SIZE];
    int i;

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (spi_dev_arry[i] == NULL) {
            continue;
        }
        spi_dev = spi_dev_arry[i];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", spi_dev->name);
        if (!strcmp(path, dev_name)) {
            DEBUG_VERBOSE("get dev_name = %s, minor = %d\n", dev_name, i);
            return spi_dev;
        }
    }

    return NULL;
}

static int spi_device_func_read_tmp(const char *path, uint32_t offset, uint8_t *buf, size_t count, bool is_atomic)
{
    struct spi_dev_info *spi_dev = NULL;
    int ret;

    if(path == NULL){
        DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if(buf == NULL){
        DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    if (count > MAX_RW_LEN) {
        DEBUG_ERROR("read count %zu, beyond max:%d.\n", count, MAX_RW_LEN);
        return -EINVAL;
    }

    spi_dev = dev_match(path);
    if (spi_dev == NULL) {
        DEBUG_ERROR("spi_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    ret = device_read(spi_dev, offset, buf, count, is_atomic);
    if (ret < 0) {
        DEBUG_ERROR("spi dev read failed, dev name: %s, offset: 0x%x, len: %zu, ret: %d\n",
            spi_dev->name, offset, count, ret);
    }

    return ret;
}

int spi_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    return spi_device_func_read_tmp(path, offset, buf, count, false);
}
EXPORT_SYMBOL(spi_device_func_read);

int spi_device_func_atomic_read(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    return spi_device_func_read_tmp(path, offset, buf, count, true);
}
EXPORT_SYMBOL(spi_device_func_atomic_read);

static int spi_device_func_write_tmp(const char *path, uint32_t offset, uint8_t *buf, size_t count, bool is_atomic)
{
    struct spi_dev_info *spi_dev = NULL;
    int ret;
    char bsp_log_dev_name[BSP_LOG_DEV_NAME_MAX_LEN];
    char bsp_log_file_path[BSP_LOG_DEV_NAME_MAX_LEN];

    if(path == NULL){
        DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if(buf == NULL){
        DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    if (count > MAX_RW_LEN) {
        DEBUG_ERROR("write count %zu, beyond max:%d.\n", count, MAX_RW_LEN);
        return -EINVAL;
    }

    spi_dev = dev_match(path);
    if (spi_dev == NULL) {
        DEBUG_ERROR("spi_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    if (spi_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Symbol]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_dev_name), "%s.%s_bsp_key_reg", BSP_LOG_DIR, spi_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(spi_dev->log_node), offset, buf, count);
    }

    ret = device_write(spi_dev, offset, buf, count, is_atomic);
    if (ret < 0) {
        DEBUG_ERROR("spi dev write failed, dev name: %s, offset: 0x%x, len: %zu, ret: %d\n",
            spi_dev->name, offset, count, ret);
    }

    return ret;
}

int spi_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    return spi_device_func_write_tmp(path, offset, buf, count, false);
}
EXPORT_SYMBOL(spi_device_func_write);

int spi_device_func_atomic_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    return spi_device_func_write_tmp(path, offset, buf, count, true);
}
EXPORT_SYMBOL(spi_device_func_atomic_write);

static ssize_t spi_dev_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->show) {
        DEBUG_ERROR("spi dev attr show is null.\n");
        return -ENOSYS;
    }

    return attribute->show(kobj, attribute, buf);
}

static ssize_t spi_dev_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf,
                   size_t len)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->store) {
        DEBUG_ERROR("spi dev attr store is null.\n");
        return -ENOSYS;
    }

    return attribute->store(kobj, attribute, buf, len);
}

static const struct sysfs_ops spi_dev_sysfs_ops = {
    .show = spi_dev_attr_show,
    .store = spi_dev_attr_store,
};

static void spi_dev_obj_release(struct kobject *kobj)
{
    return;
}

static struct kobj_type spi_dev_ktype = {
    .sysfs_ops = &spi_dev_sysfs_ops,
    .release = spi_dev_obj_release,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    .default_attrs = NULL,
#endif
};

static ssize_t alias_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_spi_dev_t *spi_dev = container_of(kobj, wb_spi_dev_t, kobj);

    if (!spi_dev) {
        DEBUG_ERROR("alias show spi dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", spi_dev->alias);
}

static ssize_t type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", PROXY_NAME);
}

static ssize_t info_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int offset;
    struct spi_controller *controller;
    struct spi_device *spi;
    ssize_t buf_len;

    wb_spi_dev_t *spi_dev = container_of(kobj, wb_spi_dev_t, kobj);
    if (!spi_dev) {
        DEBUG_ERROR("info show alias_show spi dev is null.\n");
        return -ENODEV;
    }
    spi = spi_dev->spi_device;
    if (!spi) {
        DEBUG_ERROR("can't get spi device\n");
        return -ENODEV;
    }

    controller = spi->controller;
    if (!controller) {
        DEBUG_ERROR("can't get spi master\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    offset = 0;
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "name: %s\n", spi_dev->name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "alias: %s\n", spi_dev->alias);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "spi_bus: %d\n", controller->bus_num);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "chip_select: %u\n", spi->chip_select);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "max_speed_hz: %u\n", spi->max_speed_hz);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "mode: %u\n", spi->mode);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "data_bus_width:  %u\n", spi_dev->data_bus_width);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "addr_bus_width: %u\n", spi_dev->addr_bus_width);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "per_rd_len: %u\n", spi_dev->per_rd_len);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "per_wr_len: %u\n", spi_dev->per_wr_len);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "spi_len: 0x%x\n", spi_dev->spi_len);
    buf_len = strlen(buf);
    return buf_len;
}

static ssize_t status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_spi_dev_t *spi_dev;
    uint32_t type, len;
    int ret;

    spi_dev = container_of(kobj, wb_spi_dev_t, kobj);
    if (!spi_dev) {
        DEBUG_ERROR("Failed: status show param is NULL.\n");
        return -ENODEV;
    }

    type = spi_dev->status_check.status_check_type_bmp;
    /* check logic dev support type */
    if (wb_logic_status_type_get_number(type) == 0) {
        DEBUG_ERROR("unsupport dev status check.\n");
        return -EOPNOTSUPP;
    }

    if (time_before(jiffies, spi_dev->status_check.last_jiffies + msecs_to_jiffies(status_cache_ms))) {
        /* Within the time range of status_cache_ms, directly return the last result */
        DEBUG_VERBOSE("time before last time %d ms return last status: %d\n",
            status_cache_ms, spi_dev->status_check.dev_status);
        return sprintf(buf, "%u\n", spi_dev->status_check.dev_status);
    }

    spi_dev->status_check.last_jiffies = jiffies;

    len = spi_dev->data_bus_width;
    if (len > MAX_DATA_WIDTH) {
        DEBUG_ERROR("status show rw len:%u beyond max byte.\n", len);
        return -EINVAL;
    }

    mutex_lock(&spi_dev->update_lock);
    ret = wb_logic_dev_get_status(&spi_dev->status_check, len, buf, PAGE_SIZE);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_dev_get_status fail. (ret %d)\n", ret);
    }
    mutex_unlock(&spi_dev->update_lock);

    return ret;

}

static ssize_t status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_spi_dev_t *spi_dev;
    uint32_t val;
    int ret;
    uint32_t len;

    spi_dev = container_of(kobj, wb_spi_dev_t, kobj);
    if (!spi_dev) {
        DEBUG_ERROR("status_store param is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou32(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }

    len = spi_dev->data_bus_width;
    if (len > MAX_DATA_WIDTH) {
        DEBUG_ERROR("status store rw len:%u beyond max byte.\n", len);
        return -EINVAL;
    }

    DEBUG_INFO("status store len:%u val:0x%x.\n", len, val);

    mutex_lock(&spi_dev->update_lock);
    ret = wb_logic_clear_status(&spi_dev->status_check, len, val);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_clear_status fail. (ret %d)\n", ret);
    } else {
        ret = count;
    }
    mutex_unlock(&spi_dev->update_lock);

    return ret;
}

static ssize_t status_show_with_type(struct kobject *kobj, struct kobj_attribute *attr, char *buf, uint32_t type)
{
    wb_spi_dev_t *spi_dev;
    uint32_t len;
    int ret;

    spi_dev = container_of(kobj, wb_spi_dev_t, kobj);
    if (!spi_dev) {
        DEBUG_ERROR("Failed: status show param is NULL.\n");
        return -ENODEV;
    }

    /* Input parameter detection, only one type */
    if (wb_logic_status_type_get_number(type) != 1) {
        DEBUG_ERROR("unsupport dev status check. type 0x%x\n", type);
        return -EOPNOTSUPP;
    }

    ret = wb_logic_get_cache_status_with_type(&spi_dev->status_check, type, status_cache_ms, buf, PAGE_SIZE);
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

    len = spi_dev->data_bus_width;
    if (len > MAX_DATA_WIDTH) {
        DEBUG_ERROR("status show rw len:%u beyond max 4 byte.\n", len);
        return -EINVAL;
    }

    mutex_lock(&spi_dev->update_lock);
    ret = wb_logic_dev_get_status_with_type(&spi_dev->status_check, len, buf, PAGE_SIZE, type);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_dev_get_status fail. (ret %d)\n", ret);
    }
    mutex_unlock(&spi_dev->update_lock);

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
    wb_spi_dev_t *spi_dev = container_of(kobj, wb_spi_dev_t, kobj);

    if (!spi_dev) {
        DEBUG_ERROR("file_cache_rd_show spi dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%u\n", spi_dev->file_cache_rd);
}

static ssize_t file_cache_rd_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_spi_dev_t *spi_dev = container_of(kobj, wb_spi_dev_t, kobj);
    u8 val;
    int ret;

    if (!spi_dev) {
        DEBUG_ERROR("file_cache_rd_store spi dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    spi_dev->file_cache_rd = val;

    return count;
}

static ssize_t file_cache_wr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_spi_dev_t *spi_dev = container_of(kobj, wb_spi_dev_t, kobj);

    if (!spi_dev) {
        DEBUG_ERROR("file_cache_wr_show spi dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", spi_dev->file_cache_wr);
}

static ssize_t file_cache_wr_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_spi_dev_t *spi_dev = container_of(kobj, wb_spi_dev_t, kobj);
    u8 val;
    int ret;

    if (!spi_dev) {
        DEBUG_ERROR("file_cache_wr_store spi dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    spi_dev->file_cache_wr = val;

    return count;
}

static ssize_t cache_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_spi_dev_t *spi_dev = container_of(kobj, wb_spi_dev_t, kobj);

    if (!spi_dev) {
        DEBUG_ERROR("cache_file_path_show spi dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", spi_dev->cache_file_path);
}

static ssize_t mask_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_spi_dev_t *spi_dev = container_of(kobj, wb_spi_dev_t, kobj);

    if (!spi_dev) {
        DEBUG_ERROR("mask_file_path_show spi dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", spi_dev->mask_file_path);
}

static ssize_t collect_skip_flag_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_spi_dev_t *spi_dev = container_of(kobj, wb_spi_dev_t, kobj);

    if (!spi_dev) {
        DEBUG_ERROR("collect_skip_flag_show spi dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", spi_dev->collect_skip_flag);
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
static struct kobj_attribute collect_skip_flag_attribute = __ATTR(collect_skip_flag, S_IRUGO, collect_skip_flag_show, NULL);

static struct attribute *spi_dev_attrs[] = {
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
    &collect_skip_flag_attribute.attr,
    NULL,
};

static struct attribute_group spi_dev_attr_group = {
    .attrs = spi_dev_attrs,
};

static int of_spi_dev_config_init(struct spi_device *spi, struct spi_dev_info *spi_dev)
{
    int i, ret;

    ret = 0;
    ret += of_property_read_string(spi->dev.of_node, "spi_dev_name", &spi_dev->name);
    ret += of_property_read_u32(spi->dev.of_node, "data_bus_width", &spi_dev->data_bus_width);
    ret += of_property_read_u32(spi->dev.of_node, "addr_bus_width", &spi_dev->addr_bus_width);
    ret += of_property_read_u32(spi->dev.of_node, "per_rd_len", &spi_dev->per_rd_len);
    ret += of_property_read_u32(spi->dev.of_node, "per_wr_len", &spi_dev->per_wr_len);
    ret += of_property_read_u32(spi->dev.of_node, "spi_len", &spi_dev->spi_len);
    if (ret != 0) {
        dev_err(&spi->dev, "dts config error, ret: %d.\n", ret);
        return -ENXIO;
    }

    if (spi_dev->data_bus_width == 0 || spi_dev->data_bus_width > WIDTH_4Byte) {
        dev_err(&spi->dev, "Invalid data_bus_width: %u\n", spi_dev->data_bus_width);
        return -EINVAL;
    }

    if (spi_dev->addr_bus_width == 0) {
        dev_err(&spi->dev, "Invalid addr_bus_width: %u\n", spi_dev->addr_bus_width);
        return -EINVAL;
    }

    if (of_property_read_string(spi->dev.of_node, "spi_dev_alias", &spi_dev->alias)) {
        spi_dev->alias = spi_dev->name;
    }

    ret = of_dev_status_check_config_init(&spi->dev, &spi_dev->status_check, spi_dev->data_bus_width,
                                            spi_device_func_read, spi_device_func_write, spi_dev->name);
    if (ret != 0) {
        dev_err(&spi->dev, "status_check_config_init fail: %d\n", ret);
        return -EINVAL;
    }

    ret = of_property_read_u8(spi->dev.of_node, "share_high_addr_cmd_mask", &spi_dev->share_high_addr_cmd_mask);
    if (ret == 0) {
        ret += of_property_read_u8(spi->dev.of_node, "share_high_addr_rd_cmd", &spi_dev->share_high_addr_rd_cmd);
        ret += of_property_read_u8(spi->dev.of_node, "share_high_addr_wr_cmd", &spi_dev->share_high_addr_wr_cmd);
        if (ret != 0) {
            dev_err(&spi->dev, "dts config cmd share high addr error, ret:%d.\n", ret);
            return -ENXIO;
        }
    } else {
        spi_dev->share_high_addr_cmd_mask = 0;
    }

    ret = of_property_read_u32(spi->dev.of_node, "log_num", &(spi_dev->log_node.log_num));
    if (ret == 0) {
        if ((spi_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) || (spi_dev->log_node.log_num == 0)) {
            dev_err(&spi->dev, "Invalid log_num: %u\n", spi_dev->log_node.log_num);
            return -EINVAL;
        }
        ret = of_property_read_u32_array(spi->dev.of_node, "log_index", spi_dev->log_node.log_index, spi_dev->log_node.log_num);
        if (ret != 0) {
            dev_err(&spi->dev, "Failed to get log_index config, ret: %d\n", ret);
            return -EINVAL;
        }
        for (i = 0; i < spi_dev->log_node.log_num; i++) {
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, spi_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
        spi_dev->log_node.log_num = 0;
    }

    if (of_property_read_u8(spi->dev.of_node, "collect_skip_flag", &spi_dev->collect_skip_flag)) {
        DEBUG_VERBOSE("collect_skip_flag not found, using default value.\n");
        spi_dev->collect_skip_flag = 0; /* default not skip */
    }

    DEBUG_VERBOSE("spi_name: %s, spi_alias: %s, data_bus_width: %d, addr_bus_width: %d, per_rd_len: %d, per_wr_len: %d, spi_len: 0x%x\n",
        spi_dev->name, spi_dev->alias, spi_dev->data_bus_width, spi_dev->addr_bus_width, spi_dev->per_rd_len,
        spi_dev->per_wr_len, spi_dev->spi_len);

    DEBUG_VERBOSE("share_high_addr_cmd_mask: 0x%x, share_high_addr_rd_cmd: 0x%x, share_high_addr_wr_cmd: 0x%x\n",
        spi_dev->share_high_addr_cmd_mask, spi_dev->share_high_addr_rd_cmd, spi_dev->share_high_addr_wr_cmd);

    return 0;
}

static int spi_dev_config_init(struct spi_device *spi, struct spi_dev_info *spi_dev)
{
    int i;
    spi_dev_device_t *spi_dev_device;
    int ret;

    if (spi->dev.platform_data == NULL) {
        dev_err(&spi->dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }
    spi_dev_device = spi->dev.platform_data;
    spi_dev->name = spi_dev_device->spi_dev_name;
    spi_dev->data_bus_width = spi_dev_device->data_bus_width;
    spi_dev->addr_bus_width = spi_dev_device->addr_bus_width;
    spi_dev->per_rd_len = spi_dev_device->per_rd_len;
    spi_dev->per_wr_len = spi_dev_device->per_wr_len;
    spi_dev->spi_len = spi_dev_device->spi_len;
    spi_dev->collect_skip_flag = spi_dev_device->collect_skip_flag;
    if (spi_dev->data_bus_width == 0 || spi_dev->data_bus_width > WIDTH_4Byte) {
        dev_err(&spi->dev, "Invalid data_bus_width: %u\n", spi_dev->data_bus_width);
        return -EINVAL;
    }

    if (spi_dev->addr_bus_width == 0) {
        dev_err(&spi->dev, "Invalid addr_bus_width: %u\n", spi_dev->addr_bus_width);
        return -EINVAL;
    }

    if (strlen(spi_dev_device->spi_dev_alias) == 0) {
        spi_dev->alias = spi_dev_device->spi_dev_name;
    } else {
        spi_dev->alias = spi_dev_device->spi_dev_alias;
    }

    ret = platform_dev_status_check_config_init(&spi->dev, &spi_dev->status_check, spi_dev->data_bus_width, &spi_dev_device->status_check,
                                        spi_device_func_read, spi_device_func_write, spi_dev->name);
    if (ret != 0) {
        dev_err(&spi->dev, "platform status_check_config_init fail: %d\n", ret);
        return -EINVAL;
    }

    spi_dev->share_high_addr_cmd_mask = spi_dev_device->share_high_addr_cmd_mask;
    if (spi_dev->share_high_addr_cmd_mask != 0) {
        spi_dev->share_high_addr_rd_cmd = spi_dev_device->share_high_addr_rd_cmd;
        spi_dev->share_high_addr_wr_cmd = spi_dev_device->share_high_addr_wr_cmd;
    }

    spi_dev->log_node.log_num = spi_dev_device->log_num;
    if (spi_dev->log_node.log_num != 0) {
        if (spi_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) {
            dev_err(&spi->dev, "Invalid log_num: %u\n", spi_dev->log_node.log_num);
            return -EINVAL;
        }
        for (i = 0; i < spi_dev->log_node.log_num; i++) {
            spi_dev->log_node.log_index[i] = spi_dev_device->log_index[i];
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, spi_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
    }

    DEBUG_VERBOSE("spi_name: %s, spi_alias: %s, data_bus_width: %d, addr_bus_width: %d, per_rd_len: %d, per_wr_len: %d, spi_len: 0x%x\n",
        spi_dev->name, spi_dev->alias, spi_dev->data_bus_width, spi_dev->addr_bus_width, spi_dev->per_rd_len,
        spi_dev->per_wr_len, spi_dev->spi_len);

    DEBUG_VERBOSE("share_high_addr_cmd_mask: 0x%x, share_high_addr_rd_cmd: 0x%x, share_high_addr_wr_cmd: 0x%x\n",
        spi_dev->share_high_addr_cmd_mask, spi_dev->share_high_addr_rd_cmd, spi_dev->share_high_addr_wr_cmd);

    return 0;
}

static int spi_dev_probe(struct spi_device *spi)
{
    int ret;
    struct spi_dev_info *spi_dev;
    struct miscdevice *misc;

    spi_dev = devm_kzalloc(&spi->dev, sizeof(struct spi_dev_info), GFP_KERNEL);
    if (!spi_dev) {
        dev_err(&spi->dev, "devm_kzalloc error. \n");
        return -ENOMEM;
    }

    spi_set_drvdata(spi, spi_dev);
    spi_dev->spi_device = spi;

    if (spi->dev.of_node) {
        ret = of_spi_dev_config_init(spi, spi_dev);
    } else {
        ret = spi_dev_config_init(spi, spi_dev);
    }

    if (ret < 0) {
        return ret;
    }

    mutex_init(&spi_dev->update_lock);
    mutex_init(&spi_dev->log_node.file_lock);

    if ((spi_dev->per_rd_len & (spi_dev->data_bus_width - 1))
            || (spi_dev->per_wr_len & (spi_dev->data_bus_width - 1))) {
        dev_err(&spi->dev, "Invalid config per_rd_len [%u] per_wr_len [%u] data bus_width [%u], addr bus width [%u].\n",
            spi_dev->per_rd_len, spi_dev->per_wr_len, spi_dev->data_bus_width, spi_dev->addr_bus_width);
        return -EINVAL;
    }

    if ((spi_dev->spi_len == 0) || (spi_dev->spi_len & (spi_dev->data_bus_width - 1))) {
        dev_err(&spi->dev, "Invalid config spi_len %d, data bus_width %d.\n",
            spi_dev->spi_len, spi_dev->data_bus_width);
        return -EINVAL;
    }

    spi_dev->file_cache_rd = 0;
    spi_dev->file_cache_wr = 0;
    snprintf(spi_dev->cache_file_path, sizeof(spi_dev->cache_file_path), CACHE_FILE_PATH, spi_dev->name);
    snprintf(spi_dev->mask_file_path, sizeof(spi_dev->mask_file_path), MASK_FILE_PATH, spi_dev->name);

    /* creat parent dir by dev name in /sys/logic_dev */
    ret = kobject_init_and_add(&spi_dev->kobj, &spi_dev_ktype, logic_dev_kobj, "%s", spi_dev->name);
    if (ret) {
        kobject_put(&spi_dev->kobj);
        dev_err(&spi->dev, "Failed to creat parent dir: %s, ret: %d\n", spi_dev->name, ret);
        return ret;
    }

    spi_dev->sysfs_group = &spi_dev_attr_group;
    ret = sysfs_create_group(&spi_dev->kobj, spi_dev->sysfs_group);
    if (ret) {
        dev_err(&spi->dev, "Failed to create %s sysfs group, ret: %d\n", spi_dev->name, ret);
        goto remove_parent_kobj;
    }

    misc = &spi_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = spi_dev->name;
    misc->fops = &spi_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&spi->dev, "Failed to register %s device\n", misc->name);
        ret = -ENXIO;
        goto remove_sysfs_group;
    }

    ret = add_dev_to_g_dev_list(spi_dev);
    if (ret) {
        dev_err(&spi->dev, "Failed to add_dev_to_g_dev_list, ret: %d\n", ret);
        goto deregister_misc;
    }

    dev_info(&spi->dev, "register %u data_bus_width %u addr_bus_witdh 0x%x spi_len device %s minor %d with %u per_rd_len %u per_wr_len success.\n",
        spi_dev->data_bus_width, spi_dev->addr_bus_width, spi_dev->spi_len, spi_dev->name, misc->minor, spi_dev->per_rd_len, spi_dev->per_wr_len);

    dev_info(&spi->dev, "share_high_addr_cmd_mask: 0x%x, share_high_addr_rd_cmd: 0x%x, share_high_addr_wr_cmd: 0x%x\n",
        spi_dev->share_high_addr_cmd_mask, spi_dev->share_high_addr_rd_cmd, spi_dev->share_high_addr_wr_cmd);

    ret = wb_logic_check_status_hw_init(&spi_dev->status_check, spi_dev->data_bus_width);
    if (ret != 0) {
        dev_err(&spi->dev, "Failed to wb_logic_check_status_hw_init, ret: %d\n", ret);
    }

    return 0;
deregister_misc:
    misc_deregister(misc);
remove_sysfs_group:
    sysfs_remove_group(&spi_dev->kobj, (const struct attribute_group *)spi_dev->sysfs_group);
remove_parent_kobj:
    kobject_put(&spi_dev->kobj);
    return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int spi_dev_remove(struct spi_device *spi)
#else
static void spi_dev_remove(struct spi_device *spi)
#endif
{
    int minor;
    wb_spi_dev_t *spi_dev;

    spi_dev = spi_get_drvdata(spi);
    minor = spi_dev->misc.minor;

    dev_dbg(&spi->dev, "misc_deregister %s, minor: %d\n", spi_dev->misc.name, minor);
    misc_deregister(&spi_dev->misc);
    remove_dev_from_g_dev_list(minor);

    if (spi_dev->sysfs_group) {
        dev_dbg(&spi->dev, "Unregister %s spi_dev sysfs group\n", spi_dev->name);
        sysfs_remove_group(&spi_dev->kobj, (const struct attribute_group *)spi_dev->sysfs_group);
        kobject_put(&spi_dev->kobj);
        spi_dev->sysfs_group = NULL;
    }

    dev_info(&spi->dev, "Remove %s spi device success.\n", spi_dev->name);
    spi_set_drvdata(spi, NULL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    return 0;
#endif
}

static const struct of_device_id spi_dev_of_match[] = {
    { .compatible = "wb-spi-dev" },
    { },
};

MODULE_DEVICE_TABLE(of, spi_dev_of_match);

static struct spi_driver spi_dev_driver = {
    .driver = {
        .name = PROXY_NAME,
        .of_match_table = of_match_ptr(spi_dev_of_match),
    },
    .probe      = spi_dev_probe,
    .remove     = spi_dev_remove,
};

module_spi_driver(spi_dev_driver);

MODULE_DESCRIPTION("spi dev driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
