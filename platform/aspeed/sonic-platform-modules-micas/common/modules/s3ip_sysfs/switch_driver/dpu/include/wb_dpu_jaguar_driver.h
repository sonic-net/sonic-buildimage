#ifndef _WB_DPU_JAGUAR_DRIVER_H_
#define _WB_DPU_JAGUAR_DRIVER_H_

#include "wb_dpu_driver.h"

extern dpu_func_attr_t jaguar_dpu_fw_attr_table[DFD_DPU_FW_TYPE_MAX_E];
extern dpu_func_attr_t jaguar_dpu_temp_attr_table[DFD_DPU_TEMP_TYPE_MAX_E];
extern dfd_sysfs_func_map_t jaguar_dpu_func_table[DFD_DPU_MAX_E];
extern dfd_sysfs_func_map_t jaguar_dpu_temp_func_table[DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E];

#endif /* _WB_DPU_JAGUAR_DRIVER_H_ */
