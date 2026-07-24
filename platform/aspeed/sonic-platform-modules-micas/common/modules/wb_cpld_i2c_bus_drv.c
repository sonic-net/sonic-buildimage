/*
 * cpld_i2c_bus_drv.c
 * ko to create cpld i2c adapter
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
#include <linux/ratelimit.h>
#include "cpld_i2c.h"

#include <linux/fs.h>
#include <linux/uaccess.h>

#include <wb_logic_dev_common.h>
#include <wb_bsp_i2c_debug.h>

#define DRV_NAME                      "wb-cpld-i2c"
#define DRV_VERSION                   "1.0"
#define DTS_NO_CFG_FLAG               (0)

#define CPLD_I2C_STRETCH_TIMEOUT  (0x01)
#define CPLD_I2C_DEADLOCK_FAILED  (0x02)
#define CPLD_I2C_SLAVE_NO_RESPOND (0x03)
#define CPLD_I2C_STA_FAIL         (0x01)
#define CPLD_I2C_STA_BUSY         (0x02)
#define CPLD_I2C_CTL_NO_REG       (0x01 << 2)

#define I2C_READ_MSG_NUM          (0x02)
#define I2C_WRITE_MSG_NUM         (0x01)
#define CPLD_REG_WIDTH            (4)
#define I2C_BUSY_DUMP_INTERVAL    (24 * 3600)
#define I2C_BUSY_DUMP_TIMES       (10)
#define DUMP_LINE_BYTES           (16)

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static DEFINE_RATELIMIT_STATE(cpld_print_ratelimit, I2C_BUSY_DUMP_INTERVAL * HZ, I2C_BUSY_DUMP_TIMES);

static int cpld_device_write(cpld_i2c_dev_t *cpld_i2c, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_write pfunc;

    pfunc = (device_func_write)cpld_i2c->write_intf_addr;
    return pfunc(cpld_i2c->dev_name, pos, val, size);
}

static int cpld_device_read(cpld_i2c_dev_t *cpld_i2c, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    pfunc = (device_func_read)cpld_i2c->read_intf_addr;
    return pfunc(cpld_i2c->dev_name, pos, val, size);
}

static int little_endian_dword_to_buf(uint8_t *buf, int len, uint32_t dword)
{
    uint8_t tmp_buf[CPLD_REG_WIDTH];

    if (len < 4) {
        DEBUG_ERROR("Not enough buf, dword to buf: len[%d], dword[0x%x]\n", len, dword);
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

    if (len != CPLD_REG_WIDTH) {
        DEBUG_ERROR("buf length %d error, can't convert to dowrd.\n", len);
        return -1;
    }
    dword_tmp = 0;
    for (i = 0; i < CPLD_REG_WIDTH; i++) {
        dword_tmp |= (buf[i] << (i * 8));
    }
    *dword = dword_tmp;
    return 0;
}

static int cpld_reg_write(cpld_i2c_dev_t *cpld_i2c, uint32_t addr, uint8_t val)
{
    int ret;

    ret = cpld_device_write(cpld_i2c, addr, &val, sizeof(uint8_t));
    if (ret < 0) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld reg write failed, dev name:%s, offset:0x%x, value:0x%x.\n",
            cpld_i2c->dev_name, addr, val);
        return -EIO;
    }

    DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), 
        "cpld reg write success, dev name:%s, offset:0x%x, value:0x%x.\n",
        cpld_i2c->dev_name, addr, val);
    return 0;
}

static int cpld_reg_read(cpld_i2c_dev_t *cpld_i2c, uint32_t addr, uint8_t *val)
{
    int ret;

    ret = cpld_device_read(cpld_i2c, addr, val, sizeof(uint8_t));
    if (ret < 0) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld reg read failed, dev name:%s, offset:0x%x\n",
            cpld_i2c->dev_name, addr);
        return -EIO;
    }

    DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), 
        "cpld reg read success, dev name:%s, offset:0x%x, value:0x%x.\n",
        cpld_i2c->dev_name, addr, *val);
    return 0;
}

static int cpld_data_write(cpld_i2c_dev_t *cpld_i2c, uint32_t addr, uint8_t *val, size_t size)
{
    int ret;

    ret = cpld_device_write(cpld_i2c, addr, val, size);
    if (ret < 0) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld data write failed, dev name:%s, offset:0x%x, size:%zu.\n",
            cpld_i2c->dev_name, addr, size);
        return -EIO;
    }

    DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), 
        "cpld data write success, dev name:%s, offset:0x%x, size:%zu.\n",
        cpld_i2c->dev_name, addr, size);
    return 0;
}

static int cpld_data_read(cpld_i2c_dev_t *cpld_i2c, uint32_t addr, uint8_t *val, size_t size)
{
    int ret;

    ret = cpld_device_read(cpld_i2c, addr, val, size);
    if (ret < 0) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld data read failed, dev name:%s, offset:0x%x, size:%zu.\n",
            cpld_i2c->dev_name, addr, size);
        return -EIO;
    }

    DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), 
        "cpld data read success, dev name:%s, offset:0x%x, size:%zu.\n",
        cpld_i2c->dev_name, addr, size);
    return 0;
}

static int cpld_reg_write_32(cpld_i2c_dev_t *cpld_i2c, uint32_t addr, uint32_t val)
{
    int ret;
    uint8_t buf[CPLD_REG_WIDTH];

    mem_clear(buf, sizeof(buf));
    little_endian_dword_to_buf(buf, sizeof(buf), val);
    ret = cpld_device_write(cpld_i2c, addr, buf, sizeof(buf));
    if (ret < 0) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld reg write failed, dev name: %s, offset: 0x%x, value: 0x%x.\n",
            cpld_i2c->dev_name, addr, val);
        return -EIO;
    }

    DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), 
        "cpld reg write success, dev name: %s, offset: 0x%x, value: 0x%x.\n",
        cpld_i2c->dev_name, addr, val);
    return 0;
}

static int cpld_reg_read_32(cpld_i2c_dev_t *cpld_i2c, uint32_t addr, uint32_t *val)
{
    int ret;
    uint8_t buf[CPLD_REG_WIDTH];

    mem_clear(buf, sizeof(buf));
    ret = cpld_device_read(cpld_i2c, addr, buf, sizeof(buf));
    if (ret < 0) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld reg read failed, dev name: %s, offset: 0x%x, ret: %d\n",
            cpld_i2c->dev_name, addr, ret);
        return -EIO;
    }
    little_endian_buf_to_dword(buf, sizeof(buf), val);
    DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), 
        "cpld reg read success, dev name: %s, offset: 0x%x, value: 0x%x.\n",
        cpld_i2c->dev_name, addr, *val);
    return 0;
}

static int cpld_i2c_is_busy(cpld_i2c_dev_t *cpld_i2c)
{
    uint8_t val;
    int ret;
    cpld_i2c_reg_t *reg;

    reg = &cpld_i2c->reg;
    val = 0;
    ret = cpld_reg_read(cpld_i2c, reg->i2c_status, &val);
    if (ret < 0 ) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "read cpld i2c status reg failed, reg addr:0x%x, ret:%d.\n",
            reg->i2c_status, ret);
        return 1;
    }
    if (val & CPLD_I2C_STA_BUSY) {
        DEBUG_INFO_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld i2c status busy, reg addr:0x%x, value:0x%x.\n",
            reg->i2c_status, val);
        return 1;
    } else {
        return 0;
    }
}

static void sleep_by_time(int sleep_time)
{
    if (in_interrupt() || oops_in_progress) {
        /* If currently in an interrupt context or during a panic, call the udelay function. */
        udelay(sleep_time);
    } else {
        usleep_range(sleep_time, sleep_time + 1);
    }
}

static int cpld_i2c_wait(cpld_i2c_dev_t *cpld_i2c)
{
    int retry_cnt;

    retry_cnt = CPLD_I2C_XFER_TIME_OUT/CPLD_I2C_SLEEP_TIME;
    while (retry_cnt--) {
        if (cpld_i2c_is_busy(cpld_i2c)) {
            sleep_by_time(CPLD_I2C_SLEEP_TIME);
        } else {
            return 0;
        }
    }

    return -EBUSY;
}

static int cpld_i2c_check_status(cpld_i2c_dev_t *cpld_i2c)
{
    uint8_t data;
    int ret;
    cpld_i2c_reg_t *reg;

    reg = &cpld_i2c->reg;
    data = 0;
    ret = cpld_reg_read(cpld_i2c, reg->i2c_status, &data);
    if (ret) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "read cpld i2c status reg failed, reg addr:0x%x, ret:%d.\n",
            reg->i2c_status, ret);
        return ret;
    }

    if (data & CPLD_I2C_STA_FAIL) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld i2c status error, reg addr:0x%x, value:%d.\n",
            reg->i2c_status, data);

        /* read i2c_err_vec to confirm err type*/
        if (reg->i2c_err_vec != DTS_NO_CFG_FLAG) {
            /* read i2c_err_vec reg */
            ret = cpld_reg_read(cpld_i2c, reg->i2c_err_vec, &data);
            if (ret) {
                DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
                    "read cpld i2c err vec reg failed, reg addr:0x%x, ret:%d.\n",
                    reg->i2c_err_vec, ret);
                return ret;
            }
            DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), 
                "get i2c err vec, reg addr:0x%x, read value:0x%x\n", reg->i2c_err_vec, data);

            /* match i2c_err_vec reg value and err type*/
            switch (data) {
            case CPLD_I2C_STRETCH_TIMEOUT:
                ret = -ETIMEDOUT;
                break;
            case CPLD_I2C_DEADLOCK_FAILED:
                ret = -EDEADLK;
                break;
            case CPLD_I2C_SLAVE_NO_RESPOND:
                ret = -ENXIO;
                break;
            default:
                DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
                    "get i2c err vec value out of range, reg addr:0x%x, read value:0x%x\n",
                    reg->i2c_err_vec, data);
                ret = -EREMOTEIO;
                break;
            }
            return ret;
        } else {
            DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), 
                "i2c err vec not config, cpld i2c status check return -1\n");
            return -EREMOTEIO;
        }
    }
    return 0;
}

static int cpld_i2c_do_work(cpld_i2c_dev_t *cpld_i2c, int i2c_addr,
        unsigned char *data, uint32_t length, int is_read)
{
    int ret, i;
    uint8_t op, i2c_reg_addr_len;
    uint8_t *i2c_read_addr_buf;
    cpld_i2c_reg_t *reg;
    cpld_i2c_reg_addr_t *i2c_addr_desc;

    reg = &cpld_i2c->reg;

    ret = cpld_reg_write(cpld_i2c, reg->i2c_slave, i2c_addr);
    if (ret) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "write cpld i2c slave reg failed, reg addr:0x%x, value:0x%x, ret:%d.\n",
            reg->i2c_slave, i2c_addr, ret);
        goto exit;
    }

    i2c_addr_desc = &cpld_i2c->i2c_addr_desc;
    i2c_reg_addr_len = i2c_addr_desc->reg_addr_len;
    i2c_read_addr_buf = &i2c_addr_desc->read_reg_addr[0];

    if (i2c_reg_addr_len > 0 && i2c_reg_addr_len <= I2C_REG_MAX_WIDTH) {
        ret = cpld_data_write(cpld_i2c, reg->i2c_reg, i2c_read_addr_buf, i2c_reg_addr_len);
        if (ret) {
            DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
                "write cpld i2c offset reg failed, cpld addr:0x%x, reg len:%d, ret:%d\n",
                reg->i2c_reg, i2c_reg_addr_len, ret);
            for (i = 0; i < i2c_reg_addr_len; i++) {
                DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), "%02d : %02x\n", i, 
                    i2c_read_addr_buf[i]);
            }
            goto exit;
        }
    }

    ret = cpld_reg_write_32(cpld_i2c, reg->i2c_data_len, length);
    if (ret) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "write cpld i2c date len reg failed, reg addr:0x%x, value:0x%x, ret:%d.\n",
            reg->i2c_data_len, length, ret);
        goto exit;
    }

    if (is_read) {
        op = CPLD_I2C_CTL_RD | CPLD_I2C_CTL_BG;
    } else {

        ret = cpld_data_write(cpld_i2c, reg->i2c_data_buf, data, length);
        if (ret) {
            DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
                "write cpld i2c date buf failed, reg addr:0x%x, write len:%d, ret:%d.\n",
                reg->i2c_data_buf, length, ret);
            goto exit;
        }
        op = CPLD_I2C_CTL_WR | CPLD_I2C_CTL_BG ;
    }

    ret = cpld_reg_write(cpld_i2c, reg->i2c_ctrl, op);
    if (ret) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "write cpld i2c control reg failed, reg addr:0x%x, value:%d, ret:%d.\n",
            reg->i2c_ctrl, op, ret);
        goto exit;
    }

    ret = cpld_i2c_wait(cpld_i2c);
    if (ret) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), "wait cpld i2c status timeout.\n");
        goto exit;
    }

    ret = cpld_i2c_check_status(cpld_i2c);
    if (ret) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), "check cpld i2c status error.\n");
        goto exit;
    }

    if (is_read) {

        ret = cpld_data_read(cpld_i2c, reg->i2c_data_buf, data, length);
        if (ret) {
            DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
                "read cpld i2c data buf failed, reg addr:0x%x, read len:%d, ret:%d.\n",
                reg->i2c_data_buf, length, ret);
            goto exit;
        }
    }

exit:
    return ret;
}

static int cpld_i2c_write(cpld_i2c_dev_t *cpld_i2c, int target,
                u8 *data, int length, int i2c_msg_num)
{
    int ret, i;
    cpld_i2c_reg_addr_t *i2c_addr_desc;

    if (i2c_msg_num == I2C_READ_MSG_NUM) {

        if (length > I2C_REG_MAX_WIDTH) {
            DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
                "read reg addr len %d, more than max length.\n", length);
            return -EINVAL;
        }

        i2c_addr_desc = &cpld_i2c->i2c_addr_desc;
        for (i = 0; i < length; i++) {
            i2c_addr_desc->read_reg_addr[i] = data[length -i -1];
            DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), "%02d : %02x\n", i, 
                i2c_addr_desc->read_reg_addr[i]);
        }
        i2c_addr_desc->reg_addr_len = length;
        ret = 0;
    } else {
        i2c_addr_desc = &cpld_i2c->i2c_addr_desc;
        i2c_addr_desc->read_reg_addr[0] = data[0];        
        if (length > 1) {
            memmove(data, data + 1, length - 1);
            length -= 1;
        }

        i2c_addr_desc->reg_addr_len = 1;
        ret = cpld_i2c_do_work(cpld_i2c, target, data, length, 0);
    }

    return ret;
}

/**
 * cpld_i2c_read - receive data from the bus.
 * @i2c: The struct cpld_i2c_dev_t.
 * @target: Target address.
 * @data: Pointer to the location to store the datae .
 * @length: Length of the data.
 *
 * The address is sent over the bus, then the data is read.
 *
 * Returns 0 on success, otherwise a negative errno.
 */
static int cpld_i2c_read(cpld_i2c_dev_t *cpld_i2c, int target,
        u8 *data, int length)
{
    int ret, offset_size;
    int i, tmp_val;
    cpld_i2c_reg_addr_t *i2c_addr_desc;
    uint8_t i2c_reg_addr_len;
    uint8_t *i2c_read_addr_buf;

    offset_size = 0;
    i2c_addr_desc = &cpld_i2c->i2c_addr_desc;
    i2c_reg_addr_len = i2c_addr_desc->reg_addr_len;
    i2c_read_addr_buf = &i2c_addr_desc->read_reg_addr[0];

    while (1) {
        if (length <= cpld_i2c->reg.i2c_data_buf_len) {
            return cpld_i2c_do_work(cpld_i2c, target, data + offset_size, length, 1);
        }

        ret = cpld_i2c_do_work(cpld_i2c, target, data + offset_size, cpld_i2c->reg.i2c_data_buf_len, 1);
        if (ret != 0) {
            DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
                "cpld_i2c_read failed, i2c addr:0x%x, offset:0x%x, ret:%d.\n",
                target, offset_size, ret);
            return ret;
        }

        tmp_val = i2c_read_addr_buf[0];
        tmp_val += cpld_i2c->reg.i2c_data_buf_len;
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
        offset_size += cpld_i2c->reg.i2c_data_buf_len;
        length -= cpld_i2c->reg.i2c_data_buf_len;
    }

    return ret;
}

static void cpld_i2c_dump_buf_seg(struct device *dev, uint32_t base, const uint8_t *buf, size_t bytes)
{
    int i, j, pos;
    char print_line[MAX_RW_LEN];

    for (i = 0; i < bytes; i += DUMP_LINE_BYTES) {
        pos = snprintf(print_line, sizeof(print_line), "0x%04x:", base + i);
        for (j = 0; j < DUMP_LINE_BYTES && (i + j) < bytes; j++)
            pos += snprintf(print_line + pos, sizeof(print_line) - pos, " %02x", buf[i + j]);
        dev_info(dev, "%s\n", print_line);
    }
}
static void cpld_i2c_dump_regs(cpld_i2c_dev_t *cpld_i2c)
{
    uint8_t config_buf[MAX_RW_LEN];
    uint8_t data_buf[MAX_RW_LEN];
    uint32_t read_len, config_addr_base, data_addr_base, config_bytes, data_bytes;

    if (__ratelimit(&cpld_print_ratelimit)) {

        config_bytes = MAX_RW_LEN;
        config_addr_base = cpld_i2c->reg.i2c_scale;
        data_addr_base = cpld_i2c->reg.i2c_data_buf;
        read_len = cpld_i2c->reg.i2c_data_buf_len;
        data_bytes = (read_len > sizeof(data_buf)) ? sizeof(data_buf) : read_len;

        mem_clear(config_buf, sizeof(config_buf));
        mem_clear(data_buf, sizeof(data_buf));
        cpld_data_read(cpld_i2c, config_addr_base, config_buf, config_bytes);
        cpld_data_read(cpld_i2c, data_addr_base, data_buf, data_bytes);

        dev_info(cpld_i2c->dev, "[cpld_i2c] :dump config and data buf:\n");
        cpld_i2c_dump_buf_seg(cpld_i2c->dev, config_addr_base, config_buf, config_bytes);
        dev_info(cpld_i2c->dev, "===== CPLD I2C: End dump from config buf =====\n");
        cpld_i2c_dump_buf_seg(cpld_i2c->dev, data_addr_base, data_buf, data_bytes);
        dev_info(cpld_i2c->dev, "===== CPLD I2C: End dump from data buf =====\n");
    }
    return;
}

static void cpld_i2c_reset(cpld_i2c_dev_t *cpld_i2c)
{
    cpld_i2c_reset_cfg_t *reset_cfg;
    uint32_t reset_addr;

    reset_cfg = &cpld_i2c->reset_cfg;
    reset_addr = reset_cfg->reset_addr;
    if (reset_cfg->reset_delay_b) {
        sleep_by_time(reset_cfg->reset_delay_b);
    }

    cpld_reg_write_32(cpld_i2c, reset_addr, reset_cfg->reset_on);
    if (reset_cfg->reset_delay) {
        sleep_by_time(reset_cfg->reset_delay);
    }

    cpld_reg_write_32(cpld_i2c, reset_addr, reset_cfg->reset_off);
    if (reset_cfg->reset_delay_a) {
        sleep_by_time(reset_cfg->reset_delay_a);
    }

    return;
}

/**
 * cpld_i2c_xfer - The driver's master_xfer function.
 * @adap: Pointer to the i2c_adapter structure.
 * @msgs: Pointer to the messages to be processed.
 * @num: Length of the MSGS array.
 *
 * Returns the number of messages processed, or a negative errno on
 * failure.
 */
static int cpld_i2c_adapter_init(cpld_i2c_dev_t *cpld_i2c)
{
    int ret;
    cpld_i2c_reg_t *reg;

    reg = &cpld_i2c->reg;

    ret = 0;
    ret += cpld_reg_write(cpld_i2c, reg->i2c_scale, cpld_i2c->i2c_scale_value);
    ret += cpld_reg_write(cpld_i2c, reg->i2c_filter, cpld_i2c->i2c_filter_value);
    ret += cpld_reg_write(cpld_i2c, reg->i2c_stretch, cpld_i2c->i2c_stretch_value);
    if (ret < 0) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), "cpld_i2c_init failed.\n");
        return ret;
    }

    DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), "cpld_i2c_init ok.\n");
    return 0;
}

static int cpld_i2c_params_check(cpld_i2c_dev_t *cpld_i2c)
{
    int ret;
    cpld_i2c_reg_t *reg;
    uint8_t i2c_scale_value, i2c_filter_value, i2c_stretch_value;

    reg = &cpld_i2c->reg;
    i2c_scale_value   = 0;
    i2c_filter_value  = 0;
    i2c_stretch_value = 0;
    ret = 0;
    ret += cpld_reg_read(cpld_i2c, reg->i2c_scale, &i2c_scale_value);
    ret += cpld_reg_read(cpld_i2c, reg->i2c_filter, &i2c_filter_value);
    ret += cpld_reg_read(cpld_i2c, reg->i2c_stretch, &i2c_stretch_value);
    if (ret < 0) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), "read cpld i2c params failed.\n");
        return 1;
    }

    if ((i2c_scale_value != cpld_i2c->i2c_scale_value)
            || (i2c_filter_value != cpld_i2c->i2c_filter_value)
            || (i2c_stretch_value != cpld_i2c->i2c_stretch_value)) {
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld i2c params check error, read value: i2c_scale 0x%x, "
            "i2c_filter:0x%x, i2c_stretch:0x%x.\n",
            i2c_scale_value, i2c_filter_value, i2c_stretch_value);
        DEBUG_ERROR_I2C_ADAPTER(&(cpld_i2c->adap), 
            "cpld i2c params check error, config value: i2c_scale 0x%x, "
            "i2c_filter:0x%x, i2c_stretch:0x%x.\n",
            cpld_i2c->i2c_scale_value, cpld_i2c->i2c_filter_value, cpld_i2c->i2c_stretch_value);
        return 1;
    }

    DEBUG_VERBOSE_I2C_ADAPTER(&(cpld_i2c->adap), "cpld i2c params check ok.\n");
    return 0;
}

static int cpld_i2c_xfer(struct i2c_adapter *adap,
        struct i2c_msg *msgs, int num)
{
    struct i2c_msg *pmsg;
    int i;
    int ret;
    cpld_i2c_dev_t *cpld_i2c;
    cpld_i2c_reg_addr_t *i2c_addr_desc;

    cpld_i2c = i2c_get_adapdata(adap);

    if (num != I2C_READ_MSG_NUM && num != I2C_WRITE_MSG_NUM) {
        DEBUG_ERROR_I2C_ADAPTER(adap, "unsupport i2c_msg len:%d.\n", num);
        return -EINVAL;
    }

    if ((num == I2C_WRITE_MSG_NUM) && (msgs[0].len > cpld_i2c->reg.i2c_data_buf_len)) {
        DEBUG_ERROR_I2C_ADAPTER(adap, "unsupport i2c_msg type:msg[0].flag:0x%x, buf len:0x%x.\n",
            msgs[0].flags, msgs[0].len);
        return -EINVAL;
    }

    if (num == I2C_READ_MSG_NUM ) {
        if ((msgs[0].flags & I2C_M_RD) ||!(msgs[1].flags & I2C_M_RD)) {
            DEBUG_ERROR_I2C_ADAPTER(adap, 
                "unsupport i2c_msg type:msg[0].flag:0x%x, msg[1].flag:0x%x.\n",
                msgs[0].flags, msgs[1].flags);
            return -EINVAL;
        }
    }

    if (cpld_i2c_is_busy(cpld_i2c)) {
        DEBUG_ERROR_I2C_ADAPTER(adap, "cpld i2c adapter %d is busy, do reset.\n", adap->nr);
        cpld_i2c_dump_regs(cpld_i2c);
        if (cpld_i2c->reset_cfg.i2c_adap_reset_flag == 1) {
            cpld_i2c_reset(cpld_i2c);
            cpld_i2c_adapter_init(cpld_i2c);
        }
        return -EAGAIN;
    }

    if (cpld_i2c->i2c_params_check && cpld_i2c_params_check(cpld_i2c)) {
        DEBUG_ERROR_I2C_ADAPTER(adap, "cpld i2c params check failed, try to reinitialize.\n");
        cpld_i2c_adapter_init(cpld_i2c);
    }

    ret = 0;
    i2c_addr_desc = &cpld_i2c->i2c_addr_desc;
    i2c_addr_desc->reg_addr_len = 0;
    mem_clear(i2c_addr_desc->read_reg_addr, sizeof(i2c_addr_desc->read_reg_addr));

    for (i = 0; ret == 0 && i < num; i++) {
        pmsg = &msgs[i];
        DEBUG_VERBOSE_I2C_ADAPTER(adap, "Doing %s %d byte(s) to/from 0x%02x - %d of %d messages\n",
            pmsg->flags & I2C_M_RD ? "read" : "write", pmsg->len, pmsg->addr, i + 1, num);

        if (pmsg->flags & I2C_M_RD) {
            ret = cpld_i2c_read(cpld_i2c, pmsg->addr, pmsg->buf, pmsg->len);

            if (pmsg->flags & I2C_M_RECV_LEN) {
                if ((ret != 0) || (pmsg->buf[0] > I2C_SMBUS_BLOCK_MAX) || (pmsg->buf[0] <= 0)) {
                    DEBUG_ERROR_I2C_ADAPTER(adap,
                        "smbus block data read failed, ret:%d, read len:%u.\n",
                        ret, pmsg->buf[0]);
                    return -EPROTO;
                }
                pmsg->len += pmsg->buf[0];
                DEBUG_VERBOSE_I2C_ADAPTER(adap, "smbus block data read, read len:%d.\n", pmsg->len);
                ret = cpld_i2c_read(cpld_i2c, pmsg->addr, pmsg->buf, pmsg->len);
            }
        } else {
            ret = cpld_i2c_write(cpld_i2c, pmsg->addr, pmsg->buf, pmsg->len, num);
        }
    }

    return (ret != 0) ? ret : num;
}

static u32 cpld_i2c_functionality(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static const struct i2c_algorithm cpld_i2c_algo = {
    .master_xfer = cpld_i2c_xfer,
    .functionality = cpld_i2c_functionality,
};

static struct i2c_adapter cpld_i2c_ops = {
    .owner = THIS_MODULE,
    .name = "wb_cpld_i2c",
    .algo = &cpld_i2c_algo,
};

static int cpld_i2c_config_init(cpld_i2c_dev_t *cpld_i2c)
{
    int ret = 0, rv = 0;
    cpld_i2c_reg_t *reg;
    cpld_i2c_reset_cfg_t *reset_cfg;
    struct device *dev;
    uint32_t i2c_offset_reg, i2c_data_buf_len_reg;
    int32_t i2c_offset_val;

    cpld_i2c_bus_device_t *cpld_i2c_bus_device;

    dev = cpld_i2c->dev;
    reg = &cpld_i2c->reg;
    reset_cfg = &cpld_i2c->reset_cfg;

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
        ret += of_property_read_u32(dev->of_node, "i2c_in_9548_chan", &reg->i2c_in_9548_chan);
        ret += of_property_read_u32(dev->of_node, "i2c_data_buf", &reg->i2c_data_buf);
        ret += of_property_read_string(dev->of_node, "dev_name", &cpld_i2c->dev_name);
        ret += of_property_read_u32(dev->of_node, "i2c_scale_value", &cpld_i2c->i2c_scale_value);
        ret += of_property_read_u32(dev->of_node, "i2c_filter_value", &cpld_i2c->i2c_filter_value);
        ret += of_property_read_u32(dev->of_node, "i2c_stretch_value", &cpld_i2c->i2c_stretch_value);
        ret += of_property_read_u32(dev->of_node, "i2c_timeout", &cpld_i2c->i2c_timeout);
        ret += of_property_read_u32(dev->of_node, "i2c_func_mode", &cpld_i2c->i2c_func_mode);
        ret += of_property_read_u32(dev->of_node, "i2c_reset_addr", &reset_cfg->reset_addr);
        ret += of_property_read_u32(dev->of_node, "i2c_reset_on", &reset_cfg->reset_on);
        ret += of_property_read_u32(dev->of_node, "i2c_reset_off", &reset_cfg->reset_off);
        ret += of_property_read_u32(dev->of_node, "i2c_rst_delay_b", &reset_cfg->reset_delay_b);
        ret += of_property_read_u32(dev->of_node, "i2c_rst_delay", &reset_cfg->reset_delay);
        ret += of_property_read_u32(dev->of_node, "i2c_rst_delay_a", &reset_cfg->reset_delay_a);
        ret += of_property_read_u32(dev->of_node, "i2c_adap_reset_flag", &reset_cfg->i2c_adap_reset_flag);

        if (ret != 0) {
            DEBUG_ERROR("dts config error, ret:%d.\n", ret);
            ret = -ENXIO;
            return ret;
        }

        ret = find_intf_addr(&cpld_i2c->write_intf_addr, &cpld_i2c->read_intf_addr, cpld_i2c->i2c_func_mode);
        if (ret) {
            dev_err(dev, "find_intf_addr func mode %d fail, ret: %d.\n", cpld_i2c->i2c_func_mode, ret);
            return ret;
        }

        if (!cpld_i2c->write_intf_addr || !cpld_i2c->read_intf_addr) {
            dev_err(dev, "Fail: func mode %u rw symbol undefined.\n", cpld_i2c->i2c_func_mode);
            return -ENOSYS;
        }

        i2c_data_buf_len_reg = 0;
        rv = of_property_read_u32(dev->of_node, "i2c_data_buf_len_reg", &i2c_data_buf_len_reg);
        if (rv == 0) {
            ret = cpld_reg_read_32(cpld_i2c, i2c_data_buf_len_reg, &reg->i2c_data_buf_len);
            if (ret < 0) {
                dev_err(cpld_i2c->dev, "Failed to get cpld i2c data buf length, reg addr: 0x%x, ret: %d\n",
                    i2c_data_buf_len_reg, ret);
                return ret;
            }
            DEBUG_VERBOSE("cpld i2c data buf length reg addr: 0x%x, value: %d\n",
                i2c_data_buf_len_reg, reg->i2c_data_buf_len);
            if (reg->i2c_data_buf_len == 0) {
                reg->i2c_data_buf_len = CPLD_I2C_RDWR_MAX_LEN_DEFAULT;
            }
        } else {
            ret = of_property_read_u32(dev->of_node, "i2c_data_buf_len", &reg->i2c_data_buf_len);
            if (ret != 0) {
                reg->i2c_data_buf_len = CPLD_I2C_RDWR_MAX_LEN_DEFAULT;
            }
        }

        i2c_offset_reg = 0;
        rv = of_property_read_u32(dev->of_node, "i2c_offset_reg", &i2c_offset_reg);
        if (rv == 0) {
            ret = cpld_reg_read_32(cpld_i2c, i2c_offset_reg, &i2c_offset_val);
            if (ret < 0) {
                dev_err(cpld_i2c->dev, "Failed to get cpld i2c adapter offset value, reg addr: 0x%x, ret: %d\n",
                    i2c_offset_reg, ret);
                return ret;
            }
            DEBUG_VERBOSE("cpld i2c adapter offset reg addr: 0x%x, value: %d\n",
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
            reg->i2c_data_len += i2c_offset_val;
            reg->i2c_ctrl += i2c_offset_val;
            reg->i2c_status += i2c_offset_val;
            reg->i2c_data_buf += i2c_offset_val;
        }

        ret = of_property_read_u32(dev->of_node, "i2c_err_vec", &reg->i2c_err_vec);
        if (ret != 0) {
            reg->i2c_err_vec = DTS_NO_CFG_FLAG;
            DEBUG_VERBOSE("not support i2c_err_vec cfg. ret: %d, set DTS_NO_CFG_FLAG: %d\n",
                ret, reg->i2c_err_vec);
            ret = 0;    /* Not configuring i2c_err_vec is not an error  */
        } else {
            if (i2c_offset_val != 0) {
                reg->i2c_err_vec += i2c_offset_val;
            }
        }
    } else {
        if (dev->platform_data == NULL) {
            dev_err(cpld_i2c->dev, "Failed to get platform data config.\n");
            ret = -ENXIO;
            return ret;
        }
        cpld_i2c_bus_device = dev->platform_data;
        cpld_i2c->dev_name = cpld_i2c_bus_device->dev_name;
        cpld_i2c->adap_nr = cpld_i2c_bus_device->adap_nr;
        cpld_i2c->i2c_scale_value = cpld_i2c_bus_device->i2c_scale_value;
        cpld_i2c->i2c_filter_value = cpld_i2c_bus_device->i2c_filter_value;
        cpld_i2c->i2c_stretch_value = cpld_i2c_bus_device->i2c_stretch_value;
        cpld_i2c->i2c_timeout = cpld_i2c_bus_device->i2c_timeout;
        cpld_i2c->i2c_func_mode = cpld_i2c_bus_device->i2c_func_mode;
        cpld_i2c->i2c_params_check = cpld_i2c_bus_device->i2c_params_check;

        reset_cfg->reset_addr = cpld_i2c_bus_device->i2c_reset_addr;
        reset_cfg->reset_on = cpld_i2c_bus_device->i2c_reset_on;
        reset_cfg->reset_off = cpld_i2c_bus_device->i2c_reset_off;
        reset_cfg->reset_delay_b = cpld_i2c_bus_device->i2c_rst_delay_b;
        reset_cfg->reset_delay = cpld_i2c_bus_device->i2c_rst_delay;
        reset_cfg->reset_delay_a = cpld_i2c_bus_device->i2c_rst_delay_a;
        reset_cfg->i2c_adap_reset_flag = cpld_i2c_bus_device->i2c_adap_reset_flag;

        reg->i2c_ext_9548_addr = cpld_i2c_bus_device->i2c_ext_9548_addr;
        reg->i2c_ext_9548_chan = cpld_i2c_bus_device->i2c_ext_9548_chan;
        reg->i2c_slave = cpld_i2c_bus_device->i2c_slave;
        reg->i2c_reg = cpld_i2c_bus_device->i2c_reg;
        reg->i2c_data_len = cpld_i2c_bus_device->i2c_data_len;
        reg->i2c_ctrl = cpld_i2c_bus_device->i2c_ctrl;
        reg->i2c_status = cpld_i2c_bus_device->i2c_status;
        reg->i2c_scale = cpld_i2c_bus_device->i2c_scale;
        reg->i2c_filter = cpld_i2c_bus_device->i2c_filter;
        reg->i2c_stretch = cpld_i2c_bus_device->i2c_stretch;
        reg->i2c_ext_9548_exits_flag = cpld_i2c_bus_device->i2c_ext_9548_exits_flag;
        reg->i2c_in_9548_chan = cpld_i2c_bus_device->i2c_in_9548_chan;
        reg->i2c_data_buf = cpld_i2c_bus_device->i2c_data_buf;

        ret = find_intf_addr(&cpld_i2c->write_intf_addr, &cpld_i2c->read_intf_addr, cpld_i2c->i2c_func_mode);
        if (ret) {
            dev_err(dev, "find_intf_addr func mode %d fail, ret: %d.\n", cpld_i2c->i2c_func_mode, ret);
            return ret;
        }

        if (!cpld_i2c->write_intf_addr || !cpld_i2c->read_intf_addr) {
            dev_err(dev, "Fail: func mode %u rw symbol undefined.\n", cpld_i2c->i2c_func_mode);
            return -ENOSYS;
        }

        i2c_data_buf_len_reg = cpld_i2c_bus_device->i2c_data_buf_len_reg;
        if (i2c_data_buf_len_reg > 0) {
            ret = cpld_reg_read_32(cpld_i2c, i2c_data_buf_len_reg, &reg->i2c_data_buf_len);
            if (ret < 0) {
                dev_err(cpld_i2c->dev, "Failed to get cpld i2c data buf length, reg addr: 0x%x, ret: %d\n",
                    i2c_data_buf_len_reg, ret);
                return ret;
            }
            DEBUG_VERBOSE("cpld i2c data buf length reg addr: 0x%x, value: %d\n",
                i2c_data_buf_len_reg, reg->i2c_data_buf_len);
            if (reg->i2c_data_buf_len == 0) {
                reg->i2c_data_buf_len = CPLD_I2C_RDWR_MAX_LEN_DEFAULT;
            }
        } else {
            if (cpld_i2c_bus_device->i2c_data_buf_len == 0) {
                reg->i2c_data_buf_len = CPLD_I2C_RDWR_MAX_LEN_DEFAULT;
                DEBUG_VERBOSE("not support i2c_data_buf_len cfg, set default_val:%d\n",
                    reg->i2c_data_buf_len);
            } else {
                reg->i2c_data_buf_len = cpld_i2c_bus_device->i2c_data_buf_len;
            }
        }

        i2c_offset_reg = cpld_i2c_bus_device->i2c_offset_reg;
        if (i2c_offset_reg > 0) {
            rv = cpld_reg_read_32(cpld_i2c, i2c_offset_reg, &i2c_offset_val);
            if (rv < 0) {
                dev_err(cpld_i2c->dev, "Failed to get cpld i2c adapter offset value, reg addr: 0x%x, rv: %d\n",
                    i2c_offset_reg, rv);
                return rv;
            }
            DEBUG_VERBOSE("cpld i2c adapter offset reg addr: 0x%x, value: %d\n",
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
            reg->i2c_data_len += i2c_offset_val;
            reg->i2c_ctrl += i2c_offset_val;
            reg->i2c_status += i2c_offset_val;
            reg->i2c_data_buf += i2c_offset_val;
        }

        if (cpld_i2c_bus_device->i2c_err_vec == 0) {
            reg->i2c_err_vec = DTS_NO_CFG_FLAG;
            DEBUG_VERBOSE("not support i2c_err_vec cfg, set DTS_NO_CFG_FLAG:%d\n",
                reg->i2c_err_vec);
        } else {
            reg->i2c_err_vec = cpld_i2c_bus_device->i2c_err_vec;
            if (i2c_offset_val != 0) {
                reg->i2c_err_vec += i2c_offset_val;
            }
        }
    }

    DEBUG_VERBOSE("i2c_ext_9548_addr:0x%x, i2c_ext_9548_chan:0x%x, i2c_slave:0x%x, i2c_reg:0x%x, i2c_data_len:0x%x.\n",
        reg->i2c_ext_9548_addr, reg->i2c_ext_9548_chan, reg->i2c_slave, reg->i2c_reg, reg->i2c_data_len);
    DEBUG_VERBOSE("i2c_ctrl:0x%x, i2c_status:0x%x, i2c_scale:0x%x, i2c_filter:0x%x, i2c_stretch:0x%x.\n",
        reg->i2c_ctrl, reg->i2c_status, reg->i2c_scale, reg->i2c_filter, reg->i2c_stretch);
    DEBUG_VERBOSE("i2c_ext_9548_exits_flag:0x%x, i2c_in_9548_chan:0x%x, i2c_data_buf:0x%x, i2c_data_buf_len:0x%x.\n",
        reg->i2c_ext_9548_exits_flag, reg->i2c_in_9548_chan, reg->i2c_data_buf, reg->i2c_data_buf_len);
    DEBUG_VERBOSE("dev_name:%s, i2c_scale_value:0x%x, i2c_filter_value:0x%x, i2c_stretch_value:0x%x, i2c_timeout:0x%x.\n",
        cpld_i2c->dev_name, cpld_i2c->i2c_scale_value, cpld_i2c->i2c_filter_value, cpld_i2c->i2c_stretch_value, cpld_i2c->i2c_timeout);
    DEBUG_VERBOSE("i2c_reset_addr:0x%x, i2c_reset_on:0x%x, i2c_reset_off:0x%x, i2c_rst_delay_b:0x%x, i2c_rst_delay:0x%x, i2c_rst_delay_a:0x%x.\n",
        reset_cfg->reset_addr, reset_cfg->reset_on, reset_cfg->reset_off, reset_cfg->reset_delay_b, reset_cfg->reset_delay, reset_cfg->reset_delay_a);
    DEBUG_VERBOSE("i2c_adap_reset_flag:0x%x.\n", reset_cfg->i2c_adap_reset_flag);
    DEBUG_VERBOSE("i2c_err_vec:0x%x\n", reg->i2c_err_vec);

    return ret;
}

static int cpld_i2c_probe(struct platform_device *pdev)
{
    int ret;
    cpld_i2c_dev_t *cpld_i2c;
    struct device *dev;

    cpld_i2c = devm_kzalloc(&pdev->dev, sizeof(cpld_i2c_dev_t), GFP_KERNEL);
    if (!cpld_i2c) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        ret = -ENOMEM;
        goto out;
    }

    cpld_i2c->dev = &pdev->dev;

    ret = cpld_i2c_config_init(cpld_i2c);
    if (ret !=0) {
        dev_err(cpld_i2c->dev, "Failed to get cpld i2c dts config.\n");
        goto out;
    }

    ret = cpld_i2c_adapter_init(cpld_i2c);
    if (ret !=0) {
        dev_err(cpld_i2c->dev, "Failed to init cpld i2c adapter.\n");
        goto out;
    }

    if (cpld_i2c->dev->of_node) {
        cpld_i2c->i2c_params_check = of_property_read_bool(cpld_i2c->dev->of_node, "i2c_params_check");
    }
    DEBUG_VERBOSE("cpld i2c params check flag:%d.\n", cpld_i2c->i2c_params_check);

    init_waitqueue_head(&cpld_i2c->queue);

    dev = cpld_i2c->dev;
    cpld_i2c->adap = cpld_i2c_ops;
    cpld_i2c->adap.timeout = msecs_to_jiffies(cpld_i2c->i2c_timeout);
    cpld_i2c->adap.dev.parent = &pdev->dev;
    cpld_i2c->adap.dev.of_node = pdev->dev.of_node;
    i2c_set_adapdata(&cpld_i2c->adap, cpld_i2c);
    platform_set_drvdata(pdev, cpld_i2c);

    if (cpld_i2c->dev->of_node) {
        /* adap.nr get from dts aliases */
        ret = i2c_add_adapter(&cpld_i2c->adap);
    } else {
        cpld_i2c->adap.nr = cpld_i2c->adap_nr;
        ret = i2c_add_numbered_adapter(&cpld_i2c->adap);
    }

    if (ret < 0) {
        dev_info(cpld_i2c->dev, "Failed to add adapter.\n");
        goto fail_add;
    }
    
    I2C_ADAPTER_DEBUG_INIT(&cpld_i2c->adap, cpld_i2c_dev_t, i2c_ada_dbg);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
    of_i2c_register_devices(&cpld_i2c->adap);
#endif
    dev_info(cpld_i2c->dev, "registered i2c-%d for %s using mode %d with base address:0x%x, data buf len: %d success.\n",
        cpld_i2c->adap.nr, cpld_i2c->dev_name, cpld_i2c->i2c_func_mode, cpld_i2c->reg.i2c_scale,
        cpld_i2c->reg.i2c_data_buf_len);
    return 0;

fail_add:
    platform_set_drvdata(pdev, NULL);
out:
    return ret;
};

static void cpld_i2c_remove(struct platform_device *pdev)
{
    cpld_i2c_dev_t *cpld_i2c;

    cpld_i2c = platform_get_drvdata(pdev);
    i2c_adapter_debug_exit(&cpld_i2c->adap);
    i2c_del_adapter(&cpld_i2c->adap);
    platform_set_drvdata(pdev, NULL);
};

static struct of_device_id cpld_i2c_match[] = {
    {
        .compatible = "wb-cpld-i2c",
    },
    {},
};
MODULE_DEVICE_TABLE(of, cpld_i2c_match);

static struct platform_driver wb_cpld_i2c_driver = {
    .probe = cpld_i2c_probe,
    .remove = cpld_i2c_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = DRV_NAME,
        .of_match_table = cpld_i2c_match,
    },
};

static int __init wb_cpld_i2c_init(void)
{
    return platform_driver_register(&wb_cpld_i2c_driver);
}

static void __exit wb_cpld_i2c_exit(void)
{
    platform_driver_unregister(&wb_cpld_i2c_driver);
}

module_init(wb_cpld_i2c_init);
module_exit(wb_cpld_i2c_exit);
MODULE_DESCRIPTION("cpld i2c adapter driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
