#ifndef __WB_I2C_MUX_PCA954X_H__
#define __WB_I2C_MUX_PCA954X_H__

#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

typedef enum pca9548_reset_type_s {
    PCA9548_RESET_NONE = 0,
    PCA9548_RESET_I2C = 1,
    PCA9548_RESET_GPIO = 2,
    PCA9548_RESET_IO = 3,
    PCA9548_RESET_FILE = 4,
    PCA9548_RESET_LOGIC = 5,
} pca9548_reset_type_t;

typedef enum pca9548_reset_reg_mode_s {
    PCA9548_RESET_REG_MODE_COMMON     = 0,
    PCA9548_RESET_REG_MODE_REGKEY     = 1,
    PCA9548_RESET_REG_MODE_STATICKEY  = 2,
} pca9548_reset_reg_mode_t;

#define RESET_REGKEY_MASK (0x01)

typedef struct i2c_attr_s {
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    uint32_t reg_offset;
    uint32_t mask;
    uint32_t reset_on;
    uint32_t reset_off;
} i2c_attr_t;

typedef struct io_attr_s {
    uint32_t io_addr;
    uint32_t mask;
    uint32_t reset_on;
    uint32_t reset_off;
} io_attr_t;

typedef struct file_attr_s {
    const char *dev_name;
    uint32_t offset;
    uint32_t mask;
    uint32_t reset_on;
    uint32_t reset_off;
    uint32_t width;
} file_attr_t;

typedef struct gpio_attr_s {
    int gpio_init;
    uint32_t gpio;
    uint32_t reset_on;
    uint32_t reset_off;
} gpio_attr_t;

typedef struct logic_attr_s {
    const char *dev_name;
    uint32_t offset;
    uint32_t mask;
    uint32_t reset_on;
    uint32_t reset_off;
    uint32_t width;
    uint32_t logic_func_mode;
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
} logic_attr_t;

typedef struct i2c_mux_pca954x_device_s {
    struct i2c_client *client;
    struct i2c_adapter *adap;
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    uint32_t pca9548_base_nr;
    uint32_t pca9548_reset_type;
    uint32_t reset_reg_mode;
    uint32_t rst_delay_b;                   /* delay time before reset(us) */
    uint32_t rst_delay;                     /* reset time(us) */
    uint32_t rst_delay_a;                   /* delay time after reset(us) */
    bool probe_disable;
    bool probe_hw_init;
    bool select_chan_check;
    bool close_chan_force_reset;
    union {
        i2c_attr_t i2c_attr;
        gpio_attr_t gpio_attr;
        io_attr_t io_attr;
        file_attr_t file_attr;
        logic_attr_t logic_attr;
    } attr;
} i2c_mux_pca954x_device_t;

#endif
