/*
 * Juniper Networks RE-FPGA qfx platform specific driver
 *
 * Copyright (C) 2020 Juniper Networks
 * Author: Ciju Rajan K <crajank@juniper.net>
 *
 * This driver implements various features such as
 *  - ALARM led driver
 *  - Fan full speed reset control
 *  - FAN precense detection
 *  - FAN type detection
 *  - Any new QFX specific features which uses RE-FPGA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define NUM_LEDS			7	/* Max number of Alarm + FAN LEDs */

#define ALARM_MINOR_LED			0
#define ALARM_MAJOR_LED			1

#define REFPGA_PCIE_RESET_CTRL		0x13
#define REFPGA_PCIE_ALARM		0x33
#define REFPGA_FAN0_CTRL_STAT		0x28

#define REFPGA_RESET_FAN_SPEED		BIT(3)
#define REFPGA_OPER_TYPE		BIT(0)
#define REFPGA_OPER_START		BIT(1)
#define REFPGA_OPER_DONE		BIT(2)

#define TMC_REFPGA_ADDR_REG		0x0     /* TMC offset: 0x228 */
#define TMC_REFPGA_DATA_REG		0x4     /* TMC offset: 0x22C */
#define TMC_REFPGA_CTRL_REG		0x8     /* TMC offset: 0x230 */

#define TMC_REFPGA_READ_CMD		0x3
#define TMC_REFPGA_WRITE_CMD		0x2

#define REFPGA_INTR_NR_GROUPS		1
#define REFPGA_INTR_MAX_IRQS_PG		32

#define MAX_FANS			5

#define REFPGA_IRQ_MAX_BITS_PER_REG	32

#define POLL_INTERVAL			5000

#define AFI_MASK			(0x01)
#define AFO_MASK			(0x02)
#define AFI_AFO_MASK			(0x03)
/*
 * LED specific data structures
 */
struct refpga_led {
	struct led_classdev lc;
	struct work_struct work;
	int blink;
	int on;
	int bit;
	void __iomem *addr;
};

struct refpga_led_data {
	int num_leds;
	struct refpga_led *leds;
};

static DEFINE_MUTEX(alarm_led_lock);

/*
 * Common routines
 */
struct refpga_chip {
    struct refpga_led_data *led;
};

static struct refpga_chip *refpga;

static DEFINE_MUTEX(refpga_lock);

static void __iomem *tmc_membase;

static void wait_for_refpga_oper(void __iomem *base_addr)
{
	volatile u32 done = ~(-1);
	unsigned long int timeout;
	void __iomem *addr;

	addr = base_addr + (TMC_REFPGA_CTRL_REG);
	/*
	 * Wait till the transaction is complete
	 */
	timeout = jiffies + msecs_to_jiffies(100);

	do {
		usleep_range(50, 100);
		done = ioread32(addr);
		if (done & (REFPGA_OPER_DONE))
			break;
	} while(time_before(jiffies, timeout));
}
static u32 refpga_read(void __iomem *base_addr, u32 refpga_offset)
{
	u32 value;

	mutex_lock(&refpga_lock);
	iowrite32(refpga_offset, base_addr + (TMC_REFPGA_ADDR_REG));
	iowrite32(TMC_REFPGA_READ_CMD, base_addr + (TMC_REFPGA_CTRL_REG));
	wait_for_refpga_oper(base_addr);
	value = ioread32(base_addr + (TMC_REFPGA_DATA_REG));
	mutex_unlock(&refpga_lock);

	return value;
}

static void refpga_write(void __iomem *base_addr, u32 refpga_offset, u32 val)
{
	mutex_lock(&refpga_lock);
	iowrite32(refpga_offset, base_addr + (TMC_REFPGA_ADDR_REG));
	iowrite32(val, base_addr + (TMC_REFPGA_DATA_REG));
	iowrite32(TMC_REFPGA_WRITE_CMD, base_addr + (TMC_REFPGA_CTRL_REG));
	wait_for_refpga_oper(base_addr);
	mutex_unlock(&refpga_lock);
}

static bool get_fan_presense(u8 idx)
{
	u8 value = 0x00;
	u8 offset = REFPGA_FAN0_CTRL_STAT;
	bool ret = 0;
	
	value = refpga_read(tmc_membase, (offset + (idx * 2)));
        /*
         * Get the last two bits of REFPGA_FANx_CTRL_STAT.
	 * REFPGA_FANx_CTRL_STAT register of REFPGA gives the fan airflow
	 * status. There are 5 fans in QFX5200. Last two bits give the AFI
 	 * & AFO status. If any of these bits are set, fan is present.
         */
        value = (value & BIT(0)) | (value & BIT(1));
        if (value)
            ret = 1;

	return ret;
}

static int get_fan_type(u8 idx)
{
	u8 value = 0x00;
	u8 offset = REFPGA_FAN0_CTRL_STAT;
	int ret = -1;
	
	value = refpga_read(tmc_membase, (offset + (idx * 2)));
        /*
         * Get the last two bits of REFPGA_FANx_CTRL_STAT.
	 * REFPGA_FANx_CTRL_STAT register of REFPGA gives the fan airflow
	 * status. There are 5 fans in QFX5200. Last two bits give the AFI
 	 * & AFO status. If bit1 is set, it's AFO and if bit 0 is set,
	 * it's AFI.
	 *
	 * This function will return '1' for AFO, '0' for AFI, and '-1'
	 * if there is no fan or if both AFI & AFO bits are set.
         */
	value &= AFI_AFO_MASK;

	switch(value) {
		case AFI_MASK:
			ret = 0;
			break;
		case AFO_MASK:
			ret = 1;
			break;
		default:
			ret = -1;
			break;
	};

	return ret;
}

enum sysfs_fan_attributes {
	FAN0_PRESENT,
	FAN1_PRESENT,
	FAN2_PRESENT,
	FAN3_PRESENT,
	FAN4_PRESENT,
};

enum sysfs_fan_type_attributes {
	FAN0_TYPE,
	FAN1_TYPE,
	FAN2_TYPE,
	FAN3_TYPE,
	FAN4_TYPE,
};

/* 
 * The sysfs files will be present in this path
 * /sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/refpga-tmc.15/fan*_present
 * /sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/refpga-tmc.15/fan*_type
 */

#define DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(index) \
	    static SENSOR_DEVICE_ATTR(fan##index##_present, S_IRUGO, refpga_fan_presense_show, NULL, FAN##index##_PRESENT)
#define DECLARE_FAN_PRESENT_ATTR(index)      &sensor_dev_attr_fan##index##_present.dev_attr.attr

#define DECLARE_FAN_TYPE_SENSOR_DEV_ATTR(index) \
	    static SENSOR_DEVICE_ATTR(fan##index##_type, S_IRUGO, refpga_fan_type_show, NULL, FAN##index##_TYPE)
#define DECLARE_FAN_TYPE_ATTR(index)      &sensor_dev_attr_fan##index##_type.dev_attr.attr

static ssize_t refpga_fan_presense_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
	
	return sprintf(buf, "%d\n", get_fan_presense(s_attr->index));

}

static ssize_t refpga_fan_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
	
	return sprintf(buf, "%d\n", get_fan_type(s_attr->index));

}

DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(0);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(1);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(2);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(3);
DECLARE_FAN_PRESENT_SENSOR_DEV_ATTR(4);

DECLARE_FAN_TYPE_SENSOR_DEV_ATTR(0);
DECLARE_FAN_TYPE_SENSOR_DEV_ATTR(1);
DECLARE_FAN_TYPE_SENSOR_DEV_ATTR(2);
DECLARE_FAN_TYPE_SENSOR_DEV_ATTR(3);
DECLARE_FAN_TYPE_SENSOR_DEV_ATTR(4);

static struct attribute *refpga_fan_attrs[] = {
	DECLARE_FAN_PRESENT_ATTR(0),
	DECLARE_FAN_PRESENT_ATTR(1),
	DECLARE_FAN_PRESENT_ATTR(2),
	DECLARE_FAN_PRESENT_ATTR(3),
	DECLARE_FAN_PRESENT_ATTR(4),
	DECLARE_FAN_TYPE_ATTR(0),
	DECLARE_FAN_TYPE_ATTR(1),
	DECLARE_FAN_TYPE_ATTR(2),
	DECLARE_FAN_TYPE_ATTR(3),
	DECLARE_FAN_TYPE_ATTR(4),
	NULL
};

static struct attribute_group refpga_fan_attr_group = {
	.attrs = refpga_fan_attrs,
};

/*
 * There is only a single ALARM led in QFX5200 and that
 * is used for both Major & Minor alarm indicator.
 * These are represented by two different bits in RE-FPGA
 * PCIE_ALARM register. Only one of the bit (either Red or
 * Yellow) should be set a time. If both the bits are set,
 * it's an undefined behaviour.
 *
 * The following table describes how the conditions are
 * handled in the driver as there can be both Major & Minor
 * alarms can be triggered from userspace.
 *
 *  Major   Minor   Colour
 *
 *   0       0       Nil
 *   0       1      Yellow
 *   1       1       Red
 *   1       0       Red
 *
 */
static void manage_alarm_led(void __iomem *addr, int led_type, int value)
{
	static int alarm_major = 0, alarm_minor = 0;
	u32 reg = 0x0;

	mutex_lock(&alarm_led_lock);
	reg = refpga_read(addr, REFPGA_PCIE_ALARM);

	(led_type == ALARM_MAJOR_LED) ?
			((value == 1) ? (alarm_major = 1) : (alarm_major = 0)) :
			((value == 1) ? (alarm_minor = 1) : (alarm_minor = 0));
	if (alarm_major) {
		reg &= ~BIT(ALARM_MINOR_LED);
		reg |= BIT(ALARM_MAJOR_LED);
	} else {
		if (alarm_minor) {
			reg &= ~BIT(ALARM_MAJOR_LED);
			reg |= BIT(ALARM_MINOR_LED);
		} else {
			reg &= ~BIT(ALARM_MINOR_LED);
			reg &= ~BIT(ALARM_MAJOR_LED);
		}
	}
	refpga_write(addr, REFPGA_PCIE_ALARM, reg);
	mutex_unlock(&alarm_led_lock);
}

static void manage_fan_led(void __iomem *addr, int fan_slot, int value)
{
	u8 offset = REFPGA_FAN0_CTRL_STAT + (fan_slot * 2);
	u32 reg = 0x0;

	reg = refpga_read(addr, offset);
	if(value) {
		/* Turn on s/w control */
		reg = reg | BIT(4);
		/* Turn off green led */
		reg &= ~BIT(5);
		/* Turn on yellow led & make it blink */
		reg |= (BIT(6) | BIT(7));
	} else {
		/* Clear yellow led & stop blink */
		reg &= ~(BIT(6) | BIT(7));
		/* Stop s/w control */
		reg &= ~BIT(4);
	}
	refpga_write(addr, offset, reg);
}

static void refpga_led_work(struct work_struct *work)
{
	struct refpga_led *led = container_of(work, struct refpga_led, work);
	void __iomem *addr;

	addr = led->addr;
	
	if(strstr(led->lc.name, "fan"))
		manage_fan_led(addr, led->bit, led->on);
	else
		manage_alarm_led(addr, led->bit, led->on);
}

static void refpga_led_brightness_set(struct led_classdev *lc,
				 enum led_brightness brightness)
{
	struct refpga_led *led = container_of(lc, struct refpga_led, lc);

	led->on = (brightness != LED_OFF);
	led->blink = 0; /* always turn off hw blink on brightness_set() */
	schedule_work(&led->work);
}

struct led_table
{
	const char *name;
	int reg;
};

static struct led_table qfx5200_led_data[] = {
	{
		.name = "alarm-minor",
		.reg = 0,
	},
	{
		.name = "alarm-major",
		.reg = 1,
	},
	{
		.name = "fan0-fault",
		.reg = 0,
	},
	{
		.name = "fan1-fault",
		.reg = 1,
	},
	{
		.name = "fan2-fault",
		.reg = 2,
	},
	{
		.name = "fan3-fault",
		.reg = 3,
	},
	{
		.name = "fan4-fault",
		.reg = 4,
	}
};

static int refpga_led_init_one(struct device *dev,
				struct refpga_led_data *ild,
				int num)
{
	struct refpga_led *led;
	int ret = 0;

	led = &ild->leds[num];
	led->addr = tmc_membase;

	led->lc.name = qfx5200_led_data[num].name;
	led->bit = qfx5200_led_data[num].reg;
	led->lc.brightness = LED_OFF;
	led->lc.brightness_set = refpga_led_brightness_set;

	ret = devm_led_classdev_register(dev, &led->lc);
	if (ret) {
		dev_err(dev, "devm_led_classdev_register failed\n");
		return ret;
	}

	INIT_WORK(&led->work, refpga_led_work);

	return 0;
}

static int refpga_led_qfx5200_init(struct device *dev, struct refpga_led_data *ild)
{
	int ret = 0, idx = 0;

	
	if (!dev->parent) {
		dev_err(dev, "dev->parent is null\n");
		return -ENODEV;
	}

	ild->num_leds = NUM_LEDS;
	ild->leds = devm_kzalloc(dev, sizeof(struct refpga_led) * NUM_LEDS,
					GFP_KERNEL);
	if (!ild->leds) {
		dev_err(dev, "LED allocation failed\n");
		return -ENOMEM;
	}

	for(idx=0; idx<NUM_LEDS; idx++){
		ret = refpga_led_init_one(dev, ild, idx);
		if (ret)
			return ret;
	}

	return 0;
}

static int jnx_refpga_led_probe(struct platform_device *pdev)
{	
	struct device *dev = &pdev->dev;
	struct refpga_led_data *ild;
	int ret;

	ild = devm_kzalloc(dev, sizeof(*ild), GFP_KERNEL);
	if (!ild) {
		dev_err(dev, "ild allocation failed\n");
		return -ENOMEM;
	}

	ret = refpga_led_qfx5200_init(dev, ild);
	if (ret < 0)
		return ret;
	
	refpga->led = ild;

	return 0;
}

static int jnx_refpga_led_remove(struct platform_device *pdev)
{
	struct refpga_chip *drv_data = platform_get_drvdata(pdev);	
	struct refpga_led_data *ild = drv_data->led;
	int i;

	for (i = 0; i < ild->num_leds; i++) {
		devm_led_classdev_unregister(&pdev->dev, &ild->leds[i].lc);
		cancel_work_sync(&ild->leds[i].work);
	}
    if (ild) {
		if (ild->leds)
			devm_kfree(&pdev->dev, ild->leds);
		devm_kfree(&pdev->dev, ild);
	}
	return 0;
}

static void reset_fan_full_speed(struct device *dev)
{
	u32 val = ~(-1), tmp = ~(-1);

	/*
	 * Reading the REFPGA_PCIE_RESET_CTRL register
	 */
	val = refpga_read(tmc_membase, REFPGA_PCIE_RESET_CTRL);
	/*
	 * Clearing the fan full_speed bit
	 */
	val &= ~(REFPGA_RESET_FAN_SPEED);
	/*
	 * Writing the REFPGA_PCIE_RESET_CTRL register
	 */
	refpga_write(tmc_membase, REFPGA_PCIE_RESET_CTRL, val);
	/*
	 * Reading the REFPGA_PCIE_RESET_CTRL register
	 */
	tmp = refpga_read(tmc_membase, REFPGA_PCIE_RESET_CTRL);
	dev_info(dev, "After resetting fan full speed control: %X\n", tmp);
}

static int jnx_refpga_tmc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "resource allocation failed\n");
		return -ENODEV;
	}

	tmc_membase = devm_ioremap(dev, res->start, resource_size(res));
	if (!tmc_membase) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}

	refpga = devm_kzalloc(dev, sizeof(*refpga), GFP_KERNEL);
	if (!refpga) {
		dev_err(dev, "refpga memory allocation failed\n");
		return -ENOMEM;
	}

	reset_fan_full_speed(dev);

	ret = jnx_refpga_led_probe(pdev);
	if (ret != 0) {
		dev_err(dev, "Refpga LED probe failed\n");
		return ret;
	}

	dev_info(dev, "Refpga LED probe successful: TMC memoy base: %p\n",
					tmc_membase);

	ret = sysfs_create_group(&dev->kobj, &refpga_fan_attr_group);
	if (ret != 0) {
		dev_err(dev, "sysfs_create_group failed: %d\n", ret);
		return ret;
	} 

	platform_set_drvdata(pdev, refpga);

	return 0;
}

static int jnx_refpga_tmc_remove(struct platform_device *pdev)
{
	jnx_refpga_led_remove(pdev);
	sysfs_remove_group(&pdev->dev.kobj, &refpga_fan_attr_group);

	return 0;
}

static struct platform_driver jnx_refpga_tmc_driver = {
	.driver = {
		.name  = "refpga-tmc",
		.owner = THIS_MODULE,
	},
	.probe = jnx_refpga_tmc_probe,
	.remove = jnx_refpga_tmc_remove,
};

static int __init jnx_refpga_tmc_driver_init(void)
{
	int ret = -1;

	ret = platform_driver_register(&jnx_refpga_tmc_driver);
	
	return ret;

}

static void __exit jnx_refpga_tmc_driver_exit(void)
{
	platform_driver_unregister(&jnx_refpga_tmc_driver);
}

module_init(jnx_refpga_tmc_driver_init);
module_exit(jnx_refpga_tmc_driver_exit);

MODULE_DESCRIPTION("Juniper Networks REFPGA / TMC driver");
MODULE_AUTHOR("Ciju Rajan K <crajank@juniper.net>");
MODULE_LICENSE("GPL");
