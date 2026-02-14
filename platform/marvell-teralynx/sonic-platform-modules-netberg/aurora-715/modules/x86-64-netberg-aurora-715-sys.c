/* An hwmon driver for Netberg Aurora 715 Marvell Teralynx i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-netberg-aurora-715.h"
#include "x86-64-netberg-aurora-715-common.h"
#include "x86-64-netberg-aurora-715-sys.h"

/* extern i2c_client */
extern struct i2c_client *Netberg_CPLD_30_client; //0x30 for SYS CPLD
extern struct i2c_client *Netberg_CPLD_31_client; //0x31 for Port 01-16
extern struct i2c_client *Netberg_CPLD_32_client; //0x32 for Port 17-32
extern struct i2c_client *Netberg_CPLD_23_client; //0x23 for Fan CPLD
extern struct i2c_client *Netberg_BMC_14_client;  //0x14 for BMC slave
extern struct i2c_client *Netberg_EEPROM_56_client;  //0x56 for EEPROM slave
/* end of extern i2c_client */

/* implement i2c_function */
ssize_t cpld_hw_ver_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case 23:
            if( bmc_enable() == ENABLE)
            {
                status = i2c_smbus_read_byte_data(Netberg_BMC_14_client, 0xff);
            }
            else
            {
                status = i2c_smbus_read_byte_data(Netberg_CPLD_23_client, CPLD_VER_REG);
            }
        case 30:
            status = i2c_smbus_read_byte_data(Netberg_CPLD_30_client, CPLD_VER_REG);
        case 31:
            status = i2c_smbus_read_byte_data(Netberg_CPLD_31_client, CPLD_VER_REG);
        case 32:
            status = i2c_smbus_read_byte_data(Netberg_CPLD_32_client, CPLD_VER_REG);
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
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == WDT_EN)
    {
        if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, WDT_EN_REG) & BIT_4_MASK)
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
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Netberg_CPLD_30_client, WDT_EN_REG);
    if (attr->index == WDT_EN)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == ENABLE)
        {
            value = status | WDT_EN_ENABLE;
            result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, WDT_EN_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: wdt_enable_set FAILED!\n");
            }
        }
        else if (input == DISABLE)
        {
            value = status & WDT_EN_DISABLE;
            result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, WDT_EN_REG, value);
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
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == EEPROM_WP)
    {
        if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, EEPROM_WP_REG) & BIT_2_MASK)
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
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Netberg_CPLD_30_client, EEPROM_WP_REG);
    if (attr->index == EEPROM_WP)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == ENABLE)
        {
            value = status | EEPROM_WP_ENABLE;
            result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, EEPROM_WP_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: eeprom_wp_set FAILED!\n");
            }
        }
        else if (input == DISABLE)
        {
            value = status & EEPROM_WP_DISABLE;
            result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, EEPROM_WP_REG, value);
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

static void net_eeprom_update_client(struct i2c_client *client, u8 slice)
{
        struct eeprom_data *data = i2c_get_clientdata(client);
        int i, j;
        int ret;
        int addr;


        mutex_lock(&data->update_lock);

        if (!(data->valid & (1 << slice)) ||
            time_after(jiffies, data->last_updated[slice] + 300 * HZ)) {
                dev_dbg(&client->dev, "Starting eeprom update, slice %u\n", slice);

                addr = slice << SLICE_BITS;

                ret = i2c_smbus_write_byte_data(client, ((u8)addr >> 8) & 0xFF, (u8)addr & 0xFF);
                /* select the eeprom address */
                if (ret < 0) {
                        dev_err(&client->dev, "address set failed\n");
                        goto exit;
                }

                if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_BYTE)) {
                        goto exit;
                }

                for (i = slice << SLICE_BITS; i < (slice + 1) << SLICE_BITS; i+= SLICE_SIZE) {
                        for (j = i; j < (i+SLICE_SIZE); j++) {
                                int res;

                                res = i2c_smbus_read_byte(client);
                                if (res < 0) {
                                        goto exit;
                                }

                                data->data[j] = res & 0xFF;
                        }
                }

                data->last_updated[slice] = jiffies;
                data->valid |= (1 << slice);
        }

exit:
        mutex_unlock(&data->update_lock);
}


ssize_t net_eeprom_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *bin_attr,
                           char *buf, loff_t off, size_t count)
{
        /* struct i2c_client *client = to_i2c_client(container_of(kobj, struct device, kobj)); */
        struct eeprom_data *data = i2c_get_clientdata(Netberg_EEPROM_56_client);
        u8 slice;


        if (off > EEPROM_SIZE) {
                return 0;
        }
        if (off + count > EEPROM_SIZE) {
                count = EEPROM_SIZE - off;
        }
        if (count == 0) {
                return 0;
        }

        /* Only refresh slices which contain requested bytes */
        for (slice = off >> SLICE_BITS; slice <= (off + count - 1) >> SLICE_BITS; slice++) {
                net_eeprom_update_client(Netberg_EEPROM_56_client, slice);
        }

        memcpy(buf, &data->data[off], count);

        return count;
}

ssize_t usb_enable_get(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == USB_EN)
    {
        if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, USB_EN_REG) & BIT_2_MASK)
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
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Netberg_CPLD_30_client, USB_EN_REG);
    if (attr->index == USB_EN)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == ENABLE)
        {
            value = status | USB_EN_ENABLE;
            result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, USB_EN_REG, value);
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: usb_enable_set FAILED!\n");
            }
        }
        else if (input == DISABLE)
        {
            value = status & USB_EN_DISABLE;
            result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, USB_EN_REG, value);
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
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Netberg_CPLD_30_client, MAC_RESET_REG);
    if (attr->index == RESET)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == MAC_RESET)
        {
            value = MAC_RESET;
            result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, MAC_RESET_REG, value);
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
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SHUTDOWN_REG);
    if (attr->index == SHUTDOWN_SET)
    {
        input = simple_strtol(buf, NULL, 10);
        if (input == SHUTDOWN)
        {
            value = status | SHUTDOWN;
            result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, SHUTDOWN_REG, value);
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
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    if (attr->index == BMC_PRESENT)
    {
        if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, BMC_EN_REG) & BIT_0_MASK)
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

ssize_t themal_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case TEMP_R_B_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, THERMAL_INT_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case TEMP_L_B_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, THERMAL_INT_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case TEMP_L_T_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, THERMAL_INT_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case TEMP_R_T_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, THERMAL_INT_REG) & BIT_3_MASK)
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

ssize_t themal_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case TEMP_R_B_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, THERMAL_INT_MASK_REG) & BIT_0_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case TEMP_L_B_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, THERMAL_INT_MASK_REG) & BIT_1_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case TEMP_L_T_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, THERMAL_INT_MASK_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case TEMP_R_T_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, THERMAL_INT_MASK_REG) & BIT_3_MASK)
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

ssize_t themal_int_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int status = -EPERM;
    int value  = -EPERM;
    int result = -EPERM;
    int input;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Netberg_CPLD_30_client, THERMAL_INT_MASK_REG);
    
    input = simple_strtol(buf, NULL, 10);
    switch (attr->index)
    {
        case TEMP_R_B_INT_MASK:
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
                printk(KERN_ALERT "themal_int_mask_set wrong Value\n");
                return count;
            }
            break;
        case TEMP_L_B_INT_MASK:
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
                printk(KERN_ALERT "themal_int_mask_set wrong Value\n");
                return count;
            }
            break;
        case TEMP_L_T_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x04;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfb;
            }
            else
            {
                printk(KERN_ALERT "themal_int_mask_set wrong Value\n");
                return count;
            }
            break;
        case TEMP_R_T_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x08;
            }
            else if (input == DISABLE)
            {
                value = status & 0xf7;
            }
            else
            {
                printk(KERN_ALERT "themal_int_mask_set wrong Value\n");
                return count;
            }
            break;
    }
    result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, THERMAL_INT_MASK_REG, value);
    mutex_unlock(&data->update_lock);
    return count;
}

ssize_t sys_int_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case CPLD_FP_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case CPLD_RP_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_REG) & BIT_3_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case CPLD_FAN_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_REG) & BIT_4_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case CPLD_PSU_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_REG) & BIT_5_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case THERMAL_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_REG) & BIT_6_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case USB_INT:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_REG) & BIT_7_MASK)
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

ssize_t sys_int_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int result = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    sprintf(buf, "");
    switch (attr->index)
    {
        case CPLD_FP_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_MASK_REG) & BIT_2_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case CPLD_RP_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_MASK_REG) & BIT_3_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case CPLD_FAN_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_MASK_REG) & BIT_4_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case CPLD_PSU_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_MASK_REG) & BIT_5_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case THERMAL_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_MASK_REG) & BIT_6_MASK)
            {
                result = ENABLE;
            }
            else
            {
                result = DISABLE;
            }
            break;
        case USB_INT_MASK:
            if (i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_MASK_REG) & BIT_7_MASK)
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
	struct Netberg_i2c_data *data = i2c_get_clientdata(Netberg_CPLD_30_client);
    
    mutex_lock(&data->update_lock);
    status = i2c_smbus_read_byte_data(Netberg_CPLD_30_client, SYS_INT_MASK_REG);
    
    input = simple_strtol(buf, NULL, 10);
    switch (attr->index)
    {
        case CPLD_FP_INT_MASK:
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
        case CPLD_RP_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x04;
            }
            else if (input == DISABLE)
            {
                value = status & 0xfb;
            }
            else
            {
                printk(KERN_ALERT "sys_int_mask_set wrong Value\n");
                return count;
            }
            break;
        case CPLD_FAN_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x08;
            }
            else if (input == DISABLE)
            {
                value = status & 0xf7;
            }
            else
            {
                printk(KERN_ALERT "sys_int_mask_set wrong Value\n");
                return count;
            }
            break;
        case CPLD_PSU_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x10;
            }
            else if (input == DISABLE)
            {
                value = status & 0xef;
            }
            else
            {
                printk(KERN_ALERT "sys_int_mask_set wrong Value\n");
                return count;
            }
            break;
        case THERMAL_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x20;
            }
            else if (input == DISABLE)
            {
                value = status & 0xdf;
            }
            else
            {
                printk(KERN_ALERT "sys_int_mask_set wrong Value\n");
                return count;
            }
            break;
        case USB_INT_MASK:
            if (input == ENABLE)
            {
                value = status | 0x40;
            }
            else if (input == DISABLE)
            {
                value = status & 0xbf;
            }
            else
            {
                printk(KERN_ALERT "sys_int_mask_set wrong Value\n");
                return count;
            }
            break;
    }
    result = i2c_smbus_write_byte_data(Netberg_CPLD_30_client, SYS_INT_MASK_REG, value);
    mutex_unlock(&data->update_lock);
    return count;
}
/* end of implement i2c_function */
