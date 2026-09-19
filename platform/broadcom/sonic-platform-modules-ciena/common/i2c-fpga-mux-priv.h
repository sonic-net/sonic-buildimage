#ifndef __I2C_FPGA_MUX_PRIV_H
#define __I2C_FPGA_MUX_PRIV_H

#include "sfp_mux_debounce.h"

struct regmap;
struct i2c_fpga_mux_grid;

struct i2c_fpga_mux_priv {
	void __iomem           	    *mux_reg_ptr;
	struct platform_device 	    *pdev;
	unsigned               	     parent_id;
	struct i2c_adapter     	    *parent;
	unsigned               	     reg_mask;    /* mux register mask    */
	unsigned               	     reg_shift;   /* mux register shift   */
	unsigned               	     base_nr;     /* base mux bus number  */
	unsigned               	     n_adap;      /* count of child buses */
	unsigned               	    *values;      /* child mux settings   */
	unsigned                    *revmap;      /* mux reverse mapping  */
	unsigned                     min_chan;    /* minimum reverse map  */
	unsigned                     max_chan;    /* maximum reverse map  */
	u32                    	     parked;      /* mux parking channel  */
	u32                    	     park_mask;   /* mux parking mask     */
	u32                          sda_mask;
	u32                          scl_mask;
	struct i2c_mux_core    	    *mux_core;
	struct rt_mutex        	    *reg_mutex;
	struct resource        	     reg_res;
	struct sfp_mux_debounce	     mux_debounce;
	struct regmap          	    *parent_regmap;
	struct ciena_i2c_err_state **parent_err;
	struct ciena_i2c_err_state  *children_err;
	struct i2c_fpga_mux_grid    *grid;

	struct work_struct           children_work;
	unsigned                   (*reg_read)(void *reg);
	void                       (*reg_write)(unsigned val, void *reg);
#ifdef CONFIG_CIENA_MCEE
	struct ciena_mcee_dev        mceed;
#endif
};

#endif
