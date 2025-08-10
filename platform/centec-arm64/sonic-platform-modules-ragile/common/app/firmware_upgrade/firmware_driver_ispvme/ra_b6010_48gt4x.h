#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <firmware_cpld_ispvme.h>

#define    JTAG_TDI            (32)
#define    JTAG_TDO            (67)
#define    JTAG_TCK            (65)
#define    JTAG_TMS            (6)
#define    JTAG_EN             (50)

typedef struct firmware_device_info_s {
    int type;
    int tdi;
    int tck;
    int tms;
    int tdo;
    int jtag_en;
    int select;
    gpio_group_t jtag_5;
    gpio_group_t jtag_4;
    gpio_group_t jtag_3;
    gpio_group_t jtag_2;
    gpio_group_t jtag_1;
    int cmic_start_gpio;
    int cmic_end_gpio;
} firmware_device_info_t;

#endif /* __CONFIG_H__ */