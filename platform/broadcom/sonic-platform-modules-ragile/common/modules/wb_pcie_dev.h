#ifndef __WB_PCIE_DEV_H__
#define __WB_PCIE_DEV_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

#define UPG_TYPE 'U'
#define GET_FPGA_UPG_CTL_BASE              _IOR(UPG_TYPE, 0, int)
#define GET_FPGA_UPG_FLASH_BASE            _IOR(UPG_TYPE, 1, int)

#define PCI_DEV_NAME_MAX_LEN (64)

typedef struct pci_dev_device_s {
    char pci_dev_name[PCI_DEV_NAME_MAX_LEN];
    int pci_domain;
    int pci_bus;
    int pci_slot;
    int pci_fn;
    int pci_bar;
    int bus_width;
    int upg_ctrl_base;
    int upg_flash_base;
    int device_flag;
} pci_dev_device_t;

#endif
