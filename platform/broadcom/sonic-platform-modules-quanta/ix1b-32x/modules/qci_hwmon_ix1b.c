/*
 * A hwmon driver for Cypress PSoC fan controller and thermal sensor
 *
 * This PSoC is a specific customize design only for Quanta Switch
 * This driver is also monitoring pca9555 which is related to
 * RPSU detection and LED controll in front panel. Instead a standalone
 * driver, it should be combined with gpio lib to work correctly.
 *
 * Copyright (C) 2014 Quanta Inc.
 *
 * Author: Luffy Cheng <luffy.cheng@quantatw.com>
 *
 * Based on:
 *  adt7470.c from Darrick J. Wong <djwong@us.ibm.com>
 *  Copyright (C) 2007 IBM
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/log2.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>

#define QUANTA_IX1_PSU_GPIO_PSU1_PRSNT_N  16
#define QUANTA_IX1_PSU_GPIO_PSU1_PWRGD  17
#define QUANTA_IX1_PSU_GPIO_PSU2_PRSNT_N  19
#define QUANTA_IX1_PSU_GPIO_PSU2_PWRGD  20

#define QUANTA_IX1_FAN_GPIO_FAN1_PRSNT_N 36
#define QUANTA_IX1_FAN_GPIO_FAN2_PRSNT_N 37
#define QUANTA_IX1_FAN_GPIO_FAN3_PRSNT_N 38
#define QUANTA_IX1_FAN_GPIO_FAN4_PRSNT_N 39

#define AUTO_UPDATE_INTERVAL 10000

enum psu_nr {
	PSU1 = 0,
	PSU2
};

enum fan_nr {
	FAN1 = 0,
	FAN2,
	FAN3,
	FAN4,
};

enum led_color {
	LED_RED = 1,
	LED_GREEN,
	LED_COLOR_OFF
};

int hwmon_enable = 1;

struct qci_hwmon_data {
	struct device	*hwmon_dev;
	struct task_struct	*auto_update;
	struct completion	auto_update_stop;
	unsigned int	auto_update_interval;
	struct attribute_group	attrs;

};

static void simple_atoi(const char *buf, int *output_val)
{
	int negative = 0;

	if (buf[0] == '-')
	{
		negative = 1;
		buf++;
	}

	while (*buf >= '0' && *buf <= '9') {
		*output_val = *output_val * 10 + *buf - '0';
		buf++;
	}

	if (negative)
		*output_val = 0 - *output_val;
}

int read_gpio_file(int gpio)
{
	struct file *fp;
	char buffer[512], file_path[255];
	int offset = 0;
	int ret;

	/*open the file in read mode*/
	sprintf(file_path, "/sys/class/gpio/gpio%d/value", gpio);
	fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		printk("Cannot open the file %ld\n", PTR_ERR(fp));
		return -1;
	}

	/*Read the data to the end of the file*/
	while (1) {
		ret = kernel_read(fp, offset, buffer, 512);
		if (ret > 0) {
			offset += ret;
		}
		else {
			break;
		}
	}

	filp_close(fp, NULL);

	simple_atoi(buffer, &ret);

	return ret;
}

static ssize_t get_hwmon_status(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	return sprintf(buf, "%d\n", hwmon_enable);
}

static ssize_t set_hwmon_status(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	long enable;

	if (kstrtol(buf, 0, &enable))
		return -EINVAL;

	if ((enable != 1) && (enable != 0))
		return -EINVAL;

	hwmon_enable = enable;

	return count;
}

static u8 read_fan_present(u8 fan_nr)
{
	u8 value = 0;

	if (fan_nr == PSU1)
		value = read_gpio_file(QUANTA_IX1_FAN_GPIO_FAN1_PRSNT_N);
	else if (fan_nr == FAN2)
		value = read_gpio_file(QUANTA_IX1_FAN_GPIO_FAN2_PRSNT_N);
	else if (fan_nr == FAN3)
		value = read_gpio_file(QUANTA_IX1_FAN_GPIO_FAN3_PRSNT_N);
	else if (fan_nr == FAN4)
		value = read_gpio_file(QUANTA_IX1_FAN_GPIO_FAN4_PRSNT_N);
	else
		return -1;

	return value;
}

static u8 read_psu_present(u8 psu_nr)
{
	u8 value = 0;

	if (psu_nr == PSU1)
		value = read_gpio_file(QUANTA_IX1_PSU_GPIO_PSU1_PRSNT_N);
	else if (psu_nr == PSU2)
		value = read_gpio_file(QUANTA_IX1_PSU_GPIO_PSU2_PRSNT_N);
	else
		return -1;

	return value;
}

static u8 read_psu_AC_good(u8 psu_nr)
{
	u8 value = 0;

	if (psu_nr == PSU1)
		value = read_gpio_file(QUANTA_IX1_PSU_GPIO_PSU1_PWRGD);
	else if (psu_nr == PSU2)
		value = read_gpio_file(QUANTA_IX1_PSU_GPIO_PSU2_PWRGD);
	else
		return -1;

	return value;
}

static u8 front_led_set(char *name, char *value)
{
	struct file *fp;
	char file_path[255];
	int ret;
	mm_segment_t oldfs;

	sprintf(file_path, "/sys/class/leds/%s/brightness", name);
	fp = filp_open(file_path, O_RDWR | O_CREAT, 0644);

	if (IS_ERR(fp)) {
		printk("Cannot open the file %ld\n", PTR_ERR(fp));
		return -1;
	}

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_write(fp, value, sizeof(value), &fp->f_pos);

	set_fs(oldfs);

	filp_close(fp, NULL);

	return 0;
}

static u8 update_led(u8 *fan_status, u8 *psu_status)
{
	u8 i = 0, fan_color = LED_GREEN;

	// Update FAN front LED
	for (i = 0; i < 4; i++)
	{
		if (fan_status[i] == 1)
		{
			fan_color = LED_RED;
			front_led_set("front_led_fan_red", "1");
			front_led_set("front_led_fan_green", "0");
			break;
		}
	}

	if (fan_color == LED_GREEN)
	{
		front_led_set("front_led_fan_red", "0");
		front_led_set("front_led_fan_green", "1");
	}

	// Update PSU1 front LED
	if ((psu_status[0] == 0) && (psu_status[2] == 1))
	{
		front_led_set("front_led_psu1_green", "1");
		front_led_set("front_led_psu1_red", "0");
	}
	else if (psu_status[0] == 1)
	{
		front_led_set("front_led_psu1_green", "0");
		front_led_set("front_led_psu1_red", "0");
	}
	else
	{
		front_led_set("front_led_psu1_green", "0");
		front_led_set("front_led_psu1_red", "1");
	}

	// Update PSU2 front LED
	if ((psu_status[1] == 0) && (psu_status[3] == 1))
	{
		front_led_set("front_led_psu2_green", "1");
		front_led_set("front_led_psu2_red", "0");
	}
	else if (psu_status[1] == 1)
	{
		front_led_set("front_led_psu2_green", "0");
		front_led_set("front_led_psu2_red", "0");
	}
	else
	{
		front_led_set("front_led_psu2_green", "0");
		front_led_set("front_led_psu2_red", "1");
	}

	return 0;
}

static int led_update_thread(void *p)
{
	struct platform_device *pdev = p;
	struct qci_hwmon_data *data = platform_get_drvdata(pdev);

	u8 i = 0;
	u8 psu_status[4] = {0}; // {PSU1-2 present, PSU1-2 AC good}
	u8 fan_status[4] = {0}; // {FAN1-4 present}

	while (!kthread_should_stop()) {

		if (hwmon_enable)
		{
			for (i = 0; i < 4; i++)
			{
				fan_status[i] = read_fan_present(i);
			}

			for (i = 0; i < 2; i++)
			{
				psu_status[i] = read_psu_present(i);
				psu_status[i+2] = read_psu_AC_good(i);
			}

			update_led(fan_status, psu_status);
		}

		if (kthread_should_stop())
			break;
		msleep_interruptible(data->auto_update_interval);
	}

	complete_all(&data->auto_update_stop);
	return 0;
}

static DEVICE_ATTR(hwmon_status, S_IWUSR | S_IRUGO, get_hwmon_status, set_hwmon_status);

static struct attribute *qci_hwmon_attr[] = {

	&dev_attr_hwmon_status.attr,

	NULL
};

static int qci_hwmon_probe(struct platform_device *pdev)
{
	struct qci_hwmon_data *data;
	int err;

	data = devm_kzalloc(&pdev->dev, sizeof(struct qci_hwmon_data),
				GFP_KERNEL);
	if (!data)
	{
		return -ENOMEM;
	}

	data->auto_update_interval = AUTO_UPDATE_INTERVAL;
	data->attrs.attrs = qci_hwmon_attr;

	platform_set_drvdata(pdev, data);

	dev_info(&pdev->dev, "%s device found\n", pdev->name);

	data->hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
	}

	err = sysfs_create_group(&pdev->dev.kobj, &data->attrs);
	if (err)
		return err;

	init_completion(&data->auto_update_stop);
	data->auto_update = kthread_run(led_update_thread, pdev,
					dev_name(data->hwmon_dev));
	if (IS_ERR(data->auto_update)) {
		err = PTR_ERR(data->auto_update);
		goto exit_unregister;
	}

	return 0;

exit_unregister:
	hwmon_device_unregister(data->hwmon_dev);
	return err;
}

static int qci_hwmon_remove(struct platform_device *pdev)
{
	struct qci_hwmon_data *data = platform_get_drvdata(pdev);

	kthread_stop(data->auto_update);
	wait_for_completion(&data->auto_update_stop);
	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &data->attrs);
	return 0;
}

static struct platform_driver qci_hwmon_driver = {
	.probe = qci_hwmon_probe,
	.remove = qci_hwmon_remove,
	.driver = {
		.name = "qci-hwmon",
		.owner = THIS_MODULE,
	},
};

static int __init qci_hwmon_init(void)
{
	platform_driver_register(&qci_hwmon_driver);

	return 0;
}

static void __exit qci_hwmon_exit(void)
{
	platform_driver_unregister(&qci_hwmon_driver);
}

module_init(qci_hwmon_init);
module_exit(qci_hwmon_exit);

MODULE_AUTHOR("Quanta Computer Inc.");
MODULE_DESCRIPTION("Quanta Switch Hardware Monitor driver");
MODULE_LICENSE("GPL");
