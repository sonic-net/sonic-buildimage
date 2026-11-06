#ifndef __SIRILX_CIC_OPTICS_DEFS_H
#define __SIRILX_CIC_OPTICS_DEFS_H

enum optics_cic_reg {
	OPTICS_CIC_DEVICE,
	OPTICS_CIC_TXFLT,
	OPTICS_CIC_RXLOS,
	OPTICS_CIC_PRESENT,
	OPTICS_CIC_RXLOSDD,
	OPTICS_CIC_PWRGD,
	OPTICS_CIC_NREGS,
};

enum optics_cic_modtype {
	OPTICS_CIC_SFP,
	OPTICS_CIC_SFPDD,
	OPTICS_CIC_QSFP,
	OPTICS_CIC_QSFPDD,
	OPTICS_CIC_NMODS,
};

struct optics_gpio_info {
	const char * const *fixed_gpios;
	const char *       *dyn_gpios;
	unsigned            num_fixed_gpios;
	unsigned            num_dyn_gpios;
};

#endif /* __SIRILX_CIC_OPTICS_DEFS_H */
