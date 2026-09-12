#ifndef _REGMAP_SYSFS_INCLUDED
#define _REGMAP_SYSFS_INCLUDED

#define REGMAP_SYSFS_DRIVER_NAME "regmap-sysfs"

struct ciena_sysfs_regmap_pdata {
	const char    *name;
	struct regmap *regmap;
};

#endif
