#ifndef INV_MUX_H
#define INV_MUX_H

#include <linux/types.h>

/* MUX basic information */
#define MUX_GPIO_LABEL                 "SWPS_RST_MUX"

/* MUX reset GPIO define */
#define MUX_RST_GPIO_FORCE                 (30100)
#define MUX_RST_GPIO_FORCE_RANGELEY        (30101)
#define MUX_RST_GPIO_FORCE_HEDERA          (30102)
#define MUX_RST_GPIO_48_PCA9548               (48)
#define MUX_RST_GPIO_69_PCA9548               (69)
#define MUX_RST_GPIO_249_PCA9548             (249)
#define MUX_RST_GPIO_500_PCA9548             (500)
#define MUX_RST_GPIO_505_PCA9548             (505)
#define MUX_RST_CPLD_C0_A77_70_74_RST_ALL  (30201)

/* MUX relate value define */
#define MUX_RST_WAIT_MS_PCA9548                (1)
#define MUX_RST_WAIT_MS_CPLD                  (10)
#define MUX_RST_MEM_ADDR_RANGELEY              (0)    // TBD
#define MUX_RST_MEM_ADDR_HEDERA            (0x548)

/* MUX error code define */
#define ERR_MUX_UNEXCPT                     (-399)

struct mux_obj_s {
    struct i2c_client *i2c_client_p;
    struct mux_obj_s  *next;
    unsigned gpio_num;
    int (*_pull_high)(struct mux_obj_s *self);
    int (*_pull_low)(struct mux_obj_s *self);
    int (*_init)(struct mux_obj_s *self);
    int (*_clean)(struct mux_obj_s *self);
    int (*reset)(struct mux_obj_s *self);
};


void clean_mux_objs(void);
int  reset_mux_objs(void);
int  init_mux_objs(unsigned gpio);


#endif /* INV_MUX_H */





