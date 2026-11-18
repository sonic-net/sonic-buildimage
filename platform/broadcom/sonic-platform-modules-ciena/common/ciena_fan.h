#ifndef _CIENA_FAN_H
#define _CIENA_FAN_H

#define CIENA_FAN_NAME "ciena_fan"


struct ciena_fan_pdata {
	const char    *name;
	struct regmap *regmap;
	unsigned int   index;
	unsigned int   tach_reg;
	unsigned int   tach_mask;
	unsigned int   tach_shift;
	unsigned int   stat_reg;
	unsigned int   pres_mask;
	bool           pres_invert;
	unsigned int   fault_mask;
	bool           fault_invert;
	unsigned int   thres_reg;
	unsigned int   thres_min_mask;
	unsigned int   thres_min_shift;
	unsigned int   thres_max_mask;
	unsigned int   thres_max_shift;
	unsigned int   thres_low_crit;
	unsigned int   thres_max_norm;
};

#endif
// vim: sw=8 noet
