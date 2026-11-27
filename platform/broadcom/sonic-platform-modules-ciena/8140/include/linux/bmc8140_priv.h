#include <linux/bmc8140_regmap.h>

#include "sirilx_common.h"
#include "bmcx_common.h"
#include "raw_chardev/ciena_raw_chardev.h"

#define CIENA_SIRIL_DRIVER_NAME          "uio-bmc8140-pci"
#define CIENA_SIRIL_DRIVER_VERSION       "1.0.0"

static const struct pci_device_id ciena_siril_device_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_CIENA, PCI_DEVICE_ID_CIENA_OBMC_8140) },
	{ 0, }
};


/***********************************************************************
 * no bmc8140 cic
 */
static const resource_size_t bmc8140_cic_start_offset =
	BMC8140_REG_OFFSET(BMC8140_BASE_MSI_CTRL);

static const resource_size_t bmc8140_cic_end_offset =
	offsetofend(struct Bmc8140_dev_reg, BMC8140_BASE_STATUS_FAN) -1;


/***********************************************************************
 * bmc8140 reset controller
 */
#define BMC8140_REG_BRD_RESET              "board-reset-reg"
#define BMC8140_REG_BRD_RESET_CP           "board-reset-reg-cp"
#define BMC8140_REG_FPGA_RECONFIG_RESET    "reconfig-reset-reg"
#define BMC8140_REG_FPGA_RECONFIG_RESET_CP "reconfig-reset-reg-cp"
#define BMC8140_DEV_BRD_RESET              "board-reset"
#define BMC8140_DEV_BRD_RESET_CP           "board-reset-cp"
#define BMC8140_DEV_FPGA_RECONFIG_RESET    "reconfig-reset"
#define BMC8140_DEV_FPGA_RECONFIG_RESET_CP "reconfig-reset-cp"

static const struct siril_reset_controller bmc8140_reset_controllers[] = {
	{ .pdata      = { .use_raw_value = 1,
			  .ipmi          = 1,
			  .name          = BMC8140_REG_BRD_RESET, },
	},
	{ .pdata      = { .use_raw_value = 1,
			  .ipmi          = 1,
			  .name          = BMC8140_REG_BRD_RESET_CP, },
	},
	{ .pdata      = { .use_raw_value = 1,
			  .ipmi          = 1,
			  .name          = BMC8140_REG_FPGA_RECONFIG_RESET, },
	},
	{ .pdata      = { .use_raw_value = 1,
			  .ipmi          = 1,
			  .name          = BMC8140_REG_FPGA_RECONFIG_RESET_CP, },
	},
};

static const struct ciena_sysfs_reset bmc8140_brd_resets[] = {
	{ .reset_name = "board_reset",
	  .value = CPLD_DP_BOARD_RESET, },
	{ .reset_name = NULL, },
};

static const struct ciena_sysfs_reset bmc8140_brd_resets_cp[] = {
	{ .reset_name = "board_reset",
	  .value = CPLD_BOARD_RESET, },
	{ .reset_name = NULL, },
};

static const struct ciena_sysfs_reset bmc8140_reconfig_resets[] = {
	{ .reset_name = "reconfig_reset",
	  .value = CPLD_DP_FPGA_RECONFIG, },
	{ .reset_name = NULL, },
};

static const struct ciena_sysfs_reset bmc8140_reconfig_resets_cp[] = {
	{ .reset_name = "reconfig_reset",
	  .value = CPLD_FPGA_RECONFIG, },
	{ .reset_name = NULL, },
};

static const struct siril_reset_device bmc8140_resets[] = {
	{ .dev_name = BMC8140_DEV_BRD_RESET,
	  .pdata    = { .controller_name = BMC8140_REG_BRD_RESET,
			.resets = bmc8140_brd_resets, },
	},
	{ .dev_name = BMC8140_DEV_BRD_RESET_CP,
	  .pdata    = { .controller_name = BMC8140_REG_BRD_RESET_CP,
			.resets = bmc8140_brd_resets_cp, },
	},
	{ .dev_name = BMC8140_DEV_FPGA_RECONFIG_RESET,
	  .pdata    = { .controller_name = BMC8140_REG_FPGA_RECONFIG_RESET,
			.resets = bmc8140_reconfig_resets, },
	},
	{ .dev_name = BMC8140_DEV_FPGA_RECONFIG_RESET_CP,
	  .pdata    = { .controller_name = BMC8140_REG_FPGA_RECONFIG_RESET_CP,
			.resets = bmc8140_reconfig_resets_cp, },
	},
};


/***********************************************************************
 * fake bmc8140 idps
 */
static struct resource bmc8140_idp_res[] = {
	{ .start = BMC8140_REG_OFFSET(BMC8140_IDP_DATA),
	  .end   = BMC8140_REG_OFFSET(BMC8140_IDP_THEEND) - 1,
	  .flags = IORESOURCE_REG, },
	{ .start = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_0),
	  .end   = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_1) - 1,
	  .flags = IORESOURCE_REG, },
	{ .start = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_1),
	  .end   = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_2) - 1,
	  .flags = IORESOURCE_REG, },
	{ .start = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_2),
	  .end   = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_3) - 1,
	  .flags = IORESOURCE_REG, },
	{ .start = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_3),
	  .end   = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_4) - 1,
	  .flags = IORESOURCE_REG, },
	{ .start = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_4),
	  .end   = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_5) - 1,
	  .flags = IORESOURCE_REG, },
	{ .start = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_5),
	  .end   = BMC8140_REG_OFFSET(BMC8140_FAN_FRU_IDP_THEEND) - 1,
	  .flags = IORESOURCE_REG, },
};

static struct ciena_raw_chardev_pdata bmc8140_idp_data[] = {
	{ .chardev_env = "CIENA_IDP_NAME=main-mfg" },
	{ .chardev_env = "CIENA_IDP_NAME=fan0-mfg" },
	{ .chardev_env = "CIENA_IDP_NAME=fan1-mfg" },
	{ .chardev_env = "CIENA_IDP_NAME=fan2-mfg" },
	{ .chardev_env = "CIENA_IDP_NAME=fan3-mfg" },
	{ .chardev_env = "CIENA_IDP_NAME=fan4-mfg" },
	{ .chardev_env = "CIENA_IDP_NAME=fan5-mfg" },
};

static struct ciena_siril_extra_pdev bmc8140_emulated_idps[] = {
	{ .name  = CIENA_RAW_CHARDEV_NAME,
	  .id    = PLATFORM_DEVID_AUTO,
	  .res   = &bmc8140_idp_res[0],
	  .nres  = 1,
	  .pdata = &bmc8140_idp_data[0],
	  .psize = sizeof(*bmc8140_idp_data), },
	{ .name  = CIENA_RAW_CHARDEV_NAME,
	  .id    = PLATFORM_DEVID_AUTO,
	  .res   = &bmc8140_idp_res[1],
	  .nres  = 1,
	  .pdata = &bmc8140_idp_data[1],
	  .psize = sizeof(*bmc8140_idp_data), },
	{ .name  = CIENA_RAW_CHARDEV_NAME,
	  .id    = PLATFORM_DEVID_AUTO,
	  .res   = &bmc8140_idp_res[2],
	  .nres  = 1,
	  .pdata = &bmc8140_idp_data[2],
	  .psize = sizeof(*bmc8140_idp_data), },
	{ .name  = CIENA_RAW_CHARDEV_NAME,
	  .id    = PLATFORM_DEVID_AUTO,
	  .res   = &bmc8140_idp_res[3],
	  .nres  = 1,
	  .pdata = &bmc8140_idp_data[3],
	  .psize = sizeof(*bmc8140_idp_data), },
	{ .name  = CIENA_RAW_CHARDEV_NAME,
	  .id    = PLATFORM_DEVID_AUTO,
	  .res   = &bmc8140_idp_res[4],
	  .nres  = 1,
	  .pdata = &bmc8140_idp_data[4],
	  .psize = sizeof(*bmc8140_idp_data), },
	{ .name  = CIENA_RAW_CHARDEV_NAME,
	  .id    = PLATFORM_DEVID_AUTO,
	  .res   = &bmc8140_idp_res[5],
	  .nres  = 1,
	  .pdata = &bmc8140_idp_data[5],
	  .psize = sizeof(*bmc8140_idp_data), },
	{ .name  = CIENA_RAW_CHARDEV_NAME,
	  .id    = PLATFORM_DEVID_AUTO,
	  .res   = &bmc8140_idp_res[6],
	  .nres  = 1,
	  .pdata = &bmc8140_idp_data[6],
	  .psize = sizeof(*bmc8140_idp_data), },
};

/***********************************************************************
 * read the bmc8140 FPGA ID
 */
static inline void bmc8140_fpga_id_read(struct regmap *regs,
					struct siril_id *info)
{
	regmap_read(regs, BMC8140_REG_OFFSET(BMC8140_BASE_FID), &info->fpga_id);

	regmap_read(regs, BMC8140_REG_OFFSET(BMC8140_BASE_MJR), &info->major);
	regmap_read(regs, BMC8140_REG_OFFSET(BMC8140_BASE_MNR), &info->minor);
	regmap_read(regs, BMC8140_REG_OFFSET(BMC8140_BASE_BLD), &info->build);

	regmap_read(regs, BMC8140_REG_OFFSET(BMC8140_BASE_BMC_DATE), &info->date);
}


/***********************************************************************
 * Thermal sensors and thresholds
 */
/* For signed temperature sensors */
#define TZ_S( nAME, rEG, sENSOR, vALID )   {			\
	.name          = nAME,					\
	.reg           = BMC8140_REG_OFFSET(rEG),		\
	.valid_mask    = rEG##_##vALID##___MASK,		\
	.temp_mask     = rEG##_##sENSOR##___MASK,		\
	.temp_shift    = rEG##_##sENSOR##___SHIFT		\
}

/* For unsigned temperature sensors */
#define TZ_U( nAME, rEG, sENSOR, vALID )   {			\
	.name          = nAME,					\
	.reg           = BMC8140_REG_OFFSET(rEG),		\
	.valid_mask    = rEG##_##vALID##___MASK,		\
	.temp_mask     = rEG##_##sENSOR##___MASK,		\
	.temp_shift    = rEG##_##sENSOR##___SHIFT,		\
	.temp_unsigned = true					\
}

#define TH_S( nAME, rEG, sENSOR)   {				\
	.name          = nAME,					\
	.reg           = BMC8140_REG_OFFSET(rEG),		\
	.temp_mask     = rEG##_##sENSOR##___MASK,		\
	.temp_shift    = rEG##_##sENSOR##___SHIFT,		\
	.threshold     = true					\
}

// names can be up to 20 characters
static struct ciena_siril_thermal_pdev bmc8140_thermals[] = {
	TZ_S( "front:lc",   BMC8140_TMP_5, LOCAL,  local_present),
	TZ_S( "front:rm",   BMC8140_TMP_5, REMOTE, remote_present),
	TZ_S( "rear:lc",    BMC8140_TMP_7, LOCAL,  local_present),
	TZ_S( "rear:rm",    BMC8140_TMP_7, REMOTE, remote_present),
	TZ_S( "right:lc",   BMC8140_TMP_4, LOCAL,  local_present),
	TZ_S( "right:rm",   BMC8140_TMP_4, REMOTE, remote_present),
	TZ_S( "left:lc",    BMC8140_TMP_8, LOCAL,  local_present),
	TZ_S( "left:rm",    BMC8140_TMP_8, REMOTE, remote_present),
	TZ_S( "bcm:lc",     BMC8140_TMP_6, LOCAL,  local_present),
	TZ_S( "bcm:rm",     BMC8140_TMP_6, REMOTE, remote_present),
	TZ_S( "cpu_1:lc",   BMC8140_TMP_2, LOCAL,  local_present),
	TZ_S( "cpu_1:rm",   BMC8140_TMP_2, REMOTE, remote_present),
	TZ_S( "cpu_2:lc",   BMC8140_TMP_0, LOCAL,  local_present),
	TZ_S( "cpu_2:rm",   BMC8140_TMP_0, REMOTE, remote_present),
	TZ_S( "ssd:lc",     BMC8140_TMP_1, LOCAL,  local_present),
	TZ_S( "ssd:rm",     BMC8140_TMP_1, REMOTE, remote_present),
	TZ_S( "pcie:lc",    BMC8140_TMP_3, LOCAL,  local_present),
	TZ_S( "pcie:rm",    BMC8140_TMP_3, REMOTE, remote_present),
	TH_S( "user0:",     BMC8140_TMP_THRESH_CRIT_0, local_crithi),
	TH_S( "user1:",     BMC8140_TMP_THRESH_CRIT_1, local_crithi),
	TH_S( "shutdown:",  BMC8140_TMP_THRESH_CRIT_5, local_crithi),
	TH_S( "overtemp:",  BMC8140_TMP_THRESH_CRIT_5, local_crithi),
	TH_S( "undertemp:", BMC8140_TMP_THRESH_CRIT_5, local_critlo),
};


/***********************************************************************
 * Fan devices
 */
#define FAN( nAME, fAN) {								\
	.name            = nAME,							\
	.index           = fAN,								\
	.tach_reg        = BMC8140_REG_OFFSET(BMC8140_FAN_TACH_##fAN),			\
	.tach_mask       = BMC8140_FAN_TACH_##fAN##_fan_tach_0___MASK,			\
	.tach_shift      = BMC8140_FAN_TACH_##fAN##_fan_tach_0___SHIFT,			\
	.stat_reg        = BMC8140_REG_OFFSET(BMC8140_FAN_STATUS_##fAN),		\
	.pres_mask       = BMC8140_FAN_STATUS_##fAN##_fan_present___MASK,		\
	.fault_mask      = BMC8140_FAN_STATUS_##fAN##_fan_failure___MASK,		\
	.thres_reg       = BMC8140_REG_OFFSET(BMC8140_FAN_THRESH_##fAN),		\
	.thres_min_mask  = BMC8140_FAN_THRESH_##fAN##_min___MASK,			\
	.thres_min_shift = BMC8140_FAN_THRESH_##fAN##_min___SHIFT,			\
	.thres_max_mask  = BMC8140_FAN_THRESH_##fAN##_max___MASK,			\
	.thres_max_shift = BMC8140_FAN_THRESH_##fAN##_max___SHIFT,			\
}

// names can be up to 20 characters
static struct ciena_fan_pdata bmc8140_fans[] = {
	FAN( "fan:0:0",  0 ),
	FAN( "fan:0:1",  1 ),
	FAN( "fan:1:0",  2 ),
	FAN( "fan:1:1",  3 ),
	FAN( "fan:2:0",  4 ),
	FAN( "fan:2:1",  5 ),
	FAN( "fan:3:0",  6 ),
	FAN( "fan:3:1",  7 ),
	FAN( "fan:4:0",  8 ),
	FAN( "fan:4:1",  9 ),
	FAN( "fan:5:0", 10 ),
	FAN( "fan:5:1", 11 ),
};


/***********************************************************************
 * apply early 'fixups' to siril device
 *   - this is called as soon as siril is mapped
 *   - interrupts have not been setup yet
 */
static inline void bmc8140_apply_early_fixups(struct regmap *regs)
{
	int index;

	for (index = 0; ARRAY_SIZE(bmc8140_idp_data) > index; index++)
		bmc8140_idp_data[index].chardev_regmap = regs;
}

static struct sirilx_platform_data sirilx_pdata = {
	.sirilx_cic_start_offset       = bmc8140_cic_start_offset,
	.sirilx_cic_end_offset         = bmc8140_cic_end_offset,
	.sirilx_irq_resource           = bmcx_irq_resource,
	.sirilx_irq_deresource         = bmcx_irq_deresource,
	.sirilx_apply_early_fixups     = bmc8140_apply_early_fixups,
	.sirilx_extra_devs             = bmc8140_emulated_idps,
	.sirilx_extra_devs_count       = ARRAY_SIZE(bmc8140_emulated_idps),
	/*
	 * IPMI resets disabled: in SONiC, modules load in parallel via udev.
	 * The PCI probe runs on work_for_cpu_fn — cannot block waiting for
	 * ipmi_si to finish (deadlock). -EPROBE_DEFER does not propagate
	 * through device_attach() in sirilx_platform.h. Re-enable when
	 * sirilx_platform.h gains deferred probe support or a SONiC-specific
	 * late-binding mechanism is added for reset sub-devices.
	 * IPMI resets (board-reset, reconfig-reset) still work via ipmitool.
	 */
	.sirilx_thermal_devs           = bmc8140_thermals,
	.sirilx_thermal_dev_count      = ARRAY_SIZE(bmc8140_thermals),
	.sirilx_fan_devs               = bmc8140_fans,
	.sirilx_fan_dev_count          = ARRAY_SIZE(bmc8140_fans),
	.sirilx_wdt_irq_pin            = "HOST_WDT_TIMEOUT",
	.sirilx_usb_rules              = &bmcx_usb_neuter,
};


/*
 * Local variables:
 *  c-indent-level: 8
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
// vim: sw=8 noet
