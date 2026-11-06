#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/platform_device.h>

/* -------------------------------------------------------------------------- */
/* FPGA access protocol.
 */
typedef enum {
	bfpga_smbus_word_offset,
	bfpga_i2c_word_offset,
	bfpga_i2c_byte_offset,
} bfpga_i2c_proto;

/* -------------------------------------------------------------------------- */
/* Resource adjustments required by platform sub-devices.
 */
static resource_size_t ciena_siril_dev_offset(struct device *dev);
static struct resource ciena_siril_default_irq_resource(struct device *dev);
static void            ciena_siril_default_irq_deresource(struct device *dev);

#include "sirilx_platform.h"

static const unsigned long ciena_siril_bfpga_resource_type[devtype_num] = {
	[devtype_i2c]      = IORESOURCE_REG,
	[devtype_spi]      = IORESOURCE_REG,
	[devtype_cic]      = IORESOURCE_REG,
	[devtype_reset]    = IORESOURCE_REG,
	[devtype_watchdog] = IORESOURCE_REG,
	[devtype_uart]     = IORESOURCE_REG,
	[devtype_led]      = IORESOURCE_REG,
};

/* -------------------------------------------------------------------------- */
/* Resource adjustments: I2C implementation.
 */
static resource_size_t ciena_siril_dev_offset(struct device *dev)
{
	return 0;
}

static struct resource ciena_siril_default_irq_resource(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct resource    res    = { .start = client->irq,
				      .end   = client->irq,
				      .flags = IORESOURCE_IRQ, };

	dev_info(dev, "using parent irq %u\n", client->irq);

	return res;
}

static void ciena_siril_default_irq_deresource(struct device *dev)
{
}

/* -------------------------------------------------------------------------- */
static int bfpga_word_smbus_rd(void *context, unsigned int reg,
			       unsigned int *val)
{
	struct i2c_client *client = (struct i2c_client *) context;
	int rc;

	rc = i2c_smbus_write_byte_data(client, reg >> 8, reg);
	if (rc) {
		dev_err(&client->dev, "reg=0x%x write failed (%d)\n", reg, rc);
		return rc;
	}

	rc = i2c_smbus_read_byte(client);
	if (0 <= rc) {
		*val = rc;
		rc = 0;
		dev_dbg(&client->dev, "rd: reg=0x%x val=0x%x\n", reg, *val);
	}
	else dev_err(&client->dev, "reg=0x%x read failed (%d)\n", reg, rc);

	return rc;
}

/* -------------------------------------------------------------------------- */
static int bfpga_word_smbus_wr(void *context, unsigned int reg,
			       unsigned int val)
{
	struct i2c_client *client = (struct i2c_client *) context;
	u8 data[2] = { reg, val };
	int rc;

	rc = i2c_smbus_write_i2c_block_data(client, reg >> 8,
					    ARRAY_SIZE(data), data);

	if (rc) dev_err(&client->dev, "reg=0x%x val=0x%x write failed (%d)\n",
			reg, val, rc);
	else dev_dbg(&client->dev, "wr: reg=0x%x val=0x%x\n", reg, val);

	return rc;
}

/* -------------------------------------------------------------------------- */
static int bfpga_i2c_xfer(void *context, unsigned int reg, unsigned int *val,
			  bool word_off, bool do_write)
{
	struct i2c_client *client = (struct i2c_client *) context;

	unsigned char offset_buf[2] = { reg, 0 };
	unsigned char data_buf      = do_write ? *val : 0;

	struct i2c_msg xfer_msgs[2] = {
		{ .addr  = client->addr,
		  .len   = word_off ? 2 : 1,
		  .buf   = offset_buf, },
		{ .addr  = client->addr,
		  .flags = do_write ? I2C_M_NOSTART : I2C_M_RD,
		  .len   = 1,
		  .buf   = &data_buf, },
	};

	int nmsgs = ARRAY_SIZE(xfer_msgs);
	int rc;

	if (word_off) {
		offset_buf[0] = reg >> 8;
		offset_buf[1] = reg;
	}

	rc = i2c_transfer(client->adapter, xfer_msgs, nmsgs);

	if (nmsgs != rc) {
		dev_err(&client->dev, "%s xfer failed: rc=%d\n",
			do_write ? "write" : "read", rc);
		if (0 <= rc) rc= -ECOMM;
		return rc;
	}

	if (!do_write) *val = data_buf;

	return 0;
}

/* -------------------------------------------------------------------------- */
static int bfpga_word_i2c_rd(void *context, unsigned int reg,
			     unsigned int *val)
{
	return bfpga_i2c_xfer(context, reg, val, true, false);
}

/* -------------------------------------------------------------------------- */
static int bfpga_word_i2c_wr(void *context, unsigned int reg,
			     unsigned int val)
{
	return bfpga_i2c_xfer(context, reg, &val, true, true);
}

/* -------------------------------------------------------------------------- */
static int bfpga_byte_i2c_rd(void *context, unsigned int reg,
			     unsigned int *val)
{
	return bfpga_i2c_xfer(context, reg, val, false, false);
}

/* -------------------------------------------------------------------------- */
static int bfpga_byte_i2c_wr(void *context, unsigned int reg,
			     unsigned int val)
{
	return bfpga_i2c_xfer(context, reg, &val, false, true);
}

/* -------------------------------------------------------------------------- */
static int ciena_bfpga_i2c_probe(struct i2c_client *client)
{
	struct regmap_config rmap_config = CIENA_REGMAP_CONFIG;
	struct device        *dev        = &client->dev;
	struct siril_priv    *priv;
	bfpga_i2c_proto      bfpga_i2c_access;
	int rc;

	/* BFPGA_I2C_ACCESS_PROTO must be set in foo_priv.h */
	bfpga_i2c_access = BFPGA_I2C_ACCESS_PROTO;

	/*
	 * Set up resource types
	 */
	ciena_siril_resource_type = ciena_siril_bfpga_resource_type;

	priv = devm_kzalloc(dev, sizeof(struct siril_priv), GFP_KERNEL);
	if (priv == 0)
		return -ENOMEM;

	i2c_set_clientdata(client, priv);

	switch (bfpga_i2c_access) {
	case bfpga_smbus_word_offset:
		rmap_config.reg_bits  = 16;
		rmap_config.reg_read  = bfpga_word_smbus_rd;
		rmap_config.reg_write = bfpga_word_smbus_wr;
		break;
	case bfpga_i2c_word_offset:
		rmap_config.reg_bits  = 16;
		rmap_config.reg_read  = bfpga_word_i2c_rd;
		rmap_config.reg_write = bfpga_word_i2c_wr;
		break;
	case bfpga_i2c_byte_offset:
		rmap_config.reg_read  = bfpga_byte_i2c_rd;
		rmap_config.reg_write = bfpga_byte_i2c_wr;
		break;
	default:
		dev_warn(dev, "unknown protocol: %d\n", bfpga_i2c_access);
	        return -EINVAL;
	}

	priv->regs[0] = devm_regmap_init(dev, NULL, client, &rmap_config);
	if (IS_ERR(priv->regs[0])) {
		rc = PTR_ERR(priv->regs[0]);
		dev_warn(dev, "Could not create the regmap (%d)\n", rc);
	        return rc;
	}

	if (sirilx_pdata.sirilx_apply_early_fixups)
		sirilx_pdata.sirilx_apply_early_fixups(priv->regs[0]);

	rc = ciena_siril_create_sub_devices(priv, dev);

	if (!rc) ciena_siril_post_functions(dev, true);

	return rc;
}

static void ciena_bfpga_i2c_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;

	ciena_siril_post_functions(dev, false);
	ciena_siril_destroy_sub_devices(dev);
}

/* Actual module infrastructure. */
static const struct of_device_id ciena_bfpga_of_match[] = {
	{ .compatible = CIENA_BFPGA_COMPATIBLE_NAME, },
	{},
};

static const struct i2c_device_id bfpga_i2c_id[] = {
	{ .name = MOD_NAME, },
	{},
};

MODULE_DEVICE_TABLE(i2c, bfpga_i2c_id);

MODULE_DEVICE_TABLE(of, ciena_bfpga_of_match);

static struct i2c_driver ciena_bfpga_driver = {
	.driver	      = {
		.name           = MOD_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = ciena_bfpga_of_match,
	},
	.probe     = ciena_bfpga_i2c_probe,
	.remove    = ciena_bfpga_i2c_remove,
	.id_table  = bfpga_i2c_id,
};
module_i2c_driver(ciena_bfpga_driver);

MODULE_DESCRIPTION("I2C driver for the " MOD_NAME " FPGA");
MODULE_AUTHOR("Marc St-Amand <mstamand@ciena.com>");
MODULE_LICENSE("GPL v2");
