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
#include "lpc_cpld_i2c_ocores.h"
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/version.h>

#define OCORES_FLAG_POLL BIT(0)

struct ocores_i2c {
    void __iomem *base;
    u32 reg_shift;
    u32 reg_io_width;
    unsigned long flags;
    wait_queue_head_t wait;
    struct i2c_adapter adap;
    struct i2c_msg *msg;
    int pos;
    int nmsgs;
    int state; /* see STATE_ */
    spinlock_t process_lock;
    int clock_khz;
    void (*setreg)(struct ocores_i2c *i2c, int reg, u8 value);
    u8 (*getreg)(struct ocores_i2c *i2c, int reg);
};

/* registers */
#define OCI2C_PRELOW        0x0
#define OCI2C_PREHIGH       0x1
#define OCI2C_CONTROL       0x2
#define OCI2C_DATA          0x3
#define OCI2C_CMD           0x4 /* write only */
#define OCI2C_STATUS        0x4 /* read only, same address as OCI2C_CMD */

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
#define OCI2C_WAIT_SLEEP    40

int g_lpc_cpld_i2c_debug = 0;
int g_lpc_cpld_i2c_irq = 0;
int g_lpc_cpld_i2c_error = 0;

module_param(g_lpc_cpld_i2c_debug, int, S_IRUGO | S_IWUSR);
module_param(g_lpc_cpld_i2c_error, int, S_IRUGO | S_IWUSR);
module_param(g_lpc_cpld_i2c_irq, int, S_IRUGO | S_IWUSR);

int g_irq_dump_debug = 0;
module_param(g_irq_dump_debug, int, S_IRUGO | S_IWUSR);
#define LPC_CPLD_I2C_DEBUG_DUMP(fmt, args...) do {                                        \
    if (g_irq_dump_debug) { \
        printk(KERN_ERR ""fmt, ## args); \
    } \
} while (0)
int g_irq_invalid_cnt = 0;
module_param(g_irq_invalid_cnt, int, S_IRUGO | S_IWUSR);
#define LPC_CPLD_I2C_DEBUG_XFER(fmt, args...) do {                                        \
    if (g_lpc_cpld_i2c_irq) { \
        printk(KERN_ERR "[LPC_CPLD_I2C_OCORES][XFER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define LPC_CPLD_I2C_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_lpc_cpld_i2c_debug) { \
        printk(KERN_ERR "[LPC_CPLD_I2C_OCORES][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define LPC_CPLD_I2C_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_lpc_cpld_i2c_error) { \
        printk(KERN_ERR "[LPC_CPLD_I2C_OCORES][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static int g_lpc_cpld_i2c_irq_flag = 1;

module_param(g_lpc_cpld_i2c_irq_flag, int, S_IRUGO | S_IWUSR);

static void oc_debug_dump_reg(struct ocores_i2c *i2c);
static void oc_setreg_8(struct ocores_i2c *i2c, int reg, u8 value)
{
    u64 base = (u64)i2c->base;

    outb(value, (u16)base + reg);
}

static inline u8 oc_getreg_8(struct ocores_i2c *i2c, int reg)
{
    u64 base = (u64)i2c->base;

    return inb((u16)base + reg);
}

static inline void oc_setreg(struct ocores_i2c *i2c, int reg, u8 value)
{
    i2c->setreg(i2c, reg, value);
}

static inline u8 oc_getreg(struct ocores_i2c *i2c, int reg)
{
    u8 status;

    status = i2c->getreg(i2c, reg);
    return status;
}

#define LPC_CPLD_I2C_SPIN_LOCK(lock, flags) spin_lock_irqsave(&(lock), (flags))
#define LPC_CPLD_I2C_SPIN_UNLOCK(lock, flags) spin_unlock_irqrestore(&(lock), (flags))

static void ocores_process(struct ocores_i2c *i2c, u8 stat)
{
    struct i2c_msg *msg = i2c->msg;

    LPC_CPLD_I2C_DEBUG_XFER("Enter nr %d.\n", i2c->adap.nr);

    /*
     * If we spin here is because we are in timeout, so we are going
     * to be in STATE_ERROR. See ocores_process_timeout()
     */
    if ((i2c->state == STATE_DONE) || (i2c->state == STATE_ERROR)) {
        /* stop has been sent */
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
        wake_up(&i2c->wait);
        LPC_CPLD_I2C_DEBUG_XFER("stop has been sent, exit.\n");
        goto out;
    }

    /* error */
    if (stat & OCI2C_STAT_ARBLOST) {
        i2c->state = STATE_ERROR;
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
        LPC_CPLD_I2C_DEBUG_XFER("error, exit.\n");
        goto out;
    }

    if ((i2c->state == STATE_START) || (i2c->state == STATE_WRITE)) {
        i2c->state =
            (msg->flags & I2C_M_RD) ? STATE_READ : STATE_WRITE;

        if (stat & OCI2C_STAT_NACK) {
            i2c->state = STATE_ERROR;
            oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
            LPC_CPLD_I2C_DEBUG_XFER("OCI2C_STAT_NACK, exit.\n");
            goto out;
        }
    } else
        msg->buf[i2c->pos++] = oc_getreg(i2c, OCI2C_DATA);

    /* end of msg */
    if (i2c->pos == msg->len) {
        LPC_CPLD_I2C_DEBUG_XFER("Enter end of msg.\n");
        i2c->nmsgs--;
        i2c->msg++;
        i2c->pos = 0;
        msg = i2c->msg;

        if (i2c->nmsgs) {   /* end? */
            /* send start */
            if (!(msg->flags & I2C_M_NOSTART)) {
                u8 addr = (msg->addr << 1);

                if (msg->flags & I2C_M_RD)
                    addr |= 1;

                i2c->state = STATE_START;

                oc_setreg(i2c, OCI2C_DATA, addr);
                oc_setreg(i2c, OCI2C_CMD,  OCI2C_CMD_START);
                LPC_CPLD_I2C_DEBUG_XFER("send start, exit.\n");
                goto out;
            }

            i2c->state = (msg->flags & I2C_M_RD)
                ? STATE_READ : STATE_WRITE;
        } else {
            i2c->state = STATE_DONE;
            oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
            LPC_CPLD_I2C_DEBUG_XFER("send OCI2C_CMD_STOP, exit.\n");
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
    LPC_CPLD_I2C_DEBUG_XFER("normal, exit nr %d.\n", i2c->adap.nr);
}

static irqreturn_t ocores_isr(int irq, void *dev_id)
{
    struct ocores_i2c *i2c = dev_id;    
    unsigned long flags;   
    u8 stat;
    if (!i2c) {
        return IRQ_NONE;
    }

    LPC_CPLD_I2C_SPIN_LOCK(i2c->process_lock, flags);
    stat = oc_getreg(i2c, OCI2C_STATUS);

    if (!(stat & OCI2C_STAT_IF)) {
        g_irq_invalid_cnt++;
        LPC_CPLD_I2C_SPIN_UNLOCK(i2c->process_lock, flags);
        return IRQ_NONE;
    }

    LPC_CPLD_I2C_DEBUG_XFER("Enter, irq %d nr %d addr 0x%x.\n", irq, i2c->adap.nr, i2c->msg->addr);
    ocores_process(i2c, stat);
    LPC_CPLD_I2C_DEBUG_XFER("Leave, irq %d nr %d addr 0x%x.\n", irq, i2c->adap.nr, i2c->msg->addr);
    LPC_CPLD_I2C_SPIN_UNLOCK(i2c->process_lock, flags);

    return IRQ_HANDLED;
}

/**
 * Process timeout event
 * @i2c: ocores I2C device instance
 */
static void ocores_process_timeout(struct ocores_i2c *i2c)
{
    unsigned long flags;

    LPC_CPLD_I2C_SPIN_LOCK(i2c->process_lock, flags);
    i2c->state = STATE_ERROR;
    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
    mdelay(1);
    LPC_CPLD_I2C_SPIN_UNLOCK(i2c->process_lock, flags);

}

/**
 * Wait until something change in a given register
 * @i2c: ocores I2C device instance
 * @reg: register to query
 * @mask: bitmask to apply on register value
 * @val: expected result
 * @timeout: timeout in jiffies
 *
 * Timeout is necessary to avoid to stay here forever when the chip
 * does not answer correctly.
 *
 * Return: 0 on success, -ETIMEDOUT on timeout
 */
static int ocores_wait(struct ocores_i2c *i2c,
        int reg, u8 mask, u8 val,
        const unsigned long timeout)
{
    u8 status;
    unsigned long j, jiffies_tmp;
    unsigned int usleep;
    usleep = OCI2C_WAIT_SLEEP;
    j = jiffies + timeout;
    while (1) {
        jiffies_tmp = jiffies;
        status = oc_getreg(i2c, reg);

        if ((status & mask) == val)
            break;

        if (time_after(jiffies_tmp, j)) {
            LPC_CPLD_I2C_DEBUG_XFER("STATUS timeout, mask[0x%x]  val[0x%x] status[0x%x]\n", mask, val, status);
            return -ETIMEDOUT;
        }
        usleep_range(usleep,usleep + 1);
    }
    return 0;
}

/**
 * Wait until is possible to process some data
 * @i2c: ocores I2C device instance
 *
 * Used when the device is in polling mode (interrupts disabled).
 *
 * Return: 0 on success, -ETIMEDOUT on timeout
 */
static int ocores_poll_wait(struct ocores_i2c *i2c)
{
    u8 mask;
    int err;

    if (i2c->state == STATE_DONE || i2c->state == STATE_ERROR) {
        /* transfer is over */
        mask = OCI2C_STAT_BUSY;
    } else {
        /* on going transfer */
        mask = OCI2C_STAT_TIP;
        udelay((8 * 1000) / i2c->clock_khz);
    }

    /*
     * once we are here we expect to get the expected result immediately
     * so if after 1ms we timeout then something is broken.
     */
    err = ocores_wait(i2c, OCI2C_STATUS, mask, 0, msecs_to_jiffies(100));
    if (err) {
        LPC_CPLD_I2C_DEBUG_XFER("STATUS timeout, bit 0x%x did not clear in 1ms, err %d\n", mask, err);
    }

    return err;
}


/**
 * It handles an IRQ-less transfer
 * @i2c: ocores I2C device instance
 *
 * Even if IRQ are disabled, the I2C OpenCore IP behavior is exactly the same
 * (only that IRQ are not produced). This means that we can re-use entirely
 * ocores_isr(), we just add our polling code around it.
 *
 * It can run in atomic context
 */
static int ocores_process_polling(struct ocores_i2c *i2c)
{
    irqreturn_t ret;
    int err;
    while (1) {
        err = ocores_poll_wait(i2c);
        if (err) {
            i2c->state = STATE_ERROR;
            break; /* timeout */
        }

        ret = ocores_isr(-1, i2c);
        if (ret == IRQ_NONE)
            break; /* all messages have been transfered */
    }
    return err;
}

static int ocores_xfer_core(struct ocores_i2c *i2c,
        struct i2c_msg *msgs, int num,
        bool polling)
{
    int ret;
    unsigned long flags;
    u8 ctrl;

    LPC_CPLD_I2C_DEBUG_XFER("Enter.polling %d\n", polling); 
    LPC_CPLD_I2C_SPIN_LOCK(i2c->process_lock, flags);   
    ctrl = oc_getreg(i2c, OCI2C_CONTROL);
    if (polling)
        oc_setreg(i2c, OCI2C_CONTROL, ctrl & ~OCI2C_CTRL_IEN);
    else
        oc_setreg(i2c, OCI2C_CONTROL, ctrl | OCI2C_CTRL_IEN);

    i2c->msg = msgs;
    i2c->pos = 0;
    i2c->nmsgs = num;
    i2c->state = STATE_START;

    oc_setreg(i2c, OCI2C_DATA,
            (i2c->msg->addr << 1) |
            ((i2c->msg->flags & I2C_M_RD) ? 1:0));

    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_START);
    LPC_CPLD_I2C_SPIN_UNLOCK(i2c->process_lock, flags);

    if (polling) {
        ret = ocores_process_polling(i2c);
        if (ret) {      /* timeout */
            ocores_process_timeout(i2c);
            return -ETIMEDOUT;
        }
    } else {
        ret = wait_event_timeout(i2c->wait,
                (i2c->state == STATE_ERROR) ||
                (i2c->state == STATE_DONE), HZ);
        if (ret == 0) {
            ocores_process_timeout(i2c);
            return -ETIMEDOUT;
        }
    }

    return (i2c->state == STATE_DONE) ? num : -EIO;
}

static int ocores_xfer_polling(struct i2c_adapter *adap,
        struct i2c_msg *msgs, int num)
{
    LPC_CPLD_I2C_DEBUG_XFER("Enter.\n");
    return ocores_xfer_core(i2c_get_adapdata(adap), msgs, num, true);
}

static int ocores_xfer(struct i2c_adapter *adap,
        struct i2c_msg *msgs, int num)
{
    struct ocores_i2c *i2c = i2c_get_adapdata(adap);

    if (i2c->flags & OCORES_FLAG_POLL)
        return ocores_xfer_polling(adap, msgs, num);
    return ocores_xfer_core(i2c, msgs, num, false);
}

static void ocores_init(struct ocores_i2c *i2c)
{
    int prescale;
    u8 ctrl = oc_getreg(i2c, OCI2C_CONTROL);

    LPC_CPLD_I2C_DEBUG_XFER("Enter.\n");
    spin_lock_init(&i2c->process_lock);

    /* make sure the device is disabled */
    oc_setreg(i2c, OCI2C_CONTROL, ctrl & ~(OCI2C_CTRL_EN|OCI2C_CTRL_IEN));

    prescale = (i2c->clock_khz / (5*100)) - 1;
    oc_setreg(i2c, OCI2C_PRELOW, prescale & 0xff);
    oc_setreg(i2c, OCI2C_PREHIGH, prescale >> 8);
    LPC_CPLD_I2C_DEBUG_VERBOSE("i2c->base 0x%p, i2c->clock_khz %d, prescale 0x%x.\n", i2c->base, i2c->clock_khz, prescale);

    /* Init the device */
    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
    oc_setreg(i2c, OCI2C_CONTROL, ctrl | OCI2C_CTRL_EN);
}


static u32 ocores_func(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm  ocores_algorithm = {
    .master_xfer    = ocores_xfer,
    .functionality  = ocores_func,
};

static struct i2c_adapter ocores_adapter = {
    .owner      = THIS_MODULE,
    .name       = "rg-cpld-ocrore-i2c",
    .class      = I2C_CLASS_HWMON | I2C_CLASS_SPD | I2C_CLASS_DEPRECATED,
    .algo       = &ocores_algorithm,
};

static const struct of_device_id ocores_i2c_match[] = {
    {
        .compatible = "opencores,rg-cpld-ocrore-i2c",
        .data = (void *)TYPE_OCORES,
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

    LPC_CPLD_I2C_DEBUG_VERBOSE("Enter ocores_i2c_of_probe.\n");
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

static void oc_debug_dump_reg(struct ocores_i2c *i2c)
{
    if (i2c) {
        LPC_CPLD_I2C_DEBUG_DUMP("base: %p.\n", i2c->base);
        LPC_CPLD_I2C_DEBUG_DUMP("reg_shift: %d.\n", i2c->reg_shift);
        LPC_CPLD_I2C_DEBUG_DUMP("reg_io_width: %d.\n", i2c->reg_io_width);
        LPC_CPLD_I2C_DEBUG_DUMP("adap.nr: %d.\n", i2c->adap.nr);
        LPC_CPLD_I2C_DEBUG_DUMP("msg: %p.\n", i2c->msg);
        if (i2c->msg) {
            LPC_CPLD_I2C_DEBUG_DUMP("msg->buf: %p.\n", i2c->msg->buf);
            LPC_CPLD_I2C_DEBUG_DUMP("msg->addr: 0x%x.\n", i2c->msg->addr);
            LPC_CPLD_I2C_DEBUG_DUMP("msg->flags: 0x%x.\n", i2c->msg->flags);
            LPC_CPLD_I2C_DEBUG_DUMP("msg->len: %d.\n", i2c->msg->len);            
        } else {
            LPC_CPLD_I2C_DEBUG_DUMP("msg: %p is null.\n", i2c->msg);
        }

        LPC_CPLD_I2C_DEBUG_DUMP("pos: %d.\n", i2c->pos);
        LPC_CPLD_I2C_DEBUG_DUMP("nmsgs: %d.\n", i2c->nmsgs);
        LPC_CPLD_I2C_DEBUG_DUMP("state: %d.\n", i2c->state);
        LPC_CPLD_I2C_DEBUG_DUMP("clock_khz: %d.\n", i2c->clock_khz);
        LPC_CPLD_I2C_DEBUG_DUMP("setreg: %p.\n", i2c->setreg);
        LPC_CPLD_I2C_DEBUG_DUMP("getreg: %p.\n", i2c->getreg);
        if (i2c->getreg) {
            LPC_CPLD_I2C_DEBUG_DUMP("OCI2C_PRELOW: 0x%02x.\n", oc_getreg(i2c, OCI2C_PRELOW));
            LPC_CPLD_I2C_DEBUG_DUMP("OCI2C_PREHIGH: 0x%02x.\n", oc_getreg(i2c, OCI2C_PREHIGH));
            LPC_CPLD_I2C_DEBUG_DUMP("OCI2C_CONTROL: 0x%02x.\n", oc_getreg(i2c, OCI2C_CONTROL));
            LPC_CPLD_I2C_DEBUG_DUMP("OCI2C_DATA: 0x%02x.\n", oc_getreg(i2c, OCI2C_DATA));
            LPC_CPLD_I2C_DEBUG_DUMP("OCI2C_CMD: 0x%02x.\n", oc_getreg(i2c, OCI2C_CMD));
            LPC_CPLD_I2C_DEBUG_DUMP("OCI2C_STATUS: 0x%02x.\n", oc_getreg(i2c, OCI2C_STATUS));
        } else {
            LPC_CPLD_I2C_DEBUG_DUMP("getreg: %p is null.\n", i2c->getreg);
        }
    } else {
        LPC_CPLD_I2C_DEBUG_DUMP("i2c %p is null.\n", i2c);
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
                LPC_CPLD_I2C_DEBUG_DUMP("bus %d call oc_debug_dump_reg begin.\n", bus);
                oc_debug_dump_reg(adap_data);
                LPC_CPLD_I2C_DEBUG_DUMP("bus %d call oc_debug_dump_reg end.\n", bus);
            } else {
                LPC_CPLD_I2C_DEBUG_DUMP("bus %d i2c_get_adapdata null.\n", bus);
            }
            i2c_put_adapter(adap);
        } else {
            LPC_CPLD_I2C_DEBUG_DUMP("bus %d i2c_get_adapter null.\n", bus);
        }
    }
}

static ssize_t show_oc_debug_value(struct device *dev, struct device_attribute *da, char *buf)
{
    oc_debug_dump_reg_exception();
    return 0;
}

static SENSOR_DEVICE_ATTR(oc_debug, S_IRUGO | S_IWUSR, show_oc_debug_value, NULL, 0x15);

static struct attribute *oc_debug_sysfs_attrs[] = {
    &sensor_dev_attr_oc_debug.dev_attr.attr,
    NULL
};

static const struct attribute_group oc_debug_sysfs_group = {
    .attrs = oc_debug_sysfs_attrs,
};

static void oc_debug_sysfs_init(struct platform_device *pdev)
{
    int ret;

    ret = sysfs_create_group(&pdev->dev.kobj, &oc_debug_sysfs_group);
    LPC_CPLD_I2C_DEBUG_VERBOSE("sysfs_create_group ret %d.\n", ret);
    return;
}

static void oc_debug_sysfs_exit(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj,  (const struct attribute_group *)&oc_debug_sysfs_group);
    LPC_CPLD_I2C_DEBUG_VERBOSE("sysfs_remove_group.\n");
    return;
}

static int rg_ocores_i2c_probe(struct platform_device *pdev)
{
    struct ocores_i2c *i2c;
    struct rg_ocores_cpld_i2c_platform_data *pdata;
    struct resource *res;
    int irq;
    int ret;
    int i;

    LPC_CPLD_I2C_DEBUG_VERBOSE("Enter.\n");

    i2c = devm_kzalloc(&pdev->dev, sizeof(*i2c), GFP_KERNEL);
    if (!i2c) {
        LPC_CPLD_I2C_DEBUG_ERROR("devm_kzalloc failed.\n");
        return -ENOMEM;
    }
    res = platform_get_resource(pdev, IORESOURCE_IO, 0);
    if (!res) {
        LPC_CPLD_I2C_DEBUG_ERROR("can't fetch device resource info\n");
        return -ENOMEM;
    }

    i2c->base = (void __iomem *)res->start;
    LPC_CPLD_I2C_DEBUG_VERBOSE("i2c->base is %p., res->end[%d]\n", i2c->base, (int)res->end);

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

    LPC_CPLD_I2C_DEBUG_VERBOSE("data: shift[%d], width[%d], clock_khz[%d] i2c_irq_flag=%d\n", 
            pdata->reg_shift, pdata->reg_io_width, pdata->clock_khz, pdata->i2c_irq_flag);

    if (i2c->reg_io_width == 0)
        i2c->reg_io_width = 1; /* Set to default value */


    if (!i2c->setreg || !i2c->getreg) {
        switch (i2c->reg_io_width) {
            case 1:
                i2c->setreg = oc_setreg_8;
                i2c->getreg = oc_getreg_8;
                break;
            default:
                dev_err(&pdev->dev, "Unsupported I/O width (%d)\n",
                        i2c->reg_io_width);
                return -EINVAL;
        }
    }

    init_waitqueue_head(&i2c->wait);

    irq = platform_get_irq(pdev, 0);
    LPC_CPLD_I2C_DEBUG_VERBOSE("get irq %d, ENXIO[%d]", irq, ENXIO);
    if (irq == -ENXIO) {
        i2c->flags |= OCORES_FLAG_POLL;
    } else if(g_lpc_cpld_i2c_irq_flag){
        ret = devm_request_irq(&pdev->dev, irq, ocores_isr, 0,
                pdev->name, i2c);
        if (ret) {
            dev_err(&pdev->dev, "Cannot claim IRQ\n");
        }

        if(pdata->i2c_irq_flag) {
            g_lpc_cpld_i2c_irq_flag = 0;
        }
    }

    ocores_init(i2c);

    /* hook up driver to tree */
    platform_set_drvdata(pdev, i2c);
    i2c->adap = ocores_adapter;
    i2c_set_adapdata(&i2c->adap, i2c);
    i2c->adap.dev.parent = &pdev->dev;
    i2c->adap.dev.of_node = pdev->dev.of_node;

    /* add i2c adapter to i2c tree */
    ret = i2c_add_adapter(&i2c->adap);
    if (ret) {
        dev_err(&pdev->dev, "Failed to add adapter\n");
        return ret;
    }

    /* add in known devices to the bus */
    if (pdata) {
        LPC_CPLD_I2C_DEBUG_VERBOSE("i2c device %d.\n", pdata->num_devices);
        for (i = 0; i < pdata->num_devices; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
            i2c_new_client_device(&i2c->adap, pdata->devices + i);
#else
            i2c_new_device(&i2c->adap, pdata->devices + i);
#endif
        }
    }

    oc_debug_sysfs_init(pdev);
    return 0;
}

static int rg_ocores_i2c_remove(struct platform_device *pdev)
{
    struct ocores_i2c *i2c = platform_get_drvdata(pdev);

    /* disable i2c logic */
    oc_setreg(i2c, OCI2C_CONTROL, oc_getreg(i2c, OCI2C_CONTROL)
            & ~(OCI2C_CTRL_EN|OCI2C_CTRL_IEN));

    /* remove adapter & data */
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
        .name = "rg-cpld-ocrore-i2c",
        .of_match_table = ocores_i2c_match,
        .pm = OCORES_I2C_PM,
    },
};

module_platform_driver(ocores_i2c_driver);

MODULE_AUTHOR("Peter Korsgaard <jacmet@sunsite.dk>");
MODULE_DESCRIPTION("OpenCores I2C bus driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ocores-i2c");
