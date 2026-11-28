#ifndef __WB_FPGA_SEU_H__
#define __WB_FPGA_SEU_H__

#include <wb_logic_dev_common.h>

#define FPGA_SEU_DATA_LEN_DEFAULT    (128)
#define FPGA_SEU_STATUS_VALID_MASK   (0x01)
#define FPGA_SEU_CTRL_CLEAR_MASK     (0x01)

typedef struct fpga_seu_device_s {
    char dev_name[MAX_NAME_SIZE];
    char seu_name[MAX_NAME_SIZE];
    int logic_func_mode;
    int seu_data_reg;
    int seu_data_status_reg;
    int seu_data_ctrl_reg;
    int seu_data_cnt_reg;
    int seu_data_len;
    int seu_status_valid_mask;
    int seu_ctrl_clear_mask;
    int device_flag;
} fpga_seu_device_t;

#endif