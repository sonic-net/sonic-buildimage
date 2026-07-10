#ifndef __MASTER_CPLD_SYSFS
#define __MASTER_CPLD_SYSFS

/* generic CPLD sysfs file definition macros */
#define SYSFS_RAW_RO_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, master_cpld_##field##_raw_read, NULL);

#define SYSFS_RAW_RW_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUGO, master_cpld_##field##_raw_read, master_cpld_##field##_raw_write);

#define SYSFS_MISC_RO_ATTR_DEF(field, _read)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, _read, NULL);

#define SYSFS_MISC_RW_ATTR_DEF(field, _read, _write)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUGO, _read, _write);

#define SYSFS_ATTR_PTR(field)  \
&field.attr

/* declare master CPLD file system */
SYSFS_RAW_RO_ATTR_DEF(mjr_rev)
SYSFS_RAW_RO_ATTR_DEF(mnr_rev)

SYSFS_RAW_RW_ATTR_DEF(scrtch_reg)

SYSFS_RAW_RO_ATTR_DEF(brd_rev)
SYSFS_RAW_RO_ATTR_DEF(brd_type)

SYSFS_RAW_RW_ATTR_DEF(mb_rst)
SYSFS_RAW_RW_ATTR_DEF(npu_rst)
SYSFS_RAW_RW_ATTR_DEF(mgmt_phy_rst)

SYSFS_RAW_RW_ATTR_DEF(mb_eeprom_wp)
SYSFS_RAW_RW_ATTR_DEF(cpld_spi_wp)
SYSFS_RAW_RW_ATTR_DEF(fan_eeprom_wp)

SYSFS_RAW_RW_ATTR_DEF(ps2_int_msk)
SYSFS_RAW_RW_ATTR_DEF(ps1_int_msk)
SYSFS_RAW_RW_ATTR_DEF(usb_fault_msk)
SYSFS_RAW_RW_ATTR_DEF(pcie_int_msk)
SYSFS_RAW_RW_ATTR_DEF(fan_alert_int_msk)
SYSFS_RAW_RO_ATTR_DEF(usb_fault)
SYSFS_RAW_RO_ATTR_DEF(pcie_int)
SYSFS_RAW_RO_ATTR_DEF(fan_alert_int)

SYSFS_RAW_RW_ATTR_DEF(system_led_fld)
SYSFS_RAW_RW_ATTR_DEF(power_led)
SYSFS_RAW_RW_ATTR_DEF(fan_led)
SYSFS_RAW_RW_ATTR_DEF(locate_led)

SYSFS_RAW_RW_ATTR_DEF(led_test)
SYSFS_RAW_RW_ATTR_DEF(fan_tray3_led)
SYSFS_RAW_RW_ATTR_DEF(fan_tray2_led)
SYSFS_RAW_RW_ATTR_DEF(fan_tray1_led)

SYSFS_RAW_RW_ATTR_DEF(fan_tray6_led)
SYSFS_RAW_RW_ATTR_DEF(fan_tray5_led)
SYSFS_RAW_RW_ATTR_DEF(fan_tray4_led)

SYSFS_RAW_RO_ATTR_DEF(fan_tray6_present)
SYSFS_RAW_RO_ATTR_DEF(fan_tray5_present)
SYSFS_RAW_RO_ATTR_DEF(fan_tray4_present)
SYSFS_RAW_RO_ATTR_DEF(fan_tray3_present)
SYSFS_RAW_RO_ATTR_DEF(fan_tray2_present)
SYSFS_RAW_RO_ATTR_DEF(fan_tray1_present)

SYSFS_RAW_RO_ATTR_DEF(fan_type6)
SYSFS_RAW_RO_ATTR_DEF(fan_type5)
SYSFS_RAW_RO_ATTR_DEF(fan_type4)
SYSFS_RAW_RO_ATTR_DEF(fan_type3)
SYSFS_RAW_RO_ATTR_DEF(fan_type2)
SYSFS_RAW_RO_ATTR_DEF(fan_type1)

SYSFS_RAW_RO_ATTR_DEF(ps1_ps)
SYSFS_RAW_RO_ATTR_DEF(ps1_pg)
SYSFS_RAW_RO_ATTR_DEF(ps1_int)
SYSFS_RAW_RW_ATTR_DEF(ps1_on)
SYSFS_RAW_RO_ATTR_DEF(ps2_ps)
SYSFS_RAW_RO_ATTR_DEF(ps2_pg)
SYSFS_RAW_RO_ATTR_DEF(ps2_int)
SYSFS_RAW_RW_ATTR_DEF(ps2_on)

SYSFS_RAW_RW_ATTR_DEF(usb1_vbus_en)
SYSFS_RAW_RO_ATTR_DEF(v5p0_en)
SYSFS_RAW_RO_ATTR_DEF(v3p3_en)
SYSFS_RAW_RO_ATTR_DEF(vcc_1v8_en)
SYSFS_RAW_RO_ATTR_DEF(mac_avs1v_en)
SYSFS_RAW_RO_ATTR_DEF(mac1v_en)
SYSFS_RAW_RO_ATTR_DEF(vcc_1v25_en)

SYSFS_RAW_RO_ATTR_DEF(vcc_3p3_cpld)
SYSFS_RAW_RO_ATTR_DEF(vcc5v_pg)
SYSFS_RAW_RO_ATTR_DEF(vcc3v3_pg)
SYSFS_RAW_RO_ATTR_DEF(vcc_1v8_pg)
SYSFS_RAW_RO_ATTR_DEF(mac_avs1v_pg)
SYSFS_RAW_RO_ATTR_DEF(mac1v_pg)
SYSFS_RAW_RO_ATTR_DEF(vcc_1v25_pg)

SYSFS_RAW_RO_ATTR_DEF(port8_present)
SYSFS_RAW_RO_ATTR_DEF(port7_present)
SYSFS_RAW_RO_ATTR_DEF(port6_present)
SYSFS_RAW_RO_ATTR_DEF(port5_present)
SYSFS_RAW_RO_ATTR_DEF(port4_present)
SYSFS_RAW_RO_ATTR_DEF(port3_present)
SYSFS_RAW_RO_ATTR_DEF(port2_present)
SYSFS_RAW_RO_ATTR_DEF(port1_present)

SYSFS_RAW_RO_ATTR_DEF(port16_present)
SYSFS_RAW_RO_ATTR_DEF(port15_present)
SYSFS_RAW_RO_ATTR_DEF(port14_present)
SYSFS_RAW_RO_ATTR_DEF(port13_present)
SYSFS_RAW_RO_ATTR_DEF(port12_present)
SYSFS_RAW_RO_ATTR_DEF(port11_present)
SYSFS_RAW_RO_ATTR_DEF(port10_present)
SYSFS_RAW_RO_ATTR_DEF(port9_present)

SYSFS_RAW_RW_ATTR_DEF(port8_rst)
SYSFS_RAW_RW_ATTR_DEF(port7_rst)
SYSFS_RAW_RW_ATTR_DEF(port6_rst)
SYSFS_RAW_RW_ATTR_DEF(port5_rst)
SYSFS_RAW_RW_ATTR_DEF(port4_rst)
SYSFS_RAW_RW_ATTR_DEF(port3_rst)
SYSFS_RAW_RW_ATTR_DEF(port2_rst)
SYSFS_RAW_RW_ATTR_DEF(port1_rst)

SYSFS_RAW_RW_ATTR_DEF(port16_rst)
SYSFS_RAW_RW_ATTR_DEF(port15_rst)
SYSFS_RAW_RW_ATTR_DEF(port14_rst)
SYSFS_RAW_RW_ATTR_DEF(port13_rst)
SYSFS_RAW_RW_ATTR_DEF(port12_rst)
SYSFS_RAW_RW_ATTR_DEF(port11_rst)
SYSFS_RAW_RW_ATTR_DEF(port10_rst)
SYSFS_RAW_RW_ATTR_DEF(port9_rst)

SYSFS_RAW_RW_ATTR_DEF(port8_modsel)
SYSFS_RAW_RW_ATTR_DEF(port7_modsel)
SYSFS_RAW_RW_ATTR_DEF(port6_modsel)
SYSFS_RAW_RW_ATTR_DEF(port5_modsel)
SYSFS_RAW_RW_ATTR_DEF(port4_modsel)
SYSFS_RAW_RW_ATTR_DEF(port3_modsel)
SYSFS_RAW_RW_ATTR_DEF(port2_modsel)
SYSFS_RAW_RW_ATTR_DEF(port1_modsel)

SYSFS_RAW_RW_ATTR_DEF(port16_modsel)
SYSFS_RAW_RW_ATTR_DEF(port15_modsel)
SYSFS_RAW_RW_ATTR_DEF(port14_modsel)
SYSFS_RAW_RW_ATTR_DEF(port13_modsel)
SYSFS_RAW_RW_ATTR_DEF(port12_modsel)
SYSFS_RAW_RW_ATTR_DEF(port11_modsel)
SYSFS_RAW_RW_ATTR_DEF(port10_modsel)
SYSFS_RAW_RW_ATTR_DEF(port9_modsel)

SYSFS_RAW_RW_ATTR_DEF(port8_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port7_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port6_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port5_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port4_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port3_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port2_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port1_lpmode)

SYSFS_RAW_RW_ATTR_DEF(port16_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port15_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port14_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port13_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port12_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port11_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port10_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port9_lpmode)

SYSFS_RAW_RO_ATTR_DEF(port8_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port7_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port6_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port5_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port4_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port3_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port2_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port1_irq_status)

SYSFS_RAW_RO_ATTR_DEF(port16_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port15_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port14_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port13_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port12_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port11_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port10_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port9_irq_status)

SYSFS_RAW_RW_ATTR_DEF(port8_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port7_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port6_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port5_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port4_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port3_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port2_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port1_irq_msk)

SYSFS_RAW_RW_ATTR_DEF(port16_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port15_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port14_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port13_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port12_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port11_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port10_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port9_irq_msk)

SYSFS_RAW_RO_ATTR_DEF(port_1_8_present)
SYSFS_RAW_RO_ATTR_DEF(port_9_16_present)
SYSFS_RAW_RW_ATTR_DEF(port_1_8_rst)
SYSFS_RAW_RW_ATTR_DEF(port_9_16_rst)
SYSFS_RAW_RW_ATTR_DEF(port_1_8_modsel)
SYSFS_RAW_RW_ATTR_DEF(port_9_16_modsel)
SYSFS_RAW_RO_ATTR_DEF(port_1_8_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port_9_16_irq_status)
SYSFS_RAW_RW_ATTR_DEF(port_1_8_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port_9_16_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port_1_8_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port_9_16_lpmode)

SYSFS_RAW_RO_ATTR_DEF(fan_tray1_6_present)
SYSFS_RAW_RO_ATTR_DEF(psu_en_status_fld)
#endif /* __MASTER_CPLD_SYSFS */
