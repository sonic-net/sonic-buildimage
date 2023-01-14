/*
 * i2c-ocores.c: I2C bus driver for OpenCores I2C controller
 * (http://www.opencores.org/projects.cgi/web/i2c/overview).
 *
 * Peter Korsgaard <jacmet@sunsite.dk>
 *
 * Support for the GRLIB port of the controller by
 * Andreas Larsson <andreas@gaisler.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/log2.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include "fpga_i2c_ocores.h"
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/version.h>

struct ocores_i2c {
    void __iomem *base;
    u32 reg_shift;
    u32 reg_io_width;
    wait_queue_head_t wait;
    struct i2c_adapter adap;
    struct i2c_msg *msg;
    int pos;
    int nmsgs;
    int state; /* see STATE_ */
    spinlock_t process_lock;
    struct mutex xfer_lock;
    int clock_khz;
    void (*setreg)(struct ocores_i2c *i2c, int reg, u8 value);
    u8 (*getreg)(struct ocores_i2c *i2c, int reg);
};

/* registers */
#define OCI2C_PRELOW        0x0
#define OCI2C_PREHIGH       0x4
#define OCI2C_CONTROL       0x8
#define OCI2C_DATA          0xc
#define OCI2C_CMD           0x10 /* write only */
#define OCI2C_STATUS        0x10 /* read only, same address as OCI2C_CMD */

#define OCI2C_TRAN_REV      0x14
#define OCI2C_CMD_REV       0x18


#define OCI2C_CTRL_IEN      0x40
#define OCI2C_CTRL_EN       0x80

#define OCI2C_CMD_START     0x91
#define OCI2C_CMD_STOP      0x41
#define OCI2C_CMD_READ      0x21
#define OCI2C_CMD_WRITE     0x11
#define OCI2C_CMD_READ_ACK  0x21
#define OCI2C_CMD_READ_NACK 0x29
#define OCI2C_CMD_IACK      0x01

#define OCI2C_STAT_IF       0x01
#define OCI2C_STAT_TIP      0x02
#define OCI2C_STAT_ARBLOST  0x20
#define OCI2C_STAT_BUSY     0x40
#define OCI2C_STAT_NACK     0x80

#define STATE_DONE      0
#define STATE_START     1
#define STATE_WRITE     2
#define STATE_READ      3
#define STATE_ERROR     4

#define TYPE_OCORES     0
#define TYPE_GRLIB      1

#define BUF_SIZE        256
#define DEFAULT_I2C_SCL 100
#define DEFAULT_I2C_PRE 0xF9

int g_fpga_i2c_debug = 0;
int g_fpga_i2c_irq = 0;
int g_fpga_i2c_error = 0;
int g_irq_dump_debug = 0;
int g_irq_invalid_cnt = 0;
int g_fpga_debug = 0;

module_param(g_fpga_i2c_debug, int, S_IRUGO | S_IWUSR);
module_param(g_fpga_i2c_error, int, S_IRUGO | S_IWUSR);
module_param(g_fpga_i2c_irq, int, S_IRUGO | S_IWUSR);
module_param(g_irq_dump_debug, int, S_IRUGO | S_IWUSR);
module_param(g_irq_invalid_cnt, int, S_IRUGO | S_IWUSR);
module_param(g_fpga_debug, int, S_IRUGO | S_IWUSR);

#define FPGA_I2C_DEBUG(fmt, args...) do {                                        \
    if (g_fpga_debug) { \
        printk(KERN_DEBUG ""fmt, ## args); \
    } \
} while (0)

#define FPGA_I2C_DEBUG_DUMP(fmt, args...) do {                                        \
    if (g_irq_dump_debug) { \
        printk(KERN_ERR ""fmt, ## args); \
    } \
} while (0)

#define FPGA_I2C_DEBUG_XFER(fmt, args...) do {                                        \
    if (g_fpga_i2c_irq) { \
        printk(KERN_ERR "[FPGA_I2C][XFER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_I2C_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_fpga_i2c_debug) { \
        printk(KERN_ERR "[FPGA_I2C][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_I2C_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_fpga_i2c_error) { \
        printk(KERN_ERR "[FPGA_I2C][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static int check_ocores_i2c(struct i2c_msg *msgs, int num);
static void oc_debug_dump_reg(struct ocores_i2c *i2c);
static void oc_debug_dump_reg_dump(struct ocores_i2c *i2c);
static int oc_set_scl_clk(struct ocores_i2c *i2c, int val);

static void oc_setreg_8(struct ocores_i2c *i2c, int reg, u8 value)
{
    iowrite8(value, i2c->base + (reg << i2c->reg_shift));
}

static void oc_setreg_16(struct ocores_i2c *i2c, int reg, u8 value)
{
    iowrite16(value, i2c->base + (reg << i2c->reg_shift));
}

static void oc_setreg_32(struct ocores_i2c *i2c, int reg, u8 value)
{
    iowrite32(value, i2c->base + (reg << i2c->reg_shift));
}

static inline u8 oc_getreg_8(struct ocores_i2c *i2c, int reg)
{
    return ioread8(i2c->base + (reg << i2c->reg_shift));
}

static inline u8 oc_getreg_16(struct ocores_i2c *i2c, int reg)
{
    return ioread16(i2c->base + (reg << i2c->reg_shift));
}

static inline u8 oc_getreg_32(struct ocores_i2c *i2c, int reg)
{
    return ioread32(i2c->base + (reg << i2c->reg_shift));
}

static inline void oc_setreg(struct ocores_i2c *i2c, int reg, u8 value)
{
    i2c->setreg(i2c, reg, value);
}

static inline u8 oc_getreg(struct ocores_i2c *i2c, int reg)
{
    return i2c->getreg(i2c, reg);
}

#define FPGA_I2C_SPIN_LOCK(lock, flags) spin_lock_irqsave(&(lock), (flags))
#define FPGA_I2C_SPIN_UNLOCK(lock, flags) spin_unlock_irqrestore(&(lock), (flags))
#define FPGA_I2C_MUTEX_LOCK(lock)   mutex_lock(&(lock))
#define FPGA_I2C_MUTEX_UNLOCK(lock) mutex_unlock(&(lock))

static void ocores_process(struct ocores_i2c *i2c, u8 stat)
{
    struct i2c_msg *msg = i2c->msg;

    FPGA_I2C_DEBUG_XFER("Enter nr %d.\n", i2c->adap.nr);
    if ((i2c->state == STATE_DONE) || (i2c->state == STATE_ERROR)) {
        /* stop has been sent */
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
        wake_up(&i2c->wait);
        FPGA_I2C_DEBUG_XFER("stop has been sent, exit.\n");
        goto out;
    }

    FPGA_I2C_DEBUG_XFER("Enter 111.\n");

    /* error */
    if (stat & OCI2C_STAT_ARBLOST) {
        i2c->state = STATE_ERROR;
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
        FPGA_I2C_DEBUG_XFER("error, exit.\n");
        goto out;
    }

    FPGA_I2C_DEBUG_XFER("Enter 222.\n");

    if (check_ocores_i2c(i2c->msg, i2c->nmsgs) != 0) {
        FPGA_I2C_DEBUG("i2c->msg->buf is null, i2c->state:%d exit.\n", i2c->state);
        oc_debug_dump_reg_dump(i2c);
        i2c->state = STATE_ERROR;
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
        goto out;
    }

    if ((i2c->state == STATE_START) || (i2c->state == STATE_WRITE)) {
        i2c->state =
            (msg->flags & I2C_M_RD) ? STATE_READ : STATE_WRITE;

        if (stat & OCI2C_STAT_NACK) {
            i2c->state = STATE_ERROR;
            oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
            FPGA_I2C_DEBUG_XFER("OCI2C_STAT_NACK, exit.\n");
            goto out;
        }
    } else {
        msg->buf[i2c->pos++] = oc_getreg(i2c, OCI2C_DATA);
    }
    FPGA_I2C_DEBUG_XFER("Enter 333.\n");

    /* end of msg? */
    if (i2c->pos == msg->len) {
        FPGA_I2C_DEBUG_XFER("Enter end of msg.\n");
        i2c->nmsgs--;
        i2c->msg++;
        i2c->pos = 0;
        msg = i2c->msg;

        if (i2c->nmsgs) {   /* end? */
            /* send start? */
            if (!(msg->flags & I2C_M_NOSTART)) {
                u8 addr = (msg->addr << 1);

                if (msg->flags & I2C_M_RD)
                    addr |= 1;

                i2c->state = STATE_START;

                oc_setreg(i2c, OCI2C_DATA, addr);
                oc_setreg(i2c, OCI2C_CMD,  OCI2C_CMD_START);
                FPGA_I2C_DEBUG_XFER("send start, exit.\n");
                goto out;
            }

            i2c->state = (msg->flags & I2C_M_RD)
                ? STATE_READ : STATE_WRITE;
        } else {
            i2c->state = STATE_DONE;
            oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
            FPGA_I2C_DEBUG_XFER("send OCI2C_CMD_STOP, exit.\n");
            goto out;
        }
    }

    if (i2c->state == STATE_READ) {
        oc_setreg(i2c, OCI2C_CMD, i2c->pos == (msg->len-1) ?
                OCI2C_CMD_READ_NACK : OCI2C_CMD_READ_ACK);
    } else {
        oc_setreg(i2c, OCI2C_DATA, msg->buf[i2c->pos++]);
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_WRITE);
    }

out:
    FPGA_I2C_DEBUG_XFER("normal, exit nr %d.\n", i2c->adap.nr);
}

static irqreturn_t ocores_isr(int irq, void *dev_id)
{
    struct ocores_i2c *i2c = dev_id;
    unsigned long flags;
    u8 stat;

    if (!i2c) {
        return IRQ_NONE;
    }
    /*
     * If we spin here is because we are in timeout, so we are going
     * to be in STATE_ERROR. See ocores_process_timeout()
     */
    FPGA_I2C_SPIN_LOCK(i2c->process_lock, flags);
    stat = oc_getreg(i2c, OCI2C_STATUS);
    if (!(stat & OCI2C_STAT_IF)) {
        g_irq_invalid_cnt++;
        FPGA_I2C_SPIN_UNLOCK(i2c->process_lock, flags);
        return IRQ_NONE;
    }

    FPGA_I2C_DEBUG_XFER("Enter, irq %d nr %d addr 0x%x.\n", irq, i2c->adap.nr, (!i2c->msg)?0:i2c->msg->addr);
    ocores_process(i2c, stat);
    FPGA_I2C_DEBUG_XFER("Leave, irq %d nr %d addr 0x%x.\n", irq, i2c->adap.nr, (!i2c->msg)?0:i2c->msg->addr);

    FPGA_I2C_SPIN_UNLOCK(i2c->process_lock, flags);
    return IRQ_HANDLED;
}

/**
 * Process timeout event
 * @i2c: ocores I2C device instance
 */
static void ocores_process_timeout(struct ocores_i2c *i2c)
{
    unsigned long flags;

    FPGA_I2C_SPIN_LOCK(i2c->process_lock, flags);
    FPGA_I2C_DEBUG_ERROR("wait_event_timeout i2c->state %d.\n", i2c->state);
    oc_debug_dump_reg(i2c);
    i2c->state = STATE_ERROR;
    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
    mdelay(1);
    FPGA_I2C_SPIN_UNLOCK(i2c->process_lock, flags);
}

static int check_ocores_i2c(struct i2c_msg *msgs, int num)
{
    int i;
    if (!msgs) {
        return -1;
    }
    for (i = 0; i < num; ++i) {
        if (!msgs[i].buf) {
            return -1;
        }
    }
    return 0;
}

static int ocores_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct ocores_i2c *i2c;
    int ret;
    unsigned long flags;
    int xfer_ret;

    if (!adap || check_ocores_i2c(msgs, num) != 0) {
        FPGA_I2C_DEBUG("msgs: %p , num:%d exit.\n", msgs, num);
        return -EINVAL;
    }
    i2c = i2c_get_adapdata(adap);

    FPGA_I2C_MUTEX_LOCK(i2c->xfer_lock);
    FPGA_I2C_SPIN_LOCK(i2c->process_lock, flags);
    i2c->msg = msgs;
    i2c->pos = 0;
    i2c->nmsgs = num;
    i2c->state = STATE_START;
    FPGA_I2C_DEBUG_XFER("Enter, nr %d addr 0x%x num %d.\n", adap->nr, i2c->msg->addr, num);

    oc_setreg(i2c, OCI2C_DATA,
            (i2c->msg->addr << 1) |
            ((i2c->msg->flags & I2C_M_RD) ? 1:0));

    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_START);
    FPGA_I2C_DEBUG_XFER("After, oc_setreg OCI2C_CMD.\n");
    FPGA_I2C_SPIN_UNLOCK(i2c->process_lock, flags);

    ret = wait_event_timeout(i2c->wait, (i2c->state == STATE_ERROR) ||
            (i2c->state == STATE_DONE), HZ);

    if (ret == 0) {
        ocores_process_timeout(i2c);
        FPGA_I2C_MUTEX_UNLOCK(i2c->xfer_lock);
        return -ETIMEDOUT;
    }
    xfer_ret = i2c->state;
    FPGA_I2C_MUTEX_UNLOCK(i2c->xfer_lock);
    return (xfer_ret == STATE_DONE) ? num : -EIO;
}

static void ocores_init(struct ocores_i2c *i2c)
{
    int prescale;
    u8 ctrl = oc_getreg(i2c, OCI2C_CONTROL);

    mutex_init(&i2c->xfer_lock);
    spin_lock_init(&i2c->process_lock);

    /* make sure the device is disabled */
    oc_setreg(i2c, OCI2C_CONTROL, ctrl & ~(OCI2C_CTRL_EN|OCI2C_CTRL_IEN));

    prescale = oc_set_scl_clk(i2c, DEFAULT_I2C_SCL);
    FPGA_I2C_DEBUG_VERBOSE("i2c->base 0x%p, i2c->clock_khz %d, prescale 0x%x.\n", i2c->base, i2c->clock_khz, prescale);

    /* Init the device */
    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
    oc_setreg(i2c, OCI2C_CONTROL, ctrl | OCI2C_CTRL_IEN | OCI2C_CTRL_EN);
}


static u32 ocores_func(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm ocores_algorithm = {
    .master_xfer    = ocores_xfer,
    .functionality  = ocores_func,
};

static struct i2c_adapter ocores_adapter = {
    .owner      = THIS_MODULE,
    .name       = "rg-i2c-ocores",
    .class      = I2C_CLASS_HWMON | I2C_CLASS_SPD | I2C_CLASS_DEPRECATED,
    .algo       = &ocores_algorithm,
};

static const struct of_device_id ocores_i2c_match[] = {
    {
        .compatible = "opencores,rg-i2c-ocores",
        .data = (void *)TYPE_OCORES,
    },
    {
        .compatible = "aeroflexgaisler,i2cmst",
        .data = (void *)TYPE_GRLIB,
    },
    {},
};
MODULE_DEVICE_TABLE(of, ocores_i2c_match);

#ifdef CONFIG_OF
/* Read and write functions for the GRLIB port of the controller. Registers are
 * 32-bit big endian and the PRELOW and PREHIGH registers are merged into one
 * register. The subsequent registers has their offset decreased accordingly. */
static u8 oc_getreg_grlib(struct ocores_i2c *i2c, int reg)
{
    u32 rd;
    int rreg = reg;
    if (reg != OCI2C_PRELOW)
        rreg--;
    rd = ioread32be(i2c->base + (rreg << i2c->reg_shift));
    if (reg == OCI2C_PREHIGH)
        return (u8)(rd >> 8);
    else
        return (u8)rd;
}

static void oc_setreg_grlib(struct ocores_i2c *i2c, int reg, u8 value)
{
    u32 curr, wr;
    int rreg = reg;
    if (reg != OCI2C_PRELOW)
        rreg--;
    if (reg == OCI2C_PRELOW || reg == OCI2C_PREHIGH) {
        curr = ioread32be(i2c->base + (rreg << i2c->reg_shift));
        if (reg == OCI2C_PRELOW)
            wr = (curr & 0xff00) | value;
        else
            wr = (((u32)value) << 8) | (curr & 0xff);
    } else {
        wr = value;
    }
    iowrite32be(wr, i2c->base + (rreg << i2c->reg_shift));
}

static int ocores_i2c_of_probe(struct platform_device *pdev,
        struct ocores_i2c *i2c)
{
    struct device_node *np = pdev->dev.of_node;
    const struct of_device_id *match;
    u32 val;

    if (of_property_read_u32(np, "reg-shift", &i2c->reg_shift)) {
        /* no 'reg-shift', check for deprecated 'regstep' */
        if (!of_property_read_u32(np, "regstep", &val)) {
            if (!is_power_of_2(val)) {
                dev_err(&pdev->dev, "invalid regstep %d\n",
                        val);
                return -EINVAL;
            }
            i2c->reg_shift = ilog2(val);
            dev_warn(&pdev->dev,
                    "regstep property deprecated, use reg-shift\n");
        }
    }

    if (of_property_read_u32(np, "clock-frequency", &val)) {
        dev_err(&pdev->dev,
                "Missing required parameter 'clock-frequency'\n");
        return -ENODEV;
    }
    i2c->clock_khz = val / 1000;

    of_property_read_u32(pdev->dev.of_node, "reg-io-width",
            &i2c->reg_io_width);

    match = of_match_node(ocores_i2c_match, pdev->dev.of_node);
    if (match && (long)match->data == TYPE_GRLIB) {
        dev_dbg(&pdev->dev, "GRLIB variant of i2c-ocores\n");
        i2c->setreg = oc_setreg_grlib;
        i2c->getreg = oc_getreg_grlib;
    }

    return 0;
}
#else
#define ocores_i2c_of_probe(pdev,i2c) -ENODEV
#endif


static void oc_debug_dump_reg_dump(struct ocores_i2c *i2c)
{
    if (i2c) {
        FPGA_I2C_DEBUG("base: %p.\n", i2c->base);
        FPGA_I2C_DEBUG("reg_shift: %d.\n", i2c->reg_shift);
        FPGA_I2C_DEBUG("reg_io_width: %d.\n", i2c->reg_io_width);
        FPGA_I2C_DEBUG("adap.nr: %d.\n", i2c->adap.nr);
        FPGA_I2C_DEBUG("msg: %p.\n", i2c->msg);
        if (i2c->msg) {
            FPGA_I2C_DEBUG("msg->buf: %p.\n", i2c->msg->buf);
            FPGA_I2C_DEBUG("msg->addr: 0x%x.\n", i2c->msg->addr);
            FPGA_I2C_DEBUG("msg->flags: 0x%x.\n", i2c->msg->flags);
            FPGA_I2C_DEBUG("msg->len: %d.\n", i2c->msg->len);
        } else {
            FPGA_I2C_DEBUG("msg: %p is null.\n", i2c->msg);
        }

        FPGA_I2C_DEBUG("pos: %d.\n", i2c->pos);
        FPGA_I2C_DEBUG("nmsgs: %d.\n", i2c->nmsgs);
        FPGA_I2C_DEBUG("state: %d.\n", i2c->state);
        FPGA_I2C_DEBUG("clock_khz: %d.\n", i2c->clock_khz);
        FPGA_I2C_DEBUG("setreg: %p.\n", i2c->setreg);
        FPGA_I2C_DEBUG("getreg: %p.\n", i2c->getreg);
        if (i2c->getreg) {
            FPGA_I2C_DEBUG("OCI2C_PRELOW: 0x%02x.\n", oc_getreg(i2c, OCI2C_PRELOW));
            FPGA_I2C_DEBUG("OCI2C_PREHIGH: 0x%02x.\n", oc_getreg(i2c, OCI2C_PREHIGH));
            FPGA_I2C_DEBUG("OCI2C_CONTROL: 0x%02x.\n", oc_getreg(i2c, OCI2C_CONTROL));
            FPGA_I2C_DEBUG("OCI2C_DATA: 0x%02x.\n", oc_getreg(i2c, OCI2C_DATA));
            FPGA_I2C_DEBUG("OCI2C_CMD: 0x%02x.\n", oc_getreg(i2c, OCI2C_CMD));
            FPGA_I2C_DEBUG("OCI2C_STATUS: 0x%02x.\n", oc_getreg(i2c, OCI2C_STATUS));
        } else {
            FPGA_I2C_DEBUG("getreg: %p is null.\n", i2c->getreg);
        }
    } else {
        FPGA_I2C_DEBUG("i2c %p is null.\n", i2c);
    }
}


static void oc_debug_dump_reg(struct ocores_i2c *i2c)
{
    if (i2c) {
        FPGA_I2C_DEBUG_DUMP("base: %p.\n", i2c->base);
        FPGA_I2C_DEBUG_DUMP("reg_shift: %d.\n", i2c->reg_shift);
        FPGA_I2C_DEBUG_DUMP("reg_io_width: %d.\n", i2c->reg_io_width);
        FPGA_I2C_DEBUG_DUMP("adap.nr: %d.\n", i2c->adap.nr);
        FPGA_I2C_DEBUG_DUMP("msg: %p.\n", i2c->msg);
        if (i2c->msg) {
            FPGA_I2C_DEBUG_DUMP("msg->buf: %p.\n", i2c->msg->buf);
            FPGA_I2C_DEBUG_DUMP("msg->addr: 0x%x.\n", i2c->msg->addr);
            FPGA_I2C_DEBUG_DUMP("msg->flags: 0x%x.\n", i2c->msg->flags);
            FPGA_I2C_DEBUG_DUMP("msg->len: %d.\n", i2c->msg->len);
        } else {
            FPGA_I2C_DEBUG_DUMP("msg: %p is null.\n", i2c->msg);
        }

        FPGA_I2C_DEBUG_DUMP("pos: %d.\n", i2c->pos);
        FPGA_I2C_DEBUG_DUMP("nmsgs: %d.\n", i2c->nmsgs);
        FPGA_I2C_DEBUG_DUMP("state: %d.\n", i2c->state);
        FPGA_I2C_DEBUG_DUMP("clock_khz: %d.\n", i2c->clock_khz);
        FPGA_I2C_DEBUG_DUMP("setreg: %p.\n", i2c->setreg);
        FPGA_I2C_DEBUG_DUMP("getreg: %p.\n", i2c->getreg);
        if (i2c->getreg) {
            FPGA_I2C_DEBUG_DUMP("OCI2C_PRELOW: 0x%02x.\n", oc_getreg(i2c, OCI2C_PRELOW));
            FPGA_I2C_DEBUG_DUMP("OCI2C_PREHIGH: 0x%02x.\n", oc_getreg(i2c, OCI2C_PREHIGH));
            FPGA_I2C_DEBUG_DUMP("OCI2C_CONTROL: 0x%02x.\n", oc_getreg(i2c, OCI2C_CONTROL));
            FPGA_I2C_DEBUG_DUMP("OCI2C_DATA: 0x%02x.\n", oc_getreg(i2c, OCI2C_DATA));
            FPGA_I2C_DEBUG_DUMP("OCI2C_CMD: 0x%02x.\n", oc_getreg(i2c, OCI2C_CMD));
            FPGA_I2C_DEBUG_DUMP("OCI2C_STATUS: 0x%02x.\n", oc_getreg(i2c, OCI2C_STATUS));
        } else {
            FPGA_I2C_DEBUG_DUMP("getreg: %p is null.\n", i2c->getreg);
        }
    } else {
        FPGA_I2C_DEBUG_DUMP("i2c %p is null.\n", i2c);
    }
}

void oc_debug_dump_reg_exception(void)
{
    int bus_beg, bus_end, bus;
    struct i2c_adapter *adap;
    struct ocores_i2c *adap_data;

    bus_beg = 1;
    bus_end = 14;
    for (bus = bus_beg; bus <= bus_end; bus++) {
        adap = i2c_get_adapter(bus);
        if (adap) {
            adap_data = (struct ocores_i2c *)i2c_get_adapdata(adap);
            if (adap_data) {
                FPGA_I2C_DEBUG_DUMP("bus %d call oc_debug_dump_reg begin.\n", bus);
                oc_debug_dump_reg(adap_data);
                FPGA_I2C_DEBUG_DUMP("bus %d call oc_debug_dump_reg end.\n", bus);
            } else {
                FPGA_I2C_DEBUG_DUMP("bus %d i2c_get_adapdata null.\n", bus);
            }
            i2c_put_adapter(adap);
        } else {
            FPGA_I2C_DEBUG_DUMP("bus %d i2c_get_adapter null.\n", bus);
        }
    }
}

static int oc_calculate_prescale(struct ocores_i2c *i2c, int val) {
    if (val <= 0) {
        FPGA_I2C_DEBUG_ERROR("input scl clock error, set to default clock: %d.\n", val);
        val = DEFAULT_I2C_SCL;
    }
    return (i2c->clock_khz / (5 * val)) - 1;
}

static int oc_calculate_scl_clk(struct ocores_i2c *i2c, int prescale) {
    if (prescale <= -1) {
        FPGA_I2C_DEBUG_ERROR("input prescale error, set to default prescale: %d.\n", prescale);
        prescale = DEFAULT_I2C_PRE;
    }
    return (i2c->clock_khz / (prescale + 1)) / 5;
}

static int oc_set_scl_clk(struct ocores_i2c *i2c, int val) {
    int prescale;

    prescale = oc_calculate_prescale(i2c, val);
    oc_setreg(i2c, OCI2C_PRELOW, prescale & 0xff);
    oc_setreg(i2c, OCI2C_PREHIGH, prescale >> 8);
    return prescale;
}

static int oc_get_scl_clk(struct ocores_i2c *i2c) {
    int prescale, prescale_high, prescale_low;

    prescale_low = oc_getreg(i2c, OCI2C_PRELOW);
    prescale_high = oc_getreg(i2c, OCI2C_PREHIGH);
    prescale = (prescale_high << 8) + (prescale_low & 0xff);

    return oc_calculate_scl_clk(i2c, prescale);
}

static ssize_t oc_sysfs_show_scl_clk(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_adapter *adapter;
    struct ocores_i2c *i2c;
    int scl_clk;

    adapter = to_i2c_adapter(dev);
    i2c = (struct ocores_i2c *)i2c_get_adapdata(adapter);
    scl_clk = oc_get_scl_clk(i2c);
    return snprintf(buf, BUF_SIZE, "%d\n", scl_clk);
}

static ssize_t oc_sysfs_set_scl_clk(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_adapter *adapter;
    struct ocores_i2c *i2c;
    int val;
    int ret;
    int prescale;

    adapter = to_i2c_adapter(dev);
    i2c = (struct ocores_i2c *)i2c_get_adapdata(adapter);
    ret = kstrtoint(buf, 0, &val);
    if (ret) {
        return ret;
    }
    FPGA_I2C_MUTEX_LOCK(i2c->xfer_lock);
    prescale = oc_set_scl_clk(i2c, val);
    FPGA_I2C_DEBUG_VERBOSE("i2c->base 0x%p, i2c->clock_khz %d, scl clk 0x%x.\n", i2c->base, i2c->clock_khz, prescale);
    FPGA_I2C_MUTEX_UNLOCK(i2c->xfer_lock);
    return count;
}
static ssize_t show_oc_debug_value(struct device *dev, struct device_attribute *da, char *buf)
{
    oc_debug_dump_reg_exception();
    return 0;
}

static SENSOR_DEVICE_ATTR(oc_debug, S_IRUGO | S_IWUSR, show_oc_debug_value, NULL, 0x15);
static SENSOR_DEVICE_ATTR(oc_scl_clk, S_IRUGO | S_IWUSR, oc_sysfs_show_scl_clk, oc_sysfs_set_scl_clk, 0);

static struct attribute *oc_debug_sysfs_attrs[] = {
    &sensor_dev_attr_oc_debug.dev_attr.attr,
    NULL
};

static struct attribute *oc_scl_clk_sysfs_attrs[] = {
    &sensor_dev_attr_oc_scl_clk.dev_attr.attr,
    NULL
};

static const struct attribute_group oc_debug_sysfs_group = {
    .attrs = oc_debug_sysfs_attrs,
};

static const struct attribute_group oc_scl_clk_sysfs_group = {
    .attrs = oc_scl_clk_sysfs_attrs,
};

static void oc_scl_clk_sysfs_init(struct i2c_adapter *adap)
{
    int ret;

    ret = sysfs_create_group(&adap->dev.kobj, &oc_scl_clk_sysfs_group);
    FPGA_I2C_DEBUG_VERBOSE("sysfs_create_group ret %d.\n", ret);
    return;
}

static void oc_scl_clk_sysfs_exit(struct i2c_adapter *adap)
{
    sysfs_remove_group(&adap->dev.kobj,  (const struct attribute_group *)&oc_scl_clk_sysfs_group);
    FPGA_I2C_DEBUG_VERBOSE("sysfs_remove_group.\n");
    return;
}

static void oc_debug_sysfs_init(struct platform_device *pdev)
{
    int ret;

    ret = sysfs_create_group(&pdev->dev.kobj, &oc_debug_sysfs_group);
    FPGA_I2C_DEBUG_VERBOSE("sysfs_create_group ret %d.\n", ret);
    return;
}

static void oc_debug_sysfs_exit(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj,  (const struct attribute_group *)&oc_debug_sysfs_group);
    FPGA_I2C_DEBUG_VERBOSE("sysfs_remove_group.\n");
    return;
}

static int rg_ocores_i2c_probe(struct platform_device *pdev)
{
    struct ocores_i2c *i2c;
    struct rg_ocores_i2c_platform_data *pdata;
    struct resource *res;
    int irq;
    int ret;
    int i;

    FPGA_I2C_DEBUG_VERBOSE("Enter.\n");
    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        FPGA_I2C_DEBUG_ERROR("platform_get_irq failed irq %d.\n", irq);
        return irq;
    }

    i2c = devm_kzalloc(&pdev->dev, sizeof(*i2c), GFP_KERNEL);
    if (!i2c) {
        FPGA_I2C_DEBUG_ERROR("devm_kzalloc failed.\n");
        return -ENOMEM;
    }
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    i2c->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(i2c->base)) {
        FPGA_I2C_DEBUG_ERROR("devm_ioremap_resource failed.\n");
        return PTR_ERR(i2c->base);
    }

    pdata = dev_get_platdata(&pdev->dev);
    if (pdata) {
        i2c->reg_shift = pdata->reg_shift;
        i2c->reg_io_width = pdata->reg_io_width;
        i2c->clock_khz = pdata->clock_khz;
    } else {
        ret = ocores_i2c_of_probe(pdev, i2c);
        if (ret)
            return ret;
    }

    if (i2c->reg_io_width == 0)
        i2c->reg_io_width = 1; /* Set to default value */


    if (!i2c->setreg || !i2c->getreg) {
        switch (i2c->reg_io_width) {
            case 1:
                i2c->setreg = oc_setreg_8;
                i2c->getreg = oc_getreg_8;
                break;

            case 2:
                i2c->setreg = oc_setreg_16;
                i2c->getreg = oc_getreg_16;
                break;

            case 4:
                i2c->setreg = oc_setreg_32;
                i2c->getreg = oc_getreg_32;
                break;

            default:
                dev_err(&pdev->dev, "Unsupported I/O width (%d)\n",
                        i2c->reg_io_width);
                return -EINVAL;
        }
    }

    ocores_init(i2c);

    init_waitqueue_head(&i2c->wait);
    ret = devm_request_irq(&pdev->dev, irq, ocores_isr, 0,
            pdev->name, i2c);
    if (ret) {
        dev_err(&pdev->dev, "Cannot claim IRQ\n");
        return ret;
    }

    /* hook up driver to tree */
    platform_set_drvdata(pdev, i2c);
    i2c->adap = ocores_adapter;
    if (pdata->nr) {
        i2c->adap.nr = pdata->nr;
        dev_info(&pdev->dev, "fpga ocores nr is (%d), irq %d \n", i2c->adap.nr, irq);
    }
    i2c_set_adapdata(&i2c->adap, i2c);
    i2c->adap.dev.parent = &pdev->dev;
    i2c->adap.dev.of_node = pdev->dev.of_node;

    /* add i2c adapter to i2c tree */
    ret = i2c_add_numbered_adapter(&i2c->adap);
    if (ret) {
        dev_err(&pdev->dev, "Failed to add adapter\n");
        return ret;
    }

    /* add in known devices to the bus */
    if (pdata) {
        for (i = 0; i < pdata->num_devices; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
            i2c_new_client_device(&i2c->adap, pdata->devices + i);
#else
            i2c_new_device(&i2c->adap, pdata->devices + i);
#endif
        }
    }

    oc_debug_sysfs_init(pdev);
    oc_scl_clk_sysfs_init(&i2c->adap);
    return 0;
}

static int rg_ocores_i2c_remove(struct platform_device *pdev)
{
    struct ocores_i2c *i2c = platform_get_drvdata(pdev);

    /* disable i2c logic */
    oc_setreg(i2c, OCI2C_CONTROL, oc_getreg(i2c, OCI2C_CONTROL)
            & ~(OCI2C_CTRL_EN|OCI2C_CTRL_IEN));

    /* remove adapter & data */
    oc_scl_clk_sysfs_exit(&i2c->adap);
    i2c_del_adapter(&i2c->adap);
    oc_debug_sysfs_exit(pdev);

    return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ocores_i2c_suspend(struct device *dev)
{
    struct ocores_i2c *i2c = dev_get_drvdata(dev);
    u8 ctrl = oc_getreg(i2c, OCI2C_CONTROL);

    /* make sure the device is disabled */
    oc_setreg(i2c, OCI2C_CONTROL, ctrl & ~(OCI2C_CTRL_EN|OCI2C_CTRL_IEN));

    return 0;
}

static int ocores_i2c_resume(struct device *dev)
{
    struct ocores_i2c *i2c = dev_get_drvdata(dev);

    ocores_init(i2c);

    return 0;
}

static SIMPLE_DEV_PM_OPS(ocores_i2c_pm, ocores_i2c_suspend, ocores_i2c_resume);
#define OCORES_I2C_PM   (&ocores_i2c_pm)
#else
#define OCORES_I2C_PM   NULL
#endif

static struct platform_driver ocores_i2c_driver = {
    .probe   = rg_ocores_i2c_probe,
    .remove  = rg_ocores_i2c_remove,
    .driver  = {
        .owner = THIS_MODULE,
        .name = "rg-i2c-ocores",
        .of_match_table = ocores_i2c_match,
        .pm = OCORES_I2C_PM,
    },
};

module_platform_driver(ocores_i2c_driver);

MODULE_AUTHOR("Peter Korsgaard <jacmet@sunsite.dk>");
MODULE_DESCRIPTION("OpenCores I2C bus driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ocores-i2c");
