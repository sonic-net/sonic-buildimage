#ifndef _FPGA_I2C_OCORES_H
#define _FPGA_I2C_OCORES_H

struct mc_ocores_i2c_platform_data {
	u32 reg_shift; /* register offset shift value */
	u32 reg_io_width; /* register io read/write width */
	u32 clock_khz; /* input clock in kHz */
	u8 num_devices; /* number of devices in the devices list */
	struct i2c_board_info const *devices; /* devices connected to the bus */
	int nr;                               /* i2c bus num */
};

#endif /* _FPGA_I2C_OCORES_H */
