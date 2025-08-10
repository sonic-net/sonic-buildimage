/* An hwmon driver for Cameo esc600-128Q Innovium i2c Module */
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#include "x86-64-cameo-esc600-128q.h"
#include "x86-64-cameo-esc600-128q-common.h"
#include "x86-64-cameo-esc600-128q-thermal.h"

/* extern i2c_client */
extern struct i2c_client *Cameo_BMC_14_client;  //0x14 for BMC slave
/* end of extern i2c_client */

/* implement i2c_function */
ssize_t line_card_temp_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if( bmc_enable() == ENABLE)
    {
        switch (attr->index)
        {
            /*LINE_CARD_UP*/
            case LINE_CARD_1_UP_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_1_UP_TEMP_REG);
            break;
            case LINE_CARD_2_UP_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_2_UP_TEMP_REG);
            break;
            case LINE_CARD_3_UP_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_3_UP_TEMP_REG);
            break;
            case LINE_CARD_4_UP_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_4_UP_TEMP_REG);
            break;
            case LINE_CARD_5_UP_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_5_UP_TEMP_REG);
            break;
            case LINE_CARD_6_UP_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_6_UP_TEMP_REG);
            break;
            case LINE_CARD_7_UP_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_7_UP_TEMP_REG);
            break;
            case LINE_CARD_8_UP_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_8_UP_TEMP_REG);
            break;
            /*LINE_CARD_DOWN*/
            case LINE_CARD_1_DN_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_1_DN_TEMP_REG);
            break;
            case LINE_CARD_2_DN_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_2_DN_TEMP_REG);
            break;
            case LINE_CARD_3_DN_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_3_DN_TEMP_REG);
            break;
            case LINE_CARD_4_DN_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_4_DN_TEMP_REG);
            break;
            case LINE_CARD_5_DN_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_5_DN_TEMP_REG);
            break;
            case LINE_CARD_6_DN_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_6_DN_TEMP_REG);
            break;
            case LINE_CARD_7_DN_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_7_DN_TEMP_REG);
            break;
            case LINE_CARD_8_DN_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LINE_CARD_8_DN_TEMP_REG);
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

ssize_t themal_temp_get(struct device *dev, struct device_attribute *da, char *buf)
{
    int status = -EPERM;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    sprintf(buf, "");

    if( bmc_enable() == ENABLE)
    {
        switch (attr->index)
        {
            case NCT7511_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, NCT7511_TEMP_REG);
            break;
            case LEFT_BOT_SB_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LEFT_BOT_SB_TEMP_REG);
            break;
            case CTR_TOP_SB_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, CTR_TOP_SB_TEMP_REG);
            break;
            case CTR_SB_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, CTR_SB_TEMP_REG);
            break;
            case LEFT_TOP_CB_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LEFT_TOP_CB_TEMP_REG);
            break;
            case CTR_CB_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, CTR_CB_TEMP_REG);
            break;
            case RIGHT_BOT_CB_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, RIGHT_BOT_CB_TEMP_REG);
            break;
            case LEFT_BOT_CB_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, LEFT_BOT_CB_TEMP_REG);
            break;
            case IO_BOARD_TEMP:
                status = i2c_smbus_read_byte_data(Cameo_BMC_14_client, IO_BOARD_TEMP_REG);
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
    sprintf(buf, "");
    /*TBD*/
    return sprintf(buf, "%s\n", buf);
}

ssize_t themal_temp_min_get(struct device *dev, struct device_attribute *da, char *buf)
{
    sprintf(buf, "");
    /*TBD*/
    return sprintf(buf, "%s\n", buf);
}

ssize_t themal_temp_crit_get(struct device *dev, struct device_attribute *da, char *buf)
{
    sprintf(buf, "");
    /*TBD*/
    return sprintf(buf, "%s\n", buf);
}

ssize_t themal_temp_lcrit_get(struct device *dev, struct device_attribute *da, char *buf)
{
    sprintf(buf, "");
    /*TBD*/
    return sprintf(buf, "%s\n", buf);
}
/* end of implement i2c_function */