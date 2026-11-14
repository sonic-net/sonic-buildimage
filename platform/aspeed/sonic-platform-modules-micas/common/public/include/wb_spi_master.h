/*
 * A header definition for wb_spi_master driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

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