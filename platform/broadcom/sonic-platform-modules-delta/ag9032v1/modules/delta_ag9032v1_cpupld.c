#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define CPUPLD_REG 0x31

enum cpld_type {
    cpld,       
};

struct platform_data {
    int reg_addr;
    struct i2c_client *client;
};

enum{
    BUS0 = 0,
    BUS1,
    BUS2,
    BUS3,
    BUS4,
    BUS5,
    BUS6,
    BUS7,
};

enum cpld_attributes {
    CPLD_VER,
    CPU_BOARD_VER,
    CPU_ID,
    CPLD_RST,
    MB_RST,
    I2C_SW_RST,
    MB_PWR,
    PSU_FAN_INT,
    SPI_WP_GBE,
    EEPROM_WP,
};

static void device_release(struct device *dev)
{
    return;
}

/*----------------    CPUPLD   - start   ------------- */
static struct platform_data ag9032v1_cpld_platform_data[] = {
    [cpld] = {
        .reg_addr = CPUPLD_REG,
    },
};

static struct platform_device ag9032v1_cpld = {
    .name               = "delta-ag9032v1-cpupld",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9032v1_cpld_platform_data,
                .release         = device_release
    },
};
static unsigned char cpld_reg_addr;
static ssize_t get_cpld_reg_value(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    struct platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[cpld].client, cpld_reg_addr);

    return sprintf(buf, "0x%02x\n", ret);
}

static ssize_t set_cpld_reg_value(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t count)
{
    unsigned long data;
    int err;
    struct platform_data *pdata = dev->platform_data;
    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }

    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }

    i2c_smbus_write_byte_data(pdata[cpld].client, cpld_reg_addr, data);

    return count;
}

static ssize_t get_cpld_reg_addr(struct device *dev, struct device_attribute *devattr, char *buf) 
{

    return sprintf(buf, "0x%02x\n", cpld_reg_addr);
}

static ssize_t set_cpld_reg_addr(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t count)
{
    unsigned long data;
    int err;

    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }
    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }
    cpld_reg_addr = data;

    return count;
}

static ssize_t get_cpld_data(struct device *dev, struct device_attribute *dev_attr, char *buf) 
{
    int ret;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct platform_data *pdata = dev->platform_data;
    unsigned char reg;
    int mask;
    int value;
    char note[180];
    switch (attr->index) {
    case CPLD_VER:
        reg  = 0x01;
        value = i2c_smbus_read_byte_data(pdata[cpld].client, reg);
        sprintf(note, "\nCPLD Version, controlled by CPLD editor.\n");
        return sprintf(buf, "0x%02x%s", value, note);
        break;
    case CPU_BOARD_VER:
        reg  = 0x02;
        ret = i2c_smbus_read_byte_data(pdata[cpld].client, reg);
        value = ret >> 4;
        sprintf(note, "\n“0x00”: proto A1\n“0x01”: proto A2\n“0x02”: proto B\n");
        return sprintf(buf, "0x%02x%s", value, note);
        break;
    case CPU_ID:
        reg  = 0x02;
        ret = i2c_smbus_read_byte_data(pdata[cpld].client, reg);
        value = ret & 0x0F;
        sprintf(note, "\n“0x00”: P2041 ECC\n“0x01”: Rangeley ECC\n“0x02”: T2080 ECC\n");
        return sprintf(buf, "0x%02x%s", value, note);
        break;
    case CPLD_RST:
        reg  = 0x05;
        mask = 7;
        sprintf(note, "\n“1” = Normal operation\n“0” = Reset\n");
        break;
    case MB_RST:
        reg  = 0x05;
        mask = 1;
        sprintf(note, "\n“1” = Normal operation\n“0” = Reset\n");
        break;
    case I2C_SW_RST:
        reg  = 0x05;
        mask = 0;
        sprintf(note, "\n“1” = Normal operation\n“0” = Reset\n");
        break;
    case MB_PWR:
        reg  = 0x08;
        mask = 4;
        sprintf(note, "\n“1” = Power rail is good\n“0” = Power rail is failed\n");        
        break;
    case PSU_FAN_INT:
        reg  = 0x0A;
        mask = 0;
        sprintf(note, "\n“1” = Interrupt doesn’t occur\n“0” = Interrupt occurs\n");
        break;
    case SPI_WP_GBE:
        reg  = 0x10;
        mask = 3;
        sprintf(note, "\n“1” = overrides the lock-down function enabling blocks to be erased or programmed using software commands.\n“0” = enables the lock-down mechanism.\n");
        break;
    case EEPROM_WP:
        reg  = 0x10;
        mask = 2;
        sprintf(note, "\n“1” = overrides the lock-down function enabling blocks to be erased or programmed using software commands.\n“0” = enables the lock-down mechanism.\n");
        break;
    default:
        return sprintf(buf, "%d not found", attr->index);
    }
    ret = i2c_smbus_read_byte_data(pdata[cpld].client, reg);
    value = (ret & (1 << mask)) >> mask;
    return sprintf(buf, "%d%s", value, note);
}

static ssize_t set_cpld_data(struct device *dev, struct device_attribute *dev_attr,
             const char *buf, size_t count)
{
    int mask;    
    int err;
    int ret;    
    unsigned long data;
    unsigned char reg;
    unsigned char mask_shift;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct platform_data *pdata = dev->platform_data;
    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }

    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }
    switch (attr->index) {
    case CPLD_RST:
        reg  = 0x05;
        mask = 7;
        break;
    case MB_RST:
        reg  = 0x05;
        mask = 1;
        break;
    case I2C_SW_RST:
        reg  = 0x05;
        mask = 0;
        break;
    case SPI_WP_GBE:
        reg  = 0x10;
        mask = 3;
        break;
    case EEPROM_WP:
        reg  = 0x10;
        mask = 2;
        break;
    default:
        return count;
    }

    ret = i2c_smbus_read_byte_data(pdata[cpld].client, reg);
    mask_shift = 1 << mask;
    data = (ret & ~mask_shift) | (data << mask);
    i2c_smbus_write_byte_data(pdata[cpld].client, cpld_reg_addr, data);

    return count;
}

static DEVICE_ATTR(cpld_reg_value, S_IRUGO | S_IWUSR, get_cpld_reg_value, set_cpld_reg_value);
static DEVICE_ATTR(cpld_reg_addr,  S_IRUGO | S_IWUSR, get_cpld_reg_addr,  set_cpld_reg_addr);

static SENSOR_DEVICE_ATTR(cpld_ver,      S_IRUGO,           get_cpld_data, NULL,          CPLD_VER);
static SENSOR_DEVICE_ATTR(cpu_board_ver, S_IRUGO,           get_cpld_data, NULL,          CPU_BOARD_VER);
static SENSOR_DEVICE_ATTR(cpu_id,        S_IRUGO,           get_cpld_data, NULL,          CPU_ID);
static SENSOR_DEVICE_ATTR(cpld_rst,      S_IRUGO | S_IWUSR, get_cpld_data, set_cpld_data, CPLD_RST);
static SENSOR_DEVICE_ATTR(mb_rst,        S_IRUGO | S_IWUSR, get_cpld_data, set_cpld_data, MB_RST);
static SENSOR_DEVICE_ATTR(i2c_sw_rst,    S_IRUGO | S_IWUSR, get_cpld_data, set_cpld_data, I2C_SW_RST);
static SENSOR_DEVICE_ATTR(mb_pwr,        S_IRUGO,           get_cpld_data, NULL,          MB_PWR);
static SENSOR_DEVICE_ATTR(psu_fan_int,   S_IRUGO,           get_cpld_data, NULL,          PSU_FAN_INT);
static SENSOR_DEVICE_ATTR(spi_wp_gbe,    S_IRUGO | S_IWUSR, get_cpld_data, set_cpld_data, SPI_WP_GBE);
static SENSOR_DEVICE_ATTR(eeprom_wp,     S_IRUGO | S_IWUSR, get_cpld_data, set_cpld_data, EEPROM_WP);

static struct attribute *ag9032v1_cpld_attrs[] = {
    &dev_attr_cpld_reg_value.attr,
    &dev_attr_cpld_reg_addr.attr,
    &sensor_dev_attr_cpld_ver.dev_attr.attr,
    &sensor_dev_attr_cpu_board_ver.dev_attr.attr,
    &sensor_dev_attr_cpu_id.dev_attr.attr,
    &sensor_dev_attr_cpld_rst.dev_attr.attr,
    &sensor_dev_attr_mb_rst.dev_attr.attr,
    &sensor_dev_attr_i2c_sw_rst.dev_attr.attr,
    &sensor_dev_attr_mb_pwr.dev_attr.attr,
    &sensor_dev_attr_psu_fan_int.dev_attr.attr,
    &sensor_dev_attr_spi_wp_gbe.dev_attr.attr,
    &sensor_dev_attr_eeprom_wp.dev_attr.attr,
    NULL,
};

static struct attribute_group ag9032v1_cpld_attr_group = {
    .attrs = ag9032v1_cpld_attrs,
};

static int __init cpld_probe(struct platform_device *pdev)
{
    struct platform_data *pdata;
    struct i2c_adapter *parent;
    int ret;
    int retval;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "CPUPLD platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(BUS2);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n",BUS2);
        return -ENODEV;
    }

    pdata[cpld].client = i2c_new_dummy(parent, pdata[cpld].reg_addr);
    if (!pdata[cpld].client) {
        printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[cpld].reg_addr);
        goto error;
    }

    retval = sysfs_create_group(&pdev->dev.kobj, &ag9032v1_cpld_attr_group);
    if (retval){
         printk(KERN_WARNING "Fail to create cpupld attribute group");
        goto error;
    }
    return 0;

error:
    i2c_unregister_device(pdata[cpld].client);
    i2c_put_adapter(parent);
    return -ENODEV; 
}

static int __exit cpld_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct platform_data *pdata = pdev->dev.platform_data;
    sysfs_remove_group(&pdev->dev.kobj, &ag9032v1_cpld_attr_group);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    } 
    else {
        if (pdata[cpld].client) {
            if (!parent) {
                parent = (pdata[cpld].client)->adapter;
            }
            i2c_unregister_device(pdata[cpld].client);
        }
    }
    i2c_put_adapter(parent);
    return 0;
}

static struct platform_driver cpld_driver = {
    .probe  = cpld_probe,
    .remove = __exit_p(cpld_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9032v1-cpupld",
    },
};
/*----------------    CPUPLD  - end   ------------- */


/*----------------   module initialization     ------------- */
static int __init delta_ag9032v1_cpupld_init(void)
{
    int ret;
    printk(KERN_WARNING "ag9032v1_platform_cpupld module initialization\n");

    // set the CPUPLD prob and remove
    ret = platform_driver_register(&cpld_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register cpupld driver\n");
        goto error_cpupld_driver;
    }

    // register the CPUPLD
    ret = platform_device_register(&ag9032v1_cpld);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpupld device\n");
        goto error_ag9032v1_cpupld;
    }
    return 0;

error_ag9032v1_cpupld:
    platform_driver_unregister(&cpld_driver);
error_cpupld_driver:
    return ret;
}

static void __exit delta_ag9032v1_cpupld_exit(void)
{
    platform_device_unregister(&ag9032v1_cpld);
    platform_driver_unregister(&cpld_driver);  
}
module_init(delta_ag9032v1_cpupld_init);
module_exit(delta_ag9032v1_cpupld_exit);

MODULE_DESCRIPTION("DNI ag9032v1 CPLD Platform Support");
MODULE_AUTHOR("Stanley Chi <stanley.chi@deltaww.com>");
MODULE_LICENSE("GPL");
