/* An hwmon driver for Cameo esc600-128Q Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-esc600-128q.h"
#include "x86-64-cameo-esc600-128q-common.h"
#include "x86-64-cameo-esc600-128q-sys.h"

/* extern i2c_client */
extern struct i2c_client *Cameo_CPLD_30_client; //0x30 CPLD ,XO2-2000HC-4FTG256C
extern struct i2c_client *Cameo_CPLD_31_client; //0x31 CPLD ,XO2-7000HC-4TG144C
extern struct i2c_client *Cameo_CPLD_33_client; //0x33 I/O Board CPLD ,XO2-640
extern struct i2c_client *Cameo_BMC_14_client;  //0x14 BMC ,Aspeed
/* end of extern i2c_client */

/* implement i2c_function */
ssize_t cpld_hw_ver_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case 30:
            status = i2c_smbus_read_byte_data(Cameo_CPLD_30_client, CPLD_VER_REG);
			break;
        case 31:
            status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, CPLD_VER_REG);
			break;
        case 33:
            status = i2c_smbus_read_byte_data(Cameo_CPLD_33_client, CPLD_VER_REG);
			break;
    }
    if(status < 0)
    {
        mutex_unlock(&data->update_lock);
        return status;
    }
    else
    {
        mutex_unlock(&data->update_lock);
        sprintf(buf, "%s0x%x\n", buf, status);
    }
    return sprintf(buf, "%s\n", buf);
}

ssize_t wdt_enable_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == WDT_EN)
    {
        if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, WDT_EN_REG) & BIT_4_MASK)
        {
            sprintf(buf, "%s%d\n", buf, ENABLE);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, DISABLE);
        }
    }
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%s\n", buf);
}

ssize_t wdt_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_30_client, WDT_EN_REG);
    if (attr->index == WDT_EN)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == ENABLE)
        {
            value = status | WDT_EN_ENABLE;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_30_client, WDT_EN_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: wdt_enable_set FAILED!\n");
            }
        }
        else if (input == DISABLE)
        {
            value = status & WDT_EN_DISABLE;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_30_client, WDT_EN_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: wdt_enable_set FAILED!\n");
            }
        }
        else
        {
            printk(KERN_ALERT "wdt_enable_set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t eeprom_wp_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == EEPROM_WP)
    {
        if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, EEPROM_WP_REG) & BIT_4_MASK)
        {
            sprintf(buf, "%s%d\n", buf, ENABLE);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, DISABLE);
        }
    }
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%s\n", buf);
}

ssize_t eeprom_wp_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, EEPROM_WP_REG);
    if (attr->index == EEPROM_WP)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == ENABLE)
        {
            value = status | EEPROM_WP_ENABLE;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_31_client, EEPROM_WP_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: eeprom_wp_set FAILED!\n");
            }
        }
        else if (input == DISABLE)
        {
            value = status & EEPROM_WP_DISABLE;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_31_client, EEPROM_WP_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: eeprom_wp_set FAILED!\n");
            }
        }
        else
        {
            printk(KERN_ALERT "eeprom_wp_set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t usb_enable_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == USB_EN)
    {
        if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, USB_EN_REG) & BIT_0_MASK)
        {
            sprintf(buf, "%s%d\n", buf, ENABLE);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, DISABLE);
        }
    }
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%s\n", buf);
}

ssize_t usb_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_30_client, USB_EN_REG);
    if (attr->index == USB_EN)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == ENABLE)
        {
            value = status | USB_EN_ENABLE;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_30_client, USB_EN_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: usb_enable_set FAILED!\n");
            }
        }
        else if (input == DISABLE)
        {
            value = status & USB_EN_DISABLE;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_30_client, USB_EN_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: usb_enable_set FAILED!\n");
            }
        }
        else
        {
            printk(KERN_ALERT "usb_enable_set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t reset_mac_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MAC_RESET_REG);
    if (attr->index == RESET)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == MAC_RESET)
        {
            value = MAC_RESET;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_30_client, MAC_RESET_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: reset_mac_set FAILED!\n");
            }
        }
        else
        {
            printk(KERN_ALERT "reset_mac_set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t shutdown_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SHUTDOWN_REG);
    if (attr->index == SHUTDOWN_SET)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == SHUTDOWN)
        {
            value = status | SHUTDOWN;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_30_client, SHUTDOWN_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: shutdown_set FAILED!\n");
            }
        }
        else
        {
            printk(KERN_ALERT "shutdown_set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t bmc_enable_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == BMC_PRESENT)
    {
        if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, BMC_EN_REG) & BIT_0_MASK)
        {
            sprintf(buf, "%s%d\n", buf, ENABLE);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, DISABLE);
        }
    }
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%s\n", buf);
}

ssize_t switch_alarm_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case SW_ALERT_TH0:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SW_ALARM_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SW_ALERT_TH1:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SW_ALARM_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SW_ALERT_TH2:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SW_ALARM_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SW_ALERT_TH3:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SW_ALARM_REG) & BIT_3_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t switch_alarm_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case SW_ALERT_TH0_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SW_ALERT_MASK_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SW_ALERT_TH1_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SW_ALERT_MASK_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SW_ALERT_TH2_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SW_ALERT_MASK_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SW_ALERT_TH3_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SW_ALERT_MASK_REG) & BIT_3_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t switch_alarm_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SW_ALERT_MASK_REG);
    
    input = simple_strtol(buf, NULL, 10);
    if((input != 0) && (input != 1) )
    {
	    mutex_unlock(&data->update_lock);
        printk(KERN_ALERT "switch_alarm_mask_set wrong Value\n");
        return count;
    }
    switch (attr->index)
    {
        case SW_ALERT_TH0_MASK:
            if (input == ENABLE)
            {
                value = status | 0x01;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfe;
            }
            break;
        case SW_ALERT_TH1_MASK:
            if (input == ENABLE)
            {
                value = status | 0x02;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfd;
            }
            break;
        case SW_ALERT_TH2_MASK:
            if (input == ENABLE)
            {
                value = status | 0x04;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfb;
            }
            break;
        case SW_ALERT_TH3_MASK:
            if (input == ENABLE)
            {
                value = status | 0x08;
            }
            else if (input == DISABLE)
            {
                value = status & 0xf7;
            }
            break;
    }
    result = i2c_smbus_write_byte_data(Cameo_CPLD_31_client, SW_ALERT_MASK_REG, value);
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t sensor_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute  *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data           *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case CB_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SENSOR_INT_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SB_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SENSOR_INT_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t sersor_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case CB_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SENSOR_INT_MASK_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SB_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SENSOR_INT_MASK_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t sersor_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, SENSOR_INT_MASK_REG);
    
    input = simple_strtol(buf, NULL, 10);
    switch (attr->index)
    {
        case CB_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x01;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfe;
            }
            else
            {
                printk(KERN_ALERT "sys_int_mask_set wrong Value\n");
                return count;
            }
            break;
        case SB_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x02;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfd;
            }
            else
            {
                printk(KERN_ALERT "sys_int_mask_set wrong Value\n");
                return count;
            }
            break;
    }
    result = i2c_smbus_write_byte_data(Cameo_CPLD_31_client, SENSOR_INT_MASK_REG, value);
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t module_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 result = 0;
    int card_num = 0;
    int input = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct Cameo_i2c_data *Cameo_CPLD_30_data = i2c_get_clientdata(Cameo_CPLD_30_client);

    if (attr->index == MODULE_RESET)
    {
        input = simple_strtol(buf, NULL, 10); //get input module number
        if(input <= 0 || input > 8)
        {
            printk(KERN_ALERT "ERROR: module_reset_%d RESET FAILED!\n", input);
        }
        else
        {
            mutex_lock(&Cameo_CPLD_30_data->update_lock);
            status = i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_RESET_REG); //to get register 0x30 0xa2
            status &= ~(1 << (input-1));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_30_client, MODULE_RESET_REG, status); //to set register 0x30 0xa2
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: module_reset_%d RESET FAILED!\n", card_num);
            }
        }
    }
    mutex_unlock(&Cameo_CPLD_30_data->update_lock);
    return count;
}

ssize_t module_insert_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case MODULE_1_PRESENT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_PRESENT_REG) & BIT_0_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_2_PRESENT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_PRESENT_REG) & BIT_1_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_3_PRESENT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_PRESENT_REG) & BIT_2_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_4_PRESENT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_PRESENT_REG) & BIT_3_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_5_PRESENT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_PRESENT_REG) & BIT_4_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_6_PRESENT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_PRESENT_REG) & BIT_5_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_7_PRESENT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_PRESENT_REG) & BIT_6_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_8_PRESENT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_PRESENT_REG) & BIT_7_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t module_power_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case MODULE_1_POWER:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_POWER_REG) & BIT_0_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_2_POWER:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_POWER_REG) & BIT_1_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_3_POWER:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_POWER_REG) & BIT_2_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_4_POWER:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_POWER_REG) & BIT_3_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_5_POWER:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_POWER_REG) & BIT_4_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_6_POWER:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_POWER_REG) & BIT_5_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_7_POWER:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_POWER_REG) & BIT_6_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_8_POWER:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_POWER_REG) & BIT_7_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t module_enable_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case MODULE_1_ENABLE:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG) & BIT_0_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_2_ENABLE:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG) & BIT_1_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_3_ENABLE:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG) & BIT_2_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_4_ENABLE:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG) & BIT_3_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_5_ENABLE:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG) & BIT_4_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_6_ENABLE:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG) & BIT_5_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_7_ENABLE:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG) & BIT_6_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case MODULE_8_ENABLE:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG) & BIT_7_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t module_enable_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG);
    
    input = simple_strtol(buf, NULL, 10);
    if((input != 0) && (input != 1) )
    {
	    mutex_unlock(&data->update_lock);
        printk(KERN_ALERT "module_enable_set wrong Value\n");
        return count;
    }
    switch (attr->index)
    {
        case MODULE_1_ENABLE:
            if (input == ENABLE)
            {
                value = status | 0x01;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfe;
            }
            break;
        case MODULE_2_ENABLE:
            if (input == ENABLE)
            {
                value = status | 0x02;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfd;
            }
            break;
        case MODULE_3_ENABLE:
            if (input == ENABLE)
            {
                value = status | 0x04;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfb;
            }
            break;
        case MODULE_4_ENABLE:
            if (input == ENABLE)
            {
                value = status | 0x08;
            }
            else if (input == DISABLE)
            {
                value = status & 0xf7;
            }
            break;
        case MODULE_5_ENABLE:
            if (input == ENABLE)
            {
                value = status | 0x10;
            }
            else if (input == DISABLE)
            {
                value = status & 0xef;
            }
            break;
        case MODULE_6_ENABLE:
            if (input == ENABLE)
            {
                value = status | 0x20;
            }
            else if (input == DISABLE)
            {
                value = status & 0xdf;
            }
            break;
        case MODULE_7_ENABLE:
            if (input == ENABLE)
            {
                value = status | 0x40;
            }
            else if (input == DISABLE)
            {
                value = status & 0xbf;
            }
            break;
        case MODULE_8_ENABLE:
            if (input == ENABLE)
            {
                value = status | 0x80;
            }
            else if (input == DISABLE)
            {
                value = status & 0x7f;
            }
            break;
    }
    result = i2c_smbus_write_byte_data(Cameo_CPLD_30_client, MODULE_ENABLE_REG, value);
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t switch_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case MODULE_INS_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case MODULE_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case MODULE_POWER_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case THER_SENSOR_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_REG) & BIT_3_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case IO_BOARD_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_REG) & BIT_4_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case FAN_ERROR_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_REG) & BIT_5_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case PHY_POWER_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_REG) & BIT_6_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SW_POWER_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_REG) & BIT_7_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t switch_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case MODULE_INS_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case MODULE_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case MODULE_POW_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case THER_SEN_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG) & BIT_3_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case IO_BOARD_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG) & BIT_4_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case FAN_ERROR_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG) & BIT_5_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case PHY_POWER_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG) & BIT_6_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SW_POWER_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG) & BIT_7_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t switch_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG);
    
    input = simple_strtol(buf, NULL, 10);
    if((input != 0) && (input != 1) )
    {
        mutex_unlock(&data->update_lock);
        printk(KERN_ALERT "switch_int_mask_set wrong Value\n");
        return count;
    }
    switch (attr->index)
    {
        case MODULE_INS_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x01;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfe;
            }
            break;
        case MODULE_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x02;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfd;
            }
            break;
        case MODULE_POW_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x04;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfb;
            }
            break;
        case THER_SEN_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x08;
            }
            else if (input == DISABLE)
            {
                value = status & 0xf7;
            }
            break;
        case IO_BOARD_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x10;
            }
            else if (input == DISABLE)
            {
                value = status & 0xef;
            }
            break;
        case FAN_ERROR_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x20;
            }
            else if (input == DISABLE)
            {
                value = status & 0xdf;
            }
            break;
        case PHY_POWER_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x40;
            }
            else if (input == DISABLE)
            {
                value = status & 0xbf;
            }
            break;
        case SW_POWER_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x80;
            }
            else if (input == DISABLE)
            {
                value = status & 0x7f;
            }
            break;
    }
    result = i2c_smbus_write_byte_data(Cameo_CPLD_30_client, SWITCH_INT_MASK_REG, value);
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t sfp_select_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == SFP_SELECT)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_SELECT_REG); //to get register 0x33 0x60
        sprintf(buf, "");
        if (status & 0x1)
        {
            sprintf(buf, "%s%d\n", buf, SFP_PORT_1);
        }
        else if (status & 0x2)
        {
            sprintf(buf, "%s%d\n", buf, SFP_PORT_2);
        }
        else if (status & 0x3)
        {
            sprintf(buf, "%s%d\n", buf, SFP_PORT_MGM);
        }
        else
        {
            sprintf(buf, "%s%d\n", buf, SFP_NON_SELECT);
        }
    }
    return sprintf(buf, "%s", buf);
}

ssize_t sfp_select_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 value  = -EPERM;
    u8 result = -EPERM;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_33_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_SELECT_REG); //to get register 0x33 0x60
    if (attr->index == SFP_SELECT)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == 0)
        {
            value = 0x0;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_SELECT_REG, value); //to set register 0x33 0x60
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_select_set 0 FAILED!\n");
            }
        }
        else if (i == 1)
        {
            value = 0x1;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_SELECT_REG, value); //to set register 0x33 0x60
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_select_set 1 FAILED!\n");
            }
        }
        else if (i == 2)
        {
            value = 0x2;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_SELECT_REG, value); //to set register 0x33 0x60
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_select_set 2 FAILED!\n");
            }
        }
        else if (i == 3)
        {
            value = 0x3;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_SELECT_REG, value); //to set register 0x33 0x60
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_select_set 3 FAILED!\n");
            }
        }
        else
        {
            printk(KERN_ALERT "SFP_SELECT set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t sfp_tx_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_33_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case SFP_PORT_TX_1:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_TX_REG) & BIT_0_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case SFP_PORT_TX_2:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_TX_REG) & BIT_1_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case SFP_PORT_TX_MGM:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_TX_REG) & BIT_2_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t sfp_tx_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 value  = -EPERM;
    u8 result = -EPERM;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_33_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_TX_REG); //to get register 0x33 0xa0
    if (attr->index)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == SFP_PORT_1_OFF) //i = 1 SFP_PORT_1 OFF
        {
            value = status | 0x1;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_TX_REG, value); //to set register 0x33 0xa0
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_tx_set PORT_1 OFF FAILED!\n");
            }
        }
        else if (i == SFP_PORT_1_ON) //i = 2 SFP_PORT_1 ON
        {
            value = status & 0xfe;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_TX_REG, value); //to set register 0x33 0xa0
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_tx_set PORT_1 ON FAILED!\n");
            }
        }
        else if (i == SFP_PORT_2_OFF) //i = 3 SFP_PORT_2 OFF
        {
            value = status | 0x2;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_TX_REG, value); //to set register 0x33 0xa0
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_tx_set PORT_2 OFF FAILED!\n");
            }
        }
        else if (i == SFP_PORT_2_ON) //i = 4 SFP_PORT_2 ON
        {
            value = status & 0xfd;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_TX_REG, value); //to set register 0x33 0xa0
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_tx_set PORT_2 ON FAILED!\n");
            }
        }
        else if (i == SFP_PORT_MGM_OFF) //i = 5 SFP_PORT_MGM OFF
        {
            value = status | 0x4;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_TX_REG, value); //to set register 0x33 0xa0
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_tx_set MGM OFF FAILED!\n");
            }
        }
        else if (i == SFP_PORT_MGM_ON) //i = 6 SFP_PORT_MGM ON
        {
            value = status & 0xfb;
            result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SFP_TX_REG, value); //to set register 0x33 0xa0
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: sfp_tx_set MGM ON FAILED!\n");
            }
        }
        else
        {
            printk(KERN_ALERT "SFP_TX set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t sfp_insert_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_33_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case SFP_PORT_RX_1:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_PRESENT_REG) & BIT_0_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case SFP_PORT_RX_2:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_PRESENT_REG) & BIT_1_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case SFP_PORT_RX_MGM:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_PRESENT_REG) & BIT_2_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t sfp_rx_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_33_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case SFP_PORT_RX_1:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_RX_REG) & BIT_0_MASK)
            {
                result = FALSE;
            }
            else
            {
                result = TRUE;
            }
            break;
        case SFP_PORT_RX_2:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_RX_REG) & BIT_1_MASK)
            {
                result = FALSE;
            }
            else
            {
                result = TRUE;
            }
            break;
        case SFP_PORT_RX_MGM:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_RX_REG) & BIT_2_MASK)
            {
                result = FALSE;
            }
            else
            {
                result = TRUE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t sys_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_33_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case SFP_LOSS_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_INT_REG) & BIT_0_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
        case SFP_ABS_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SFP_INT_REG) & BIT_1_MASK)
            {
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t sys_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_33_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case SFP_LOSS_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SYS_INT_MASK_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case SFP_ABS_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SYS_INT_MASK_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t sys_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_33_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_33_client, SYS_INT_MASK_REG);
    
    input = simple_strtol(buf, NULL, 10);
    if((input != 0) && (input != 1) )
    {
        mutex_unlock(&data->update_lock);
        printk(KERN_ALERT "sys_int_mask_set wrong Value\n");
        return count;
    }
    switch (attr->index)
    {
        case SFP_LOSS_MASK:
            if (input == ENABLE)
            {
                value = status | 0x01;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfe;
            }
            break;
        case SFP_ABS_MASK:
            if (input == ENABLE)
            {
                value = status | 0x02;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfd;
            }
            break;
    }
    result = i2c_smbus_write_byte_data(Cameo_CPLD_33_client, SYS_INT_MASK_REG, value);
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t thermal_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case ALERT_TH0_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH1_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH2_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH3_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_REG) & BIT_3_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH4_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_REG) & BIT_4_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH5_INT:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_REG) & BIT_5_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t thermal_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case ALERT_TH0_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_MASK_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH1_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_MASK_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH2_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_MASK_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH3_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_MASK_REG) & BIT_3_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH4_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_MASK_REG) & BIT_4_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case ALERT_TH5_INT_MASK:
            if (i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_MASK_REG) & BIT_5_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
    }
    mutex_unlock(&data->update_lock);
    sprintf(buf, "%s%d\n", buf, result);
    return sprintf(buf, "%s\n", buf);
}

ssize_t thermal_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(Cameo_CPLD_31_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Cameo_CPLD_31_client, THERMAL_INT_MASK_REG);
    
    input = simple_strtol(buf, NULL, 10);
    if((input != 0) && (input != 1) )
    {
        mutex_unlock(&data->update_lock);
        printk(KERN_ALERT "switch_int_mask_set wrong Value\n");
        return count;
    }
    switch (attr->index)
    {
        case ALERT_TH0_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x01;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfe;
            }
            break;
        case ALERT_TH1_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x02;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfd;
            }
            break;
        case ALERT_TH2_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x04;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfb;
            }
            break;
        case ALERT_TH3_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x08;
            }
            else if (input == DISABLE)
            {
                value = status & 0xf7;
            }
            break;
        case ALERT_TH4_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x10;
            }
            else if (input == DISABLE)
            {
                value = status & 0xef;
            }
            break;
        case ALERT_TH5_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x20;
            }
            else if (input == DISABLE)
            {
                value = status & 0xdf;
            }
            break;
    }
    result = i2c_smbus_write_byte_data(Cameo_CPLD_31_client, THERMAL_INT_MASK_REG, value);
    mutex_unlock(&data->update_lock);
    return count;
}
/* end of implement i2c_function */