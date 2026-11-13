#ifndef __I2C_FPGA_MUX_H
#define __I2C_FPGA_MUX_H

#define I2C_FPGA_MUX_DRIVER_NAME "i2c_fpga_mux"

struct regmap;
struct ciena_i2c_err_state;

struct i2c_fpga_mux_info {
	int                            shared_io;
	int                            parent_adapter_id;
	int                            children_base_id;
	int                            num_children;
	int                            reg_width;
	unsigned                       reg_mask;
	u32                            parked;
	u32                            park_mask; /* use when parked == 0 */
	u32                            select_delay_usec;
	u32                            deselect_delay_usec;
	unsigned                      *children_muxsel;
	struct regmap                 *parent_regmap;
	struct ciena_i2c_err_state   **parent_err;
	const int                     *bus_offsets;

	u32                            scl_mask;
	u32                            sda_mask;

	int	                       num_mux_slaves;
	const struct i2c_board_info ***mux_slaves;
	int	                       num_deferred_slaves;
	const struct i2c_board_info ***deferred_slaves;
};

#endif
