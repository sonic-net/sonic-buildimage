/*
 * Centec QSPI controller driver
 *
 * Author: wangyb <wangyb@centecnetworks.com>
 *
 * Copyright 2005-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/omap-dma.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/printk.h>

#define SPI_MODE            0x00

#define PIO_MODE_1          0x01
#define PIO_MODE_2          0x02

#define PIO_BASE(NR)        (0x20*(NR-1))
#define PIO_GO(NR)          (0x10 + PIO_BASE(NR))
#define PIO_CTRL(NR)        (0x14 + PIO_BASE(NR))
#define PIO_STEP0_CONF(NR)  (0x20 + PIO_BASE(NR))
#define PIO_STEP1_CONF(NR)  (0x24 + PIO_BASE(NR))
#define PIO_STEP2_CONF(NR)  (0x28 + PIO_BASE(NR))
#define PIO_STEP3_CONF(NR)  (0x2c + PIO_BASE(NR))

#define PP_GO               0x80
#define PP_CTRL             0x84
#define PP_CMD_CODE         0x90
#define PP_CMD_CONF         0x94
#define PP_ADDR_CODE        0x98
#define PP_ADDR_CONF        0X9c
#define PP_DUMMY_CODE       0xa0
#define PP_DUMMY_CONF       0xa4
#define PP_DATA_CONF        0xa8

#define CTRL_IDLE_CYCLE(V)      (((V)&3)<<24)
#define CTRL_PRE_CYCLE(V)       (((V)&3)<<22)
#define CTRL_POST_CYCLE(V)      (((V)&3)<<20)
#define CTRL_SCLK_DEFAULT(V)    (((V)&1)<<19)
#define CTRL_SOUT3_DEFAULT(V)   (((V)&1)<<18)
#define CTRL_SOUT2_DEFAULT(V)   (((V)&1)<<17)
#define CTRL_SOUT1_DEFAULT(V)   (((V)&1)<<16)
#define CTRL_CS(V)              (((V)&0xff)<<8)
#define CTRL_DIV_SCLK(V)        (((V)&0xff)<<0)

#define CTC_QSPI_TX_BUFF         0x100
#define CTC_QSPI_RX_BUFF         0x180

#define CTC_QSPI_RX_BUFFER_SIZE		128
#define CTC_QSPI_TX_BUFFER_SIZE		256

#define SPINOR_OP_PP		0x02	/* Page program (up to 256 bytes) */

#define TIMEOUT_COUNT 65535

#define SPI_INT_EN 0x04
#define SPI_INT_STATUS 0x0c

struct ctc_qspi {
	void __iomem *regs;
	u32 speed_hz;
	u32 bus_clk;
	u32 num_chipselect;
	u32 idlecycle;
	u32 precycle;
	u32 postcycle;
	u32 sout1def;
	u32 sout2def;
	u32 sout3def;
	u32 qspi_mode;
	u32 clkdiv;
	u32 sclkdef;
	u32 cs_select;
	u32 bytes_to_transfer;
	u8 step;
	u32 tx_entry;
};

enum ctc_qspi_mode {
	PIO_MODE1 = 1,
	PIO_MODE2,
	PP_MODE,
	BOOT_MODE,
	XIP_MODE,
	MODE_MAX
};

enum type_mode {
	TYPE_PP = 1,
	TYPE_PIO,
	TYPE_MAX
};

static int ctc_reg_read(struct ctc_qspi *ctc_qspi, u32 reg, u32 * value)
{
	*value = readl(ctc_qspi->regs + reg);
	return *value;
}

static int ctc_reg_write(struct ctc_qspi *ctc_qspi, u32 reg, u32 value)
{
	writel(value, ctc_qspi->regs + reg);

	return 0;
}

static int ctc_reg_write_mask(struct ctc_qspi *ctc_qspi, u32 reg, u32 value,
			      u32 mask)
{
	u32 temp;

	ctc_reg_read(ctc_qspi, reg, &temp);
	temp &= ~mask;
	temp |= value;
	ctc_reg_write(ctc_qspi, reg, temp);

	return 0;
}

static int ctc_qspi_setup(struct spi_device *spi)
{
	struct ctc_qspi *ctc_qspi = spi_master_get_devdata(spi->master);

	if (spi->master->busy)
		return -EBUSY;

	ctc_qspi->sout1def = 1;
	ctc_qspi->sout2def = 1;
	ctc_qspi->sout3def = 1;

	ctc_qspi->speed_hz = spi->max_speed_hz;

	ctc_qspi->clkdiv = (ctc_qspi->bus_clk / (ctc_qspi->speed_hz * 2));

	if ((spi->mode & 0x3) == SPI_MODE_0)
		ctc_qspi->sclkdef = 0;
	else if ((spi->mode & 0x3) == SPI_MODE_3)
		ctc_qspi->sclkdef = 1;
	else
		ctc_qspi->sclkdef = 1;
	return 0;
}

static noinline int ctc_write_tx_buf(struct ctc_qspi *ctc_qspi, u8 offset,
				     u32 value)
{
	writel(value, ctc_qspi->regs + CTC_QSPI_TX_BUFF + offset);

	return 0;
}

static noinline int check_buf_ok(u8 * buf, int i)
{
	return buf && (buf + i);
}

static noinline int fill_tx_entry(struct ctc_qspi *ctc_qspi, u8 * buf, int i,
				  u8 off)
{
	ctc_qspi->tx_entry |= buf[i] << (off % 4) * 8;

	return 0;
}

static noinline void update_offset(u8 * offset, u8 off)
{
	*offset = off;
}

static void ctc_fill_tx_buf(struct ctc_qspi *ctc_qspi, u8 * offset, u8 * buf,
			    u32 len)
{

	int i = 0;
	u8 off = *offset;

	while (i < len) {
		if (check_buf_ok(buf, i)) {
			fill_tx_entry(ctc_qspi, buf, i, off);
		}

		if (off % 4 == 0) {
			ctc_write_tx_buf(ctc_qspi, off, ctc_qspi->tx_entry);
			ctc_qspi->tx_entry = 0;
		}
		i++;
		off--;
	}

	update_offset(offset, off);

}

static void ctc_fill_pp_buf(struct ctc_qspi *ctc_qspi, u32 * offset, u8 * buf,
			    u32 len)
{
	u32 i = 0, j = 0;
	u32 off = *offset;

	while (i < len) {
		for (j = 0; j < 4; j++) {
			if (buf && (buf + i)) {
				ctc_qspi->tx_entry |= buf[i + j] << (j % 4) * 8;
			}
		}
		ctc_write_tx_buf(ctc_qspi, off, ctc_qspi->tx_entry);
		ctc_qspi->tx_entry = 0;

		i = i + 4;
		off += 4;
	}
	*offset = off;
}

static void ctc_stepx_conf_init(struct ctc_qspi *ctc_qspi)
{
	ctc_qspi->step = 0;

	ctc_reg_write_mask(ctc_qspi, PIO_STEP0_CONF(ctc_qspi->qspi_mode), 0,
			   0xffffffff);
	ctc_reg_write_mask(ctc_qspi, PIO_STEP1_CONF(ctc_qspi->qspi_mode), 0,
			   0xffffffff);
	ctc_reg_write_mask(ctc_qspi, PIO_STEP2_CONF(ctc_qspi->qspi_mode), 0,
			   0xffffffff);
	ctc_reg_write_mask(ctc_qspi, PIO_STEP3_CONF(ctc_qspi->qspi_mode), 0,
			   0xffffffff);
}

static void ctc_stepx_conf(struct ctc_qspi *ctc_qspi, u8 lanes, u32 bytes,
			   u32 output_en)
{
	u32 cycle = 0;
	u32 stepx_conf = 0;

	if (bytes <= 0)
		return;

	cycle = (bytes * 8) / lanes;

	if (lanes == 1) {
		stepx_conf = (0xd << 20) | (lanes << 16) | (cycle);
	} else if (lanes == 2) {
		stepx_conf = output_en ? (0x3 << 20) | (lanes << 16) | (cycle) :
		    (0xc << 20) | (lanes << 16) | (cycle);
	} else if (lanes == 4) {
		stepx_conf = output_en ? (0xf << 20) | (lanes << 16) | (cycle) :
		    (0x0 << 20) | (lanes << 16) | (cycle);
	}

	if (ctc_qspi->step == 0) {
		ctc_reg_write_mask(ctc_qspi,
				   PIO_STEP0_CONF(ctc_qspi->qspi_mode),
				   stepx_conf, 0xffffffff);
		ctc_qspi->step++;
	} else if (ctc_qspi->step == 1) {
		ctc_reg_write_mask(ctc_qspi,
				   PIO_STEP1_CONF(ctc_qspi->qspi_mode),
				   stepx_conf, 0xffffffff);
		ctc_qspi->step++;
	} else if (ctc_qspi->step == 2) {
		ctc_reg_write_mask(ctc_qspi,
				   PIO_STEP2_CONF(ctc_qspi->qspi_mode),
				   stepx_conf, 0xffffffff);
		ctc_qspi->step++;
	} else if (ctc_qspi->step == 3) {
		ctc_reg_write_mask(ctc_qspi,
				   PIO_STEP3_CONF(ctc_qspi->qspi_mode),
				   stepx_conf, 0xffffffff);
		ctc_qspi->step++;
	}

}

static void ctc_select_qspi_mode(struct ctc_qspi *ctc_qspi, u32 qspi_mode)
{
	if ((qspi_mode == PIO_MODE1) || (qspi_mode == PIO_MODE2)) {
		ctc_reg_write_mask(ctc_qspi, SPI_MODE, ctc_qspi->qspi_mode,
				   0xffffffff);
	} else if (qspi_mode == PP_MODE) {
		ctc_reg_write_mask(ctc_qspi, SPI_MODE, 0x100, 0xffffffff);
	}
}

static void ctc_qspi_pio_ctrl(struct ctc_qspi *ctc_qspi)
{
	u32 ctrl = 0;

	ctrl =
	    CTRL_IDLE_CYCLE(ctc_qspi->idlecycle) | CTRL_PRE_CYCLE(ctc_qspi->
								  precycle)
	    | CTRL_POST_CYCLE(ctc_qspi->
			      postcycle) | CTRL_SCLK_DEFAULT(ctc_qspi->sclkdef)
	    | CTRL_SOUT3_DEFAULT(ctc_qspi->
				 sout3def) | CTRL_SOUT2_DEFAULT(ctc_qspi->
								sout2def)
	    | CTRL_SOUT1_DEFAULT(ctc_qspi->sout1def) | CTRL_CS(ctc_qspi->
							       cs_select)
	    | CTRL_DIV_SCLK(ctc_qspi->clkdiv);

	ctc_reg_write_mask(ctc_qspi, PIO_CTRL(ctc_qspi->qspi_mode), ctrl,
			   0xffffffff);
}

static void ctc_qspi_pp_ctrl(struct ctc_qspi *ctc_qspi)
{
	u32 ctrl = 0;

	ctrl =
	    CTRL_IDLE_CYCLE(ctc_qspi->idlecycle) | CTRL_PRE_CYCLE(ctc_qspi->
								  precycle)
	    | CTRL_POST_CYCLE(ctc_qspi->
			      postcycle) | CTRL_SCLK_DEFAULT(ctc_qspi->sclkdef)
	    | CTRL_SOUT3_DEFAULT(ctc_qspi->
				 sout3def) | CTRL_SOUT2_DEFAULT(ctc_qspi->
								sout2def)
	    | CTRL_SOUT1_DEFAULT(ctc_qspi->sout1def) | CTRL_CS(ctc_qspi->
							       cs_select)
	    | CTRL_DIV_SCLK(ctc_qspi->clkdiv);

	ctc_reg_write_mask(ctc_qspi, PP_CTRL, ctrl, 0xffffffff);
}

static u32 ctc_pp_conf(u8 lanes, u32 len)
{
	u32 cycle = 0;

	cycle = (len * 8) / lanes;

	return (lanes << 16) | (cycle);
}

static int ctc_read_rx_buf(struct ctc_qspi *ctc_qspi, u8 offset, u8 * value)
{
	*value = readb(ctc_qspi->regs + CTC_QSPI_RX_BUFF + offset);

	return 0;
}

static void ctc_extra_rx_buf(struct ctc_qspi *ctc_qspi, u8 offset, u8 * buf,
			     u8 len)
{
	int i = 0;
	while (i < len) {
		ctc_read_rx_buf(ctc_qspi, offset, &buf[i++]);
		offset--;
	}
}

static int ctc_transfer_for_PP_mode(struct ctc_qspi *ctc_qspi,
				    struct spi_transfer *p_xfers[],
				    u8 xfers_num, u32 msg_len)
{
	u8 i;
	u32 pp_conf;
	u32 timeout = 0;
	u32 temp;
	u32 offset;
	struct spi_mem_op mem_ops;

	memset(&mem_ops, 0, sizeof(mem_ops));

	mem_ops.cmd.opcode = ((u8 *) p_xfers[0]->tx_buf)[0];
	mem_ops.cmd.buswidth = p_xfers[0]->tx_nbits;

	mem_ops.addr.nbytes = p_xfers[1]->len;
	mem_ops.addr.buswidth = p_xfers[1]->tx_nbits;
	for (i = 0; i < mem_ops.addr.nbytes; i++) {
		mem_ops.addr.val |=
		    ((u8 *) p_xfers[1]->tx_buf)[i] << (8 *
						       (mem_ops.addr.nbytes -
							i - 1));
	}

	if (xfers_num >= 4) {
		mem_ops.addr.nbytes = p_xfers[2]->len;
		mem_ops.addr.buswidth = p_xfers[2]->tx_nbits;
	}

	mem_ops.data.nbytes = p_xfers[xfers_num - 1]->len;
	mem_ops.data.buswidth = p_xfers[xfers_num - 1]->tx_nbits;
	mem_ops.data.buf.out = p_xfers[xfers_num - 1]->tx_buf;

	ctc_qspi->qspi_mode = PP_MODE;
	ctc_select_qspi_mode(ctc_qspi, ctc_qspi->qspi_mode);
	ctc_qspi_pp_ctrl(ctc_qspi);

	/* Fill buffer */
	offset = 0;
	ctc_qspi->tx_entry = 0;
	ctc_fill_pp_buf(ctc_qspi, &offset, (u8 *) mem_ops.data.buf.out,
			mem_ops.data.nbytes);

	/* PP CMD */
	ctc_reg_write_mask(ctc_qspi, PP_CMD_CODE, mem_ops.cmd.opcode,
			   0xffffffff);
	pp_conf = ctc_pp_conf(mem_ops.cmd.buswidth, 1);
	ctc_reg_write_mask(ctc_qspi, PP_CMD_CONF, pp_conf, 0xffffffff);

	/* PP ADDR */
	ctc_reg_write_mask(ctc_qspi, PP_ADDR_CODE, mem_ops.addr.val,
			   0xffffffff);
	pp_conf = ctc_pp_conf(mem_ops.addr.buswidth, mem_ops.addr.nbytes);
	ctc_reg_write_mask(ctc_qspi, PP_ADDR_CONF, pp_conf, 0xffffffff);

	/* PP DUMMY */
	ctc_reg_write_mask(ctc_qspi, PP_DUMMY_CODE, 0x00000000, 0xffffffff);
	pp_conf = ctc_pp_conf(mem_ops.dummy.buswidth, mem_ops.dummy.nbytes);
	ctc_reg_write_mask(ctc_qspi, PP_DUMMY_CONF, pp_conf, 0xffffffff);

	/* PP DATA */
	pp_conf = ctc_pp_conf(mem_ops.data.buswidth, mem_ops.data.nbytes);
	ctc_reg_write_mask(ctc_qspi, PP_DATA_CONF, pp_conf, 0xffffffff);

	/* PP GO */
	ctc_reg_write_mask(ctc_qspi, PP_GO, 0x01, 0xffffffff);
	while (ctc_reg_read(ctc_qspi, PP_GO, &temp) & 0x1) {
		if (timeout++ > TIMEOUT_COUNT)
			break;
		udelay(1);
	}

	return msg_len;
}

static int ctc_transfer_for_PIO(struct ctc_qspi *ctc_qspi,
				struct spi_transfer *p_xfers[], u8 xfers_num,
				u32 msg_len)
{
	u8 i;
	u8 offset;
	u32 timeout = 0;
	u32 temp;

	ctc_qspi->qspi_mode = PIO_MODE1;
	ctc_select_qspi_mode(ctc_qspi, ctc_qspi->qspi_mode);
	ctc_qspi_pio_ctrl(ctc_qspi);
	ctc_stepx_conf_init(ctc_qspi);

	offset = msg_len - 1;
	ctc_qspi->tx_entry = 0;
	for (i = 0; i < xfers_num; i++) {
		if (p_xfers[i]->tx_buf) {
			ctc_fill_tx_buf(ctc_qspi, &offset,
					(u8 *) p_xfers[i]->tx_buf,
					p_xfers[i]->len);
			ctc_stepx_conf(ctc_qspi, p_xfers[i]->tx_nbits,
				       p_xfers[i]->len, 1);
		} else {
			ctc_fill_tx_buf(ctc_qspi, &offset,
					(u8 *) p_xfers[i]->rx_buf,
					p_xfers[i]->len);
			ctc_stepx_conf(ctc_qspi, p_xfers[i]->rx_nbits,
				       p_xfers[i]->len, 0);
		}
	}

	/* PIO write start transfer */
	ctc_reg_write_mask(ctc_qspi, PIO_GO(ctc_qspi->qspi_mode), 0x01,
			   0xffffffff);
	while (ctc_reg_read(ctc_qspi, PIO_GO(ctc_qspi->qspi_mode), &temp) & 0x1) {
		if (timeout++ > TIMEOUT_COUNT)
			break;
		udelay(1);
	}

	if (p_xfers[xfers_num - 1]->rx_buf) {
		ctc_extra_rx_buf(ctc_qspi, p_xfers[xfers_num - 1]->len - 1,
				 p_xfers[xfers_num - 1]->rx_buf,
				 p_xfers[xfers_num - 1]->len);
	}

	return msg_len;
}

static int ctc_qspi_start_transfer_one(struct spi_master *master,
				       struct spi_message *msg)
{
	struct ctc_qspi *ctc_qspi = spi_master_get_devdata(master);
	struct spi_device *spi = msg->spi;
	struct spi_transfer *t;
	u8 xfer_num = 0;
	struct spi_transfer *xfers[4];
	u32 msg_len = 0;

	ctc_qspi->cs_select = (0x1 << spi->chip_select);
	list_for_each_entry(t, &msg->transfers, transfer_list) {
		xfers[xfer_num] = t;
		xfer_num++;
		msg_len += t->len;
	}

	if (xfers[xfer_num - 1]->tx_buf && xfer_num >= 3) {
		msg->actual_length =
		    ctc_transfer_for_PP_mode(ctc_qspi, xfers, xfer_num,
					     msg_len);
	} else {
		msg->actual_length =
		    ctc_transfer_for_PIO(ctc_qspi, xfers, xfer_num, msg_len);
	}

	msg->status = 0;
	spi_finalize_current_message(master);

	return 0;
}

int ctc_qspi_adjust_op_size(struct spi_mem *mem, struct spi_mem_op *op)
{
	//max data transfer size = tx buffer size - (cmd - addr -dummy )
	if (op->data.dir == SPI_MEM_DATA_IN) {
		if (op->data.nbytes > CTC_QSPI_RX_BUFFER_SIZE - 6) {
			op->data.nbytes = CTC_QSPI_RX_BUFFER_SIZE - 6;
		}
	} else {
		if (op->data.nbytes > CTC_QSPI_TX_BUFFER_SIZE) {
			op->data.nbytes = CTC_QSPI_TX_BUFFER_SIZE;
		}
	}

	return 0;
}

static int ctc_qspi_exec_mem_op(struct spi_mem *mem,
				const struct spi_mem_op *op)
{
	return -ENOTSUPP;
}

static const struct spi_controller_mem_ops ctc_qspi_mem_ops = {
	.adjust_op_size = ctc_qspi_adjust_op_size,
	.exec_op = ctc_qspi_exec_mem_op,
};

static int ctc_qspi_probe(struct platform_device *pdev)
{
	int ret = 0, irq;
	struct spi_master *master;
	struct ctc_qspi *ctc_qspi;
	struct resource *res;
	struct device_node *np = pdev->dev.of_node;
	u32 tmp;

	master = spi_alloc_master(&pdev->dev, sizeof(*ctc_qspi));
	if (!master)
		return -ENOMEM;

	master->mode_bits =
	    SPI_MODE_3 | SPI_MODE_0 | SPI_TX_DUAL | SPI_RX_DUAL | SPI_TX_QUAD |
	    SPI_RX_QUAD;
	master->setup = ctc_qspi_setup;
	master->transfer_one_message = ctc_qspi_start_transfer_one;
	master->bits_per_word_mask =
	    SPI_BPW_MASK(32) | SPI_BPW_MASK(16) | SPI_BPW_MASK(8);
	master->mem_ops = &ctc_qspi_mem_ops;
	if (!of_property_read_u32(np, "num-cs", &tmp))
		master->num_chipselect = tmp;

	ctc_qspi = spi_master_get_devdata(master);
	master->dev.of_node = pdev->dev.of_node;
	platform_set_drvdata(pdev, master);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ctc_qspi->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ctc_qspi->regs)) {
		ret = PTR_ERR(ctc_qspi->regs);
		goto remove_master;
	}

	if (!of_property_read_u32(np, "pclk", &tmp))
		ctc_qspi->bus_clk = tmp;
	if (!of_property_read_u32(np, "idle-cycle", &tmp))
		ctc_qspi->idlecycle = tmp;
	if (!of_property_read_u32(np, "pre-cycle", &tmp))
		ctc_qspi->precycle = tmp;
	if (!of_property_read_u32(np, "post-cycle", &tmp))
		ctc_qspi->postcycle = tmp;
	if (!of_property_read_u32(np, "qspi-mode", &tmp))
		ctc_qspi->qspi_mode = tmp;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return irq;
	}
	ret = devm_spi_register_master(&pdev->dev, master);
	if (!ret)
		return 0;
	return 0;

remove_master:
	spi_master_put(master);

	return ret;
}

static int ctc_qspi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);

	spi_unregister_master(master);
	return 0;
}

static const struct of_device_id ctc_qspi_match[] = {
	{
	 .compatible = "ctc, igdaxi001a-qspi",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, ctc_qspi_match);

/* Structure for a device driver */
static struct platform_driver ctc_qspi_driver = {
	.driver = {
		   .name = "ctc-qspi",
		   .of_match_table = ctc_qspi_match,
		   },
	.probe = ctc_qspi_probe,
	.remove = ctc_qspi_remove,
};

module_platform_driver(ctc_qspi_driver);

MODULE_AUTHOR("Centec, Inc.");
MODULE_LICENSE("GPL");
