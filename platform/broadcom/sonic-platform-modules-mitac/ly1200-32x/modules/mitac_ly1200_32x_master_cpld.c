#include <linux/module.h>
#include <linux/i2c.h>

#include "master_cpld_reg.h"
#include "master_cpld_sysfs.h"

static int debug_flag = 0;

struct master_cpld_data {
    struct mutex lock;

    struct i2c_client *client;
    struct device_attribute bin;
};


static const struct i2c_device_id master_cpld_ids[] = {
    { "master_cpld", 0 },
    { /* END OF LIST */ }
};
MODULE_DEVICE_TABLE(i2c, master_cpld_ids);

static int master_cpld_raw_read(struct device *dev, struct device_attribute *attr, char *buf,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name){
    unsigned int reg_val = 0, fld_val;
    static int debug_flag;
    struct master_cpld_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int err;

    if (reg_width != 8){
        printk("%s: Register table width setting failed.\n", reg_name);
        return -EINVAL;
    }
    mutex_lock(&data->lock);
    if ((err = i2c_smbus_read_byte_data(client, (u8)reg_offset)) < 0) {
        /* CPLD read error condition */;
        mutex_unlock(&data->lock);
        printk("%s: i2c read failed, error code = %d.\n", reg_name, err);
        return err;
    }
    reg_val = err;
    if (debug_flag) {
            printk("%s: reg_offset = %d, width = %d, cur value = 0x%x.\n", reg_name, reg_offset, reg_width, reg_val);
    }
    mutex_unlock(&data->lock);
    if (fld_width == reg_width) {fld_val = reg_val & fld_mask;}
    else {fld_val = (reg_val >> fld_shift) & fld_mask;}
    return sprintf(buf, "0x%x\n", fld_val);
}

static int master_cpld_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name){
    int ret_code;
    unsigned int reg_val, fld_val;
    unsigned long val;
    static int debug_flag;
    struct master_cpld_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    if (reg_width != 8){
        printk("%s: Register table width setting failed.\n", reg_name);
        return -EINVAL;
    }
    /* Parse buf and store to fld_val */
    if ((ret_code = kstrtoul(buf, 16, &val))){
        printk("%s: Conversion value = %s failed, errno = %d.\n", reg_name, buf, ret_code);
        return ret_code;
    }
    fld_val = (unsigned int)val;
    mutex_lock(&data->lock);
    if ((ret_code = i2c_smbus_read_byte_data(client, (u8)reg_offset)) < 0) {
        /* Handle CPLD read error condition */;
        mutex_unlock(&data->lock);
        printk("%s: i2c read failed, error code = %d.\n",  reg_name, ret_code);
        return ret_code;
    }
    reg_val = ret_code;
    if (debug_flag) {
        printk("%s: offset = %d, width = %d, cur value = 0x%x.\n", reg_name, reg_offset, reg_width, reg_val);
    }
    if (fld_width == reg_width) {reg_val = fld_val & fld_mask;}
    else {reg_val = (reg_val & ~(fld_mask << fld_shift)) |
                    ((fld_val & (fld_mask)) << fld_shift);}
    if ((ret_code = i2c_smbus_write_byte_data(client, (u8)reg_offset, (u8)reg_val)) != 0) {
        /* Handle CPLD write error condition */;
        mutex_unlock(&data->lock);
        printk("%s: i2c write failed, error code = %d.\n",  reg_name, ret_code);
        return ret_code;
    }
    else if (debug_flag) {
        printk("%s: offset = %d, width = %d, new value = 0x%x.\n", reg_name, reg_offset, reg_width, reg_val);
    }
    mutex_unlock(&data->lock);
    return count;
}

/*--------------------special file for debug---------------------- */
static ssize_t master_cpld_debug_read(struct device *dev, struct device_attribute *attr,
             char *buf){

    return sprintf(buf, "%d\n", debug_flag);
}


static ssize_t master_cpld_debug_write(struct device *dev, struct device_attribute *attr,
            const char *buf, size_t count)
{
    int temp;
    int error;

    error = kstrtoint(buf, 10, &temp);
    if (error){
        printk(KERN_INFO "%s: Conversion value = %s failed.\n", __FUNCTION__, buf);
        return count;
    }
    debug_flag = temp;

    if(debug_flag)
        printk("%s, debug_flag = %d\n", __FUNCTION__, debug_flag);

    return count;
}
SYSFS_MISC_RW_ATTR_DEF(debug, master_cpld_debug_read, master_cpld_debug_write)




/* ----------------define port group---------------------------- */
static struct attribute *port1_attributes[] = {
    SYSFS_ATTR_PTR(port1_present),        /* register: zqsfp28_present_8_1_status */
    SYSFS_ATTR_PTR(port1_rst),            /* register: zqsfp28_rst_8_1 */
    SYSFS_ATTR_PTR(port1_modsel),         /* register: zqsfp28_modsel_8_1 */
    SYSFS_ATTR_PTR(port1_lpmode),         /* register: zqsfp28_lpmode_8_1 */
    SYSFS_ATTR_PTR(port1_irq_status),     /* register: zqsfp28_irq_8_1_status */
    SYSFS_ATTR_PTR(port1_irq_msk),        /* register: zqsfp28_irq_msk_8_1_status */
    NULL
};


static struct attribute *port2_attributes[] = {
    SYSFS_ATTR_PTR(port2_present),        /* register: zqsfp28_present_8_1_status */
    SYSFS_ATTR_PTR(port2_rst),            /* register: zqsfp28_rst_8_1 */
    SYSFS_ATTR_PTR(port2_modsel),         /* register: zqsfp28_modsel_8_1 */
    SYSFS_ATTR_PTR(port2_lpmode),         /* register: zqsfp28_lpmode_8_1 */
    SYSFS_ATTR_PTR(port2_irq_status),     /* register: zqsfp28_irq_8_1_status */
    SYSFS_ATTR_PTR(port2_irq_msk),        /* register: zqsfp28_irq_msk_8_1_status */
    NULL
};


static struct attribute *port3_attributes[] = {
    SYSFS_ATTR_PTR(port3_present),        /* register: zqsfp28_present_8_1_status */
    SYSFS_ATTR_PTR(port3_rst),            /* register: zqsfp28_rst_8_1 */
    SYSFS_ATTR_PTR(port3_modsel),         /* register: zqsfp28_modsel_8_1 */
    SYSFS_ATTR_PTR(port3_lpmode),         /* register: zqsfp28_lpmode_8_1 */
    SYSFS_ATTR_PTR(port3_irq_status),     /* register: zqsfp28_irq_8_1_status */
    SYSFS_ATTR_PTR(port3_irq_msk),        /* register: zqsfp28_irq_msk_8_1_status */
    NULL
};

static struct attribute *port4_attributes[] = {
    SYSFS_ATTR_PTR(port4_present),        /* register: zqsfp28_present_8_1_status */
    SYSFS_ATTR_PTR(port4_rst),            /* register: zqsfp28_rst_8_1 */
    SYSFS_ATTR_PTR(port4_modsel),         /* register: zqsfp28_modsel_8_1 */
    SYSFS_ATTR_PTR(port4_lpmode),         /* register: zqsfp28_lpmode_8_1 */
    SYSFS_ATTR_PTR(port4_irq_status),     /* register: zqsfp28_irq_8_1_status */
    SYSFS_ATTR_PTR(port4_irq_msk),        /* register: zqsfp28_irq_msk_8_1_status */
    NULL
};

static struct attribute *port5_attributes[] = {
    SYSFS_ATTR_PTR(port5_present),        /* register: zqsfp28_present_8_1_status */
    SYSFS_ATTR_PTR(port5_rst),            /* register: zqsfp28_rst_8_1 */
    SYSFS_ATTR_PTR(port5_modsel),         /* register: zqsfp28_modsel_8_1 */
    SYSFS_ATTR_PTR(port5_lpmode),         /* register: zqsfp28_lpmode_8_1 */
    SYSFS_ATTR_PTR(port5_irq_status),     /* register: zqsfp28_irq_8_1_status */
    SYSFS_ATTR_PTR(port5_irq_msk),        /* register: zqsfp28_irq_msk_8_1_status */
    NULL
};

static struct attribute *port6_attributes[] = {
    SYSFS_ATTR_PTR(port6_present),        /* register: zqsfp28_present_8_1_status */
    SYSFS_ATTR_PTR(port6_rst),            /* register: zqsfp28_rst_8_1 */
    SYSFS_ATTR_PTR(port6_modsel),         /* register: zqsfp28_modsel_8_1 */
    SYSFS_ATTR_PTR(port6_lpmode),         /* register: zqsfp28_lpmode_8_1 */
    SYSFS_ATTR_PTR(port6_irq_status),     /* register: zqsfp28_irq_8_1_status */
    SYSFS_ATTR_PTR(port6_irq_msk),        /* register: zqsfp28_irq_msk_8_1_status */
    NULL
};

static struct attribute *port7_attributes[] = {
    SYSFS_ATTR_PTR(port7_present),        /* register: zqsfp28_present_8_1_status */
    SYSFS_ATTR_PTR(port7_rst),            /* register: zqsfp28_rst_8_1 */
    SYSFS_ATTR_PTR(port7_modsel),         /* register: zqsfp28_modsel_8_1 */
    SYSFS_ATTR_PTR(port7_lpmode),         /* register: zqsfp28_lpmode_8_1 */
    SYSFS_ATTR_PTR(port7_irq_status),     /* register: zqsfp28_irq_8_1_status */
    SYSFS_ATTR_PTR(port7_irq_msk),        /* register: zqsfp28_irq_msk_8_1_status */
    NULL
};

static struct attribute *port8_attributes[] = {
    SYSFS_ATTR_PTR(port8_present),        /* register: zqsfp28_present_8_1_status */
    SYSFS_ATTR_PTR(port8_rst),            /* register: zqsfp28_rst_8_1 */
    SYSFS_ATTR_PTR(port8_modsel),         /* register: zqsfp28_modsel_8_1 */
    SYSFS_ATTR_PTR(port8_lpmode),         /* register: zqsfp28_lpmode_8_1 */
    SYSFS_ATTR_PTR(port8_irq_status),     /* register: zqsfp28_irq_8_1_status */
    SYSFS_ATTR_PTR(port8_irq_msk),        /* register: zqsfp28_irq_msk_8_1_status */
    NULL
};

static struct attribute *port9_attributes[] = {
    SYSFS_ATTR_PTR(port9_present),        /* register: zqsfp28_present_16_9_status */
    SYSFS_ATTR_PTR(port9_rst),            /* register: zqsfp28_rst_16_9 */
    SYSFS_ATTR_PTR(port9_modsel),         /* register: zqsfp28_modsel_16_9 */
    SYSFS_ATTR_PTR(port9_lpmode),         /* register: zqsfp28_lpmode_16_9 */
    SYSFS_ATTR_PTR(port9_irq_status),     /* register: zqsfp28_irq_16_9_status */
    SYSFS_ATTR_PTR(port9_irq_msk),        /* register: zqsfp28_irq_msk_16_9_status */
    NULL
};

static struct attribute *port10_attributes[] = {
    SYSFS_ATTR_PTR(port10_present),       /* register: zqsfp28_present_16_9_status */
    SYSFS_ATTR_PTR(port10_rst),           /* register: zqsfp28_rst_16_9 */
    SYSFS_ATTR_PTR(port10_modsel),        /* register: zqsfp28_modsel_16_9 */
    SYSFS_ATTR_PTR(port10_lpmode),        /* register: zqsfp28_lpmode_16_9 */
    SYSFS_ATTR_PTR(port10_irq_status),    /* register: zqsfp28_irq_16_9_status */
    SYSFS_ATTR_PTR(port10_irq_msk),       /* register: zqsfp28_irq_msk_16_9_status */
    NULL
};

static struct attribute *port11_attributes[] = {
    SYSFS_ATTR_PTR(port11_present),       /* register: zqsfp28_present_16_9_status */
    SYSFS_ATTR_PTR(port11_rst),           /* register: zqsfp28_rst_16_9 */
    SYSFS_ATTR_PTR(port11_modsel),        /* register: zqsfp28_modsel_16_9 */
    SYSFS_ATTR_PTR(port11_lpmode),        /* register: zqsfp28_lpmode_16_9 */
    SYSFS_ATTR_PTR(port11_irq_status),    /* register: zqsfp28_irq_16_9_status */
    SYSFS_ATTR_PTR(port11_irq_msk),       /* register: zqsfp28_irq_msk_16_9_status */
    NULL
};

static struct attribute *port12_attributes[] = {
    SYSFS_ATTR_PTR(port12_present),       /* register: zqsfp28_present_16_9_status */
    SYSFS_ATTR_PTR(port12_rst),           /* register: zqsfp28_rst_16_9 */
    SYSFS_ATTR_PTR(port12_modsel),        /* register: zqsfp28_modsel_16_9 */
    SYSFS_ATTR_PTR(port12_lpmode),        /* register: zqsfp28_lpmode_16_9 */
    SYSFS_ATTR_PTR(port12_irq_status),    /* register: zqsfp28_irq_16_9_status */
    SYSFS_ATTR_PTR(port12_irq_msk),       /* register: zqsfp28_irq_msk_16_9_status */
    NULL
};

static struct attribute *port13_attributes[] = {
    SYSFS_ATTR_PTR(port13_present),       /* register: zqsfp28_present_16_9_status */
    SYSFS_ATTR_PTR(port13_rst),           /* register: zqsfp28_rst_16_9 */
    SYSFS_ATTR_PTR(port13_modsel),        /* register: zqsfp28_modsel_16_9 */
    SYSFS_ATTR_PTR(port13_lpmode),        /* register: zqsfp28_lpmode_16_9 */
    SYSFS_ATTR_PTR(port13_irq_status),    /* register: zqsfp28_irq_16_9_status */
    SYSFS_ATTR_PTR(port13_irq_msk),       /* register: zqsfp28_irq_msk_16_9_status */
    NULL
};

static struct attribute *port14_attributes[] = {
    SYSFS_ATTR_PTR(port14_present),       /* register: zqsfp28_present_16_9_status */
    SYSFS_ATTR_PTR(port14_rst),           /* register: zqsfp28_rst_16_9 */
    SYSFS_ATTR_PTR(port14_modsel),        /* register: zqsfp28_modsel_16_9 */
    SYSFS_ATTR_PTR(port14_lpmode),        /* register: zqsfp28_lpmode_16_9 */
    SYSFS_ATTR_PTR(port14_irq_status),    /* register: zqsfp28_irq_16_9_status */
    SYSFS_ATTR_PTR(port14_irq_msk),       /* register: zqsfp28_irq_msk_16_9_status */
    NULL
};

static struct attribute *port15_attributes[] = {
    SYSFS_ATTR_PTR(port15_present),       /* register: zqsfp28_present_16_9_status */
    SYSFS_ATTR_PTR(port15_rst),           /* register: zqsfp28_rst_16_9 */
    SYSFS_ATTR_PTR(port15_modsel),        /* register: zqsfp28_modsel_16_9 */
    SYSFS_ATTR_PTR(port15_lpmode),        /* register: zqsfp28_lpmode_16_9 */
    SYSFS_ATTR_PTR(port15_irq_status),    /* register: zqsfp28_irq_16_9_status */
    SYSFS_ATTR_PTR(port15_irq_msk),       /* register: zqsfp28_irq_msk_16_9_status */
    NULL
};

static struct attribute *port16_attributes[] = {
    SYSFS_ATTR_PTR(port16_present),       /* register: zqsfp28_present_16_9_status */
    SYSFS_ATTR_PTR(port16_rst),           /* register: zqsfp28_rst_16_9 */
    SYSFS_ATTR_PTR(port16_modsel),        /* register: zqsfp28_modsel_16_9 */
    SYSFS_ATTR_PTR(port16_lpmode),        /* register: zqsfp28_lpmode_16_9 */
    SYSFS_ATTR_PTR(port16_irq_status),    /* register: zqsfp28_irq_16_9_status */
    SYSFS_ATTR_PTR(port16_irq_msk),       /* register: zqsfp28_irq_msk_16_9_status */
    NULL
};


static const struct attribute_group master_cpld_port_group[] = {
    {.attrs = port1_attributes,
    .name = "port1",},
    {.attrs = port2_attributes,
    .name = "port2",},
    {.attrs = port3_attributes,
    .name = "port3",},
    {.attrs = port4_attributes,
    .name = "port4",},
    {.attrs = port5_attributes,
    .name = "port5",},
    {.attrs = port6_attributes,
    .name = "port6",},
    {.attrs = port7_attributes,
    .name = "port7",},
    {.attrs = port8_attributes,
    .name = "port8",},
    {.attrs = port9_attributes,
    .name = "port9",},
    {.attrs = port10_attributes,
    .name = "port10",},
    {.attrs = port11_attributes,
    .name = "port11",},
    {.attrs = port12_attributes,
    .name = "port12",},
    {.attrs = port13_attributes,
    .name = "port13",},
    {.attrs = port14_attributes,
    .name = "port14",},
    {.attrs = port15_attributes,
    .name = "port15",},
    {.attrs = port16_attributes,
    .name = "port16",}
};

/* ----------------define misc group---------------------------- */
static struct attribute *misc_attributes[] = {
    SYSFS_ATTR_PTR(mjr_rev),             /* register: mstr_cpld_rev */
    SYSFS_ATTR_PTR(mnr_rev),             /* register: mstr_cpld_rev */

    SYSFS_ATTR_PTR(scrtch_reg),          /* register: mstr_cpld_gpr */

    SYSFS_ATTR_PTR(brd_rev),             /* register: mb_brd_rev_type */
    SYSFS_ATTR_PTR(brd_type),            /* register: mb_brd_rev_type */

    SYSFS_ATTR_PTR(mb_rst),              /* register: mstr_srr */
    SYSFS_ATTR_PTR(npu_rst),             /* register: mstr_srr */
    SYSFS_ATTR_PTR(mgmt_phy_rst),        /* register: mstr_srr */

    SYSFS_ATTR_PTR(mb_eeprom_wp),        /* register: eeprom_wp */
    SYSFS_ATTR_PTR(cpld_spi_wp),         /* register: eeprom_wp */
    SYSFS_ATTR_PTR(fan_eeprom_wp),       /* register: eeprom_wp */

    SYSFS_ATTR_PTR(ps2_int_msk),         /* register: mstr_irq */
    SYSFS_ATTR_PTR(ps1_int_msk),         /* register: mstr_irq */
    SYSFS_ATTR_PTR(usb_fault_msk),       /* register: mstr_irq */
    SYSFS_ATTR_PTR(pcie_int_msk),        /* register: mstr_irq */
    SYSFS_ATTR_PTR(fan_alert_int_msk),   /* register: mstr_irq */
    SYSFS_ATTR_PTR(usb_fault),           /* register: mstr_irq */
    SYSFS_ATTR_PTR(pcie_int),            /* register: mstr_irq */
    SYSFS_ATTR_PTR(fan_alert_int),       /* register: mstr_irq */
    SYSFS_ATTR_PTR(system_led_fld),      /* register: mstr_irq */
    SYSFS_ATTR_PTR(power_led),           /* register: mstr_irq */
    SYSFS_ATTR_PTR(fan_led),             /* register: mstr_irq */
    SYSFS_ATTR_PTR(locate_led),          /* register: mstr_irq */

    SYSFS_ATTR_PTR(led_test),            /* register: fan_tray_3_1_led */
    SYSFS_ATTR_PTR(fan_tray3_led),       /* register: fan_tray_3_1_led */
    SYSFS_ATTR_PTR(fan_tray2_led),       /* register: fan_tray_3_1_led */
    SYSFS_ATTR_PTR(fan_tray1_led),       /* register: fan_tray_3_1_led */
    SYSFS_ATTR_PTR(fan_tray6_led),       /* register: fan_tray_6_4_led */
    SYSFS_ATTR_PTR(fan_tray5_led),       /* register: fan_tray_6_4_led */
    SYSFS_ATTR_PTR(fan_tray4_led),       /* register: fan_tray_6_4_led */

    SYSFS_ATTR_PTR(fan_tray6_present),   /* register: fan_tray_status */
    SYSFS_ATTR_PTR(fan_tray5_present),   /* register: fan_tray_status */
    SYSFS_ATTR_PTR(fan_tray4_present),   /* register: fan_tray_status */
    SYSFS_ATTR_PTR(fan_tray3_present),   /* register: fan_tray_status */
    SYSFS_ATTR_PTR(fan_tray2_present),   /* register: fan_tray_status */
    SYSFS_ATTR_PTR(fan_tray1_present),   /* register: fan_tray_status */

    SYSFS_ATTR_PTR(fan_type6),           /* register: fan_type_status */
    SYSFS_ATTR_PTR(fan_type5),           /* register: fan_type_status */
    SYSFS_ATTR_PTR(fan_type4),           /* register: fan_type_status */
    SYSFS_ATTR_PTR(fan_type3),           /* register: fan_type_status */
    SYSFS_ATTR_PTR(fan_type2),           /* register: fan_type_status */
    SYSFS_ATTR_PTR(fan_type1),           /* register: fan_type_status */

    SYSFS_ATTR_PTR(ps1_ps),              /* register: psu_en_status */
    SYSFS_ATTR_PTR(ps1_pg),              /* register: psu_en_status */
    SYSFS_ATTR_PTR(ps1_int),             /* register: psu_en_status */
    SYSFS_ATTR_PTR(ps1_on),              /* register: psu_en_status */
    SYSFS_ATTR_PTR(ps2_ps),              /* register: psu_en_status */
    SYSFS_ATTR_PTR(ps2_pg),              /* register: psu_en_status */
    SYSFS_ATTR_PTR(ps2_int),             /* register: psu_en_status */
    SYSFS_ATTR_PTR(ps2_on),              /* register: psu_en_status */

    SYSFS_ATTR_PTR(usb1_vbus_en),        /* register: mb_pwr_en_status */
    SYSFS_ATTR_PTR(v5p0_en),             /* register: mb_pwr_en_status */
    SYSFS_ATTR_PTR(v3p3_en),             /* register: mb_pwr_en_status */
    SYSFS_ATTR_PTR(vcc_1v8_en),          /* register: mb_pwr_en_status */
    SYSFS_ATTR_PTR(mac_avs1v_en),        /* register: mb_pwr_en_status */
    SYSFS_ATTR_PTR(mac1v_en),            /* register: mb_pwr_en_status */
    SYSFS_ATTR_PTR(vcc_1v25_en),         /* register: mb_pwr_en_status */

    SYSFS_ATTR_PTR(vcc_3p3_cpld),        /* register: mb_pwr_status */
    SYSFS_ATTR_PTR(vcc5v_pg),            /* register: mb_pwr_status */
    SYSFS_ATTR_PTR(vcc3v3_pg),           /* register: mb_pwr_status */
    SYSFS_ATTR_PTR(vcc_1v8_pg),          /* register: mb_pwr_status */
    SYSFS_ATTR_PTR(mac_avs1v_pg),        /* register: mb_pwr_status */
    SYSFS_ATTR_PTR(mac1v_pg),            /* register: mb_pwr_status */
    SYSFS_ATTR_PTR(vcc_1v25_pg),         /* register: mb_pwr_status */

    SYSFS_ATTR_PTR(port_1_8_present),    /* register: zqsfp28_present_8_1_status */
    SYSFS_ATTR_PTR(port_9_16_present),   /* register: zqsfp28_present_16_9_status */
    SYSFS_ATTR_PTR(port_1_8_rst),        /* register: zqsfp28_rst_8_1 */
    SYSFS_ATTR_PTR(port_9_16_rst),       /* register: zqsfp28_rst_16_9 */
    SYSFS_ATTR_PTR(port_1_8_modsel),     /* register: zqsfp28_modsel_8_1 */
    SYSFS_ATTR_PTR(port_9_16_modsel),    /* register: zqsfp28_modsel_16_9 */
    SYSFS_ATTR_PTR(port_1_8_irq_status), /* register: zqsfp28_irq_8_1_status */
    SYSFS_ATTR_PTR(port_9_16_irq_status),/* register: zqsfp28_irq_16_9_status */
    SYSFS_ATTR_PTR(port_1_8_irq_msk),    /* register: zqsfp28_irq_msk_8_1_status */
    SYSFS_ATTR_PTR(port_9_16_irq_msk),   /* register: zqsfp28_irq_msk_16_9_status */
    SYSFS_ATTR_PTR(port_1_8_lpmode),     /* register: zqsfp28_lpmode_8_1 */
    SYSFS_ATTR_PTR(port_9_16_lpmode),    /* register: zqsfp28_lpmode_16_9 */

    SYSFS_ATTR_PTR(fan_tray1_6_present), /* register: fan_tray_status */
    SYSFS_ATTR_PTR(psu_en_status_fld),   /* register: psu_en_status*/

    SYSFS_ATTR_PTR(debug),               /* debug flag for print more messages */
    NULL
};
static const struct attribute_group master_cpld_group_misc = {
    .attrs = misc_attributes,
};

static int master_cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct master_cpld_data *master_cpld;
    int err, i;
    int grp_number = (int)( sizeof(master_cpld_port_group) / sizeof(master_cpld_port_group[0]));

    /* allocate memory to master_cpld */
    master_cpld = devm_kzalloc(&client->dev, sizeof(struct master_cpld_data) , GFP_KERNEL);

    if (!master_cpld)
        return -ENOMEM;

    mutex_init(&master_cpld->lock);

    for(i = 0 ; i < grp_number ; i++){
        err = sysfs_create_group(&client->dev.kobj, &master_cpld_port_group[i]);
        if (err){
            printk("%s: Error creeat port group %d.\n", __FUNCTION__, i+1);
        }
    }
    err = sysfs_create_group(&client->dev.kobj, &master_cpld_group_misc);
    if (err){
        printk("%s: Error creeat misc group.\n", __FUNCTION__);
    }

    master_cpld->client = client;
    i2c_set_clientdata(client, master_cpld);

    printk(KERN_INFO "%s: Master CPLD LCMXO3LF created.\n", __FUNCTION__);

    return 0;

}

static int master_cpld_remove(struct i2c_client *client)
{
    int i;
    int grp_number = (int)( sizeof(master_cpld_port_group) / sizeof(master_cpld_port_group[0]));

    for(i = 0 ; i < grp_number ; i++){
        sysfs_remove_group(&client->dev.kobj, &master_cpld_port_group[i]);
    }
    sysfs_remove_group(&client->dev.kobj, &master_cpld_group_misc);

    printk(KERN_INFO "%s: Master CPLD removed.\n", __FUNCTION__);
    return 0;
}

static struct i2c_driver master_cpld_driver = {
    .driver = {
        .name = "master_cpld",
        .owner = THIS_MODULE,
    },
    .probe = master_cpld_probe,
    .remove = master_cpld_remove,
    .id_table = master_cpld_ids,
};

static int __init master_cpld_init(void)
{
    printk(KERN_INFO "%s: init.\n", __FUNCTION__);
    return i2c_add_driver(&master_cpld_driver);
}
module_init(master_cpld_init);

static void __exit master_cpld_exit(void)
{
    printk(KERN_INFO "%s: exit.\n", __FUNCTION__);
    i2c_del_driver(&master_cpld_driver);
}
module_exit(master_cpld_exit);

MODULE_DESCRIPTION("mitac_ly1200_32x_master_cpld driver");
MODULE_AUTHOR("Eddy Weng <eddy.weng@mic.com.tw>");
MODULE_LICENSE("GPL");

