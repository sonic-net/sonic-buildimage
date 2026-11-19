#ifndef __MC_CPLD_H__
#define __MC_CPLD_H__

#include <linux/i2c.h>

extern int port_cpld_read_sfp_status(u8 reg, u8 *value);
extern int port_cpld_ctl_sfp_led(u8 reg, u8 value);

#endif
