/*
 * csu550.c - PMBUS for csu550
 *
 * Copyright (c) 2018 sonic_rd <sonic_rd@ruijie.com.cn>
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,152)
#include <linux/i2c/pmbus.h>
#else
#include <linux/pmbus.h>
#endif
#include <linux/err.h>
#include <linux/mutex.h>
#include "rg_pmbus.h"

/*
 * Find sensor groups and status registers on each page.
 */
static void pmbus_find_sensor_groups(struct i2c_client *client,
				     struct pmbus_driver_info *info)
{
	int page;

	/* Sensors detected on page 0 only */
	if (rg_pmbus_check_word_register(client, 0, PMBUS_READ_VIN))
		info->func[0] |= PMBUS_HAVE_VIN;
	if (rg_pmbus_check_word_register(client, 0, PMBUS_READ_IIN))
		info->func[0] |= PMBUS_HAVE_IIN;
	if (rg_pmbus_check_word_register(client, 0, PMBUS_READ_PIN))
		info->func[0] |= PMBUS_HAVE_PIN;
	if (info->func[0]
	    && rg_pmbus_check_byte_register(client, 0, PMBUS_STATUS_INPUT))
		info->func[0] |= PMBUS_HAVE_STATUS_INPUT;
	if (rg_pmbus_check_byte_register(client, 0, PMBUS_FAN_CONFIG_12) &&
	    rg_pmbus_check_word_register(client, 0, PMBUS_READ_FAN_SPEED_1)) {
		info->func[0] |= PMBUS_HAVE_FAN12;
		if (rg_pmbus_check_byte_register(client, 0, PMBUS_STATUS_FAN_12))
			info->func[0] |= PMBUS_HAVE_STATUS_FAN12;
	}
	if (rg_pmbus_check_word_register(client, 0, PMBUS_READ_TEMPERATURE_1))
		info->func[0] |= PMBUS_HAVE_TEMP;
	if (rg_pmbus_check_word_register(client, 0, PMBUS_READ_TEMPERATURE_2))
		info->func[0] |= PMBUS_HAVE_TEMP2;
	if (rg_pmbus_check_word_register(client, 0, PMBUS_READ_TEMPERATURE_3))
		info->func[0] |= PMBUS_HAVE_TEMP3;
	if (info->func[0] & (PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2
			     | PMBUS_HAVE_TEMP3)
	    && rg_pmbus_check_byte_register(client, 0,
					 PMBUS_STATUS_TEMPERATURE))
			info->func[0] |= PMBUS_HAVE_STATUS_TEMP;

	/* Sensors detected on all pages */
	for (page = 0; page < info->pages; page++) {
		if (rg_pmbus_check_word_register(client, page, PMBUS_READ_VOUT)) {
			info->func[page] |= PMBUS_HAVE_VOUT;
			if (rg_pmbus_check_byte_register(client, page,
						      PMBUS_STATUS_VOUT))
				info->func[page] |= PMBUS_HAVE_STATUS_VOUT;
		}
		if (rg_pmbus_check_word_register(client, page, PMBUS_READ_IOUT)) {
			info->func[page] |= PMBUS_HAVE_IOUT;
			if (rg_pmbus_check_byte_register(client, 0,
						      PMBUS_STATUS_IOUT))
				info->func[page] |= PMBUS_HAVE_STATUS_IOUT;
		}
		if (rg_pmbus_check_word_register(client, page, PMBUS_READ_POUT))
			info->func[page] |= PMBUS_HAVE_POUT;
	}
}

/*
 * Identify chip parameters.
 */
static int pmbus_identify(struct i2c_client *client,
			  struct pmbus_driver_info *info)
{
	int ret = 0;

	if (!info->pages) {
		/*
		 * Check if the PAGE command is supported. If it is,
		 * keep setting the page number until it fails or until the
		 * maximum number of pages has been reached. Assume that
		 * this is the number of pages supported by the chip.
		 */
		if (rg_pmbus_check_byte_register(client, 0, PMBUS_PAGE)) {
			int page;

			for (page = 1; page < PMBUS_PAGES; page++) {
				if (rg_pmbus_set_page(client, page) < 0)
					break;
			}
			(void)rg_pmbus_set_page(client, 0);
			info->pages = page;
		} else {
			info->pages = 1;
		}
	}

	if (rg_pmbus_check_byte_register(client, 0, PMBUS_VOUT_MODE)) {
		int vout_mode;

		vout_mode = rg_pmbus_read_byte_data(client, 0, PMBUS_VOUT_MODE);
		if (vout_mode >= 0 && vout_mode != 0xff) {
			switch (vout_mode >> 5) {
			case 0:
				break;
			case 1:
				info->format[PSC_VOLTAGE_OUT] = vid;
				break;
			case 2:
				info->format[PSC_VOLTAGE_OUT] = direct;
				break;
			default:
				ret = -ENODEV;
				goto abort;
			}
		}
	}

	/*
	 * We should check if the COEFFICIENTS register is supported.
	 * If it is, and the chip is configured for direct mode, we can read
	 * the coefficients from the chip, one set per group of sensor
	 * registers.
	 *
	 * To do this, we will need access to a chip which actually supports the
	 * COEFFICIENTS command, since the command is too complex to implement
	 * without testing it. Until then, abort if a chip configured for direct
	 * mode was detected.
	 */
	if (info->format[PSC_VOLTAGE_OUT] == direct) {
		ret = -ENODEV;
		goto abort;
	}

	/* Try to find sensor groups  */
	pmbus_find_sensor_groups(client, info);
abort:
	return ret;
}

static int pmbus_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	struct pmbus_driver_info *info;
	struct pmbus_platform_data *pdata = NULL;
	struct device *dev = &client->dev;

	info = devm_kzalloc(&client->dev, sizeof(struct pmbus_driver_info),
			    GFP_KERNEL);
	if (!info)
		return -ENOMEM;

    if (!strncmp(id->name, "dps460", sizeof("dps460")) ||
		!strncmp(id->name, "rg_fsp1200", sizeof("rg_fsp1200")) || !strncmp(id->name, "rg_dps550", sizeof("rg_dps550"))) {
        pdata = kzalloc(sizeof(struct pmbus_platform_data), GFP_KERNEL);
        if (!pdata) {
                kfree(info);
                return -ENOMEM;
        }
            pdata->flags = PMBUS_SKIP_STATUS_CHECK;
    }

	info->pages = id->driver_data;
	info->identify = pmbus_identify;
	dev->platform_data = pdata;

	return rg_pmbus_do_probe(client, id, info);
}

static const struct i2c_device_id pmbus_id[] = {
   {"rg_csu550", 0},
   {"rg_csu800", 1},
   {"rg_fsp1200", 1},
   {"rg_dps550", 1},
   {}
};
MODULE_DEVICE_TABLE(i2c, pmbus_id);

/* This is the driver that will be inserted */
static struct i2c_driver pmbus_driver = {
	.driver = {
		   .name = "rg_pmbus",
		   },
	.probe = pmbus_probe,
	.remove = rg_pmbus_do_remove,
	.id_table = pmbus_id,
};

module_i2c_driver(pmbus_driver);

MODULE_AUTHOR("sonic_rd <sonic_rd@ruijie.com.cn>");
MODULE_DESCRIPTION("ruijie psupmbus driver");
MODULE_LICENSE("GPL");
