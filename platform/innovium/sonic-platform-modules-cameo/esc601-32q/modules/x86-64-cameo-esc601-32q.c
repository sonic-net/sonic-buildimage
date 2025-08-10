/* An hwmon driver for Cameo ESC601-32Q Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-esc601-32q.h"

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x30, 0x31, 0x32, I2C_CLIENT_END };
#ifdef EEPROM_WANTED
static u_int8_t eeprom[1024];
#endif
static int	debug = 0;

#if (defined THEMAL_WANTED)|| (defined ASPEED_BMC_WANTED)
int read_8bit_temp(u8 sign,u8 value)
{
    int result = 0;
    if(sign)
    {
        //printf("read_8bit_temp UP %d\n", value & 0x80);
        value = ~(value)+1;
        result = value;
        return result;
    }
    else
    {
        //printf("read_8bit_temp DOWN %d\n", value & 0x80);
        result = value;
        return result;
    }
}
#endif

/* i2c-0 function */
#ifdef EEPROM_WANTED
static ssize_t tlv_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u16 i = 0;
    u16 addr = 0;
    u16 ret = 0x0;
    u16 header_len = sizeof(tlvinfo_header_t);
    u16 tlvinfo_len = 0, hibyte = 0, lobyte = 0;

    sprintf(buf, "\n");
    debug_print((KERN_ALERT "tlv_status_get header_len = %d\n", header_len));
    for (i = 0; i < header_len; i++)
    {
        ret = i2c_smbus_read_byte_data(Cameo_EEPROM_client, addr);
        memcpy(&eeprom[i], &ret, 1);

        debug_print((KERN_ALERT "tlv_status_get data = %c\n", ret));
        debug_print((KERN_ALERT "tlv_status_get addr = 0x%X\n", addr));

        if(i == header_len - 2)
        {
            hibyte = i2c_smbus_read_byte_data(Cameo_EEPROM_client, addr);
            debug_print((KERN_ALERT "tlv_status_get hibyte = 0x%x\n", hibyte));
        }
        if(i == header_len - 1)
        {
            lobyte = i2c_smbus_read_byte_data(Cameo_EEPROM_client, addr);
            debug_print((KERN_ALERT "tlv_status_get lobyte = 0x%x\n", lobyte));
        }
        addr++;
    }
    
    tlvinfo_len = (hibyte << 8) | lobyte;
    debug_print((KERN_ALERT "tlv_status_get tlvinfo_len = 0x%x\n", tlvinfo_len));

    addr = header_len;
    for (i = 0; i < tlvinfo_len; i++)
    {
        ret = i2c_smbus_read_byte_data(Cameo_EEPROM_client, addr);
        memcpy(&eeprom[i + header_len], &ret, 1);

        debug_print((KERN_ALERT "tlv_status_get data = %c\n", ret));
        debug_print((KERN_ALERT "tlv_status_get addr = 0x%X\n", addr));

        addr++;
    }
    eeprom[header_len + tlvinfo_len]='\0';
    show_eeprom(eeprom, buf, tlvinfo_len);
    
    return sprintf(buf, "%s\n", buf);
}
#endif

static ssize_t psu_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 hw_ver = 0x10; // set 0x10 as default HW version
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    // masks for hw_ver > 0x10
    u8 PSU1_present_mask = 0x02;
    u8 PSU2_present_mask = 0x01;
    u8 PSU1_status_mask = 0x08;
    u8 PSU2_status_mask = 0x04;
    // get hw version
    hw_ver = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0x20);

    if(hw_ver < 0x10)
    {
        PSU1_present_mask = 0x01;
        PSU2_present_mask = 0x02;
        PSU1_status_mask = 0x04;
        PSU2_status_mask = 0x08;
    }
    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa0);

    debug_print((KERN_DEBUG "DEBUG : PSU_PRESENT status = %x\n",status));
    sprintf(buf, "");
    switch (attr->index)
    {
        case PSU_PRESENT:
            if(hw_ver < 0x10 && hw_ver != 0x0d)
            {
                sprintf(buf, "%sPSU 2 is %s\n", buf, (status&PSU2_present_mask)?"not present":"present");
                sprintf(buf, "%sPSU 1 is %s\n", buf, (status&PSU1_present_mask)?"not present":"present");
            } 
            else 
            {
                sprintf(buf, "%sPSU 2 is %s\n", buf, (status&PSU2_present_mask)?"present":"not present");
                sprintf(buf, "%sPSU 1 is %s\n", buf, (status&PSU1_present_mask)?"present":"not present");
            }

            break;
        case PSU_STATUS:
            sprintf(buf, "%sPSU 2 is %s\n", buf, (status&PSU2_status_mask)?"not power Good":"power Good");
            sprintf(buf, "%sPSU 1 is %s\n", buf, (status&PSU1_status_mask)?"not power Good":"power Good");

            break;
    }
    return sprintf(buf, "%s\n", buf);
}

#ifdef THEMAL_WANTED
static long read_reg_linear(s32 data)
{
	s16 exponent;
	s32 mantissa;
	long val;

    exponent = ((s16)data) >> 11;
    mantissa = ((s16)((data & 0x7ff) << 5)) >> 5;

	val = mantissa;
    val = val * 1000L;

	if (exponent >= 0)
		val <<= exponent;
	else
		val >>= -exponent;

	return val/1000;
}

static long read_reg_linear_1000(s32 data)
{
	s16 exponent;
	s32 mantissa;
	long val;

    exponent = ((s16)data) >> 11;
    mantissa = ((s16)((data & 0x7ff) << 5)) >> 5;

	val = mantissa;
    val = val * 1000L;

	if (exponent >= 0)
		val <<= exponent;
	else
		val >>= -exponent;

	return val;
}

static long read_reg_linear_auto(s8 mode, u16 data)
{
	s16 exponent;
	s32 mantissa;
	long val;

    exponent = ((s8)(mode << 3)) >> 3;
    mantissa = ((u16)data);

	val = mantissa;

    /*printk(KERN_ALERT "exponent= %d, mantissa= %d, val= %d\n",exponent,mantissa,val);*/
    
	if (exponent >= 0)
		val <<= exponent;
	else
		val >>= -exponent;

	return val*1000;
}
#endif

#ifdef PSU_DEBUG
static long read_reg_vid(s32 data)
{
	long val;
    val = (((data-1)*5)+250)*1000L;
	return val/1000;
}

static long read_reg_vid_10mv(s32 data)
{
	long val;
    val = (((data-1)*10)+500)*1000L;
	return val/1000;
}

static long read_reg_vid_13mv(s32 data)
{
	long val;
    val = ((((data-1)*1333)+65000)/100)*1000L;
	return val/1000;
}
#endif

#ifdef PSU_STAT_WANTED
static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static ssize_t psu_module_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 bmc_present = -EPERM;
    u8 module_num = 0;
    u8 psu_table [3][11] = 
    {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a},
        {0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a}
    };
    u32 psu_status [11] = {0};
    u8 mask = 0x1;
    u8 i = 0;
    u16 u16_val = 0;
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;  // lm-sensor uint: mV, mA, mC
    
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "\n");
    
    bmc_present = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa5);
    if (bmc_present & mask)
    {
        switch(attr->index)
        {
            case PSU_MODULE_1:
                module_num = 1;
                break;
            case PSU_MODULE_2:
                module_num = 2;
                break;
        }

        for(i = 0; i < 10; i ++)
        {
            u16_val = i2c_smbus_read_word_data(Cameo_BMC_client, psu_table[module_num][i]);
            /* word data with linear format */
            if (i != 2 && i != 8) {
                multiplier = 1000;
                if (i == 6 || i == 7) /* pin, pout */
                    multiplier = 1000000;   // lm-sensor unit: uW
                if ( i == 5 ) /* fan_speed */
                    multiplier = 1;
                
                exponent = two_complement_to_int(u16_val >> 11, 5, 0x1f);
                mantissa = two_complement_to_int(u16_val & 0x7ff, 11, 0x7ff);
                psu_status[i] = (exponent >= 0) ? ((mantissa << exponent)*multiplier) : \
                                                  (mantissa*multiplier / (1 << -exponent));
            }
        }
        /* vout mode */
        multiplier = 1000;
        u16_val = i2c_smbus_read_byte_data(Cameo_BMC_client, psu_table[module_num][10]);
        psu_status[10] = u16_val;   
        exponent = two_complement_to_int(u16_val & 0x1f, 5, 0x1f);
        /* vout */
        u16_val = i2c_smbus_read_word_data(Cameo_BMC_client, psu_table[module_num][2]);
        psu_status[2] = (exponent >= 0) ? ((u16_val << exponent)*multiplier) : \
                                          (u16_val*multiplier / (1 << -exponent));
        
        sprintf(buf, "%sPSU %d VIN          is %u\n", buf, module_num, psu_status[0]);
        sprintf(buf, "%sPSU %d IIN          is %u\n", buf, module_num, psu_status[1]);
        sprintf(buf, "%sPSU %d VOUT         is %u\n", buf, module_num, psu_status[2]);
        sprintf(buf, "%sPSU %d IOUT         is %u\n", buf, module_num, psu_status[3]);
        sprintf(buf, "%sPSU %d TEMP_1       is %u\n", buf, module_num, psu_status[4]);
        sprintf(buf, "%sPSU %d FAN_SPEED    is %u\n", buf, module_num, psu_status[5]);
        sprintf(buf, "%sPSU %d POUT         is %u\n", buf, module_num, psu_status[6]);
        sprintf(buf, "%sPSU %d PIN          is %u\n", buf, module_num, psu_status[7]);
        sprintf(buf, "%sPSU %d MFR_MODEL    is 0x%x\n", buf, module_num, psu_status[8]);
        sprintf(buf, "%sPSU %d MFR_IOUT_MAX is %u\n", buf, module_num, psu_status[9]);
        sprintf(buf, "%sPSU %d VMODE        is %u\n", buf, module_num, psu_status[10]);
    }
    else
    {
        sprintf(buf, "%sBMC Module is not present\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t dc_chip_switch_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 bmc_present = -EPERM;
    u8 dc_table [10] = {0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x9a};
    u16 dc_status [10] = {0}; 
    u8 mask = 0x1;
    u8 i = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "\n");
    if (attr->index == DC_CHIP_SWITCH)
    {
        bmc_present = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa5); //to get 0x31 0xa3
        if (bmc_present & mask)
        {
            for(i = 0; i < 9; i ++)
            {
                dc_status[i] = i2c_smbus_read_word_data(Cameo_BMC_client, dc_table[i]);
            }
            sprintf(buf, "%sTPS53681 0x6c 0xd4  is 0x%x\n", buf, dc_status[0]);
            sprintf(buf, "%sTPS53681 0x6c 0x8c  is 0x%x\n", buf, dc_status[1]);
            sprintf(buf, "%sTPS53681 0x6c 0x96  is 0x%x\n", buf, dc_status[2]);
            sprintf(buf, "%sTPS53681 0x6e 0xd4  is 0x%x\n", buf, dc_status[3]);
            sprintf(buf, "%sTPS53681 0x6e 0x8c  is 0x%x\n", buf, dc_status[4]);
            sprintf(buf, "%sTPS53681 0x6e 0x96  is 0x%x\n", buf, dc_status[5]);
            sprintf(buf, "%sTPS53681 0x70 0xd4  is 0x%x\n", buf, dc_status[6]);
            sprintf(buf, "%sTPS53681 0x70 0x8c  is 0x%x\n", buf, dc_status[7]);
            sprintf(buf, "%sTPS53681 0x70 0x96  is 0x%x\n", buf, dc_status[8]);
            sprintf(buf, "%sTPS53681 0x70 0x96  is 0x%x\n", buf, dc_status[9]);
        }
        else
        {
            sprintf(buf, "%sBMC Module is not present\n", buf);
        }
    }
    return sprintf(buf, "%s\n", buf);
}

#endif

#ifdef USB_CTRL_WANTED
static ssize_t usb_power_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 res = 0x1;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa2);
    debug_print((KERN_DEBUG "DEBUG : USB_POWER status = %x\n",status));
    sprintf(buf, "");
    if (attr->index == USB_POWER)
    {
        for (i = 1; i <= 2; i++)
        {
            if (i == GET_USB)
            {
                if (status & res)
                {
                    sprintf(buf, "%sUSB Power is ON\n", buf);
                }
                else
                {
                    sprintf(buf, "%sUSB Power is OFF\n", buf);
                }
            }
            res = res << 1;
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t usb_power_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 value  = -EPERM;
    u8 result = -EPERM;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESC_601_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_lock\n"));
    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa2);
    debug_print((KERN_DEBUG "DEBUG : USB_POWER status = %x\n",status));
    if (attr->index == USB_POWER)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == TURN_ON)
        {
            value = status | USB_ON;
            debug_print((KERN_DEBUG "DEBUG : USB_POWER value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa2, value);
            debug_print((KERN_DEBUG "DEBUG : USB_POWER result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: usb_ctrl_set ON FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "USB Power is ON\n"));
            }
        }
        else if (i == TURN_OFF)
        {
            value = status & USB_OFF;
            debug_print((KERN_DEBUG "DEBUG : USB_POWER value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa2, value);
            debug_print((KERN_DEBUG "DEBUG : USB_POWER result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: usb_power_set OFF FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "USB Power is OFF\n"));
            }
        }
        else
        {
            printk(KERN_ALERT "USB_POWER set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}
#endif

#ifdef LED_CTRL_WANTED
static ssize_t led_ctrl_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 res = 0x1;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    if (attr->index == LED_CTRL)
    {
        status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa2);
        debug_print((KERN_DEBUG "DEBUG : LED_CTRL status = %x\n",status));
        sprintf(buf, "");
        if (status & res)
        {
            sprintf(buf, "%sFiber LED is set to ON\n", buf);
        }
        else
        {
            sprintf(buf, "%sFiber LED is set to OFF\n", buf);
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t led_ctrl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = 0;
    u8 value  = 0;
    u8 result = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESC_601_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_lock\n"));
    if (attr->index == LED_CTRL)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == TURN_ON)
        {
            status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa2);
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL status = %x\n",status));
            value = status | LED_ON;
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa2, value);
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: led_ctrl_set on FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "DEBUG : Fiber LED is set to ON\n"));
            }
        }
        else if (i == TURN_OFF)
        {
            status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa2);
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL status = %x\n",status));
            value = status & LED_OFF;
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa2, value);
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: led_ctrl_set off FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "DEBUG : Fiber LED is set to OFF\n"));
            }
        }
        else
        {
            printk(KERN_ALERT "LED set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}
#endif

static ssize_t led_loc_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 res = 0x1;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa3);
    debug_print((KERN_DEBUG "DEBUG : LED_LOC status = %x\n",status));
    sprintf(buf, "");
    if (attr->index == LED_LOC)
    {
        for (i = 1; i <= 3; i++)
        {
            if (i == GET_LOC)
            {
                if (status & res)
                {
                    sprintf(buf, "%sLocate LED is set to OFF\n", buf);
                }
                else
                {
                    sprintf(buf, "%sLocate LED is set to Blinking\n", buf);
                }
            }
            res = res << 1;
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t led_loc_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = 0;
    u8 value  = 0;
    u8 result = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESC_601_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_lock\n"));
    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa3);
    debug_print((KERN_DEBUG "DEBUG : LED_LOC status = %x\n",status));
    if (attr->index == LED_LOC)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == LOC_OFF)
        {
            value = status | LOC_LED_OFF;
            debug_print((KERN_DEBUG "DEBUG : LED_LOC value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa3, value);
            debug_print((KERN_DEBUG "DEBUG : LED_LOC result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: led_loc_set OFF FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "Locate LED is set to OFF\n"));
            }
        }
        else if (i == LOC_BLINK)
        {
            value = status & LOC_LED_BLINK;
            debug_print((KERN_DEBUG "DEBUG : LED_LOC value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa3, value);
            debug_print((KERN_DEBUG "DEBUG : LED_LOC result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: led_loc_set blinking FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "Locate LED is set to blinking\n"));
            }
        }
        else
        {
            printk(KERN_ALERT "LED_LOC set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}

static ssize_t led_alarm_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa3);
    debug_print((KERN_DEBUG "DEBUG : LED_ALARM status = %x\n",status));
    sprintf(buf, "");
    if (attr->index == LED_ALARM)
    {
        if((status & 0x1) && (status & 0x2))
        {
            sprintf(buf, "%sAlarm LED is set to OFF\n", buf);
        }
        if((status & 0x1) && (!(status & 0x2)))
        {
            sprintf(buf, "%sAlarm LED is set to Amber\n", buf);
        }
        if((!(status & 0x1)) && (status & 0x2))
        {
            sprintf(buf, "%sAlarm LED is set to Green\n", buf);
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t led_alarm_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = 0;
    u8 value  = 0;
    u8 result = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESC_601_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_lock\n"));
    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa3);
    debug_print((KERN_DEBUG "DEBUG : LED_ALARM status = %x\n",status));
    if (attr->index == LED_ALARM)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == ALARM_OFF)
        {
            value = status;
            value |= 0x1;
            value |= 0x2;
            debug_print((KERN_DEBUG "DEBUG : LED_ALARM value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa3, value);
            debug_print((KERN_DEBUG "DEBUG : LED_ALARM result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: LED_ALARM set OFF FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "Alarm LED is set to OFF\n"));
            }
        }
        else if (i == ALARM_AMBER)
        {
            value = status;
            value |= 0x1;
            value &= ~(0x2);
            debug_print((KERN_DEBUG "DEBUG : LED_ALARM value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa3, value);
            debug_print((KERN_DEBUG "DEBUG : LED_ALARM result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: LED_ALARM set AMBER FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "Alarm LED is set to AMBER\n"));
            }
        }
        else if (i == ALARM_GREEN)
        {
            value = status;
            value |= 0x2;
            value &= ~(0x1);
            debug_print((KERN_DEBUG "DEBUG : LED_ALARM value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa3, value);
            debug_print((KERN_DEBUG "DEBUG : LED_ALARM result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: LED_ALARM set GREEN FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "Alarm LED is set to GREEN\n"));
            }
        }
        else
        {
            printk(KERN_ALERT "LED_ALARM set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}

static ssize_t reset_mac_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = 0;
    u8 value  = 0;
    u8 result = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESC_601_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_lock\n"));
    status  = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa4);
    debug_print((KERN_DEBUG "DEBUG : RESET_MAC status = %x\n",status));
    if (attr->index == RESET_MAC)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == 0)
        {
            value = 0x0;
            debug_print((KERN_DEBUG "DEBUG : RESET_MAC value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xa4, value);
            debug_print((KERN_DEBUG "DEBUG : RESET_MAC result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: RESET_MAC set FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "Switch MAC chip is reset\n"));
            }
        }
        else
        {
            printk(KERN_ALERT "RESET_MAC set wrong Value\n");
        }
    }
    mutex_unlock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}

static ssize_t themal_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 res = 0x1;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xc0);
    debug_print((KERN_DEBUG "DEBUG : SENSOR_STATUS status = %x\n",status));
    sprintf(buf, "");
    if (attr->index == SENSOR_STATUS)
    {
        for (i = 1; i <= 4; i++)
        {
            if (status & res)
            {
                sprintf(buf, "%sSensor %d is OK\n", buf, i);
            }
            else
            {
                sprintf(buf, "%sSensor %d is NG\n", buf, i);
            }
            res = res << 1;
        }
    }
    return sprintf(buf, "%s\n", buf);
}

#ifdef THEMAL_WANTED
static ssize_t themal_temp_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 channel_status = -EPERM;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_2_data = i2c_get_clientdata(Cameo_Switch_2_client);
	struct Cameo_i2c_data *Sensor_data = i2c_get_clientdata(Cameo_Sensor_client);
    
    i = attr->index;
    debug_print((KERN_DEBUG "DEBUG : themal_temp_get %d\n", i));
    mutex_lock(&Switch_2_data->update_lock);
    mutex_lock(&Sensor_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : themal_temp_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == SENSOR_TEMP)
    {
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x02);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: themal_temp_get set channel 2 FAILED\n");
        }
        status = i2c_smbus_read_byte_data(Cameo_Sensor_client, 0X0);
        debug_print((KERN_DEBUG "DEBUG : Sensor 1 status = %x\n",status));
        if(status & 0x80)
        {
            sprintf(buf, "%sSensor 1 temp is -%d degrees (C)\n", buf, read_8bit_temp((status & 0x80),status));
        }
        else
        {
            sprintf(buf, "%sSensor 1 temp is %d degrees (C)\n", buf, read_8bit_temp((status & 0x80),status));
        }
        
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x04);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: themal_temp_get set channel 3 FAILED\n");
        }
        status = i2c_smbus_read_byte_data(Cameo_Sensor_client, 0X0);
        debug_print((KERN_DEBUG "DEBUG : Sensor 2 status = %x\n",status));
        if(status & 0x80)
        {
            sprintf(buf, "%sSensor 2 temp is -%d degrees (C)\n", buf, read_8bit_temp((status & 0x80),status));
        }
        else
        {
            sprintf(buf, "%sSensor 2 temp is %d degrees (C)\n", buf, read_8bit_temp((status & 0x80),status));
        }
        
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x08);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: themal_temp_get set channel 4 FAILED\n");
        }
        status = i2c_smbus_read_byte_data(Cameo_Sensor_client, 0X0);
        debug_print((KERN_DEBUG "DEBUG : Sensor 3 status = %x\n",status));
        if(status & 0x80)
        {
            sprintf(buf, "%sSensor 3 temp is -%d degrees (C)\n", buf, read_8bit_temp((status & 0x80),status));
        }
        else
        {
            sprintf(buf, "%sSensor 3 temp is %d degrees (C)\n", buf, read_8bit_temp((status & 0x80),status));
        }
        
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x01);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: themal_temp_get set channel 1 FAILED\n");
        }
        status = i2c_smbus_read_byte_data(Cameo_Sensor_fan_client, 0X1);
        debug_print((KERN_DEBUG "DEBUG : Sensor 4 status = %x\n",status));
        if(status & 0x80)
        {
            sprintf(buf, "%sSensor 4 temp is -%d degrees (C)\n", buf, read_8bit_temp((status & 0x80),status));
        }
        else
        {
            sprintf(buf, "%sSensor 4 temp is %d degrees (C)\n", buf, read_8bit_temp((status & 0x80),status));
        }
    }
    channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x0);
    if(channel_status < 0)
    {
        printk(KERN_ALERT "ERROR: themal_temp_get reset channel FAILED\n");
    }
    mutex_unlock(&Switch_2_data->update_lock);
    mutex_unlock(&Sensor_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return sprintf(buf, "%s\n", buf);
}

static ssize_t mac_temp_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u16 status = -EPERM;
    u8 channel_status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_2_data = i2c_get_clientdata(Cameo_Switch_2_client);
    
    mutex_lock(&Switch_2_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mac_temp_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == MAC_TEMP)
    {
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x40);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: mac_temp_get set channel 6 FAILED\n");
        }
        status = i2c_smbus_read_word_data(Cameo_MAC_Sensor_client, 0X0);
        debug_print((KERN_DEBUG "DEBUG : MAC Sensor status = %x\n",status));

        sprintf(buf, "%s%x\n", buf,status);
    }
    channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x0);
    if(channel_status < 0)
    {
        printk(KERN_ALERT "ERROR: mac_temp_get reset channel FAILED\n");
    }
    mutex_unlock(&Switch_2_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return sprintf(buf, "%s\n", buf);
}
#endif 

static ssize_t themal_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 res = 0x1;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xc1);
    debug_print((KERN_DEBUG "DEBUG : SENSOR_INT_MASK status = %x\n",status));
    sprintf(buf, "");
    if (attr->index == SENSOR_INT_MASK)
    {
        for (i = 1; i <= 4; i++)
        {
            if (status & res)
            {
                sprintf(buf, "%sSensor %d interrupt is enabled\n", buf, i);
            }
            else
            {
                sprintf(buf, "%sSensor %d interrupt is disabled\n", buf, i);
            }
            res = res << 1;
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t themal_mask_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 value  = -EPERM;
    u8 result  = -EPERM;
    u8 res = 0x1;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct Cameo_i2c_data *data = i2c_get_clientdata(ESC_601_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : themal_mask_set mutex_lock\n"));
    i = simple_strtol(buf, NULL, 10);
    debug_print((KERN_DEBUG "DEBUG : themal_mask_set input %d\n", i));
    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xc1);
    debug_print((KERN_DEBUG "DEBUG : themal_mask_set status = %x\n",status));
    if (attr->index == 1)
    {
        if (i == TURN_ON)
        {
            value = status | res;

        }
        else if (i == TURN_OFF)
        {
            value = status & (~res);
        }
        else
        {
            printk(KERN_ALERT "themal_mask_set set wrong value\n");
        }
    }
    else if (attr->index == 2)
    {
        res = res << 1;
        if (i == TURN_ON)
        {
            value = status | res;

        }
        else if (i == TURN_OFF)
        {
            value = status & (~res);
        }
        else
        {
            printk(KERN_ALERT "themal_mask_set set wrong value\n");
        }
    }
    else if (attr->index == 3)
    {
        res = res << 2;
        if (i == TURN_ON)
        {
            value = status | res;

        }
        else if (i == TURN_OFF)
        {
            value = status & (~res);
        }
        else
        {
            printk(KERN_ALERT "themal_mask_set set wrong value\n");
        }
    }
    else if (attr->index == 4)
    {
        res = res << 3;
        if (i == TURN_ON)
        {
            value = status | res;

        }
        else if (i == TURN_OFF)
        {
            value = status & (~res);
        }
        else
        {
            printk(KERN_ALERT "themal_mask_set set wrong value\n");
        }
    }
    debug_print((KERN_DEBUG "DEBUG : themal_mask_set %d value = %x\n", attr->index, value));
    result = i2c_smbus_write_byte_data(ESC_601_i2c_client, 0xc1, value);
    debug_print((KERN_DEBUG "DEBUG : themal_mask_set %d result = %x\n", attr->index,result));
    if (result < 0)
    {
        printk(KERN_ALERT "ERROR: themal_mask_set %d FAILED!\n", attr->index);
    }
    else
    {
        debug_print((KERN_DEBUG "themal_mask_set %d : %x\n", attr->index, value));
    }
    mutex_unlock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}

static ssize_t int_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 res = 0x1;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    res = 0x1;
    status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xd0);
    debug_print((KERN_DEBUG "DEBUG : INT_STATUS status = %x\n",status));
    sprintf(buf, "");
    if (attr->index == INT_STATUS)
    {
        for (i = 1; i <= 7; i++)
        {
            if ( i == PCIE_INT)
            {
                if (!(status & res))
                {
                    sprintf(buf, "%sInterrupt is triggered by PCIe\n", buf);
                }
            }
            else if( i == QSFP_1_INT)
            {
                if (!(status & res))
                {
                    sprintf(buf, "%sInterrupt is triggered by QSFP\n", buf);
                }
            }
            else if( i == QSFP_2_INT)
            {
                if (!(status & res))
                {
                    sprintf(buf, "%sInterrupt is triggered by QSFP\n", buf);
                }
            }
            else if( i == FAN_INT)
            {
                if (!(status & res))
                {
                    sprintf(buf, "%sInterrupt is triggered by FAN\n", buf);
                }
            }
            else if( i == PSU_INT)
            {
                if (!(status & res))
                {
                    sprintf(buf, "%sInterrupt is triggered by PSU\n", buf);
                }
            }
            else if( i == SENSOR_INT)
            {
                if (!(status & res))
                {
                    sprintf(buf, "%sInterrupt is triggered by Sensor\n", buf);
                }
            }
            else if( i == USB_INT)
            {
                if (!(status & res))
                {
                    sprintf(buf, "%sInterrupt is triggered by USB\n", buf);
                }
            }
            res = res << 1;
        }
        if(status == 0xf)
        {
            sprintf(buf, "%sNo interrupt is triggered\n", buf);
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t low_power_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status_1 = -EPERM; //Low power mode 01-08 port stat
    u8 status_2 = -EPERM; //Low power mode 09-16 port stat
    u8 status_3 = -EPERM; //Low power mode 17-24 port stat
    u8 status_4 = -EPERM; //Low power mode 25-32 port stat
    u8 res = 0x1;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status_1 = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x60);
    status_2 = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x61);
    status_3 = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x60);
    status_4 = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x61);
    debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 status = %x\n",status_1));
    debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_2 status = %x\n",status_2));
    debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_3 status = %x\n",status_3));
    debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_4 status = %x\n",status_4));
    sprintf(buf, "");
    if (attr->index == QSFP_LOW_POWER_ALL)
    {
        for (i = 1; i <= 8; i++)
        {
            if (status_1 & res)
            {
                sprintf(buf, "%sQSFP %02d low power mode: ON\n", buf, i);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d low power mode: OFF\n", buf, i);
            }
            res = res << 1;
        }
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_2 & res)
            {
                sprintf(buf, "%sQSFP %02d low power mode: ON\n", buf, i + 8);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d low power mode: OFF\n", buf, i + 8);
            }
            res = res << 1;
        }
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_3 & res)
            {
                sprintf(buf, "%sQSFP %02d low power mode: ON\n", buf, i + 16);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d low power mode: OFF\n", buf, i + 16);
            }
            res = res << 1;
        }
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_4 & res)
            {
                sprintf(buf, "%sQSFP %02d low power mode: ON\n", buf, i + 24);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d low power mode: OFF\n", buf, i + 24);
            }
            res = res << 1;
        }
    }
    return sprintf(buf, "%s\n", buf);
}
static ssize_t low_power_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 value  = 0x0;
    u8 status_1 = -EPERM; //Low power mode 01-08 port stat
    u8 status_2 = -EPERM; //Low power mode 09-16 port stat
    u8 status_3 = -EPERM; //Low power mode 17-24 port stat
    u8 status_4 = -EPERM; //Low power mode 25-32 port stat
    u8 result_1 = -EPERM;
    u8 result_2 = -EPERM;
    u8 result_3 = -EPERM;
    u8 result_4 = -EPERM;
    u8 j = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *CPLD_2_data = i2c_get_clientdata(Cameo_CPLD_2_client);
	struct Cameo_i2c_data *CPLD_3_data = i2c_get_clientdata(Cameo_CPLD_3_client);
    
    if (attr->index == QSFP_LOW_POWER_ALL)
    {
        i = simple_strtol(buf, NULL, 10);
        status_1 = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x60);
        status_2 = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x61);
        status_3 = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x60);
        status_4 = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x61);
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 status_1 = %x\n",status_1));
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_2 status_2 = %x\n",status_2));
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_3 status_3 = %x\n",status_3));
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_4 status_4 = %x\n",status_4));
        mutex_lock(&CPLD_2_data->update_lock);
        mutex_lock(&CPLD_3_data->update_lock);
        debug_print((KERN_DEBUG "DEBUG : low_power_all_set mutex_lock\n"));
        if (i == TURN_ON)
        {
            value = 0xff;
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL value = %x\n",value));
            result_1 = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0x60, value);
            result_2 = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0x61, value);
            result_3 = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x60, value);
            result_4 = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x61, value);
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result_1 = %x\n",result_1));
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result_2 = %x\n",result_2));
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result_3 = %x\n",result_3));
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result_4 = %x\n",result_4));
            if (result_1 < 0 || result_2 < 0 || result_3 < 0 || result_4 < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_LOW_POWER_ALL set ON FAILED!\n");
            }
            else
            {
                for(j=1; j<=32; j++)
                {
                    debug_print((KERN_DEBUG "QSFP %02d low power mode: ON\n", j));
                }
            }
        }
        else if(i == TURN_OFF)
        {
            value = 0x0;
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL value = %x\n",value));
            result_1 = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0x60, value);
            result_2 = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0x61, value);
            result_3 = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x60, value);
            result_4 = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x61, value);
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result_1 = %x\n",result_1));
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result_2 = %x\n",result_2));
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result_3 = %x\n",result_3));
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result_4 = %x\n",result_4));
            if (result_1 < 0 || result_2 < 0 || result_3 < 0 || result_4 < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_LOW_POWER_ALL set OFF FAILED!\n");
            }
            else
            {
                for(j=1; j<=32; j++)
                {
                    debug_print((KERN_DEBUG "QSFP %02d low power mode: OFF\n", j));
                }
            }
        }
        else
        {
            printk(KERN_ALERT "QSFP_LOW_POWER_ALL set wrong value\n");
        }
        mutex_unlock(&CPLD_2_data->update_lock);
        mutex_unlock(&CPLD_3_data->update_lock);
        debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    }
    return count;
}

static ssize_t low_power_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM; //Low power mode 01-08 port stat
    u8 res = 0x1;
    int i, j = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    i = attr->index;
    sprintf(buf, "");
    debug_print((KERN_DEBUG "DEBUG : low_power_get port %d\n", i));

    if (i >= 1 && i <= 8)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x60);
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_%d status = %x\n", i, status));
        for (j = 1; j <= 8; j++)
        {
            if (j == i)
            {
                if (status & res)
                {
                    sprintf(buf, "%sQSFP %02d low power mode: ON\n", buf, i);
                }
                else
                {
                    sprintf(buf, "%sQSFP %02d low power mode: OFF\n", buf, i);
                }
            }
            res = res << 1;
        }
    }
    else if (i >= 9 && i <= 16)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x61);
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_%d status = %x\n", i, status));
        for (j = 1; j <= 8; j++)
        {
            if (j == (i-8))
            {
                if (status & res)
                {
                    sprintf(buf, "%sQSFP %02d low power mode: ON\n", buf, i);
                }
                else
                {
                    sprintf(buf, "%sQSFP %02d low power mode: OFF\n", buf, i);
                }
            }
            res = res << 1;
        }
    }
    else if (i >= 17 && i <= 24)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x60);
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_%d status = %x\n", i, status));
        for (j = 1; j <= 8; j++)
        {
            if (j == (i-16))
            {
                if (status & res)
                {
                    sprintf(buf, "%sQSFP %02d low power mode: ON\n", buf, i);
                }
                else
                {
                    sprintf(buf, "%sQSFP %02d low power mode: OFF\n", buf, i);
                }
            }
            res = res << 1;
        }
    }
    else if (i >= 25 && i <= 32)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x61);
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_%d status = %x\n", i, status));
        for (j = 1; j <= 8; j++)
        {
            if (j == (i-24))
            {
                if (status & res)
                {
                    sprintf(buf, "%sQSFP %02d low power mode: ON\n", buf, i);
                }
                else
                {
                    sprintf(buf, "%sQSFP %02d low power mode: OFF\n", buf, i);
                }
            }
            res = res << 1;
        }
    }
    else
    {
        printk(KERN_ALERT "QSFP_low_power_%d get wrong value\n", i);
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t low_power_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = 0;
    u8 result = 0;
    int i = 0;
    int j = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *CPLD_2_data = i2c_get_clientdata(Cameo_CPLD_2_client);
	struct Cameo_i2c_data *CPLD_3_data = i2c_get_clientdata(Cameo_CPLD_3_client);
    
    i = attr->index;
    j = simple_strtol(buf, NULL, 10);
    debug_print((KERN_DEBUG "DEBUG : low_power_set port %d\n", i));
    mutex_lock(&CPLD_2_data->update_lock);
    mutex_lock(&CPLD_3_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : low_power_set mutex_lock\n"));
    if (i >= 1 && i <= 8)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x60);
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 status = %x\n",status));
        if( j == TURN_ON)
        {
            status |= (1 << (i-1));
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0x60, status);
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_low_power_%d set ON FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d low power mode: ON\n", i));
            }
        }
        else if( j == TURN_OFF)
        {
            status &= ~(1 << (i-1));
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0X60, status);
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_low_power_%d set OFF FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d low power mode: OFF\n", i));
            }
        }
        else
        {
            printk(KERN_ALERT "QSFP_low_power_%d set wrong value\n", i);
        }
    }
    else if (i >= 9 && i <= 16)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x61);
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 status = %x\n",status));
        if( j == TURN_ON)
        {
            status |= (1 << (i-9));
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0x61, status);
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_low_power_%d set ON FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d low power mode: ON\n", i));
            }
        }
        else if( j == TURN_OFF)
        {
            status &= ~(1 << (i-9));
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0x61, status);
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_low_power_%d set OFF FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d low power mode: OFF\n", i));
            }
        }
        else
        {
            printk(KERN_ALERT "QSFP_low_power_%d set wrong value\n", i);
        }
    }
    else if (i >= 17 && i <= 24)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x60);
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 status = %x\n",status));
        if( j == TURN_ON)
        {
            status |= (1 << (i-17));
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x60, status);
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_low_power_%d set ON FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d low power mode: ON\n", i));
            }
        }
        else if( j == TURN_OFF)
        {
            status &= ~(1 << (i-17));
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x60, status);
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_low_power_%d set OFF FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d low power mode: OFF\n", i));
            }
        }
        else
        {
            printk(KERN_ALERT "QSFP_low_power_%d set wrong value\n", i);
        }
    }
    else if (i >= 25 && i <= 32)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x61);
        debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 status = %x\n",status));
        if( j == TURN_ON)
        {
            status |= (1 << (i-25));
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x61, status);
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_low_power_%d set ON FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d low power mode: ON\n", i));
            }
        }
        else if( j == TURN_OFF)
        {
            status &= ~(1 << (i-25));
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x61, status);
            debug_print((KERN_DEBUG "DEBUG : LOW_POWER_MODE_1 result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_low_power_%d set OFF FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d low power mode: OFF\n", i));
            }
        }
        else
        {
            printk(KERN_ALERT "QSFP_low_power_%d set wrong value\n", i);
        }
    }
    else
    {
        printk(KERN_ALERT "low_power_set wrong value\n");
    }
    mutex_unlock(&CPLD_2_data->update_lock);
    mutex_unlock(&CPLD_3_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}

static ssize_t qsfp_reset_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = 0;
    u8 value  = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *CPLD_2_data = i2c_get_clientdata(Cameo_CPLD_2_client);
	struct Cameo_i2c_data *CPLD_3_data = i2c_get_clientdata(Cameo_CPLD_3_client);
    
    mutex_lock(&CPLD_2_data->update_lock);
    mutex_lock(&CPLD_3_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set mutex_lock\n"));
    if (attr->index == QSFP_RESET)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i >= 1 && i <= 8)
        {
            value  = 0;
            value  = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x70);
            debug_print((KERN_DEBUG "DEBUG : QSFP_RESET_1 value = %x\n",value));
            value ^= (1 << (i - 1));
            debug_print((KERN_DEBUG "DEBUG : QSFP_RESET_1 set value = %x\n",value));
            status = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0x70, value);
            if (status < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_RESET port %02d FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d reset success\n", i));
            }
        }
        else if (i >= 9 && i <= 16)
        {
            value  = 0;
            value  = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x71);
            debug_print((KERN_DEBUG "DEBUG : QSFP_RESET_2 value = %x\n",value));
            value ^= (1 << (i - 9));
            debug_print((KERN_DEBUG "DEBUG : QSFP_RESET_2 set value = %x\n",value));
            status = i2c_smbus_write_byte_data(Cameo_CPLD_2_client, 0x71, value);
            if (status < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_RESET port %02d FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d reset success\n", i));
            }
        }
        else if (i >= 17 && i <= 24)
        {
            value  = 0;
            value  = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x70);
            debug_print((KERN_DEBUG "DEBUG : QSFP_RESET_3 value = %x\n",value));
            value ^= (1 << (i - 17));
            debug_print((KERN_DEBUG "DEBUG : QSFP_RESET_3 set value = %x\n",value));
            status = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x70, value);
            if (status < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_RESET port %02d FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d reset success\n", i));
            }
        }
        else if (i >= 25 && i <= 32)
        {
            value  = 0;
            value  = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x71);
            debug_print((KERN_DEBUG "DEBUG : QSFP_RESET_4 value = %x\n",value));
            value ^= (1 << (i - 25));
            debug_print((KERN_DEBUG "DEBUG : QSFP_RESET_4 set value = %x\n",value));
            status = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x71, value);
            if (status < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_RESET port %02d FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d reset success\n", i));
            }
        }
        else
        {
            printk(KERN_ALERT "qsfp_reset_set wrong value\n");
        }
    }
    mutex_unlock(&CPLD_2_data->update_lock);
    mutex_unlock(&CPLD_3_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}

static ssize_t qsfp_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status_1 = -EPERM; //qsfp_status 01-08 port stat
    u8 status_2 = -EPERM; //qsfp_status 09-16 port stat
    u8 status_3 = -EPERM; //qsfp_status 17-24 port stat
    u8 status_4 = -EPERM; //qsfp_status 25-32 port stat
    u8 res = 0x1;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == QSFP_PRESENT)
    {
        sprintf(buf, "");
        status_1 = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x80);
        status_2 = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x81);
        status_3 = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x80);
        status_4 = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x81);
        debug_print((KERN_DEBUG "DEBUG : QSFP_PRESENT_1 status = %x\n",status_1));
        debug_print((KERN_DEBUG "DEBUG : QSFP_PRESENT_2 status = %x\n",status_2));
        debug_print((KERN_DEBUG "DEBUG : QSFP_PRESENT_3 status = %x\n",status_3));
        debug_print((KERN_DEBUG "DEBUG : QSFP_PRESENT_4 status = %x\n",status_4));
        for (i = 1; i <= 8; i++)
        {
            if (status_1 & res)
            {
                sprintf(buf, "%sQSFP %02d is not present\n", buf, i);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d is present\n", buf, i);
            }
            res = res << 1;
        }
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_2 & res)
            {
                sprintf(buf, "%sQSFP %02d is not present\n", buf, i + 8);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d is present\n", buf, i + 8);
            }
            res = res << 1;
        }
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_3 & res)
            {
                sprintf(buf, "%sQSFP %02d is not present\n", buf, i + 16);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d is present\n", buf, i + 16);
            }
            res = res << 1;
        }
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_4 & res)
            {
                sprintf(buf, "%sQSFP %02d is not present\n", buf, i + 24);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d is present\n", buf, i + 24);
            }
            res = res << 1;
        }
    }
    if (attr->index == QSFP_INT)
    {
        sprintf(buf, "");
        status_1 = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x90);
        status_2 = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, 0x91);
        status_3 = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x90);
        status_4 = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x91);
        debug_print((KERN_DEBUG "DEBUG : QSFP_INT_1 status = %x\n",status_1));
        debug_print((KERN_DEBUG "DEBUG : QSFP_INT_2 status = %x\n",status_2));
        debug_print((KERN_DEBUG "DEBUG : QSFP_INT_3 status = %x\n",status_3));
        debug_print((KERN_DEBUG "DEBUG : QSFP_INT_4 status = %x\n",status_4));
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_1 & res)
            {
                sprintf(buf, "%sQSFP %02d is OK\n", buf, i);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d is abnormal\n", buf, i);
            }
            res = res << 1;
        }
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_2 & res)
            {
                sprintf(buf, "%sQSFP %02d is OK\n", buf, i + 8);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d is abnormal\n", buf, i + 8);
            }
            res = res << 1;
        }
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_3 & res)
            {
                sprintf(buf, "%sQSFP %02d is OK\n", buf, i + 16);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d is abnormal\n", buf, i + 16);
            }
            res = res << 1;
        }
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status_4 & res)
            {
                sprintf(buf, "%sQSFP %02d is OK\n", buf, i + 24);
            }
            else
            {
                sprintf(buf, "%sQSFP %02d is abnormal\n", buf, i + 24);
            }
            res = res << 1;
        }
    }
    return sprintf(buf, "%s\n", buf);
}

#ifdef QSFP_WANTED
static ssize_t qsfp_temp_get(struct device *dev, struct device_attribute *da, char *buf)
{
    s32 res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_temp_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_TEMP)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_temp_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_temp_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_word_data(Cameo_QSFP_client, 0x16);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d temp read FAILED\n", buf, port_num);
                    }
                    else
                    {
                        res = ((res&0xff00)>>8) | ((res&0xff)<<8);
                        res = res / 256;
                        sprintf(buf, "%sQSFP %02d temp is %d degrees (C)\n", buf, port_num, res);
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_date_get(struct device *dev, struct device_attribute *da, char *buf)
{
    s32 res = -EPERM;
    char xbuf[6];
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_date_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_DATE)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_date_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_date_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xd4);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Date Code: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        xbuf[0] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xd4);
                        xbuf[1] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xd5);
                        xbuf[2] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xd6);
                        xbuf[3] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xd7);
                        xbuf[4] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xd8);
                        xbuf[5] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xd9);
                        sprintf(buf, "%sQSFP %02d Date Code: 20%c%c-%c%c-%c%c\n", buf, port_num, xbuf[0], xbuf[1], xbuf[2], xbuf[3], xbuf[4], xbuf[5]);
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_sn_get(struct device *dev, struct device_attribute *da, char *buf)
{
    s32 res = -EPERM;
    char xbuf[16];
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u8 reg;
    u16 i;
    u16 j;
    u16 k;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_sn_get mutex_lock\n"));
    sprintf(buf, "\n");
    if (attr->index == QSFP_SN)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_sn_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_sn_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xc4);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d SN: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        reg = 0xc4;
                        for(k = 0; k < 15; k++)
                        {
                            xbuf[k] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, reg);
                            reg = reg + 1;
                        }
                        sprintf(buf, "%sQSFP %02d SN: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c \n", buf, port_num, 
                        xbuf[0], xbuf[1], xbuf[2], xbuf[3],
                        xbuf[4], xbuf[5], xbuf[6], xbuf[7],
                        xbuf[8], xbuf[9], xbuf[10], xbuf[11],
                        xbuf[12], xbuf[13], xbuf[14], xbuf[15]
                        );
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_pn_get(struct device *dev, struct device_attribute *da, char *buf)
{
    s32 res = -EPERM;
    char xbuf[16];
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u8 reg;
    u16 i;
    u16 j;
    u16 k;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_pn_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_PN)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_pn_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_pn_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xa8);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d PN: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        reg = 0xa8;
                        for(k = 0; k < 15; k++)
                        {
                            xbuf[k] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, reg);
                            reg = reg + 1;
                        }
                        sprintf(buf, "%sQSFP %02d PN: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", buf, port_num, 
                        xbuf[0], xbuf[1], xbuf[2], xbuf[3],
                        xbuf[4], xbuf[5], xbuf[6], xbuf[7],
                        xbuf[8], xbuf[9], xbuf[10], xbuf[11],
                        xbuf[12], xbuf[13], xbuf[14], xbuf[15]
                        );
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_name_get(struct device *dev, struct device_attribute *da, char *buf)
{
    s32 res = -EPERM;
    char xbuf[16];
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u8 reg;
    u16 i;
    u16 j;
    u16 k;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_name_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_NAME)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_name_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_name_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x94);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Name: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        reg = 0x94;
                        for(k = 0; k < 15; k++)
                        {
                            xbuf[k] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, reg);
                            reg = reg + 1;
                        }
                        sprintf(buf, "%sQSFP %02d Name: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", buf, port_num, 
                        xbuf[0], xbuf[1], xbuf[2], xbuf[3],
                        xbuf[4], xbuf[5], xbuf[6], xbuf[7],
                        xbuf[8], xbuf[9], xbuf[10], xbuf[11],
                        xbuf[12], xbuf[13], xbuf[14], xbuf[15]
                        );
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_oui_get(struct device *dev, struct device_attribute *da, char *buf)
{
    s32 res = -EPERM;
    char xbuf[3];
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u8 reg;
    u16 i;
    u16 j;
    u16 k;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_oui_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_OUI)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_oui_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_oui_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xa5);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d OUI: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        reg = 0xa5;
                        for(k = 0; k < 3; k++)
                        {
                            xbuf[k] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, reg);
                            reg = reg + 1;
                        }
                        sprintf(buf, "%sQSFP %02d OUI: %02X-%02X-%02X\n", buf, port_num, 
                        xbuf[0], xbuf[1], xbuf[2]);
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_rev_get(struct device *dev, struct device_attribute *da, char *buf)
{
    s32 res = -EPERM;
    char xbuf[2];
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u8 reg;
    u16 i;
    u16 j;
    u16 k;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_rev_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_REV)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_rev_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_rev_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0xb8);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Rev: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        reg = 0xb8;
                        for(k = 0; k < 2; k++)
                        {
                            xbuf[k] = (uint8_t)i2c_smbus_read_byte_data(Cameo_QSFP_client, reg);
                            reg = reg + 1;
                        }
                        sprintf(buf, "%sQSFP %02d Rev: %c\n", buf, port_num, 
                        xbuf[0]);
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_connector_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    u16 k;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_connector_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_CONNECTOR)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_connector_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_connector_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x82);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Connector: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        for(k=0; k<17; k++)
                        {
                            if(conn[k].v == res)
                            {
                                sprintf(buf, "%sQSFP %02d Connector: %s\n", buf, port_num, conn[k].n);
                                break;
                            }
                        }
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_encoding_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    u16 k;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_encoding_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_ENCODING)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_encoding_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_encoding_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x8b);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Encoding: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        for(k=0; k<7; k++)
                        {
                            if(encoding[k].v == res)
                            {
                                sprintf(buf, "%sQSFP %02d Encoding: %s\n", buf, port_num, encoding[k].n);
                                break;
                            }
                        }
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_nominal_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_nominal_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_NOMINAL)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_nominal_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_nominal_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x8c);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Nominal Bit Rate(100Mbs): FAILED\n", buf, port_num);
                    }
                    else
                    {
                        sprintf(buf, "%sQSFP %02d Nominal Bit Rate(100Mbs): %d\n", buf, port_num, res);
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_ext_rate_com_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_ext_rate_com_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_EXT_RATE_COM)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_ext_rate_com_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_ext_rate_com_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x8d);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Extended RateSelect Compliance: FAILED\n", buf, port_num);
                    }
                    else if(res == 0)
                    {
                        sprintf(buf, "%sQSFP %02d Extended RateSelect Compliance: QSFP+ Rate Select Version 1\n", buf, port_num);
                    }
                    else
                    {
                        sprintf(buf, "%sQSFP %02d Extended RateSelect Compliance: Unknown\n", buf, port_num);
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_eth_com_code_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    u16 k;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_eth_com_code_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_ETH_COM_CODE)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_eth_com_code_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_eth_com_code_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x83);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d 10/40G Ethernet Compliance Code: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        for(k=0; k<9; k++)
                        {
                            if(eth_1040g[k].v == res)
                            {
                                sprintf(buf, "%sQSFP %02d 10/40G Ethernet Compliance Code: %s\n", buf, port_num, eth_1040g[k].n);
                                break;
                            }
                            if(eth_1040g[k].n == NULL)
                            {
                                sprintf(buf, "%sQSFP %02d 10/40G Ethernet Compliance Code: Unknown\n", buf, port_num);
                                break;
                            }
                        }
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_identifier_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_identifier_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_IDENTIFIER)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_identifier_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_identifier_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x80);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Identifier: FAILED\n", buf, port_num);
                    }
                    else if(res == 0x0c)
                    {
                        sprintf(buf, "%sQSFP %02d Identifier: QSFP\n", buf, port_num);
                    }
                    else if(res == 0x0d)
                    {
                        sprintf(buf, "%sQSFP %02d Identifier: QSFP+\n", buf, port_num);
                    }
                    else
                    {
                        sprintf(buf, "%sQSFP %02d Identifier: Unknown\n", buf, port_num);
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_fc_media_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    u16 k;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_fc_media_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_FCMEDIA)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_fc_media_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_fc_media_get set qsfp channel FAILED\n");
                }
                else
                {
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x89);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Fibre Channel transmission media: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        for(k=0; k<9; k++)
                        {
                            if(fc_media[k].v == res)
                            {
                                sprintf(buf, "%sQSFP %02d Fibre Channel transmission media: %s\n", buf, port_num, fc_media[k].n);
                                break;
                            }
                        }
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_fc_speed_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    const char *tech_speed;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_fc_speed_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_FCSPEED)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_fc_speed_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_fc_speed_get set qsfp channel FAILED\n");
                }
                else
                {
                    tech_speed = NULL;
                    res = i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x8a);
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Fibre Channel Speed: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        tech_speed = find_zero_bit(fc_speed, res, 1);
                        sprintf(buf, "%sQSFP %02d Fibre Channel Speed: %s\n", buf, port_num, tech_speed);
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t qsfp_cab_tech_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int res = -EPERM;
    u8 channel_status = -EPERM;
    u8 qsfp_channel_status = -EPERM;
    u8 port_num = 1;
    u16 i;
    u16 j;
    const char *tech_tech;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_1_data = i2c_get_clientdata(Cameo_Switch_1_client);
	struct Cameo_i2c_data *QSFP_Switch_data = i2c_get_clientdata(Cameo_QSFP_Switch_client);

    debug_print((KERN_DEBUG "DEBUG : qsfp_cab_tech_get mutex_lock\n"));
    sprintf(buf, "");
    if (attr->index == QSFP_FCTECH)
    {
        for(j = 0x10; j <= 0x80; j = j*2)
        {
            //QSFP Port 01-32
            mutex_lock(&Switch_1_data->update_lock);
            channel_status = i2c_smbus_write_byte(Cameo_Switch_1_client, j);
            mutex_unlock(&Switch_1_data->update_lock);
            if(channel_status < 0)
            {
                printk(KERN_ALERT "ERROR: qsfp_cab_tech_get set channel 0x10 FAILED\n");
            }
            for(i = 0x1; i <=  0x80; i = i*2)
            {
                mutex_lock(&QSFP_Switch_data->update_lock);
                qsfp_channel_status = i2c_smbus_write_byte(Cameo_QSFP_Switch_client, i);
                if(qsfp_channel_status < 0)
                {
                    printk(KERN_ALERT "ERROR: qsfp_cab_tech_get set qsfp channel FAILED\n");
                }
                else
                {
                    tech_tech = NULL;
                    res = ((i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x87) << 8) | i2c_smbus_read_byte_data(Cameo_QSFP_client, 0x88));
                    if (res < 0)
                    {
                        sprintf(buf, "%sQSFP %02d Fibre Channel link length/Transmitter Technology: FAILED\n", buf, port_num);
                    }
                    else
                    {
                        tech_tech = find_zero_bit(cab_tech, res, 2);
                        sprintf(buf, "%sQSFP %02d Fibre Channel link length/Transmitter Technology: %s\n", buf, port_num, tech_tech);
                    }
                }
                port_num++;
                mutex_unlock(&QSFP_Switch_data->update_lock);
                debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
            }
        }
    }
    return sprintf(buf, "%s\n", buf);
}
#endif

static ssize_t fan_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 res=0x1;
    u16 fan_speed;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *target_client = NULL;  

    sprintf(buf, "");
    if (attr->index == FAN_STATUS) 
    {
        if(i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa5) == 0x1)
        {
            status = i2c_smbus_read_byte_data(Cameo_BMC_client, 0x80);
        }
        else
        {
            status = i2c_smbus_read_byte_data(Cameo_CPLD_4_client, 0x0);
        }
        debug_print((KERN_DEBUG "DEBUG : FAN_STATUS status = %x\n",status));
        for(i=1; i<=5; i++)
        {
            if(status & res)
            {
                sprintf(buf, "%sFan %d is Good\n", buf, i);
            }
            else
            {
                sprintf(buf, "%sFan %d is Fail\n", buf, i);
            } 
            res=res<<1;
        }
    }
    else if (attr->index == FAN_PRESENT) 
    {
        if(i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa5) == 0x1)
        {
            status = i2c_smbus_read_byte_data(Cameo_BMC_client, 0x81);
        }
        else
        {
            status = i2c_smbus_read_byte_data(Cameo_CPLD_4_client, 0x1);
        }
        debug_print((KERN_DEBUG "DEBUG : FAN_PRESENT status = %x\n",status));
        for(i=1; i<=5; i++)
        {
            if(status & res)
            {
                sprintf(buf, "%sFan %d is present\n", buf, i);
            }
            else
            {
                sprintf(buf, "%sFan %d is not present\n", buf, i);
            } 
            res=res<<1;
        }
    }
    else if (attr->index == FAN_POWER) 
    {
        if(i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa5) == 0x1)
        {
            status = i2c_smbus_read_byte_data(Cameo_BMC_client, 0x82);
        }
        else
        {
            status = i2c_smbus_read_byte_data(Cameo_CPLD_4_client, 0x2);
        }
        debug_print((KERN_DEBUG "DEBUG : FAN_POWER status = %x\n",status));
        for(i=1; i<=5; i++)
        {
            if(status & res)
            {
                sprintf(buf, "%sFan %d is power Good\n", buf, i);
            }
            else
            {
                sprintf(buf, "%sFan %d is not power Good\n", buf, i);
            } 
            res=res<<1;
        }
    }
    else if (attr->index == FAN_SPEED_RPM)
    {
        res = -EPERM;
        if(i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa5) == 0x1)
        {
            target_client = Cameo_BMC_client;
            res = i2c_smbus_read_byte_data(Cameo_BMC_client, 0x81);
        }
        else
        {
            target_client = Cameo_CPLD_4_client;
            res = i2c_smbus_read_byte_data(Cameo_CPLD_4_client, 0x1);
        }

        if(res < 0) {
            sprintf(buf, "%sCheck fan present error\n", buf);
            return sprintf(buf, "%s\n",buf);
        }        

        // first fan of couple
        for(i=0; i<SYSFAN_MAX_NUM; i++)
        {
            // skip the fan which is not present
            if(!(res & (0x01<<i)))
            {
                sprintf(buf, "%sFanModule%i Front : N/A\n", buf, i+1);
                sprintf(buf, "%sFanModule%i Rear  : N/A\n", buf, i+1);
                continue;
            }

            // front fan of couple
            // read high byte
            status = i2c_smbus_read_byte_data(target_client, 0xA0+(i*2)+1);
            if(status < 0){
                sprintf(buf, "%sFanModule%i Front : read error\n", buf, i+1);
                continue;
            }
            fan_speed = status;
            
            // read low byte
            status = i2c_smbus_read_byte_data(target_client, 0xA0+(i*2));
            if(status < 0){
                sprintf(buf, "%sFanModule%i Front : read error\n", buf, i+1);
                continue;
            }
            fan_speed = ((fan_speed<<8) + status)*30;

            sprintf(buf, "%sFanModule%i Front : %d RPM\n", buf, i+1, fan_speed);

            // rear fan of couple
            // read high byte
            status = i2c_smbus_read_byte_data(target_client, 0xB0+(i*2)+1);
            if(status < 0){
                sprintf(buf, "%sFanModule%i Rear  : read error\n", buf, i+1);
                continue;
            }
            fan_speed = status;
            
            // read low byte
            status = i2c_smbus_read_byte_data(target_client, 0xB0+(i*2));
            if(status < 0){
                sprintf(buf, "%sFanModule%i Rear  : read error\n", buf, i+1);
                continue;
            }
            fan_speed = ((fan_speed<<8) + status)*30;

            sprintf(buf, "%sFanModule%i Rear  : %d RPM\n", buf, i+1, fan_speed);
        }
       
    }
    
    return sprintf(buf, "%s\n",buf);
}

#ifdef FAN_CTRL_WANTED
static ssize_t fan_mode_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 channel_status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_2_data = i2c_get_clientdata(Cameo_Switch_2_client);
    mutex_lock(&Switch_2_data->update_lock);
    
    sprintf(buf, "");
    if (attr->index == FAN_MODE) 
    {
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x01);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: fan_mode_get set channel 1 FAILED\n");
        }
        
        status = i2c_smbus_read_byte_data(Cameo_Sensor_fan_client, 0x64);
        debug_print((KERN_DEBUG "DEBUG : FAN_MODE get status = %x\n",status));
        
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x00);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: fan_mode_get channel reset FAILED\n");
        }
        sprintf(buf, "%s0x%x\n", buf, status);
    }
    mutex_unlock(&Switch_2_data->update_lock);
    return sprintf(buf, "%s\n",buf);
}

static ssize_t fan_mode_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 channel_status = -EPERM;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_2_data = i2c_get_clientdata(Cameo_Switch_2_client);
	struct Cameo_i2c_data *Sensor_fan_data = i2c_get_clientdata(Cameo_Sensor_fan_client);
    mutex_lock(&Switch_2_data->update_lock);
    mutex_lock(&Sensor_fan_data->update_lock);
    if (attr->index == FAN_MODE) 
    {
        i = simple_strtol(buf, NULL, 16);
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x01);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: fan_mode_get set channel 1 FAILED\n");
        }
     
        status = i2c_smbus_write_byte_data(Cameo_Sensor_fan_client, 0x64, i);
        debug_print((KERN_DEBUG "DEBUG : FAN_MODE set status = %x\n", status));
        
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x00);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: fan_mode_get channel reset FAILED\n");
        }
    }
    mutex_unlock(&Switch_2_data->update_lock);
    mutex_unlock(&Sensor_fan_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}
static ssize_t fan_rpm_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 channel_status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_2_data = i2c_get_clientdata(Cameo_Switch_2_client);
    mutex_lock(&Switch_2_data->update_lock);
    
    sprintf(buf, "");
    if (attr->index == FAN_RPM) 
    {
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x01);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: fan_rpm_get set channel 1 FAILED\n");
        }
        
        status = i2c_smbus_read_byte_data(Cameo_Sensor_fan_client, 0x60);
        debug_print((KERN_DEBUG "DEBUG : FAN_RPM status = %x\n",status));
        
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x00);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: fan_rpm_get channel reset FAILED\n");
        }
        sprintf(buf, "%s0x%x\n", buf, status);
    }
    mutex_unlock(&Switch_2_data->update_lock);
    return sprintf(buf, "%s\n",buf);
}

static ssize_t fan_rpm_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 channel_status = -EPERM;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *Switch_2_data = i2c_get_clientdata(Cameo_Switch_2_client);
	struct Cameo_i2c_data *Sensor_fan_data = i2c_get_clientdata(Cameo_Sensor_fan_client);
    mutex_lock(&Switch_2_data->update_lock);
    mutex_lock(&Sensor_fan_data->update_lock);
    if (attr->index == FAN_RPM) 
    {
        i = simple_strtol(buf, NULL, 16);
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x01);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: fan_mode_get set channel 1 FAILED\n");
        }
     
        status = i2c_smbus_write_byte_data(Cameo_Sensor_fan_client, 0x60, i);
        debug_print((KERN_DEBUG "DEBUG : FAN_RPM set status = %x\n", status));
        
        channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x00);
        if(channel_status < 0)
        {
            printk(KERN_ALERT "ERROR: fan_mode_get channel reset FAILED\n");
        }
    }
    mutex_unlock(&Switch_2_data->update_lock);
    mutex_unlock(&Sensor_fan_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}
#endif

#ifdef EEPROM_WANTED
const char * find_value(struct qsfp_table *x, int value)
{
	for (; x->n != NULL; x++)
		if (x->v == value)
			return (x->n);
	return (NULL);
}

const char * find_zero_bit(struct qsfp_table *x, int value, int sz)
{
	int v, m;
	const char *s;

	v = 1;
	for (v = 1, m = 1 << (8 * sz); v < m; v *= 2) {
		if ((value & v) == 0)
			continue;
		if ((s = find_value(x, value & v)) != NULL) {
			value &= ~v;
			return (s);
		}
	}

	return (NULL);
}
#endif

#ifdef FAN_DUTY_CTRL_WANTED
static ssize_t fan_duty_control(void)
{
    u8 channel_status = -EPERM;
    u8 fan_status = -EPERM;
	struct Cameo_i2c_data *Switch_2_data = i2c_get_clientdata(Cameo_Switch_2_client);
	struct Cameo_i2c_data *Sensor_data = i2c_get_clientdata(Cameo_Sensor_client);
    
    mutex_lock(&Switch_2_data->update_lock);
    mutex_lock(&Sensor_data->update_lock);
    channel_status = i2c_smbus_write_byte(Cameo_Switch_2_client, 0x01);
    if(channel_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set channel 0x01 FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x4a, 0x20);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set PWM 360KHz FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x4d, 0x07);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set PWM 25.71KHz FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x4c, 0x07);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set default duty 50 FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x50, 0x00);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set point 1 FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x51, 0x05);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set point 1 duty 35 FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x52, 0x20);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set point 2  FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x53, 0x07);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set point 2 duty 50 FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x54, 0x24);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set point 3 FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x55, 0x0a);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set point 3 duty 70 FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x56, 0x2c);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set point 4 FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x57, 0x0a);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set point 4 duty 85 FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x4f, 0x03);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control set FAILED\n");
        goto set_failed;
    }
    fan_status = i2c_smbus_write_byte_data(Cameo_Sensor_client, 0x4a, 0x00);
    if(fan_status < 0)
    {
        printk(KERN_ALERT "ERROR: fan_duty_control enable lookup table FAILED\n");
        goto set_failed;
    }
    mutex_unlock(&Switch_2_data->update_lock);
    mutex_unlock(&Sensor_data->update_lock);
    return 0;
    
    set_failed:
    mutex_unlock(&Switch_2_data->update_lock);
    mutex_unlock(&Sensor_data->update_lock);
    return -1;
}
#endif /*FAN_DUTY_CTRL_WANTED*/

#ifdef ASPEED_BMC_WANTED
static ssize_t bmc_register_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 reg    = -EPERM;
    u32 status = -EPERM;
    u8  len    = -EPERM;
    u8  idex =0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    sprintf(buf, "");
    switch (attr->index)
    {
        case BMC_SERSOR_1:
            reg = 0x10;
            len = 1;
            idex = 1;
        break;
        case BMC_SERSOR_2:
            reg = 0x20;
            len = 1;
            idex = 2;
        break;
        case BMC_SERSOR_3:
            reg = 0x30;
            len = 1;
            idex = 3;
        break;
        case BMC_SERSOR_4:
            reg = 0x40;
            len = 1;
            idex = 4;
        break;
        case BMC_MAC_SENSOR:
            reg = 0x70;
            len = 2;
            idex = 5;
        break;
    }
    if(len == 1)
    {
        status = i2c_smbus_read_byte_data(Cameo_BMC_client, reg);
        debug_print((KERN_DEBUG "DEBUG : BMC byte status = 0x%x\n", status));
    }
    else if (len == 2)
    {
        status = i2c_smbus_read_word_data(Cameo_BMC_client, reg);
        debug_print((KERN_DEBUG "DEBUG : BMC word status = 0x%x\n", status));
    }
    if(status == 0xfffffffa || status == 0xff || status == 0xffff)
    {
        sprintf(buf, "%sAccess BMC module FAILED\n", buf);
    }
    else
    {
        
        if(len == 1)
          sprintf(buf, "%sSensor %d temp is %s%d degrees (C)\n", buf, idex,(status & 0x80)!=0 ? "-":"",read_8bit_temp((status & 0x80),status));
        else
          sprintf(buf, "%s0x%x\n", buf, ((status&0xff)<<8)|((status&0xFF00)>>8));      
    }
    return sprintf(buf, "%s\n", buf);
}
static ssize_t bmc_module_detect(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    sprintf(buf, "");
    if(attr->index == BMC_DETECT)
    {
        status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0xa5);
        debug_print((KERN_DEBUG "DEBUG : BMC byte status = 0x%x\n", status));
    }
    if(status == 0x1)
    {
        sprintf(buf, "%sBMC module is present\n", buf);
    }
    else
    {
        sprintf(buf, "%sBMC module is not present\n", buf);
    }
    return sprintf(buf, "%s\n", buf);
}
#endif /*ASPEED_BMC_WANTED*/

#ifdef WDT_CTRL_WANTED
static ssize_t wdt_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    sprintf(buf, "");
    return sprintf(buf, "%s\n", buf);
}
static ssize_t wdt_status_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    return count;
}
#endif /*WDT_CTRL_WANTED*/

#ifdef EEPROM_WP_WANTED
static ssize_t eeprom_wp_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    sprintf(buf, "");
    return sprintf(buf, "%s\n", buf);
}
static ssize_t eeprom_wp_status_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    return count;
}
#endif /*EEPROM_WP_WANTED*/

static ssize_t hw_version_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    sprintf(buf, "");
    if(attr->index == HW_VER)
    {
        status = i2c_smbus_read_byte_data(ESC_601_i2c_client, 0x20);
    }
    sprintf(buf, "%sHW version is 0x%x\n", buf, status);
    return sprintf(buf, "%s\n", buf);
}

static ssize_t cpld_version_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u32 status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    sprintf(buf, "");
    
    struct i2c_client *target_cpld_client;
    
    switch(attr->index)
    {
        case SWITCH_BORAD_CPLD1:
            target_cpld_client = ESC_601_i2c_client;
            break;
        case SWITCH_BORAD_CPLD2:
            target_cpld_client = Cameo_CPLD_2_client;
            break;
        case SWITCH_BORAD_CPLD3:
            target_cpld_client = Cameo_CPLD_3_client;
            break;
        case FAN_BORAD_CPLD:
            target_cpld_client = Cameo_CPLD_4_client;
            break;
        default:
             return sprintf(buf, "ERR\n");
    }

    status = i2c_smbus_read_byte_data(target_cpld_client, 0x20);

    sprintf(buf, "%s0x%x\n", buf, status);
    return sprintf(buf, "%s\n", buf);
}

#ifdef EEPROM_WANTED
/*
 *  decode_tlv_value
 *
 *  Decode a single TLV value into a string.

 *  The validity of EEPROM contents and the TLV field have been verified
 *  prior to calling this function.
 */
#define DECODE_NAME_MAX     20

static void decode_tlv_value(tlvinfo_tlv_t * tlv, char* value)
{
    int i;

    switch (tlv->type)
    {
        case TLV_CODE_PRODUCT_NAME:
        case TLV_CODE_PART_NUMBER:
        case TLV_CODE_SERIAL_NUMBER:
        case TLV_CODE_MANUF_DATE:
        case TLV_CODE_LABEL_REVISION:
        case TLV_CODE_PLATFORM_NAME:
        case TLV_CODE_ONIE_VERSION:
        case TLV_CODE_MANUF_NAME:
        case TLV_CODE_MANUF_COUNTRY:
        case TLV_CODE_VENDOR_NAME:
        case TLV_CODE_DIAG_VERSION:
        case TLV_CODE_SERVICE_TAG:
            memcpy(value, tlv->value, tlv->length);
            value[tlv->length] = 0;
            break;

        case TLV_CODE_MAC_BASE:
            sprintf(value, "%02X:%02X:%02X:%02X:%02X:%02X",
                tlv->value[0], tlv->value[1], tlv->value[2],
                tlv->value[3], tlv->value[4], tlv->value[5]);
            break;

        case TLV_CODE_DEVICE_VERSION:
            sprintf(value, "%u", tlv->value[0]);
            break;

        case TLV_CODE_MAC_SIZE:
            sprintf(value, "%u", (tlv->value[0] << 8) | tlv->value[1]);
            break;

        case TLV_CODE_VENDOR_EXT:
            value[0] = 0;
            for (i = 0; (i < (TLV_DECODE_VALUE_MAX_LEN/5)) && (i < tlv->length); i++)
            {
                sprintf(value, "%s 0x%02X", value, tlv->value[i]);
            }
            break;

        case TLV_CODE_CRC_32:
            sprintf(value, "0x%02X%02X%02X%02X",
                tlv->value[0], tlv->value[1], tlv->value[2],
                tlv->value[3]);
            break;

        default:
            break;
    }

}

static inline const char* tlv_type2name(u_int8_t type)
{
    char* name = "Unknown";
    int   i;

    for (i = 0; i < sizeof(tlv_code_list)/sizeof(tlv_code_list[0]); i++) {
	if (tlv_code_list[i].m_code == type) {
	    name = tlv_code_list[i].m_name;
	    break;
	}
    }
    return name;
}

/*
 *  decode_tlv
 *
 *  Print a string representing the contents of the TLV field. The format of
 *  the string is:
 *      1. The name of the field left justified in 20 characters
 *      2. The type code in hex right justified in 5 characters
 *      3. The length in decimal right justified in 4 characters
 *      4. The value, left justified in however many characters it takes
 *  The validity of EEPROM contents and the TLV field have been verified
 *  prior to calling this function.
 */
#define DECODE_NAME_MAX     20

static void decode_tlv(tlvinfo_tlv_t * tlv, char *buf)
{
    char name[DECODE_NAME_MAX];
    char value[TLV_DECODE_VALUE_MAX_LEN];

    decode_tlv_value(tlv, value);

    strncpy(name, tlv_type2name(tlv->type), DECODE_NAME_MAX);
    name[DECODE_NAME_MAX-1] = 0;
#ifdef DEBUG_MSG
    printk(KERN_ALERT "%-20s 0x%02X %3d %s\n", name, tlv->type, tlv->length, value);
#endif
    sprintf(buf, "%s%-20s 0x%02X %3d %s\n", buf, name, tlv->type, tlv->length, value);
}

/*
 *  show_eeprom
 *
 *  Display the contents of the EEPROM
 */
void show_eeprom(u_int8_t *eeprom, char *buf, int tlv_len )
{
    int tlv_end;
    int curr_tlv;
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t    * eeprom_tlv;

#ifdef DEBUG_MSG
    printk(KERN_ALERT "TlvInfo Header:\n");
    printk(KERN_ALERT "   Id String:    %s\n", eeprom_hdr->signature);
    printk(KERN_ALERT "   Version:      %d\n", eeprom_hdr->version);
    printk(KERN_ALERT "   Total Length: %d\n", tlv_len);
    printk(KERN_ALERT "TLV Name             Code Len Value\n");
    printk(KERN_ALERT "-------------------- ---- --- -----\n");
#endif
    sprintf(buf, "%sTlvInfo Header:\n", buf);
    sprintf(buf, "%s   Id String:    %s\n", buf, &eeprom_hdr->signature[0]);
    sprintf(buf, "%s   Version:      %d\n", buf, eeprom_hdr->version);
    sprintf(buf, "%s   Total Length: %d\n", buf, tlv_len);
    sprintf(buf, "%sTLV Name             Code Len Value\n", buf);
    sprintf(buf, "%s-------------------- ---- --- -----\n", buf);
    
    curr_tlv = sizeof(tlvinfo_header_t);
    tlv_end  = sizeof(tlvinfo_header_t) + tlv_len;

    while (curr_tlv < tlv_end)
    {
        eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[curr_tlv];
        decode_tlv(eeprom_tlv, buf);
        curr_tlv += sizeof(tlvinfo_tlv_t) + eeprom_tlv->length;
    }
    
    return;
}
#endif

/* end of function */

static int Cameo_i2c_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    struct Cameo_i2c_data *data;
    struct Cameo_i2c_data *CPLD_2_data;
    struct Cameo_i2c_data *CPLD_3_data;
    struct Cameo_i2c_data *CPLD_4_data;
#ifdef I2C_SWITCH_WANTED
    struct Cameo_i2c_data *Switch_1_data;
    struct Cameo_i2c_data *Switch_2_data;
#endif
#ifdef THEMAL_WANTED
    struct Cameo_i2c_data *Sensor_data;
    struct Cameo_i2c_data *MAC_Sensor_data;
    struct Cameo_i2c_data *Sensor_fan_data;
#endif
#ifdef QSFP_WANTED
    struct Cameo_i2c_data *QSFP_Switch_data;
    struct Cameo_i2c_data *QSFP_data;
#endif
#ifdef EEPROM_WANTED
    struct Cameo_i2c_data *EEPROM_data;
#endif

#ifdef ASPEED_BMC_WANTED
    struct Cameo_i2c_data *Cameo_BMC_data;
#endif
    int status;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
    {
        status = -EIO;
        goto exit;
    }
    data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    CPLD_2_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!CPLD_2_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    CPLD_3_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!CPLD_3_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    CPLD_4_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!CPLD_4_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
#ifdef I2C_SWITCH_WANTED
    Switch_1_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Switch_1_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    Switch_2_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Switch_2_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
#endif
#ifdef THEMAL_WANTED
    Sensor_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Sensor_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    MAC_Sensor_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!MAC_Sensor_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    Sensor_fan_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Sensor_fan_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
#endif
#ifdef QSFP_WANTED
    QSFP_Switch_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!QSFP_Switch_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
    QSFP_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!QSFP_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
#endif
#ifdef EEPROM_WANTED
    EEPROM_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!EEPROM_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
#endif

#ifdef ASPEED_BMC_WANTED
    Cameo_BMC_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Cameo_BMC_data)
    {
        printk(KERN_ALERT "kzalloc fail\n");
        status = -ENOMEM;
        goto exit;
    }
#endif
    i2c_set_clientdata(client, data);
    i2c_set_clientdata(Cameo_CPLD_2_client, CPLD_2_data);
    i2c_set_clientdata(Cameo_CPLD_3_client, CPLD_3_data);
    i2c_set_clientdata(Cameo_CPLD_4_client, CPLD_4_data);
#ifdef I2C_SWITCH_WANTED
    i2c_set_clientdata(Cameo_Switch_1_client, Switch_1_data);
    i2c_set_clientdata(Cameo_Switch_2_client, Switch_2_data);
#endif
#ifdef THEMAL_WANTED
    i2c_set_clientdata(Cameo_Sensor_client, Sensor_data);
    i2c_set_clientdata(Cameo_MAC_Sensor_client, MAC_Sensor_data);
    i2c_set_clientdata(Cameo_Sensor_fan_client, Sensor_fan_data);
#endif
#ifdef QSFP_WANTED
    i2c_set_clientdata(Cameo_QSFP_Switch_client, QSFP_Switch_data);
    i2c_set_clientdata(Cameo_QSFP_client, QSFP_data);
#endif
#ifdef EEPROM_WANTED
    i2c_set_clientdata(Cameo_EEPROM_client, EEPROM_data);
#endif
    mutex_init(&CPLD_2_data->update_lock);
    mutex_init(&CPLD_3_data->update_lock);
    mutex_init(&CPLD_4_data->update_lock);
#ifdef I2C_SWITCH_WANTED
    mutex_init(&Switch_1_data->update_lock);
    mutex_init(&Switch_2_data->update_lock);
#endif
#ifdef THEMAL_WANTED
    mutex_init(&Sensor_data->update_lock);
    mutex_init(&MAC_Sensor_data->update_lock);
    mutex_init(&Sensor_fan_data->update_lock);
#endif
#ifdef QSFP_WANTED
    mutex_init(&QSFP_Switch_data->update_lock);
    mutex_init(&QSFP_data->update_lock);
#endif
#ifdef EEPROM_WANTED
    mutex_init(&EEPROM_data->update_lock);
#endif
#ifdef ASPEED_BMC_WANTED
    mutex_init(&Cameo_BMC_data->update_lock);
#endif
    data->valid = 0;
    mutex_init(&data->update_lock);
    dev_info(&client->dev, "chip found\n");
    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &ESC601_PSU_group);
    if (status)
    {
        goto exit_free;
    }
#ifdef USB_CTRL_WANTED
    status = sysfs_create_group(&client->dev.kobj, &ESC601_USB_group);
    if (status)
    {
        goto exit_free;
    }
#endif
    status = sysfs_create_group(&client->dev.kobj, &ESC601_LED_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESC601_Reset_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESC601_Sensor_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESC601_INT_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESC601_QSFP_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESC601_FAN_group);
    if (status)
    {
        goto exit_free;
    }
#ifdef EEPROM_WANTED
    status = sysfs_create_group(&client->dev.kobj, &ESC601_EEPROM_group);
    if (status)
    {
        goto exit_free;
    }
#endif
#ifdef ASPEED_BMC_WANTED
    status = sysfs_create_group(&client->dev.kobj, &ESC601_BMC_group);
    if (status)
    {
        goto exit_free;
    }
#endif
    status = sysfs_create_group(&client->dev.kobj, &ESC601_SYS_group);
    if (status)
    {
        goto exit_free;
    }

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev))
    {
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }
    dev_info(&client->dev, "%s: '%s'\n", dev_name(data->hwmon_dev), client->name);
#ifdef FAN_DUTY_CTRL_WANTED
    if(fan_duty_control() != 0)
    {
        printk(KERN_ALERT "ERROR: FAN DUTY CONTROL SET FAILED\n");
    }
    else
    {
        printk(KERN_ALERT "ESC601-32Q set fan duty control success\n");
    }
#endif /*FAN_DUTY_CTRL_WANTED*/
    return 0;
exit_remove:
    sysfs_remove_group(&client->dev.kobj, &ESC601_PSU_group);
#ifdef USB_CTRL_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESC601_USB_group);
#endif
    sysfs_remove_group(&client->dev.kobj, &ESC601_LED_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_Reset_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_Sensor_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_INT_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_QSFP_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_FAN_group);
#ifdef EEPROM_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESC601_EEPROM_group);
#endif
#ifdef ASPEED_BMC_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESC601_BMC_group);
#endif
    sysfs_remove_group(&client->dev.kobj, &ESC601_SYS_group);

exit_free:
    kfree(data);
exit:
    return status;
}

static int Cameo_i2c_remove(struct i2c_client *client)
{
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &ESC601_PSU_group);
#ifdef USB_CTRL_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESC601_USB_group);
#endif
    sysfs_remove_group(&client->dev.kobj, &ESC601_LED_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_Reset_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_Sensor_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_INT_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_QSFP_group);
    sysfs_remove_group(&client->dev.kobj, &ESC601_FAN_group);
#ifdef EEPROM_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESC601_EEPROM_group);
#endif
#ifdef ASPEED_BMC_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESC601_BMC_group);
#endif
    sysfs_remove_group(&client->dev.kobj, &ESC601_SYS_group);

    kfree(data);
    return 0;
}

static const struct i2c_device_id Cameo_i2c_id[] =
{
    { "ESC_601_i2c", 0 },
    {},
};
MODULE_DEVICE_TABLE(i2c, Cameo_i2c_id);

static struct i2c_driver Cameo_i2c_driver =
{
    .class        = I2C_CLASS_HWMON,
    .driver =
    {
        .name     = "ESC_601_i2c",
    },
    .probe        = Cameo_i2c_probe,
    .remove       = Cameo_i2c_remove,
    .id_table     = Cameo_i2c_id,
    .address_list = normal_i2c,
};

/*For main Switch board*/
static struct i2c_board_info ESC_601_i2c_info[] __initdata =
{
    {
        I2C_BOARD_INFO("ESC_601_i2c", 0x30),
        .platform_data = NULL,
    },
};

/*For QSFP Port 01 - 16*/
static struct i2c_board_info Cameo_CPLD_2_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_CPLD_2", 0x31),
        .platform_data = NULL,
    },
};
/*For QSFP Port 17 - 32*/
static struct i2c_board_info Cameo_CPLD_3_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_CPLD_3", 0x32),
        .platform_data = NULL,
    },
};
/*For Fan status*/
static struct i2c_board_info Cameo_CPLD_4_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_CPLD_4", 0x23),
        .platform_data = NULL,
    },
};
#ifdef I2C_SWITCH_WANTED
/*0x73*/
static struct i2c_board_info Cameo_Switch_1_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_Switch_1", 0x73),
        .platform_data = NULL,
    },
};

/*0x77*/
static struct i2c_board_info Cameo_Switch_2_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_Switch_2", 0x77),
        .platform_data = NULL,
    },
};
#endif
#ifdef THEMAL_WANTED
/*0x4c*/
static struct i2c_board_info Cameo_Sensor_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_Sensor", 0x4c),
        .platform_data = NULL,
    },
};

/*0x4c*/
static struct i2c_board_info Cameo_MAC_Sensor_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_MAC_Sensor", 0x68),
        .platform_data = NULL,
    },
};

/*0x2e*/
static struct i2c_board_info Cameo_Sensor_fan_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_Sensor_fan", 0x2e),
        .platform_data = NULL,
    },
};
#endif
#ifdef QSFP_WANTED
/*0x74*/
static struct i2c_board_info Cameo_QSFP_Switch_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_QSFP_Switch", 0x74),
        .platform_data = NULL,
    },
};

/*0x50*/
static struct i2c_board_info Cameo_QSFP_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_QSFP", 0x50),
        .platform_data = NULL,
    },
};
#endif
#ifdef EEPROM_WANTED
/*0x56*/
static struct i2c_board_info Cameo_EEPROM_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_EEPROM", 0x56),
        .platform_data = NULL,
    },
};
#endif

#ifdef ASPEED_BMC_WANTED
static struct i2c_board_info Cameo_BMC_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_BMC", 0x14),
        .platform_data = NULL,
    },
};
#endif
static int __init Cameo_i2c_init(void)
{
    int ret;
    int cmp;
    char keyword[] = "SMBus I801";
    char buf1[128];
    struct i2c_adapter *i2c_adap;
    struct file *fp;  
    mm_segment_t fs;  
    loff_t pos; 
 
    fp = filp_open("/sys/class/i2c-dev/i2c-0/name", O_RDONLY , 0644);  
    if (IS_ERR(fp)) {  
        printk("Open file FAILED\n");  
        return -1;  
    } 

    fs = get_fs();  
    set_fs(KERNEL_DS);
    pos = 0;
    ret = vfs_read(fp, buf1, sizeof(buf1), &pos);
    set_fs(fs);
    filp_close(fp, NULL);
	if(ret<0)
	  return -1;	  
	buf1[ret-1]='\0';
    printk("Detect %s\n", buf1);
    cmp = strncmp(keyword, buf1, sizeof(keyword)-1);	
	
    if(cmp == 0)
    {
        i2c_adap = i2c_get_adapter(0);
        printk("SMBus I801 is at bus 0\n");
    }
    else
    {
        i2c_adap = i2c_get_adapter(1);
        printk("SMBus I801 is at bus 1\n");
    }

    debug_print((KERN_DEBUG "Cameo_i2c_init\n"));
    if (i2c_adap == NULL)
    {
        printk("ERROR: i2c_get_adapter FAILED!\n");
        return -1;
    }
    ESC_601_i2c_client = i2c_new_device(i2c_adap, &ESC_601_i2c_info[0]);
    Cameo_CPLD_2_client = i2c_new_device(i2c_adap, &Cameo_CPLD_2_info[0]);
    Cameo_CPLD_3_client = i2c_new_device(i2c_adap, &Cameo_CPLD_3_info[0]);
    Cameo_CPLD_4_client = i2c_new_device(i2c_adap, &Cameo_CPLD_4_info[0]);
#ifdef I2C_SWITCH_WANTED
    Cameo_Switch_1_client = i2c_new_device(i2c_adap, &Cameo_Switch_1_info[0]);
    Cameo_Switch_2_client = i2c_new_device(i2c_adap, &Cameo_Switch_2_info[0]);
#endif
#ifdef THEMAL_WANTED
    Cameo_Sensor_client = i2c_new_device(i2c_adap, &Cameo_Sensor_info[0]);
    Cameo_MAC_Sensor_client = i2c_new_device(i2c_adap, &Cameo_MAC_Sensor_info[0]);
    Cameo_Sensor_fan_client = i2c_new_device(i2c_adap, &Cameo_Sensor_fan_info[0]);
#endif
#ifdef QSFP_WANTED
    Cameo_QSFP_Switch_client = i2c_new_device(i2c_adap, &Cameo_QSFP_Switch_info[0]);
    Cameo_QSFP_client = i2c_new_device(i2c_adap, &Cameo_QSFP_info[0]);
#endif
#ifdef EEPROM_WANTED
    Cameo_EEPROM_client = i2c_new_device(i2c_adap, &Cameo_EEPROM_info[0]);
#endif

#ifdef ASPEED_BMC_WANTED
    Cameo_BMC_client = i2c_new_device(i2c_adap, &Cameo_BMC_info[0]);
#endif

    if (ESC_601_i2c_info == NULL || Cameo_CPLD_2_info == NULL || Cameo_CPLD_3_info == NULL || Cameo_CPLD_4_info == NULL )
    {
        printk("ERROR: i2c_new_device FAILED!\n");
        return -1;
    }
#ifdef I2C_SWITCH_WANTED    
    if (Cameo_Switch_1_info == NULL || Cameo_Switch_2_info == NULL )
    {
        printk("ERROR: i2c_new_device FAILED!\n");
        return -1;
    }
#endif
#ifdef THEMAL_WANTED
    if (Cameo_Sensor_info == NULL || Cameo_MAC_Sensor_info == NULL || Cameo_Sensor_fan_info == NULL )
    {
        printk("ERROR: i2c_new_device FAILED!\n");
        return -1;
    }
#endif
#ifdef QSFP_WANTED
    if (Cameo_QSFP_Switch_info == NULL || Cameo_QSFP_info == NULL )
    {
        printk("ERROR: i2c_new_device FAILED!\n");
        return -1;
    }
#endif
#ifdef EEPROM_WANTED
    if (Cameo_EEPROM_info == NULL )
    {
        printk("ERROR: i2c_new_device FAILED!\n");
        return -1;
    }
#endif

#ifdef ASPEED_BMC_WANTED
    if (Cameo_BMC_info == NULL )
    {
        printk("ERROR: i2c_new_device FAILED!\n");
        return -1;
    }
#endif
    i2c_put_adapter(i2c_adap);
    ret = i2c_add_driver(&Cameo_i2c_driver);
    printk(KERN_ALERT "ESC601-32Q i2c Driver Version: %s\n", DRIVER_VERSION);
    printk(KERN_ALERT "ESC601-32Q i2c Driver INSTALL SUCCESS\n");
    return ret;
}

static void __exit Cameo_i2c_exit(void)
{
    i2c_unregister_device(ESC_601_i2c_client);
    i2c_unregister_device(Cameo_CPLD_2_client);
    i2c_unregister_device(Cameo_CPLD_3_client);
    i2c_unregister_device(Cameo_CPLD_4_client);
#ifdef I2C_SWITCH_WANTED
    i2c_unregister_device(Cameo_Switch_1_client);
    i2c_unregister_device(Cameo_Switch_2_client);
#endif
#ifdef THEMAL_WANTED
    i2c_unregister_device(Cameo_Sensor_client);
    i2c_unregister_device(Cameo_MAC_Sensor_client);
    i2c_unregister_device(Cameo_Sensor_fan_client);
#endif
#ifdef QSFP_WANTED
    i2c_unregister_device(Cameo_QSFP_Switch_client);
    i2c_unregister_device(Cameo_QSFP_client);
#endif
#ifdef EEPROM_WANTED
    i2c_unregister_device(Cameo_EEPROM_client);
#endif

#ifdef ASPEED_BMC_WANTED
    i2c_unregister_device(Cameo_BMC_client);
#endif
    i2c_del_driver(&Cameo_i2c_driver);
    printk(KERN_ALERT "ESC601-32Q i2c Driver UNINSTALL SUCCESS\n");
}

MODULE_AUTHOR("Cameo Inc.");
MODULE_DESCRIPTION("Cameo ESC601-32Q i2c Driver");
MODULE_LICENSE("GPL");
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "Enable debugging (0-1)");

module_init(Cameo_i2c_init);
module_exit(Cameo_i2c_exit);
