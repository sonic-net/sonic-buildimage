#ifndef _SIRILX_COMMON_H_INCLUDED
#define _SIRILX_COMMON_H_INCLUDED

#include <linux/io.h>
#include <linux/ioport.h>

#define CIENA_SIRIL_MSI_INDEX 0

#define _CIENA_SMB_REG(fPGA, iD, oFF)				\
	[CIENA_SMB_REG_ ## iD] = fPGA ## _REG_OFFSET(oFF)

#define DECLARE_DIAG_I2C_REGS(nAME, fPGA, pREFIX, sWdONEpREFIX, wRrDn)	       \
	static const unsigned						       \
	nAME[CIENA_SMB_REG_MAX] =  {					       \
		_CIENA_SMB_REG(fPGA, CTRL,				       \
			       fPGA ## _ ## pREFIX ## _DIAG_CTRL),	       \
		_CIENA_SMB_REG(fPGA, 4X,				       \
			       fPGA ## _ ## pREFIX ## _DIAG_4X),	       \
		_CIENA_SMB_REG(fPGA, DEVADDR,				       \
			       fPGA ## _ ## pREFIX ## _DIAG_DEVADDR),	       \
		_CIENA_SMB_REG(fPGA, DATAADDR,				       \
			       fPGA ## _ ## pREFIX ## _DIAG_DATAADDR),	       \
		_CIENA_SMB_REG(fPGA, DATA_VLD,				       \
			       fPGA ## _ ## pREFIX ## _DIAG_DATA_VLD),	       \
		_CIENA_SMB_REG(fPGA, DEBUG,				       \
			       fPGA ## _ ## pREFIX ## _DIAG_DEBUG),	       \
		_CIENA_SMB_REG(fPGA, DATA_WR,				       \
			       fPGA ## _ ## pREFIX ## _DIAG_DATA_WR ## wRrDn), \
		_CIENA_SMB_REG(fPGA, DATA_RD,				       \
			       fPGA ## _ ## pREFIX ## _DIAG_DATA_RD ## wRrDn), \
		_CIENA_SMB_REG(fPGA, NO_DATAADDR,			       \
			       fPGA ## _ ## pREFIX ## _DIAG_CTRL),	       \
		_CIENA_SMB_REG(fPGA, I2C_DONE,				       \
			       fPGA ## _ ## sWdONEpREFIX ## _SW_DONE),	       \
	};

#define DECLARE_DIAG_I2C_OFFSETS(nAME, fPGA, pREFIX, sWdONEpREFIX)	\
	DECLARE_DIAG_I2C_REGS(nAME, fPGA, pREFIX, sWdONEpREFIX, 0)

static const int siril_spi_master_fifo_depth = 256;

struct siril_id {
	u32  fpga_id;
	u32  major;
	u32  minor;
	u32  build;
	u32  date;
};

enum siril_i2c_bus_id {
	siril_ofpga_i2c_bus_id                 = 20,
	siril_main_i2c_bus_id                  = 30,
	siril_idp_i2c_bus_id                   = 31,
	siril_ps_i2c_bus_id                    = 32,
	siril_zl_i2c_bus_id                    = 33,
	siril_optics_i2c0_bus_id               = 40,
	siril_optics_i2c1_bus_id               = 41,
	siril_optics_i2c2_bus_id               = 42,
	siril_optics_i2c3_bus_id               = 43,
	siril_optics_i2c4_bus_id               = 44,
	siril_optics_hs1_sel_bus_id            = 48,
	siril_optics_hs_sel_bus_id             = 49,
	siril_pwra_i2c_bus_id                  = 50,
	siril_pwrb_i2c_bus_id                  = 51,
	siril_serdes_i2c_bus_id                = 52,
	siril_q2c_pex_bus_id                   = 53,
	siril_pwrgd_i2c_bus_id                 = 54,
	siril_pmbus1_i2c_bus_id                = 55,
	siril_pmbus2_i2c_bus_id                = 56,
	siril_rtc_i2c_bus_id                   = 57,
	siril_cpld_i2c_bus_id                  = 58,
	siril_pfpga_i2c_bus_id                 = 59,
	siril_owi_i2c_bus_id                   = 60,
	siril_usb_i2c_bus_id                   = 61,
	siril_th3_pmb_i2c_bus_id               = 70,
	siril_mezz1_i2c_bus_id                 = 71,
	siril_mezz2_i2c_bus_id                 = 72,
	siril_mezz1_pmb_i2c_bus_id             = 73,
	siril_mezz2_pmb_i2c_bus_id             = 74,
	siril_si5345_i2c_bus_id                = 75,
	siril_q2a_pmb_i2c_bus_id               = 76,
	siril_ofpga_pmb_i2c_bus_id             = 77,
	siril_1v8_pmb_i2c_bus_id               = 78,
	siril_fru1_i2c_bus_id                  = 81,
	siril_fru2_i2c_bus_id                  = 82,
	siril_fru3_i2c_bus_id                  = 83,
	siril_fru4_i2c_bus_id                  = 84,
	siril_fruxp1_i2c_bus_id                = 86,
	siril_fruxp2_i2c_bus_id                = 87,
	siril_fruxp3_i2c_bus_id                = 88,
	siril_fruxp4_i2c_bus_id                = 89,
	siril_ioexp0_i2c_bus_id                = 90,
	siril_ioexp1_i2c_bus_id                = 91,
	siril_ioexp2_i2c_bus_id                = 92,
	siril_ioexp3_i2c_bus_id                = 93,
	siril_ioexp4_i2c_bus_id                = 94,
	siril_ioexp5_i2c_bus_id                = 95,
	siril_optics_fru1_hs_sel_bus_id        = 96,
	siril_optics_fru2_hs_sel_bus_id        = 97,
	siril_adc_i2c_bus_id                   = 98,
	siril_bfpga_adc_i2c_bus_id             = 99,
	siril_optics_sfp01_bus_id              = 101,
	siril_fru1m_i2c_bus_id                 = 201,
	siril_fru2m_i2c_bus_id                 = 221,
	siril_optics_qsfp01_bus_id             = 301,
	siril_optics_hs_qsfp01_bus_id          = 401,
	siril_optics_fru1_hs_qsfp01_bus_id     = 501,
	siril_optics_fru2_hs_qsfp01_bus_id     = 521,
	siril_optics_ctrl_i2c_bus_id           = 940,
	siril_pwrgd_sw_if_sel_i2c_bus_id       = 950,
	siril_pwrgd_sw_override_i2c_bus_id     = 951,
	siril_q2c_pex_sw_if_sel_i2c_bus_id     = 952,
	siril_q2c_pex_sw_override_i2c_bus_id   = 953,
	siril_psu_mux_i2c_bus_id               = 960,
	siril_adc_mux_i2c_bus_id               = 980,
};

struct siril_i2c_mux {
	const char                     *name;
	const struct i2c_fpga_mux_info *info;
	const struct resource           res;
};

struct ciena_i2c_err_state;

struct siril_i2c_master {
	const char                    *driver_name;
	const char                    *adapter_name;
	bool                           is_smbus;
	bool                           no_watch;
	int                            bus_number;
	int                            reg_width;
	int                            reg_offset;
	int                            reg_gap;
	int                            num_slaves;
	int                            num_deferred;
	int                            i2c_timeout;
	unsigned                       pre_start_gap;
	unsigned                       deferred_children_delay_ms;
	const unsigned                *offsets;
	const struct i2c_board_info   *slaves;
	const struct i2c_board_info   *deferred;
	int                            num_muxes;
	const struct siril_i2c_mux    *muxes;
	const char                    *pin;
	const struct resource          res;
	struct ciena_i2c_err_state    *err;
	const char                    *of_node;
};

struct siril_spi_client {
	struct spi_board_info          board_info;
};

struct siril_spi_master {
	const char                    *name;
	u16                            bus_number;
	u16                            num_chipsel;
	int                            reg_width;
	int                            num_clients;
	const struct siril_spi_client *clients;
	const struct resource          res;
	const char                    *of_node;
	bool                           gpio_chipsel;
};

struct siril_uart_master {
	const char                    *name;
	int                            devid;
	const struct resource          res;
	const char                    *pin;
	const unsigned char           *oflow_str;
	const int                      oflow_sz;
};

struct siril_i2c_pca953x_dir {
	const int                      direction;
	const int                      value;
};

struct siril_i2c_pca953x {
	const char *const                  *names;
	const struct siril_i2c_pca953x_dir *dir;
};

struct siril_reset_controller {
	const struct ciena_fpga_reset_pdata pdata;
	const unsigned                      reg_offset;
};

struct siril_reset_device {
	const  char*                        dev_name;
	struct ciena_sysfs_reset_pdata      pdata;
};

struct siril_fpga_watchdog {
	const unsigned                      wdt_ctl_reg;
	const unsigned                      wdt_clr_reg;
};

struct ciena_siril_extra_pdev {
	const char                         *name;
	int                                 id;
	struct resource                    *res;
	unsigned                            nres;
	void                               *pdata;
	size_t                              psize;
	const char                         *of_node;
	const char                         *driver;
};

struct ciena_siril_led_pdev {
	const char                         *name;
	unsigned int                        reg;
	unsigned int                        mask;
	unsigned int                        val;
	bool                                use_val;
	bool                                invert;
	unsigned int                        blnk;
};

struct ciena_siril_thermal_pdev {
	const char                         *name;
	struct regmap                      *regmap;
	unsigned int                        reg;
	unsigned int                        valid_mask;
	unsigned int                        temp_mask;
	unsigned int                        temp_shift;
	unsigned int                        temp_unsigned;
	unsigned int                        threshold;
};

struct ciena_fan_pdata;

struct sirilx_usb_neuter {
	bool        do_enable;
	const char *whitelist;
};

typedef void (*ciena_cic_fixups)                   (struct regmap *regs, struct device *cic_dev);
typedef struct resource (*ciena_siril_irq_resource)(struct device *dev);
typedef void (*ciena_siril_irq_deresource)         (struct device *dev);
typedef void (*ciena_siril_apply_early_fixups)     (struct regmap *regs);
typedef void (*ciena_siril_sw_i2c_enable)          (struct regmap *regs, bool on);
typedef void (*ciena_siril_resets_init)            (struct regmap *regs);
typedef void (*ciena_siril_ofpga_eeprom_enable)    (struct regmap *regs, bool on);
typedef void (*cfpga_uart_buffer_enable)           (struct regmap *regs, bool on);

struct sirilx_platform_data {
	const resource_size_t                  sirilx_cic_start_offset;
	resource_size_t                        sirilx_cic_end_offset;
	struct generic_cic_config             *sirilx_cic_pdata;
	size_t                                 sirilx_cic_pdata_size;
	ciena_cic_fixups                       sirilx_cic_fixups;
	const char                            *sirilx_cic_of_node;

	ciena_siril_irq_resource               sirilx_irq_resource;
	ciena_siril_irq_deresource             sirilx_irq_deresource;

	ciena_siril_apply_early_fixups         sirilx_apply_early_fixups;

	const struct ciena_siril_extra_pdev   *sirilx_extra_devs;
	size_t                                 sirilx_extra_devs_count;

	struct siril_i2c_master               *sirilx_i2c_masters;
	size_t                                 sirilx_i2c_master_count;
	ciena_siril_sw_i2c_enable              sirilx_sw_i2c_enable;

	const struct ciena_siril_led_pdev     *sirilx_led_devs;
	size_t                                 sirilx_led_dev_count;

	const struct siril_reset_device       *sirilx_resets;
	size_t                                 sirilx_reset_count;
	const struct siril_reset_controller   *sirilx_reset_controllers;
	size_t                                 sirilx_reset_controller_count;
	ciena_siril_resets_init                sirilx_resets_init;

	const struct siril_spi_master         *sirilx_spi_masters;
	size_t                                 sirilx_spi_master_count;
	ciena_siril_ofpga_eeprom_enable        sirilx_ofpga_eeprom_enable;

	const struct ciena_siril_thermal_pdev *sirilx_thermal_devs;
	size_t                                 sirilx_thermal_dev_count;

	const struct ciena_fan_pdata          *sirilx_fan_devs;
	size_t                                 sirilx_fan_dev_count;

	const struct siril_uart_master        *sirilx_uart_masters;
	size_t                                 sirilx_uart_master_count;
	cfpga_uart_buffer_enable               sirilx_uart_buffer_enable;

	const struct siril_fpga_watchdog      *sirilx_watchdog;
	const char                            *sirilx_wdt_irq_pin;

	struct sirilx_usb_neuter              *sirilx_usb_rules;
};


static int siril_resolve_cic_pin(struct regmap *regs,
				 struct device *cic_dev,
				 const char    *pin);

#ifdef __LITTLE_ENDIAN
static inline u32 siril_reg_rd32(void __iomem *r) { return ioread32(r); }
static inline void siril_reg_wr32(u32 v, void __iomem *r) { iowrite32(v, r); }
#else
static inline u32 siril_reg_rd32(void __iomem *r) { return ioread32be(r); }
static inline void siril_reg_wr32(u32 v, void __iomem *r) { iowrite32be(v, r); }
#endif

#endif
// vim: sw=8 noet
