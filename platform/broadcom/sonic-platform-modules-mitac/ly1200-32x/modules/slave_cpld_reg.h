#ifndef __SLAVE_CPLD_REG
#define __SLAVE_CPLD_REG

static int slave_cpld_raw_read(struct device *dev, struct device_attribute *attr, char *buf,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name);
static int slave_cpld_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name);

/* generic CPLD read function */
#define FLD_RAW_RD_FUNC(_reg, _fld, _wdh) static ssize_t \
slave_cpld_##_fld##_raw_read(struct device *dev, struct device_attribute *attr, char *buf) { \
    return slave_cpld_raw_read(dev, attr, buf, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
}

/* generic CPLD write function */
#define FLD_RAW_WR_FUNC(_reg, _fld, _wdh) static ssize_t \
slave_cpld_##_fld##_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { \
    return slave_cpld_raw_write(dev, attr, buf, count, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
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

/* declare slave CPLD registers */
/*                      register name                           offset  width */
/*                      --------------------------------------- ------- ----- */
REG_DEF(                slv_cpld_rev,                           0x00,   8)
REG_DEF(                slv_cpld_gpr,                           0x01,   8)
REG_DEF(                mb_brd_rev_type,                        0x02,   8)

REG_DEF(                zqsfp28_present_24_17_status,           0x10,   8)
REG_DEF(                zqsfp28_present_32_25_status,           0x11,   8)
REG_DEF(                zqsfp28_rst_24_17,                      0x12,   8)
REG_DEF(                zqsfp28_rst_32_25,                      0x13,   8)
REG_DEF(                zqsfp28_modsel_24_17,                   0x14,   8)
REG_DEF(                zqsfp28_modsel_32_25,                   0x15,   8)
REG_DEF(                zqsfp28_lpmode_24_17,                   0x16,   8)
REG_DEF(                zqsfp28_lpmode_32_25,                   0x17,   8)
REG_DEF(                zqsfp28_irq_24_17_status,               0x18,   8)
REG_DEF(                zqsfp28_irq_32_25_status,               0x19,   8)
REG_DEF(                zqsfp28_irq_msk_24_17_status,           0x1A,   8)
REG_DEF(                zqsfp28_irq_msk_32_25_status,           0x1B,   8)


/* declare slave CPLD register's fields */
/*                      register name                  field name           shift  width */
/*                      ----------------------         ----------------     ------ ----- */
FLD_RAW_RO_DEF(         slv_cpld_rev,                  mjr_rev,             4,     4)
FLD_RAW_RO_DEF(         slv_cpld_rev,                  mnr_rev,             0,     4)

FLD_RAW_RW_DEF(         slv_cpld_gpr,                  scrtch_reg,          0,     8)

FLD_RAW_RO_DEF(         mb_brd_rev_type,               brd_rev,             4,     4)
FLD_RAW_RO_DEF(         mb_brd_rev_type,               brd_type,            0,     4)

FLD_RAW_RO_DEF(         zqsfp28_present_24_17_status,  port24_present,      7,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_24_17_status,  port23_present,      6,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_24_17_status,  port22_present,      5,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_24_17_status,  port21_present,      4,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_24_17_status,  port20_present,      3,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_24_17_status,  port19_present,      2,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_24_17_status,  port18_present,      1,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_24_17_status,  port17_present,      0,     1)

FLD_RAW_RO_DEF(         zqsfp28_present_32_25_status,  port32_present,      7,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_32_25_status,  port31_present,      6,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_32_25_status,  port30_present,      5,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_32_25_status,  port29_present,      4,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_32_25_status,  port28_present,      3,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_32_25_status,  port27_present,      2,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_32_25_status,  port26_present,      1,     1)
FLD_RAW_RO_DEF(         zqsfp28_present_32_25_status,  port25_present,      0,     1)

FLD_RAW_RW_DEF(         zqsfp28_rst_24_17,             port24_rst,          7,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_24_17,             port23_rst,          6,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_24_17,             port22_rst,          5,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_24_17,             port21_rst,          4,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_24_17,             port20_rst,          3,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_24_17,             port19_rst,          2,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_24_17,             port18_rst,          1,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_24_17,             port17_rst,          0,     1)

FLD_RAW_RW_DEF(         zqsfp28_rst_32_25,             port32_rst,          7,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_32_25,             port31_rst,          6,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_32_25,             port30_rst,          5,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_32_25,             port29_rst,          4,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_32_25,             port28_rst,          3,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_32_25,             port27_rst,          2,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_32_25,             port26_rst,          1,     1)
FLD_RAW_RW_DEF(         zqsfp28_rst_32_25,             port25_rst,          0,     1)

FLD_RAW_RW_DEF(         zqsfp28_modsel_24_17,          port24_modsel,       7,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_24_17,          port23_modsel,       6,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_24_17,          port22_modsel,       5,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_24_17,          port21_modsel,       4,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_24_17,          port20_modsel,       3,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_24_17,          port19_modsel,       2,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_24_17,          port18_modsel,       1,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_24_17,          port17_modsel,       0,     1)

FLD_RAW_RW_DEF(         zqsfp28_modsel_32_25,          port32_modsel,       7,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_32_25,          port31_modsel,       6,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_32_25,          port30_modsel,       5,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_32_25,          port29_modsel,       4,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_32_25,          port28_modsel,       3,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_32_25,          port27_modsel,       2,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_32_25,          port26_modsel,       1,     1)
FLD_RAW_RW_DEF(         zqsfp28_modsel_32_25,          port25_modsel,       0,     1)

FLD_RAW_RW_DEF(         zqsfp28_lpmode_24_17,          port24_lpmode,       7,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_24_17,          port23_lpmode,       6,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_24_17,          port22_lpmode,       5,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_24_17,          port21_lpmode,       4,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_24_17,          port20_lpmode,       3,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_24_17,          port19_lpmode,       2,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_24_17,          port18_lpmode,       1,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_24_17,          port17_lpmode,       0,     1)

FLD_RAW_RW_DEF(         zqsfp28_lpmode_32_25,          port32_lpmode,       7,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_32_25,          port31_lpmode,       6,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_32_25,          port30_lpmode,       5,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_32_25,          port29_lpmode,       4,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_32_25,          port28_lpmode,       3,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_32_25,          port27_lpmode,       2,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_32_25,          port26_lpmode,       1,     1)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_32_25,          port25_lpmode,       0,     1)

FLD_RAW_RO_DEF(         zqsfp28_irq_24_17_status,      port24_irq_status,   7,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_24_17_status,      port23_irq_status,   6,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_24_17_status,      port22_irq_status,   5,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_24_17_status,      port21_irq_status,   4,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_24_17_status,      port20_irq_status,   3,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_24_17_status,      port19_irq_status,   2,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_24_17_status,      port18_irq_status,   1,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_24_17_status,      port17_irq_status,   0,     1)

FLD_RAW_RO_DEF(         zqsfp28_irq_32_25_status,      port32_irq_status,   7,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_32_25_status,      port31_irq_status,   6,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_32_25_status,      port30_irq_status,   5,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_32_25_status,      port29_irq_status,   4,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_32_25_status,      port28_irq_status,   3,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_32_25_status,      port27_irq_status,   2,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_32_25_status,      port26_irq_status,   1,     1)
FLD_RAW_RO_DEF(         zqsfp28_irq_32_25_status,      port25_irq_status,   0,     1)

FLD_RAW_RW_DEF(         zqsfp28_irq_msk_24_17_status,  port24_irq_msk,      7,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_24_17_status,  port23_irq_msk,      6,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_24_17_status,  port22_irq_msk,      5,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_24_17_status,  port21_irq_msk,      4,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_24_17_status,  port20_irq_msk,      3,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_24_17_status,  port19_irq_msk,      2,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_24_17_status,  port18_irq_msk,      1,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_24_17_status,  port17_irq_msk,      0,     1)

FLD_RAW_RW_DEF(         zqsfp28_irq_msk_32_25_status,  port32_irq_msk,      7,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_32_25_status,  port31_irq_msk,      6,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_32_25_status,  port30_irq_msk,      5,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_32_25_status,  port29_irq_msk,      4,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_32_25_status,  port28_irq_msk,      3,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_32_25_status,  port27_irq_msk,      2,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_32_25_status,  port26_irq_msk,      1,     1)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_32_25_status,  port25_irq_msk,      0,     1)

FLD_RAW_RO_DEF(         zqsfp28_present_24_17_status, port_17_24_present,   0,     8)
FLD_RAW_RO_DEF(         zqsfp28_present_32_25_status, port_25_32_present,   0,     8)
FLD_RAW_RW_DEF(         zqsfp28_rst_24_17,            port_17_24_rst,       0,     8)
FLD_RAW_RW_DEF(         zqsfp28_rst_32_25,            port_25_32_rst,       0,     8)
FLD_RAW_RW_DEF(         zqsfp28_modsel_24_17,         port_17_24_modsel,    0,     8)
FLD_RAW_RW_DEF(         zqsfp28_modsel_32_25,         port_25_32_modsel,    0,     8)
FLD_RAW_RO_DEF(         zqsfp28_irq_24_17_status,     port_17_24_irq_status,0,     8)
FLD_RAW_RO_DEF(         zqsfp28_irq_32_25_status,     port_25_32_irq_status,0,     8)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_24_17_status, port_17_24_irq_msk,   0,     8)
FLD_RAW_RW_DEF(         zqsfp28_irq_msk_32_25_status, port_25_32_irq_msk,   0,     8)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_24_17,         port_17_24_lpmode,    0,     8)
FLD_RAW_RW_DEF(         zqsfp28_lpmode_32_25,         port_25_32_lpmode,    0,     8)

#endif /* __SLAVE_CPLD_REG */
