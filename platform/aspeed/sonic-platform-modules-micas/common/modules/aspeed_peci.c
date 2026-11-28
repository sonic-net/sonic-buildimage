/*
 *  driver/misc/aspeed/aspeed_peci.c
 *
 *  ASPEED PECI controller driver
 *
 *  Copyright (C) 2012-2020  ASPEED Technology Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  History:
 *   2013.01.30: Initial version [Ryan Chen]
 *   2016.08.06: Porting to kernel 4.7 [Vadim Pasternak vadimp at mellanox.com]
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/configfs.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <linux/jiffies.h>

int g_peci_debug = 0;
int g_peci_error = 0;
module_param(g_peci_debug, int, S_IRUGO | S_IWUSR);
module_param(g_peci_error, int, S_IRUGO | S_IWUSR);

#define PECI_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_peci_debug) { \
        printk(KERN_ERR "[PECI][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PECI_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_peci_error) { \
        printk(KERN_ERR "[PECI][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PECI_AST2500     (0)
#define PECI_AST2600     (1)

#define PECI_IDLE_CHECK_TIMEOUT_USEC    50000
#define PECI_IDLE_CHECK_INTERVAL_USEC   10000
#define PECI_IDLE_CHECK_TIMEOUT_MS      50
#define PECI_IDLE_CHECK_INTERVAL_MS     10

#define PECI_COMMOND_PASS    0x40

#define SCU_BASE                        0x1E6E2000
#define SCU_SYSTEM_RESET                0x04
#define SCU_PECI_RESET_MASK             (1 << 10)
#define SCU_SYSTEM_RESET_2              0x50
#define SCU_SYSTEM_RESET_CLEAR_2        0x54
#define SCU_PECI_RESET_MASK_AST26       (1 << 4)

/*AST PECI Register Definition */
#define AST_PECI_CTRL		0x00
#define AST_PECI_TIMING		0x04
#define AST_PECI_CMD		0x08
#define AST_PECI_CMD_CTRL	0x0C
#define AST_PECI_EXP_FCS	0x10
#define AST_PECI_CAP_FCS	0x14
#define AST_PECI_INT_CTRL	0x18
#define AST_PECI_INT_STS	0x1C
#define AST_PECI_W_DATA0	0x20
#define AST_PECI_W_DATA1	0x24
#define AST_PECI_W_DATA2	0x28
#define AST_PECI_W_DATA3	0x2c
#define AST_PECI_R_DATA0	0x30
#define AST_PECI_R_DATA1	0x34
#define AST_PECI_R_DATA2	0x38
#define AST_PECI_R_DATA3	0x3c
#define AST_PECI_W_DATA4	0x40
#define AST_PECI_W_DATA5	0x44
#define AST_PECI_W_DATA6	0x48
#define AST_PECI_W_DATA7	0x4c
#define AST_PECI_R_DATA4	0x50
#define AST_PECI_R_DATA5	0x54
#define AST_PECI_R_DATA6	0x58
#define AST_PECI_R_DATA7	0x5c

/* AST_PECI_CTRL - 0x00 : Control Register */
#define PECI_CTRL_SAMPLING_MASK		(0xf << 16)
#define PECI_CTRL_SAMPLING(x)		(x << 16)
#define PECI_CTRL_READ_MODE_MASK	(0xf << 12)
#define PECI_CTRL_CONT_MODE		(1 << 16)
#define PECI_CTRL_DBG_MODE		(2 << 16)
#define PECI_CTRL_CLK_SOURCE		(0x1 << 11) /*0: 24Mhz, 1: MCLK */
#define PECI_CTRL_CLK_DIV_MASK		(0x3 << 8)
#define PECI_CTRL_CLK_DIV(x)		(x << 8)
#define PECI_CTRL_INVERT_OUT		(0x1 << 7)
#define PECI_CTRL_INVERT_IN		(0x1 << 6)
#define PECI_CTRL_BUS_CONTENT_EN	(0x1 << 5)
#define PECI_CTRL_PECI_EN		(0x1 << 4)
#define PECI_CTRL_PECI_CLK_EN		(0x1)

/* AST_PECI_TIMING - 0x04 : Timing Negotiation */
#define PECI_TIMING_MESSAGE_GET(x)	((x & 0xff00) >> 8)
#define PECI_TIMING_MESSAGE(x)		(x << 8)
#define PECI_TIMING_ADDRESS_GET(x)	(x & 0xff)
#define PECI_TIMING_ADDRESS(x)		(x)

/* AST_PECI_CMD	- 0x08 : Command Register */
/* #define PECI_CMD_PIN_MON		(0x1 << 31) */
/* #define PECI_CMD_STS			(0xf << 24) */
#define PECI_CMD_FIRE			(0x1)
#define PECI_CMD_PIN_MON    BIT(31)
#define PECI_CMD_STS_MASK   GENMASK(27, 24)
#define PECI_CMD_STS_GET(x) (((x) & PECI_CMD_STS_MASK) >> 24)

/* AST_PECI_LEN	- 0x0C : Read/Write Length Register */
#define PECI_AW_FCS_EN			(0x1 << 31)
#define PECI_READ_LEN_MASK		(0xff << 16)
#define PECI_READ_LEN(x)		(x << 16)
#define PECI_WRITE_LEN_MASK		(0xff << 8)
#define PECI_WRITE_LEN(x)		(x << 8)
#define PECI_TAGET_ADDR_MASK		(0xff)
#define PECI_TAGET_ADDR(x)		(x)

/* AST_PECI_EXP_FCS	- 0x10 : Expected FCS Data Register  */
#define PECI_PROGRAM_AW_FCS		(0xf << 24)
#define PECI_EXPECT_READ_FCS		(0xf << 16)
#define PECI_EXPECT_AW_FCS_AUTO		(0xf << 8)
#define PECI_EXPECT_WRITE_FCS		(0xf)

/* AST_PECI_CAP_FCS	- 0x14 : Captured FCS Data Register */
#define PECI_CAPTURE_READ_FCS(x)	((x & 0xff0000) >> 16)
#define PECI_CAPTURE_WRITE_FCS		(0xff)

/* AST_PECI_INT_CTRL/ STS  - 0x18/0x1c  : Interrupt Register */
#define PECI_INT_TIMING_RESULT_MASK	(0x3 << 30)
#define PECI_INT_TIMEOUT		(0x1 << 4)
#define PECI_INT_CONNECT		(0x1 << 3)
#define PECI_INT_W_FCS_BAD		(0x1 << 2)
#define PECI_INT_W_FCS_ABORT		(0x1 << 1)
#define PECI_INT_CMD_DONE			(0x1)

#define AUTO_GEN_AWFCS		1
#define DISABLE_ENGINE		0
#define ENABLE_RX_ENGINE	(1 << 0)
#define ENABLE_TX_ENGINE	(1 << 1)
#define LEFT_CHANNEL_HIGH	(1 << 16)
#define DELAY_CLOCK_CYCLE	(1 << 17)

struct timing_negotiation {
	u8		msg_timing;
	u8		addr_timing;
};

struct xfer_msg {
	u8		client_addr;
	u8		tx_len;
	u8		rx_len;
	u8		tx_fcs;
	u8		rx_fcs;
	u8		fcs_en;
	u8		sw_fcs;
	u8		tx_buf[32];
	u8		rx_buf[32];
	u32		sts;
};

#define PECI_DEVICE "/dev/aspeed-peci"
#define PECIIOC_BASE 'P'
#define AST_PECI_IOCRTIMING _IOR(PECIIOC_BASE, 0, struct timing_negotiation*)
#define AST_PECI_IOCWTIMING _IOW(PECIIOC_BASE, 1, struct timing_negotiation*)
#define AST_PECI_IOCXFER _IOWR(PECIIOC_BASE, 2, struct xfer_msg*)

#define PECI_TEMP_RECRY_CNT    (6)
#define PECI_TEMP_INVALID      (99999999)

#define PECI_SIZE		256

struct peci_hwmon_data {
    struct platform_device *platform_device;
    struct device		*hwmon_dev;
    struct mutex        update_lock;
    char            valid;       /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8          data[PECI_SIZE]; /* Register value */
};

static struct aspeed_peci_data {
	struct device		*misc_dev;
	void __iomem		*reg_base;
	int			irq;
	int			recover_count;
    int         peci_type;
	struct completion	xfer_complete;
	u32			sts;
	struct mutex		lock;
	spinlock_t		spin_lock;
    struct peci_hwmon_data *data;
    bool   probe_flag;
} aspeed_peci;

static void aspeed_peci_ctrl_init(void);
static inline void aspeed_peci_write(u32 val, u32 reg)
{
	dev_dbg(aspeed_peci.misc_dev, "write offset: %x, val: %x\n", reg, val);
	writel(val, aspeed_peci.reg_base + reg);
}

static inline u32 aspeed_peci_read(u32 reg)
{
	u32 val = readl(aspeed_peci.reg_base + reg);

	dev_dbg(aspeed_peci.misc_dev, "read offset: %x, val: %x\n", reg, val);

	return val;
}

static void aspeed_toggle_scu_reset(int peci_type)
{
    u32 val, mask, clear;
    char __iomem *scu_system_reset, *scu_system_reset_clear;

    if (peci_type == PECI_AST2600) {
        scu_system_reset = ioremap(SCU_BASE + SCU_SYSTEM_RESET_2, 4);
        scu_system_reset_clear = ioremap(SCU_BASE + SCU_SYSTEM_RESET_CLEAR_2, 4);
        mask = SCU_PECI_RESET_MASK_AST26;

        val = readl(scu_system_reset);
        dev_dbg(aspeed_peci.misc_dev, "read val: %x\n", val);
        /* if controller not reset */
        if (!(val & mask)) {
            /* reset first */
            val = val | mask;
            writel(val, scu_system_reset);
        }
        /* unreset */
        clear = mask;
        writel(clear, scu_system_reset_clear);
        iounmap(scu_system_reset_clear);
    } else {
        scu_system_reset = ioremap(SCU_BASE + SCU_SYSTEM_RESET, 4);
        mask = SCU_PECI_RESET_MASK;

        val = readl(scu_system_reset);
        dev_dbg(aspeed_peci.misc_dev, "read val: %x\n", val);
        /* if controller not reset */
        if (!(val & mask)) {
            /* reset first */
            val = val | mask;
            writel(val, scu_system_reset);
        }
        /* unreset */
        clear = val & ~mask;
        writel(clear, scu_system_reset);
    }
    iounmap(scu_system_reset);
}

static long aspeed_peci_xfer(struct peci_hwmon_data *data, struct xfer_msg *msg)
{
    u32 peci_head;
    int i = 0;
    long ret = 0;
    unsigned long timeout, jiffies_tmp;
    u32 status, offset, remainder;
    u32 *tx_buf0 = (u32 *) (aspeed_peci.reg_base + AST_PECI_W_DATA0);
    u32 *tx_buf1 = (u32 *) (aspeed_peci.reg_base + AST_PECI_W_DATA4);
    u32 *rx_buf0 = (u32 *) (aspeed_peci.reg_base + AST_PECI_R_DATA0);
    u32 *rx_buf1 = (u32 *) (aspeed_peci.reg_base + AST_PECI_R_DATA4);
    u32 rx_data = 0;
    u32 tx_data = 0;
    unsigned long flags;

    mutex_lock(&data->update_lock);
    spin_lock_irqsave(&aspeed_peci.spin_lock, flags);

    timeout = jiffies + msecs_to_jiffies(PECI_IDLE_CHECK_TIMEOUT_MS);
    while (1) {
        jiffies_tmp = jiffies;
        status = aspeed_peci_read(AST_PECI_CMD);
        if (status & (PECI_CMD_STS_MASK | PECI_CMD_PIN_MON)) {
            dev_dbg(aspeed_peci.misc_dev, "wait for free\n");

            if (time_after(jiffies_tmp, timeout)) {
                PECI_DEBUG_ERROR("Timeout waiting for idle state! 0x%x\n", status);
                ret = -ETIMEDOUT;
                goto out;
            }

            usleep_range(PECI_IDLE_CHECK_INTERVAL_MS * 1000,
                        (PECI_IDLE_CHECK_INTERVAL_MS * 1000) + 1000);
        } else {
            break;
        }
    }

    if (msg->fcs_en)
        peci_head = PECI_TAGET_ADDR(msg->client_addr) |
                    PECI_WRITE_LEN(msg->tx_len) |
                    PECI_READ_LEN(msg->rx_len) |
                    PECI_AW_FCS_EN;
    else
        peci_head = PECI_TAGET_ADDR(msg->client_addr) |
                    PECI_WRITE_LEN(msg->tx_len) |
                    PECI_READ_LEN(msg->rx_len);

    aspeed_peci_write(peci_head, AST_PECI_CMD_CTRL);
    for (i = 0; i < msg->tx_len; i++) {
        offset = i / 4;
        remainder = i % 4;

        tx_buf0[offset] = (remainder == 0) ? 0 : tx_buf0[offset];
        tx_buf1[offset] = (remainder == 0) ? 0 : tx_buf1[offset];
        tx_data = (i < 16) ? tx_buf0[offset] | (msg->tx_buf[i] << ((remainder) * 8)) :
                            tx_buf1[offset] | (msg->tx_buf[i] << ((remainder) * 8));

        tx_buf0[offset] = (i < 16) ? tx_data : tx_buf0[offset];
        tx_buf1[offset] = (i < 16) ? tx_data : tx_buf1[offset];
    }

    init_completion(&aspeed_peci.xfer_complete);
    /* Fire Command */
    aspeed_peci_write(PECI_CMD_FIRE, AST_PECI_CMD);
    spin_unlock_irqrestore(&aspeed_peci.spin_lock, flags);

    ret = wait_for_completion_timeout(&aspeed_peci.xfer_complete, 10 * HZ);

    spin_lock_irqsave(&aspeed_peci.spin_lock, flags);
    if (ret <= 0){
        PECI_DEBUG_ERROR("Interrupt! Try to recover peci controller ret %ld \n", ret);
        aspeed_toggle_scu_reset(aspeed_peci.peci_type);
        aspeed_peci_ctrl_init();
        aspeed_peci.recover_count++;
        if (ret == 0) {
            PECI_DEBUG_ERROR("Timeout: wait completion interrupt\n");
            ret = -ETIMEDOUT;
            goto out;
        }

        /* -ERESTARTSYS */
        PECI_DEBUG_ERROR("System termination\n");

        goto out;
    }

    for (i = 0; i < msg->rx_len; i++) {
        offset = i / 4;
        remainder = i % 4;
        rx_data = (i < 16) ? rx_buf0[offset] : rx_buf1[offset];
        msg->rx_buf[i] = (rx_data >> (remainder * 8)) & 0xff;
    }

    msg->sts = aspeed_peci.sts;
    msg->rx_fcs = PECI_CAPTURE_READ_FCS(aspeed_peci_read(AST_PECI_CAP_FCS));

    ret = 0;

out:
    spin_unlock_irqrestore(&aspeed_peci.spin_lock, flags);
    mutex_unlock(&data->update_lock);
    return ret;
}

static long
aspeed_peci_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct xfer_msg msg;
	struct timing_negotiation tim_ng;
	long ret = 0;
    struct peci_hwmon_data *data = aspeed_peci.data;

    memset(&msg, 0, sizeof(struct xfer_msg));
	dev_dbg(aspeed_peci.misc_dev, "aspeed_peci_ioctl cmd %x\n", cmd);

	switch (cmd) {
	case AST_PECI_IOCRTIMING:
		tim_ng.msg_timing = PECI_TIMING_MESSAGE_GET(
					aspeed_peci_read(AST_PECI_TIMING));
		tim_ng.addr_timing = PECI_TIMING_ADDRESS_GET(
					aspeed_peci_read(AST_PECI_TIMING));
		if (copy_to_user(argp, &tim_ng,
				 sizeof(struct timing_negotiation)))
			ret = -EFAULT;
		break;

	case AST_PECI_IOCWTIMING:
		if (copy_from_user(&tim_ng, argp,
				   sizeof(struct timing_negotiation)))
			ret = -EFAULT;
		else
			aspeed_peci_write(
				PECI_TIMING_MESSAGE(tim_ng.msg_timing) |
				PECI_TIMING_ADDRESS(tim_ng.addr_timing),
				AST_PECI_TIMING);

		break;

    case AST_PECI_IOCXFER:
        if (copy_from_user(&msg, argp, sizeof(struct xfer_msg))) {
            ret = -EFAULT;
            break;
        }
        mutex_lock(&aspeed_peci.lock);
        ret = aspeed_peci_xfer(data, &msg);
        if (ret) {
            PECI_DEBUG_ERROR("aspeed_peci_xfer Failed ret %ld.\n", ret);
            mutex_unlock(&aspeed_peci.lock);
            return -1;
        }

        if (copy_to_user(argp, &msg, sizeof(struct xfer_msg))) {
            ret = -EFAULT;
        }
        mutex_unlock(&aspeed_peci.lock);
        break;

	default:
		PECI_DEBUG_ERROR("aspeed_peci_ioctl command fail\n");
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static int aspeed_peci_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int aspeed_peci_release(struct inode *inode, struct file *file)
{
	return 0;
}

static irqreturn_t aspeed_peci_handler(int this_irq, void *dev_id)
{
	/* struct aspeed_peci_data *peci_data = dev_id; */
	spin_lock(&aspeed_peci.spin_lock);

	aspeed_peci.sts = (0x1f & aspeed_peci_read(AST_PECI_INT_STS));
    aspeed_peci_write(aspeed_peci.sts, AST_PECI_INT_STS);

	if (aspeed_peci.sts & PECI_INT_TIMEOUT){
        PECI_DEBUG_VERBOSE("PECI_INT_TIMEOUT\n");
    }
    if (aspeed_peci.sts & PECI_INT_CONNECT){
        PECI_DEBUG_VERBOSE("PECI_INT_CONNECT\n");
    }
    if (aspeed_peci.sts & PECI_INT_W_FCS_BAD){
        PECI_DEBUG_VERBOSE("PECI_INT_W_FCS_BAD\n");
    }
    if (aspeed_peci.sts & PECI_INT_W_FCS_ABORT){
        PECI_DEBUG_VERBOSE("PECI_INT_W_FCS_ABORT\n");
    }
    if (aspeed_peci.sts & PECI_INT_CMD_DONE){
		aspeed_peci_write(0, AST_PECI_CMD);
        complete(&aspeed_peci.xfer_complete);
    }

	spin_unlock(&aspeed_peci.spin_lock);

	return IRQ_HANDLED;

}

static void aspeed_peci_ctrl_init(void)
{
	aspeed_peci_write(PECI_CTRL_SAMPLING(8) |
			  PECI_CTRL_PECI_CLK_EN, AST_PECI_CTRL);

	/* iming Setting : should 4 times of peci clk period 64 = 16 * 4 */
	aspeed_peci_write(PECI_TIMING_MESSAGE(64) | PECI_TIMING_ADDRESS(64),
			  AST_PECI_TIMING);

	/* peci Spec wide speed rangs [2kbps~2Mbps]
	 * ampling 8/16, READ mode : Point Sampling , CLK source : 24Mhz , DIV
	 * by 8 : 3 --> CLK is 3Mhz
	 * peci CTRL Enable
	 */
	aspeed_peci_write(PECI_CTRL_SAMPLING(8) | PECI_CTRL_CLK_DIV(3) |
			  PECI_CTRL_PECI_EN | PECI_CTRL_PECI_CLK_EN,
			  AST_PECI_CTRL);

	/* Clear Interrupt */
	aspeed_peci_write(PECI_INT_TIMEOUT | PECI_INT_CONNECT |
			  PECI_INT_W_FCS_BAD | PECI_INT_W_FCS_ABORT |
			  PECI_INT_CMD_DONE, AST_PECI_INT_STS);

	/* peci Negotiation Selection , interrupt enable.
	 * Set nego mode :  1st bit of addr negotiation
	 */
	aspeed_peci_write(PECI_INT_TIMEOUT | PECI_INT_CONNECT |
			  PECI_INT_W_FCS_BAD | PECI_INT_W_FCS_ABORT |
			  PECI_INT_CMD_DONE, AST_PECI_INT_CTRL);
}

static const struct file_operations aspeed_peci_fops = {
	.owner		= THIS_MODULE,
	.llseek		= noop_llseek,
	.unlocked_ioctl = aspeed_peci_ioctl,
	.open		= aspeed_peci_open,
	.release	= aspeed_peci_release,
};

struct miscdevice aspeed_peci_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "aspeed-peci",
	.fops = &aspeed_peci_fops,
};

static long aspeed_peci_read_tmax(void)
{
    struct peci_hwmon_data *data = aspeed_peci.data;
    struct xfer_msg msg;
    long ret;
    int temp = -PECI_TEMP_INVALID;
    u8 index = 0x10;
    u16 para = 0x0000;
    int retry_cnt;

    memset(&msg, 0, sizeof(struct xfer_msg));

    msg.fcs_en = 0x0;
    msg.client_addr = 0x30;
    msg.tx_len = 0x05;
    msg.rx_len = 0x05;
    msg.tx_buf[0] = 0xa1; /* cmd */
    msg.tx_buf[1] = 0x0; /* Host ID[7:1] & Retry[0] */
    msg.tx_buf[2] = index;
    msg.tx_buf[3] = (u8)(para & 0xff);
    msg.tx_buf[4] = (u8)((para & 0xff00) >> 8);

    PECI_DEBUG_VERBOSE("Enter aspeed_peci_read_tmax.\n");
    retry_cnt = PECI_TEMP_RECRY_CNT;
    while (retry_cnt--) {
        ret = aspeed_peci_xfer(data, &msg);
        if (ret || msg.rx_buf[0] != PECI_COMMOND_PASS) {
            PECI_DEBUG_ERROR("Failed to read peci for host cpu temp max, recry_cnt %d, ret %ld, pass_cmd 0x%x.\n", retry_cnt, ret, msg.rx_buf[0]);
            continue;
        }
        PECI_DEBUG_VERBOSE("success retry_cnt %d.\n", retry_cnt);
        break;
    }
    if (retry_cnt < 0) {
        PECI_DEBUG_ERROR("Failed to read peci for host cpu temp max, recry_cnt %d, ret %ld.\n", retry_cnt, ret);
        if (ret < 0) {
            return ret;
        } else {
            return -EIO;;
        }
    }

    temp = msg.rx_buf[3];

    PECI_DEBUG_VERBOSE("Leave aspeed_peci_read_tmax %d.\n", temp);
    return temp;
}

static ssize_t show_host_cpu_temp(struct device *dev, struct device_attribute *da, char *buf)
{
    struct peci_hwmon_data *data = aspeed_peci.data;
    struct xfer_msg msg;
    long ret;
    int temp = -PECI_TEMP_INVALID;
    u8 index = 0x02;
    u16 para = 0x0000;
    int retry_cnt;
    u8 tmp;
    int tmp_temp;
    u8 tmax;

    mutex_lock(&aspeed_peci.lock);
    ret = aspeed_peci_read_tmax();
    if (ret < 0) {
        mutex_unlock(&aspeed_peci.lock);
        PECI_DEBUG_ERROR("Failed to read peci for host cpu temp max (rv %ld).\n", ret);
        return ret;
    }

    tmax = (u8)ret;
    PECI_DEBUG_VERBOSE("temp max %d,rv %ld.\n", tmax, ret);
    memset(&msg, 0, sizeof(struct xfer_msg));

    msg.fcs_en = 0x0;
    msg.client_addr = 0x30;
    msg.tx_len = 0x05;
    msg.rx_len = 0x05;
    msg.tx_buf[0] = 0xa1; /* cmd */
    msg.tx_buf[1] = 0x0; /* Host ID[7:1] & Retry[0] */
    msg.tx_buf[2] = index;
    msg.tx_buf[3] = (u8)(para & 0xff);
    msg.tx_buf[4] = (u8)((para & 0xff00) >> 8);

    PECI_DEBUG_VERBOSE("Enter.\n");
    retry_cnt = PECI_TEMP_RECRY_CNT;
    while (retry_cnt--) {
        ret = aspeed_peci_xfer(data, &msg);
        if (ret || msg.rx_buf[0] != PECI_COMMOND_PASS) {
            PECI_DEBUG_ERROR("Failed to read peci for host cpu temp, recry_cnt %d, ret %ld, pass_cmd 0x%x.\n", retry_cnt, ret, msg.rx_buf[0]);
            continue;
        }
        PECI_DEBUG_VERBOSE("success retry_cnt %d.\n", retry_cnt);
        break;
    }
    if (retry_cnt < 0) {
        PECI_DEBUG_ERROR("Failed to read peci for host cpu temp, recry_cnt %d, ret %ld.\n", retry_cnt, ret);
        mutex_unlock(&aspeed_peci.lock);
        if (ret < 0) {
            return ret;
        } else {
            return -EIO;
        }
    }

    tmp = ((((~(msg.rx_buf[2])) & 0x7F) << 2) | ((~(msg.rx_buf[1]) & 0xC0) >> 6)) + 1;
    PECI_DEBUG_VERBOSE("tmax %d\n",tmax);
    if ((msg.rx_buf[2]) & 0x80){
        tmp_temp = tmax - tmp;
    } else {
        tmp_temp = tmax + tmp;
    }

    /* To determine the temperature range, reserve first */
    if ((tmp_temp >= 200) || (tmp_temp <= -100)) {
        PECI_DEBUG_ERROR("Failed msg.rx_buf[2]=0x%x, msg.rx_buf[1]=0x%x.\n", msg.rx_buf[2], msg.rx_buf[1]);
        PECI_DEBUG_ERROR("Failed temp=%d.\n", temp);
        mutex_unlock(&aspeed_peci.lock);
        return -1;
    }

    temp = tmp_temp;

    PECI_DEBUG_VERBOSE("Leave.\n");
    mutex_unlock(&aspeed_peci.lock);
    return snprintf(buf, PECI_SIZE, "%d\n", temp * 1000);
}

static ssize_t show_recover_count(struct device *dev, struct device_attribute *da, char *buf)
{
    return snprintf(buf, PECI_SIZE, "%d\n", aspeed_peci.recover_count);
}

/* host_cpu_temp -- temp4_input */
static SENSOR_DEVICE_ATTR(temp4_input, S_IRUGO, show_host_cpu_temp, NULL, 0);
static SENSOR_DEVICE_ATTR(recover_count, S_IRUGO, show_recover_count, NULL, 0);

static struct attribute *peci_hwmon_attrs[] = {
    &sensor_dev_attr_temp4_input.dev_attr.attr,
    &sensor_dev_attr_recover_count.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(peci_hwmon);

static const struct of_device_id aspeed_peci_of_table[] = {
	{ .compatible = "aspeed,ast2500-peci", .data = (void *)PECI_AST2500},
    { .compatible = "aspeed,ast2600-peci", .data = (void *)PECI_AST2600},
	{ },
};
MODULE_DEVICE_TABLE(of, aspeed_peci_of_table);

static int aspeed_peci_probe(struct platform_device *pdev)
{
	struct resource *res;
    struct peci_hwmon_data *data;
	int ret = 0;
    const struct of_device_id *match;

    PECI_DEBUG_ERROR("Enter.\n");
    if (aspeed_peci.probe_flag) {
        dev_err(&pdev->dev, "driver already probe!\n");
        return -EBUSY;
    }

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "cannot get IORESOURCE_MEM\n");
		ret = -ENOENT;
		goto out;
	}

	aspeed_peci.reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(aspeed_peci.reg_base)) {
		ret = -EIO;
		goto out_region;
	}
	aspeed_peci.irq = platform_get_irq(pdev, 0);
	if (aspeed_peci.irq < 0) {
		dev_err(&pdev->dev, "no irq specified\n");
		ret = -ENOENT;
		goto out_region;
	}
	ret = devm_request_irq(&pdev->dev, aspeed_peci.irq,
			       aspeed_peci_handler, 0, "aspeed-peci",
			       &aspeed_peci);

	if (ret) {
		dev_err(&pdev->dev, "PECI: Failed request irq %d\n",
			 aspeed_peci.irq);
		goto out_region;
	}

    match = of_match_node(aspeed_peci_of_table, pdev->dev.of_node);
	if (!match) {
        aspeed_peci.peci_type = PECI_AST2500;
    } else {
        aspeed_peci.peci_type = (int)(unsigned long)match->data;
    }

	aspeed_peci.misc_dev = &pdev->dev;
	platform_set_drvdata(pdev, &aspeed_peci);
	/* Reset PECI controller */
	aspeed_toggle_scu_reset(aspeed_peci.peci_type);
	aspeed_peci_ctrl_init();
	dev_info(&pdev->dev, "aspeed_peci: driver successfully loaded, type %d.\n", aspeed_peci.peci_type);

    data = devm_kzalloc(&pdev->dev, sizeof(struct peci_hwmon_data), GFP_KERNEL);
    if (!data) {
        ret = -ENOMEM;
        goto out_region;
    }
    data->platform_device = pdev;
    aspeed_peci.data = data;
    aspeed_peci.recover_count = 0;
    mutex_init(&data->update_lock);
    mutex_init(&aspeed_peci.lock);
    spin_lock_init(&aspeed_peci.spin_lock);
    ret = misc_register(&aspeed_peci_misc);
    if (ret) {
        dev_err(&pdev->dev,
            "aspeed_peci : failed to misc_register \n");
        goto out_data;
    }

    data->hwmon_dev = hwmon_device_register_with_groups(&pdev->dev, pdev->name, data, peci_hwmon_groups);
    if (IS_ERR(data->hwmon_dev)) {
        ret = PTR_ERR(data->hwmon_dev);
        goto out_misc;
    }
    aspeed_peci.probe_flag = true;
	return 0;

out_misc:
    misc_deregister(&aspeed_peci_misc);
out_data:
    kfree(data);
out_region:
	release_mem_region(res->start, res->end - res->start + 1);
out:
	dev_warn(&pdev->dev, "aspeed_peci: driver init failed (ret=%d)!\n",
		  ret);
	return ret;
}

static void aspeed_peci_remove(struct platform_device *pdev)
{
    struct resource *res;

    aspeed_peci.probe_flag = false;
    misc_deregister(&aspeed_peci_misc);
    hwmon_device_unregister(aspeed_peci.data->hwmon_dev);
    devm_free_irq(&pdev->dev, aspeed_peci.irq, &aspeed_peci);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    ioport_unmap(aspeed_peci.reg_base);
    release_mem_region(res->start, res->end - res->start + 1);
    devm_kfree(&pdev->dev, aspeed_peci.data);
}

#ifdef CONFIG_PM
static int
aspeed_peci_suspend(struct platform_device *pdev, pm_message_t state)
{
	PECI_DEBUG_VERBOSE("aspeed_peci_suspend : TODO\n");
	return 0;
}

static int aspeed_peci_resume(struct platform_device *pdev)
{
	aspeed_peci_ctrl_init();

	return 0;
}

#else
#define aspeed_peci_suspend        NULL
#define aspeed_peci_resume         NULL
#endif

static struct platform_driver aspeed_peci_driver = {
	.probe		= aspeed_peci_probe,
	.remove		= aspeed_peci_remove,
	.suspend        = aspeed_peci_suspend,
	.resume         = aspeed_peci_resume,
	.driver         = {
		.name   = KBUILD_MODNAME,
		.owner  = THIS_MODULE,
		.of_match_table = aspeed_peci_of_table,
	},
};

module_platform_driver(aspeed_peci_driver);

MODULE_AUTHOR("Ryan Chen <ryan_chen at aspeedtech.com>");
MODULE_DESCRIPTION("AST PECI driver");
MODULE_LICENSE("GPL");
