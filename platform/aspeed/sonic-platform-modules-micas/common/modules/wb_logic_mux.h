#ifndef __WB_LOGIC_MUX_H__
#define __WB_LOGIC_MUX_H__
#include <wb_logic_dev_common.h>

typedef struct logic_mux_device_s {
    struct i2c_client *client;
    struct i2c_adapter *adap;
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    char dev_name[MAX_NAME_SIZE];
    uint32_t reg_offset;
    uint32_t logic_dev_func_mode;
    uint32_t base_nr;
    bool probe_hw_init;
    bool no_deselect;
} logic_mux_device_t;

#endif /*  __WB_LOGIC_MUX_H__ */

