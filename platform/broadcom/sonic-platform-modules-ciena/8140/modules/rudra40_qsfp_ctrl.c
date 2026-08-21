// SPDX-License-Identifier: GPL-2.0
/*
 * Ciena Rudra40 QSFP Control GPIO Driver
 *
 * Exposes QSFP control signals (low-power mode, reset) and SFP TX
 * Disable as read/write GPIO pins in /sys/class/gpio/, consistent
 * with the CIC driver's read-only status GPIOs.
 *
 * FPGA Registers (Rudra40 CFPGA BAR0):
 *   RUDRA40_OPTICS_QSFP_LOW_PWR_0    bits[7:0]   RW
 *     1 = low-power mode, 0 = high-power mode.  Bits 7-0 → QSFP 8-1.
 *   RUDRA40_OPTICS_QSFP_RESET_0      bits[7:0]   RW
 *     1 = out of reset,   0 = in reset.          Bits 7-0 → QSFP 8-1.
 *   RUDRA40_OPTICS_SFP_TX_DISABLE    bits[31:0]  RW
 *     1 = TX disabled,    0 = TX enabled.         Bits 31-0 → SFP 32-1.
 *   RUDRA40_OPTICS_SFP_TX_DISABLE_2  bits[7:0]   RW
 *     1 = TX disabled,    0 = TX enabled.         Bits 7-0 → SFP 40-33.
 *
 * Pin layout (gpio_chip offsets):
 *   0-7    QSFP_LPMODE_P0 … P7    (maps to QSFP_LOW_PWR_0    bits 0-7)
 *   8-15   QSFP_RESET_P0  … P7    (maps to QSFP_RESET_0      bits 0-7)
 *   16-47  SFP_TXDIS_P0  … P31    (maps to SFP_TX_DISABLE    bits 0-31)
 *   48-55  SFP_TXDIS_P32 … P39    (maps to SFP_TX_DISABLE_2  bits 0-7)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/rudra40_regmap.h>

#define CIENA_VENDOR_ID                  0x16fc
#define RUDRA40_DEVICE_ID                0x033b

#define SFPCTRL_ENUM2NAME(ENM) [ENM] = #ENM

/*
 * PCI device table for udev autoload.  The actual PCI binding is owned
 * by sirilx_uio; this module piggybacks via pci_get_device() in init.
 */
static const struct pci_device_id rudra40_qsfp_ctrl_ids[] = {
	{ PCI_DEVICE(CIENA_VENDOR_ID, RUDRA40_DEVICE_ID) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, rudra40_qsfp_ctrl_ids);

/*
 * GPIO pin list — 8 QSFPs + 40 SFPs
 */
enum {
	QSFP_LPMODE_P0,
	QSFP_LPMODE_P1,
	QSFP_LPMODE_P2,
	QSFP_LPMODE_P3,
	QSFP_LPMODE_P4,
	QSFP_LPMODE_P5,
	QSFP_LPMODE_P6,
	QSFP_LPMODE_P7,
	QSFP_LPMODE_MAX,
	QSFP_RESET_P0 = QSFP_LPMODE_MAX,
	QSFP_RESET_P1,
	QSFP_RESET_P2,
	QSFP_RESET_P3,
	QSFP_RESET_P4,
	QSFP_RESET_P5,
	QSFP_RESET_P6,
	QSFP_RESET_P7,
	QSFP_RESET_MAX,
	SFP_TXDIS_P0 = QSFP_RESET_MAX,
	SFP_TXDIS_P1,
	SFP_TXDIS_P2,
	SFP_TXDIS_P3,
	SFP_TXDIS_P4,
	SFP_TXDIS_P5,
	SFP_TXDIS_P6,
	SFP_TXDIS_P7,
	SFP_TXDIS_P8,
	SFP_TXDIS_P9,
	SFP_TXDIS_P10,
	SFP_TXDIS_P11,
	SFP_TXDIS_P12,
	SFP_TXDIS_P13,
	SFP_TXDIS_P14,
	SFP_TXDIS_P15,
	SFP_TXDIS_P16,
	SFP_TXDIS_P17,
	SFP_TXDIS_P18,
	SFP_TXDIS_P19,
	SFP_TXDIS_P20,
	SFP_TXDIS_P21,
	SFP_TXDIS_P22,
	SFP_TXDIS_P23,
	SFP_TXDIS_P24,
	SFP_TXDIS_P25,
	SFP_TXDIS_P26,
	SFP_TXDIS_P27,
	SFP_TXDIS_P28,
	SFP_TXDIS_P29,
	SFP_TXDIS_P30,
	SFP_TXDIS_P31,
	SFP_TXDIS_SFP_MAX,
	SFP_TXDIS_P32 = SFP_TXDIS_SFP_MAX,
	SFP_TXDIS_P33,
	SFP_TXDIS_P34,
	SFP_TXDIS_P35,
	SFP_TXDIS_P36,
	SFP_TXDIS_P37,
	SFP_TXDIS_P38,
	SFP_TXDIS_P39,
	SFP_TXDIS_QSFP_MAX,
	SFP_PIN_MAX = SFP_TXDIS_QSFP_MAX
};


static const char * const pin_names[SFP_PIN_MAX] = {
	SFPCTRL_ENUM2NAME(QSFP_LPMODE_P0),
	SFPCTRL_ENUM2NAME(QSFP_LPMODE_P1),
	SFPCTRL_ENUM2NAME(QSFP_LPMODE_P2),
	SFPCTRL_ENUM2NAME(QSFP_LPMODE_P3),
	SFPCTRL_ENUM2NAME(QSFP_LPMODE_P4),
	SFPCTRL_ENUM2NAME(QSFP_LPMODE_P5),
	SFPCTRL_ENUM2NAME(QSFP_LPMODE_P6),
	SFPCTRL_ENUM2NAME(QSFP_LPMODE_P7),
	SFPCTRL_ENUM2NAME(QSFP_RESET_P0),
	SFPCTRL_ENUM2NAME(QSFP_RESET_P1),
	SFPCTRL_ENUM2NAME(QSFP_RESET_P2),
	SFPCTRL_ENUM2NAME(QSFP_RESET_P3),
	SFPCTRL_ENUM2NAME(QSFP_RESET_P4),
	SFPCTRL_ENUM2NAME(QSFP_RESET_P5),
	SFPCTRL_ENUM2NAME(QSFP_RESET_P6),
	SFPCTRL_ENUM2NAME(QSFP_RESET_P7),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P0),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P1),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P2),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P3),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P4),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P5),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P6),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P7),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P8),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P9),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P10),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P11),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P12),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P13),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P14),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P15),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P16),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P17),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P18),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P19),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P20),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P21),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P22),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P23),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P24),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P25),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P26),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P27),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P28),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P29),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P30),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P31),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P32),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P33),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P34),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P35),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P36),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P37),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P38),
	SFPCTRL_ENUM2NAME(SFP_TXDIS_P39),
};

struct rudra40_qsfp_ctrl {
	struct gpio_chip gc;
	void __iomem    *base;
	spinlock_t       lock;
	struct pci_dev  *pdev;
	DECLARE_BITMAP(exported_pins, SFP_PIN_MAX);
};

/* Singleton — only one Rudra40 FPGA per system */
static struct rudra40_qsfp_ctrl *g_ctrl;

/* ---- helpers ------------------------------------------------------------ */

static void pin_to_reg(unsigned int offset, u32 *reg_off, unsigned int *bit)
{
	if (offset < QSFP_LPMODE_MAX) {
		/* QSFP low-power mode: pins 0-7 */
		*reg_off = RUDRA40_REG_OFFSET(RUDRA40_OPTICS_QSFP_LOW_PWR_0);
		*bit     = offset;
	} else if (offset < QSFP_RESET_MAX) {
		/* QSFP reset: pins 8-15 */
		*reg_off = RUDRA40_REG_OFFSET(RUDRA40_OPTICS_QSFP_RESET_0);
		*bit     = offset - QSFP_RESET_P0;
	} else if (offset < SFP_TXDIS_SFP_MAX) {
		/* SFP TX disable, ports 0-31: pins 16-47 */
		*reg_off = RUDRA40_REG_OFFSET(RUDRA40_OPTICS_SFP_TX_DISABLE);
		*bit     = offset - SFP_TXDIS_P0;
	} else {
		/* SFP TX disable, ports 32-39: pins 48-55 */
		*reg_off = RUDRA40_REG_OFFSET(RUDRA40_OPTICS_SFP_TX_DISABLE_2);
		*bit     = offset - SFP_TXDIS_P32;
	}
}

/* ---- gpio_chip callbacks ------------------------------------------------ */

static int rudra40_qsfp_get(struct gpio_chip *gc, unsigned int offset)
{
	struct rudra40_qsfp_ctrl *ctrl = gpiochip_get_data(gc);
	u32          reg_off, val;
	unsigned int bit;

	pin_to_reg(offset, &reg_off, &bit);
	val = ioread32(ctrl->base + reg_off);
	return !!(val & BIT(bit));
}

static void rudra40_qsfp_set(struct gpio_chip *gc, unsigned int offset,
			     int value)
{
	struct rudra40_qsfp_ctrl *ctrl = gpiochip_get_data(gc);
	u32          reg_off, val;
	unsigned int bit;
	unsigned long flags;

	pin_to_reg(offset, &reg_off, &bit);

	spin_lock_irqsave(&ctrl->lock, flags);
	val = ioread32(ctrl->base + reg_off);
	if (value)
		val |= BIT(bit);
	else
		val &= ~BIT(bit);
	iowrite32(val, ctrl->base + reg_off);
	spin_unlock_irqrestore(&ctrl->lock, flags);
}

static int rudra40_qsfp_direction_input(struct gpio_chip *gc,
				       unsigned int offset)
{
	/* Bidirectional register — allow input reads at any time */
	return 0;
}

static int rudra40_qsfp_direction_output(struct gpio_chip *gc,
					unsigned int offset, int value)
{
	rudra40_qsfp_set(gc, offset, value);
	return 0;
}

static int rudra40_qsfp_get_direction(struct gpio_chip *gc,
				     unsigned int offset)
{
	/*
	 * These are always output-capable control registers.
	 * Return 0 = GPIO_LINE_DIRECTION_OUT (kernel 5.x+).
	 */
	return 0;
}

/* ---- module init / exit ------------------------------------------------- */

static int __init rudra40_qsfp_ctrl_init(void)
{
	struct rudra40_qsfp_ctrl *ctrl = NULL;
	struct pci_dev *pdev = NULL;
	resource_size_t bar0_start = 0, bar0_len = 0;
	int ret = 0;
	int i;

	pdev = pci_get_device(CIENA_VENDOR_ID, RUDRA40_DEVICE_ID, NULL);
	if (!pdev) {
		pr_err("rudra40_qsfp_ctrl: Rudra40 FPGA PCI device not found\n");
		ret = -ENODEV;
	}

	if (!ret) {
		bar0_start = pci_resource_start(pdev, 0);
		bar0_len   = pci_resource_len(pdev, 0);
		if (!bar0_start || !bar0_len) {
			pr_err("rudra40_qsfp_ctrl: BAR0 not available\n");
			ret = -ENODEV;
		}
	}

	if (!ret) {
		ctrl = kzalloc(sizeof(*ctrl), GFP_KERNEL);
		if (!ctrl)
			ret = -ENOMEM;
	}

	if (!ret) {
		ctrl->pdev = pdev;
		spin_lock_init(&ctrl->lock);
		ctrl->base = ioremap(bar0_start, bar0_len);
		if (!ctrl->base) {
			pr_err("rudra40_qsfp_ctrl: ioremap BAR0 failed\n");
			ret = -ENOMEM;
		}
	}

	if (!ret) {
		ctrl->gc.label            = "rudra40-qsfp-ctrl";
		ctrl->gc.owner            = THIS_MODULE;
		ctrl->gc.parent           = &pdev->dev;
		ctrl->gc.base             = -1;   /* dynamic */
		ctrl->gc.ngpio            = SFP_PIN_MAX;
		ctrl->gc.names            = pin_names;
		ctrl->gc.get              = rudra40_qsfp_get;
		ctrl->gc.set              = rudra40_qsfp_set;
		ctrl->gc.direction_input  = rudra40_qsfp_direction_input;
		ctrl->gc.direction_output = rudra40_qsfp_direction_output;
		ctrl->gc.get_direction    = rudra40_qsfp_get_direction;
		ctrl->gc.can_sleep        = false;

		ret = gpiochip_add_data(&ctrl->gc, ctrl);
		if (ret)
			pr_err("rudra40_qsfp_ctrl: gpiochip_add failed: %d\n",
			       ret);
	}

	/* Export all pins to /sys/class/gpio/ with named symlinks */
	if (!ret) {
		for (i = 0; i < SFP_PIN_MAX; i++) {
			int gpio_num = ctrl->gc.base + i;
			int cur_val  = rudra40_qsfp_get(&ctrl->gc, i);
			struct gpio_desc *desc;
			int rc;

			rc = gpio_request(gpio_num, pin_names[i]);
			if (rc) {
				pr_warn("rudra40_qsfp_ctrl: gpio_request(%d) = %d\n",
					gpio_num, rc);
				continue;
			}
			set_bit(i, ctrl->exported_pins);
			gpio_direction_output(gpio_num, cur_val);

			desc = gpio_to_desc(gpio_num);
			if (!desc) {
				pr_warn("rudra40_qsfp_ctrl: gpio_to_desc(%d) failed\n",
					gpio_num);
				continue;
			}

			rc = gpiod_export(desc, false);
			if (rc)
				pr_warn("rudra40_qsfp_ctrl: gpiod_export(%d) = %d\n",
					gpio_num, rc);

			rc = gpiod_export_link(&pdev->dev, pin_names[i], desc);
			if (rc)
				pr_warn("rudra40_qsfp_ctrl: gpiod_export_link(%s) = %d\n",
					pin_names[i], rc);
		}

		g_ctrl = ctrl;
		pr_info("rudra40_qsfp_ctrl: %d control GPIOs registered "
			"(%d QSFP + %d SFP TX Disable, BAR0 @ 0x%llx)\n",
			SFP_PIN_MAX, QSFP_RESET_MAX,
			SFP_TXDIS_QSFP_MAX - SFP_TXDIS_P0,
			(unsigned long long)bar0_start);
	}

	/* Cleanup on failure */
	if (ret) {
		if (ctrl && ctrl->base)
			iounmap(ctrl->base);
		kfree(ctrl);
		if (pdev)
			pci_dev_put(pdev);
	}

	return ret;
}

static void __exit rudra40_qsfp_ctrl_exit(void)
{
	struct rudra40_qsfp_ctrl *ctrl = g_ctrl;
	int i;

	if (ctrl) {
		for (i = 0; i < SFP_PIN_MAX; i++) {
			int gpio_num = ctrl->gc.base + i;
			struct gpio_desc *desc;

			if (!test_bit(i, ctrl->exported_pins))
				continue;

			desc = gpio_to_desc(gpio_num);
			if (desc) {
				sysfs_remove_link(&ctrl->pdev->dev.kobj,
						  pin_names[i]);
				gpiod_unexport(desc);
			}
			gpio_free(gpio_num);
		}

		gpiochip_remove(&ctrl->gc);
		iounmap(ctrl->base);
		pci_dev_put(ctrl->pdev);
		kfree(ctrl);
		g_ctrl = NULL;

		pr_info("rudra40_qsfp_ctrl: removed\n");
	}
}

module_init(rudra40_qsfp_ctrl_init);
module_exit(rudra40_qsfp_ctrl_exit);

MODULE_AUTHOR("Ciena Corporation");
MODULE_DESCRIPTION("Rudra40 QSFP low-power / reset and SFP TX Disable control GPIOs");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
