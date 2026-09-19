#ifndef _FPGA_WATCHDOG_INCLUDED
#define _FPGA_WATCHDOG_INCLUDED

#define FPGA_WATCHDOG_DRIVER_NAME "ciena-fpga-watchdog"

struct ciena_fpga_watchdog_pdata {
	const char     *name;
	struct regmap  *regmap;
	unsigned        wdt_ctl_reg;
	unsigned        wdt_clr_reg;
};

#endif
