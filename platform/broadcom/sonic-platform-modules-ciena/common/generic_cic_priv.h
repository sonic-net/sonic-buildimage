#ifndef _GENERIC_CIC_PRIV_H
#define _GENERIC_CIC_PRIV_H

#include "generic_cic_config.h"
#include <linux/device.h>
#include <linux/module.h>          /* module_init/exit, MODULE* */
#include <linux/export.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h> /* platform_*                */
#include <linux/list.h>
#include <linux/regmap.h>
#include <linux/rtmutex.h>
#include <linux/slab.h>            /* kzalloc/kfree             */
#include <linux/gpio/driver.h>
#ifdef CONFIG_PCI_MSI
#include <linux/pci.h>
#endif
#ifdef CONFIG_CIENA_MCEE
#include <linux/ciena_mcee.h>
#endif
#include "generic_cic.h"
#include "generic_cic_common.h"
#include "generic_cic_dumper.h"
#include "generic_cic_tp.h"
#include "gpiotest-sysfs.h"

#define DRIVER_NAME "ciena-" MOD_NAME "-cic-drv"
#define DEVICE_NAME CIC_NAME
#define CIC_MAX_REPEAT 10000
#define CIC_NO_IRQ 0

static const char *generic_cic_name        = MOD_NAME;
static const char *generic_cic_name_msi    = "MSI_CTRL";
static const char *generic_cic_name_master = "MASTER";
static const char *generic_cic_suffix_enb  = "ENB";
static const char *generic_cic_suffix_irq  = "IRQ";
static const char *generic_cic_suffix_mask = "MSK";
static const char *generic_cic_suffix_stat = "STAT";

#define BIT_TO_BYTE(bit_width) (bit_width >> 3)

/* -------------------------------------------------------------------------- */
/* private data per cic */

typedef void  (*cic_lock_func)(struct generic_cic_priv *priv);
typedef void  (*cic_unlock_func)(struct generic_cic_priv *priv);
typedef reg_t (*cic_read_func)(struct generic_cic_priv *priv,
			       unsigned int reg);
typedef void  (*cic_write_func)(struct generic_cic_priv *priv,
				unsigned int reg, reg_t val);
typedef void  (*cic_irqclr_func)(struct generic_cic_priv *priv,
				 unsigned int reg, reg_t val, const char *nm);

struct generic_cic_priv {
	irq_hw_number_t            in_hwirq;
	unsigned int               in_virq;
	u64                        reg_base;
	void __iomem              *iobase;
	int                        irq_sources_max;
	irq_hw_number_t            hwirq_max;
	int                        level;
	unsigned                   nregs;
	irq_reg_info_t            *irq_reg_table;
	irq_reg_info_t             irq_test_reg;
	irq_hw_number_t           *pin_to_hwirq;
	int                       *hwirq_to_pin;
	const char               **gpio_labels;
	int                        master_present;
	struct device             *dev;
	struct pci_dev            *msi_parent;
	struct gpio_chip           gc;
	struct irq_chip            ic;
	struct platform_device    *gpio;
	struct platform_device   **gpiotest_sysfs;
	struct gpio_desc         **gdescs;
	int                        num_gdescs;
	struct generic_cic_config  config;
	spinlock_t                 generic_cic_lock;
	unsigned long              spin_flags;
	struct rt_mutex            generic_cic_mutex;
	struct gpiod_lookup_table *gpio_lkup_table;
	struct regmap             *parent_regmap;
	unsigned long              irq_en_dirty;
	unsigned                  *irq_regnums;
	unsigned                  *irq_en_regs;
	unsigned                  *irq_scratch;
	u32                        irq_seq;
	ktime_t                    msi_in;
	ktime_t                    msi_out;
	ktime_t                    irq_out;
	struct ciena_cic_dumper    ccd;
	cic_lock_func              cic_lock;
	cic_unlock_func            cic_unlock;
	cic_read_func              cic_rd;
	cic_write_func             cic_wr;
	cic_irqclr_func            cic_irqclr;
#ifdef CONFIG_CIENA_MCEE
	struct ciena_mcee_dev      mceed;
#endif
};

static inline struct generic_cic_priv *to_ciena_generic_gpio_priv(struct gpio_chip *gc)
{
	return container_of(gc, struct generic_cic_priv, gc);
}

/* -------------------------------------------------------------------------- */
/* OF config parsing */
static inline
struct generic_cic_config *generic_cic_get_of_config(struct device_node *np,
						     struct generic_cic_config *config)
{
	if (NULL == np) return NULL;
#ifdef CONFIG_OF
	{
		u64 start_offset;
		u32 gpio_base;

		if (!of_property_read_u64(np, "start-offset", &start_offset))
			config->start_offset = start_offset;

		if (of_find_property(np, "shared-io", NULL))
			config->shared_io = true;

		if (of_find_property(np, "little-endian", NULL))
			config->little_endian = true;

		if (of_find_property(np, "use-parent-msi", NULL))
			config->use_parent_msi = true;

		if (!of_property_read_u32(np, "gpio-base", &gpio_base))
			config->gpio_base = gpio_base;
	}
#endif
	return config;
}

/* -------------------------------------------------------------------------- */
static reg_t generic_cic_read_regmap(struct generic_cic_priv *priv,
				     unsigned int reg)
{
	unsigned int value = 0;
	int rc;

	BUG_ON(priv == NULL);

	rc = regmap_read(priv->parent_regmap, reg, &value);

	dev_dbg(priv->dev, "%-29s:reg             (0x%08x) = 0x%08x, rc=%d\n",
		__func__, reg, value, rc);

	return value;
}

static void generic_cic_write_regmap(struct generic_cic_priv *priv,
				     unsigned int reg, reg_t val)
{
	int rc;

	rc = regmap_write(priv->parent_regmap, reg, val);

	dev_dbg(priv->dev, "%-29s:reg             (0x%08x) = 0x%08x, rc=%d\n",
		__func__, reg, val, rc);
}

/* -------------------------------------------------------------------------- */
static reg_t generic_cic_read_mmio(struct generic_cic_priv *priv,
				   unsigned int reg)
{
	u64 offset = ((u64)reg - priv->reg_base);
	reg_t value = 0;

	BUG_ON(priv == NULL);

	switch (BIT_TO_BYTE(REG_WIDTH)) {
		case sizeof(u16):
			value = ioread16(priv->iobase + offset);
			break;
		case sizeof(u32):
			value = ioread32(priv->iobase + offset);
			break;
		default:
			BUG();
	}

	dev_dbg(priv->dev, "%-29s:reg             (0x%08x) = 0x%08x\n",
		__func__, reg, value);

	return value;
}

static void generic_cic_write_mmio(struct generic_cic_priv *priv,
				   unsigned int reg, reg_t val)
{
	u64 offset = ((u64)reg - priv->reg_base);

	BUG_ON(priv == NULL);

	switch (BIT_TO_BYTE(REG_WIDTH)) {
		case sizeof(u16):
			iowrite16(val, priv->iobase + offset);
			break;
		case sizeof(u32):
			iowrite32(val, priv->iobase + offset);
			break;
		default:
			BUG();
	}

	dev_dbg(priv->dev, "%-29s:reg             (0x%08x) = 0x%08x\n",
		__func__, reg, val);
}

/* -------------------------------------------------------------------------- */
static inline reg_t generic_cic_read(struct generic_cic_priv *priv,
				     unsigned int reg,
				     const char *reg_name,
				     const char *suffix)
{
	reg_t val = (*priv->cic_rd)(priv, reg);

	trace_ciena_cic_reg_rd(generic_cic_name, reg_name, suffix, reg, val);

	return val;
}

static inline void generic_cic_write(struct generic_cic_priv *priv,
				     unsigned int reg, reg_t val,
				     const char *reg_name,
				     const char *suffix)
{
	trace_ciena_cic_reg_wr(generic_cic_name, reg_name, suffix, reg, val);

	(*priv->cic_wr)(priv, reg, val);
}

/* -------------------------------------------------------------------------- */
static void generic_cic_irq_set2clr_mmio(struct generic_cic_priv *priv,
					 unsigned int reg, reg_t val,
					 const char *name)
{
	trace_ciena_cic_reg_wr(generic_cic_name, name,
			       generic_cic_suffix_irq, reg, val);

	generic_cic_write_mmio(priv, reg, val);
}

static void generic_cic_irq_set2clr_regmap(struct generic_cic_priv *priv,
					   unsigned int reg, reg_t val,
					   const char *name)
{
	trace_ciena_cic_reg_wr(generic_cic_name, name,
			       generic_cic_suffix_irq, reg, val);

	generic_cic_write_regmap(priv, reg, val);
}

static void generic_cic_irq_clr2clr_mmio(struct generic_cic_priv *priv,
					 unsigned int reg, reg_t val,
					 const char *name)
{
	reg_t wrval = generic_cic_read_mmio(priv, reg);

	wrval &= ~val;

	trace_ciena_cic_reg_wr(generic_cic_name, name,
			       generic_cic_suffix_irq, reg, wrval);

	generic_cic_write_mmio(priv, reg, wrval);
}

static void generic_cic_irq_clr2clr_regmap(struct generic_cic_priv *priv,
					   unsigned int reg, reg_t val,
					   const char *name)
{
	int rc;

	trace_ciena_cic_reg_wr(generic_cic_name, name,
			       generic_cic_suffix_irq, reg,
			       generic_cic_read_regmap(priv, reg) & ~val);

	rc = regmap_write_bits(priv->parent_regmap, reg, val, 0);

	dev_dbg(priv->dev, "%-29s:reg             (0x%08x) = 0x%08x, rc=%d\n",
		__func__, reg, val, rc);
}

/* -------------------------------------------------------------------------- */
static void generic_cic_spin_lock(struct generic_cic_priv *priv)
{
	spin_lock_irqsave(&priv->generic_cic_lock, priv->spin_flags);
}

static void generic_cic_spin_unlock(struct generic_cic_priv *priv)
{
	spin_unlock_irqrestore(&priv->generic_cic_lock, priv->spin_flags);
}

/* -------------------------------------------------------------------------- */
static void generic_cic_mutex_lock(struct generic_cic_priv *priv)
{
	rt_mutex_lock(&priv->generic_cic_mutex);
}

static void generic_cic_mutex_unlock(struct generic_cic_priv *priv)
{
	rt_mutex_unlock(&priv->generic_cic_mutex);
}

/* -------------------------------------------------------------------------- */
static inline void generic_cic_lock(struct generic_cic_priv *priv)
{
	(*priv->cic_lock)(priv);
}

static inline void generic_cic_unlock(struct generic_cic_priv *priv)
{
	(*priv->cic_unlock)(priv);
}

/* -------------------------------------------------------------------------- */
/* Straight functions for interrupt controllers with a mask
 * (i.e. write-one-to-disable) register.
 */
static reg_t generic_cic_rd_disable_mask(struct generic_cic_priv *p,
					 irq_reg_info_t *iri)
{
	return generic_cic_read(p, iri->disable, iri->name,
				generic_cic_suffix_mask);
}

static void generic_cic_wr_disable_mask(struct generic_cic_priv *p,
					irq_reg_info_t *iri, reg_t v)
{
	generic_cic_write(p, iri->disable, v, iri->name,
			  generic_cic_suffix_mask);
}

/* -------------------------------------------------------------------------- */
/* Backwards functions for interrupt controllers with an enable
 * (i.e. write-zero-to-disable) register.
 */
static reg_t generic_cic_rd_enable_mask(struct generic_cic_priv *p,
					irq_reg_info_t *iri)
{
	return ~generic_cic_read(p, iri->enable, iri->name,
				 generic_cic_suffix_enb);
}

static void generic_cic_wr_enable_mask(struct generic_cic_priv *p,
				       irq_reg_info_t *iri, reg_t v)
{
	generic_cic_write(p, iri->enable, ~v, iri->name,
			  generic_cic_suffix_enb);
}

/* -------------------------------------------------------------------------- */
static void generic_mask_test_irq(struct generic_cic_priv *priv)
{
	irq_reg_info_t *test_reg = &priv->irq_test_reg;
	u32 test_mask = test_reg->mask;
	if (test_mask) {
		/* there is a test IRQ pin: disable it before it gets noisy */
		u32 test_intr = generic_cic_read(priv, test_reg->intr,
						 test_reg->name, NULL);
		dev_dbg(priv->dev, "%-29s: %s = 0x%x & 0x%x)\n",
			 __func__, test_reg->name, test_intr, ~test_mask);
		generic_cic_write(priv, test_reg->intr, test_intr & ~test_mask,
				  test_reg->name, NULL);
	}
}

/* -------------------------------------------------------------------------- */
static inline
irq_hw_number_t cic_regnum_bit_to_hwirq(struct generic_cic_priv *priv,
					unsigned                 regnum,
					int                      bit)
{
	irq_hw_number_t hwirq;
	int             pin;

	pin   = bit + regnum * REG_WIDTH;
	hwirq = priv->pin_to_hwirq[pin];

	dev_dbg(priv->dev, "%-29s: reg=%d + bit=%d -> pin=%d\n",
		__func__, regnum, bit, pin);

	return hwirq;
}

/* -------------------------------------------------------------------------- */
static inline int generic_find_hwirq(struct generic_cic_priv *priv,
				     irq_hw_number_t          hwirq,
				     irq_reg_info_t         **reg,
				     unsigned                *bit)
{
	int pin;

	if (hwirq >= priv->hwirq_max)
		return -1;

	pin = priv->hwirq_to_pin[hwirq];

	*reg = priv->irq_reg_table + (pin / REG_WIDTH);
	*bit = pin % REG_WIDTH;

	if ((*reg)->mask & BIT(*bit)) return 0;

	return -1;
}

/* -------------------------------------------------------------------------- */
static void generic_cic_pin_do_mask_ack(struct generic_cic_priv *priv,
					irq_reg_info_t *reg, unsigned bit);

static void generic_cic_pin_mask_ack(struct generic_cic_priv *priv,
				     irq_hw_number_t hwirq, bool do_lock);

struct generic_cic_pending_info {
	struct generic_cic_priv *priv;
	unsigned int             in_virq;
	unsigned                 master;
	unsigned                 nregs;
	const unsigned          *regmask;
	unsigned                *regnums;
	unsigned                *regvals;
};

static inline int generic_cic_pending_irqs(struct generic_cic_pending_info *i)
{
	struct generic_cic_priv *priv    = i->priv;
	struct device           *dev     = priv->dev;
	unsigned int             in_virq = i->in_virq;
	u32                      cause   = 0;
	u32                      enable  = 0;
	u32                      result  = 0;
	u32                      master  = 0;
	u32                      m_sum   = ~0U;
	u32                      m_mask  = ~0U;
	int                      regcnt  = 0;
	irq_reg_info_t          *irq_reg;

	dev_dbg(dev, "%s: in_virq(%d) priv->in_virq(%d)\n",
		__func__, in_virq, priv->in_virq);

	/* Check irq line routed to the MPIC */
	if (in_virq != priv->in_virq) {
		dev_err(dev, "%-29s:in_virq(%d) mismatch(%d)\n",
			__func__, in_virq, priv->in_virq);
		return -1;
	}

	if (priv->master_present) {
		u32 master_mask = cic_master_mask(dev);

		m_sum  = generic_cic_read(priv, cic_master_sum(dev),
					  generic_cic_name_master,
					  generic_cic_suffix_irq);
		m_mask = (master_mask ?
			  ~generic_cic_read(priv, master_mask,
					    generic_cic_name_master,
					    generic_cic_suffix_mask) :
			  ~master_mask);
		master = m_sum & m_mask;

		if (!master) {
			generic_mask_test_irq(priv);
			return 0;
		}
		i->master = master;
	}

	trace_ciena_cic_in(generic_cic_name, in_virq, m_sum, m_mask);

	for_each_irq_reg(priv, irq_reg) {
		unsigned regnum = irq_reg - priv->irq_reg_table;

		if ((master & irq_reg->master) == 0)
			continue;

		cause = generic_cic_read(priv, irq_reg->intr, irq_reg->name,
					 generic_cic_suffix_irq);

		/* cause == 0 is okay as some sources can share the
		 * same master */
		if ( cause == 0 )
			continue;

		enable = (i->regmask ?
			  i->regmask[regnum] :
			  ~(*irq_reg->rd_mask)(priv, irq_reg));

		result = cause & enable & irq_reg->mask;

		if ( result ) {
			i->regnums[regcnt] = regnum;
			i->regvals[regcnt] = result;
			dev_dbg(dev, "%-29s:%s (master=0x%.8x "
				"cause=0x%.8x enable=0x%.8x)\n", __func__,
				irq_reg->name, master, cause, enable);

			if (++regcnt == i->nregs) break;
		}
	}

	if (unlikely(!regcnt)) {
		/* the bottom has been reached: quietly ack the master */
		(priv->cic_irqclr)(priv, cic_master_sum(dev), master,
				   generic_cic_name_master);
		generic_mask_test_irq(priv);
		dev_dbg(dev, "%-29s:the end (master=0x%.8x "
			"cause=0x%.8x enable=0x%.8x)\n",
			__func__, master, cause, enable);
	}

	return regcnt;
}

/* -------------------------------------------------------------------------- */
static inline unsigned generic_cic_get_out_virq(struct generic_cic_priv *priv,
						unsigned int in_virq,
						struct gpio_chip *gc)
{
	irq_reg_info_t *irq_reg;
	unsigned        regnum;
	unsigned        result;
	int             bit     = 0;
	unsigned int    virq    = CIC_NO_IRQ;
	irq_hw_number_t hwirq;
	int             pin;

	struct generic_cic_pending_info info = {
		.priv    = priv,
		.in_virq = in_virq,
		.nregs   = 1,
		.regnums = &regnum,
		.regvals = &result,
	};

	if (0 >= generic_cic_pending_irqs(&info))
		return CIC_NO_IRQ;

	irq_reg = priv->irq_reg_table + regnum;

	bit = ffs(result) - 1;

	if (unlikely(!(irq_reg->mask & BIT(bit)))) {
		/* A bit fired that we don't know about? */
		generic_cic_pin_do_mask_ack(priv, irq_reg, bit);

		dev_err(priv->dev, "%s: %s.%d invalid interrupt "
			"(result=0x%.8x mask=0x%.8x)\n", __func__,
			irq_reg->name, bit, result, irq_reg->mask);

		return CIC_NO_IRQ;
	}

	hwirq = cic_regnum_bit_to_hwirq(priv, regnum, bit);
	pin   = priv->hwirq_to_pin[hwirq];

	dev_dbg(priv->dev, "%s: %s.%d -> pin %d (result=0x%.8x "
		"mask=0x%.8x)\n", __func__, irq_reg->name, bit,
		pin, result, irq_reg->mask );

	virq = irq_find_mapping(priv->gc.irq.domain, hwirq);
	if (unlikely( CIC_NO_IRQ == virq )) {
		dev_err(priv->dev, "%-29s:pin(%d) has no revmap\n",
			__func__, pin);
		generic_cic_pin_mask_ack(priv, hwirq, false);
	}
	else {
		u16 *count = &irq_reg->irq_count[bit];
		u32 *seq   = &irq_reg->irq_seq[bit];

		trace_ciena_cic_out(generic_cic_name, virq,
				    cic_to_str(priv->dev, pin, priv->level),
				    irq_reg->mask, irq_reg->name,
				    bit, result);

		if (likely( priv->irq_seq != *seq )) {
			*count = 0;
			*seq   = priv->irq_seq;
		}
		else {
			(*count)++;

			if (unlikely( CIC_MAX_REPEAT <= *count )) {
				dev_err(priv->dev, "disabling stuck irq %s",
					cic_to_str(priv->dev, pin, priv->level));
				generic_cic_pin_mask_ack(priv, hwirq, false);
			}
			/* The same interrupt is re-occurring. We may be
			 * dealing with a sticky test interrupt that does not
			 * self clear. */
			generic_mask_test_irq(priv);
		}
	}

	return virq;
}

static void generic_cic_cascade(struct irq_desc *desc)
{
	unsigned int             in_virq  = irq_desc_get_irq(desc);
	struct gpio_chip        *gc       = irq_desc_get_handler_data(desc);
	struct generic_cic_priv *priv     = to_ciena_generic_gpio_priv(gc);
	struct device           *dev      = priv->dev;
	struct irq_chip         *irqchip  = irq_desc_get_chip(desc);
	uint32_t                 msi_ctrl = cic_msi_ctrl(dev);
	unsigned int             out_virq;

	/* Stop the MSIs from coming in. */
	if (msi_ctrl) {
		generic_cic_write(priv, msi_ctrl, 0,
				  generic_cic_name_msi, NULL);
		priv->msi_in = ktime_get();
	}

	dev_dbg(dev, "%s: in_virq(%d) gc(%s}\n", __func__, in_virq, gc->label);


	priv->irq_seq++;

	chained_irq_enter(irqchip, desc);

	while (true) {
		generic_cic_lock(priv);

		out_virq = generic_cic_get_out_virq(priv, in_virq, gc);

		generic_cic_unlock(priv);

		if (CIC_NO_IRQ == out_virq) break;

		generic_handle_irq(out_virq);
		priv->irq_out = ktime_get();
	}

	chained_irq_exit(irqchip, desc);

	/* Re-enable the MSIs. */
	if (msi_ctrl) {
		priv->msi_out = ktime_get();
		generic_cic_write(priv, msi_ctrl, ~0U,
				  generic_cic_name_msi, NULL);
	}
}

/* -------------------------------------------------------------------------- */
static irqreturn_t generic_cic_irq_handler(int irq, void *d)
{
	/* some FPGA interrupts are very noisy: disable them immediately */
	disable_irq_nosync(irq);
	return IRQ_WAKE_THREAD;
}

/* -------------------------------------------------------------------------- */
static irqreturn_t generic_cic_irq_thread(int irq, void *d)
{
	struct generic_cic_priv *priv = (struct generic_cic_priv *) d;
	struct device           *dev  = priv->dev;
	irq_reg_info_t          *irq_reg;
	unsigned                 irq_posted;
	unsigned                 out_irq;
	unsigned                 result;
	unsigned                 regnum;
	irq_hw_number_t          hwirq;
	int                      regcnt;
	int                      idx;
	int                      pin;
	int                      bit;

	struct generic_cic_pending_info info = {
		.priv    = priv,
		.in_virq = irq,
		.nregs   = priv->nregs,
		.regmask = priv->irq_en_regs,
		.regnums = priv->irq_regnums,
		.regvals = priv->irq_scratch,
	};

	generic_cic_lock(priv);

	priv->irq_seq++;

	regcnt = generic_cic_pending_irqs(&info);

	/* acknowledge all the interrupts at once */
	idx = 0;
	while (idx < regcnt) {
		regnum  = priv->irq_regnums[idx];
		irq_reg = priv->irq_reg_table + regnum;
		result  = priv->irq_scratch[idx];

		irq_posted = 0;

		while (0 != (bit = ffs(result))) {
			bit--;
			if (unlikely(!(irq_reg->mask & BIT(bit)))) {
				/* A bit fired that we don't know about? */
				generic_cic_pin_do_mask_ack(priv, irq_reg, bit);
				dev_err(dev, "%s: %s.%d not posted "
					"(result=0x%.8x mask=0x%.8x)\n",
					__func__, irq_reg->name, bit, result,
					irq_reg->mask);
			}
			else irq_posted |= BIT(bit);
			result &= ~BIT(bit);
		}

		dev_dbg(dev, "%-29s:%s posted=0x%.8x\n",
			__func__, irq_reg->name, irq_posted);

		(priv->cic_irqclr)(priv, irq_reg->intr, irq_posted,
				   irq_reg->name);
		priv->irq_scratch[idx] = irq_posted;
		idx++;
	}
	/* acknowledge the master interrupt */
	(priv->cic_irqclr)(priv, cic_master_sum(dev), info.master,
			   generic_cic_name_master);

	/* now post all the interrupts that fired */
	idx = 0;
	while (idx < regcnt) {
		regnum  = priv->irq_regnums[idx];
		irq_reg = priv->irq_reg_table + regnum;
		result  = priv->irq_scratch[idx];

		while (0 != (bit = ffs(result))) {
			bit--;
			hwirq = cic_regnum_bit_to_hwirq(priv, regnum, bit);
			pin   = priv->hwirq_to_pin[hwirq];

			out_irq = irq_find_mapping(priv->gc.irq.domain, hwirq);

			if (unlikely(CIC_NO_IRQ == out_irq)) {
				dev_err(dev, "%-29s:pin(%d) has no revmap\n",
					__func__, pin);
			}
			else {
				irq_reg->irq_seq[bit] = priv->irq_seq;
				trace_ciena_cic_out(generic_cic_name, out_irq,
						    cic_to_str(dev, pin,
							       priv->level),
						    irq_reg->mask,
						    irq_reg->name,
						    bit, result);
				handle_nested_irq(out_irq);
				priv->irq_out = ktime_get();
			}
			result &= ~BIT(bit);
		}
		idx++;
	}

	generic_cic_unlock(priv);

	/* re-enable parent interrupts now */
	enable_irq(irq);

	return (0 >= regcnt) ? IRQ_NONE : IRQ_HANDLED;
}

/* -------------------------------------------------------------------------- */
/* MMIO irq_chip functions. They all suppose that IRQ registers are
 * accessed with plain non-blocking read/write operations.
 */
static void generic_cic_ack(struct irq_data *out_data)
{
	struct gpio_chip        *gc    = irq_data_get_irq_chip_data(out_data);
	struct generic_cic_priv *priv  = to_ciena_generic_gpio_priv(gc);
	irq_hw_number_t          hwirq = irqd_to_hwirq(out_data);
	irq_reg_info_t          *reg;
	unsigned                 bit;

	if (unlikely(generic_find_hwirq(priv, hwirq, &reg, &bit)))
		dev_err(priv->dev, "%-24s:pin=%3lu bit not defined\n",
			__func__, hwirq);
	else {
		generic_cic_lock(priv);

		/* Write-to-clear the bit in the intr register */
		(priv->cic_irqclr)(priv, reg->intr, BIT(bit), reg->name);

		generic_cic_unlock(priv);

		dev_dbg(priv->dev, "%-24s:out_hwirq=%3lu mask=0x%08lx\n",
			__func__, hwirq, BIT(bit));
	}
}

static void generic_cic_pin_mask(struct generic_cic_priv *priv,
				 irq_hw_number_t          hwirq)
{
	irq_reg_info_t *reg;
	reg_t           mask;
	unsigned        bit;

	if (unlikely(generic_find_hwirq(priv, hwirq, &reg, &bit)))
		dev_err(priv->dev, "%-24s:pin=%3lu bit not defined\n",
			__func__, hwirq);
	else {
		generic_cic_lock(priv);

		/* Set the bit in the disable register */
		mask = ((reg->rd_mask)(priv, reg) | ~reg->mask | BIT(bit));
		(*reg->wr_mask)(priv, reg, mask);

		generic_cic_unlock(priv);

		dev_dbg(priv->dev, "%-24s:hwirq=%3lu mask=0x%08x\n",
			__func__, hwirq, mask);}
}

static void generic_cic_mask(struct irq_data *out_data)
{

	struct gpio_chip        *gc   = irq_data_get_irq_chip_data(out_data);
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);

	generic_cic_pin_mask(priv, irqd_to_hwirq(out_data));
}

static void generic_cic_pin_do_mask_ack(struct generic_cic_priv *priv,
					irq_reg_info_t *reg, unsigned bit)
{
	reg_t mask;

	/* Set the bit in the disable register */
	mask =((reg->rd_mask)(priv, reg) | ~reg->mask | BIT(bit));
	(reg->wr_mask)(priv, reg, mask);

	/* Write-to-clear the bit in the intr register */
	(priv->cic_irqclr)(priv, reg->intr, BIT(bit), reg->name);

	dev_dbg(priv->dev, "%-24s:%s mask=0x%08x\n",
		__func__, reg->name, mask);
}

static void generic_cic_pin_mask_ack(struct generic_cic_priv *priv,
				     irq_hw_number_t hwirq, bool do_lock)
{
	irq_reg_info_t *reg;
	unsigned        bit;

	if (unlikely(generic_find_hwirq(priv, hwirq, &reg, &bit)))
		dev_err(priv->dev, "%-24s:pin=%3lu bit not defined\n",
			__func__, hwirq);
	else {
		if (do_lock) generic_cic_lock(priv);

		generic_cic_pin_do_mask_ack(priv, reg, bit);

		if (do_lock) generic_cic_unlock(priv);
	}
}

static void generic_cic_mask_ack(struct irq_data *out_data)
{
	struct gpio_chip        *gc   = irq_data_get_irq_chip_data(out_data);
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);

	generic_cic_pin_mask_ack(priv, irqd_to_hwirq(out_data), true);
}

static void generic_cic_pin_unmask(struct generic_cic_priv *priv,
				   irq_hw_number_t          hwirq)
{
	irq_reg_info_t *reg;
	reg_t           mask;
	unsigned        bit;

	if (unlikely(generic_find_hwirq(priv, hwirq, &reg, &bit)))
		dev_err(priv->dev, "%-24s:hwirq=%3lu bit not defined\n",
			__func__, hwirq);
	else {
		generic_cic_lock(priv);

		/* Clear the bit in the disable register */
		mask = ((reg->rd_mask)(priv, reg) & ~(reg->mask & BIT(bit)));
		(reg->wr_mask)(priv, reg, mask);

		generic_cic_unlock(priv);

		dev_dbg(priv->dev, "%-24s:hwirq=%3lu mask=0x%08x\n",
			__func__, hwirq, mask);
	}
}

static void generic_cic_unmask(struct irq_data *out_data)
{
	struct gpio_chip        *gc   = irq_data_get_irq_chip_data(out_data);
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);

	if (priv)
		generic_cic_pin_unmask(priv, irqd_to_hwirq(out_data));
}

static void generic_cic_eoi(struct irq_data *out_data)
{
	struct gpio_chip        *gc   = irq_data_get_irq_chip_data(out_data);
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);

	dev_info(priv->dev, "%-24s:irq=%3u do nothing\n",
		 __func__, out_data->irq);
}

static unsigned int generic_cic_startup(struct irq_data *out_data)
{
	struct gpio_chip        *gc   = irq_data_get_irq_chip_data(out_data);
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);

	dev_dbg(priv->dev, "%-24s:out_virq=%u unmasking\n",
		__func__, out_data->irq);
	generic_cic_unmask(out_data);
	return 0;
}

static int generic_cic_set_type(struct irq_data *data, unsigned int type)
{
	struct gpio_chip        *gc    = irq_data_get_irq_chip_data(data);
	struct generic_cic_priv *priv  = to_ciena_generic_gpio_priv(gc);
	struct device           *dev   = priv->dev;
	irq_hw_number_t          hwirq = irqd_to_hwirq(data);
	int                      pin;
	void   (*handle_func)(struct irq_desc *desc);

	pin = priv->hwirq_to_pin[hwirq];

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
	case IRQ_TYPE_EDGE_FALLING:
	case IRQ_TYPE_EDGE_BOTH:
		handle_func = handle_edge_irq;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
	case IRQ_TYPE_LEVEL_LOW:
		handle_func = handle_level_irq;
		break;
	default:
		dev_err(dev, "%s: unexpected type 0x%08x\n",
			cic_to_str(dev, pin, priv->level), type);
		return -EINVAL;
	}

	irq_set_chip_handler_name_locked(data, &priv->ic, handle_func,
					 cic_to_str(dev, pin, priv->level));

	return IRQ_SET_MASK_OK;
}

static const struct irq_chip generic_cic_mmio_irq_chip = {
	.name         = " " MOD_NAME "-CIC ",
	.irq_startup  = generic_cic_startup,
	.irq_ack      = generic_cic_ack,
	.irq_mask     = generic_cic_mask,
	.irq_mask_ack = generic_cic_mask_ack,
	.irq_unmask   = generic_cic_unmask,
	.irq_eoi      = generic_cic_eoi,
	.irq_set_type = generic_cic_set_type,
	.flags        = IRQCHIP_IMMUTABLE,
};

/* -------------------------------------------------------------------------- */
/* Regmap irq_chip functions. All blocking IRQ register accesses must
 * happen inside generic_cic_bus_sync_unlock(), before releasing the
 * bus lock.
 */
static void generic_cic_bus_lock(struct irq_data *data)
{
	struct gpio_chip        *gc   = irq_data_get_irq_chip_data(data);
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);

	if (priv) generic_cic_lock(priv);
}

static void generic_cic_bus_sync_unlock(struct irq_data *data)
{
	struct gpio_chip        *gc   = irq_data_get_irq_chip_data(data);
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);
	irq_reg_info_t          *reg;
	int                      regnum;

	if (!priv) return;

	while (0 != (regnum = ffs(priv->irq_en_dirty))) {
		unsigned mask;

		reg = &priv->irq_reg_table[--regnum];

		priv->irq_en_dirty &= ~BIT(regnum);

		/* the mask and interrupt ack values are the inverse
		 * of the enabled bits */
		mask = ~(priv->irq_en_regs[regnum] & reg->mask);
		(reg->wr_mask)(priv, reg, mask);
		(priv->cic_irqclr)(priv, reg->intr, mask, reg->name);

		dev_dbg(priv->dev, "%-24s:regnum=%u mask=0x%08x\n",
			__func__, regnum, mask);
	}

	generic_cic_unlock(priv);
}

static void generic_cic_irq_change(struct irq_data *data, bool enable)
{
	struct gpio_chip        *gc     = irq_data_get_irq_chip_data(data);
	struct generic_cic_priv *priv   = to_ciena_generic_gpio_priv(gc);
	irq_hw_number_t          hwirq  = irqd_to_hwirq(data);
	unsigned long            regnum = 0;
	unsigned long            bit    = 0;
	int                      pin;

	if (!priv) return;

	pin = priv->hwirq_to_pin[hwirq];

	regnum = pin / REG_WIDTH;
	bit    = pin % REG_WIDTH;

	if (enable) priv->irq_en_regs[regnum] |= BIT(bit);
	else        priv->irq_en_regs[regnum] &= ~BIT(bit);

	priv->irq_en_dirty |= BIT(regnum);

	dev_dbg(priv->dev, "%-24s:pin=%d %s\n",
		__func__, pin, enable ? "enable" : "disable");
}

static void generic_cic_irq_disable(struct irq_data *data)
{
	generic_cic_irq_change(data, false);
}

static void generic_cic_irq_enable(struct irq_data *data)
{
	generic_cic_irq_change(data, true);
}

static const struct irq_chip generic_cic_regmap_irq_chip = {
	.name                = " " MOD_NAME "-CIC ",
	.irq_bus_lock        = generic_cic_bus_lock,
	.irq_bus_sync_unlock = generic_cic_bus_sync_unlock,
	.irq_disable         = generic_cic_irq_disable,
	.irq_enable          = generic_cic_irq_enable,
	.irq_set_type        = generic_cic_set_type,
	.flags               = IRQCHIP_IMMUTABLE,
};

/* -------------------------------------------------------------------------- */
static int generic_gpio_request(struct gpio_chip *gc, unsigned offset)
{
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);

	dev_dbg(priv->dev, "%s: gpio %d\n", __func__, offset);
	return 0;
}

static int generic_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);

	dev_dbg(priv->dev, "%s: gpio %d\n", __func__, offset);
	return 0;
}

static int generic_gpio_direction_output(struct gpio_chip *gc,
			unsigned offset, int value)
{
	struct generic_cic_priv *priv = to_ciena_generic_gpio_priv(gc);

	dev_dbg(priv->dev, "%s: gpio %d value=%d\n", __func__, offset, value);
	return EFAULT;
}

static int generic_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct generic_cic_priv *priv  = to_ciena_generic_gpio_priv(gc);
	int                      value = 0;
	irq_reg_info_t          *reg;
	unsigned                 bit;

	if (unlikely(generic_find_hwirq(priv, offset, &reg, &bit)))
		dev_err(priv->dev, "%-24s:hwirq=%3u bit not defined\n",
			__func__, offset);
	else {
		value = !!(generic_cic_read(priv, reg->status, reg->name,
					    generic_cic_suffix_stat) &
			   BIT(bit));
	        dev_dbg(priv->dev, "%-24s:hwirq=%d status is 0x%x \n",
			__func__, offset, value);
	}

	return value;
}

static void generic_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
	struct generic_cic_priv *priv  = to_ciena_generic_gpio_priv(gc);

	dev_dbg(priv->dev, "%s: gpio %d not set to %d\n",
		__func__, offset, value);
}

/* -------------------------------------------------------------------------- */
/* Override the platform device driver name.
*/
static int generic_cic_driver_override(struct platform_device *pdev,
				       const char *custom_name,
				       const char *use_driver)
{
	struct device *dev = &pdev->dev;
	int            rc;

	if (NULL == custom_name) return 0;

	rc = driver_set_override(dev, &pdev->driver_override, use_driver,
				 strlen(use_driver));
	if (rc) {
		dev_err(dev, "Cannot set driver override %s (%d)\n",
			use_driver, rc);
		return rc;
	}

	request_module(use_driver);

	rc = device_attach(dev);
	if (0 >= rc) {
		dev_err(dev, "cannot attach to %s (%d)\n", use_driver, rc);
		return (0 > rc) ? rc : -EINVAL;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
static int generic_gpiotest_sysfs_create(struct platform_device *pdev)
{
	struct generic_cic_priv            *priv       = platform_get_drvdata(pdev);
	int                                 level      = 0;
	int                                 index      = 0;
	struct platform_device            **pd;
	irq_reg_info_t                     *reg;
	unsigned                            bit;
	struct device                      *dev;
	irq_hw_number_t                     hwirq;
	struct ciena_sysfs_gpiotest_pdata   pdata;
	int                                 rc;

	dev_info(&pdev->dev, "%s\n", __func__);
	dev  = priv->dev;

	priv->gpiotest_sysfs = devm_kzalloc(dev,
					    priv->hwirq_max *
					    sizeof(*(priv->gpiotest_sysfs)),
					    GFP_KERNEL);

	if (NULL == priv->gpiotest_sysfs) {
		dev_err(dev, "no memory for gpiotest sysfs data\n");
		return -ENOMEM;
	}

	for (hwirq = 0; hwirq < priv->hwirq_max; hwirq++) {
		char dev_name[128];

		if (!generic_find_hwirq(priv,
				       	hwirq,
				       	&reg,
				       	&bit) && (reg->test)) {
			snprintf(dev_name, sizeof(dev_name), "gpiotest-%s",
				 cic_to_str(dev,
					    priv->hwirq_to_pin[hwirq],
					    level));
			memset(&pdata, 0, sizeof(pdata));
			pdata.regmap  = dev_get_regmap(dev->parent, NULL);
			pdata.reg     = reg->test;
			pdata.mask    = BIT(bit);

			dev_dbg(dev, "%s %x %x\n",
				dev_name,
				pdata.reg,
				pdata.mask);

			pd = priv->gpiotest_sysfs + index++;
			*pd = platform_device_register_data(dev,
							    dev_name,
							    PLATFORM_DEVID_NONE,
							    &pdata,
							    sizeof(pdata));
			if (NULL == *pd) {
				dev_err(dev, "cannot register %s gpiotest_sysfs\n",
					dev_name);
				return -1;
			}

			rc = generic_cic_driver_override(*pd, dev_name,
							 GPIOTEST_SYSFS_DRIVER_NAME);
			if (rc) return rc;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
static int generic_gpiotest_sysfs_destroy(struct platform_device *pdev)
{
	struct device           *dev   = &pdev->dev;
	struct generic_cic_priv *priv;
	irq_hw_number_t          hwirq;
	int                      index = 0;

	dev_info(dev, "%s\n", __func__);

	priv = platform_get_drvdata(pdev); /* from platform_set_drvdata() */

	if (NULL == priv->gpiotest_sysfs)
		return 0;

	for (hwirq = 0; hwirq < priv->hwirq_max; hwirq++) {
		if (priv->gpiotest_sysfs[index]) {
			dev_dbg(dev, "%04ld %s\n", hwirq,
				cic_to_str(dev,
					   priv->hwirq_to_pin[hwirq], 0));
			platform_device_del(priv->gpiotest_sysfs[index]);
			index++;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
int CIENA_GPIO_EXPORT_FUNC(struct platform_device *pdev)
{
	struct generic_cic_priv    *priv;
	irq_reg_info_t             *reg;
	unsigned                    bit;
	ssize_t                     lkup_size;
	struct device              *dev;
	struct gpiod_lookup_table  *lkup_tbl   = NULL;
	struct gpiod_lookup        *lkup       = NULL;
	struct gpio_desc          **desc;
	irq_hw_number_t             hwirq;
	const char                 *label;
	unsigned long               flags;
	enum cic_gpio_fate          fate;
	int                         lkup_count = 1;
	int                         desc_count = 0;
	int                         level      = 0;
	int                         pin;
	int                         rc;

	dev_info(&pdev->dev, "%s\n", __func__);

	priv = platform_get_drvdata(pdev); /* from platform_set_drvdata() */
	dev  = priv->dev;

	/* Create a lookup table spanning every gpio under this cic. */
	for (pin = 0; pin < priv->irq_sources_max ; pin++) {
		hwirq = priv->pin_to_hwirq[pin];
		if ((0 == generic_find_hwirq(priv, hwirq, &reg, &bit)) &&
		    (NULL != cic_to_str(dev, pin, level))) {
			fate = cic_gpio_lkup(dev, pin, level);
			/* skip the dropped pins */
			if (cic_gpio_drop == fate) continue;
			/* all non-dropped gpios have a lookup */
			lkup_count++;
			/* but no internal descriptor for gpios that
			 * remain in the kernel */
			desc_count += (cic_gpio_gpio != fate);
		}
	}

	if (1 >= lkup_count) {
		dev_warn(&pdev->dev, "there are no gpios under this cic\n");
		return 0;
	}

	lkup_size = (sizeof(*lkup_tbl) + lkup_count * sizeof(*lkup) +
		     desc_count * sizeof(*desc));

	lkup_tbl  = devm_kzalloc(&pdev->dev, lkup_size, GFP_KERNEL);
	if (NULL == lkup_tbl) {
		dev_err(&pdev->dev, "no memory for gpio lookups\n");
		return -ENOMEM;
	}

	lkup_tbl->dev_id = dev_name(&pdev->dev);
	lkup             = lkup_tbl->table;
	desc             = (struct gpio_desc **) (lkup + lkup_count);
	priv->gdescs     = desc;

	/* Now populate the gpio lookup table. */
	for (pin = 0; pin < priv->irq_sources_max ; pin++) {
		fate  = cic_gpio_lkup(dev, pin, level);
		hwirq = priv->pin_to_hwirq[pin];
		label = cic_to_str(dev, pin, level);
		flags = (cic_gpio_active_low(dev, pin, level) ?
			 GPIO_ACTIVE_LOW : GPIO_ACTIVE_HIGH);

		if (generic_find_hwirq(priv, hwirq, &reg, &bit) || !label ||
		    (cic_gpio_drop == fate))
			continue;

		lkup->key        = dev_name(&pdev->dev);
		lkup->chip_hwnum = hwirq;
		lkup->con_id     = label;
		lkup->flags      = flags;

		lkup++;
	}

	gpiod_add_lookup_table(lkup_tbl);
	priv->gpio_lkup_table = lkup_tbl;
	dev_info(&pdev->dev, "gpio lookup table with %d entr%s\n",
		 lkup_count - 1, (2 < lkup_count) ? "ies" : "y");

	rc = generic_gpiotest_sysfs_create(pdev);
	if (rc) {
		dev_err(priv->dev, "cannot create gpiotest (%d)\n", rc);
		return rc;
	}

	/* Enable the nodes in /sys/class/gpio for userland. */
	for (pin = 0; pin < priv->irq_sources_max ; pin++) {
		struct cic_gpio_export exp = { };

		hwirq = priv->pin_to_hwirq[pin];
		label = cic_to_str(dev, pin, level);

		if (generic_find_hwirq(priv, hwirq, &reg, &bit) || !label)
			continue;

		switch (cic_gpio_lkup(dev, pin, level)) {
		case cic_gpio_drop:
		case cic_gpio_gpio:
			continue;
		case cic_gpio_irq:
			exp.internal = true;
			break;
		case cic_gpio_export:
		default:
			break;
		}

		exp.dev   = &pdev->dev;
		exp.label = label;

		if (cic_gpio_do_export(&exp)) {
			*desc = exp.desc;
			desc++;
			priv->num_gdescs++;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(CIENA_GPIO_EXPORT_FUNC);

/* -------------------------------------------------------------------------- */
int CIENA_GPIO_UNEXPORT_FUNC(struct platform_device *pdev)
{
	struct device           *dev   = &pdev->dev;
	struct generic_cic_priv *priv;
	int                      i;

	dev_info(dev, "%s\n", __func__);

	priv = platform_get_drvdata(pdev); /* from platform_set_drvdata() */

	/*
	 * Iterate only the descriptors that were successfully exported.
	 */
	if (priv->gdescs) {
		for (i = 0; i < priv->num_gdescs; i++) {
			struct cic_gpio_export exp = { };

			exp.dev  = &pdev->dev;
			exp.desc = priv->gdescs[i];

			cic_gpio_do_unexport(&exp);
		}
		priv->num_gdescs = 0;
	}

	generic_gpiotest_sysfs_destroy(pdev);

	if (priv->gpio_lkup_table)
		gpiod_remove_lookup_table(priv->gpio_lkup_table);

	return 0;
}
EXPORT_SYMBOL_GPL(CIENA_GPIO_UNEXPORT_FUNC);

/* -------------------------------------------------------------------------- */
/*  GPIO exported to sysfs
 */
static int ciena_gpio_create(struct platform_device *pdev)
{
	struct platform_device *pd;
	struct generic_cic_priv *priv  = platform_get_drvdata(pdev); /* from platform_set_drvdata() */

	pd = platform_device_register_resndata(&pdev->dev,
					       GPIO_NAME,
					       PLATFORM_DEVID_NONE,
					       NULL, 0,
					       NULL, 0);
	if (!pd) {
		dev_err(&pdev->dev, "failed to register device\n");
		return -1;
	}

	priv->gpio = pd;

	return 0;
}

/* -------------------------------------------------------------------------- */
static void ciena_gpio_destroy(struct platform_device *pdev)
{
	struct generic_cic_priv *priv  = platform_get_drvdata(pdev); /* from platform_set_drvdata() */

	if (priv->gpio)
		platform_device_del(priv->gpio);
}

/* -------------------------------------------------------------------------- */
static unsigned int generic_cic_of_irq(struct device_node *np)
{
	int virq = of_irq_get(np, 0);

	if (CIC_NO_IRQ < virq) return virq;

	return CIC_NO_IRQ;
}

/* -------------------------------------------------------------------------- */
static unsigned int generic_cic_parent_msi(struct generic_cic_priv *priv,
					   struct platform_device *pdev)
{
	unsigned int msi_irq = CIC_NO_IRQ;
#ifdef CONFIG_PCI_MSI
	struct device *parent;
	struct pci_dev *pparent = NULL;
	int rc;

	for (parent = pdev->dev.parent; parent; parent = parent->parent) {
		if (dev_is_pci(parent)) {
			pparent = to_pci_dev(parent);
			if (pci_is_pcie(pparent)) {
				pparent = pci_dev_get(pparent);
				break;
			}
		}
	}

	if (NULL == pparent) {
		dev_warn(&pdev->dev, "cannot find PCI parent device\n");
		return CIC_NO_IRQ;
	}

	rc = pci_enable_msi(pparent);
	if (rc) {
		dev_warn(&pdev->dev, "cannot enable MSI on parent %s (%d)\n",
			 dev_name(&pparent->dev), rc);
		pci_dev_put(pparent);
		return CIC_NO_IRQ;
	}

	msi_irq = pparent->irq;
	if (CIC_NO_IRQ == msi_irq) pci_dev_put(pparent);
	else {
		pci_set_master(pparent);
		priv->msi_parent = pparent;
	}
#endif
	dev_dbg(&pdev->dev, "msi irq=%u msi_parent=%s\n", msi_irq,
		priv->msi_parent ? dev_name(&priv->msi_parent->dev) : "none");

	return msi_irq;
}

/* -------------------------------------------------------------------------- */
static int generic_cic_gpio_irq(struct platform_device *pdev,
				struct generic_cic_priv *priv)
{
	struct device *gpio_dev = priv->config.parent_irq.dev;
	const char *pin = priv->config.parent_irq.pin;
	struct gpio_desc *gd;
	int gpioflags = GPIOD_ASIS | GPIOD_FLAGS_BIT_NONEXCLUSIVE;
	int rc = 0;
	int irq;

	gd = gpiod_get(gpio_dev, pin, gpioflags);
	if (IS_ERR_OR_NULL(gd)) {
		dev_dbg(&pdev->dev, "cannot get %s/%s (%ld)\n",
			dev_name(gpio_dev), pin, PTR_ERR(gd));
		return -EPROBE_DEFER;
	}

	irq = gpiod_to_irq(gd);
	if (0 < irq) {
		priv->in_virq = irq;
		dev_info(&pdev->dev, "using source irq %d (%s/%s)\n",
			 irq, dev_name(gpio_dev), pin);
	}
	else {
		dev_warn(&pdev->dev, "cannot get irq %s/%s (%d)\n",
			 dev_name(gpio_dev), pin, irq);
		rc = irq;
	}

	/*
	 * Do NOT call gpiod_put() here.  The NONEXCLUSIVE flag causes the
	 * kernel to return an already-requested descriptor WITHOUT taking a
	 * new module reference (kernel "hack" in gpiod_find_and_request).
	 * Calling gpiod_put() would call module_put() on the chip owner
	 * without a matching module_get(), underflowing the refcount and
	 * crashing on shutdown.  The descriptor is owned by the CIC export
	 * path and will be properly released by europa_gpio_unexport().
	 */

	return rc;
}

/* -------------------------------------------------------------------------- */
static void generic_cic_config_regmap(struct platform_device *pdev,
				      struct generic_cic_priv *priv)
{
	priv->cic_lock   = generic_cic_mutex_lock;
	priv->cic_unlock = generic_cic_mutex_unlock;

	priv->cic_rd = generic_cic_read_regmap;
	priv->cic_wr = generic_cic_write_regmap;

	if (priv->config.clear_to_clear)
		priv->cic_irqclr = generic_cic_irq_clr2clr_regmap;
	else
		priv->cic_irqclr = generic_cic_irq_set2clr_regmap;
}

/* -------------------------------------------------------------------------- */
static void generic_cic_config_mmio(struct platform_device *pdev,
				    struct generic_cic_priv *priv)
{
	struct device      *dev = &pdev->dev;
	struct device_node *np  = dev->of_node;
	struct resource    *mem = NULL;
	struct resource     res;

	/* Get memory mapped resource. */
	if (np && (0 == of_address_to_resource(np, 0, &res))) mem = &res;
	if (NULL == mem)
		mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	BUG_ON(mem == NULL);
	priv->iobase = devm_ioremap(dev, mem->start, resource_size(mem));
	BUG_ON(priv->iobase == NULL);
	dev_dbg(dev, "%pR, iobase=%p\n", mem, priv->iobase);

	priv->cic_lock   = generic_cic_spin_lock;
	priv->cic_unlock = generic_cic_spin_unlock;

	priv->cic_rd = generic_cic_read_mmio;
	priv->cic_wr = generic_cic_write_mmio;

	if (priv->config.clear_to_clear)
		priv->cic_irqclr = generic_cic_irq_clr2clr_mmio;
	else
		priv->cic_irqclr = generic_cic_irq_set2clr_mmio;


#ifdef CONFIG_CIENA_MCEE
	ciena_mcee_dev_init(pdev, priv->iobase, resource_size(mem),
			    &priv->mceed);
#endif
}

/* -------------------------------------------------------------------------- */
static int generic_cic_regmap_irq(struct generic_cic_priv *priv)
{
	unsigned        nregs    = priv->nregs;
	size_t          regsize  = nregs * sizeof(*priv->irq_en_regs);
	int             rc;

	if (!priv->parent_regmap) return 0;

	/* If the REG_WIDTH does not match the regmap stride,
	 * something is very wrong. */
	BUG_ON((REG_WIDTH / 8) != regmap_get_reg_stride(priv->parent_regmap));

	priv->irq_en_dirty = ~0UL;
	if (0 == (priv->irq_en_dirty >> nregs)) {
		dev_err(priv->dev, "not enough bits in priv->irq_en_dirty "
			"(nregs=%u)\n", nregs);
		return -EINVAL;
	}
	priv->irq_en_dirty = 0;

	priv->irq_regnums = devm_kzalloc(priv->dev, regsize, GFP_KERNEL);
	priv->irq_en_regs = devm_kzalloc(priv->dev, regsize, GFP_KERNEL);
	priv->irq_scratch = devm_kzalloc(priv->dev, regsize, GFP_KERNEL);
	if (!priv->irq_regnums || !priv->irq_en_regs || !priv->irq_scratch) {
		dev_err(priv->dev, "no memory for irq register cache\n");
		return -ENOMEM;
	}

	rc = request_threaded_irq(priv->in_virq, generic_cic_irq_handler,
				  generic_cic_irq_thread, IRQF_ONESHOT,
				  priv->gc.irq.chip->name, priv);
	if (rc) {
		dev_err(priv->dev, "cannot request threaded irq (%d)\n", rc);
		return rc;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
static void generic_cic_deregmap_irq(struct generic_cic_priv *priv)
{
	if (priv->parent_regmap) {
		free_irq(priv->in_virq, priv);
	}
}

/* -------------------------------------------------------------------------- */
static int generic_cic_create_xlate_tables(struct generic_cic_priv *priv)
{
	int             pin;
	irq_hw_number_t hwirq;
	size_t          sz;

	for (pin = 0; pin < priv->irq_sources_max ; pin++)
		if (cic_to_str(priv->dev, pin, priv->level))
			priv->hwirq_max++;

	sz = priv->irq_sources_max * sizeof(*priv->pin_to_hwirq);
	priv->pin_to_hwirq = devm_kzalloc(priv->dev, sz, GFP_KERNEL);

	sz = priv->hwirq_max * sizeof(*priv->hwirq_to_pin);
	priv->hwirq_to_pin = devm_kzalloc(priv->dev, sz, GFP_KERNEL);

	sz = priv->hwirq_max * sizeof(*priv->gpio_labels);
	priv->gpio_labels = devm_kzalloc(priv->dev, sz, GFP_KERNEL);

	if (NULL == priv->hwirq_to_pin ||
	    NULL == priv->pin_to_hwirq ||
	    NULL == priv->gpio_labels) {
		dev_err(priv->dev, "no memory for tables\n");
		return -ENOMEM;
	}

	/* Populate pin_to_hwirq and hwirq_to_pin*/
	hwirq = 0;
	for (pin = 0; pin < priv->irq_sources_max ; pin++) {
		if (cic_to_str(priv->dev, pin, priv->level)) {
			priv->pin_to_hwirq[pin]     = hwirq;
			priv->hwirq_to_pin[hwirq++] = pin;
		} else
			priv->pin_to_hwirq[pin] = -1;
	}

	/* Populate gpio_labels */
	for (hwirq = 0; hwirq < priv->hwirq_max ; hwirq++) {
		priv->gpio_labels[hwirq] =
			cic_to_str(priv->dev,
				   priv->hwirq_to_pin[hwirq],
				   priv->level);
		dev_dbg(priv->dev, "%-29s: pin= %3d hwirq=%3lu %s\n",
			__func__,
			priv->hwirq_to_pin[hwirq],
			hwirq,
			priv->gpio_labels[hwirq]);
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/* device files are limited to one page in size */
#define CIC_SUM_PR(_b, _p, _fmt, _args...)				\
	(_b ?								\
	 snprintf(_b + _p,						\
		  _p > (PAGE_SIZE - 1) ? 0 : PAGE_SIZE - 1 - _p,	\
		  _fmt, ## _args) :					\
	 pr_info(_fmt, ## _args))

#ifdef CIENA_HAS_CIC_IRQ_SUMMARY
static ssize_t generic_cic_msi_summary(struct generic_cic_priv *priv,
				       char                    *buf,
				       ssize_t                  pos)
{
	reg_t msic;
	s64   kt_usec;

	if (!cic_msi_ctrl(priv->dev)) return pos;

	msic = generic_cic_read(priv, cic_msi_ctrl(priv->dev),
				generic_cic_name_msi, NULL);

	pos += CIC_SUM_PR(buf, pos, "%s: 0x%x\n", generic_cic_name_msi, msic);

	kt_usec = ktime_to_ns(priv->msi_in) / 1000;
	pos += CIC_SUM_PR(buf, pos, "msi_in: %lld.%06lld\n",
			  kt_usec / 1000000, kt_usec % 1000000);

	kt_usec = ktime_to_ns(priv->msi_out) / 1000;
	pos += CIC_SUM_PR(buf, pos, "msi_out: %lld.%06lld\n",
			  kt_usec / 1000000, kt_usec % 1000000);

	return pos;
}

static ssize_t generic_cic_reg_summary(struct generic_cic_priv *priv,
				       irq_reg_info_t          *ireg,
				       char                    *buf,
				       ssize_t                  pos)
{
	struct irq_desc *irqd;
	unsigned     	 regnum = ireg - priv->irq_reg_table;
	unsigned     	 virq;
	unsigned     	 bit;
	reg_t        	 mask;
	reg_t        	 stat;
	reg_t        	 req;
	int     	 hwirq;

	mask = (ireg->rd_mask)(priv, ireg);
	req  = generic_cic_read(priv, ireg->intr, ireg->name,
				generic_cic_suffix_irq);
	stat = generic_cic_read(priv, ireg->status, ireg->name,
				generic_cic_suffix_stat);

	pos += CIC_SUM_PR(buf, pos, "%s: 0x%x 0x%x 0x%x\n",
			  ireg->name, req, mask, stat);

	for (bit = 0; bit < REG_WIDTH; bit++) {
		hwirq = cic_regnum_bit_to_hwirq(priv, regnum, bit);
		if (0 > hwirq) continue;

		virq = irq_find_mapping(priv->gc.irq.domain, hwirq);

		if (CIC_NO_IRQ == virq) continue;

		irqd = irq_to_desc(virq);
		if (NULL == irqd) continue;

		/* skip the interrupts that never happened, and that
		 * are not posted */
		if (!irqd->tot_count && !(req & BIT(bit))) continue;

		pos += CIC_SUM_PR(buf, pos, " %s: %s%s%s %u %u %u\n",
				  cic_to_str(priv->dev,
					     (regnum * REG_WIDTH) + bit,
					     priv->level),
				  req & BIT(bit) ? "R" : "-",
				  mask & BIT(bit) ? "M" : "-",
				  stat & BIT(bit) ? "S" : "-",
				  irqd->tot_count,
				  ireg->irq_seq[bit],
				  ireg->irq_count[bit]);
	}

	return (pos >= PAGE_SIZE) ? (PAGE_SIZE - 1) : pos;
}

static ssize_t generic_cic_summary(struct generic_cic_priv *priv,
				   char                    *buf)
{
	irq_reg_info_t *ireg;
	ssize_t         pos  = 0;
	reg_t           reg;
	s64             kt_usec;

	pos += CIC_SUM_PR(buf, pos, "%s: interrupt summary\n",
			  dev_name(priv->dev));

	if (priv->master_present) {
		reg = generic_cic_read(priv, cic_master_sum(priv->dev),
				       generic_cic_name_master,
				       generic_cic_suffix_irq);

		pos += CIC_SUM_PR(buf, pos, "%s_%s: 0x%x\n",
				  generic_cic_name_master,
				  generic_cic_suffix_irq, reg);

		if (cic_master_mask(priv->dev)) {
			reg = generic_cic_read(priv, cic_master_mask(priv->dev),
					       generic_cic_name_master,
					       generic_cic_suffix_mask);

			pos += CIC_SUM_PR(buf, pos, "%s_%s: 0x%x\n",
					  generic_cic_name_master,
					  generic_cic_suffix_mask, reg);
		}
	}

	pos = generic_cic_msi_summary(priv, buf, pos);

	pos += CIC_SUM_PR(buf, pos, "global sequence: %u\n", priv->irq_seq);

	kt_usec = ktime_to_ns(priv->irq_out) / 1000;
	pos += CIC_SUM_PR(buf, pos, "irq_out: %lld.%06lld\n",
			  kt_usec / 1000000, kt_usec % 1000000);


	for_each_irq_reg(priv, ireg)
		pos = generic_cic_reg_summary(priv, ireg, buf, pos);

	return pos;
}
#endif /* CIENA_HAS_CIC_IRQ_SUMMARY */
/* -------------------------------------------------------------------------- */
static bool generic_cic_dumper_function(void *data, int irq)
{
	struct generic_cic_priv *priv = data;

#ifdef CIENA_HAS_CIC_IRQ_SUMMARY
	struct irq_chip         *irqc = NULL;
	struct irq_desc         *irqd;
	uint32_t                 msic;
	/* skip if the interrupt is specified, and not for us */
	if (irq) {
		irqd = irq_to_desc(irq);
		if (irqd) irqc = irq_desc_get_chip(irqd);
		if (irqc != &priv->ic) return false;
	}

	(void) generic_cic_summary(priv, NULL);

	/* If the cic_panic is disabled, and MSIs are disabled, then
	 * the system has fallen into a rare, weird and yet
	 * unexplained (en-17.1.0) MSI stall rut.
	 *
	 * Re-enable the MSIs and get on with it. It is better than
	 * let the system sit there forever with broken interrupts.
	 */
	if (!ciena_cic_panic_enable) {
		msic = cic_msi_ctrl(priv->dev);
		if ((msic) && (0 == generic_cic_read(priv, msic,
						     generic_cic_name_msi,
						     NULL))) {
			dev_warn(priv->dev, "MSI disabled, re-enabling\n");
			generic_cic_write(priv, msic, ~0U,
					  generic_cic_name_msi, NULL);
		}
	}

	return ciena_cic_panic_enable;
#else
	dev_warn(priv->dev, "CIC IRQ summary not available (irq_to_desc)\n");
	return false;
#endif /* CIENA_HAS_CIC_IRQ_SUMMARY */
}

/* -------------------------------------------------------------------------- */
static ssize_t generic_cic_summary_show(struct device           *dev,
					struct device_attribute *attr,
					char                    *buf)
{
#ifdef CIENA_HAS_CIC_IRQ_SUMMARY
	struct platform_device  *pdev = to_platform_device(dev);
	struct generic_cic_priv *priv = platform_get_drvdata(pdev);

	return generic_cic_summary(priv, buf);
#else
	return 0;
#endif
}
static DEVICE_ATTR(cic_summary, 0444, generic_cic_summary_show, NULL);

/* -------------------------------------------------------------------------- */
static int generic_cic_platform_probe(struct platform_device *pdev)
{
	struct generic_cic_priv *priv;
	struct gpio_irq_chip    *girq;
	bool                     gpio_can_sleep;
	bool                     irq_threaded;
	unsigned                *parent_virq;
	unsigned                 num_parent_irqs;
	irq_flow_handler_t       irq_parent_handler;
	irq_reg_info_t          *ignore_table;
	irq_reg_info_t          *irq_reg;
	irq_level_t             *irq_interrupt_table;
	int                      rc;
	int                      source;
	int                      level  = 0;
	struct device           *dev    = &pdev->dev;
	struct device_node      *np     = dev->of_node;
	void                    *pdata  = dev_get_platdata(dev);

	irq_interrupt_table = cic_interrupt_table(dev);
	if (!irq_interrupt_table)
		return -ENODEV;
	if (IS_ERR(irq_interrupt_table))
		return (int) -PTR_ERR(irq_interrupt_table);

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(dev, "Unable to allocate priv\n");
		return -ENOMEM;
	}

	if (pdata)
		priv->config = *(struct generic_cic_config *) pdata;
	if (pdev->dev.of_node)
		generic_cic_get_of_config(pdev->dev.of_node, &priv->config);

	/* Get irq resource. */
	if (priv->config.parent_irq.dev && priv->config.parent_irq.pin) {
		rc = generic_cic_gpio_irq(pdev, priv);
		if (rc) goto out_clear_dev_data;
	}
	if ((CIC_NO_IRQ == priv->in_virq) && priv->config.use_parent_msi)
		priv->in_virq = generic_cic_parent_msi(priv, pdev);
	/* no irq yet? look in the device tree or platform data */
	if ((CIC_NO_IRQ == priv->in_virq) && np)
		priv->in_virq = generic_cic_of_irq(np);
	if (CIC_NO_IRQ == priv->in_virq)
		priv->in_virq = platform_get_irq(pdev, 0);
	dev_dbg(dev, "Got irq %d\n", priv->in_virq);

	/* sanity check: everyone should be using a dynamic GPIO range */
	if (0 <= priv->config.gpio_base) {
		dev_info(dev, "%d means dynamic gpio base\n",
			 priv->config.gpio_base);
		priv->config.gpio_base = -1;
	}

	if (priv->config.parent_regmap) {
		priv->parent_regmap = priv->config.parent_regmap;
		generic_cic_config_regmap(pdev, priv);

		gpio_can_sleep     = true;
		irq_threaded       = true;
		irq_parent_handler = NULL;
		parent_virq        = NULL;
		num_parent_irqs    = 0;
	} else {
		generic_cic_config_mmio(pdev, priv);

		gpio_can_sleep     = false;
		irq_threaded       = false;
		irq_parent_handler = generic_cic_cascade;
		parent_virq        = &priv->in_virq;
		num_parent_irqs    = 1;
	}

	priv->dev             = dev;
	priv->reg_base        = priv->config.start_offset;
	priv->master_present  = 1;
	priv->level           = level;
	priv->irq_sources_max = irq_interrupt_table[level].irq_sources_max;
	priv->irq_reg_table   = irq_interrupt_table[level].irq_reg_table;

	if (irq_interrupt_table[level].irq_test_reg)
		priv->irq_test_reg = *(irq_interrupt_table[level].irq_test_reg);

	/* Count the number of ISR registers one time */
	for_each_irq_reg(priv, irq_reg)
		priv->nregs++;
	dev_info(dev, "nregs=%d, REG_WIDTH=%d\n",
		 priv->nregs, REG_WIDTH);

	rc = generic_cic_create_xlate_tables(priv);
	if (rc) goto out_clear_dev_data;

	spin_lock_init(&priv->generic_cic_lock);
	rt_mutex_init(&priv->generic_cic_mutex);

	/* gpio_chip */
	priv->gc.label            = dev_name(dev);
	priv->gc.owner            = THIS_MODULE;
	priv->gc.parent           = dev;
	priv->gc.request          = generic_gpio_request;
	priv->gc.direction_input  = generic_gpio_direction_input;
	priv->gc.direction_output = generic_gpio_direction_output;
	priv->gc.get              = generic_gpio_get;
	priv->gc.set              = generic_gpio_set;
	priv->gc.base             = priv->config.gpio_base;
	priv->gc.ngpio            = priv->hwirq_max;
	priv->gc.can_sleep        = gpio_can_sleep;
	priv->gc.names            = priv->gpio_labels;

	/* irq_chip */
	priv->ic = (priv->parent_regmap ?
		    generic_cic_regmap_irq_chip :
		    generic_cic_mmio_irq_chip);

	/* gpio_irq_chip */
	girq                 = &priv->gc.irq;
	girq->chip           = &priv->ic;
	girq->default_type   = IRQ_TYPE_NONE;
	girq->handler        = handle_edge_irq;
	girq->threaded       = irq_threaded;
	girq->num_parents    = num_parent_irqs;
	girq->parents        = parent_virq;
	girq->parent_handler = irq_parent_handler;

	platform_set_drvdata(pdev, priv);  /* req'd by generic_cic_platform_remove() */

	generic_cic_lock(priv);

	for_each_irq_reg(priv, irq_reg) {
		if (irq_reg->disable) {
			irq_reg->rd_mask = generic_cic_rd_disable_mask;
			irq_reg->wr_mask = generic_cic_wr_disable_mask;
		}
		else if (irq_reg->enable) {
			irq_reg->rd_mask = generic_cic_rd_enable_mask;
			irq_reg->wr_mask = generic_cic_wr_enable_mask;
		}
		else {
			generic_cic_unlock(priv);
			dev_err(dev, "%s has neither disable nor "
				" enable register\n", irq_reg->name);
			goto out_mutex_destroy;
		}
		(*irq_reg->wr_mask)(priv, irq_reg, ~0);
		(priv->cic_irqclr)(priv, irq_reg->intr, irq_reg->mask,
				   irq_reg->name);
	}

	ignore_table = irq_interrupt_table[level].irq_reg_ignore_table;

	for (source = 0;
	     priv->master_present && ignore_table[source].mask != 0;
	     source++) {
		/* permanently enable summary interrupts, else no interrupts occur */
		u32 value = ~0;

		/* Choose method to unmask interrupt parent entities */
		if (irq_interrupt_table[level].use_mask_in_ignore)
			/* Use the .mask values in irq_reg_ignore_table to unmask interrupt tree */
			value = ~ignore_table[source].mask;
		else
			/* Assumption: there is only one master parent, so combine all the
			 * .master values from the irq_reg_table together */
			for_each_irq_reg(priv, irq_reg)
				value &= ~irq_reg->master;

		generic_cic_write(priv, ignore_table[source].disable, value,
				  ignore_table[source].name,
				  generic_cic_suffix_mask);
		generic_cic_write(priv, ignore_table[source].intr, ~0,
				  ignore_table[source].name,
				  generic_cic_suffix_irq);
	}

	generic_cic_unlock(priv);

	rc = generic_cic_regmap_irq(priv);
	if (rc) goto out_mutex_destroy;

	rc = gpiochip_add_data(&priv->gc, priv);
	if (rc) {
		dev_err(dev, "gpiochip_add error %d\n", rc);
		goto out_deregmap_irq;
	}

	rc = ciena_gpio_create(pdev);
	if (rc) {
		dev_err(dev, "could not create gpio exports%d\n", rc);
		goto out_gpiochip_remove;
	}

	rc = device_create_file(&pdev->dev, &dev_attr_cic_summary);
	if (rc) {
		dev_err(&pdev->dev, "cannot create cic_summary: %d\n", rc);
		goto out_gpio_destroy;
	}

	priv->ccd.dump_fn = generic_cic_dumper_function;
	priv->ccd.data    = priv;
	ciena_cic_register_dumper(&priv->ccd);

	/* Allow MSI Interrupts.
	 * Use the longest interval possible to avoid pile-ups. */
	if (cic_msi_ctrl(dev))
		generic_cic_write(priv, cic_msi_ctrl(dev), ~0U,
				  generic_cic_name_msi, NULL);

	dev_info(dev, "Set up Cascaded Interrupt Controller on irq %d\n",
		 priv->in_virq);

	return 0;

out_gpio_destroy:
	ciena_gpio_destroy(pdev);
out_gpiochip_remove:
	gpiochip_remove(&priv->gc);
out_deregmap_irq:
	generic_cic_deregmap_irq(priv);
out_mutex_destroy:
#ifdef CONFIG_CIENA_MCEE
	ciena_mcee_dev_deinit(&priv->mceed);
#endif
	if (priv->msi_parent) {
		pci_clear_master(priv->msi_parent);
		pci_disable_msi(priv->msi_parent);
		pci_dev_put(priv->msi_parent);
	}
out_clear_dev_data:
	platform_set_drvdata(pdev, NULL);
	return rc;
}

/* -------------------------------------------------------------------------- */
static void generic_cic_platform_remove(struct platform_device *pdev)
{
	struct generic_cic_priv *priv  = platform_get_drvdata(pdev); /* from platform_set_drvdata() */

	dev_info(&pdev->dev, "Removing driver %s\n", __func__);
	ciena_cic_deregister_dumper(&priv->ccd);
	device_remove_file(&pdev->dev, &dev_attr_cic_summary);
	ciena_gpio_destroy(pdev);
	gpiochip_remove(&priv->gc);
	generic_cic_deregmap_irq(priv);
#ifdef CONFIG_CIENA_MCEE
	ciena_mcee_dev_deinit(&priv->mceed);
#endif
	if (priv->msi_parent) {
		pci_clear_master(priv->msi_parent);
		pci_disable_msi(priv->msi_parent);
		pci_dev_put(priv->msi_parent);
	}
	platform_set_drvdata(pdev, NULL);
}

#ifdef CONFIG_OF
/* ------------------------------------------------------------------------- */
static const struct of_device_id generic_cic_of_match[] = {
	{ .compatible = "ciena," MOD_NAME "-cic", },
	{},
};
#ifndef CIENA_CIC_DRIVER
MODULE_DEVICE_TABLE(of, generic_cic_of_match);
#endif
#endif

/* -------------------------------------------------------------------------- */
static const struct platform_device_id generic_cic_platform_ids[] = {
	{.name = DEVICE_NAME},
	{},
};
#ifndef CIENA_CIC_DRIVER
#define CIENA_CIC_MODULE_EXPORT
MODULE_DEVICE_TABLE(platform, generic_cic_platform_ids);
#endif

/* -------------------------------------------------------------------------- */
#ifndef CIENA_CIC_DRIVER
#define CIENA_CIC_DRIVER generic_cic_driver
static
#endif
struct platform_driver CIENA_CIC_DRIVER = {
	.driver = {
		.name           = DRIVER_NAME,
		.owner          = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(generic_cic_of_match),
#endif
	},
	.probe    = generic_cic_platform_probe,
	.remove   = generic_cic_platform_remove,
	.id_table = generic_cic_platform_ids,
};

#ifdef CIENA_CIC_MODULE_EXPORT

module_platform_driver(generic_cic_driver);

MODULE_DESCRIPTION("Cascading interrupt driver (" MOD_NAME ")");
MODULE_AUTHOR("Ron Belaire <rbelaire@ciena.com>");
MODULE_LICENSE("GPL v2");

#endif /* CIENA_CIC_MODULE_EXPORT */

#endif /* _GENERIC_CIC_PRIV_H */
// vim: sw=8 noet
