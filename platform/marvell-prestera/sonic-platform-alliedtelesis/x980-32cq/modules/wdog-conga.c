// SPDX-License-Identifier: GPL-2.0-only
/*
 * Watchdog Driver for the CongaTec Board Controller.
 *
 * Register addresses come from CongaTec's BSD-licensed CgosDrv driver.
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>

#define DRV_NAME		    "wdog-conga"

#define GEN5_HCNM_BASE		    0x0e20
#define GEN5_HCC0_BASE		    0x0e00

#define GEN5_HCNM_COMMAND	    (GEN5_HCNM_BASE + 0x00)
#define   GEN5_HCNM_IDLE	    0x00
#define   GEN5_HCNM_REQUEST	    0x01
#define GEN5_HCNM_DATA		    (GEN5_HCNM_BASE + 0x01)
#define GEN5_HCNM_STATUS	    (GEN5_HCNM_BASE + 0x02)
#define   GEN5_HCNM_FREE	    0x0003
#define GEN5_HCNM_ACCESS	    (GEN5_HCNM_BASE + 0x04)
#define   GEN5_HCNM_GAINED	    0x00000000

#define GEN5_HCC_STROBE		    (GEN5_HCC0_BASE + 0x00)
#define GEN5_HCC_INDEX		    (GEN5_HCC0_BASE + 0x02)
#define   GEN5_HCC_INDEX_CBM_MAN8   0x00
#define   GEN5_HCC_INDEX_CBM_AUTO32 0x03
#define GEN5_HCC_DATA		    (GEN5_HCC0_BASE + 0x04)
#define GEN5_HCC_ACCESS		    (GEN5_HCC0_BASE + 0x0C)

#define CGBC_TIMEOUT_US		    80000

/* Board Controller Status Byte */
#define CGBC_STAT_MSK		    0xC0
#define CGBC_RDY_STAT		    0x40
#define CGBC_ERR_STAT		    0xC0
#define CGBC_DAT_STAT		    0x00

#define CGBC_DAT_CNT_MSK	    0x1F
#define CGBC_ERR_COD_MSK	    0x1F

/* Watchdog */
#define WATCHDOG_MAX_STAGES 3	/* total number of available watchdog stages */
#define WATCHDOG_TIMEOUT 60
unsigned long wdt_delay = 30;	/* in seconds */

enum {
	WATCHDOG_OPMODE_DISABLED = 0,
	WATCHDOG_OPMODE_ONETIME_TRIG,
	WATCHDOG_OPMODE_SINGLE_EVENT,
	WATCHDOG_OPMODE_EVENT_REPEAT
};

enum {
	WATCHDOG_EVENT_INTERRUPT = 0,
	WATCHDOG_EVENT_SMI_SCI,
	WATCHDOG_EVENT_RESET,
	WATCHDOG_EVENT_BUTTON
};

static unsigned int timeout;
static u64 last_jiffies = 0;
module_param(timeout, uint, S_IRUGO);
MODULE_PARM_DESC(timeout, "Watchdog timeout in seconds "
		 "(default=" __MODULE_STRING(WATCHDOG_TIMEOUT) ")");

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, S_IRUGO);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started "
		 "(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

#define CGBC_CMD_WDOG_TRIGGER 0x27
#define CGBC_CMD_WDOG_INIT 0x28

static int client_number;

static struct platform_device *platform_device;

static int conga_gen5_hcnm_occupy(struct device *dev)
{
	u8 val;
	int ret;

	ret = readx_poll_timeout(inb, GEN5_HCNM_STATUS, val,
				 val == GEN5_HCNM_FREE, 0, CGBC_TIMEOUT_US);
	if (ret < 0) {
		dev_err(dev, "timeout in hcnm occupy");
		return ret;
	}

	if (inl(GEN5_HCNM_ACCESS) == GEN5_HCNM_GAINED) {
		return 0;
	} else {
		dev_err(dev, "access not gained");
		return -EIO;
	}
}

static int conga_gen5_hcnm_command(struct device *dev, u8 cmd)
{
	u8 val;
	int ret;

	ret = readx_poll_timeout(inb, GEN5_HCNM_COMMAND, val,
				 val == GEN5_HCNM_IDLE, 0, CGBC_TIMEOUT_US);
	if (ret < 0) {
		dev_err(dev, "timeout waiting for hcnm idle");
		return ret;
	}

	outb(cmd, GEN5_HCNM_COMMAND);

	ret = readx_poll_timeout(inb, GEN5_HCNM_COMMAND, val,
				 val == GEN5_HCNM_IDLE, 0, CGBC_TIMEOUT_US);
	if (ret < 0) {
		dev_err(dev, "timeout waiting for hcnm command to complete");
		return ret;
	}

	return inb(GEN5_HCNM_DATA);
}

static int conga_gen5_command(struct device *dev, u8 *cmd_buf, int cmd_len,
			      u8 *ret_buf, int ret_len, u8 *bc_ret)
{
	u8 val;
	int ret;
	u8 access = 0;
	u32 mod_change_index = 0xffffffff;
	u8 cmd_checksum = 0;
	u8 ret_checksum = 0;
	u8 bc_checksum;
	u8 bc_status;
	ktime_t timeout;
	int i;

	/* Request access */
	timeout = ktime_add_us(ktime_get(), CGBC_TIMEOUT_US);

	while (access != client_number) {
		outb(client_number, GEN5_HCC_ACCESS);
		access = inb(GEN5_HCC_ACCESS);

		if (ktime_get() >= timeout) {
			dev_err(dev, "timeout requesting access");
			return -ETIMEDOUT;
		}
	}

	/* Wait for strobe register to clear */
	ret = readx_poll_timeout(inb, GEN5_HCC_STROBE, val,
				 val == 0, 0, CGBC_TIMEOUT_US);
	if (ret < 0) {
		dev_err(dev, "timeout waiting for strobe to clear");
		return ret;
	}

	/* Write command packet */
	if (cmd_len <= 2) {
		outb(GEN5_HCC_INDEX_CBM_MAN8, GEN5_HCC_INDEX);
	} else {
		outb(GEN5_HCC_INDEX_CBM_AUTO32, GEN5_HCC_INDEX);
		if ((cmd_len % 4) != 3) {
			mod_change_index = (cmd_len & 0xfffc) - 1;
		}
	}

	for (i = 0; i < cmd_len; i++) {
		outb(cmd_buf[i], GEN5_HCC_DATA + (i % 4));
		cmd_checksum ^= cmd_buf[i];

		if (i == mod_change_index) {
			outb(GEN5_HCC_INDEX_CBM_MAN8 | (i + 1), GEN5_HCC_INDEX);
		}
	}

	/* Append checksum byte */
	outb(cmd_checksum, GEN5_HCC_DATA + (i % 4));

	/* strobe */
	outb(client_number, GEN5_HCC_STROBE);

	/* Rewind HCC buffer index */
	outb(GEN5_HCC_INDEX_CBM_AUTO32, GEN5_HCC_INDEX);

	/* Wait for command to complete */
	ret = readx_poll_timeout(inb, GEN5_HCC_STROBE, val,
				 val == 0, 0, CGBC_TIMEOUT_US);
	if (ret < 0) {
		dev_err(dev, "timeout waiting for command to complete");
		return ret;
	}

	/* Determine command status */
	bc_status = inb(GEN5_HCC_DATA);
	ret_checksum ^= bc_status;

	switch (bc_status & CGBC_STAT_MSK) {
	case CGBC_DAT_STAT:
		/* Got data back from board controller */
		if (bc_status < ret_len) {
			ret_len = bc_status;
		}
		for (i = 0; i < ret_len; i++) {
			ret_buf[i] = inb(GEN5_HCC_DATA + ((i + 1) % 4));
			ret_checksum ^= ret_buf[i];
		}
		bc_checksum = inb(GEN5_HCC_DATA + ((i + 1) % 4));
		bc_status &= CGBC_DAT_CNT_MSK;
		break;

	case CGBC_RDY_STAT:
		/* Got status back from board controller */
		bc_checksum = inb(GEN5_HCC_DATA + 1);
		bc_status &= CGBC_ERR_COD_MSK;
		break;

	case CGBC_ERR_STAT:
	default:
		/* Got error back from board controller */
		bc_checksum = inb(GEN5_HCC_DATA + 1);
		bc_status &= CGBC_ERR_COD_MSK;
		dev_err(dev, "board controller reports error code %#x",
			bc_status);
		ret = -EIO;
		break;
	}

	/* Release HCC */
	outb(client_number, GEN5_HCC_ACCESS);

	/* Checksum verification */
	if (ret == 0 && ret_checksum != bc_checksum) {
		dev_err(dev,
			"BC checksum %#x does not match calculated checksum %#x",
			bc_checksum, ret_checksum);
		ret = -EIO;
	}

	*bc_ret = bc_status;
	return ret;
}

static int conga_bc_wdog_stop(struct watchdog_device *wdd)
{
	struct device *dev = wdd->parent;
	u8 cmd_buf[15];
	u8 status;

	if (last_jiffies == 0)
	    return 0;

	last_jiffies = 0;

	memset(cmd_buf, 0, sizeof(cmd_buf));

	cmd_buf[0] = CGBC_CMD_WDOG_INIT;
	cmd_buf[1] = WATCHDOG_OPMODE_DISABLED;

	return conga_gen5_command(dev, cmd_buf, sizeof(cmd_buf), NULL, 0,
				  &status);
}

static int conga_bc_wdog_start(struct watchdog_device *wdd)
{
	struct device *dev = wdd->parent;
	u8 cmd_buf[15];
	u8 status;
	u8 i;
	unsigned long wdt_timeout_ms;
	const unsigned long wdt_delay_ms = wdt_delay * 1000;
	if (wdd->timeout == 0) {
	    wdd->timeout = WATCHDOG_TIMEOUT;
	}
	wdt_timeout_ms = wdd->timeout * 1000;
	last_jiffies = jiffies;
	memset(cmd_buf, 0, sizeof(cmd_buf));

	/* Populate message to the Congatec board controller:
	 * op-mode is either single event (watchdog switches off after first event of the last
	 * stage), repeated event (keep generating events in the last stage), or single trigger
	 * (switches off after first event of the last stage and also when triggered for the first
	 * time).
	 * Set stage count to 3 as there is no need to restrict the number of stages. If the first
	 * stage does a system reset the following stages are irrelevant.
	 * Set each stage to do a system reset with the defined timeout.
	 * Other possible events are ACPI system control interrupt or power button signal.
	 * The last 3 bytes set the delay (in milliseconds) after driver startup before the
	 * watchdog starts working and requires regular triggering.
	 */
	cmd_buf[0] = CGBC_CMD_WDOG_INIT;
	cmd_buf[1] = WATCHDOG_OPMODE_SINGLE_EVENT;
	cmd_buf[2] =
	    WATCHDOG_MAX_STAGES | (WATCHDOG_EVENT_RESET << 2) |
	    (WATCHDOG_EVENT_RESET << 4) | (WATCHDOG_EVENT_RESET << 6);

	/* To configure only one stage, set cmd_buf[2] to
	 * 1 | (WATCHDOG_EVENT_RESET << 2)
	 */

	// Set same timeout for all stages
	for (i = 0; i < WATCHDOG_MAX_STAGES; i++) {
		cmd_buf[3 + i * WATCHDOG_MAX_STAGES] = wdt_timeout_ms & 0xff;
		cmd_buf[4 + i * WATCHDOG_MAX_STAGES] =
		    (wdt_timeout_ms & 0xff00) >> 8;
		cmd_buf[5 + i * WATCHDOG_MAX_STAGES] =
		    (wdt_timeout_ms & 0xff0000) >> 16;
	}

	// Set delay before watchdog starts working
	cmd_buf[12] = wdt_delay_ms & 0xff;
	cmd_buf[13] = (wdt_delay_ms & 0xff00) >> 8;
	cmd_buf[14] = (wdt_delay_ms & 0xff0000) >> 16;

	return conga_gen5_command(dev, cmd_buf, sizeof(cmd_buf), NULL, 0,
				  &status);
}

static int conga_bc_wdog_trigger(struct watchdog_device *wdd)
{
	struct device *dev = wdd->parent;
	u8 cmd_buf[1] = { CGBC_CMD_WDOG_TRIGGER };
	u8 status;

	return conga_gen5_command(dev, cmd_buf, sizeof(cmd_buf), NULL, 0,
				  &status);
}

static int conga_set_timeout(struct watchdog_device *wdd, unsigned int timeout)
{
	conga_bc_wdog_stop(wdd);
	wdd->timeout = timeout;
	return conga_bc_wdog_start(wdd);
}

static unsigned int conga_get_timeleft(struct watchdog_device *wdd)
{
	u32 time_left = 0;
        u64 diff = jiffies - last_jiffies;
	u32 elapsed = (u32)(diff / HZ);

	if ((test_bit(WDOG_ACTIVE, &wdd->status)) && wdd->timeout != 0) {
	    if (last_jiffies == 0) {
		time_left = wdd->timeout;
	    } else {
		time_left = wdd->timeout - elapsed;
	    }
	}

	return time_left;
}


static struct watchdog_info conga_wdt_info = {
	.identity = DRV_NAME,
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
};

static const struct watchdog_ops conga_wdt_ops = {
	.owner = THIS_MODULE,
	.start = conga_bc_wdog_start,
	.stop = conga_bc_wdog_stop,
	.ping = conga_bc_wdog_trigger,
	.set_timeout = conga_set_timeout,
	.get_timeleft = conga_get_timeleft,
};

static struct watchdog_device conga_wdt_dev = {
	.info = &conga_wdt_info,
	.ops = &conga_wdt_ops,
	.timeout = WATCHDOG_TIMEOUT,
};

static int conga_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	conga_wdt_dev.parent = dev;

	/* Obtain a client number */
	ret = conga_gen5_hcnm_occupy(dev);
	if (ret == 0) {
		client_number = conga_gen5_hcnm_command(dev, GEN5_HCNM_REQUEST);
		outw(GEN5_HCNM_FREE, GEN5_HCNM_STATUS);

		if (client_number < 0x02 || client_number > 0xfe) {
			dev_err(dev, "invalid client number");
		}

	} else {
		/* Log the error, but don't propagate it. Not all
		 * development boards have the board-controller.
		 */
		dev_err(dev, "unable to occupy board controller");
		return 0;
	}

	watchdog_init_timeout(&conga_wdt_dev, timeout, NULL);
	watchdog_set_nowayout(&conga_wdt_dev, nowayout);
	watchdog_stop_on_reboot(&conga_wdt_dev);
	watchdog_stop_on_unregister(&conga_wdt_dev);

	ret = devm_watchdog_register_device(dev, &conga_wdt_dev);
	if (ret)
		return ret;

	dev_info(dev, "initialized (timeout=%ds, nowayout=%d)\n",
		 conga_wdt_dev.timeout, nowayout);

	return 0;
}

static int conga_wdt_suspend(struct platform_device *dev, pm_message_t state)
{
	return conga_bc_wdog_stop(&conga_wdt_dev);
}

static int conga_wdt_resume(struct platform_device *dev)
{
	return conga_bc_wdog_start(&conga_wdt_dev);
}

static struct platform_driver conga_wdt_driver = {
	.probe = conga_wdt_probe,
	.suspend = conga_wdt_suspend,
	.resume = conga_wdt_resume,
	.driver = {
		   .name = DRV_NAME,
		   },
};

static int __init conga_wdt_init_module(void)
{
	int err;

	err = platform_driver_register(&conga_wdt_driver);
	if (err)
		return err;

	platform_device =
	    platform_device_register_simple(DRV_NAME, -1, NULL, 0);
	if (IS_ERR(platform_device)) {
		err = PTR_ERR(platform_device);
		platform_driver_unregister(&conga_wdt_driver);
	}

	return err;
}

static void __exit conga_wdt_cleanup_module(void)
{
	platform_device_unregister(platform_device);
	platform_driver_unregister(&conga_wdt_driver);
}

module_init(conga_wdt_init_module);
module_exit(conga_wdt_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alliedtelesis");
MODULE_DESCRIPTION("Congatec board controller watchdog driver");
