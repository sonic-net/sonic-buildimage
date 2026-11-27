#ifndef _I2C_CIENA_SMB_H
#define _I2C_CIENA_SMB_H

struct i2c_board_info;
struct regmap;

#define CIENA_I2C_SMBUS_DRIVER_NAME  "i2c-ciena-smb"

enum ciena_smb_register {
	CIENA_SMB_REG_CTRL = 0,
	CIENA_SMB_REG_4X,
	CIENA_SMB_REG_DEVADDR,
	CIENA_SMB_REG_DATAADDR,
	CIENA_SMB_REG_DATA_VLD,
	CIENA_SMB_REG_DEBUG,
	CIENA_SMB_REG_DATA_WR,
	CIENA_SMB_REG_DATA_RD,
	CIENA_SMB_REG_DATA_RD_MAX,
	CIENA_SMB_REG_NO_DATAADDR,
	CIENA_SMB_REG_I2C_DONE,
	CIENA_SMB_REG_MAX,
};

struct i2c_ciena_smb_config {
	int                          bus_number;
	unsigned                     pre_start_gap;
	unsigned                     timeout_usecs;
	unsigned                     deferred_children_delay_ms;
	const char                  *adap_name;
	const unsigned              *offsets;
	struct ciena_i2c_err_state **err_state;
	int                          num_children;
	const struct i2c_board_info *children;
	int                          num_deferred_children;
	const struct i2c_board_info *deferred_children;
};

#endif
