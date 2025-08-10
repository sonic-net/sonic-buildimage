/* An hwmon driver for Cameo esqc610-56sq Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-esqc610-56sq.h"
#include "x86-64-cameo-esqc610-56sq-common.h"
#include "x86-64-cameo-esqc610-56sq-sfp.h"

/* i2c_client Declaration */
extern struct i2c_client *Cameo_CPLD_31_client; //0x31 for Port 01-32
extern struct i2c_client *Cameo_CPLD_32_client; //0x32 for Port 33-48
/* end of i2c_client Declaration */

/* extern i2c_function */
/* end of extern i2c_function */

/* implement i2c_function */
ssize_t sfp_tx_enable_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status   = -EPERM;
    u64 result  = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == SFP_TX_ENABLE_ALL)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, SFP_41_48_TX_ENABLE_REG); //41-48
        result = status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, SFP_33_40_TX_ENABLE_REG); //33-40
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_25_32_TX_ENABLE_REG); //25-32
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_17_24_TX_ENABLE_REG); //17-24
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_9_16_TX_ENABLE_REG);  //9-16
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_1_8_TX_ENABLE_REG);   //1-8
        result = (result << 8) | status;
        result = (~(result)) & 0xFFFFFFFFFFFF;
        sprintf(buf, "%s0x%llx\n", buf, result);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t sfp_tx_enable_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int value   = 0x0;
    int result  = 0;
    int input   = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_31_data = i2c_get_clientdata(Cameo_CPLD_31_client);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    
    mutex_lock(&Cameo_CPLD_31_data->update_lock);
    mutex_lock(&Cameo_CPLD_32_data->update_lock);
    if (attr->index == SFP_TX_ENABLE_ALL)
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
            printk(KERN_ALERT "sfp_tx_enable_all_set wrong value\n");
            return count;
        }
        result += i2c_smbus_write_byte_data(Cameo_CPLD_31_client, SFP_1_8_TX_ENABLE_REG, value);
        result += i2c_smbus_write_byte_data(Cameo_CPLD_31_client, SFP_9_16_TX_ENABLE_REG, value);
        result += i2c_smbus_write_byte_data(Cameo_CPLD_31_client, SFP_17_24_TX_ENABLE_REG, value);
        result += i2c_smbus_write_byte_data(Cameo_CPLD_31_client, SFP_25_32_TX_ENABLE_REG, value);
        result += i2c_smbus_write_byte_data(Cameo_CPLD_32_client, SFP_33_40_TX_ENABLE_REG, value);
        result += i2c_smbus_write_byte_data(Cameo_CPLD_32_client, SFP_41_48_TX_ENABLE_REG, value);
        
        if(result != 0)
        {
            printk(KERN_ALERT "sfp_tx_enable_all_set FAILED\n");
            return count;
        }
    }
    mutex_unlock(&Cameo_CPLD_31_data->update_lock);
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    return count;
}

ssize_t sfp_tx_enable_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    port_index = attr->index;
    sprintf(buf, "");

    if (port_index >= 1 && port_index <= 32)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, sfp_tx_enable_regs[port_index][0]);
    }
    else if (port_index >= 33 && port_index <= 48)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, sfp_tx_enable_regs[port_index][0]);
    }
    
    if (status & sfp_tx_enable_regs [port_index][1])
    {
        sprintf(buf, "%s%d\n", buf, DISABLE);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ENABLE);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t sfp_tx_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int result = 0;
    int input  = 0;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_31_data = i2c_get_clientdata(Cameo_CPLD_31_client);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    struct i2c_client *target_client = NULL; 
    
    port_index = attr->index;
    input = simple_strtol(buf, NULL, 10);
    mutex_lock(&Cameo_CPLD_31_data->update_lock);
    mutex_lock(&Cameo_CPLD_32_data->update_lock);

    if (port_index >= 1 && port_index <= 32)
    {
        target_client = Cameo_CPLD_31_client;

    }
    else if (port_index >= 33 && port_index <= 48)
    {
        target_client = Cameo_CPLD_32_client;
    }
    
    status = i2c_smbus_read_byte_data(target_client, sfp_tx_enable_regs[port_index][0]);
    if( input == ENABLE)
    {
        status |= sfp_tx_enable_regs[port_index][1];
        result = i2c_smbus_write_byte_data(target_client, sfp_tx_enable_regs[port_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: sfp_tx_enable_set ON FAILED!\n");
        }
    }
    else if( input == DISABLE)
    {
        status &= ~(sfp_tx_enable_regs[port_index][1]);
        result = i2c_smbus_write_byte_data(target_client, sfp_tx_enable_regs[port_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: sfp_tx_enable_set OFF FAILED!\n");
        }
    }
    else
    {
        printk(KERN_ALERT "ERROR: sfp_tx_enable_set WRONG VALUE\n");
    }
    
    mutex_unlock(&Cameo_CPLD_31_data->update_lock);
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    
    return count;
}

ssize_t sfp_rx_loss_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status   = -EPERM;
    u64 result  = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == SFP_RX_LOSS_ALL)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, SFP_41_48_RX_LOSS_REG); //41-48
        result = status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, SFP_33_40_RX_LOSS_REG); //33-40
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_25_32_RX_LOSS_REG); //25-32
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_17_24_RX_LOSS_REG); //17-24
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_9_16_RX_LOSS_REG);  //9-16
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_1_8_RX_LOSS_REG);   //1-8
        result = (result << 8) | status;
        result = (~(result)) & 0xFFFFFFFFFFFF;
        sprintf(buf, "%s0x%llx\n", buf, result);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t sfp_rx_loss_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    port_index = attr->index;
    sprintf(buf, "");

    if (port_index >= 1 && port_index <= 32)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, sfp_rx_loss[port_index][0]);
    }
    else if (port_index >= 33 && port_index <= 48)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, sfp_rx_loss[port_index][0]);
    }
    
    if (status & sfp_rx_loss [port_index][1])
    {
        sprintf(buf, "%s%d\n", buf, DISABLE);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ENABLE);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t sfp_present_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status   = -EPERM;
    u64 result  = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == SFP_PRESENT_ALL)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, SFP_41_48_PRESENT_REG); //41-48
        result = status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, SFP_33_40_PRESENT_REG); //33-40
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_25_32_PRESENT_REG); //25-32
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_17_24_PRESENT_REG); //17-24
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_9_16_PRESENT_REG);  //9-16
        result = (result << 8) | status;
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SFP_1_8_PRESENT_REG);   //1-8
        result = (result << 8) | status;
        result = (~(result)) & 0xFFFFFFFFFFFF;
        sprintf(buf, "%s0x%llx\n", buf, result);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t sfp_present_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int port_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    port_index = attr->index;
    sprintf(buf, "");

    if (port_index >= 1 && port_index <= 32)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, sfp_present_regs[port_index][0]);
    }
    else if (port_index >= 33 && port_index <= 48)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, sfp_present_regs[port_index][0]);
    }
    
    if (status & sfp_present_regs[port_index][1])
    {
        sprintf(buf, "%s%d\n", buf, DISABLE);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ENABLE);
    }
    
    return sprintf(buf, "%s\n", buf);
}

ssize_t sfp_rx_loss_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int int_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    int_index = attr->index;
    sprintf(buf, "");

    if (int_index >= 1 && int_index <= 4)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, sfp_rx_loss_int_regs[int_index][0]);
    }
    else if (int_index >= 5 && int_index <= 6)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, sfp_rx_loss_int_regs[int_index][0]);
    }
    
    if (status & sfp_rx_loss_int_regs[int_index][1])
    {
        sprintf(buf, "%s%d\n", buf, NORMAL);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ABNORMAL);
    }
    
    return sprintf(buf, "%s\n", buf);
}
ssize_t sfp_rx_loss_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int int_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    int_index = attr->index;
    sprintf(buf, "");

    if (int_index >= 1 && int_index <= 4)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, sfp_rx_loss_int_mask_regs[int_index][0]);
    }
    else if (int_index >= 5 && int_index <= 6)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, sfp_rx_loss_int_mask_regs[int_index][0]);
    }
    
    if (status & sfp_rx_loss_int_mask_regs[int_index][1])
    {
        sprintf(buf, "%s%d\n", buf, NORMAL);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ABNORMAL);
    }
    
    return sprintf(buf, "%s\n", buf);
}
ssize_t sfp_rx_loss_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int result = 0;
    int input  = 0;
    int quter_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_31_data = i2c_get_clientdata(Cameo_CPLD_31_client);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    struct i2c_client *target_client = NULL; 
    
    quter_index = attr->index;
    input = simple_strtol(buf, NULL, 10);
    mutex_lock(&Cameo_CPLD_31_data->update_lock);
    mutex_lock(&Cameo_CPLD_32_data->update_lock);

    if (quter_index >= 1 && quter_index <= 4)
    {
        target_client = Cameo_CPLD_31_client;

    }
    else if (quter_index >= 5 && quter_index <= 6)
    {
        target_client = Cameo_CPLD_32_client;
    }
    
    status = i2c_smbus_read_byte_data(target_client, sfp_rx_loss_int_mask_regs[quter_index][0]);
    if( input == DISABLE)
    {
        status |= sfp_rx_loss_int_mask_regs[quter_index][1];
        result = i2c_smbus_write_byte_data(target_client, sfp_rx_loss_int_mask_regs[quter_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: sfp_rx_loss_int_mask_set ON FAILED!\n");
        }
    }
    else if( input == ENABLE)
    {
        status &= ~(sfp_rx_loss_int_mask_regs[quter_index][1]);
        result = i2c_smbus_write_byte_data(target_client, sfp_rx_loss_int_mask_regs[quter_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: sfp_rx_loss_int_mask_set OFF FAILED!\n");
        }
    }
    else
    {
        printk(KERN_ALERT "ERROR: sfp_rx_loss_int_mask_set WRONG VALUE\n");
    }
    
    mutex_unlock(&Cameo_CPLD_31_data->update_lock);
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    
    return count;
}
ssize_t sfp_present_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int int_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    int_index = attr->index;
    sprintf(buf, "");

    if (int_index >= 1 && int_index <= 4)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, sfp_present_int_regs[int_index][0]);
    }
    else if (int_index >= 5 && int_index <= 6)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, sfp_present_int_regs[int_index][0]);
    }
    
    if (status & sfp_present_int_regs[int_index][1])
    {
        sprintf(buf, "%s%d\n", buf, NORMAL);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ABNORMAL);
    }
    
    return sprintf(buf, "%s\n", buf);
}
ssize_t sfp_present_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    int int_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    int_index = attr->index;
    sprintf(buf, "");

    if (int_index >= 1 && int_index <= 4)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, sfp_present_int_mask_regs[int_index][0]);
    }
    else if (int_index >= 5 && int_index <= 6)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_32_client, sfp_present_int_mask_regs[int_index][0]);
    }
    
    if (status & sfp_present_int_mask_regs[int_index][1])
    {
        sprintf(buf, "%s%d\n", buf, NORMAL);
    }
    else
    {
        sprintf(buf, "%s%d\n", buf, ABNORMAL);
    }
    
    return sprintf(buf, "%s\n", buf);
}
ssize_t sfp_present_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int result = 0;
    int input  = 0;
    int quter_index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Cameo_CPLD_31_data = i2c_get_clientdata(Cameo_CPLD_31_client);
	struct Cameo_i2c_data *Cameo_CPLD_32_data = i2c_get_clientdata(Cameo_CPLD_32_client);
    struct i2c_client *target_client = NULL; 
    
    quter_index = attr->index;
    input = simple_strtol(buf, NULL, 10);
    mutex_lock(&Cameo_CPLD_31_data->update_lock);
    mutex_lock(&Cameo_CPLD_32_data->update_lock);

    if (quter_index >= 1 && quter_index <= 4)
    {
        target_client = Cameo_CPLD_31_client;

    }
    else if (quter_index >= 5 && quter_index <= 6)
    {
        target_client = Cameo_CPLD_32_client;
    }
    
    status = i2c_smbus_read_byte_data(target_client, sfp_present_int_mask_regs[quter_index][0]);
    if( input == DISABLE)
    {
        status |= sfp_present_int_mask_regs[quter_index][1];
        result = i2c_smbus_write_byte_data(target_client, sfp_present_int_mask_regs[quter_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: sfp_present_int_mask_set ON FAILED!\n");
        }
    }
    else if( input == ENABLE)
    {
        status &= ~(sfp_present_int_mask_regs[quter_index][1]);
        result = i2c_smbus_write_byte_data(target_client, sfp_present_int_mask_regs[quter_index][0], status);
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: sfp_present_int_mask_set OFF FAILED!\n");
        }
    }
    else
    {
        printk(KERN_ALERT "ERROR: sfp_present_int_mask_set WRONG VALUE\n");
    }
    
    mutex_unlock(&Cameo_CPLD_31_data->update_lock);
    mutex_unlock(&Cameo_CPLD_32_data->update_lock);
    
    return count;
}
/* end of implement i2c_function */