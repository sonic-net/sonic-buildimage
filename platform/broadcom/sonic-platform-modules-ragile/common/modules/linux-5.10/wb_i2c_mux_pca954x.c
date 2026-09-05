/*
 * I2C multiplexer
 *
 * Copyright (c) 2008-2009 Rodolfo Giometti <giometti@linux.it>
 * Copyright (c) 2008-2009 Eurotech S.p.A. <info@eurotech.it>
 *
 * This module supports the PCA954x series of I2C multiplexer/switch chips
 * made by Philips Semiconductors.
 * This includes the:
 *     PCA9540, PCA9542, PCA9543, PCA9544, PCA9545, PCA9546, PCA9547
 *     and PCA9548.
 *
 * These chips are all controlled via the I2C bus itself, and all have a
 * single 8-bit register. The upstream "parent" bus fans out to two,
 * four, or eight downstream busses or channels; which of these
 * are selected is determined by the chip type and register contents. A
 * mux can select only one sub-bus at a time; a switch can select any
 * combination simultaneously.
 *
 * Based on:
 *    pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *    pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *    i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *    pca9540.c from Jean Delvare <jdelvare@suse.de>.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/version.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c-smbus.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "wb_i2c_mux_pca954x.h"

#define PCA954X_MAX_NCHANS 8
#define PCA954X_IRQ_OFFSET 4

#define I2C_RETRY_TIMES         5
#define I2C_RETRY_WAIT_TIMES    10      /*delay 10ms*/

typedef struct pca9548_cfg_info_s {
    uint32_t pca9548_base_nr;
    uint32_t pca9548_reset_type;
    uint32_t rst_delay_b; /* delay time before reset(us) */
    uint32_t rst_delay;   /* reset time(us) */
    uint32_t rst_delay_a; /* delay time after reset(us) */
    union {
        i2c_attr_t i2c_attr;
        gpio_attr_t gpio_attr;
        io_attr_t io_attr;
        file_attr_t file_attr;
    } attr;
    bool select_chan_check;
    bool close_chan_force_reset;
} pca9548_cfg_info_t;

int g_pca954x_debug = 0;
int g_pca954x_error = 0;

module_param(g_pca954x_debug, int, S_IRUGO | S_IWUSR);
module_param(g_pca954x_error, int, S_IRUGO | S_IWUSR);

#define PCA954X_DEBUG(fmt, args...) do {                                        \
    if (g_pca954x_debug) { \
        printk(KERN_INFO "[PCA95x][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PCA954X_ERROR(fmt, args...) do {                                        \
    if (g_pca954x_error) { \
        printk(KERN_ERR "[PCA95x][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

extern int pca9641_setmuxflag(int nr, int flag);
enum pca_type {
    pca_9540,
    pca_9542,
    pca_9543,
    pca_9544,
    pca_9545,
    pca_9546,
    pca_9547,
    pca_9548,
};

struct chip_desc {
    u8 nchans;
    u8 enable;    /* used for muxes only */
    u8 has_irq;
    enum muxtype {
        pca954x_ismux = 0,
        pca954x_isswi
    } muxtype;
};

struct pca954x {
    const struct chip_desc *chip;
    u8 last_chan;        /* last register value */
    u8 deselect;
    struct i2c_client *client;
    struct irq_domain *irq;
    unsigned int irq_mask;
    raw_spinlock_t lock;
    pca9548_cfg_info_t pca9548_cfg_info; /* pca9548 reset cfg */
};

/* Provide specs for the PCA954x types we know about */
static const struct chip_desc chips[] = {
    [pca_9540] = {
        .nchans = 2,
        .enable = 0x4,
        .muxtype = pca954x_ismux,
    },
    [pca_9542] = {
        .nchans = 2,
        .enable = 0x4,
        .has_irq = 1,
        .muxtype = pca954x_ismux,
    },
    [pca_9543] = {
        .nchans = 2,
        .has_irq = 1,
        .muxtype = pca954x_isswi,
    },
    [pca_9544] = {
        .nchans = 4,
        .enable = 0x4,
        .has_irq = 1,
        .muxtype = pca954x_ismux,
    },
    [pca_9545] = {
        .nchans = 4,
        .has_irq = 1,
        .muxtype = pca954x_isswi,
    },
    [pca_9546] = {
        .nchans = 4,
        .muxtype = pca954x_isswi,
    },
    [pca_9547] = {
        .nchans = 8,
        .enable = 0x8,
        .muxtype = pca954x_ismux,
    },
    [pca_9548] = {
        .nchans = 8,
        .muxtype = pca954x_isswi,
    },
};

static const struct i2c_device_id pca954x_id[] = {
    { "wb_pca9540", pca_9540 },
    { "wb_pca9542", pca_9542 },
    { "wb_pca9543", pca_9543 },
    { "wb_pca9544", pca_9544 },
    { "wb_pca9545", pca_9545 },
    { "wb_pca9546", pca_9546 },
    { "wb_pca9547", pca_9547 },
    { "wb_pca9548", pca_9548 },
    { }
};
MODULE_DEVICE_TABLE(i2c, pca954x_id);

#ifdef CONFIG_OF
static const struct of_device_id pca954x_of_match[] = {
    { .compatible = "nxp,wb_pca9540", .data = &chips[pca_9540] },
    { .compatible = "nxp,wb_pca9542", .data = &chips[pca_9542] },
    { .compatible = "nxp,wb_pca9543", .data = &chips[pca_9543] },
    { .compatible = "nxp,wb_pca9544", .data = &chips[pca_9544] },
    { .compatible = "nxp,wb_pca9545", .data = &chips[pca_9545] },
    { .compatible = "nxp,wb_pca9546", .data = &chips[pca_9546] },
    { .compatible = "nxp,wb_pca9547", .data = &chips[pca_9547] },
    { .compatible = "nxp,wb_pca9548", .data = &chips[pca_9548] },
    {}
};
MODULE_DEVICE_TABLE(of, pca954x_of_match);
#endif

/* Write to mux register. Don't use i2c_transfer()/i2c_smbus_xfer()
   for this as they will try to lock adapter a second time */
static int pca954x_reg_write(struct i2c_adapter *adap,
                 struct i2c_client *client, u8 val)
{
    int ret = -ENODEV;

    if (adap->algo->master_xfer) {
        struct i2c_msg msg;
        char buf[1];

        msg.addr = client->addr;
        msg.flags = 0;
        msg.len = 1;
        buf[0] = val;
        msg.buf = buf;
        ret = __i2c_transfer(adap, &msg, 1);

        if (ret >= 0 && ret != 1)
            ret = -EREMOTEIO;
    } else {
        union i2c_smbus_data data;
        ret = adap->algo->smbus_xfer(adap, client->addr,
                         client->flags,
                         I2C_SMBUS_WRITE,
                         val, I2C_SMBUS_BYTE, &data);
    }
    return ret;
}

 static int pca954x_reg_read(struct i2c_adapter *adap,
                  struct i2c_client *client, u8 *val)
 {
     int ret = -ENODEV;
     u8 tmp_val;

     if (adap->algo->master_xfer) {
         struct i2c_msg msg;

         msg.addr = client->addr;
         msg.flags = I2C_M_RD;
         msg.len = 1;
         msg.buf = &tmp_val;
         ret = __i2c_transfer(adap, &msg, 1);

         if (ret >= 0 && ret != 1)
             ret = -EREMOTEIO;
     } else {
         union i2c_smbus_data data;
         ret = adap->algo->smbus_xfer(adap, client->addr,
                          client->flags,
                          I2C_SMBUS_READ,
                          0, I2C_SMBUS_BYTE, &data);

         if (!ret) {
             tmp_val = data.byte;
         }
     }

     *val = tmp_val;
     return ret;
 }

static int pca954x_setmuxflag(struct i2c_client *client, int flag)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);

    pca9641_setmuxflag(adap->nr, flag);
    return 0;
}

static int pca9548_gpio_init(gpio_attr_t *gpio_attr)
{
    int err;

    if (gpio_attr->gpio_init) {
        PCA954X_DEBUG("gpio%d already init, do nothing.\n", gpio_attr->gpio);
        return 0;
    }

    PCA954X_DEBUG("gpio%d init.\n", gpio_attr->gpio);
    err = gpio_request(gpio_attr->gpio, "pca9548_reset");
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
    PCA954X_ERROR("pca9548_gpio_init failed, ret:%d.\n", err);
    return err;
}

static void pca9548_gpio_free(gpio_attr_t *gpio_attr)
{
    if (gpio_attr->gpio_init == 1) {
        PCA954X_DEBUG("gpio%d release.\n", gpio_attr->gpio);
        gpio_free(gpio_attr->gpio);
        gpio_attr->gpio_init = 0;
    }
}

static int pca954x_reset_file_read(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        PCA954X_ERROR("read open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_read(filp, val, size, &tmp_pos);
    if (ret < 0) {
        PCA954X_ERROR("kernel_read failed, path=%s, addr=0x%x, size=%ld, ret=%d\r\n", path, pos, size, ret);
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

static int pca954x_reset_file_write(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        PCA954X_ERROR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_write(filp, val, size, &tmp_pos);
    if (ret < 0) {
        PCA954X_ERROR("kernel_write failed, path=%s, addr=0x%x, size=%ld, ret=%d\r\n", path, pos, size, ret);
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

static int pca954x_reset_i2c_read(uint32_t bus, uint32_t addr, uint32_t offset_addr,
            unsigned char *buf, uint32_t size)
{
    struct file *fp;
    struct i2c_client client;
    char i2c_path[32];
    int i ,j ;
    int rv;

    rv = 0;
    mem_clear(i2c_path, sizeof(i2c_path));
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", bus);
    fp = filp_open(i2c_path, O_RDWR, S_IRUSR | S_IWUSR);
    if (IS_ERR(fp)) {
        PCA954X_ERROR("i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = addr;
    for (j = 0 ;j < size ;j++) {
        for (i = 0; i < I2C_RETRY_TIMES; i++) {
            rv = i2c_smbus_read_byte_data(&client, (offset_addr + j));
            if (rv < 0) {
                PCA954X_ERROR("i2c read failed, try again.\n");
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

static int pca954x_reset_i2c_write(uint32_t bus, uint32_t dev_addr, uint32_t offset_addr,
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
        PCA954X_ERROR("i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = dev_addr;
    for (i = 0; i < I2C_RETRY_TIMES; i++) {
        rv = i2c_smbus_write_byte_data(&client, offset_addr, write_buf);
        if (rv < 0) {
            PCA954X_ERROR("i2c write failed, try again.\n");
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

static void pca954x_close_chan_finally(struct i2c_mux_core * muxc)
{
    struct pca954x *data;
    struct i2c_adapter *adapter;
    struct i2c_client *client;
    int adapter_timeout;

    data = i2c_mux_priv(muxc);
    client = data->client;
    adapter = muxc->parent;
    /* get bus info */
    while (i2c_parent_is_i2c_adapter(adapter)) {
        adapter = to_i2c_adapter(adapter->dev.parent);
    }
    adapter_timeout = adapter->timeout;
    adapter->timeout = msecs_to_jiffies(50);
    pca954x_reg_write(muxc->parent, client, data->last_chan);
    adapter->timeout = adapter_timeout;

    return;
}

static int pca954x_do_file_reset(struct i2c_mux_core *muxc)
{
    int ret, timeout, err;
    struct pca954x *data;
    struct i2c_client *client;
    pca9548_cfg_info_t *reset_cfg;
    file_attr_t *file_attr;
    u8 val;

    data = i2c_mux_priv(muxc);
    client = data->client;
    reset_cfg = &data->pca9548_cfg_info;
    file_attr = &reset_cfg->attr.file_attr;
    ret = -1;

    PCA954X_DEBUG("rst_delay_b:%u, rst_delay:%u, rst_delay_a:%u.\n",
        reset_cfg->rst_delay_b, reset_cfg->rst_delay, reset_cfg->rst_delay_a);
    PCA954X_DEBUG("dev_name:%s, offset:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
        file_attr->dev_name, file_attr->offset, file_attr->mask,
        file_attr->reset_on, file_attr->reset_off);

    if (reset_cfg->rst_delay_b) {
        udelay(reset_cfg->rst_delay_b);
    }

    err = pca954x_reset_file_read(file_attr->dev_name, file_attr->offset, &val, sizeof(val));
    if (err < 0) {
        goto out;
    }
    val &= ~(file_attr->mask);
    val |= file_attr->reset_on;
    err = pca954x_reset_file_write(file_attr->dev_name, file_attr->offset, &val, sizeof(val));
    if (err < 0) {
        goto out;
    }

    if (reset_cfg->rst_delay) {
        udelay(reset_cfg->rst_delay);
    }

    val &= ~(file_attr->mask);
    val |= file_attr->reset_off;
    err = pca954x_reset_file_write(file_attr->dev_name, file_attr->offset, &val, sizeof(val));
    if (err < 0) {
        goto out;
    }

    timeout = reset_cfg->rst_delay_a;
    while (timeout > 0) {
        udelay(1);
        err = pca954x_reset_file_read(file_attr->dev_name, file_attr->offset, &val, sizeof(val));
        if (err < 0) {
            goto out;
        }
        val &= (file_attr->mask);
        if (val == file_attr->reset_off) {
            ret = 0;
            pca954x_close_chan_finally(muxc);
            PCA954X_DEBUG("pca954x_do_file_reset success.\n");
            break;
        }
        if (timeout >= 1000 && (timeout % 1000 == 0)) {
            schedule();
        }
        timeout--;
    }
    if (ret < 0) {
        PCA954X_ERROR("pca954x_do_file_reset timeout.\n");
    }
out:
    if (err < 0) {
        PCA954X_ERROR("pca954x_do_file_reset file rd/wr failed, ret:%d.\n", err);
    }

    return ret;
}

static int pca954x_do_io_reset(struct i2c_mux_core *muxc)
{
    int ret, timeout;
    struct pca954x *data;
    struct i2c_client *client;
    pca9548_cfg_info_t *reset_cfg;
    io_attr_t *io_attr;
    u8 val;

    data = i2c_mux_priv(muxc);
    client = data->client;
    reset_cfg = &data->pca9548_cfg_info;
    io_attr = &reset_cfg->attr.io_attr;

    PCA954X_DEBUG("rst_delay_b:%u, rst_delay:%u, rst_delay_a:%u.\n",
        reset_cfg->rst_delay_b, reset_cfg->rst_delay, reset_cfg->rst_delay_a);
    PCA954X_DEBUG("io_addr:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
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
            pca954x_close_chan_finally(muxc);
            PCA954X_DEBUG("pca954x_do_io_reset success.\n");
            break;
        }
        if (timeout >= 1000 && (timeout % 1000 == 0)) {
            schedule();
        }
        timeout--;
    }

    if (ret < 0) {
        PCA954X_ERROR("pca954x_do_io_reset timeout.\n");
    }

    return ret;
}

static int pca954x_do_gpio_reset(struct i2c_mux_core *muxc)
{
    int ret, timeout;
    struct pca954x *data;
    struct i2c_client *client;
    pca9548_cfg_info_t *reset_cfg;
    gpio_attr_t *gpio_attr;
    u8 val;

    data = i2c_mux_priv(muxc);
    client = data->client;
    reset_cfg = &data->pca9548_cfg_info;
    gpio_attr = &reset_cfg->attr.gpio_attr;

    ret = pca9548_gpio_init(gpio_attr);
    if (ret) {
        return -1;
    }

    if (reset_cfg->rst_delay_b) {
        udelay(reset_cfg->rst_delay_b);
    }

    /* reset on */
    __gpio_set_value(gpio_attr->gpio, gpio_attr->reset_on);

    if (reset_cfg->rst_delay) {
        udelay(reset_cfg->rst_delay);
    }

    /* reset off */
    __gpio_set_value(gpio_attr->gpio, gpio_attr->reset_off);
    ret = -1;
    timeout = reset_cfg->rst_delay_a;
    while (timeout > 0) {
        udelay(1);
        val = __gpio_get_value(gpio_attr->gpio);
        if (val == gpio_attr->reset_off) {
            ret = 0;
            pca954x_close_chan_finally(muxc);
            PCA954X_DEBUG("pca954x_do_gpio_reset success.\n");
            break;
        }
        if (timeout >= 1000 && (timeout % 1000 == 0)) {
            /* 1MS schedule*/
            schedule();
        }
        timeout--;
    }

    if (ret < 0) {
        PCA954X_ERROR("pca954x_do_gpio_reset timeout.\n");
    }

    pca9548_gpio_free(gpio_attr);
    return ret;
}

static int pca954x_do_i2c_reset(struct i2c_mux_core *muxc)
{
    int ret, timeout, err;
    struct pca954x *data;
    struct i2c_client *client;
    pca9548_cfg_info_t *reset_cfg;
    i2c_attr_t *i2c_attr;
    u8 val;

    data = i2c_mux_priv(muxc);
    client = data->client;
    reset_cfg = &data->pca9548_cfg_info;
    i2c_attr = &reset_cfg->attr.i2c_attr;
    ret = -1;

    PCA954X_DEBUG("rst_delay_b:%u, rst_delay:%u, rst_delay_a:%u.\n",
        reset_cfg->rst_delay_b, reset_cfg->rst_delay, reset_cfg->rst_delay_a);
    PCA954X_DEBUG("bus:0x%x, addr:0x%x, reg:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
        i2c_attr->i2c_bus, i2c_attr->i2c_addr, i2c_attr->reg_offset,
        i2c_attr->mask, i2c_attr->reset_on, i2c_attr->reset_off);

    if (reset_cfg->rst_delay_b) {
        udelay(reset_cfg->rst_delay_b);
    }

    err = pca954x_reset_i2c_read(i2c_attr->i2c_bus, i2c_attr->i2c_addr,
              i2c_attr->reg_offset, &val, sizeof(val));
    if (err < 0) {
        goto out;
    }
    val &= ~(i2c_attr->mask);
    val |= i2c_attr->reset_on;
    err = pca954x_reset_i2c_write(i2c_attr->i2c_bus, i2c_attr->i2c_addr,
              i2c_attr->reg_offset, val);
    if (err < 0) {
        goto out;
    }

    if (reset_cfg->rst_delay) {
        udelay(reset_cfg->rst_delay);
    }

    val &= ~(i2c_attr->mask);
    val |= i2c_attr->reset_off;
    err = pca954x_reset_i2c_write(i2c_attr->i2c_bus, i2c_attr->i2c_addr,
              i2c_attr->reg_offset, val);
    if (err < 0) {
        goto out;
    }

    timeout = reset_cfg->rst_delay_a;
    while (timeout > 0) {
        udelay(1);
        err = pca954x_reset_i2c_read(i2c_attr->i2c_bus, i2c_attr->i2c_addr,
                  i2c_attr->reg_offset, &val, sizeof(val));
        if (err < 0) {
            goto out;
        }
        val &= (i2c_attr->mask);
        if (val == i2c_attr->reset_off) {
            ret = 0;
            pca954x_close_chan_finally(muxc);
            PCA954X_DEBUG("pca954x_do_i2c_reset success.\n");
            break;
        }
        if (timeout >= 1000 && (timeout % 1000 == 0)) {
            schedule();
        }
        timeout--;
    }
    if (ret < 0) {
        PCA954X_ERROR("pca954x_do_i2c_reset timeout.\n");
    }
out:
    if (err < 0) {
        PCA954X_ERROR("pca954x_do_i2c_reset i2c op failed, ret:%d.\n", err);
    }
    return ret;
}

static int pca954x_do_reset(struct i2c_mux_core *muxc)
{
    int ret;
    struct pca954x *data;

    data = i2c_mux_priv(muxc);
    if (data->pca9548_cfg_info.pca9548_reset_type == PCA9548_RESET_NONE) {
        ret = -1;
        PCA954X_DEBUG("Don't need to reset.\n");
    } else if (data->pca9548_cfg_info.pca9548_reset_type == PCA9548_RESET_I2C) {
        ret = pca954x_do_i2c_reset(muxc);
    } else if (data->pca9548_cfg_info.pca9548_reset_type == PCA9548_RESET_GPIO) {
        ret = pca954x_do_gpio_reset(muxc);
    } else if (data->pca9548_cfg_info.pca9548_reset_type == PCA9548_RESET_IO) {
        ret = pca954x_do_io_reset(muxc);
    } else if (data->pca9548_cfg_info.pca9548_reset_type == PCA9548_RESET_FILE) {
        ret = pca954x_do_file_reset(muxc);
    } else {
        ret = -1;
        PCA954X_ERROR("Unsupport reset type:0x%x.\n",
            data->pca9548_cfg_info.pca9548_reset_type);
    }

    if (ret < 0) {
        PCA954X_ERROR("pca9548_reset_ctrl failed, reset type:%u, ret:%d.\n",
            data->pca9548_cfg_info.pca9548_reset_type, ret);
    }
    return ret;
}

static int pca954x_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
    struct pca954x *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    const struct chip_desc *chip = data->chip;
    u8 regval;
    int ret = 0;
    u8 read_val = 0;
    int rv;

    /* we make switches look like muxes, not sure how to be smarter */
    if (chip->muxtype == pca954x_ismux)
        regval = chan | chip->enable;
    else
        regval = 1 << chan;

    /* Only select the channel if its different from the last channel */
    if (data->last_chan != regval) {
        pca954x_setmuxflag(client, 0);
        ret = pca954x_reg_write(muxc->parent, client, regval);
        data->last_chan = ret < 0 ? 0 : regval;
    }

    if (data->pca9548_cfg_info.select_chan_check) { /* check chan */
        ret = pca954x_reg_read(muxc->parent, client, &read_val);
        /* read failed or chan not open, reset pca9548 */
        if ((ret < 0) || (read_val != data->last_chan)) {
            dev_warn(&client->dev, "pca954x open channle %u failed, do reset.\n", chan);
            PCA954X_DEBUG("ret = %d, read_val = %d, last_chan = %d.\n", ret, read_val, data->last_chan);
            rv = pca954x_do_reset(muxc);
            if (rv >= 0) {
                PCA954X_DEBUG("pca954x_do_reset success, rv = %d.\n", rv);
            } else {
                PCA954X_DEBUG("pca954x_do_reset failed, rv = %d.\n", rv);
            }
            if (ret >= 0) {
                ret = -EIO; /* chan not match, return IO error */
            }
        }
    }

    return ret;
}

static int pca954x_deselect_mux(struct i2c_mux_core *muxc, u32 chan)
{
    struct pca954x *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    int ret, rv;

    /* Deselect active channel */
    data->last_chan = 0;
    if (data->pca9548_cfg_info.close_chan_force_reset) {
        ret = pca954x_do_reset(muxc);
    } else {
        ret = pca954x_reg_write(muxc->parent, client, data->last_chan);
        if (ret < 0 ) {

            dev_warn(&client->dev, "pca954x close channel %u failed, do reset.\n", chan);
            rv = pca954x_do_reset(muxc);
            if (rv == 0) {
                ret = 0;
            }
        }
    }

    pca954x_setmuxflag(client, 1);
    (void)pca954x_reg_write(muxc->parent, client, data->last_chan);

    return ret;

}

static irqreturn_t pca954x_irq_handler(int irq, void *dev_id)
{
    struct pca954x *data = dev_id;
    unsigned int child_irq;
    int ret, i, handled = 0;

    ret = i2c_smbus_read_byte(data->client);
    if (ret < 0)
        return IRQ_NONE;

    for (i = 0; i < data->chip->nchans; i++) {
        if (ret & BIT(PCA954X_IRQ_OFFSET + i)) {
            child_irq = irq_linear_revmap(data->irq, i);
            handle_nested_irq(child_irq);
            handled++;
        }
    }
    return handled ? IRQ_HANDLED : IRQ_NONE;
}

static void pca954x_irq_mask(struct irq_data *idata)
{
    struct pca954x *data = irq_data_get_irq_chip_data(idata);
    unsigned int pos = idata->hwirq;
    unsigned long flags;

    raw_spin_lock_irqsave(&data->lock, flags);

    data->irq_mask &= ~BIT(pos);
    if (!data->irq_mask)
        disable_irq(data->client->irq);

    raw_spin_unlock_irqrestore(&data->lock, flags);
}

static void pca954x_irq_unmask(struct irq_data *idata)
{
    struct pca954x *data = irq_data_get_irq_chip_data(idata);
    unsigned int pos = idata->hwirq;
    unsigned long flags;

    raw_spin_lock_irqsave(&data->lock, flags);

    if (!data->irq_mask)
        enable_irq(data->client->irq);
    data->irq_mask |= BIT(pos);

    raw_spin_unlock_irqrestore(&data->lock, flags);
}

static int pca954x_irq_set_type(struct irq_data *idata, unsigned int type)
{
    if ((type & IRQ_TYPE_SENSE_MASK) != IRQ_TYPE_LEVEL_LOW)
        return -EINVAL;
    return 0;
}

static struct irq_chip pca954x_irq_chip = {
    .name = "i2c-mux-pca954x",
    .irq_mask = pca954x_irq_mask,
    .irq_unmask = pca954x_irq_unmask,
    .irq_set_type = pca954x_irq_set_type,
};

static int of_pca954x_irq_setup(struct i2c_mux_core *muxc)
{
    struct pca954x *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    int c, err, irq;

    if (!data->chip->has_irq || client->irq <= 0)
        return 0;

    raw_spin_lock_init(&data->lock);

    data->irq = irq_domain_add_linear(client->dev.of_node,
                      data->chip->nchans,
                      &irq_domain_simple_ops, data);
    if (!data->irq)
        return -ENODEV;

    for (c = 0; c < data->chip->nchans; c++) {
        irq = irq_create_mapping(data->irq, c);
        irq_set_chip_data(irq, data);
        irq_set_chip_and_handler(irq, &pca954x_irq_chip,
            handle_simple_irq);
    }

    err = devm_request_threaded_irq(&client->dev, data->client->irq, NULL,
                    pca954x_irq_handler,
                    IRQF_ONESHOT | IRQF_SHARED,
                    "pca954x", data);
    if (err)
        goto err_req_irq;

    disable_irq(data->client->irq);

    return 0;
err_req_irq:
    for (c = 0; c < data->chip->nchans; c++) {
        irq = irq_find_mapping(data->irq, c);
        irq_dispose_mapping(irq);
    }
    irq_domain_remove(data->irq);

    return err;
}

static int pca954x_irq_setup(struct i2c_mux_core *muxc)
{
    return 0;
}

static int of_pca954x_reset_data_init(struct pca954x *data)
{
    int err;
    struct device *dev = &data->client->dev;
    pca9548_cfg_info_t *reset_cfg;

    reset_cfg = &data->pca9548_cfg_info;
    if (dev == NULL || dev->of_node == NULL) {
        PCA954X_DEBUG("dev or dev->of_node is NUll, no reset.\n");
        reset_cfg->pca9548_reset_type = PCA9548_RESET_NONE;
        return 0;
    }

    reset_cfg->select_chan_check = of_property_read_bool(dev->of_node, "select_chan_check");
    reset_cfg->close_chan_force_reset = of_property_read_bool(dev->of_node, "close_chan_force_reset");
    PCA954X_DEBUG("select_chan_check:%d, close_chan_force_reset:%d.\n", reset_cfg->select_chan_check,
        reset_cfg->close_chan_force_reset);

    if (of_property_read_u32(dev->of_node, "pca9548_reset_type", &reset_cfg->pca9548_reset_type)) {

        PCA954X_DEBUG("pca9548_reset_type not found, no reset.\n");
        reset_cfg->pca9548_reset_type = PCA9548_RESET_NONE;
        return 0;
    }
    err = of_property_read_u32(dev->of_node, "rst_delay_b", &reset_cfg->rst_delay_b);
    err |= of_property_read_u32(dev->of_node, "rst_delay", &reset_cfg->rst_delay);
    err |= of_property_read_u32(dev->of_node, "rst_delay_a", &reset_cfg->rst_delay_a);

    if (err) {
        goto dts_config_err;
    }
    PCA954X_DEBUG("reset_type:0x%x, rst_delay_b:0x%x, rst_delay:0x%x, rst_delay_a:0x%x.\n",
        reset_cfg->pca9548_reset_type, reset_cfg->rst_delay_b,
        reset_cfg->rst_delay, reset_cfg->rst_delay_a);

    if (reset_cfg->pca9548_reset_type == PCA9548_RESET_I2C) {

        PCA954X_DEBUG("reset by i2c.\n");
        err = of_property_read_u32(dev->of_node, "i2c_bus", &reset_cfg->attr.i2c_attr.i2c_bus);
        err |=of_property_read_u32(dev->of_node, "i2c_addr", &reset_cfg->attr.i2c_attr.i2c_addr);
        err |=of_property_read_u32(dev->of_node, "reg_offset", &reset_cfg->attr.i2c_attr.reg_offset);
        err |=of_property_read_u32(dev->of_node, "mask", &reset_cfg->attr.i2c_attr.mask);
        err |=of_property_read_u32(dev->of_node, "reset_on", &reset_cfg->attr.i2c_attr.reset_on);
        err |=of_property_read_u32(dev->of_node, "reset_off", &reset_cfg->attr.i2c_attr.reset_off);
        if (err) {
            goto dts_config_err;
        }
        PCA954X_DEBUG("bus:%u, addr:0x%x, offset:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
            reset_cfg->attr.i2c_attr.i2c_bus, reset_cfg->attr.i2c_attr.i2c_addr,
            reset_cfg->attr.i2c_attr.reg_offset, reset_cfg->attr.i2c_attr.mask,
            reset_cfg->attr.i2c_attr.reset_on, reset_cfg->attr.i2c_attr.reset_off);
    } else if (reset_cfg->pca9548_reset_type == PCA9548_RESET_GPIO) {

        PCA954X_DEBUG("reset by gpio.\n");
        err = of_property_read_u32(dev->of_node, "gpio", &reset_cfg->attr.gpio_attr.gpio);
        err |=of_property_read_u32(dev->of_node, "reset_on", &reset_cfg->attr.gpio_attr.reset_on);
        err |=of_property_read_u32(dev->of_node, "reset_off", &reset_cfg->attr.gpio_attr.reset_off);
        if (err) {
            goto dts_config_err;
        }
        PCA954X_DEBUG("gpio number:%u, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.gpio_attr.gpio, reset_cfg->attr.gpio_attr.reset_on,
            reset_cfg->attr.gpio_attr.reset_off);
        reset_cfg->attr.gpio_attr.gpio_init = 0;
    } else if (reset_cfg->pca9548_reset_type == PCA9548_RESET_IO) {

        PCA954X_DEBUG("reset by io.\n");
        err = of_property_read_u32(dev->of_node, "io_addr", &reset_cfg->attr.io_attr.io_addr);
        err |=of_property_read_u32(dev->of_node, "mask", &reset_cfg->attr.io_attr.mask);
        err |=of_property_read_u32(dev->of_node, "reset_on", &reset_cfg->attr.io_attr.reset_on);
        err |=of_property_read_u32(dev->of_node, "reset_off", &reset_cfg->attr.io_attr.reset_off);
        if (err) {
            goto dts_config_err;
        }
        PCA954X_DEBUG("io_addr:0x%x, mask:0x%x, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.io_attr.io_addr, reset_cfg->attr.io_attr.mask,
            reset_cfg->attr.io_attr.reset_on, reset_cfg->attr.io_attr.reset_off);
    } else if (reset_cfg->pca9548_reset_type == PCA9548_RESET_FILE) {

        PCA954X_DEBUG("reset by file.\n");
        err = of_property_read_string(dev->of_node, "dev_name", &reset_cfg->attr.file_attr.dev_name);
        err |=of_property_read_u32(dev->of_node, "offset", &reset_cfg->attr.file_attr.offset);
        err |=of_property_read_u32(dev->of_node, "mask", &reset_cfg->attr.file_attr.mask);
        err |=of_property_read_u32(dev->of_node, "reset_on", &reset_cfg->attr.file_attr.reset_on);
        err |=of_property_read_u32(dev->of_node, "reset_off", &reset_cfg->attr.file_attr.reset_off);
        if (err) {
            goto dts_config_err;
        }
        PCA954X_DEBUG("dev_name:%s, mask:0x%x, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.file_attr.dev_name, reset_cfg->attr.file_attr.mask,
            reset_cfg->attr.file_attr.reset_on, reset_cfg->attr.file_attr.reset_off);
    } else {
        PCA954X_ERROR("Unsupport reset type:%d.\n", reset_cfg->pca9548_reset_type);
        goto dts_config_err;
    }
    return 0;
dts_config_err:
    PCA954X_ERROR("dts config error, ret:%d.\n", err);
    return -EINVAL;
}

static int pca954x_reset_data_init(struct pca954x *data)
{
    pca9548_cfg_info_t *reset_cfg;
    i2c_mux_pca954x_device_t *i2c_mux_pca954x_device;

    if (data->client->dev.platform_data == NULL) {
        PCA954X_DEBUG("pca954x has no reset platform data config.\n");
        return 0;
    }
    reset_cfg = &data->pca9548_cfg_info;
    i2c_mux_pca954x_device = data->client->dev.platform_data;
    reset_cfg->select_chan_check = i2c_mux_pca954x_device->select_chan_check;
    reset_cfg->close_chan_force_reset = i2c_mux_pca954x_device->close_chan_force_reset;
    PCA954X_DEBUG("select_chan_check:%d, close_chan_force_reset:%d.\n", reset_cfg->select_chan_check,
        reset_cfg->close_chan_force_reset);

    reset_cfg->pca9548_reset_type = i2c_mux_pca954x_device->pca9548_reset_type;
    if (reset_cfg->pca9548_reset_type == PCA9548_RESET_NONE) {
        PCA954X_DEBUG("pca9548_reset_type not found, no reset.\n");
        return 0;
    }

    reset_cfg->rst_delay_b = i2c_mux_pca954x_device->rst_delay_b;
    reset_cfg->rst_delay = i2c_mux_pca954x_device->rst_delay;
    reset_cfg->rst_delay_a = i2c_mux_pca954x_device->rst_delay_a;
    PCA954X_DEBUG("reset_type:0x%x, rst_delay_b:0x%x, rst_delay:0x%x, rst_delay_a:0x%x.\n",
        reset_cfg->pca9548_reset_type, reset_cfg->rst_delay_b,
        reset_cfg->rst_delay, reset_cfg->rst_delay_a);

    if (reset_cfg->pca9548_reset_type == PCA9548_RESET_I2C) {

        PCA954X_DEBUG("reset by i2c.\n");
        reset_cfg->attr.i2c_attr.i2c_bus = i2c_mux_pca954x_device->attr.i2c_attr.i2c_bus;
        reset_cfg->attr.i2c_attr.i2c_addr = i2c_mux_pca954x_device->attr.i2c_attr.i2c_addr;
        reset_cfg->attr.i2c_attr.reg_offset = i2c_mux_pca954x_device->attr.i2c_attr.reg_offset;
        reset_cfg->attr.i2c_attr.mask = i2c_mux_pca954x_device->attr.i2c_attr.mask;
        reset_cfg->attr.i2c_attr.reset_on = i2c_mux_pca954x_device->attr.i2c_attr.reset_on;
        reset_cfg->attr.i2c_attr.reset_off = i2c_mux_pca954x_device->attr.i2c_attr.reset_off;
        PCA954X_DEBUG("bus:%u, addr:0x%x, offset:0x%x, mask:0x%x, on:0x%x, off:0x%x.\n",
            reset_cfg->attr.i2c_attr.i2c_bus, reset_cfg->attr.i2c_attr.i2c_addr,
            reset_cfg->attr.i2c_attr.reg_offset, reset_cfg->attr.i2c_attr.mask,
            reset_cfg->attr.i2c_attr.reset_on, reset_cfg->attr.i2c_attr.reset_off);
    } else if (reset_cfg->pca9548_reset_type == PCA9548_RESET_GPIO) {

        PCA954X_DEBUG("reset by gpio.\n");
        reset_cfg->attr.gpio_attr.gpio = i2c_mux_pca954x_device->attr.gpio_attr.gpio;
        reset_cfg->attr.gpio_attr.reset_on = i2c_mux_pca954x_device->attr.gpio_attr.reset_on;
        reset_cfg->attr.gpio_attr.reset_off = i2c_mux_pca954x_device->attr.gpio_attr.reset_off;
        PCA954X_DEBUG("gpio number:%u, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.gpio_attr.gpio, reset_cfg->attr.gpio_attr.reset_on,
            reset_cfg->attr.gpio_attr.reset_off);
        reset_cfg->attr.gpio_attr.gpio_init = 0;
    } else if (reset_cfg->pca9548_reset_type == PCA9548_RESET_IO) {

        PCA954X_DEBUG("reset by io.\n");
        reset_cfg->attr.io_attr.io_addr = i2c_mux_pca954x_device->attr.io_attr.io_addr;
        reset_cfg->attr.io_attr.mask = i2c_mux_pca954x_device->attr.io_attr.mask;
        reset_cfg->attr.io_attr.reset_on = i2c_mux_pca954x_device->attr.io_attr.reset_on;
        reset_cfg->attr.io_attr.reset_off = i2c_mux_pca954x_device->attr.io_attr.reset_off;
        PCA954X_DEBUG("io_addr:0x%x, mask:0x%x, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.io_attr.io_addr, reset_cfg->attr.io_attr.mask,
            reset_cfg->attr.io_attr.reset_on, reset_cfg->attr.io_attr.reset_off);
    } else if (reset_cfg->pca9548_reset_type == PCA9548_RESET_FILE) {

        reset_cfg->attr.file_attr.dev_name = i2c_mux_pca954x_device->attr.file_attr.dev_name;
        reset_cfg->attr.file_attr.offset = i2c_mux_pca954x_device->attr.file_attr.offset;
        reset_cfg->attr.file_attr.mask = i2c_mux_pca954x_device->attr.file_attr.mask;
        reset_cfg->attr.file_attr.reset_on = i2c_mux_pca954x_device->attr.file_attr.reset_on;
        reset_cfg->attr.file_attr.reset_off = i2c_mux_pca954x_device->attr.file_attr.reset_off;
        PCA954X_DEBUG("dev_name:%s, mask:0x%x, reset_on:0x%x, reset_off:0x%x.\n",
            reset_cfg->attr.file_attr.dev_name, reset_cfg->attr.file_attr.mask,
            reset_cfg->attr.file_attr.reset_on, reset_cfg->attr.file_attr.reset_off);
    } else {
        PCA954X_ERROR("Unsupport reset type:%d.\n", reset_cfg->pca9548_reset_type);
        return -EINVAL;
    }
    return 0;
}

/*
 * I2C init/probing/exit functions
 */
static int pca954x_probe(struct i2c_client *client,
             const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    struct device_node *of_node = client->dev.of_node;
    bool idle_disconnect_dt;
    struct gpio_desc *gpio;
    int num, force, class;
    struct i2c_mux_core *muxc;
    struct pca954x *data;
    const struct of_device_id *match;
    unsigned int probe_disable;
    int ret, dynamic_nr;
    i2c_mux_pca954x_device_t *i2c_mux_pca954x_device;

    PCA954X_DEBUG("pca954x_probe, parent bus: %d, 9548 addr:0x%x.\n", adap->nr, client->addr);

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
        return -ENODEV;

    muxc = i2c_mux_alloc(adap, &client->dev,
                 PCA954X_MAX_NCHANS, sizeof(*data), 0,
                 pca954x_select_chan, pca954x_deselect_mux);
    if (!muxc)
        return -ENOMEM;
    data = i2c_mux_priv(muxc);

    i2c_set_clientdata(client, muxc);
    data->client = client;

    /* Get the mux out of reset if a reset GPIO is specified. */
    gpio = devm_gpiod_get_optional(&client->dev, "reset", GPIOD_OUT_LOW);
    if (IS_ERR(gpio))
        return PTR_ERR(gpio);

    /* check device connection status */

    if (client->dev.of_node == NULL) {
        if (client->dev.platform_data == NULL) {
            probe_disable = 1;
            PCA954X_DEBUG("has no platform data config, set probe_disable = 1.\n");
        } else {
            i2c_mux_pca954x_device = client->dev.platform_data;
            probe_disable = i2c_mux_pca954x_device->probe_disable;
        }
    } else {
        probe_disable = of_property_read_bool(of_node, "probe_disable");
    }

    /* Write the mux register at addr to verify
     * that the mux is in fact present. This also
     * initializes the mux to disconnected state.
     */
    if (!probe_disable && (i2c_smbus_write_byte(client, 0) < 0)) {
        dev_warn(&client->dev, "probe failed\n");
        return -ENODEV;
    }

    match = of_match_device(of_match_ptr(pca954x_of_match), &client->dev);
    if (match)
        data->chip = of_device_get_match_data(&client->dev);
    else
        data->chip = &chips[id->driver_data];

    data->last_chan = 0;           /* force the first selection */

    if (client->dev.of_node == NULL) {
        idle_disconnect_dt = false;
    } else {
        idle_disconnect_dt = of_node &&
            of_property_read_bool(of_node, "i2c-mux-idle-disconnect");
    }

    if (client->dev.of_node) {
        ret= of_pca954x_reset_data_init(data);
    } else {
        ret= pca954x_reset_data_init(data);
    }
    if (ret < 0) {
        dev_err(&client->dev, "pca954x reset config err, ret:%d.\n", ret);
        return ret;
    }

    if (client->dev.of_node) {
        ret = of_pca954x_irq_setup(muxc);
    } else {
        ret = pca954x_irq_setup(muxc);
    }
    if (ret) {
        goto fail_del_adapters;
    }

    if (client->dev.of_node == NULL) {
        if (client->dev.platform_data == NULL) {
            dynamic_nr = 1;
            PCA954X_DEBUG("platform data is NULL, use dynamic adap number.\n");
        } else {
            i2c_mux_pca954x_device = client->dev.platform_data;
            data->pca9548_cfg_info.pca9548_base_nr = i2c_mux_pca954x_device->pca9548_base_nr;
            if (data->pca9548_cfg_info.pca9548_base_nr == 0) {
                dynamic_nr = 1;
                PCA954X_DEBUG("pca9548_base_nr = 0, use dynamic adap number.\n");
            } else {
                dynamic_nr = 0;
                PCA954X_DEBUG("pca9548_base_nr:%u.\n", data->pca9548_cfg_info.pca9548_base_nr);
            }
        }
    } else {
        if (of_property_read_u32(of_node, "pca9548_base_nr", &data->pca9548_cfg_info.pca9548_base_nr)) {

            dynamic_nr = 1;
            PCA954X_DEBUG("pca9548_base_nr not found, use dynamic adap number");
        } else {
            dynamic_nr = 0;
            PCA954X_DEBUG("pca9548_base_nr:%u.\n", data->pca9548_cfg_info.pca9548_base_nr);
        }
    }

    /* Now create an adapter for each channel */
    for (num = 0; num < data->chip->nchans; num++) {
        bool idle_disconnect_pd = false;
        if (dynamic_nr == 1) {
            force = 0;              /* dynamic adap number */
        } else {
            force = data->pca9548_cfg_info.pca9548_base_nr + num;
        }

        class = 0;              /* no class by default */
        data->deselect |= (idle_disconnect_pd ||
                   idle_disconnect_dt) << num;

        ret = i2c_mux_add_adapter(muxc, force, num, class);
        if (ret)
            goto fail_del_adapters;
    }

    dev_info(&client->dev,
         "registered %d multiplexed busses for I2C %s %s\n",
         num, data->chip->muxtype == pca954x_ismux
                ? "mux" : "switch", client->name);

    return 0;

fail_del_adapters:
    i2c_mux_del_adapters(muxc);
    return ret;
}

static int pca954x_remove(struct i2c_client *client)
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct pca954x *data = i2c_mux_priv(muxc);
    int c, irq;

    if (data->irq) {
        for (c = 0; c < data->chip->nchans; c++) {
            irq = irq_find_mapping(data->irq, c);
            irq_dispose_mapping(irq);
        }
        irq_domain_remove(data->irq);
    }

    i2c_mux_del_adapters(muxc);
    return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pca954x_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct pca954x *data = i2c_mux_priv(muxc);

    data->last_chan = 0;
    return i2c_smbus_write_byte(client, 0);
}
#endif

static SIMPLE_DEV_PM_OPS(pca954x_pm, NULL, pca954x_resume);

static struct i2c_driver pca954x_driver = {
    .driver        = {
        .name    = "wb_pca954x",
        .pm    = &pca954x_pm,
        .of_match_table = of_match_ptr(pca954x_of_match),
    },
    .probe        = pca954x_probe,
    .remove        = pca954x_remove,
    .id_table    = pca954x_id,
};

module_i2c_driver(pca954x_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("PCA954x I2C mux/switch driver");
MODULE_LICENSE("GPL");
