// SPDX-License-Identifier: GPL-2.0
/*
 * i2c-ocores.c: I2C bus driver for OpenCores I2C controller
 * (https://opencores.org/project/i2c/overview)
 *
 * Peter Korsgaard <peter@korsgaard.com>
 *
 * Support for the GRLIB port of the controller by
 * Andreas Larsson <andreas@gaisler.com>
 */

#include <linux/clk.h>
#include <linux/delay.h>
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
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "wb_i2c_ocores.h"
#include <wb_logic_dev_common.h>
#include <wb_bsp_i2c_debug.h>

#define OCORES_FLAG_POLL      BIT(0)

/* registers */
#define OCI2C_PRELOW          (0)
#define OCI2C_PREHIGH         (1)
#define OCI2C_CONTROL         (2)
#define OCI2C_DATA            (3)
#define OCI2C_CMD             (4) /* write only */
#define OCI2C_STATUS          (4) /* read only, same address as OCI2C_CMD */

#define OCI2C_CTRL_IEN        (0x40)
#define OCI2C_CTRL_EN         (0x80)

#define OCI2C_CMD_START       (0x91)
#define OCI2C_CMD_STOP        (0x41)
#define OCI2C_CMD_READ        (0x21)
#define OCI2C_CMD_WRITE       (0x11)
#define OCI2C_CMD_READ_ACK    (0x21)
#define OCI2C_CMD_READ_NACK   (0x29)
#define OCI2C_CMD_IACK        (0x01)
#define OCI2C_CMD_RD_STOP     (0x61)

#define OCI2C_STAT_IF         (0x01)
#define OCI2C_STAT_TIP        (0x02)
#define OCI2C_STAT_ARBLOST    (0x20)
#define OCI2C_STAT_BUSY       (0x40)
#define OCI2C_STAT_NACK       (0x80)

#define TYPE_OCORES           (0)
#define TYPE_GRLIB            (1)

#define OCORE_WAIT_SCH        (40)
#define REG_IO_WIDTH_1        (1)
#define REG_IO_WIDTH_2        (2)
#define REG_IO_WIDTH_4        (4)
#define I2C_WAIT_RD_STOP_TIMEOUT_MS   (1)

#define I2C_STATE_UNKNOWN "STATE_UNKNOWN"

typedef enum {
    STATE_DONE  = 0,
    STATE_START = 1,
    STATE_WRITE = 2,
    STATE_READ  = 3,
    STATE_ERROR = 4,
} ocores_state_t;

static const char * const i2c_state_strings[] = {
    "STATE_DONE",
    "STATE_START",
    "STATE_WRITE",
    "STATE_READ",
    "STATE_ERROR",
};

char *stri2cstate(int state)
{
    if ((state >= 0) && (state < ARRAY_SIZE(i2c_state_strings))) {
        return (char *)i2c_state_strings[state];
    }

    return I2C_STATE_UNKNOWN;
}

typedef struct wb_pci_dev_s {
    uint32_t domain;
    uint32_t bus;
    uint32_t slot;
    uint32_t fn;
    uint32_t check_pci_id;
    uint32_t pci_id;
} wb_pci_dev_t;

/*
 * 'process_lock' exists because ocores_process() and ocores_process_timeout()
 * can't run in parallel.
 */
struct ocores_i2c {
    /* struct i2c_adapter_debug must be the first member */
    struct i2c_adapter_debug i2c_ada_dbg;
    uint32_t base_addr;
    uint32_t reg_shift;
    uint32_t reg_io_width;
    unsigned long flags;
    wait_queue_head_t wait;
    struct i2c_adapter adap;
    int adap_nr;
    struct i2c_msg *msg;
    int pos;
    int nmsgs;
    int state;
    spinlock_t process_lock;
    uint32_t ip_clock_khz;
    uint32_t bus_clock_khz;
    void (*setreg)(struct ocores_i2c *i2c, int reg, u32 value);
    u32 (*getreg)(struct ocores_i2c *i2c, int reg);
    const char *dev_name;
    uint32_t reg_access_mode;
    uint32_t big_endian;
    uint32_t irq_offset;
    wb_pci_dev_t wb_pci_dev;
    struct device *dev;
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
};

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#if 0
int __attribute__((weak)) i2c_device_func_read(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    DEBUG_ERROR("enter __weak i2c func read\r\n");
    return -EINVAL;
}

int __attribute__((weak)) i2c_device_func_write(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    DEBUG_ERROR("enter __weak i2c func write\r\n");
    return -EINVAL;
}

int __attribute__((weak)) pcie_device_func_read(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    DEBUG_ERROR("enter __weak pcie func read\r\n");
    return -EINVAL;
}

int __attribute__((weak)) pcie_device_func_write(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    DEBUG_ERROR("enter __weak pcie func write\r\n");
    return -EINVAL;
}

int __attribute__((weak)) io_device_func_read(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    DEBUG_ERROR("enter __weak io func read\r\n");
    return -EINVAL;
}

int __attribute__((weak)) io_device_func_write(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    DEBUG_ERROR("enter __weak io func write\r\n");
    return -EINVAL;
}
#endif

static int ocores_i2c_reg_write(struct ocores_i2c *i2c, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_write pfunc;

    pfunc = (device_func_write)i2c->write_intf_addr;
    return pfunc(i2c->dev_name, pos, val, size);
}

static int ocores_i2c_reg_read(struct ocores_i2c *i2c, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    pfunc = (device_func_read)i2c->read_intf_addr;
    return pfunc(i2c->dev_name, pos, val, size);
}
static void oc_setreg_8(struct ocores_i2c *i2c, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_1];
    u32 pos;

    pos = i2c->base_addr + (reg << i2c->reg_shift);
    DEBUG_INFO("reg write, path: %s, access mode: %d, offset: 0x%x, write value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);

    buf_tmp[0] = (value & 0Xff);
    ocores_i2c_reg_write(i2c, pos, buf_tmp, REG_IO_WIDTH_1);
    return;
}

static void oc_setreg_16(struct ocores_i2c *i2c, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_2];
    u32 pos;

    pos = i2c->base_addr + (reg << i2c->reg_shift);
    DEBUG_INFO("reg write, path: %s, access mode: %d, offset: 0x%x, write value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);

    buf_tmp[0] = (value & 0Xff);
    buf_tmp[1] = (value >> 8) & 0xff;
    ocores_i2c_reg_write(i2c, pos, buf_tmp, REG_IO_WIDTH_2);
    return;
}

static void oc_setreg_32(struct ocores_i2c *i2c, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_4];
    u32 pos;

    pos = i2c->base_addr + (reg << i2c->reg_shift);
    DEBUG_INFO("reg write, path: %s, access mode: %d, offset: 0x%x, write value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);

    buf_tmp[0] = (value & 0xff);
    buf_tmp[1] = (value >> 8) & 0xff;
    buf_tmp[2] = (value >> 16) & 0xff;
    buf_tmp[3] = (value >> 24) & 0xff;

    ocores_i2c_reg_write(i2c, pos, buf_tmp, REG_IO_WIDTH_4);
    return;
}

static void oc_setreg_16be(struct ocores_i2c *i2c, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_2];
    u32 pos;

    pos = i2c->base_addr + (reg << i2c->reg_shift);
    DEBUG_INFO("reg write, path: %s, access mode: %d, offset: 0x%x, write value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);

    buf_tmp[0] = (value >> 8) & 0xff;
    buf_tmp[1] = (value & 0Xff);
    ocores_i2c_reg_write(i2c, pos, buf_tmp, REG_IO_WIDTH_2);
    return;
}

static void oc_setreg_32be(struct ocores_i2c *i2c, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_4];
    u32 pos;

    pos = i2c->base_addr + (reg << i2c->reg_shift);
    DEBUG_INFO("reg write, path: %s, access mode: %d, offset: 0x%x, write value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);

    buf_tmp[0] = (value >> 24) & 0xff;
    buf_tmp[1] = (value >> 16) & 0xff;
    buf_tmp[2] = (value >> 8) & 0xff;
    buf_tmp[3] = (value & 0xff);
    ocores_i2c_reg_write(i2c, pos, buf_tmp, REG_IO_WIDTH_4);
    return;
}

static inline u32 oc_getreg_8(struct ocores_i2c *i2c, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_1];
    u32 value, pos;

    pos = i2c->base_addr + (reg << i2c->reg_shift);
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ocores_i2c_reg_read(i2c, pos, buf_tmp, REG_IO_WIDTH_1);
    value = buf_tmp[0];

    DEBUG_INFO("reg read, path: %s, access mode: %d, offset: 0x%x, read value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);

    return value;
}

static inline u32 oc_getreg_16(struct ocores_i2c *i2c, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_2];
    u32 value, pos;
    int i;

    pos = i2c->base_addr + (reg << i2c->reg_shift);
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ocores_i2c_reg_read(i2c, pos, buf_tmp, REG_IO_WIDTH_2);

    value = 0;
    for (i = 0; i < REG_IO_WIDTH_2 ; i++) {
        value |= buf_tmp[i] << (8 * i);
    }

    DEBUG_INFO("reg read, path: %s, access mode: %d, offset: 0x%x, read value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);
    return value;
}

static inline u32 oc_getreg_32(struct ocores_i2c *i2c, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_4];
    u32 value, pos;
    int i;

    pos = i2c->base_addr + (reg << i2c->reg_shift);
    mem_clear(buf_tmp, sizeof(buf_tmp));
    ocores_i2c_reg_read(i2c, pos, buf_tmp, REG_IO_WIDTH_4);

    value = 0;
    for (i = 0; i < REG_IO_WIDTH_4 ; i++) {
        value |= buf_tmp[i] << (8 * i);
    }
    DEBUG_INFO("reg read, path: %s, access mode: %d, offset: 0x%x, read value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);
    return value;
}

static inline u32 oc_getreg_16be(struct ocores_i2c *i2c, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_2];
    u32 value, pos;
    int i;

    pos = i2c->base_addr + (reg << i2c->reg_shift);

    mem_clear(buf_tmp, sizeof(buf_tmp));
    ocores_i2c_reg_read(i2c, pos, buf_tmp, REG_IO_WIDTH_2);

    value = 0;
    for (i = 0; i < REG_IO_WIDTH_2 ; i++) {
        value |= buf_tmp[i] << (8 * (REG_IO_WIDTH_2 -i - 1));
    }

    DEBUG_INFO("reg read, path: %s, access mode: %d, offset: 0x%x, read value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);
    return value;
}

static inline u32 oc_getreg_32be(struct ocores_i2c *i2c, int reg)
{
    u8 buf_tmp[REG_IO_WIDTH_4];
    u32 value, pos;
    int i;

    pos = i2c->base_addr + (reg << i2c->reg_shift);

    mem_clear(buf_tmp, sizeof(buf_tmp));
    ocores_i2c_reg_read(i2c, pos, buf_tmp, REG_IO_WIDTH_4);

    value = 0;
    for (i = 0; i < REG_IO_WIDTH_4 ; i++) {
        value |= buf_tmp[i] << (8 * (REG_IO_WIDTH_4 -i - 1));
    }

    DEBUG_INFO("reg read, path: %s, access mode: %d, offset: 0x%x, read value: 0x%x\n",
        i2c->dev_name, i2c->reg_access_mode, pos, value);
    return value;

}

static inline void oc_setreg(struct ocores_i2c *i2c, int reg, u32 value)
{
    i2c->setreg(i2c, reg, value);
    return;
}

static inline u32 oc_getreg(struct ocores_i2c *i2c, int reg)
{
    return i2c->getreg(i2c, reg);
}

static int ocores_msg_check(struct i2c_msg *msgs, int num)
{
    int i, ret = 0;

    if (!msgs) {
        ret = -EFAULT;
        goto out;
    }

    for (i = 0; i < num; ++i) {
        if (!msgs[i].buf) {
            ret = -EFAULT;
            goto out;
        }
    }

out:
    return ret;
}

static void ocores_process(struct ocores_i2c *i2c, u8 stat)
{
    struct i2c_msg *msg = i2c->msg;
    u8 val;

    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, "Enter i2c-%d ocores_process, current remaining nmsgs: %d\n",
        i2c->adap.nr, i2c->nmsgs);
    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, "Current i2c->state: %s(%d), ocores status: 0x%02x\n",
        stri2cstate(i2c->state), i2c->state, stat);
    if ((i2c->state == STATE_DONE) || (i2c->state == STATE_ERROR)) {
        /* stop has been sent */
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "i2c->state: %s(%d), Write CMD_IACK to Command register and finish i2c operation\n",
            stri2cstate(i2c->state), i2c->state);
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
        wake_up(&i2c->wait);
        goto out;
    }

    /* error? */
    if (stat & OCI2C_STAT_ARBLOST) {
        i2c->state = STATE_ERROR;
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "[Arbitration lost]ocores status: 0x%02x, set i2c->state to: %s(%d) and Write CMD_STOP to Command register\n",
            stat, stri2cstate(i2c->state), i2c->state);
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
        goto out;
    }

    if (ocores_msg_check(i2c->msg, i2c->nmsgs) != 0) {
        i2c->state = STATE_ERROR;
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "ocores_msg_check failed, msg buf is NULL, set i2c->state to: %s(%d) and Write CMD_STOP to Command register\n",
            stri2cstate(i2c->state), i2c->state);
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
        goto out;
    }

    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
        "Current msg slave addr: 0x%02x, msg flags: 0x%04x, msg len: %d, i2c->pos: %d\n",
        msg->addr, msg->flags, msg->len, i2c->pos);

    if ((i2c->state == STATE_START) || (i2c->state == STATE_WRITE)) {
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "i2c->state: %s(%d), check if ACK is received\n", stri2cstate(i2c->state), i2c->state);
        i2c->state = (msg->flags & I2C_M_RD) ? STATE_READ : STATE_WRITE;
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "Set i2c->state to: %s(%d), according to i2c msg flags: 0x%04x\n",
            stri2cstate(i2c->state), i2c->state, msg->flags);
        if (stat & OCI2C_STAT_NACK) {
            i2c->state = STATE_ERROR;
            DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                "[NACK]ocores status: 0x%02x, set i2c->state to: %s(%d) and Write CMD_STOP to Command register\n",
                stat, stri2cstate(i2c->state), i2c->state);
            oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
            goto out;
        }
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, "Check ACK ok, recevice ACK\n");
    } else {
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "i2c->state: %s(%d), read Receive register\n", stri2cstate(i2c->state), i2c->state);
        val = oc_getreg(i2c, OCI2C_DATA);
        msg->buf[i2c->pos++] = val;
        /* Some SMBus transactions require that we receive the
           transaction length as the first read byte. */
        if ((i2c->pos == 1) && (msg->flags & I2C_M_RECV_LEN)) {
            if ((val <= 0) || (val > I2C_SMBUS_BLOCK_MAX)) {
                i2c->state = STATE_ERROR;
                DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                    "Invalid SMBus block read len: %d, set i2c->state to: %s(%d) and Write CMD_STOP to Command register\n",
                    val, stri2cstate(i2c->state), i2c->state);
                oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
                goto out;
            }
            msg->len += val;
            DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                "SMBus block read len: %d, msg->len adjust to: %d \n", val, msg->len);
        }
    }

    /* end of msg? */
    if (i2c->pos == msg->len) {
        i2c->nmsgs--;
        i2c->msg++;
        i2c->pos = 0;
        msg = i2c->msg;
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "End of i2c msg, current remaining nmsgs: %d\n", i2c->nmsgs);
        if (i2c->nmsgs) {    /* end? */
            /* send start? */
            if (!(msg->flags & I2C_M_NOSTART)) {
                u8 addr = i2c_8bit_addr_from_msg(msg);
                i2c->state = STATE_START;
                DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                    "Deal next i2c msg, set i2c->state: %s(%d) and Write i2c 8bit addr 0x%02x to Transmit register\n",
                    stri2cstate(i2c->state), i2c->state, addr);
                oc_setreg(i2c, OCI2C_DATA, addr);
                DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                    "Write CMD_START to Command register and start i2c %s operation\n",
                    msg->flags & I2C_M_RD ? "read" : "write");
                oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_START);
                goto out;
            }
            i2c->state = (msg->flags & I2C_M_RD) ? STATE_READ : STATE_WRITE;
            DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                "Deal next i2c msg with I2C_M_NOSTART, set i2c->state to: %s(%d)\n",
                stri2cstate(i2c->state), i2c->state);
        } else {
            i2c->state = STATE_DONE;
            DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                "End of all i2c msgs, set i2c->state to: %s(%d) and Write CMD_STOP to Command register\n",
                stri2cstate(i2c->state), i2c->state);
            oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
            goto out;
        }
    }

    if (i2c->state == STATE_READ) {
        /* SMBus block read, msg->flags=I2C_M_RECV_LEN
         * Reading the first byte data requires sending an ACK
         */
         if ((i2c->pos == 0) && (msg->flags & I2C_M_RECV_LEN)) {
            DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                "i2c->state: %s(%d), SMBus block read the first byte with READ_ACK to Command register\n",
                stri2cstate(i2c->state), i2c->state);
             oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_READ_ACK);
         } else {
             if (i2c->pos == (msg->len - 1)) {
                val = OCI2C_CMD_READ_NACK;
                DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                    "i2c->state: %s(%d), i2c msg len: %d, i2c->pos: %d, read the last byte with READ_NACK to Command register\n",
                    stri2cstate(i2c->state), i2c->state, msg->len, i2c->pos);
             } else {
                val = OCI2C_CMD_READ_ACK;
                DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                    "i2c->state: %s(%d), i2c msg len: %d, i2c->pos: %d, read the data with READ_ACK to Command register\n",
                    stri2cstate(i2c->state), i2c->state, msg->len, i2c->pos);
             }
             oc_setreg(i2c, OCI2C_CMD, val);
         }
    } else {
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "i2c->state: %s(%d), Write data to Transmit register\n", stri2cstate(i2c->state),
            i2c->state);
        oc_setreg(i2c, OCI2C_DATA, msg->buf[i2c->pos++]);
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "Write CMD_WRITE to Command register and start i2c write operation\n");
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_WRITE);
    }

out:
    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, "Normal exit i2c-%d ocores_process, i2c->state: %s(%d)\n",
        i2c->adap.nr, stri2cstate(i2c->state), i2c->state);
    return;
}

static irqreturn_t ocores_isr(int irq, void *dev_id)
{
    struct ocores_i2c *i2c = dev_id;
    u8 stat;
    unsigned long flags;

    if (!i2c) {
        return IRQ_NONE;
    }

    spin_lock_irqsave(&i2c->process_lock, flags);
    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
        "Read Status register to check if byte transfer is completed\n");
    stat = oc_getreg(i2c, OCI2C_STATUS);
    if (!(stat & OCI2C_STAT_IF)) {
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "Status register value: 0x%02x, Interrupt flag not set, IRQ_NONE\n", stat);
        spin_unlock_irqrestore(&i2c->process_lock, flags);
        return IRQ_NONE;
    }
    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
        "Status register value: 0x%02x, byte transfer completed, start ocores_process\n", stat);
    ocores_process(i2c, stat);
    spin_unlock_irqrestore(&i2c->process_lock, flags);

    return IRQ_HANDLED;
}

/**
 * Process timeout event
 * @i2c: ocores I2C device instance
 */
static void ocores_process_timeout(struct ocores_i2c *i2c)
{
    unsigned long flags;

    spin_lock_irqsave(&i2c->process_lock, flags);
    i2c->state = STATE_ERROR;
    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
        "ocores_process_timeout, set i2c->state to: %s(%d) and Write CMD_STOP to Command register\n",
        stri2cstate(i2c->state), i2c->state);
    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
    mdelay(1);
    spin_unlock_irqrestore(&i2c->process_lock, flags);
    return;
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

    usleep = OCORE_WAIT_SCH;
    j = jiffies + timeout;
    while (1) {
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "Read Status register to check if the ocores status is ok, mask: 0x%02x, except value: 0x%02x\n",
            mask, val);
        jiffies_tmp = jiffies;
        status = oc_getreg(i2c, reg);
        if ((status & mask) == val) {
            DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                "Ocores wait ok, ocores status: 0x%02x, mask: 0x%02x, except value: 0x%02x\n",
                status, mask, val);
            break;
        }

        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "Ocores wait retry, ocores status: 0x%02x, mask: 0x%02x, except value: 0x%02x\n",
            status, mask, val);

        if (time_after(jiffies_tmp, j)) {
            DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                "Ocores wait timeout, ocores status: 0x%02x, mask: 0x%02x, except value: 0x%02x\n",
                status, mask, val);
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
        /*
         * We wait for the data to be transferred (8bit),
         * then we start polling on the ACK/NACK bit
         */
        udelay((8 * 1000) / i2c->bus_clock_khz);
    }

    /*
     * once we are here we expect to get the expected result immediately
     * so if after 100ms we timeout then something is broken.
     */
    err = ocores_wait(i2c, OCI2C_STATUS, mask, 0, msecs_to_jiffies(100));
    if (err) {
         DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "ocores status timeout, bit 0x%x did not clear in 100ms, err %d\n", mask, err);
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
            DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                "ocores_poll_wait timeout, ret: %d, set i2c->state to: %s(%d)\n",
                err, stri2cstate(i2c->state), i2c->state);
            break; /* timeout */
        }

        ret = ocores_isr(-1, i2c);
        if (ret == IRQ_NONE) {
            DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
                "All messages have been transferred, exit ocores_process_polling\n");
            break; /* all messages have been transferred */
        }
    }

    return err;
}

static void ocores_i2c_unblock(struct ocores_i2c *i2c)
{
    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_RD_STOP);
    return;
}

static void ocores_check_i2c_unblock(struct ocores_i2c *i2c, bool polling)
{
    u8 stat, ctrl;
    unsigned long flags;
    int err;

    spin_lock_irqsave(&i2c->process_lock, flags);
    /* I2C Arbitration lost (Deadlock detection) */
    stat = oc_getreg(i2c, OCI2C_STATUS);
    if (!(stat & OCI2C_STAT_ARBLOST)) {
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
            "ocores dev_name: %s, base_addr: 0x%x, status: 0x%x, not Arbitration lost\n",
            i2c->dev_name, i2c->base_addr, stat);
        spin_unlock_irqrestore(&i2c->process_lock, flags);
        return;
    }
    DEBUG_WARN_I2C_ADAPTER(&i2c->adap, 
        "ocores dev_name: %s, base_addr: 0x%x, status: 0x%x, Arbitration lost, try to send 9 clock\n",
        i2c->dev_name, i2c->base_addr, stat);
    /* Disable interrupts first in interrupt mode */
    if (!polling) {
        ctrl = oc_getreg(i2c, OCI2C_CONTROL);
        oc_setreg(i2c, OCI2C_CONTROL, ctrl & ~OCI2C_CTRL_IEN);
    }
    /* Sending RD + STOP triggers 9 clock */
    ocores_i2c_unblock(i2c);
    spin_unlock_irqrestore(&i2c->process_lock, flags);

    /* Wait until RD + STOP completes sending */
    err = ocores_wait(i2c, OCI2C_STATUS, OCI2C_STAT_TIP, 0, msecs_to_jiffies(I2C_WAIT_RD_STOP_TIMEOUT_MS));
    if (err) {
        DEBUG_WARN_I2C_ADAPTER(&i2c->adap, 
            "ocores dev_name: %s, base_addr: 0x%x, wait  transfer complete timeout in %dms, err: %d\n",
            i2c->dev_name, i2c->base_addr, I2C_WAIT_RD_STOP_TIMEOUT_MS, err);
    }

    /* After RD + STOP is sent, clear the interrupt flag bit */
    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
    return;
}

static int ocores_xfer_core(struct ocores_i2c *i2c,
                struct i2c_msg *msgs, int num,
                bool polling)
{
    int ret;
    u8 ctrl;
    unsigned long flags;

    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, "Enter ocores_xfer_core, polling mode: %d\n", polling);
    /* I2C deadlock detection and 9clock mechanism */
    ocores_check_i2c_unblock(i2c, polling);
    spin_lock_irqsave(&i2c->process_lock, flags);

    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, "Read Control register\n");
    ctrl = oc_getreg(i2c, OCI2C_CONTROL);
    if (polling) {
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, "Polling mode, write Control register to disable irq\n");
        oc_setreg(i2c, OCI2C_CONTROL, ctrl & ~OCI2C_CTRL_IEN);
    } else {
        DEBUG_INFO_I2C_ADAPTER(&i2c->adap, "Irq mode, write Control register to enable irq\n");
        oc_setreg(i2c, OCI2C_CONTROL, ctrl | OCI2C_CTRL_IEN);
    }

    i2c->msg = msgs;
    i2c->pos = 0;
    i2c->nmsgs = num;
    i2c->state = STATE_START;
    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
        "Set i2c->state: %s(%d) and Write i2c 8bit addr 0x%02x to Transmit register\n",
        stri2cstate(i2c->state), i2c->state, i2c_8bit_addr_from_msg(i2c->msg));
    oc_setreg(i2c, OCI2C_DATA, i2c_8bit_addr_from_msg(i2c->msg));
    DEBUG_INFO_I2C_ADAPTER(&i2c->adap, 
        "Write CMD_START to Command register and start i2c %s operation\n",
        i2c->msg->flags & I2C_M_RD ? "read" : "write");
    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_START);

    spin_unlock_irqrestore(&i2c->process_lock, flags);

    if (polling) {
        ret = ocores_process_polling(i2c);
        if (ret) {
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

static int ocores_xfer(struct i2c_adapter *adap,
               struct i2c_msg *msgs, int num)
{
    struct ocores_i2c *i2c;
    int ret, i;

    DEBUG_INFO_I2C_ADAPTER(adap, "Enter ocores_xfer.\n");
    if (!adap || ocores_msg_check(msgs, num)) {
        DEBUG_ERROR_I2C_ADAPTER(adap, "[MAYBE USER SPACE ERROR]: msg buf is NULL\n");
        return -EFAULT;
    }
    DEBUG_INFO_I2C_ADAPTER(adap, "i2c-%d, ocores_xfer total msgs num: %d\n", adap->nr, num);
    for (i = 0; i < num; i++) {
        DEBUG_INFO_I2C_ADAPTER(adap, "msg[%d] slave addr: 0x%02x, msg flags: 0x%04x, msg len: %d\n",
            i, msgs[i].addr, msgs[i].flags, msgs[i].len);
    }

    i2c = i2c_get_adapdata(adap);

    /* Enter xfer, Initialize the controller status to DONE */
    i2c->state = STATE_DONE;
    ret = ocores_poll_wait(i2c);
    if (ret) {
        DEBUG_INFO_I2C_ADAPTER(adap, "Enter ocores_xfer and find adapter:%d addr:0x%x is busy.\n",
            adap->nr, i2c->msg->addr);
        return ret;
    }

    if (i2c->flags & OCORES_FLAG_POLL) {
        ret = ocores_xfer_core(i2c, msgs, num, true);
    } else {
        ret = ocores_xfer_core(i2c, msgs, num, false);
    }

    DEBUG_INFO_I2C_ADAPTER(adap, "Exit ocores_xfer, ret: %d\n", ret);
    return ret;
}

static int ocores_init(struct device *dev, struct ocores_i2c *i2c)
{
    int prescale;
    int diff;
    u8 ctrl = oc_getreg(i2c, OCI2C_CONTROL);

    /* make sure the device is disabled */
    ctrl &= ~(OCI2C_CTRL_EN | OCI2C_CTRL_IEN);
    oc_setreg(i2c, OCI2C_CONTROL, ctrl);

    prescale = (i2c->ip_clock_khz / (5 * i2c->bus_clock_khz)) - 1;
    prescale = clamp(prescale, 0, 0xffff);

    diff = i2c->ip_clock_khz / (5 * (prescale + 1)) - i2c->bus_clock_khz;
    if (abs(diff) > i2c->bus_clock_khz / 10) {
        dev_err(dev, "Unsupported clock settings: core: %d KHz, bus: %d KHz\n",
            i2c->ip_clock_khz, i2c->bus_clock_khz);
        return -EINVAL;
    }

    oc_setreg(i2c, OCI2C_PRELOW, prescale & 0xff);
    oc_setreg(i2c, OCI2C_PREHIGH, prescale >> 8);

    /* Init the device */
    oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
    oc_setreg(i2c, OCI2C_CONTROL, ctrl | OCI2C_CTRL_EN);

    return 0;
}

static u32 ocores_func(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static const struct i2c_algorithm ocores_algorithm = {
    .master_xfer = ocores_xfer,
    .functionality = ocores_func,
};

static const struct i2c_adapter ocores_adapter = {
    .owner = THIS_MODULE,
    .name = "wb-i2c-ocores",
    .class = I2C_CLASS_DEPRECATED,
    .algo = &ocores_algorithm,
};

static const struct of_device_id ocores_i2c_match[] = {
    {
        .compatible = "opencores,wb-i2c-ocores",
        .data = (void *)TYPE_OCORES,
    },
    {},
};
MODULE_DEVICE_TABLE(of, ocores_i2c_match);

static int fpga_ocores_i2c_get_irq(struct ocores_i2c *i2c)
{
    int devfn, irq;
    uint32_t pci_id;
    struct device *dev;
    wb_pci_dev_t *wb_pci_dev;
    struct pci_dev *pci_dev;
    i2c_ocores_device_t *i2c_ocores_device;
    int ret;

    dev = i2c->dev;
    wb_pci_dev = &i2c->wb_pci_dev;

    if (dev->of_node) {
        ret = 0;
        ret += of_property_read_u32(dev->of_node, "pci_domain", &wb_pci_dev->domain);
        ret += of_property_read_u32(dev->of_node, "pci_bus", &wb_pci_dev->bus);
        ret += of_property_read_u32(dev->of_node, "pci_slot", &wb_pci_dev->slot);
        ret += of_property_read_u32(dev->of_node, "pci_fn", &wb_pci_dev->fn);

        if (ret != 0) {
            DEBUG_ERROR("dts config error, ret:%d.\n", ret);
            ret = -EINVAL;
            return ret;
        }
        ret = of_property_read_u32(dev->of_node, "check_pci_id", &wb_pci_dev->check_pci_id);
        if (ret == 0) {
            ret = of_property_read_u32(dev->of_node, "pci_id", &wb_pci_dev->pci_id);
            if (ret != 0) {
                DEBUG_ERROR("need to check pci id, but pci id not config.\n");
                return -EINVAL;
            }
        }
    } else {
        if (i2c->dev->platform_data == NULL) {
            DEBUG_ERROR("Failed to get platform data config.\n");
            ret = -EINVAL;
            return ret;
        }
        i2c_ocores_device = i2c->dev->platform_data;
        wb_pci_dev->domain = i2c_ocores_device->pci_domain;
        wb_pci_dev->bus = i2c_ocores_device->pci_bus;
        wb_pci_dev->slot = i2c_ocores_device->pci_slot;
        wb_pci_dev->fn = i2c_ocores_device->pci_fn;
        wb_pci_dev->check_pci_id = i2c_ocores_device->check_pci_id;
        wb_pci_dev->pci_id = i2c_ocores_device->pci_id;
    }

    DEBUG_VERBOSE("pci_domain:0x%x, pci_bus:0x%x, pci_slot:0x%x, pci_fn:0x%x.\n",
        wb_pci_dev->domain, wb_pci_dev->bus, wb_pci_dev->slot, wb_pci_dev->fn);

    devfn = PCI_DEVFN(wb_pci_dev->slot, wb_pci_dev->fn);
    pci_dev = pci_get_domain_bus_and_slot(wb_pci_dev->domain, wb_pci_dev->bus, devfn);
    if (pci_dev == NULL) {
        DEBUG_ERROR("Failed to find pci_dev, domain:0x%04x, bus:0x%02x, devfn:0x%x\n",
            wb_pci_dev->domain, wb_pci_dev->bus, devfn);
        return -ENODEV;
    }
    if (wb_pci_dev->check_pci_id == 1) {
        pci_id = (pci_dev->vendor << 16) | pci_dev->device;
        if (wb_pci_dev->pci_id != pci_id) {
            DEBUG_ERROR("Failed to check pci id, except: 0x%x, really: 0x%x\n",
                wb_pci_dev->pci_id, pci_id);
            return -ENXIO;
        }
        DEBUG_VERBOSE("pci id check ok, pci_id: 0x%x", pci_id);
    }

    irq = pci_dev->irq + i2c->irq_offset;
    DEBUG_VERBOSE("get irq no: %d.\n", irq);
    return irq;
}

static int ocores_i2c_config_init(struct ocores_i2c *i2c)
{
    int ret;
    struct device *dev;
    i2c_ocores_device_t *i2c_ocores_device;

    dev = i2c->dev;
    ret = 0;

    if (dev->of_node) {
        ret += of_property_read_string(dev->of_node, "dev_name", &i2c->dev_name);
        ret += of_property_read_u32(dev->of_node, "dev_base", &i2c->base_addr);
        ret += of_property_read_u32(dev->of_node, "reg_shift", &i2c->reg_shift);
        ret += of_property_read_u32(dev->of_node, "reg_io_width", &i2c->reg_io_width);
        ret += of_property_read_u32(dev->of_node, "ip_clock_khz", &i2c->ip_clock_khz);
        ret += of_property_read_u32(dev->of_node, "bus_clock_khz", &i2c->bus_clock_khz);
        ret += of_property_read_u32(dev->of_node, "reg_access_mode", &i2c->reg_access_mode);

        if (ret != 0) {
            DEBUG_ERROR("dts config error, ret:%d.\n", ret);
            ret = -ENXIO;
            return ret;
        }
    } else {
        if (i2c->dev->platform_data == NULL) {
            DEBUG_ERROR("Failed to get platform data config.\n");
            ret = -ENXIO;
            return ret;
        }
        i2c_ocores_device = i2c->dev->platform_data;
        i2c->dev_name = i2c_ocores_device->dev_name;
        i2c->adap_nr = i2c_ocores_device->adap_nr;
        i2c->big_endian = i2c_ocores_device->big_endian;
        i2c->base_addr = i2c_ocores_device->dev_base;
        i2c->reg_shift = i2c_ocores_device->reg_shift;
        i2c->reg_io_width = i2c_ocores_device->reg_io_width;
        i2c->ip_clock_khz = i2c_ocores_device->ip_clock_khz;
        i2c->bus_clock_khz = i2c_ocores_device->bus_clock_khz;
        i2c->reg_access_mode = i2c_ocores_device->reg_access_mode;
    }

    ret = find_intf_addr(&i2c->write_intf_addr, &i2c->read_intf_addr, i2c->reg_access_mode);
    if (ret) {
        dev_err(dev, "find_intf_addr func mode %d fail, ret: %d.\n", i2c->reg_access_mode, ret);
        return ret;
    }

    if (!i2c->write_intf_addr || !i2c->read_intf_addr) {
        DEBUG_ERROR("Fail: func mode %u rw symbol undefined.\n", i2c->reg_access_mode);
        return -ENOSYS;
    }

    DEBUG_VERBOSE("name:%s, base:0x%x, reg_shift:0x%x, io_width:0x%x, ip_clock_khz:0x%x, bus_clock_khz:0x%x.\n",
        i2c->dev_name, i2c->base_addr, i2c->reg_shift, i2c->reg_io_width, i2c->ip_clock_khz, i2c->bus_clock_khz);
    DEBUG_VERBOSE("reg access mode:%d.\n", i2c->reg_access_mode);
    return ret;
}

static int ocores_i2c_probe(struct platform_device *pdev)
{
    struct ocores_i2c *i2c;
    int irq, ret;
    bool be;
    i2c_ocores_device_t *i2c_ocores_device;

    DEBUG_VERBOSE("Enter main probe\n");

    i2c = devm_kzalloc(&pdev->dev, sizeof(*i2c), GFP_KERNEL);
    if (!i2c) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    spin_lock_init(&i2c->process_lock);

    i2c->dev = &pdev->dev;
    ret = ocores_i2c_config_init(i2c);
    if (ret !=0) {
        dev_err(i2c->dev, "Failed to get ocores i2c dts config.\n");
        goto out;
    }

    if (i2c->dev->of_node) {
        if (of_property_read_u32(i2c->dev->of_node, "big_endian", &i2c->big_endian)) {

            be = 0;
        } else {
            be = i2c->big_endian;
        }
    } else {
        be = i2c->big_endian;
    }

    if (i2c->reg_io_width == 0) {
        i2c->reg_io_width = 1; /* Set to default value */
    }

    if (!i2c->setreg || !i2c->getreg) {
        switch (i2c->reg_io_width) {
        case REG_IO_WIDTH_1:
            i2c->setreg = oc_setreg_8;
            i2c->getreg = oc_getreg_8;
            break;

        case REG_IO_WIDTH_2:
            i2c->setreg = be ? oc_setreg_16be : oc_setreg_16;
            i2c->getreg = be ? oc_getreg_16be : oc_getreg_16;
            break;

        case REG_IO_WIDTH_4:
            i2c->setreg = be ? oc_setreg_32be : oc_setreg_32;
            i2c->getreg = be ? oc_getreg_32be : oc_getreg_32;
            break;

        default:
            dev_err(i2c->dev, "Unsupported I/O width (%d)\n",
                i2c->reg_io_width);
            ret = -EINVAL;
            goto out;
        }
    }

    init_waitqueue_head(&i2c->wait);
    irq = -1;

    if (i2c->dev->of_node) {
        if (of_property_read_u32(i2c->dev->of_node, "irq_offset", &i2c->irq_offset)) {

            i2c->flags |= OCORES_FLAG_POLL;
        } else {

            irq = fpga_ocores_i2c_get_irq(i2c);
            if (irq < 0 ) {
                dev_err(i2c->dev, "Failed to get  ocores i2c irq number, ret: %d.\n", irq);
                ret = irq;
                goto out;
            }
        }
    } else {
        if (i2c->dev->platform_data == NULL) {

            i2c->flags |= OCORES_FLAG_POLL;
            DEBUG_VERBOSE("Failed to get platform data config, set OCORES_FLAG_POLL.\n");
        } else {
            i2c_ocores_device = i2c->dev->platform_data;
            if (i2c_ocores_device->irq_type == 0) {

                i2c->flags |= OCORES_FLAG_POLL;
            } else {
                i2c->irq_offset = i2c_ocores_device->irq_offset;
                irq = fpga_ocores_i2c_get_irq(i2c);
                if (irq < 0 ) {
                    dev_err(i2c->dev, "Failed to get ocores i2c irq number, ret: %d.\n", irq);
                    ret = irq;
                    goto out;
                }
            }
        }
    }

    if (!(i2c->flags & OCORES_FLAG_POLL)) {
        ret = devm_request_irq(&pdev->dev, irq, ocores_isr, 0,
                       pdev->name, i2c);
        if (ret) {
            dev_err(i2c->dev, "Cannot claim IRQ\n");
            goto out;
        }
    }

    ret = ocores_init(i2c->dev, i2c);
    if (ret) {
        goto out;
    }

    /* hook up driver to tree */
    platform_set_drvdata(pdev, i2c);
    i2c->adap = ocores_adapter;
    i2c_set_adapdata(&i2c->adap, i2c);
    i2c->adap.dev.parent = &pdev->dev;
    i2c->adap.dev.of_node = pdev->dev.of_node;

    if (i2c->dev->of_node) {
        /* adap.nr get from dts aliases */
        ret = i2c_add_adapter(&i2c->adap);
    } else {
        i2c->adap.nr = i2c->adap_nr;
        ret = i2c_add_numbered_adapter(&i2c->adap);
    }
    if (ret) {
        goto fail_add;
    }

    I2C_ADAPTER_DEBUG_INIT(&i2c->adap, struct ocores_i2c, i2c_ada_dbg);
    DEBUG_VERBOSE("Main probe out\n");
    dev_info(i2c->dev, "registered i2c-%d for %s with base address:0x%x success.\n",
        i2c->adap.nr, i2c->dev_name, i2c->base_addr);
    return 0;
fail_add:
    platform_set_drvdata(pdev, NULL);
out:
    return ret;
}

static int ocores_i2c_remove(struct platform_device *pdev)
{
    struct ocores_i2c *i2c = platform_get_drvdata(pdev);
    u8 ctrl = oc_getreg(i2c, OCI2C_CONTROL);
    int i2c_bus;

    DEBUG_VERBOSE("Enter ocores_i2c_remove\n");
    i2c_adapter_debug_exit(&i2c->adap);
    i2c_bus = i2c->adap.nr;
    /* disable i2c logic */
    ctrl &= ~(OCI2C_CTRL_EN | OCI2C_CTRL_IEN);
    oc_setreg(i2c, OCI2C_CONTROL, ctrl);

    /* remove adapter & data */
    DEBUG_VERBOSE("Starting unregistered i2c-%d.\n", i2c_bus);
    i2c_del_adapter(&i2c->adap);
    dev_info(&pdev->dev, "Unregistered i2c-%d success.\n", i2c_bus);
    return 0;
}

static struct platform_driver ocores_i2c_driver = {
    .probe   = ocores_i2c_probe,
    .remove  = ocores_i2c_remove,
    .driver  = {
        .name = "wb-ocores-i2c",
        .of_match_table = ocores_i2c_match,
    },
};

module_platform_driver(ocores_i2c_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("OpenCores I2C bus driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ocores-i2c");
