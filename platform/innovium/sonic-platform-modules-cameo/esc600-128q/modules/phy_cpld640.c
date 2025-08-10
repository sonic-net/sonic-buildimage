/* An hwmon driver for Cameo ESC600-128Q i2c Module */

#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define ESC_600_INT_WANTED
//#define PREVIOUS_CHECK_TYPE

#define DEBUG_MSG
#ifdef DEBUG_MSG
    #define debug_print(s) printk s
#else
    #define debug_print(s)
#endif

#define TURN_OFF            0
#define TURN_ON             1

enum model_type{ 
    MODEL_TYPE_100G = 0,
    MODEL_TYPE_400G = 1,
    MODEL_TYPE_UNKNOWN = 2
};

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x32, I2C_CLIENT_END };

/*0x32 PHY CPLD*/
static ssize_t portnum_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t model_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t phy_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
#ifdef ESC_600_INT_WANTED
static ssize_t QSFP_int_get(struct device *dev, struct device_attribute *da, char *buf);
#endif
static ssize_t QSFP_status_all_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t low_power_all_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t low_power_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t low_power_get(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t low_power_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t qsfp_status_get(struct device *dev, struct device_attribute *da, char *buf);

/* struct i2c_data */
struct Cameo_i2c_data
{
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;          /* !=0 if registers are valid */
    unsigned long       last_updated;   /* In jiffies */
#ifdef PREVIOUS_CHECK_TYPE    
    u8                  model_type;
#endif
};

/* struct i2c_sysfs_attributes */
enum Cameo_i2c_sysfs_attributes
{
    /*0x32 PHY CPLD*/
    SLOT_PORTNUM,
    SLOT_MODEL,
    QSFP_LOW_POWER_ALL,
    QSFP_RESET,
    QSFP_PRESENT,
    QSFP_INT
};
/* end of struct i2c_sysfs_attributes */

/* sysfs attributes for SENSOR_DEVICE_ATTR */
/*phy_cpld640_QSFP_attributes*/
#ifdef ESC_600_INT_WANTED
static SENSOR_DEVICE_ATTR(QSFP_int                  , S_IRUGO           , QSFP_int_get                  , NULL                      , QSFP_INT);
#endif
static SENSOR_DEVICE_ATTR(phy_reset                 , S_IRUGO | S_IWUSR , NULL                          , phy_reset_set             , 0);
static SENSOR_DEVICE_ATTR(portnum                   , S_IRUGO           , portnum_get                   , NULL                      , SLOT_PORTNUM);
static SENSOR_DEVICE_ATTR(model                     , S_IRUGO           , model_get                     , NULL                      , SLOT_MODEL);
static SENSOR_DEVICE_ATTR(QSFP_reset_1              , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 1);
static SENSOR_DEVICE_ATTR(QSFP_reset_2              , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 2);
static SENSOR_DEVICE_ATTR(QSFP_reset_3              , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 3);
static SENSOR_DEVICE_ATTR(QSFP_reset_4              , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 4);
static SENSOR_DEVICE_ATTR(QSFP_reset_5              , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 5);
static SENSOR_DEVICE_ATTR(QSFP_reset_6              , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 6);
static SENSOR_DEVICE_ATTR(QSFP_reset_7              , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 7);
static SENSOR_DEVICE_ATTR(QSFP_reset_8              , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 8);
static SENSOR_DEVICE_ATTR(QSFP_reset_9              , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 9);
static SENSOR_DEVICE_ATTR(QSFP_reset_10             , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 10);
static SENSOR_DEVICE_ATTR(QSFP_reset_11             , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 11);
static SENSOR_DEVICE_ATTR(QSFP_reset_12             , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 12);
static SENSOR_DEVICE_ATTR(QSFP_reset_13             , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 13);
static SENSOR_DEVICE_ATTR(QSFP_reset_14             , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 14);
static SENSOR_DEVICE_ATTR(QSFP_reset_15             , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 15);
static SENSOR_DEVICE_ATTR(QSFP_reset_16             , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , 16);
static SENSOR_DEVICE_ATTR(QSFP_low_power_1          , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 1);
static SENSOR_DEVICE_ATTR(QSFP_low_power_2          , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 2);
static SENSOR_DEVICE_ATTR(QSFP_low_power_3          , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 3);
static SENSOR_DEVICE_ATTR(QSFP_low_power_4          , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 4);
static SENSOR_DEVICE_ATTR(QSFP_low_power_5          , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 5);
static SENSOR_DEVICE_ATTR(QSFP_low_power_6          , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 6);
static SENSOR_DEVICE_ATTR(QSFP_low_power_7          , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 7);
static SENSOR_DEVICE_ATTR(QSFP_low_power_8          , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 8);
static SENSOR_DEVICE_ATTR(QSFP_low_power_9          , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 9);
static SENSOR_DEVICE_ATTR(QSFP_low_power_10         , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 10);
static SENSOR_DEVICE_ATTR(QSFP_low_power_11         , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 11);
static SENSOR_DEVICE_ATTR(QSFP_low_power_12         , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 12);
static SENSOR_DEVICE_ATTR(QSFP_low_power_13         , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 13);
static SENSOR_DEVICE_ATTR(QSFP_low_power_14         , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 14);
static SENSOR_DEVICE_ATTR(QSFP_low_power_15         , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 15);
static SENSOR_DEVICE_ATTR(QSFP_low_power_16         , S_IRUGO | S_IWUSR , low_power_get                 , low_power_set             , 16);

static SENSOR_DEVICE_ATTR(QSFP_low_power_all        , S_IRUGO | S_IWUSR , low_power_all_get             , low_power_all_set         , QSFP_LOW_POWER_ALL);
//static SENSOR_DEVICE_ATTR(QSFP_reset                , S_IRUGO | S_IWUSR , NULL                          , qsfp_reset_set            , QSFP_RESET);
static SENSOR_DEVICE_ATTR(QSFP_present_all          , S_IRUGO           , QSFP_status_all_get           , NULL                      , QSFP_PRESENT);
static SENSOR_DEVICE_ATTR(QSFP_present_1            , S_IRUGO           , qsfp_status_get               , NULL                      , 1);
static SENSOR_DEVICE_ATTR(QSFP_present_2            , S_IRUGO           , qsfp_status_get               , NULL                      , 2);
static SENSOR_DEVICE_ATTR(QSFP_present_3            , S_IRUGO           , qsfp_status_get               , NULL                      , 3);
static SENSOR_DEVICE_ATTR(QSFP_present_4            , S_IRUGO           , qsfp_status_get               , NULL                      , 4);
static SENSOR_DEVICE_ATTR(QSFP_present_5            , S_IRUGO           , qsfp_status_get               , NULL                      , 5);
static SENSOR_DEVICE_ATTR(QSFP_present_6            , S_IRUGO           , qsfp_status_get               , NULL                      , 6);
static SENSOR_DEVICE_ATTR(QSFP_present_7            , S_IRUGO           , qsfp_status_get               , NULL                      , 7);
static SENSOR_DEVICE_ATTR(QSFP_present_8            , S_IRUGO           , qsfp_status_get               , NULL                      , 8);
static SENSOR_DEVICE_ATTR(QSFP_present_9            , S_IRUGO           , qsfp_status_get               , NULL                      , 9);
static SENSOR_DEVICE_ATTR(QSFP_present_10           , S_IRUGO           , qsfp_status_get               , NULL                      , 10);
static SENSOR_DEVICE_ATTR(QSFP_present_11           , S_IRUGO           , qsfp_status_get               , NULL                      , 11);
static SENSOR_DEVICE_ATTR(QSFP_present_12           , S_IRUGO           , qsfp_status_get               , NULL                      , 12);
static SENSOR_DEVICE_ATTR(QSFP_present_13           , S_IRUGO           , qsfp_status_get               , NULL                      , 13);
static SENSOR_DEVICE_ATTR(QSFP_present_14           , S_IRUGO           , qsfp_status_get               , NULL                      , 14);
static SENSOR_DEVICE_ATTR(QSFP_present_15           , S_IRUGO           , qsfp_status_get               , NULL                      , 15);
static SENSOR_DEVICE_ATTR(QSFP_present_16           , S_IRUGO           , qsfp_status_get               , NULL                      , 16);
/* end of sysfs attributes for SENSOR_DEVICE_ATTR */


/* sysfs attributes for hwmon */
static struct attribute *phy_cpld640_QSFP_attributes[] =
{
#ifdef ESC_600_INT_WANTED
    &sensor_dev_attr_QSFP_int.dev_attr.attr,
#endif
    &sensor_dev_attr_portnum.dev_attr.attr,
    &sensor_dev_attr_model.dev_attr.attr,
    &sensor_dev_attr_phy_reset.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_1.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_2.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_3.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_4.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_5.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_6.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_7.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_8.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_9.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_10.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_11.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_12.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_13.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_14.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_15.dev_attr.attr,
    &sensor_dev_attr_QSFP_reset_16.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_all.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_1.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_2.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_3.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_4.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_5.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_6.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_7.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_8.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_9.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_10.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_11.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_12.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_13.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_14.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_15.dev_attr.attr,
    &sensor_dev_attr_QSFP_low_power_16.dev_attr.attr,
//    &sensor_dev_attr_QSFP_reset.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_all.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_1.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_2.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_3.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_4.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_5.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_6.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_7.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_8.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_9.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_10.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_11.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_12.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_13.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_14.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_15.dev_attr.attr,
    &sensor_dev_attr_QSFP_present_16.dev_attr.attr,
    NULL
};
/* end of sysfs attributes for hwmon */


#ifdef PREVIOUS_CHECK_TYPE

static umode_t phy_cpld640_is_attribute_visible(struct kobject *kobj,
        struct attribute *attr, int n)
{
    struct device *dev = kobj_to_dev(kobj);
    struct Cameo_i2c_data *data = dev_get_drvdata(dev);
    umode_t mode = attr->mode;
    
    if (attr == &sensor_dev_attr_QSFP_reset_5.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_6.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_7.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_8.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_9.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_10.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_11.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_12.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_13.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_14.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_15.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_reset_16.dev_attr.attr){

        if (data->model_type == MODEL_TYPE_400G)
            mode = 0;
    }


    if (attr == &sensor_dev_attr_QSFP_low_power_5.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_6.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_7.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_8.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_9.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_10.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_11.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_12.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_13.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_14.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_15.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_low_power_16.dev_attr.attr){

        if (data->model_type == MODEL_TYPE_400G)
            mode = 0;
    }

    if (attr == &sensor_dev_attr_QSFP_present_5.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_6.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_7.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_8.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_9.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_10.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_11.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_12.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_13.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_14.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_15.dev_attr.attr ||
        attr == &sensor_dev_attr_QSFP_present_16.dev_attr.attr){

        if (data->model_type == MODEL_TYPE_400G)
            mode = 0;
    }

    return mode;
}


#else
    
static u8 get_model_type(struct i2c_client *client, struct Cameo_i2c_data *data)
{
    u8 card_model = -EPERM;
    
	mutex_lock(&data->update_lock);
    card_model = i2c_smbus_read_byte_data(client, 0xb0);
    mutex_unlock(&data->update_lock);
    
    switch (card_model)
    {
    case 0x00:
    case 0x10:
        return MODEL_TYPE_100G;
        
    case 0x01:
    case 0x11:
        return MODEL_TYPE_400G;
        
    default:
        return MODEL_TYPE_UNKNOWN;
    }
}

#endif //PREVIOUS_CHECK_TYPE



static const struct attribute_group phy_cpld640_QSFP_group =
{
#ifdef PREVIOUS_CHECK_TYPE
    .is_visible = phy_cpld640_is_attribute_visible,
#endif    
    .attrs = phy_cpld640_QSFP_attributes,
};

/*function */

static ssize_t portnum_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    u8 res = 0;

    sprintf(buf, "\n");
    if (attr->index == SLOT_PORTNUM)
    {
    #ifdef PREVIOUS_CHECK_TYPE
        if (data->model_type == MODEL_TYPE_100G)
            res = 16;
        else if (data->model_type == MODEL_TYPE_400G)
            res = 4;
    #else    
        if (get_model_type(client, data) ==  MODEL_TYPE_100G)
            res = 16;
        else if (get_model_type(client, data) == MODEL_TYPE_400G)
            res = 4;
    #endif    
        else
            res = 0;
    }

    return sprintf(buf, "%s%d\n", buf, res);
}


/********************************************************************************/
/*    Function Name      : model_get                                       */
/*    Description        : This is the function to get module id 100G/400G      */
/*                                                                              */
/*    Input(s)           : None.                                                */
/*    Output(s)          : None.                                                */
/*    Returns            : String.                                              */
/********************************************************************************/
static ssize_t model_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 card_model = -EPERM;
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == SLOT_MODEL)
    {
        mutex_lock(&data->update_lock);
        card_model = i2c_smbus_read_byte_data(client, 0xb0);
        mutex_unlock(&data->update_lock);

        if (card_model < 0)
        {
            return sprintf(buf, "%serr(%d)\n", buf, card_model);
        }

        if (card_model == 0x00)     //Inphi 100G 16 Port
        {
            sprintf(buf, "%sInphi 100G", buf);
        }
        else if(card_model == 0x01) //Inphi 400G 4 Port
        {
            sprintf(buf, "%sInphi 400G", buf);
        }
        else if(card_model == 0x10) //Credo 100G 16 Port
        {
            sprintf(buf, "%sCredo 100G", buf);
        }
        else if(card_model == 0x11) //Credo 400G 4 Port
        {
            sprintf(buf, "%sCredo 400G", buf);
        }
        else
        {
            sprintf(buf, "%sUnknown", buf);
        }
    }

    return sprintf(buf, "%s\n", buf);
}

/********************************************************************************/
/*    Function Name      : slot_phy_reset_set                                   */
/*    Description        : This is the function to reset PHY module             */
/*                                                                              */
/*    Input(s)           : PHY module number.                                   */
/*    Output(s)          : None.                                                */
/*    Returns            : None.                                                */
/********************************************************************************/
static ssize_t phy_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 value = -EPERM;
    
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    int input = 0;
    
    input = simple_strtol(buf, NULL, 10);
    if (input == 0 || input == 1)
    {
        if(input == 0)
        {
            value = 0x00; //turn off
        }
        else if (input == 1)
        {
            value = 0xff; //turn on
        }
        printk(KERN_ALERT "phy_reset_set value = %x\n", value);

        mutex_lock(&data->update_lock);
        status = i2c_smbus_write_byte_data(client, 0xa0, value); //to reset phy
        mutex_unlock(&data->update_lock);
        if(status < 0)
        {
            printk(KERN_ALERT "ERROR: phy_reset_set FAILED\n");
            return count;
        }
        printk(KERN_ALERT "phy_reset_set set value Done\n");

    }
    else
    {
        printk(KERN_ALERT "phy_reset_set wrong value\n");
        return count;
    }
    return count;
}

#ifdef ESC_600_INT_WANTED
/********************************************************************************/
/*    Function Name      : qsfp_int_get                                         */
/*    Description        : This is the function to get qsfp interrupt status    */
/*                         0x33 0xd0                                            */
/*    Input(s)           : None.                                                */
/*    Output(s)          : None.                                                */
/*    Returns            : String.                                              */
/********************************************************************************/
static ssize_t QSFP_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 qsfp_stat = 0;
    u8 res = 0;
    u8 max_port_num = 4;
    int l;
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    u8 model_type = MODEL_TYPE_UNKNOWN;
    
    if (attr->index == QSFP_INT)
    {
    #ifdef PREVIOUS_CHECK_TYPE
        model_type = data->model_type;
    #else
        model_type = get_model_type(client, data);
    #endif
    
        if (model_type == MODEL_TYPE_UNKNOWN)
            return sprintf(buf, "%sTYPEERR\n", buf);
    
        res = i2c_smbus_read_byte_data(client, 0x90); //to get register 0x32 0x90
        qsfp_stat = res;
        if (model_type == MODEL_TYPE_100G)
        {
            max_port_num = 16;
            qsfp_stat = i2c_smbus_read_byte_data(client, 0x91); //to get register 0x32 0x91
            qsfp_stat = (qsfp_stat<<8) | res;
        }
        
        debug_print((KERN_DEBUG "DEBUG : QSFP_int_get status = %x\n", qsfp_stat));

        for (l = 1; l <= max_port_num; l++)
        {
            if (qsfp_stat & 0x01)
            {
                sprintf(buf, "%sQSFP %03d is abnormal\n", buf, l);
            }
            else
            {
                sprintf(buf, "%sQSFP %03d is OK\n", buf, l);
            }
            qsfp_stat = qsfp_stat >> 1;
        }

    }

    return sprintf(buf, "%s\n", buf);
}
#endif

/*0x32 PHY CPLD*/
/********************************************************************************/
/*    Function Name      : low_power_all_get                                    */
/*    Description        : This is the function to get all  QSFP low power mode */
/*                         0x32 0x60 0x61                                       */
/*    Input(s)           : None.                                                */
/*    Output(s)          : None.                                                */
/*    Returns            : String.                                              */
/********************************************************************************/
static ssize_t low_power_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 qsfp_stat = 0;
    u8 res = 0;
    u8 max_port_num = 4;
    u8 model_type = MODEL_TYPE_UNKNOWN;
    int l;
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == QSFP_LOW_POWER_ALL)
    {
        
    #ifdef PREVIOUS_CHECK_TYPE
        model_type = data->model_type;
    #else
        model_type = get_model_type(client, data);
    #endif
    
        if (model_type == MODEL_TYPE_UNKNOWN)
            return sprintf(buf, "%sTYPEERR\n", buf);
        
        res = i2c_smbus_read_byte_data(client, 0x60); //to get register 0x32 0x60
        qsfp_stat = res;

        if (model_type == MODEL_TYPE_100G)
        {
            max_port_num = 16;
            qsfp_stat = i2c_smbus_read_byte_data(client, 0x61); //to get register 0x32 0x61
            qsfp_stat = (qsfp_stat<<8) | res;
        }

        debug_print((KERN_DEBUG "DEBUG : low_power_all_get status = %x\n",qsfp_stat));

        for (l = 1; l <= max_port_num; l++)
        {
            if (qsfp_stat & 0x01)
            {
                sprintf(buf, "%sQSFP %02d low power mode: ON\n", buf, l);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d low power mode: OFF\n", buf, l);
            }
            qsfp_stat = qsfp_stat >>1;
        }
    
    }
    return sprintf(buf, "%s\n", buf);
}

/********************************************************************************/
/*    Function Name      : low_power_all_set                                    */
/*    Description        : This is the function to set all QSFP low power mode  */
/*                         0x32 0x60 0x61                                       */
/*    Input(s)           : 1 or 0.                                              */
/*    Output(s)          : None.                                                */
/*    Returns            : None.                                                */
/********************************************************************************/
static ssize_t low_power_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 value  = 0x0;
    u8 result = 0;
    u8 enable = 0;
    u8 model_type = MODEL_TYPE_UNKNOWN;
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == QSFP_LOW_POWER_ALL)
    {
        enable = simple_strtol(buf, NULL, 10);

        if (enable == TURN_ON)
        {
            value = 0xff;
        }
        else if(enable == TURN_OFF)
        {
            value = 0x0;
        }
        else
        {
            printk(KERN_ALERT "QSFP_LOW_POWER_ALL set wrong value\n");
        }

    #ifdef PREVIOUS_CHECK_TYPE
        model_type = data->model_type;
    #else
        model_type = get_model_type(client, data);
    #endif
    
        if (model_type == MODEL_TYPE_UNKNOWN)
        {   
            printk(KERN_ALERT "ERROR: QSFP_LOW_POWER_ALL type ERR\n");
            return count;
        }
        
        mutex_lock(&data->update_lock);
        result = i2c_smbus_write_byte_data(client, 0x60, value); //to set register 0x32 0x60
        mutex_unlock(&data->update_lock);

        if (model_type == MODEL_TYPE_100G)
        {
            mutex_lock(&data->update_lock);
            result = i2c_smbus_write_byte_data(client, 0x61, value); //to set register 0x32 0x61
            mutex_unlock(&data->update_lock);
        }

        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: QSFP_LOW_POWER_ALL set FAILED!\n");
        }
        else
        {
            debug_print((KERN_DEBUG "QSFP_LOW_POWER_ALL set %d\n", enable));
        }

    }
    return count;
}

/********************************************************************************/
/*    Function Name      : low_power_get                                        */
/*    Description        : This is the function to get QSFP low power mode      */
/*                         0x32 0x60 0x61                                       */
/*    Input(s)           : None.                                                */
/*    Output(s)          : None.                                                */
/*    Returns            : String.                                              */
/********************************************************************************/
static ssize_t low_power_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 ret = -EPERM;
    u8 reg = 0;
    u8 offset = 0;
    u8 model_type = MODEL_TYPE_UNKNOWN;
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

#ifdef PREVIOUS_CHECK_TYPE
    model_type = data->model_type;
#else
    model_type = get_model_type(client, data);
#endif
    
    if (model_type == MODEL_TYPE_UNKNOWN)
        return sprintf(buf, "TYPEERR\n");

#ifndef PREVIOUS_CHECK_TYPE
    if (model_type == MODEL_TYPE_400G)
    {
        if (attr->index > 4)
            return sprintf(buf, "out of range\n");
    }
#endif

    if (attr->index <= 8)
    {
        reg = 0x60;
        offset = attr->index-1;
    }
    else 
    {
        reg = 0x61;
        offset = attr->index-9;
    }
    
    ret = i2c_smbus_read_byte_data(client, reg);
    if (ret < 0)
    {
        return sprintf(buf, "%serr(%d)\n", buf, ret);
    }
    
    ret = (ret>>offset) & 0x1;
    
    return sprintf(buf, "%s%d\n", buf, ret);
}

/********************************************************************************/
/*    Function Name      : low_power_set                                        */
/*    Description        : This is the function to set QSFP low power mode      */
/*                         0x32 0x60 0x61                                       */
/*    Input(s)           : None.                                                */
/*    Output(s)          : None.                                                */
/*    Returns            : None.                                                */
/********************************************************************************/
static ssize_t low_power_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 ret = -EPERM;
    u8 value = 0;
    u8 reg = 0;
    u8 offset = 0;
    u8 input;
    u8 model_type = MODEL_TYPE_UNKNOWN;
    
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
#ifdef PREVIOUS_CHECK_TYPE
    model_type = data->model_type;
#else
    model_type = get_model_type(client, data);
#endif
    
    if (model_type == MODEL_TYPE_UNKNOWN)
    {   
        printk(KERN_ALERT "low_power_set type ERR\n");
        return count;
    }
#ifndef PREVIOUS_CHECK_TYPE
    if (model_type == MODEL_TYPE_400G)
    {
        if (attr->index > 4)
        {
            printk(KERN_ALERT "low_power_set out of range\n");
            return count;
        }
    }
#endif
    
    input = simple_strtol(buf, NULL, 10);
    if(input == 0 || input == 1)
    {
        if (attr->index <= 8)
        {
            reg = 0x60;
            offset = attr->index-1;
        }
        else {
            reg = 0x61;
            offset = attr->index-9;
        }

        if (model_type == MODEL_TYPE_400G && reg == 0x61)
        {   
            printk(KERN_ALERT "low_power_set out of range\n");
            return count;
        }
        
        // read current setting
        ret = i2c_smbus_read_byte_data(client, reg);
        if (ret < 0)
        {
            printk(KERN_ALERT "low_power_set read err(%d)\n", ret);
            return count;
        }
        
        // set new setting
        if (input == 0)
            value = ret & ( ~(1<<offset) );
        else if (input == 1)
            value = ret | (1<<offset);
        
        mutex_lock(&data->update_lock);
        ret = i2c_smbus_write_byte_data(client, reg, value); //to set register 0x32 0x60
        mutex_unlock(&data->update_lock);
        
        if (ret < 0)
        {
            printk(KERN_ALERT "low_power_set write err(%d)\n", ret);
        }
        
    }
    else
    {
        printk(KERN_ALERT "low_power_set wrong value\n");
        return count;
    }
    return count;
}

/********************************************************************************/
/*    Function Name      : qsfp_reset_set                                       */
/*    Description        : This is the function to reset QSFP module            */
/*                         0x32 0x70 0x71                                       */
/*    Input(s)           : None.                          */
/*    Output(s)          : None.                                                */
/*    Returns            : None.                                                */
/********************************************************************************/
static ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 ret = -EPERM;
    u8 value = 0;
    u8 offset = 0;
    u8 reg = 0;
    u8 model_type = MODEL_TYPE_UNKNOWN;
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

#ifdef PREVIOUS_CHECK_TYPE
    model_type = data->model_type;
#else
    model_type = get_model_type(client, data);
#endif
    
    if (model_type == MODEL_TYPE_UNKNOWN)
    {
        printk(KERN_ALERT "qsfp_reset_set type ERR\n");
        return count;
    }
    
#ifndef PREVIOUS_CHECK_TYPE
    if (model_type == MODEL_TYPE_400G)
    {
        if (attr->index > 4)
        {
            printk(KERN_ALERT "qsfp_reset_set out of range\n");
            return count;
        }
    }
#endif
    
    if (attr->index == QSFP_RESET)
    {
        if (attr->index <= 8)
        {
            reg = 0x70;
            offset = attr->index-1;
        }
        else 
        {
            reg = 0x71;
            offset = attr->index-9;
        }
        
        debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set port %03d\n", attr->index));
        
        value = ret & ( ~( 1<<offset ));

        mutex_lock(&data->update_lock);
        i2c_smbus_write_byte_data(client, reg, value); //to set register 0x32 0x70
        mutex_unlock(&data->update_lock);
        debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set_%03d set = %x\n", attr->index, value));
        if (ret < 0)
        {
            printk(KERN_ALERT "qsfp_reset_set write err(%d)\n", ret);
        }
    }

    return count;
}

/********************************************************************************/
/*    Function Name      : QSFP_status_all_get                                  */
/*    Description        : This is the function to get all QSFP insert status   */
/*                         0x32 0x80 0x81                                       */
/*    Input(s)           : None.                                                */
/*    Output(s)          : None.                                                */
/*    Returns            : String.                                              */
/********************************************************************************/
static ssize_t QSFP_status_all_get(struct device *dev, struct device_attribute *da, char *buf)
{

    u32 qsfp_stat = 0;
    u8 res = 0;
    u8 max_port_num = 4;
    u8 model_type = MODEL_TYPE_UNKNOWN;
    int port_num;
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    if (attr->index == QSFP_PRESENT)
    {
        res = i2c_smbus_read_byte_data(client, 0x80); //to get register 0x32 0x80
        qsfp_stat = res;

    #ifdef PREVIOUS_CHECK_TYPE
        model_type = data->model_type;
    #else
        model_type = get_model_type(client, data);
    #endif
    
        if (model_type == MODEL_TYPE_UNKNOWN)
            return sprintf(buf, "TYPEERR\n");
        
        if (model_type == MODEL_TYPE_100G)
        {
            max_port_num = 16;
            qsfp_stat = i2c_smbus_read_byte_data(client, 0x81); //to get register 0x32 0x81
            qsfp_stat = (qsfp_stat<<8) | res;
        }

        debug_print((KERN_DEBUG "DEBUG : QSFP_status_all_get status = %x\n",qsfp_stat));

        for (port_num = 1; port_num <= max_port_num; port_num++)
        {
            if (qsfp_stat & 0x01)
            {
                sprintf(buf, "%sQSFP %02d is not present\n", buf, port_num);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d is present\n", buf, port_num);
            }
            qsfp_stat = qsfp_stat >> 1;
        }
    
    }
    return sprintf(buf, "%s\n", buf);

}

/********************************************************************************/
/*    Function Name      : qsfp_status_get                                      */
/*    Description        : This is the function to get QSFP insert status       */
/*                         0x32 0x80 0x81                                       */
/*    Input(s)           : attr->index.                                         */
/*    Output(s)          : None.                                                */
/*    Returns            : String.                                              */
/********************************************************************************/
static ssize_t qsfp_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 ret = -EPERM;
    u8 reg = 0;
    u8 offset = 0;
    u8 model_type = MODEL_TYPE_UNKNOWN;
    struct i2c_client *client = to_i2c_client(dev);
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

#ifdef PREVIOUS_CHECK_TYPE
    model_type = data->model_type;
#else
    model_type = get_model_type(client, data);
#endif

    if (model_type == MODEL_TYPE_UNKNOWN)
        return sprintf(buf, "TYPEERR\n");

#ifndef PREVIOUS_CHECK_TYPE
    if (model_type == MODEL_TYPE_400G)
    {
        if (attr->index > 4)
            return sprintf(buf, "out of range\n");
    }
#endif

    if (attr->index <= 8)
    {
        reg = 0x80;
        offset = attr->index-1;
    }
    else 
    {
        reg = 0x81;
        offset = attr->index-9;
    }
    
    ret = i2c_smbus_read_byte_data(client, reg);
    if (ret < 0)
    {
        return sprintf(buf, "%serr(%d)\n", buf, ret);
    }
    
    ret = (ret>>offset) & 0x1;
    
    return sprintf(buf, "%s%d\n", buf, ((ret)?0:1));
    
}

/* end of function */
/********************************************************************************/
/*    Function Name      : Cameo_i2c_probe                                      */
/*    Description        : To probe i2c device                                  */
/*                                                                              */
/*    Input(s)           : None.                                                */
/*    Output(s)          : None.                                                */
/*    Returns            : None.                                                */
/********************************************************************************/
static int phy_cpld640_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{

    struct Cameo_i2c_data *data;
#ifdef PREVIOUS_CHECK_TYPE    
    u8 card_model;
#endif

    int status;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
    {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!data)
    {
        printk(KERN_ALERT "Cameo_PHY_CPLD_data kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);

    mutex_init(&data->update_lock);
    data->valid = 0;
    dev_info(&client->dev, "chip found\n");

#ifdef PREVIOUS_CHECK_TYPE	
	/* check model type */
	mutex_lock(&data->update_lock);
    card_model = i2c_smbus_read_byte_data(client, 0xb0);
    mutex_unlock(&data->update_lock);
    if (card_model < 0)
    {
        status = -EPERM;
        goto exit_free;
    }

    switch (card_model)
    {
    case 0x00:
    case 0x10:
        data->model_type = MODEL_TYPE_100G;
        break;
    case 0x01:
    case 0x11:
        data->model_type = MODEL_TYPE_400G;
        break;
    default:
        return -EPERM;
    }

#endif

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &phy_cpld640_QSFP_group);
    if (status)
    {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev))
    {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }
    dev_info(&client->dev, "%s: '%s'\n", dev_name(data->hwmon_dev), client->name);

    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &phy_cpld640_QSFP_group);

exit_free:
    kfree(data);
exit:
    return status;
}

static int phy_cpld640_remove(struct i2c_client *client)
{
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &phy_cpld640_QSFP_group);
    kfree(data);
    return 0;
}

static const struct i2c_device_id phy_cpld640_id[] =
{
    { "phy_cpld640", 0 },
    {},
};
MODULE_DEVICE_TABLE(i2c, phy_cpld640_id);

static struct i2c_driver phy_cpld640_driver =
{
    .class        = I2C_CLASS_HWMON,
    .driver =
    {
        .name     = "phy_cpld640",
    },
    .probe        = phy_cpld640_probe,
    .remove       = phy_cpld640_remove,
    .id_table     = phy_cpld640_id,
    .address_list = normal_i2c,
};


module_i2c_driver(phy_cpld640_driver)
MODULE_AUTHOR("Cameo Inc.");
MODULE_DESCRIPTION("Cameo phy_cpld640 i2c driver");
MODULE_LICENSE("GPL");
