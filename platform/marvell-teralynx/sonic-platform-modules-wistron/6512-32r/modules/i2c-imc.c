// SPDX-License-Identifier: GPL-2.0

/*
 * Intel Memory Controller iMC SMBus Driver to DIMMs.
 *
 * Copyright (c) 2013-2016 Andrew Lutomirski <luto@kernel.org>
 * Copyright (c) 2020 Stefan Schaeckeler <sschaeck@cisco.com>, Cisco Systems
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/i2c.h>

/* iMC Main, PCI dev 0x13, fn 0, 8086.6fa8 */
#define PCI_DEVICE_ID_INTEL_BROADWELL_IMC_TA 0x6fa8

/* Register offsets for channel pairs 0+1 and 2+3 */
#define SMBSTAT(i)                 (0x180 + 0x10*(i))
#define SMBCMD(i)                  (0x184 + 0x10*(i))
#define SMBCNTL(i)                 (0x188 + 0x10*(i))

/* SMBSTAT fields */
#define SMBSTAT_RDO                (1U << 31)      /* Read Data Valid */
#define SMBSTAT_WOD                (1U << 30)      /* Write Operation Done */
#define SMBSTAT_SBE                (1U << 29)      /* SMBus Error */
#define SMBSTAT_SMB_BUSY           (1U << 28)      /* SMBus Busy State */
#define SMBSTAT_RDATA_MASK         0xffff          /* Result of a read */

/* SMBCMD fields */
#define SMBCMD_TRIGGER             (1U << 31)      /* CMD Trigger */
#define SMBCMD_WORD_ACCESS         (1U << 29)      /* Word (vs byte) access */
#define SMBCMD_TYPE_READ           (0U << 27)      /* Read */
#define SMBCMD_TYPE_WRITE          (1U << 27)      /* Write */
#define SMBCMD_SA_SHIFT            24
#define SMBCMD_BA_SHIFT            16

/* SMBCNTL fields */
#define SMBCNTL_DTI_MASK           0xf0000000      /* Slave Address low bits */
#define SMBCNTL_DTI_SHIFT          28              /* Slave Address low bits */
#define SMBCNTL_DIS_WRT            (1U << 26)      /* Disable Write */
#define SMBCNTL_TSOD_PRES_MASK     0xff            /* DIMM Present mask */

/* For sanity check: bits that might randomly change if we race with firmware */
#define SMBCMD_OUR_BITS            (~(u32)SMBCMD_TRIGGER)
#define SMBCNTL_OUR_BITS           (SMBCNTL_DTI_MASK)


/* System Address Decoder, PCI dev 0xf fn 5, 8086.6ffd */
#define PCI_DEVICE_ID_INTEL_BROADWELL_IMC_SAD 0x6ffd

/* Register offsets */
#define SADCNTL                    0xf4

/* SADCNTL fields */
#define SADCNTL_LOCAL_NODEID_MASK  0xf             /* Local NodeID of socket */


/* Power Control Unit, PCI dev 0x1e fn 1, 8086.6f99 */
#define PCI_DEVICE_ID_INTEL_BROADWELL_IMC_PCU 0x6f99

/* Register offsets */
#define TSODCNTL                   0xe0

/* TSODCNTL fields */


/* DIMMs hold jc42 thermal sensors starting at i2c address 0x18 */
#define DIMM_SENSOR_DRV            "oldjc42"
#define DIMM_SENSOR_BASE_ADR       0x18


#define sanitycheck 1

struct imc_channelpair {
	struct i2c_adapter adapter;
	bool can_write, cltt;
};

struct imc_pcu {
	struct pci_dev *pci_dev;
	u32 tsod_polling_interval;
	struct mutex mutex; /* see imc_channelpair_claim() */
};

struct imc_priv {
	struct pci_dev *pci_dev;
	struct imc_channelpair channelpair[2];
	struct imc_pcu pcu;
	bool suspended;
};

static int imc_channelpair_claim(struct imc_priv *priv, int i)
{
	if (priv->suspended)
		return -EIO;

	/*
	 * i2c controllers need exclusive access to a psu register and wait
	 * then for 10ms before starting their transaction.
	 *
	 * Possible optimization: Once an i2c controller modified the psu
	 * register and waits, the other controller does not need to wait for
	 * the whole 10ms, but then only this other controller has to clean up
	 * the psu register.
	 */
	mutex_lock(&priv->pcu.mutex);

	if (priv->channelpair[i].cltt) {
		pci_write_config_dword(priv->pcu.pci_dev, TSODCNTL, 0);
		usleep_range(10000, 10500);
	}
	return 0;
}

static void imc_channelpair_release(struct imc_priv *priv, int i)
{
	if (priv->channelpair[i].cltt) {
		/* set tosd_control.tsod_polling_interval to previous value */
		pci_write_config_dword(priv->pcu.pci_dev, TSODCNTL,
				priv->pcu.tsod_polling_interval);
	}
	mutex_unlock(&priv->pcu.mutex);
}

static bool imc_wait_for_transaction(struct imc_priv *priv, int i, u32 *stat)
{
	int j;
	static int busywaits = 1;

	/*
	 * Distribution of transaction time from 10000 collected samples:
	 *
	 * 70us: 1, 80us: 12, 90us: 34, 100us: 132, 110us: 424, 120us: 1138,
	 * 130us: 5224, 140us: 3035.
	 *
	 */
	usleep_range(131, 140);

	/* Don't give up, yet */
	for (j = 0; j < 20; j++) {
		pci_read_config_dword(priv->pci_dev, SMBSTAT(i), stat);
		if (!(*stat & SMBSTAT_SMB_BUSY)) {
			if (j > busywaits) {
				busywaits = j;
				dev_warn(&priv->pci_dev->dev,
						"Discovered surprisingly long transaction time (%d)\n",
						busywaits);
			}
			return true;
		}
		udelay(9);
	}
	return false;
}

/*
 * The iMC supports five access types. The terminology is rather inconsistent.
 * These are the types:
 *
 * "Write to pointer register SMBus": I2C_SMBUS_WRITE, I2C_SMBUS_BYTE
 *
 * Read byte/word: I2C_SMBUS_READ, I2C_SMBUS_{BYTE|WORD}_DATA
 *
 * Write byte/word: I2C_SMBUS_WRITE, I2C_SMBUS_{BYTE|WORD}_DATA
 */

static u32 imc_func(struct i2c_adapter *adapter)
{
	int i;
	struct imc_channelpair *cp;
	struct imc_priv *priv = i2c_get_adapdata(adapter);

	i = (adapter == &priv->channelpair[0].adapter ? 0 : 1);
	cp = &priv->channelpair[i];

	if (cp->can_write)
		return I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA;
	else
		return I2C_FUNC_SMBUS_READ_BYTE_DATA |
			I2C_FUNC_SMBUS_READ_WORD_DATA;
}

static s32 imc_smbus_xfer(struct i2c_adapter *adap, u16 addr,
		unsigned short flags, char read_write, u8 command,
		int size, union i2c_smbus_data *data)
{
	int ret, i;
	u32 cmd = 0, cntl, stat;
#ifdef sanitycheck
	u32 final_cmd, final_cntl;
#endif
	struct imc_channelpair *cp;
	struct imc_priv *priv = i2c_get_adapdata(adap);

	i = (adap == &priv->channelpair[0].adapter ? 0 : 1);
	cp = &priv->channelpair[i];

	/* Encode CMD part of addresses and access size */
	cmd |= ((u32)addr & 0x7) << SMBCMD_SA_SHIFT;
	cmd |= ((u32)command) << SMBCMD_BA_SHIFT;
	if (size == I2C_SMBUS_WORD_DATA)
		cmd |= SMBCMD_WORD_ACCESS;

	/* Encode read/write and data to write */
	if (read_write == I2C_SMBUS_READ) {
		cmd |= SMBCMD_TYPE_READ;
	} else {
		cmd |= SMBCMD_TYPE_WRITE;
		cmd |= (size == I2C_SMBUS_WORD_DATA
				? swab16(data->word)
				: data->byte);
	}

	ret = imc_channelpair_claim(priv, i);
	if (ret)
		return ret;

	pci_read_config_dword(priv->pci_dev, SMBCNTL(i), &cntl);
	cntl &= ~SMBCNTL_DTI_MASK;
	cntl |= ((u32)addr >> 3) << SMBCNTL_DTI_SHIFT;
	pci_write_config_dword(priv->pci_dev, SMBCNTL(i), cntl);

	cmd |= SMBCMD_TRIGGER;
	pci_write_config_dword(priv->pci_dev, SMBCMD(i), cmd);

	if (!imc_wait_for_transaction(priv, i, &stat)) {
		dev_warn(&priv->pci_dev->dev, "smbus transaction did not complete.\n");
		ret = -ETIMEDOUT;
		goto xfer_out_release;
	}

#ifdef sanitycheck /* This is a young driver. Keep the checks for now */
	pci_read_config_dword(priv->pci_dev, SMBCMD(i), &final_cmd);
	pci_read_config_dword(priv->pci_dev, SMBCNTL(i), &final_cntl);
	if (((cmd ^ final_cmd) & SMBCMD_OUR_BITS) ||
			((cntl ^ final_cntl) & SMBCNTL_OUR_BITS)) {
		dev_err(&priv->pci_dev->dev,
				"Access to channel pair %d-%d raced with hardware: cmd 0x%08X->0x%08X, cntl 0x%08X->0x%08X\n",
				2*i, 2*i+1, cmd, final_cmd, cntl, final_cntl);
		ret = -EIO;
		goto xfer_out_release;
	}
#endif

	if (stat & SMBSTAT_SBE) {
		/*
		 * While SBE is set hardware TSOD polling is disabled. This is
		 * very bad as this bit is RO-V and will only be cleared after
		 * a further software initiated transaction finishes
		 * successfully.
		 */
		dev_err(&priv->pci_dev->dev,
				"smbus error: sbe is set 0x%x\n", stat);
		ret = -ENXIO;
		goto xfer_out_release;
	}

	if (read_write == I2C_SMBUS_READ) {
		if (!(stat & SMBSTAT_RDO)) {
			dev_warn(&priv->pci_dev->dev,
					"Unexpected read status 0x%08X\n", stat);
			ret = -EIO;
			goto xfer_out_release;
		}
		/*
		 * The iMC SMBus controller thinks of SMBus words as being
		 * big-endian (MSB first). Linux treats them as little-endian,
		 * so we need to swap them.
		 */
		if (size == I2C_SMBUS_WORD_DATA)
			data->word = swab16(stat & SMBSTAT_RDATA_MASK);
		else
			data->byte = stat & 0xFF;
	} else {
		if (!(stat & SMBSTAT_WOD)) {
			dev_warn(&priv->pci_dev->dev,
					"Unexpected write status 0x%08X\n", stat);
			ret = -EIO;
		}
	}

xfer_out_release:
	imc_channelpair_release(priv, i);

	return ret;
}

static const struct i2c_algorithm imc_smbus_algorithm = {
	.smbus_xfer	= imc_smbus_xfer,
	.functionality	= imc_func,
};

static void imc_instantiate_sensors(struct i2c_adapter *adapter, u8 presence)
{
	struct i2c_board_info info = {};

	strcpy(info.type, DIMM_SENSOR_DRV);
	info.addr = DIMM_SENSOR_BASE_ADR;

	/*
	 * Presence is a bit vector. Bits from right to left map into i2c slave
	 * addresses starting 0x18.
	 */
	while (presence) {
		if (presence & 0x1)
			i2c_new_client_device(adapter, &info);
		info.addr++;
		presence >>= 1;
	}
}

static int imc_init_channelpair(struct imc_priv *priv, int i, int socket)
{
	int err;
	u32 val;
	struct imc_channelpair *cp = &priv->channelpair[i];

	i2c_set_adapdata(&cp->adapter, priv);
	cp->adapter.owner = THIS_MODULE;
	cp->adapter.algo = &imc_smbus_algorithm;
	cp->adapter.dev.parent = &priv->pci_dev->dev;

	pci_read_config_dword(priv->pci_dev, SMBCNTL(i), &val);
	cp->can_write = !(val & SMBCNTL_DIS_WRT);

	/*
	 * A TSOD polling interval of > 0 tells us if CLTT mode is enabled on
	 * some channel pair.
	 *
	 * Is there a better way to check for CLTT mode? In particular, is
	 * there a way to distingush the mode on a channel pair basis?
	 */
	cp->cltt = (priv->pcu.tsod_polling_interval  > 0);

	snprintf(cp->adapter.name, sizeof(cp->adapter.name),
			"iMC socket %d for channel pair %d-%d", socket, 2*i, 2*i+1);
	err = i2c_add_adapter(&cp->adapter);
	if (err)
		return err;

	/* For reasons unknown, TSOD_PRES_MASK is only set in CLTT mode. */
	if (cp->cltt) {
		dev_info(&priv->pci_dev->dev,
				"CLTT is enabled on channel pair %d-%d. Thermal sensors will be automatically enabled\n",
				2*i, 2*i+1);
	} else {
		dev_info(&priv->pci_dev->dev,
				"CLTT is disabled on channel pair %d-%d. Thermal sensors need to be manually enabled\n",
				2*i, 2*i+1);
	}

	imc_instantiate_sensors(&cp->adapter, val & SMBCNTL_TSOD_PRES_MASK);

	return 0;
}

static void imc_free_channelpair(struct imc_priv *priv, int i)
{
	struct imc_channelpair *cp = &priv->channelpair[i];

	i2c_del_adapter(&cp->adapter);
}

static struct pci_dev *imc_get_related_device(struct pci_bus *bus,
		unsigned int devfn, u16 devid)
{
	struct pci_dev *dev = pci_get_slot(bus, devfn);

	if (!dev)
		return NULL;

	if (dev->vendor != PCI_VENDOR_ID_INTEL || dev->device != devid) {
		pci_dev_put(dev);
		return NULL;
	}
	return dev;
}

static int imc_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int i, j, err;
	struct imc_priv *priv;
	struct pci_dev *sad;  /* System Address Decoder */
	u32 sadcntl;

	/* Sanity check. This device is always at 0x13.0 */
	if (dev->devfn != PCI_DEVFN(0x13, 0))
		return -ENODEV;

	priv = devm_kzalloc(&dev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->pci_dev = dev;
	pci_set_drvdata(dev, priv);

	/*
	 * From sad, we learn the local node id of the socket.
	 *
	 * The socket will not change at runtime and so we throw away sad.
	 */
	sad = imc_get_related_device(dev->bus, PCI_DEVFN(0x0f, 5),
			PCI_DEVICE_ID_INTEL_BROADWELL_IMC_SAD);
	if (!sad) {
		err = -ENODEV;
		goto probe_out_free;
	}
	pci_read_config_dword(sad, SADCNTL, &sadcntl);
	pci_dev_put(sad);

	/*
	 * From pcu, we access the CLTT polling interval.
	 *
	 * The polling interval is set by BIOS. We assume it will not change at
	 * runtime and cache the initial value.
	 */
	priv->pcu.pci_dev = imc_get_related_device(dev->bus, PCI_DEVFN(0x1e, 1),
			PCI_DEVICE_ID_INTEL_BROADWELL_IMC_PCU);
	if (!priv->pcu.pci_dev) {
		err = -ENODEV;
		goto probe_out_free;
	}
	pci_read_config_dword(priv->pcu.pci_dev, TSODCNTL,
			&priv->pcu.tsod_polling_interval);

	mutex_init(&priv->pcu.mutex);

	for (i = 0; i < 2; i++) {
		err = imc_init_channelpair(priv, i,
				sadcntl & SADCNTL_LOCAL_NODEID_MASK);
		if (err)
			goto probe_out_free_channelpair;
	}

	return 0;

probe_out_free_channelpair:
	for (j = 0; j < i; j++)
		imc_free_channelpair(priv, j);

	mutex_destroy(&priv->pcu.mutex);

probe_out_free:
	kfree(priv);
	return err;
}

static void imc_remove(struct pci_dev *dev)
{
	int i;
	struct imc_priv *priv = pci_get_drvdata(dev);

	for (i = 0; i < 2; i++)
		imc_free_channelpair(priv, i);

	/* set tosd_control.tsod_polling_interval to initial value */
	pci_write_config_dword(priv->pcu.pci_dev, TSODCNTL,
			priv->pcu.tsod_polling_interval);

	mutex_destroy(&priv->pcu.mutex);
}

static int imc_suspend(struct pci_dev *dev, pm_message_t mesg)
{
	struct imc_priv *priv = pci_get_drvdata(dev);

	/* BIOS is in charge. We should finish any pending transaction */
	priv->suspended = true;

	return 0;
}

static int imc_resume(struct pci_dev *dev)
{
	struct imc_priv *priv = pci_get_drvdata(dev);

	priv->suspended = false;

	return 0;
}

static const struct pci_device_id imc_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL,
			PCI_DEVICE_ID_INTEL_BROADWELL_IMC_TA) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, imc_ids);

static struct pci_driver imc_pci_driver = {
	.name		= "imc_smbus",
	.id_table	= imc_ids,
	.probe		= imc_probe,
	.remove		= imc_remove,
	.suspend	= imc_suspend,
	.resume		= imc_resume,
};

static int __init i2c_imc_init(void)
{
	return pci_register_driver(&imc_pci_driver);
}
module_init(i2c_imc_init);

static void __exit i2c_imc_exit(void)
{
	pci_unregister_driver(&imc_pci_driver);
}
module_exit(i2c_imc_exit);

MODULE_AUTHOR("Stefan Schaeckeler <sschaeck@cisco.com>");
MODULE_DESCRIPTION("iMC SMBus driver");
MODULE_LICENSE("GPL v2");
