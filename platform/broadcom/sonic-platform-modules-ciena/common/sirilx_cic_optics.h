#ifndef __SIRILX_CIC_OPTICS_H
#define __SIRILX_CIC_OPTICS_H

#include <linux/device.h>
#include "generic_cic.h"

#define foreach_cic_reg(__reg)					\
	for (__reg = 0; OPTICS_CIC_NREGS > __reg; __reg++)

#define foreach_cic_mod(__mod)					\
	for (__mod = 0; OPTICS_CIC_NMODS > __mod; __mod++)

struct optics_cic_context {
	irq_level_t             *irq_level;
	struct optics_gpio_info *gpios;
	unsigned                 nmods[OPTICS_CIC_NMODS];
	unsigned                 nregs[OPTICS_CIC_NREGS];
};

/* GPIO names must be 15-character strings at most */
#define OPTICS_CIC_MAXLEN 16

static const char optics_cic_sdd_intl[]   = "SDD_INTL%u";
static const char optics_cic_qsfp_intl[]  = "QSFP_INTL%u";
static const char optics_cic_sfp_txflt[]  = "SFP_TFLT%u";
static const char optics_cic_sfp_rxlos[]  = "SFP_RLOS%u";
static const char optics_cic_sfp_pres[]   = "SFP_PRES%u";
static const char optics_cic_qsfp_pres[]  = "QSFP_PRES%u";
static const char optics_cic_sdd_rxlos[]  = "SDD_RLOS%u";
static const char optics_cic_sfp_pwrgd[]  = "SFP_PGD%u";
static const char optics_cic_qsfp_pwrgd[] = "QSFP_PGD%u";

/* Here is the magic decoder ring: the driver knows the meaning of the
 * interrupts in the big generic table, and it presents them to user
 * space in predictable 15 characters or fewer. The non-applicable
 * interrupts do not show up to avoid burning precious GPIO space.
 */
static
const char * const optics_cic_formats[OPTICS_CIC_NREGS][OPTICS_CIC_NMODS] = {
	[OPTICS_CIC_DEVICE]  = {
		[OPTICS_CIC_SFP]    = NULL,
		[OPTICS_CIC_SFPDD]  = optics_cic_sdd_intl,
		[OPTICS_CIC_QSFP]   = optics_cic_qsfp_intl,
		[OPTICS_CIC_QSFPDD] = optics_cic_qsfp_intl,
	},
	[OPTICS_CIC_TXFLT]   = {
		[OPTICS_CIC_SFP]    = optics_cic_sfp_txflt,
		[OPTICS_CIC_SFPDD]  = optics_cic_sfp_txflt,
		[OPTICS_CIC_QSFP]   = NULL,
		[OPTICS_CIC_QSFPDD] = NULL,
	},
	[OPTICS_CIC_RXLOS]   = {
		[OPTICS_CIC_SFP]    = optics_cic_sfp_rxlos,
		[OPTICS_CIC_SFPDD]  = optics_cic_sfp_rxlos,
		[OPTICS_CIC_QSFP]   = NULL,
		[OPTICS_CIC_QSFPDD] = NULL,
	},
	[OPTICS_CIC_PRESENT] = {
		[OPTICS_CIC_SFP]    = optics_cic_sfp_pres,
		[OPTICS_CIC_SFPDD]  = optics_cic_sfp_pres,
		[OPTICS_CIC_QSFP]   = optics_cic_qsfp_pres,
		[OPTICS_CIC_QSFPDD] = optics_cic_qsfp_pres,
	},
	[OPTICS_CIC_RXLOSDD] = {
		[OPTICS_CIC_SFP]    = NULL,
		[OPTICS_CIC_SFPDD]  = optics_cic_sdd_rxlos,
		[OPTICS_CIC_QSFP]   = NULL,
		[OPTICS_CIC_QSFPDD] = NULL,
	},
	[OPTICS_CIC_PWRGD]   = {
		[OPTICS_CIC_SFP]    = optics_cic_sfp_pwrgd,
		[OPTICS_CIC_SFPDD]  = optics_cic_sfp_pwrgd,
		[OPTICS_CIC_QSFP]   = optics_cic_qsfp_pwrgd,
		[OPTICS_CIC_QSFPDD] = optics_cic_qsfp_pwrgd,
	},
};

static int optics_cic_dyn_gpio_alloc(struct device             *dev,
				     struct optics_cic_context *occ,
				     size_t                     nm_store)
{
	struct optics_gpio_info *gpios;
	unsigned                 regw;
	size_t                   irtsz;

	gpios = occ->gpios;
	regw  = REG_WIDTH;
	irtsz = sizeof(*occ->irq_level->irq_reg_table);

	if (0 == gpios->num_dyn_gpios)
		dev_warn(dev, "no optics interrupts!\n");

	/* Allocate the store for the dynamic GPIO names, GPIO table
	 * and interrupt table in one big happy block.
	 */
	nm_store += irtsz * (1 + ((gpios->num_fixed_gpios +
				   gpios->num_dyn_gpios) / regw));

	nm_store += gpios->num_dyn_gpios * sizeof(*gpios->dyn_gpios);

	gpios->dyn_gpios = devm_kzalloc(dev, nm_store, GFP_KERNEL);

	if (NULL == gpios->dyn_gpios) {
		dev_err(dev, "no memory for %u GPIO names\n",
			gpios->num_dyn_gpios);
		return -1;
	}

	/* copy the fixed interrupt table */
	memcpy(gpios->dyn_gpios + gpios->num_dyn_gpios,
	       occ->irq_level->irq_reg_table,
	       irtsz * (gpios->num_fixed_gpios / regw));

	return 0;
}

static int optics_cic_regs_fixup(struct device             *dev,
				 struct optics_cic_context *occ,
				 bool                       for_real)
{
	struct optics_gpio_info *gpios;
	const irq_reg_info_t    *iri_src;
	const irq_reg_info_t    *iri;
	const irq_reg_info_t    *iri_next;
	irq_reg_info_t          *iri_dst;
	irq_reg_info_t          *iri_out;
	const char             **names;
	const char              *fmt;
	unsigned                 pin_count;
	unsigned                 mask;
	unsigned                 regw;
	unsigned                 port;
	unsigned                 reg;
	unsigned                 mod;
	unsigned                 bit;
	size_t                   reqsz;
	size_t                   maxl;
	size_t                   nmsz;
	char                    *nm_pos;
	char                     scratch[OPTICS_CIC_MAXLEN];

	reqsz    = 0;
	maxl     = sizeof(scratch);
	gpios    = occ->gpios;
	regw     = REG_WIDTH;
	names    = gpios->dyn_gpios;
	iri_src  = occ->irq_level->irq_reg_table;
	iri_src += (gpios->num_fixed_gpios / regw);

	if (for_real) {
		/* The dynamic interrupt table sits directly below the
		 * dynamic GPIO table.
		 */
		iri_dst = (irq_reg_info_t *) (gpios->dyn_gpios +
					      gpios->num_dyn_gpios);

		/* Dynamic storage for the variable optics GPIO names
		 * lives at the end of the dynamic interrupt table.
		 */
		nm_pos = (char *) (iri_dst + 1 +
				   ((gpios->num_fixed_gpios +
				     gpios->num_dyn_gpios) / regw));
	}
	else {
		iri_dst = NULL;
		nm_pos  = scratch;
	}

	/* Skip the fixed registers. */
	iri_out = iri_dst + (gpios->num_fixed_gpios / regw);

	foreach_cic_reg(reg) {
		iri_next = iri_src + occ->nregs[reg];
		if (iri_next == iri_src) continue;

		iri  = iri_src;
		mask = 0;
		port = 0;
		bit  = 0;

		foreach_cic_mod(mod) {
			/* SFP and QSFP port numbers start at zero. */
			if (OPTICS_CIC_QSFP == mod) port = 0;

			pin_count = occ->nmods[mod];
			fmt       = optics_cic_formats[reg][mod];

			while (pin_count--) {
				if (NULL != fmt) {
					if (iri_next <= iri) {
						dev_err(dev, "overflow! "
							"reg=%u mod=%u\n",
							reg, mod);
						return -1;
					}

					/* Every live pin gets a GPIO name. */
					nmsz   = snprintf(nm_pos, maxl,
							  fmt, port);
					nmsz  += sizeof('\0');
					reqsz += nmsz;

					mask |= (1 << bit);

					if (for_real) {
						names[bit]  = nm_pos;
						nm_pos     += nmsz;
					}
				}

				port++;
				bit++;
				/* Flush the mask on register boundaries. */
				if (regw == bit) {
					if (mask) {
						if (for_real) {
							*iri_out      = *iri;
							iri_out->mask = mask;
						}
						names += regw;
						iri_out++;
					}
					bit = mask = 0;
					iri++;
				}
			}
		}

		/* Flush the mask at the end of every register sweep. */
		if (bit && mask) {
			if (for_real) {
				*iri_out      = *iri;
				iri_out->mask = mask;
			}
			names += regw;
			iri_out++;
		}

		iri_src = iri_next;
	}

	if (for_real) {
		/* Final mask = 0 marks the end of the register list. */
		iri_out->mask = 0;

		/* The core driver to use the dynamic interrupt table */
		occ->irq_level->irq_reg_table = iri_dst;
	}
	else {
		/* The dynamic gpio table sizes are now known.
		 * Allocate away,
		 */
		gpios->num_dyn_gpios = names - gpios->dyn_gpios;
		if (optics_cic_dyn_gpio_alloc(dev, occ, reqsz))
			return -1;
	}

	return 0;
}

static int optics_cic_sanity_check(struct device             *dev,
				   struct optics_cic_context *occ)
{
	irq_reg_info_t *iri;
	unsigned        regw;
	unsigned        npins;
	unsigned        nregs;
	unsigned        cic_reg;

	BUG_ON(NULL == occ);
	BUG_ON(NULL == occ->gpios);
	BUG_ON(NULL == occ->irq_level);
	BUG_ON(NULL == occ->irq_level->irq_reg_table);

	iri  = occ->irq_level->irq_reg_table;
	regw = REG_WIDTH;

	if (occ->gpios->num_fixed_gpios % regw) {
		dev_err(dev, "%u must be a multiple of %u\n",
			occ->gpios->num_fixed_gpios, regw);
		return -1;
	}

	/* Compute the theoretical size of the register table. */
	nregs = occ->gpios->num_fixed_gpios / regw;
	foreach_cic_reg(cic_reg)
		nregs += occ->nregs[cic_reg];

	/* Move to the end of the interrupt register table to find out
	 * how many interrupt registers there are.
	 */
	while (iri && iri->mask) iri++;

	/* If the number of registers do not match, then something is
	 * insane in the input data.
	 */
	if (nregs != (iri - occ->irq_level->irq_reg_table)) {
		dev_err(dev, "irq table size: %zu, register count: %u\n",
			iri - occ->irq_level->irq_reg_table, nregs);
		return -1;
	}

	/* Now consider the GPIO pin count. */
	npins = nregs * regw;

	if (occ->irq_level->irq_sources_max < npins) {
		dev_err(dev, "asking for %u pins, but irq_sources_max is %u\n",
			npins, occ->irq_level->irq_sources_max);
		return -1;
	}

	return 0;
}

static irq_level_t *optics_cic_populate(struct device             *dev,
					struct optics_cic_context *occ)
{
	if (optics_cic_sanity_check(dev, occ)) return NULL;

	/* Pre-allocating a fixed table for gpio names would be a big
	 * 12kB waste. The cpio count is figured out dynamically, at
	 * the cost of fewer than 12kB worth of code (we hope).
	 *
	 * The first pass figures out how much dynamic storage is
	 * required.
	 */
	if (optics_cic_regs_fixup(dev, occ, false)) return NULL;

	/* Re-run the same logic, this time for real. */
	if (optics_cic_regs_fixup(dev, occ, true)) return NULL;

	return occ->irq_level;
}

#endif /* __SIRILX_CIC_OPTICS_H */
