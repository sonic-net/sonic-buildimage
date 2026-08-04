#ifndef __SYSTEM_CPLD_REG
#define __SYSTEM_CPLD_REG

static int system_cpld_raw_read(struct device *dev, struct device_attribute *attr, char *buf,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name);
static int system_cpld_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name);

/* generic CPLD read function */
#define FLD_RAW_RD_FUNC(_reg, _fld, _wdh) static ssize_t \
system_cpld_##_fld##_raw_read(struct device *dev, struct device_attribute *attr, char *buf) { \
    return system_cpld_raw_read(dev, attr, buf, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
}

/* generic CPLD write function */
#define FLD_RAW_WR_FUNC(_reg, _fld, _wdh) static ssize_t \
system_cpld_##_fld##_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { \
    return system_cpld_raw_write(dev, attr, buf, count, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
}

/* CPLD register definition macros */
#define REG_DEF(_reg, _off, _wdh) \
static unsigned int _reg##_offset = (unsigned int)(_off); \
static unsigned int _reg##_width = (unsigned int)(_wdh);

/* CPLD register field definition macros, with generic read/write function */
#define FLD_RAW_RO_DEF(_reg, _fld, _sft, _wdh) \
static unsigned int _fld##_shift = (unsigned int)(_sft); \
static unsigned int _fld##_width = (unsigned int)(_wdh); \
static unsigned int _fld##_mask = ((((unsigned int)1) << (_wdh)) - 1); \
FLD_RAW_RD_FUNC(_reg, _fld, _wdh)

#define FLD_RAW_RW_DEF(_reg, _fld, _sft, _wdh) \
static unsigned int _fld##_shift = (unsigned int)(_sft); \
static unsigned int _fld##_width = (unsigned int)(_wdh); \
static unsigned int _fld##_mask = ((((unsigned int)1) << (_wdh)) - 1); \
FLD_RAW_RD_FUNC(_reg, _fld, _wdh) FLD_RAW_WR_FUNC(_reg, _fld, _wdh)

/* declare system CPLD registers */
/*                      register name                           offset  width */
/*                      --------------------------------------- ------- ----- */
REG_DEF(                sys_cpld_rev,                           0x00,   8)
REG_DEF(                sys_cpld_gpr,                           0x01,   8)
REG_DEF(                cpu_brd_rev_type,                       0x02,   8)
REG_DEF(                sys_srr,                                0x03,   8)
REG_DEF(                sys_eeprom_wp,                          0x04,   8)
REG_DEF(                sys_irq,                                0x05,   8)
REG_DEF(                sys_wd,                                 0x06,   8)
REG_DEF(                sys_mb_rst_en,                          0x07,   8)
REG_DEF(                cpu_pwr_en_status,                      0x08,   8)
REG_DEF(                cpu_pwr_status,                         0x09,   8)


/* declare system CPLD register's fields */
/*                      register name               field name           shift  width */
/*                      ----------------------      ----------------     ------ ----- */
FLD_RAW_RO_DEF(         sys_cpld_rev,               mjr_rev,             4,     4)
FLD_RAW_RO_DEF(         sys_cpld_rev,               mnr_rev,             0,     4)

FLD_RAW_RW_DEF(         sys_cpld_gpr,               scrtch_reg,          0,     8)

FLD_RAW_RO_DEF(         cpu_brd_rev_type,           brd_rev,             4,     4)
FLD_RAW_RO_DEF(         cpu_brd_rev_type,           brd_type,            0,     4)

FLD_RAW_RO_DEF(         sys_srr,                    ssd_present,         3,     1)
FLD_RAW_RW_DEF(         sys_srr,                    spi_cs_sel,          2,     1)
FLD_RAW_RW_DEF(         sys_srr,                    rst_bios_switch,     1,     1)
FLD_RAW_RW_DEF(         sys_srr,                    cpld_upgrade_rst,    0,     1)

FLD_RAW_RW_DEF(         sys_eeprom_wp,              cpld_spi_wp,         4,     1)
FLD_RAW_RW_DEF(         sys_eeprom_wp,              system_id_eeprom_wp, 3,     1)
FLD_RAW_RW_DEF(         sys_eeprom_wp,              spi_me_wp,           2,     1)
FLD_RAW_RW_DEF(         sys_eeprom_wp,              spi_bios_wp,         1,     1)
FLD_RAW_RW_DEF(         sys_eeprom_wp,              spi_bak_bios_wp,     0,     1)

FLD_RAW_RW_DEF(         sys_irq,                    vrhot_irq_en,        6,     1)
FLD_RAW_RW_DEF(         sys_irq,                    cpu_thermtrip_irq_en,5,     1)
FLD_RAW_RW_DEF(         sys_irq,                    temp_alert_irq_en,   4,     1)
FLD_RAW_RO_DEF(         sys_irq,                    vrhot_irq,           2,     1)
FLD_RAW_RO_DEF(         sys_irq,                    cpu_thermtrip_irq,   1,     1)
FLD_RAW_RO_DEF(         sys_irq,                    temp_alert_irq,      0,     1)

FLD_RAW_RW_DEF(         sys_wd,                     wd_timer,            4,     4)
FLD_RAW_RW_DEF(         sys_wd,                     wd_en,               1,     1)
FLD_RAW_RW_DEF(         sys_wd,                     wd_punch,            0,     1)

FLD_RAW_RW_DEF(         sys_mb_rst_en,              mb_rst_en,           0,     1)

FLD_RAW_RO_DEF(         cpu_pwr_en_status,          pwr_v3p3_en,         7,     1)
FLD_RAW_RO_DEF(         cpu_pwr_en_status,          pwr_vcc_vnn_en,      6,     1)
FLD_RAW_RO_DEF(         cpu_pwr_en_status,          pwr_vccsram_en,      5,     1)
FLD_RAW_RO_DEF(         cpu_pwr_en_status,          pwr_vddq_en,         4,     1)
FLD_RAW_RO_DEF(         cpu_pwr_en_status,          pwr_vcc_ref_en,      3,     1)
FLD_RAW_RO_DEF(         cpu_pwr_en_status,          pwr_v1p05_en,        2,     1)
FLD_RAW_RO_DEF(         cpu_pwr_en_status,          pwr_v1p8_en,         1,     1)
FLD_RAW_RO_DEF(         cpu_pwr_en_status,          pwr_v2p5_en,         0,     1)

FLD_RAW_RO_DEF(         cpu_pwr_status,             pg_v3p3,             7,     1)
FLD_RAW_RO_DEF(         cpu_pwr_status,             pg_vcc_vnn,          6,     1)
FLD_RAW_RO_DEF(         cpu_pwr_status,             pg_vccsram,          5,     1)
FLD_RAW_RO_DEF(         cpu_pwr_status,             pg_vddq,             4,     1)
FLD_RAW_RO_DEF(         cpu_pwr_status,             pg_vcc_ref,          3,     1)
FLD_RAW_RO_DEF(         cpu_pwr_status,             pg_v1p05,            2,     1)
FLD_RAW_RO_DEF(         cpu_pwr_status,             pg_v1p8,             1,     1)
FLD_RAW_RO_DEF(         cpu_pwr_status,             pg_v2p5,             0,     1)

#endif /* __SYSTEM_CPLD_REG */
