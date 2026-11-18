#ifndef _GPIOTEST_SYSFS_INCLUDED
#define _GPIOTEST_SYSFS_INCLUDED

#define GPIOTEST_SYSFS_DRIVER_NAME "gpiotest-sysfs"

struct ciena_sysfs_gpiotest_pdata {
	struct regmap *regmap;
	unsigned int   reg;
	unsigned int   mask;

};

#endif
// vim: sw=8 noet
