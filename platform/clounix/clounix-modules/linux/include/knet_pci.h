#ifndef __CLX_PCI_H__
#define __CLX_PCI_H__

#include <linux/device.h>
#include <linux/version.h>
#include "knet_types.h"

#define CLX_PCIE_VENDOR_ID_0 (0x1D9F)
#define CLX_PCIE_VENDOR_ID_1 (0x1F83)

#define CLX_DEVICE_ID_CL8300  (0x8300)
#define CLX_DEVICE_ID_CL8400  (0x8400)
#define CLX_DEVICE_ID_CL8500  (0x8500)
#define CLX_DEVICE_ID_CL8600  (0x8600)
#define CLX_INVALID_DEVICE_ID (0xFFFFFFFF)

#define CLX_DEVICE_IS_NAMCHABARWA(__dev_id__) (CLX_DEVICE_ID_CL8600 == (__dev_id__ & 0xFF00))
#define CLX_DEVICE_IS_KAWAGARBO(__dev_id__)   (CLX_DEVICE_ID_CL8400 == (__dev_id__ & 0xFF00))

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
#define IOREMAP_API(a, b) ioremap_nocache(a, b)
#else
#define IOREMAP_API(a, b) ioremap(a, b)
#endif

int
clx_pci_init(void);
int
clx_pci_deinit(void);

int
clx_read_pci_reg(int unit, uint32_t offset, uint32_t *ptr_data, uint32_t len);
int
clx_write_pci_reg(int unit, uint32_t offset, uint32_t *ptr_data, uint32_t len);

#endif // __CLX_PCI_H__