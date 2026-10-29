#ifndef _CIENA_RAW_CHARDEV_H
#define _CIENA_RAW_CHARDEV_H

#define CIENA_RAW_CHARDEV_NAME "ciena_raw_chardev"


struct ciena_raw_chardev_pdata {
	struct regmap *chardev_regmap;
	const char    *chardev_env;
};

#endif
