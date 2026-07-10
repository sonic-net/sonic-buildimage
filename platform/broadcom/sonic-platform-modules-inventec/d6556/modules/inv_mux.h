/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef INV_MUX_H
#define INV_MUX_H


/* MUX basic information */
#define MUX_GPIO_LABEL                 "SWPS_RST_MUX"

/* MUX reset GPIO define */
#define MUX_RST_GPIO_FORCE            (30100)
#define MUX_RST_GPIO_FORCE_RANGELEY   (30101)
#define MUX_RST_GPIO_FORCE_HEDERA     (30102)
#define MUX_RST_GPIO_48_PAC9548          (48)
#define MUX_RST_GPIO_69_PAC9548          (69)
#define MUX_RST_GPIO_249_PCA9548        (249)
#define MUX_RST_GPIO_500_PAC9548        (500)
#define MUX_RST_GPIO_505_PCA9548        (505)

/* MUX relate value define */
#define MUX_RST_WAIT_MS                   (1)
#define MUX_RST_MEM_ADDR_RANGELEY         (0)    // TBD
#define MUX_RST_MEM_ADDR_HEDERA       (0x548)

struct mux_obj_s {
    unsigned gpio_num;
    int (*_pull_high)(struct mux_obj_s *self);
    int (*_pull_low)(struct mux_obj_s *self);
    int (*_init)(struct mux_obj_s *self);
    int (*reset)(struct mux_obj_s *self);
};


void clean_mux_gpio(void);
int  reset_mux_gpio(void);
int  init_mux_gpio(unsigned gpio);


#endif /* INV_MUX_H */



