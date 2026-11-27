#ifndef _CIENA_THERMAL_H
#define _CIENA_THERMAL_H

#define CIENA_THERMAL_NAME "ciena_thermal"


struct ciena_thermal_pdata {
	const char    *name;
	struct regmap *regmap;
	unsigned int   reg;
	unsigned int   valid_mask;
	unsigned int   temp_mask;
	unsigned int   temp_shift;
	unsigned int   temp_qnbits;
	unsigned int   temp_unsigned;
	unsigned int   threshold;
};

#endif
// vim: sw=8 noet
