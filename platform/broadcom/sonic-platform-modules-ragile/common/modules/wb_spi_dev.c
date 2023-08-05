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

#include "wb_spi_dev.h"

#define MAX_SPI_DEV_NUM      (256)
#define MAX_RW_LEN           (256)
#define MAX_NAME_SIZE        (20)
#define MAX_ADDR_BUS_WIDTH   (4)

#define TRANSFER_WRITE_BUFF  (1 + MAX_ADDR_BUS_WIDTH + MAX_RW_LEN)

#define WIDTH_1Byte          (1)
#define WIDTH_2Byte          (2)
#define WIDTH_4Byte          (4)

#define OP_READ             (0x3)
#define OP_WRITE            (0x2)

int g_spi_dev_debug = 0;
int g_spi_dev_error = 0;

module_param(g_spi_dev_debug, int, S_IRUGO | S_IWUSR);
module_param(g_spi_dev_error, int, S_IRUGO | S_IWUSR);

#define SPI_DEV_DEBUG(fmt, args...) do {                                        \
    if (g_spi_dev_debug) { \
        printk(KERN_ERR "[SPI_DEV][DEBUG][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SPI_DEV_ERROR(fmt, args...) do {                                        \
    if (g_spi_dev_error) { \
        printk(KERN_ERR "[SPI_DEV][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static struct spi_dev_info* spi_dev_arry[MAX_SPI_DEV_NUM];

struct spi_dev_info {
    const char *name;
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
    struct miscdevice misc;
    struct spi_device *spi_device;
};

static int transfer_read(struct spi_dev_info *spi_dev, u8 *buf, uint32_t regaddr, size_t count)
{
    int i, ret;
    u8 tx_buf[MAX_ADDR_BUS_WIDTH + 1];
    struct spi_message m;
    struct spi_transfer xfer[2];

    i = 0;
    mem_clear(tx_buf, sizeof(tx_buf));
    tx_buf[i++] = OP_READ;

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
        SPI_DEV_ERROR("Only support 1,2,4 Byte Width,but set width = %u\n",
            spi_dev->addr_bus_width);
        return -EINVAL;
    }

    mem_clear(xfer, sizeof(xfer));
    spi_message_init(&m);
    xfer[0].tx_buf = tx_buf;
    xfer[0].len = spi_dev->addr_bus_width + 1;
    spi_message_add_tail(&xfer[0], &m);

    xfer[1].rx_buf = buf;
    xfer[1].len = count;
    spi_message_add_tail(&xfer[1], &m);

    ret = spi_sync(spi_dev->spi_device, &m);
    if (ret) {
        SPI_DEV_ERROR("transfer_read failed, reg addr:0x%x, len:%lu, ret:%d.\n",
            regaddr, count, ret);
        return -EIO;
    }
    return 0;
}

static int transfer_write(struct spi_dev_info *spi_dev, u8 *buf, uint32_t regaddr, size_t count)
{
    int i, ret;
    u8 tx_buf[TRANSFER_WRITE_BUFF];
    struct spi_message m;
    struct spi_transfer xfer ;

    i = 0;
    mem_clear(tx_buf, sizeof(tx_buf));
    tx_buf[i++] = OP_WRITE;
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
        SPI_DEV_ERROR("Only support 1,2,4 Byte Width, but set width = %u\n",
            spi_dev->addr_bus_width);
        return -EINVAL;
    }

    memcpy(tx_buf + i, buf, count);

    mem_clear(&xfer, sizeof(xfer));
    spi_message_init(&m);
    xfer.tx_buf = tx_buf;
    xfer.len = count + i;
    spi_message_add_tail(&xfer, &m);

    ret = spi_sync(spi_dev->spi_device, &m);
    if (ret) {
        SPI_DEV_ERROR("transfer_write failed, reg addr:0x%x, len:%lu, ret:%d.\n",
            regaddr, count, ret);
        return -EIO;
    }
    return 0;
}

static long spi_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int spi_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    struct spi_dev_info *spi_dev;

    if (minor >= MAX_SPI_DEV_NUM) {
        SPI_DEV_ERROR("minor out of range, minor = %d.\n", minor);
        return -ENODEV;
    }

    spi_dev = spi_dev_arry[minor];
    if (spi_dev == NULL) {
        SPI_DEV_ERROR("spi_dev is NULL, open failed, minor = %d\n", minor);
        return -ENODEV;
    }

    file->private_data = spi_dev;

    return 0;
}

static int spi_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;

    return 0;
}

static int device_read(struct spi_dev_info *spi_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, j, ret;
    u8 val[MAX_RW_LEN];
    u32 data_width, rd_len, per_len, tmp;
    u32 max_per_len;

    data_width = spi_dev->data_bus_width;

    if (offset % data_width) {
        SPI_DEV_ERROR("data bus width:%d, offset:0x%x, read size %lu invalid.\n",
            data_width, offset, count);
        return -EINVAL;
    }

    max_per_len = spi_dev->per_rd_len;
    tmp = (data_width - 1) & count;
    rd_len = (tmp == 0) ? count : count + data_width - tmp;
    per_len = (rd_len > max_per_len) ? (max_per_len) : (rd_len);

    mem_clear(val, sizeof(val));
    for (i = 0; i < rd_len; i += per_len) {
        ret = transfer_read(spi_dev, val + i, offset + i, per_len);
        if (ret < 0) {
            SPI_DEV_ERROR("read error.read offset = %u\n", (offset + i));
            return -EFAULT;
        }
    }

    if (data_width == WIDTH_1Byte) {
        memcpy(buf, val, count);
    } else {
        for (i = 0; i < count; i += data_width) {
            for (j = 0; (j < data_width) && (i + j < count); j++) {
                buf[i + j] = val[i + data_width - j - 1];
            }
        }
    }

    return 0;
}

static int device_write(struct spi_dev_info *spi_dev, uint32_t offset, uint8_t *buf, size_t count)
{
    int i, j, ret;
    u32 data_width;
    u8 val[MAX_RW_LEN];
    u32 wr_len, per_len, tmp;
    u32 max_per_len;

    data_width = spi_dev->data_bus_width;

    if (offset % data_width) {
        SPI_DEV_ERROR("data bus width:%d, offset:0x%x, read size %lu invalid.\n",
            data_width, offset, count);
        return -EINVAL;
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
        ret = transfer_write(spi_dev, val + i, offset + i, per_len);
        if (ret < 0) {
            SPI_DEV_ERROR("write error.offset = %u\n", (offset + i));
            return -EFAULT;
        }
    }
    return 0;
}

static ssize_t spi_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int ret;
    struct spi_dev_info *spi_dev;

    if (count <= 0 || count > sizeof(val)) {
        SPI_DEV_ERROR("read conut %lu , beyond max:%lu.\n", count, sizeof(val));
        return -EINVAL;
    }

    spi_dev = file->private_data;
    if (spi_dev == NULL) {
        SPI_DEV_ERROR("can't get read private_data .\n");
        return -EINVAL;
    }

    ret = device_read(spi_dev, (uint32_t)*offset, val, count);
    if (ret < 0) {
        SPI_DEV_ERROR("spi dev read failed, dev name:%s, offset:0x%x, len:%lu.\n",
            spi_dev->name, (uint32_t)*offset, count);
        return -EINVAL;
    }

    if (copy_to_user(buf, val, count)) {
        SPI_DEV_ERROR("copy_to_user error \n");
        return -EFAULT;
    } else{
        *offset += count;
    }

    return count;
}

static ssize_t spi_dev_write(struct file *file, const char __user *buf,
                   size_t count, loff_t *offset)
{
    u8 val[MAX_RW_LEN];
    int ret;
    struct spi_dev_info *spi_dev;

    if (count <= 0 || count > sizeof(val)) {
        SPI_DEV_ERROR("write conut %lu, beyond max val:%lu.\n", count, sizeof(val));
        return -EINVAL;
    }

    spi_dev = file->private_data;
    if (spi_dev == NULL) {
        SPI_DEV_ERROR("get write private_data error.\n");
        return -EINVAL;
    }

    mem_clear(val, sizeof(val));
    if (copy_from_user(val, buf, count)) {
        SPI_DEV_ERROR("copy_from_user error.\n");
        return -EFAULT;
    }

    ret = device_write(spi_dev, (uint32_t)*offset, val, count);
    if (ret < 0) {
        SPI_DEV_ERROR("spi dev write failed, dev name:%s, offset:0x%llx, len:%lu.\n",
            spi_dev->name, *offset, count);
        return -EINVAL;
    }

    *offset += count;
    return count;
}

static loff_t spi_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;

    switch (origin) {
    case SEEK_SET:
        if (offset < 0) {
            SPI_DEV_ERROR("SEEK_SET, offset:%lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:
        if (file->f_pos + offset < 0) {
            SPI_DEV_ERROR("SEEK_CUR out of range, f_ops:%lld, offset:%lld.\n",
                 file->f_pos, offset);
        }
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        SPI_DEV_ERROR("unsupport llseek type:%d.\n", origin);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static const struct file_operations spi_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = spi_dev_llseek,
    .read       = spi_dev_read,
    .write      = spi_dev_write,
    .unlocked_ioctl = spi_dev_ioctl,
    .open       = spi_dev_open,
    .release    = spi_dev_release,
};

static struct spi_dev_info * dev_match(const char *path)
{
    struct spi_dev_info * spi_dev;
    char dev_name[MAX_NAME_SIZE];
    int i;
    for (i = 0; i < MAX_SPI_DEV_NUM; i++) {
        if (spi_dev_arry[ i ] == NULL) {
            continue;
        }
        spi_dev = spi_dev_arry[ i ];
        snprintf(dev_name, MAX_NAME_SIZE,"/dev/%s", spi_dev->name);
        if (!strcmp(path, dev_name)) {
            SPI_DEV_DEBUG("get dev_name = %s, minor = %d\n", dev_name, i);
            return spi_dev;
        }
    }

    return NULL;
}

int spi_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct spi_dev_info *spi_dev = NULL;
    int ret;

    if(path == NULL){
        SPI_DEV_ERROR("path NULL");
        return -EINVAL;
    }

    if(buf == NULL){
        SPI_DEV_ERROR("buf NULL");
        return -EINVAL;
    }

    if (count > MAX_RW_LEN) {
        SPI_DEV_ERROR("read conut %lu, beyond max:%d.\n", count, MAX_RW_LEN);
        return -EINVAL;
    }

    spi_dev = dev_match(path);
    if (spi_dev == NULL) {
        SPI_DEV_ERROR("spi_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    ret = device_read(spi_dev, offset, buf, count);
    if (ret < 0) {
        SPI_DEV_ERROR("spi dev read failed, dev name:%s, offset:0x%x, len:%lu.\n",
            spi_dev->name, offset, count);
        return -EINVAL;
    }

    return count;
}
EXPORT_SYMBOL(spi_device_func_read);

int spi_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count)
{
    struct spi_dev_info *spi_dev = NULL;
    int ret;

    if(path == NULL){
        SPI_DEV_ERROR("path NULL");
        return -EINVAL;
    }

    if(buf == NULL){
        SPI_DEV_ERROR("buf NULL");
        return -EINVAL;
    }

    if (count > MAX_RW_LEN) {
        SPI_DEV_ERROR("write conut %lu, beyond max:%d.\n", count, MAX_RW_LEN);
        return -EINVAL;
    }

    spi_dev = dev_match(path);
    if (spi_dev == NULL) {
        SPI_DEV_ERROR("i2c_dev match failed. dev path = %s", path);
        return -EINVAL;
    }

    ret = device_write (spi_dev, offset, buf, count);
    if (ret < 0) {
        SPI_DEV_ERROR("i2c dev write failed, dev name:%s, offset:0x%x, len:%lu.\n",
            spi_dev->name, offset, count);
        return -EINVAL;
    }

    return count;
}
EXPORT_SYMBOL(spi_device_func_write);

static int spi_dev_probe(struct spi_device *spi)
{
    int ret;
    struct spi_dev_info *spi_dev;
    struct miscdevice *misc;
    spi_dev_device_t *spi_dev_device;

    spi_dev = devm_kzalloc(&spi->dev, sizeof(struct spi_dev_info), GFP_KERNEL);
    if (!spi_dev) {
        dev_err(&spi->dev, "devm_kzalloc error. \n");
        return -ENOMEM;
    }

    spi_set_drvdata(spi, spi_dev);
    spi_dev->spi_device = spi;

    if (spi->dev.of_node) {

        ret = 0;
        ret += of_property_read_string(spi->dev.of_node, "spi_dev_name", &spi_dev->name);
        ret += of_property_read_u32(spi->dev.of_node, "data_bus_width", &spi_dev->data_bus_width);
        ret += of_property_read_u32(spi->dev.of_node, "addr_bus_width", &spi_dev->addr_bus_width);
        ret += of_property_read_u32(spi->dev.of_node, "per_rd_len", &spi_dev->per_rd_len);
        ret += of_property_read_u32(spi->dev.of_node, "per_wr_len", &spi_dev->per_wr_len);
        if (ret != 0) {
            dev_err(&spi->dev, "dts config error.ret:%d.\n", ret);
            return -ENXIO;
        }
    } else {
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
    }

    if ((spi_dev->per_rd_len & (spi_dev->data_bus_width - 1))
            || (spi_dev->per_wr_len & (spi_dev->data_bus_width - 1))) {
        dev_err(&spi->dev, "Invalid config per_rd_len [%u] per_wr_len [%u] data bus_width [%u], addr bus width [%u].\n",
            spi_dev->per_rd_len, spi_dev->per_wr_len, spi_dev->data_bus_width, spi_dev->addr_bus_width);
        return -ENXIO;
    }

    misc = &spi_dev->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = spi_dev->name;
    misc->fops = &spi_dev_fops;
    misc->mode = 0666;
    if (misc_register(misc) != 0) {
        dev_err(&spi->dev, "register %s faild.\n", misc->name);
        return -ENXIO;
    }

    if (misc->minor >= MAX_SPI_DEV_NUM) {
        dev_err(&spi->dev, "minor number beyond the limit! is %d.\n", misc->minor);
        misc_deregister(misc);
        return -ENXIO;
    }
    spi_dev_arry[misc->minor] = spi_dev;

    dev_info(&spi->dev, "register %u data_bus_width %u addr_bus_witdh device %s with %u per_rd_len %u per_wr_len success.\n",
        spi_dev->data_bus_width, spi_dev->addr_bus_width, spi_dev->name, spi_dev->per_rd_len, spi_dev->per_wr_len);

    return 0;
}

static int spi_dev_remove(struct spi_device *spi)
{
    int i;

    for (i = 0; i < MAX_SPI_DEV_NUM; i++) {
        if (spi_dev_arry[i] != NULL) {
            misc_deregister(&spi_dev_arry[i]->misc);
            spi_dev_arry[i] = NULL;
        }
    }
    return 0;
}

static const struct of_device_id spi_dev_of_match[] = {
    { .compatible = "wb-spi-dev" },
    { },
};

MODULE_DEVICE_TABLE(of, spi_dev_of_match);

static struct spi_driver spi_dev_driver = {
    .driver = {
        .name = "wb-spi-dev",
        .of_match_table = of_match_ptr(spi_dev_of_match),
    },
    .probe      = spi_dev_probe,
    .remove     = spi_dev_remove,
};

module_spi_driver(spi_dev_driver);

MODULE_DESCRIPTION("spi dev driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
