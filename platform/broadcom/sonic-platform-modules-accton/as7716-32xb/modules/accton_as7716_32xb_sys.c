#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>

#define STRING_TO_DEC_VALUE		10
#define EEPROM_DATA_SIZE   		256


/* Addresses scanned 
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };
#define MAX_PORT_NAME_LEN 20
/* Each client has this additional data 
 */
struct as7716_32xb_sys_data {
    struct device  *hwmon_dev;
    struct mutex   lock;
    u8  index;
    unsigned char  eeprom[EEPROM_DATA_SIZE];
};


/* sysfs attributes for hwmon 
 */

static ssize_t sys_info_store(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t sys_info_show(struct device *dev, struct device_attribute *da,
             char *buf);
       
static SENSOR_DEVICE_ATTR(eeprom,  S_IWUSR|S_IRUGO, sys_info_show, sys_info_store, 0);



static struct attribute *as7716_32xb_sys_attributes[] = {
    &sensor_dev_attr_eeprom.dev_attr.attr,
    NULL
};


static ssize_t sys_info_show(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_sys_data *data = i2c_get_clientdata(client);
    int i;
    
    mutex_lock(&data->lock);
    memcpy(buf, data->eeprom, EEPROM_DATA_SIZE);
   
    
    memcpy(buf, data->eeprom, 256);
    
    
    mutex_unlock(&data->lock);
    
    return 256;
}

static ssize_t sys_info_store(struct device *dev, struct device_attribute *da,
			const char *buf, size_t size)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_sys_data *data = i2c_get_clientdata(client);
    int i=0, j=0, k=0;
    unsigned char str[3];
    unsigned int val;
   
    k=0;
    mutex_lock(&data->lock);  
    memset(data->eeprom, 0xFF, EEPROM_DATA_SIZE);  
    memset(str, 0x0, 3);
    if(strlen(buf) >= 256 )
    {
        for(i=0; i < strlen(buf) ; i++)
        {   
            for(j=0;j<2; j++)
            {
                str[j]=buf[i+j];
            }
            sscanf(str, "%x", &val);
            i=j+i-1;
            if(k>=EEPROM_DATA_SIZE)
            {
                break;
            }
            data->eeprom[k]=(unsigned char)val;
            k++;
        }
    }
    
    mutex_unlock(&data->lock);
    return size;

}

static const struct attribute_group as7716_32xb_sys_group = {
    .attrs = as7716_32xb_sys_attributes,
};

static int as7716_32xb_sys_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct as7716_32xb_sys_data *data;
    int status;

    data = kzalloc(sizeof(struct as7716_32xb_sys_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }
    i2c_set_clientdata(client, data);
    data->index = dev_id->driver_data;
    mutex_init(&data->lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as7716_32xb_sys_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: sys '%s'\n",
         dev_name(data->hwmon_dev), client->name);
    
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_sys_group);
exit_free:
    kfree(data);
exit:
    
    return status;
}

static void as7716_32xb_sys_remove(struct i2c_client *client)
{
    struct as7716_32xb_sys_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_sys_group);
    kfree(data);
    
}


static const struct i2c_device_id as7716_32xb_sys_id[] = {
    { "as7716_32xb_sys", 0 },    
    {}
};
MODULE_DEVICE_TABLE(i2c, as7716_32xb_sys_id);

static struct i2c_driver as7716_32xb_sys_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as7716_32xb_sys",
    },
    .probe        = as7716_32xb_sys_probe,
    .remove       = as7716_32xb_sys_remove,
    .id_table     = as7716_32xb_sys_id,
    .address_list = normal_i2c,
};






static int __init as7716_32xb_sys_init(void)
{
    return i2c_add_driver(&as7716_32xb_sys_driver);
}

static void __exit as7716_32xb_sys_exit(void)
{
    i2c_del_driver(&as7716_32xb_sys_driver);
}

module_init(as7716_32xb_sys_init);
module_exit(as7716_32xb_sys_exit);

MODULE_AUTHOR("Jostar yang <jostar_yang@accton.com.tw>");
MODULE_DESCRIPTION("as7716_32xb_sys driver");
MODULE_LICENSE("GPL");
