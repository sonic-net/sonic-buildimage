#ifndef __SLAVE_CPLD_SYSFS
#define __SLAVE_CPLD_SYSFS

/* generic CPLD sysfs file definition macros */
#define SYSFS_RAW_RO_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, slave_cpld_##field##_raw_read, NULL);

#define SYSFS_RAW_RW_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUGO, slave_cpld_##field##_raw_read, slave_cpld_##field##_raw_write);

#define SYSFS_MISC_RO_ATTR_DEF(field, _read)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, _read, NULL);

#define SYSFS_MISC_RW_ATTR_DEF(field, _read, _write)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUGO, _read, _write);

#define SYSFS_ATTR_PTR(field)  \
&field.attr


/* declare slave CPLD file system */
SYSFS_RAW_RO_ATTR_DEF(mjr_rev)
SYSFS_RAW_RO_ATTR_DEF(mnr_rev)

SYSFS_RAW_RW_ATTR_DEF(scrtch_reg)

SYSFS_RAW_RO_ATTR_DEF(brd_rev)
SYSFS_RAW_RO_ATTR_DEF(brd_type)

SYSFS_RAW_RO_ATTR_DEF(port24_present)
SYSFS_RAW_RO_ATTR_DEF(port23_present)
SYSFS_RAW_RO_ATTR_DEF(port22_present)
SYSFS_RAW_RO_ATTR_DEF(port21_present)
SYSFS_RAW_RO_ATTR_DEF(port20_present)
SYSFS_RAW_RO_ATTR_DEF(port19_present)
SYSFS_RAW_RO_ATTR_DEF(port18_present)
SYSFS_RAW_RO_ATTR_DEF(port17_present)

SYSFS_RAW_RO_ATTR_DEF(port32_present)
SYSFS_RAW_RO_ATTR_DEF(port31_present)
SYSFS_RAW_RO_ATTR_DEF(port30_present)
SYSFS_RAW_RO_ATTR_DEF(port29_present)
SYSFS_RAW_RO_ATTR_DEF(port28_present)
SYSFS_RAW_RO_ATTR_DEF(port27_present)
SYSFS_RAW_RO_ATTR_DEF(port26_present)
SYSFS_RAW_RO_ATTR_DEF(port25_present)

SYSFS_RAW_RW_ATTR_DEF(port24_rst)
SYSFS_RAW_RW_ATTR_DEF(port23_rst)
SYSFS_RAW_RW_ATTR_DEF(port22_rst)
SYSFS_RAW_RW_ATTR_DEF(port21_rst)
SYSFS_RAW_RW_ATTR_DEF(port20_rst)
SYSFS_RAW_RW_ATTR_DEF(port19_rst)
SYSFS_RAW_RW_ATTR_DEF(port18_rst)
SYSFS_RAW_RW_ATTR_DEF(port17_rst)

SYSFS_RAW_RW_ATTR_DEF(port32_rst)
SYSFS_RAW_RW_ATTR_DEF(port31_rst)
SYSFS_RAW_RW_ATTR_DEF(port30_rst)
SYSFS_RAW_RW_ATTR_DEF(port29_rst)
SYSFS_RAW_RW_ATTR_DEF(port28_rst)
SYSFS_RAW_RW_ATTR_DEF(port27_rst)
SYSFS_RAW_RW_ATTR_DEF(port26_rst)
SYSFS_RAW_RW_ATTR_DEF(port25_rst)

SYSFS_RAW_RW_ATTR_DEF(port24_modsel)
SYSFS_RAW_RW_ATTR_DEF(port23_modsel)
SYSFS_RAW_RW_ATTR_DEF(port22_modsel)
SYSFS_RAW_RW_ATTR_DEF(port21_modsel)
SYSFS_RAW_RW_ATTR_DEF(port20_modsel)
SYSFS_RAW_RW_ATTR_DEF(port19_modsel)
SYSFS_RAW_RW_ATTR_DEF(port18_modsel)
SYSFS_RAW_RW_ATTR_DEF(port17_modsel)

SYSFS_RAW_RW_ATTR_DEF(port32_modsel)
SYSFS_RAW_RW_ATTR_DEF(port31_modsel)
SYSFS_RAW_RW_ATTR_DEF(port30_modsel)
SYSFS_RAW_RW_ATTR_DEF(port29_modsel)
SYSFS_RAW_RW_ATTR_DEF(port28_modsel)
SYSFS_RAW_RW_ATTR_DEF(port27_modsel)
SYSFS_RAW_RW_ATTR_DEF(port26_modsel)
SYSFS_RAW_RW_ATTR_DEF(port25_modsel)

SYSFS_RAW_RW_ATTR_DEF(port24_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port23_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port22_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port21_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port20_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port19_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port18_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port17_lpmode)

SYSFS_RAW_RW_ATTR_DEF(port32_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port31_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port30_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port29_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port28_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port27_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port26_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port25_lpmode)

SYSFS_RAW_RO_ATTR_DEF(port24_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port23_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port22_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port21_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port20_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port19_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port18_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port17_irq_status)

SYSFS_RAW_RO_ATTR_DEF(port32_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port31_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port30_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port29_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port28_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port27_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port26_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port25_irq_status)

SYSFS_RAW_RW_ATTR_DEF(port24_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port23_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port22_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port21_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port20_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port19_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port18_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port17_irq_msk)

SYSFS_RAW_RW_ATTR_DEF(port32_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port31_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port30_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port29_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port28_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port27_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port26_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port25_irq_msk)

SYSFS_RAW_RO_ATTR_DEF(port_17_24_present)
SYSFS_RAW_RO_ATTR_DEF(port_25_32_present)
SYSFS_RAW_RW_ATTR_DEF(port_17_24_rst)
SYSFS_RAW_RW_ATTR_DEF(port_25_32_rst)
SYSFS_RAW_RW_ATTR_DEF(port_17_24_modsel)
SYSFS_RAW_RW_ATTR_DEF(port_25_32_modsel)
SYSFS_RAW_RO_ATTR_DEF(port_17_24_irq_status)
SYSFS_RAW_RO_ATTR_DEF(port_25_32_irq_status)
SYSFS_RAW_RW_ATTR_DEF(port_17_24_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port_25_32_irq_msk)
SYSFS_RAW_RW_ATTR_DEF(port_17_24_lpmode)
SYSFS_RAW_RW_ATTR_DEF(port_25_32_lpmode)

#endif /* __SLAVE_CPLD_SYSFS */
