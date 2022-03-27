/*
 * I2C multiplexer driver for PCA9641 bus master selector
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

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0)    
#include <linux/i2c/pca954x.h>
#endif
#include "pegatron_pub.h"

/*
 * The PCA9641 is a bus master selector. It supports two I2C masters connected
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

#define PCA9641_ID              0x00
#define PCA9641_ID_MAGIC        0x38

#define PCA9641_CONTROL         0x01
#define PCA9641_STATUS          0x02
#define PCA9641_TIME            0x03

#define PCA9641_CTL_LOCK_REQ            BIT(0)
#define PCA9641_CTL_LOCK_GRANT          BIT(1)
#define PCA9641_CTL_BUS_CONNECT         BIT(2)
#define PCA9641_CTL_BUS_INIT            BIT(3)
#define PCA9641_CTL_SMBUS_SWRST         BIT(4)
#define PCA9641_CTL_IDLE_TIMER_DIS      BIT(5)
#define PCA9641_CTL_SMBUS_DIS           BIT(6)
#define PCA9641_CTL_PRIORITY            BIT(7)

#define PCA9641_STS_OTHER_LOCK          BIT(0)
#define PCA9641_STS_BUS_INIT_FAIL       BIT(1)
#define PCA9641_STS_BUS_HUNG            BIT(2)
#define PCA9641_STS_MBOX_EMPTY          BIT(3)
#define PCA9641_STS_MBOX_FULL           BIT(4)
#define PCA9641_STS_TEST_INT            BIT(5)
#define PCA9641_STS_SCL_IO              BIT(6)
#define PCA9641_STS_SDA_IO              BIT(7)

/* arbitration timeouts, in jiffies */
#define ARB_TIMEOUT (HZ / 8)    /* 125 ms until forcing bus ownership */
#define ARB2_TIMEOUT    (HZ / 4)    /* 250 ms until acquisition failure */

/* arbitration retry delays, in us */
#define SELECT_DELAY_SHORT  50
#define SELECT_DELAY_LONG   1000

static void pca9641_release_bus(struct i2c_client *client);
static int pca9641_arbitrate(struct i2c_client *client);
static uint loglevel = LOG_INFO | LOG_WARNING | LOG_ERR;

enum pca_type {
    pca_9541,
    pca_9641,
};

struct pca9641 {
    struct i2c_client *client;
    unsigned long select_timeout;
    unsigned long arb_timeout;
};

static const struct i2c_device_id pca9641_id[] = {
    { "pca9541", pca_9541 },
    { "pca9641", pca_9641 },
    {}
};

MODULE_DEVICE_TABLE(i2c, pca9641_id);

#ifdef CONFIG_OF
static const struct of_device_id pca9641_of_match[] = {
    { .compatible = "nxp,pca9541" },
    { .compatible = "nxp,pca9641" },
    {}
};
MODULE_DEVICE_TABLE(of, pca9641_of_match);
#endif

/*
 * Write to chip register. Don't use i2c_transfer()/i2c_smbus_xfer()
 * as they will try to lock the adapter a second time.
 */
static int pca9641_reg_write(struct i2c_client *client, u8 command, u8 val)
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
        ret = adap->algo->master_xfer(adap, &msg, 1);
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
static int pca9641_reg_read(struct i2c_client *client, u8 command)
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
        ret = adap->algo->master_xfer(adap, msg, 2);
        if (ret == 2) {
            ret = val;
        }
        else if (ret >= 0) {
            ret = -EIO;
        }
    } else {
        union i2c_smbus_data data;

        ret = adap->algo->smbus_xfer(adap, client->addr,
                client->flags,
                I2C_SMBUS_READ,
                command,
                I2C_SMBUS_BYTE_DATA, &data);
        if (!ret) {
            ret = data.byte;
        }
    }
    return ret;
}

/*
 * Arbitration management functions
 */

#define pca9641_other_lock(r)   ((r) & PCA9641_STS_OTHER_LOCK)
#define pca9641_lock_grant(r)   ((r) & PCA9641_CTL_LOCK_GRANT)

static inline int pca9641_busoff(int ctl, int sts)
{
    return !(ctl & PCA9641_CTL_LOCK_GRANT) &&
        !(sts & PCA9641_STS_OTHER_LOCK);
}


/* Release bus. Also reset NTESTON and BUSINIT if it was set. */
static void pca9641_release_bus(struct i2c_client *client)
{
    pca9641_reg_write(client, PCA9641_CONTROL, 0);
}


/*
 * Arbitration is defined as a two-step process. A bus master can only activate
 * the slave bus if it owns it; otherwise it has to request ownership first.
 * This multi-step process ensures that access contention is resolved
 * gracefully.
 *
 * Bus  Ownership   Other master    Action
 * state        requested access
 * ----------------------------------------------------
 * off  -       yes     wait for arbitration timeout or
 *                  for other master to drop request
 * off  no      no      take ownership
 * off  yes     no      turn on bus
 * on   yes     -       done
 * on   no      -       wait for arbitration timeout or
 *                  for other master to release bus
 *
 * The main contention point occurs if the slave bus is off and both masters
 * request ownership at the same time. In this case, one master will turn on
 * the slave bus, believing that it owns it. The other master will request
 * bus ownership. Result is that the bus is turned on, and master which did
 * _not_ own the slave bus before ends up owning it.
 */

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
    struct pca9641 *data = i2c_mux_priv(muxc);
    int reg_ctl, reg_sts;

    reg_ctl = pca9641_reg_read(client, PCA9641_CONTROL);
    if (reg_ctl < 0) {
        return reg_ctl;
    }
    reg_sts = pca9641_reg_read(client, PCA9641_STATUS);
    pega_print(DEBUG, "PCA9641_CONTROL:0x%x,PCA9641_STATUS:0x%x\n", reg_ctl,reg_sts);

    if (pca9641_busoff(reg_ctl, reg_sts)) {
        /*
         * Bus is off. Request ownership or turn it on unless
         * other master requested ownership.
         */
        //printk(KERN_WARNING "(ar)lock req\n");
        reg_ctl |= PCA9641_CTL_LOCK_REQ | PCA9641_CTL_IDLE_TIMER_DIS | PCA9641_CTL_SMBUS_DIS;
        pca9641_reg_write(client, PCA9641_CONTROL, reg_ctl);
        reg_ctl = pca9641_reg_read(client, PCA9641_CONTROL);
        if (pca9641_lock_grant(reg_ctl)) {
            /*
             * Other master did not request ownership,
             * or arbitration timeout expired. Take the bus.
             */
            //printk(KERN_WARNING "(ar)grant\n");
            reg_ctl |= PCA9641_CTL_BUS_CONNECT
                | PCA9641_CTL_LOCK_REQ;
            pca9641_reg_write(client, PCA9641_CONTROL, reg_ctl);
            data->select_timeout = SELECT_DELAY_SHORT;

            return 1;
        } else {
            /*
             * Other master requested ownership.
             * Set extra long timeout to give it time to acquire it.
             */
            data->select_timeout = SELECT_DELAY_LONG * 2;
        }
    } else if (pca9641_lock_grant(reg_ctl)) {
        /*
         * Bus is on, and we own it. We are done with acquisition.
         */
        //printk(KERN_WARNING "(ar)grant already\n");
        reg_ctl |= PCA9641_CTL_BUS_CONNECT | PCA9641_CTL_LOCK_REQ;
        pca9641_reg_write(client, PCA9641_CONTROL, reg_ctl);

        return 1;
    } else if (pca9641_other_lock(reg_sts)) {
        /*
         * Other master owns the bus.
         * If arbitration timeout has expired, force ownership.
         * Otherwise request it.
         */
        //printk(KERN_WARNING "(ar) other grant already\n");
        data->select_timeout = SELECT_DELAY_LONG;
        reg_ctl |= PCA9641_CTL_LOCK_REQ;
        pca9641_reg_write(client, PCA9641_CONTROL, reg_ctl);
    }
    return 0;
}

static int pca9641_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
    struct pca9641 *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    int ret;
    unsigned long timeout = jiffies + ARB2_TIMEOUT;
    /* give up after this time */

    pega_print(DEBUG, "(ar)select\n");
    data->arb_timeout = jiffies + ARB_TIMEOUT;
    /* force bus ownership after this time */
    do {
        ret = pca9641_arbitrate(client);
        if (ret) {
            return ret < 0 ? ret : 0;
        }

        if (data->select_timeout == SELECT_DELAY_SHORT) {
            udelay(data->select_timeout);
        }
        else {
            msleep(data->select_timeout / 1000);
        }
    } while (time_is_after_eq_jiffies(timeout));
    pega_print(WARNING,"arbiter acquisition failure (timeout)\n");
    return -ETIMEDOUT;
}

static int pca9641_release_chan(struct i2c_mux_core *muxc, u32 chan)
{
    struct pca9641 *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    pca9641_release_bus(client);
    return 0;
}

/*
 * I2C init/probing/exit functions
 */
static int pca9641_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = client->adapter;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)    
    struct pca954x_platform_data *pdata = dev_get_platdata(&client->dev);
#endif
    struct i2c_mux_core *muxc;
    struct pca9641 *data;
    int force;
    int ret = -ENODEV;
    //int detect_id;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;

    /*
     * I2C accesses are unprotected here.
     * We have to lock the adapter before releasing the bus.
     */
    i2c_lock_bus(adap, I2C_LOCK_SEGMENT);
    pca9641_release_bus(client);
    i2c_unlock_bus(adap, I2C_LOCK_SEGMENT);

    /* Create mux adapter */

    force = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0)    
    if (pdata)
        force = pdata->modes[0].adap_id;
#endif
    muxc = i2c_mux_alloc(adap, &client->dev, 1, sizeof(*data),
            I2C_MUX_ARBITRATOR,
            pca9641_select_chan, pca9641_release_chan);
    if (!muxc)
        return -ENOMEM;

    data = i2c_mux_priv(muxc);
    data->client = client;

    i2c_set_clientdata(client, muxc);

    ret = i2c_mux_add_adapter(muxc, force, 0, 0);
    if (ret) {
        dev_err(&client->dev, "failed to register master selector\n");
        return ret;
    }

    dev_info(&client->dev, "registered master selector for I2C %s\n",
            client->name);

    return 0;
}

static int pca9641_remove(struct i2c_client *client)
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);

    i2c_mux_del_adapters(muxc);
    return 0;
}

static struct i2c_driver pca9641_driver = {
    .driver = {
        .name = "pca9641",
        .of_match_table = of_match_ptr(pca9641_of_match),
    },
    .probe = pca9641_probe,
    .remove = pca9641_remove,
    .id_table = pca9641_id,
};

module_i2c_driver(pca9641_driver);

module_param(loglevel,uint,0664);
MODULE_PARM_DESC(loglevel,"0x01-LOG_ERR,0x02-LOG_WARNING,0x04-LOG_INFO,0x08-LOG_DEBUG");
MODULE_AUTHOR("Guenter Roeck <linux@roeck-us.net>");
MODULE_DESCRIPTION("PCA9641 I2C master selector driver");
MODULE_LICENSE("GPL v2");
