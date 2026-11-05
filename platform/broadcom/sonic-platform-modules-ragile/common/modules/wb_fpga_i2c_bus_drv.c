/*
 * fpga_i2c_bus_drv.c
 * ko to create fpga i2c adapter
 */
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
#include <linux/of_i2c.h>
#endif
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/of.h>
#include "fpga_i2c.h"

#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRV_NAME                      "wb-fpga-i2c"
#define DRV_VERSION                   "1.0"
#define DTS_NO_CFG_FLAG               (0)

extern int i2c_device_func_write(const char *path, uint32_t pos, uint8_t *val, size_t size);
extern int i2c_device_func_read(const char *path, uint32_t pos, uint8_t *val, size_t size);
extern int pcie_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int pcie_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int io_device_func_write(const char *path, uint32_t pos, uint8_t *val, size_t size);
extern int io_device_func_read(const char *path, uint32_t pos, uint8_t *val, size_t size);
extern int spi_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int spi_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);


#define FPGA_I2C_STRETCH_TIMEOUT  (0x01)
#define FPGA_I2C_DEADLOCK_FAILED  (0x02)
#define FPGA_I2C_SLAVE_NO_RESPOND (0x03)
#define FPGA_I2C_STA_FAIL         (0x01)
#define FPGA_I2C_STA_BUSY         (0x02)
#define FPGA_I2C_CTL_BG           (0x01 << 1)
#define FPGA_I2C_CTL_NO_REG       (0x01 << 2)
#define FPGA_I2C_CTL_RD           (0x01)
#define FPGA_I2C_CTL_WR           (0x00)
#define I2C_READ_MSG_NUM          (0x02)
#define I2C_WRITE_MSG_NUM         (0x01)
#define FPGA_REG_WIDTH            (4)

#define SYMBOL_I2C_DEV_MODE       (1)
#define FILE_MODE                 (2)
#define SYMBOL_PCIE_DEV_MODE      (3)
#define SYMBOL_IO_DEV_MODE        (4)
#define SYMBOL_SPI_DEV_MODE       (5)

int g_wb_fpga_i2c_debug = 0;
int g_wb_fpga_i2c_error = 0;

module_param(g_wb_fpga_i2c_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_fpga_i2c_error, int, S_IRUGO | S_IWUSR);

#define FPGA_I2C_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_fpga_i2c_debug) { \
        printk(KERN_INFO "[FPFA_I2C_BUS][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_I2C_ERROR(fmt, args...) do {                                        \
    if (g_wb_fpga_i2c_error) { \
        printk(KERN_ERR "[FPFA_I2C_BUS][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static int fpga_file_read(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        FPGA_I2C_ERROR("read open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_read(filp, val, size, &tmp_pos);
    if (ret < 0) {
        FPGA_I2C_ERROR("kernel_read failed, path=%s, addr=0x%x, size=%ld, ret=%d\r\n", path, pos, size, ret);
        goto exit;
    }

    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

static int fpga_file_write(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        FPGA_I2C_ERROR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_write(filp, val, size, &tmp_pos);
    if (ret < 0) {
        FPGA_I2C_ERROR("kernel_write failed, path=%s, addr=0x%x, size=%ld, ret=%d\r\n", path, pos, size, ret);
        goto exit;
    }
    vfs_fsync(filp, 1);
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

static int fpga_device_write(fpga_i2c_dev_t *fpga_i2c, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;

    switch (fpga_i2c->i2c_func_mode) {
    case SYMBOL_I2C_DEV_MODE:
        ret = i2c_device_func_write(fpga_i2c->dev_name, pos, val, size);
        break;
    case FILE_MODE:
        ret = fpga_file_write(fpga_i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_PCIE_DEV_MODE:
        ret = pcie_device_func_write(fpga_i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_IO_DEV_MODE:
        ret = io_device_func_write(fpga_i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_SPI_DEV_MODE:
        ret = spi_device_func_write(fpga_i2c->dev_name, pos, val, size);
        break;
    default:
        FPGA_I2C_ERROR("err func_mode, write failed.\n");
        return -EINVAL;
    }
    return ret;

}

static int fpga_device_read(fpga_i2c_dev_t *fpga_i2c, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;

    switch (fpga_i2c->i2c_func_mode) {
    case SYMBOL_I2C_DEV_MODE:
        ret = i2c_device_func_read(fpga_i2c->dev_name, pos, val, size);
        break;
    case FILE_MODE:
        ret = fpga_file_read(fpga_i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_PCIE_DEV_MODE:
        ret = pcie_device_func_read(fpga_i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_IO_DEV_MODE:
        ret = io_device_func_read(fpga_i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_SPI_DEV_MODE:
        ret = spi_device_func_read(fpga_i2c->dev_name, pos, val, size);
        break;
    default:
        FPGA_I2C_ERROR("err func_mode, read failed.\n");
        return -EINVAL;
    }

    return ret;
}

static int little_endian_dword_to_buf(uint8_t *buf, int len, uint32_t dword)
{
    uint8_t tmp_buf[FPGA_REG_WIDTH];

    if (len < 4) {
        FPGA_I2C_ERROR("Not enough buf, dword to buf: len[%d], dword[0x%x]\n", len, dword);
        return -1;
    }

    mem_clear(tmp_buf, sizeof(tmp_buf));
    tmp_buf[0] = dword & 0xff;
    tmp_buf[1] = (dword >> 8) & 0xff;
    tmp_buf[2] = (dword >> 16) & 0xff;
    tmp_buf[3] = (dword >> 24) & 0xff;

    memcpy(buf, tmp_buf, sizeof(tmp_buf));

    return 0;
}

static int little_endian_buf_to_dword(uint8_t *buf, int len, uint32_t *dword)
{
    int i;
    uint32_t dword_tmp;

    if (len != FPGA_REG_WIDTH) {
        FPGA_I2C_ERROR("buf length %d error, can't convert to dowrd.\n", len);
        return -1;
    }
    dword_tmp = 0;
    for (i = 0; i < FPGA_REG_WIDTH; i++) {
        dword_tmp |= (buf[i] << (i * 8));
    }
    *dword = dword_tmp;
    return 0;
}

static int fpga_reg_write(fpga_i2c_dev_t *fpga_i2c, uint32_t addr, uint8_t val)
{
    int ret;

    ret = fpga_device_write(fpga_i2c, addr, &val, sizeof(uint8_t));
    if (ret < 0) {
        FPGA_I2C_ERROR("fpga reg write failed, dev name:%s, offset:0x%x, value:0x%x.\n",
            fpga_i2c->dev_name, addr, val);
        return -EIO;
    }

    FPGA_I2C_VERBOSE("fpga reg write success, dev name:%s, offset:0x%x, value:0x%x.\n",
        fpga_i2c->dev_name, addr, val);
    return 0;
}

static int fpga_reg_read(fpga_i2c_dev_t *fpga_i2c, uint32_t addr, uint8_t *val)
{
    int ret;

    ret = fpga_device_read(fpga_i2c, addr, val, sizeof(uint8_t));
    if (ret < 0) {
        FPGA_I2C_ERROR("fpga reg read failed, dev name:%s, offset:0x%x\n",
            fpga_i2c->dev_name, addr);
        return -EIO;
    }

    FPGA_I2C_VERBOSE("fpga reg read success, dev name:%s, offset:0x%x, value:0x%x.\n",
        fpga_i2c->dev_name, addr, *val);
    return 0;
}

static int fpga_data_write(fpga_i2c_dev_t *fpga_i2c, uint32_t addr, uint8_t *val, size_t size)
{
    int ret;

    ret = fpga_device_write(fpga_i2c, addr, val, size);
    if (ret < 0) {
        FPGA_I2C_ERROR("fpga data write failed, dev name:%s, offset:0x%x, size:%lu.\n",
            fpga_i2c->dev_name, addr, size);
        return -EIO;
    }

    FPGA_I2C_VERBOSE("fpga data write success, dev name:%s, offset:0x%x, size:%lu.\n",
        fpga_i2c->dev_name, addr, size);
    return 0;
}

static int fpga_data_read(fpga_i2c_dev_t *fpga_i2c, uint32_t addr, uint8_t *val, size_t size)
{
    int ret;

    ret = fpga_device_read(fpga_i2c, addr, val, size);
    if (ret < 0) {
        FPGA_I2C_ERROR("fpga data read failed, dev name:%s, offset:0x%x, size:%lu.\n",
            fpga_i2c->dev_name, addr, size);
        return -EIO;
    }

    FPGA_I2C_VERBOSE("fpga data read success, dev name:%s, offset:0x%x, size:%lu.\n",
        fpga_i2c->dev_name, addr, size);
    return 0;
}

static int fpga_reg_write_32(fpga_i2c_dev_t *fpga_i2c, uint32_t addr, uint32_t val)
{
    int ret;
    uint8_t buf[FPGA_REG_WIDTH];

    mem_clear(buf, sizeof(buf));
    little_endian_dword_to_buf(buf, sizeof(buf), val);
    ret = fpga_device_write(fpga_i2c, addr, buf, sizeof(buf));
    if (ret < 0) {
        FPGA_I2C_ERROR("fpga reg write failed, dev name: %s, offset: 0x%x, value: 0x%x.\n",
            fpga_i2c->dev_name, addr, val);
        return -EIO;
    }

    FPGA_I2C_VERBOSE("fpga reg write success, dev name: %s, offset: 0x%x, value: 0x%x.\n",
        fpga_i2c->dev_name, addr, val);
    return 0;
}

static int fpga_reg_read_32(fpga_i2c_dev_t *fpga_i2c, uint32_t addr, uint32_t *val)
{
    int ret;
    uint8_t buf[FPGA_REG_WIDTH];

    mem_clear(buf, sizeof(buf));
    ret = fpga_device_read(fpga_i2c, addr, buf, sizeof(buf));
    if (ret < 0) {
        FPGA_I2C_ERROR("fpga reg read failed, dev name: %s, offset: 0x%x, ret: %d\n",
            fpga_i2c->dev_name, addr, ret);
        return -EIO;
    }
    little_endian_buf_to_dword(buf, sizeof(buf), val);
    FPGA_I2C_VERBOSE("fpga reg read success, dev name: %s, offset: 0x%x, value: 0x%x.\n",
        fpga_i2c->dev_name, addr, *val);
    return 0;
}

static int fpga_i2c_is_busy(fpga_i2c_dev_t *fpga_i2c)
{
    uint8_t val;
    int ret;
    fpga_i2c_reg_t *reg;

    reg = &fpga_i2c->reg;
    ret = fpga_reg_read(fpga_i2c, reg->i2c_status, &val);
    if (ret < 0 ) {
        FPGA_I2C_ERROR("read fpga i2c status reg failed, reg addr:0x%x, ret:%d.\n",
            reg->i2c_status, ret);
        return 1;
    }
    if (val & FPGA_I2C_STA_BUSY) {
        FPGA_I2C_ERROR("fpga i2c status busy, reg addr:0x%x, value:0x%x.\n",
            reg->i2c_status, val);
        return 1;
    } else {
        return 0;
    }
}

static int fpga_i2c_wait(fpga_i2c_dev_t *fpga_i2c)
{
    int retry_cnt;

    retry_cnt = FPGA_I2C_XFER_TIME_OUT/FPGA_I2C_SLEEP_TIME;
    while (retry_cnt--) {
        if (fpga_i2c_is_busy(fpga_i2c)) {
            usleep_range(FPGA_I2C_SLEEP_TIME, FPGA_I2C_SLEEP_TIME + 1);
        } else {
            return 0;
        }
    }

    return -EBUSY;
}

static int fpga_i2c_check_status(fpga_i2c_dev_t *fpga_i2c)
{
    uint8_t data;
    int ret;
    fpga_i2c_reg_t *reg;

    reg = &fpga_i2c->reg;

    ret = fpga_reg_read(fpga_i2c, reg->i2c_status, &data);
    if (ret) {
        FPGA_I2C_ERROR("read fpga i2c status reg failed, reg addr:0x%x, ret:%d.\n",
            reg->i2c_status, ret);
        return ret;
    }

    if (data & FPGA_I2C_STA_FAIL) {
        FPGA_I2C_ERROR("fpga i2c status error, reg addr:0x%x, value:%d.\n",
            reg->i2c_status, data);

        /* read i2c_err_vec to confirm err type*/
        if (reg->i2c_err_vec != DTS_NO_CFG_FLAG) {
            /* read i2c_err_vec reg */
            ret = fpga_reg_read(fpga_i2c, reg->i2c_err_vec, &data);
            if (ret) {
                FPGA_I2C_ERROR("read fpga i2c err vec reg failed, reg addr:0x%x, ret:%d.\n",
                    reg->i2c_err_vec, ret);
                return ret;
            }
            FPGA_I2C_VERBOSE("get i2c err vec, reg addr:0x%x, read value:0x%x\n", reg->i2c_err_vec, data);

            /* match i2c_err_vec reg value and err type*/
            switch (data) {
            case FPGA_I2C_STRETCH_TIMEOUT:
                ret = -ETIMEDOUT;
                break;
            case FPGA_I2C_DEADLOCK_FAILED:
                ret = -EDEADLK;
                break;
            case FPGA_I2C_SLAVE_NO_RESPOND:
                ret = -ENXIO;
                break;
            default:
                FPGA_I2C_ERROR("get i2c err vec value out of range, reg addr:0x%x, read value:0x%x\n",
                    reg->i2c_err_vec, data);
                ret = -EREMOTEIO;
                break;
            }
            return ret;
        } else {
            FPGA_I2C_VERBOSE("i2c err vec not config, fpga i2c status check return -1\n");
            return -EREMOTEIO;
        }
    }
    return 0;
}

static int fpga_i2c_do_work(fpga_i2c_dev_t *fpga_i2c, int i2c_addr,
        unsigned char *data, uint32_t length, int is_read)
{
    int ret, i;
    uint8_t op, i2c_reg_addr_len;
    uint8_t *i2c_read_addr_buf;
    fpga_i2c_reg_t *reg;
    fpga_i2c_reg_addr_t *i2c_addr_desc;

    reg = &fpga_i2c->reg;

    ret = fpga_reg_write(fpga_i2c, reg->i2c_slave, i2c_addr);
    if (ret) {
        FPGA_I2C_ERROR("write fpga i2c slave reg failed, reg addr:0x%x, value:0x%x, ret:%d.\n",
            reg->i2c_slave, i2c_addr, ret);
        goto exit;
    }

    i2c_addr_desc = &fpga_i2c->i2c_addr_desc;
    i2c_reg_addr_len = i2c_addr_desc->reg_addr_len;
    i2c_read_addr_buf = &i2c_addr_desc->read_reg_addr[0];

    if (i2c_reg_addr_len > 0 && i2c_reg_addr_len <= I2C_REG_MAX_WIDTH) {
        ret = fpga_data_write(fpga_i2c, reg->i2c_reg, i2c_read_addr_buf, i2c_reg_addr_len);
        if (ret) {
            FPGA_I2C_ERROR("write fpga i2c offset reg failed, fpga addr:0x%x, reg len:%d, ret:%d\n",
                reg->i2c_reg, i2c_reg_addr_len, ret);
            for (i = 0; i < i2c_reg_addr_len; i++) {
                FPGA_I2C_ERROR("%02d : %02x\n", i, i2c_read_addr_buf[i]);
            }
            goto exit;
        }
    }

    ret = fpga_reg_write_32(fpga_i2c, reg->i2c_data_len, length);
    if (ret) {
        FPGA_I2C_ERROR("write fpga i2c date len reg failed, reg addr:0x%x, value:0x%x, ret:%d.\n",
            reg->i2c_data_len, length, ret);
        goto exit;
    }

    ret = fpga_reg_write(fpga_i2c, reg->i2c_reg_len, i2c_reg_addr_len);
    if (ret) {
        FPGA_I2C_ERROR("write fpga i2c reg len reg failed, reg addr:0x%x, value:0x%x, ret:%d.\n",
            reg->i2c_reg_len, i2c_reg_addr_len, ret);
        goto exit;
    }

    if (is_read) {
        op = FPGA_I2C_CTL_RD | FPGA_I2C_CTL_BG;
    } else {

        ret = fpga_data_write(fpga_i2c, reg->i2c_data_buf, data, length);
        if (ret) {
            FPGA_I2C_ERROR("write fpga i2c date buf failed, reg addr:0x%x, write len:%d, ret:%d.\n",
                reg->i2c_data_buf, length, ret);
            goto exit;
        }
        op = FPGA_I2C_CTL_WR | FPGA_I2C_CTL_BG ;
    }

    ret = fpga_reg_write(fpga_i2c, reg->i2c_ctrl, op);
    if (ret) {
        FPGA_I2C_ERROR("write fpga i2c control reg failed, reg addr:0x%x, value:%d, ret:%d.\n",
            reg->i2c_ctrl, op, ret);
        goto exit;
    }

    ret = fpga_i2c_wait(fpga_i2c);
    if (ret) {
        FPGA_I2C_ERROR("wait fpga i2c status timeout.\n");
        goto exit;
    }

    ret = fpga_i2c_check_status(fpga_i2c);
    if (ret) {
        FPGA_I2C_ERROR("check fpga i2c status error.\n");
        goto exit;
    }

    if (is_read) {

        ret = fpga_data_read(fpga_i2c, reg->i2c_data_buf, data, length);
        if (ret) {
            FPGA_I2C_ERROR("read fpga i2c data buf failed, reg addr:0x%x, read len:%d, ret:%d.\n",
                reg->i2c_data_buf, length, ret);
            goto exit;
        }
    }

exit:
    return ret;
}

static int fpga_i2c_write(fpga_i2c_dev_t *fpga_i2c, int target,
                u8 *data, int length, int i2c_msg_num)
{
    int ret, i;
    fpga_i2c_reg_addr_t *i2c_addr_desc;

    if (i2c_msg_num == I2C_READ_MSG_NUM) {

        if (length > I2C_REG_MAX_WIDTH) {
            FPGA_I2C_ERROR("read reg addr len %d, more than max length.\n", length);
            return -EINVAL;
        }

        i2c_addr_desc = &fpga_i2c->i2c_addr_desc;
        for (i = 0; i < length; i++) {
            i2c_addr_desc->read_reg_addr[i] = data[length -i -1];
            FPGA_I2C_VERBOSE("%02d : %02x\n", i, i2c_addr_desc->read_reg_addr[i]);
        }
        i2c_addr_desc->reg_addr_len = length;
        ret = 0;
    } else {

        ret = fpga_i2c_do_work(fpga_i2c, target, data, length, 0);
    }

    return ret;
}

/**
 * fpga_i2c_read - receive data from the bus.
 * @i2c: The struct fpga_i2c_dev_t.
 * @target: Target address.
 * @data: Pointer to the location to store the datae .
 * @length: Length of the data.
 *
 * The address is sent over the bus, then the data is read.
 *
 * Returns 0 on success, otherwise a negative errno.
 */
static int fpga_i2c_read(fpga_i2c_dev_t *fpga_i2c, int target,
        u8 *data, int length)
{
    int ret, offset_size;
    int i, tmp_val;
    fpga_i2c_reg_addr_t *i2c_addr_desc;
    uint8_t i2c_reg_addr_len;
    uint8_t *i2c_read_addr_buf;

    offset_size = 0;
    i2c_addr_desc = &fpga_i2c->i2c_addr_desc;
    i2c_reg_addr_len = i2c_addr_desc->reg_addr_len;
    i2c_read_addr_buf = &i2c_addr_desc->read_reg_addr[0];

    while (1) {
        if (length <= fpga_i2c->reg.i2c_data_buf_len) {
            return fpga_i2c_do_work(fpga_i2c, target, data + offset_size, length, 1);
        }

        ret = fpga_i2c_do_work(fpga_i2c, target, data + offset_size, fpga_i2c->reg.i2c_data_buf_len, 1);
        if (ret != 0) {
            FPGA_I2C_ERROR("fpga_i2c_read failed, i2c addr:0x%x, offset:0x%x, ret:%d.\n",
                target, offset_size, ret);
            return ret;
        }

        tmp_val = i2c_read_addr_buf[0];
        tmp_val += fpga_i2c->reg.i2c_data_buf_len;
        if (tmp_val > 0xff) {
            i2c_read_addr_buf[0] = tmp_val & 0xff;
            for (i = 1; i < i2c_reg_addr_len; i++) {
                if (i2c_read_addr_buf[i] == 0xff) {
                    i2c_read_addr_buf[i] = 0;
                } else {
                    i2c_read_addr_buf[i]++;
                    break;
                }
            }
        } else {
            i2c_read_addr_buf[0] = tmp_val & 0xff;
        }
        offset_size += fpga_i2c->reg.i2c_data_buf_len;
        length -= fpga_i2c->reg.i2c_data_buf_len;
    }

    return ret;
}

static void fpga_i2c_reset(fpga_i2c_dev_t *fpga_i2c) {
    fpga_i2c_reset_cfg_t *reset_cfg;
    uint32_t reset_addr;

    reset_cfg = &fpga_i2c->reset_cfg;
    reset_addr = reset_cfg->reset_addr;
    if (reset_cfg->reset_delay_b) {
        usleep_range(reset_cfg->reset_delay_b, reset_cfg->reset_delay_b + 1);
    }

    fpga_reg_write_32(fpga_i2c, reset_addr, reset_cfg->reset_on);
    if (reset_cfg->reset_delay) {
        usleep_range(reset_cfg->reset_delay, reset_cfg->reset_delay + 1);
    }

    fpga_reg_write_32(fpga_i2c, reset_addr, reset_cfg->reset_off);
    if (reset_cfg->reset_delay_a) {
        usleep_range(reset_cfg->reset_delay_a, reset_cfg->reset_delay_a + 1);
    }

    return;
}

/**
 * fpga_i2c_xfer - The driver's master_xfer function.
 * @adap: Pointer to the i2c_adapter structure.
 * @msgs: Pointer to the messages to be processed.
 * @num: Length of the MSGS array.
 *
 * Returns the number of messages processed, or a negative errno on
 * failure.
 */
static int fpga_i2c_adapter_init(fpga_i2c_dev_t *fpga_i2c)
{
    int ret;
    fpga_i2c_reg_t *reg;

    reg = &fpga_i2c->reg;

    ret = 0;
    ret += fpga_reg_write(fpga_i2c, reg->i2c_scale, fpga_i2c->i2c_scale_value);
    ret += fpga_reg_write(fpga_i2c, reg->i2c_filter, fpga_i2c->i2c_filter_value);
    ret += fpga_reg_write(fpga_i2c, reg->i2c_stretch, fpga_i2c->i2c_stretch_value);
    if (ret < 0) {
        FPGA_I2C_ERROR("fpga_i2c_init failed.\n");
        return ret;
    }

    FPGA_I2C_VERBOSE("fpga_i2c_init ok.\n");
    return 0;
}

static int fpga_i2c_params_check(fpga_i2c_dev_t *fpga_i2c)
{
    int ret;
    fpga_i2c_reg_t *reg;
    uint8_t i2c_scale_value, i2c_filter_value, i2c_stretch_value;

    reg = &fpga_i2c->reg;
    ret = 0;
    ret += fpga_reg_read(fpga_i2c, reg->i2c_scale, &i2c_scale_value);
    ret += fpga_reg_read(fpga_i2c, reg->i2c_filter, &i2c_filter_value);
    ret += fpga_reg_read(fpga_i2c, reg->i2c_stretch, &i2c_stretch_value);
    if (ret < 0) {
        FPGA_I2C_ERROR("read fpga i2c params failed.\n");
        return 1;
    }

    if ((i2c_scale_value != fpga_i2c->i2c_scale_value)
            || (i2c_filter_value != fpga_i2c->i2c_filter_value)
            || (i2c_stretch_value != fpga_i2c->i2c_stretch_value)) {
        FPGA_I2C_ERROR("fpga i2c params check error, read value: i2c_scale 0x%x, i2c_filter:0x%x, i2c_stretch:0x%x.\n",
            i2c_scale_value, i2c_filter_value, i2c_stretch_value);
        FPGA_I2C_ERROR("fpga i2c params check error, config value: i2c_scale 0x%x, i2c_filter:0x%x, i2c_stretch:0x%x.\n",
            fpga_i2c->i2c_scale_value, fpga_i2c->i2c_filter_value, fpga_i2c->i2c_stretch_value);
        return 1;
    }

    FPGA_I2C_VERBOSE("fpga i2c params check ok.\n");
    return 0;
}

static int fpga_i2c_xfer(struct i2c_adapter *adap,
        struct i2c_msg *msgs, int num)
{
    struct i2c_msg *pmsg;
    int i;
    int ret;
    fpga_i2c_dev_t *fpga_i2c;
    fpga_i2c_reg_addr_t *i2c_addr_desc;

    fpga_i2c = i2c_get_adapdata(adap);

    if (num != I2C_READ_MSG_NUM && num != I2C_WRITE_MSG_NUM) {
        FPGA_I2C_ERROR("unsupport i2c_msg len:%d.\n", num);
        return -EINVAL;
    }

    if ((num == I2C_WRITE_MSG_NUM) && (msgs[0].len > fpga_i2c->reg.i2c_data_buf_len)) {
        FPGA_I2C_ERROR("unsupport i2c_msg type:msg[0].flag:0x%x, buf len:0x%x.\n",
            msgs[0].flags, msgs[0].len);
        return -EINVAL;
    }

    if (num == I2C_READ_MSG_NUM ) {
        if ((msgs[0].flags & I2C_M_RD) ||!(msgs[1].flags & I2C_M_RD)) {
            FPGA_I2C_ERROR("unsupport i2c_msg type:msg[0].flag:0x%x, msg[1].flag:0x%x.\n",
                msgs[0].flags, msgs[1].flags);
            return -EINVAL;
        }
    }

    if (fpga_i2c_is_busy(fpga_i2c)) {
        FPGA_I2C_ERROR("fpga i2c adapter %d is busy, do reset.\n", adap->nr);
        if (fpga_i2c->reset_cfg.i2c_adap_reset_flag == 1) {

            fpga_i2c_reset(fpga_i2c);

            fpga_i2c_adapter_init(fpga_i2c);
        }
        return -EAGAIN;
    }

    if (fpga_i2c->i2c_params_check && fpga_i2c_params_check(fpga_i2c)) {
        FPGA_I2C_ERROR("fpga i2c params check failed, try to reinitialize.\n");
        fpga_i2c_adapter_init(fpga_i2c);
    }

    ret = 0;
    i2c_addr_desc = &fpga_i2c->i2c_addr_desc;
    i2c_addr_desc->reg_addr_len = 0;
    mem_clear(i2c_addr_desc->read_reg_addr, sizeof(i2c_addr_desc->read_reg_addr));

    for (i = 0; ret == 0 && i < num; i++) {
        pmsg = &msgs[i];
        FPGA_I2C_VERBOSE("Doing %s %d byte(s) to/from 0x%02x - %d of %d messages\n",
            pmsg->flags & I2C_M_RD ? "read" : "write", pmsg->len, pmsg->addr, i + 1, num);

        if (pmsg->flags & I2C_M_RD) {
            ret = fpga_i2c_read(fpga_i2c, pmsg->addr, pmsg->buf, pmsg->len);

            if ((pmsg->len == 1) && (pmsg->flags & I2C_M_RECV_LEN)) {
                if ((ret != 0) || (pmsg->buf[0] > I2C_SMBUS_BLOCK_MAX)) {
                    FPGA_I2C_ERROR("smbus block data read failed, ret:%d, read len:%u.\n",
                        ret, pmsg->buf[0]);
                    return -EPROTO;
                }
                pmsg->len = 1 + pmsg->buf[0];
                FPGA_I2C_VERBOSE("smbus block data read, read len:%d.\n", pmsg->len);
                ret = fpga_i2c_read(fpga_i2c, pmsg->addr, pmsg->buf, pmsg->len);
            }
        } else {
            ret = fpga_i2c_write(fpga_i2c, pmsg->addr, pmsg->buf, pmsg->len, num);
        }
    }

    return (ret != 0) ? ret : num;
}

static u32 fpga_i2c_functionality(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static const struct i2c_algorithm fpga_i2c_algo = {
    .master_xfer = fpga_i2c_xfer,
    .functionality = fpga_i2c_functionality,
};

static struct i2c_adapter fpga_i2c_ops = {
    .owner = THIS_MODULE,
    .name = "wb_fpga_i2c",
    .algo = &fpga_i2c_algo,
};

static int fpga_i2c_config_init(fpga_i2c_dev_t *fpga_i2c)
{
    int ret = 0, rv = 0;
    fpga_i2c_reg_t *reg;
    fpga_i2c_reset_cfg_t *reset_cfg;
    struct device *dev;
    uint32_t i2c_offset_reg, i2c_data_buf_len_reg;
    int32_t i2c_offset_val;

    fpga_i2c_bus_device_t *fpga_i2c_bus_device;

    dev = fpga_i2c->dev;
    reg = &fpga_i2c->reg;
    reset_cfg = &fpga_i2c->reset_cfg;

    i2c_offset_val = 0;

    if (dev->of_node) {
        ret = 0;
        ret += of_property_read_u32(dev->of_node, "i2c_ext_9548_addr", &reg->i2c_ext_9548_addr);
        ret += of_property_read_u32(dev->of_node, "i2c_ext_9548_chan", &reg->i2c_ext_9548_chan);
        ret += of_property_read_u32(dev->of_node, "i2c_slave", &reg->i2c_slave);
        ret += of_property_read_u32(dev->of_node, "i2c_reg", &reg->i2c_reg);
        ret += of_property_read_u32(dev->of_node, "i2c_data_len", &reg->i2c_data_len);
        ret += of_property_read_u32(dev->of_node, "i2c_ctrl", &reg->i2c_ctrl);
        ret += of_property_read_u32(dev->of_node, "i2c_status", &reg->i2c_status);
        ret += of_property_read_u32(dev->of_node, "i2c_scale", &reg->i2c_scale);
        ret += of_property_read_u32(dev->of_node, "i2c_filter", &reg->i2c_filter);
        ret += of_property_read_u32(dev->of_node, "i2c_stretch", &reg->i2c_stretch);
        ret += of_property_read_u32(dev->of_node, "i2c_ext_9548_exits_flag", &reg->i2c_ext_9548_exits_flag);
        ret += of_property_read_u32(dev->of_node, "i2c_reg_len", &reg->i2c_reg_len);
        ret += of_property_read_u32(dev->of_node, "i2c_in_9548_chan", &reg->i2c_in_9548_chan);
        ret += of_property_read_u32(dev->of_node, "i2c_data_buf", &reg->i2c_data_buf);
        ret += of_property_read_string(dev->of_node, "dev_name", &fpga_i2c->dev_name);
        ret += of_property_read_u32(dev->of_node, "i2c_scale_value", &fpga_i2c->i2c_scale_value);
        ret += of_property_read_u32(dev->of_node, "i2c_filter_value", &fpga_i2c->i2c_filter_value);
        ret += of_property_read_u32(dev->of_node, "i2c_stretch_value", &fpga_i2c->i2c_stretch_value);
        ret += of_property_read_u32(dev->of_node, "i2c_timeout", &fpga_i2c->i2c_timeout);
        ret += of_property_read_u32(dev->of_node, "i2c_func_mode", &fpga_i2c->i2c_func_mode);
        ret += of_property_read_u32(dev->of_node, "i2c_reset_addr", &reset_cfg->reset_addr);
        ret += of_property_read_u32(dev->of_node, "i2c_reset_on", &reset_cfg->reset_on);
        ret += of_property_read_u32(dev->of_node, "i2c_reset_off", &reset_cfg->reset_off);
        ret += of_property_read_u32(dev->of_node, "i2c_rst_delay_b", &reset_cfg->reset_delay_b);
        ret += of_property_read_u32(dev->of_node, "i2c_rst_delay", &reset_cfg->reset_delay);
        ret += of_property_read_u32(dev->of_node, "i2c_rst_delay_a", &reset_cfg->reset_delay_a);
        ret += of_property_read_u32(dev->of_node, "i2c_adap_reset_flag", &reset_cfg->i2c_adap_reset_flag);

        if (ret != 0) {
            FPGA_I2C_ERROR("dts config error, ret:%d.\n", ret);
            ret = -ENXIO;
            return ret;
        }

        rv = of_property_read_u32(dev->of_node, "i2c_data_buf_len_reg", &i2c_data_buf_len_reg);
        if (rv == 0) {
            ret = fpga_reg_read_32(fpga_i2c, i2c_data_buf_len_reg, &reg->i2c_data_buf_len);
            if (ret < 0) {
                dev_err(fpga_i2c->dev, "Failed to get fpga i2c data buf length, reg addr: 0x%x, ret: %d\n",
                    i2c_data_buf_len_reg, ret);
                return ret;
            }
            FPGA_I2C_VERBOSE("fpga i2c data buf length reg addr: 0x%x, value: %d\n",
                i2c_data_buf_len_reg, reg->i2c_data_buf_len);
            if (reg->i2c_data_buf_len == 0) {
                reg->i2c_data_buf_len = FPGA_I2C_RDWR_MAX_LEN_DEFAULT;
            }
        } else {
            ret = of_property_read_u32(dev->of_node, "i2c_data_buf_len", &reg->i2c_data_buf_len);
            if (ret != 0) {
                reg->i2c_data_buf_len = FPGA_I2C_RDWR_MAX_LEN_DEFAULT;
                ret = 0;
            }
        }

        rv = of_property_read_u32(dev->of_node, "i2c_offset_reg", &i2c_offset_reg);
        if (rv == 0) {
            ret = fpga_reg_read_32(fpga_i2c, i2c_offset_reg, &i2c_offset_val);
            if (ret < 0) {
                dev_err(fpga_i2c->dev, "Failed to get fpga i2c adapter offset value, reg addr: 0x%x, ret: %d\n",
                    i2c_offset_reg, ret);
                return ret;
            }
            FPGA_I2C_VERBOSE("fpga i2c adapter offset reg addr: 0x%x, value: %d\n",
                i2c_offset_reg, i2c_offset_val);
            reg->i2c_scale +=i2c_offset_val;
            reg->i2c_filter += i2c_offset_val;
            reg->i2c_stretch += i2c_offset_val;
            reg->i2c_ext_9548_exits_flag += i2c_offset_val;
            reg->i2c_ext_9548_addr += i2c_offset_val;
            reg->i2c_ext_9548_chan += i2c_offset_val;
            reg->i2c_in_9548_chan += i2c_offset_val;
            reg->i2c_slave += i2c_offset_val;
            reg->i2c_reg += i2c_offset_val;
            reg->i2c_reg_len += i2c_offset_val;
            reg->i2c_data_len += i2c_offset_val;
            reg->i2c_ctrl += i2c_offset_val;
            reg->i2c_status += i2c_offset_val;
            reg->i2c_data_buf += i2c_offset_val;
        }

        ret = of_property_read_u32(dev->of_node, "i2c_err_vec", &reg->i2c_err_vec);
        if (ret != 0) {
            reg->i2c_err_vec = DTS_NO_CFG_FLAG;
            FPGA_I2C_VERBOSE("not support i2c_err_vec cfg. ret: %d, set DTS_NO_CFG_FLAG: %d\n",
                ret, reg->i2c_err_vec);
            ret = 0;    /* Not configuring i2c_err_vec is not an error  */
        } else {
            if (i2c_offset_val != 0) {
                reg->i2c_err_vec += i2c_offset_val;
            }
        }
    } else {
        if (dev->platform_data == NULL) {
            dev_err(fpga_i2c->dev, "Failed to get platform data config.\n");
            ret = -ENXIO;
            return ret;
        }
        fpga_i2c_bus_device = dev->platform_data;
        fpga_i2c->dev_name = fpga_i2c_bus_device->dev_name;
        fpga_i2c->adap_nr = fpga_i2c_bus_device->adap_nr;
        fpga_i2c->i2c_scale_value = fpga_i2c_bus_device->i2c_scale_value;
        fpga_i2c->i2c_filter_value = fpga_i2c_bus_device->i2c_filter_value;
        fpga_i2c->i2c_stretch_value = fpga_i2c_bus_device->i2c_stretch_value;
        fpga_i2c->i2c_timeout = fpga_i2c_bus_device->i2c_timeout;
        fpga_i2c->i2c_func_mode = fpga_i2c_bus_device->i2c_func_mode;
        fpga_i2c->i2c_params_check = fpga_i2c_bus_device->i2c_func_mode;

        reset_cfg->reset_addr = fpga_i2c_bus_device->i2c_reset_addr;
        reset_cfg->reset_on = fpga_i2c_bus_device->i2c_reset_on;
        reset_cfg->reset_off = fpga_i2c_bus_device->i2c_reset_off;
        reset_cfg->reset_delay_b = fpga_i2c_bus_device->i2c_rst_delay_b;
        reset_cfg->reset_delay = fpga_i2c_bus_device->i2c_rst_delay;
        reset_cfg->reset_delay_a = fpga_i2c_bus_device->i2c_rst_delay_a;
        reset_cfg->i2c_adap_reset_flag = fpga_i2c_bus_device->i2c_adap_reset_flag;

        reg->i2c_ext_9548_addr = fpga_i2c_bus_device->i2c_ext_9548_addr;
        reg->i2c_ext_9548_chan = fpga_i2c_bus_device->i2c_ext_9548_chan;
        reg->i2c_slave = fpga_i2c_bus_device->i2c_slave;
        reg->i2c_reg = fpga_i2c_bus_device->i2c_reg;
        reg->i2c_data_len = fpga_i2c_bus_device->i2c_data_len;
        reg->i2c_ctrl = fpga_i2c_bus_device->i2c_ctrl;
        reg->i2c_status = fpga_i2c_bus_device->i2c_status;
        reg->i2c_scale = fpga_i2c_bus_device->i2c_scale;
        reg->i2c_filter = fpga_i2c_bus_device->i2c_filter;
        reg->i2c_stretch = fpga_i2c_bus_device->i2c_stretch;
        reg->i2c_ext_9548_exits_flag = fpga_i2c_bus_device->i2c_ext_9548_exits_flag;
        reg->i2c_reg_len = fpga_i2c_bus_device->i2c_reg_len;
        reg->i2c_in_9548_chan = fpga_i2c_bus_device->i2c_in_9548_chan;
        reg->i2c_data_buf = fpga_i2c_bus_device->i2c_data_buf;

        i2c_data_buf_len_reg = fpga_i2c_bus_device->i2c_data_buf_len_reg;
        if (i2c_data_buf_len_reg > 0) {
            ret = fpga_reg_read_32(fpga_i2c, i2c_data_buf_len_reg, &reg->i2c_data_buf_len);
            if (ret < 0) {
                dev_err(fpga_i2c->dev, "Failed to get fpga i2c data buf length, reg addr: 0x%x, ret: %d\n",
                    i2c_data_buf_len_reg, ret);
                return ret;
            }
            FPGA_I2C_VERBOSE("fpga i2c data buf length reg addr: 0x%x, value: %d\n",
                i2c_data_buf_len_reg, reg->i2c_data_buf_len);
            if (reg->i2c_data_buf_len == 0) {
                reg->i2c_data_buf_len = FPGA_I2C_RDWR_MAX_LEN_DEFAULT;
            }
        } else {
            if (fpga_i2c_bus_device->i2c_data_buf_len == 0) {
                reg->i2c_data_buf_len = FPGA_I2C_RDWR_MAX_LEN_DEFAULT;
                FPGA_I2C_VERBOSE("not support i2c_data_buf_len cfg, set default_val:%d\n",
                    reg->i2c_data_buf_len);
            } else {
                reg->i2c_data_buf_len = fpga_i2c_bus_device->i2c_data_buf_len;
            }
        }

        i2c_offset_reg = fpga_i2c_bus_device->i2c_offset_reg;
        if (i2c_offset_reg > 0) {
            rv = fpga_reg_read_32(fpga_i2c, i2c_offset_reg, &i2c_offset_val);
            if (rv < 0) {
                dev_err(fpga_i2c->dev, "Failed to get fpga i2c adapter offset value, reg addr: 0x%x, rv: %d\n",
                    i2c_offset_reg, rv);
                return rv;
            }
            FPGA_I2C_VERBOSE("fpga i2c adapter offset reg addr: 0x%x, value: %d\n",
                i2c_offset_reg, i2c_offset_val);
            reg->i2c_scale +=i2c_offset_val;
            reg->i2c_filter += i2c_offset_val;
            reg->i2c_stretch += i2c_offset_val;
            reg->i2c_ext_9548_exits_flag += i2c_offset_val;
            reg->i2c_ext_9548_addr += i2c_offset_val;
            reg->i2c_ext_9548_chan += i2c_offset_val;
            reg->i2c_in_9548_chan += i2c_offset_val;
            reg->i2c_slave += i2c_offset_val;
            reg->i2c_reg += i2c_offset_val;
            reg->i2c_reg_len += i2c_offset_val;
            reg->i2c_data_len += i2c_offset_val;
            reg->i2c_ctrl += i2c_offset_val;
            reg->i2c_status += i2c_offset_val;
            reg->i2c_data_buf += i2c_offset_val;
        }

        if (fpga_i2c_bus_device->i2c_err_vec == 0) {
            reg->i2c_err_vec = DTS_NO_CFG_FLAG;
            FPGA_I2C_VERBOSE("not support i2c_err_vec cfg, set DTS_NO_CFG_FLAG:%d\n",
                reg->i2c_err_vec);
        } else {
            reg->i2c_err_vec = fpga_i2c_bus_device->i2c_err_vec;
            if (i2c_offset_val != 0) {
                reg->i2c_err_vec += i2c_offset_val;
            }
        }
    }

    FPGA_I2C_VERBOSE("i2c_ext_9548_addr:0x%x, i2c_ext_9548_chan:0x%x, i2c_slave:0x%x, i2c_reg:0x%x, i2c_data_len:0x%x.\n",
        reg->i2c_ext_9548_addr, reg->i2c_ext_9548_chan, reg->i2c_slave, reg->i2c_reg, reg->i2c_data_len);
    FPGA_I2C_VERBOSE("i2c_ctrl:0x%x, i2c_status:0x%x, i2c_scale:0x%x, i2c_filter:0x%x, i2c_stretch:0x%x.\n",
        reg->i2c_ctrl, reg->i2c_status, reg->i2c_scale, reg->i2c_filter, reg->i2c_stretch);
    FPGA_I2C_VERBOSE("i2c_ext_9548_exits_flag:0x%x, i2c_in_9548_chan:0x%x, i2c_data_buf:0x%x, i2c_reg_len:0x%x, i2c_data_buf_len:0x%x.\n",
        reg->i2c_ext_9548_exits_flag, reg->i2c_in_9548_chan, reg->i2c_data_buf, reg->i2c_reg_len, reg->i2c_data_buf_len);
    FPGA_I2C_VERBOSE("dev_name:%s, i2c_scale_value:0x%x, i2c_filter_value:0x%x, i2c_stretch_value:0x%x, i2c_timeout:0x%x.\n",
        fpga_i2c->dev_name, fpga_i2c->i2c_scale_value, fpga_i2c->i2c_filter_value, fpga_i2c->i2c_stretch_value, fpga_i2c->i2c_timeout);
    FPGA_I2C_VERBOSE("i2c_reset_addr:0x%x, i2c_reset_on:0x%x, i2c_reset_off:0x%x, i2c_rst_delay_b:0x%x, i2c_rst_delay:0x%x, i2c_rst_delay_a:0x%x.\n",
        reset_cfg->reset_addr, reset_cfg->reset_on, reset_cfg->reset_off, reset_cfg->reset_delay_b, reset_cfg->reset_delay, reset_cfg->reset_delay_a);
    FPGA_I2C_VERBOSE("i2c_adap_reset_flag:0x%x.\n", reset_cfg->i2c_adap_reset_flag);
    FPGA_I2C_VERBOSE("i2c_err_vec:0x%x\n", reg->i2c_err_vec);

    return ret;
}

static int fpga_i2c_probe(struct platform_device *pdev)
{
    int ret;
    fpga_i2c_dev_t *fpga_i2c;
    struct device *dev;

    fpga_i2c = devm_kzalloc(&pdev->dev, sizeof(fpga_i2c_dev_t), GFP_KERNEL);
    if (!fpga_i2c) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        ret = -ENOMEM;
        goto out;
    }

    fpga_i2c->dev = &pdev->dev;

    ret = fpga_i2c_config_init(fpga_i2c);
    if (ret !=0) {
        dev_err(fpga_i2c->dev, "Failed to get fpga i2c dts config.\n");
        goto out;
    }

    ret = fpga_i2c_adapter_init(fpga_i2c);
    if (ret !=0) {
        dev_err(fpga_i2c->dev, "Failed to init fpga i2c adapter.\n");
        goto out;
    }

    if (fpga_i2c->dev->of_node) {
        fpga_i2c->i2c_params_check = of_property_read_bool(fpga_i2c->dev->of_node, "i2c_params_check");
    }
    FPGA_I2C_VERBOSE("fpga i2c params check flag:%d.\n", fpga_i2c->i2c_params_check);

    init_waitqueue_head(&fpga_i2c->queue);

    dev = fpga_i2c->dev;
    fpga_i2c->adap = fpga_i2c_ops;
    fpga_i2c->adap.timeout = msecs_to_jiffies(fpga_i2c->i2c_timeout);
    fpga_i2c->adap.dev.parent = &pdev->dev;
    fpga_i2c->adap.dev.of_node = pdev->dev.of_node;
    i2c_set_adapdata(&fpga_i2c->adap, fpga_i2c);
    platform_set_drvdata(pdev, fpga_i2c);

    if (fpga_i2c->dev->of_node) {
        /* adap.nr get from dts aliases */
        ret = i2c_add_adapter(&fpga_i2c->adap);
    } else {
        fpga_i2c->adap.nr = fpga_i2c->adap_nr;
        ret = i2c_add_numbered_adapter(&fpga_i2c->adap);
    }

    if (ret < 0) {
        dev_info(fpga_i2c->dev, "Failed to add adapter.\n");
        goto fail_add;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
    of_i2c_register_devices(&fpga_i2c->adap);
#endif
    dev_info(fpga_i2c->dev, "registered i2c-%d for %s using mode %d with base address:0x%x, data buf len: %d success.\n",
        fpga_i2c->adap.nr, fpga_i2c->dev_name, fpga_i2c->i2c_func_mode, fpga_i2c->reg.i2c_scale,
        fpga_i2c->reg.i2c_data_buf_len);
    return 0;

fail_add:
    platform_set_drvdata(pdev, NULL);
out:
    return ret;
};

static int fpga_i2c_remove(struct platform_device *pdev)
{
    fpga_i2c_dev_t *fpga_i2c;

    fpga_i2c = platform_get_drvdata(pdev);
    i2c_del_adapter(&fpga_i2c->adap);
    platform_set_drvdata(pdev, NULL);
    return 0;
};

static struct of_device_id fpga_i2c_match[] = {
    {
        .compatible = "wb-fpga-i2c",
    },
    {},
};
MODULE_DEVICE_TABLE(of, fpga_i2c_match);

static struct platform_driver wb_fpga_i2c_driver = {
    .probe = fpga_i2c_probe,
    .remove = fpga_i2c_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = DRV_NAME,
        .of_match_table = fpga_i2c_match,
    },
};

static int __init wb_fpga_i2c_init(void)
{
    return platform_driver_register(&wb_fpga_i2c_driver);
}

static void __exit wb_fpga_i2c_exit(void)
{
    platform_driver_unregister(&wb_fpga_i2c_driver);
}

module_init(wb_fpga_i2c_init);
module_exit(wb_fpga_i2c_exit);
MODULE_DESCRIPTION("fpga i2c adapter driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
