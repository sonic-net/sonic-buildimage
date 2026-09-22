// SPDX-License-Identifier: GPL-2.0-only
/*
 * GPIO Driver for the CongaTec Board Controller.
 *
 * Register addresses come from CongaTec's BSD-licensed CgosDrv driver.
 */

#include <linux/device.h>
#include <linux/dmi.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/gpio/driver.h>

#define DRV_NAME		    "gpio-conga"
#define GPIO_COUNT		    8

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
#define   GEN5_HCC_INDEX_CBI_MSK    0xFC
#define   GEN5_HCC_INDEX_CBM_MSK    0x03
#define   GEN5_HCC_INDEX_CBM_MAN8   0x00
#define   GEN5_HCC_INDEX_CBM_AUTO32 0x03
#define GEN5_HCC_DATA		    (GEN5_HCC0_BASE + 0x04)
#define GEN5_HCC_ACCESS		    (GEN5_HCC0_BASE + 0x0C)

#define CGBC_CMD_GPIO_DAT_RD	    0x64
#define CGBC_CMD_GPIO_DAT_WR	    0x65
#define CGBC_CMD_GPIO_CFG_RD	    0x66
#define CGBC_CMD_GPIO_CFG_WR	    0x67

#define CGBC_TIMEOUT_US		    80000

/* Board Controller Status Byte */
#define CGBC_STAT_MSK		    0xC0
#define CGBC_IDL_STAT		    0x00
#define CGBC_BSY_STAT		    0x80
#define CGBC_RDY_STAT		    0x40
#define CGBC_ERR_STAT		    0xC0
#define CGBC_DAT_STAT		    0x00

#define CGBC_DAT_CNT_MSK	    0x1F
#define CGBC_RET_COD_MSK	    0x1F
#define CGBC_ERR_COD_MSK	    0x1F
#define CGBC_IDL_FLG_MSK	    0x1F

static int client_number = 0;

static const char *conga_bc_gpio_names[GPIO_COUNT] = {
	"GPI0", "GPI1", "GPI2", "GPI3", "GPO0", "GPO1", "GPO2", "GPO3"
};

/*
 * Mapping from GPI# to IRQ#. Zero represents 'none'.
 * This mapping needs to match what we have configured in the BIOS.
 */
static const int conga_irq_map[] = {
	3,			/* GPI0 */
	6,			/* GPI1 */
	0,			/* GPI2 */
	0,			/* GPI3 */
};

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

static int conga_bc_gpio_get_value(struct gpio_chip *gc, unsigned int off)
{
	struct device *dev = gc->parent;
	u8 cmd_buf[3] = { CGBC_CMD_GPIO_DAT_RD, 0x00, 0x00 };
	u8 ret_buf[1];
	u8 status;
	int ret;

	if (off >= GPIO_COUNT)
		return -EINVAL;

	ret = conga_gen5_command(dev, cmd_buf, sizeof(cmd_buf),
				 ret_buf, sizeof(ret_buf), &status);
	if (ret < 0)
		return ret;

	return !!(ret_buf[0] & BIT(off));
}

static void conga_bc_gpio_set_value(struct gpio_chip *gc, unsigned int off,
				    int val)
{
	struct device *dev = gc->parent;
	u8 cmd_buf[3] = { CGBC_CMD_GPIO_DAT_RD, 0x00, 0x00 };
	u8 ret_buf[1];
	u8 status;
	u8 new_value;
	int ret;

	if (off >= GPIO_COUNT)
		return;

	/* Read */
	ret = conga_gen5_command(dev, cmd_buf, sizeof(cmd_buf),
				 ret_buf, sizeof(ret_buf), &status);
	if (ret < 0)
		return;

	/* Modify */
	new_value = ret_buf[0] & ~(1 << off);
	if (val)
		new_value |= (1 << off);

	/* Write */
	cmd_buf[0] = CGBC_CMD_GPIO_DAT_WR;
	cmd_buf[1] = 0x00;
	cmd_buf[2] = new_value;

	conga_gen5_command(dev, cmd_buf, sizeof(cmd_buf), ret_buf,
			   sizeof(ret_buf), &status);
}

static int conga_bc_gpio_get_direction(struct gpio_chip *gc, unsigned int off)
{
	struct device *dev = gc->parent;
	u8 cmd_buf[3] = { CGBC_CMD_GPIO_CFG_RD, 0x00, 0x00 };
	u8 ret_buf[1];
	u8 status;
	int ret;

	if (off >= GPIO_COUNT)
		return -EINVAL;

	ret = conga_gen5_command(dev, cmd_buf, sizeof(cmd_buf),
				 ret_buf, sizeof(ret_buf), &status);
	if (ret < 0)
		return ret;

	if (ret_buf[0] & BIT(off))
		return GPIO_LINE_DIRECTION_OUT;

	return GPIO_LINE_DIRECTION_IN;
}

static int conga_bc_gpio_direction_input(struct gpio_chip *gc, unsigned int off)
{
	int direction;

	/* For now, we don't support modifying GPI / GPO pin directions.
	 * Report success if the pin type already matches, otherwise error. */
	direction = conga_bc_gpio_get_direction(gc, off);

	if (direction == GPIO_LINE_DIRECTION_IN)
		return 0;

	return -EINVAL;
}

static int conga_bc_gpio_direction_output(struct gpio_chip *gc,
					  unsigned int off, int val)
{
	int direction;

	/* For now, we don't support modifying GPI / GPO pin directions.
	 * Set the value if the pin is already output, otherwise error. */
	direction = conga_bc_gpio_get_direction(gc, off);

	if (direction == GPIO_LINE_DIRECTION_OUT) {
		conga_bc_gpio_set_value(gc, off, val);
		return 0;
	}

	return -EINVAL;
}

/* Mapping to allow other drivers to find the IRQ# associated with a GPIO */
static int conga_bc_gpio_to_irq(struct gpio_chip *chip, unsigned int gpi)
{
	int irq;

	if (gpi >= 4) {
		return -EINVAL;
	}

	irq = conga_irq_map[gpi];

	return irq ? irq : -ENODATA;
}

/* Note: We assume the 5th generation Board Controller */
static int conga_bc_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct gpio_chip *chip;
	int ret;

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
		 * development boards have the board-controller. */
		dev_err(dev, "unable to occupy board controller");
		return 0;
	}

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->label = DRV_NAME;
	chip->parent = dev;
	chip->owner = THIS_MODULE;
	chip->get_direction = conga_bc_gpio_get_direction;
	chip->direction_input = conga_bc_gpio_direction_input;
	chip->direction_output = conga_bc_gpio_direction_output;
	chip->get = conga_bc_gpio_get_value;
	chip->set = conga_bc_gpio_set_value;
	chip->base = -1;
	chip->ngpio = GPIO_COUNT;
	chip->names = conga_bc_gpio_names;
	chip->to_irq = conga_bc_gpio_to_irq;

	ret = devm_gpiochip_add_data(dev, chip, NULL);
	if (ret)
		return ret;

	return 0;
}

static struct platform_driver conga_bc_gpio_driver = {
	.probe = conga_bc_gpio_probe,
	.driver.name = DRV_NAME,
};

static struct platform_device conga_bc_gpio_device = {
	.name = DRV_NAME,
};

static int conga_bc_gpio_init(void)
{
	int ret;

	ret = platform_driver_register(&conga_bc_gpio_driver);

	if (ret == 0)
		ret = platform_device_register(&conga_bc_gpio_device);

	return ret;
}

module_init(conga_bc_gpio_init);
MODULE_DESCRIPTION("Congatec board controller GPIO driver");
MODULE_LICENSE("GPL");
