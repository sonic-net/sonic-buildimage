/*
 * An hwmon driver for the Accton Redundant Power Module
 *
 */

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

#define DRIVER_DESCRIPTION_NAME "accton i2c psu driver"
/* PMBus Protocol. */
#define PMBUS_LITERAL_DATA_MULTIPLIER           1000
#define PMBUS_REGISTER_VOUT_MODE                0x20
#define PMBUS_REGISTER_STATUS_BYTE              0x78
#define PMBUS_REGISTER_STATUS_WORD              0x79
#define PMBUS_REGISTER_STATUS_FAN               0x81
#define PMBUS_REGISTER_READ_VIN                 0x88
#define PMBUS_REGISTER_READ_IIN                 0x89
#define PMBUS_REGISTER_READ_VOUT                0x8B
#define PMBUS_REGISTER_READ_IOUT                0x8C
#define PMBUS_REGISTER_READ_TEMPERATURE_1       0x8D
#define PMBUS_REGISTER_READ_TEMPERATURE_2       0x8E
#define PMBUS_REGISTER_READ_TEMPERATURE_3       0x8F
#define PMBUS_REGISTER_READ_FAN_SPEED_1         0x90
#define PMBUS_REGISTER_READ_FAN_SPEED_2         0x91

#define PMBUS_REGISTER_READ_FAN_CONFIG_1        0x3A
#define PMBUS_REGISTER_FAN_COMMAND_1            0x3B

#define PMBUS_REGISTER_READ_POUT                0x96
#define PMBUS_REGISTER_READ_PIN                 0x97
#define PMBUS_REGISTER_MFR_ID                   0x99
#define PMBUS_REGISTER_MFR_MODEL                0x9A
#define PMBUS_REGISTER_MFR_REVISION             0x9B
#define PMBUS_REGISTER_MFR_SERIAL               0x9E


#define MAX_FAN_DUTY_CYCLE      100
#define I2C_RW_RETRY_COUNT		10
#define I2C_RW_RETRY_INTERVAL	60 /* ms */

/* Addresses scanned 
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

/* Each client has this additional data 
 */
struct accton_i2c_psu_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    u8   vout_mode;     /* Register value */
    u16  v_in;          /* Register value */
    u16  v_out;         /* Register value */
    u16  i_in;          /* Register value */
    u16  i_out;         /* Register value */
    u16  p_in;          /* Register value */
    u16  p_out;         /* Register value */
    u16  temp_input[2]; /* Register value */
    u8   fan_fault;     /* Register value */
    u16  fan_duty_cycle[2];  /* Register value */
    u16  fan_speed[2];  /* Register value */
    u8   pmbus_revision; /* Register value */
    u8   mfr_id[10];	 /* Register value */
	u8   mfr_model[16]; /* Register value */
	u8   mfr_revsion[3]; /* Register value */
	u8   mfr_serial[26]; /* Register value */
};

static ssize_t show_linear(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_fan_fault(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_vout(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_fan_duty_cycle(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t show_ascii(struct device *dev, struct device_attribute *da,
			 char *buf);
static ssize_t show_byte(struct device *dev, struct device_attribute *da,
			 char *buf);
			 			 
static int accton_i2c_psu_write_word(struct i2c_client *client, u8 reg, u16 value);
static struct accton_i2c_psu_data *accton_i2c_psu_update_device(struct device *dev);

enum accton_i2c_psu_sysfs_attributes {
    PSU_V_IN,
    PSU_V_OUT,
    PSU_I_IN,
    PSU_I_OUT,
    PSU_P_IN,
    PSU_P_OUT_UV,
    PSU_P_OUT,
    PSU_TEMP1_INPUT,
    PSU_FAN1_FAULT,
    PSU_FAN1_DUTY_CYCLE,
    PSU_FAN1_SPEED,
    PSU_PMBUS_REVISION,
	PSU_MFR_ID,
	PSU_MFR_MODEL,
	PSU_MFR_REVISION,
	PSU_MFR_SERIAL,
};

/* sysfs attributes for hwmon 
 */
static SENSOR_DEVICE_ATTR(psu_v_in,        S_IRUGO, show_linear,      NULL, PSU_V_IN);
static SENSOR_DEVICE_ATTR(psu_v_out,       S_IRUGO, show_vout,        NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(psu_i_in,        S_IRUGO, show_linear,      NULL, PSU_I_IN);
static SENSOR_DEVICE_ATTR(psu_i_out,       S_IRUGO, show_linear,      NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(psu_p_in,        S_IRUGO, show_linear,      NULL, PSU_P_IN);
static SENSOR_DEVICE_ATTR(psu_p_out,       S_IRUGO, show_linear,      NULL, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(psu_temp1_input, S_IRUGO, show_linear,      NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(psu_fan1_fault,  S_IRUGO, show_fan_fault,   NULL, PSU_FAN1_FAULT);
static SENSOR_DEVICE_ATTR(psu_fan1_duty_cycle_percentage, S_IWUSR | S_IRUGO, show_linear, set_fan_duty_cycle, PSU_FAN1_DUTY_CYCLE);
static SENSOR_DEVICE_ATTR(psu_fan1_speed_rpm, S_IRUGO, show_linear,   NULL, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(psu_pmbus_revision,S_IRUGO, show_byte,   NULL, PSU_PMBUS_REVISION);
static SENSOR_DEVICE_ATTR(psu_mfr_id,		S_IRUGO, show_ascii,  NULL, PSU_MFR_ID);
static SENSOR_DEVICE_ATTR(psu_mfr_model,	S_IRUGO, show_ascii,  NULL, PSU_MFR_MODEL);
static SENSOR_DEVICE_ATTR(psu_mfr_revision,	S_IRUGO, show_ascii, NULL, PSU_MFR_REVISION);
static SENSOR_DEVICE_ATTR(psu_mfr_serial,	S_IRUGO, show_ascii, NULL, PSU_MFR_SERIAL);


/*Duplicate nodes for lm-sensors.*/
static SENSOR_DEVICE_ATTR(in3_input, S_IRUGO, show_vout,    NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(curr2_input, S_IRUGO, show_linear,    NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, show_linear,    NULL, PSU_P_OUT_UV);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_linear,    NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_linear, NULL, PSU_FAN1_SPEED);
//static SENSOR_DEVICE_ATTR(temp1_fault,  S_IRUGO, show_word,      NULL, PSU_TEMP_FAULT);


static struct attribute *accton_i2c_psu_attributes[] = {
    &sensor_dev_attr_psu_v_in.dev_attr.attr,
    &sensor_dev_attr_psu_v_out.dev_attr.attr,
    &sensor_dev_attr_psu_i_in.dev_attr.attr,
    &sensor_dev_attr_psu_i_out.dev_attr.attr,
    &sensor_dev_attr_psu_p_in.dev_attr.attr,
    &sensor_dev_attr_psu_p_out.dev_attr.attr,
    &sensor_dev_attr_psu_temp1_input.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_fault.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_duty_cycle_percentage.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_psu_pmbus_revision.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_id.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_revision.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_serial.dev_attr.attr,
     /*Duplicate nodes for lm-sensors.*/
    &sensor_dev_attr_curr2_input.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_power2_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    //&sensor_dev_attr_temp1_fault.dev_attr.attr,
    NULL
};

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static ssize_t set_fan_duty_cycle(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct accton_i2c_psu_data *data = i2c_get_clientdata(client);
    int nr = (attr->index == PSU_FAN1_DUTY_CYCLE) ? 0 : 1;
	long speed;
	int error;

	error = kstrtol(buf, 10, &speed);
	if (error)
		return error;

    if (speed < 0 || speed > MAX_FAN_DUTY_CYCLE)
        return -EINVAL;

    mutex_lock(&data->update_lock);
    data->fan_duty_cycle[nr] = speed;
    accton_i2c_psu_write_word(client, PMBUS_REGISTER_FAN_COMMAND_1 + nr, data->fan_duty_cycle[nr]);
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t show_linear(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct accton_i2c_psu_data *data = accton_i2c_psu_update_device(dev);

    u16 value = 0;
    int exponent, mantissa;
    int multiplier = 0;
    
    switch (attr->index) {
    case PSU_V_IN:
        value = data->v_in;
        break;
    case PSU_I_IN:
        value = data->i_in;
        break;
    case PSU_I_OUT:
        value = data->i_out;
        break;
    case PSU_P_IN:
        value = data->p_in;
        break;
    case PSU_P_OUT_UV:
        multiplier=1;
    case PSU_P_OUT:
        value = data->p_out;
        break;
    case PSU_TEMP1_INPUT:
        value = data->temp_input[0];
        break;
    case PSU_FAN1_DUTY_CYCLE:
        multiplier = 1;
        value = data->fan_duty_cycle[0];
        break;
    case PSU_FAN1_SPEED:
        multiplier = 1;
        value = data->fan_speed[0];
        break;
    default:
        break;
    }
    
    exponent = two_complement_to_int(value >> 11, 5, 0x1f);
    mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);

    if(!multiplier)
        multiplier = PMBUS_LITERAL_DATA_MULTIPLIER;
    if(attr->index==PSU_P_OUT_UV)
        multiplier = 1000000;
    
    return (exponent >= 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * multiplier) :
                             sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));                      
}

static ssize_t show_fan_fault(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct accton_i2c_psu_data *data = accton_i2c_psu_update_device(dev);

    u8 shift = (attr->index == PSU_FAN1_FAULT) ? 7 : 6;

    return sprintf(buf, "%d\n", data->fan_fault >> shift);
}

static ssize_t show_vout(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct accton_i2c_psu_data *data = accton_i2c_psu_update_device(dev);
    int exponent, mantissa;    

    exponent = two_complement_to_int(data->vout_mode, 5, 0x1f);
    mantissa = data->v_out;

    return (exponent > 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * PMBUS_LITERAL_DATA_MULTIPLIER) :
                            sprintf(buf, "%d\n", (mantissa * PMBUS_LITERAL_DATA_MULTIPLIER) / (1 << -exponent));
}

static ssize_t show_byte(struct device *dev, struct device_attribute *da,
			 char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct accton_i2c_psu_data *data = accton_i2c_psu_update_device(dev);
	
	if (!data->valid) {
		return 0;
	}

	return (attr->index == PSU_PMBUS_REVISION) ? sprintf(buf, "%d\n", data->pmbus_revision) :
								 sprintf(buf, "0\n");
}

static ssize_t show_ascii(struct device *dev, struct device_attribute *da,
			 char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct accton_i2c_psu_data *data = accton_i2c_psu_update_device(dev);
	u8 *ptr = NULL;

	if (!data->valid) {
		return 0;
	}	
	switch (attr->index) {

	case PSU_MFR_ID:
			ptr = data->mfr_id;
		break;
	case PSU_MFR_MODEL:
			ptr = data->mfr_model;
		break;
	case PSU_MFR_REVISION:
			ptr = data->mfr_revsion;
		break;
	case PSU_MFR_SERIAL:
		ptr = data->mfr_serial;
		break;
	default:
		return 0;
	}

	return sprintf(buf, "%s\n", ptr);
}


static const struct attribute_group accton_i2c_psu_group = {
    .attrs = accton_i2c_psu_attributes,
};

static int accton_i2c_psu_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct accton_i2c_psu_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, 
        I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct accton_i2c_psu_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &accton_i2c_psu_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: psu '%s'\n",
         dev_name(data->hwmon_dev), client->name);
    
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &accton_i2c_psu_group);
exit_free:
    kfree(data);
exit:
    
    return status;
}

static void accton_i2c_psu_remove(struct i2c_client *client)
{
    struct accton_i2c_psu_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &accton_i2c_psu_group);
    kfree(data);
    
}
/* Support psu moduel
 */
static const struct i2c_device_id accton_i2c_psu_id[] = {
    { "acbel_fsh082", 0 },
    { "yesm1300am", 1 },
    {}
};
MODULE_DEVICE_TABLE(i2c, accton_i2c_psu_id);

static struct i2c_driver accton_i2c_psu_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "accton_i2c_psu",
    },
    .probe        = accton_i2c_psu_probe,
    .remove       = accton_i2c_psu_remove,
    .id_table     = accton_i2c_psu_id,
    .address_list = normal_i2c,
};

static int accton_i2c_psu_read_byte(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}

static int accton_i2c_psu_read_word(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_word_data(client, reg);
}

static int accton_i2c_psu_write_word(struct i2c_client *client, u8 reg, u16 value)
{
    return i2c_smbus_write_word_data(client, reg, value);
}

static int accton_i2c_psu_read_block(struct i2c_client *client, u8 command, u8 *data,
			  int data_len)
{
	int status = 0, retry = I2C_RW_RETRY_COUNT;

	while (retry) {
		status = i2c_smbus_read_i2c_block_data(client, command, data_len, data);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

    return status;
}


static int accton_i2c_psu_read_block_data(struct i2c_client *client, u8 command, u8 *data, int data_length)
{
    int status = -EIO;
    int length;
    u8 buffer[128] = {0}, *ptr = buffer;

    status = accton_i2c_psu_read_byte(client, command);
    if (status < 0)
    {
        dev_dbg(&client->dev, "Unable to get data from offset 0x%02X\r\n", command);
        status = -EIO;
        goto EXIT_READ_BLOCK_DATA;
    }
  
    status = (status & 0xFF) + 1;
    if ( status > 128)
    {
        dev_dbg(&client->dev, "Unable to get big data from offset 0x%02X\r\n", command);
        status = -EINVAL;
        goto EXIT_READ_BLOCK_DATA;
    }
    
    length = status;
    status = accton_i2c_psu_read_block(client, command, buffer, length);
    if (unlikely(status < 0))
        goto EXIT_READ_BLOCK_DATA;
    if (unlikely(status != length)) {
        status = -EIO;
        goto EXIT_READ_BLOCK_DATA;
    } 
    /* The first byte is the count byte of string. */   
    ptr++;
    status--;

    length=status>(data_length-1)?(data_length-1):status;
    memcpy(data, ptr, length);
    data[length-1] = 0;

EXIT_READ_BLOCK_DATA:
    
    return status;
}


struct reg_data_byte {
    u8   reg;
    u8  *value;
};

struct reg_data_word {
    u8   reg;
    u16 *value;
};

static struct accton_i2c_psu_data *accton_i2c_psu_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct accton_i2c_psu_data *data = i2c_get_clientdata(client);
    
    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
        || !data->valid) {
        int i, status;
        //u8 command, buf;
        struct reg_data_byte regs_byte[] = { {PMBUS_REGISTER_VOUT_MODE, &data->vout_mode},
                                             {PMBUS_REGISTER_STATUS_FAN, &data->fan_fault}};
        struct reg_data_word regs_word[] = { {PMBUS_REGISTER_READ_VIN, &data->v_in},
                                             {PMBUS_REGISTER_READ_VOUT, &data->v_out},
                                             {PMBUS_REGISTER_READ_IIN, &data->i_in},
                                             {PMBUS_REGISTER_READ_IOUT, &data->i_out},
                                             {PMBUS_REGISTER_READ_POUT, &data->p_out},
                                             {PMBUS_REGISTER_READ_PIN, &data->p_in},
                                             {PMBUS_REGISTER_READ_TEMPERATURE_1, &(data->temp_input[0])},
                                             {PMBUS_REGISTER_READ_TEMPERATURE_2, &(data->temp_input[1])},
                                             {PMBUS_REGISTER_FAN_COMMAND_1, &(data->fan_duty_cycle[0])},
                                             {PMBUS_REGISTER_READ_FAN_SPEED_1, &(data->fan_speed[0])},
                                             {PMBUS_REGISTER_READ_FAN_SPEED_2, &(data->fan_speed[1])},
                                             };

        dev_dbg(&client->dev, "Starting accton_i2c_psu update\n");

        /* Read byte data */        
        for (i = 0; i < ARRAY_SIZE(regs_byte); i++) {
            status = accton_i2c_psu_read_byte(client, regs_byte[i].reg);
            
            if (status < 0) {
                dev_dbg(&client->dev, "reg %d, err %d\n",
                        regs_byte[i].reg, status);
            }
            else {
                *(regs_byte[i].value) = status;
            }
        }
                    
        /* Read word data */                    
        for (i = 0; i < ARRAY_SIZE(regs_word); i++) {
            status = accton_i2c_psu_read_word(client, regs_word[i].reg);
            
            if (status < 0) {
                dev_dbg(&client->dev, "reg %d, err %d\n",
                        regs_word[i].reg, status);
            }
            else {
                *(regs_word[i].value) = status;
            }
            
        }
        /* Read mfr_id */
		status = accton_i2c_psu_read_block_data(client, PMBUS_REGISTER_MFR_ID, data->mfr_id,
										 ARRAY_SIZE(data->mfr_id));
		if (status < 0) {
			dev_dbg(&client->dev, "reg %d, err %d\n", PMBUS_REGISTER_MFR_ID, status);
			goto exit;
		}		
		/* Read mfr_model */		
		status = accton_i2c_psu_read_block_data(client, PMBUS_REGISTER_MFR_MODEL, data->mfr_model,
										 ARRAY_SIZE(data->mfr_model));
		if (status < 0) {
			dev_dbg(&client->dev, "reg %d, err %d\n", PMBUS_REGISTER_MFR_MODEL, status);
			goto exit;
		}
        /* Read mfr_revsion */		
		status = accton_i2c_psu_read_block_data(client, PMBUS_REGISTER_MFR_REVISION, data->mfr_revsion,
										 ARRAY_SIZE(data->mfr_revsion));
		if (status < 0) {
			dev_dbg(&client->dev, "reg %d, err %d\n", PMBUS_REGISTER_MFR_REVISION, status);
			goto exit;
		}
		/* Read mfr_serial */
		status = accton_i2c_psu_read_block_data(client, PMBUS_REGISTER_MFR_SERIAL, data->mfr_serial,
										 ARRAY_SIZE(data->mfr_serial));
		if (status < 0) {
			dev_dbg(&client->dev, "reg %d, err %d\n", PMBUS_REGISTER_MFR_SERIAL, status);
			goto exit;
		}
        
        data->last_updated = jiffies;
        data->valid = 1;
    }

exit:
    mutex_unlock(&data->update_lock);

    return data;
}

static int __init accton_i2c_psu_init(void)
{
    return i2c_add_driver(&accton_i2c_psu_driver);
}

static void __exit accton_i2c_psu_exit(void)
{
    i2c_del_driver(&accton_i2c_psu_driver);
}

MODULE_AUTHOR("Jostar Yang <jostar_yang@accton.com.tw>");
MODULE_DESCRIPTION(DRIVER_DESCRIPTION_NAME);
MODULE_LICENSE("GPL");

module_init(accton_i2c_psu_init);
module_exit(accton_i2c_psu_exit);
