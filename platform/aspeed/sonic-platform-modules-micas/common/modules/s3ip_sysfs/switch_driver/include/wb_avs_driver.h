#ifndef _WB_AVS_DRIVER_H_
#define _WB_AVS_DRIVER_H_

extern dfd_sysfs_func_map_t avs_func_table[DFD_AVS_MAX_E];

typedef enum avs_power_fault_e {
    AVS_PHASE_LOSS_FAULT,
    AVS_VIN_OV_FAULT,
    AVS_VOUT_OV_FAULT,
    AVS_VIN_UV_FAULT,
    AVS_VOUT_UV_FAULT,
    AVS_IIN_OC_WARNING,
    AVS_IOUT_OC_FAULT,
    AVS_OT_FAULT,
    AVS_PHASE_CURRENT_IMBALANCE_FAULT,
    AVS_POWER_FAULT_MAX,
} avs_power_fault_t;

extern dfd_debug_data_key_map_t avs_dbg_key_table[DFD_AVS_MAX_E];

#endif /* _WB_AVS_DRIVER_H_ */