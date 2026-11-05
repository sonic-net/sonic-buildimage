/*
 * I2C multiplexer driver for PCA9541 bus master selector
 *
 * Copyright (c) 2010 Ericsson AB.
 *
 * Author: Guenter Roeck <linux@roeck-us.net>
 *
 * Derived from:
 *  pca954x.c
 *
 *  Copyright (c) 2008-2009 Rodolfo Giometti <giometti@linux.it>
 *  Copyright (c) 2008-2009 Eurotech S.p.A. <info@eurotech.it>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "wb_i2c_mux_pca9641.h"

/*
 * The PCA9541 is a bus master selector. It supports two I2C masters connected
 * to a single slave bus.
 *
 * Before each bus transaction, a master has to acquire bus ownership. After the
 * transaction is complete, bus ownership has to be released. This fits well
 * into the I2C multiplexer framework, which provides select and release
 * functions for this purpose. For this reason, this driver is modeled as
 * single-channel I2C bus multiplexer.
 *
 * This driver assumes that the two bus masters are controlled by two different
 * hosts. If a single host controls both masters, platform code has to ensure
 * that only one of the masters is instantiated at any given time.
 */

#define PCA9541_CONTROL		0x01
#define PCA9541_ISTAT		0x02

#define PCA9541_CTL_MYBUS	(1 << 0)
#define PCA9541_CTL_NMYBUS	(1 << 1)
#define PCA9541_CTL_BUSON	(1 << 2)
#define PCA9541_CTL_NBUSON	(1 << 3)
#define PCA9541_CTL_BUSINIT	(1 << 4)
#define PCA9541_CTL_TESTON	(1 << 6)
#define PCA9541_CTL_NTESTON	(1 << 7)
#define PCA9541_ISTAT_INTIN	(1 << 0)
#define PCA9541_ISTAT_BUSINIT	(1 << 1)
#define PCA9541_ISTAT_BUSOK	(1 << 2)
#define PCA9541_ISTAT_BUSLOST	(1 << 3)
#define PCA9541_ISTAT_MYTEST	(1 << 6)
#define PCA9541_ISTAT_NMYTEST	(1 << 7)
#define PCA9641_ID             0x00
#define PCA9641_ID_MAGIC       0x38
#define PCA9641_CONTROL        0x01
#define PCA9641_STATUS         0x02
#define PCA9641_TIME           0x03
#define PCA9641_CTL_LOCK_REQ           BIT(0)
#define PCA9641_CTL_LOCK_GRANT         BIT(1)
#define PCA9641_CTL_BUS_CONNECT        BIT(2)
#define PCA9641_CTL_BUS_INIT           BIT(3)
#define PCA9641_CTL_SMBUS_SWRST        BIT(4)
#define PCA9641_CTL_IDLE_TIMER_DIS     BIT(5)
#define PCA9641_CTL_SMBUS_DIS          BIT(6)
#define PCA9641_CTL_PRIORITY           BIT(7)
#define PCA9641_STS_OTHER_LOCK         BIT(0)
#define PCA9641_STS_BUS_INIT_FAIL      BIT(1)
#define PCA9641_STS_BUS_HUNG           BIT(2)
#define PCA9641_STS_MBOX_EMPTY         BIT(3)
#define PCA9641_STS_MBOX_FULL          BIT(4)
#define PCA9641_STS_TEST_INT           BIT(5)
#define PCA9641_STS_SCL_IO             BIT(6)
#define PCA9641_STS_SDA_IO             BIT(7)
#define PCA9641_RES_TIME       0x03
#define BUSON		(PCA9541_CTL_BUSON | PCA9541_CTL_NBUSON)
#define MYBUS		(PCA9541_CTL_MYBUS | PCA9541_CTL_NMYBUS)
#define mybus(x)	(!((x) & MYBUS) || ((x) & MYBUS) == MYBUS)
#define busoff(x)	(!((x) & BUSON) || ((x) & BUSON) == BUSON)
#define BUSOFF(x, y)   (!((x) & PCA9641_CTL_LOCK_GRANT) && \
                       !((y) & PCA9641_STS_OTHER_LOCK))
#define other_lock(x)  ((x) & PCA9641_STS_OTHER_LOCK)
#define lock_grant(x)  ((x) & PCA9641_CTL_LOCK_GRANT)

#define PCA9641_RETRY_TIME     (8)
#define PCA9641_RESET_DELAY    (150)

typedef struct i2c_muxs_struct_flag
{
	int nr;
	char name[48];
	struct mutex	update_lock;
	int flag;
}i2c_mux_flag;

i2c_mux_flag pca_flag = {
	.flag = -1,
};

int pca9641_setmuxflag(int nr, int flag)
{
	if (pca_flag.nr == nr) {
	    pca_flag.flag = flag;
	}
	return 0;
}
EXPORT_SYMBOL(pca9641_setmuxflag);

static int g_debug_info = 0;
static int g_debug_err = 0;

module_param(g_debug_info, int, S_IRUGO | S_IWUSR);
module_param(g_debug_err, int, S_IRUGO | S_IWUSR);

#define PCA_DEBUG(fmt, args...) do {                                        \
    if (g_debug_info) { \
        printk(KERN_INFO "[pca9641][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PCA_DEBUG_ERR(fmt, args...) do {                                        \
    if (g_debug_err) { \
        printk(KERN_ERR "[pca9641][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

/* arbitration timeouts, in jiffies */
#define ARB_TIMEOUT	(HZ / 8)	/* 125 ms until forcing bus ownership */
#define ARB2_TIMEOUT	(HZ / 4)	/* 250 ms until acquisition failure */

/* arbitration retry delays, in us */
#define SELECT_DELAY_SHORT	50
#define SELECT_DELAY_LONG	1000
#define I2C_RETRY_TIMES         (5)
#define I2C_RETRY_WAIT_TIMES    (10)      /*delay 10ms*/

typedef struct pca9641_cfg_info_s {
    uint32_t pca9641_reset_type;
    uint32_t rst_delay_b; /* delay time before reset(us) */
    uint32_t rst_delay;   /* reset time(us) */
    uint32_t rst_delay_a; /* delay time after reset(us) */
    union {
        i2c_attr_t i2c_attr;
        gpio_attr_t gpio_attr;
        io_attr_t io_attr;
        file_attr_t file_attr;
    } attr;
} pca9641_cfg_info_t;

struct pca9541 {
	struct i2c_client *client;
	unsigned long select_timeout;
	unsigned long arb_timeout;
    uint32_t pca9641_nr;
    pca9641_cfg_info_t pca9641_cfg_info; /* pca9641 reset cfg */
};

static const struct i2c_device_id pca9541_id[] = {
	{"wb_pca9541", 0},
	{"wb_pca9641", 1},
	{}
};

MODULE_DEVICE_TABLE(i2c, pca9541_id);

#ifdef CONFIG_OF
static const struct of_device_id pca9541_of_match[] = {
    { .compatible = "nxp,wb_pca9541" },
    { .compatible = "nxp,wb_pca9641" },
    {}
};
MODULE_DEVICE_TABLE(of, pca9541_of_match);
#endif

static int pca9641_gpio_init(gpio_attr_t *gpio_attr)
{
    int err;

    if (gpio_attr->gpio_init) {
        PCA_DEBUG("gpio%d already init, do nothing.\n", gpio_attr->gpio);
        return 0;
    }

    PCA_DEBUG("gpio%d init.\n", gpio_attr->gpio);
    err = gpio_request(gpio_attr->gpio, "pca9641_reset");
    if (err) {
        goto error;
    }
    err = gpio_direction_output(gpio_attr->gpio, gpio_attr->reset_off);
    if (err) {
        gpio_free(gpio_attr->gpio);
        goto error;
    }
    gpio_attr->gpio_init = 1;
    return 0;
error:
    PCA_DEBUG_ERR("pca9641_gpio_init failed, ret:%d.\n", err);
    return err;
}

static void pca9641_gpio_free(gpio_attr_t *gpio_attr)
{
    if (gpio_attr->gpio_init == 1) {
        PCA_DEBUG("gpio%d release.\n", gpio_attr->gpio);
        gpio_free(gpio_attr->gpio);
        gpio_attr->gpio_init = 0;
    }
    return;
}

static int pca9641_reset_file_read(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        PCA_DEBUG_ERR("read open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_read(filp, val, size, &tmp_pos);
    if (ret < 0) {
        PCA_DEBUG_ERR("kernel_read failed, path=%s, addr=0x%x, size=%ld, ret=%d\r\n", path, pos, size, ret);
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

static int pca9641_reset_file_write(const char *path, uint32_t pos, uint8_t *val, size_t size)
{

    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        PCA_DEBUG_ERR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_write(filp, val, size, &tmp_pos);
    if (ret < 0) {
        PCA_DEBUG_ERR("kernel_write failed, path=%s, addr=0x%x, size=%ld, ret=%d\r\n", path, pos, size, ret);
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

static int pca9641_reset_i2c_read(uint32_t bus, uint32_t addr, uint32_t offset_addr,
               unsigned char *buf, uint32_t size)
{
    struct file *fp;
    struct i2c_client client;
    char i2c_path[32];
    int i, j;
    int rv;

    rv = 0;
    mem_clear(i2c_path, sizeof(i2c_path));
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", bus);
    fp = filp_open(i2c_path, O_RDWR, S_IRUSR | S_IWUSR);
    if (IS_ERR(fp)) {
        PCA_DEBUG_ERR("i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = addr;
    for (j = 0; j < size; j++) {
        for (i = 0; i < I2C_RETRY_TIMES; i++) {
            rv = i2c_smbus_read_byte_data(&client, (offset_addr + j));
            if (rv < 0) {
                PCA_DEBUG_ERR("i2c read failed, try again.\n");
                msleep(I2C_RETRY_WAIT_TIMES);
                if (i >= (I2C_RETRY_TIMES - 1)) {
                    goto out;
                }
                continue;
          }
          *(buf + j) = (unsigned char)rv;
          break;
        }
    }
out:
    filp_close(fp, NULL);
    return rv;
}

static int pca9641_reset_i2c_write(uint32_t bus, uint32_t dev_addr, uint32_t offset_addr,
               uint8_t write_buf)
{
    struct file *fp;
    struct i2c_client client;
    char i2c_path[32];
    int i;
    int rv;

    rv = 0;
    mem_clear(i2c_path, sizeof(i2c_path));
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", bus);
    fp = filp_open(i2c_path, O_RDWR, S_IRUSR | S_IWUSR);
    if (IS_ERR(fp)) {
        PCA_DEBUG_ERR("i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = dev_addr;
    for (i = 0; i < I2C_RETRY_TIMES; i++) {
        rv = i2c_smbus_write_byte_data(&client, offset_addr, write_buf);
        if (rv < 0) {
            PCA_DEBUG_ERR("i2c write failed, try again.\n");
            msleep(I2C_RETRY_WAIT_TIMES);
            if (i >= (I2C_RETRY_TIMES - 1)) {
                goto out;
            }
            continue;
        }
        break;
    }
out:
    filp_close(fp, NULL);
    return rv;
}

static int pca9641_do_file_reset(struct i2c_mux_core *muxc)
{
    int ret, timeout, err;
    struct pca9541 *data;
    pca9641_cfg_info_t *reset_cfg;
    file_attr_t *file_attr;
    u8 val;

    data = i2c_mux_priv(muxc);
    reset_cfg = &data->pca9641_cfg_info;
    file_attr = &reset_cfg->attr.file_attr;
    ret = -1;

    PCA_DEBUG("rst_delay_b:%u, rst_delay:%u, rst_delay_a:%u.\n",
        reset_cfg->rst_delay_b, reset_cfg->rst_delay, reset_cfg->rst_delay_a);
    PCA_DEBUG("dev_name:%s, offset:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
        file_attr->dev_name, file_attr->offset, file_attr->mask,
        file_attr->reset_on, file_attr->reset_off);

    if (reset_cfg->rst_delay_b) {
        udelay(reset_cfg->rst_delay_b);
    }

    err = pca9641_reset_file_read(file_attr->dev_name, file_attr->offset, &val, sizeof(val));
    if (err < 0) {
        goto out;
    }

    val &= ~(file_attr->mask);
    val |= file_attr->reset_on;
    err = pca9641_reset_file_write(file_attr->dev_name, file_attr->offset, &val, sizeof(val));
    if (err < 0) {
        goto out;
    }

    if (reset_cfg->rst_delay) {
        udelay(reset_cfg->rst_delay);
    }

    val &= ~(file_attr->mask);
    val |= file_attr->reset_off;
    err = pca9641_reset_file_write(file_attr->dev_name, file_attr->offset, &val, sizeof(val));
    if (err < 0) {
        goto out;
    }

    timeout = reset_cfg->rst_delay_a;
    while (timeout > 0) {
        udelay(1);
        err = pca9641_reset_file_read(file_attr->dev_name, file_attr->offset, &val, sizeof(val));
        if (err < 0) {
            goto out;
        }
        val &= (file_attr->mask);
        if (val == file_attr->reset_off) {
            ret = 0;
            PCA_DEBUG("pca9641_do_file_reset success.\n");
            break;
        }
        if (timeout >= 1000 && (timeout % 1000 == 0)) {
            schedule();
        }
        timeout--;
    }
    if (ret < 0) {
        PCA_DEBUG_ERR("pca9641_do_file_reset timeout.\n");
    }
out:
    if (err < 0) {
        PCA_DEBUG_ERR("pca9641_do_file_reset file rd/wr failed, ret:%d.\n", err);
    }

    return ret;
}

static int pca9641_do_io_reset(struct i2c_mux_core *muxc)
{
    int ret, timeout;
    struct pca9541 *data;
    pca9641_cfg_info_t *reset_cfg;
    io_attr_t *io_attr;
    u8 val;

    data = i2c_mux_priv(muxc);
    reset_cfg = &data->pca9641_cfg_info;
    io_attr = &reset_cfg->attr.io_attr;

    PCA_DEBUG("rst_delay_b:%u, rst_delay:%u, rst_delay_a:%u.\n",
        reset_cfg->rst_delay_b, reset_cfg->rst_delay, reset_cfg->rst_delay_a);
    PCA_DEBUG("io_addr:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
        io_attr->io_addr, io_attr->mask, io_attr->reset_on, io_attr->reset_off);

    if (reset_cfg->rst_delay_b) {
        udelay(reset_cfg->rst_delay_b);
    }

    val = inb(io_attr->io_addr);
    val &= ~(io_attr->mask);
    val |= io_attr->reset_on;
    outb(val, io_attr->io_addr);

    if (reset_cfg->rst_delay) {
        udelay(reset_cfg->rst_delay);
    }

    val &= ~(io_attr->mask);
    val |= io_attr->reset_off;
    outb(val, io_attr->io_addr);

    ret = -1;
    timeout = reset_cfg->rst_delay_a;
    while (timeout > 0) {
        udelay(1);
        val = inb(io_attr->io_addr);
        val &= (io_attr->mask);
        if (val == io_attr->reset_off) {
            ret = 0;
            PCA_DEBUG("pca9641_do_io_reset success.\n");
            break;
        }
        if (timeout >= 1000 && (timeout % 1000 == 0)) {
            schedule();
        }
        timeout--;
    }

    if (ret < 0) {
        PCA_DEBUG_ERR("pca9641_do_io_reset timeout.\n");
    }

    return ret;
}

static int pca9641_do_gpio_reset(struct i2c_mux_core *muxc)
{
    int ret, timeout;
    struct pca9541 *data;
    pca9641_cfg_info_t *reset_cfg;
    gpio_attr_t *gpio_attr;
    u8 val;

    data = i2c_mux_priv(muxc);
    reset_cfg = &data->pca9641_cfg_info;
    gpio_attr = &reset_cfg->attr.gpio_attr;

    ret = pca9641_gpio_init(gpio_attr);
    if (ret) {
        return -1;
    }

    if (reset_cfg->rst_delay_b) {
        udelay(reset_cfg->rst_delay_b);
    }

    __gpio_set_value(gpio_attr->gpio, gpio_attr->reset_on);

    if (reset_cfg->rst_delay) {
        udelay(reset_cfg->rst_delay);
    }

    __gpio_set_value(gpio_attr->gpio, gpio_attr->reset_off);
    ret = -1;
    timeout = reset_cfg->rst_delay_a;
    while (timeout > 0) {
        udelay(1);
        val = __gpio_get_value(gpio_attr->gpio);
        if (val == gpio_attr->reset_off) {
            ret = 0;
            PCA_DEBUG("pca9641_do_gpio_reset success.\n");
            break;
        }
        if (timeout >= 1000 && (timeout % 1000 == 0)) {
            /* 1MS schedule*/
            schedule();
        }
        timeout--;
    }

    if (ret < 0) {
        PCA_DEBUG_ERR("pca9641_do_gpio_reset timeout.\n");
    }

    pca9641_gpio_free(gpio_attr);
    return ret;
}

static int pca9641_do_i2c_reset(struct i2c_mux_core *muxc)
{
    int ret, timeout, err;
    struct pca9541 *data;
    pca9641_cfg_info_t *reset_cfg;
    i2c_attr_t *i2c_attr;
    u8 val;

    data = i2c_mux_priv(muxc);
    reset_cfg = &data->pca9641_cfg_info;
    i2c_attr = &reset_cfg->attr.i2c_attr;
    ret = -1;

    PCA_DEBUG("rst_delay_b:%u, rst_delay:%u, rst_delay_a:%u.\n",
        reset_cfg->rst_delay_b, reset_cfg->rst_delay, reset_cfg->rst_delay_a);
    PCA_DEBUG("bus:0x%x, addr:0x%x, reg:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
        i2c_attr->i2c_bus, i2c_attr->i2c_addr, i2c_attr->reg_offset,
        i2c_attr->mask, i2c_attr->reset_on, i2c_attr->reset_off);

    if (reset_cfg->rst_delay_b) {
        udelay(reset_cfg->rst_delay_b);
    }

    err = pca9641_reset_i2c_read(i2c_attr->i2c_bus, i2c_attr->i2c_addr,
              i2c_attr->reg_offset, &val, sizeof(val));
    if (err < 0) {
        goto out;
    }

    val &= ~(i2c_attr->mask);
    val |= i2c_attr->reset_on;
    err = pca9641_reset_i2c_write(i2c_attr->i2c_bus, i2c_attr->i2c_addr,
              i2c_attr->reg_offset, val);
    if (err < 0) {
        goto out;
    }

    if (reset_cfg->rst_delay) {
        udelay(reset_cfg->rst_delay);
    }

    val &= ~(i2c_attr->mask);
    val |= i2c_attr->reset_off;
    err = pca9641_reset_i2c_write(i2c_attr->i2c_bus, i2c_attr->i2c_addr,
              i2c_attr->reg_offset, val);
    if (err < 0) {
        goto out;
    }

    timeout = reset_cfg->rst_delay_a;
    while (timeout > 0) {
        udelay(1);
        err = pca9641_reset_i2c_read(i2c_attr->i2c_bus, i2c_attr->i2c_addr,
                  i2c_attr->reg_offset, &val, sizeof(val));
        if (err < 0) {
            goto out;
        }
        val &= (i2c_attr->mask);
        if (val == i2c_attr->reset_off) {
            ret = 0;
            PCA_DEBUG("pca9641_do_i2c_reset success.\n");
            break;
        }
        if (timeout >= 1000 && (timeout % 1000 == 0)) {
            schedule();
        }
        timeout--;
    }
    if (ret < 0) {
        PCA_DEBUG_ERR("pca9641_do_i2c_reset timeout.\n");
    }
out:
    if (err < 0) {
        PCA_DEBUG_ERR("pca9641_do_i2c_reset i2c op failed, ret:%d.\n", err);
    }
    return ret;
}

static int pca9641_do_reset(struct i2c_mux_core *muxc)
{
    int ret;
    struct pca9541 *data;

    data = i2c_mux_priv(muxc);
    if (data->pca9641_cfg_info.pca9641_reset_type == PCA9641_RESET_NONE) {
        ret = -1;
        PCA_DEBUG("Don't need to reset.\n");
    } else if (data->pca9641_cfg_info.pca9641_reset_type == PCA9641_RESET_I2C) {
        ret = pca9641_do_i2c_reset(muxc);
    } else if (data->pca9641_cfg_info.pca9641_reset_type == PCA9641_RESET_GPIO) {
        ret = pca9641_do_gpio_reset(muxc);
    } else if (data->pca9641_cfg_info.pca9641_reset_type == PCA9641_RESET_IO) {
        ret = pca9641_do_io_reset(muxc);
    } else if (data->pca9641_cfg_info.pca9641_reset_type == PCA9641_RESET_FILE) {
        ret = pca9641_do_file_reset(muxc);
    } else {
        ret = -1;
        PCA_DEBUG_ERR("Unsupport reset type:0x%x.\n",
            data->pca9641_cfg_info.pca9641_reset_type);
    }

    if (ret < 0) {
        PCA_DEBUG_ERR("pca9641_reset_ctrl failed, reset type:%u, ret:%d.\n",
            data->pca9641_cfg_info.pca9641_reset_type, ret);
    } else {
        udelay(PCA9641_RESET_DELAY);
    }
    return ret;
}

/*
 * Write to chip register. Don't use i2c_transfer()/i2c_smbus_xfer()
 * as they will try to lock the adapter a second time.
 */
static int pca9541_reg_write(struct i2c_client *client, u8 command, u8 val)
{
	struct i2c_adapter *adap = client->adapter;
	int ret;

	if (adap->algo->master_xfer) {
		struct i2c_msg msg;
		char buf[2];

		msg.addr = client->addr;
		msg.flags = 0;
		msg.len = 2;
		buf[0] = command;
		buf[1] = val;
		msg.buf = buf;
		ret = __i2c_transfer(adap, &msg, 1);
	} else {
		union i2c_smbus_data data;

		data.byte = val;
		ret = adap->algo->smbus_xfer(adap, client->addr,
					     client->flags,
					     I2C_SMBUS_WRITE,
					     command,
					     I2C_SMBUS_BYTE_DATA, &data);
	}

	return ret;
}

/*
 * Read from chip register. Don't use i2c_transfer()/i2c_smbus_xfer()
 * as they will try to lock adapter a second time.
 */
static int pca9541_reg_read(struct i2c_client *client, u8 command)
{
	struct i2c_adapter *adap = client->adapter;
	int ret;
	u8 val;

	if (adap->algo->master_xfer) {
		struct i2c_msg msg[2] = {
			{
				.addr = client->addr,
				.flags = 0,
				.len = 1,
				.buf = &command
			},
			{
				.addr = client->addr,
				.flags = I2C_M_RD,
				.len = 1,
				.buf = &val
			}
		};
		ret = __i2c_transfer(adap, msg, 2);
		if (ret == 2)
			ret = val;
		else if (ret >= 0)
			ret = -EIO;
	} else {
		union i2c_smbus_data data;

		ret = adap->algo->smbus_xfer(adap, client->addr,
					     client->flags,
					     I2C_SMBUS_READ,
					     command,
					     I2C_SMBUS_BYTE_DATA, &data);
		if (!ret)
			ret = data.byte;
	}
	return ret;
}

/*
 * Arbitration management functions
 */

/* Release bus. Also reset NTESTON and BUSINIT if it was set. */
static void pca9541_release_bus(struct i2c_client *client)
{
	int reg;

	reg = pca9541_reg_read(client, PCA9541_CONTROL);
	if (reg >= 0 && !busoff(reg) && mybus(reg))
		pca9541_reg_write(client, PCA9541_CONTROL,
				  (reg & PCA9541_CTL_NBUSON) >> 1);
}

/*
 * Arbitration is defined as a two-step process. A bus master can only activate
 * the slave bus if it owns it; otherwise it has to request ownership first.
 * This multi-step process ensures that access contention is resolved
 * gracefully.
 *
 * Bus	Ownership	Other master	Action
 * state		requested access
 * ----------------------------------------------------
 * off	-		yes		wait for arbitration timeout or
 *					for other master to drop request
 * off	no		no		take ownership
 * off	yes		no		turn on bus
 * on	yes		-		done
 * on	no		-		wait for arbitration timeout or
 *					for other master to release bus
 *
 * The main contention point occurs if the slave bus is off and both masters
 * request ownership at the same time. In this case, one master will turn on
 * the slave bus, believing that it owns it. The other master will request
 * bus ownership. Result is that the bus is turned on, and master which did
 * _not_ own the slave bus before ends up owning it.
 */

/* Control commands per PCA9541 datasheet */
static const u8 pca9541_control[16] = {
	4, 0, 1, 5, 4, 4, 5, 5, 0, 0, 1, 1, 0, 4, 5, 1
};

/*
 * Channel arbitration
 *
 * Return values:
 *  <0: error
 *  0 : bus not acquired
 *  1 : bus acquired
 */
static int pca9541_arbitrate(struct i2c_client *client)
{
	struct i2c_mux_core *muxc = i2c_get_clientdata(client);
	struct pca9541 *data = i2c_mux_priv(muxc);
	int reg;

	reg = pca9541_reg_read(client, PCA9541_CONTROL);
	if (reg < 0)
		return reg;

	if (busoff(reg)) {
		int istat;
		/*
		 * Bus is off. Request ownership or turn it on unless
		 * other master requested ownership.
		 */
		istat = pca9541_reg_read(client, PCA9541_ISTAT);
		if (!(istat & PCA9541_ISTAT_NMYTEST)
		    || time_is_before_eq_jiffies(data->arb_timeout)) {
			/*
			 * Other master did not request ownership,
			 * or arbitration timeout expired. Take the bus.
			 */
			pca9541_reg_write(client,
					  PCA9541_CONTROL,
					  pca9541_control[reg & 0x0f]
					  | PCA9541_CTL_NTESTON);
			data->select_timeout = SELECT_DELAY_SHORT;
		} else {
			/*
			 * Other master requested ownership.
			 * Set extra long timeout to give it time to acquire it.
			 */
			data->select_timeout = SELECT_DELAY_LONG * 2;
		}
	} else if (mybus(reg)) {
		/*
		 * Bus is on, and we own it. We are done with acquisition.
		 * Reset NTESTON and BUSINIT, then return success.
		 */
		if (reg & (PCA9541_CTL_NTESTON | PCA9541_CTL_BUSINIT))
			pca9541_reg_write(client,
					  PCA9541_CONTROL,
					  reg & ~(PCA9541_CTL_NTESTON
						  | PCA9541_CTL_BUSINIT));
		return 1;
	} else {
		/*
		 * Other master owns the bus.
		 * If arbitration timeout has expired, force ownership.
		 * Otherwise request it.
		 */
		data->select_timeout = SELECT_DELAY_LONG;
		if (time_is_before_eq_jiffies(data->arb_timeout)) {
			/* Time is up, take the bus and reset it. */
			pca9541_reg_write(client,
					  PCA9541_CONTROL,
					  pca9541_control[reg & 0x0f]
					  | PCA9541_CTL_BUSINIT
					  | PCA9541_CTL_NTESTON);
		} else {
			/* Request bus ownership if needed */
			if (!(reg & PCA9541_CTL_NTESTON))
				pca9541_reg_write(client,
						  PCA9541_CONTROL,
						  reg | PCA9541_CTL_NTESTON);
		}
	}
	return 0;
}

static int pca9541_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
	struct pca9541 *data = i2c_mux_priv(muxc);
	struct i2c_client *client = data->client;
	int ret;
	unsigned long timeout = jiffies + ARB2_TIMEOUT;
		/* give up after this time */

	data->arb_timeout = jiffies + ARB_TIMEOUT;
		/* force bus ownership after this time */

	do {
		ret = pca9541_arbitrate(client);
		if (ret)
			return ret < 0 ? ret : 0;

		if (data->select_timeout == SELECT_DELAY_SHORT)
			udelay(data->select_timeout);
		else
			msleep(data->select_timeout / 1000);
	} while (time_is_after_eq_jiffies(timeout));

    dev_warn(&client->dev, "pca9541 select channel timeout.\n");
	return -ETIMEDOUT;
}

static int pca9541_release_chan(struct i2c_mux_core *muxc, u32 chan)
{
    struct pca9541 *data = i2c_mux_priv(muxc);
	struct i2c_client *client = data->client;
	pca9541_release_bus(client);
	return 0;
}

/*
* Arbitration management functions
*/
static void pca9641_release_bus(struct i2c_client *client)
{
   pca9541_reg_write(client, PCA9641_CONTROL, 0x80); //master 0x80
}

/*
* Channel arbitration
*
* Return values:
*  <0: error
*  0 : bus not acquired
*  1 : bus acquired
*/
static int pca9641_arbitrate(struct i2c_client *client)
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct pca9541 *data = i2c_mux_priv(muxc);
    int reg_ctl, reg_sts;

    reg_ctl = pca9541_reg_read(client, PCA9641_CONTROL);
    if (reg_ctl < 0) {
        PCA_DEBUG_ERR("pca9641 read control register failed, ret:%d.\n", reg_ctl);
        return reg_ctl;
    }

    reg_sts = pca9541_reg_read(client, PCA9641_STATUS);
    if (reg_sts < 0) {
        PCA_DEBUG_ERR("pca9641 read status register failed, ret:%d.\n", reg_sts);
        return reg_sts;
    }

    if (BUSOFF(reg_ctl, reg_sts)) {
        /*
         * Bus is off. Request ownership or turn it on unless
         * other master requested ownership.
         */
        reg_ctl |= PCA9641_CTL_LOCK_REQ;
        pca9541_reg_write(client, PCA9641_CONTROL, reg_ctl);
        reg_ctl = pca9541_reg_read(client, PCA9641_CONTROL);
        if (reg_ctl < 0) {
            PCA_DEBUG_ERR("Bus is off, but read control register failed, ret:%d.\n", reg_ctl);
            return reg_ctl;
        }

        if (lock_grant(reg_ctl)) {
            /*
            * Other master did not request ownership,
            * or arbitration timeout expired. Take the bus.
            */
            PCA_DEBUG("Bus is off, get pca9641 arbitration success.\n");
            reg_ctl |= PCA9641_CTL_BUS_CONNECT | PCA9641_CTL_LOCK_REQ;
            pca9541_reg_write(client, PCA9641_CONTROL, reg_ctl);
            return 1;
        } else {
            /*
            * Other master requested ownership.
            * Set extra long timeout to give it time to acquire it.
            */
            PCA_DEBUG("Bus is off, but get pca9641 arbitration failed.\n");
            data->select_timeout = SELECT_DELAY_LONG * 2;
        }
    } else if (lock_grant(reg_ctl)) {
        /*
        * Bus is on, and we own it. We are done with acquisition.
        */
        PCA_DEBUG("Bus is on, get pca9641 arbitration success.\n");
        reg_ctl |= PCA9641_CTL_BUS_CONNECT | PCA9641_CTL_LOCK_REQ;
        pca9541_reg_write(client, PCA9641_CONTROL, reg_ctl);
        return 1;
    } else if (other_lock(reg_sts)) {
        /*
        * Other master owns the bus.
        * If arbitration timeout has expired, force ownership.
        * Otherwise request it.
        */
        PCA_DEBUG("Other master owns the bus, try to request it.\n");
        data->select_timeout = SELECT_DELAY_LONG;
        reg_ctl |= PCA9641_CTL_LOCK_REQ;
        pca9541_reg_write(client, PCA9641_CONTROL, reg_ctl);
    }
    return 0;
}

int pca9641_select_chan_single(struct i2c_mux_core *muxc, u32 chan)
{
	struct pca9541 *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    int ret;
    int result;
    unsigned long msleep_time;
    unsigned long timeout = jiffies + ARB2_TIMEOUT;
    /* give up after this time */
    data->arb_timeout = jiffies + ARB_TIMEOUT;
    /* force bus ownership after this time */
   	for (result = 0 ; result < PCA9641_RETRY_TIME ; result ++) {
   	   do {
   		   ret = pca9641_arbitrate(client);
           if (ret) {
               return ret < 0 ? -EIO : 0;
           }
           msleep_time = data->select_timeout / 1000;
           if (msleep_time < 1) {
               msleep(1);
           } else {
               msleep(msleep_time);
           }
   	   } while (time_is_after_eq_jiffies(timeout));
       timeout = jiffies + ARB2_TIMEOUT;
   	}
    dev_warn(&client->dev, "pca9641 select channel timeout.\n");
    return -ETIMEDOUT;
}

static int pca9641_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
    int ret, rv;

    ret = pca9641_select_chan_single(muxc, chan);
    if (ret < 0) {
        PCA_DEBUG_ERR("pca9641 select channel failed, ret:%d, try to reset pca9641.\n", ret);
        rv = pca9641_do_reset(muxc);

        if (rv < 0) {
            PCA_DEBUG_ERR("pca9641 reset failed, rv:%d.\n", rv);
            return ret;
        }

        ret = pca9641_select_chan_single(muxc, chan);
        if (ret < 0) {
            PCA_DEBUG_ERR("after pca9641 reset, select channel still failed, ret:%d.\n", ret);
        }
    }
    return ret;
}

static int pca9641_release_chan(struct i2c_mux_core *muxc, u32 chan)
{
	struct pca9541 *data = i2c_mux_priv(muxc);
	struct i2c_client *client = data->client;
	if (pca_flag.flag) {
		pca9641_release_bus(client);
	}
	return 0;
}

static int pca9641_detect_id(struct i2c_client *client)
{
   int reg;

   reg = pca9541_reg_read(client, PCA9641_ID);
   if (reg == PCA9641_ID_MAGIC)
           return 1;
   else
           return 0;
}

static int pca9641_recordflag(struct i2c_adapter *adap) {
    if (pca_flag.flag != -1) {
        pr_err(" %s %d has init already!!!", __func__, __LINE__);
        return -1 ;
    }
    pca_flag.nr = adap->nr;
	PCA_DEBUG(" adap->nr:%d\n",  adap->nr);
	snprintf(pca_flag.name, sizeof(pca_flag.name),adap->name);
    return 0;
}

static int of_pca9641_reset_data_init(struct pca9541 *data)
{
    int err;
    struct device *dev = &data->client->dev;
    pca9641_cfg_info_t *reset_cfg;

    reset_cfg = &data->pca9641_cfg_info;
    if (dev == NULL || dev->of_node == NULL) {
        PCA_DEBUG("dev or dev->of_node is NUll, no reset.\n");
        reset_cfg->pca9641_reset_type = PCA9641_RESET_NONE;
        return 0;
    }

    if (of_property_read_u32(dev->of_node, "pca9641_reset_type", &reset_cfg->pca9641_reset_type)) {

        PCA_DEBUG("pca9641_reset_type not found, no reset.\n");
        reset_cfg->pca9641_reset_type = PCA9641_RESET_NONE;
        return 0;
    }
    err = of_property_read_u32(dev->of_node, "rst_delay_b", &reset_cfg->rst_delay_b);
    err |= of_property_read_u32(dev->of_node, "rst_delay", &reset_cfg->rst_delay);
    err |= of_property_read_u32(dev->of_node, "rst_delay_a", &reset_cfg->rst_delay_a);

    if (err) {
        goto dts_config_err;
    }
    PCA_DEBUG("reset_type:0x%x, rst_delay_b:0x%x, rst_delay:0x%x, rst_delay_a:0x%x.\n",
        reset_cfg->pca9641_reset_type, reset_cfg->rst_delay_b,
        reset_cfg->rst_delay, reset_cfg->rst_delay_a);

    if (reset_cfg->pca9641_reset_type == PCA9641_RESET_I2C) {

        PCA_DEBUG("reset by i2c.\n");
        err = of_property_read_u32(dev->of_node, "i2c_bus", &reset_cfg->attr.i2c_attr.i2c_bus);
        err |=of_property_read_u32(dev->of_node, "i2c_addr", &reset_cfg->attr.i2c_attr.i2c_addr);
        err |=of_property_read_u32(dev->of_node, "reg_offset", &reset_cfg->attr.i2c_attr.reg_offset);
        err |=of_property_read_u32(dev->of_node, "mask", &reset_cfg->attr.i2c_attr.mask);
        err |=of_property_read_u32(dev->of_node, "reset_on", &reset_cfg->attr.i2c_attr.reset_on);
        err |=of_property_read_u32(dev->of_node, "reset_off", &reset_cfg->attr.i2c_attr.reset_off);
        if (err) {
            goto dts_config_err;
        }
        PCA_DEBUG("bus:%u, addr:0x%x, offset:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
            reset_cfg->attr.i2c_attr.i2c_bus, reset_cfg->attr.i2c_attr.i2c_addr,
            reset_cfg->attr.i2c_attr.reg_offset, reset_cfg->attr.i2c_attr.mask,
            reset_cfg->attr.i2c_attr.reset_on, reset_cfg->attr.i2c_attr.reset_off);
    } else if (reset_cfg->pca9641_reset_type == PCA9641_RESET_GPIO) {

        PCA_DEBUG("reset by gpio.\n");
        err = of_property_read_u32(dev->of_node, "gpio", &reset_cfg->attr.gpio_attr.gpio);
        err |=of_property_read_u32(dev->of_node, "reset_on", &reset_cfg->attr.gpio_attr.reset_on);
        err |=of_property_read_u32(dev->of_node, "reset_off", &reset_cfg->attr.gpio_attr.reset_off);
        if (err) {
            goto dts_config_err;
        }
        PCA_DEBUG("gpio number:%u, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.gpio_attr.gpio, reset_cfg->attr.gpio_attr.reset_on,
            reset_cfg->attr.gpio_attr.reset_off);
        reset_cfg->attr.gpio_attr.gpio_init = 0;
    } else if (reset_cfg->pca9641_reset_type == PCA9641_RESET_IO) {

        PCA_DEBUG("reset by io.\n");
        err = of_property_read_u32(dev->of_node, "io_addr", &reset_cfg->attr.io_attr.io_addr);
        err |=of_property_read_u32(dev->of_node, "mask", &reset_cfg->attr.io_attr.mask);
        err |=of_property_read_u32(dev->of_node, "reset_on", &reset_cfg->attr.io_attr.reset_on);
        err |=of_property_read_u32(dev->of_node, "reset_off", &reset_cfg->attr.io_attr.reset_off);
        if (err) {
            goto dts_config_err;
        }
        PCA_DEBUG("io_addr:0x%x, mask:0x%x, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.io_attr.io_addr, reset_cfg->attr.io_attr.mask,
            reset_cfg->attr.io_attr.reset_on, reset_cfg->attr.io_attr.reset_off);
    } else if (reset_cfg->pca9641_reset_type == PCA9641_RESET_FILE) {

        PCA_DEBUG("reset by file.\n");
        err = of_property_read_string(dev->of_node, "dev_name", &reset_cfg->attr.file_attr.dev_name);
        err |=of_property_read_u32(dev->of_node, "offset", &reset_cfg->attr.file_attr.offset);
        err |=of_property_read_u32(dev->of_node, "mask", &reset_cfg->attr.file_attr.mask);
        err |=of_property_read_u32(dev->of_node, "reset_on", &reset_cfg->attr.file_attr.reset_on);
        err |=of_property_read_u32(dev->of_node, "reset_off", &reset_cfg->attr.file_attr.reset_off);
        if (err) {
            goto dts_config_err;
        }
        PCA_DEBUG("dev_name:%s, mask:0x%x, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.file_attr.dev_name, reset_cfg->attr.file_attr.mask,
            reset_cfg->attr.file_attr.reset_on, reset_cfg->attr.file_attr.reset_off);
    } else {
        PCA_DEBUG_ERR("Unsupport reset type:%d.\n", reset_cfg->pca9641_reset_type);
        goto dts_config_err;
    }
    return 0;
dts_config_err:
    PCA_DEBUG_ERR("dts config error, ret:%d.\n", err);
    return -EINVAL;
}

static int pca9641_reset_data_init(struct pca9541 *data)
{
    pca9641_cfg_info_t *reset_cfg;
    i2c_mux_pca9641_device_t *i2c_mux_pca9641_device;

    if (data->client->dev.platform_data == NULL) {
        PCA_DEBUG("pca9641 has no reset platform data config.\n");
        return 0;
    }
    reset_cfg = &data->pca9641_cfg_info;
    i2c_mux_pca9641_device = data->client->dev.platform_data;
    reset_cfg->pca9641_reset_type = i2c_mux_pca9641_device->pca9641_reset_type;
    if (reset_cfg->pca9641_reset_type == PCA9641_RESET_NONE) {
        PCA_DEBUG("pca9641 has no reset function.\n");
        return 0;
    }

    reset_cfg->rst_delay_b = i2c_mux_pca9641_device->rst_delay_b;
    reset_cfg->rst_delay = i2c_mux_pca9641_device->rst_delay;
    reset_cfg->rst_delay_a = i2c_mux_pca9641_device->rst_delay_a;
    PCA_DEBUG("reset_type:0x%x, rst_delay_b:0x%x, rst_delay:0x%x, rst_delay_a:0x%x.\n",
        reset_cfg->pca9641_reset_type, reset_cfg->rst_delay_b,
        reset_cfg->rst_delay, reset_cfg->rst_delay_a);

    if (reset_cfg->pca9641_reset_type == PCA9641_RESET_I2C) {

        PCA_DEBUG("reset by i2c.\n");
        reset_cfg->attr.i2c_attr.i2c_bus = i2c_mux_pca9641_device->attr.i2c_attr.i2c_bus;
        reset_cfg->attr.i2c_attr.i2c_addr = i2c_mux_pca9641_device->attr.i2c_attr.i2c_addr;
        reset_cfg->attr.i2c_attr.reg_offset = i2c_mux_pca9641_device->attr.i2c_attr.reg_offset;
        reset_cfg->attr.i2c_attr.mask = i2c_mux_pca9641_device->attr.i2c_attr.mask;
        reset_cfg->attr.i2c_attr.reset_on = i2c_mux_pca9641_device->attr.i2c_attr.reset_on;
        reset_cfg->attr.i2c_attr.reset_off = i2c_mux_pca9641_device->attr.i2c_attr.reset_off;
        PCA_DEBUG("bus:%u, addr:0x%x, offset:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
            reset_cfg->attr.i2c_attr.i2c_bus, reset_cfg->attr.i2c_attr.i2c_addr,
            reset_cfg->attr.i2c_attr.reg_offset, reset_cfg->attr.i2c_attr.mask,
            reset_cfg->attr.i2c_attr.reset_on, reset_cfg->attr.i2c_attr.reset_off);
    } else if (reset_cfg->pca9641_reset_type == PCA9641_RESET_GPIO) {

        PCA_DEBUG("reset by gpio.\n");
        reset_cfg->attr.gpio_attr.gpio = i2c_mux_pca9641_device->attr.gpio_attr.gpio;
        reset_cfg->attr.gpio_attr.reset_on = i2c_mux_pca9641_device->attr.gpio_attr.reset_on;
        reset_cfg->attr.gpio_attr.reset_off = i2c_mux_pca9641_device->attr.gpio_attr.reset_off;
        PCA_DEBUG("gpio number:%u, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.gpio_attr.gpio, reset_cfg->attr.gpio_attr.reset_on,
            reset_cfg->attr.gpio_attr.reset_off);
        reset_cfg->attr.gpio_attr.gpio_init = 0;
    } else if (reset_cfg->pca9641_reset_type == PCA9641_RESET_IO) {

        PCA_DEBUG("reset by io.\n");
        reset_cfg->attr.io_attr.io_addr = i2c_mux_pca9641_device->attr.io_attr.io_addr;
        reset_cfg->attr.io_attr.mask = i2c_mux_pca9641_device->attr.io_attr.mask;
        reset_cfg->attr.io_attr.reset_on = i2c_mux_pca9641_device->attr.io_attr.reset_on;
        reset_cfg->attr.io_attr.reset_off = i2c_mux_pca9641_device->attr.io_attr.reset_off;
        PCA_DEBUG("io_addr:0x%x, mask:0x%x, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.io_attr.io_addr, reset_cfg->attr.io_attr.mask,
            reset_cfg->attr.io_attr.reset_on, reset_cfg->attr.io_attr.reset_off);
    } else if (reset_cfg->pca9641_reset_type == PCA9641_RESET_FILE) {

        PCA_DEBUG("reset by file.\n");
        reset_cfg->attr.file_attr.dev_name = i2c_mux_pca9641_device->attr.file_attr.dev_name;
        reset_cfg->attr.file_attr.offset = i2c_mux_pca9641_device->attr.file_attr.offset;
        reset_cfg->attr.file_attr.mask = i2c_mux_pca9641_device->attr.file_attr.mask;
        reset_cfg->attr.file_attr.reset_on = i2c_mux_pca9641_device->attr.file_attr.reset_on;
        reset_cfg->attr.file_attr.reset_off = i2c_mux_pca9641_device->attr.file_attr.reset_off;
        PCA_DEBUG("dev_name:%s, mask:0x%x, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.file_attr.dev_name, reset_cfg->attr.file_attr.mask,
            reset_cfg->attr.file_attr.reset_on, reset_cfg->attr.file_attr.reset_off);
    } else {
        PCA_DEBUG_ERR("Unsupport reset type:%d.\n", reset_cfg->pca9641_reset_type);
        return -EINVAL;
    }
    return 0;
}

/*
 * I2C init/probing/exit functions
 */
static int pca9541_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = client->adapter;
    struct i2c_mux_core *muxc;
    struct pca9541 *data;
    int force;
    int ret = -ENODEV;
    int detect_id;
    i2c_mux_pca9641_device_t *i2c_mux_pca9641_device;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;

    detect_id = pca9641_detect_id(client);

    /*
     * I2C accesses are unprotected here.
     * We have to lock the adapter before releasing the bus.
     */
    if (detect_id == 0) {
        i2c_lock_bus(adap, I2C_LOCK_ROOT_ADAPTER);
        pca9541_release_bus(client);
        i2c_unlock_bus(adap, I2C_LOCK_ROOT_ADAPTER);
    } else {
        i2c_lock_bus(adap, I2C_LOCK_ROOT_ADAPTER);
        pca9641_release_bus(client);
        i2c_unlock_bus(adap, I2C_LOCK_ROOT_ADAPTER);
    }

    if (detect_id == 0) { /* pca9541 */
        muxc = i2c_mux_alloc(adap, &client->dev, 1, sizeof(*data),
                     I2C_MUX_ARBITRATOR,
                     pca9541_select_chan, pca9541_release_chan);
        if (!muxc)
            return -ENOMEM;

        data = i2c_mux_priv(muxc);
        data->client = client;

        i2c_set_clientdata(client, muxc);
        /* Create mux adapter */
        if (of_property_read_u32(client->dev.of_node, "pca9641_nr", &data->pca9641_nr)) {

            force = 0;
            PCA_DEBUG("pca9641_nr not found, use dynamic adap number.\n");
        } else {
            force = data->pca9641_nr;
            PCA_DEBUG("pca9641_nr: %d.\n", force);
        }

        ret = i2c_mux_add_adapter(muxc, force, 0, 0);
        if (ret)
            return ret;
    } else {
        muxc = i2c_mux_alloc(adap, &client->dev, 1, sizeof(*data), I2C_MUX_ARBITRATOR,
                   pca9641_select_chan, pca9641_release_chan);
        if (!muxc) {
            dev_err(&client->dev, "i2c_mux_alloc failed, out of memory.\n");
            return -ENOMEM;
        }

        data = i2c_mux_priv(muxc);
        data->client = client;

        i2c_set_clientdata(client, muxc);

        if (client->dev.of_node) {
            ret= of_pca9641_reset_data_init(data);
        } else {
            ret= pca9641_reset_data_init(data);
        }
        if (ret < 0) {
            dev_err(&client->dev, "pca9641 reset config err, ret:%d.\n", ret);
            return ret;
        }

        if (client->dev.of_node == NULL) {
            if (client->dev.platform_data == NULL) {
                force = 0;
                PCA_DEBUG("platform data is NULL, use dynamic adap number.\n");
            } else {
                i2c_mux_pca9641_device = client->dev.platform_data;
                data->pca9641_nr = i2c_mux_pca9641_device->pca9641_nr;
                if (data->pca9641_nr == 0) {
                    force = 0;
                    PCA_DEBUG("pca9641_nr = 0, use dynamic adap number.\n");
                } else {
                    force = data->pca9641_nr;
                    PCA_DEBUG("pca9641_nr: %d.\n", force);
                }
            }
        } else {
            /* Create mux adapter */
            if (of_property_read_u32(client->dev.of_node, "pca9641_nr", &data->pca9641_nr)) {

                force = 0;
                PCA_DEBUG("pca9641_nr not found, use dynamic adap number.\n");
            } else {
                force = data->pca9641_nr;
                PCA_DEBUG("pca9641_nr: %d.\n", force);
            }
        }

        ret = i2c_mux_add_adapter(muxc, force, 0, 0);
        if (ret) {
            dev_err(&client->dev, "Failed to register master selector.\n");
            return ret;
        }
    }
    pca9641_recordflag(muxc->adapter[0]);

    dev_info(&client->dev, "registered master selector for I2C %s\n", client->name);

    return 0;
}

static int pca9541_remove(struct i2c_client *client)
{
	struct i2c_mux_core *muxc = i2c_get_clientdata(client);

	i2c_mux_del_adapters(muxc);
	return 0;
}

static struct i2c_driver pca9641_driver = {
	.driver = {
		   .name = "wb_pca9641",
           .of_match_table = of_match_ptr(pca9541_of_match),
		   },
	.probe = pca9541_probe,
	.remove = pca9541_remove,
	.id_table = pca9541_id,
};

module_i2c_driver(pca9641_driver);
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("PCA9541 I2C master selector driver");
MODULE_LICENSE("GPL v2");
