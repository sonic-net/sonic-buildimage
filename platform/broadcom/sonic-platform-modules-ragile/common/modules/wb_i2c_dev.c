/*
 * wb_io_dev.c
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

#define MAX_I2C_DEV_NUM      (256)
#define FPGA_MAX_LEN         (256)
#define MAX_NAME_SIZE        (20)
#define MAX_BUS_WIDTH        (16)
#define TRANSFER_WRITE_BUFF  (FPGA_MAX_LEN + MAX_BUS_WIDTH)

#define WIDTH_1Byte          (1)
#define WIDTH_2Byte          (2)
#define WIDTH_4Byte          (4)

static int g_i2c_dev_debug = 0;
static int g_i2c_dev_error = 0;

module_param(g_i2c_dev_debug, int, S_IRUGO | S_IWUSR);
module_param(g_i2c_dev_error, int, S_IRUGO | S_IWUSR);

#define I2C_DEV_DEBUG_DMESG(fmt, args...) do {                                        \
    if (g_i2c_dev_debug) { \
        printk(KERN_ERR "[I2C_DEV][DEBUG][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define I2C_DEV_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_i2c_dev_error) { \
        printk(KERN_ERR "[I2C_DEV][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static struct i2c_dev_info* i2c_dev_arry[MAX_I2C_DEV_NUM];

struct i2c_dev_info {
    const char *name;
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
    uint32_t i2c_len;
    struct miscdevice misc;
    struct i2c_client *client;
};

static int transfer_read(struct i2c_client *client, u8 *buf, loff_t regaddr, size_t count)
{
    struct i2c_adapter *adap;
    int i;
    u8 offset_buf[MAX_BUS_WIDTH];
    struct i2c_msg msgs[2];
    int msgs_num, ret;
    struct i2c_dev_info *i2c_dev;

    if (!client) {
        I2C_DEV_DEBUG_ERROR("can't get read client\n");
        return -ENODEV;
    }

    adap = client->adapter;
    if (!adap) {
        I2C_DEV_DEBUG_ERROR("can't get read adap\n");
        return -ENODEV;
    }

    i2c_dev = i2c_get_clientdata(client);
    if (!i2c_dev) {
        I2C_DEV_DEBUG_ERROR("can't get read i2c_dev\n");
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
        I2C_DEV_DEBUG_ERROR("Only support 1,2,4 Byte Address Width,but set width = %u\n",
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
        ret = i2c_transfer(client->adapter, msgs, msgs_num);
        if (ret != msgs_num) {
            I2C_DEV_DEBUG_ERROR("i2c_transfer read error\n");
            return -EINVAL;
        }
    } else {
        I2C_DEV_DEBUG_ERROR("don't find read master_xfer\n");
        return -EINVAL;

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
        I2C_DEV_DEBUG_ERROR("can't get write client\n");
        return -ENODEV;
    }

    adap = client->adapter;
    if (!adap) {
        I2C_DEV_DEBUG_ERROR("can't get write adap\n");
        return -ENODEV;
    }

    i2c_dev = i2c_get_clientdata(client);
    if (!i2c_dev) {
        I2C_DEV_DEBUG_ERROR("can't get read i2c_dev\n");
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
        I2C_DEV_DEBUG_ERROR("Only support 1,2,4 Byte Address Width,but set width = %u\n",
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
        ret = i2c_transfer(adap, msgs, msgs_num);
        if (ret != msgs_num) {
            I2C_DEV_DEBUG_ERROR("i2c_transfer write error\n");
            return -EINVAL;
        }
    } else {
        I2C_DEV_DEBUG_ERROR("don't find write master_xfer\n");
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
    u8 val[FPGA_MAX_LEN];
    u32 width, rd_len, per_len, tmp;
    u32 max_per_len;

    if (offset > i2c_dev->i2c_len) {
        I2C_DEV_DEBUG_DMESG("offset: 0x%x, i2c len: 0x%x, count: %lu, EOF.\n",
            offset, i2c_dev->i2c_len, count);
        return 0;
    }

    if (count > (i2c_dev->i2c_len - offset)) {
        I2C_DEV_DEBUG_DMESG("read count out of range. input len:%lu, read len:%u.\n",
            count, i2c_dev->i2c_len - offset);
        count = i2c_dev->i2c_len - offset;
    }

    if (count == 0) {
        I2C_DEV_DEBUG_DMESG("offset: 0x%x, i2c len: 0x%x, read len: %lu, EOF.\n",
            offset, i2c_dev->i2c_len, count);
        return 0;
    }

    width = i2c_dev->data_bus_width;
    switch (width) {
    case WIDTH_4Byte:
        tmp_offset = offset & 0x3;
        if (tmp_offset) {
            I2C_DEV_DEBUG_ERROR("data bus width:%u, offset:%u, read size %lu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_2Byte:
        tmp_offset = offset & 0x1;
        if (tmp_offset) {
            I2C_DEV_DEBUG_ERROR("data bus width:%u, offset:%u, read size %lu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_1Byte:
        break;
    default:
        I2C_DEV_DEBUG_ERROR("Only support 1,2,4 Byte Data Width,but set width = %u\n", width);
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
            I2C_DEV_DEBUG_ERROR("read error.read offset = %u\n", (offset + i));
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

    return count;
}

static int device_write(struct i2c_dev_info *i2c_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, j, ret;
    u8 tmp_offset;
    u32 width;
    u8 val[FPGA_MAX_LEN];
    u32 wr_len, per_len, tmp;
    u32 max_per_len;

    if (offset > i2c_dev->i2c_len) {
        I2C_DEV_DEBUG_DMESG("offset: 0x%x, i2c len: 0x%x, count: %lu, EOF.\n",
            offset, i2c_dev->i2c_len, count);
        return 0;
    }

    if (count > (i2c_dev->i2c_len - offset)) {
        I2C_DEV_DEBUG_DMESG("read count out of range. input len:%lu, read len:%u.\n",
            count, i2c_dev->i2c_len - offset);
        count = i2c_dev->i2c_len - offset;
    }

    if (count == 0) {
        I2C_DEV_DEBUG_DMESG("offset: 0x%x, i2c len: 0x%x, read len: %lu, EOF.\n",
            offset, i2c_dev->i2c_len, count);
        return 0;
    }

    width = i2c_dev->data_bus_width;
    switch (width) {
    case WIDTH_4Byte:
        tmp_offset = offset & 0x3;
        if (tmp_offset) {
            I2C_DEV_DEBUG_ERROR("data bus width:%u, offset:%u, read size %lu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_2Byte:
        tmp_offset = offset & 0x1;
        if (tmp_offset) {
            I2C_DEV_DEBUG_ERROR("data bus width:%u, offset:%u, read size %lu invalid.\n",
                width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_1Byte:
        break;
    default:
        I2C_DEV_DEBUG_ERROR("Only support 1,2,4 Byte Data Width,but set width = %u\n", width);
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
            I2C_DEV_DEBUG_ERROR("write error.offset = %u\n", (offset + i));
            return -EFAULT;
        }
    }
    return count;
}

static ssize_t i2c_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    u8 val[FPGA_MAX_LEN];
    int ret, read_len;
    struct i2c_dev_info *i2c_dev;

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
        I2C_DEV_DEBUG_ERROR("can't get read private_data.n");
        return -EINVAL;
    }

    if (count == 0) {
        I2C_DEV_DEBUG_ERROR("Invalid params, read count is 0.n");
        return -EINVAL;
    }

    if (count > sizeof(val)) {
        I2C_DEV_DEBUG_DMESG("read conut %lu exceed max %lu.\n", count, sizeof(val));
        count = sizeof(val);
    }

    mem_clear(val, sizeof(val));
    read_len = device_read(i2c_dev, (uint32_t)*offset, val, count);
    if (read_len < 0) {
        I2C_DEV_DEBUG_ERROR("i2c dev read failed, dev name:%s, offset:0x%x, len:%lu.\n",
            i2c_dev->name, (uint32_t)*offset, count);
        return read_len;
    }

    if (access_ok(buf, read_len)) {
        I2C_DEV_DEBUG_DMESG("user space read, buf: %p, offset: %lld, read conut %lu.\n",
            buf, *offset, count);
        if (copy_to_user(buf, val, read_len)) {
            I2C_DEV_DEBUG_ERROR("copy_to_user failed.\n");
            return -EFAULT;
        }
    } else {
        I2C_DEV_DEBUG_DMESG("kernel space read, buf: %p, offset: %lld, read conut %lu.\n",
            buf, *offset, count);
        memcpy(buf, val, read_len);
    }

    *offset += read_len;
    ret = read_len;
    return ret;
}

static ssize_t i2c_dev_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    int ret;

    I2C_DEV_DEBUG_DMESG("i2c_dev_read_iter, file: %p, count: %lu, offset: %lld\n",
        iocb->ki_filp, to->count, iocb->ki_pos);
    ret = i2c_dev_read(iocb->ki_filp, to->kvec->iov_base, to->count, &iocb->ki_pos);
    return ret;
}

static ssize_t i2c_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    u8 val[FPGA_MAX_LEN];
    int write_len;
    struct i2c_dev_info *i2c_dev;

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
        I2C_DEV_DEBUG_ERROR("get write private_data error.\n");
        return -EINVAL;
    }

    if (count == 0) {
        I2C_DEV_DEBUG_ERROR("Invalid params, write count is 0.\n");
        return -EINVAL;
    }

    if (count > sizeof(val)) {
        I2C_DEV_DEBUG_DMESG("write conut %lu exceed max %lu.\n", count, sizeof(val));
        count = sizeof(val);
    }

    mem_clear(val, sizeof(val));
    if (access_ok(buf, count)) {
        I2C_DEV_DEBUG_DMESG("user space write, buf: %p, offset: %lld, write conut %lu.\n",
            buf, *offset, count);
        if (copy_from_user(val, buf, count)) {
            I2C_DEV_DEBUG_ERROR("copy_from_user failed.\n");
            return -EFAULT;
        }
    } else {
        I2C_DEV_DEBUG_DMESG("kernel space write, buf: %p, offset: %lld, write conut %lu.\n",
            buf, *offset, count);
        memcpy(val, buf, count);
    }

    write_len = device_write(i2c_dev, (uint32_t)*offset, val, count);
    if (write_len < 0) {
        I2C_DEV_DEBUG_ERROR("i2c dev write failed, dev name:%s, offset:0x%llx, len:%lu.\n",
            i2c_dev->name, *offset, count);
        return write_len;
    }

    *offset += write_len;
    return write_len;
}

static ssize_t i2c_dev_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    int ret;

    I2C_DEV_DEBUG_DMESG("i2c_dev_write_iter, file: %p, count: %lu, offset: %lld\n",
        iocb->ki_filp, from->count, iocb->ki_pos);
    ret = i2c_dev_write(iocb->ki_filp, from->kvec->iov_base, from->count, &iocb->ki_pos);
    return ret;
}

static loff_t i2c_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;
    struct i2c_dev_info *i2c_dev;

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
        I2C_DEV_DEBUG_ERROR("i2c_dev is NULL, llseek failed.\n");
        return -EINVAL;
    }

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            I2C_DEV_DEBUG_ERROR("SEEK_SET, offset:%lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        if (offset > i2c_dev->i2c_len) {
            I2C_DEV_DEBUG_ERROR("SEEK_SET out of range, offset:%lld, i2c_len:0x%x.\n",
                offset, i2c_dev->i2c_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (((file->f_pos + offset) > i2c_dev->i2c_len) || ((file->f_pos + offset) < 0)) {
            I2C_DEV_DEBUG_ERROR("SEEK_CUR out of range, f_ops:%lld, offset:%lld, i2c_len:0x%x.\n",
                file->f_pos, offset, i2c_dev->i2c_len);
            ret = - EINVAL;
            break;
        }
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        I2C_DEV_DEBUG_ERROR("unsupport llseek type:%d.\n", origin);
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

static struct i2c_dev_info * dev_match(const char *path)
{
    struct i2c_dev_info * i2c_dev;
    char dev_name[MAX_NAME_SIZE];
    int i;
    for (i = 0; i < MAX_I2C_DEV_NUM; i++) {
        if (i2c_dev_arry[ i ] == NULL) {
            continue;
        }
        i2c_dev = i2c_dev_arry[ i ];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", i2c_dev->name);
        if (!strcmp(path, dev_name)) {
            I2C_DEV_DEBUG_DMESG("get dev_name = %s, minor = %d\n", dev_name, i);
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
        I2C_DEV_DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if(buf == NULL){
        I2C_DEV_DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    if (count > FPGA_MAX_LEN) {
        I2C_DEV_DEBUG_ERROR("read conut %lu, beyond max:%d.\n", count, FPGA_MAX_LEN);
        return -EINVAL;
    }

    i2c_dev = dev_match(path);
    if (i2c_dev == NULL) {
        I2C_DEV_DEBUG_ERROR("i2c_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    ret = device_read(i2c_dev, offset, buf, count);
    if (ret < 0) {
        I2C_DEV_DEBUG_ERROR("fpga i2c dev read failed, dev name:%s, offset:0x%x, len:%lu.\n",
            i2c_dev->name, offset, count);
        return -EINVAL;
    }

    return count;
}
EXPORT_SYMBOL(i2c_device_func_read);

int i2c_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct i2c_dev_info *i2c_dev = NULL;
    int ret;

    if(path == NULL){
        I2C_DEV_DEBUG_ERROR("path NULL");
        return -EINVAL;
    }

    if(buf == NULL){
        I2C_DEV_DEBUG_ERROR("buf NULL");
        return -EINVAL;
    }

    if (count > FPGA_MAX_LEN) {
        I2C_DEV_DEBUG_ERROR("write conut %lu, beyond max:%d.\n", count, FPGA_MAX_LEN);
        return -EINVAL;
    }

    i2c_dev = dev_match(path);
    if (i2c_dev == NULL) {
        I2C_DEV_DEBUG_ERROR("i2c_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    ret = device_write (i2c_dev, offset, buf, count);
    if (ret < 0) {
        I2C_DEV_DEBUG_ERROR("i2c dev write failed, dev name:%s, offset:0x%x, len:%lu.\n",
            i2c_dev->name, offset, count);
        return -EINVAL;
    }

    return count;
}
EXPORT_SYMBOL(i2c_device_func_write);

static int i2c_dev_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    struct i2c_dev_info *i2c_dev;
    struct miscdevice *misc;
    i2c_dev_device_t *i2c_dev_device;

    i2c_dev = devm_kzalloc(&client->dev, sizeof(struct i2c_dev_info), GFP_KERNEL);
    if (!i2c_dev) {
        dev_err(&client->dev, "devm_kzalloc error. \n");
        return -ENOMEM;
    }

    i2c_set_clientdata(client, i2c_dev);
    i2c_dev->client = client;

    if (client->dev.of_node) {

        ret += of_property_read_string(client->dev.of_node, "i2c_name", &i2c_dev->name);
        ret += of_property_read_u32(client->dev.of_node, "data_bus_width", &i2c_dev->data_bus_width);
        ret += of_property_read_u32(client->dev.of_node, "addr_bus_width", &i2c_dev->addr_bus_width);
        ret += of_property_read_u32(client->dev.of_node, "per_rd_len", &i2c_dev->per_rd_len);
        ret += of_property_read_u32(client->dev.of_node, "per_wr_len", &i2c_dev->per_wr_len);
        ret += of_property_read_u32(client->dev.of_node, "i2c_len", &i2c_dev->i2c_len);
        if (ret != 0) {
            dev_err(&client->dev, "dts config error.ret:%d.\n", ret);
            return -ENXIO;
        }
    } else {
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
    }

    if ((i2c_dev->per_rd_len & (i2c_dev->data_bus_width - 1)) ||
        (i2c_dev->per_wr_len & (i2c_dev->data_bus_width - 1))) {
        dev_err(&client->dev, "Invalid config per_rd_len %d per_wr_len %d data bus_width %d.\n",
            i2c_dev->per_rd_len, i2c_dev->per_wr_len, i2c_dev->data_bus_width);
        return -ENXIO;
    }

    if ((i2c_dev->i2c_len == 0) || (i2c_dev->i2c_len & (i2c_dev->data_bus_width - 1))) {
        dev_err(&client->dev, "Invalid config i2c_len %d, data bus_width %d.\n",
            i2c_dev->i2c_len, i2c_dev->data_bus_width);
        return -ENXIO;
    }

    misc = &i2c_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = i2c_dev->name;
    misc->fops = &i2c_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&client->dev, "register %s faild.\n", misc->name);
        return -ENXIO;
    }

    if (misc->minor >= MAX_I2C_DEV_NUM) {
        dev_err(&client->dev, "minor number beyond the limit! is %d.\n", misc->minor);
        misc_deregister(misc);
        return -ENXIO;
    }
    i2c_dev_arry[misc->minor] = i2c_dev;

    dev_info(&client->dev, "register %u addr_bus_width %u data_bus_width 0x%x i2c_len device %s with %u per_rd_len %u per_wr_len success.\n",
        i2c_dev->addr_bus_width, i2c_dev->data_bus_width, i2c_dev->i2c_len, i2c_dev->name, i2c_dev->per_rd_len, i2c_dev->per_wr_len);

    return 0;
}

static int i2c_dev_remove(struct i2c_client *client)
{
    int i;
    for (i = 0; i < MAX_I2C_DEV_NUM; i++) {
        if (i2c_dev_arry[i] != NULL) {
            misc_deregister(&i2c_dev_arry[i]->misc);
            i2c_dev_arry[i] = NULL;
        }
    }
    return 0;
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
        .name = "wb-i2c-dev",
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
