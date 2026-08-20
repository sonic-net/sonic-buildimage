#include <linux/sutra_regmap.h>
#include <linux/string.h>

#include "sirilx_common.h"

#ifndef UINT8_C
#define UINT8_C(c)  __UINT8_C(c)
#endif

#define BFPGA_I2C_ACCESS_PROTO           bfpga_smbus_word_offset
#define CIENA_BFPGA_COMPATIBLE_NAME      "ciena,fpga-sutra"
#define CIENA_SIRIL_DRIVER_NAME          "regmap-sutra-i2c"
#define CIENA_SIRIL_DRIVER_VERSION       "1.0.0"

/***********************************************************************
 * sutra i2c devices
 */
static const struct property_entry sutra_i2c_idp_eeprom[] = {
	PROPERTY_ENTRY_U32("size", 0x1000),
	PROPERTY_ENTRY_U32("pagesize", 32),
	PROPERTY_ENTRY_U32("address-width", 16),
	{ }
};

static const struct property_entry sutra_i2c_ps_eeprom[] = {
	PROPERTY_ENTRY_U32("size", 0x100),
	PROPERTY_ENTRY_U32("pagesize", 1),
	{ }
};

static const struct software_node sutra_i2c_idp_eeprom_node = {
	.properties = sutra_i2c_idp_eeprom,
};

static const struct software_node sutra_i2c_ps_eeprom_node = {
	.properties = sutra_i2c_ps_eeprom,
};

/***********************************************************************
 * sutra i2c slaves
 */
#define SUTRA_I2C_USB_MUX_TYPE "hd3ss3220"
static struct i2c_board_info sutra_main_i2c_slaves[] = {
	/* ADC MAX1139EEE */
	{ .type     = "max1139",
	  .addr     = 0x35,
	  .dev_name = "adc", },
	/* HD3SS3220IRNHT */
	{ .type     = SUTRA_I2C_USB_MUX_TYPE,
	  .addr     = 0x47,
	  .dev_name = "usb-mux", },
	/* IDP EEPROM 24LC32A */
	{ .type       = "24c32",
	  .addr       = 0x54,
	  .dev_name   = "sutra-eeprom",
	  .swnode     = &sutra_i2c_idp_eeprom_node, },
};

static struct i2c_board_info sutra_pmbus_i2c_slaves[] = {
	/*
	 * No .dev_name: keep standard <bus>-<addr> i2c naming
	 */
	{ .type     = "pmbus",
	  .addr     = 0x60,   /* VREF = 1.7V*154k/(154k+475k) = 0.416V */ },
};

/***********************************************************************
 * sutra i2c mux slaves
 */
static const struct i2c_board_info sutra_pwra_eeprom = {
	.type       = "24c02",
	.addr       = 0x56,
	.dev_name   = "pwra-eeprom",
	.swnode     = &sutra_i2c_ps_eeprom_node,
};

static const struct i2c_board_info sutra_pwra_pmbus = {
	.type       = "pmbus",
	.addr       = 0x5e,
};

static const struct i2c_board_info sutra_pwrb_eeprom = {
	.type       = "24c02",
	.addr       = 0x57,
	.dev_name   = "pwrb-eeprom",
	.swnode     = &sutra_i2c_ps_eeprom_node,
};

static const struct i2c_board_info sutra_pwrb_pmbus = {
	.type       = "pmbus",
	.addr       = 0x5f,
};

static const struct i2c_board_info *sutra_pwra_i2c_slaves[] = {
	&sutra_pwra_eeprom,
	&sutra_pwra_pmbus,
	NULL,
};

static const struct i2c_board_info *sutra_pwrb_i2c_slaves[] = {
	&sutra_pwrb_eeprom,
	&sutra_pwrb_pmbus,
	NULL,
};

static const struct i2c_board_info **sutra_ps_i2c_slaves[] = {
	sutra_pwra_i2c_slaves,
	sutra_pwrb_i2c_slaves,
};

/***********************************************************************
 * sutra power supply mux settings
 */
unsigned sutra_ps_mux_values[] = {
	0x00,
	0x01,
};

static const struct i2c_fpga_mux_info sutra_ps_mux_info = {
	.parent_adapter_id   = siril_ps_i2c_bus_id,
	.children_base_id    = siril_pwra_i2c_bus_id,
	.num_children        = ARRAY_SIZE(sutra_ps_mux_values),
	.children_muxsel     = sutra_ps_mux_values,
	.num_deferred_slaves = ARRAY_SIZE(sutra_ps_i2c_slaves),
	.deferred_slaves     = sutra_ps_i2c_slaves,
};

static const struct siril_i2c_mux sutra_ps_muxes[] = {
	{ .name = I2C_FPGA_MUX_DRIVER_NAME,
	  .info = &sutra_ps_mux_info,
	  .res  = { .start = SUTRA_REG_OFFSET(SUTRA_GLUE_PS_I2C_MUX_SEL),
		    .end   = offsetofend(struct Sutra_dev_reg,
					 SUTRA_GLUE_PS_I2C_MUX_SEL) - 1,
		    .name  = "sutra_glue_ps_i2c_mux_sel-regs",
		    .flags = IORESOURCE_REG, }, },
};

/***********************************************************************
 * sutra i2c masters
 */
#define SUTRA_I2C_REG(_id, _off)				\
	[CIENA_SMB_REG_ ## _id] = SUTRA_REG_OFFSET(_off)

static const unsigned sutra_main_i2c_offsets[CIENA_SMB_REG_MAX] = {
	SUTRA_I2C_REG(CTRL,        SUTRA_MAIN_I2C_DIAG_CTRL),
	SUTRA_I2C_REG(4X,          SUTRA_MAIN_I2C_DIAG_4X),
	SUTRA_I2C_REG(DEVADDR,     SUTRA_MAIN_I2C_DIAG_DEVADDR),
	SUTRA_I2C_REG(DATAADDR,    SUTRA_MAIN_I2C_DIAG_DATAADDR_0),
	SUTRA_I2C_REG(DATA_VLD,    SUTRA_MAIN_I2C_DIAG_DATA_VLD_0),
	SUTRA_I2C_REG(DEBUG,       SUTRA_MAIN_I2C_DIAG_DEBUG_0),
	SUTRA_I2C_REG(DATA_WR,     SUTRA_MAIN_I2C_DIAG_DATA_WR0),
	SUTRA_I2C_REG(DATA_RD,     SUTRA_MAIN_I2C_DIAG_DATA_RD0),
	SUTRA_I2C_REG(NO_DATAADDR, SUTRA_MAIN_I2C_DIAG_CTRL),
	SUTRA_I2C_REG(I2C_DONE,    SUTRA_MAIN_I2C_SW_DONE),
};

static const unsigned sutra_ps_i2c_offsets[CIENA_SMB_REG_MAX] = {
	SUTRA_I2C_REG(CTRL,        SUTRA_MORE_I2C_PS_DIAG_CTRL),
	SUTRA_I2C_REG(4X,          SUTRA_MORE_I2C_PS_DIAG_4X),
	SUTRA_I2C_REG(DEVADDR,     SUTRA_MORE_I2C_PS_DIAG_DEVADDR),
	SUTRA_I2C_REG(DATAADDR,    SUTRA_MORE_I2C_PS_DIAG_DATAADDR_0),
	SUTRA_I2C_REG(DATA_VLD,    SUTRA_MORE_I2C_PS_DIAG_DATA_VLD_0),
	SUTRA_I2C_REG(DEBUG,       SUTRA_MORE_I2C_PS_DIAG_DEBUG_0),
	SUTRA_I2C_REG(DATA_WR,     SUTRA_MORE_I2C_PS_DIAG_DATA_WR0),
	SUTRA_I2C_REG(DATA_RD,     SUTRA_MORE_I2C_PS_DIAG_DATA_RD0),
	SUTRA_I2C_REG(NO_DATAADDR, SUTRA_MORE_I2C_PS_DIAG_CTRL),
	SUTRA_I2C_REG(I2C_DONE,    SUTRA_MORE_I2C_PS_SW_DONE),
};

static const unsigned sutra_pcie_i2c_offsets[CIENA_SMB_REG_MAX] = {
	SUTRA_I2C_REG(CTRL,        SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL),
	SUTRA_I2C_REG(4X,          SUTRA_MORE_I2C_PCIE_SW_DIAG_4X),
	SUTRA_I2C_REG(DEVADDR,     SUTRA_MORE_I2C_PCIE_SW_DIAG_DEVADDR),
	SUTRA_I2C_REG(DATAADDR,    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATAADDR_0),
	SUTRA_I2C_REG(DATA_VLD,    SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_VLD_0),
	SUTRA_I2C_REG(DEBUG,       SUTRA_MORE_I2C_PCIE_SW_DIAG_DEBUG_0),
	SUTRA_I2C_REG(DATA_WR,     SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_WR0),
	SUTRA_I2C_REG(DATA_RD,     SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD0),
	SUTRA_I2C_REG(NO_DATAADDR, SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL),
	SUTRA_I2C_REG(I2C_DONE,    SUTRA_MORE_I2C_PCIE_SW_SW_DONE),
};

static const unsigned sutra_pmbus_i2c_offsets[CIENA_SMB_REG_MAX] = {
	SUTRA_I2C_REG(CTRL,        SUTRA_MORE_I2C_PMBUS_DIAG_CTRL),
	SUTRA_I2C_REG(4X,          SUTRA_MORE_I2C_PMBUS_DIAG_4X),
	SUTRA_I2C_REG(DEVADDR,     SUTRA_MORE_I2C_PMBUS_DIAG_DEVADDR),
	SUTRA_I2C_REG(DATAADDR,    SUTRA_MORE_I2C_PMBUS_DIAG_DATAADDR_0),
	SUTRA_I2C_REG(DATA_VLD,    SUTRA_MORE_I2C_PMBUS_DIAG_DATA_VLD_0),
	SUTRA_I2C_REG(DEBUG,       SUTRA_MORE_I2C_PMBUS_DIAG_DEBUG_0),
	SUTRA_I2C_REG(DATA_WR,     SUTRA_MORE_I2C_PMBUS_DIAG_DATA_WR0),
	SUTRA_I2C_REG(DATA_RD,     SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD0),
	SUTRA_I2C_REG(NO_DATAADDR, SUTRA_MORE_I2C_PMBUS_DIAG_CTRL),
	SUTRA_I2C_REG(I2C_DONE,    SUTRA_MORE_I2C_PMBUS_SW_DONE),
};

static struct siril_i2c_master sutra_i2c_masters[] = {
	/* Sutra main i2c */
	{ .driver_name  = CIENA_I2C_SMBUS_DRIVER_NAME,
	  .adapter_name = "sutra_main_i2c",
	  .bus_number   = siril_idp_i2c_bus_id,
	  .is_smbus     = true,
	  .num_deferred = ARRAY_SIZE(sutra_main_i2c_slaves),
	  .deferred     = sutra_main_i2c_slaves,
	  .pin          = "SUTRA_SW_I2C_MB_DONE",
	  .offsets      = sutra_main_i2c_offsets,
	  .res          = { .start = SUTRA_REG_OFFSET(SUTRA_MAIN_I2C_DIAG_CTRL),
			    .end   = offsetofend(struct Sutra_dev_reg,
						 SUTRA_MAIN_I2C_DIAG_DATA_RD3) - 1,
			    .name  = "sutra_main_i2c-regs",
			    .flags = IORESOURCE_REG, }, },
	/* Sutra ps i2c */
	{ .driver_name   = CIENA_I2C_SMBUS_DRIVER_NAME,
	  .adapter_name  = "sutra_ps_i2c",
	  .bus_number    = siril_ps_i2c_bus_id,
	  .pre_start_gap = 2000, /* fussy PMBus giant delay before a START */
	  .is_smbus      = true,
	  .num_muxes     = ARRAY_SIZE(sutra_ps_muxes),
	  .muxes         = sutra_ps_muxes,
	  .pin           = "SUTRA_SW_I2C_PS_DONE",
	  .offsets       = sutra_ps_i2c_offsets,
	  .res           = { .start = SUTRA_REG_OFFSET(SUTRA_MORE_I2C_PS_DIAG_CTRL),
			     .end   = offsetofend(struct Sutra_dev_reg,
						  SUTRA_MORE_I2C_PS_DIAG_DATA_RD3) - 1,
			     .name  = "sutra_ps_i2c-regs",
			     .flags = IORESOURCE_REG, }, },
	/* Sutra PCIe i2c */
	{ .driver_name  = CIENA_I2C_SMBUS_DRIVER_NAME,
	  .adapter_name = "sutra_pcie_i2c",
	  .bus_number   = siril_serdes_i2c_bus_id,
	  .is_smbus     = true,
	  .pin          = "SUTRA_SW_I2C_PCIE_DONE",
	  .offsets      = sutra_pcie_i2c_offsets,
	  .res          = { .start = SUTRA_REG_OFFSET(SUTRA_MORE_I2C_PCIE_SW_DIAG_CTRL),
			    .end   = offsetofend(struct Sutra_dev_reg,
						 SUTRA_MORE_I2C_PCIE_SW_DIAG_DATA_RD3) - 1,
			    .name  = "sutra_pcie_i2c-regs",
			    .flags = IORESOURCE_REG, }, },
	/* Sutra PMBus i2c */
	{ .driver_name   = CIENA_I2C_SMBUS_DRIVER_NAME,
	  .adapter_name  = "sutra_pmbus_i2c",
	  .bus_number    = siril_pmbus1_i2c_bus_id,
	  .pre_start_gap = 2000, /* fussy PMBus giant delay before a START */
	  .deferred_children_delay_ms = 5000,  /* allow early PMBus probing to not block systemd services */
	  .is_smbus      = true,
	  .num_deferred  = ARRAY_SIZE(sutra_pmbus_i2c_slaves),
	  .deferred      = sutra_pmbus_i2c_slaves,
	  .pin           = "SUTRA_SW_I2C_PMBUS_DONE",
	  .offsets       = sutra_pmbus_i2c_offsets,
	  .res           = { .start = SUTRA_REG_OFFSET(SUTRA_MORE_I2C_PMBUS_DIAG_CTRL),
			     .end   = offsetofend(struct Sutra_dev_reg,
						  SUTRA_MORE_I2C_PMBUS_DIAG_DATA_RD3) - 1,
			     .name  = "sutra_pmbus_i2c-regs",
			     .flags = IORESOURCE_REG, }, },
};

/***********************************************************************
 * sutra cic
 */
static const resource_size_t sutra_cic_start_offset =
	SUTRA_REG_OFFSET(SUTRA_GLUE_IST_MASTER_EVENT);

static const resource_size_t sutra_cic_end_offset =
	offsetofend(struct Sutra_dev_reg,
		    SUTRA_GLUE_IST_BUTTON) - 1;


/***********************************************************************
 * sutra reset controller
 */
#define SUTRA_REG_RESET "cpld-reset-reg"
#define SUTRA_DEV_RESET "cpld-reset"

static const struct siril_reset_controller sutra_reset_controllers[] = {
	{ .pdata      = { .reg_size  = sizeof(u8),
			  .negative  = 1,
			  .name      = SUTRA_REG_RESET, },
	  .reg_offset = SUTRA_REG_OFFSET(SUTRA_GLUE_DEVICE_RESET),
	},
};

static const struct ciena_sysfs_reset sutra_dev_resets[] = {
	{ .reset_name = "bmc",
	  .bit_offset = 0, },
	{ .reset_name = "pcie_switch",
	  .bit_offset = 1, },
	{ .reset_name = "mgmt_nic",
	  .bit_offset = 2, },
	{ .reset_name = "ssd",
	  .bit_offset = 3, },
	{ .reset_name = "rtc",
	  .bit_offset = 4, },
	{ .reset_name = "tpm",
	  .bit_offset = 5, },
	{ .reset_name = "pcie_qspi",
	  .bit_offset = 6, },
	{ .reset_name = NULL, },
};

static const struct siril_reset_device sutra_resets[] = {
	{ .dev_name = SUTRA_DEV_RESET,
	  .pdata    = { .controller_name = SUTRA_REG_RESET,
			.resets = sutra_dev_resets, },
	},
};


/***********************************************************************
 * read the sutra FPGA ID
 */
static inline void sutra_fpga_id_read(struct regmap *regs,
				      struct siril_id *info)
{
	unsigned int pid;
	unsigned int did;
	unsigned int date;
	unsigned int month;

	regmap_read(regs, SUTRA_REG_OFFSET(SUTRA_BASE_PID), &pid);
	regmap_read(regs, SUTRA_REG_OFFSET(SUTRA_BASE_DID), &did);

	info->fpga_id = (pid << 8) | did;

	regmap_read(regs, SUTRA_REG_OFFSET(SUTRA_BASE_MJR), &info->major);
	regmap_read(regs, SUTRA_REG_OFFSET(SUTRA_BASE_MNR), &info->minor);
	regmap_read(regs, SUTRA_REG_OFFSET(SUTRA_BASE_BLD), &info->build);

	regmap_read(regs, SUTRA_REG_OFFSET(SUTRA_BASE_DATE), &date);
	regmap_read(regs, SUTRA_REG_OFFSET(SUTRA_BASE_MONTH), &month);

	info->date = (month << 8) | date;
}


/***********************************************************************
 * enable the software-controlled ("malden") i2c controllers
 */
static inline void sutra_sw_i2c_enable(struct regmap *regs,
				       bool on)
{
	u32 i2c_enbl = 0;

	i2c_enbl |= SUTRA_GLUE_I2C_SW_IF_SEL_pmbus_sel_sw_i2c___MASK;
	i2c_enbl |= SUTRA_GLUE_I2C_SW_IF_SEL_pcie_sel_sw_i2c___MASK;
	i2c_enbl |= SUTRA_GLUE_I2C_SW_IF_SEL_psb_sel_sw_i2c___MASK;
	i2c_enbl |= SUTRA_GLUE_I2C_SW_IF_SEL_psa_sel_sw_i2c___MASK;
	i2c_enbl |= SUTRA_GLUE_I2C_SW_IF_SEL_mb_sel_sw_i2c___MASK;

	/* use the new and improved diags i2c controller */
	regmap_write_bits(regs,
			  SUTRA_REG_OFFSET(SUTRA_GLUE_I2C_SW_IF_SEL),
			  i2c_enbl, 0);
}
/***********************************************************************
 * sutra cic fixups: resolve the USB mux interrupt
 */
static void sutra_cic_fixups(struct regmap *regs,
			     struct device *cic_dev)
{
	struct device    *dev       = regmap_get_device(regs);
	const char       *mux_type  = SUTRA_I2C_USB_MUX_TYPE;
	const char       *usb_pin   = "SUTRA_MISC_USB";
	unsigned          mux_index = 0;
	int               usb_irq;

	/* find the USB mux in the slave array */
	while (ARRAY_SIZE(sutra_main_i2c_slaves) > mux_index) {
		if (!strcmp(sutra_main_i2c_slaves[mux_index].type, mux_type))
			break;
		mux_index++;
	}

	if (ARRAY_SIZE(sutra_main_i2c_slaves) == mux_index) {
		dev_warn(dev, "cannot find i2c client %s\n", mux_type);
		return;
	}

	usb_irq = siril_resolve_cic_pin(regs, cic_dev, usb_pin);
	if (0 < usb_irq)
		sutra_main_i2c_slaves[mux_index].irq = usb_irq;
}

static struct ciena_siril_led_pdev sutra_leds[] = {
	{
		.name    = "front::all",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_0),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_0_enable_all_leds___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_0_blink_all_leds___MASK,
	},
	{
		.name    = "psa:green:ok",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_STATUS_PWR),
		.mask    = SUTRA_GLUE_STATUS_PWR_PSA_PWR_OK___MASK,
	},
	{
		.name    = "psb:green:ok",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_STATUS_PWR),
		.mask    = SUTRA_GLUE_STATUS_PWR_PSB_PWR_OK___MASK,
	},
	{
		.name    = "front:yellow:alarm",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_2),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_2_alarm_led_yellow___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_2_alarm_led_blink_en___MASK,
	},
	{
		.name    = "front:red:gnss",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_2),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_red___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_red_blink_en___MASK,
	},
	{
		.name    = "front:green:gnss",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_2),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_grn___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_2_gnss_led_grn_blink_en___MASK,
	},
	{
		.name    = "front:blue:sync",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_3),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_blue___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_blink_en___MASK,
	},
	{
		.name    = "front:red:sync",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_3),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_grn___MASK|
			   SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_red___MASK,
		.use_val = true,
		.val     = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_red___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_blink_en___MASK,
	},
	{
		.name    = "front:green:sync",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_3),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_grn___MASK|
			   SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_red___MASK,
		.use_val = true,
		.val     = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_grn___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_blink_en___MASK,
	},
	{
		.name    = "front:yellow:sync",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_3),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_grn___MASK|
			   SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_red___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_3_sync_led_blink_en___MASK,
	},
	{
		.name    = "front:blue:status",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_3),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_3_status_led_blue___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_3_status_led_blink_en___MASK,
	},
	{
		.name    = "front:red:status",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_3),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_3_status_led_red___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_3_status_led_blink_en___MASK,
	},
	{
		.name    = "front:green:status",
		.reg     = SUTRA_REG_OFFSET(SUTRA_GLUE_LED_SYS_STATUS_3),
		.mask    = SUTRA_GLUE_LED_SYS_STATUS_3_status_led_green___MASK,
		.blnk    = SUTRA_GLUE_LED_SYS_STATUS_3_status_led_blink_en___MASK,
	},
};


static struct sirilx_platform_data sirilx_pdata = {
	.sirilx_cic_start_offset       = sutra_cic_start_offset,
	.sirilx_cic_end_offset         = sutra_cic_end_offset,
	.sirilx_cic_fixups             = sutra_cic_fixups,
	.sirilx_i2c_masters            = sutra_i2c_masters,
	.sirilx_i2c_master_count       = ARRAY_SIZE(sutra_i2c_masters),
	.sirilx_sw_i2c_enable          = sutra_sw_i2c_enable,
	.sirilx_led_devs               = sutra_leds,
	.sirilx_led_dev_count          = ARRAY_SIZE(sutra_leds),
	.sirilx_resets                 = sutra_resets,
	.sirilx_reset_count            = ARRAY_SIZE(sutra_resets),
	.sirilx_reset_controllers      = sutra_reset_controllers,
	.sirilx_reset_controller_count = ARRAY_SIZE(sutra_reset_controllers),
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
