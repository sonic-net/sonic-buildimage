#ifndef _WB_PLL_DRIVER_H_
#define _WB_PLL_DRIVER_H_

#define DFD_CLOCK_FAULT_XTAL    (0)
#define DFD_CLOCK_FAULT_BUS     (1)
#define DFD_CLOCK_FAULT_APLL    (4)

extern dfd_sysfs_func_map_t pll_func_table[DFD_PLL_MAX_E];
#endif /* _WB_PLL_DRIVER_H_ */