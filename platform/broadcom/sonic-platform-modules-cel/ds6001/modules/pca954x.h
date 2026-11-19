/* SPDX-License-Identifier: GPL-2.0 */
/*
 * pca954x.h - I2C multiplexer/switch support
 *
 * Copyright (c) 2008-2009 Rodolfo Giometti <giometti@linux.it>
 * Copyright (c) 2008-2009 Eurotech S.p.A. <info@eurotech.it>
 * Michael Lawnick <michael.lawnick.ext@nsn.com>
 *
 */


#ifndef _LINUX_I2C_PCA954X_H
#define _LINUX_I2C_PCA954X_H

/* Platform data for the PCA954x I2C multiplexers */

/* Per channel initialisation data:
 * @adap_id: bus number for the adapter. 0 = don't care
 * @deselect_on_exit: set this entry to 1, if your H/W needs deselection
 *                    of this channel after transaction.
 *
 */
struct pca954x_platform_mode {
	int		adap_id;
	unsigned int	deselect_on_exit:1;
	unsigned int	class;
};

/* Per mux/switch data, used with i2c_register_board_info */
struct pca954x_platform_data {
	struct pca954x_platform_mode *modes;
	int num_modes;
};

#endif /* _LINUX_I2C_PCA954X_H */
