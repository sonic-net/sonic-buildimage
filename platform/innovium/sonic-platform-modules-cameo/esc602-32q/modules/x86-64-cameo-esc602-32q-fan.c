/* An hwmon driver for Cameo esc602-32Q Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-esc602-32q.h"
#include "x86-64-cameo-esc602-32q-common.h"
#include "x86-64-cameo-esc602-32q-fan.h"

/* extern i2c_client */
extern struct i2c_client *Cameo_CPLD_23_client; //0x23 for Fan CPLD
extern struct i2c_client *Cameo_BMC_14_client;  //0x14 for BMC slave
/* end of extern i2c_client */

/* implement i2c_function */
ssize_t fan_ctrl_mode_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_BMC_14_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == FANCTRL_MODE)
    {
        if( bmc_enable() == ENABLE)
        {
            status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, FANCTRL_MODE_REG);
            if(status == 0xff || status < 0)
            {
                sprintf(buf, "%sAccess BMC module FAILED\n", buf);
            }
            else
            {
                sprintf(buf, "%s0x%x\n", buf, status);
            }
        }
        else
        {
            sprintf(buf, "%sAccess BMC module FAILED\n", buf);
        }
    }
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%s\n", buf);
}

ssize_t fan_ctrl_rpm_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_BMC_14_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == FANCTRL_RPM)
    {
        if( bmc_enable() == ENABLE)
        {
            status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, FANCTRL_RPM_REG);
            if(status == 0xff || status < 0)
            {
                sprintf(buf, "%sAccess BMC module FAILED\n", buf);
            }
            else
            {
                sprintf(buf, "%s0x%x\n", buf, status);
            }
        }
        else
        {
            sprintf(buf, "%sAccess BMC module FAILED\n", buf);
        }
    }
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%s\n", buf);
}

ssize_t fan_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int result = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_23_data = i2c_get_clientdata(Cameo_CPLD_23_client);
    struct Cameo_i2c_data *Cameo_BMC_14_data = i2c_get_clientdata(Cameo_BMC_14_client);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        mutex_lock(&Cameo_BMC_14_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, BMC_FAN_STAT_REG);
        mutex_unlock(&Cameo_BMC_14_data->update_lock);
    }
    else
    {
        mutex_lock(&Cameo_CPLD_23_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_CPLD_23_client, FAN_STAT_REG);
        mutex_unlock(&Cameo_CPLD_23_data->update_lock);
    }
    
    result = FAILED;
    switch (attr->index)
    {
        case 1:
            if(status & BIT_0_MASK)
            {
                result = PASSED;
            }
            break;
        case 2:
            if(status & BIT_1_MASK)
            {
                result = PASSED;
            }
            break;
        case 3:
            if(status & BIT_2_MASK)
            {
                result = PASSED;
            }
            break;
        case 4:
            if(status & BIT_3_MASK)
            {
                result = PASSED;
            }
            break;
        case 5:
            if(status & BIT_4_MASK)
            {
                result = PASSED;
            }
            break;
    }
    if(result != PASSED)
    {
        return sprintf(buf, "%s%d\n", buf, FAILED);
    }
    return sprintf(buf, "%s%d\n", buf, PASSED);
}

ssize_t fan_present_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int result = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_23_data = i2c_get_clientdata(Cameo_CPLD_23_client);
    struct Cameo_i2c_data *Cameo_BMC_14_data = i2c_get_clientdata(Cameo_BMC_14_client);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        mutex_lock(&Cameo_BMC_14_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, BMC_FAN_PRESENT_REG);
        mutex_unlock(&Cameo_BMC_14_data->update_lock);
    }
    else
    {
        mutex_lock(&Cameo_CPLD_23_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_CPLD_23_client, FAN_PRESENT_REG);
        mutex_unlock(&Cameo_CPLD_23_data->update_lock);
    }
    
    result = FAILED;
    switch (attr->index)
    {
        case 1:
            if(status & BIT_0_MASK)
            {
                result = PASSED;
            }
            break;
        case 2:
            if(status & BIT_1_MASK)
            {
                result = PASSED;
            }
            break;
        case 3:
            if(status & BIT_2_MASK)
            {
                result = PASSED;
            }
            break;
        case 4:
            if(status & BIT_3_MASK)
            {
                result = PASSED;
            }
            break;
        case 5:
            if(status & BIT_4_MASK)
            {
                result = PASSED;
            }
            break;
    }
    if(result != PASSED)
    {
        return sprintf(buf, "%s%d\n", buf, FAILED);
    }
    return sprintf(buf, "%s%d\n", buf, PASSED);
}

ssize_t fan_power_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int result = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_23_data = i2c_get_clientdata(Cameo_CPLD_23_client);
    struct Cameo_i2c_data *Cameo_BMC_14_data = i2c_get_clientdata(Cameo_BMC_14_client);
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        mutex_lock(&Cameo_BMC_14_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, BMC_FAN_POWER_REG);
        mutex_unlock(&Cameo_BMC_14_data->update_lock);
    }
    else
    {
        mutex_lock(&Cameo_CPLD_23_data->update_lock);
        status = i2c_smbus_read_byte_data(Cameo_CPLD_23_client, FAN_POWER_REG);
        mutex_unlock(&Cameo_CPLD_23_data->update_lock);
    }
    
    result = FAILED;
    switch (attr->index)
    {
        case 1:
            if(status & BIT_0_MASK)
            {
                result = PASSED;
            }
            break;
        case 2:
            if(status & BIT_1_MASK)
            {
                result = PASSED;
            }
            break;
        case 3:
            if(status & BIT_2_MASK)
            {
                result = PASSED;
            }
            break;
        case 4:
            if(status & BIT_3_MASK)
            {
                result = PASSED;
            }
            break;
        case 5:
            if(status & BIT_4_MASK)
            {
                result = PASSED;
            }
            break;
    }
    if(result != PASSED)
    {
        return sprintf(buf, "%s%d\n", buf, FAILED);
    }
    return sprintf(buf, "%s%d\n", buf, PASSED);
}

ssize_t fan_rpm_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status          = -EPERM;
    int fan_location    = 0;
    int fan_offset      = 0; 
    u16 fan_speed       = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *target_client = NULL; 
    
    sprintf(buf, "");
    if( bmc_enable() == ENABLE)
    {
        target_client = Cameo_BMC_14_client;
    }
    else
    {
        target_client = Cameo_CPLD_23_client;
    }
    
    switch (attr->index)
    {
        case FAN1_FRONT_RPM:
            fan_location = 0;
            fan_offset = 0; 
            break;
        case FAN2_FRONT_RPM:
            fan_location = 0;
            fan_offset = 1; 
            break;
        case FAN3_FRONT_RPM:
            fan_location = 0;
            fan_offset = 2; 
            break;
        case FAN4_FRONT_RPM:
            fan_location = 0;
            fan_offset = 3; 
            break;
        case FAN5_FRONT_RPM:
            fan_location = 0;
            fan_offset = 4; 
            break;
        case FAN1_REAR_RPM:
            fan_location = 1;
            fan_offset = 0; 
            break;
        case FAN2_REAR_RPM:
            fan_location = 1;
            fan_offset = 1; 
            break;
        case FAN3_REAR_RPM:
            fan_location = 1;
            fan_offset = 2; 
            break;
        case FAN4_REAR_RPM:
            fan_location = 1;
            fan_offset = 3; 
            break;
        case FAN5_REAR_RPM:
            fan_location = 1;
            fan_offset = 4; 
            break;
    }
    if(fan_location == 0)
    {
        // front fan of couple
        // read high byte
        status = i2c_smbus_read_byte_data(target_client, FAN_F_RPM_REG+(fan_offset*2)+1);
        fan_speed = status;
        if(status < 0 || status == 0xff)
        {
            fan_speed = 0; 
        }
        // read low byte
        status = i2c_smbus_read_byte_data(target_client, FAN_F_RPM_REG+(fan_offset*2));
        fan_speed = ((fan_speed<<8) + status)*30;
        if(status < 0 || status == 0xff)
        {
            fan_speed = 0;
        }
    }
    else
    {
        // rear fan of couple
        // read high byte
        status = i2c_smbus_read_byte_data(target_client, FAN_R_RPM_REG+(fan_offset*2)+1);
        fan_speed = status;
        if(status < 0 || status == 0xff)
        {
            fan_speed = 0; 
        }
        // read low byte
        status = i2c_smbus_read_byte_data(target_client, FAN_R_RPM_REG+(fan_offset*2));
        fan_speed = ((fan_speed<<8) + status)*30;
        if(status < 0 || status == 0xff)
        {
            fan_speed = 0;
        }
    }
    sprintf(buf, "%s%d\n", buf, fan_speed);
    return sprintf(buf, "%s\n",buf);
}
/* end of implement i2c_function */