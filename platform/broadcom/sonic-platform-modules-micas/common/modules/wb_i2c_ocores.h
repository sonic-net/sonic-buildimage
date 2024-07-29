#ifndef __WB_I2C_OCORES_H__
#define __WB_I2C_OCORES_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define I2C_OCORES_DEV_NAME_MAX_LEN (64)

typedef struct i2c_ocores_device_s {
    uint32_t big_endian;
    char dev_name[I2C_OCORES_DEV_NAME_MAX_LEN];
    int adap_nr;
    uint32_t dev_base;
    uint32_t reg_shift;
    uint32_t reg_io_width;
    uint32_t ip_clock_khz;
    uint32_t bus_clock_khz;
    uint32_t reg_access_mode;

    uint32_t irq_type;
    uint32_t irq_offset;
    uint32_t pci_domain;
    uint32_t pci_bus;
    uint32_t pci_slot;
    uint32_t pci_fn;
    int device_flag;
} i2c_ocores_device_t;

#endif
