/*
 * wb_i2c_dev.c
 * ko to read/write i2c client through /dev/XXX device
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/export.h>
#include <linux/uio.h>

#include "wb_i2c_dev.h"
#include <wb_bsp_kernel_debug.h>
#include <wb_kernel_io.h>

#define PROXY_NAME "wb-i2c-dev"
#define MAX_BUS_WIDTH        (16)
#define TRANSFER_WRITE_BUFF  (MAX_RW_LEN + MAX_BUS_WIDTH)

/* Use the wb_bsp_kernel_debug header file must define debug variable */
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int status_cache_ms = 0;
module_param(status_cache_ms, int, S_IRUGO | S_IWUSR);

static struct i2c_dev_info* i2c_dev_arry[MAX_DEV_NUM];

typedef struct i2c_dev_info {
    const char *name;
    const char *alias;
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
    uint32_t i2c_len;
    struct miscdevice misc;
    struct i2c_client *client;
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
} wb_i2c_dev_t;

static int transfer_read(struct i2c_client *client, u8 *buf, loff_t regaddr, size_t count)
{
    struct i2c_adapter *adap;
    union i2c_smbus_data data;
    int i, j;
    u8 offset_buf[MAX_BUS_WIDTH];
    struct i2c_msg msgs[2];
    int msgs_num, ret;
    struct i2c_dev_info *i2c_dev;
    u8 offset;
    u8 length;

    if (!client) {
        DEBUG_ERROR("can't get read client\n");
        return -ENODEV;
    }

    adap = client->adapter;
    if (!adap) {
        DEBUG_ERROR("can't get read adap\n");
        return -ENODEV;
    }

    i2c_dev = i2c_get_clientdata(client);
    if (!i2c_dev) {
        DEBUG_ERROR("can't get read i2c_dev\n");
        return -ENODEV;
    }

    i = 0;

    mem_clear(offset_buf, sizeof(offset_buf));
    mem_clear(&data, sizeof(data));

    switch (i2c_dev->addr_bus_width) {
    case WIDTH_4Byte:
        offset_buf[i++] = (regaddr >> 24) & 0xFF;
        offset_buf[i++] = (regaddr >> 16) & 0xFF;
        offset_buf[i++] = (regaddr >> 8) & 0xFF;
        offset_buf[i++] = regaddr & 0xFF;
        break;
    case WIDTH_2Byte:
        offset_buf[i++] = (regaddr >> 8) & 0xFF;
        offset_buf[i++] = regaddr & 0xFF;
        break;
    case WIDTH_1Byte:
        offset_buf[i++] = regaddr & 0xFF;
        break;
    default:
        DEBUG_ERROR("Only support 1,2,4 Byte Address Width,but set width = %u\n",
            i2c_dev->addr_bus_width);
        return -EINVAL;
    }

    if (adap->algo->master_xfer) {
        mem_clear(msgs, sizeof(msgs));
        msgs[0].addr = client->addr;
        msgs[0].flags = 0;
        msgs[0].len = i2c_dev->addr_bus_width;
        msgs[0].buf = offset_buf;

        msgs[1].addr = client->addr;
        msgs[1].flags = I2C_M_RD;
        msgs[1].len = count;
        msgs[1].buf = buf;

        msgs_num = 2;
        if (in_interrupt() || oops_in_progress) {
            ret = __i2c_transfer(client->adapter, msgs, msgs_num);
        } else {
            ret = i2c_transfer(client->adapter, msgs, msgs_num);
        }
        if (ret != msgs_num) {
            DEBUG_ERROR("i2c_transfer read error\n");
            return -EINVAL;
        }
    } else {
        if (i2c_dev->addr_bus_width == WIDTH_1Byte) {
            offset = regaddr & 0xFF;
            if (i2c_check_functionality(adap, I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
                for (j = 0; j < count; j += I2C_SMBUS_BLOCK_MAX) {
                    if (count - j > I2C_SMBUS_BLOCK_MAX) {
                        length = I2C_SMBUS_BLOCK_MAX;
                    } else {
                        length = count - j;
                    }
                    data.block[0] = length;
                    ret = adap->algo->smbus_xfer(adap, client->addr,
                                    0,
                                    I2C_SMBUS_READ,
                                    offset, I2C_SMBUS_I2C_BLOCK_DATA, &data);
                    if (ret) {
                        DEBUG_ERROR("smbus_xfer read block error, ret = %d\n", ret);
                        return -EFAULT;
                    }
                    memcpy(buf + j, data.block + 1, length);
                    offset += length;
                }
            } else {
                for (j = 0; j < count; j++) {
                    ret = adap->algo->smbus_xfer(adap, client->addr,
                                    0,
                                    I2C_SMBUS_READ,
                                    offset, I2C_SMBUS_BYTE_DATA, &data);

                    if (!ret) {
                        buf[j] = data.byte;
                    } else {
                        DEBUG_ERROR("smbus_xfer read byte error, ret = %d\n", ret);
                        return -EFAULT;
                    }
                    offset++;
                }
            }
        } else {
            DEBUG_ERROR("smbus_xfer not support addr_bus_width = %d\n", i2c_dev->addr_bus_width);
            return -EINVAL;
        }
    }
    return 0;
}

static int transfer_write(struct i2c_client *client, u8 *buf, loff_t regaddr, size_t count)
{
    struct i2c_adapter *adap;
    int i;
    u8 offset_buf[TRANSFER_WRITE_BUFF];
    struct i2c_msg msgs[1];
    int msgs_num, ret;
    struct i2c_dev_info *i2c_dev;

    if (!client) {
        DEBUG_ERROR("can't get write client\n");
        return -ENODEV;
    }

    adap = client->adapter;
    if (!adap) {
        DEBUG_ERROR("can't get write adap\n");
        return -ENODEV;
    }

    i2c_dev = i2c_get_clientdata(client);
    if (!i2c_dev) {
        DEBUG_ERROR("can't get read i2c_dev\n");
        return -ENODEV;
    }

    i = 0;

    mem_clear(offset_buf, sizeof(offset_buf));

    switch (i2c_dev->addr_bus_width) {
    case WIDTH_4Byte:
        offset_buf[i++] = (regaddr >> 24) & 0xFF;
        offset_buf[i++] = (regaddr >> 16) & 0xFF;
        offset_buf[i++] = (regaddr >> 8) & 0xFF;
        offset_buf[i++] = regaddr & 0xFF;
        break;
    case WIDTH_2Byte:
        offset_buf[i++] = (regaddr >> 8) & 0xFF;
        offset_buf[i++] = regaddr & 0xFF;
        break;
    case WIDTH_1Byte:
        offset_buf[i++] = regaddr & 0xFF;
        break;
    default:
        DEBUG_ERROR("Only support 1,2,4 Byte Address Width,but set width = %u\n",
            i2c_dev->addr_bus_width);
        return -EINVAL;
    }

    memcpy(offset_buf + i2c_dev->addr_bus_width, buf, count);

    if (adap->algo->master_xfer) {
        mem_clear(msgs, sizeof(msgs));

        msgs[0].addr = client->addr;
        msgs[0].flags = 0;
        msgs[0].len = i2c_dev->addr_bus_width + count;
        msgs[0].buf = offset_buf;

        msgs_num = 1;
        if (in_interrupt() || oops_in_progress) {
            ret = __i2c_transfer(adap, msgs, msgs_num);
        } else {
            ret = i2c_transfer(adap, msgs, msgs_num);
        }
        if (ret != msgs_num) {
            DEBUG_ERROR("i2c_transfer write error\n");
            return -EINVAL;
        }
    } else {
        DEBUG_ERROR("don't find write master_xfer\n");
        return -EINVAL;
    }

    return 0;
}

static long i2c_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int i2c_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    struct i2c_dev_info *i2c_dev;

    if (minor >= MAX_DEV_NUM) {
        DEBUG_ERROR("minor [%d] is greater than max dev num [%d], open fail\n", minor, MAX_DEV_NUM);
        return -ENODEV;
    }

    i2c_dev = i2c_dev_arry[minor];
    if (i2c_dev == NULL) {
        return -ENODEV;
    }

    file->private_data = i2c_dev;

    return 0;
}

static int i2c_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;

    return 0;
}

static int device_read(struct i2c_dev_info *i2c_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, j, ret;
    u8 tmp_offset;
    u8 val[MAX_RW_LEN];
    u32 width, rd_len, per_len, tmp;
    u32 max_per_len;

    if (offset >= i2c_dev->i2c_len) {
        DEBUG_VERBOSE("offset: 0x%x, i2c len: 0x%x, count: %zu, EOF.\n",
            offset, i2c_dev->i2c_len, count);
        return 0;
    }

    if (count > (i2c_dev->i2c_len - offset)) {
        DEBUG_VERBOSE("read count out of range. input len:%zu, read len:%u.\n",
            count, i2c_dev->i2c_len - offset);
        count = i2c_dev->i2c_len - offset;
    }

    width = i2c_dev->data_bus_width;
    switch (width) {
    case WIDTH_4Byte:
        tmp_offset = offset & 0x3;
        if (tmp_offset) {
            DEBUG_ERROR("data bus width: %u, offset: 0x%x, read size %zu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_2Byte:
        tmp_offset = offset & 0x1;
        if (tmp_offset) {
            DEBUG_ERROR("data bus width: %u, offset: 0x%x, read size %zu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_1Byte:
        break;
    default:
        DEBUG_ERROR("Only support 1,2,4 Byte Data Width,but set width = %u\n", width);
        return -EINVAL;
    }

    max_per_len = i2c_dev->per_rd_len;
    tmp = (width - 1) & count;
    rd_len = (tmp == 0) ? count : count + width - tmp;
    per_len = (rd_len > max_per_len) ? (max_per_len) : (rd_len);

    mem_clear(val, sizeof(val));
    for (i = 0; i < rd_len; i += per_len) {
        ret = transfer_read(i2c_dev->client, val + i, offset + i, per_len);
        if (ret < 0) {
            DEBUG_ERROR("read error.read offset = %u\n", (offset + i));
            return -EFAULT;
        }
    }

    if (width == WIDTH_1Byte) {
        memcpy(buf, val, count);
    } else {
        for (i = 0; i < count; i += width) {
            for (j = 0; (j < width) && (i + j < count); j++) {
                buf[i + j] = val[i + width - j - 1];
            }
        }
    }

    if (i2c_dev->file_cache_rd) {
        ret = cache_value_read(i2c_dev->mask_file_path, i2c_dev->cache_file_path, offset, buf, count);
        if (ret < 0) {
            DEBUG_ERROR("i2c data offset: 0x%x, read_len: %zu, read cache file fail, ret: %d, return act value\n",
                offset, count, ret);
        } else {
            DEBUG_VERBOSE("i2c data offset: 0x%x, read_len: %zu success, read from cache value\n",
                offset, count);
        }
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(i2c_dev->name, offset, buf, count, true);
    }

    return count;
}

static int device_write(struct i2c_dev_info *i2c_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, j, ret;
    u8 tmp_offset;
    u32 width;
    u8 val[MAX_RW_LEN];
    u32 wr_len, per_len, tmp;
    u32 max_per_len;

    if (offset >= i2c_dev->i2c_len) {
        DEBUG_VERBOSE("offset: 0x%x, i2c len: 0x%x, count: %zu, EOF.\n",
            offset, i2c_dev->i2c_len, count);
        return 0;
    }

    if (count > (i2c_dev->i2c_len - offset)) {
        DEBUG_VERBOSE("read count out of range. input len:%zu, read len:%u.\n",
            count, i2c_dev->i2c_len - offset);
        count = i2c_dev->i2c_len - offset;
    }

    width = i2c_dev->data_bus_width;
    switch (width) {
    case WIDTH_4Byte:
        tmp_offset = offset & 0x3;
        if (tmp_offset) {
            DEBUG_ERROR("data bus width: %u, offset: 0x%x, read size %zu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_2Byte:
        tmp_offset = offset & 0x1;
        if (tmp_offset) {
            DEBUG_ERROR("data bus width: %u, offset: 0x%x, read size %zu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_1Byte:
        break;
    default:
        DEBUG_ERROR("Only support 1,2,4 Byte Data Width,but set width = %u\n", width);
        return -EINVAL;
    }

    mem_clear(val, sizeof(val));

    if (width == WIDTH_1Byte) {
        memcpy(val, buf, count);
    } else {
        for (i = 0; i < count; i += width) {
            for (j = 0; (j < width) && (i + j < count); j++) {
                val[i + width - j - 1] = buf[i + j];
            }
        }
    }

    max_per_len = i2c_dev->per_wr_len;
    tmp = (width - 1) & count;
    wr_len = (tmp == 0) ? count : count + width - tmp;
    per_len = (wr_len > max_per_len) ? (max_per_len) : (wr_len);

    for (i = 0; i < wr_len; i += per_len) {
        ret = transfer_write(i2c_dev->client, val + i, offset + i, per_len);
        if (ret < 0) {
            DEBUG_ERROR("write error.offset = %u\n", (offset + i));
            return -EFAULT;
        }
    }

    if (debug & DEBUG_DUMP_DATA_LEVEL) {
        logic_dev_dump_data(i2c_dev->name, offset, buf, count, false);
    }

    return count;
}

static ssize_t i2c_dev_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int ret, read_len;
    struct i2c_dev_info *i2c_dev;

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, read failed.\n");
        return -EINVAL;
    }

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
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
    read_len = device_read(i2c_dev, (uint32_t)*offset, val, count);
    if (read_len < 0) {
        DEBUG_ERROR("i2c dev read failed, dev name: %s, offset: 0x%x, len: %zu.\n",
            i2c_dev->name, (uint32_t)*offset, count);
        return read_len;
    }

    if (read_len == 0) {
        DEBUG_VERBOSE("i2c dev read EOF, offset: 0x%llx, count: %zu\n", *offset, count);
        return 0;
    }

    DEBUG_VERBOSE("read, buf: %p, offset: 0x%llx, read count %zu.\n", buf, *offset, count);
    memcpy(buf, val, read_len);

    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t i2c_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    DEBUG_VERBOSE("i2c_dev_read_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(to), iocb->ki_pos);
    return wb_iov_iter_read(iocb, to, i2c_dev_read);
}

static ssize_t i2c_dev_write(struct file *file, char *buf, size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int write_len;
    struct i2c_dev_info *i2c_dev;
    char bsp_log_dev_name[BSP_LOG_DEV_NAME_MAX_LEN];
    char bsp_log_file_path[BSP_LOG_DEV_NAME_MAX_LEN];

    if (offset == NULL || *offset < 0) {
        DEBUG_ERROR("offset invalid, write failed.\n");
        return -EINVAL;
    }

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
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

    if (i2c_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Devfs]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_dev_name), "%s.%s_bsp_key_reg", BSP_LOG_DIR, i2c_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(i2c_dev->log_node), (uint32_t)*offset, val, count);
    }

    write_len = device_write(i2c_dev, (uint32_t)*offset, val, count);
    if (write_len < 0) {
        DEBUG_ERROR("i2c dev write failed, dev name: %s, offset: 0x%llx, len: %zu, ret: %d\n",
            i2c_dev->name, *offset, count, write_len);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t i2c_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    DEBUG_VERBOSE("i2c_dev_write_iter, file: %p, count: %zu, offset: %lld\n",
        iocb->ki_filp, iov_iter_count(from), iocb->ki_pos);
    return wb_iov_iter_write(iocb, from, i2c_dev_write);
}

static loff_t i2c_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    struct i2c_dev_info *i2c_dev;

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
        DEBUG_ERROR("i2c_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            DEBUG_ERROR("SEEK_SET, offset: %lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > i2c_dev->i2c_len) {
            DEBUG_ERROR("SEEK_SET out of range, offset: %lld, i2c_len:0x%x.\n",
                offset, i2c_dev->i2c_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > i2c_dev->i2c_len) || ((file->f_pos + offset) < 0)) {
            DEBUG_ERROR("SEEK_CUR out of range, f_ops: %lld, offset: %lld, i2c_len:0x%x.\n",
                file->f_pos, offset, i2c_dev->i2c_len);
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

static const struct file_operations i2c_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = i2c_dev_llseek,
    .read_iter     = i2c_dev_read_iter,
    .write_iter    = i2c_dev_write_iter,
    .unlocked_ioctl = i2c_dev_ioctl,
    .open       = i2c_dev_open,
    .release    = i2c_dev_release,
};

static struct i2c_dev_info *dev_match(const char *path)
{
    struct i2c_dev_info * i2c_dev;
    char dev_name[MAX_NAME_SIZE];
    int i;

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (i2c_dev_arry[i] == NULL) {
            continue;
        }
        i2c_dev = i2c_dev_arry[i];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", i2c_dev->name);
        if (!strcmp(path, dev_name)) {
            DEBUG_VERBOSE("get dev_name = %s, minor = %d\n", dev_name, i);
            return i2c_dev;
        }
    }

    return NULL;
}

int i2c_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct i2c_dev_info *i2c_dev = NULL;
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

    i2c_dev = dev_match(path);
    if (i2c_dev == NULL) {
        DEBUG_ERROR("i2c_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    ret = device_read(i2c_dev, offset, buf, count);
    if (ret < 0) {
        DEBUG_ERROR("i2c dev read failed, dev name: %s, offset: 0x%x, len: %zu, ret: %d\n",
            i2c_dev->name, offset, count, ret);
    }

    return ret;
}
EXPORT_SYMBOL(i2c_device_func_read);

int i2c_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct i2c_dev_info *i2c_dev = NULL;
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

    i2c_dev = dev_match(path);
    if (i2c_dev == NULL) {
        DEBUG_ERROR("i2c_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    if (i2c_dev->log_node.log_num > 0) {
        mem_clear(bsp_log_dev_name, sizeof(bsp_log_dev_name));
        mem_clear(bsp_log_file_path, sizeof(bsp_log_file_path));
        snprintf(bsp_log_dev_name, sizeof(bsp_log_dev_name), "[Symbol]");
        snprintf(bsp_log_file_path, sizeof(bsp_log_dev_name), "%s.%s_bsp_key_reg", BSP_LOG_DIR, i2c_dev->name);
        (void)wb_bsp_key_device_log(bsp_log_dev_name, bsp_log_file_path, WB_BSP_LOG_MAX,
                &(i2c_dev->log_node), offset, buf, count);
    }

    ret = device_write(i2c_dev, offset, buf, count);
    if (ret < 0) {
        DEBUG_ERROR("i2c dev write failed, dev name: %s, offset: 0x%x, len: %zu, ret: %d\n",
            i2c_dev->name, offset, count, ret);
    }

    return ret;
}
EXPORT_SYMBOL(i2c_device_func_write);

static ssize_t i2c_dev_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->show) {
        DEBUG_ERROR("i2c dev attr show is null.\n");
        return -ENOSYS;
    }

    return attribute->show(kobj, attribute, buf);
}

static ssize_t i2c_dev_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf,
                   size_t len)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->store) {
        DEBUG_ERROR("i2c dev attr store is null.\n");
        return -ENOSYS;
    }

    return attribute->store(kobj, attribute, buf, len);
}

static const struct sysfs_ops i2c_dev_sysfs_ops = {
    .show = i2c_dev_attr_show,
    .store = i2c_dev_attr_store,
};

static void i2c_dev_obj_release(struct kobject *kobj)
{
    return;
}

static struct kobj_type i2c_dev_ktype = {
    .sysfs_ops = &i2c_dev_sysfs_ops,
    .release = i2c_dev_obj_release,
};

static ssize_t alias_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_i2c_dev_t *i2c_dev = container_of(kobj, wb_i2c_dev_t, kobj);

    if (!i2c_dev) {
        DEBUG_ERROR("alias show i2c dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", i2c_dev->alias);
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

    wb_i2c_dev_t *i2c_dev = container_of(kobj, wb_i2c_dev_t, kobj);
    if (!i2c_dev) {
        DEBUG_ERROR("info show alias_show i2c dev is null.\n");
        return -ENODEV;
    }

    if (!i2c_dev->client) {
        DEBUG_ERROR("can't get read client\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    offset = 0;
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "name: %s\n", i2c_dev->name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "alias: %s\n", i2c_dev->alias);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "i2c_bus: %d\n", i2c_dev->client->adapter->nr);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "i2c_addr: 0x%x\n", i2c_dev->client->addr);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "data_bus_width: %u\n", i2c_dev->data_bus_width);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "addr_bus_width: %u\n", i2c_dev->addr_bus_width);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "per_rd_len: %u\n", i2c_dev->per_rd_len);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "per_wr_len: %u\n", i2c_dev->per_wr_len);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "i2c_len: 0x%x\n", i2c_dev->i2c_len);
    buf_len = strlen(buf);
    return buf_len;
}

static int rw_status_check_one_time(wb_i2c_dev_t *i2c_dev, uint32_t offset, uint32_t len, uint32_t type)
{
    uint8_t wr_buf[WIDTH_4Byte];
    uint8_t rd_buf[WIDTH_4Byte];
    int ret, i;

    if (len > sizeof(wr_buf) || len > sizeof(rd_buf)) {
        DEBUG_ERROR("input param error: len:%d out of range.\n", len);
        return -EINVAL;
    }

    for (i = 0; i < len; i++) {
        wr_buf[i] = i2c_dev->test_data[i];
    }

    ret = device_write(i2c_dev, offset, wr_buf, len);
    if ((ret < 0) || (ret != len)) {
        DEBUG_ERROR("STATUS_NOT_OK dev status wr offset: 0x%x, len: %u failed, ret: %d.\n",
            offset, len, ret);
        return -EIO;
    }

    mem_clear(rd_buf, sizeof(rd_buf));
    ret = device_read(i2c_dev, offset, rd_buf, len);
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

static int rw_status_check(wb_i2c_dev_t *i2c_dev, uint32_t offset, uint32_t len, uint32_t type)
{
    int ret, i;

    ret = 0;
    for (i = 0; i < LOGIC_DEV_RETRY_TIME; i++) {
        ret = rw_status_check_one_time(i2c_dev, offset, len, type);
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
    wb_i2c_dev_t *i2c_dev;
    uint32_t offset, type, len;
    int ret, i;

    i2c_dev = container_of(kobj, wb_i2c_dev_t, kobj);
    if (!i2c_dev) {
        DEBUG_ERROR("Failed: status show param is NULL.\n");
        return -ENODEV;
    }

    type = i2c_dev->status_check_type;

    if (type == NOT_SUPPORT_CHECK_STATUS) {
        DEBUG_ERROR("unsupport dev status check.\n");
        return -EOPNOTSUPP;
    }

    if (time_before(jiffies, i2c_dev->last_jiffies + msecs_to_jiffies(status_cache_ms))) {
        /* Within the time range of status_cache_ms, directly return the last result */
        DEBUG_VERBOSE("time before last time %d ms return last status: %d\n",
            status_cache_ms, i2c_dev->dev_status);
        return sprintf(buf, "%u\n", i2c_dev->dev_status);
    }

    i2c_dev->last_jiffies = jiffies;

    len = i2c_dev->data_bus_width;
    if (len > WIDTH_4Byte) {
        DEBUG_ERROR("status show rw len:%u beyond max 4 byte.\n", len);
        return -EINVAL;
    }

    mutex_lock(&i2c_dev->update_lock);

    for (i = 0; i < len; i++) {
        /* reverse to ensure that different data is read and written each time the verification is performed.*/
        i2c_dev->test_data[i] = ~i2c_dev->test_data[i];
    }
    for (i = 0; i < i2c_dev->test_reg_num; i++) {
        offset = i2c_dev->test_reg[i];
        ret = rw_status_check(i2c_dev, offset, len, type);
        if (ret < 0) {
            i2c_dev->dev_status = LOGIC_DEV_STATUS_NOT_OK;
            DEBUG_ERROR("STATUS_NOT_OK result check all retry failed.\n");
            mutex_unlock(&i2c_dev->update_lock);
            return sprintf(buf, "%u\n", i2c_dev->dev_status);
        }
    }

    i2c_dev->dev_status = LOGIC_DEV_STATUS_OK;
    mutex_unlock(&i2c_dev->update_lock);
    return sprintf(buf, "%u\n", i2c_dev->dev_status);
}

static ssize_t file_cache_rd_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_i2c_dev_t *i2c_dev = container_of(kobj, wb_i2c_dev_t, kobj);

    if (!i2c_dev) {
        DEBUG_ERROR("file_cache_rd_show i2c dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_dev->file_cache_rd);
}

static ssize_t file_cache_rd_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_i2c_dev_t *i2c_dev = container_of(kobj, wb_i2c_dev_t, kobj);
    u8 val;
    int ret;

    if (!i2c_dev) {
        DEBUG_ERROR("file_cache_rd_store i2c dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    i2c_dev->file_cache_rd = val;

    return count;
}

static ssize_t file_cache_wr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_i2c_dev_t *i2c_dev = container_of(kobj, wb_i2c_dev_t, kobj);

    if (!i2c_dev) {
        DEBUG_ERROR("file_cache_wr_show i2c dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", i2c_dev->file_cache_wr);
}

static ssize_t file_cache_wr_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_i2c_dev_t *i2c_dev = container_of(kobj, wb_i2c_dev_t, kobj);
    u8 val;
    int ret;

    if (!i2c_dev) {
        DEBUG_ERROR("file_cache_wr_store i2c dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    i2c_dev->file_cache_wr = val;

    return count;
}

static ssize_t cache_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_i2c_dev_t *i2c_dev = container_of(kobj, wb_i2c_dev_t, kobj);

    if (!i2c_dev) {
        DEBUG_ERROR("cache_file_path_show i2c dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", i2c_dev->cache_file_path);
}

static ssize_t mask_file_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_i2c_dev_t *i2c_dev = container_of(kobj, wb_i2c_dev_t, kobj);

    if (!i2c_dev) {
        DEBUG_ERROR("mask_file_path_show i2c dev is null.\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", i2c_dev->mask_file_path);
}

static struct kobj_attribute alias_attribute = __ATTR(alias, S_IRUGO, alias_show, NULL);
static struct kobj_attribute type_attribute = __ATTR(type, S_IRUGO, type_show, NULL);
static struct kobj_attribute info_attribute = __ATTR(info, S_IRUGO, info_show, NULL);
static struct kobj_attribute status_attribute = __ATTR(status, S_IRUGO, status_show, NULL);
static struct kobj_attribute file_cache_rd_attribute = __ATTR(file_cache_rd, S_IRUGO  | S_IWUSR, file_cache_rd_show, file_cache_rd_store);
static struct kobj_attribute file_cache_wr_attribute = __ATTR(file_cache_wr, S_IRUGO  | S_IWUSR, file_cache_wr_show, file_cache_wr_store);
static struct kobj_attribute cache_file_path_attribute = __ATTR(cache_file_path, S_IRUGO, cache_file_path_show, NULL);
static struct kobj_attribute mask_file_path_attribute = __ATTR(mask_file_path, S_IRUGO, mask_file_path_show, NULL);

static struct attribute *i2c_dev_attrs[] = {
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

static struct attribute_group i2c_dev_attr_group = {
    .attrs = i2c_dev_attrs,
};

static int of_i2c_dev_config_init(struct i2c_client *client, struct i2c_dev_info *i2c_dev)
{
    int i, ret;
    uint32_t type;

    ret = 0;
    ret += of_property_read_string(client->dev.of_node, "i2c_name", &i2c_dev->name);
    ret += of_property_read_u32(client->dev.of_node, "data_bus_width", &i2c_dev->data_bus_width);
    ret += of_property_read_u32(client->dev.of_node, "addr_bus_width", &i2c_dev->addr_bus_width);
    ret += of_property_read_u32(client->dev.of_node, "per_rd_len", &i2c_dev->per_rd_len);
    ret += of_property_read_u32(client->dev.of_node, "per_wr_len", &i2c_dev->per_wr_len);
    ret += of_property_read_u32(client->dev.of_node, "i2c_len", &i2c_dev->i2c_len);
    if (ret != 0) {
        dev_err(&client->dev, "dts config error, ret:%d.\n", ret);
        return -ENXIO;
    }

    if (i2c_dev->data_bus_width == 0) {
        dev_err(&client->dev, "Invalid data_bus_width: %u\n", i2c_dev->data_bus_width);
        return -EINVAL;
    }

    if (i2c_dev->addr_bus_width == 0) {
        dev_err(&client->dev, "Invalid addr_bus_width: %u\n", i2c_dev->addr_bus_width);
        return -EINVAL;
    }

    if (of_property_read_string(client->dev.of_node, "i2c_alias", &i2c_dev->alias)) {
        i2c_dev->alias = i2c_dev->name;
    }

    ret = of_property_read_u32(client->dev.of_node, "status_check_type", &i2c_dev->status_check_type);
    if (ret == 0) {
        type = i2c_dev->status_check_type;
        if ((type != READ_BACK_CHECK) && (type != READ_BACK_NAGATIVE_CHECK)) {
            dev_err(&client->dev, "Invalid status_check_type: %u\n", type);
            return -EINVAL;
        }
        DEBUG_VERBOSE("status_check_type: %u\n", i2c_dev->status_check_type);

        ret = of_property_read_u32(client->dev.of_node, "test_reg_num", &i2c_dev->test_reg_num);
        if (ret != 0) {
            dev_err(&client->dev, "Failed to get test_reg_num config, ret: %d\n", ret);
            return -ENXIO;
        }
        if ((i2c_dev->test_reg_num == 0) || (i2c_dev->test_reg_num > TEST_REG_MAX_NUM)) {
            dev_err(&client->dev, "Invalid test_reg_num: %u\n", i2c_dev->test_reg_num);
            return -EINVAL;
        }

        ret = of_property_read_u32_array(client->dev.of_node, "test_reg", i2c_dev->test_reg, i2c_dev->test_reg_num);
        if(ret != 0) {
            dev_err(&client->dev, "Failed to get test_reg config, ret: %d\n", ret);
            return -ENXIO;
        }

        i2c_dev->last_jiffies = jiffies;
        i2c_dev->dev_status = LOGIC_DEV_STATUS_OK;

        for (i = 0; i < WIDTH_4Byte; i++) {
            i2c_dev->test_data[i] = INIT_TEST_DATA;
        }

        for (i = 0; i < i2c_dev->test_reg_num; i++) {
            DEBUG_VERBOSE("test_reg[%d] address = 0x%x.\n", i, i2c_dev->test_reg[i]);
        }
    } else {
        i2c_dev->status_check_type = NOT_SUPPORT_CHECK_STATUS;
        DEBUG_VERBOSE("not support dev status check sysfs.\n");
    }

    ret = of_property_read_u32(client->dev.of_node, "log_num", &(i2c_dev->log_node.log_num));
    if (ret == 0) {
        if ((i2c_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) || (i2c_dev->log_node.log_num == 0)) {
            dev_err(&client->dev, "Invalid log_num: %u\n", i2c_dev->log_node.log_num);
            return -EINVAL;
        }
        ret = of_property_read_u32_array(client->dev.of_node, "log_index", i2c_dev->log_node.log_index, i2c_dev->log_node.log_num);
        if (ret != 0) {
            dev_err(&client->dev, "Failed to get log_index config, ret: %d\n", ret);
            return -EINVAL;
        }
        for (i = 0; i < i2c_dev->log_node.log_num; i++) {
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, i2c_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
        i2c_dev->log_node.log_num = 0;
    }

    DEBUG_VERBOSE("i2c_name: %s, i2c_alias: %s, data_bus_width: %d, addr_bus_width: %d, per_rd_len: %d, per_wr_len: %d, i2c_len: 0x%x\n",
        i2c_dev->name, i2c_dev->alias, i2c_dev->data_bus_width, i2c_dev->addr_bus_width, i2c_dev->per_rd_len,
        i2c_dev->per_wr_len, i2c_dev->i2c_len);
    return 0;
}


static int i2c_dev_config_init(struct i2c_client *client, struct i2c_dev_info *i2c_dev)
{
    int i;
    uint32_t type;
    i2c_dev_device_t *i2c_dev_device;

    if (client->dev.platform_data == NULL) {
        dev_err(&client->dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }

    i2c_dev_device = client->dev.platform_data;
    i2c_dev->name = i2c_dev_device->i2c_name;
    i2c_dev->data_bus_width = i2c_dev_device->data_bus_width;
    i2c_dev->addr_bus_width = i2c_dev_device->addr_bus_width;
    i2c_dev->per_rd_len = i2c_dev_device->per_rd_len;
    i2c_dev->per_wr_len = i2c_dev_device->per_wr_len;
    i2c_dev->i2c_len = i2c_dev_device->i2c_len;
    if (i2c_dev->data_bus_width == 0) {
        dev_err(&client->dev, "Invalid data_bus_width: %u\n", i2c_dev->data_bus_width);
        return -EINVAL;
    }

    if (i2c_dev->addr_bus_width == 0) {
        dev_err(&client->dev, "Invalid addr_bus_width: %u\n", i2c_dev->addr_bus_width);
        return -EINVAL;
    }

    if (strlen(i2c_dev_device->i2c_alias) == 0) {
        i2c_dev->alias = i2c_dev_device->i2c_name;
    } else {
        i2c_dev->alias = i2c_dev_device->i2c_alias;
    }

    i2c_dev->status_check_type = i2c_dev_device->status_check_type;
    if (i2c_dev->status_check_type != NOT_SUPPORT_CHECK_STATUS) {
        type = i2c_dev->status_check_type;
        if ((type != READ_BACK_CHECK) && (type != READ_BACK_NAGATIVE_CHECK)) {
            dev_err(&client->dev, "Invalid status_check_type: %u\n", type);
            return -EINVAL;
        }
        DEBUG_VERBOSE("status_check_type: %u\n", i2c_dev->status_check_type);

        i2c_dev->test_reg_num = i2c_dev_device->test_reg_num;
        if ((i2c_dev->test_reg_num == 0) || (i2c_dev->test_reg_num > TEST_REG_MAX_NUM)) {
            dev_err(&client->dev, "Invalid test_reg_num: %u\n", i2c_dev->test_reg_num);
            return -EINVAL;
        }
        for (i = 0; i < i2c_dev->test_reg_num; i++) {
            i2c_dev->test_reg[i] = i2c_dev_device->test_reg[i];
            DEBUG_VERBOSE("test_reg[%d] address = 0x%x.\n", i, i2c_dev->test_reg[i]);
        }

        i2c_dev->last_jiffies = jiffies;
        i2c_dev->dev_status = LOGIC_DEV_STATUS_OK;
        for (i = 0; i < WIDTH_4Byte; i++) {
            i2c_dev->test_data[i] = INIT_TEST_DATA;
        }
    } else {
        DEBUG_VERBOSE("not support dev status check sysfs.\n");
    }

    i2c_dev->log_node.log_num = i2c_dev_device->log_num;
    if (i2c_dev->log_node.log_num != 0) {
        if (i2c_dev->log_node.log_num > BSP_KEY_DEVICE_NUM_MAX) {
            dev_err(&client->dev, "Invalid log_num: %u\n", i2c_dev->log_node.log_num);
            return -EINVAL;
        }
        for (i = 0; i < i2c_dev->log_node.log_num; i++) {
            i2c_dev->log_node.log_index[i] = i2c_dev_device->log_index[i];
            DEBUG_VERBOSE("log_index[%d] address = 0x%x\n", i, i2c_dev->log_node.log_index[i]);
        }
    } else {
        DEBUG_VERBOSE("Don't support bsp key record.\n");
    }

    DEBUG_VERBOSE("i2c_name: %s, i2c_alias: %s, data_bus_width: %d, addr_bus_width: %d, per_rd_len: %d, per_wr_len: %d, i2c_len: 0x%x\n",
        i2c_dev->name, i2c_dev->alias, i2c_dev->data_bus_width, i2c_dev->addr_bus_width, i2c_dev->per_rd_len,
        i2c_dev->per_wr_len, i2c_dev->i2c_len);
    return 0;
}

static int i2c_dev_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_dev_info *i2c_dev;
    struct miscdevice *misc;
    int ret;

    i2c_dev = devm_kzalloc(&client->dev, sizeof(struct i2c_dev_info), GFP_KERNEL);
    if (!i2c_dev) {
        dev_err(&client->dev, "devm_kzalloc error. \n");
        return -ENOMEM;
    }

    i2c_set_clientdata(client, i2c_dev);
    i2c_dev->client = client;

    if (client->dev.of_node) {
        ret = of_i2c_dev_config_init(client, i2c_dev);
    } else {
        ret = i2c_dev_config_init(client, i2c_dev);
    }

    if (ret < 0) {
        return ret;
    }

    mutex_init(&i2c_dev->update_lock);
    mutex_init(&i2c_dev->log_node.file_lock);

    if ((i2c_dev->per_rd_len & (i2c_dev->data_bus_width - 1)) ||
        (i2c_dev->per_wr_len & (i2c_dev->data_bus_width - 1))) {
        dev_err(&client->dev, "Invalid config per_rd_len %d per_wr_len %d data bus_width %d.\n",
            i2c_dev->per_rd_len, i2c_dev->per_wr_len, i2c_dev->data_bus_width);
        return -EINVAL;
    }

    if ((i2c_dev->i2c_len == 0) || (i2c_dev->i2c_len & (i2c_dev->data_bus_width - 1))) {
        dev_err(&client->dev, "Invalid config i2c_len %d, data bus_width %d.\n",
            i2c_dev->i2c_len, i2c_dev->data_bus_width);
        return -EINVAL;
    }
    i2c_dev->file_cache_rd = 0;
    i2c_dev->file_cache_wr = 0;
    snprintf(i2c_dev->cache_file_path, sizeof(i2c_dev->cache_file_path), CACHE_FILE_PATH, i2c_dev->name);
    snprintf(i2c_dev->mask_file_path, sizeof(i2c_dev->mask_file_path), MASK_FILE_PATH, i2c_dev->name);

    /* creat parent dir by dev name in /sys/logic_dev */
    ret = kobject_init_and_add(&i2c_dev->kobj, &i2c_dev_ktype, logic_dev_kobj, "%s", i2c_dev->name);
    if (ret) {
        kobject_put(&i2c_dev->kobj);
        dev_err(&client->dev, "Failed to creat parent dir: %s, ret: %d\n", i2c_dev->name, ret);
        return ret;
    }

    i2c_dev->sysfs_group = &i2c_dev_attr_group;
    ret = sysfs_create_group(&i2c_dev->kobj, i2c_dev->sysfs_group);
    if (ret) {
        dev_err(&client->dev, "Failed to create %s sysfs group, ret: %d\n", i2c_dev->name, ret);
        goto remove_parent_kobj;
    }

    misc = &i2c_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = i2c_dev->name;
    misc->fops = &i2c_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&client->dev, "Failed to register %s device\n", misc->name);
        ret = -ENXIO;
        goto remove_sysfs_group;
    }

    if (misc->minor >= MAX_DEV_NUM) {
        dev_err(&client->dev, "Error: device minor[%d] more than max device num[%d]\n",
            misc->minor, MAX_DEV_NUM);
        ret = -EINVAL;
        goto deregister_misc;
    }
    i2c_dev_arry[misc->minor] = i2c_dev;

    dev_info(&client->dev, "register %u addr_bus_width %u data_bus_width 0x%x i2c_len device %s minor %d with %u per_rd_len %u per_wr_len success.\n",
        i2c_dev->addr_bus_width, i2c_dev->data_bus_width, i2c_dev->i2c_len, i2c_dev->name, misc->minor, i2c_dev->per_rd_len, i2c_dev->per_wr_len);

    return 0;
deregister_misc:
    misc_deregister(misc);
remove_sysfs_group:
    sysfs_remove_group(&i2c_dev->kobj, (const struct attribute_group *)i2c_dev->sysfs_group);
remove_parent_kobj:
    kobject_put(&i2c_dev->kobj);
    return ret;
}

static void i2c_dev_remove(struct i2c_client *client)
{
    int minor;
    wb_i2c_dev_t *i2c_dev;

    i2c_dev = i2c_get_clientdata(client);
    minor = i2c_dev->misc.minor;
    if (minor < MAX_DEV_NUM && (i2c_dev_arry[minor] != NULL)) {
        dev_dbg(&client->dev, "misc_deregister %s, minor: %d\n", i2c_dev->misc.name, minor);
        misc_deregister(&i2c_dev_arry[minor]->misc);
        i2c_dev_arry[minor] = NULL;
    }

    if (i2c_dev->sysfs_group) {
        dev_dbg(&client->dev, "Unregister %s i2c_dev sysfs group\n", i2c_dev->name);
        sysfs_remove_group(&i2c_dev->kobj, (const struct attribute_group *)i2c_dev->sysfs_group);
        kobject_put(&i2c_dev->kobj);
        i2c_dev->sysfs_group = NULL;
    }

    dev_info(&client->dev, "Remove %s i2c device success.\n", i2c_dev->name);
    i2c_set_clientdata(client, NULL);
    return;
}

static const struct i2c_device_id i2c_dev_id[] = {
    { "wb-i2c-dev", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, i2c_dev_id);

static const struct of_device_id i2c_dev_of_match[] = {
    { .compatible = "wb-i2c-dev" },
    { },
};
MODULE_DEVICE_TABLE(of, i2c_dev_of_match);

static struct i2c_driver i2c_dev_driver = {
    .driver = {
        .name = PROXY_NAME,
        .of_match_table = i2c_dev_of_match,
    },
    .probe      = i2c_dev_probe,
    .remove     = i2c_dev_remove,
    .id_table   = i2c_dev_id,
};
module_i2c_driver(i2c_dev_driver);

MODULE_DESCRIPTION("i2c dev driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
