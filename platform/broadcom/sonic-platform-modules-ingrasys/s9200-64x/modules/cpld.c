#include <linux/module.h>   
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/platform_data/i2c-mux-gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#define S9200_MUX_BASE_NR   0
#define CPLD_DEVICE_NUM     5

/* CPLD registers */
#define CPLD_REG_REV 0x01
#define CPLD_REG_ID  0x02
#define CPLD_REG_10G_MUX  0x41

/* QSFP signal bit in register */
#define BIT_RST 0
#define BIT_LPM 2
#define BIT_INT 0
#define BIT_ABS 1

static void device_release(struct device *dev)
{
    return;
}

/*
 * S9200 CPLD register addresses 
 */
static const int int_abs_reg[CPLD_DEVICE_NUM][2]= {
    {0x20, 0x2B},
    {0x20, 0x2C},
    {0x20, 0x2C},
    {0x20, 0x2C},
    {0x20, 0x2C}
};

static const int rst_lp_reg[CPLD_DEVICE_NUM][2]= {
    {0x30, 0x3B},
    {0x30, 0x3C},
    {0x30, 0x3C},
    {0x30, 0x3C},
    {0x30, 0x3C}
};

/*
 * S9200 CPLD
 */

enum cpld_type {
    cpld_1,
    cpld_2,
    cpld_3,
    cpld_4,
    cpld_5,
};

enum qsfp_signal {
    sig_int,
    sig_abs,
    sig_rst,
    sig_lpm
};

struct cpld_platform_data {
    int reg_addr;
    struct i2c_client *client;
};

static struct cpld_platform_data s9200_cpld_platform_data[] = {
    [cpld_1] = {
        .reg_addr = 0x33,
    },

    [cpld_2] = {
        .reg_addr = 0x33,
    },

    [cpld_3] = {
        .reg_addr = 0x33,
    },

    [cpld_4] = {
        .reg_addr = 0x33,
    },

    [cpld_5] = {
        .reg_addr = 0x33,
    },
};

static struct platform_device s9200_cpld = {
    .name               = "ingrasys-s9200-cpld",
    .id                 = 0,
    .dev                = {
                .platform_data   = s9200_cpld_platform_data,
                .release         = device_release
    },
};

/*
 * S9200 I2C DEVICES
 */

struct i2c_device_platform_data {
    int parent;
    struct i2c_board_info           info;
    struct i2c_client              *client;
};

/* module_platform_driver */
static ssize_t get_cpld_reg(struct device *dev, struct device_attribute *devattr, char *buf, int signal)
{
    int ret;
    u64 data = 0;
    u64 shift = 0;
    int i = 0; 
    int j = 0;
    int port = 0;
    int bit = 0;
    int bit_mask = 0;
    int (*reg)[CPLD_DEVICE_NUM][2];
    struct cpld_platform_data *pdata = NULL;

    pdata = dev->platform_data;

    switch(signal) {
        case sig_int:
            bit = BIT_INT;
            reg = (typeof(reg)) &int_abs_reg;
            break;
        case sig_abs:
            bit = BIT_ABS;
            reg = (typeof(reg)) &int_abs_reg;
            break;
        case sig_rst:
            bit = BIT_RST;
            reg = (typeof(reg)) &rst_lp_reg;
            break;
        case sig_lpm:
            bit = BIT_LPM;
            reg = (typeof(reg)) &rst_lp_reg;
            break;
        default:
            return sprintf(buf, "na");
    }
    bit_mask = 0x1 << bit;

    for (i=0; i<CPLD_DEVICE_NUM; ++i) {
        for (j = (*reg)[i][0]; j <= (*reg)[i][1]; ++j) {
            ret = i2c_smbus_read_byte_data(pdata[i].client, j);
            if (ret < 0) {
                return sprintf(buf, "na");
            }
            shift = ((u64) ((ret & bit_mask) >> bit)) << port;
            data |= shift;
            port++;
        }
    }

    return sprintf(buf, "0x%016llx\n", data);
}

static ssize_t set_cpld_reg(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count, int signal) 
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;
    u8 current_reg_val = 0;
    u8 new_reg_val = 0;
    int value;
    int i = 0;
    int j = 0;
    int port = 0;
    int ret = 0;
    int bit = 0;
    int (*reg)[CPLD_DEVICE_NUM][2];

    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;

    switch(signal) {
        case sig_int:
            bit = BIT_INT;
            reg = (typeof(reg)) &int_abs_reg;
            break;
        case sig_abs:
            bit = BIT_ABS;
            reg = (typeof(reg)) &int_abs_reg;
            break;
        case sig_rst:
            bit = BIT_RST;
            reg = (typeof(reg)) &rst_lp_reg;
            break;
        case sig_lpm:
            bit = BIT_LPM;
            reg = (typeof(reg)) &rst_lp_reg;
            break;
        default:
            return sprintf(buf, "na");
    }

    for (i=0; i<CPLD_DEVICE_NUM; ++i) {
        for (j = (*reg)[i][0]; j <= (*reg)[i][1]; ++j) {
            //read reg value
            current_reg_val = i2c_smbus_read_byte_data(pdata[i].client, j);
            if (current_reg_val < 0) {
                return current_reg_val;
            }

            //get new value of port N from data
            value = (data >> port) & 0x1;

            //set value on bit N of new_reg_val
            if (value > 0) {
                new_reg_val = current_reg_val | (u8) (0x1 << bit);
            } else {
                new_reg_val = current_reg_val & (u8) ~(0x1 << bit);
            } 
            //write reg value if changed
            if (current_reg_val != new_reg_val) {
                ret = i2c_smbus_write_byte_data(pdata[i].client, j, 
                                                (u8)(new_reg_val));
                if (ret < 0){
                    return ret;
                }
            }
            port++;
        }
    }

    return count;
}

static ssize_t get_lpmode(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    return get_cpld_reg(dev, devattr, buf, sig_lpm);
}

static ssize_t set_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    return set_cpld_reg(dev, devattr, buf, count, sig_lpm);
}

static ssize_t get_reset(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    return get_cpld_reg(dev, devattr, buf, sig_rst);
}

static ssize_t set_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    return set_cpld_reg(dev, devattr, buf, count, sig_rst);
}

static ssize_t get_modprs(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    return get_cpld_reg(dev, devattr, buf, sig_abs);
}

static ssize_t get_int(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    return get_cpld_reg(dev, devattr, buf, sig_int);
}

static ssize_t get_cpld_version(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int i = 0;
    int cnt = 0; 
    u8 reg_val_rev[CPLD_DEVICE_NUM];
    u8 reg_val_id[CPLD_DEVICE_NUM];

    struct cpld_platform_data *pdata = dev->platform_data;

    //get reg value
    for (i=0; i<CPLD_DEVICE_NUM; ++i) {
        reg_val_rev[i] = i2c_smbus_read_byte_data(pdata[i].client, CPLD_REG_REV);
        if (reg_val_rev[i] < 0) {
            return sprintf(buf, "na");
        }
        reg_val_id[i] = i2c_smbus_read_byte_data(pdata[i].client, CPLD_REG_ID);
        if (reg_val_id[i] < 0) {
            return sprintf(buf, "na");
        }
    }

    //output reg value
    for (i=0; i<CPLD_DEVICE_NUM; ++i) {

        cnt += sprintf(buf + cnt, 
                "CPLD[%d]:\n"
                "  [1] Code Revision Bit = %d\n"
                "  [2] Release Bit       = %d\n"
                "  [3] ID                = %d\n", 
                i, 
                reg_val_rev[i] & 0x1F,
                reg_val_rev[i] >> 6 & 0x1,
                reg_val_id[i] & 0x7);
    }

    return cnt;
}

static ssize_t set_10g_mux(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    u8 data;
    u8 current_reg_val = 0;
    u8 new_reg_val = 0;
    int cpld_dev_num=cpld_1;
    int reg_offset=CPLD_REG_10G_MUX;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;
    int value;
    int i = 0;
    int port = 0;
    int ret = 0;
    int bit = 0;
    
    err = kstrtou8(buf, 16, &new_reg_val);
    if (err)
        return err;

    //read reg value
    current_reg_val = i2c_smbus_read_byte_data(pdata[cpld_dev_num].client, reg_offset);
    if (current_reg_val < 0) {
        return current_reg_val;
    }

    //write reg value if changed
    if (current_reg_val != new_reg_val) {
        ret = i2c_smbus_write_byte_data(pdata[cpld_dev_num].client, reg_offset,
                                        (u8)(new_reg_val));
        if (ret < 0){
            return ret;
        }
    }

    return count;
}

static ssize_t get_10g_mux(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int cpld_dev_num=cpld_1;
    int reg_offset=CPLD_REG_10G_MUX;
    u8 reg_val=0;
    struct cpld_platform_data *pdata = dev->platform_data;

    //read 10G mux register
    reg_val = i2c_smbus_read_byte_data(pdata[cpld_dev_num].client, reg_offset);
    if (reg_val < 0) {
        return sprintf(buf, "na");
    }

    return sprintf(buf, "0x%x\n", reg_val);
}

static DEVICE_ATTR(qsfp_int, S_IRUGO, get_int, NULL);
static DEVICE_ATTR(qsfp_modprs, S_IRUGO, get_modprs, NULL);
static DEVICE_ATTR(qsfp_lpmode, S_IRUGO | S_IWUSR, get_lpmode, set_lpmode);
static DEVICE_ATTR(qsfp_reset,  S_IRUGO | S_IWUSR, get_reset, set_reset);
static DEVICE_ATTR(cpld_version, S_IRUGO, get_cpld_version, NULL);
static DEVICE_ATTR(cpld_10g_mux, S_IRUGO | S_IWUSR, get_10g_mux, set_10g_mux);

static struct attribute *s9200_cpld_attrs[] = {
    &dev_attr_qsfp_int.attr,
    &dev_attr_qsfp_lpmode.attr,
    &dev_attr_qsfp_reset.attr,
    &dev_attr_qsfp_modprs.attr,
    &dev_attr_cpld_version.attr,
    &dev_attr_cpld_10g_mux.attr,
    NULL,
};

static struct attribute_group s9200_cpld_attr_grp = {
    .attrs = s9200_cpld_attrs,
};

static int __init cpld_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent[CPLD_DEVICE_NUM];
    int i;
    int ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
        return -ENODEV;
    }

    for (i = 0; i < CPLD_DEVICE_NUM; i++) {
        parent[i] = i2c_get_adapter(S9200_MUX_BASE_NR + i + 1);
        if (!parent[i]) {
            printk(KERN_WARNING "Parent adapter (%d) not found\n",
                S9200_MUX_BASE_NR + i + 1);
            return -ENODEV;
        }
        pdata[i].client = i2c_new_dummy(parent[i], pdata[i].reg_addr);
        if (!pdata[i].client) {
            printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[i].reg_addr);
            goto error;
        }
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &s9200_cpld_attr_grp);
    if (ret) 
        goto error;

    return 0;

error:
    i2c_put_adapter(parent[i]);
    i--;
    for (; i >= 0; i--) {
        if (pdata[i].client) {
            i2c_unregister_device(pdata[i].client);
            i2c_put_adapter(parent[i]);
        }
    }
    
    return -ENODEV; 
}

static int __exit cpld_remove(struct platform_device *pdev)
{
    int i;
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;

    sysfs_remove_group(&pdev->dev.kobj, &s9200_cpld_attr_grp);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    } else {
        for (i = 0; i < CPLD_DEVICE_NUM; i++) {
            if (pdata[i].client) {
                parent = (pdata[i].client)->adapter;
                i2c_unregister_device(pdata[i].client);
                i2c_put_adapter(parent);
            }
        }
    }

    return 0;
}

static struct platform_driver cpld_driver = {
    .probe  = cpld_probe,
    .remove = __exit_p(cpld_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "ingrasys-s9200-cpld",
    },
};

static int __init ingrasys_s9200_platform_init(void)
{
    int ret = 0;

    printk("ingrasysl_s9200_platform module initialization\n");

    mdelay(10000);
    
    ret = platform_driver_register(&cpld_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register cpld driver\n");
        goto error_cpld_driver;
    }
    ret = platform_device_register(&s9200_cpld);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld device\n");
        goto error_cpld;
    }

    return 0;

error_cpld:    
    platform_driver_unregister(&cpld_driver);
error_cpld_driver:
    return ret;
}

static void __exit ingrasys_s9200_platform_exit(void)
{
    platform_device_unregister(&s9200_cpld);
    platform_driver_unregister(&cpld_driver);
}

module_init(ingrasys_s9200_platform_init);
module_exit(ingrasys_s9200_platform_exit);

MODULE_DESCRIPTION("Ingrasys S9200 Platform Support");
MODULE_AUTHOR("Jason Tsai <feng.cf.lee@ingrasys.com>");
MODULE_LICENSE("GPL");
