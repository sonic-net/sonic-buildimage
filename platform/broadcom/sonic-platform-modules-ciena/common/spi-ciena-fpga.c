/*
 * SPI master controller driver implemented in a Ciena FPGA
 */
#include <linux/device.h>
#include <linux/module.h>       /* module_init/exit, MODULE* */
#include <linux/platform_device.h> /* platform_* */
#include <linux/io.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include "spi-ciena-fpga.h"
#include <linux/types.h>
#include <linux/delay.h>
#ifdef CONFIG_CIENA_MCEE
#include <linux/ciena_mcee.h>
#endif

#define DRIVER_NAME "spi-ciena-fpga"

/* Offsets of registers in FPGA SPI block */
#define CIENA_FPGA_SPI_DATA     0x0
#define CIENA_FPGA_SPI_CTRL     0x4
#define CIENA_FPGA_SPI_TX_LEVEL 0x6
#define CIENA_FPGA_SPI_RX_LEVEL 0x8
#define CIENA_FPGA_SPI_ERROR    0xa

#define SPI_WRITE_TIMEOUT  10000 /* 10 ms */

#define SPI_CTRL_FLUSH_FIFO           0x8000
#define SPI_CTRL_WR_FIFO_OVERFLOW     0x4000
#define SPI_CTRL_RD_FIFO_UNDERFLOW    0x2000
#define SPI_CTRL_WRITE_FIFO_EMPTY     0x1000
#define SPI_CTRL_READ_FIFO_EMPTY      0x0800
#define SPI_CTRL_CS_MASK              0x000f

#define SPI_CTRL_ERR_MASK             (SPI_CTRL_WR_FIFO_OVERFLOW | SPI_CTRL_RD_FIFO_UNDERFLOW)
#define SPI_CTRL_ERR_SHIFT            13

#define SPI_ERROR_RS_UNCORRECTABLE    0x0008
#define SPI_ERROR_RS_CORRECTABLE      0x0004
#define SPI_ERROR_WR_FIFO_OVERFLOW    0x0002
#define SPI_ERROR_RD_FIFO_UNDERFLOW   0x0001
#define SPI_FLASH_READ_SUCCESS        0x0040
#define SPI_FLASH_WRITE_SUCCESS       0x0020

#define SPI_ERROR_MASK                (SPI_ERROR_RS_UNCORRECTABLE | SPI_ERROR_RS_CORRECTABLE | SPI_ERROR_WR_FIFO_OVERFLOW | SPI_ERROR_RD_FIFO_UNDERFLOW)

// the following definition for spidev is specific to
// narsil and durin fpga on thorin
#define CIENA_FPGA_SPI_DEV_BASE    0x100
#define FPGA_SPI_READ_OPERATION    0x5A
#define FPGA_SPI_WRITE_OPERATION   0x5B
#define FPGA_SPI_UPPER_8_BIT_SHIFT    8
#define FPGA_SPI_LOWER_8_BIT_MASK  0xFF

struct ciena_fpga_spi_info {
	u16 num_chip_select;
	int ctrlreg_err;
};

#define INFO(_num_chip_select, _ctrlreg_err)	\
	((kernel_ulong_t)&(struct ciena_fpga_spi_info) {	\
		.num_chip_select = (_num_chip_select),	\
		.ctrlreg_err     = (_ctrlreg_err), \
	})

struct spi_ciena_fpga_data {
	struct spi_device  *spi;
	void __iomem       *regs;
	int                 reg_width;
	int                 fifo_depth;
	int                 xfer_width;
	uint32_t            wdata_init;
	uint32_t            xfer_delay;
	uint32_t           (*read)(struct spi_ciena_fpga_data *data, void *addr);
	void               (*write)(struct spi_ciena_fpga_data *data, uint32_t val, void *addr);
	uint32_t           (*read_errors)(struct spi_ciena_fpga_data *data);
	void               (*clear_errors)(struct spi_ciena_fpga_data *data, uint32_t errors);
#ifdef CONFIG_CIENA_MCEE
	struct ciena_mcee_dev *mceed;
#endif
};

#define SPI_CTRL(p)  (p->regs + (1 + (32 == p->reg_width)) * CIENA_FPGA_SPI_CTRL)
#define SPI_DATA(p)  (p->regs + (1 + (32 == p->reg_width)) * CIENA_FPGA_SPI_DATA)
#define SPI_RFILL(p) (p->regs + (1 + (32 == p->reg_width)) * CIENA_FPGA_SPI_RX_LEVEL)
#define SPI_ERROR(p) (p->regs + (1 + (32 == p->reg_width)) * CIENA_FPGA_SPI_ERROR)

static uint32_t spi_ciena_fpga_read16be(struct spi_ciena_fpga_data *data, void *addr) { return ioread16be(addr); }
static uint32_t spi_ciena_fpga_read16le(struct spi_ciena_fpga_data *data, void *addr) { return ioread16(addr); }
static uint32_t spi_ciena_fpga_read32be(struct spi_ciena_fpga_data *data, void *addr) { return ioread32be(addr); }
static uint32_t spi_ciena_fpga_read32le(struct spi_ciena_fpga_data *data, void *addr) { return ioread32(addr); }

static void spi_ciena_fpga_write16be(struct spi_ciena_fpga_data *data, uint32_t val, void *addr) { iowrite16be(val, addr); }
static void spi_ciena_fpga_write16le(struct spi_ciena_fpga_data *data, uint32_t val, void *addr) { iowrite16(val, addr); }
static void spi_ciena_fpga_write32be(struct spi_ciena_fpga_data *data, uint32_t val, void *addr) { iowrite32be(val, addr); }
static void spi_ciena_fpga_write32le(struct spi_ciena_fpga_data *data, uint32_t val, void *addr) { iowrite32(val, addr); }

/*----------------------------------------------------------------------------*/
static uint32_t spi_ciena_fpga_read_errors_ctrlreg(struct spi_ciena_fpga_data *data)
{
	uint32_t ctrl;

	ctrl = data->read(data, SPI_CTRL(data));
	ctrl &= SPI_CTRL_ERR_MASK;
	ctrl >>= SPI_CTRL_ERR_SHIFT;
	return ctrl;
}

/*----------------------------------------------------------------------------*/
static uint32_t spi_ciena_fpga_read_errors(struct spi_ciena_fpga_data *data)
{
	return data->read(data, SPI_ERROR(data));
}

/*----------------------------------------------------------------------------*/
static void spi_ciena_fpga_clear_errors_ctrlreg(struct spi_ciena_fpga_data *data, uint32_t errors)
{
	errors <<= SPI_CTRL_ERR_SHIFT;
	errors &= SPI_CTRL_ERR_MASK;
	errors |= (data->read(data, SPI_CTRL(data)) & ~(SPI_CTRL_ERR_MASK));

	data->write(data, errors, SPI_CTRL(data));
}

/*----------------------------------------------------------------------------*/
static void spi_ciena_fpga_clear_errors(struct spi_ciena_fpga_data *data, uint32_t errors)
{
	errors |= SPI_ERROR_MASK;
	errors |= (data->read(data, SPI_ERROR(data)) & ~(SPI_ERROR_MASK));

	data->write(data, errors, SPI_ERROR(data));
}

/*----------------------------------------------------------------------------*/
static void spi_ciena_fpga_chipselect(struct spi_ciena_fpga_data *priv,
				      struct spi_device          *spi,
				      int                         is_on)
{
	struct gpio_desc *gd      = spi_get_csgpiod(spi, 0);
	uint16_t          ctrl;
	uint16_t          csmask  = (1 << spi_get_chipselect(spi, 0));
	uint16_t          was_on;

	ctrl    = priv->read(priv, SPI_CTRL(priv));
	was_on  = ctrl & SPI_CTRL_CS_MASK;
	ctrl   &= ~SPI_CTRL_CS_MASK;

	dev_dbg(&spi->dev, "chip select %u %s%s\n",
		spi_get_chipselect(spi, 0),
		is_on ? "on" : "off",
		was_on ? " (already on)": "");

	if (is_on) {
		ctrl |= (csmask & SPI_CTRL_CS_MASK);
		if (was_on)
			ctrl |= SPI_CTRL_FLUSH_FIFO;
	}

	priv->write(priv, ctrl, SPI_CTRL(priv));

	if (gd)
		gpiod_set_value_cansleep(gd, is_on);

}

/*----------------------------------------------------------------------------*/
static int spi_ciena_fpga_prepare_xfer_hw(struct spi_controller *ctlr)
{
	struct spi_ciena_fpga_data *priv = spi_controller_get_devdata(ctlr);
	uint16_t ctrl, errors;
	bool reset_fifo = false;

	dev_dbg(&ctlr->dev, "%s\n", __func__);

	ctrl = priv->read(priv, SPI_CTRL(priv));

	if (!(ctrl & SPI_CTRL_WRITE_FIFO_EMPTY)) {
		dev_err(&ctlr->dev,
			"Write FIFO not empty [SPI_CTRL=%04x]\n",
			ctrl);
		reset_fifo = true;
	}

	if (!(ctrl & SPI_CTRL_READ_FIFO_EMPTY)) {
		dev_err(&ctlr->dev,
			"Read FIFO not empty [SPI_CTRL=%04x]\n",
			ctrl);
		reset_fifo = true;
	}

	/* Clear all chip selects. */
	ctrl &= ~SPI_CTRL_CS_MASK;

	/* Reset FIFOs but preserve chip select bits. */
	if (reset_fifo) {
		ctrl |= SPI_CTRL_FLUSH_FIFO;
		priv->write(priv, ctrl, SPI_CTRL(priv));
	}

	errors = priv->read_errors(priv);
	if (errors) {
		dev_dbg(&ctlr->dev, "errors (0x%04x) detected\n", errors);
		/* clear all errors */
		priv->clear_errors(priv, errors);
	}

#ifdef CONFIG_CIENA_MCEE
	if (ciena_mcee_dev_is_stale(priv->mceed))
		return -ENXIO;
#endif
	return 0;
}

/*----------------------------------------------------------------------------*/
static int spi_ciena_fpga_unprepare_xfer_hw(struct spi_controller *ctlr)
{
	dev_dbg(&ctlr->dev, "%s\n", __func__);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int spi_ciena_fpga_setup(struct spi_device *spi)
{
	if (spi->bits_per_word != 8) {
		dev_err(&spi->dev, "%d bits per word is not supported\n",
			spi->bits_per_word);
		return -EINVAL;
	}

	if (spi->chip_select[0] >= spi->controller->num_chipselect) {
		dev_err(&spi->dev, "%d chip select is out of range\n",
			spi->chip_select[0]);
		return -EINVAL;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static void spi_ciena_fpga_check_fifos(struct spi_device *spi,
				       const char *when)
{
	struct spi_ciena_fpga_data *priv = spi_controller_get_devdata(spi->controller);
	u32 errors;
	u32 rdata;
	u32 filled;

	rdata = priv->read(priv, SPI_CTRL(priv));
	if ((0 == (rdata & SPI_CTRL_READ_FIFO_EMPTY)) ||
	    (0 == (rdata & SPI_CTRL_WRITE_FIFO_EMPTY))) {

		filled = priv->read(priv, SPI_RFILL(priv));
		errors = priv->read_errors(priv);
		if (errors) priv->clear_errors(priv, errors);

		dev_err(&spi->dev, "FIFOs undrained %s "
			"[SPI_CTRL=%04x err=%04x fill=%u]\n",
			when, rdata, errors, filled);

		/* drain the fifos; maybe the next guy will not complain */
		rdata |= SPI_CTRL_FLUSH_FIFO;
		priv->write(priv, rdata, SPI_CTRL(priv));
	}
}

/*----------------------------------------------------------------------------*/
static u32 spi_ciena_fpga_txrx_bufs_fifo(struct spi_device *spi,
					 struct spi_transfer *t)
{
	struct spi_ciena_fpga_data *priv = spi_controller_get_devdata(spi->controller);
	unsigned remaining = t->len;
	unsigned written;
	unsigned queued;
	const void *tpos = t->tx_buf;
	void *rpos = t->rx_buf;
	u32 wdata = priv->wdata_init;
	u32 rdata;
	u32 errors;
	int filled;
	int xmove = priv->xfer_width;
	int timeout;

	spi_ciena_fpga_check_fifos(spi, "at start");

	while (remaining) {

		written = 0;
		while ((written < remaining) &&
		       (written < priv->fifo_depth)) {
			if (tpos) {
				memcpy(&wdata, tpos, xmove);
				tpos += xmove;
			}
			priv->write(priv, wdata, SPI_DATA(priv));
			written += xmove;
			dev_dbg(&spi->dev, "wrote %02x (%u/%u)\n",
				wdata, written, priv->fifo_depth);
		}

		queued = written;
		timeout = SPI_WRITE_TIMEOUT;
		while (queued) {

			rdata = priv->read(priv, SPI_CTRL(priv));
#ifdef CONFIG_CIENA_MCEE
			if (ciena_mcee_dev_is_stale(priv->mceed))
				return -ENXIO;
#endif
			filled = !(rdata & SPI_CTRL_READ_FIFO_EMPTY);
			if (!filled) {
				if (timeout-- > 0) {
					udelay(1);
					continue;
				}
				errors = priv->read_errors(priv);
				if (errors) priv->clear_errors(priv, errors);
				dev_err(&spi->dev, "Read FIFO underfilled "
					"[SPI_CTRL=%04x err=%04x q=%u/%u]\n",
					rdata, errors, queued, written);
				break;
			}

			while (filled && queued) {
				rdata = priv->read(priv, SPI_DATA(priv));
				if (rpos) {
					memcpy(rpos, &rdata, xmove);
					rpos += xmove;
				}
				dev_dbg(&spi->dev, "read %02x (%u/%u)\n",
					rdata, queued, written);
				queued -= xmove;
				rdata = priv->read(priv, SPI_CTRL(priv));
				filled = !(rdata & SPI_CTRL_READ_FIFO_EMPTY);
			}

			if (filled)
				dev_warn(&spi->dev, "Read FIFO overfilled "
					 "[fill=%u]\n",
					 priv->read(priv, SPI_RFILL(priv)));

		}

		spi_ciena_fpga_check_fifos(spi, "after transfer");
		remaining -= written;
	}

#ifdef CONFIG_CIENA_MCEE
	if (ciena_mcee_dev_is_stale(priv->mceed))
		return -ENXIO;
#endif

	return t->len;
}

/*----------------------------------------------------------------------------*/
static u32 spi_ciena_fpga_txrx_bufs(struct spi_device *spi, struct spi_transfer *t)
{
	dev_dbg(&spi->dev, "transfer: have %stx, have %srx, len=%u\n",
		t->tx_buf ? "" : "no ", t->rx_buf ? "" : "no ", t->len);

	return spi_ciena_fpga_txrx_bufs_fifo(spi, t);
}

/*----------------------------------------------------------------------------*/
static int _spi_ciena_fpga_transfer_msg(struct spi_controller *ctlr,
					struct spi_message *mesg)
{
	struct spi_device *spi = mesg->spi;
	struct spi_transfer *t = NULL;
	struct spi_ciena_fpga_data *priv = spi_controller_get_devdata(ctlr);
	unsigned cs_change = 1;
	int status = 0;

	mesg->actual_length = 0;

	list_for_each_entry (t, &mesg->transfers, transfer_list) {

		if (t->bits_per_word && (8 != t->bits_per_word)) {
			dev_err(&spi->dev, "%d bits per word is not "
				"supported\n", t->bits_per_word);
			status = -EINVAL;
			break;
		}

		if (cs_change) spi_ciena_fpga_chipselect(priv, spi, 1);
		cs_change = t->cs_change;

		status = spi_ciena_fpga_txrx_bufs(spi, t);

		if (cs_change) spi_ciena_fpga_chipselect(priv, spi, 0);

		if (status > 0) mesg->actual_length += status;
		if (status != t->len) {
			/* always report some kind of error */
			if (status >= 0) {
				dev_err(&spi->dev, "%d != %d\n", status, t->len);
				status = -EREMOTEIO;
			}
			break;
		}
		status = 0;
		if (t->transfer_list.next == &mesg->transfers) break;

		if (0 == t->delay.value) {
			t->delay.value = priv->xfer_delay;
			t->delay.unit = SPI_DELAY_UNIT_USECS;
		}
		spi_transfer_delay_exec(t);
	}

	spi_ciena_fpga_chipselect(priv, spi, 0);

	mesg->status = status;

	return status;
}

/*----------------------------------------------------------------------------*/
static int spi_ciena_fpga_transfer_one(struct spi_controller *ctlr,
				       struct spi_message *mesg)
{
	int status;

	status = _spi_ciena_fpga_transfer_msg(ctlr, mesg);

	spi_finalize_current_message(ctlr);

	return status;
}

/*----------------------------------------------------------------------------*/
#ifdef CONFIG_CIENA_SPI_PANIC_SYNC
static int spi_ciena_fpga_panic_sync(struct spi_controller *ctlr,
				     struct spi_message *mesg)
{
	int status;

	status = spi_ciena_fpga_prepare_xfer_hw(ctlr);
	if (!status)
		status = _spi_ciena_fpga_transfer_msg(ctlr, mesg);

	return status;
}
#endif

/*----------------------------------------------------------------------------*/
static int spi_ciena_fpga_parse_of_nodes(struct device *dev,
					 struct device_node *node,
					 struct spi_ciena_fpga_platform_data *pdata)
{
	const u32 *prop;
	int       len;

	prop = of_get_property(node, "little-endian", &len);
	pdata->big_endian = (prop == NULL);

	prop = of_get_property(node, "fpga-reg-width", &len);
	if (prop) {
		if (len != sizeof(*prop)) {
			dev_err(dev,
				"invalid fpga-reg-width for '%s'\n",
				node->full_name);
			return -EINVAL;
		}
		pdata->width = be32_to_cpup(prop);
	}

	prop = of_get_property(node, "bus-num", &len);
	if (prop) {
		if (len != sizeof(*prop)) {
			dev_err(dev, "invalid bus-num for '%s'\n",
				node->full_name);
			return -EINVAL;
		}
		pdata->bus_num = be32_to_cpup(prop);
	}

	prop = of_get_property(node, "num-chip-selects", &len);
	if (prop) {
		if (len != sizeof(*prop)) {
			dev_err(dev,
				"invalid num-chip-selects for '%s'\n",
				node->full_name);
			return -EINVAL;
		}
		pdata->num_chipselect = be32_to_cpup(prop);
	}

	prop = of_get_property(node, "pos-pointer-size", &len);
	if (prop) {
		if (len != sizeof(*prop)) {
			dev_err(dev,
				"invalid pos-pointer-size for '%s'\n",
				node->full_name);
			return -EINVAL;
		}
		pdata->ppos_size = be32_to_cpup(prop);
	}

        prop = of_get_property(node, "fifo-depth", &len);
        if (prop) {
                if (len != sizeof(*prop)) {
                        dev_err(dev,
                                "invalid fifo-depth for '%s'\n",
                                node->full_name);
                        return -EINVAL;
                }
                pdata->fifo_depth = be32_to_cpup(prop);
        }


	prop = of_get_property(node, "fpga-ctrlreg-err", &len);
	pdata->ctrlreg_err = (prop != NULL);

	prop = of_get_property(node, "transfer-delay-usecs", &len);
	if (prop) pdata->xfer_delay = be32_to_cpup(prop);

	dev_dbg(dev, "of data: big_endian=%d width=%d bus_num=%d "
		"num_chipselect=%d ppos_size=%d ctrlreg_err=%d "
		"xfer_delay=%u\n", pdata->big_endian, pdata->width,
		pdata->bus_num, pdata->num_chipselect, pdata->ppos_size,
		pdata->ctrlreg_err, pdata->xfer_delay);

	return 0;
}

/*----------------------------------------------------------------------------*/
static int spi_ciena_fpga_probe(struct platform_device *pdev)
{
	struct device				*dev  = &pdev->dev;
	struct spi_controller			*ctlr;
	struct spi_ciena_fpga_platform_data	 platform_data = {};
	struct spi_ciena_fpga_platform_data	*pdata = &platform_data;
	struct spi_ciena_fpga_data		*priv;
	struct device_node			*node = dev->of_node;
	struct resource				*mem;
	int status;

	ctlr = spi_alloc_host(dev, sizeof(*priv));
	if (ctlr == NULL) {
		dev_err(dev, "spi_alloc_host failed\n");
		return -ENOMEM;
	}

	priv = spi_controller_get_devdata(ctlr);

	if (node) {
		status = spi_ciena_fpga_parse_of_nodes(dev, node, pdata);
		if (status < 0)
			goto fail;
	} else {
		pdata = pdev->dev.platform_data;
		if (pdata == NULL) {
			dev_err(dev, "platform data is missing\n");
			status = -ENODATA;
			goto fail;
		}
	}

	if (pdata->ppos_size) {
		priv->xfer_width = pdata->ppos_size/8;
		priv->wdata_init = 0x10000000;
	} else {
		priv->xfer_width = 1;
		priv->wdata_init = 0;
	}

	switch (pdata->width) {
	case 16:
		if (pdata->big_endian) {
			priv->read = spi_ciena_fpga_read16be;
			priv->write = spi_ciena_fpga_write16be;
		} else {
			priv->read = spi_ciena_fpga_read16le;
			priv->write = spi_ciena_fpga_write16le;
		}
		break;
	case 32:
		if (pdata->big_endian) {
			priv->read = spi_ciena_fpga_read32be;
			priv->write = spi_ciena_fpga_write32be;
		} else {
			priv->read = spi_ciena_fpga_read32le;
			priv->write = spi_ciena_fpga_write32le;
		}
		break;
	default:
		dev_err(dev, "unsupported bus width %d\n", pdata->width);
		status = -ENODATA;
		goto fail;
	}
	priv->reg_width = pdata->width;
	priv->xfer_delay = pdata->xfer_delay;

	if (pdata->ctrlreg_err) {
		priv->read_errors = spi_ciena_fpga_read_errors_ctrlreg;
		priv->clear_errors = spi_ciena_fpga_clear_errors_ctrlreg;
	} else {
		priv->read_errors = spi_ciena_fpga_read_errors;
		priv->clear_errors = spi_ciena_fpga_clear_errors;
	}

	priv->fifo_depth = pdata->fifo_depth;
	if (priv->fifo_depth < 1) priv->fifo_depth = priv->xfer_width;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(dev, "missing memory resource\n");
		status = -ENXIO;
		goto fail;
	}

	priv->regs = ioremap(mem->start, resource_size(mem));
	if (!priv->regs) {
		dev_err(dev, "failed to ioremap registers\n");
		status = -EFAULT;
		goto fail;
	}

	ctlr->dev.of_node                 = node;
	ctlr->num_chipselect              = pdata->num_chipselect;
	ctlr->bus_num                     = pdata->bus_num;
	ctlr->setup                       = spi_ciena_fpga_setup;
	ctlr->prepare_transfer_hardware   = spi_ciena_fpga_prepare_xfer_hw;
	ctlr->transfer_one_message        = spi_ciena_fpga_transfer_one;
	ctlr->unprepare_transfer_hardware = spi_ciena_fpga_unprepare_xfer_hw;
	ctlr->use_gpio_descriptors        = pdata->gpio_chipsel;
#ifdef CONFIG_CIENA_SPI_PANIC_SYNC
	ctlr->panic_sync                  = spi_ciena_fpga_panic_sync;
#endif
#ifdef CONFIG_CIENA_MCEE
	priv->mceed = devm_kzalloc(dev, sizeof(*priv->mceed), GFP_KERNEL);
	if (!priv->mceed) {
		dev_err(dev, "failed to allocate mceed\n");
		status = -ENOMEM;
		goto fail;
	}
	ciena_mcee_dev_init(pdev, priv->regs, resource_size(mem), priv->mceed);
#endif

	platform_set_drvdata(pdev, ctlr);

	status = spi_register_controller(ctlr);
	if (status) {
		dev_err(dev, "spi_register_controller failed\n");
		goto fail;
	}

	dev_info(dev, "num_chipselect: %u, bus_num: %u, %p -> %pR\n",
		 pdata->num_chipselect, pdata->bus_num, priv->regs, mem);

	return 0;

fail:
#ifdef CONFIG_CIENA_MCEE
	if (priv->mceed)
		ciena_mcee_dev_deinit(priv->mceed);
#endif
	if (priv->regs) {
		iounmap(priv->regs);
	}

	spi_controller_put(ctlr);

	return status;
}

/*----------------------------------------------------------------------------*/
static void spi_ciena_fpga_remove (struct platform_device * pdev)
{
	struct spi_controller		*ctlr = platform_get_drvdata(pdev);
	struct spi_ciena_fpga_data	*priv = spi_controller_get_devdata(ctlr);
	void __iomem                    *regs = priv->regs;
#ifdef CONFIG_CIENA_MCEE
	struct ciena_mcee_dev           *mceed = priv->mceed;
#endif

	platform_set_drvdata(pdev, NULL);
	spi_unregister_controller(ctlr);

#ifdef CONFIG_CIENA_MCEE
	/* The spi stack involves multiple threads that can muck with
	 * the controller registers until the last possible moment.
	 * Hold on to the machine check eater until the spi master is
	 * unregistered. */
	ciena_mcee_dev_deinit(mceed);
#endif
	iounmap(regs);
}

/*----------------------------------------------------------------------------*/
static const struct of_device_id spi_ciena_fpga_match_ids[] = {
	{.compatible = "ciena,spi-fpga"},
	{}
};
MODULE_DEVICE_TABLE(of, spi_ciena_fpga_match_ids);

static const struct spi_device_id ciena_fpga_spi_ids[] = {
	{ "ciena_fpga_spidev-1",  INFO(1, 0)},
	{  },
};

MODULE_DEVICE_TABLE(spi, ciena_fpga_spi_ids);

static uint32_t spidev_ciena_fpga_read(struct spi_ciena_fpga_data *data, void* addr)
{
	struct spi_message	message;
	uint8_t tx[3] = {FPGA_SPI_READ_OPERATION,
			 (uintptr_t)addr >> FPGA_SPI_UPPER_8_BIT_SHIFT,
			 (uintptr_t)addr & FPGA_SPI_LOWER_8_BIT_MASK, };

	uint8_t rx[3];

	struct spi_transfer	x[2] = {
		{.len = sizeof(tx), .tx_buf = tx,},
		{.len = sizeof(rx), .rx_buf = rx,},
	};

	if (data == NULL) {
		dev_err(NULL, "cannot find ciena spi fpga data \n");
		return 0;
	}

	if (data->spi == NULL) {
		dev_err(NULL, "cannot find ciena spi fpga device \n");
		return 0;
	}

	spi_message_init(&message);
	spi_message_add_tail(&x[0], &message);
	spi_message_add_tail(&x[1], &message);

	if ( spi_sync(data->spi, &message) == 0 ) {
		if ((rx[2] != SPI_FLASH_READ_SUCCESS)  &&
            (rx[2] != 0)) {
			dev_err(&data->spi->dev, "error %x found while reading from register %lx\n", 
					rx[2], (uintptr_t)addr);
		}
		return (rx[0]  << FPGA_SPI_UPPER_8_BIT_SHIFT) | rx[1];
	}
	else {
		dev_err(&data->spi->dev, "spi_sync failed in reading\n");
	}
	return 0;
}

static void spidev_ciena_fpga_write(struct spi_ciena_fpga_data *data, uint32_t val, void *addr )
{
	uint8_t tx[] = { FPGA_SPI_WRITE_OPERATION,
					 (uintptr_t)addr >> FPGA_SPI_UPPER_8_BIT_SHIFT,
					 (uintptr_t)addr & FPGA_SPI_LOWER_8_BIT_MASK,
					 val >> FPGA_SPI_UPPER_8_BIT_SHIFT,
					 val & FPGA_SPI_LOWER_8_BIT_MASK };
	uint8_t rx[1];

	struct spi_message	message;
	struct spi_transfer	x[2] = {
		{.len = sizeof(tx), .tx_buf = tx,},
		{.len = sizeof(rx), .rx_buf = rx,},
	};

	if (data == NULL) {
		dev_err(NULL, "cannot find ciena spi fpga data \n");
		return;
	}

	if (data->spi == NULL) {
		dev_err(NULL, "cannot find ciena spi fpga device \n");
		return;
	}

	spi_message_init(&message);
	spi_message_add_tail(&x[0], &message);
	spi_message_add_tail(&x[1], &message);

	if ( spi_sync(data->spi, &message) != 0 ) {
		dev_err(&data->spi->dev, "spi_sync failed in writing\n");
	}

	if ((rx[0] != SPI_FLASH_WRITE_SUCCESS) &&
	    (rx[0] != 0)) {
		dev_err(&data->spi->dev, "error %x found while writing to register %lx\n",
				rx[0], (uintptr_t)addr);
	}
}

static int spidev_ciena_fpga_probe(struct spi_device *spi)
{
	const struct spi_device_id       *id = spi_get_device_id(spi);
	struct device                    *dev = &spi->dev;
	struct spi_controller            *ctlr;
	struct spi_ciena_fpga_data       *priv;
	const struct ciena_fpga_spi_info *info = (void*)id->driver_data;
	int status;

	ctlr = spi_alloc_host(dev, sizeof(struct spi_ciena_fpga_data));
	if (ctlr == NULL) {
		dev_err(dev, "spi_alloc_host failed\n");
		return -ENOMEM;
	}

	if (id == NULL) {
		dev_err(dev, "id table is NULL\n");
		return -ENODEV;
	}

	ctlr->num_chipselect              = info->num_chip_select;
	ctlr->dev.of_node                 = dev->of_node;
	ctlr->bus_num                     = -1;
	ctlr->setup                       = spi_ciena_fpga_setup;
	ctlr->prepare_transfer_hardware   = spi_ciena_fpga_prepare_xfer_hw;
	ctlr->transfer_one_message        = spi_ciena_fpga_transfer_one;
	ctlr->unprepare_transfer_hardware = spi_ciena_fpga_unprepare_xfer_hw;

	priv = spi_controller_get_devdata(ctlr);
	priv->read  = spidev_ciena_fpga_read;
	priv->write = spidev_ciena_fpga_write;
	priv->spi   = spi;
	priv->regs  = (void *__iomem)CIENA_FPGA_SPI_DEV_BASE;

	/* always use the byte-wise transfers for now */
	priv->fifo_depth = 1;

	if (info->ctrlreg_err) {
		priv->read_errors = spi_ciena_fpga_read_errors_ctrlreg;
		priv->clear_errors = spi_ciena_fpga_clear_errors_ctrlreg;
	} else {
		priv->read_errors = spi_ciena_fpga_read_errors;
		priv->clear_errors = spi_ciena_fpga_clear_errors;
	}

	spi_set_drvdata(spi, ctlr);

	status = spi_register_controller(ctlr);
	if (status) {
		dev_err(dev, "spi_register_controller failed\n");
		spi_controller_put(ctlr);
		return status;
	}

	return 0;
}

static void spidev_ciena_fpga_remove (struct spi_device * spi)
{
	struct spi_controller		*ctlr = spi_get_drvdata(spi);

	spi_unregister_controller(ctlr);
}

/*----------------------------------------------------------------------------*/
static struct platform_driver spi_ciena_fpga_driver = {
	.driver	= {
		.name		= DRIVER_NAME,
		.of_match_table	= spi_ciena_fpga_match_ids,
	},
	.probe  = spi_ciena_fpga_probe,
	.remove = spi_ciena_fpga_remove,
};

static struct spi_driver spidev_ciena_spi_driver = {
	.driver = {
		.name  =	"spidev_ciena_fpga",
		.owner =	THIS_MODULE,
	},
	.id_table = ciena_fpga_spi_ids,
	.probe    =	spidev_ciena_fpga_probe,
	.remove   =	spidev_ciena_fpga_remove,
};

static int __init spi_ciena_fpga_init(void)
{
	int ret;

	ret = platform_driver_register(&spi_ciena_fpga_driver);
	if (ret)
		return ret;

	ret = spi_register_driver(&spidev_ciena_spi_driver);
	if (ret) {
		platform_driver_unregister(&spi_ciena_fpga_driver);
		return ret;
	}

	return 0;
}
module_init(spi_ciena_fpga_init);

static void __exit spi_ciena_fpga_exit(void)
{
	spi_unregister_driver(&spidev_ciena_spi_driver);
	platform_driver_unregister(&spi_ciena_fpga_driver);
}
module_exit(spi_ciena_fpga_exit);

MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("Brian Daniels <bdaniels@ciena.com>");
MODULE_DESCRIPTION("SPI master controller driver implemented in a Ciena FPGA");
MODULE_LICENSE("GPL");
