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

#include "wb_platform_i2c_dev.h"

#define PROXY_NAME "wb-platform-i2c-dev"
#define MAX_I2C_DEV_NUM      (256)
#define FPGA_MAX_LEN         (256)
#define MAX_NAME_SIZE        (20)
#define MAX_BUS_WIDTH        (16)
#define TRANSFER_WRITE_BUFF  (FPGA_MAX_LEN + MAX_BUS_WIDTH)

#define WIDTH_1Byte          (1)
#define WIDTH_2Byte          (2)
#define WIDTH_4Byte          (4)

int g_i2c_dev_debug = 0;
int g_i2c_dev_error = 0;

module_param(g_i2c_dev_debug, int, S_IRUGO | S_IWUSR);
module_param(g_i2c_dev_error, int, S_IRUGO | S_IWUSR);

#define I2C_DEV_DEBUG_DMESG(fmt, args...) do {                                        \
    if (g_i2c_dev_debug) { \
        printk(KERN_ERR "[I2C_DEV][DEBUG][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define I2C_DEV_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_i2c_dev_error) { \
        printk(KERN_ERR "[I2C_DEV][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static struct platform_i2c_dev_info* i2c_dev_arry[MAX_I2C_DEV_NUM];

struct platform_i2c_dev_info {
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    const char *name;
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
    struct miscdevice misc;
};

static int transfer_read(struct platform_i2c_dev_info *i2c_dev, u8 *buf, loff_t regaddr, size_t count)
{
    int i, j;
    struct i2c_adapter *adap;
    union i2c_smbus_data data;
    u8 offset_buf[MAX_BUS_WIDTH];
    struct i2c_msg msgs[2];
    int msgs_num, ret;
    u8 offset;
    u8 length;

    if (!i2c_dev) {
        I2C_DEV_DEBUG_ERROR("can't get read i2c_dev\r\n");
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
        I2C_DEV_DEBUG_ERROR("Only support 1,2,4 Byte Address Width,but set width = %u\r\n", i2c_dev->addr_bus_width);
        return -EINVAL;
    }

    adap = i2c_get_adapter(i2c_dev->i2c_bus);
    if (adap == NULL) {
        I2C_DEV_DEBUG_ERROR("get i2c adapter %d faild.\n", i2c_dev->i2c_bus);
        return -ENXIO;
    }

    if (adap->algo->master_xfer) {
        mem_clear(msgs, sizeof(msgs));
        msgs[0].addr = i2c_dev->i2c_addr;
        msgs[0].flags = 0;
        msgs[0].len = i2c_dev->addr_bus_width;
        msgs[0].buf = offset_buf;

        msgs[1].addr = i2c_dev->i2c_addr;
        msgs[1].flags = I2C_M_RD;
        msgs[1].len = count;
        msgs[1].buf = buf;

        msgs_num = 2;
        ret = i2c_transfer(adap, msgs, msgs_num);
        if (ret != msgs_num) {
            I2C_DEV_DEBUG_ERROR("i2c_transfer read error\r\n");
            ret = -EFAULT;
            goto error_exit;
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
                    ret = adap->algo->smbus_xfer(adap, i2c_dev->i2c_addr,
                                    0,
                                    I2C_SMBUS_READ,
                                    offset, I2C_SMBUS_I2C_BLOCK_DATA, &data);
                    if (ret) {
                        I2C_DEV_DEBUG_ERROR("smbus_xfer read block error, ret = %d\r\n", ret);
                        ret = -EFAULT;
                        goto error_exit;
                    }
                    memcpy(buf + j, data.block + 1, length);
                    offset += length;
                }
            } else {
                for (j = 0; j < count; j++) {
                    ret = adap->algo->smbus_xfer(adap, i2c_dev->i2c_addr,
                                    0,
                                    I2C_SMBUS_READ,
                                    offset, I2C_SMBUS_BYTE_DATA, &data);

                    if (!ret) {
                        buf[j] = data.byte;
                    } else {
                        I2C_DEV_DEBUG_ERROR("smbus_xfer read byte error, ret = %d\r\n", ret);
                        ret = -EFAULT;
                        goto error_exit;
                    }
                    offset++;
                }
            }
        } else {
            I2C_DEV_DEBUG_ERROR("smbus_xfer not support addr_bus_width = %d\r\n", i2c_dev->addr_bus_width);
            ret = -EINVAL;
            goto error_exit;
        }
    }

    i2c_put_adapter(adap);
    return 0;
error_exit:
    i2c_put_adapter(adap);
    return ret;
}

static int transfer_write(struct platform_i2c_dev_info *i2c_dev, u8 *buf, loff_t regaddr, size_t count)
{
    int i, j;
    struct i2c_adapter *adap;
    union i2c_smbus_data data;
    u8 offset_buf[TRANSFER_WRITE_BUFF];
    struct i2c_msg msgs[1];
    int msgs_num, ret;
    u8 offset;
    u8 length;

    if (!i2c_dev) {
        I2C_DEV_DEBUG_ERROR("can't get read i2c_dev\r\n");
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
        I2C_DEV_DEBUG_ERROR("Only support 1,2,4 Byte Address Width,but set width = %u\r\n", i2c_dev->addr_bus_width);
        return -EINVAL;
    }

    memcpy(offset_buf + i2c_dev->addr_bus_width, buf, count);

    adap = i2c_get_adapter(i2c_dev->i2c_bus);
    if (adap == NULL) {
        I2C_DEV_DEBUG_ERROR("get i2c adapter %d faild.\n", i2c_dev->i2c_bus);
        return -ENXIO;
    }

    if (adap->algo->master_xfer) {
        mem_clear(msgs, sizeof(msgs));

        msgs[0].addr = i2c_dev->i2c_addr;
        msgs[0].flags = 0;
        msgs[0].len = i2c_dev->addr_bus_width + count;
        msgs[0].buf = offset_buf;

        msgs_num = 1;
        ret = i2c_transfer(adap, msgs, msgs_num);
        if (ret != msgs_num) {
            I2C_DEV_DEBUG_ERROR("i2c_transfer write error\r\n");
            ret = -EFAULT;
            goto error_exit;
        }
    } else {
        if (i2c_dev->addr_bus_width == WIDTH_1Byte) {
            offset = regaddr & 0xFF;
            if (i2c_check_functionality(adap, I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) {
                for (j = 0; j < count; j += I2C_SMBUS_BLOCK_MAX) {
                    if (count - j > I2C_SMBUS_BLOCK_MAX) {
                        length = I2C_SMBUS_BLOCK_MAX;
                    } else {
                        length = count - j;
                    }
                    data.block[0] = length;
                    memcpy(data.block + 1, buf + j, length);
                    ret = adap->algo->smbus_xfer(adap, i2c_dev->i2c_addr,
                                    0,
                                    I2C_SMBUS_WRITE,
                                    offset, I2C_SMBUS_I2C_BLOCK_DATA, &data);
                    if (ret) {
                        I2C_DEV_DEBUG_ERROR("smbus_xfer write block error, ret = %d\r\n", ret);
                        ret = -EFAULT;
                        goto error_exit;
                    }
                    offset += length;
                }
            } else {
                for (j = 0; j < count; j++) {
                    data.byte = buf[j];
                    ret = adap->algo->smbus_xfer(adap, i2c_dev->i2c_addr,
                                    0,
                                    I2C_SMBUS_WRITE,
                                    offset, I2C_SMBUS_BYTE_DATA, &data);
                    if (ret) {
                        I2C_DEV_DEBUG_ERROR("smbus_xfer write byte error, ret = %d\r\n", ret);
                        ret = -EFAULT;
                        goto error_exit;
                    }
                    offset += 1;
                }
            }
        } else {
            I2C_DEV_DEBUG_ERROR("smbus_xfer not support addr_bus_width = %d\r\n", i2c_dev->addr_bus_width);
            ret = -EINVAL;
            goto error_exit;
        }
    }

    i2c_put_adapter(adap);
    return 0;
error_exit:
    i2c_put_adapter(adap);
    return ret;
}

static long i2c_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int i2c_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    struct platform_i2c_dev_info *i2c_dev;

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

static int device_read(struct platform_i2c_dev_info *i2c_dev, uint32_t offset, uint8_t *buf, int count)
{
    int i, j, ret;
    u8 tmp_offset;
    u8 val[FPGA_MAX_LEN];
    u32 width, rd_len, per_len, tmp;
    u32 max_per_len;

    width = i2c_dev->data_bus_width;
    switch (width) {
    case WIDTH_4Byte:
        tmp_offset = offset & 0x3;
        if (tmp_offset) {
            I2C_DEV_DEBUG_ERROR("data bus width:%u, offset:%u, read size %d invalid.\r\n", width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_2Byte:
        tmp_offset = offset & 0x1;
        if (tmp_offset) {
            I2C_DEV_DEBUG_ERROR("data bus width:%u, offset:%u, read size %d invalid.\r\n", width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_1Byte:
        break;
    default:
        I2C_DEV_DEBUG_ERROR("Only support 1,2,4 Byte Data Width,but set width = %u\r\n", width);
        return -EINVAL;
    }

    max_per_len = i2c_dev->per_rd_len;
    tmp = (width - 1) & count;
    rd_len = (tmp == 0) ? count : count + width - tmp;
    per_len = (rd_len > max_per_len) ? (max_per_len) : (rd_len);

    mem_clear(val, sizeof(val));
    for (i = 0; i < rd_len; i += per_len) {
        ret = transfer_read(i2c_dev, val + i, offset + i, per_len);
        if (ret < 0) {
            I2C_DEV_DEBUG_ERROR("read error.read offset = %u\r\n", (offset + i));
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

    return 0;
}

static int device_write(struct platform_i2c_dev_info *i2c_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, j, ret;
    u8 tmp_offset;
    u32 width;
    u8 val[FPGA_MAX_LEN];
    u32 wr_len, per_len, tmp;
    u32 max_per_len;

    width = i2c_dev->data_bus_width;
    switch (width) {
    case WIDTH_4Byte:
        tmp_offset = offset & 0x3;
        if (tmp_offset) {
            I2C_DEV_DEBUG_ERROR("data bus width:%u, offset:%u, read size %lu invalid.\r\n", width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_2Byte:
        tmp_offset = offset & 0x1;
        if (tmp_offset) {
            I2C_DEV_DEBUG_ERROR("data bus width:%u, offset:%u, read size %lu invalid.\r\n", width, offset, count);
            return -EINVAL;
        }
        break;
    case WIDTH_1Byte:
        break;
    default:
        I2C_DEV_DEBUG_ERROR("Only support 1,2,4 Byte Data Width,but set width = %u\r\n", width);
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
        ret = transfer_write(i2c_dev, val + i, offset + i, per_len);
        if (ret < 0) {
            I2C_DEV_DEBUG_ERROR("write error.offset = %u\r\n", (offset + i));
            return -EFAULT;
        }
    }
    return 0;
}

static ssize_t i2c_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    u8 val[FPGA_MAX_LEN];
    int ret;
    struct platform_i2c_dev_info *i2c_dev;

    if (count <= 0 || count > sizeof(val)) {
        I2C_DEV_DEBUG_ERROR("read conut %lu , beyond max:%lu.\n", count, sizeof(val));
        return -EINVAL;
    }

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
        I2C_DEV_DEBUG_ERROR("can't get read private_data .\r\n");
        return -EINVAL;
    }

    ret = device_read(i2c_dev, (uint32_t)*offset, val, count);
    if (ret < 0) {
        I2C_DEV_DEBUG_ERROR("i2c dev read failed, dev name:%s, offset:0x%x, len:%lu.\n",
            i2c_dev->name, (uint32_t)*offset, count);
        return -EINVAL;
    }

    if (copy_to_user(buf, val, count)) {
        I2C_DEV_DEBUG_ERROR("copy_to_user error \r\n");
        return -EFAULT;
    } else{
        *offset += count;
    }

    return count;
}

static ssize_t i2c_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    u8 val[FPGA_MAX_LEN];
    int ret;
    struct platform_i2c_dev_info *i2c_dev;

    if (count <= 0 || count > sizeof(val)) {
        I2C_DEV_DEBUG_ERROR("write conut %lu, beyond max val:%lu.\n", count, sizeof(val));
        return -EINVAL;
    }

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
        I2C_DEV_DEBUG_ERROR("get write private_data error.\r\n");
        return -EINVAL;
    }

    mem_clear(val, sizeof(val));
    if (copy_from_user(val, buf, count)) {
        I2C_DEV_DEBUG_ERROR("copy_from_user error.\r\n");
        return -EFAULT;
    }

    ret = device_write (i2c_dev, (uint32_t)*offset, val, count);
    if (ret < 0) {
        I2C_DEV_DEBUG_ERROR("i2c dev write failed, dev name:%s, offset:0x%llx, len:%lu.\n",
            i2c_dev->name, *offset, count);
        return -EINVAL;
    }

    *offset += count;
    return count;
}

static loff_t i2c_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            I2C_DEV_DEBUG_ERROR("SEEK_SET, offset:%lld, invalid.\r\n", offset);
            ret = -EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (file->f_pos + offset < 0) {
            I2C_DEV_DEBUG_ERROR("SEEK_CUR out of range, f_ops:%lld, offset:%lld.\n",
                 file->f_pos, offset);
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
    .read       = i2c_dev_read,
    .write      = i2c_dev_write,
    .unlocked_ioctl = i2c_dev_ioctl,
    .open       = i2c_dev_open,
    .release    = i2c_dev_release,
};

static struct platform_i2c_dev_info * dev_match(const char *path)
{
    struct platform_i2c_dev_info *i2c_dev;
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

int platform_i2c_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct platform_i2c_dev_info *i2c_dev = NULL;
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
EXPORT_SYMBOL(platform_i2c_device_func_read);

int platform_i2c_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct platform_i2c_dev_info *i2c_dev = NULL;
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
EXPORT_SYMBOL(platform_i2c_device_func_write);

static int platform_i2c_dev_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct platform_i2c_dev_info *i2c_dev;
    struct miscdevice *misc;
    platform_i2c_dev_device_t *platform_i2c_dev_device;

    i2c_dev = devm_kzalloc(&pdev->dev, sizeof(struct platform_i2c_dev_info), GFP_KERNEL);
    if (!i2c_dev) {
        dev_err(&pdev->dev, "devm_kzalloc error. \r\n");
        return -ENOMEM;
    }

    if (pdev->dev.of_node) {

        ret += of_property_read_u32(pdev->dev.of_node, "i2c_bus", &i2c_dev->i2c_bus);
        ret += of_property_read_u32(pdev->dev.of_node, "i2c_addr", &i2c_dev->i2c_addr);
        ret += of_property_read_string(pdev->dev.of_node, "i2c_name", &i2c_dev->name);
        ret += of_property_read_u32(pdev->dev.of_node, "data_bus_width", &i2c_dev->data_bus_width);
        ret += of_property_read_u32(pdev->dev.of_node, "addr_bus_width", &i2c_dev->addr_bus_width);
        ret += of_property_read_u32(pdev->dev.of_node, "per_rd_len", &i2c_dev->per_rd_len);
        ret += of_property_read_u32(pdev->dev.of_node, "per_wr_len", &i2c_dev->per_wr_len);
        if (ret != 0) {
            dev_err(&pdev->dev, "dts config error.ret:%d.\r\n", ret);
            return -ENXIO;
        }
    } else {
        if (pdev->dev.platform_data == NULL) {
            dev_err(&pdev->dev, "Failed to get platform data config.\n");
            return -ENXIO;
        }
        platform_i2c_dev_device = pdev->dev.platform_data;
        i2c_dev->i2c_bus = platform_i2c_dev_device->i2c_bus;
        i2c_dev->i2c_addr = platform_i2c_dev_device->i2c_addr;
        i2c_dev->name = platform_i2c_dev_device->i2c_name;
        i2c_dev->data_bus_width = platform_i2c_dev_device->data_bus_width;
        i2c_dev->addr_bus_width = platform_i2c_dev_device->addr_bus_width;
        i2c_dev->per_rd_len = platform_i2c_dev_device->per_rd_len;
        i2c_dev->per_wr_len = platform_i2c_dev_device->per_wr_len;
    }

    if ((i2c_dev->per_rd_len & (i2c_dev->data_bus_width - 1)) || (i2c_dev->per_wr_len & (i2c_dev->data_bus_width - 1))) {
        dev_err(&pdev->dev, "Invalid config per_rd_len %d per_wr_len %d data bus_width %d.\r\n", i2c_dev->per_rd_len,
            i2c_dev->per_wr_len, i2c_dev->data_bus_width);
        return -ENXIO;
    }

    misc = &i2c_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = i2c_dev->name;
    misc->fops = &i2c_dev_fops;
    if (misc_register(misc) != 0) {
        dev_err(&pdev->dev, "register %s faild.\r\n", misc->name);
        return -ENXIO;
    }

    if (misc->minor >= MAX_I2C_DEV_NUM) {
        dev_err(&pdev->dev, "minor number beyond the limit! is %d.\r\n", misc->minor);
        misc_deregister(misc);
        return -ENXIO;
    }
    i2c_dev_arry[misc->minor] = i2c_dev;

    dev_info(&pdev->dev, "register %u addr_bus_width %u data_bus_width device %s with %u per_rd_len %u per_wr_len success.\r\n",
        i2c_dev->addr_bus_width, i2c_dev->data_bus_width, i2c_dev->name, i2c_dev->per_rd_len, i2c_dev->per_wr_len);

    return 0;
}

static int platform_i2c_dev_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < MAX_I2C_DEV_NUM ; i++) {
        if (i2c_dev_arry[i] != NULL) {
            misc_deregister(&i2c_dev_arry[i]->misc);
            i2c_dev_arry[i] = NULL;
        }
    }

    return 0;
}

static const struct of_device_id platform_i2c_dev_of_match[] = {
    { .compatible = "wb-platform-i2c-dev" },
    { },
};
MODULE_DEVICE_TABLE(of, platform_i2c_dev_of_match);

static struct platform_driver wb_platform_i2c_dev_driver = {
    .probe      = platform_i2c_dev_probe,
    .remove     = platform_i2c_dev_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = PROXY_NAME,
        .of_match_table = platform_i2c_dev_of_match,
    },
};

static int __init wb_platform_i2c_dev_init(void)
{
    return platform_driver_register(&wb_platform_i2c_dev_driver);
}

static void __exit wb_platform_i2c_dev_exit(void)
{
    platform_driver_unregister(&wb_platform_i2c_dev_driver);
}

module_init(wb_platform_i2c_dev_init);
module_exit(wb_platform_i2c_dev_exit);

MODULE_DESCRIPTION("platform i2c dev driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
