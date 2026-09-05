/* -------------------------------------------------------------------- 

	 * A hwmon driver for the CIG cs6436-56P
	 *
	 * Copyright (C) 2018 Cambridge, Inc.
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

/* --------------------------------------------------------------------	*/


#ifndef _LINUX_I2C_ALGO_LPC_H
#define _LINUX_I2C_ALGO_LPC_H

struct i2c_algo_lpc_data {
	void *data;		/* private data for lolevel routines	*/
	void (*setlpc) (void *data, int ctl, int val);
	int  (*getlpc) (void *data, int ctl);
	int  (*getown) (void *data);
	int  (*getclock) (void *data);
	void (*waitforpin) (void *data);

	int (*xfer_begin) (void *data);
	int (*xfer_end) (void *data);

	/* Multi-master lost arbitration back-off delay (msecs)
	 * This should be set by the bus adapter or knowledgable client
	 * if bus is multi-mastered, else zero
	 */
	unsigned long lab_mdelay;
};

int lpc_add_iic_bus(struct i2c_adapter * adap, unsigned int id);

#endif /* _LINUX_I2C_ALGO_LPC_H */
