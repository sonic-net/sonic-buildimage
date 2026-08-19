#ifndef __MC_CPLD_H__
#define __MC_CPLD_H__

#include <linux/i2c.h>

// 对外导出的读取SFP状态函数
extern int port_cpld_read_sfp_status(u8 reg, u8 *value);
extern int port_cpld_ctl_sfp_led(u8 reg, u8 value);

#endif
