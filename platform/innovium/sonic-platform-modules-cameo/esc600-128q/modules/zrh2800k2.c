

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>

#define DEBUG_MSG
#ifdef DEBUG_MSG
    #define debug_print(s) printk s
#else
    #define debug_print(s)
#endif

//#define SYMBOL_FOR_LM_SENSOR

enum power_modules{ zrh2800k2, zrh2ab0k2 };

struct zrh2800k2_data {
    struct device   *hwmon_dev;
    struct mutex    update_lock;
    char            valid;          /* !=0 if registers are valid */
    unsigned long   last_updated;   /* In jiffies */
    u64             last_energy_value_EIN;
    u64             last_smaple_count_EIN;
    u64             last_energy_value_EOUT;
    u64             last_smaple_count_EOUT;        
};

typedef enum _access_type_{
    READ_BYTE,
    READ_WORD,
    READ_BLOCK
}ACCESS_TYPE;

typedef enum _value_format_{
    FORMAT_NORMAL,
    FORMAT_DIRECT,
    FORMAT_LINEAR
}VALUE_FORMAT;

typedef enum _zrh2800k2_regs_ {
    REG_CAPABILITY          = 0x19,
    REG_QUERY               = 0X1A,
    REG_VOUT_MODE           = 0x20,
    REG_COEFFICIENTS        = 0X30,
    REG_FAN_CONFIG_1_2      = 0x3A,
    REG_STATUS_WORD         = 0x79,
    REG_STATUS_VOUT         = 0x7A,
    REG_STATUS_IOUT         = 0x7B,
    REG_STATUS_INPUT        = 0x7C,
    REG_STATUS_TEMPERATURE  = 0x7D,
    REG_STATUS_FANS_1_2     = 0x81,
    REG_READ_EIN            = 0x86, /* direct data format */
    REG_READ_EOUT           = 0x87, /* direct data format */
    REG_READ_VIN            = 0x88,
    REG_READ_IIN            = 0x89,
    REG_READ_VOUT           = 0x8B, /* linear data format */
    REG_READ_IOUT           = 0x8C, /* linear data format */
    REG_READ_TEMPERATURE1   = 0x8D, /* linear data format */
    REG_READ_FAN_SPEED_1    = 0x90, /* linear data format */
    REG_READ_POUT           = 0x96, /* linear data format */
    REG_READ_PIN            = 0x97, /* linear data format */
    REG_READ_PMBUS_REVISION = 0x98,
    REG_READ_MFR_ID         = 0x99, /* ZIPPY 5 BYTES */
    REG_READ_MFR_MODEL      = 0x9A,
    REG_READ_MFR_VIN_MAX    = 0xA4, /* linear data format */
    REG_READ_MFR_IOUT_MAX   = 0xA6  /* lineat data format */
}ZRH2800K2_REGS;

enum zrh2800k2_sysfs_attributes {
    PSU_CAPABILITY,             /* 0 */
    PSU_QUERY,
    PSU_VOUT_MODE,
    PSU_COEFFICIENTS,
    PSU_FAN_CONFIG_1_2,
    PSU_STATUS_WORD,            /* 5 */
    PSU_STATUS_VOUT,
    PSU_STATUS_IOUT,            
    PSU_STATUS_INPUT,
    PSU_STATUS_TEMPERATURE,
    PSU_STATUS_FANS_1_2,        /* 10 */
    PSU_EIN,
    PSU_EOUT,                   
    PSU_VIN,
    PSU_IIN,
    PSU_VOUT,                   /* 15 */
    PSU_IOUT,
    PSU_TEMPERATURE_1,
    PSU_FAN_SPEED_1,
    PSU_POUT,
    PSU_PIN,                    /* 20 */
    PSU_PMBUS_REVISION,
    PSU_MFR_ID,
    PSU_MFR_MODEL,
    PSU_MFR_VIN_MAX,
    PSU_MFR_IOUT_MAX
};

struct _OPERATION_SET_ {
    ZRH2800K2_REGS  reg;
    ACCESS_TYPE     type;
    u8 data_size; // unit: byte, only used for block read
};

/* the index of operations are mapping to the zrh2800k2_sysfs_attributes */
static struct _OPERATION_SET_ operation_set[] = {
    { REG_CAPABILITY,       READ_BYTE,  1 }, // 0
    { REG_QUERY,            READ_BYTE,  1}, // 1
    { REG_VOUT_MODE,        READ_BYTE,  1 }, // 2
    { REG_COEFFICIENTS,     READ_BLOCK, 5 }, // 3
    { REG_FAN_CONFIG_1_2 ,  READ_BYTE,  1 }, // 4
    { REG_STATUS_WORD,      READ_WORD,  2 }, // 5
    { REG_STATUS_VOUT,      READ_BYTE,  1 }, // 6
    { REG_STATUS_IOUT,      READ_BYTE,  1 }, // 7
    { REG_STATUS_INPUT,     READ_BYTE,  1 }, // 8
    { REG_STATUS_TEMPERATURE, READ_BYTE, 1 }, //9
    { REG_STATUS_FANS_1_2,  READ_BYTE,  1 }, // 10
    { REG_READ_EIN,         READ_BLOCK, 6 }, // 11
    { REG_READ_EOUT,        READ_BLOCK, 6 }, // 12
    { REG_READ_VIN,         READ_WORD,  2 }, // 13
    { REG_READ_IIN,         READ_WORD,  2 }, // 14
    { REG_READ_VOUT,        READ_WORD,  2 }, // 15
    { REG_READ_IOUT,        READ_WORD,  2 }, // 16
    { REG_READ_TEMPERATURE1, READ_WORD, 2 }, // 17
    { REG_READ_FAN_SPEED_1, READ_WORD,  2 }, // 18
    { REG_READ_POUT,        READ_WORD,  2 }, // 19
    { REG_READ_PIN,         READ_WORD,  2 }, // 20
    { REG_READ_PMBUS_REVISION, READ_BYTE, 1 }, //21
    { REG_READ_MFR_ID,      READ_BLOCK, 5 }, // 22
    { REG_READ_MFR_MODEL,   READ_BLOCK, 9 }, // 23
    { REG_READ_MFR_VIN_MAX, READ_WORD,  2 }, // 24
    { REG_READ_MFR_IOUT_MAX, READ_WORD, 2 }  // 25
};


static int zrh2800k2_remove(struct i2c_client *client);
static int zrh2800k2_probe(struct i2c_client *client, const struct i2c_device_id *dev_id);
static ssize_t show_value(struct device *dev, struct device_attribute *da, char *buf);

static ssize_t show_capability(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t psu_pm_query(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t psu_coefficient(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_fan_config(struct device *dev, struct device_attribute *da, const char *buf, size_t count);


/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x58, 0x59, I2C_CLIENT_END };

/* sysfs attributes for hwmon */
static SENSOR_DEVICE_ATTR(psu_query, S_IRUGO, psu_pm_query, NULL, PSU_QUERY);
static SENSOR_DEVICE_ATTR(psu_coeff, S_IRUGO, psu_coefficient,NULL, PSU_COEFFICIENTS);
static SENSOR_DEVICE_ATTR(psu_fan_config_1_2, S_IRUGO|S_IWUSR, show_value, set_fan_config, PSU_FAN_CONFIG_1_2);
static SENSOR_DEVICE_ATTR(psu_capability,   S_IRUGO, show_capability, NULL, PSU_CAPABILITY);
static SENSOR_DEVICE_ATTR(psu_vout_mode,    S_IRUGO, show_value, NULL, PSU_VOUT_MODE);
static SENSOR_DEVICE_ATTR(psu_status_word,  S_IRUGO, show_value, NULL, PSU_STATUS_WORD);
static SENSOR_DEVICE_ATTR(psu_status_vout,  S_IRUGO, show_value, NULL, PSU_STATUS_VOUT);
static SENSOR_DEVICE_ATTR(psu_status_iout,  S_IRUGO, show_value, NULL, PSU_STATUS_IOUT);
static SENSOR_DEVICE_ATTR(psu_status_input, S_IRUGO, show_value, NULL, PSU_STATUS_INPUT);
static SENSOR_DEVICE_ATTR(psu_status_temp,  S_IRUGO, show_value, NULL, PSU_STATUS_TEMPERATURE);
static SENSOR_DEVICE_ATTR(psu_status_fan_1_2, S_IRUGO, show_value, NULL, PSU_STATUS_FANS_1_2);
static SENSOR_DEVICE_ATTR(psu_ein, S_IRUGO, show_value, NULL, PSU_EIN);
static SENSOR_DEVICE_ATTR(psu_eout, S_IRUGO, show_value, NULL, PSU_EOUT);
static SENSOR_DEVICE_ATTR(psu_pmbus_rev,S_IRUGO, show_value, NULL, PSU_PMBUS_REVISION);
static SENSOR_DEVICE_ATTR(psu_mfr_id,   S_IRUGO, show_value, NULL, PSU_MFR_ID);
static SENSOR_DEVICE_ATTR(psu_mfr_model,S_IRUGO, show_value, NULL, PSU_MFR_MODEL);

static SENSOR_DEVICE_ATTR(psu_vin, S_IRUGO, show_value, NULL, PSU_VIN);
static SENSOR_DEVICE_ATTR(psu_vout, S_IRUGO, show_value, NULL, PSU_VOUT);
static SENSOR_DEVICE_ATTR(psu_iin, S_IRUGO, show_value, NULL, PSU_IIN);
static SENSOR_DEVICE_ATTR(psu_iout, S_IRUGO, show_value, NULL, PSU_IOUT);
static SENSOR_DEVICE_ATTR(psu_iout_max, S_IRUGO, show_value, NULL, PSU_MFR_IOUT_MAX);
static SENSOR_DEVICE_ATTR(psu_pin, S_IRUGO, show_value, NULL, PSU_PIN);
static SENSOR_DEVICE_ATTR(psu_pout, S_IRUGO, show_value, NULL, PSU_POUT);

static SENSOR_DEVICE_ATTR(psu_temp_1, S_IRUGO, show_value, NULL, PSU_TEMPERATURE_1);
static SENSOR_DEVICE_ATTR(psu_fan_speed_1,  S_IRUGO, show_value, NULL, PSU_FAN_SPEED_1);

/* section for lm-sensor */
#ifdef SYMBOL_FOR_LM_SENSOR
static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, show_value, NULL, PSU_VIN);
// static SENSOR_DEVICE_ATTR(in1_max, S_IRUGO, show_value, NULL, PSU_MFR_VIN_MAX);   -> not support
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, show_value, NULL, PSU_VOUT);
static SENSOR_DEVICE_ATTR(curr1_input, S_IRUGO, show_value, NULL, PSU_IIN);
static SENSOR_DEVICE_ATTR(curr2_input, S_IRUGO, show_value, NULL, PSU_IOUT);
static SENSOR_DEVICE_ATTR(curr2_max, S_IRUGO, show_value, NULL, PSU_MFR_IOUT_MAX);
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, show_value, NULL, PSU_PIN);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, show_value, NULL, PSU_POUT);

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_value, NULL, PSU_TEMPERATURE_1);
static SENSOR_DEVICE_ATTR(fan1_input,  S_IRUGO, show_value, NULL, PSU_FAN_SPEED_1);
#endif

static struct attribute *zrh2800k2_attributes[] = {
    &sensor_dev_attr_psu_query.dev_attr.attr,
    &sensor_dev_attr_psu_coeff.dev_attr.attr,
    &sensor_dev_attr_psu_fan_config_1_2.dev_attr.attr,
    &sensor_dev_attr_psu_capability.dev_attr.attr,
    &sensor_dev_attr_psu_vout_mode.dev_attr.attr,
    &sensor_dev_attr_psu_status_word.dev_attr.attr,
    &sensor_dev_attr_psu_status_vout.dev_attr.attr,
    &sensor_dev_attr_psu_status_iout.dev_attr.attr,
    &sensor_dev_attr_psu_status_input.dev_attr.attr,
    &sensor_dev_attr_psu_status_temp.dev_attr.attr,
    &sensor_dev_attr_psu_status_fan_1_2.dev_attr.attr,
    &sensor_dev_attr_psu_ein.dev_attr.attr,
    &sensor_dev_attr_psu_eout.dev_attr.attr,
    &sensor_dev_attr_psu_pmbus_rev.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_id.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu_vin.dev_attr.attr,
    &sensor_dev_attr_psu_vout.dev_attr.attr,
    &sensor_dev_attr_psu_iin.dev_attr.attr,
    &sensor_dev_attr_psu_iout.dev_attr.attr,
    &sensor_dev_attr_psu_iout_max.dev_attr.attr,
    &sensor_dev_attr_psu_pin.dev_attr.attr,
    &sensor_dev_attr_psu_pout.dev_attr.attr,
    &sensor_dev_attr_psu_temp_1.dev_attr.attr,
    &sensor_dev_attr_psu_fan_speed_1.dev_attr.attr,
#ifdef SYMBOL_FOR_LM_SENSOR
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_curr1_input.dev_attr.attr,
    &sensor_dev_attr_curr2_input.dev_attr.attr,
    &sensor_dev_attr_curr2_max.dev_attr.attr,
    &sensor_dev_attr_power1_input.dev_attr.attr,
    &sensor_dev_attr_power2_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
#endif    
    NULL
};

static const struct attribute_group zrh2800k2_group = {
        .attrs = zrh2800k2_attributes,
};

static u32 easy_pow(u32 num, u32 power)
{
    if(power == 0) 
        return 1;

    power--;

    while(power) {
        num = num*num;
        power--;
    }
    return num;
}

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}


static int zrh2800k2_read(struct device *dev, ACCESS_TYPE rtype , ZRH2800K2_REGS reg)
{

    struct i2c_client *client = to_i2c_client(dev);
    struct zrh2800k2_data *data = i2c_get_clientdata(client);

    int result;

    mutex_lock(&data->update_lock);
    
    if (rtype == READ_BYTE) {
        result = i2c_smbus_read_byte_data(client, (u8)reg);
    }else if(rtype == READ_WORD) {
        result = i2c_smbus_read_word_data(client, (u8)reg);
    }else{
        printk(KERN_ALERT "ERROR: unknown read type");
    }
    
    mutex_unlock(&data->update_lock);

    return result;   

}


static int zrh2800k2_read_block(struct device *dev, ZRH2800K2_REGS reg, u8* block_data, int block_data_len)
{

    struct i2c_client *client = to_i2c_client(dev);
    struct zrh2800k2_data *data = i2c_get_clientdata(client);

    int result;
    
    mutex_lock(&data->update_lock);

    result = i2c_smbus_read_i2c_block_data(client, (u8)reg, block_data_len, block_data);

    mutex_unlock(&data->update_lock);

    if (unlikely(result < 0)) {
        goto read_block_exit;
    }

    if (result != block_data_len) {
        result = -EIO;
        goto read_block_exit;
    }

    result = 0;

read_block_exit:
    return result;

}

static int get_coefficient(struct device* dev, u16* m, u16* b, u8* R)
{
    u8 buf_block[6] = {0};
    int ret = zrh2800k2_read_block(dev, REG_COEFFICIENTS, buf_block, 6);

    
    // [ byte_count,m-l,m-h,b-l,b-h,R ]
    if (ret < 0) {
        printk(KERN_ALERT "get coefficient fail(%d)\n", ret);
        return -1;
    } 
    
    *R = buf_block[5];
    *m = buf_block[2];
    *m = ((*m)<<8 )+ buf_block[1];
    *b = buf_block[4];
    *b = ((*b)<<8 )+ buf_block[3];

    debug_print((KERN_DEBUG " coefficient read : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x \n", buf_block[0], buf_block[1], buf_block[2],
                                                                                   buf_block[3], buf_block[4], buf_block[5]));
    debug_print((KERN_DEBUG " coefficient r m b: 0x%x, 0x%x, 0x%x \n", *R, *m, *b));

    return 0;

}


/* read a byte or word value and show*/
static ssize_t show_value(struct device *dev, struct device_attribute *da, char *buf)
{

    u16 u16_val = 0;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;  // lm-sensor uint: mV, mA, mC
    
    u8 buf_block[11] = {0}; // used to save enough data from read block.

    char *ascii = NULL;
    int ret = 0;

    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    debug_print((KERN_DEBUG "show value op[%d]: reg %d\n", attr->index, 
                                               operation_set[attr->index].reg ));

    switch (operation_set[attr->index].type) {
        
        case READ_BYTE:
        case READ_WORD:
            ret = zrh2800k2_read(dev, operation_set[attr->index].type , operation_set[attr->index].reg);
            break;

        case READ_BLOCK:
            ret = zrh2800k2_read_block(dev, operation_set[attr->index].reg, buf_block, operation_set[attr->index].data_size + 1);
            break;

        default:
            printk(KERN_ALERT "unknown access type\n");
            return 0;

    }

    if (ret < 0) {
        printk(KERN_ALERT "ERROR: Read fail ret(%d)\n", ret);
        return 0;
    }

    /* arrange return buf */
    switch (attr->index) {
        
        /* case directly return */
        case PSU_STATUS_WORD:
        case PSU_VOUT_MODE:
            return sprintf(buf, "%d\n", ret);

        case PSU_STATUS_VOUT:
            return sprintf(buf, 
                        "VOUT Over Voltage Fault    : %d \nVOUT Over Voltage Warning  : %d \nVOUT Under Voltage Warning : %d \nVOUT Under Voltage Fault   : %d \n",
                         (ret>>7)&0x1,(ret>>6)&0x1,
                         (ret>>5)&0x1,(ret>>4)&0x1);
            
        case PSU_STATUS_IOUT:
            return sprintf(buf,
                        "IOUT Overcurrent Fault     : %d \nIOUT Overcurrent Warnning  : %d \nPOUT Overcurrent Fault     : %d \nPOUT Overcurrent Warnning  : %d \n",
                         (ret>>7)&0x1, (ret>>5)&0x1,
                         (ret>>1)&0x1, ret&0x1);

        case PSU_STATUS_INPUT:
            return sprintf(buf,
                        "PIN Overpower Warning  : %d \n", (ret&0x1));

        case PSU_STATUS_TEMPERATURE:
            return sprintf(buf,
                        "Overtemperature  Fault    : %d \nOvertemperature  Warning  : %d \nUbdertemperature Warning  : %d \nUbdertemperature Fault    : %d \n",
                         (ret>>7)&0x1,(ret>>6)&0x1,
                         (ret>>5)&0x1,(ret>>4)&0x1);
 
        case PSU_STATUS_FANS_1_2:
            return sprintf(buf,
                        "Fan Fault     : %d \nFan Warning   : %d \n",
                         (ret>>7)&0x1, (ret>>5)&0x1);        

        case PSU_FAN_CONFIG_1_2:
            debug_print((KERN_DEBUG "PSU_FAN_CONFIG_1_2:  0x%X\n",ret));
            return sprintf(buf,
                        "Fan is installed in Position1: %s\n" \
                        "Fan1 speed Unit: %s\n" \
                        "Fan1 Tachometer Pulses Per Revolution 0x%x\n" \
                        "Fan install in Position2: %s\n" \
                        "Fan2 speed Unit: %s\n" \
                        "Fan2 Tachometer Pulses Per Revolution 0x%x\n",
                        (ret>>7)?"YES":"NONE",
                        ((ret>>6)&0x1)?"RPM":"Duty Cycle",
                        (ret>>4)&0x3,
                        (ret>>3&0x01)?"YES":"NONE",
                        ((ret>>2)&0x1)?"RPM":"Duty Cycle",
                        ret&0x3);
                
        /* special case for READ_VOUT */
        case PSU_VOUT:
            /* save mantissa */
            mantissa = ret;

            debug_print((KERN_DEBUG "PSU_VOUT: mantissa 0x%X\n",mantissa));

            /* read the exponent from REG_READ_VMODE */
            ret = zrh2800k2_read(dev, READ_BYTE , REG_VOUT_MODE);
            if (ret < 0) {
                printk(KERN_ALERT "Error: Read fail ret(%d)\n", ret);
                return 0;
            }
            exponent = two_complement_to_int(ret & 0x1f, 5, 0x1f);
            
            debug_print((KERN_DEBUG "PSU_VOUT: VOUT_MODE 0x%X\n",ret));
            debug_print((KERN_DEBUG "PSU_VOUT: exponent 0x%X\n",exponent));

            return (exponent >= 0) ? sprintf(buf, "%d\n", (mantissa << exponent)*multiplier ) : \
                                     sprintf(buf, "%d\n", (mantissa*multiplier / (1 << -exponent)));

        /* word data with linear format */
        case PSU_POUT:
        case PSU_PIN:
            multiplier = 1000000;   // lm-sensor unit: uW
        case PSU_VIN:
        case PSU_IIN:
        case PSU_IOUT:
        case PSU_TEMPERATURE_1:
        case PSU_FAN_SPEED_1:
        case PSU_MFR_VIN_MAX:
        case PSU_MFR_IOUT_MAX:

            if (attr->index == PSU_FAN_SPEED_1)
                multiplier = 1;

            u16_val = ret;
            exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
            mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
   
            debug_print((KERN_DEBUG "REG(%d): ret 0x%X, u16_val: 0x%x\n", attr->index, ret, u16_val));
            debug_print((KERN_DEBUG "REG(%d): exponent 0x%X\n", attr->index, exponent));
            debug_print((KERN_DEBUG "REG(%d): mantissa 0x%X\n", attr->index, mantissa));
    
            return (exponent >= 0) ? sprintf(buf, "%d\n", (mantissa << exponent)*multiplier ) : \
                                     sprintf(buf, "%d\n", (mantissa*multiplier / (1 << -exponent)));

        case PSU_EIN:
        case PSU_EOUT: {

            u16 m,b;
            u8 R;
            
            u64 ev = buf_block[2];
            u8 rc = buf_block[3];
            u64 sc = buf_block[6];
            u32 sc_mid = buf_block[5];
            u64 average_value = 0;

            struct i2c_client *client = to_i2c_client(dev);
            struct zrh2800k2_data *data = i2c_get_clientdata(client);

            if (get_coefficient(dev, &m, &b, &R) < 0) {
                return sprintf(buf, "ERROR, fail to get coefficient\n");
            }

            // [ bytecount, energy_count-l, energy_count-h, ROLLOVER_count ,
            //   sample_count-l, sample_count-mid, sample_count-h ]
            // maximum_direct_format_value = (m*32767+b)*(10)^R
            // energy_value = Rollover_count * maximum_direct_format_value + energy_count

           debug_print((KERN_DEBUG "[ec-l,ec-h,rc,sc-l,sc-,sc-h]: [0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]\n", buf_block[0], buf_block[1], buf_block[2],  
                                                                                          buf_block[3], buf_block[4], buf_block[5], buf_block[6]));

            ev = rc * (m*32767+b)*easy_pow(10,R) + ((ev<<8) + buf_block[1]);
            sc = (sc<<16) + (sc_mid<<8) + buf_block[4];

            if(attr->index == PSU_EIN) {
                average_value = ((ev - data->last_energy_value_EIN)*1000) / (sc - data->last_smaple_count_EIN);
                data->last_energy_value_EIN = ev;
                data->last_smaple_count_EIN = sc;
            } else {
                average_value = ((ev - data->last_energy_value_EOUT)*1000) / (sc - data->last_smaple_count_EOUT);
                data->last_energy_value_EOUT = ev;
                data->last_smaple_count_EOUT = sc;
            }
            return sprintf(buf, "%llu.%llu\n", average_value/1000, average_value%1000);

        }

        case PSU_MFR_ID:
        case PSU_MFR_MODEL:
            debug_print((KERN_DEBUG "[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]\n", buf_block[0], buf_block[1], buf_block[2],
                                                                         buf_block[3], buf_block[4], buf_block[5]));

            ascii = &buf_block[1];
            return sprintf(buf, "%s\n", ascii);
            

        case PSU_PMBUS_REVISION:
            return sprintf(buf, "Part1 Revision: 1.%d, Part2 Revision: 1.%d\n",
                                (ret>>5), (ret&0x7) );

        /* not implement yet */
        default:
            return sprintf(buf, "not implement yet\n");

    }

    /* should not goto here */
    return sprintf(buf, "unknown case\n");

}


static ssize_t show_capability(struct device *dev, struct device_attribute *da, char *buf)
{
    /* todo */
    return sprintf(buf, "not implement yet\n");
}

static ssize_t psu_pm_query(struct device *dev, struct device_attribute *da, char *buf)
{
    /* todo */
    return sprintf(buf, "not implement yet\n");
}

static ssize_t psu_coefficient(struct device *dev, struct device_attribute *da, char *buf)
{
    /* todo */
    return sprintf(buf, "not implement yet\n");
}

static ssize_t set_fan_config(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct zrh2800k2_data *data = i2c_get_clientdata(client);

    int result;
    u8 input_val;

    mutex_lock(&data->update_lock);
    
    input_val = simple_strtol(buf, NULL, 10);

    result = i2c_smbus_write_byte_data(client, REG_FAN_CONFIG_1_2, input_val); 
   
    mutex_unlock(&data->update_lock);
     
    if (result < 0) {
        printk(KERN_ALERT "ERROR: SET_FAN_CONFIG %s fail\n", buf);
    } else {
        debug_print((KERN_DEBUG "SET_FAN_CONFIG %s success\n", buf));
    }

    
    return count;

}


static const struct i2c_device_id zrh2800k2_id[] = {
    { "zrh2800k2", zrh2800k2 },
    { "zrh2ab0k2", zrh2ab0k2 },
    {}
};

static struct i2c_driver zrh2800k2_driver = {
    .class          = I2C_CLASS_HWMON,
    .driver = 
    {
        .name       = "ZRH2xxxK2",
    },
    .probe          = zrh2800k2_probe,
    .remove         = zrh2800k2_remove,
    .id_table       = zrh2800k2_id,
    .address_list   = normal_i2c,
};

static int zrh2800k2_remove(struct i2c_client *client)
{
    struct zrh2800k2_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &zrh2800k2_group);
    kfree(data);
    return 0;

}

static int zrh2800k2_probe(struct i2c_client *client, 
                         const struct i2c_device_id *dev_id)
{

    struct zrh2800k2_data *data;
    int status;

    if(!i2c_check_functionality(client->adapter, 
                                I2C_FUNC_SMBUS_BYTE_DATA | 
                                I2C_FUNC_SMBUS_WORD_DATA)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct zrh2800k2_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &zrh2800k2_group);
    if (status) {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }

    dev_info(&client->dev, "%s: psu '%s'\n", dev_name(data->hwmon_dev), 
                                             client->name);

    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &zrh2800k2_group);

exit_free:
    kfree(data);

exit:
    return status;

}


module_i2c_driver(zrh2800k2_driver);
MODULE_AUTHOR("Cameo Inc.");
MODULE_DESCRIPTION("Power Supply zrh-2800k2 driver");
MODULE_LICENSE("GPL");

