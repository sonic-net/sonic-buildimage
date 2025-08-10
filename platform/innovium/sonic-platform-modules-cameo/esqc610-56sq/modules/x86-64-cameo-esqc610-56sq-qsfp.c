/* An hwmon driver for Cameo esqc610-56sq Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-esqc610-56sq.h"
#include "x86-64-cameo-esqc610-56sq-common.h"
#include "x86-64-cameo-esqc610-56sq-qsfp.h"

/* i2c_client Declaration */
extern struct i2c_client *Cameo_CPLD_32_client; //0x32 for Port 01-08
/* end of i2c_client Declaration */

/* extern i2c_function */
/* end of extern i2c_function */

/* implement i2c_function */
ssize_t qsfp_low_power_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status   = -EPERM;
    u8 result   = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == QSFP_LOW_POWER_ALL)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, QSFP_LOW_POWER_REG);  //1-8
        result = status;
        sprintf(buf, "%s0x%x\n", buf, result);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t qsfp_low_power_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int value   = 0x0;
    int result  = 0;
    int input   = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    
    mutex_lock(&Cameo_CPLD_32_data->update_lock);
    if (attr->index == QSFP_LOW_POWER_ALL)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == ENABLE)
        {
            value = 0xff;
        }
        else if(input == DISABLE)
        {
            value = 0x00;
        }
        else
        {
            printk(KERN_ALERT "qsfp_low_power_all_set wrong value\n");
            return count;
        }
        result += i2c_smbus_write_byte_data(Cameo_CPLD_32_client, QSFP_LOW_POWER_REG, value);
        
        if(result != 0)
        {
            printk(KERN_ALERT "qsfp_low_power_all_set FAILED\n");
            return count;
        }
    }
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    return count;
}

ssize_t qsfp_low_power_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    port_index = attr->index;
    sprintf(buf, "");

    if (port_index >= 1 && port_index <= 8)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, qsfp_low_power_regs[port_index][0]);
    }
    
    if (status & qsfp_low_power_regs[port_index][1])
    {
        sprintf(buf, "%s%d\n", buf, ENABLE);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, DISABLE);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t qsfp_low_power_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int result = 0;
    int input  = 0;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    struct i2c_client *target_client = NULL; 
    
    port_index = attr->index;
    input = simple_strtol(buf, NULL, 10);
    mutex_lock(&Cameo_CPLD_32_data->update_lock);

    if (port_index >= 1 && port_index <= 8)
    {
        target_client = Cameo_CPLD_32_client;

    }

    status = i2c_smbus_read_byte_data(target_client, qsfp_low_power_regs[port_index][0]);
    if( input == ENABLE)
    {
        status |= qsfp_low_power_regs[port_index][1];
        result = i2c_smbus_write_byte_data(target_client, qsfp_low_power_regs[port_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: qsfp_low_power_set ON FAILED!\n");
        }
    }
    else if( input == DISABLE)
    {
        status &= ~(qsfp_low_power_regs[port_index][1]);
        result = i2c_smbus_write_byte_data(target_client, qsfp_low_power_regs[port_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: qsfp_low_power_set OFF FAILED!\n");
        }
    }
    else
    {
        printk(KERN_ALERT "ERROR: qsfp_low_power_set WRONG VALUE\n");
    }
    
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    
    return count;
}

ssize_t qsfp_reset_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int value   = 0x0;
    int result  = 0;
    int input   = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    
    mutex_lock(&Cameo_CPLD_32_data->update_lock);
    if (attr->index == QSFP_RESET_ALL)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == QSFP_RESET)
        {
            value = 0x00;
        }
        else
        {
            printk(KERN_ALERT "qsfp_reset_all_set wrong value\n");
            return count;
        }
        
        result += i2c_smbus_write_byte_data(Cameo_CPLD_32_client, QSFP_RESET_REG, value);
        
        if(result != 0)
        {
            printk(KERN_ALERT "qsfp_reset_all_set FAILED\n");
            return count;
        }
    }
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    return count;
}

ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int result = 0;
    int input  = 0;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    struct i2c_client *target_client = NULL; 
    
    port_index = attr->index;
    input = simple_strtol(buf, NULL, 10);
    mutex_lock(&Cameo_CPLD_32_data->update_lock);

    if (port_index >= 1 && port_index <= 8)
    {
        target_client = Cameo_CPLD_32_client;

    }
    
    status = i2c_smbus_read_byte_data(target_client, qsfp_reset_regs[port_index][0]);
    if( input == QSFP_RESET)
    {
        status |= qsfp_reset_regs[port_index][1];
        result = i2c_smbus_write_byte_data(target_client, qsfp_reset_regs[port_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: qsfp_reset_set FAILED!\n");
        }
    }
    else
    {
        printk(KERN_ALERT "ERROR: qsfp_reset_set WRONG VALUE\n");
    }
    
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    
    return count;
}

ssize_t qsfp_present_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status   = -EPERM;
    u8 result   = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == QSFP_PRESENT_ALL)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, QSFP_PRESENT_REG);  //1-8
        result = status;
        result = ~(result);
        sprintf(buf, "%s0x%x\n", buf, result);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t qsfp_present_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    port_index = attr->index;
    sprintf(buf, "");

    if (port_index >= 1 && port_index <= 8)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, qsfp_present_regs[port_index][0]);
    }
    
    if (status & qsfp_present_regs[port_index][1])
    {
        sprintf(buf, "%s%d\n", buf, DISABLE);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ENABLE);
    }
    
    return sprintf(buf, "%s\n", buf);
}
ssize_t qsfp_int_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status   = -EPERM;
    u8 result   = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == QSFP_INT_ALL)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, QSFP_INT_REG);  //1-8
        result = status;
        sprintf(buf, "%s0x%x\n", buf, result);
    }
    return sprintf(buf, "%s\n", buf);
}
ssize_t qsfp_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    port_index = attr->index;
    sprintf(buf, "");

    if (port_index >= 1 && port_index <= 8)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, qsfp_int_regs[port_index][0]);
    }
    
    if (status & qsfp_int_regs[port_index][1])
    {
        sprintf(buf, "%s%d\n", buf, ENABLE);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, DISABLE);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t qsfp_quter_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int quter_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    quter_index = attr->index;
    sprintf(buf, "");

    status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, qsfp_quter_int_regs[quter_index][0]);
    
    if (status & qsfp_quter_int_regs[quter_index][1])
    {
        sprintf(buf, "%s%d\n", buf, NORMAL);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ABNORMAL);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t qsfp_quter_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int quter_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    quter_index = attr->index;
    sprintf(buf, "");

    status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, qsfp_quter_int_mask_regs[quter_index][0]);
    
    if (status & qsfp_quter_int_mask_regs[quter_index][1])
    {
        sprintf(buf, "%s%d\n", buf, DISABLE);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ENABLE);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t qsfp_quter_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int result = 0;
    int input  = 0;
    int quter_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    struct i2c_client *target_client = NULL; 
    
    quter_index = attr->index;
    input = simple_strtol(buf, NULL, 10);
    mutex_lock(&Cameo_CPLD_32_data->update_lock);

    target_client = Cameo_CPLD_32_client;
    
    status = i2c_smbus_read_byte_data(target_client, qsfp_quter_int_mask_regs[quter_index][0]);
    if( input == DISABLE)
    {
        status |= qsfp_quter_int_mask_regs[quter_index][1];
        result = i2c_smbus_write_byte_data(target_client, qsfp_quter_int_mask_regs[quter_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: qsfp_quter_int_mask_set ON FAILED!\n");
        }
    }
    else if( input == ENABLE)
    {
        status &= ~(qsfp_quter_int_mask_regs[quter_index][1]);
        result = i2c_smbus_write_byte_data(target_client, qsfp_quter_int_mask_regs[quter_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: qsfp_quter_int_mask_set OFF FAILED!\n");
        }
    }
    else
    {
        printk(KERN_ALERT "ERROR: qsfp_quter_int_mask_set WRONG VALUE\n");
    }
    
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    
    return count;
}

ssize_t qsfp_modprs_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int quter_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    quter_index = attr->index;
    sprintf(buf, "");

    status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, qsfp_modprs_int_regs[quter_index][0]);
    
    if (status & qsfp_modprs_int_regs[quter_index][1])
    {
        sprintf(buf, "%s%d\n", buf, NORMAL);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ABNORMAL);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t qsfp_modprs_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int quter_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    quter_index = attr->index;
    sprintf(buf, "");

    status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, qsfp_modprs_int_mask_regs[quter_index][0]);
    
    if (status & qsfp_modprs_int_mask_regs[quter_index][1])
    {
        sprintf(buf, "%s%d\n", buf, DISABLE);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ENABLE);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t qsfp_modprs_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int result = 0;
    int input  = 0;
    int quter_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    struct i2c_client *target_client = NULL; 
    
    quter_index = attr->index;
    input = simple_strtol(buf, NULL, 10);
    mutex_lock(&Cameo_CPLD_32_data->update_lock);

    target_client = Cameo_CPLD_32_client;

    status = i2c_smbus_read_byte_data(target_client, qsfp_modprs_int_mask_regs[quter_index][0]);
    if( input == DISABLE)
    {
        status |= qsfp_modprs_int_mask_regs[quter_index][1];
        result = i2c_smbus_write_byte_data(target_client, qsfp_modprs_int_mask_regs[quter_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: qsfp_modprs_int_mask_set ON FAILED!\n");
        }
    }
    else if( input == ENABLE)
    {
        status &= ~(qsfp_modprs_int_mask_regs[quter_index][1]);
        result = i2c_smbus_write_byte_data(target_client, qsfp_modprs_int_mask_regs[quter_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: qsfp_modprs_int_mask_set OFF FAILED!\n");
        }
    }
    else
    {
        printk(KERN_ALERT "ERROR: qsfp_modprs_int_mask_set WRONG VALUE\n");
    }
    
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    
    return count;
}
/* end of implement i2c_function */