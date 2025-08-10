/* An hwmon driver for Cameo ESQC610-56SQ Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-esqc610-56sq.h"

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x30, 0x31, 0x32, I2C_CLIENT_END };

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

static ssize_t psu_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 bmc_present = -EPERM;
    u8 status = -EPERM;
    u8 mask = 0x1;
    u8 res = 0x1;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    bmc_present = i2c_smbus_read_byte_data(ESQC_610_i2c_client, BMC_PRESENT_OFFSET);
    if (bmc_present & mask)
    {
        status = i2c_smbus_read_byte_data(Cameo_BMC_client, 0xc0); //BMC 0x14 0xc0 
    }
    else
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_5_client, 0xa0); //CPLD 0x35 0xa0 
    }
    debug_print((KERN_DEBUG "DEBUG : PSU_PRESENT status = %x\n",status));
    sprintf(buf, "");
    switch (attr->index)
    {
        case PSU_PRESENT:
            for (i = 1; i <= 2; i++)
            {
                if (status & res)
                {
                    sprintf(buf, "%sPSU %d is present\n", buf, i);
                }
                else
                {
                    sprintf(buf, "%sPSU %d is not present\n", buf, i);
                }
                res = res << 1;
            }
            break;
        case PSU_STATUS:
            res = 0x1;
            res = res << 2;
            if (status & res)
            {
                sprintf(buf, "%sPSU 1 is not power Good\n", buf);
            }
            else
            {
                sprintf(buf, "%sPSU 1 is power Good\n", buf);
            }
            res = 0x1;
            res = res << 3;
            if (status & res)
            {
                sprintf(buf, "%sPSU 2 is not power Good\n", buf);
            }
            else
            {
                sprintf(buf, "%sPSU 2 is power Good\n", buf);
            }
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
    
    bmc_present = i2c_smbus_read_byte_data(ESQC_610_i2c_client, BMC_PRESENT_OFFSET);
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

        sprintf(buf, "%sPSU %d VIN          is %d\n", buf, module_num, psu_status[0]);
        sprintf(buf, "%sPSU %d IIN          is %d\n", buf, module_num, psu_status[1]);
        sprintf(buf, "%sPSU %d VOUT         is %d\n", buf, module_num, psu_status[2]);
        sprintf(buf, "%sPSU %d IOUT         is %d\n", buf, module_num, psu_status[3]);
        sprintf(buf, "%sPSU %d TEMP_1       is %d\n", buf, module_num, psu_status[4]);
        sprintf(buf, "%sPSU %d FAN_SPEED    is %d\n", buf, module_num, psu_status[5]);
        sprintf(buf, "%sPSU %d POUT         is %d\n", buf, module_num, psu_status[6]);
        sprintf(buf, "%sPSU %d PIN          is %d\n", buf, module_num, psu_status[7]);
        sprintf(buf, "%sPSU %d MFR_MODEL    is %d\n", buf, module_num, psu_status[8]);
        sprintf(buf, "%sPSU %d MFR_IOUT_MAX is %d\n", buf, module_num, psu_status[9]);
        sprintf(buf, "%sPSU %d VMODE        is %d\n", buf, module_num, psu_status[10]);
    }
    else
    {
        sprintf(buf, "%sBMC Module is not present\n", buf);
    }
    return sprintf(buf, "%s", buf);
}

static ssize_t dc_chip_switch_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 bmc_present = -EPERM;
    u8 dc_table [12] = {0x90, 0x91, 0x92, 0x94, 0x95, 0x96, 0x98, 0x99, 0x9a, 0x9c, 0x9d, 0x9e};
    u16 dc_status [12] = {0}; 
    u8 mask = 0x1;
    u8 i = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "\n");
    if (attr->index == DC_CHIP_SWITCH)
    {
        bmc_present = i2c_smbus_read_byte_data(ESQC_610_i2c_client, BMC_PRESENT_OFFSET); //CPLD 0x30 0xa4
        if (bmc_present & mask)
        {
            for(i = 0; i < 12; i ++)
            {
                dc_status[i] = i2c_smbus_read_word_data(Cameo_BMC_client, dc_table[i]);
            }
            sprintf(buf, "%sTPS53681 0x6e page 0 Vin  0x%x\n", buf, dc_status[0]);
            sprintf(buf, "%sTPS53681 0x6e page 0 Iout 0x%x\n", buf, dc_status[1]);
            sprintf(buf, "%sTPS53681 0x6e page 0 Pout 0x%x\n", buf, dc_status[2]);
            sprintf(buf, "%sTPS53681 0x6e page 1 Vin  0x%x\n", buf, dc_status[3]);
            sprintf(buf, "%sTPS53681 0x6e page 1 Iout 0x%x\n", buf, dc_status[4]);
            sprintf(buf, "%sTPS53681 0x6e page 1 Pout 0x%x\n", buf, dc_status[5]);
            sprintf(buf, "%sTPS53681 0x70 page 0 Vin  0x%x\n", buf, dc_status[6]);
            sprintf(buf, "%sTPS53681 0x70 page 0 Iout 0x%x\n", buf, dc_status[7]);
            sprintf(buf, "%sTPS53681 0x70 page 0 Pout 0x%x\n", buf, dc_status[8]);
            sprintf(buf, "%sTPS53681 0x70 page 1 Vin  0x%x\n", buf, dc_status[9]);
            sprintf(buf, "%sTPS53681 0x70 page 1 Iout 0x%x\n", buf, dc_status[10]);
            sprintf(buf, "%sTPS53681 0x70 page 1 Pout 0x%x\n", buf, dc_status[11]);
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

    status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa0);
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
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESQC_610_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_lock\n"));
    status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa0);
    debug_print((KERN_DEBUG "DEBUG : USB_POWER status = %x\n",status));
    if (attr->index == USB_POWER)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == TURN_ON)
        {
            value = status | USB_ON;
            debug_print((KERN_DEBUG "DEBUG : USB_POWER value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESQC_610_i2c_client, 0xa0, value);
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
            result = i2c_smbus_write_byte_data(ESQC_610_i2c_client, 0xa0, value);
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
        status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa0);
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
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESQC_610_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_lock\n"));
    if (attr->index == LED_CTRL)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == TURN_ON)
        {
            status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa0);
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL status = %x\n",status));
            value = status | LED_ON;
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESQC_610_i2c_client, 0xa0, value);
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
            status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa0);
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL status = %x\n",status));
            value = status & LED_OFF;
            debug_print((KERN_DEBUG "DEBUG : LED_CTRL value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESQC_610_i2c_client, 0xa0, value);
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

static ssize_t sys_led_ctrl_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 res = 0x1;
    int i;
    int led_a_status = 0;
    int led_g_status = 0;
    int led_b_status = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa2); //to get register 0x30 0xa2
    debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_get led status = %x\n",status));
    sprintf(buf, "");
    
    for (i = 1; i <= 4; i++)
    {
        if ( i == attr->index)
        {
            if (status & res)
            {
                led_a_status = TURN_ON;
            }
            else
            {
                led_a_status = TURN_OFF;
            }
        }
        res = res << 1;
        if( i == (attr->index + 1) )
        {
            if (status & res)
            {
                led_g_status = TURN_ON;
            }
            else
            {
                led_g_status = TURN_OFF;
            }
        }
        res = res << 1;
    }
    res = 0x1;

    status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa3); //to get register 0x30 0xa3
    debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_get blk status = %x\n",status));
    for (i = 1; i <= 4; i++)
    {
        if ( i == attr->index)
        {
            if (status & res)
            {
                led_b_status = TURN_ON;
            }
            else
            {
                led_b_status = TURN_OFF;
            }
        }
        res = res << 1;
    }
    
    if(attr->index == 1)
    {
        sprintf(buf, "%sSYS LED is set to ", buf);
    }
    else if(attr->index == 2)
    {
        sprintf(buf, "%sFLOW LED is set to ", buf);
    }
    else if(attr->index == 3)
    {
        sprintf(buf, "%sSwitch LED 1 is set to ", buf);
    }
    else if(attr->index == 4)
    {
        sprintf(buf, "%sSwitch LED 2 is set to ", buf);
    }
    
    if(led_a_status == TURN_ON && led_b_status == TURN_ON)
    {
        sprintf(buf, "%samber and blink\n", buf);
    }
    else if(led_a_status == TURN_ON && led_b_status == TURN_OFF)
    {
        sprintf(buf, "%samber\n", buf);
    }
    else if(led_g_status == TURN_ON && led_b_status == TURN_ON)
    {
        sprintf(buf, "%sgreen and blink\n", buf);
    }
    else if(led_g_status == TURN_ON && led_b_status == TURN_OFF)
    {
        sprintf(buf, "%sgreen\n", buf);
    }
    else
    {
        sprintf(buf, "%sOFF\n", buf);
    }
    return sprintf(buf, "%s", buf);
}
static ssize_t sys_led_ctrl_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 led_value  = -EPERM;
    u8 blk_value  = -EPERM;
    u8 result = -EPERM;
    u8 offset = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESQC_610_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_set lock\n"));
    led_value = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa2);
    debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_set led_value = %x\n",led_value));
    blk_value = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa3);
    debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_set blk_value = %x\n",blk_value));
    if (attr->index != 0)
    {
        i = simple_strtol(buf, NULL, 10);
        debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_set value = %d\n",i));
        debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_set led attr->index = %d\n",attr->index));
        if(attr->index == 1)
        {
            offset = 0;
        }
        else
        {
            offset = 2*((attr->index)-1);
        }
        switch(i)
        {
            case SWITCH_LED_OFF: //i=0
                led_value   &= ~(0x03 << offset);
                blk_value   &= ~(1 << ((attr->index)-1));
                break;
            case SWITCH_LED_A_N: //i=1
                led_value   &= ~(0x03 << offset);
                led_value   |= (0x01 << offset);
                blk_value   &= ~(1 << ((attr->index)-1));
                break;
            case SWITCH_LED_A_B: //i=2
                led_value   &= ~(0x03 << offset);
                led_value   |= (0x01 << offset);
                blk_value   |= (1 << ((attr->index)-1));
                break;
            case SWITCH_LED_G_N: //i=3
                led_value   &= ~(0x03 << offset);
                led_value   |= (0x02 << offset);
                blk_value   &= ~(1 << ((attr->index)-1));
                break;
            case SWITCH_LED_G_B: //i=4
                led_value   &= ~(0x03 << offset);
                led_value   |= (0x02 << offset);
                blk_value   |= (1 << ((attr->index)-1));
                break;
            default:
                printk(KERN_ALERT "Switch LED set wrong Value\n");
        }
        debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_set led_value = %x\n",led_value));
        debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_set blk_value = %x\n",blk_value));
        result  = i2c_smbus_write_byte_data(ESQC_610_i2c_client, 0xa2, led_value);
        result |= i2c_smbus_write_byte_data(ESQC_610_i2c_client, 0xa3, blk_value);
        debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_set result = %x\n",result));
        if (result < 0)
        {
            printk(KERN_ALERT "ERROR: sys_led_ctrl_set SYS_LED_OFF FAILED!\n");
        }
        else
        {
            debug_print((KERN_DEBUG "Switch LED is set Success\n"));
        }
    }
    mutex_unlock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : sys_led_ctrl_set unlock\n"));
    return count;
}

static ssize_t reset_mac_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = 0;
    u8 value  = 0;
    u8 result = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESQC_610_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_lock\n"));
    status  = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa1);
    debug_print((KERN_DEBUG "DEBUG : RESET_MAC status = %x\n",status));
    if (attr->index == RESET_MAC)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == 0)
        {
            value = 0x0;
            debug_print((KERN_DEBUG "DEBUG : RESET_MAC value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESQC_610_i2c_client, 0xa1, value);
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

static ssize_t shutdown_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = 0;
    u8 value  = 0;
    u8 result = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *data = i2c_get_clientdata(ESQC_610_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_lock\n"));
    status  = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa1);
    debug_print((KERN_DEBUG "DEBUG : shutdown_set status = %x\n",status));
    if (attr->index == SHUTDOWN_DUT)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i == 1)
        {
            value = status & 0xef;
            debug_print((KERN_DEBUG "DEBUG : shutdown_set value = %x\n",value));
            result = i2c_smbus_write_byte_data(ESQC_610_i2c_client, 0xa1, value);
            debug_print((KERN_DEBUG "DEBUG : shutdown_set result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: shutdown_set set FAILED!\n");
            }
            else
            {
                debug_print((KERN_DEBUG "Switch is shutdown\n"));
            }
        }
        else
        {
            printk(KERN_ALERT "shutdown_set set wrong Value\n");
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
    
    status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xc0);
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

#endif 

static ssize_t themal_mask_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 res = 0x1;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xc1);
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
    struct Cameo_i2c_data *data = i2c_get_clientdata(ESQC_610_i2c_client);
    
    mutex_lock(&data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : themal_mask_set mutex_lock\n"));
    i = simple_strtol(buf, NULL, 10);
    debug_print((KERN_DEBUG "DEBUG : themal_mask_set input %d\n", i));
    status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xc1);
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
    result = i2c_smbus_write_byte_data(ESQC_610_i2c_client, 0xc1, value);
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
    status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xd0);
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

static ssize_t sfp_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM;
    u8 present_reg[7] = {0x00, 0x80, 0x81, 0x82, 0x83, 0x80, 0x81}; //CPLD present register
    u8 rx_loss_reg[7] = {0x00, 0x90, 0x91, 0x92, 0x93, 0x90, 0x91}; //CPLD present register
    u8 tx_ctrl_reg[7] = {0x00, 0x70, 0x71, 0x72, 0x73, 0x70, 0x71}; //CPLD present register
    u8 mask = 0x1;
    u8 port_num;
    u8 i,j;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == SFP_PRESENT)
    {
        sprintf(buf, "");
        port_num = 1;
        for (i = 1; i <= 6; i++)
        {
            if(i <= 4) //for 1~32 port read 0x31 CPLD
            {
                status = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, present_reg[i]);
            }
            else
            {
                status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, present_reg[i]);
            }
            debug_print((KERN_DEBUG "DEBUG : SFP_PRESENT status = %x\n",status));
            for (j = 1; j <= 8; j++)
            {
                if (status & mask)
                {
                    sprintf(buf, "%sSFP %02d is not present\n", buf, port_num);
                }
                else
                {
                    sprintf(buf, "%sSFP %02d is present\n", buf, port_num);
                }
                port_num++;
                mask = mask << 1;
            }
            mask = 0x1;
        }
    }
    else if (attr->index == SFP_RX_LOSS)
    {
        sprintf(buf, "");
        port_num = 1;
        for (i = 1; i <= 6; i++)
        {
            if(i <= 4) //for 1~32 port read 0x31 CPLD
            {
                status = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, rx_loss_reg[i]);
            }
            else
            {
                status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, rx_loss_reg[i]);
            }
            debug_print((KERN_DEBUG "DEBUG : SFP_PRESENT status = %x\n",status));
            for (j = 1; j <= 8; j++)
            {
                if (status & mask)
                {
                    sprintf(buf, "%sSFP %02d loss of signal\n", buf, port_num);
                }
                else
                {
                    sprintf(buf, "%sSFP %02d signal detected\n", buf, port_num);
                }
                port_num++;
                mask = mask << 1;
            }
            mask = 0x1;
        }
    }
    else if (attr->index == SFP_TX_STAT)
    {
        sprintf(buf, "");
        port_num = 1;
        for (i = 1; i <= 6; i++)
        {
            if(i <= 4) //for 1~32 port read 0x31 CPLD
            {
                status = i2c_smbus_read_byte_data(Cameo_CPLD_2_client, tx_ctrl_reg[i]);
            }
            else
            {
                status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, tx_ctrl_reg[i]);
            }
            debug_print((KERN_DEBUG "DEBUG : SFP_PRESENT status = %x\n",status));
            for (j = 1; j <= 8; j++)
            {
                if (status & mask)
                {
                    sprintf(buf, "%sSFP %02d Disable TX\n", buf, port_num);
                }
                else
                {
                    sprintf(buf, "%sSFP %02d Enable TX\n", buf, port_num);
                }
                port_num++;
                mask = mask << 1;
            }
            mask = 0x1;
        }
    }
    return sprintf(buf, "%s\n", buf);
}

static ssize_t sfp_tx_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 status = -EPERM;
    u8 result = -EPERM;
    u8 input = 0;
    u8 port_num = 0;
    u8 offset = 0;
    u8 reg = 0x0;
    u8 tx_ctrl_reg[7] = {0x00, 0x70, 0x71, 0x72, 0x73, 0x70, 0x71}; //CPLD present register
    
    struct i2c_client *target_client = NULL; 
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct Cameo_i2c_data *CPLD_2_data = i2c_get_clientdata(Cameo_CPLD_2_client);
	struct Cameo_i2c_data *CPLD_3_data = i2c_get_clientdata(Cameo_CPLD_3_client);
    
    input = simple_strtol(buf, NULL, 10); //user input, 0 disable, 1 enable
    port_num = attr->index;
    if (port_num >= 1 && port_num <= 8)
    {
        target_client = Cameo_CPLD_2_client;
        reg = tx_ctrl_reg[1];
        offset = port_num;
    }
    else if(port_num >= 9 && port_num <= 16)
    {
        target_client = Cameo_CPLD_2_client;
        reg = tx_ctrl_reg[2];
        offset = port_num % 8;
    }
    else if(port_num >= 17 && port_num <= 24)
    {
        target_client = Cameo_CPLD_2_client;
        reg = tx_ctrl_reg[3];
        offset = port_num % 8;
    }
    else if(port_num >= 25 && port_num <= 32)
    {
        target_client = Cameo_CPLD_2_client;
        reg = tx_ctrl_reg[4];
        offset = port_num % 8;
    }
    else if(port_num >= 33 && port_num <= 40)
    {
        target_client = Cameo_CPLD_3_client;
        reg = tx_ctrl_reg[5];
        offset = port_num % 8;
    }
    else if(port_num >= 41 && port_num <= 48)
    {
        target_client = Cameo_CPLD_3_client;
        reg = tx_ctrl_reg[6];
        offset = port_num % 8;
    }
    else 
    {
        printk(KERN_ALERT "sfp_tx_set wrong value\n");
        return count;
    }
    
    status = i2c_smbus_read_byte_data(target_client, reg);
    debug_print((KERN_DEBUG "DEBUG : sfp_tx_set status = %x\n",status));
    if(input == TURN_ON)
    {
        status &= ~(1 << (offset-1));
    }
    else if(input == TURN_OFF)
    {
        status |= (1 << (offset-1));
    }
    else
    {
        printk(KERN_ALERT "sfp_tx_set set wrong value\n");
        return count;
    }
    debug_print((KERN_DEBUG "DEBUG : sfp_tx_set value = %x\n",status));
    mutex_lock(&CPLD_2_data->update_lock);
    mutex_lock(&CPLD_3_data->update_lock);
    result = i2c_smbus_write_byte_data(target_client, reg, status);
    mutex_unlock(&CPLD_2_data->update_lock);
    mutex_unlock(&CPLD_3_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : sfp_tx_set result = %x\n",result));
    if (result < 0)
    {
        printk(KERN_ALERT "ERROR: sfp_tx_set sfp %d FAILED!\n", port_num);
    }
    else
    {
        debug_print((KERN_DEBUG "sfp_tx_set port %02d : %d\n", port_num, input));
    }
    return count;
}

static ssize_t low_power_all_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM; //Low power mode 01-08 port stat

    u8 res = 0x1;
    int i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);


    status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x62); //CPLD 0x32 0x62
    debug_print((KERN_DEBUG "DEBUG : low_power_all_get status = %x\n",status));

    sprintf(buf, "");
    if (attr->index == QSFP_LOW_POWER_ALL)
    {
        for (i = 1; i <= 8; i++)
        {
            if (status & res)
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
    }
    return sprintf(buf, "%s\n", buf);
}
static ssize_t low_power_all_set(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    u8 value  = 0x0;
    u8 status = -EPERM; //Low power mode 01-08 port stat
    u8 result = -EPERM;
    u8 j = 0;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct Cameo_i2c_data *CPLD_3_data = i2c_get_clientdata(Cameo_CPLD_3_client);
    
    if (attr->index == QSFP_LOW_POWER_ALL)
    {
        i = simple_strtol(buf, NULL, 10);
        status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x62);
        debug_print((KERN_DEBUG "DEBUG : low_power_all_set status = %x\n",status));
        mutex_lock(&CPLD_3_data->update_lock);
        debug_print((KERN_DEBUG "DEBUG : low_power_all_set mutex_lock\n"));
        if (i == TURN_ON)
        {
            value = 0xf;
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL value = %x\n",value));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x62, value);
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_LOW_POWER_ALL set ON FAILED!\n");
            }
            else
            {
                for(j=1; j<=8; j++)
                {
                    debug_print((KERN_DEBUG "QSFP %02d low power mode: ON\n", j));
                }
            }
        }
        else if(i == TURN_OFF)
        {
            value = 0x0;
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL value = %x\n",value));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x62, value);
            debug_print((KERN_DEBUG "DEBUG : QSFP_LOW_POWER_ALL result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: QSFP_LOW_POWER_ALL set OFF FAILED!\n");
            }
            else
            {
                for(j=1; j<=8; j++)
                {
                    debug_print((KERN_DEBUG "QSFP %02d low power mode: OFF\n", j));
                }
            }
        }
        else
        {
            printk(KERN_ALERT "QSFP_LOW_POWER_ALL set wrong value\n");
        }
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
        status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x62);
        debug_print((KERN_DEBUG "DEBUG : low_power_get status = %x\n", status));
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
    else
    {
        printk(KERN_ALERT "low_power_get get %02d wrong value\n", i);
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
	struct Cameo_i2c_data *CPLD_3_data = i2c_get_clientdata(Cameo_CPLD_3_client);
    
    i = attr->index;
    j = simple_strtol(buf, NULL, 10);
    debug_print((KERN_DEBUG "DEBUG : low_power_set port %d\n", i));
    mutex_lock(&CPLD_3_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : low_power_set mutex_lock\n"));
    if (i >= 1 && i <= 8)
    {
        status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x62);
        debug_print((KERN_DEBUG "DEBUG : low_power_set status = %x\n",status));
        if( j == TURN_ON)
        {
            status |= (1 << (i-1));
            debug_print((KERN_DEBUG "DEBUG : low_power_set value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x62, status);
            debug_print((KERN_DEBUG "DEBUG : low_power_set result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: low_power_set qsfp %d ON FAILED!\n", i);
            }
            else
            {
                debug_print((KERN_DEBUG "QSFP %02d low power mode: ON\n", i));
            }
        }
        else if( j == TURN_OFF)
        {
            status &= ~(1 << (i-1));
            debug_print((KERN_DEBUG "DEBUG : low_power_set value = %x\n",status));
            result = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x62, status);
            debug_print((KERN_DEBUG "DEBUG : low_power_set result = %x\n",result));
            if (result < 0)
            {
                printk(KERN_ALERT "ERROR: low_power_set qsfp %d OFF FAILED!\n", i);
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
	struct Cameo_i2c_data *CPLD_3_data = i2c_get_clientdata(Cameo_CPLD_3_client);

    mutex_lock(&CPLD_3_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set mutex_lock\n"));
    if (attr->index == QSFP_RESET)
    {
        i = simple_strtol(buf, NULL, 10);
        if (i >= 1 && i <= 8)
        {
            value  = 0;
            value  = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x72);
            debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set value = %x\n",value));
            value ^= (1 << (i - 1));
            debug_print((KERN_DEBUG "DEBUG : qsfp_reset_set set value = %x\n",value));
            status = i2c_smbus_write_byte_data(Cameo_CPLD_3_client, 0x72, value);
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
    mutex_unlock(&CPLD_3_data->update_lock);
    debug_print((KERN_DEBUG "DEBUG : mutex_unlock\n"));
    return count;
}

static ssize_t qsfp_status_get(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 status = -EPERM; //qsfp_status 01-08 port stat
    u8 res = 0x1;
    u16 i;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    
    if (attr->index == QSFP_PRESENT)
    {
        sprintf(buf, "");
        status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x82);
        for (i = 1; i <= 8; i++)
        {
            if (status & res)
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
    }
    if (attr->index == QSFP_INT)
    {
        sprintf(buf, "");
        status = i2c_smbus_read_byte_data(Cameo_CPLD_3_client, 0x92);
        debug_print((KERN_DEBUG "DEBUG : QSFP_INT_1 status = %x\n",status));
        res = 0x1;
        for (i = 1; i <= 8; i++)
        {
            if (status & res)
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
    }
    return sprintf(buf, "%s\n", buf);
}

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
        if(i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa4) == 0x1) //check BMC present
        {
            status = i2c_smbus_read_byte_data(Cameo_BMC_client, 0x80);
        }
        else
        {
            status = i2c_smbus_read_byte_data(Cameo_CPLD_4_client, 0x0);
        }
        debug_print((KERN_DEBUG "DEBUG : FAN_STATUS status = %x\n",status));
        for(i=1; i<=4; i++)
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
        if(i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa4) == 0x1) //check BMC present
        {
            status = i2c_smbus_read_byte_data(Cameo_BMC_client, 0x81);
        }
        else
        {
            status = i2c_smbus_read_byte_data(Cameo_CPLD_4_client, 0x01);
        }
        debug_print((KERN_DEBUG "DEBUG : FAN_PRESENT status = %x\n",status));
        for(i=1; i<=4; i++)
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
        if(i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa4) == 0x1) //check BMC present
        {
            status = i2c_smbus_read_byte_data(Cameo_BMC_client, 0x82);
        }
        else
        {
            status = i2c_smbus_read_byte_data(Cameo_CPLD_4_client, 0x02);
        }
        debug_print((KERN_DEBUG "DEBUG : FAN_POWER status = %x\n",status));
        for(i=1; i<=4; i++)
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
        if(i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa4) == 0x1) //check BMC present
        {
            target_client = Cameo_BMC_client;
            res = i2c_smbus_read_byte_data(Cameo_BMC_client, 0x81); //check Fan present
        }
        else
        {
            target_client = Cameo_CPLD_4_client;
            res = i2c_smbus_read_byte_data(Cameo_CPLD_4_client, 0x01); //check Fan present
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
        status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0xa4);
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
        status = i2c_smbus_read_byte_data(ESQC_610_i2c_client, 0x20);
    }
    sprintf(buf, "%sHW version is 0x%x\n", buf, status);
    return sprintf(buf, "%s\n", buf);
}

/* end of function */

static int Cameo_i2c_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    struct Cameo_i2c_data *data;
    struct Cameo_i2c_data *CPLD_2_data;
    struct Cameo_i2c_data *CPLD_3_data;
    struct Cameo_i2c_data *CPLD_4_data;
    struct Cameo_i2c_data *CPLD_5_data;
#ifdef I2C_SWITCH_WANTED
    struct Cameo_i2c_data *Switch_1_data;
    struct Cameo_i2c_data *Switch_2_data;
#endif
#ifdef THEMAL_WANTED
    struct Cameo_i2c_data *Sensor_data;
    struct Cameo_i2c_data *Sensor_fan_data;
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
    CPLD_5_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!CPLD_5_data)
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
    Sensor_fan_data = kzalloc(sizeof(struct Cameo_i2c_data), GFP_KERNEL);
    if (!Sensor_fan_data)
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
    i2c_set_clientdata(Cameo_CPLD_5_client, CPLD_5_data);
#ifdef I2C_SWITCH_WANTED
    i2c_set_clientdata(Cameo_Switch_1_client, Switch_1_data);
    i2c_set_clientdata(Cameo_Switch_2_client, Switch_2_data);
#endif
#ifdef THEMAL_WANTED
    i2c_set_clientdata(Cameo_Sensor_client, Sensor_data);
    i2c_set_clientdata(Cameo_Sensor_fan_client, Sensor_fan_data);
#endif
    mutex_init(&CPLD_2_data->update_lock);
    mutex_init(&CPLD_3_data->update_lock);
    mutex_init(&CPLD_4_data->update_lock);
    mutex_init(&CPLD_5_data->update_lock);
#ifdef I2C_SWITCH_WANTED
    mutex_init(&Switch_1_data->update_lock);
    mutex_init(&Switch_2_data->update_lock);
#endif
#ifdef THEMAL_WANTED
    mutex_init(&Sensor_data->update_lock);
    mutex_init(&Sensor_fan_data->update_lock);
#endif
#ifdef ASPEED_BMC_WANTED
    mutex_init(&Cameo_BMC_data->update_lock);
#endif
    data->valid = 0;
    mutex_init(&data->update_lock);
    dev_info(&client->dev, "chip found\n");
    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_PSU_group);
    if (status)
    {
        goto exit_free;
    }
#ifdef USB_CTRL_WANTED
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_USB_group);
    if (status)
    {
        goto exit_free;
    }
#endif
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_LED_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_Reset_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_Sensor_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_INT_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_SFP_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_QSFP_group);
    if (status)
    {
        goto exit_free;
    }
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_FAN_group);
    if (status)
    {
        goto exit_free;
    }
#ifdef ASPEED_BMC_WANTED
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_BMC_group);
    if (status)
    {
        goto exit_free;
    }
#endif
    status = sysfs_create_group(&client->dev.kobj, &ESQC610_SYS_group);
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
    return 0;
exit_remove:
    sysfs_remove_group(&client->dev.kobj, &ESQC610_PSU_group);
#ifdef USB_CTRL_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESQC610_USB_group);
#endif
    sysfs_remove_group(&client->dev.kobj, &ESQC610_LED_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_Reset_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_Sensor_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_INT_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_SFP_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_QSFP_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_FAN_group);
#ifdef ASPEED_BMC_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESQC610_BMC_group);
#endif
    sysfs_remove_group(&client->dev.kobj, &ESQC610_SYS_group);

exit_free:
    kfree(data);
exit:
    return status;
}

static int Cameo_i2c_remove(struct i2c_client *client)
{
    struct Cameo_i2c_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_PSU_group);
#ifdef USB_CTRL_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESQC610_USB_group);
#endif
    sysfs_remove_group(&client->dev.kobj, &ESQC610_LED_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_Reset_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_Sensor_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_INT_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_SFP_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_QSFP_group);
    sysfs_remove_group(&client->dev.kobj, &ESQC610_FAN_group);
#ifdef ASPEED_BMC_WANTED
    sysfs_remove_group(&client->dev.kobj, &ESQC610_BMC_group);
#endif
    sysfs_remove_group(&client->dev.kobj, &ESQC610_SYS_group);

    kfree(data);
    return 0;
}

static const struct i2c_device_id Cameo_i2c_id[] =
{
    { "ESQC_610_i2c", 0 },
    {},
};
MODULE_DEVICE_TABLE(i2c, Cameo_i2c_id);

static struct i2c_driver Cameo_i2c_driver =
{
    .class        = I2C_CLASS_HWMON,
    .driver =
    {
        .name     = "ESQC_610_i2c",
    },
    .probe        = Cameo_i2c_probe,
    .remove       = Cameo_i2c_remove,
    .id_table     = Cameo_i2c_id,
    .address_list = normal_i2c,
};

/*For main Switch board*/
static struct i2c_board_info ESQC_610_i2c_info[] __initdata =
{
    {
        I2C_BOARD_INFO("ESQC_610_i2c", 0x30),
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
/*For Power status*/
static struct i2c_board_info Cameo_CPLD_5_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_CPLD_5", 0x35),
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

/*0x75*/
static struct i2c_board_info Cameo_Switch_2_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_Switch_2", 0x75),
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

/*0x2e*/
static struct i2c_board_info Cameo_Sensor_fan_info[] __initdata =
{
    {
        I2C_BOARD_INFO("Cameo_Sensor_fan", 0x2e),
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

    printk("Open file...\n");  
    fp = filp_open("/sys/class/i2c-dev/i2c-0/name", O_RDONLY , 0644);  
    if (IS_ERR(fp)) {  
        printk("Open file FAILED\n");  
        return -1;  
    } 

    fs = get_fs();  
    set_fs(KERNEL_DS);
    pos = 0;
    vfs_read(fp, buf1, sizeof(buf1), &pos);
    printk("Detect %s\n", buf1);
    cmp = strncmp(keyword, buf1, sizeof(keyword)-1);
    set_fs(fs);

    filp_close(fp, NULL);

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
    ESQC_610_i2c_client = i2c_new_device(i2c_adap, &ESQC_610_i2c_info[0]);
    Cameo_CPLD_2_client = i2c_new_device(i2c_adap, &Cameo_CPLD_2_info[0]);
    Cameo_CPLD_3_client = i2c_new_device(i2c_adap, &Cameo_CPLD_3_info[0]);
    Cameo_CPLD_4_client = i2c_new_device(i2c_adap, &Cameo_CPLD_4_info[0]);
    Cameo_CPLD_5_client = i2c_new_device(i2c_adap, &Cameo_CPLD_5_info[0]);
#ifdef I2C_SWITCH_WANTED
    Cameo_Switch_1_client = i2c_new_device(i2c_adap, &Cameo_Switch_1_info[0]);
    Cameo_Switch_2_client = i2c_new_device(i2c_adap, &Cameo_Switch_2_info[0]);
#endif
#ifdef THEMAL_WANTED
    Cameo_Sensor_client = i2c_new_device(i2c_adap, &Cameo_Sensor_info[0]);
    Cameo_Sensor_fan_client = i2c_new_device(i2c_adap, &Cameo_Sensor_fan_info[0]);
#endif
#ifdef ASPEED_BMC_WANTED
    Cameo_BMC_client = i2c_new_device(i2c_adap, &Cameo_BMC_info[0]);
#endif

    if (ESQC_610_i2c_client == NULL || Cameo_CPLD_2_client == NULL || Cameo_CPLD_3_client == NULL 
    || Cameo_CPLD_4_client == NULL || Cameo_CPLD_5_client == NULL)
    {
        printk("ERROR: ESQC_610_i2c_client FAILED!\n");
        return -1;
    }
#ifdef I2C_SWITCH_WANTED    
    if (Cameo_Switch_1_client == NULL || Cameo_Switch_2_client == NULL )
    {
        printk("ERROR: Cameo_Switch_client FAILED!\n");
        return -1;
    }
#endif
#ifdef THEMAL_WANTED
    if (Cameo_Sensor_client == NULL || Cameo_Sensor_fan_client == NULL )
    {
        printk("ERROR: Cameo_Sensor_client FAILED!\n");
        return -1;
    }
#endif
#ifdef ASPEED_BMC_WANTED
    if (Cameo_BMC_client == NULL )
    {
        printk("ERROR: Cameo_BMC_client FAILED!\n");
        return -1;
    }
#endif
    i2c_put_adapter(i2c_adap);
    ret = i2c_add_driver(&Cameo_i2c_driver);
    printk(KERN_ALERT "ESQC610-56SQ i2c Driver Version: %s\n", DRIVER_VERSION);
    printk(KERN_ALERT "ESQC610-56SQ i2c Driver INSTALL SUCCESS\n");
    return ret;
}

static void __exit Cameo_i2c_exit(void)
{
    i2c_unregister_device(ESQC_610_i2c_client);
    i2c_unregister_device(Cameo_CPLD_2_client);
    i2c_unregister_device(Cameo_CPLD_3_client);
    i2c_unregister_device(Cameo_CPLD_4_client);
    i2c_unregister_device(Cameo_CPLD_5_client);
#ifdef I2C_SWITCH_WANTED
    i2c_unregister_device(Cameo_Switch_1_client);
    i2c_unregister_device(Cameo_Switch_2_client);
#endif
#ifdef THEMAL_WANTED
    i2c_unregister_device(Cameo_Sensor_client);
    i2c_unregister_device(Cameo_Sensor_fan_client);
#endif
#ifdef ASPEED_BMC_WANTED
    i2c_unregister_device(Cameo_BMC_client);
#endif
    i2c_del_driver(&Cameo_i2c_driver);
    printk(KERN_ALERT "ESQC610-56SQ i2c Driver UNINSTALL SUCCESS\n");
}

MODULE_AUTHOR("Cameo Inc.");
MODULE_DESCRIPTION("Cameo ESQC610-56SQ i2c Driver");
MODULE_LICENSE("GPL");

module_init(Cameo_i2c_init);
module_exit(Cameo_i2c_exit);
