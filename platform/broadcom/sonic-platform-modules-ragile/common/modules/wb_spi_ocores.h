#ifndef __WB_SPI_OCORES_H__
#define __WB_SPI_OCORES_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define SPI_OCORES_DEV_NAME_MAX_LEN (64)

typedef struct spi_ocores_device_s {
    uint32_t bus_num;
    uint32_t big_endian;
    char dev_name[SPI_OCORES_DEV_NAME_MAX_LEN];
    uint32_t reg_access_mode;
    uint32_t dev_base;
    uint32_t reg_shift;
    uint32_t reg_io_width;
    uint32_t clock_frequency;
    uint32_t num_chipselect;
    int device_flag;
} spi_ocores_device_t;

#endif
