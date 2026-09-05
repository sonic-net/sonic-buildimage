#ifndef __WB_I2C_MUX_PCA9641_H__
#define __WB_I2C_MUX_PCA9641_H__

#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

typedef enum pca9641_reset_type_s {
    PCA9641_RESET_NONE = 0,
    PCA9641_RESET_I2C = 1,
    PCA9641_RESET_GPIO = 2,
    PCA9641_RESET_IO = 3,
    PCA9641_RESET_FILE = 4,
} pca9641_reset_type_t;

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
} file_attr_t;

typedef struct gpio_attr_s {
    int gpio_init;
    uint32_t gpio;
    uint32_t reset_on;
    uint32_t reset_off;
} gpio_attr_t;

typedef struct i2c_mux_pca9641_device_s {
    struct i2c_client *client;
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    uint32_t pca9641_nr;
    uint32_t pca9641_reset_type;
    uint32_t rst_delay_b;                   /* delay time before reset(us) */
    uint32_t rst_delay;                     /* reset time(us) */
    uint32_t rst_delay_a;                   /* delay time after reset(us) */
    union {
        i2c_attr_t i2c_attr;
        gpio_attr_t gpio_attr;
        io_attr_t io_attr;
        file_attr_t file_attr;
    } attr;
} i2c_mux_pca9641_device_t;

#endif
