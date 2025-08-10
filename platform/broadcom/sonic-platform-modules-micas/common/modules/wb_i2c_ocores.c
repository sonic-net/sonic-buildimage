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

#define OCI2C_STAT_IF         (0x01)
#define OCI2C_STAT_TIP        (0x02)
#define OCI2C_STAT_ARBLOST    (0x20)
#define OCI2C_STAT_BUSY       (0x40)
#define OCI2C_STAT_NACK       (0x80)

#define STATE_DONE            (0)
#define STATE_START           (1)
#define STATE_WRITE           (2)
#define STATE_READ            (3)
#define STATE_ERROR           (4)

#define TYPE_OCORES           (0)
#define TYPE_GRLIB            (1)

#define OCORE_WAIT_SCH        (40)
#define REG_IO_WIDTH_1        (1)
#define REG_IO_WIDTH_2        (2)
#define REG_IO_WIDTH_4        (4)

#define SYMBOL_I2C_DEV_MODE   (1)
#define FILE_MODE             (2)
#define SYMBOL_PCIE_DEV_MODE  (3)
#define SYMBOL_IO_DEV_MODE    (4)

typedef struct wb_pci_dev_s {
    uint32_t domain;
    uint32_t bus;
    uint32_t slot;
    uint32_t fn;
} wb_pci_dev_t;

/*
 * 'process_lock' exists because ocores_process() and ocores_process_timeout()
 * can't run in parallel.
 */
struct ocores_i2c {
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
};

int g_wb_ocores_i2c_debug = 0;
int g_wb_ocores_i2c_error = 0;
int g_wb_ocores_i2c_xfer = 0;

module_param(g_wb_ocores_i2c_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_ocores_i2c_error, int, S_IRUGO | S_IWUSR);
module_param(g_wb_ocores_i2c_xfer, int, S_IRUGO | S_IWUSR);

#define OCORES_I2C_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_ocores_i2c_debug) { \
        printk(KERN_INFO "[OCORES_I2C][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define OCORES_I2C_ERROR(fmt, args...) do {                                        \
    if (g_wb_ocores_i2c_error) { \
        printk(KERN_ERR "[OCORES_I2C][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define OCORES_I2C_XFER(fmt, args...) do {                                        \
    if (g_wb_ocores_i2c_xfer) { \
        printk(KERN_INFO "[OCORES_I2C][XFER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

extern int i2c_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int i2c_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int pcie_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int pcie_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int io_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
extern int io_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);
#if 0
int __attribute__((weak)) i2c_device_func_read(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    OCORES_I2C_ERROR("enter __weak i2c func read\r\n");
    return -EINVAL;
}

int __attribute__((weak)) i2c_device_func_write(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    OCORES_I2C_ERROR("enter __weak i2c func write\r\n");
    return -EINVAL;
}

int __attribute__((weak)) pcie_device_func_read(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    OCORES_I2C_ERROR("enter __weak pcie func read\r\n");
    return -EINVAL;
}

int __attribute__((weak)) pcie_device_func_write(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    OCORES_I2C_ERROR("enter __weak pcie func write\r\n");
    return -EINVAL;
}

int __attribute__((weak)) io_device_func_read(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    OCORES_I2C_ERROR("enter __weak io func read\r\n");
    return -EINVAL;
}

int __attribute__((weak)) io_device_func_write(const char *path, uint32_t offset,
                              uint8_t *buf, size_t count)
{
    OCORES_I2C_ERROR("enter __weak io func write\r\n");
    return -EINVAL;
}
#endif
static int ocores_i2c_file_read(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        OCORES_I2C_ERROR("read open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_read(filp, val, size, &tmp_pos);
    if (ret < 0) {
        OCORES_I2C_ERROR("kernel_read failed, path=%s, addr=%d, size=%ld, ret=%d\r\n", path, pos, size, ret);
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

static int ocores_i2c_file_write(const char *path, uint32_t pos, uint8_t *val, size_t size)
{

    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        OCORES_I2C_ERROR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_write(filp, val, size, &tmp_pos);
    if (ret < 0) {
        OCORES_I2C_ERROR("kernel_write failed, path=%s, addr=%d, size=%ld, ret=%d\r\n", path, pos, size, ret);
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

static int ocores_i2c_reg_write(struct ocores_i2c *i2c, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;

    switch (i2c->reg_access_mode) {
    case SYMBOL_I2C_DEV_MODE:
        ret = i2c_device_func_write(i2c->dev_name, pos, val, size);
        break;
    case FILE_MODE:
        ret = ocores_i2c_file_write(i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_PCIE_DEV_MODE:
        ret = pcie_device_func_write(i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_IO_DEV_MODE:
        ret = io_device_func_write(i2c->dev_name, pos, val, size);
        break;
    default:
        OCORES_I2C_ERROR("err func_mode, write failed.\n");
        return -EINVAL;
    }

    return ret;
}

static int ocores_i2c_reg_read(struct ocores_i2c *i2c, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;

    switch (i2c->reg_access_mode) {
    case SYMBOL_I2C_DEV_MODE:
        ret = i2c_device_func_read(i2c->dev_name, pos, val, size);
        break;
    case FILE_MODE:
        ret = ocores_i2c_file_read(i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_PCIE_DEV_MODE:
        ret = pcie_device_func_read(i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_IO_DEV_MODE:
        ret = io_device_func_read(i2c->dev_name, pos, val, size);
        break;
    default:
        OCORES_I2C_ERROR("err func_mode, read failed.\n");
        return -EINVAL;
    }

    return ret;
}
static void oc_setreg_8(struct ocores_i2c *i2c, int reg, u32 value)
{
    u8 buf_tmp[REG_IO_WIDTH_1];
    u32 pos;

    pos = i2c->base_addr + (reg << i2c->reg_shift);
    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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
    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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
    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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
    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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
    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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
    ocores_i2c_reg_read(i2c, pos, buf_tmp, REG_IO_WIDTH_1);
    value = buf_tmp[0];

    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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

    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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
    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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

    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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

    OCORES_I2C_VERBOSE("path:%s, access mode:%d, pos:0x%x, value0x%x.\n",
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

    OCORES_I2C_XFER("Enter nr %d.\n", i2c->adap.nr);
    if ((i2c->state == STATE_DONE) || (i2c->state == STATE_ERROR)) {
        /* stop has been sent */
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
        wake_up(&i2c->wait);
        OCORES_I2C_XFER("stop has been sent, exit.\n");
        goto out;
    }

    /* error? */
    if (stat & OCI2C_STAT_ARBLOST) {
        i2c->state = STATE_ERROR;
        oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
        OCORES_I2C_XFER("error exit, lose arbitration.\n");
        goto out;
    }

    if (ocores_msg_check(i2c->msg, i2c->nmsgs) != 0) {
        OCORES_I2C_XFER("msg buf is NULL\n");
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
            OCORES_I2C_XFER("OCI2C_STAT_NACK, exit.\n");
            goto out;
        }
    } else {
        msg->buf[i2c->pos++] = oc_getreg(i2c, OCI2C_DATA);
    }

    /* end of msg? */
    if (i2c->pos == msg->len) {
        OCORES_I2C_XFER("Enter end of msg.\n");
        i2c->nmsgs--;
        i2c->msg++;
        i2c->pos = 0;
        msg = i2c->msg;

        if (i2c->nmsgs) {    /* end? */
            /* send start? */
            if (!(msg->flags & I2C_M_NOSTART)) {
                u8 addr = i2c_8bit_addr_from_msg(msg);

                i2c->state = STATE_START;

                oc_setreg(i2c, OCI2C_DATA, addr);
                oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_START);
                OCORES_I2C_XFER("send start, exit.\n");
                goto out;
            }
            i2c->state = (msg->flags & I2C_M_RD)
                ? STATE_READ : STATE_WRITE;
        } else {
            i2c->state = STATE_DONE;
            oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
            OCORES_I2C_XFER("send OCI2C_CMD_STOP, exit.\n");
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
    OCORES_I2C_XFER("normal, exit nr %d.\n", i2c->adap.nr);
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
    stat = oc_getreg(i2c, OCI2C_STATUS);
    if (!(stat & OCI2C_STAT_IF)) {
        spin_unlock_irqrestore(&i2c->process_lock, flags);
        return IRQ_NONE;
    }
    OCORES_I2C_XFER("Enter, irq %d nr %d addr 0x%x.\n", irq, i2c->adap.nr, (!i2c->msg)? 0 : i2c->msg->addr);
    ocores_process(i2c, stat);
    OCORES_I2C_XFER("Leave, irq %d nr %d addr 0x%x.\n", irq, i2c->adap.nr, (!i2c->msg)? 0 : i2c->msg->addr);
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
        jiffies_tmp = jiffies;
        status = oc_getreg(i2c, reg);

        if ((status & mask) == val) {
            break;
        }

        if (time_after(jiffies_tmp, j)) {
            OCORES_I2C_XFER("STATUS timeout, mask[0x%x]  val[0x%x] status[0x%x]\n", mask, val, status);
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
         OCORES_I2C_XFER("STATUS timeout, bit 0x%x did not clear in 100ms, err %d\n", mask, err);
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
        if (ret == IRQ_NONE) {
            break; /* all messages have been transferred */
        }
    }

    return err;
}

static int ocores_xfer_core(struct ocores_i2c *i2c,
                struct i2c_msg *msgs, int num,
                bool polling)
{
    int ret;
    u8 ctrl;
    unsigned long flags;

    OCORES_I2C_VERBOSE("Enter ocores_xfer_core. polling mode:%d.\n", polling);
    spin_lock_irqsave(&i2c->process_lock, flags);

    ctrl = oc_getreg(i2c, OCI2C_CONTROL);
    if (polling) {
        oc_setreg(i2c, OCI2C_CONTROL, ctrl & ~OCI2C_CTRL_IEN);
    } else {
        oc_setreg(i2c, OCI2C_CONTROL, ctrl | OCI2C_CTRL_IEN);
    }

    i2c->msg = msgs;
    i2c->pos = 0;
    i2c->nmsgs = num;
    i2c->state = STATE_START;

    oc_setreg(i2c, OCI2C_DATA, i2c_8bit_addr_from_msg(i2c->msg));
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
    int ret;

    OCORES_I2C_VERBOSE("Enter ocores_xfer.\n");
    if (!adap || ocores_msg_check(msgs, num)) {
        OCORES_I2C_ERROR("[MAY BE USER SPACE ERROR]:msg buf is NULL\n");
        return -EFAULT;
    }
    OCORES_I2C_VERBOSE("i2c bus:%d, msgs num:%d.\n", adap->nr, num);

    i2c = i2c_get_adapdata(adap);

    if (i2c->flags & OCORES_FLAG_POLL) {
        ret = ocores_xfer_core(i2c, msgs, num, true);
    } else {
        ret = ocores_xfer_core(i2c, msgs, num, false);
    }

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
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
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
            OCORES_I2C_ERROR("dts config error, ret:%d.\n", ret);
            ret = -EINVAL;
            return ret;
        }
    } else {
        if (i2c->dev->platform_data == NULL) {
            OCORES_I2C_ERROR("Failed to get platform data config.\n");
            ret = -EINVAL;
            return ret;
        }
        i2c_ocores_device = i2c->dev->platform_data;
        wb_pci_dev->domain = i2c_ocores_device->pci_domain;
        wb_pci_dev->bus = i2c_ocores_device->pci_bus;
        wb_pci_dev->slot = i2c_ocores_device->pci_slot;
        wb_pci_dev->fn = i2c_ocores_device->pci_fn;
    }

    OCORES_I2C_VERBOSE("pci_domain:0x%x, pci_bus:0x%x, pci_slot:0x%x, pci_fn:0x%x.\n",
        wb_pci_dev->domain, wb_pci_dev->bus, wb_pci_dev->slot, wb_pci_dev->fn);

    devfn = PCI_DEVFN(wb_pci_dev->slot, wb_pci_dev->fn);
    pci_dev = pci_get_domain_bus_and_slot(wb_pci_dev->domain, wb_pci_dev->bus, devfn);
    if (pci_dev == NULL) {
        OCORES_I2C_ERROR("Failed to find pci_dev, domain:0x%04x, bus:0x%02x, devfn:0x%x\n",
            wb_pci_dev->domain, wb_pci_dev->bus, devfn);
        return -ENODEV;
    }
    irq = pci_dev->irq + i2c->irq_offset;
    OCORES_I2C_VERBOSE("get irq no:%d.\n", irq);
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
            OCORES_I2C_ERROR("dts config error, ret:%d.\n", ret);
            ret = -ENXIO;
            return ret;
        }
    } else {
        if (i2c->dev->platform_data == NULL) {
            OCORES_I2C_ERROR("Failed to get platform data config.\n");
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

    OCORES_I2C_VERBOSE("name:%s, base:0x%x, reg_shift:0x%x, io_width:0x%x, ip_clock_khz:0x%x, bus_clock_khz:0x%x.\n",
        i2c->dev_name, i2c->base_addr, i2c->reg_shift, i2c->reg_io_width, i2c->ip_clock_khz, i2c->bus_clock_khz);
    OCORES_I2C_VERBOSE("reg access mode:%d.\n", i2c->reg_access_mode);
    return ret;
}

static int ocores_i2c_probe(struct platform_device *pdev)
{
    struct ocores_i2c *i2c;
    int irq, ret;
    bool be;
    i2c_ocores_device_t *i2c_ocores_device;

    OCORES_I2C_VERBOSE("Enter main probe\n");

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
            OCORES_I2C_VERBOSE("Failed to get platform data config, set OCORES_FLAG_POLL.\n");
        } else {
            i2c_ocores_device = i2c->dev->platform_data;
            if (i2c_ocores_device->irq_type == 0) {

                i2c->flags |= OCORES_FLAG_POLL;
            } else {

                irq = fpga_ocores_i2c_get_irq(i2c);
                if (irq < 0 ) {
                    dev_err(i2c->dev, "Failed to get  ocores i2c irq number, ret: %d.\n", irq);
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
    OCORES_I2C_VERBOSE("Main probe out\n");
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

    /* disable i2c logic */
    ctrl &= ~(OCI2C_CTRL_EN | OCI2C_CTRL_IEN);
    oc_setreg(i2c, OCI2C_CONTROL, ctrl);

    /* remove adapter & data */
    i2c_del_adapter(&i2c->adap);
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
