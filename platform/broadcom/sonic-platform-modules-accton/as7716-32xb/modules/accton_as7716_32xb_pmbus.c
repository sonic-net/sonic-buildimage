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


/* Addresses scanned 
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data 
 */
struct as7716_32xb_pmbus_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    u8  index;
    u8   capability; 
    u16  status_word;
    u8   fan_fault;
    u8   over_temp;
    u16  v_out;
    u16  i_out;
    u16  p_out;
    u16  temp;
    u16  fan_speed;
    u16  fan_duty_cycle;
    u8   fan_dir[4];
    u8   pmbus_revision;
    u8   mfr_id[10];     
    u8   mfr_model[10];
    u8   mfr_revision[3];
    u16  mfr_vin_min;
    u16  mfr_vin_max;
    u16  mfr_iin_max;
    u16  mfr_iout_max;
    u16  mfr_pin_max;
    u16  mfr_pout_max;
    u16  mfr_vout_min;
    u16  mfr_vout_max;
    u16  power_on;
    u16  temp_fault;
    u16  power_good;
};



enum as7716_32xb_pmbus_sysfs_attributes {
    PSU_POWER_ON = 0,
    PSU_TEMP_FAULT,
    PSU_POWER_GOOD,
    PSU_FAN1_FAULT,
    PSU_FAN_DIRECTION,
    PSU_OVER_TEMP,
    PSU_V_OUT,
    PSU_I_OUT,
    PSU_P_OUT,
    PSU_P_OUT_UV,     /*In Unit of microVolt, instead of mini.*/
    PSU_TEMP1_INPUT,
    PSU_FAN1_SPEED,
    PSU_FAN1_DUTY_CYCLE,
    PSU_PMBUS_REVISION,
    PSU_MFR_ID,
    PSU_MFR_MODEL,
    PSU_MFR_REVISION,
    PSU_MFR_VIN_MIN,
    PSU_MFR_VIN_MAX,
    PSU_MFR_VOUT_MIN,
    PSU_MFR_VOUT_MAX,
    PSU_MFR_IIN_MAX,
    PSU_MFR_IOUT_MAX,
    PSU_MFR_PIN_MAX,
    PSU_MFR_POUT_MAX
};

/* sysfs attributes for hwmon 
 */
static ssize_t pmbus_info_show(struct device *dev, struct device_attribute *da,
             char *buf);
static ssize_t pmbus_info_store(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static SENSOR_DEVICE_ATTR(psu_power_on,   S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_POWER_ON);
static SENSOR_DEVICE_ATTR(psu_temp_fault, S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_TEMP_FAULT);
static SENSOR_DEVICE_ATTR(psu_power_good, S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_POWER_GOOD);
static SENSOR_DEVICE_ATTR(psu_fan1_fault, S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_FAN1_FAULT);
static SENSOR_DEVICE_ATTR(psu_over_temp,  S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_OVER_TEMP);
static SENSOR_DEVICE_ATTR(psu_v_out,      S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(psu_i_out,      S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(psu_p_out,      S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(psu_temp1_input,S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(psu_fan1_speed_rpm,S_IWUSR| S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(psu_fan1_duty_cycle_percentage, S_IWUSR | S_IRUGO,    pmbus_info_show, pmbus_info_store, PSU_FAN1_DUTY_CYCLE);
static SENSOR_DEVICE_ATTR(psu_fan_dir,    S_IWUSR| S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_FAN_DIRECTION);
static SENSOR_DEVICE_ATTR(psu_pmbus_revision,S_IWUSR| S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_PMBUS_REVISION);
static SENSOR_DEVICE_ATTR(psu_mfr_id,        S_IWUSR| S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_MFR_ID);
static SENSOR_DEVICE_ATTR(psu_mfr_model,    S_IWUSR|  S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_MFR_MODEL);
static SENSOR_DEVICE_ATTR(psu_mfr_revision,   S_IWUSR| S_IRUGO, pmbus_info_show,pmbus_info_store, PSU_MFR_REVISION);
static SENSOR_DEVICE_ATTR(psu_mfr_vin_min,   S_IWUSR| S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_MFR_VIN_MIN);
static SENSOR_DEVICE_ATTR(psu_mfr_vin_max,   S_IWUSR| S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_MFR_VIN_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_vout_min,  S_IWUSR| S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_MFR_VOUT_MIN);
static SENSOR_DEVICE_ATTR(psu_mfr_vout_max,  S_IWUSR| S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_MFR_VOUT_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_iin_max,  S_IWUSR| S_IRUGO, pmbus_info_show,  pmbus_info_store, PSU_MFR_IIN_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_iout_max, S_IWUSR|  S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_MFR_IOUT_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_pin_max,   S_IWUSR|S_IRUGO, pmbus_info_show,  pmbus_info_store, PSU_MFR_PIN_MAX);
static SENSOR_DEVICE_ATTR(psu_mfr_pout_max, S_IWUSR|  S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_MFR_POUT_MAX);

/*Duplicate nodes for lm-sensors.*/
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_P_OUT_UV);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, pmbus_info_show,    pmbus_info_store, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, pmbus_info_show, pmbus_info_store, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(temp1_fault,  S_IRUGO, pmbus_info_show,      pmbus_info_store, PSU_TEMP_FAULT);


static struct attribute *as7716_32xb_pmbus_attributes[] = {
    &sensor_dev_attr_psu_power_on.dev_attr.attr,
    &sensor_dev_attr_psu_temp_fault.dev_attr.attr,
    &sensor_dev_attr_psu_power_good.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_fault.dev_attr.attr,
    &sensor_dev_attr_psu_over_temp.dev_attr.attr,
    &sensor_dev_attr_psu_v_out.dev_attr.attr,
    &sensor_dev_attr_psu_i_out.dev_attr.attr,
    &sensor_dev_attr_psu_p_out.dev_attr.attr,
    &sensor_dev_attr_psu_temp1_input.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_duty_cycle_percentage.dev_attr.attr,
    &sensor_dev_attr_psu_fan_dir.dev_attr.attr,
    &sensor_dev_attr_psu_pmbus_revision.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_id.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_revision.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_vin_min.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_vin_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_pout_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_iin_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_pin_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_vout_min.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_vout_max.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_iout_max.dev_attr.attr,
    /*Duplicate nodes for lm-sensors.*/
    &sensor_dev_attr_power1_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    &sensor_dev_attr_temp1_fault.dev_attr.attr,
    NULL
};


static ssize_t pmbus_info_show(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_pmbus_data *data = i2c_get_clientdata(client);
    int status = -EINVAL;
    mutex_lock(&data->update_lock);
    switch (attr->index)
    {
        case PSU_POWER_ON:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->power_on);
            break;
        case PSU_TEMP_FAULT:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->temp_fault);
            break;
        case PSU_POWER_GOOD:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->power_good);
            break;
        case PSU_FAN1_FAULT:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->fan_fault);
            break;
        case PSU_FAN_DIRECTION:
            status=snprintf(buf, PAGE_SIZE-1, "%s\r\n", data->fan_dir);
            break;
        case PSU_OVER_TEMP:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->over_temp);
            break;
        case PSU_V_OUT:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->v_out);
            break;
        case PSU_I_OUT:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->i_out);
            break;        
        case PSU_P_OUT:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->p_out);
            break;
        case PSU_P_OUT_UV:
            status=snprintf(buf, PAGE_SIZE-1, "%ld\r\n", data->p_out * 1000000);
            break;
        case PSU_TEMP1_INPUT:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->temp);
            break;
        case PSU_FAN1_SPEED: 
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->fan_speed);
            break;
        case PSU_FAN1_DUTY_CYCLE:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->fan_duty_cycle);
            break;
        case PSU_PMBUS_REVISION:
            break;
        case PSU_MFR_ID:
            status=snprintf(buf, PAGE_SIZE-1, "%s\r\n", data->mfr_id);
            break;
        case PSU_MFR_MODEL:
            status=snprintf(buf, PAGE_SIZE-1, "%s\r\n", data->mfr_model);
            break;
        case PSU_MFR_REVISION:
            status=snprintf(buf, PAGE_SIZE-1, "%s\r\n", data->mfr_revision);
            break;
        case PSU_MFR_VIN_MIN:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->mfr_vin_min);
            break;
        case PSU_MFR_VIN_MAX:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->mfr_vin_max);
            break;
        case PSU_MFR_VOUT_MIN:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->mfr_vin_max);
            break;
        case PSU_MFR_VOUT_MAX:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->mfr_vout_max);
            break;
        case PSU_MFR_IIN_MAX:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->mfr_iin_max);
            break;
        case PSU_MFR_IOUT_MAX:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->mfr_iout_max);
            break;
        case PSU_MFR_PIN_MAX:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->mfr_pin_max);
            break;
        case PSU_MFR_POUT_MAX:
            status=snprintf(buf, PAGE_SIZE-1, "%d\r\n", data->mfr_pout_max);
            break;
        default :
            break;
    }
    mutex_unlock(&data->update_lock);
    return status;
}

static ssize_t pmbus_info_store(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct as7716_32xb_pmbus_data *data = i2c_get_clientdata(client);
    long keyin = 0;
    int status = -EINVAL;
    mutex_lock(&data->update_lock);
    status = kstrtol(buf, STRING_TO_DEC_VALUE, &keyin);
    switch (attr->index)
    {
        case PSU_POWER_ON:
            data->power_on=keyin;
            break;
        case PSU_TEMP_FAULT: 
            data->temp_fault=keyin;
            break;
        case PSU_POWER_GOOD:
            data->power_good=keyin;
            break;
        case PSU_FAN1_FAULT:
            break;
        case PSU_FAN_DIRECTION:
            memcpy(data->fan_dir, buf, sizeof(data->fan_dir));
            break;
        case PSU_OVER_TEMP:
            data->over_temp=keyin;
            break;
        case PSU_V_OUT:
            data->v_out=keyin;
            break;
        case PSU_I_OUT:
            data->i_out=keyin;
            break;        
        case PSU_P_OUT:
            data->p_out=keyin;
            break;
        case PSU_P_OUT_UV:
            //multiplier = 1000000;  /*For lm-sensors, unit is micro-Volt.*/
            
            break;
        case PSU_TEMP1_INPUT:
            data->temp=keyin;
            break;
        case PSU_FAN1_SPEED:
            data->fan_speed=keyin;
            break;
        case PSU_FAN1_DUTY_CYCLE:
            break;
        case PSU_PMBUS_REVISION:
            data->pmbus_revision=keyin;
            break;
        case PSU_MFR_ID:
            memcpy(data->mfr_id, buf, sizeof(data->mfr_id));
            break;
        case PSU_MFR_MODEL:
            memcpy(data->mfr_model, buf, sizeof(data->mfr_model));
            break;
        case PSU_MFR_REVISION:
            memcpy(data->mfr_revision, buf, sizeof(data->mfr_revision));
            break;
        case PSU_MFR_VIN_MIN:
            data->mfr_vin_min=keyin;
            break;
        case PSU_MFR_VIN_MAX:
            data->mfr_vin_max=keyin;
            break;
        case PSU_MFR_VOUT_MIN:
            data->mfr_vout_min=keyin;
            break;
        case PSU_MFR_VOUT_MAX:
            data->mfr_vout_max=keyin;
            break;
        case PSU_MFR_IIN_MAX:
            data->mfr_iin_max=keyin;
            break;
        case PSU_MFR_IOUT_MAX:
            data->mfr_iout_max=keyin;
            break;
        case PSU_MFR_PIN_MAX:
            data->mfr_pin_max=keyin;
            break;
        case PSU_MFR_POUT_MAX:
            data->mfr_pout_max=keyin;
            break;
        default :
            break;
    }
    mutex_unlock(&data->update_lock);
    return count;
}


static const struct attribute_group as7716_32xb_pmbus_group = {
    .attrs = as7716_32xb_pmbus_attributes,
};

static int as7716_32xb_pmbus_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct as7716_32xb_pmbus_data *data;
    int status;

    data = kzalloc(sizeof(struct as7716_32xb_pmbus_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }
    i2c_set_clientdata(client, data);
    data->index = dev_id->driver_data;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as7716_32xb_pmbus_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: pmbus '%s'\n",
         dev_name(data->hwmon_dev), client->name);
    
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_pmbus_group);
exit_free:
    kfree(data);
exit:
    
    return status;
}

static void as7716_32xb_pmbus_remove(struct i2c_client *client)
{
    struct as7716_32xb_pmbus_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as7716_32xb_pmbus_group);
    kfree(data);
    
}


static const struct i2c_device_id as7716_32xb_pmbus_id[] = {
    { "as7716_32xb_pmbus", 0 },    
    {}
};
MODULE_DEVICE_TABLE(i2c, as7716_32xb_pmbus_id);

static struct i2c_driver as7716_32xb_pmbus_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as7716_32xb_pmbus",
    },
    .probe        = as7716_32xb_pmbus_probe,
    .remove       = as7716_32xb_pmbus_remove,
    .id_table     = as7716_32xb_pmbus_id,
    .address_list = normal_i2c,
};






static int __init as7716_32xb_pmbus_init(void)
{
    return i2c_add_driver(&as7716_32xb_pmbus_driver);
}

static void __exit as7716_32xb_pmbus_exit(void)
{
    i2c_del_driver(&as7716_32xb_pmbus_driver);
}

module_init(as7716_32xb_pmbus_init);
module_exit(as7716_32xb_pmbus_exit);

MODULE_AUTHOR("Jostar yang <jostar_yang@accton.com.tw>");
MODULE_DESCRIPTION("as7716_32xb_pmbus driver");
MODULE_LICENSE("GPL");
