#ifndef _WB_PCIE_DRIVER_H_
#define _WB_PCIE_DRIVER_H_

#define PCIE_DEVICE_CURRENT_LINK_SPEED       "/sys/bus/pci/devices/%s/current_link_speed"
#define PCIE_DEVICE_CURRENT_LINK_WIDTH       "/sys/bus/pci/devices/%s/current_link_width"
#define PCIE_DEVICE_MAX_LINK_SPEED           "/sys/bus/pci/devices/%s/max_link_speed"
#define PCIE_DEVICE_MAX_LINK_WIDTH           "/sys/bus/pci/devices/%s/max_link_width"
#define PCIE_DEVICE_INVALID_VALUE            "Unknown"
#define PCIE_DEVICE_ABNORMAL_VALUE           (63)
#define PCIE_DEVICE_BDF_MAX_LENGTH           (13)

extern dfd_sysfs_func_map_t pcie_func_table[DFD_PCIE_MAX_E];
extern dfd_debug_data_key_map_t pcie_dbg_key_table[DFD_PCIE_MAX_E];
#endif /* _WB_PCIE_DRIVER_H_ */