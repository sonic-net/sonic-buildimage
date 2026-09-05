#include <linux/module.h>
#include <linux/i2c.h>

#include "slave_cpld_reg.h"
#include "slave_cpld_sysfs.h"

static int debug_flag = 0;

struct slave_cpld_data {
    struct mutex lock;

    struct i2c_client *client;
    struct device_attribute bin;
};


static const struct i2c_device_id slave_cpld_ids[] = {
    { "slave_cpld", 0 },
    { /* END OF LIST */ }
};
MODULE_DEVICE_TABLE(i2c, slave_cpld_ids);

static int slave_cpld_raw_read(struct device *dev, struct device_attribute *attr, char *buf,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name){
    unsigned int reg_val = 0, fld_val;
    static int debug_flag;
    struct slave_cpld_data *data = dev_get_drvdata(dev);
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

static int slave_cpld_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name){
    int ret_code;
    unsigned int reg_val, fld_val;
    unsigned long val;
    static int debug_flag;
    struct slave_cpld_data *data = dev_get_drvdata(dev);
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
static ssize_t slave_cpld_debug_read(struct device *dev, struct device_attribute *attr,
             char *buf){

    return sprintf(buf, "%d\n", debug_flag);
}


static ssize_t slave_cpld_debug_write(struct device *dev, struct device_attribute *attr,
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
SYSFS_MISC_RW_ATTR_DEF(debug, slave_cpld_debug_read, slave_cpld_debug_write)




/* ----------------define port group---------------------------- */
static struct attribute *port17_attributes[] = {
    SYSFS_ATTR_PTR(port17_present),       /* register: zqsfp28_present_24_17_status */
    SYSFS_ATTR_PTR(port17_rst),           /* register: zqsfp28_rst_24_17 */
    SYSFS_ATTR_PTR(port17_modsel),        /* register: zqsfp28_modsel_24_17 */
    SYSFS_ATTR_PTR(port17_lpmode),        /* register: zqsfp28_lpmode_24_17 */
    SYSFS_ATTR_PTR(port17_irq_status),    /* register: zqsfp28_irq_24_17_status */
    SYSFS_ATTR_PTR(port17_irq_msk),       /* register: zqsfp28_irq_msk_24_17_status */
    NULL
};

static struct attribute *port18_attributes[] = {
    SYSFS_ATTR_PTR(port18_present),       /* register: zqsfp28_present_24_17_status */
    SYSFS_ATTR_PTR(port18_rst),           /* register: zqsfp28_rst_24_17 */
    SYSFS_ATTR_PTR(port18_modsel),        /* register: zqsfp28_modsel_24_17 */
    SYSFS_ATTR_PTR(port18_lpmode),        /* register: zqsfp28_lpmode_24_17 */
    SYSFS_ATTR_PTR(port18_irq_status),    /* register: zqsfp28_irq_24_17_status */
    SYSFS_ATTR_PTR(port18_irq_msk),       /* register: zqsfp28_irq_msk_24_17_status */
    NULL
};

static struct attribute *port19_attributes[] = {
    SYSFS_ATTR_PTR(port19_present),       /* register: zqsfp28_present_24_17_status */
    SYSFS_ATTR_PTR(port19_rst),           /* register: zqsfp28_rst_24_17 */
    SYSFS_ATTR_PTR(port19_modsel),        /* register: zqsfp28_modsel_24_17 */
    SYSFS_ATTR_PTR(port19_lpmode),        /* register: zqsfp28_lpmode_24_17 */
    SYSFS_ATTR_PTR(port19_irq_status),    /* register: zqsfp28_irq_24_17_status */
    SYSFS_ATTR_PTR(port19_irq_msk),       /* register: zqsfp28_irq_msk_24_17_status */
    NULL
};

static struct attribute *port20_attributes[] = {
    SYSFS_ATTR_PTR(port20_present),       /* register: zqsfp28_present_24_17_status */
    SYSFS_ATTR_PTR(port20_rst),           /* register: zqsfp28_rst_24_17 */
    SYSFS_ATTR_PTR(port20_modsel),        /* register: zqsfp28_modsel_24_17 */
    SYSFS_ATTR_PTR(port20_lpmode),        /* register: zqsfp28_lpmode_24_17 */
    SYSFS_ATTR_PTR(port20_irq_status),    /* register: zqsfp28_irq_24_17_status */
    SYSFS_ATTR_PTR(port20_irq_msk),       /* register: zqsfp28_irq_msk_24_17_status */
    NULL
};

static struct attribute *port21_attributes[] = {
    SYSFS_ATTR_PTR(port21_present),       /* register: zqsfp28_present_24_17_status */
    SYSFS_ATTR_PTR(port21_rst),           /* register: zqsfp28_rst_24_17 */
    SYSFS_ATTR_PTR(port21_modsel),        /* register: zqsfp28_modsel_24_17 */
    SYSFS_ATTR_PTR(port21_lpmode),        /* register: zqsfp28_lpmode_24_17 */
    SYSFS_ATTR_PTR(port21_irq_status),    /* register: zqsfp28_irq_24_17_status */
    SYSFS_ATTR_PTR(port21_irq_msk),       /* register: zqsfp28_irq_msk_24_17_status */
    NULL
};

static struct attribute *port22_attributes[] = {
    SYSFS_ATTR_PTR(port22_present),       /* register: zqsfp28_present_24_17_status */
    SYSFS_ATTR_PTR(port22_rst),           /* register: zqsfp28_rst_24_17 */
    SYSFS_ATTR_PTR(port22_modsel),        /* register: zqsfp28_modsel_24_17 */
    SYSFS_ATTR_PTR(port22_lpmode),        /* register: zqsfp28_lpmode_24_17 */
    SYSFS_ATTR_PTR(port22_irq_status),    /* register: zqsfp28_irq_24_17_status */
    SYSFS_ATTR_PTR(port22_irq_msk),       /* register: zqsfp28_irq_msk_24_17_status */
    NULL
};

static struct attribute *port23_attributes[] = {
    SYSFS_ATTR_PTR(port23_present),       /* register: zqsfp28_present_24_17_status */
    SYSFS_ATTR_PTR(port23_rst),           /* register: zqsfp28_rst_24_17 */
    SYSFS_ATTR_PTR(port23_modsel),        /* register: zqsfp28_modsel_24_17 */
    SYSFS_ATTR_PTR(port23_lpmode),        /* register: zqsfp28_lpmode_24_17 */
    SYSFS_ATTR_PTR(port23_irq_status),    /* register: zqsfp28_irq_24_17_status */
    SYSFS_ATTR_PTR(port23_irq_msk),       /* register: zqsfp28_irq_msk_24_17_status */
    NULL
};

static struct attribute *port24_attributes[] = {
    SYSFS_ATTR_PTR(port24_present),       /* register: zqsfp28_present_24_17_status */
    SYSFS_ATTR_PTR(port24_rst),           /* register: zqsfp28_rst_24_17 */
    SYSFS_ATTR_PTR(port24_modsel),        /* register: zqsfp28_modsel_24_17 */
    SYSFS_ATTR_PTR(port24_lpmode),        /* register: zqsfp28_lpmode_24_17 */
    SYSFS_ATTR_PTR(port24_irq_status),    /* register: zqsfp28_irq_24_17_status */
    SYSFS_ATTR_PTR(port24_irq_msk),       /* register: zqsfp28_irq_msk_24_17_status */
    NULL
};

static struct attribute *port25_attributes[] = {
    SYSFS_ATTR_PTR(port25_present),       /* register: zqsfp28_present_32_25_status */
    SYSFS_ATTR_PTR(port25_rst),           /* register: zqsfp28_rst_32_25 */
    SYSFS_ATTR_PTR(port25_modsel),        /* register: zqsfp28_modsel_32_25 */
    SYSFS_ATTR_PTR(port25_lpmode),        /* register: zqsfp28_lpmode_32_25 */
    SYSFS_ATTR_PTR(port25_irq_status),    /* register: zqsfp28_irq_32_25_status */
    SYSFS_ATTR_PTR(port25_irq_msk),       /* register: zqsfp28_irq_msk_32_25_status */
    NULL
};

static struct attribute *port26_attributes[] = {
    SYSFS_ATTR_PTR(port26_present),       /* register: zqsfp28_present_32_25_status */
    SYSFS_ATTR_PTR(port26_rst),           /* register: zqsfp28_rst_32_25 */
    SYSFS_ATTR_PTR(port26_modsel),        /* register: zqsfp28_modsel_32_25 */
    SYSFS_ATTR_PTR(port26_lpmode),        /* register: zqsfp28_lpmode_32_25 */
    SYSFS_ATTR_PTR(port26_irq_status),    /* register: zqsfp28_irq_32_25_status */
    SYSFS_ATTR_PTR(port26_irq_msk),       /* register: zqsfp28_irq_msk_32_25_status */
    NULL
};

static struct attribute *port27_attributes[] = {
    SYSFS_ATTR_PTR(port27_present),       /* register: zqsfp28_present_32_25_status */
    SYSFS_ATTR_PTR(port27_rst),           /* register: zqsfp28_rst_32_25 */
    SYSFS_ATTR_PTR(port27_modsel),        /* register: zqsfp28_modsel_32_25 */
    SYSFS_ATTR_PTR(port27_lpmode),        /* register: zqsfp28_lpmode_32_25 */
    SYSFS_ATTR_PTR(port27_irq_status),    /* register: zqsfp28_irq_32_25_status */
    SYSFS_ATTR_PTR(port27_irq_msk),       /* register: zqsfp28_irq_msk_32_25_status */
    NULL
};

static struct attribute *port28_attributes[] = {
    SYSFS_ATTR_PTR(port28_present),       /* register: zqsfp28_present_32_25_status */
    SYSFS_ATTR_PTR(port28_rst),           /* register: zqsfp28_rst_32_25 */
    SYSFS_ATTR_PTR(port28_modsel),        /* register: zqsfp28_modsel_32_25 */
    SYSFS_ATTR_PTR(port28_lpmode),        /* register: zqsfp28_lpmode_32_25 */
    SYSFS_ATTR_PTR(port28_irq_status),    /* register: zqsfp28_irq_32_25_status */
    SYSFS_ATTR_PTR(port28_irq_msk),       /* register: zqsfp28_irq_msk_32_25_status */
    NULL
};

static struct attribute *port29_attributes[] = {
    SYSFS_ATTR_PTR(port29_present),       /* register: zqsfp28_present_32_25_status */
    SYSFS_ATTR_PTR(port29_rst),           /* register: zqsfp28_rst_32_25 */
    SYSFS_ATTR_PTR(port29_modsel),        /* register: zqsfp28_modsel_32_25 */
    SYSFS_ATTR_PTR(port29_lpmode),        /* register: zqsfp28_lpmode_32_25 */
    SYSFS_ATTR_PTR(port29_irq_status),    /* register: zqsfp28_irq_32_25_status */
    SYSFS_ATTR_PTR(port29_irq_msk),       /* register: zqsfp28_irq_msk_32_25_status */
    NULL
};

static struct attribute *port30_attributes[] = {
    SYSFS_ATTR_PTR(port30_present),       /* register: zqsfp28_present_32_25_status */
    SYSFS_ATTR_PTR(port30_rst),           /* register: zqsfp28_rst_32_25 */
    SYSFS_ATTR_PTR(port30_modsel),        /* register: zqsfp28_modsel_32_25 */
    SYSFS_ATTR_PTR(port30_lpmode),        /* register: zqsfp28_lpmode_32_25 */
    SYSFS_ATTR_PTR(port30_irq_status),    /* register: zqsfp28_irq_32_25_status */
    SYSFS_ATTR_PTR(port30_irq_msk),       /* register: zqsfp28_irq_msk_32_25_status */
    NULL
};

static struct attribute *port31_attributes[] = {
    SYSFS_ATTR_PTR(port31_present),       /* register: zqsfp28_present_32_25_status */
    SYSFS_ATTR_PTR(port31_rst),           /* register: zqsfp28_rst_32_25 */
    SYSFS_ATTR_PTR(port31_modsel),        /* register: zqsfp28_modsel_32_25 */
    SYSFS_ATTR_PTR(port31_lpmode),        /* register: zqsfp28_lpmode_32_25 */
    SYSFS_ATTR_PTR(port31_irq_status),    /* register: zqsfp28_irq_32_25_status */
    SYSFS_ATTR_PTR(port31_irq_msk),       /* register: zqsfp28_irq_msk_32_25_status */
    NULL
};

static struct attribute *port32_attributes[] = {
    SYSFS_ATTR_PTR(port32_present),       /* register: zqsfp28_present_32_25_status */
    SYSFS_ATTR_PTR(port32_rst),           /* register: zqsfp28_rst_32_25 */
    SYSFS_ATTR_PTR(port32_modsel),        /* register: zqsfp28_modsel_32_25 */
    SYSFS_ATTR_PTR(port32_lpmode),        /* register: zqsfp28_lpmode_32_25 */
    SYSFS_ATTR_PTR(port32_irq_status),    /* register: zqsfp28_irq_32_25_status */
    SYSFS_ATTR_PTR(port32_irq_msk),       /* register: zqsfp28_irq_msk_32_25_status */
    NULL
};

static const struct attribute_group slave_cpld_port_group[] = {
    {.attrs = port17_attributes,
    .name = "port17",},
    {.attrs = port18_attributes,
    .name = "port18",},
    {.attrs = port19_attributes,
    .name = "port19",},
    {.attrs = port20_attributes,
    .name = "port20",},
    {.attrs = port21_attributes,
    .name = "port21",},
    {.attrs = port22_attributes,
    .name = "port22",},
    {.attrs = port23_attributes,
    .name = "port23",},
    {.attrs = port24_attributes,
    .name = "port24",},
    {.attrs = port25_attributes,
    .name = "port25",},
    {.attrs = port26_attributes,
    .name = "port26",},
    {.attrs = port27_attributes,
    .name = "port27",},
    {.attrs = port28_attributes,
    .name = "port28",},
    {.attrs = port29_attributes,
    .name = "port29",},
    {.attrs = port30_attributes,
    .name = "port30",},
    {.attrs = port31_attributes,
    .name = "port31",},
    {.attrs = port32_attributes,
    .name = "port32",}
};

/* ----------------define misc group---------------------------- */
static struct attribute *misc_attributes[] = {
    SYSFS_ATTR_PTR(mjr_rev),              /* register: slv_cpld_rev */
    SYSFS_ATTR_PTR(mnr_rev),              /* register: slv_cpld_rev */
    SYSFS_ATTR_PTR(scrtch_reg),           /* register: slv_cpld_gpr */
    SYSFS_ATTR_PTR(brd_rev),              /* register: mb_brd_rev_type */
    SYSFS_ATTR_PTR(brd_type),             /* register: mb_brd_rev_type */

    SYSFS_ATTR_PTR(port_17_24_present),   /* register: zqsfp28_present_24_17_status */
    SYSFS_ATTR_PTR(port_25_32_present),   /* register: zqsfp28_present_32_25_status */
    SYSFS_ATTR_PTR(port_17_24_rst),       /* register: zqsfp28_rst_24_17 */
    SYSFS_ATTR_PTR(port_25_32_rst),       /* register: zqsfp28_rst_32_25 */
    SYSFS_ATTR_PTR(port_17_24_modsel),    /* register: zqsfp28_modsel_24_17 */
    SYSFS_ATTR_PTR(port_25_32_modsel),    /* register: zqsfp28_modsel_32_25 */
    SYSFS_ATTR_PTR(port_17_24_irq_status),/* register: zqsfp28_irq_24_17_status */
    SYSFS_ATTR_PTR(port_25_32_irq_status),/* register: zqsfp28_irq_32_25_status */
    SYSFS_ATTR_PTR(port_17_24_irq_msk),   /* register: zqsfp28_irq_msk_24_17_status */
    SYSFS_ATTR_PTR(port_25_32_irq_msk),   /* register: zqsfp28_irq_msk_32_25_status */
    SYSFS_ATTR_PTR(port_17_24_lpmode),    /* register: zqsfp28_lpmode_24_17 */
    SYSFS_ATTR_PTR(port_25_32_lpmode),    /* register: zqsfp28_lpmode_32_25 */

    SYSFS_ATTR_PTR(debug),                /* debug flag for print more messages */
    NULL
};
static const struct attribute_group slave_cpld_group_misc = {
    .attrs = misc_attributes,
};

static int slave_cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct slave_cpld_data *slave_cpld;
    int err, i;
    int grp_number = (int)( sizeof(slave_cpld_port_group) / sizeof(slave_cpld_port_group[0]));

    /* allocate memory to slave_cpld */
    slave_cpld = devm_kzalloc(&client->dev, sizeof(struct slave_cpld_data) , GFP_KERNEL);

    if (!slave_cpld)
        return -ENOMEM;

    mutex_init(&slave_cpld->lock);

    for(i = 0 ; i < grp_number ; i++){
        err = sysfs_create_group(&client->dev.kobj, &slave_cpld_port_group[i]);
        if (err){
            printk("%s: Error creeat port group %d.\n", __FUNCTION__, i+1);
        }
    }
    err = sysfs_create_group(&client->dev.kobj, &slave_cpld_group_misc);
    if (err){
        printk("%s: Error creeat misc group.\n", __FUNCTION__);
    }

    slave_cpld->client = client;
    i2c_set_clientdata(client, slave_cpld);

    printk(KERN_INFO "%s: Slave CPLD LCMXO3LF created.\n", __FUNCTION__);

    return 0;

}

static int slave_cpld_remove(struct i2c_client *client)
{
    int i;
    int grp_number = (int)( sizeof(slave_cpld_port_group) / sizeof(slave_cpld_port_group[0]));

    for(i = 0 ; i < grp_number ; i++){
        sysfs_remove_group(&client->dev.kobj, &slave_cpld_port_group[i]);
    }
    sysfs_remove_group(&client->dev.kobj, &slave_cpld_group_misc);

    printk(KERN_INFO "%s: Slave CPLD removed.\n", __FUNCTION__);
    return 0;
}

static struct i2c_driver slave_cpld_driver = {
    .driver = {
        .name = "slave_cpld",
        .owner = THIS_MODULE,
    },
    .probe = slave_cpld_probe,
    .remove = slave_cpld_remove,
    .id_table = slave_cpld_ids,
};

static int __init slave_cpld_init(void)
{
    printk(KERN_INFO "%s: init.\n", __FUNCTION__);
    return i2c_add_driver(&slave_cpld_driver);
}
module_init(slave_cpld_init);

static void __exit slave_cpld_exit(void)
{
    printk(KERN_INFO "%s: exit.\n", __FUNCTION__);
    i2c_del_driver(&slave_cpld_driver);
}
module_exit(slave_cpld_exit);

MODULE_DESCRIPTION("mitac_ly1200_32x_slave_cpld driver");
MODULE_AUTHOR("Eddy Weng <eddy.weng@mic.com.tw>");
MODULE_LICENSE("GPL");

