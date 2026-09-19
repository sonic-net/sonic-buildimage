#ifndef _RESET_FPGA_INCLUDED
#define _RESET_FPGA_INCLUDED

#define RESET_FPGA_DRIVER_NAME "reset-fpga"
#define RESET_GPIO_DRIVER_NAME "reset-gpio"
#define RESET_IPMI_DRIVER_NAME "reset-ipmi"

struct i2c_client;
struct regmap;

enum ciena_fpga_bit_flags {
	ciena_fpga_bit_inversed = 0x80000000,
};

struct ciena_fpga_reset_pdata {
	unsigned           reg_size;
	unsigned           shared_io;
	unsigned           negative;
	unsigned           use_raw_value;
	unsigned           ipmi;
	struct i2c_client *i2cdev;
	unsigned           i2creg;
	struct regmap     *parent_regmap;
	const char        *name;
	unsigned           num_names;
	const char       **reset_names;
};

#endif
