#ifndef __WB_SPI_MASTER_H__
#define __WB_SPI_MASTER_H__

#include <linux/types.h>
#include <linux/spi/spi.h>

/**
 * wb_spi_master_busnum_to_master - look up master associated with bus_num
 * @bus_num: the master's bus number
 * Context: can sleep
 *
 * Return: the SPI master structure on success, else NULL.
 */
struct spi_controller *wb_spi_master_busnum_to_master(u16 bus_num);

#endif  /* #ifndef __WB_SPI_MASTER_H__ */