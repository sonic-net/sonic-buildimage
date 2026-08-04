#ifndef __MASTER_CPLD_REG
#define __MASTER_CPLD_REG

static int master_cpld_raw_read(struct device *dev, struct device_attribute *attr, char *buf,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name);
static int master_cpld_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name);

/* generic CPLD read function */
#define FLD_RAW_RD_FUNC(_reg, _fld, _wdh) static ssize_t \
master_cpld_##_fld##_raw_read(struct device *dev, struct device_attribute *attr, char *buf) { \
    return master_cpld_raw_read(dev, attr, buf, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
}

/* generic CPLD write function */
#define FLD_RAW_WR_FUNC(_reg, _fld, _wdh) static ssize_t \
master_cpld_##_fld##_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { \
    return master_cpld_raw_write(dev, attr, buf, count, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
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

/* declare master CPLD registers */
/*                      register name                           offset  width */
/*                      --------------------------------------- ------- ----- */
REG_DEF(                mstr_cpld_rev,                          0x00,   8)
REG_DEF(                mstr_cpld_gpr,                          0x01,   8)
REG_DEF(                mb_brd_rev_type,                        0x02,   8)
REG_DEF(                mstr_srr,                               0x03,   8)
REG_DEF(                eeprom_wp,                              0x04,   8)
REG_DEF(                mstr_irq,                               0x05,   8)
REG_DEF(                system_led,                             0x06,   8)
REG_DEF(                fan_tray_3_1_led,                       0x07,   8)
REG_DEF(                fan_tray_6_4_led,                       0x08,   8)
REG_DEF(                fan_tray_status,                        0x09,   8)
REG_DEF(                fan_type_status,                        0x0A,   8)
REG_DEF(                psu_en_status,                          0x0B,   8)
REG_DEF(                mb_pwr_en_status,                       0x0C,   8)
REG_DEF(                mb_pwr_status,                          0x0D,   8)

REG_DEF(                zqsfp28_present_8_1_status,             0x10,   8)
REG_DEF(                zqsfp28_present_16_9_status,            0x11,   8)
REG_DEF(                zqsfp28_rst_8_1,                        0x12,   8)
REG_DEF(                zqsfp28_rst_16_9,                       0x13,   8)
REG_DEF(                zqsfp28_modsel_8_1,                     0x14,   8)
REG_DEF(                zqsfp28_modsel_16_9,                    0x15,   8)
REG_DEF(                zqsfp28_lpmode_8_1,                     0x16,   8)
REG_DEF(                zqsfp28_lpmode_16_9,                    0x17,   8)
REG_DEF(                zqsfp28_irq_8_1_status,                 0x18,   8)
REG_DEF(                zqsfp28_irq_16_9_status,                0x19,   8)
REG_DEF(                zqsfp28_irq_msk_8_1_status,             0x1A,   8)
REG_DEF(                zqsfp28_irq_msk_16_9_status,            0x1B,   8)


/* declare master CPLD register's fields */
/*                      register name               field name           shift  width */
/*                      ----------------------      ----------------     ------ ----- */
FLD_RAW_RO_DEF(         mstr_cpld_rev,              mjr_rev,             4,     4)
FLD_RAW_RO_DEF(         mstr_cpld_rev,              mnr_rev,             0,     4)

FLD_RAW_RW_DEF(         mstr_cpld_gpr,              scrtch_reg,          0,     8)

FLD_RAW_RO_DEF(         mb_brd_rev_type,            brd_rev,             4,     4)
FLD_RAW_RO_DEF(         mb_brd_rev_type,            brd_type,            0,     4)

FLD_RAW_RW_DEF(         mstr_srr,                   mb_rst,              2,     1)
FLD_RAW_RW_DEF(         mstr_srr,                   npu_rst,             1,     1)
FLD_RAW_RW_DEF(         mstr_srr,                   mgmt_phy_rst,        0,     1)

FLD_RAW_RW_DEF(         eeprom_wp,                  mb_eeprom_wp,        2,     1)
FLD_RAW_RW_DEF(         eeprom_wp,                  cpld_spi_wp,         1,     1)
FLD_RAW_RW_DEF(         eeprom_wp,                  fan_eeprom_wp,       0,     1)

FLD_RAW_RW_DEF(         mstr_irq,                   ps2_int_msk,         7,     1)
FLD_RAW_RW_DEF(         mstr_irq,                   ps1_int_msk,         6,     1)
FLD_RAW_RW_DEF(         mstr_irq,                   usb_fault_msk,       5,     1)
FLD_RAW_RW_DEF(         mstr_irq,                   pcie_int_msk,        4,     1)
FLD_RAW_RW_DEF(         mstr_irq,                   fan_alert_int_msk,   3,     1)
FLD_RAW_RO_DEF(         mstr_irq,                   usb_fault,           2,     1)
FLD_RAW_RO_DEF(         mstr_irq,                   pcie_int,            1,     1)
FLD_RAW_RO_DEF(         mstr_irq,                   fan_alert_int,       0,     1)

FLD_RAW_RW_DEF(         system_led,                 system_led_fld,      6,     2)
FLD_RAW_RW_DEF(         system_led,                 power_led,           4,     2)
FLD_RAW_RW_DEF(         system_led,                 fan_led,             2,     2)
FLD_RAW_RW_DEF(         system_led,                 locate_led,          1,     1)

FLD_RAW_RW_DEF(         fan_tray_3_1_led,           led_test,            6,     2)
FLD_RAW_RW_DEF(         fan_tray_3_1_led,           fan_tray3_led,       4,     2)
FLD_RAW_RW_DEF(         fan_tray_3_1_led,           fan_tray2_led,       2,     2)
FLD_RAW_RW_DEF(         fan_tray_3_1_led,           fan_tray1_led,       0,     2)

FLD_RAW_RW_DEF(         fan_tray_6_4_led,           fan_tray6_led,       4,     2)
FLD_RAW_RW_DEF(         fan_tray_6_4_led,           fan_tray5_led,       2,     2)
FLD_RAW_RW_DEF(         fan_tray_6_4_led,           fan_tray4_led,       0,     2)

FLD_RAW_RO_DEF(         fan_tray_status,            fan_tray6_present,   5,     1)
FLD_RAW_RO_DEF(         fan_tray_status,            fan_tray5_present,   4,     1)
FLD_RAW_RO_DEF(         fan_tray_status,            fan_tray4_present,   3,     1)
FLD_RAW_RO_DEF(         fan_tray_status,            fan_tray3_present,   2,     1)
FLD_RAW_RO_DEF(         fan_tray_status,            fan_tray2_present,   1,     1)
FLD_RAW_RO_DEF(         fan_tray_status,            fan_tray1_present,   0,     1)

FLD_RAW_RO_DEF(         fan_type_status,            fan_type6,           5,     1)
FLD_RAW_RO_DEF(         fan_type_status,            fan_type5,           4,     1)
FLD_RAW_RO_DEF(         fan_type_status,            fan_type4,           3,     1)
FLD_RAW_RO_DEF(         fan_type_status,            fan_type3,           2,     1)
FLD_RAW_RO_DEF(         fan_type_status,            fan_type2,           1,     1)
FLD_RAW_RO_DEF(         fan_type_status,            fan_type1,           0,     1)

FLD_RAW_RO_DEF(         psu_en_status,              ps1_ps,              7,     1)
FLD_RAW_RO_DEF(         psu_en_status,              ps1_pg,              6,     1)
FLD_RAW_RO_DEF(         psu_en_status,              ps1_int,             5,     1)
FLD_RAW_RW_DEF(         psu_en_status,              ps1_on,              4,     1)
FLD_RAW_RO_DEF(         psu_en_status,              ps2_ps,              3,     1)
FLD_RAW_RO_DEF(         psu_en_status,              ps2_pg,              2,     1)
FLD_RAW_RO_DEF(         psu_en_status,              ps2_int,             1,     1)
FLD_RAW_RW_DEF(         psu_en_status,              ps2_on,              0,     1)

FLD_RAW_RW_DEF(         mb_pwr_en_status,           usb1_vbus_en,        7,     1)
FLD_RAW_RO_DEF(         mb_pwr_en_status,           v5p0_en,             5,     1)
FLD_RAW_RO_DEF(         mb_pwr_en_status,           v3p3_en,             4,     1)
FLD_RAW_RO_DEF(         mb_pwr_en_status,           vcc_1v8_en,          3,     1)
FLD_RAW_RO_DEF(         mb_pwr_en_status,           mac_avs1v_en,        2,     1)
FLD_RAW_RO_DEF(         mb_pwr_en_status,           mac1v_en,            1,     1)
FLD_RAW_RO_DEF(         mb_pwr_en_status,           vcc_1v25_en,         0,     1)

FLD_RAW_RO_DEF(         mb_pwr_status,              vcc_3p3_cpld,        6,     1)
FLD_RAW_RO_DEF(         mb_pwr_status,              vcc5v_pg,            5,     1)
FLD_RAW_RO_DEF(         mb_pwr_status,              vcc3v3_pg,           4,     1)
FLD_RAW_RO_DEF(         mb_pwr_status,              vcc_1v8_pg,          3,     1)
FLD_RAW_RO_DEF(         mb_pwr_status,              mac_avs1v_pg,        2,     1)
FLD_RAW_RO_DEF(         mb_pwr_status,              mac1v_pg,            1,     1)
FLD_RAW_RO_DEF(         mb_pwr_status,              vcc_1v25_pg,         0,     1)

FLD_RAW_RO_DEF(         zqsfp28_present_8_1_status, port8_present,       7,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_8_1_status, port7_present,       6,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_8_1_status, port6_present,       5,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_8_1_status, port5_present,       4,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_8_1_status, port4_present,       3,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_8_1_status, port3_present,       2,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_8_1_status, port2_present,       1,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_8_1_status, port1_present,       0,     1)

FLD_RAW_RO_DEF(         zqsfp28_present_16_9_status,port16_present,      7,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_16_9_status,port15_present,      6,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_16_9_status,port14_present,      5,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_16_9_status,port13_present,      4,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_16_9_status,port12_present,      3,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_16_9_status,port11_present,      2,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_16_9_status,port10_present,      1,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_16_9_status,port9_present,       0,     1)

FLD_RAW_RW_DEF(         zqsfp28_rst_8_1,            port8_rst,           7,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_8_1,            port7_rst,           6,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_8_1,            port6_rst,           5,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_8_1,            port5_rst,           4,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_8_1,            port4_rst,           3,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_8_1,            port3_rst,           2,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_8_1,            port2_rst,           1,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_8_1,            port1_rst,           0,     1)

FLD_RAW_RW_DEF(         zqsfp28_rst_16_9,           port16_rst,          7,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_16_9,           port15_rst,          6,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_16_9,           port14_rst,          5,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_16_9,           port13_rst,          4,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_16_9,           port12_rst,          3,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_16_9,           port11_rst,          2,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_16_9,           port10_rst,          1,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_16_9,           port9_rst,           0,     1)

FLD_RAW_RW_DEF(         zqsfp28_modsel_8_1,         port8_modsel,        7,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_8_1,         port7_modsel,        6,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_8_1,         port6_modsel,        5,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_8_1,         port5_modsel,        4,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_8_1,         port4_modsel,        3,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_8_1,         port3_modsel,        2,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_8_1,         port2_modsel,        1,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_8_1,         port1_modsel,        0,     1)

FLD_RAW_RW_DEF(         zqsfp28_modsel_16_9,        port16_modsel,       7,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_16_9,        port15_modsel,       6,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_16_9,        port14_modsel,       5,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_16_9,        port13_modsel,       4,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_16_9,        port12_modsel,       3,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_16_9,        port11_modsel,       2,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_16_9,        port10_modsel,       1,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_16_9,        port9_modsel,        0,     1)

FLD_RAW_RW_DEF(         zqsfp28_lpmode_8_1,         port8_lpmode,        7,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_8_1,         port7_lpmode,        6,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_8_1,         port6_lpmode,        5,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_8_1,         port5_lpmode,        4,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_8_1,         port4_lpmode,        3,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_8_1,         port3_lpmode,        2,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_8_1,         port2_lpmode,        1,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_8_1,         port1_lpmode,        0,     1)

FLD_RAW_RW_DEF(         zqsfp28_lpmode_16_9,        port16_lpmode,       7,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_16_9,        port15_lpmode,       6,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_16_9,        port14_lpmode,       5,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_16_9,        port13_lpmode,       4,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_16_9,        port12_lpmode,       3,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_16_9,        port11_lpmode,       2,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_16_9,        port10_lpmode,       1,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_16_9,        port9_lpmode,        0,     1)

FLD_RAW_RO_DEF(         zqsfp28_irq_8_1_status,     port8_irq_status,    7,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_8_1_status,     port7_irq_status,    6,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_8_1_status,     port6_irq_status,    5,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_8_1_status,     port5_irq_status,    4,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_8_1_status,     port4_irq_status,    3,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_8_1_status,     port3_irq_status,    2,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_8_1_status,     port2_irq_status,    1,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_8_1_status,     port1_irq_status,    0,     1)

FLD_RAW_RO_DEF(         zqsfp28_irq_16_9_status,    port16_irq_status,   7,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_16_9_status,    port15_irq_status,   6,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_16_9_status,    port14_irq_status,   5,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_16_9_status,    port13_irq_status,   4,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_16_9_status,    port12_irq_status,   3,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_16_9_status,    port11_irq_status,   2,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_16_9_status,    port10_irq_status,   1,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_16_9_status,    port9_irq_status,    0,     1)

FLD_RAW_RW_DEF(         zqsfp28_irq_msk_8_1_status, port8_irq_msk,       7,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_8_1_status, port7_irq_msk,       6,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_8_1_status, port6_irq_msk,       5,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_8_1_status, port5_irq_msk,       4,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_8_1_status, port4_irq_msk,       3,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_8_1_status, port3_irq_msk,       2,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_8_1_status, port2_irq_msk,       1,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_8_1_status, port1_irq_msk,       0,     1)

FLD_RAW_RW_DEF(         zqsfp28_irq_msk_16_9_status,port16_irq_msk,      7,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_16_9_status,port15_irq_msk,      6,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_16_9_status,port14_irq_msk,      5,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_16_9_status,port13_irq_msk,      4,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_16_9_status,port12_irq_msk,      3,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_16_9_status,port11_irq_msk,      2,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_16_9_status,port10_irq_msk,      1,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_16_9_status,port9_irq_msk,       0,     1)

FLD_RAW_RO_DEF(         zqsfp28_present_8_1_status, port_1_8_present,    0,     8)
FLD_RAW_RO_DEF(         zqsfp28_present_16_9_status,port_9_16_present,   0,     8)
FLD_RAW_RW_DEF(         zqsfp28_rst_8_1,            port_1_8_rst,        0,     8)
FLD_RAW_RW_DEF(         zqsfp28_rst_16_9,           port_9_16_rst,       0,     8)
FLD_RAW_RW_DEF(         zqsfp28_modsel_8_1,         port_1_8_modsel,     0,     8)
FLD_RAW_RW_DEF(         zqsfp28_modsel_16_9,        port_9_16_modsel,    0,     8)
FLD_RAW_RO_DEF(         zqsfp28_irq_8_1_status,     port_1_8_irq_status, 0,     8)
FLD_RAW_RO_DEF(         zqsfp28_irq_16_9_status,    port_9_16_irq_status,0,     8)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_8_1_status, port_1_8_irq_msk,    0,     8)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_16_9_status,port_9_16_irq_msk,   0,     8)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_8_1,         port_1_8_lpmode,     0,     8)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_16_9,        port_9_16_lpmode,    0,     8)

FLD_RAW_RO_DEF(         fan_tray_status,            fan_tray1_6_present, 0,     8)
FLD_RAW_RO_DEF(         psu_en_status,              psu_en_status_fld,   0,     8)
#endif /* __MASTER_CPLD_REG */
