#ifndef _GENERIC_CIC_CONFIG_H
#define _GENERIC_CIC_CONFIG_H

#include <linux/of.h>

#ifdef CIENA_CIC_REG_WIDTH
#define CIC_REG_WIDTH CIENA_CIC_REG_WIDTH
#else
#define CIC_REG_WIDTH 32
#endif

struct regmap;

struct generic_cic_gpio {
	struct device *dev;
	const char    *pin;
};

struct generic_cic_config {
	resource_size_t         start_offset;
	struct generic_cic_gpio parent_irq;
	struct regmap          *parent_regmap;
	bool                    shared_io;
	bool                    little_endian;
	bool                    use_parent_msi;
	bool                    clear_to_clear;
	int                     gpio_base;
};

typedef uint32_t reg_t;

struct generic_cic_priv;

struct irq_reg_info_s {
	reg_t  disable;                /* ISM    - MASK */
	reg_t  status;                 /* STATUS - STAT */
	reg_t  intr;                   /* ISR    - INTR */
	reg_t  test;                   /* IST    - TEST */
	reg_t  enable;                 /* ENABLE */
	u32    mask;
	u32    master;
	int    start;
	int    end;
	char  *name;
	u32    bits;                  /* # of bits used */
	u32    irq_seq[CIC_REG_WIDTH];
	u16    irq_count[CIC_REG_WIDTH];
	reg_t (*rd_mask)(struct generic_cic_priv *p,
			 struct irq_reg_info_s *iri);
	void  (*wr_mask)(struct generic_cic_priv *p,
			 struct irq_reg_info_s *iri,
			 reg_t v);
};
typedef struct irq_reg_info_s irq_reg_info_t;

typedef struct {
	int             irq_sources_max;
	irq_reg_info_t *irq_reg_table;
	irq_reg_info_t *irq_reg_ignore_table;
	irq_reg_info_t *irq_test_reg;
	int             use_mask_in_ignore;
} irq_level_t;

/* what the cic driver must do with each individual gpio */
enum cic_gpio_fate {
	/* this pin has no name */
	cic_gpio_drop,
	/* export to sysfs: cic does gpiod_get() and gpiod_export() */
	cic_gpio_export,
	/* irq: gpiod_get() by cic, request_irq() by other driver */
	cic_gpio_irq,
	/* gpio: other driver does gpiod_get() */
	cic_gpio_gpio,
};

#endif /* _GENERIC_CIC_CONFIG_H */
// vim: sw=8 noet
