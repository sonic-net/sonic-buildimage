#ifndef _FPGA_I2C_H
#define _FPGA_I2C_H

#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/kallsyms.h>
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

#if 0

#define FPGA_I2C_EXT_9548_ADDR        (0x00)
#define FPGA_I2C_EXT_9548_CHAN        (0x04)
#define FPGA_I2C_DEV_SLAVE_ADDR       (0x08)
#define FPGA_I2C_DEV_REG_ADDR         (0x0C)
#define FPGA_I2C_DEV_RDWR_LEN         (0x10)
#define FPGA_I2C_CTRL_REG             (0x14)
#define FPGA_I2C_STATUS_REG           (0x18)
#define FPGA_I2C_SCALE_REG            (0x1C)
#define FPGA_I2C_FILTER_REG           (0x20)
#define FPGA_I2C_STRETCH_REG          (0x24)
#define FPGA_I2C_EXT_9548_EXITS_FLAG  (0x28)
#define FPGA_I2C_INTERNAL_9548_CHAN   (0x2C)
#define FPGA_I2C_RDWR_DATA_BUF        (0x80)
#endif
#define FPGA_I2C_RDWR_MAX_LEN_DEFAULT (128)
#define I2C_REG_MAX_WIDTH             (16)

#define DEV_NAME_MAX_LEN              (64)

#define FPGA_I2C_MAX_TIMES            (10)
#define FPGA_I2C_XFER_TIME_OUT        (100000)
#define FPGA_I2C_SLEEP_TIME           (40)

typedef struct fpga_i2c_reg_s {
    uint32_t i2c_scale;
    uint32_t i2c_filter;
    uint32_t i2c_stretch;
    uint32_t i2c_ext_9548_exits_flag;
    uint32_t i2c_ext_9548_addr;
    uint32_t i2c_ext_9548_chan;
    uint32_t i2c_in_9548_chan;
    uint32_t i2c_slave;
    uint32_t i2c_reg;
    uint32_t i2c_reg_len;
    uint32_t i2c_data_len;
    uint32_t i2c_ctrl;
    uint32_t i2c_status;
    uint32_t i2c_err_vec;
    uint32_t i2c_data_buf;
    uint32_t i2c_data_buf_len;
} fpga_i2c_reg_t;

typedef struct fpga_i2c_reset_cfg_s {
    uint32_t i2c_adap_reset_flag;
    uint32_t reset_addr;
    uint32_t reset_on;
    uint32_t reset_off;
    uint32_t reset_delay_b;
    uint32_t reset_delay;
    uint32_t reset_delay_a;
} fpga_i2c_reset_cfg_t;

typedef struct fpga_i2c_reg_addr_s {
    uint8_t reg_addr_len;
    uint8_t read_reg_addr[I2C_REG_MAX_WIDTH];
} fpga_i2c_reg_addr_t;

typedef struct fpga_i2c_dev_s {
    fpga_i2c_reg_t reg;
    fpga_i2c_reset_cfg_t reset_cfg;
    fpga_i2c_reg_addr_t i2c_addr_desc;
    const char *dev_name;
    uint32_t i2c_scale_value;
    uint32_t i2c_filter_value;
    uint32_t i2c_stretch_value;
    uint32_t i2c_timeout;
    uint32_t i2c_func_mode;
    wait_queue_head_t queue;
    struct i2c_adapter adap;
    int adap_nr;
    struct device *dev;
    bool i2c_params_check;
} fpga_i2c_dev_t;

typedef struct fpga_i2c_bus_device_s {
    int i2c_timeout;
    int i2c_scale;
    int i2c_filter;
    int i2c_stretch;
    int i2c_ext_9548_exits_flag;
    int i2c_ext_9548_addr;
    int i2c_ext_9548_chan;
    int i2c_in_9548_chan;
    int i2c_slave;
    int i2c_reg;
    int i2c_reg_len;
    int i2c_data_len;
    int i2c_ctrl;
    int i2c_status;
    int i2c_err_vec;
    int i2c_data_buf;
    int i2c_data_buf_len;
    char dev_name[DEV_NAME_MAX_LEN];
    int adap_nr;
    int i2c_scale_value;
    int i2c_filter_value;
    int i2c_stretch_value;
    int i2c_func_mode;
    int i2c_adap_reset_flag;
    int i2c_reset_addr;
    int i2c_reset_on;
    int i2c_reset_off;
    int i2c_rst_delay_b;        /* delay time before reset(us) */
    int i2c_rst_delay;          /* reset time(us) */
    int i2c_rst_delay_a;        /* delay time after reset(us) */
    int device_flag;
    bool i2c_params_check;
    int i2c_data_buf_len_reg;
    int i2c_offset_reg;
} fpga_i2c_bus_device_t;

typedef struct fpga_pca954x_device_s {
    struct i2c_client *client;
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    uint32_t fpga_9548_flag;
    uint32_t fpga_9548_reset_flag;
    uint32_t pca9548_base_nr;
} fpga_pca954x_device_t;

#endif /* _FPGA_I2C_H */
