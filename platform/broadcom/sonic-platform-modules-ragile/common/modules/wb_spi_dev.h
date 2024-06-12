#ifndef __WB_SPI_DEV_H__
#define __WB_SPI_DEV_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define SPI_DEV_NAME_MAX_LEN (64)

typedef struct spi_dev_device_s {
    char spi_dev_name[SPI_DEV_NAME_MAX_LEN];
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
} spi_dev_device_t;

#endif
