/* An hwmon driver for Cameo ESCC601-32Q Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-escc601-32q.h"
#include "x86-64-cameo-escc601-32q-common.h"

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x30, 0x31, 0x32, I2C_CLIENT_END };
static int	debug = 0;


/* i2c_client Declaration */
struct i2c_client *Cameo_CPLD_30_client; //0x30 for SYS CPLD
struct i2c_client *Cameo_CPLD_31_client; //0x31 for Port 01-16
struct i2c_client *Cameo_CPLD_32_client; //0x32 for Port 17-32
struct i2c_client *Cameo_CPLD_23_client; //0x23 for Fan CPLD
struct i2c_client *Cameo_CPLD_35_client; //0x35 for Power CPLD
struct i2c_client *Cameo_BMC_14_client;  //0x14 for BMC slave
/* end of i2c_client Declaration */

/* register offset define */
#define BMC_EN_REG      0xA4
/* end of register offset define */

/* common function */
int bmc_enable(void)
{
    if ((i2c_smbus_read_byte_data(Cameo_CPLD_30_client, BMC_EN_REG) & BIT_0_MASK) == 0x01)
    {
        return ENABLE;
    }
    else
    {
        return DISABLE;
    }
}

int read_8bit_temp(u8 sign,u8 value)
{
    int result = 0;
    if(sign)
    {
        //printf("read_8bit_temp UP %d\n", value & 0x80);
        value = ~(value)+1;
        result = value;
        return result;
    }
    else
    {
        //printf("read_8bit_temp DOWN %d\n", value & 0x80);
        result = value;
        return result;
    }
}

/* end of common function*/

static int Cameo_i2c_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    struct Cameo_i2c_data *Cameo_CPLD_30_data;
    struct Cameo_i2c_data *Cameo_CPLD_31_data;
    struct Cameo_i2c_data *Cameo_CPLD_32_data;
    struct Cameo_i2c_data *Cameo_CPLD_23_data;
    struct Cameo_i2c_data *Cameo_CPLD_35_data;
    struct Cameo_i2c_data *Cameo_BMC_14_data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
    {
        status = -EIO;
        goto exit;
    }
    Cameo_CPLD_30_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Cameo_CPLD_30_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    Cameo_CPLD_31_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Cameo_CPLD_31_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }

    Cameo_CPLD_32_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Cameo_CPLD_32_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    Cameo_CPLD_23_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Cameo_CPLD_23_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    Cameo_CPLD_35_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Cameo_CPLD_35_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    Cameo_BMC_14_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Cameo_BMC_14_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    i2c_set_clientdata(Cameo_CPLD_30_client, Cameo_CPLD_30_data);
    i2c_set_clientdata(Cameo_CPLD_31_client, Cameo_CPLD_31_data);
    i2c_set_clientdata(Cameo_CPLD_32_client, Cameo_CPLD_32_data);
    i2c_set_clientdata(Cameo_CPLD_23_client, Cameo_CPLD_23_data);
    i2c_set_clientdata(Cameo_CPLD_35_client, Cameo_CPLD_35_data);
    i2c_set_clientdata(Cameo_BMC_14_client , Cameo_BMC_14_data);

    mutex_init(&Cameo_CPLD_30_data->update_lock);
    mutex_init(&Cameo_CPLD_31_data->update_lock);
    mutex_init(&Cameo_CPLD_32_data->update_lock);
    mutex_init(&Cameo_CPLD_23_data->update_lock);
    mutex_init(&Cameo_CPLD_35_data->update_lock);
    mutex_init(&Cameo_BMC_14_data->update_lock);

    Cameo_CPLD_30_data->valid = 0;
    mutex_init(&Cameo_CPLD_30_data->update_lock);
    dev_info(&client->dev, "chip found\n");
    
    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &ESCC601_SYS_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &ESCC601_LED_group);
    if (status)
    {
        goto exit_free;
    }

    status = sysfs_create_group(&client->dev.kobj, &ESCC601_FAN_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &ESCC601_THERMAL_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &ESCC601_POWER_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &ESCC601_QSFP_group);
    if (status)
    {
        goto exit_free;
    }

    Cameo_CPLD_30_data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(Cameo_CPLD_30_data->hwmon_dev))
    {
        status = PTR_ERR(Cameo_CPLD_30_data->hwmon_dev);
        goto exit_remove;
    }
    dev_info(&client->dev, "%s: '%s'\n", dev_name(Cameo_CPLD_30_data->hwmon_dev), client->name);
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &ESCC601_SYS_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_LED_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_FAN_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_THERMAL_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_POWER_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_QSFP_group);

exit_free:
    kfree(Cameo_CPLD_30_data);

exit:
    return status;
}

static int Cameo_i2c_remove(struct i2c_client *client)
{
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_SYS_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_LED_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_FAN_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_THERMAL_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_POWER_group);
    sysfs_remove_group(&client->dev.kobj, &ESCC601_QSFP_group);

    kfree(data);
    return 0;
}

static const struct i2c_device_id Cameo_i2c_id[] =
{
    { "Cameo_CPLD_30", 0 },
    {},
};
MODULE_DEVICE_TABLE(i2c, Cameo_i2c_id);

static struct i2c_driver Cameo_i2c_driver =
{
    .class        = I2C_CLASS_HWMON,
    .driver =
    {
        .name     = "ESCC_601_i2c",
    },
    .probe        = Cameo_i2c_probe,
    .remove       = Cameo_i2c_remove,
    .id_table     = Cameo_i2c_id,
    .address_list = normal_i2c,
};

/*For main Switch board*/
static struct i2c_board_info Cameo_CPLD_30_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_CPLD_30", 0x30),
        .platform_data = NULL,
    },
};

/*For QSFP Port 01 - 16*/
static struct i2c_board_info Cameo_CPLD_31_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_CPLD_31", 0x31),
        .platform_data = NULL,
    },
};
/*For QSFP Port 17 - 32*/
static struct i2c_board_info Cameo_CPLD_32_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_CPLD_32", 0x32),
        .platform_data = NULL,
    },
};
/*For Fan status*/
static struct i2c_board_info Cameo_CPLD_23_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_CPLD_23", 0x23),
        .platform_data = NULL,
    },
};
/*For Power status*/
static struct i2c_board_info Cameo_CPLD_35_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_CPLD_35", 0x35),
        .platform_data = NULL,
    },
};
/*For BMC Slave*/
static struct i2c_board_info Cameo_BMC_14_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_BMC_14", 0x14),
        .platform_data = NULL,
    },
};

static int __init Cameo_i2c_init(void)
{
    int ret;
    int cmp;
    char keyword[] = "SMBus I801";
    char buf1[128];
    struct i2c_adapter *i2c_adap;
    struct file *fp;  
    mm_segment_t fs;  
    loff_t pos; 

    fp = filp_open("/sys/class/i2c-dev/i2c-0/name", O_RDONLY , 0644);  
    if (IS_ERR(fp)) {  
        printk("Open file FAILED\n");  
        return -1;  
    } 

    fs = get_fs();  
    set_fs(KERNEL_DS);
    pos = 0;
    ret = vfs_read(fp, buf1, sizeof(buf1), &pos);
    set_fs(fs);
    filp_close(fp, NULL);
	if(ret<0)
	  return -1;	  
	buf1[ret-1]='\0';
    printk("Detect %s\n", buf1);
    cmp = strncmp(keyword, buf1, sizeof(keyword)-1);

    if(cmp == 0)
    {
        i2c_adap = i2c_get_adapter(0);
        printk("SMBus I801 is at bus 0\n");
    }
    else
    {
        i2c_adap = i2c_get_adapter(1);
        printk("SMBus I801 is at bus 1\n");
    }

    if (i2c_adap == NULL)
    {
        printk("ERROR: i2c_get_adapter FAILED!\n");
        return -1;
    }
    Cameo_CPLD_30_client = i2c_new_device(i2c_adap, &Cameo_CPLD_30_info[0]);
    Cameo_CPLD_31_client = i2c_new_device(i2c_adap, &Cameo_CPLD_31_info[0]);
    Cameo_CPLD_32_client = i2c_new_device(i2c_adap, &Cameo_CPLD_32_info[0]);
    Cameo_CPLD_23_client = i2c_new_device(i2c_adap, &Cameo_CPLD_23_info[0]);
    Cameo_CPLD_35_client = i2c_new_device(i2c_adap, &Cameo_CPLD_35_info[0]);
    Cameo_BMC_14_client  = i2c_new_device(i2c_adap, &Cameo_BMC_14_info[0]);

    if (Cameo_CPLD_30_info == NULL || Cameo_CPLD_31_info == NULL || Cameo_CPLD_32_info == NULL 
     || Cameo_CPLD_23_info == NULL || Cameo_CPLD_35_info == NULL || Cameo_BMC_14_info  == NULL)
    {
        printk("ERROR: i2c_new_device FAILED!\n");
        return -1;
    }
    
    i2c_put_adapter(i2c_adap);
    ret = i2c_add_driver(&Cameo_i2c_driver);
    printk(KERN_ALERT "ESCC601-32Q i2c Driver Version: %s\n", DRIVER_VERSION);
    printk(KERN_ALERT "ESCC601-32Q i2c Driver INSTALL SUCCESS\n");
    return ret;
}

static void __exit Cameo_i2c_exit(void)
{
    i2c_unregister_device(Cameo_CPLD_30_client);
    i2c_unregister_device(Cameo_CPLD_31_client);
    i2c_unregister_device(Cameo_CPLD_32_client);
    i2c_unregister_device(Cameo_CPLD_23_client);
    i2c_unregister_device(Cameo_CPLD_35_client);
    i2c_unregister_device(Cameo_BMC_14_client);
    i2c_del_driver(&Cameo_i2c_driver);
    printk(KERN_ALERT "ESCC601-32Q i2c Driver UNINSTALL SUCCESS\n");
}

MODULE_AUTHOR("Cameo Inc.");
MODULE_DESCRIPTION("Cameo ESCC601-32Q i2c Driver");
MODULE_LICENSE("GPL");
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "Enable debugging (0-1)");

module_init(Cameo_i2c_init);
module_exit(Cameo_i2c_exit);
