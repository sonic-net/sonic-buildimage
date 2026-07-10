#include "delta_ag9064_common.h"

unsigned char swpld1_reg_addr;
unsigned char swpld2_reg_addr;
unsigned char swpld3_reg_addr;
unsigned char swpld4_reg_addr;

/*----------------    CPLD  - start   ------------- */
/*    SWPLD1  -- device   */
static struct platform_device swpld1_device = {
    .name               = "delta-ag9064-swpld1",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9064_swpld1_platform_data,
                .release         = device_release
    },
};

static struct platform_device swpld2_device = {
    .name               = "delta-ag9064-swpld2",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9064_swpld2_platform_data,
                .release         = device_release
    },
};
static struct platform_device swpld3_device = {
    .name               = "delta-ag9064-swpld3",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9064_swpld3_platform_data,
                .release         = device_release
    },
};
static struct platform_device swpld4_device = {
    .name               = "delta-ag9064-swpld4",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9064_swpld4_platform_data,
                .release         = device_release
    },
};

static ssize_t get_swpld_reg(struct device *dev, struct device_attribute *dev_attr, char *buf) 
{
    int ret;
    int mask;
    int value;
    int cmd_data_len;
    char note[200];
    uint8_t cmd_data[4]={0};
    uint8_t get_cmd;       
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);

    dni_klock();
    cmd_data_len = sizeof(cmd_data);
    get_cmd = CMD_GETDATA;
    cmd_data[0] = BMC_BUS_5;
    cmd_data[3] = 1;
    mask  = attribute_data[attr->index].mask;
    sprintf(note, "\n%s\n",attribute_data[attr->index].note);

    if (attr->index <= SWPLD4_REG_VALUE){
        switch (attr->index) {
            case SWPLD1_REG_ADDR:
                dni_kunlock();
                return sprintf(buf, "0x%02x\n", swpld1_reg_addr);
            case SWPLD2_REG_ADDR:
                dni_kunlock();
                return sprintf(buf, "0x%02x\n", swpld2_reg_addr);
            case SWPLD3_REG_ADDR:
                dni_kunlock();
                return sprintf(buf, "0x%02x\n", swpld3_reg_addr);
            case SWPLD4_REG_ADDR:
                dni_kunlock();
                return sprintf(buf, "0x%02x\n", swpld4_reg_addr);
            case SWPLD1_REG_VALUE:
                cmd_data[1] = SWPLD1_ADDR;
                cmd_data[2] = swpld1_reg_addr;
                break;
            case SWPLD2_REG_VALUE:
                cmd_data[1] = SWPLD2_ADDR;
                cmd_data[2] = swpld2_reg_addr;
                break;
            case SWPLD3_REG_VALUE:
                cmd_data[1] = SWPLD3_ADDR;
                cmd_data[2] = swpld3_reg_addr;
                break;
            case SWPLD4_REG_VALUE:
                cmd_data[1] = SWPLD4_ADDR;
                cmd_data[2] = swpld4_reg_addr;
                break;
            default:
                dni_kunlock();
                return sprintf(buf, "%d not found", attr->index);
        }
        ret = dni_bmc_cmd(get_cmd, cmd_data, cmd_data_len);
        ret = ret & 0xff;
        dni_kunlock();
        return sprintf(buf, "0x%02x\n", ret);
    }else{

        switch (attr->index) {
            case SWPLD1_MAJOR_VER ... PSU_LED_MODE :
                cmd_data[1] = SWPLD1_ADDR;
                break;
            case SWPLD2_MAJOR_VER ... FAN_MOD4_LED :
                cmd_data[1] = SWPLD2_ADDR;
                break;
            case SWPLD3_MAJOR_VER ... PLATFORM_TYPE :
                cmd_data[1] = SWPLD3_ADDR;
                break;
            case SW_BOARD_ID1 ... FAN_EEPROM_WP :
                cmd_data[1] = SWPLD4_ADDR;
                break;
            default:
                dni_kunlock();
                return sprintf(buf, "%d not found", attr->index);
        }
        cmd_data[2] = attribute_data[attr->index].reg;
        value = dni_bmc_cmd(get_cmd, cmd_data, cmd_data_len);
        value = value & mask;
        switch (mask) {
            case 0xFF:
                dni_kunlock();
                return sprintf(buf, "0x%02x%s", value, note);
            case 0x0F:
                dni_kunlock();
                return sprintf(buf, "0x%01x%s", value, note);
            case 0xF0:
                value = value >> 4;
                dni_kunlock();
                return sprintf(buf, "0x%01x%s", value, note);
            case 0xC0:
                value = value >> 6;
                dni_kunlock();
                return sprintf(buf, "0x%01x%s", value, note);
            case 0x30:
                value = value >> 4;
                dni_kunlock();
                return sprintf(buf, "0x%01x%s", value, note);       
            default :
                value = value >> dni_log2(mask);
                dni_kunlock();
                return sprintf(buf, "%d%s", value, note);
        }        
    }
    dni_kunlock();
    return sprintf(buf, "%d not found", attr->index); 
}

static ssize_t set_swpld_reg(struct device *dev, struct device_attribute *dev_attr,
             const char *buf, size_t count)
{
    int err; 
    int value; 
    int set_data; 
    int cmd_data_len;    
    uint8_t cmd_data[4]={0};
    uint8_t set_cmd;
    uint8_t get_cmd;
    unsigned long set_data_ul;
    unsigned char mask;  
    unsigned char mask_out;      
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);

    cmd_data_len = sizeof(cmd_data);
    set_cmd = CMD_SETDATA;
    get_cmd = CMD_GETDATA;

    dni_klock();

    err = kstrtoul(buf, 0, &set_data_ul);
    if (err){
        dni_kunlock();
        return err;
    }

    if (set_data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        dni_kunlock();
        return count;
    }

    set_data = (int)set_data_ul;
    if (attr->index <= SWPLD4_REG_VALUE){
        cmd_data[0] = BMC_BUS_5;
        cmd_data[3] = set_data;
        switch (attr->index) {
        //reg_addr
            case SWPLD1_REG_ADDR:
                swpld1_reg_addr = set_data;
                dni_kunlock();
                return count;
            case SWPLD2_REG_ADDR:
                swpld2_reg_addr = set_data;
                dni_kunlock();
                return count;
            case SWPLD3_REG_ADDR:
                swpld3_reg_addr = set_data;
                dni_kunlock();
                return count;         
            case SWPLD4_REG_ADDR:
                swpld4_reg_addr = set_data;
                dni_kunlock();
                return count;
        //reg_value       
            case SWPLD1_REG_VALUE:
                cmd_data[1] = SWPLD1_ADDR;
                cmd_data[2] = swpld1_reg_addr;
                break;
            case SWPLD2_REG_VALUE:
                cmd_data[1] = SWPLD2_ADDR;
                cmd_data[2] = swpld2_reg_addr;
                break;
            case SWPLD3_REG_VALUE:
                cmd_data[1] = SWPLD3_ADDR;
                cmd_data[2] = swpld3_reg_addr;
                break;
            case SWPLD4_REG_VALUE:
                cmd_data[1] = SWPLD4_ADDR;
                cmd_data[2] = swpld4_reg_addr;
                break;
            default :
                dni_kunlock();
                return sprintf(buf, "%d not found", attr->index); 
        }
        dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
        dni_kunlock();
        return count;
    }
    else{
        cmd_data[0] = BMC_BUS_5;
        cmd_data[2] = attribute_data[attr->index].reg;
        cmd_data[3] = 1;        
        switch (attr->index) {
        //attributes      
            case SWPLD1_MAJOR_VER ... PSU_LED_MODE://SWPLD1
                cmd_data[1] = SWPLD1_ADDR;
                break;
            case SWPLD2_MAJOR_VER ... FAN_MOD4_LED://SWPLD2
                cmd_data[1] = SWPLD2_ADDR;
                break;
            case SWPLD3_MAJOR_VER ... PLATFORM_TYPE://SWPLD3
                cmd_data[1] = SWPLD3_ADDR;
                break;
            case SW_BOARD_ID1 ... FAN_EEPROM_WP://SWPLD4
                cmd_data[1] = SWPLD4_ADDR;
                break;
            default:
                dni_kunlock();
                return sprintf(buf, "%d not found", attr->index); 
        }

        value = dni_bmc_cmd(get_cmd, cmd_data, cmd_data_len);
        mask  = attribute_data[attr->index].mask;
        mask_out = value & ~(mask);
        cmd_data[3] = set_data;
        switch (mask) {
            case 0xFF:
                set_data = mask_out | (set_data & mask);  
                break;
            case 0x0F:
                set_data = mask_out | (set_data & mask);
                break;
            case 0xF0:
                set_data = set_data << 4;
                set_data = mask_out | (set_data & mask);
                break; 
            case 0xC0:
                set_data = set_data << 6;
                set_data = mask_out | (set_data & mask);
                break;
            case 0x30:
                set_data = set_data << 4;
                set_data = mask_out | (set_data & mask);
                break;
            default :
                set_data = mask_out | (set_data << dni_log2(mask) );        
        }   
        dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
        dni_kunlock();
        return count;
    }
    dni_kunlock();
    return count;
}

//SWPLD
static SENSOR_DEVICE_ATTR(swpld1_reg_addr,   S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD1_REG_ADDR);
static SENSOR_DEVICE_ATTR(swpld1_reg_value,  S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD1_REG_VALUE);
static SENSOR_DEVICE_ATTR(swpld2_reg_addr,   S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD2_REG_ADDR);
static SENSOR_DEVICE_ATTR(swpld2_reg_value,  S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD2_REG_VALUE);
static SENSOR_DEVICE_ATTR(swpld3_reg_addr,   S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD3_REG_ADDR);
static SENSOR_DEVICE_ATTR(swpld3_reg_value,  S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD3_REG_VALUE);
static SENSOR_DEVICE_ATTR(swpld4_reg_addr,   S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD4_REG_ADDR);
static SENSOR_DEVICE_ATTR(swpld4_reg_value,  S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD4_REG_VALUE);
//SWPLD1
static SENSOR_DEVICE_ATTR(swpld1_major_ver,  S_IRUGO,           get_swpld_reg, NULL,          SWPLD1_MAJOR_VER);
static SENSOR_DEVICE_ATTR(swpld1_minor_ver,  S_IRUGO,           get_swpld_reg, NULL,          SWPLD1_MINOR_VER);
static SENSOR_DEVICE_ATTR(swpld1_scrtch_reg, S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD1_SCRTCH_REG);
static SENSOR_DEVICE_ATTR(psu1_pwr_ok,       S_IRUGO,           get_swpld_reg, NULL,          PSU1_PWR_OK);
static SENSOR_DEVICE_ATTR(psu1_int,          S_IRUGO,           get_swpld_reg, NULL,          PSU1_INT);
static SENSOR_DEVICE_ATTR(psu2_pwr_ok,       S_IRUGO,           get_swpld_reg, NULL,          PSU2_PWR_OK);
static SENSOR_DEVICE_ATTR(psu2_int,          S_IRUGO,           get_swpld_reg, NULL,          PSU2_INT);
static SENSOR_DEVICE_ATTR(psu1_green_led,    S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, PSU1_GREEN_LED);
static SENSOR_DEVICE_ATTR(psu1_red_led,      S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, PSU1_RED_LED);
static SENSOR_DEVICE_ATTR(psu2_green_led,    S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, PSU2_GREEN_LED);
static SENSOR_DEVICE_ATTR(psu2_red_led,      S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, PSU2_RED_LED);
static SENSOR_DEVICE_ATTR(psu_led_mode,      S_IRUGO,           get_swpld_reg, NULL,          PSU_LED_MODE);
//SWPLD2
static SENSOR_DEVICE_ATTR(swpld2_major_ver,  S_IRUGO,           get_swpld_reg, NULL,          SWPLD2_MAJOR_VER);
static SENSOR_DEVICE_ATTR(swpld2_minor_ver,  S_IRUGO,           get_swpld_reg, NULL,          SWPLD2_MINOR_VER);
static SENSOR_DEVICE_ATTR(swpld2_scrtch_reg, S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD2_SCRTCH_REG);
static SENSOR_DEVICE_ATTR(fan_led,           S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, FAN_LED);
static SENSOR_DEVICE_ATTR(sys_led,           S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SYS_LED);
static SENSOR_DEVICE_ATTR(fan_mod1_led,      S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, FAN_MOD1_LED);
static SENSOR_DEVICE_ATTR(fan_mod2_led,      S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, FAN_MOD2_LED);
static SENSOR_DEVICE_ATTR(fan_mod3_led,      S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, FAN_MOD3_LED);
static SENSOR_DEVICE_ATTR(fan_mod4_led,      S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, FAN_MOD4_LED);
//SWPLD3
static SENSOR_DEVICE_ATTR(swpld3_major_ver,  S_IRUGO,           get_swpld_reg, NULL,          SWPLD3_MAJOR_VER);
static SENSOR_DEVICE_ATTR(swpld3_minor_ver,  S_IRUGO,           get_swpld_reg, NULL,          SWPLD3_MINOR_VER);
static SENSOR_DEVICE_ATTR(swpld3_scrtch_reg, S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SWPLD3_SCRTCH_REG);
static SENSOR_DEVICE_ATTR(sb_ver,            S_IRUGO,           get_swpld_reg, NULL,          SB_VER);
static SENSOR_DEVICE_ATTR(platform_type,     S_IRUGO,           get_swpld_reg, NULL,          PLATFORM_TYPE);

//SWPLD4
static SENSOR_DEVICE_ATTR(sw_board_id1,      S_IRUGO,           get_swpld_reg, NULL,          SW_BOARD_ID1);
static SENSOR_DEVICE_ATTR(sw_board_id2,      S_IRUGO,           get_swpld_reg, NULL,          SW_BOARD_ID2);
static SENSOR_DEVICE_ATTR(swbd_ver,          S_IRUGO,           get_swpld_reg, NULL,          SWBD_VER);
static SENSOR_DEVICE_ATTR(swpld4_ver,        S_IRUGO,           get_swpld_reg, NULL,          SWPLD4_VER);
static SENSOR_DEVICE_ATTR(psu_fan_event,     S_IRUGO,           get_swpld_reg, NULL,          PSU_FAN_EVENT);
static SENSOR_DEVICE_ATTR(cpu_thermal_int,   S_IRUGO,           get_swpld_reg, NULL,          CPU_THERMAL_INT);
static SENSOR_DEVICE_ATTR(fan_int,           S_IRUGO,           get_swpld_reg, NULL,          FAN_INT);
static SENSOR_DEVICE_ATTR(cpld_spi_wp,       S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, CPLD_SPI_WP);
static SENSOR_DEVICE_ATTR(rj45_console_sel,  S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, RJ45_CONSOLE_SEL);
static SENSOR_DEVICE_ATTR(system_int,        S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, SYSTEM_INT);
static SENSOR_DEVICE_ATTR(cpld_mb_rst_done,  S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, CPLD_MB_RST_DONE);
static SENSOR_DEVICE_ATTR(mb_pwr_ok,         S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, MB_PWR_OK);
static SENSOR_DEVICE_ATTR(fan_eeprom_wp,     S_IRUGO | S_IWUSR, get_swpld_reg, set_swpld_reg, FAN_EEPROM_WP);

static struct attribute *swpld1_device_attrs[] = {
    &sensor_dev_attr_swpld1_reg_value.dev_attr.attr,
    &sensor_dev_attr_swpld1_reg_addr.dev_attr.attr,
    &sensor_dev_attr_swpld1_major_ver.dev_attr.attr,
    &sensor_dev_attr_swpld1_minor_ver.dev_attr.attr,
    &sensor_dev_attr_swpld1_scrtch_reg.dev_attr.attr,
    &sensor_dev_attr_psu1_pwr_ok.dev_attr.attr,
    &sensor_dev_attr_psu1_int.dev_attr.attr,
    &sensor_dev_attr_psu2_pwr_ok.dev_attr.attr,
    &sensor_dev_attr_psu2_int.dev_attr.attr,
    &sensor_dev_attr_psu1_green_led.dev_attr.attr,
    &sensor_dev_attr_psu1_red_led.dev_attr.attr,
    &sensor_dev_attr_psu2_green_led.dev_attr.attr,
    &sensor_dev_attr_psu2_red_led.dev_attr.attr,
    &sensor_dev_attr_psu_led_mode.dev_attr.attr,
    NULL,
};

static struct attribute *swpld2_device_attrs[] = {
    &sensor_dev_attr_swpld2_reg_value.dev_attr.attr,
    &sensor_dev_attr_swpld2_reg_addr.dev_attr.attr,
    &sensor_dev_attr_swpld2_major_ver.dev_attr.attr,
    &sensor_dev_attr_swpld2_minor_ver.dev_attr.attr,
    &sensor_dev_attr_swpld2_scrtch_reg.dev_attr.attr,
    &sensor_dev_attr_fan_led.dev_attr.attr,
    &sensor_dev_attr_sys_led.dev_attr.attr,
    &sensor_dev_attr_fan_mod1_led.dev_attr.attr,
    &sensor_dev_attr_fan_mod2_led.dev_attr.attr,
    &sensor_dev_attr_fan_mod3_led.dev_attr.attr,
    &sensor_dev_attr_fan_mod4_led.dev_attr.attr,
    NULL,
};

static struct attribute *swpld3_device_attrs[] = {
    &sensor_dev_attr_swpld3_reg_value.dev_attr.attr,
    &sensor_dev_attr_swpld3_reg_addr.dev_attr.attr,
    &sensor_dev_attr_swpld3_major_ver.dev_attr.attr,
    &sensor_dev_attr_swpld3_minor_ver.dev_attr.attr,
    &sensor_dev_attr_swpld3_scrtch_reg.dev_attr.attr,
    &sensor_dev_attr_sb_ver.dev_attr.attr,
    &sensor_dev_attr_platform_type.dev_attr.attr,
    NULL,
};

static struct attribute *swpld4_device_attrs[] = {
    &sensor_dev_attr_sw_board_id1.dev_attr.attr,
    &sensor_dev_attr_sw_board_id2.dev_attr.attr,
    &sensor_dev_attr_swbd_ver.dev_attr.attr,
    &sensor_dev_attr_swpld4_ver.dev_attr.attr,
    &sensor_dev_attr_psu_fan_event.dev_attr.attr,
    &sensor_dev_attr_cpu_thermal_int.dev_attr.attr,
    &sensor_dev_attr_fan_int.dev_attr.attr,
    &sensor_dev_attr_cpld_spi_wp.dev_attr.attr,
    &sensor_dev_attr_rj45_console_sel.dev_attr.attr,
    &sensor_dev_attr_system_int.dev_attr.attr,
    &sensor_dev_attr_cpld_mb_rst_done.dev_attr.attr,
    &sensor_dev_attr_mb_pwr_ok.dev_attr.attr,
    &sensor_dev_attr_fan_eeprom_wp.dev_attr.attr,
    NULL,
};

static struct attribute_group swpld1_device_attr_grp = {
    .attrs = swpld1_device_attrs,
};

static struct attribute_group swpld2_device_attr_grp = {
    .attrs = swpld2_device_attrs,
};

static struct attribute_group swpld3_device_attr_grp = {
    .attrs = swpld3_device_attrs,
};

static struct attribute_group swpld4_device_attr_grp = {
    .attrs = swpld4_device_attrs,
};

static int __init swpld1_probe(struct platform_device *pdev)
{
    int ret;
    ret = sysfs_create_group(&pdev->dev.kobj, &swpld1_device_attr_grp);
   if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        return -ENODEV;
   }
    return 0;
}

static int __init swpld2_probe(struct platform_device *pdev)
{
    int ret;
    ret = sysfs_create_group(&pdev->dev.kobj, &swpld2_device_attr_grp);
   if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        return -ENODEV;
   }
    return 0;
}

static int __init swpld3_probe(struct platform_device *pdev)
{
    int ret;
    ret = sysfs_create_group(&pdev->dev.kobj, &swpld3_device_attr_grp);
   if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        return -ENODEV;
   }
    return 0;
}

static int __init swpld4_probe(struct platform_device *pdev)
{
    int ret;
    ret = sysfs_create_group(&pdev->dev.kobj, &swpld4_device_attr_grp);
   if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        return -ENODEV;
   }
    return 0;
}

static int __exit swpld1_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &swpld1_device_attr_grp);
    return 0;
}

static int __exit swpld2_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &swpld2_device_attr_grp);
    return 0;
}

static int __exit swpld3_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &swpld3_device_attr_grp);
    return 0;
}

static int __exit swpld4_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &swpld4_device_attr_grp);
    return 0;
}

static struct platform_driver swpld1_driver = {
    .probe  = swpld1_probe,
    .remove = __exit_p(swpld1_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9064-swpld1",
    },
};

static struct platform_driver swpld2_driver = {
    .probe  = swpld2_probe,
    .remove = __exit_p(swpld2_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9064-swpld2",
    },
};

static struct platform_driver swpld3_driver = {
    .probe  = swpld3_probe,
    .remove = __exit_p(swpld3_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9064-swpld3",
    },
};

static struct platform_driver swpld4_driver = {
    .probe  = swpld4_probe,
    .remove = __exit_p(swpld4_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9064-swpld4",
    },
};
/*----------------    CPLD  - end   ------------- */

/*----------------   module initialization     ------------- */
static int __init delta_ag9064_swpld_init(void)
{
    int ret;
    printk(KERN_WARNING "ag9064_platform_swpld module initialization\n");

    // set the SWPLD prob and remove
    ret = platform_driver_register(&swpld1_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld driver\n");
        goto error_swpld1_driver;
    }

    // register the SWPLD
    ret = platform_device_register(&swpld1_device);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld device\n");
        goto error_swpld1_device;
    }

    // set the SWPLD prob and remove
    ret = platform_driver_register(&swpld2_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld driver\n");
        goto error_swpld2_driver;
    }

    // register the SWPLD
    ret = platform_device_register(&swpld2_device);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld device\n");
        goto error_swpld2_device;
    }

    // set the SWPLD prob and remove
    ret = platform_driver_register(&swpld3_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld driver\n");
        goto error_swpld3_driver;
    }

    // register the SWPLD
    ret = platform_device_register(&swpld3_device);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld device\n");
        goto error_swpld3_device;
    }

    // set the SWPLD prob and remove
    ret = platform_driver_register(&swpld4_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld driver\n");
        goto error_swpld4_driver;
    }

    // register the SWPLD
    ret = platform_device_register(&swpld4_device);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld device\n");
        goto error_swpld4_device;
    }
    return 0;

error_swpld4_device:
    platform_driver_unregister(&swpld4_driver);
error_swpld4_driver:
    platform_device_unregister(&swpld3_device);
error_swpld3_device:
    platform_driver_unregister(&swpld3_driver);
error_swpld3_driver:
    platform_device_unregister(&swpld2_device);
error_swpld2_device:
    platform_driver_unregister(&swpld2_driver);
error_swpld2_driver:
    platform_device_unregister(&swpld1_device);
error_swpld1_device:
    platform_driver_unregister(&swpld1_driver);
error_swpld1_driver:
    return ret;
}

static void __exit delta_ag9064_swpld_exit(void)
{
    platform_device_unregister(&swpld1_device);
    platform_driver_unregister(&swpld1_driver);  
    platform_device_unregister(&swpld2_device);
    platform_driver_unregister(&swpld2_driver);
    platform_device_unregister(&swpld3_device);
    platform_driver_unregister(&swpld3_driver);
    platform_device_unregister(&swpld4_device);
    platform_driver_unregister(&swpld4_driver);
}
module_init(delta_ag9064_swpld_init);
module_exit(delta_ag9064_swpld_exit);

MODULE_DESCRIPTION("DNI ag9064 CPLD Platform Support");
MODULE_AUTHOR("Stanley Chi <stanley.chi@deltaww.com>");
MODULE_LICENSE("GPL"); 
