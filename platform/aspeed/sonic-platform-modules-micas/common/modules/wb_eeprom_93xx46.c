// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for 93xx46 EEPROMs
 *
 * (C) 2011 DENX Software Engineering, Anatolij Gustschin <agust@denx.de>
 */

#include <linux/bits.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0)
#include <linux/kstrtox.h>
#else
#include <linux/kernel.h>
#endif
#include <linux/log2.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>

#include <linux/nvmem-provider.h>
#include <linux/string.h>
#include "wb_93xx46_common.h"
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define OP_START	0x4
#define OP_WRITE	(OP_START | 0x1)
#define OP_READ		(OP_START | 0x2)
#define ADDR_EWDS	0x00
#define ADDR_ERAL	0x20
#define ADDR_EWEN	0x30

struct eeprom_93xx46_devtype_data {
	unsigned int quirks;
	unsigned char flags;
};

static const struct eeprom_93xx46_devtype_data at93c46_data = {
	.flags = EE_SIZE1K,
};

static const struct eeprom_93xx46_devtype_data at93c56_data = {
	.flags = EE_SIZE2K,
    .quirks = EEPROM_93XXC56_QUIRK_EXTRA_READ_CYCLE,
};

static const struct eeprom_93xx46_devtype_data at93c66_data = {
	.flags = EE_SIZE4K,
};

static const struct eeprom_93xx46_devtype_data atmel_at93c46d_data = {
	.flags = EE_SIZE1K,
	.quirks = EEPROM_93XX46_QUIRK_SINGLE_WORD_READ |
		  EEPROM_93XX46_QUIRK_INSTRUCTION_LENGTH,
};

static const struct eeprom_93xx46_devtype_data microchip_93lc46b_data = {
	.flags = EE_SIZE1K,
	.quirks = EEPROM_93XX46_QUIRK_EXTRA_READ_CYCLE,
};

struct eeprom_93xx46_dev {
	struct spi_device *spi;
	struct eeprom_93xx46_platform_data *pdata;
	struct mutex lock;
	struct nvmem_config nvmem_config;
	struct nvmem_device *nvmem;
	int addrlen;
	int size;
};

static inline bool has_quirk_single_word_read(struct eeprom_93xx46_dev *edev)
{
	return edev->pdata->quirks & EEPROM_93XX46_QUIRK_SINGLE_WORD_READ;
}

static inline bool has_quirk_instruction_length(struct eeprom_93xx46_dev *edev)
{
	return edev->pdata->quirks & EEPROM_93XX46_QUIRK_INSTRUCTION_LENGTH;
}

static inline bool has_quirk_extra_read_cycle(struct eeprom_93xx46_dev *edev)
{
	return edev->pdata->quirks & EEPROM_93XX46_QUIRK_EXTRA_READ_CYCLE;
}

static inline bool has_quirk_extra_cycle_gt93c56a(struct eeprom_93xx46_dev *edev)
{
	return edev->pdata->quirks & EEPROM_93XXC56_QUIRK_EXTRA_READ_CYCLE;
}

static int eeprom_93xx46_read(void *priv, unsigned int off,
			      void *val, size_t count)
{
	struct eeprom_93xx46_dev *edev = priv;
	char *buf = val;
	int err = 0;
	int bits;
	size_t nbytes;
	int data_bit;

	if (unlikely(off >= edev->size))
		return 0;
	if ((off + count) > edev->size)
		count = edev->size - off;
	if (unlikely(!count))
		return count;

	mutex_lock(&edev->lock);

	gpiod_set_value_cansleep(edev->pdata->select, 1);

	/* The opcode in front of the address is three bits. */
	bits = edev->addrlen + 3;

	while (count) {
		struct spi_message m;
		struct spi_transfer t[2] = {};

		u16 cmd_addr = OP_READ << edev->addrlen;
        if (has_quirk_extra_cycle_gt93c56a(edev)) {
            cmd_addr = cmd_addr << 1;
            bits += 1;
        }
		nbytes = count;

		if (edev->pdata->flags & EE_ADDR8) {
			cmd_addr |= off;
			data_bit = 8;

			if (has_quirk_single_word_read(edev))
				nbytes = 1;
		} else {
			cmd_addr |= (off >> 1);
			data_bit = 16;
			if (has_quirk_single_word_read(edev))
				nbytes = 2;
		}

		dev_dbg(&edev->spi->dev, "read cmd 0x%x, %d Hz, bits %d addrlen %d\n",
			cmd_addr, edev->spi->max_speed_hz, bits, edev->addrlen);

		if (has_quirk_extra_read_cycle(edev)) {
			cmd_addr <<= 1;
			bits += 1;
		}

		t[0].tx_buf = (char *)&cmd_addr;
		t[0].len = 2;
		t[0].bits_per_word = bits;

		t[1].rx_buf = buf;
		t[1].len = nbytes;
		t[1].bits_per_word = data_bit;

		spi_message_init_with_transfers(&m, t, ARRAY_SIZE(t));

		err = spi_sync(edev->spi, &m);
		/* have to wait at least Tcsl ns */
		ndelay(250);

		if (err) {
			dev_err(&edev->spi->dev, "read %zu bytes at %u: err. %d\n",
				nbytes, off, err);
			break;
		}

		buf += nbytes;
		off += nbytes;
		count -= nbytes;
	}

	gpiod_set_value_cansleep(edev->pdata->select, 0);

	mutex_unlock(&edev->lock);

	return err;
}

static int eeprom_93xx46_ew(struct eeprom_93xx46_dev *edev, int is_on)
{
	struct spi_message m;
	struct spi_transfer t = {};
	int bits, ret;
	u16 cmd_addr;

	/* The opcode in front of the address is three bits. */
	bits = edev->addrlen + 3;

    if (has_quirk_extra_cycle_gt93c56a(edev)) {
        cmd_addr = OP_START << (edev->addrlen + 1);
    } else {
        cmd_addr = OP_START << edev->addrlen;
    }

	if (edev->pdata->flags & EE_ADDR8)
        if (has_quirk_extra_cycle_gt93c56a(edev)) {
            cmd_addr |= (is_on ? ADDR_EWEN : ADDR_EWDS) << 3;
            bits += 1;
        } else {
            cmd_addr |= (is_on ? ADDR_EWEN : ADDR_EWDS) << 1;
        }
	else{
        if (has_quirk_extra_cycle_gt93c56a(edev)) {
            cmd_addr |= (is_on ? ADDR_EWEN : ADDR_EWDS) << 2;
            bits += 1;
        } else {
            cmd_addr |= (is_on ? ADDR_EWEN : ADDR_EWDS);
        }
   }
	if (has_quirk_instruction_length(edev)) {
		cmd_addr <<= 2;
		bits += 2;
	}

	dev_dbg(&edev->spi->dev, "ew %s cmd 0x%04x, %d bits\n",
		str_enable_disable(is_on), cmd_addr, bits);

	t.tx_buf = &cmd_addr;
	t.len = 2;
	t.bits_per_word = bits;

	spi_message_init_with_transfers(&m, &t, 1);

	mutex_lock(&edev->lock);

	gpiod_set_value_cansleep(edev->pdata->select, 1);

	ret = spi_sync(edev->spi, &m);
	/* have to wait at least Tcsl ns */
	ndelay(250);
	if (ret)
		dev_err(&edev->spi->dev, "erase/write %s error %d\n",
			str_enable_disable(is_on), ret);

	gpiod_set_value_cansleep(edev->pdata->select, 0);

	mutex_unlock(&edev->lock);
	return ret;
}

static ssize_t
eeprom_93xx46_write_word(struct eeprom_93xx46_dev *edev,
			 const char *buf, unsigned off)
{
	struct spi_message m;
	struct spi_transfer t[2] = {};
	int bits, data_len, ret;
	u16 cmd_addr;
	int data_bit;

	if (unlikely(off >= edev->size))
		return -EINVAL;

	/* The opcode in front of the address is three bits. */
	bits = edev->addrlen + 3;

	cmd_addr = OP_WRITE << edev->addrlen;
    if (has_quirk_extra_cycle_gt93c56a(edev)) {
        cmd_addr = cmd_addr << 1;
        bits += 1;
    }

	if (edev->pdata->flags & EE_ADDR8) {
		cmd_addr |= off;
		data_len = 1;
		data_bit = 8;

	} else {
		cmd_addr |= (off >> 1);
		data_len = 2;
		data_bit = 16;
	}

	dev_dbg(&edev->spi->dev, "write cmd 0x%x bits %d addrlen %d\n", cmd_addr, bits, edev->addrlen);

	t[0].tx_buf = (char *)&cmd_addr;
	t[0].len = 2;
	t[0].bits_per_word = bits;

	t[1].tx_buf = buf;
	t[1].len = data_len;
	t[1].bits_per_word = data_bit;

	spi_message_init_with_transfers(&m, t, ARRAY_SIZE(t));

	ret = spi_sync(edev->spi, &m);
	/* have to wait program cycle time Twc ms */
	mdelay(6);
	return ret;
}

static int eeprom_93xx46_write(void *priv, unsigned int off,
				   void *val, size_t count)
{
	struct eeprom_93xx46_dev *edev = priv;
	char *buf = val;
	int ret, step = 1;
	unsigned int i;

	if (unlikely(off >= edev->size))
		return -EFBIG;
	if ((off + count) > edev->size)
		count = edev->size - off;
	if (unlikely(!count))
		return count;

	/* only write even number of bytes on 16-bit devices */
	if (edev->pdata->flags & EE_ADDR16) {
		step = 2;
		count &= ~1;
	}

	/* erase/write enable */
	ret = eeprom_93xx46_ew(edev, 1);
	if (ret)
		return ret;

	mutex_lock(&edev->lock);

	gpiod_set_value_cansleep(edev->pdata->select, 1);

	for (i = 0; i < count; i += step) {
		ret = eeprom_93xx46_write_word(edev, &buf[i], off + i);
		if (ret) {
			dev_err(&edev->spi->dev, "write failed at %u: %d\n", off + i, ret);
			break;
		}
	}

	gpiod_set_value_cansleep(edev->pdata->select, 0);

	mutex_unlock(&edev->lock);

	/* erase/write disable */
	eeprom_93xx46_ew(edev, 0);
	return ret;
}

static int eeprom_93xx46_eral(struct eeprom_93xx46_dev *edev)
{
	struct spi_message m;
	struct spi_transfer t = {};
	int bits, ret;
	u16 cmd_addr;

	/* The opcode in front of the address is three bits. */
	bits = edev->addrlen + 3;

    if (has_quirk_extra_cycle_gt93c56a(edev)) {
        cmd_addr = OP_START << (edev->addrlen + 1);
    } else {
        cmd_addr = OP_START << edev->addrlen;
    }
	if (edev->pdata->flags & EE_ADDR8)
        if (has_quirk_extra_cycle_gt93c56a(edev)) {
            cmd_addr |= ADDR_ERAL << 3;
            bits += 1;
        } else {
            cmd_addr |= ADDR_ERAL << 1;
        }
	else {
        if (has_quirk_extra_cycle_gt93c56a(edev)) {
            cmd_addr |= ADDR_ERAL << 2;
            bits += 1;
        } else {
            cmd_addr |= ADDR_ERAL;
        }
    }
	if (has_quirk_instruction_length(edev)) {
		cmd_addr <<= 2;
		bits += 2;
	}

	dev_dbg(&edev->spi->dev, "eral cmd 0x%04x, %d bits\n", cmd_addr, bits);

	t.tx_buf = &cmd_addr;
	t.len = 2;
	t.bits_per_word = bits;

	spi_message_init_with_transfers(&m, &t, 1);

	mutex_lock(&edev->lock);

	gpiod_set_value_cansleep(edev->pdata->select, 1);

	ret = spi_sync(edev->spi, &m);
	if (ret)
		dev_err(&edev->spi->dev, "erase error %d\n", ret);
	/* have to wait erase cycle time Tec ms */
	mdelay(6);

	gpiod_set_value_cansleep(edev->pdata->select, 0);

	mutex_unlock(&edev->lock);
	return ret;
}

static ssize_t erase_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct eeprom_93xx46_dev *edev = dev_get_drvdata(dev);
	bool erase;
	int ret;

	ret = kstrtobool(buf, &erase);
	if (ret)
		return ret;

	if (erase) {
		ret = eeprom_93xx46_ew(edev, 1);
		if (ret)
			return ret;
		ret = eeprom_93xx46_eral(edev);
		if (ret)
			return ret;
		ret = eeprom_93xx46_ew(edev, 0);
		if (ret)
			return ret;
	}
	return count;
}
static DEVICE_ATTR_WO(erase);

static const struct of_device_id eeprom_93xx46_of_table[] = {
	{ .compatible = "wb-eeprom-93xx46", .data = &at93c46_data, },
	{ .compatible = "wb-atmel,at93c46", .data = &at93c46_data, },
	{ .compatible = "wb-atmel,at93c46d", .data = &atmel_at93c46d_data, },
	{ .compatible = "wb-atmel,at93c56", .data = &at93c56_data, },
	{ .compatible = "wb-atmel,at93c66", .data = &at93c66_data, },
	{ .compatible = "wb-microchip,93lc46b", .data = &microchip_93lc46b_data, },
	{}
};
MODULE_DEVICE_TABLE(of, eeprom_93xx46_of_table);

static const struct spi_device_id eeprom_93xx46_spi_ids[] = {
	{ .name = "wb_93xx46",
	  .driver_data = (kernel_ulong_t)&at93c46_data, },
	{ .name = "wb_eeprom-93xx46",
	  .driver_data = (kernel_ulong_t)&at93c46_data, },
	{ .name = "wb_at93c46",
	  .driver_data = (kernel_ulong_t)&at93c46_data, },
	{ .name = "wb_at93c46d",
	  .driver_data = (kernel_ulong_t)&atmel_at93c46d_data, },
	{ .name = "wb_at93c56",
	  .driver_data = (kernel_ulong_t)&at93c56_data, },
	{ .name = "wb_at93c66",
	  .driver_data = (kernel_ulong_t)&at93c66_data, },
	{ .name = "wb_93lc46b",
	  .driver_data = (kernel_ulong_t)&microchip_93lc46b_data, },
	{}
};
MODULE_DEVICE_TABLE(spi, eeprom_93xx46_spi_ids);

static int eeprom_93xx46_probe_fw(struct device *dev)
{
	const struct eeprom_93xx46_devtype_data *data;
	struct eeprom_93xx46_platform_data *pd;
	u32 tmp;
	int ret;

	pd = devm_kzalloc(dev, sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return -ENOMEM;

	tmp = 0;
	ret = device_property_read_u32(dev, "data-size", &tmp);
	if (ret < 0) {
		dev_err(dev, "data-size property not found\n");
		return ret;
	}

	if (tmp == 8) {
		pd->flags |= EE_ADDR8;
	} else if (tmp == 16) {
		pd->flags |= EE_ADDR16;
	} else {
		dev_err(dev, "invalid data-size (%d)\n", tmp);
		return -EINVAL;
	}

	if (device_property_read_bool(dev, "read-only"))
		pd->flags |= EE_READONLY;

	pd->select = devm_gpiod_get_optional(dev, "select", GPIOD_OUT_LOW);
	if (IS_ERR(pd->select))
		return PTR_ERR(pd->select);
	gpiod_set_consumer_name(pd->select, "93xx46 EEPROMs OE");

	data = (struct eeprom_93xx46_devtype_data *)spi_get_device_id(to_spi_device(dev))->driver_data;
	if (data) {
		pd->quirks = data->quirks;
		pd->flags |= data->flags;
	}

	dev->platform_data = pd;

	return 0;
}

static int eeprom_93xx46_probe(struct spi_device *spi)
{
	struct eeprom_93xx46_platform_data *pd;
	struct eeprom_93xx46_dev *edev;
	struct device *dev = &spi->dev;
	int err;
	if (spi->dev.of_node) {
		err = eeprom_93xx46_probe_fw(dev);
		if (err < 0)
			return err;
	}

	pd = spi->dev.platform_data;
	if (!pd) {
		dev_err(&spi->dev, "missing platform data\n");
		return -ENODEV;
	}

    dev_dbg(&spi->dev, "pd->flags 0x%x\n", pd->flags);
    dev_dbg(&spi->dev, "pd->quirks 0x%x\n", pd->quirks);
	edev = devm_kzalloc(&spi->dev, sizeof(*edev), GFP_KERNEL);
	if (!edev)
		return -ENOMEM;

	if (pd->flags & EE_SIZE1K)
		edev->size = 128;
	else if (pd->flags & EE_SIZE2K)
		edev->size = 256;
	else if (pd->flags & EE_SIZE4K)
		edev->size = 512;
	else {
		dev_err(&spi->dev, "unspecified size\n");
		return -EINVAL;
	}

	if (pd->flags & EE_ADDR8)
		edev->addrlen = ilog2(edev->size);
	else if (pd->flags & EE_ADDR16)
		edev->addrlen = ilog2(edev->size) - 1;
	else {
		dev_err(&spi->dev, "unspecified address type\n");
		return -EINVAL;
	}

	mutex_init(&edev->lock);

	edev->spi = spi;
	edev->pdata = pd;

	edev->nvmem_config.type = NVMEM_TYPE_EEPROM;
	edev->nvmem_config.name = dev_name(&spi->dev);
	edev->nvmem_config.dev = &spi->dev;
	edev->nvmem_config.read_only = pd->flags & EE_READONLY;
	edev->nvmem_config.root_only = true;
	edev->nvmem_config.owner = THIS_MODULE;
	edev->nvmem_config.compat = true;
	edev->nvmem_config.base_dev = &spi->dev;
	edev->nvmem_config.reg_read = eeprom_93xx46_read;
	edev->nvmem_config.reg_write = eeprom_93xx46_write;
	edev->nvmem_config.priv = edev;
	edev->nvmem_config.stride = 4;
	edev->nvmem_config.word_size = 1;
	edev->nvmem_config.size = edev->size;

	edev->nvmem = devm_nvmem_register(&spi->dev, &edev->nvmem_config);
	if (IS_ERR(edev->nvmem))
		return PTR_ERR(edev->nvmem);
	if (debug) {
		dev_info(&spi->dev, "%d-bit eeprom containing %d bytes %s\n",
			(pd->flags & EE_ADDR8) ? 8 : 16,
			edev->size,
			(pd->flags & EE_READONLY) ? "(readonly)" : "");
	}

	if (!(pd->flags & EE_READONLY)) {
		if (device_create_file(&spi->dev, &dev_attr_erase))
			dev_err(&spi->dev, "can't create erase interface\n");
	}

	spi_set_drvdata(spi, edev);
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int eeprom_93xx46_remove(struct spi_device *spi)
#else 
static void eeprom_93xx46_remove(struct spi_device *spi)
#endif
{
	struct eeprom_93xx46_dev *edev = spi_get_drvdata(spi);

	if (!(edev->pdata->flags & EE_READONLY))
		device_remove_file(&spi->dev, &dev_attr_erase);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
	return 0;
#endif
}

static struct spi_driver wb_eeprom_93xx46_driver = {
	.driver = {
		.name	= "wb_93xx46",
		.of_match_table = eeprom_93xx46_of_table,
	},
	.probe		= eeprom_93xx46_probe,
	.remove		= eeprom_93xx46_remove,
	.id_table	= eeprom_93xx46_spi_ids,
};

module_spi_driver(wb_eeprom_93xx46_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for 93xx46 EEPROMs");
MODULE_AUTHOR("support");
MODULE_ALIAS("spi:93xx46");
