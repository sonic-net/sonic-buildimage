#ifndef __SYSTEM_CPLD_SYSFS
#define __SYSTEM_CPLD_SYSFS

/* generic CPLD sysfs file definition macros */
#define SYSFS_RAW_RO_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, system_cpld_##field##_raw_read, NULL);

#define SYSFS_RAW_RW_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUGO, system_cpld_##field##_raw_read, system_cpld_##field##_raw_write);

#define SYSFS_MISC_RO_ATTR_DEF(field, _read)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, _read, NULL);

#define SYSFS_MISC_RW_ATTR_DEF(field, _read, _write)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUGO, _read, _write);

#define SYSFS_ATTR_PTR(field)  \
&field.attr


/* declare system CPLD file system */
SYSFS_RAW_RO_ATTR_DEF(mjr_rev)
SYSFS_RAW_RO_ATTR_DEF(mnr_rev)

SYSFS_RAW_RW_ATTR_DEF(scrtch_reg)

SYSFS_RAW_RO_ATTR_DEF(brd_rev)
SYSFS_RAW_RO_ATTR_DEF(brd_type)

SYSFS_RAW_RO_ATTR_DEF(ssd_present)
SYSFS_RAW_RW_ATTR_DEF(spi_cs_sel)
SYSFS_RAW_RW_ATTR_DEF(rst_bios_switch)
SYSFS_RAW_RW_ATTR_DEF(cpld_upgrade_rst)

SYSFS_RAW_RW_ATTR_DEF(cpld_spi_wp)
SYSFS_RAW_RW_ATTR_DEF(system_id_eeprom_wp)
SYSFS_RAW_RW_ATTR_DEF(spi_me_wp)
SYSFS_RAW_RW_ATTR_DEF(spi_bios_wp)
SYSFS_RAW_RW_ATTR_DEF(spi_bak_bios_wp)

SYSFS_RAW_RW_ATTR_DEF(vrhot_irq_en)
SYSFS_RAW_RW_ATTR_DEF(cpu_thermtrip_irq_en)
SYSFS_RAW_RW_ATTR_DEF(temp_alert_irq_en)
SYSFS_RAW_RO_ATTR_DEF(vrhot_irq)
SYSFS_RAW_RO_ATTR_DEF(cpu_thermtrip_irq)
SYSFS_RAW_RO_ATTR_DEF(temp_alert_irq)

SYSFS_RAW_RW_ATTR_DEF(wd_timer)
SYSFS_RAW_RW_ATTR_DEF(wd_en)
SYSFS_RAW_RW_ATTR_DEF(wd_punch)

SYSFS_RAW_RW_ATTR_DEF(mb_rst_en)

SYSFS_RAW_RO_ATTR_DEF(pwr_v3p3_en)
SYSFS_RAW_RO_ATTR_DEF(pwr_vcc_vnn_en)
SYSFS_RAW_RO_ATTR_DEF(pwr_vccsram_en)
SYSFS_RAW_RO_ATTR_DEF(pwr_vddq_en)
SYSFS_RAW_RO_ATTR_DEF(pwr_vcc_ref_en)
SYSFS_RAW_RO_ATTR_DEF(pwr_v1p05_en)
SYSFS_RAW_RO_ATTR_DEF(pwr_v1p8_en)
SYSFS_RAW_RO_ATTR_DEF(pwr_v2p5_en)

SYSFS_RAW_RO_ATTR_DEF(pg_v3p3)
SYSFS_RAW_RO_ATTR_DEF(pg_vcc_vnn)
SYSFS_RAW_RO_ATTR_DEF(pg_vccsram)
SYSFS_RAW_RO_ATTR_DEF(pg_vddq)
SYSFS_RAW_RO_ATTR_DEF(pg_vcc_ref)
SYSFS_RAW_RO_ATTR_DEF(pg_v1p05)
SYSFS_RAW_RO_ATTR_DEF(pg_v1p8)
SYSFS_RAW_RO_ATTR_DEF(pg_v2p5)

#endif /* __SYSTEM_CPLD_SYSFS */
