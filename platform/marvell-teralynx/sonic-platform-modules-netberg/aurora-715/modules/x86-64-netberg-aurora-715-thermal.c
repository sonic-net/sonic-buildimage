/* An hwmon driver for Netberg Aurora 715 Marvell Teralynx i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-netberg-aurora-715.h"
#include "x86-64-netberg-aurora-715-common.h"
#include "x86-64-netberg-aurora-715-thermal.h"

/* extern i2c_client */
extern struct i2c_client *Netberg_BMC_14_client;  //0x14 for BMC slave
/* end of extern i2c_client */

/* implement i2c_function */
ssize_t themal_temp_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if( bmc_enable() == ENABLE)
    {
        switch (attr->index)
        {
            case TEMP_R_B_F:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_F_REG);
            break;
            case TEMP_R_B_B:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_B_REG);
            break;
            case TEMP_L_B_F:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_F_REG);
            break;
            case TEMP_L_B_B:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_B_REG);
            break;
            case TEMP_R_T_F:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_F_REG);
            break;
            case TEMP_R_T_B:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_B_REG);
            break;
            case TEMP_L_T_F:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_F_REG);
            break;
            case TEMP_L_T_B:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_B_REG);
            break;
        }
        if(status == 0xff || status < 0)
        {
            sprintf(buf, "%sAccess BMC module FAILED\n", buf);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, (read_8bit_temp((status & 0x80), status))*1000);
        }
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t themal_temp_max_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if( bmc_enable() == ENABLE)
    {
        switch (attr->index)
        {
            case TEMP_R_B_F_MAX:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_F_MAX_REG);
            break;
            case TEMP_L_B_F_MAX:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_F_MAX_REG);
            break;
            case TEMP_R_T_F_MAX:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_F_MAX_REG);
            break;
            case TEMP_L_T_F_MAX:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_F_MAX_REG);
            break;
            case TEMP_R_B_B_MAX:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_B_MAX_REG);
            break;
            case TEMP_L_B_B_MAX:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_B_MAX_REG);
            break;
            case TEMP_R_T_B_MAX:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_B_MAX_REG);
            break;
            case TEMP_L_T_B_MAX:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_B_MAX_REG);
            break;
        }
        if(status == 0xff || status < 0)
        {
            sprintf(buf, "%sAccess BMC module FAILED\n", buf);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, (read_8bit_temp((status & 0x80), status))*1000);
        }
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t themal_temp_min_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if( bmc_enable() == ENABLE)
    {
        switch (attr->index)
        {
            case TEMP_R_B_F_MIN:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_F_MIN_REG);
            break;
            case TEMP_L_B_F_MIN:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_F_MIN_REG);
            break;
            case TEMP_R_T_F_MIN:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_F_MIN_REG);
            break;
            case TEMP_L_T_F_MIN:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_F_MIN_REG);
            break;
            case TEMP_R_B_B_MIN:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_B_MIN_REG);
            break;
            case TEMP_L_B_B_MIN:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_B_MIN_REG);
            break;
            case TEMP_R_T_B_MIN:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_B_MIN_REG);
            break;
            case TEMP_L_T_B_MIN:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_B_MIN_REG);
            break;
        }
        if(status == 0xff || status < 0)
        {
            sprintf(buf, "%sAccess BMC module FAILED\n", buf);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, (read_8bit_temp((status & 0x80), status))*1000);
        }
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t themal_temp_crit_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if( bmc_enable() == ENABLE)
    {
        switch (attr->index)
        {
            case TEMP_R_B_F_CRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_F_CRIT_REG);
            break;
            case TEMP_L_B_F_CRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_F_CRIT_REG);
            break;
            case TEMP_R_T_F_CRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_F_CRIT_REG);
            break;
            case TEMP_L_T_F_CRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_F_CRIT_REG);
            break;
            case TEMP_R_B_B_CRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_B_CRIT_REG);
            break;
            case TEMP_L_B_B_CRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_B_CRIT_REG);
            break;
            case TEMP_R_T_B_CRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_B_CRIT_REG);
            break;
            case TEMP_L_T_B_CRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_B_CRIT_REG);
            break;
        }
        if(status == 0xff || status < 0)
        {
            sprintf(buf, "%sAccess BMC module FAILED\n", buf);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, (read_8bit_temp((status & 0x80), status))*1000);
        }
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t themal_temp_lcrit_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if( bmc_enable() == ENABLE)
    {
        switch (attr->index)
        {
            case TEMP_R_B_F_LCRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_F_LCRIT_REG);
            break;
            case TEMP_L_B_F_LCRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_F_LCRIT_REG);
            break;
            case TEMP_R_T_F_LCRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_F_LCRIT_REG);
            break;
            case TEMP_L_T_F_LCRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_F_LCRIT_REG);
            break;
            case TEMP_R_B_B_LCRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_B_B_LCRIT_REG);
            break;
            case TEMP_L_B_B_LCRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_B_B_LCRIT_REG);
            break;
            case TEMP_R_T_B_LCRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_R_T_B_LCRIT_REG);
            break;
            case TEMP_L_T_B_LCRIT:
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, TEMP_L_T_B_LCRIT_REG);
            break;
        }
        if(status == 0xff || status < 0)
        {
            sprintf(buf, "%sAccess BMC module FAILED\n", buf);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, (read_8bit_temp((status & 0x80), status))*1000);
        }
    }
    else
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    
    return sprintf(buf, "%s\n", buf);
}
/* end of implement i2c_function */
