/* An hwmon driver for Cameo esqc610-56sq Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-esqc610-56sq.h"
#include "x86-64-cameo-esqc610-56sq-common.h"
#include "x86-64-cameo-esqc610-56sq-thermal.h"

/* extern i2c_client */
extern struct i2c_client *Cameo_BMC_14_client;  //0x14 for BMC slave
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
            case TEMP_TH0_T:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_T_REG);
            break;
            case TEMP_TH0_B:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_B_REG);
            break;
            case TEMP_TH0_R:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_R_REG);
            break;
            case TEMP_TH1_T:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_T_REG);
            break;
            case TEMP_TH1_B:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_B_REG);
            break;
            case TEMP_TH3_T:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_T_REG);
            break;
            case TEMP_TH3_B:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_B_REG);
            break;
            case TEMP_TH2_T:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_T_REG);
            break;
            case TEMP_TH2_B:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_B_REG);
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
            case TEMP_TH0_T_MAX:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_T_MAX_REG);
            break;
            case TEMP_TH1_T_MAX:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_T_MAX_REG);
            break;
            case TEMP_TH3_T_MAX:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_T_MAX_REG);
            break;
            case TEMP_TH2_T_MAX:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_T_MAX_REG);
            break;
            case TEMP_TH0_B_MAX:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_B_MAX_REG);
            break;
            case TEMP_TH0_R_MAX:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_R_MAX_REG);
            break;
            case TEMP_TH1_B_MAX:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_B_MAX_REG);
            break;
            case TEMP_TH3_B_MAX:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_B_MAX_REG);
            break;
            case TEMP_TH2_B_MAX:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_B_MAX_REG);
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
            case TEMP_TH0_T_MIN:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_T_MIN_REG);
            break;
            case TEMP_TH1_T_MIN:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_T_MIN_REG);
            break;
            case TEMP_TH3_T_MIN:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_T_MIN_REG);
            break;
            case TEMP_TH2_T_MIN:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_T_MIN_REG);
            break;
            case TEMP_TH0_B_MIN:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_B_MIN_REG);
            break;
            case TEMP_TH0_R_MIN:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_R_MIN_REG);
            break;
            case TEMP_TH1_B_MIN:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_B_MIN_REG);
            break;
            case TEMP_TH3_B_MIN:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_B_MIN_REG);
            break;
            case TEMP_TH2_B_MIN:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_B_MIN_REG);
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
            case TEMP_TH0_T_CRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_T_CRIT_REG);
            break;
            case TEMP_TH1_T_CRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_T_CRIT_REG);
            break;
            case TEMP_TH3_T_CRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_T_CRIT_REG);
            break;
            case TEMP_TH2_T_CRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_T_CRIT_REG);
            break;
            case TEMP_TH0_B_CRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_B_CRIT_REG);
            break;
            case TEMP_TH0_R_CRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_R_CRIT_REG);
            break;
            case TEMP_TH1_B_CRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_B_CRIT_REG);
            break;
            case TEMP_TH3_B_CRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_B_CRIT_REG);
            break;
            case TEMP_TH2_B_CRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_B_CRIT_REG);
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
            case TEMP_TH0_T_LCRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_T_LCRIT_REG);
            break;
            case TEMP_TH1_T_LCRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_T_LCRIT_REG);
            break;
            case TEMP_TH3_T_LCRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_T_LCRIT_REG);
            break;
            case TEMP_TH2_T_LCRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_T_LCRIT_REG);
            break;
            case TEMP_TH0_B_LCRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_B_LCRIT_REG);
            break;
            case TEMP_TH0_R_LCRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH0_R_LCRIT_REG);
            break;
            case TEMP_TH1_B_LCRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH1_B_LCRIT_REG);
            break;
            case TEMP_TH3_B_LCRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH3_B_LCRIT_REG);
            break;
            case TEMP_TH2_B_LCRIT:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, TEMP_TH2_B_LCRIT_REG);
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