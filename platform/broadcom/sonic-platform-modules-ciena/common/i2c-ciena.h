#ifndef CIENA_I2C_H
#define CIENA_I2C_H

#include <linux/types.h>

/*----------------------------------------------------------------------------*/
#define CIENA_I2C_DRIVER_NAME "i2c-ciena"

/*----------------------------------------------------------------------------*/
struct regmap;
struct ciena_i2c_err_state;

struct ciena_i2c_info {
	bool                        shared_io;
	bool                        little_endian;
	bool                        no_watch;
	int                         bus_number;
	unsigned int                reg_width;
	unsigned int                reg_offset;
	unsigned int                reg_gap;
	unsigned int                sw_if_sel;
	unsigned int                clock_freq;
	unsigned int                num_board_info;
	const struct i2c_board_info *board_info;
	unsigned int                num_deferred_info;
	const struct i2c_board_info *deferred_info;
	const char                  *name;
	unsigned int                i2c_timeout;
	unsigned int                pre_start_gap;
	struct regmap               *parent_regmap;
	struct ciena_i2c_err_state **err_state;
};
#endif /* CIENA_I2C_H */
