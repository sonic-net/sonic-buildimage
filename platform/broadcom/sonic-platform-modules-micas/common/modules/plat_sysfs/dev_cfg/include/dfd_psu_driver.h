/*
 * A header definition for dfd_psu_driver driver
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

#ifndef _DFD_PSU_DRIVER_H_
#define _DFD_PSU_DRIVER_H_

int dfd_get_psu_present_status(unsigned int psu_index);

int dfd_get_psu_output_status(unsigned int psu_index);

int dfd_get_psu_alert_status(unsigned int psu_index);

#endif /* _DFD_PSU_DRIVER_H_ */
