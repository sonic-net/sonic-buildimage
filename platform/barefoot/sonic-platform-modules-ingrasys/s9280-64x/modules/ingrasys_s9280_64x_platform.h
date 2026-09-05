#ifndef _S9230_64X_PLATFORM_H
#define _S9230_64X_PLATFORM_H

#include <linux/i2c.h>

// remove debug before release
#define DEBUG

enum bus_order {
    I2C_BUS_MAIN,
    MUX_9548_0_CH0,
    MUX_9548_0_CH1,
    MUX_9548_0_CH2,
    MUX_9548_0_CH3,
    MUX_9548_0_CH4,
    MUX_9548_0_CH5,
    MUX_9548_0_CH6,
    MUX_9548_0_CH7,
    MUX_9548_1_CH0,
    MUX_9548_1_CH1,
    MUX_9548_1_CH2,
    MUX_9548_1_CH3,
    MUX_9548_1_CH4,
    MUX_9548_1_CH5,
    MUX_9548_1_CH6,
    MUX_9548_1_CH7,
    MUX_9548_2_CH0,
    MUX_9548_2_CH1,
    MUX_9548_2_CH2,
    MUX_9548_2_CH3,
    MUX_9548_2_CH4,
    MUX_9548_2_CH5,
    MUX_9548_2_CH6,
    MUX_9548_2_CH7,
    MUX_9546_0_CH0,
    MUX_9546_0_CH1,
    MUX_9546_0_CH2,
    MUX_9546_0_CH3,
    MUX_9546_1_CH0,
    MUX_9546_1_CH1,
    MUX_9546_1_CH2,
    MUX_9546_1_CH3,
    MUX_9548_11_CH0,
    MUX_9548_11_CH1,
    MUX_9548_11_CH2,
    MUX_9548_11_CH3,
    MUX_9548_11_CH4,
    MUX_9548_11_CH5,
    MUX_9548_11_CH6,
    MUX_9548_11_CH7,
    MUX_9548_3_CH0,
    MUX_9548_3_CH1,
    MUX_9548_3_CH2,
    MUX_9548_3_CH3,
    MUX_9548_3_CH4,
    MUX_9548_3_CH5,
    MUX_9548_3_CH6,
    MUX_9548_3_CH7,
    MUX_9548_4_CH0,
    MUX_9548_4_CH1,
    MUX_9548_4_CH2,
    MUX_9548_4_CH3,
    MUX_9548_4_CH4,
    MUX_9548_4_CH5,
    MUX_9548_4_CH6,
    MUX_9548_4_CH7,
    MUX_9548_5_CH0,
    MUX_9548_5_CH1,
    MUX_9548_5_CH2,
    MUX_9548_5_CH3,
    MUX_9548_5_CH4,
    MUX_9548_5_CH5,
    MUX_9548_5_CH6,
    MUX_9548_5_CH7,
    MUX_9548_6_CH0,
    MUX_9548_6_CH1,
    MUX_9548_6_CH2,
    MUX_9548_6_CH3,
    MUX_9548_6_CH4,
    MUX_9548_6_CH5,
    MUX_9548_6_CH6,
    MUX_9548_6_CH7,
    MUX_9548_7_CH0,
    MUX_9548_7_CH1,
    MUX_9548_7_CH2,
    MUX_9548_7_CH3,
    MUX_9548_7_CH4,
    MUX_9548_7_CH5,
    MUX_9548_7_CH6,
    MUX_9548_7_CH7,
    MUX_9548_8_CH0,
    MUX_9548_8_CH1,
    MUX_9548_8_CH2,
    MUX_9548_8_CH3,
    MUX_9548_8_CH4,
    MUX_9548_8_CH5,
    MUX_9548_8_CH6,
    MUX_9548_8_CH7,
    MUX_9548_9_CH0,
    MUX_9548_9_CH1,
    MUX_9548_9_CH2,
    MUX_9548_9_CH3,
    MUX_9548_9_CH4,
    MUX_9548_9_CH5,
    MUX_9548_9_CH6,
    MUX_9548_9_CH7,
    MUX_9548_10_CH0,
    MUX_9548_10_CH1,
    MUX_9548_10_CH2,
    MUX_9548_10_CH3,
    MUX_9548_10_CH4,
    MUX_9548_10_CH5,
    MUX_9548_10_CH6,
    MUX_9548_10_CH7,
};

#define I2C_ADDR_MUX_9555_0      (0x20)
#define I2C_ADDR_MUX_9555_1      (0x24)
#define I2C_ADDR_MUX_9555_2      (0x25)
#define I2C_ADDR_MUX_9555_3      (0x26)
#define I2C_ADDR_MUX_9539_0      (0x76)
#define I2C_ADDR_MUX_9539_1      (0x76)
#define I2C_BUS_FAN_STATUS       (I2C_BUS_MAIN)
#define I2C_BUS_SYS_LED          (MUX_9548_1_CH1)
#define I2C_BUS_PSU_STATUS       (I2C_BUS_MAIN)
#define I2C_ADDR_PSU_STATUS      (I2C_ADDR_MUX_9555_2)

#define NUM_OF_I2C_MUX              (11)
#define NUM_OF_CPLD                     (5)
#define NUM_OF_QSFP_PORT          (64)
#define NUM_OF_SFP_PORT            (2)
#define QSFP_EEPROM_I2C_ADDR      (0x50)

enum gpio_reg {
    REG_PORT0_IN,
    REG_PORT1_IN,
    REG_PORT0_OUT,
    REG_PORT1_OUT,
    REG_PORT0_POL,
    REG_PORT1_POL,
    REG_PORT0_DIR,
    REG_PORT1_DIR,
};

struct ing_i2c_board_info {
    int ch;
    int size;
    struct i2c_board_info *board_info;
};

struct i2c_init_data {
    __u16 ch;
    __u16 addr;
    __u8  reg;
    __u8  value; 
};

#endif
