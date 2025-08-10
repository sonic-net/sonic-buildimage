/* An hwmon driver for Cameo escc601-32Q Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-escc601-32q.h"
#include "x86-64-cameo-escc601-32q-common.h"
#include "x86-64-cameo-escc601-32q-power.h"

/* extern i2c_client */
extern struct i2c_client *Cameo_CPLD_35_client; //0x35 for Power CPLD
extern struct i2c_client *Cameo_BMC_14_client;  //0x14 for BMC slave
/* end of extern i2c_client */

/* convert function */
static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}
/* end of convert function */

/* implement i2c_function */
ssize_t psu_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    u32 result = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_35_data = i2c_get_clientdata(Cameo_CPLD_35_client);
    struct Cameo_i2c_data *Cameo_BMC_14_data = i2c_get_clientdata(Cameo_BMC_14_client);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        mutex_lock(&Cameo_BMC_14_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, BMC_PSU_STAT_REG);
        mutex_unlock(&Cameo_BMC_14_data->update_lock);
    }
    else
    {
        mutex_lock(&Cameo_CPLD_35_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_CPLD_35_client, PSU_STAT_REG);
        mutex_unlock(&Cameo_CPLD_35_data->update_lock);
    }
    
    result = TRUE;
    switch (attr->index)
    {
        case 1:
            if(status & BIT_2_MASK)
            {
                result = FALSE;
            }
            break;
        case 2:
            if(status & BIT_3_MASK)
            {
                result = FALSE;
            }
            break;
    }
    if(result != TRUE)
    {
        return sprintf(buf, "%s%d\n", buf, FALSE);
    }
    return sprintf(buf, "%s%d\n", buf, TRUE);
}
ssize_t psu_present_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    u32 result = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_35_data = i2c_get_clientdata(Cameo_CPLD_35_client);
    struct Cameo_i2c_data *Cameo_BMC_14_data = i2c_get_clientdata(Cameo_BMC_14_client);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        mutex_lock(&Cameo_BMC_14_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, BMC_PSU_STAT_REG);
        mutex_unlock(&Cameo_BMC_14_data->update_lock);
    }
    else
    {
        mutex_lock(&Cameo_CPLD_35_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_CPLD_35_client, PSU_STAT_REG);
        mutex_unlock(&Cameo_CPLD_35_data->update_lock);
    }
    
    result = FALSE;
    switch (attr->index)
    {
        case 1:
            if(status & BIT_0_MASK)
            {
                result = TRUE;
            }
            break;
        case 2:
            if(status & BIT_1_MASK)
            {
                result = TRUE;
            }
            break;
    }
    if(result != TRUE)
    {
        return sprintf(buf, "%s%d\n", buf, FALSE);
    }
    return sprintf(buf, "%s%d\n", buf, TRUE);
}
ssize_t psu_vin_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_VIN:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_1_VIN_REG);
                break;
            case PSU2_VIN:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_2_VIN_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000;
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}
ssize_t psu_iin_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_IIN:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_1_IIN_REG);
                break;
            case PSU2_IIN:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_2_IIN_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000;
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}
ssize_t psu_vout_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 result = -EPERM;
    int exponent = 0;
    int multiplier = 1000;
    u16 u16_vmode = 0;
    u16 u16_vout  = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_VOUT:
                u16_vmode = i2c_smbus_read_byte_data(Cameo_BMC_14_client, PSU_1_VMODE_REG); 
                u16_vout  = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_1_VOUT_REG); 
                break;
            case PSU2_VOUT:
                u16_vmode = i2c_smbus_read_byte_data(Cameo_BMC_14_client, PSU_2_VMODE_REG); 
                u16_vout  = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_2_VOUT_REG); 
                break;
        }
        if(u16_vout == 0xffff || u16_vout == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        /* vout mode */
        multiplier = 1000;
        exponent = two_complement_to_int(u16_vmode & 0x1f, 5, 0x1f);
        /* vout */
        result = (exponent >= 0) ? ((u16_vout << exponent)*multiplier) : \
                (u16_vout*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t psu_iout_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_IOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_1_IOUT_REG);
                break;
            case PSU2_IOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_2_IOUT_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000;
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t psu_temp_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u16 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_TEMP:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_1_TEMP_1_REG);
                break;
            case PSU2_TEMP:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_2_TEMP_1_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000;
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t psu_fan_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u16 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_FAN_SPEED:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_1_FAN_SPEED_REG);
                break;
            case PSU2_FAN_SPEED:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_2_FAN_SPEED_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1;
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t psu_pout_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_POUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_1_POUT_REG);
                break;
            case PSU2_POUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_2_POUT_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000000; // lm-sensor unit: uW
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t psu_pin_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_PIN:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_1_PIN_REG);
                break;
            case PSU2_PIN:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_2_PIN_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000000; // lm-sensor unit: uW
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t psu_mfr_model_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u16 u16_val = 0;
    char model[I2C_SMBUS_BLOCK_MAX] = {0};
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_MFR_MODEL:
                u16_val = i2c_smbus_read_i2c_block_data(Cameo_BMC_14_client, PSU_1_MFR_MODEL_REG, I2C_SMBUS_BLOCK_MAX, model);
                break;
            case PSU2_MFR_MODEL:
                u16_val = i2c_smbus_read_i2c_block_data(Cameo_BMC_14_client, PSU_2_MFR_MODEL_REG, I2C_SMBUS_BLOCK_MAX, model);
                break;
        }
        if(u16_val != I2C_SMBUS_BLOCK_MAX)
        {
            return sprintf(buf, "%sERROR\n", buf);
        }
        if (model[1] < 0x20 || model[1] > 0x7f)
        {
            return sprintf(buf, "%sERROR\n", buf);
        }
        sprintf(buf, "%s%s", buf, model);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t psu_iout_max_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_MFR_IOUT_MAX:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_1_MFR_IOUT_MAX_REG);
                break;
            case PSU2_MFR_IOUT_MAX:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, PSU_2_MFR_IOUT_MAX_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000; // lm-sensor unit: uW
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t psu_vmode_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u16 u16_vmode = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case PSU1_VMODE:
                u16_vmode = i2c_smbus_read_byte_data(Cameo_BMC_14_client, PSU_1_VMODE_REG); 
                break;
            case PSU2_VMODE:
                u16_vmode = i2c_smbus_read_byte_data(Cameo_BMC_14_client, PSU_2_VMODE_REG); 
                break;
        }
        if(u16_vmode == 0xffff || u16_vmode == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        /* vout mode */
        sprintf(buf, "%s%d\n", buf, u16_vmode);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t dc_vout_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u16 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case DC6E_P0_VOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_6E_P0_VOUT_REG);
                break;
            case DC6E_P1_VOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_6E_P1_VOUT_REG);
                break;
            case DC70_P0_VOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_70_P0_VOUT_REG);
                break;
            case DC70_P1_VOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_70_P1_VOUT_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000;
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t dc_iout_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u16 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case DC6E_P0_IOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_6E_P0_IOUT_REG);
                break;
            case DC6E_P1_IOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_6E_P1_IOUT_REG);
                break;
            case DC70_P0_IOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_70_P0_IOUT_REG);
                break;
            case DC70_P1_IOUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_70_P1_IOUT_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000;
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t dc_pout_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u16 result = -EPERM;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;
    u16 u16_val = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        switch(attr->index)
        {
            case DC6E_P0_POUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_6E_P0_POUT_REG);
                break;
            case DC6E_P1_POUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_6E_P1_POUT_REG);
                break;
            case DC70_P0_POUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_70_P0_POUT_REG);
                break;
            case DC70_P1_POUT:
                u16_val = i2c_smbus_read_word_data(Cameo_BMC_14_client, DC_CHIP_70_P1_POUT_REG);
                break;
        }
        if(u16_val == 0xffff || u16_val == -1)
        {
            return sprintf(buf, "%s0\n", buf);
        }
        exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
        mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
        multiplier = 1000000;
        result   = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                        (mantissa*multiplier / (1 << -exponent));
        sprintf(buf, "%s%d\n", buf, result);
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}
/* end of implement i2c_function */