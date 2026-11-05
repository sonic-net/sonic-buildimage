
#include<linux/module.h> // included for all kernel modules
#include <linux/kernel.h> // included for KERN_INFO
#include <linux/init.h> // included for __init and __exit macros
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>


//iom cpld slave address
#define  IOM_CPLD_SLAVE_ADD   0x3e

#define  CPLD_SEP_RST0 0x5

//iom cpld ver register
#define  IOM_CPLD_SLAVE_VER   0x00

//qsfp reset cntrl reg on each iom
#define  QSFP_RST_CRTL_REG0  0x10 
#define  QSFP_RST_CRTL_REG1  0x11 

//qsfp lp mode reg on each iom
#define  QSFP_LPMODE_REG0 0x12 
#define  QSFP_LPMODE_REG1 0x13

//qsfp mod presence reg on each iom
#define  QSFP_MOD_PRS_REG0 0x16 
#define  QSFP_MOD_PRS_REG1 0x17 

//qsfp interrupt registers
#define  QSFP_INT_STA_REG0 0x14
#define  QSFP_INT_STA_REG1 0x15
#define  QSFP_ABS_STA_REG0 0x16
#define  QSFP_ABS_STA_REG1 0x17
#define  QSFP_TRIG_MOD     0x20
#define  QSFP_INT_COMBINE  0x21
#define  QSFP_INT0         0x22
#define  QSFP_INT1         0x23
#define  QSFP_ABS_INT0     0x24
#define  QSFP_ABS_INT1     0x25
#define  QSFP_INT_MASK0    0x26
#define  QSFP_INT_MASK1    0x27
#define  QSFP_ABS_MASK0    0x28
#define  QSFP_ABS_MASK1    0x29

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
};

static void dell_s6100_iom_cpld_add_client(struct i2c_client *client)
{
    struct cpld_data *data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
}

static void dell_s6100_iom_cpld_remove_client(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    kfree(data);
}

int dell_s6100_iom_cpld_read(struct cpld_data *data,unsigned short cpld_addr, u8 reg)
{
    int ret = -EPERM;
    u8 high_reg =0x00;

    mutex_lock(&data->update_lock);
    ret = i2c_smbus_write_byte_data(data->client, high_reg,reg);
    ret = i2c_smbus_read_byte(data->client);
    mutex_unlock(&data->update_lock);

    return ret;
}

int dell_s6100_iom_cpld_write(struct cpld_data *data,unsigned short cpld_addr, u8 reg, u8 value)
{
    int ret = -EIO;
    u16 devdata=0;
    u8 high_reg =0x00;

    mutex_lock(&data->update_lock);
    devdata = (value << 8) | reg;
    i2c_smbus_write_word_data(data->client,high_reg,devdata);
    mutex_unlock(&data->update_lock);

    return ret;
}
static ssize_t get_cpldver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u8 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,IOM_CPLD_SLAVE_VER);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u8)ret & 0xff;
    return sprintf(buf,"IOM CPLD Version:0x%02x\n",devdata);
}

static ssize_t get_modprs(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_MOD_PRS_REG0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_MOD_PRS_REG1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t get_lpmode(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_LPMODE_REG0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_LPMODE_REG1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t get_reset(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_RST_CRTL_REG0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_RST_CRTL_REG1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t set_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned long devdata;
    int err;
    struct cpld_data *data = dev_get_drvdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,QSFP_LPMODE_REG0,(u8)(devdata & 0xff));
    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,QSFP_LPMODE_REG1,(u8)((devdata >> 8) & 0xff));

    return count;
}

static ssize_t set_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned long devdata;
    int err;
    struct cpld_data *data = dev_get_drvdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,QSFP_RST_CRTL_REG0,(u8)(devdata & 0xff));
    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,QSFP_RST_CRTL_REG1,(u8)((devdata >> 8) & 0xff));

    return count;
}

static ssize_t get_int_sta(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_INT_STA_REG0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_INT_STA_REG1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t get_abs_sta(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_ABS_STA_REG0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_ABS_STA_REG1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t get_trig_mod(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u8 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_TRIG_MOD);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u8)ret & 0xff;
    return sprintf(buf,"0x%02x\n",devdata);
}

static ssize_t set_trig_mod(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct cpld_data *data = dev_get_drvdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,QSFP_TRIG_MOD,(u8)(devdata & 0xff));

    return count;
}

static ssize_t get_int_combine(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u8 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_INT_COMBINE);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u8)ret & 0xff;
    return sprintf(buf,"0x%02x\n",devdata);
}

static ssize_t get_int(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_INT0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_INT1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t get_abs_int(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_ABS_INT0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_ABS_INT1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t get_int_mask(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_INT_MASK0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_INT_MASK1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t set_int_mask(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct cpld_data *data = dev_get_drvdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,QSFP_INT_MASK0,(u8)(devdata & 0xff));
    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,QSFP_INT_MASK1,(u8)((devdata >> 8) & 0xff));

    return count;
}

static ssize_t get_abs_mask(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_ABS_MASK0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,QSFP_ABS_MASK1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t set_abs_mask(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct cpld_data *data = dev_get_drvdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,QSFP_ABS_MASK0,(u8)(devdata & 0xff));
    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,QSFP_ABS_MASK1,(u8)((devdata >> 8) & 0xff));

    return count;
}

static ssize_t get_sep_reset(struct device *dev, struct device_attribute *devattr, char *buf)
{
    int ret;
    u8 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = dell_s6100_iom_cpld_read(data,IOM_CPLD_SLAVE_ADD,CPLD_SEP_RST0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u8)ret & 0xff;
    return sprintf(buf,"0x%02x\n",devdata);
}

static ssize_t set_sep_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct cpld_data *data = dev_get_drvdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    dell_s6100_iom_cpld_write(data,IOM_CPLD_SLAVE_ADD,CPLD_SEP_RST0,(u8)(devdata & 0xff));

    return count;
}

static DEVICE_ATTR(iom_cpld_vers,S_IRUGO,get_cpldver, NULL);
static DEVICE_ATTR(qsfp_modprs, S_IRUGO,get_modprs, NULL);
static DEVICE_ATTR(qsfp_lpmode, S_IRUGO | S_IWUSR,get_lpmode,set_lpmode);
static DEVICE_ATTR(qsfp_reset,  S_IRUGO | S_IWUSR,get_reset, set_reset);
static DEVICE_ATTR(qsfp_int_sta, S_IRUGO, get_int_sta, NULL);
static DEVICE_ATTR(qsfp_abs_sta, S_IRUGO, get_abs_sta, NULL);
static DEVICE_ATTR(qsfp_trig_mod, S_IRUGO | S_IWUSR, get_trig_mod, set_trig_mod);
static DEVICE_ATTR(qsfp_int_combine, S_IRUGO, get_int_combine, NULL);
static DEVICE_ATTR(qsfp_int, S_IRUGO, get_int, NULL);
static DEVICE_ATTR(qsfp_abs_int, S_IRUGO, get_abs_int, NULL);
static DEVICE_ATTR(qsfp_int_mask, S_IRUGO | S_IWUSR, get_int_mask, set_int_mask);
static DEVICE_ATTR(qsfp_abs_mask, S_IRUGO | S_IWUSR, get_abs_mask, set_abs_mask);
static DEVICE_ATTR(sep_reset, S_IRUGO | S_IWUSR, get_sep_reset, set_sep_reset);

static struct attribute *i2c_cpld_attrs[] = {
    &dev_attr_qsfp_lpmode.attr,
    &dev_attr_qsfp_reset.attr,
    &dev_attr_qsfp_modprs.attr,
    &dev_attr_iom_cpld_vers.attr,
    &dev_attr_qsfp_int_sta.attr,
    &dev_attr_qsfp_abs_sta.attr,
    &dev_attr_qsfp_trig_mod.attr,
    &dev_attr_qsfp_int_combine.attr,
    &dev_attr_qsfp_int.attr,
    &dev_attr_qsfp_abs_int.attr,
    &dev_attr_qsfp_int_mask.attr,
    &dev_attr_qsfp_abs_mask.attr,
    &dev_attr_sep_reset.attr,
    NULL,
};

static struct attribute_group i2c_cpld_attr_grp = {
    .attrs = i2c_cpld_attrs,
};

static int dell_s6100_iom_cpld_probe(struct i2c_client *client)
{
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    dell_s6100_iom_cpld_add_client(client);

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &i2c_cpld_attr_grp);
    if (status) {
        printk(KERN_INFO "Cannot create sysfs\n");
    }
    return 0;

exit:
    return status;
}

static void dell_s6100_iom_cpld_remove(struct i2c_client *client)
{
    dell_s6100_iom_cpld_remove_client(client);
}


static const struct i2c_device_id dell_s6100_iom_cpld_id[] = {
    { "dell_s6100_iom_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, dell_s6100_iom_cpld_id);

static struct i2c_driver dell_s6100_iom_cpld_driver = {
    .driver = {
        .name     = "dell_s6100_iom_cpld",
    },
    .probe        = dell_s6100_iom_cpld_probe,
    .remove       = dell_s6100_iom_cpld_remove,
    .id_table     = dell_s6100_iom_cpld_id,
};



static int __init dell_s6100_iom_cpld_init(void)
{
    return i2c_add_driver(&dell_s6100_iom_cpld_driver);
}

static void __exit dell_s6100_iom_cpld_exit(void)
{
    i2c_del_driver(&dell_s6100_iom_cpld_driver);
}

MODULE_AUTHOR("Srideep Devireddy  <srideep_devireddy@dell.com>");
MODULE_DESCRIPTION("dell_s6100_iom_cpld driver");
MODULE_LICENSE("GPL");

module_init(dell_s6100_iom_cpld_init);
module_exit(dell_s6100_iom_cpld_exit);
