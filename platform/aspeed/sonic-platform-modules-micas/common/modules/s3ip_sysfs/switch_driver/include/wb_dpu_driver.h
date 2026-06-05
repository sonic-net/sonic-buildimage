#ifndef _WB_DPU_DRIVER_H_
#define _WB_DPU_DRIVER_H_

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"

#define DPU_MAX_NUMBER                  (255)
#define DPU_EEPROM_SIZE                 (256)
#define DPU_OP_NOT_PRESENT              (0xfe)
#define DPU_OP_ERROR                    (0xfd)
#define ORDER_DIRECTION_MASK            (0x00C0)

typedef enum {
    DPU_BINARY_BASE         = (1 << 1),
    DPU_OCTAL_BASE          = (1 << 2),
    DPU_DECIMAL_BASE        = (1 << 3),
    DPU_HEX_BASE            = (1 << 4),
    DPU_ASCII_BASE          = (1 << 5),
    DPU_FORWARD_BASE        = (1 << 6),
    DPU_REVERSE_BASE        = (1 << 7),
    DPU_MAPPING_BASE        = (1 << 8),
    DPU_X1000_BASE          = (1 << 9),
    DPU_X1000000_BASE       = (1 << 10),
} dpu_base_type;

#define DFD_GET_DPU_KEY1(dpu_type, dpu_id) \
    ((((dpu_type) & 0xffff) << 16) | ((dpu_id) & 0xff))

#define DFD_GET_DPU_COMMON_KEY2(dpu_id, func) \
        ((((dpu_id) & 0xff) << 8) | ((func) & 0xff))

#define DFD_DPU_GET_SENSOR_DPU_ID(main_dev_id)  \
        (((main_dev_id) >> 16) & 0xff)

#define DFD_DPU_GET_SENSOR_SENSOR_ID(main_dev_id)  \
        ((main_dev_id) & 0xff)

#define DFD_DPU_GET_SENSOR_FW_ID(main_dev_id)  \
        ((main_dev_id) & 0xff)

#define DFD_COM_DPU_SENSOR_DPU_SENSOR_ID(dpu_id, sensor_id)  \
        (((dpu_id) & 0xff) << 16 | ((sensor_id) & 0xff))

#define DFD_COM_DPU_FW_DPU_FW_ID(dpu_id, fw_id)  \
    (((dpu_id) & 0xff) << 16 | ((fw_id) & 0xff))

#define DFD_DPU_TYPE_IS_VALID(dpu_type)  \
        (((dpu_type) < WB_MAIN_DPU_DEV_MAX) && ((dpu_type) >= 0))

#define IS_TARGET_SENSOR_STATIC(x) \
    ((x) == DFD_BMC_MANAGED_SENSOR_ALIAS_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_MAX_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_MIN_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_HIGH_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_LOW_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E || \
     (x) == DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E)

typedef struct {
    const char *name;
    int offset;
    int size;
    unsigned int base;
} dpu_func_attr_t;

typedef struct {
    const char *name;
    uint8_t eeprom_addr;
    uint8_t ven_addr;
    uint8_t dev_addr;
    unsigned int size;
    uint16_t vendor_id;
    uint16_t device_id;
    dfd_sysfs_func_map_t *dpu_attr_funcs;
    unsigned int dpu_attr_funcs_size;
    dpu_func_attr_t *fw_attr_table;
    unsigned int fw_number;
    dpu_func_attr_t *temp_attr_table;
    unsigned int temp_number;
    dfd_sysfs_func_map_t *dpu_temp_funcs;
    unsigned int dpu_temp_funcs_size;
} dpu_list_t;

typedef struct {
    int dpu_type;
    int monitor_last_flag;
    int monitor_curr_flag;
} dpu_device_type_t;

typedef enum {
    DPU_KEY_MODE_SENSOR,
    DPU_KEY_MODE_COMMON
} dpu_key_mode_t;

int dfd_get_dpu_type_by_slot(unsigned int main_dev_id);
int dfd_get_dpu_bus_num(int main_dev_id);
int get_dpu_temp_para_val(unsigned int main_dev_id, unsigned int sub_dev_id,
                          unsigned int sensor_index, int dpu_type,
                          char *val, char *buf, dpu_func_attr_t func_attr,
                          unsigned int count);
int dfd_common_get_dpu_fw_number(void);
int dfd_common_get_dpu_temp_number(void);
ssize_t dfd_common_get_dpu_attr(unsigned int index, unsigned int type, char *buf, size_t count);
int dfd_common_set_dpu_attr(unsigned int index, unsigned int type, unsigned int value);
ssize_t dfd_common_get_dpu_fw_attr(unsigned int index, unsigned int fw_index,
    unsigned int type, char *buf, size_t count);
ssize_t dfd_common_get_dpu_temp_attr(unsigned int dpu_index, unsigned int temp_index,
    unsigned int type, char *buf, size_t count);

ssize_t dfd_get_dpu_name(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_vendor(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_device(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_sn(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_pn(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_mac(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_monitor_flag(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_fw_alias(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_fw_version(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_fw_support_upgrade(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_fw_upgrade_type(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_temp_alias(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
ssize_t dfd_get_dpu_temp_value(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
int dpu_data_to_buf(char *buf, ssize_t buf_size, uint8_t *val, int valid_size, unsigned int mode);


extern dpu_device_type_t dpu_id_info_table[DPU_MAX_NUMBER];
extern dpu_list_t dpu_list[WB_MAIN_DPU_DEV_MAX];

#endif /* _WB_DPU_DRIVER_H_ */
