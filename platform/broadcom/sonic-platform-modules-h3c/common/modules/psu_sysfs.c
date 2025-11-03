/*
* Copyright (c) 2019  <sonic@h3c.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/*公有文件引入*/
#include <asm/io.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/random.h>
/*私有文件*/
#include "pub.h"
#include "bsp_base.h"
#include "psu.h"

module_param_named(psu_debug_level, bsp_module_debug_level[BSP_PSU_MODULE], int, 0644);
MODULE_PARM_DESC(psu_debug_level, "DEBUG 0x4, ERROR 0x2, INFO 0x1, ALL 0x7, DEBUG_OFF 0x0; Default value is ERROR");

enum psu_hwmon_sysfs_attributes
{
    IN1_MAX = 0,
    IN_MAX_BUTT = IN1_MAX + MAX_PSU_NUM - 1,
    IN1_MIN,
    IN_MIN_BUTT = IN1_MIN + MAX_PSU_NUM - 1,
    IN1_INPUT,
    IN_INPUT_BUTT = IN1_INPUT + MAX_PSU_NUM - 1,
    IN1_LABEL,
    IN_LABEL_BUTT = IN1_LABEL + MAX_PSU_NUM - 1,
    CURR1_MAX,
    CURR_MAX_BUTT = CURR1_MAX + MAX_PSU_NUM - 1,
    CURR1_INPUT,
    CURR_INPUT_BUTT = CURR1_INPUT + MAX_PSU_NUM - 1,
    CURR1_LABEL,
    CURR_LABEL_BUTT = CURR1_LABEL + MAX_PSU_NUM - 1,
    POWER1_MAX,
    POWER_MAX_BUTT = POWER1_MAX + MAX_PSU_NUM - 1,
    POWER1_INPUT,
    POWER_INPUT_BUTT = POWER1_INPUT + MAX_PSU_NUM - 1,
    POWER1_LABEL,
    POWER_LABEL_BUTT = POWER1_LABEL + MAX_PSU_NUM - 1,
    PSU_ATTRIBUTE_BUTT
};

enum psu_customer_sysfs_attributes
{
    NUM_PSUS,
    MODEL_NAME,
    VENDOR,
    MFR_ID,
    SN,
    PN,
    HW_VERSION,
    IN_VOL_TYPE,
    FW_VERSION,
    IN_CURR,
    IN_VOL,
    OUT_CURR,
    OUT_VOL,
    TEMP_INPUT,
    TEMPALIAS,
    TEMPTYPE,
    TEMPMAX,
    TEMPMAX_HYST,
    TEMPMIN,
    STATUS_WORD,
    STATUS,
    SPEED,
    LED_STATUS,
    IN_POWER,
    OUT_POWER,
    DIS_PSU_MON,
    RAW_DATA,
    DATE,
    ALARM,
    ALARM_THRESHOLD_CURR,
    ALARM_THRESHOLD_VOL,
    MAX_OUTPUT_POWER,
    NUM_TEMP_SENSORS,
    PSU_LOG,
    BBOX_CONTENT,
    BBOX_STATUS
};

static psu_info_st psu_info[MAX_PSU_NUM] = {{0}};
static struct sensor_device_attribute psu_hwmon_dev_attr[PSU_ATTRIBUTE_BUTT];       //device attribute
static struct attribute *psu_hwmon_attributes[MAX_PSU_NUM][PSU_ATTRIBUTE_BUTT / MAX_PSU_NUM + 1] = {{0}};    //attribute array, MAX_PSU_NUM 必须大于1
static struct attribute_group psu_hwmon_attribute_group[MAX_PSU_NUM] = {{0}};                                //attribute group
static const struct attribute_group *psu_hwmon_attribute_groups[MAX_PSU_NUM][2] = {{0}};
static char psu_hwmon_attr_name[PSU_ATTRIBUTE_BUTT][ATTR_NAME_MAX_LEN] = {{0}};                             //attribute name string
static char psu_hwmon_name_str[MAX_PSU_NUM][MAX_HWMON_NAME_LEN] = {{0}};

static struct kobject *kobj_psu_root = NULL;
static struct kobject *kobj_psu_sub[MAX_PSU_NUM] = {NULL};
static struct kobject *kobj_temp_sub[MAX_PSU_NUM] = {NULL};

//找到dev对应的psu index
int bsp_sysfs_psu_get_index_from_kobj(struct kobject *kobj)
{
    int i;
    int psu_num = bsp_get_board_data()->psu_num;
    for (i = 0; i < psu_num; i++)
    {
        if (psu_info[i].customer_kobj == kobj)
            return i;
    }
    DBG_ECHO(DEBUG_ERR, "matched psu index for kobj=%p not found", kobj);
    return -1;
}

static ssize_t bsp_sysfs_psu_hwmon_get_attr(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int psu_index = 0;
    u32 tempval = 0;
    int int_value = 0;
    int float_value = 0;
    int ret = ERROR_SUCCESS;

    if ((attr->index >= IN1_LABEL) && (attr->index <= IN_LABEL_BUTT))
    {
        return scnprintf(buf, PAGE_SIZE, "Voltage %d\n", attr->index - IN1_LABEL + 1);
    }
    else if ((attr->index >= CURR1_LABEL) && (attr->index <= CURR_LABEL_BUTT))
    {
        return scnprintf(buf, PAGE_SIZE, "Current %d\n", attr->index - CURR1_LABEL + 1);
    }
    else if ((attr->index >= POWER1_LABEL) && (attr->index <= POWER_LABEL_BUTT))
    {
        return scnprintf(buf, PAGE_SIZE, "Power %d\n", attr->index - CURR1_LABEL + 1);
    }
    else if ((attr->index >= IN1_INPUT) && (attr->index <= IN_INPUT_BUTT))
    {
        psu_index = attr->index - IN1_INPUT;
        ret = bsp_psu_get_in_vol(psu_index, &tempval);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "psu %d get voltage out failed!", psu_index + 1);
            return ret;
        }
        parameter_int_to_float_deal(tempval, &int_value, &float_value);
        return scnprintf(buf, PAGE_SIZE, "%d.%d\n", int_value, float_value);
    }
    else if ((attr->index >= CURR1_INPUT) && (attr->index <= CURR_INPUT_BUTT))
    {
        psu_index = attr->index - CURR1_INPUT;
        ret = bsp_psu_get_in_curr(psu_index, &tempval);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "psu %d get current out failed!", psu_index + 1);
            return ret;
        }

        parameter_int_to_float_deal(tempval, &int_value, &float_value);
        return scnprintf(buf, PAGE_SIZE, "%d.%d\n", int_value, float_value);
    }
    else if ((attr->index >= POWER1_INPUT) && (attr->index <= POWER_INPUT_BUTT))
    {
        psu_index = attr->index - POWER1_INPUT;
        ret = bsp_psu_get_power_in(psu_index, &tempval);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "psu %d get power out failed!", psu_index + 1);
            return ret;
        }

        parameter_int_to_float_deal(tempval, &int_value, &float_value);
        return scnprintf(buf, PAGE_SIZE, "%d.%d\n", int_value, float_value);
    }
    else
    {
        return sprintf(buf, "0\n");
    }

    return 0;

}

static ssize_t bsp_sysfs_psu_custom_set_attr(struct device *kobject, struct device_attribute *da, const char *buf, size_t count)
{
    int psu_index = 0;
    int temp_value = 0;
    int ret = ERROR_SUCCESS;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_sysfs_psu_custom_set_attr get bd failed");
        return -EINVAL;
    }

    if (attr->index != NUM_PSUS)
    {
        psu_index = bsp_sysfs_psu_get_index_from_kobj((struct kobject *)kobject);
    }

    if (-1 == psu_index)
    {
        DBG_ECHO(DEBUG_ERR, "Get psu index failed %d\n", attr->index);
        return -EINVAL;
    }

    switch (attr->index)
    {
        case RAW_DATA:
            if (count > (bd->eeprom_used_size))
            {
                DBG_ECHO(DEBUG_ERR, "%d bytes larger than psu eeprom used size %d, abort!\n", (int)count, (int)bd->eeprom_used_size);
                return -ENOMEM;
            }

            if (false == bsp_psu_set_data(psu_index, buf, count))
            {
                return -EIO;
            }
            break;
        case BBOX_CONTENT:
        {
            sscanf(buf, "%d", &temp_value);
            if (1 == temp_value)
            {
                /* 清除黑盒数据 */
                ret = drv_powerfan_ClearPowerBlackbox(psu_index);
                if (ret != ERROR_SUCCESS)
                {
                     DBG_ECHO(DEBUG_ERR, "drv_powerfan_ClearPowerBlackbox failed.ret=%d",ret);
                     count = ret;
                }
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "Not support param for BBOX_CONTENT\n");
                count = -EINVAL;
            }
            break;
        }
        case BBOX_STATUS:
        {
            sscanf(buf, "%d", &temp_value);
            if ((1 == temp_value) ||(0 == temp_value))
            {
                /* 1 打开黑盒 ;0 关闭*/
                ret = bsp_psu_set_bbox_status(psu_index,(u8)temp_value);
                if (ret != ERROR_SUCCESS)
                {
                     DBG_ECHO(DEBUG_ERR, "bsp_psu_set_bbox_status failed.ret=%d",ret);
                     count = ret;
                }
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "Not support param for BBOX_STATUS \n");
                count = -EINVAL;
            }
            break;
        }
        default:
            DBG_ECHO(DEBUG_ERR, "Not support\n");
            count = -ENOSYS;
            break;
    }

    return count;
}
static ssize_t bsp_sysfs_psu_customer_get_attr(struct device *kobj, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int index = 0;
    int psu_index = -1;
    int int_value = 0;
    int float_value = 0;
    int ret = ERROR_SUCCESS;
    unsigned int used_buf = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_sysfs_psu_customer_get_attr get bd failed");
        return -EINVAL;
    }

    if (attr->index != NUM_PSUS)
    {
        psu_index = bsp_sysfs_psu_get_index_from_kobj((struct kobject *)kobj);
    }
    else
    {
        psu_index = 0;
    }

    if (-1 == psu_index)
    {
        index = scnprintf(buf, PAGE_SIZE, "Get psu index failed %d\n", attr->index);
        return index;
    }

    switch (attr->index)
    {
        case BBOX_CONTENT:
        {
            index = (int)drv_powerfan_DisplayPowerBlackbox(psu_index, 102400, &used_buf, buf);
            break;
        }
        case BBOX_STATUS:
        {
            ret = bsp_psu_get_bbox_status(psu_index,&int_value);
            if(ret != ERROR_SUCCESS){
                DBG_ECHO(DEBUG_ERR, "bsp_psu_get_bbox_status failed! ret = %d",ret);
                index = ret;
                break;
            }

            if(int_value != 0)
                int_value = 1;
            index = scnprintf(buf,PAGE_SIZE, "%d\n", int_value);
            break;
        }
        case NUM_PSUS:
            index = scnprintf(buf, PAGE_SIZE, "%u\n", bsp_psu_get_number());
            break;
        case MODEL_NAME:
            index = bsp_psu_get_model_name(psu_index, buf);
            break;
        case VENDOR:
            index = bsp_psu_get_vendor(psu_index, buf);
            break;
        case SN:
            index = bsp_psu_get_sn(psu_index, buf);
            break;
        case PN:
            index = bsp_psu_get_pn(psu_index, buf);
            break;
        case DATE:
            index = bsp_psu_get_date(psu_index, buf);
            break;
        case ALARM:
            if (bsp_psu_get_alarm(psu_index, &(psu_info[psu_index].alarm)))
            {
                index = scnprintf(buf, PAGE_SIZE, "%d\n", psu_info[psu_index].alarm);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "psu %d get ALARM failed!", psu_index + 1);
                index = -EIO;
            }
            break;
        case ALARM_THRESHOLD_CURR:
            bsp_psu_get_alarm_threshold_curr(psu_index, &(psu_info[psu_index].alarm_threshold_curr));
            index = scnprintf(buf, PAGE_SIZE, "%d.000\n", psu_info[psu_index].alarm_threshold_curr);    //A
            break;
        case ALARM_THRESHOLD_VOL:
            bsp_psu_get_alarm_threshold_vol(psu_index, &(psu_info[psu_index].alarm_threshold_vol));
            index = scnprintf(buf, PAGE_SIZE, "%d.000\n", psu_info[psu_index].alarm_threshold_vol);    //A
            break;
        case MAX_OUTPUT_POWER:
            bsp_psu_get_max_output_power(psu_index, &(psu_info[psu_index].output_power));
            index = scnprintf(buf, PAGE_SIZE, "%d.000\n", psu_info[psu_index].output_power);    //W
            break;
        case NUM_TEMP_SENSORS:
            index = scnprintf(buf, PAGE_SIZE, "%d\n", bd->psu_num_temp_sensors);
            break;
        case MFR_ID:
            index = bsp_psu_get_mfr_id(psu_index, buf);
            break;
        case HW_VERSION:
            index = bsp_psu_get_hw_version(psu_index, buf);
            break;
        case FW_VERSION:
            index = bsp_psu_get_fw_version(psu_index, buf);
            break;
        case IN_CURR:
            ret = bsp_psu_get_in_curr(psu_index, &(psu_info[psu_index].iin));
            if (ERROR_SUCCESS == ret)
            {
                parameter_int_to_float_deal(psu_info[psu_index].iin, &int_value, &float_value);
                index = scnprintf(buf, PAGE_SIZE, "%d.%03d\n", int_value, float_value);
            }
            else
            {
                index = ret;    //返回错误码
            }
            break;
        case IN_VOL:
            ret = bsp_psu_get_in_vol(psu_index, &(psu_info[psu_index].vin));
            if (ERROR_SUCCESS == ret)
            {
                parameter_int_to_float_deal(psu_info[psu_index].vin, &int_value, &float_value);
                index = scnprintf(buf, PAGE_SIZE, "%d.%03d\n", int_value, float_value);
            }
            else
            {
                index = ret;    //返回错误码
            }
            break;
        case IN_VOL_TYPE:
            index = bsp_psu_get_in_vol_type(psu_index, buf);
            break;
        case OUT_CURR:
            ret = bsp_psu_get_out_curr(psu_index, &(psu_info[psu_index].iout));
            if (ERROR_SUCCESS == ret)
            {
                parameter_int_to_float_deal(psu_info[psu_index].iout, &int_value, &float_value);
                index = scnprintf(buf, PAGE_SIZE, "%d.%03d\n", int_value, float_value);
            }
            else
            {
                index = ret;    //返回错误码
            }
            break;
        case OUT_VOL:
            ret = bsp_psu_get_out_vol(psu_index, &(psu_info[psu_index].vout));
            if (ERROR_SUCCESS == ret)
            {
                parameter_int_to_float_deal(psu_info[psu_index].vout, &int_value, &float_value);
                index = scnprintf(buf, PAGE_SIZE, "%d.%03d\n", int_value, float_value);
            }
            else
            {
                index = ret;    //返回错误码
            }
            break;
        case STATUS_WORD:
            ret = bsp_psu_get_status_word(psu_index, &(psu_info[psu_index].status_word));
            if (ret == ERROR_SUCCESS)
            {
                index = scnprintf(buf, PAGE_SIZE, "%d\n", psu_info[psu_index].status_word);
            }
            else
            {
                DBG_ECHO(DEBUG_ERR, "psu %d get STATUS_WORD failed!", psu_index + 1);
                index = ret;
            }
            break;
        case STATUS:
            psu_info[psu_index].status = bsp_psu_get_status(psu_index);
            index = scnprintf(buf, PAGE_SIZE, "%d\n", psu_info[psu_index].status);
            break;
        case SPEED:
            if (bsp_psu_get_fan_speed(psu_index, &(psu_info[psu_index].fan_speed)) != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "psu %d get fan speed failed!", psu_index + 1);
                psu_info[psu_index].fan_speed = 0;
            }
            parameter_int_to_float_deal(psu_info[psu_index].fan_speed, &int_value, &float_value);
            index = scnprintf(buf, PAGE_SIZE, "%d\n", int_value);
            break;
        case LED_STATUS:
            psu_info[psu_index].status = bsp_psu_get_status(psu_index);
            if (psu_info[psu_index].status == PSU_STATUS_ABSENT)           //(absent)
            {
                index = scnprintf(buf, PAGE_SIZE, "%d\n", LED_COLOR_DARK);
            }
            else if (psu_info[psu_index].status == PSU_STATUS_NOT_OK)     //(fault)
            {
                index = scnprintf(buf, PAGE_SIZE, "%d\n", LED_COLOR_YELLOW);
            }
            else if (psu_info[psu_index].status == PSU_STATUS_OK)    //(normal)
            {
                index = scnprintf(buf, PAGE_SIZE, "%d\n", LED_COLOR_GREEN);
            }
            break;
        case IN_POWER:
            if (bsp_psu_get_power_in(psu_index, &(psu_info[psu_index].pin)) != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "psu %d get power in failed!", psu_index + 1);
                psu_info[psu_index].pin = 0;
            }
            parameter_int_to_float_deal(psu_info[psu_index].pin, &int_value, &float_value);
            index = scnprintf(buf, PAGE_SIZE, "%d.%03d\n", int_value, float_value);
            break;
        case OUT_POWER:
            if (bsp_psu_get_power_out(psu_index, &(psu_info[psu_index].pout)) != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "psu %d get power out failed!", psu_index + 1);
                psu_info[psu_index].pout = 0;
            }
            parameter_int_to_float_deal(psu_info[psu_index].pout, &int_value, &float_value);
            index = scnprintf(buf, PAGE_SIZE, "%d.%03d\n", int_value, float_value);
            break;
        case RAW_DATA:
            index = bsp_psu_get_data(psu_index, buf);
            break;
        case PSU_LOG:
            DBG_ECHO_PSU(DEBUG_ERR, "=================================== \n");

            if (bsp_psu_get_status_word(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power STATUS_WORD: 0x%x \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get STATUS_WORD failed!", psu_index + 1);
            }

            if (bsp_psu_get_input_status(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power STATUS_INPUT: 0x%x \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get PSU_STATUS_INPUT_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_vout_status(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power STATUS_VOUT: 0x%x \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get PSU_STATUS_VOUT_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_status_temperature(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power STATUS_TEMP: 0x%x \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get STATUS_TEMPERATURE_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_fan_status(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power STATUS_FAN: 0x%x \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get PSU_STATUS_FAN_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_alarm(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power INNER_TEMP: %d \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get PSU_ALARM_TEMP_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_fan_speed(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power READ_FAN_SPEED: %d \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get FAN_SPEED_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_out_vol(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power READ_VOUT: %d \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get VOLTAGE_OUT_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_in_vol(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power READ_VIN: %d \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get VOLTAGE_IN_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_out_curr(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power READ_IOUT: %d \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get CURRENT_OUT_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_in_curr(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power READ_IIN: %d \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get CURRENT_IN_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_power_out(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power READ_POUT: %d \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get POWER_OUT_CMD failed!", psu_index + 1);
            }

            if (bsp_psu_get_power_in(psu_index, &int_value) == ERROR_SUCCESS)
            {
                DBG_ECHO_PSU(DEBUG_ERR, "Power READ_PIN: %d \n", int_value);
            }
            else
            {
                DBG_ECHO_PSU(DEBUG_ERR, "psu %d get POWER_IN_CMD failed!", psu_index + 1);
            }

            DBG_ECHO_PSU(DEBUG_ERR, "=================================== \n");

            index = scnprintf(buf, PAGE_SIZE, "the log of Power %u state is created, path:%s or %s\n", psu_index + 1, LOG_FILE_PATH_2, LOG_FILE_PATH_3);

            break;
        default:
            index = -ENOSYS;
            break;
    }
    return index;
}

static ssize_t bsp_sysfs_psu_temp_sensor_get_attr(struct device *dev, struct device_attribute *da, char *buf)
{
    ssize_t index = 0;
    int psu_index = -1;
    int int_value = 0;
    int float_value = 0;
    int ret = ERROR_SUCCESS;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    psu_index = bsp_sysfs_psu_get_index_from_kobj(((struct kobject *)dev)->parent);
    if (-1 == psu_index)
    {
        DBG_ECHO(DEBUG_ERR, "Get psu index failed %d\n", attr->index);
        return -EINVAL;
    }

    switch (attr->index)
    {
        case TEMP_INPUT:
            ret = bsp_psu_get_temp_input(psu_index, &(psu_info[psu_index].tempinput));
            if (ret != ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "psu %d get current temperature failed!", psu_index + 1);
                index = ret;
                break;
            }
            parameter_int_to_float_deal(psu_info[psu_index].tempinput, &int_value, &float_value);
            index = scnprintf(buf, PAGE_SIZE, "%d.%d\n", int_value, float_value);
            break;
        case TEMPALIAS:
            index = scnprintf(buf, PAGE_SIZE, "psu_inner\n");
            break;
        case TEMPTYPE:
            index = scnprintf(buf, PAGE_SIZE, "psu sensor%d\n", attr->index);
            break;
        case TEMPMAX:
            index = scnprintf(buf, PAGE_SIZE, "%d\n", 80);
            break;
        case TEMPMAX_HYST:
            index = scnprintf(buf, PAGE_SIZE, "%d\n", 5);
            break;
        case TEMPMIN:
            index = scnprintf(buf, PAGE_SIZE, "%d\n", 0);
            break;
        default:
            index = -ENOSYS;
            break;
    }
    return index;
}
//customer private node
static SENSOR_DEVICE_ATTR(num, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, NUM_PSUS);
static SENSOR_DEVICE_ATTR(model_name, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, MODEL_NAME);
static SENSOR_DEVICE_ATTR(vendor, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, VENDOR);
static SENSOR_DEVICE_ATTR(serial_number, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, SN);
static SENSOR_DEVICE_ATTR(part_number, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, PN);
static SENSOR_DEVICE_ATTR(vendor_name_id, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, MFR_ID);
static SENSOR_DEVICE_ATTR(hardware_version, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, HW_VERSION);
static SENSOR_DEVICE_ATTR(fw_version, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, FW_VERSION);
static SENSOR_DEVICE_ATTR(in_curr, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, IN_CURR);
static SENSOR_DEVICE_ATTR(in_vol, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, IN_VOL);
static SENSOR_DEVICE_ATTR(out_curr, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, OUT_CURR);
static SENSOR_DEVICE_ATTR(out_vol, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, OUT_VOL);

static SENSOR_DEVICE_ATTR(temp_input, S_IRUGO, bsp_sysfs_psu_temp_sensor_get_attr, NULL, TEMP_INPUT);
static SENSOR_DEVICE_ATTR(temp_alias, S_IRUGO, bsp_sysfs_psu_temp_sensor_get_attr, NULL, TEMPALIAS);
static SENSOR_DEVICE_ATTR(temp_type, S_IRUGO, bsp_sysfs_psu_temp_sensor_get_attr, NULL, TEMPTYPE);
static SENSOR_DEVICE_ATTR(temp_max, S_IRUGO, bsp_sysfs_psu_temp_sensor_get_attr, NULL, TEMPMAX);
static SENSOR_DEVICE_ATTR(temp_max_hyst, S_IRUGO, bsp_sysfs_psu_temp_sensor_get_attr, NULL, TEMPMAX_HYST);
static SENSOR_DEVICE_ATTR(temp_min, S_IRUGO, bsp_sysfs_psu_temp_sensor_get_attr, NULL, TEMPMIN);

static SENSOR_DEVICE_ATTR(status_word, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, STATUS_WORD);
static SENSOR_DEVICE_ATTR(status, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, STATUS);
static SENSOR_DEVICE_ATTR(fan_speed, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, SPEED);
static SENSOR_DEVICE_ATTR(led_status, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, LED_STATUS);
static SENSOR_DEVICE_ATTR(in_power, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, IN_POWER);
static SENSOR_DEVICE_ATTR(out_power, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, OUT_POWER);
static SENSOR_DEVICE_ATTR(type, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, IN_VOL_TYPE);
static SENSOR_DEVICE_ATTR(date, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, DATE);
static SENSOR_DEVICE_ATTR(alarm, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, ALARM);
static SENSOR_DEVICE_ATTR(alarm_threshold_curr, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, ALARM_THRESHOLD_CURR);
static SENSOR_DEVICE_ATTR(alarm_threshold_vol, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, ALARM_THRESHOLD_VOL);
static SENSOR_DEVICE_ATTR(max_output_power, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, MAX_OUTPUT_POWER);
static SENSOR_DEVICE_ATTR(num_temp_sensors, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, NUM_TEMP_SENSORS);
static SENSOR_DEVICE_ATTR(psu_log, S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, PSU_LOG);
static SENSOR_DEVICE_ATTR(bbox_content, S_IRUGO|S_IWUSR, bsp_sysfs_psu_customer_get_attr, bsp_sysfs_psu_custom_set_attr, BBOX_CONTENT);
static SENSOR_DEVICE_ATTR(bbox_status, S_IRUGO|S_IWUSR, bsp_sysfs_psu_customer_get_attr, bsp_sysfs_psu_custom_set_attr, BBOX_STATUS);

#if (FT_TEST == 1)
static SENSOR_DEVICE_ATTR(raw_data,          S_IRUGO | S_IWUSR, bsp_sysfs_psu_customer_get_attr, bsp_sysfs_psu_custom_set_attr, RAW_DATA);
#else
static SENSOR_DEVICE_ATTR(raw_data,          S_IRUGO, bsp_sysfs_psu_customer_get_attr, NULL, RAW_DATA);
#endif

BSPMODULE_DEBUG_ATTR_DEF(debug, BSP_PSU_MODULE);
BSPMODULE_DEBUG_RW_ATTR_DEF(loglevel, BSP_PSU_MODULE);

//psu根结点
static struct attribute *psu_customer_device_attributes_num_psu[] =
{
    &sensor_dev_attr_num.dev_attr.attr,
    &bspmodule_debug.attr,
    &bspmodule_loglevel.attr,
    NULL
};

static const struct attribute_group psu_customer_group_num_psu =
{
    .attrs = psu_customer_device_attributes_num_psu,
};

//添加psu1和psu2子结点
static struct attribute *psu_customer_device_attributes[] =
{
    &sensor_dev_attr_model_name.dev_attr.attr,
    &sensor_dev_attr_serial_number.dev_attr.attr,
    &sensor_dev_attr_part_number.dev_attr.attr,
    &sensor_dev_attr_vendor_name_id.dev_attr.attr,
    &sensor_dev_attr_hardware_version.dev_attr.attr,
    &sensor_dev_attr_fw_version.dev_attr.attr,
    &sensor_dev_attr_in_curr.dev_attr.attr,
    &sensor_dev_attr_in_vol.dev_attr.attr,
    &sensor_dev_attr_type.dev_attr.attr,
    &sensor_dev_attr_out_curr.dev_attr.attr,
    &sensor_dev_attr_out_vol.dev_attr.attr,
    &sensor_dev_attr_status.dev_attr.attr,
    &sensor_dev_attr_led_status.dev_attr.attr,
    &sensor_dev_attr_in_power.dev_attr.attr,
    &sensor_dev_attr_out_power.dev_attr.attr,
    &sensor_dev_attr_vendor.dev_attr.attr,
    &sensor_dev_attr_status_word.dev_attr.attr,
    &sensor_dev_attr_raw_data.dev_attr.attr,
    &sensor_dev_attr_date.dev_attr.attr,
    &sensor_dev_attr_alarm.dev_attr.attr,
    &sensor_dev_attr_alarm_threshold_curr.dev_attr.attr,
    &sensor_dev_attr_alarm_threshold_vol.dev_attr.attr,
    &sensor_dev_attr_max_output_power.dev_attr.attr,
    &sensor_dev_attr_num_temp_sensors.dev_attr.attr,
    &sensor_dev_attr_psu_log.dev_attr.attr,
    &sensor_dev_attr_fan_speed.dev_attr.attr,
    &sensor_dev_attr_bbox_content.dev_attr.attr,
    &sensor_dev_attr_bbox_status.dev_attr.attr,
    NULL
};

static const struct attribute_group psu_customer_group =
{
    .attrs = psu_customer_device_attributes,
};

//temp目录下的结点（三级结点）
static struct attribute *psu_num_temp_sensors_attributes[] =
{

    &sensor_dev_attr_temp_input.dev_attr.attr,
    &sensor_dev_attr_temp_alias.dev_attr.attr,
    &sensor_dev_attr_temp_type.dev_attr.attr,
    &sensor_dev_attr_temp_max.dev_attr.attr,
    &sensor_dev_attr_temp_max_hyst.dev_attr.attr,
    &sensor_dev_attr_temp_min.dev_attr.attr,
    NULL
};

static const struct attribute_group psu_num_temp_sensor_group =
{
    .attrs = psu_num_temp_sensors_attributes,
};

#define PSU_HWMON_SENSOR_DEV_ATTR(__attr_name_str,__attr_index,__mode,__show,__store)  \
    psu_hwmon_dev_attr[__attr_index].dev_attr.attr.name = (__attr_name_str); \
    psu_hwmon_dev_attr[__attr_index].dev_attr.attr.mode = (__mode); \
    psu_hwmon_dev_attr[__attr_index].dev_attr.show = (__show); \
    psu_hwmon_dev_attr[__attr_index].dev_attr.store = (__store); \
    psu_hwmon_dev_attr[__attr_index].index = (__attr_index);

//设置出口函数
void psu_sysfs_exit(void)
{
    int i = 0;
    int j = 0;
    int index = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: psu_sysfs_exit get bd failed");
        return ;
    }

    for (i = 0; i < bd->psu_num; i++)
    {
        if (psu_info[i].parent_hwmon != NULL)
        {
            hwmon_device_unregister(psu_info[i].parent_hwmon);
            psu_info[i].parent_hwmon = NULL;
        }

        for (j = 0; j < 1; j++)
        {
            if (kobj_temp_sub[index] != NULL)
            {
                sysfs_remove_group(kobj_temp_sub[index], &psu_num_temp_sensor_group);
                kobject_put(kobj_temp_sub[index]);
                index++;
            }
        }
        if (kobj_psu_sub[i] != NULL)
        {
            sysfs_remove_group(kobj_psu_sub[i], &psu_customer_group);
            kobject_put(kobj_psu_sub[i]);
        }
    }

    if (kobj_psu_root != NULL)
    {
        sysfs_remove_group(kobj_psu_root, &psu_customer_group_num_psu);
        kobject_put(kobj_psu_root);
    }
    return;
}

int psu_sysfs_init(void)
{
    int ret = ERROR_SUCCESS;
    int psu_index = 0;
    int i = 0;
    int j = 0;
    int index = 0;
    int temp_attr_index = 0;
    int temp_attr_arrary_index = 0;
    char temp_str[128] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: psu_sysfs_init get bd failed");
        return -EINVAL;
    }

    memset(psu_info, 0, sizeof(psu_info));
    memset(psu_hwmon_dev_attr, 0, sizeof(psu_hwmon_dev_attr));
    memset(psu_hwmon_attributes, 0, sizeof(psu_hwmon_attributes));
    memset(psu_hwmon_attribute_group, 0, sizeof(psu_hwmon_attribute_group));
    memset(psu_hwmon_attribute_groups, 0, sizeof(psu_hwmon_attribute_groups));
    memset(kobj_psu_sub, 0, sizeof(kobj_psu_sub));

    //添加hwmon节点
    for (psu_index = 0; psu_index < bd->psu_num; psu_index++)
    {
        temp_attr_arrary_index = 0;

        temp_attr_index = IN1_LABEL + psu_index;
        scnprintf(psu_hwmon_attr_name[temp_attr_index], sizeof(psu_hwmon_attr_name[temp_attr_index]), "in%d_label", 1);
        PSU_HWMON_SENSOR_DEV_ATTR(psu_hwmon_attr_name[temp_attr_index], temp_attr_index, (S_IRUGO), bsp_sysfs_psu_hwmon_get_attr, NULL);
        psu_hwmon_attributes[psu_index][temp_attr_arrary_index ++] = &(psu_hwmon_dev_attr[temp_attr_index].dev_attr.attr);

        temp_attr_index = IN1_INPUT + psu_index;
        scnprintf(psu_hwmon_attr_name[temp_attr_index], sizeof(psu_hwmon_attr_name[temp_attr_index]), "in%d_input", 1);
        PSU_HWMON_SENSOR_DEV_ATTR(psu_hwmon_attr_name[temp_attr_index], temp_attr_index, (S_IRUGO), bsp_sysfs_psu_hwmon_get_attr, NULL);
        psu_hwmon_attributes[psu_index][temp_attr_arrary_index ++] = &(psu_hwmon_dev_attr[temp_attr_index].dev_attr.attr);

        temp_attr_index = CURR1_LABEL + psu_index;
        scnprintf(psu_hwmon_attr_name[temp_attr_index], sizeof(psu_hwmon_attr_name[temp_attr_index]), "curr%d_label", 1);
        PSU_HWMON_SENSOR_DEV_ATTR(psu_hwmon_attr_name[temp_attr_index], temp_attr_index, (S_IRUGO), bsp_sysfs_psu_hwmon_get_attr, NULL);
        psu_hwmon_attributes[psu_index][temp_attr_arrary_index ++] = &(psu_hwmon_dev_attr[temp_attr_index].dev_attr.attr);

        temp_attr_index = CURR1_INPUT + psu_index;
        scnprintf(psu_hwmon_attr_name[temp_attr_index], sizeof(psu_hwmon_attr_name[temp_attr_index]), "curr%d_input", 1);
        PSU_HWMON_SENSOR_DEV_ATTR(psu_hwmon_attr_name[temp_attr_index], temp_attr_index, (S_IRUGO), bsp_sysfs_psu_hwmon_get_attr, NULL);
        psu_hwmon_attributes[psu_index][temp_attr_arrary_index ++] = &(psu_hwmon_dev_attr[temp_attr_index].dev_attr.attr);

        temp_attr_index = POWER1_LABEL + psu_index;
        scnprintf(psu_hwmon_attr_name[temp_attr_index], sizeof(psu_hwmon_attr_name[temp_attr_index]), "power%d_label", 1);
        PSU_HWMON_SENSOR_DEV_ATTR(psu_hwmon_attr_name[temp_attr_index], temp_attr_index, (S_IRUGO), bsp_sysfs_psu_hwmon_get_attr, NULL);
        psu_hwmon_attributes[psu_index][temp_attr_arrary_index ++] = &(psu_hwmon_dev_attr[temp_attr_index].dev_attr.attr);

        temp_attr_index = POWER1_INPUT + psu_index;
        scnprintf(psu_hwmon_attr_name[temp_attr_index], sizeof(psu_hwmon_attr_name[temp_attr_index]), "power%d_input", 1);
        PSU_HWMON_SENSOR_DEV_ATTR(psu_hwmon_attr_name[temp_attr_index], temp_attr_index, (S_IRUGO), bsp_sysfs_psu_hwmon_get_attr, NULL);
        psu_hwmon_attributes[psu_index][temp_attr_arrary_index ++] = &(psu_hwmon_dev_attr[temp_attr_index].dev_attr.attr);

        psu_hwmon_attributes[psu_index][temp_attr_arrary_index] = NULL;    //属性组最后一个属性为NULL
        psu_hwmon_attribute_group[psu_index].attrs = psu_hwmon_attributes[psu_index];
        psu_hwmon_attribute_groups[psu_index][0] = &(psu_hwmon_attribute_group[psu_index]);
        psu_hwmon_attribute_groups[psu_index][1] = NULL;

        scnprintf(psu_hwmon_name_str[psu_index], sizeof(psu_hwmon_name_str[psu_index]), "Power%d", psu_index + 1);

        //添加hwmon节点
        psu_info[psu_index].status = PSU_STATUS_ABSENT;
        psu_info[psu_index].parent_hwmon = hwmon_device_register_with_groups(NULL, psu_hwmon_name_str[psu_index], NULL, psu_hwmon_attribute_groups[psu_index]);
        if (IS_ERR(psu_info[psu_index].parent_hwmon))
        {
            psu_info[psu_index].parent_hwmon = NULL;
            DBG_ECHO(DEBUG_ERR, "psu %d hwmon register failed!\n", psu_index + 1);
            ret = -EACCES;
            goto exit;
        }
    }

    //添加订制节点
    kobj_psu_root = kobject_create_and_add("psu", kobj_switch);
    if (kobj_psu_root == NULL)
    {
        DBG_ECHO(DEBUG_INFO, "create psu custom root node failed!");
        ret =  -EACCES;
        goto exit;
    }

    //添加根结点
    ret = sysfs_create_group(kobj_psu_root, &psu_customer_group_num_psu);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create num group failed");
    //添加psu结点
    for (i = 0; i < bd->psu_num; i ++)
    {
        scnprintf(temp_str, sizeof(temp_str), "psu%d", i + 1);
        kobj_psu_sub[i] = kobject_create_and_add(temp_str, kobj_psu_root);
        if (kobj_psu_sub[i] == NULL)
        {
            DBG_ECHO(DEBUG_INFO, "create sub psu node psu%d failed!", i + 1);
            ret =  -EACCES;
            goto exit;
        }
        ret = sysfs_create_group(kobj_psu_sub[i], &psu_customer_group);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "create psu custome group failed!");
        psu_info[i].customer_kobj = kobj_psu_sub[i];
        DBG_ECHO(DEBUG_DBG, "psu %d kobj=%p", i, kobj_psu_sub[i]);

        //创建temp目录，以及结点
        for (j = 0; j < bd->psu_num_temp_sensors; j ++)
        {
            sprintf(temp_str, "temp%d", j);
            kobj_temp_sub[index] = kobject_create_and_add(temp_str, kobj_psu_sub[i]);
            if (kobj_temp_sub[index] == NULL)
            {
                DBG_ECHO(DEBUG_INFO, "create sub psu node temp%d failed!", j);
                ret =  -EACCES;
                goto exit;
            }
            ret = sysfs_create_group(kobj_temp_sub[index], &psu_num_temp_sensor_group);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "create temp custome group failed!");

            index++;
        }
    }

exit:
    if (ret != ERROR_SUCCESS)
    {
        psu_sysfs_exit();
    }

    return ret;
}

//设置初始化入口函数
static int __init __psu_init(void)
{
    int ret = ERROR_SUCCESS;
    INIT_PRINT("psu module init started");
    ret = psu_sysfs_init();
    if (ERROR_SUCCESS == ret)
    {
        INIT_PRINT("psu module init success!");
    }
    else
    {
        INIT_PRINT("psu module init failed!");
    }
    eeprom_table_init(I2C_DEV_PSU);
    return ret;
}

//设置出口函数
static void __exit __psu_exit(void)
{
    psu_sysfs_exit();
    INIT_PRINT("module uninstalled !\n");
    return;
}

module_init(__psu_init);
module_exit(__psu_exit);

MODULE_AUTHOR("Qianchaoyang <qian.chaoyang@h3c.com>");
MODULE_DESCRIPTION("h3c system eeprom driver");
MODULE_LICENSE("Dual BSD/GPL");
