#include <linux/pci.h>
#include <linux/rudra40_regmap.h>
#include <linux/gpio/machine.h>
#include <linux/i2c.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_data/pca953x.h>
#include <linux/spi/flash.h>

#include "sirilx_common.h"

#define CIENA_SIRIL_DRIVER_NAME          "uio-rudra40-pci"
#define CIENA_SIRIL_DRIVER_VERSION       "1.0.0"
#define CIENA_SIRIL_NUM_BARS             1

static const char *ciena_siril_bar_names[CIENA_SIRIL_NUM_BARS] = {
	"Rudra40 Registers",
};

enum {
	PCA95xx_INPUT  = 1,
	PCA95xx_OUTPUT = 2,
};

static const struct pci_device_id ciena_siril_device_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_CIENA, PCI_DEVICE_ID_CIENA_RUDRA40) },
	{ 0, }
};

/***********************************************************************
 * rudra40 pwd good i2c slaves (54)
 */
//	CFPGA_ADC_IOEXP_PG_SCL/SDA - Miscellaneous I2C devices and their addresses
//		RUDRA40_MORE_I2C_PWRGD_SW_CTRL RUDRA40_MORE_I2C_PWRGD_I2C_SW_OVERRIDE

//	Device		I2C Address	Signals connected to Serial expanders
//	TCA9555RTW	0x20		Power good signals from main board power supplies
//	TCA9555RTW	0x21		Power good signals from main board power supplies
//	MAX1139EEE	0x35		Refer to '12V Subsystem Power Monitoring section' of this document
#ifdef CIENA_I2C_PWRGD_OVERRIDE
/* By default the FPGA auto-polls the power good devices below. */
static const struct i2c_board_info rudra40_more_i2c_pwrgd_slaves[] = {
	{
		.type       = "pca9555",
		.addr       = 0x20,
	},
	{
		.type       = "pca9555",
		.addr       = 0x21,
	},
	{
		.type       = "max1139",
		.addr       = 0x35,
	},
};
#endif

/***********************************************************************
 * rudra40 j2c ioexp i2c slaves (70)
 */
//	CFPGA_J2CP_IOEXP_LOCK_SCL/SDA - Miscellaneous I2C devices and their addresses
//		RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL RUDRA40_MORE_I2C_J2C_IOEXP_I2C_SW_OVERRIDE

//	Device		I2C Address	Signals connected to Serial expanders
//	TCA9555RTW	0x20		BS, C, FAB, NIF, U, TS_PLL lock indication signal of both J2C+ A & B devices
//	J2C+ 'A' (U4)	0x44		This I2C can be used for PCIe debug & QSPI programming
#ifdef CIENA_I2C_J2C_IOEXP_OVERRIDE
/* By default the FPGA auto-polls the J2C io expander. */
static const struct i2c_board_info rudra40_more_i2c_j2c_ioexp_slaves[] = {
	{
		.type       = "pca9555",
		.addr       = 0x20,
	},
};
#endif

/***********************************************************************
 * rudra40 main i2c slaves (30) [ciena-i2c.6.auto/i2c-30]
 */
//	MB_TIMING_I2C_SCL/SDA - Miscellaneous I2C devices and their addresses
//		RUDRA40_MAIN_I2C_SW_CTRL

//	Device		I2C Address	Signals connected to Serial expanders
//	IDP EEPROM      0x54            24LC32A
//	ZL30733(LDG1)	0x70		BS, C, FAB, NIF, U, TS_PLL lock indication signal of both J2C+(W/O FABRIC)
//	SI5345_1	0x74		This is used to generate free running clocks to both J2C+(W/O FABRIC)
//	SI5345_2	0x75		This is used to generate fab and NIF serdes reference clock for J2C+(W/O FABRIC) 'A'
//	SI52146		0x6B		PCIe clock generator
//	EEPROM		0x56/0x57	EEPROM to store HBM tunning parameters for both J2C+(W/O FABRIC)
//	EEPROM		0x57		EEPROM to store HBM tunning parameters for both J2C+(W/O FABRIC)
//	0x56 0x57, 24CM01, 1Mb 16-bit address, 256 byte block read

static const struct property_entry i2c_j2cp_eeprom[] = {
	PROPERTY_ENTRY_U32("size", 0x20000),
	PROPERTY_ENTRY_U32("pagesize", 256),
	PROPERTY_ENTRY_U32("address-width", 16),
	{ }
};

static const struct software_node i2c_j2cp_eeprom_node = {
	.properties = i2c_j2cp_eeprom,
};

static const struct i2c_board_info rudra40_main_i2c_slaves[] = {
	{
		.type       = "zl3073x",
		.addr       = 0x70,
	},
	{
		.type       = "si5345",
		.addr       = 0x74,
	},
	{
		.type       = "si5345",
		.addr       = 0x75,
	},
	{
		.type       = "si52146",
		.addr       = 0x6b,
	},
	{
		.type       = "24c1024",
		.addr       = 0x56,
		.dev_name   = "bcm-dram-tuning",
		.swnode     = &i2c_j2cp_eeprom_node,
	},
};

//	RUDRA40_MORE_I2C_PMBUS1_SW_CTRL RUDRA40_MORE_I2C_PMBUS1_I2C_SW_OVERRIDE
//	SIRIL_PMBUS1_SCK/SDA_R - PMBUS interface from Rudra and devices connected
//	Device		I2C Address	Max Speed supported	Device Use (Gen voltage rail)
//	TPS536C7	0x5E		1MHz			Generating J2C+ 'A' Core voltage POL (+0.75V_J2CPA_VDDC_500A)
//	TPS536C7	0x60		1MHz			Generating J2C+ 'A' RTVDD0P75 voltage POL (+0.75V_J2CPA_RTVDD0P75_85A)
//	TPS536C7	0x64		1MHz			Generating 3.3V_QSFPDD voltage POL (+3.3V_QSFPDD_G1 & +3.3V_QSFPDD_G2)

//	RUDRA40_MORE_I2C_PMBUS2_SW_CTRL RUDRA40_MORE_I2C_PMBUS2_I2C_SW_OVERRIDE
//	SIRIL_PMBUS2_SCK/SDA_R - PMBUS interface from Rudra and devices connected
//	TPS549D22	0x10		1MHz			Generating J2C+ 'A' HBM0 voltage POL (+1.2V_J2CPA_HBM0_VDD1P2_15A)
//	TPS549D22	0x11		1MHz			Generating J2C+ 'A' HBM1 voltage POL (+1.2V_J2CPA_HBM1_VDD1P2_15A)

/***********************************************************************
 *  * rudra40 sfp mux settings
 *   */
unsigned rudra40_sfp_mux_values[] = {
	/* 01: 00.00.000 */	0b0000000,	/* 0x00 */
	/* 02: 00.01.000 */	0b0001000,	/* 0x08 */
	/* 03: 00.00.001 */	0b0000001,	/* 0x01 */
	/* 04: 00.01.001 */	0b0001001,	/* 0x09 */
	/* 05: 00.00.010 */	0b0000010,	/* 0x02 */
	/* 06: 00.01.010 */	0b0001010,	/* 0x0a */
	/* 07: 01.00.000 */	0b0100000,	/* 0x20 */
	/* 08: 01.01.000 */	0b0101000,	/* 0x28 */
	/* 09: 01.00.001 */	0b0100001,	/* 0x21 */
	/* 10: 01.01.001 */	0b0101001,	/* 0x29 */
	/* 11: 00.10.101 */	0b0010101,	/* 0x15 */
	/* 12: 00.11.101 */	0b0011101,	/* 0x1d */
	/* 13: 00.10.110 */	0b0010110,	/* 0x16 */
	/* 14: 00.11.110 */	0b0011110,	/* 0x1e */
	/* 15: 00.10.111 */	0b0010111,	/* 0x17 */
	/* 16: 00.11.111 */	0b0011111,	/* 0x1f */
	/* 17: 01.10.101 */	0b0110101,	/* 0x35 */
	/* 18: 01.11.101 */	0b0111101,	/* 0x3d */
	/* 19: 01.10.110 */	0b0110110,	/* 0x36 */
	/* 20: 01.11.110 */	0b0111110,	/* 0x3e */
	/* 21: 10.00.001 */	0b1000001,	/* 0x41 */
	/* 22: 10.01.001 */	0b1001001,	/* 0x49 */
	/* 23: 10.00.010 */	0b1000010,	/* 0x42 */
	/* 24: 10.01.010 */	0b1001010,	/* 0x4a */
	/* 25: 11.00.000 */	0b1100000,	/* 0x60 */
	/* 26: 11.01.000 */	0b1101000,	/* 0x68 */
	/* 27: 11.00.001 */	0b1100001,	/* 0x61 */
	/* 28: 11.01.001 */	0b1101001,	/* 0x69 */
	/* 29: 11.00.010 */	0b1100010,	/* 0x62 */
	/* 30: 11.01.010 */	0b1101010,	/* 0x6a */
	/* 31: 10.10.110 */	0b1010110,	/* 0x56 */
	/* 32: 10.11.110 */	0b1011110,	/* 0x5e */
	/* 33: 10.10.111 */	0b1010111,	/* 0x57 */
	/* 34: 10.11.111 */	0b1011111,	/* 0x5f */
	/* 35: 11.10.101 */	0b1110101,	/* 0x75 */
	/* 36: 11.11.101 */	0b1111101,	/* 0x7d */
	/* 37: 11.10.110 */	0b1110110,	/* 0x76 */
	/* 38: 11.11.110 */	0b1111110,	/* 0x7e */
	/* 39: 11.10.111 */	0b1110111,	/* 0x77 */
	/* 40: 11.11.111 */	0b1111111,	/* 0x7f */
};

/***********************************************************************
 * rudra40 qsfp mux settings
 */
unsigned rudra40_qsfp_mux_values[] = {
	/* 41: 01.00.010 */	0b0100010,	/* 0x22 */
	/* 42: 01.01.010 */	0b0101010,	/* 0x2a */
	/* 43: 10.00.000 */	0b1000000,	/* 0x40 */
	/* 44: 10.01.000 */	0b1001000,	/* 0x48 */
	/* 45: 01.10.111 */	0b0110111,	/* 0x37 */
	/* 46: 01.11.111 */	0b0111111,	/* 0x3f */
	/* 47: 10.10.101 */	0b1010101,	/* 0x55 */
	/* 48: 10.11.101 */	0b1011101,	/* 0x5d */
};

/***********************************************************************
 * rudra40 1MHz qsfp mux setting
 */
const u32 rudra40_default_sfp_i2c_clk_div = 0xf9;
unsigned rudra40_1mhz_sel_values[] = {
	(( rudra40_default_sfp_i2c_clk_div / 10 ) <<
	 RUDRA40_OPTICS_I2C0_DIAG_4X_CLK_DIV___SHIFT),
};

/***********************************************************************
 * i2c sfp mux
 */
static const struct i2c_fpga_mux_info rudra40_sfp_mux_info = {
	.shared_io         = 1,
	.parent_adapter_id = siril_optics_i2c0_bus_id,
	.children_base_id  = siril_optics_sfp01_bus_id,
	.num_children      = ARRAY_SIZE(rudra40_sfp_mux_values),
	.children_muxsel   = rudra40_sfp_mux_values,
};

/***********************************************************************
 * i2c qsfp mux
 */
static const struct i2c_fpga_mux_info rudra40_qsfp_mux_info = {
	.shared_io         = 1,
	.parent_adapter_id = siril_optics_i2c0_bus_id,
	.children_base_id  = siril_optics_qsfp01_bus_id,
	.num_children      = ARRAY_SIZE(rudra40_qsfp_mux_values),
	.children_muxsel   = rudra40_qsfp_mux_values,
};

/***********************************************************************
 * 1MHz speed selector
 */
static const struct i2c_fpga_mux_info rudra40_1mhz_sel_mux_info = {
	.shared_io         = 1,
	.parent_adapter_id = siril_optics_i2c0_bus_id,
	.children_base_id  = siril_optics_hs_sel_bus_id,
	.num_children      = ARRAY_SIZE(rudra40_1mhz_sel_values),
	.reg_mask          = RUDRA40_OPTICS_I2C0_DIAG_4X_CLK_DIV___MASK,
	.parked            = (rudra40_default_sfp_i2c_clk_div <<
			      RUDRA40_OPTICS_I2C0_DIAG_4X_CLK_DIV___SHIFT),
	.children_muxsel   = rudra40_1mhz_sel_values,
};

/***********************************************************************
 * 1MHz qsfp mux
 */
static const struct i2c_fpga_mux_info rudra40_1mhz_qsfp_mux_info = {
	.shared_io         = 1,
	.parent_adapter_id = siril_optics_hs_sel_bus_id,
	.children_base_id  = siril_optics_hs_qsfp01_bus_id,
	.num_children      = ARRAY_SIZE(rudra40_qsfp_mux_values),
	.children_muxsel   = rudra40_qsfp_mux_values,
};

static const struct siril_i2c_mux rudra40_sfp_muxes[] = {
	{ .name = I2C_FPGA_MUX_DRIVER_NAME,
	  .info = &rudra40_sfp_mux_info,
	  .res  = { .start = RUDRA40_REG_OFFSET(RUDRA40_OPTICS_I2C_MUX_SEL),
		    .end   = offsetofend(struct Rudra40_dev_reg,
					 RUDRA40_OPTICS_I2C_MUX_SEL) - 1,
		    .name  = "rudra40_sfp_i2c_mux_sel-regs",
		    .flags = IORESOURCE_MEM, }, },
	{ .name = I2C_FPGA_MUX_DRIVER_NAME,
	  .info = &rudra40_qsfp_mux_info,
	  .res  = { .start = RUDRA40_REG_OFFSET(RUDRA40_OPTICS_I2C_MUX_SEL),
		    .end   = offsetofend(struct Rudra40_dev_reg,
					 RUDRA40_OPTICS_I2C_MUX_SEL) - 1,
		    .name  = "rudra40_qsfp_i2c_mux_sel-regs",
		    .flags = IORESOURCE_MEM, }, },
	{   .name = I2C_FPGA_MUX_DRIVER_NAME,
	    .info = &rudra40_1mhz_sel_mux_info,
	    .res  = { .start = RUDRA40_REG_OFFSET(RUDRA40_OPTICS_I2C0_DIAG_4X),
		      .end   = offsetofend(struct Rudra40_dev_reg,
					   RUDRA40_OPTICS_I2C0_DIAG_4X) - 1,
		      .name  = "rudra40_1mhz_sel-regs",
		      .flags = IORESOURCE_MEM, }, },
	{   .name = I2C_FPGA_MUX_DRIVER_NAME,
	    .info = &rudra40_1mhz_qsfp_mux_info,
	    .res  = { .start = RUDRA40_REG_OFFSET(RUDRA40_OPTICS_I2C_MUX_SEL),
		      .end   = offsetofend(struct Rudra40_dev_reg,
					   RUDRA40_OPTICS_I2C_MUX_SEL) - 1,
		      .name  = "rudra40_1mhz_qsfp_i2c_mux_sel-regs",
		      .flags = IORESOURCE_MEM|IORESOURCE_BUSY, }, },
};

/***********************************************************************
 * rudra40 i2c masters
 */
#define RUDRA40_I2C_REG(_id, _off)				\
	[CIENA_SMB_REG_ ## _id] = RUDRA40_REG_OFFSET(_off)

static const unsigned rudra40_optics_i2c_offsets[CIENA_SMB_REG_MAX] = {
	RUDRA40_I2C_REG(CTRL,        RUDRA40_OPTICS_I2C0_DIAG_CTRL),
	RUDRA40_I2C_REG(4X,          RUDRA40_OPTICS_I2C0_DIAG_4X),
	RUDRA40_I2C_REG(DEVADDR,     RUDRA40_OPTICS_I2C0_DIAG_DEVADDR),
	RUDRA40_I2C_REG(DATAADDR,    RUDRA40_OPTICS_I2C0_DIAG_DATAADDR),
	RUDRA40_I2C_REG(DATA_VLD,    RUDRA40_OPTICS_I2C0_DIAG_DATA_VLD),
	RUDRA40_I2C_REG(DEBUG,       RUDRA40_OPTICS_I2C0_DIAG_DEBUG),
	RUDRA40_I2C_REG(DATA_WR,     RUDRA40_OPTICS_I2C0_DIAG_DATA_WR0),
	RUDRA40_I2C_REG(DATA_RD,     RUDRA40_OPTICS_I2C0_DIAG_DATA_RD0),
	RUDRA40_I2C_REG(NO_DATAADDR, RUDRA40_OPTICS_I2C0_DIAG_CTRL),
	RUDRA40_I2C_REG(I2C_DONE,    RUDRA40_OPTICS_I2C0_SW_DONE),
};

static const unsigned rudra40_main_i2c_offsets[CIENA_SMB_REG_MAX] = {
	RUDRA40_I2C_REG(CTRL,        RUDRA40_MAIN_I2C_DIAG_CTRL),
	RUDRA40_I2C_REG(4X,          RUDRA40_MAIN_I2C_DIAG_4X),
	RUDRA40_I2C_REG(DEVADDR,     RUDRA40_MAIN_I2C_DIAG_DEVADDR),
	RUDRA40_I2C_REG(DATAADDR,    RUDRA40_MAIN_I2C_DIAG_DATAADDR),
	RUDRA40_I2C_REG(DATA_VLD,    RUDRA40_MAIN_I2C_DIAG_DATA_VLD),
	RUDRA40_I2C_REG(DEBUG,       RUDRA40_MAIN_I2C_DIAG_DEBUG),
	RUDRA40_I2C_REG(DATA_WR,     RUDRA40_MAIN_I2C_DIAG_DATA_WR0),
	RUDRA40_I2C_REG(DATA_RD,     RUDRA40_MAIN_I2C_DIAG_DATA_RD0),
	RUDRA40_I2C_REG(NO_DATAADDR, RUDRA40_MAIN_I2C_DIAG_CTRL),
	RUDRA40_I2C_REG(I2C_DONE,    RUDRA40_MAIN_I2C_SW_DONE),
};

static struct siril_i2c_master rudra40_i2c_masters[] = {
	/* RUDRA40_OPTICS_I2C0 */
	{ .driver_name   = CIENA_I2C_SMBUS_DRIVER_NAME,
	  .adapter_name  = "sfp0",
	  .bus_number    = siril_optics_i2c0_bus_id,
	  .is_smbus      = true,
	  .pre_start_gap = 40, /* fussy 40-µsec delay before a START */
	  .reg_width     = sizeof(u32),
	  .num_muxes     = ARRAY_SIZE(rudra40_sfp_muxes),
	  .muxes         = rudra40_sfp_muxes,
	  .pin           = "SW_I2C_SFP_DONE",
	  .offsets       = rudra40_optics_i2c_offsets,
	  .res           = { .start = RUDRA40_REG_OFFSET(RUDRA40_OPTICS_I2C0_DIAG_CTRL),
			    .end   = offsetofend(struct Rudra40_dev_reg,
						 RUDRA40_OPTICS_I2C0_DIAG_DATA_RD63) - 1,
			    .name  = "rudra40_optics_qsfp_i2c0-regs",
			    .flags = IORESOURCE_REG, }, },
	/* RUDRA40_MAIN_I2C */
	{ .driver_name   = CIENA_I2C_SMBUS_DRIVER_NAME,
	  .adapter_name  = "rudra40_main_i2c",
	  .bus_number    = siril_main_i2c_bus_id,
	  .is_smbus      = true,
	  .reg_width     = sizeof(u32),
	  .num_slaves    = ARRAY_SIZE(rudra40_main_i2c_slaves),
	  .slaves        = rudra40_main_i2c_slaves,
	  .pin           = "SW_I2C_MB_DONE",
	  .offsets       = rudra40_main_i2c_offsets,
	  .res           = { .start = RUDRA40_REG_OFFSET(RUDRA40_MAIN_I2C_DIAG_CTRL),
			    .end   = offsetofend(struct Rudra40_dev_reg,
						 RUDRA40_MAIN_I2C_DIAG_DATA_RD63) - 1,
			    .name  = "rudra40_main_i2c-regs",
			    .flags = IORESOURCE_REG, }, },
	/* RUDRA40_MORE_I2C_PWRGD */
	{ .driver_name  = CIENA_I2C_DRIVER_NAME,
	  .adapter_name = "rudra40_more_i2c_pwrgd",
	  .bus_number   = siril_pwrgd_i2c_bus_id,
	  .reg_width    = sizeof(u32),
#ifdef CIENA_I2C_PWRGD_OVERRIDE
	  .num_slaves   = ARRAY_SIZE(rudra40_more_i2c_pwrgd_slaves),
	  .slaves       = rudra40_more_i2c_pwrgd_slaves,
#endif
	  .pin          = "SW_I2C_PWRGD_DONE",
	  .res          = { .start = RUDRA40_REG_OFFSET(RUDRA40_MORE_I2C_PWRGD_SW_CTRL),
			    .end   = offsetofend(struct Rudra40_dev_reg,
						 RUDRA40_MORE_I2C_PWRGD_SW_DONE) - 1,
			    .name  = "rudra40_more_i2c_pwrgd-regs",
			    .flags = IORESOURCE_MEM, }, },
	/* RUDRA40_MORE_I2C_J2C_IOEXP */
	{ .driver_name  = CIENA_I2C_DRIVER_NAME,
	  .adapter_name = "rudra40_more_i2c_j2c_ioexp",
	  .bus_number   = siril_th3_pmb_i2c_bus_id,
	  .reg_width    = sizeof(u32),
#ifdef CIENA_I2C_J2C_IOEXP_OVERRIDE
	  .num_slaves   = ARRAY_SIZE(rudra40_more_i2c_j2c_ioexp_slaves),
	  .slaves       = rudra40_more_i2c_j2c_ioexp_slaves,
#endif
	  .pin          = "SW_I2C_J2C_DONE",
	  .res          = { .start = RUDRA40_REG_OFFSET(RUDRA40_MORE_I2C_J2C_IOEXP_SW_CTRL),
			    .end   = offsetofend(struct Rudra40_dev_reg,
						 RUDRA40_MORE_I2C_J2C_IOEXP_SW_DONE) - 1,
			    .name  = "rudra40_more_i2c_j2c_ioexp-regs",
			    .flags = IORESOURCE_MEM, }, },
};

/***********************************************************************
 * rudra40 cic
 */
static const resource_size_t rudra40_cic_start_offset =
	offsetof(struct Rudra40_dev_reg,
		 RUDRA40_GLUE_ISR_MASTER_EVENT);

static const resource_size_t rudra40_cic_end_offset =
	offsetofend(struct Rudra40_dev_reg,
		    RUDRA40_OPTICS_STATUS_QSFP_PWR_GD_1) - 1;

/***********************************************************************
 * rudra40 MTD partitions
 */
static struct mtd_partition rudra40_flash_parts[] = {
	{ .name   = "rudra40-gold",
	  .size   = 0x280000,
	  .offset = 0, },
	{ .name   = "rudra40-user",
	  .size   = MTDPART_SIZ_FULL,
	  .offset = MTDPART_OFS_NXTBLK, },
};

static const struct flash_platform_data rudra40_flash_pdata = {
	.name     = "rudra40 flash partitions",
	.parts    = rudra40_flash_parts,
	.nr_parts = sizeof(rudra40_flash_parts)/sizeof(*rudra40_flash_parts),
#ifdef CONFIG_CIENA_SPI_NOR_PLATFORM_HWCAPS
	.broken_fast_read = true,
#endif
};

static struct mtd_partition rudra40_j2ca_parts[] = {
	{ .name   = "j2ca-user",
	  .size   = MTDPART_SIZ_FULL,
	  .offset = 0, },
};

static const struct flash_platform_data rudra40_j2ca_pdata = {
	.name     = "j2ca flash partitions",
	.parts    = rudra40_j2ca_parts,
	.nr_parts = sizeof(rudra40_j2ca_parts)/sizeof(*rudra40_j2ca_parts),
};

/***********************************************************************
 * rudra40 spi clients
 */
static const struct siril_spi_client rudra40_spi_local_flash = {
	.board_info = { .modalias      = "spi-nor",
			.platform_data = &rudra40_flash_pdata,
			.chip_select   = 0, },
};

static const struct siril_spi_client rudra40_spi_adc = {
	.board_info = { .modalias    = "spidev",
			.chip_select = 0, },
};

static const struct siril_spi_client rudra40_spi_j2ca = {
	.board_info = { .modalias    = "spidev",
			.chip_select = 0, },
};

/***********************************************************************
 * rudra40 spi master
 */
static const struct siril_spi_master rudra40_spi_masters[] = {
	{ .name        = "spi-ciena-fpga",
	  .bus_number  = 100,
	  .num_chipsel = 1,
	  .reg_width   = 32,
	  .num_clients = 1,
	  .clients     = &rudra40_spi_local_flash,
	  .res         = { .start = RUDRA40_REG_OFFSET(RUDRA40_FLASH_SPI_DATA),
			   .end   = offsetofend(struct Rudra40_dev_reg,
						RUDRA40_FLASH_SPI_ERROR) - 1,
			   .name  = "RUDRA40_FLASH_SPI-regs",
			   .flags = IORESOURCE_MEM|IORESOURCE_BUSY, },
	},
	{ .name        = "spi-ciena-fpga",
	  .bus_number  = 101,
	  .num_chipsel = 1,
	  .reg_width   = 32,
	  .num_clients = 1,
	  .clients     = &rudra40_spi_adc,
	  .res         = { .start = RUDRA40_REG_OFFSET(RUDRA40_ADC_SPI_DATA),
			   .end   = offsetofend(struct Rudra40_dev_reg,
						RUDRA40_ADC_SPI_ERROR) - 1,
			   .name  = "RUDRA40_ADC_SPI-regs",
			   .flags = IORESOURCE_MEM|IORESOURCE_BUSY, },
	},
	{ .name        = "spi-ciena-fpga",
	  .bus_number  = 102,
	  .num_chipsel = 1,
	  .reg_width   = 32,
	  .num_clients = 1,
	  .clients     = &rudra40_spi_j2ca,
	  .res         = { .start = RUDRA40_REG_OFFSET(RUDRA40_J2CA_SPI_DATA),
			   .end   = offsetofend(struct Rudra40_dev_reg,
						RUDRA40_J2CA_SPI_ERROR) - 1,
			   .name  = "RUDRA40_J2CA_SPI-regs",
			   .flags = IORESOURCE_MEM|IORESOURCE_BUSY, },
	},
};

/***********************************************************************
 * rudra40 uart
 */
static inline void rudra40_uart_buffer_enable(struct regmap *regs,
					      bool on)
{
	regmap_write_bits(regs,
			  RUDRA40_REG_OFFSET(RUDRA40_GLUE_GENERAL_CTL),
			  RUDRA40_GLUE_GENERAL_CTL_gps_buffer_en_l___MASK,
			  on ? 0 : RUDRA40_GLUE_GENERAL_CTL_gps_buffer_en_l___MASK);
}

static const struct siril_uart_master rudra40_uart_masters[] = {
	{   .name  = GNSS_NAME,
	    .devid = 0,
	    .res   = { .start = offsetof(struct Rudra40_dev_reg,
					 RUDRA40_SW_UART_DATA),
		       .end   = offsetofend(struct Rudra40_dev_reg,
					    RUDRA40_SW_UART_READ_FIFO_DEPTH) - 1,
		       .name  = GNSS_NAME"-gnss-regs",
		       .flags = IORESOURCE_MEM|IORESOURCE_BUSY, },
	    .pin   = "MISC_SW_UART_DATA_RCVD",
	},
	{   .name  = GNSS_NAME,
	    .devid = 1,
	    .res   = { .start = offsetof(struct Rudra40_dev_reg,
					 RUDRA40_RJ45_UART_DATA),
		       .end   = offsetofend(struct Rudra40_dev_reg,
					    RUDRA40_RJ45_UART_READ_FIFO_DEPTH) - 1,
		       .name  = GNSS_NAME"-tod-regs",
		       .flags = IORESOURCE_MEM|IORESOURCE_BUSY, },
	    .pin   = "MISC_RJ45_UART_DATA_RCVD",
	},
};


/***********************************************************************
 * rudra40 reset controller
 */
#define RUDRA40_REG_PWR_CTRL   "power-ctrl-reg"
#define RUDRA40_REG_PCIE_RESET "pcie-reset-reg"
#define RUDRA40_REG_RESET      "device-reset-reg"
#define RUDRA40_REG_PWR_RESET  "power-reset-reg"

#define RUDRA40_DEV_PWR_CTRL   "power-ctrl"
#define RUDRA40_DEV_PCIE_RESET "pcie-reset"
#define RUDRA40_DEV_RESET      "device-reset"
#define RUDRA40_DEV_PWR_RESET  "power-reset"

static const struct siril_reset_controller rudra40_reset_controllers[] = {
	{ .pdata      = { .reg_size  = sizeof(u32),
			  .shared_io = 1,
			  .name      = RUDRA40_REG_PWR_CTRL, },
	  .reg_offset = RUDRA40_REG_OFFSET(RUDRA40_GLUE_PWR_CTL),
	},
	{ .pdata      = { .reg_size  = sizeof(u32),
			  .shared_io = 1,
			  .name      = RUDRA40_REG_PCIE_RESET, },
	  .reg_offset = RUDRA40_REG_OFFSET(RUDRA40_GLUE_PCIE_RESET_MASK),
	},
	{ .pdata      = { .reg_size  = sizeof(u32),
			  .shared_io = 1,
			  .negative  = 1,
			  .name      = RUDRA40_REG_RESET, },
	  .reg_offset = RUDRA40_REG_OFFSET(RUDRA40_GLUE_DEVICE_RESET),
	},
	{ .pdata      = { .reg_size  = sizeof(u32),
			  .shared_io = 1,
			  .name      = RUDRA40_REG_PWR_RESET, },
	  .reg_offset = RUDRA40_REG_OFFSET(RUDRA40_GLUE_GENERAL_CTL),
	},
};

/* RUDRA40_GLUE_PWR_CTL */
static const struct ciena_sysfs_reset rudra40_pwr_ctrls[] = {
	{ .reset_name = "global_5v",
	  .bit_offset = 0, },
	{ .reset_name = "sync_3v3",
	  .bit_offset = 1, },
	{ .reset_name = "sync_1v8",
	  .bit_offset = 2, },
	{ .reset_name = "j2c_3v0",
	  .bit_offset = 3, },
	{ .reset_name = "j2cpa_vddo1p8",
	  .bit_offset = 4, },
	{ .reset_name = "j2cpa_vddc",
	  .bit_offset = 6, },
	{ .reset_name = "j2cpa_avdd1p8",
	  .bit_offset = 8, },
	{ .reset_name = "j2cpa_pvdd0p75",
	  .bit_offset = 10, },
	{ .reset_name = "j2cpa_rtvdd0p75",
	  .bit_offset = 12, },
	{ .reset_name = "j2cpa_srd_tvdd1p2",
	  .bit_offset = 14, },
	{ .reset_name = "j2cpa_hbm0_vpp2p5",
	  .bit_offset = 16, },
	{ .reset_name = "j2cpa_hbm1_vpp2p5",
	  .bit_offset = 17, },
	{ .reset_name = "j2cpa_hbm0_vdd1p2",
	  .bit_offset = 20, },
	{ .reset_name = "j2cpa_hbm1_vdd1p2",
	  .bit_offset = 21, },
	{ .reset_name = "qsfpdd_3v3_dco_1",
	  .bit_offset = 24, },
	{ .reset_name = "qsfpdd_3v3_dco_2",
	  .bit_offset = 25, },
	{ .reset_name = "avdd_0v8_mux_pwr",
	  .bit_offset = 26, },
	{ .reset_name = "j2ca_enable",
	  .bit_offset = 27, },
	{ .reset_name = "dvdd_0v72_mux",
	  .bit_offset = 29, },
	{ .reset_name = "gps_power",
	  .bit_offset = 30, },
	{ .reset_name = "vddio_1v8_mux_pwr",
	  .bit_offset = 31, },
	{ .reset_name = NULL, },
};

/* RUDRA40_GLUE_PCIE_RESET_MASK */
static const struct ciena_sysfs_reset rudra40_pcie_resets[] = {
	{ .reset_name = "j2pca_pcie_rst",
	  .bit_offset = 3, },
	{ .reset_name = "j2cpa_sys_rst",
	  .bit_offset = 5, },
	{ .reset_name = NULL, },
};

/* RUDRA40_GLUE_DEVICE_RESET */
static const struct ciena_sysfs_reset rudra40_dev_resets[] = {
	{ .reset_name = "zl30603",
	  .bit_offset = 0, },
	{ .reset_name = "j2cpa_si5345_156m",
	  .bit_offset = 1, },
	{ .reset_name = "j2cp_si5345_50m",
	  .bit_offset = 3, },
	{ .reset_name = "j2cp_rov",
	  .bit_offset = 4, },
	{ .reset_name = "j2cpa_qspi",
	  .bit_offset = 5, },
	{ .reset_name = "j2cpa_sys",
	  .bit_offset = 7, },
	{ .reset_name = "j2cpa_pcie",
	  .bit_offset = 9, },
	{ .reset_name = "rudra_mux",
	  .bit_offset = 10, },
	{ .reset_name = NULL, },
};

/* RUDRA40_GLUE_GENERAL_CTL */
static const struct ciena_sysfs_reset rudra40_pwr_resets[] = {
	{ .reset_name = "j2cpa_disconnect",
	  .bit_offset = 0, },
	{ .reset_name = "zarl",
	  .bit_offset = 2, },
	{ .reset_name = "ant_pwr_sel",
	  .bit_offset = 5, },
	{ .reset_name = "ant_pwr_enb",
	  .bit_offset = 6, },
	{ .reset_name = "main_i2c_ioexp0",
	  .bit_offset = 7, },
	{ .reset_name = "main_i2c_ioexp1",
	  .bit_offset = 8, },
	{ .reset_name = "gps",
	  .bit_offset = 14, },
	{ .reset_name = "siril_wp",
	  .bit_offset = 17, },
	{ .reset_name = "siril_wp_opt",
	  .bit_offset = 18, },
	{ .reset_name = "sw_override_cfpga_pmbus",
	  .bit_offset = 19, },
	{ .reset_name = "sw_cfpga_pmbus_mux",
	  .bit_offset = 20, },
	{ .reset_name = "sw_override_socket",
	  .bit_offset = 21, },
	{ .reset_name = "sw_socket_test",
	  .bit_offset = 22, },
	{ .reset_name = "disable_uart",
	  .bit_offset = 25, },
	{ .reset_name = "adc1_optics_spi_override",
	  .bit_offset = 26, },
	{ .reset_name = "adc2_optics_spi_override",
	  .bit_offset = 27, },
	{ .reset_name = "adc3_optics_spi_override",
	  .bit_offset = 28, },
	{ .reset_name = "adc4_skt_spi_override",
	  .bit_offset = 29, },
	{ .reset_name = "adc5_skt_mon_spi_override",
	  .bit_offset = 30, },
	{ .reset_name = "rclk_mux_sel",
	  .bit_offset = 31, },
	{ .reset_name = NULL, },
};

static const struct siril_reset_device rudra40_resets[] = {
	{ .dev_name = RUDRA40_DEV_PWR_CTRL,
	  .pdata    = { .controller_name = RUDRA40_REG_PWR_CTRL,
			.resets = rudra40_pwr_ctrls, },
	},
	{ .dev_name = RUDRA40_DEV_PCIE_RESET,
	  .pdata    = { .controller_name = RUDRA40_REG_PCIE_RESET,
			.resets = rudra40_pcie_resets, },
	},
	{ .dev_name = RUDRA40_DEV_RESET,
	  .pdata    = { .controller_name = RUDRA40_REG_RESET,
			.resets = rudra40_dev_resets, },
	},
	{ .dev_name = RUDRA40_DEV_PWR_RESET,
	  .pdata    = { .controller_name = RUDRA40_REG_PWR_RESET,
			.resets = rudra40_pwr_resets, },
	},
};


/***********************************************************************
 * read the rudra40 FPGA ID
 */
static inline void rudra40_fpga_id_read(struct regmap *regs,
					struct siril_id *info)
{
	regmap_read(regs, RUDRA40_REG_OFFSET(RUDRA40_BASE_FID),       &info->fpga_id);
	regmap_read(regs, RUDRA40_REG_OFFSET(RUDRA40_BASE_MJR),       &info->major);
	regmap_read(regs, RUDRA40_REG_OFFSET(RUDRA40_BASE_MNR),       &info->minor);
	regmap_read(regs, RUDRA40_REG_OFFSET(RUDRA40_BASE_BLD),       &info->build);
	regmap_read(regs, RUDRA40_REG_OFFSET(RUDRA40_BASE_FPGA_DATE), &info->date);
}

/***********************************************************************
 * enable the software-controlled ("malden") i2c controllers
 */
static inline void rudra40_sw_i2c_enable(struct regmap *regs,
					 bool on)
{
	u32 i2c_enbl = 0;
	int index;
	const u32 ioexp_regs[] =
		{ RUDRA40_REG_OFFSET(RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0),
		  RUDRA40_REG_OFFSET(RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL1),
		  RUDRA40_REG_OFFSET(RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL2),
		  RUDRA40_REG_OFFSET(RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL3),
		  RUDRA40_REG_OFFSET(RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL4),
		  RUDRA40_REG_OFFSET(RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL5), };

	/* The qsfp and main i2c controllers are in diags mode. */
	i2c_enbl |= 1 << RUDRA40_GLUE_I2C_SW_IF_SEL_sfp_sel_sw_i2c___SHIFT;
	i2c_enbl |= 1 << RUDRA40_GLUE_I2C_SW_IF_SEL_mb_sel_sw_i2c___SHIFT;
	regmap_write_bits(regs, RUDRA40_REG_OFFSET(RUDRA40_GLUE_I2C_SW_IF_SEL), i2c_enbl, 0);

	i2c_enbl = 1 << RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL_pwrgd_sel_sw_i2c___SHIFT;
	regmap_write_bits(regs, RUDRA40_REG_OFFSET(RUDRA40_MORE_I2C_PWRGD_SW_IF_SEL), i2c_enbl, on ? i2c_enbl : 0);

	i2c_enbl = 1 << RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL_j2c_sel_sw_i2c___SHIFT;
	regmap_write_bits(regs, RUDRA40_REG_OFFSET(RUDRA40_MORE_I2C_J2C_IOEXP_SW_IF_SEL), i2c_enbl, on ? i2c_enbl : 0);

	/* Enabling the SFP power */
	i2c_enbl = RUDRA40_OPTICS_SFP_PWR_EN_pwr_enable___MASK;
	regmap_write_bits(regs, RUDRA40_REG_OFFSET(RUDRA40_OPTICS_SFP_PWR_EN), i2c_enbl, i2c_enbl);
	i2c_enbl = RUDRA40_OPTICS_SFP_PWR_EN_2_pwr_enable___MASK;
	regmap_write_bits(regs, RUDRA40_REG_OFFSET(RUDRA40_OPTICS_SFP_PWR_EN_2), i2c_enbl, i2c_enbl);

	/* Enabling the QSFP power */
	i2c_enbl = RUDRA40_OPTICS_QSFP_PWR_EN_0_pwr_enable___MASK;
	regmap_write_bits(regs, RUDRA40_REG_OFFSET(RUDRA40_OPTICS_QSFP_PWR_EN_0), i2c_enbl, i2c_enbl);

	/* Here are the IO expander I2C SW interface selectors:
	 * always keep them off, the RUDRA40 controls them directly */
	i2c_enbl = 1 << RUDRA40_MAIN_I2C_IOEXP_SW_IF_SEL0_exp_sel_sw_i2c___SHIFT;
	for (index = 0; index < (sizeof(ioexp_regs)/sizeof(*ioexp_regs)); index++) {
		regmap_write_bits(regs, ioexp_regs[index], i2c_enbl, 0);
	}
}

static struct ciena_siril_led_pdev rudra40_leds[] = {
	{
		.name = "main:yellow:debug",
		.reg  = RUDRA40_REG_OFFSET(RUDRA40_GLUE_LED_SYS_STATUS),
		.mask = RUDRA40_GLUE_LED_SYS_STATUS_debug_led_glow___MASK,
		.blnk = RUDRA40_GLUE_LED_SYS_STATUS_debug_led_blink_en___MASK,
	},
};


/***********************************************************************
 * apply early 'fixups' to siril device
 *   - this is called as soon as siril is mapped
 *   - interrupts have not been setup yet
 */
static inline void rudra40_apply_early_fixups(struct regmap *regs)
{
	/* write to 0 (default) to disable MSI interrupts until mask
	 * registers are reverted to initial value. */
	regmap_write(regs, RUDRA40_REG_OFFSET(RUDRA40_GLUE_MSI_CTRL), 0);

	/* enable the rudra40 SPI bridge */
	regmap_write_bits(regs, RUDRA40_REG_OFFSET(RUDRA40_GLUE_SPI_SEL),
			  RUDRA40_GLUE_SPI_SEL_spi_sel___MASK,
			  RUDRA40_GLUE_SPI_SEL_spi_sel___MASK);
}

static struct sirilx_platform_data sirilx_pdata = {
	.sirilx_cic_start_offset       = rudra40_cic_start_offset,
	.sirilx_cic_end_offset         = rudra40_cic_end_offset,
	.sirilx_apply_early_fixups     = rudra40_apply_early_fixups,
	.sirilx_i2c_masters            = rudra40_i2c_masters,
	.sirilx_i2c_master_count       = ARRAY_SIZE(rudra40_i2c_masters),
	.sirilx_sw_i2c_enable          = rudra40_sw_i2c_enable,
	.sirilx_led_devs               = rudra40_leds,
	.sirilx_led_dev_count          = ARRAY_SIZE(rudra40_leds),
	.sirilx_resets                 = rudra40_resets,
	.sirilx_reset_count            = ARRAY_SIZE(rudra40_resets),
	.sirilx_reset_controllers      = rudra40_reset_controllers,
	.sirilx_reset_controller_count = ARRAY_SIZE(rudra40_reset_controllers),
	.sirilx_spi_masters            = rudra40_spi_masters,
	.sirilx_spi_master_count       = ARRAY_SIZE(rudra40_spi_masters),
	.sirilx_uart_masters           = rudra40_uart_masters,
	.sirilx_uart_master_count      = ARRAY_SIZE(rudra40_uart_masters),
	.sirilx_uart_buffer_enable     = rudra40_uart_buffer_enable,
};


/*
 * Local variables:
 *  c-indent-level: 8
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
/* set ts=8 noet sw=8 */
