/* An hwmon driver for Netberg Aurora 715 Marvell Teralynx i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-netberg-aurora-715.h"
#include "x86-64-netberg-aurora-715-common.h"

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x30, 0x31, 0x32, I2C_CLIENT_END };
static int	debug = 0;


/* i2c_client Declaration */
struct i2c_client *Netberg_CPLD_30_client; //0x30 for SYS CPLD
struct i2c_client *Netberg_CPLD_31_client; //0x31 for Port 01-16
struct i2c_client *Netberg_CPLD_32_client; //0x32 for Port 17-32
struct i2c_client *Netberg_CPLD_23_client; //0x23 for Fan CPLD
struct i2c_client *Netberg_CPLD_35_client; //0x35 for Power CPLD
struct i2c_client *Netberg_BMC_14_client;  //0x14 for BMC slave
struct i2c_client *Netberg_EEPROM_56_client;  //0x56 for EEPROM

/* end of i2c_client Declaration */

/* register offset define */
#define BMC_EN_REG      0xA4
/* end of register offset define */

/* common function */
int bmc_enable(void)
{
    if ((i2c_smbus_read_byte_data(Netberg_CPLD_30_client, BMC_EN_REG) & BIT_0_MASK) == 0x01)
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

static int Netberg_i2c_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    struct Netberg_i2c_data *Netberg_CPLD_30_data;
    struct Netberg_i2c_data *Netberg_CPLD_31_data;
    struct Netberg_i2c_data *Netberg_CPLD_32_data;
    struct Netberg_i2c_data *Netberg_CPLD_23_data;
    struct Netberg_i2c_data *Netberg_CPLD_35_data;
    struct Netberg_i2c_data *Netberg_BMC_14_data;
    
    struct eeprom_data *Netberg_EEPROM_56_data;
    
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
    {
        status = -EIO;
        goto exit;
    }

    if (!(Netberg_CPLD_30_data = kzalloc(sizeof(struct Netberg_i2c_data), GFP_KERNEL)))
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    if (!(Netberg_CPLD_31_data = kzalloc(sizeof(struct Netberg_i2c_data), GFP_KERNEL)))
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }

    if (!(Netberg_CPLD_32_data = kzalloc(sizeof(struct Netberg_i2c_data), GFP_KERNEL)))
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    if (!(Netberg_CPLD_23_data = kzalloc(sizeof(struct Netberg_i2c_data), GFP_KERNEL)))
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    if (!(Netberg_CPLD_35_data = kzalloc(sizeof(struct Netberg_i2c_data), GFP_KERNEL)))
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    
    if (!(Netberg_BMC_14_data = kzalloc(sizeof(struct Netberg_i2c_data), GFP_KERNEL)))
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }

    if (!(Netberg_EEPROM_56_data = kzalloc(sizeof(struct eeprom_data), GFP_KERNEL))) 
    {
        status = -ENOMEM;
        goto exit;
    }
    memset(Netberg_EEPROM_56_data->data, 0xff, EEPROM_SIZE);
    
    i2c_set_clientdata(Netberg_CPLD_30_client, Netberg_CPLD_30_data);
    i2c_set_clientdata(Netberg_CPLD_31_client, Netberg_CPLD_31_data);
    i2c_set_clientdata(Netberg_CPLD_32_client, Netberg_CPLD_32_data);
    i2c_set_clientdata(Netberg_CPLD_23_client, Netberg_CPLD_23_data);
    i2c_set_clientdata(Netberg_CPLD_35_client, Netberg_CPLD_35_data);
    i2c_set_clientdata(Netberg_BMC_14_client , Netberg_BMC_14_data);
    i2c_set_clientdata(Netberg_EEPROM_56_client, Netberg_EEPROM_56_data);

    mutex_init(&Netberg_CPLD_30_data->update_lock);
    mutex_init(&Netberg_CPLD_31_data->update_lock);
    mutex_init(&Netberg_CPLD_32_data->update_lock);
    mutex_init(&Netberg_CPLD_23_data->update_lock);
    mutex_init(&Netberg_CPLD_35_data->update_lock);
    mutex_init(&Netberg_BMC_14_data->update_lock);
    mutex_init(&Netberg_EEPROM_56_data->update_lock);

    Netberg_CPLD_30_data->valid = 0;
    mutex_init(&Netberg_CPLD_30_data->update_lock);
    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &NBA715_SYS_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &NBA715_LED_group);
    if (status)
    {
        goto exit_free;
    }

    status = sysfs_create_group(&client->dev.kobj, &NBA715_FAN_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &NBA715_THERMAL_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &NBA715_POWER_group);
    if (status)
    {
        goto exit_free;
    }
    
    status = sysfs_create_group(&client->dev.kobj, &NBA715_QSFP_group);
    if (status)
    {
        goto exit_free;
    }

    /* create the sysfs eeprom file */
    status = sysfs_create_bin_file(&client->dev.kobj, &net_eeprom_attr);
    if (status) {
        goto exit_free;
    }


    Netberg_CPLD_30_data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(Netberg_CPLD_30_data->hwmon_dev))
    {
        status = PTR_ERR(Netberg_CPLD_30_data->hwmon_dev);
        goto exit_remove;
    }
    dev_info(&client->dev, "%s: '%s'\n", dev_name(Netberg_CPLD_30_data->hwmon_dev), client->name);
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &NBA715_SYS_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_LED_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_FAN_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_THERMAL_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_POWER_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_QSFP_group);

exit_free:
    kfree(Netberg_CPLD_30_data);
    kfree(Netberg_EEPROM_56_data);

exit:
    return status;
}

static int Netberg_i2c_remove(struct i2c_client *client)
{
    struct Netberg_i2c_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &NBA715_SYS_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_LED_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_FAN_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_THERMAL_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_POWER_group);
    sysfs_remove_group(&client->dev.kobj, &NBA715_QSFP_group);

    sysfs_remove_bin_file(&client->dev.kobj, &net_eeprom_attr);

    kfree(data);
    return 0;
}

static const struct i2c_device_id Netberg_i2c_id[] =
{
    { "Netberg_CPLD_30", 0 },
    {},
};
MODULE_DEVICE_TABLE(i2c, Netberg_i2c_id);

static struct i2c_driver Netberg_i2c_driver =
{
    .class        = I2C_CLASS_HWMON,
    .driver =
    {
        .name     = "NBA_715_i2c",
    },
    .probe        = Netberg_i2c_probe,
    .remove       = Netberg_i2c_remove,
    .id_table     = Netberg_i2c_id,
    .address_list = normal_i2c,
};

/*For main Switch board*/
static struct i2c_board_info Netberg_CPLD_30_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Netberg_CPLD_30", 0x30),
        .platform_data = NULL,
    },
};

/*For QSFP Port 01 - 16*/
static struct i2c_board_info Netberg_CPLD_31_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Netberg_CPLD_31", 0x31),
        .platform_data = NULL,
    },
};
/*For QSFP Port 17 - 32*/
static struct i2c_board_info Netberg_CPLD_32_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Netberg_CPLD_32", 0x32),
        .platform_data = NULL,
    },
};
/*For Fan status*/
static struct i2c_board_info Netberg_CPLD_23_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Netberg_CPLD_23", 0x23),
        .platform_data = NULL,
    },
};
/*For Power status*/
static struct i2c_board_info Netberg_CPLD_35_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Netberg_CPLD_35", 0x35),
        .platform_data = NULL,
    },
};
/*For BMC Slave*/
static struct i2c_board_info Netberg_BMC_14_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Netberg_BMC_14", 0x14),
        .platform_data = NULL,
    },
};

static struct i2c_board_info Netberg_EEPROM_56_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Netberg_EEPROM_56", 0x56),
        .platform_data = NULL,
    },
};


static int __init Netberg_i2c_init(void)
{
    int ret;
    int cmp;
    char keyword[] = "SMBus I801";
    char buf1[128];
    struct i2c_adapter *i2c_adap;
    struct file *fp;  
    mm_segment_t fs;  
    loff_t pos; 

    printk("Open file...\n");  
    fp = filp_open("/sys/class/i2c-dev/i2c-0/name", O_RDONLY , 0644);  
    if (IS_ERR(fp)) {  
        printk("Open file FAILED\n");  
        return -1;  
    } 

    fs = get_fs();  
    set_fs(KERNEL_DS);
    pos = 0;
    vfs_read(fp, buf1, sizeof(buf1), &pos);
    printk("Detect %s\n", buf1);
    cmp = strncmp(keyword, buf1, sizeof(keyword)-1);
    set_fs(fs);

    filp_close(fp, NULL);

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
    Netberg_CPLD_30_client = i2c_new_device(i2c_adap, &Netberg_CPLD_30_info[0]);
    Netberg_CPLD_31_client = i2c_new_device(i2c_adap, &Netberg_CPLD_31_info[0]);
    Netberg_CPLD_32_client = i2c_new_device(i2c_adap, &Netberg_CPLD_32_info[0]);
    Netberg_CPLD_23_client = i2c_new_device(i2c_adap, &Netberg_CPLD_23_info[0]);
    Netberg_CPLD_35_client = i2c_new_device(i2c_adap, &Netberg_CPLD_35_info[0]);
    Netberg_BMC_14_client  = i2c_new_device(i2c_adap, &Netberg_BMC_14_info[0]);
    Netberg_EEPROM_56_client  = i2c_new_device(i2c_adap, &Netberg_EEPROM_56_info[0]);

    if (Netberg_CPLD_30_info == NULL || Netberg_CPLD_31_info == NULL || Netberg_CPLD_32_info == NULL 
     || Netberg_CPLD_23_info == NULL || Netberg_CPLD_35_info == NULL || Netberg_BMC_14_info  == NULL
     || Netberg_EEPROM_56_info == NULL)
    {
        printk("ERROR: i2c_new_device FAILED!\n");
        return -1;
    }
    
    i2c_put_adapter(i2c_adap);
    ret = i2c_add_driver(&Netberg_i2c_driver);
    printk(KERN_ALERT "NBA715 i2c Driver Version: %s\n", DRIVER_VERSION);
    printk(KERN_ALERT "NBA715 i2c Driver INSTALL SUCCESS\n");
    return ret;
}

static void __exit Netberg_i2c_exit(void)
{
    i2c_unregister_device(Netberg_CPLD_30_client);
    i2c_unregister_device(Netberg_CPLD_31_client);
    i2c_unregister_device(Netberg_CPLD_32_client);
    i2c_unregister_device(Netberg_CPLD_23_client);
    i2c_unregister_device(Netberg_CPLD_35_client);
    i2c_unregister_device(Netberg_BMC_14_client);
    i2c_unregister_device(Netberg_EEPROM_56_client);
    i2c_del_driver(&Netberg_i2c_driver);
    printk(KERN_ALERT "NBA715 i2c Driver UNINSTALL SUCCESS\n");
}

MODULE_AUTHOR("Netberg Inc.");
MODULE_DESCRIPTION("Netberg Aurora 715 i2c Driver");
MODULE_LICENSE("GPL");
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "Enable debugging (0-1)");

module_init(Netberg_i2c_init);
module_exit(Netberg_i2c_exit);
