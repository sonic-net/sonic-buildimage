#ifndef _LED_SYSFS_INCLUDED
#define _LED_SYSFS_INCLUDED

#include <linux/types.h>

#define LED_SYSFS_DRIVER_NAME "ciena-led-sysfs"

struct ciena_led_sysfs_pdata {
	const char                *name;
	unsigned int               reg;
	unsigned int               mask;
	unsigned int               val;
	bool                       use_val;
	bool                       invert;
	unsigned int               blnk;
};

#endif
