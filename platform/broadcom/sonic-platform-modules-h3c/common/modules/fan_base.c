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
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
/*私有文件*/
#include "pub.h"
#include "bsp_base.h"
#include "fan.h"

extern fan_info_st fan_info[MAX_FAN_NUM];
char *fan_status_string[] = {"Absent", "OK", "NOT OK", "Unknown"};

/*
 * 获取风扇状态
 * 注意: 入参的正确性由调用者保证!
 */
int bsp_fan_get_status(int fan_index)
{
    u8 absent = 0;
    u8 good = 0;
    int temp_status = FAN_STATUS_ABSENT;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_get_status get bd failed");
        return -EINVAL;
    }

    ret = bsp_cpld_get_fan_absent(&absent, fan_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "fan %d get absent failed!", fan_index + 1);

    temp_status = FAN_STATUS_NOT_OK;
    ret = bsp_cpld_get_fan_status(&good, fan_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "fan %d get status failed!", fan_index + 1);
    temp_status = (absent == CODE_FAN_ABSENT) ? FAN_STATUS_ABSENT :
                  (good == bd->cpld_fan_good_flag) ? FAN_STATUS_OK : FAN_STATUS_NOT_OK;

exit:
    return temp_status;

}
EXPORT_SYMBOL(bsp_fan_get_status);

/*
 *获取风扇状态
 * ！注意索引入参的正确性由调用者保证
 */
int bsp_fan_motor_get_status(int fan_index, int motor_index, int *status)
{
    int ret = ERROR_SUCCESS;
    u16 speed = 0;
    int temp_status = FAN_STATUS_ABSENT;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_motor_get_status get bd failed");
        return -EINVAL;
    }

    if (status == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "param err: status == NULL");
        return -EINVAL;
    }

    temp_status = bsp_fan_get_status(fan_index);
    switch (temp_status)
    {
        case FAN_STATUS_NOT_OK:
            ret = bsp_cpld_get_fan_speed(&speed, fan_index, motor_index);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "fan %d moter %d get speed failed!", fan_index + 1, motor_index + 1);
            *status = (speed < bd->fan_min_speed) ? FAN_STATUS_NOT_OK : FAN_STATUS_OK;
            break;
        case FAN_STATUS_ABSENT:
        case FAN_STATUS_OK:
            *status = temp_status;
            break;
        default:
            *status = FAN_STATUS_NOT_OK;
            break;
    }

exit:
    return ret;
}

char *bsp_fan_get_status_string(int status)
{
    if ((status >= FAN_STATUS_ABSENT) && (status <= FAN_STATUS_NOT_OK))
    {
        return fan_status_string[status];
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "fan unknown status %d", status);
        return fan_status_string[FAN_STATUS_NOT_OK];
    }
}

int bsp_fan_get_status_color(int status)
{
    int color;
    switch (status)
    {
        case FAN_STATUS_OK:
            color = LED_COLOR_GREEN;
            break;
        case FAN_STATUS_NOT_OK:
            color = LED_COLOR_RED;
            break;
        default:
            color = LED_COLOR_DARK;
            break;
    }
    return color;
}

int bsp_fan_get_fan_number(int *fan_number)
{
    *fan_number = bsp_get_board_data()->fan_num;
    return ERROR_SUCCESS;
}

int bsp_fan_get_info_from_eeprom_by_h3c(int fan_index, int info_type, OUT u8 *info_data)
{
    u16 start_offset = 0;
    u16 read_length = 0;
    int attr_index = info_type - 18;
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E enDevAddr = I2C_DEV_FAN + fan_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_get_info_from_eeprom_by_h3c get bd failed");
        return -EINVAL;
    }
    if (fan_index < 0 || fan_index > bd->fan_num)
    {
        DBG_ECHO(DEBUG_ERR, "fan index %d is invalied!", fan_index);
        ret = EINVAL;
        goto exit;
    }

    if (bsp_fan_get_status(fan_index) == FAN_STATUS_ABSENT)
    {
        *info_data = '\0';
        ret = ERROR_SUCCESS;
        goto exit;
    }

    switch (info_type)
    {
        case PRODUCT_NAME:
            start_offset = REG_ADDR_FAN_PDT_NAME;
            read_length = FAN_PDT_NAME_BYTE_COUNT;
            break;
        case SN:
        case PN:
            start_offset = REG_ADDR_FAN_SN;
            read_length = FAN_SN_BYTE_COUNT;
            break;
        case HW_VERSION:
            start_offset = REG_ADDR_FAN_HW_VER;
            read_length = FAN_HW_VER_BYTE_COUNT;
            break;
        case VENDOR:
            start_offset = REG_ADDR_FAN_VENDOR_NAME;
            read_length = FAN_VENDOR_NAME_BYTE_COUNT;
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "not support info type %d", info_type);
            ret = -ENOSYS;
            goto exit;
    }

    ret = bsp_i2c_24LC128_eeprom_read_bytes(bd->i2c_addr_fan[fan_index], start_offset, read_length, info_data, enDevAddr);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "fan %d get raw data from eeprom failed!", fan_index + 1);
    if ((0 != memcmp(info_data, fan_table[fan_index].tlv_attr_table[attr_index].value, read_length)))
    {
        ret = eeprom_table_update(enDevAddr, fan_table + fan_index);
        if (ret == ERROR_SUCCESS)
        {
            memcpy(info_data, fan_table[fan_index].tlv_attr_table[attr_index].value, read_length);
        }
        else if (ret == EXCHANGE_FLAG)
        {
            /*新插入的fan为tlvfan, 将返回值置为1*/
            DBG_ECHO(DEBUG_INFO, "ntlv eeprom change to tlv eeprom")
            goto exit;
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "%d tlv table update failed", fan_index + 1)
            goto exit;
        }
    }
exit:
    return ret;
}

int bsp_fan_get_info_from_eeprom_by_tlv(int fan_index, int info_type, OUT u8 *info_data)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_get_info_from_eeprom_by_tlv get bd failed");
        return -EINVAL;
    }

    if ((fan_index < 0) || (fan_index > bd->fan_num))
    {
        DBG_ECHO(DEBUG_ERR, "param err: fan index %d is invalid!", fan_index);
        ret = -EINVAL;
        goto exit;
    }

    switch (info_type)
    {
        case VENDOR:
        {
            /* the same with td3, vendor is replease with svendor (h3c--delta)  */
            ret = bsp_syseeprom_get_onie_tlv((I2C_DEV_FAN + fan_index), TLV_CODE_SVENDOR, info_data);
            if (ret < ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "fan %d get vendor failed!", fan_index + 1);
            }
            break;
        }
        case PRODUCT_NAME:
            ret = bsp_syseeprom_get_onie_tlv((I2C_DEV_FAN + fan_index), TLV_CODE_PRODUCT_NAME, info_data);
            if (ret < ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "fan %d get product name failed!", fan_index + 1);
            }
            break;
        case SN:
        case PN:
            ret = bsp_syseeprom_get_onie_tlv((I2C_DEV_FAN + fan_index), TLV_CODE_SERIAL_NUMBER, info_data);
            if (ret < ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "fan %d get product SN/PN failed!", fan_index + 1);
            }
            break;
        case HW_VERSION:
            ret = bsp_syseeprom_get_onie_tlv((I2C_DEV_FAN + fan_index), TLV_CODE_HW_VERSION, info_data);
            if (ret < ERROR_SUCCESS)
            {
                DBG_ECHO(DEBUG_ERR, "fan %d get product HW version failed!", fan_index + 1);
            }
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "not support info type %d for fan get info", info_type);
            ret = -EINVAL;
            goto exit;
            break;
    }
exit:
    return ret;
}

int bsp_fan_get_info_from_eeprom(int fan_index, int info_type, OUT u8 *info_data)
{
    int ret = ERROR_SUCCESS;
    u8 absent = 0;
    I2C_DEVICE_E enDevIndex = I2C_DEV_BUTT;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_get_info_from_eeprom get bd failed");
        return -EINVAL;
    }

    if ((fan_index < 0) || (fan_index > bd->fan_num))
    {
        DBG_ECHO(DEBUG_ERR, "param err: fan index %d is invalid!", fan_index);
        ret = -EINVAL;
        goto exit;
    }
    ret = bsp_cpld_get_fan_absent(&absent, fan_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "fan %d get absent failed!", fan_index + 1);
    if (CODE_FAN_ABSENT == absent)
    {
        ret = -ENODEV;
        goto exit;
    }

    enDevIndex = I2C_DEV_FAN + fan_index;
    if (fan_table[fan_index].is_tlv_type)
    {
        ret = bsp_fan_get_info_from_eeprom_by_tlv(fan_index, info_type, info_data);
        /*如果bsp_fan_get_info_from_eeprom_by_tlv的返回值为EXCHANGE_FLAG(1) 表示进行了utlv eeprom update
            接下来使用非tlv的方式读取eeprom, 这个情况发生在将tlv格式的eeprom替换为非tlv格式的风扇*/
        if (ret == EXCHANGE_FLAG)
        {
            ret = bsp_fan_get_info_from_eeprom_by_h3c(fan_index, info_type, info_data);
        }
        else if (ret < 0)
        {
            DBG_ECHO(DEBUG_ERR, "fan %d get eeprom by tlv failed!", fan_index + 1);
        }
    }
    else
    {
        ret = bsp_fan_get_info_from_eeprom_by_h3c(fan_index, info_type, info_data);
        /*如果bsp_fan_get_info_from_eeprom_by_h3c的返回值为EXCHANGE_FLAG(1) 表示进行了tlv eeprom update
        接下来使用tlv的方式读取eeprom, 这个情况发生在将非tlv格式的eeprom替换为tlv格式的风扇*/
        if (ret == EXCHANGE_FLAG)
        {
            ret = bsp_fan_get_info_from_eeprom_by_tlv(fan_index, info_type, info_data);
        }
        else if (ret < 0)
        {
            DBG_ECHO(DEBUG_ERR, "fan %d get eeprom by ntlv failed!", fan_index + 1);
        }
    }

exit:
    return ret;
}

#if 0

ssize_t bsp_fan_sysfs_show(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    return 0;
}

ssize_t  bsp_fan_sysfs_store(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    return count;
}

#endif

//设置板卡eeprom写保护, ENABLE为开启写保护, DISABLE为关闭写保护
int bsp_fan_eeprom_write_protect_set(int index, int enable_disable_flag)
{
    u8  set_value = 0;
    u8 mask = 0;
    u8 shift  = 0;
    board_static_data *bdata = bsp_get_board_data();

    mask = (u8)(0x01 << index);
    shift = (u8)index;
    set_value = enable_disable_flag;
    return bsp_cpld_write_part(set_value, bdata->cpld_addr_fan_eeprom_write_protect, mask, shift);
}

//找到dev对应的fan index
int bsp_sysfs_fan_get_index_from_dev(struct device *dev)
{
    int i;
    int fan_num = bsp_get_board_data()->fan_num;
    for (i = 0; i < fan_num; i++)
    {
        if (fan_info[i].parent_hwmon == dev)
            return i;
    }
    DBG_ECHO(DEBUG_ERR, "matched fan hwmon for dev=%p not found", dev);
    return -1;
}

unsigned int bsp_fan_custom_get_num_fans(void)
{
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_custom_get_num_fans get bd failed");
        return -EINVAL;
    }
    return bd->fan_num;
}

ssize_t bsp_fan_custom_get_vendor(unsigned int fan_index, char *buf)
{
    u8 fan_eeprom_info[FAN_EEPROM_STRING_MAX_LENGTH] = {0};
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;

    ret = bsp_fan_get_info_from_eeprom(fan_index, VENDOR, fan_eeprom_info);
    if (ret == ERROR_SUCCESS)
    {
        len = scnprintf(buf, PAGE_SIZE,  "%s\n", (char *)fan_eeprom_info);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_fan_custom_get_sn(unsigned int fan_index, char *buf)
{
    u8 fan_eeprom_info[FAN_EEPROM_STRING_MAX_LENGTH] = {0};
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;

    ret = bsp_fan_get_info_from_eeprom(fan_index, SN, fan_eeprom_info);
    if (ret == ERROR_SUCCESS)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", (char *)fan_eeprom_info);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_fan_custom_get_pn(unsigned int fan_index, char *buf)
{
    u8 fan_eeprom_info[FAN_EEPROM_STRING_MAX_LENGTH] = {0};
    ssize_t len = 0;
    char pn[PART_NUMBER_LEN + 1] = {0};
    int ret = ERROR_SUCCESS;

    ret = bsp_fan_get_info_from_eeprom(fan_index, PN, fan_eeprom_info);
    if (ret == ERROR_SUCCESS)
    {
        strncpy(pn, fan_eeprom_info + PART_NUMBER_OFFSET_LEN, PART_NUMBER_LEN);
        len = scnprintf(buf, PAGE_SIZE, "%s\n", pn);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_fan_custom_get_product_name(unsigned int fan_index, char *buf)
{
    u8 fan_eeprom_info[FAN_EEPROM_STRING_MAX_LENGTH] = {0};
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;

    ret = bsp_fan_get_info_from_eeprom(fan_index, PRODUCT_NAME, fan_eeprom_info);
    if (ret == ERROR_SUCCESS)
    {
        len = scnprintf(buf, PAGE_SIZE, "%s\n", (char *)fan_eeprom_info);
    }
    else
    {
        len = ret;
    }
    return len;
}

ssize_t bsp_fan_custom_get_hw_version(unsigned int fan_index, char *buf)
{
    u8 fan_eeprom_info[FAN_EEPROM_STRING_MAX_LENGTH] = {0};
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;

    memset(fan_eeprom_info, 0, sizeof(fan_eeprom_info));

    ret = bsp_fan_get_info_from_eeprom(fan_index, HW_VERSION, fan_eeprom_info);
    if (ret == ERROR_SUCCESS)
    {
        len = scnprintf(buf,  PAGE_SIZE, "%s\n", (fan_eeprom_info[0] == 0xff || fan_eeprom_info[0] == 0x0) ? "1.0" : (char *)fan_eeprom_info);
    }
    else
    {
        len = ret;
    }
    return len;
}

int bsp_fan_custom_get_num_motors(char *buf)
{
    ssize_t len = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_custom_get_num_motors get bd failed");
        return -EINVAL;
    }

    len = scnprintf(buf,  PAGE_SIZE, "%d\n", (int)bd->motors_per_fan);

    return len;
}

int bsp_fan_custom_get_status(int fan_index, char *buf)
{
    ssize_t len = 0;
    u8 status = 0;

    status = bsp_fan_get_status(fan_index);
    len = scnprintf(buf,  PAGE_SIZE, "%d\n", status);

    return len;
}

bool bsp_fan_custom_get_present(unsigned long *bitmap)
{
    int i;
    int status = FAN_STATUS_ABSENT;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_custom_get_present get bd failed");
        return -EINVAL;
    }

    if (NULL == bitmap)
    {
        DBG_ECHO(DEBUG_ERR, "function invalid pointer.");
        return false;
    }
    for (i = 0; i < bd->fan_num; i++)
    {
        status = bsp_fan_get_status(i);

        if (FAN_STATUS_ABSENT != status)
        {
            set_bit(i, bitmap);
        }
    }

    return true;
}

int bsp_fan_custom_get_led_status(int fan_index, char *buf)
{
    ssize_t len = 0;
    u8 tempu8 = 0;
    int ret = ERROR_SUCCESS;
    u8 status = 0;

    ret = bsp_cpld_get_fan_led_red(&tempu8, fan_index);
    if (ret != ERROR_SUCCESS)
    {
        len = ret;
        CHECK_IF_ERROR_GOTO_EXIT(ret, "fan %d get red led status failed!", fan_index + 1);
    }
    ret = bsp_cpld_get_fan_led_green(&status, fan_index);
    if (ERROR_SUCCESS != ret)
    {
        len = ret;
        CHECK_IF_ERROR_GOTO_EXIT(ret, "fan %d get green led status failed!", fan_index + 1);
    }

    if ((1 == tempu8) && (1 == status))
    {
        len = scnprintf(buf,  PAGE_SIZE, "%d\n", LED_COLOR_DARK);
    }
    else if ((1 == tempu8) && (0 == status))
    {
        len = scnprintf(buf,  PAGE_SIZE, "%d\n", LED_COLOR_GREEN);
    }
    else if ((0 == tempu8) && (1 == status))
    {
        len = scnprintf(buf,  PAGE_SIZE, "%d\n", LED_COLOR_RED);
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "get fan led status failed!");
        len = -ENODATA;
    }

exit:
    return len;
}

bool bsp_fan_custom_get_fan_pwm(unsigned int fan_index, u8 *buf)
{
    u8 tempu8 = 0;
    int ret = ERROR_SUCCESS;

    ret = bsp_cpld_get_fan_pwm_reg(&tempu8);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan pwm from cpld failed");
        return false;
    }
    else
    {
        *buf = tempu8;
        return true;
    }
}

int bsp_fan_custom_get_raw_data(int fan_index, char *buf)
{
    ssize_t len = 0;
    I2C_DEVICE_E enDevAddr = I2C_DEV_BUTT;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_custom_get_raw_data get bd failed");
        return -EINVAL;
    }

    enDevAddr = I2C_DEV_FAN + fan_index;
    ret = bsp_i2c_24LC128_eeprom_read_bytes(bd->i2c_addr_fan[fan_index], 0,  bd->eeprom_used_size, buf, enDevAddr);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan raw data from eeprom failed");
        len = ret;
    }
    else
    {
        len =  bd->eeprom_used_size;
    }

    return len;
}

ssize_t bsp_fan_custom_get_speed(unsigned int fan_index, unsigned int *speed)
{
    int motor_index;
    int ret = ERROR_SUCCESS;
    u16 fan_motor_speed = 0;
    int fan_speed = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_custom_get_speed get bd failed");
        return -EINVAL;
    }

    for (motor_index = 0; motor_index < bd->motors_per_fan; motor_index++)
    {
        ret = bsp_cpld_get_fan_speed(&fan_motor_speed, fan_index, motor_index);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "get fan %d motor %d speed failed", fan_index + 1, motor_index + 1);
            return ret;
        }
        fan_speed += fan_motor_speed;
    }

    *speed = fan_speed / bd->motors_per_fan;
    return ret;
}

bool bsp_fan_custom_set_fan_pwm(unsigned int fan_index, u8 pwm)
{
    int ret = ERROR_SUCCESS;

    ret = bsp_cpld_set_fan_pwm_reg(pwm);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "set fan pwm cpld failed");
        return false;
    }

    return true;
}

int bsp_fan_custom_set_led_status(int fan_index, int led_status)
{
    int ret = ERROR_SUCCESS;

    if (led_status == LED_COLOR_DARK)
    {
        ret = bsp_cpld_set_fan_led_red(CODE_LED_OFF, fan_index);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "set fan %d led red off failed", fan_index + 1);
        }
        ret = bsp_cpld_set_fan_led_green(CODE_LED_OFF, fan_index);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "set fan %d led green off  failed", fan_index + 1);
        }
    }
    else if (led_status == LED_COLOR_GREEN)
    {
        ret = bsp_cpld_set_fan_led_green(CODE_LED_ON, fan_index);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "set fan %d led green on failed", fan_index + 1);
        }
        ret = bsp_cpld_set_fan_led_red(CODE_LED_OFF, fan_index);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "set fan %d led red off failed", fan_index + 1);
        }
    }
    else if (led_status == LED_COLOR_RED)
    {
        ret = bsp_cpld_set_fan_led_green(CODE_LED_OFF, fan_index);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "set fan %d led green off failed", fan_index + 1);
        }
        ret = bsp_cpld_set_fan_led_red(CODE_LED_ON, fan_index);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "set fan %d led red on failed", fan_index + 1);
        }
    }
    else
    {
        DBG_ECHO(DEBUG_INFO, "fan led status %d out of range!", led_status);
        ret = -EINVAL;
    }

    return ret;
}

int bsp_fan_custom_set_raw_data(int fan_index, const char *buf, size_t count)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E enDevAddr = I2C_DEV_BUTT;
    u8 TepBuf = 0;
    int i;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_custom_motor_get_ratio get bd failed");
        return -EINVAL;
    }

    if (count > (bd->eeprom_used_size))
    {
        DBG_ECHO(DEBUG_ERR, "%d bytes larger than fan eeprom used size %d, abort!", (int)count, (int)bd->eeprom_used_size);
        return -ENOMEM;
    }

    enDevAddr = I2C_DEV_FAN + fan_index;
    //关闭eeprom写保护
    bsp_fan_eeprom_write_protect_set(fan_index, DISABLE);
    //前边用buf覆盖，后边全写0
    for (i = 0; i < bd->eeprom_used_size; i++)
    {
        TepBuf = (i < count) ? buf[i] : 0;
        ret = bsp_i2c_24LC128_eeprom_write_byte(bd->i2c_addr_fan[fan_index], i, TepBuf, enDevAddr);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "write fan eeprom fail");
        }
    }
    ret = bsp_fan_eeprom_write_protect_set(fan_index, ENABLE);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "write fan %d eeprom protect fail", fan_index + 1);
    }

    return ret;
}

int bsp_fan_custom_motor_get_status(int fan_index, int motor_index, char *buf)
{
    ssize_t len = 0;
    int fan_motor_status = 0;
    int ret = ERROR_SUCCESS;

    ret = bsp_fan_motor_get_status(fan_index, motor_index, &fan_motor_status);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan %d motor %d status failed", fan_index + 1, motor_index + 1);
        return ret;
    }
    len = scnprintf(buf, PAGE_SIZE, "%d\n", fan_motor_status);

    return len;
}

int bsp_fan_custom_motor_get_speed(int fan_index, int motor_index, char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    u16 fan_motor_speed = 0;

    ret = bsp_cpld_get_fan_speed(&fan_motor_speed, fan_index, motor_index);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan %d motor %d speed failed", fan_index + 1, motor_index + 1);
        return ret;
    }

    len = scnprintf(buf, PAGE_SIZE, "%d\n", fan_motor_speed);
    return len;
}

int bsp_fan_custom_motor_get_ratio(int fan_index, int motor_index, char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    u16 fan_motor_speed = 0;
    int temp = 0;
    u8 fan_vendor_name[FAN_EEPROM_STRING_MAX_LENGTH] = {0};
    int fan_max_speed = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_custom_motor_get_ratio get bd failed");
        return -EINVAL;
    }

    ret = bsp_cpld_get_fan_speed(&fan_motor_speed, fan_index, motor_index);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan %d motor %d speed failed", fan_index + 1, motor_index + 1);
        return ret;
    }

    ret = bsp_fan_get_info_from_eeprom(fan_index, VENDOR, fan_vendor_name);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan %d vendor failed", fan_index + 1);
        return ret;
    }

    if (0 == strcmp(fan_vendor_name, FAN_VENDOR_DELTA))
    {
        fan_max_speed = bd->fan_delta_max_speed[motor_index];
    }
    else if (0 == strcmp(fan_vendor_name, FAN_VENDOR_FOXOCNN))
    {
        fan_max_speed = bd->fan_foxconn_max_speed[motor_index];
    }
    else
    {
        fan_max_speed = bd->fan_max_speed;
    }


    temp = fan_motor_speed * 100 / fan_max_speed;
    if (temp > 100)
    {
        temp = 100;
    }
    len = scnprintf(buf, PAGE_SIZE, "%d\n", temp);

    return len;
}

int bsp_fan_custom_motor_get_tolerance(int fan_index, int motor_index, char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    u8 fan_vendor_name[FAN_EEPROM_STRING_MAX_LENGTH] = {0};
    int fan_max_speed = 0;
    int speed_tolerance;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_custom_motor_get_tolerance get bd failed");
        return -EINVAL;
    }

    ret = bsp_fan_get_info_from_eeprom(fan_index, VENDOR, fan_vendor_name);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan %d vendor failed", fan_index + 1);
        return ret;
    }

    if (0 == strncmp(fan_vendor_name, FAN_VENDOR_DELTA, FAN_EEPROM_STRING_MAX_LENGTH))
    {
        fan_max_speed = bd->fan_delta_max_speed[motor_index];
    }
    else if (0 == strncmp(fan_vendor_name, FAN_VENDOR_FOXOCNN, FAN_EEPROM_STRING_MAX_LENGTH))
    {
        fan_max_speed = bd->fan_foxconn_max_speed[motor_index];
    }
    else
    {
        fan_max_speed = bd->fan_max_speed;
    }
    speed_tolerance = 20 * fan_max_speed / 100;

    len = scnprintf(buf, PAGE_SIZE, "%d\n", speed_tolerance);
    return len;
}

int bsp_fan_custom_motor_get_target(int fan_index, int motor_index, char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    u8 tempu8;
    int fan_pwm_ratio = 0;
    int pwm_target;
    int fan_max_speed = 0;
    u8 fan_vendor_name[FAN_EEPROM_STRING_MAX_LENGTH] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_fan_custom_motor_get_target get bd failed");
        return -EINVAL;
    }

    ret = bsp_cpld_get_fan_pwm_reg(&tempu8);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan pwm from cpld failed");
        return ret;
    }

    if (tempu8 <= bd->fan_min_speed_pwm)
    {
        fan_pwm_ratio = bd->fan_min_speed_pwm;
    }
    else
    {
        fan_pwm_ratio = (tempu8 - bd->fan_min_speed_pwm) * (bd->fan_max_pwm_speed_percentage - bd->fan_min_pwm_speed_percentage) / (bd->fan_max_speed_pwm - bd->fan_min_speed_pwm) + bd->fan_min_speed_pwm;
    }

    ret = bsp_fan_get_info_from_eeprom(fan_index, VENDOR, fan_vendor_name);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan %d vendor failed", fan_index + 1);
        return ret;
    }

    if (0 == strncmp(fan_vendor_name, FAN_VENDOR_DELTA, FAN_EEPROM_STRING_MAX_LENGTH))
    {
        fan_max_speed = bd->fan_delta_max_speed[motor_index];
    }
    else if (0 == strncmp(fan_vendor_name, FAN_VENDOR_FOXOCNN, FAN_EEPROM_STRING_MAX_LENGTH))
    {
        fan_max_speed = bd->fan_foxconn_max_speed[motor_index];
    }
    else
    {
        fan_max_speed = bd->fan_max_speed;
    }
    pwm_target = fan_pwm_ratio * fan_max_speed / 100;
    len = scnprintf(buf, PAGE_SIZE, "%d\n", pwm_target);

    return len;
}

int bsp_fan_custom_motor_get_direction(int fan_index, int motor_index, char *buf)
{
    ssize_t len = 0;
    int ret = ERROR_SUCCESS;
    u8 fan_direction = 0;

    ret = bsp_cpld_get_fan_direction(&fan_direction, fan_index, motor_index);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get fan %d motor %d direction failed", fan_index + 1, motor_index + 1);
        return ret;
    }
    /* to cpld ,0 is B2F ,to sysfs 1 is B2F */
    fan_direction = (fan_direction == 0) ? 1 : 0;
    len = scnprintf(buf, PAGE_SIZE, "%d\n", fan_direction);

    return len;
}
