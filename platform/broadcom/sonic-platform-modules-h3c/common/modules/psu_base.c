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
#include <linux/module.h>
#include <asm/io.h>
/*私有文件*/
#include "pub.h"
#include "bsp_base.h"
#include "psu.h"


char *psu_status_string[] = {"Absent", "Normal", "Fault", "Unknown"};

//除法返回浮点数字符串
char *math_int_divide(int dividend, int divisor, int decimal_width, char *result)
{
    int i = 0;
    int mod = dividend % divisor;
    int integer = 0;
    int decimal = 0;
    //char temp[20] = {0};
    int len = 0;
    integer = dividend / divisor;
    decimal_width = decimal_width > 10 ? 10 : decimal_width;
    len = sprintf(result, "%d%s", integer, decimal_width == 0 ? "" : ".");
    for (i = 0; i < decimal_width; i++, decimal *= 10)
    {
        decimal = mod * 10 / divisor;
        len += sprintf(result + len, "%d", decimal);
        mod = mod * 10 % divisor;
    }
    return result;
}

s32 linear11_to_s32_with_scale(const u16 val, const s32 scale)
{
    const s32 exp = ((s16)val) >> 11;
    const s32 mant = ((s16)((val & 0x7ff) << 5)) >> 5;
    const s32 result = mant * scale;
    return (exp >= 0) ? (result << exp) : (result >> -exp);
}

//mv
u32 calc_voltage(u8 *CalcNum)
{
    u32 Vout = (CalcNum[0] + (((u32)CalcNum[1]) << 8)) * 1000 / 512;
    return Vout;
}

s32 linear16_to_s32_with_scale(const u16 val, const s32 scale)
{
    u32 usTempMant = (u32)val * scale;
    return usTempMant >> 9;
}

unsigned int bsp_psu_get_number(void)
{
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_number get bd failed");
        return -EINVAL;
    }
    return (unsigned int)bd->psu_num;
}

ssize_t bsp_psu_get_vendor(unsigned int index, char *buf)
{
    ssize_t ret = -ENOSYS;
    u8 absent = 0;
    char temp_value[PSU_MAX_INFO_LEN] = {0};
    I2C_DEVICE_E i2c_device_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_vendor get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_psu_absent(&absent, index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get absent failed!", index + 1);
    if (1 == absent)
    {
        ret = -ENODEV;
        goto exit;
    }

    i2c_device_index = I2C_DEV_PSU + index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu[index], REG_ADDR_PW650W_VENDOR, PW650_VENDOR_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_vendor failed", index + 1);
        ret = scnprintf(buf, PSU_MAX_VENDOR_NAME_LEN, "%s\n", temp_value);
    }
    else if (bd->psu_type == PSU_TYPE_1600W_TD4)
    {
        // unlock_i2c_path(); //  bsp_syseeprom_get_onie_tlv also get lock ,在i2c  整改后，这个地方在优化
        ret = bsp_syseeprom_get_onie_tlv(i2c_device_index, TLV_CODE_VENDOR_NAME, temp_value);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_vendor failed", index + 1);
        ret = scnprintf(buf, PSU_MAX_VENDOR_NAME_LEN, "%s\n", temp_value);
    }
    else
    {
        ret = -ENOSYS;
    }

exit:
    return ret;
}

ssize_t bsp_psu_get_mfr_id(unsigned int index, char *buf)
{
    ssize_t ret = -ENOSYS;
    int i;
    unsigned char temp_value[PSU_MAX_INFO_LEN] = {0};
    unsigned len = 0;
    I2C_DEVICE_E i2c_device_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_mfr_id get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + index;
    memset(temp_value, 0x00, PSU_MAX_INFO_LEN);
    ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[index], REG_ADDR_MFR_ID, PSU_MAX_MFR_ID_LEN, temp_value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_vendor_name_id failed", index + 1);
    len = temp_value[0];

    if (temp_value[0] <= (PSU_MAX_MFR_ID_LEN - 1))
    {
        /*first byte is the element length, followed by the content*/
        temp_value[temp_value[0] + 1] = '\0';
    }
    else
    {
        temp_value[PSU_MAX_MFR_ID_LEN] = '\0';
    }

    /*trim 0xff*/
    for (i = 0; i < PSU_MAX_MFR_ID_LEN; i++)
    {
        if (temp_value[i] == (u8)0xff)
            temp_value[i] = '\0';
    }
    /*数据长度+字符串终止符+换行*/
    ret = scnprintf(buf, len + 2, "%s\n", temp_value + 1);

exit:
    return ret;
}

int bsp_psu_get_bbox_status(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_bbox get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        DBG_ECHO(DEBUG_ERR, "psu_get_bbox not support!");
        ret = -ENOSYS;
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_BBOX, 1, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_bbox failed", psu_index + 1);
        *value = temp_value[0]&0x01;
    }

exit:
    return ret;
}

ssize_t bsp_psu_get_model_name(unsigned int index, char *buf)
{
    ssize_t ret = -ENOSYS;
    u8 absent = 0;
    char temp_value[PSU_MAX_INFO_LEN] = {0};
    I2C_DEVICE_E i2c_device_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_model_name get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_psu_absent(&absent, index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get absent failed!", index + 1);
    if (1 == absent)
    {
        ret = -ENODEV;
        goto exit;
    }

    i2c_device_index = I2C_DEV_PSU + index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        //以sn判断电源类型
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu[index], REG_ADDR_PW650W_SN, PW650_SN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_product_name failed", index + 1);
        if (NULL != strstr(temp_value, "0231A0QM"))
        {
            ret = sprintf(buf, "%s\n", "LSVM1AC650");
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "psu %d get_temp_value invalid\n", index + 1);
            ret = -EINVAL;
        }
    }
    else if (bd->psu_type == PSU_TYPE_1600W_TD4)
    {
        // unlock_i2c_path(); //  bsp_syseeprom_get_onie_tlv also get lock ,在i2c  整改后，这个地方在优化
        ret = bsp_syseeprom_get_onie_tlv(i2c_device_index, TLV_CODE_PRODUCT_NAME, temp_value);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_product_name failed", index + 1);
        ret = scnprintf(buf, PSU_MAX_PRODUCT_NAME_LEN, "%s\n", temp_value);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu[index], REG_ADDR_PW1600W_PDTNAME, PW1600_PRONUMB_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_product_name failed", index + 1);
        ret = sprintf(buf, "%s\n", temp_value);
    }

exit:
    return ret;
}

ssize_t bsp_psu_get_date(unsigned int index, char *buf)
{
    ssize_t ret = -ENOSYS;
    char temp_value[PSU_MAX_INFO_LEN] = {0};
    I2C_DEVICE_E i2c_device_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_date get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + index;
    if (bd->psu_type == PSU_TYPE_1600W_TD4)
    {
        // unlock_i2c_path(); //  bsp_syseeprom_get_onie_tlv also get lock ,在i2c  整改后，这个地方在优化
        ret = bsp_syseeprom_get_onie_tlv(i2c_device_index, TLV_CODE_MANUF_DATE, temp_value);
        if (ERROR_SUCCESS == ret)
        {
            ret = scnprintf((u8 *)buf, PSU_MAX_SN_LEN, "%s\n", temp_value);
        }
    }
    else
    {
        ret = -ENOSYS;
    }

    return ret;
}


ssize_t bsp_psu_get_hw_version(unsigned int index, char *buf)
{
    int i;
    u8 absent = 0;
    ssize_t ret = -ENOSYS;
    char temp_value[PSU_MAX_INFO_LEN] = {0};
    I2C_DEVICE_E i2c_device_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_hw_version get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_psu_absent(&absent, index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get absent failed!", index + 1);
    if (1 == absent)
    {
        ret = -ENODEV;
        goto exit;
    }

    i2c_device_index = I2C_DEV_PSU + index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[index], REG_ADDR_PW650W_HW_VER, PSU_MAX_HW_VERSION_LEN, temp_value, i2c_device_index);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[index], REG_ADDR_PW1600W_HW_VER, PSU_MAX_HW_VERSION_LEN, temp_value, i2c_device_index);
    }
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_hw_version failed", index + 1);

    if (temp_value[0] < (PSU_MAX_HW_VERSION_LEN - 1))
    {
        //first byte is the element length, followed by the content
        temp_value[temp_value[0] + 1] = '\0';
    }
    else
    {
        temp_value[PSU_MAX_HW_VERSION_LEN] = '\0';
    }
    //trim 0xff
    for (i = 0; i < PSU_MAX_HW_VERSION_LEN; i++)
    {
        if (temp_value[i] == (u8)0xff)
            temp_value[i] = '\0';
    }
    ret = scnprintf(buf, PAGE_SIZE, "%s\n", temp_value + 1);

exit:
    return ret;
}

ssize_t bsp_psu_get_fw_version(unsigned int index, char *buf)
{
    int i;
    u8 absent = 0;
    ssize_t ret = -ENOSYS;
    char temp_value[PSU_MAX_INFO_LEN] = {0};
    I2C_DEVICE_E i2c_device_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_fw_version get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_psu_absent(&absent, index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get absent failed!", index + 1);
    if (1 == absent)
    {
        ret = -ENODEV;
        goto exit;
    }

    i2c_device_index = I2C_DEV_PSU + index;

    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[index], REG_ADDR_PW650W_FW_VER, PSU_MAX_FW_VERSION_LEN, temp_value, i2c_device_index);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[index], REG_ADDR_PW1600W_HW_VER, PSU_MAX_FW_VERSION_LEN, temp_value, i2c_device_index);
    }
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_fw_version failed", index + 1);
    if (temp_value[0] < (PSU_MAX_FW_VERSION_LEN - 1))
    {
        //first byte is the element length, followed by the content
        temp_value[temp_value[0] + 1] = '\0';
    }
    else
    {
        temp_value[PSU_MAX_FW_VERSION_LEN] = '\0';
    }
    //trim 0xff
    for (i = 0; i < PSU_MAX_FW_VERSION_LEN; i++)
    {
        if (temp_value[i] == (u8)0xff)
            temp_value[i] = '\0';
    }
    ret = scnprintf(buf, PAGE_SIZE, "%s\n", temp_value + 1);

exit:
    return ret;
}

ssize_t bsp_psu_get_sn(unsigned int index, char *buf)
{
    ssize_t ret = -ENOSYS;
    u8 absent = 0;
    char temp_value[PSU_MAX_INFO_LEN] = {0};
    I2C_DEVICE_E i2c_device_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_sn get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_psu_absent(&absent, index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get absent failed!", index + 1);
    if (1 == absent)
    {
        ret = -ENODEV;
        goto exit;
    }

    i2c_device_index = I2C_DEV_PSU + index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu[index], REG_ADDR_PW650W_SN, PW650_SN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_sn failed", index + 1);
        scnprintf(buf, PSU_MAX_SN_LEN, "%s", temp_value);
    }
    else if (bd->psu_type == PSU_TYPE_1600W_TD4)
    {
        // unlock_i2c_path(); //  bsp_syseeprom_get_onie_tlv also get lock ,在i2c  整改后，这个地方在优化
        ret = bsp_syseeprom_get_onie_tlv(i2c_device_index, TLV_CODE_SERIAL_NUMBER, temp_value);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_sn failed", index + 1);
        ret = scnprintf(buf, PSU_MAX_SN_LEN, "%s\n", temp_value);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu[index], REG_ADDR_PW1600_SN, PW1600_SN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_sn failed", index + 1);
        ret = scnprintf(buf, PSU_MAX_SN_LEN, "%s\n", temp_value);
    }

exit:
    return ret;
}

ssize_t bsp_psu_get_pn(unsigned int index, char *buf)
{
    ssize_t ret = -ENOSYS;
    u8 absent = 0;
    char temp_value[PSU_MAX_INFO_LEN] = {0};
    I2C_DEVICE_E i2c_device_index;
    char pn[PART_NUMBER_LEN + 1] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_pn get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_psu_absent(&absent, index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get absent failed!", index + 1);
    if (1 == absent)
    {
        ret = -ENODEV;
        goto exit;
    }

    i2c_device_index = I2C_DEV_PSU + index;
    if (bd->psu_type == PSU_TYPE_1600W_TD4)
    {
        // unlock_i2c_path(); //  bsp_syseeprom_get_onie_tlv also get lock ,在i2c  整改后，这个地方在优化
        ret = bsp_syseeprom_get_onie_tlv(i2c_device_index, TLV_CODE_SERIAL_NUMBER, temp_value);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_pn failed", index + 1);
        strncpy(pn, temp_value + PART_NUMBER_OFFSET_LEN, PART_NUMBER_LEN);
        ret = scnprintf(buf, PSU_MAX_PRODUCT_NUM_LEN, "%s\n", pn);
    }
    else if (bd->psu_type == PSU_TYPE_1600W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu[index], REG_ADDR_PW1600_SN, PW1600_SN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_pn failed", index + 1);
        ret = scnprintf(buf, PSU_MAX_SN_LEN, "%s\n", temp_value);
    }
    else
    {
        ret = -ENOSYS;
    }

exit:
    return ret;
}

bool bsp_psu_get_alarm(unsigned int index, int *alarm)
{
    int tmp_alarm = 0;
    int value = 0;
    ssize_t ret = -ENOSYS;
    char temp_value[PSU_MAX_INFO_LEN] = {0};
    I2C_DEVICE_E i2c_device_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_alarm get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + index;
    if (PSU_TYPE_1600W_TD4 == bd->psu_type)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[index], REG_ADDR_PW1600W_ALARM_TEMP, PW1600_FAN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_value failed", index + 1);
        if (0 != (temp_value[0] & REG_ADDR_PW1600W_ALARM_TEMP_MASK))
        {
            value = 1;
        }
        tmp_alarm |= value << PSU_ALARM_TEMP_SHIFT;

        value = 0;
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[index], REG_ADDR_PW1600W_ALARM_FAN, PW1600_FAN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_value failed", index + 1);
        if (0 != (temp_value[0] & REG_ADDR_PW1600W_ALARM_FAN_MASK))

        {
            value = 1;
        }
        tmp_alarm |= value << PSU_ALARM_FAN_SHIFT;

        value = 0;
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[index], REG_ADDR_PW1600W_ALARM_STATUS_IOUT, PW1600_FAN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_value failed", index + 1);
        if (0 != (temp_value[0] & REG_ADDR_PW1600W_ALARM_STATUS_IOUT_MASK))
        {
            value = 1;
        }
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[index], REG_ADDR_PW1600W_ALARM_STATUS_INPUT, PW1600_FAN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_value failed", index + 1);
        if (0 != (temp_value[0] & REG_ADDR_PW1600W_ALARM_STATUS_INPUT_MASK))
        {
            value = 1;
        }
        tmp_alarm |= (value << PSU_ALARM_VOL_SHIFT);
        *alarm = tmp_alarm;
    }
    else
    {
        ret = -ENOSYS;
    }

exit:
    if (ERROR_SUCCESS == ret)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bsp_psu_get_alarm_threshold_curr(unsigned int index, int *alarm)
{
    *alarm = 145;    //A
    return true;
}

bool bsp_psu_get_alarm_threshold_vol(unsigned int index, int *alarm)
{
    *alarm = 15;    //V
    return true;
}

bool bsp_psu_get_max_output_power(unsigned int index, int *power)
{
    *power = 1600;    //W
    return true;
}

ssize_t bsp_psu_get_data(unsigned int index, char *buf)
{
    ssize_t ret = -ENOSYS;
    I2C_DEVICE_E i2c_device_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_data get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + index;
    ret = bsp_i2c_24LC128_eeprom_read_bytes(bd->i2c_addr_psu[index], PSU_1600W_MANU_ADDR,  bd->eeprom_used_size, buf, i2c_device_index);
    if (ERROR_SUCCESS == ret)
    {
        ret = bd->eeprom_used_size;
    }

    return ret;
}

bool bsp_psu_get_present(unsigned long *bitmap)
{
    int i = 0;
    int ret = ERROR_SUCCESS;
    u8 absent;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_present get bd failed");
        return -EINVAL;
    }

    if (NULL == bitmap)
    {
        DBG_ECHO(DEBUG_ERR, "function invalid pointer");
        return false;
    }

    for (i = 0; i < bd->psu_num; i++)
    {
        ret = bsp_cpld_get_psu_absent(&absent, i);
        if (ERROR_SUCCESS != ret)
        {
            DBG_ECHO(DEBUG_ERR, "psu %d absent = %d\n", i + 1, (int)absent);
            return false;
        }
        if (1 != absent)
        {
            set_bit(i, bitmap);
        }
    }

    return true;
}

int bsp_psu_get_in_curr(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_in_curr get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        DBG_ECHO(DEBUG_ERR, "psu_get_in_curr not support!");
        ret = -ENOSYS;
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_IIN, PW1600_IIN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_in_curr failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }

exit:
    return ret;
}

int bsp_psu_get_out_curr(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_out_curr get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW650W_IOUT, PW650_IOUT_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_out_curr failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_IOUT, PW1600_IOUT_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_out_curr failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }

exit:
    return ret;
}

int bsp_psu_get_in_vol(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_in_vol get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        DBG_ECHO(DEBUG_ERR, "psu_get_in_vol not support!");
        ret = -ENOSYS;
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_VIN, PW1600_VIN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_in_vol failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }

exit:
    return ret;
}

int bsp_psu_get_out_vol(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_out_vol get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW650W_VOUT, PW650_VOUT_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_out_vol failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_VOUT, PW1600_VOUT_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_out_vol failed", psu_index + 1);
        *value = calc_voltage(temp_value) / 10;
    }

exit:
    return ret;
}

int bsp_psu_get_status_word(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_status_word get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW650W_WORDTATUS, PW650_STAWORD_BYTE_COUNT, temp_value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_status_word failed", psu_index + 1);
    *value = (temp_value[0] + (temp_value[1] << 8));

exit:
    return ret;
}

int bsp_psu_get_status_temperature(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_status_temperature get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW650W_TSTATUS, PW650_STATEMPURE_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_status_temperature failed", psu_index + 1);
        *value = temp_value[0];
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_TSTATUS, PW1600_STATEMPURE_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_status_temperature failed", psu_index + 1);
        *value = temp_value[0];
    }

exit:
    return ret;
}

int bsp_psu_get_temp_input(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_temp_input get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW650W_TEMPER, PW650_TEMPURE_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_temp_input failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_TEMPER, PW1600_TEMPURE_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_temp_input failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }

exit:
    return ret;
}

int bsp_psu_get_fan_speed(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_fan_speed get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW650W_FAN_2, PW650_FAN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_fan_speed failed", psu_index + 1);
        if (temp_value[0] == 0 && temp_value[1] == 0)
        {
            ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW650W_FAN_1, PW650_FAN_BYTE_COUNT, temp_value, i2c_device_index);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_fan_speed failed", psu_index + 1);
        }
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_FAN, PW1600_FAN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_fan_speed failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }
exit:
    return ret;
}

int bsp_psu_get_power_in(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_power_in get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW650W_PIN, PW650_PIN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_power_in failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_PIN, PW1600_PIN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_power_in failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }

exit:
    return ret;
}

int bsp_psu_get_power_out(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_power_out get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW650W_POUT, PW650_FAN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_power_out failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_POUT, PW1600_FAN_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_power_out failed", psu_index + 1);
        *value = linear11_to_s32_with_scale(((u16)temp_value[1] << 8) | temp_value[0], 100);
    }

exit:
    return ret;
}

int bsp_psu_get_fan_status(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_fan_status get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_1600W_TD4)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_STATUS_FAN, PW1600_ONE_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_fan_status failed", psu_index + 1);
        *value = temp_value[0];
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "psu_get_fan_status not support!");
        ret = -EINVAL;
    }

exit:
    return ret;
}

int bsp_psu_get_vout_status(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_vout_status get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_1600W_TD4)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_STATUS_VOUT, PW1600_ONE_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_vout_status failed", psu_index + 1);
        *value = temp_value[0];
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "psu_get_vout_status not support!");
        ret = -EINVAL;
    }

exit:
    return ret;
}

int bsp_psu_get_input_status(unsigned int psu_index, int *value)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_input_status get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_1600W_TD4)
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_STATUS_INPUT, PW1600_ONE_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_input_status failed", psu_index + 1);
        *value = temp_value[0];
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "psu_get_input_status not support!");
        ret = -EINVAL;
    }

exit:
    return ret;
}

int bsp_psu_get_in_vol_type(unsigned int psu_index, char *buf)
{
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E i2c_device_index;
    u8 temp_value[PSU_MAX_INFO_LEN] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_in_vol_type get bd failed");
        return -EINVAL;
    }

    i2c_device_index = I2C_DEV_PSU + psu_index;
    if (bd->psu_type == PSU_TYPE_650W)
    {
        DBG_ECHO(DEBUG_ERR, "psu_get_in_vol not support!");
        ret = -ENOSYS;
    }
    else
    {
        ret = bsp_i2c_power_reg_read(bd->i2c_addr_psu_pmbus[psu_index], REG_ADDR_PW1600W_INVOL_TYPE, PW1600_VIN_TYPE_BYTE_COUNT, temp_value, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get_in_vol failed", psu_index + 1);
        switch (temp_value[0])
        {
            case PSU_IN_VOL_TYPE_NO_INPUT:
                ret = scnprintf(buf, PAGE_SIZE, "NA\n");
                break;
            case PSU_IN_VOL_TYPE_AC:
                ret = scnprintf(buf, PAGE_SIZE, "AC\n");
                break;
            case PSU_IN_VOL_TYPE_HVDC:
                ret = scnprintf(buf, PAGE_SIZE, "HVDC\n");
                break;
            case PSU_IN_VOL_TYPE_NOT_SUPPORT:
                ret = scnprintf(buf, PAGE_SIZE, "NA\n");
                break;
            default:
                ret = -ENODATA;
                break;
        }
    }

exit:
    return ret;
}

char *bsp_psu_get_status_string(int status)
{
    if ((status >= PSU_STATUS_ABSENT) && (status <= PSU_STATUS_NOT_OK))
    {
        return psu_status_string[status];
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "psu unknown status %d", status);
        return psu_status_string[PSU_STATUS_NOT_OK];
    }
}

/*
 *获取电源状态
 * ！注意入参的正确性由调用者保证
 */
int bsp_psu_get_status(int psu_index)
{
    u8 absent = 0;
    u8 good = 0;
    int temp_status = PSU_STATUS_ABSENT;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_get_status get bd failed");
        return -EINVAL;
    }

    if (psu_index < 0 || psu_index >= bd->psu_num)
    {
        DBG_ECHO(DEBUG_INFO, "param err: psu_get_status index %d is out of range", psu_index);
        return PSU_STATUS_NOT_OK;
    }

    ret = bsp_cpld_get_psu_absent(&absent, psu_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get absent failed!", psu_index + 1);

    temp_status = PSU_STATUS_NOT_OK;
    ret = bsp_cpld_get_psu_good(&good, psu_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "psu %d get status failed!", psu_index + 1);

    temp_status = (absent == 1) ? PSU_STATUS_ABSENT :
                  ((good == 1) ? PSU_STATUS_OK : PSU_STATUS_NOT_OK);

exit:
    return temp_status;
}

bool bsp_psu_set_data(int psu_index, const char *buf, size_t count)
{
    int i = 0;
    int temp = 0;
    int ret = ERROR_SUCCESS;
    I2C_DEVICE_E enDevAddr = I2C_DEV_BUTT;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_psu_set_data get bd failed");
        return -EINVAL;
    }

    enDevAddr = I2C_DEV_PSU + psu_index;
    //关闭eeprom写保护
    //前边用buf覆盖，后边全写0
    for (i = 0; i < bd->eeprom_used_size; i++)
    {
        temp = (i < count) ? buf[i] : 0;
        ret = bsp_i2c_24LC128_eeprom_write_byte(bd->i2c_addr_psu[psu_index], (i + PSU_1600W_MANU_ADDR), temp, enDevAddr);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "write psu eeprom fail\n");
            return false;
        }
    }

    return true;
}

/* 清除黑盒电源数据 */
INT drv_powerfan_ClearPowerBlackbox(IN UINT uiPowerIndex)
{
    u8 aucMcuWriteInfo[2];
    u8 ucCrcCode;
    u8 data[64] = {0};
    u8 data_len = 0;

    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();

    /* 计算crc */
    ucCrcCode = McuWriteCrcCal(uiPowerIndex, REG_ADDR_PW1600W_BBOX_CLEAR, data,data_len);
    aucMcuWriteInfo[0] = ucCrcCode;
    ret = bsp_i2c_pmbus_eeprom_write_bytes(bd->i2c_addr_psu_pmbus[uiPowerIndex], (u16)REG_ADDR_PW1600W_BBOX_CLEAR, 1, aucMcuWriteInfo, I2C_DEV_PSU + uiPowerIndex);
    CHECK_IF_ERROR(ret, "clear info failed!");

    return ret;
}

/* 开启/关闭黑盒电源功能 */
INT bsp_psu_set_bbox_status(IN UINT uiPowerIndex, u8 flag)
{
    u8 aucMcuWriteInfo[2];
    u8 ucCrcCode;
    u8 data[64] = {0};
    u8 data_len = 0;

    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();

    data[0] = flag;
    data_len = 1;

    /* 计算crc */
    ucCrcCode = McuWriteCrcCal(uiPowerIndex, REG_ADDR_PW1600W_BBOX, data,data_len);
    aucMcuWriteInfo[0] = flag;
    aucMcuWriteInfo[1] = ucCrcCode;

    ret = bsp_i2c_pmbus_eeprom_write_bytes(bd->i2c_addr_psu_pmbus[uiPowerIndex], (u16)REG_ADDR_PW1600W_BBOX, 2, aucMcuWriteInfo, I2C_DEV_PSU + uiPowerIndex);
    CHECK_IF_ERROR(ret, "set bbox_status failed!");

    return ret;
}


/* step 1 */
/*****************************************************************************
 Func Name    : drv_powerfan_DisplayPowerBlackbox
 Description  : 回显电源MCU的黑匣子信息
 Input        : IN UINT uiPowerIndex,
                IN UINT uiBuffLength
                INOUT UINT *puiUsedLength
 Output       : OUT CHAR *pcHeader
 Return       : VOID
 Caution      :
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
------------------------------------------------------------------------------

*****************************************************************************/
UINT drv_powerfan_DisplayPowerBlackbox(IN UINT psu_index, IN UINT uiBuffLength, INOUT UINT *puiUsedLength, OUT char *buf)
{
    u32 uiUsedLen;
    u32 uiPowerNum;
    board_static_data *bd = bsp_get_board_data();

    uiUsedLen = *puiUsedLength;

    uiPowerNum = bd->psu_num;

    if (psu_index >= uiPowerNum)
    {
        uiUsedLen += (UINT)scnprintf(buf + uiUsedLen,
                                        (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                        " Power %u does not exist!\r\n",
                                        psu_index + 1);
        printk(KERN_ERR"num error\n");
    }
    else
    {
        drv_powerfan_DisplayPowerBlackboxByID(psu_index, uiBuffLength, &uiUsedLen, buf);
    }

    *puiUsedLength = uiUsedLen;
    return uiUsedLen;
}

/* step 2 */
VOID drv_powerfan_DisplayPowerBlackboxByID(IN UINT psu_index, IN UINT uiBuffLength, INOUT UINT *puiUsedLength, OUT CHAR *buf)
{
    int enPowerState;
    UINT uiUsedLen;

    uiUsedLen = *puiUsedLength;

    /* 不在位情况 */
    enPowerState = bsp_psu_get_status(psu_index);
    if (PSU_STATUS_ABSENT == enPowerState)
    {
        uiUsedLen += (UINT)scnprintf(buf + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     " Power module %u : absent!\r\n",
                                     psu_index+1);

        *puiUsedLength = uiUsedLen;
        return;
    }

    /* TODO:支持框式 */
    uiUsedLen += (UINT)scnprintf(buf + uiUsedLen,
                                (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                " Blackbox content for power module %u : ",
                                psu_index+1);

    /* TODO:判断是否支持黑盒，现默认支持 */
    drv_powerfan_DisplayBlackboxInfoByID(psu_index, uiBuffLength, &uiUsedLen, buf);
    *puiUsedLength = uiUsedLen;
    return;
}

/* step 3 */
/*****************************************************************************
 Func Name    : drv_powerfan_DisplayBlackboxInfoByID
 Description  : 按ID回显电源黑匣子信息
 Input        : IN UINT uiPowerIndex
                IN UINT uiBuffLength
                INOUT UINT *puiUsedLength
 Output       : OUT CHAR *pcHeader
 Return       : VOID
 Caution      :
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
------------------------------------------------------------------------------

*****************************************************************************/
VOID drv_powerfan_DisplayBlackboxInfoByID( IN UINT uiPowerIndex, IN UINT uiBuffLength, INOUT UINT *puiUsedLength, OUT CHAR *pcHeader)
{
    ULONG ulRet;
    u8 aucRegVal[DRV_POWER_BLACKBOX_MAX_LEN];
    UINT uiUsedLen = 0;
    UINT uiIndex;
    DRV_POWERFAN_BLACKBOX_INFO_S stPowerEvent =
    {
        .uiPowerOnTime = 1,
        .uiRealTime = 1,
        .usStatusWord = 1,
        .ucStatusIOut = 1,
        .ucStatusInPut = 1,
        .ucStatusTemperature = 1,
        .ucStatusFan = 1,
        .usReadVIn = 1,
        .usReadIIn = 1,
        .usReadIOut = 1,
        .usReadTemFrist = 1,
        .usReadTemSecond = 1,
        .usReadFanSpeed = 1,
        .usReadPIn = 1,
        .usReadVOut = 1
    };
    UINT uiData;
    UINT uiTimeStamp;

    uiUsedLen = *puiUsedLength;
    aucRegVal[0] = '\0';
    /* 先写获取TimeStamp，再写获取5条事件 */
    ulRet = PDT_POWERFAN_BlackBoxEventGet(uiPowerIndex, 0, aucRegVal);
    memcpy((void *)&uiTimeStamp, (void *)&aucRegVal[1], sizeof(uiTimeStamp));

    if(ERROR_SUCCESS != ulRet)
    {
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n This power not support get blackbox info.\r\n");
        *puiUsedLength = uiUsedLen;
        return;
    }

    uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                 (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                 "\r\n Time Stamp : %u", uiTimeStamp);

    for (uiIndex = 1; uiIndex < DRV_POWER_BLACKBOX_MAX_EVENT + 1; uiIndex++)
    {
        ulRet = PDT_POWERFAN_BlackBoxEventGet(uiPowerIndex, (u8)uiIndex, aucRegVal);
        memcpy(&stPowerEvent, &aucRegVal[1], (ULONG)DRV_POWER_BLACKBOX_EVENT_LEN);
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n------------------------------------------------------------------------"
                                     "\r\n Power supply event :%d"
                                     "\r\n Power on time : %u",
                                     uiIndex,
                                     stPowerEvent.uiPowerOnTime);

        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n Power real time : %u",
                                     stPowerEvent.uiRealTime);

        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n STATUS_WORD : 0x%x",
                                     stPowerEvent.usStatusWord);

        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n STATUS_IOUT : 0x%x    STATUS_INPUT : 0x%x",
                                     stPowerEvent.ucStatusIOut,
                                     stPowerEvent.ucStatusInPut);

        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n STATUS_TEMPERTATURE : 0x%x    STATUS_FAN : 0x%x",
                                     stPowerEvent.ucStatusTemperature,
                                     stPowerEvent.ucStatusFan);

        uiData = PDT_POWERFAN_PowerMcuValPrasing(DRV_POWER_INNER_IN_VOL, 1,
                                                 (u8 *)&stPowerEvent.usReadVIn);
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n READ_VIN : %u",
                                     uiData);

        uiData = PDT_POWERFAN_PowerMcuValPrasing(DRV_POWER_INNER_IN_CUR, 1,
                                                 (u8 *)&stPowerEvent.usReadIIn);
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n READ_IIN : %u",
                                     uiData);

        uiData = PDT_POWERFAN_PowerMcuValPrasing(DRV_POWER_INNER_OUT_CUR, 1,
                                                 (u8 *)&stPowerEvent.usReadIOut);
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n READ_IOUT : %u",
                                     uiData);

        uiData = PDT_POWERFAN_PowerMcuValPrasing(DRV_POWER_INNER_TEMP, 1,
                                                 (u8 *)&stPowerEvent.usReadTemFrist);
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n READ_TEM_1 : %u",
                                     uiData/1000);

        uiData = PDT_POWERFAN_PowerMcuValPrasing(DRV_POWER_INNER_TEMP, 1,
                                                 (u8 *)&stPowerEvent.usReadTemSecond);
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n READ_TEM_2 : %u",
                                     uiData/1000);

        uiData = PDT_POWERFAN_PowerMcuValPrasing(DRV_POWER_INNER_FAN_SPEED1, 1,
                                                 (u8 *)&stPowerEvent.usReadFanSpeed);
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n READ_FAN_SPEED : %u",
                                     uiData/1000);

        uiData = PDT_POWERFAN_PowerMcuValPrasing(DRV_POWER_INNER_IN_WATT, 1,
                                                 (u8 *)&stPowerEvent.usReadPIn);
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n READ_PIN : %u",
                                     uiData);

        uiData = PDT_POWERFAN_PowerMcuValPrasing(DRV_POWER_INNER_OUT_VOL, 1,
                                                 (u8 *)&stPowerEvent.usReadVOut);
        uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen,
                                     (ULONG)uiBuffLength - (ULONG)uiUsedLen,
                                     "\r\n READ_VOUT : %u",
                                     uiData);
    }

    uiUsedLen += (UINT)scnprintf(pcHeader + uiUsedLen, (ULONG)uiBuffLength - (ULONG)uiUsedLen, "\r\n");
    *puiUsedLength = uiUsedLen;
    return;
}

/* step 4 */
/*****************************************************************************
 Func Name    : PDT_POWERFAN_BlackBoxEventGet
 Description  : 从MCU获取Blackbox内容
 Input        : IN UINT uiPowerIndex   电源索引值
                IN u8 ucEventId     事件ID
                IN DRV_POWERFAN_POWERTYPE_E enPowerType 电源类型
 Output       : OUT u8 *pucBlackbox
 Return       : ULONG
                ERROR_SUCCESS 成功
                ERROR_FAILED  失败
 Caution      : 1、电源风扇模块调用
                2、调用本接口不会发生任务切换
                3、本接口未使用信号量/锁
                4、不涉及内存释放
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
------------------------------------------------------------------------------

*****************************************************************************/
ULONG PDT_POWERFAN_BlackBoxEventGet
(
    IN UINT uiPowerIndex,
    IN u8 ucEventId,
    OUT u8 *pucBlackbox
)
{
    ULONG ulRet;
    /* TODO：支持多种产品，现在只有一种 */
    ulRet = pdt_powerfan_BlackBoxEventGetTD4(uiPowerIndex, ucEventId, pucBlackbox);

    return ulRet;
}

/* step 5 */
/*****************************************************************************
 Func Name    : pdt_powerfan_BlackBoxEventGetTD4
 Description  : 从MCU获取Blackbox内容
 Input        : IN UINT uiPowerIndex   电源索引值
                IN u8 ucEventId     事件ID
 Output       : OUT u8 *pucBlackbox
 Return       : ULONG
                ERROR_SUCCESS 成功
                ERROR_FAILED  失败
 Caution      : 1、电源风扇模块调用
                2、调用本接口不会发生任务切换
                3、本接口未使用信号量/锁
                4、不涉及内存释放
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
------------------------------------------------------------------------------

*****************************************************************************/
ULONG pdt_powerfan_BlackBoxEventGetTD4
(
    IN UINT uiPowerIndex,
    IN u8 ucEventId,
    OUT u8 *pucBlackbox
)
{
    u8 aucBlackbox[DRV_POWER_BLACKBOX_EVENT_READ_LEN + 1] = {0};
    u8 aucMcuWriteInfo[2];
    u8 ucCrcCode;
    int ret = ERROR_SUCCESS;

    board_static_data *bd = bsp_get_board_data();

    aucMcuWriteInfo[0] = ucEventId;
    /* 计算crc */
    ucCrcCode = pdt_powerfan_McuWriteCrcCal(uiPowerIndex, 0xdb, aucMcuWriteInfo, 1);
    aucMcuWriteInfo[1] = ucCrcCode;

    /* 更换接口，写0xdb，读0xdc到aucBlackbox */
    ret = bsp_i2c_pmbus_eeprom_write_bytes(bd->i2c_addr_psu_pmbus[uiPowerIndex], (u16)0xdb, 2, aucMcuWriteInfo, I2C_DEV_PSU + uiPowerIndex);
    CHECK_IF_ERROR(ret, "write psu inner 0xdb failed");

    ret = bsp_i2c_pmbus_eeprom_read_bytes(bd->i2c_addr_psu_pmbus[uiPowerIndex], (u16)0xdc, DRV_POWER_BLACKBOX_EVENT_READ_LEN, aucBlackbox, I2C_DEV_PSU + uiPowerIndex);
    CHECK_IF_ERROR(ret, "read psu 0xdc failed");
    /* 置结束符 */
    aucBlackbox[DRV_POWER_BLACKBOX_EVENT_READ_LEN] = '\0';

    memcpy(pucBlackbox, aucBlackbox, (ULONG)DRV_POWER_BLACKBOX_EVENT_READ_LEN);

    return ret;
}


/*****************************************************************************
 Func Name    : PDT_POWERFAN_PowerMcuValPrasing
 Description  : 解析电源mcu中的信息
 Input        : IN DRV_POWERFAN_POWER_INNER_TYPE_E enParaType 获取的mcu数据类型
                IN UINT uiLen 缓冲区长度
                IN CHAR *pcMcuVal 缓冲区地址
 Output       : None
 Return       : 解析后的数值
 Caution      : 1、电源风扇模块调用
                2、调用本接口不会发生任务切换
                3、本接口未使用信号量/锁
                4、不涉及内存释放
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
------------------------------------------------------------------------------

*****************************************************************************/
UINT PDT_POWERFAN_PowerMcuValPrasing
(
    IN DRV_POWERFAN_POWER_INNER_TYPE_E enParaType,
    IN UINT uiLen,
    IN const u8 *pucMcuVal
)
{
    UINT uiValue;
    uiValue = pdt_powerfan_PowerMcuValPrasingTD4(enParaType, uiLen, pucMcuVal);
    return uiValue;
}

/*****************************************************************************
 Func Name    : pdt_powerfan_PowerMcuValPrasingTD4
 Description  : 解析TD4电源mcu中的信息
 Input        : IN DRV_POWERFAN_POWER_INNER_TYPE_E enParaType 获取的mcu数据类型
                IN UINT uiLen 缓冲区长度
                IN CHAR *pcMcuVal 缓冲区地址
 Output       :
 Return       : UINT 解析后的值
 Caution      :
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
------------------------------------------------------------------------------

*****************************************************************************/
UINT pdt_powerfan_PowerMcuValPrasingTD4(IN DRV_POWERFAN_POWER_INNER_TYPE_E enParaType, IN UINT uiLen, IN const u8 *pucMcuVal)
{
    UINT uiValue;
    u16 usRegValue;
    u16 usY;
    CHAR cN;
    u16 usTemp;
    UINT  uiDig;

    (VOID)uiLen;

    /* 将两个字节结合成一个数据 */
    usRegValue = pucMcuVal[1] << DRV_POWER_REG_LEN;
    usRegValue = usRegValue + pucMcuVal[0];

    /* 取低11位 */
    usY = usRegValue & DRV_POWER_REG_Y;

    /* 如果最高位是符号，则需要对N进行取补码 */
    if (DRV_POWER_REG_SYMBOL == (usRegValue & DRV_POWER_REG_SYMBOL))
    {
        /* 对N进行取补码 */
        cN = (((~usRegValue) & DRV_POWER_REG_N) >> DRV_POWER_REG_Y_LEN) + 1;

        /* 由于是负数，所有需要右移数据*/
        uiValue = ((UINT)usY >> (UINT)(u8)cN);

        /* 这部分主要是取小数点的值,因为右移会将原来的值全部移掉 */
        usTemp = (u16)((UINT)DRV_POWER_REG_DEFAULT << (UINT)(u8)cN) - (u16)1;
        usTemp = usY & usTemp;

        /* 我们这里只保留后3位小数点 */
        if (cN > DRV_POWER_REG_DEC_BIT)
        {
            usTemp = usTemp >> ((UINT)(u8)cN - DRV_POWER_REG_DEC_BIT);
        }

        /* 这里因为只取了后3位，所以需要只要将得到的值乘以125就是
         *小数点的值 ,因为小数点是乘以2的，因此，小数点每多一位，就需要将
         * 0.5多除以2，而只取3位,125就是这个值 */
        uiDig = DRV_POWER_REG_DEC_VALUE * usTemp;
        uiValue = uiValue * DRV_POWER_REG_ENLARGE + uiDig;
    }
    else
    {
        cN = (usRegValue & DRV_POWER_REG_N) >> DRV_POWER_REG_Y_LEN;
        uiValue = (UINT)usY << (UINT)(u8)cN;
        uiValue = uiValue * DRV_POWER_REG_ENLARGE;
    }

    if (DRV_POWER_INNER_OUT_VOL == enParaType)
    {
        uiValue = usRegValue / 512;
        uiValue = (uiValue * DRV_POWER_REG_ENLARGE) + (((usRegValue % 512) * DRV_POWER_REG_ENLARGE) / 512);
    }

    return uiValue;
}

/*****************************************************************************
 Func Name    : pdt_powerfan_McuWriteCrcCal
 Description  : PMBUS协议CRC校验值计算
 Input        : IN UINT uiPowerIndex  电源索引值
                IN UINT uiOffsetAddr  写MCU的地址
                IN u8 *pucCrc      待计算的CRC
                IN UINT uiLength      长度
 Output       : NONE
 Return       : u8 CRC结果
                DRV_u8_INVALID  无效值
 Caution      :
------------------------------------------------------------------------------
  Modification History
  DATE        NAME             DESCRIPTION
------------------------------------------------------------------------------

*****************************************************************************/
u8 pdt_powerfan_McuWriteCrcCal(IN UINT uiPowerIndex, IN UINT uiOffsetAddr, IN const u8 *pucCrc, IN UINT uiLen)
{
    u8 ucCrc;
    UINT uiIndex;
    UINT uiDataIndex;
    UINT uiCrcDataLen;
    UINT uiCrcDataPos;
    u8 ucMcuDevAddr;
    u8 aucCrcData[DRV_POWER_MCU_CRC_LEN];
    board_static_data *bd = bsp_get_board_data();

    ucMcuDevAddr = (u8)bd->i2c_addr_psu_pmbus[uiPowerIndex];
    aucCrcData[0] = (u8)((ucMcuDevAddr << 1) & 0xfe);
    aucCrcData[1] = (u8)uiOffsetAddr;

    for (uiIndex = 0; uiIndex < uiLen; uiIndex++)
    {
        uiCrcDataPos = DRV_POWER_MCU_WRITE_CRC_FRONT_LEN + uiIndex;
        aucCrcData[uiCrcDataPos] = pucCrc[uiIndex];
    }

    uiCrcDataLen = DRV_POWER_MCU_WRITE_CRC_FRONT_LEN + uiLen;

    ucCrc = 0;
    for(uiDataIndex = 0; uiDataIndex < uiCrcDataLen; uiDataIndex++)
    {
        ucCrc ^= aucCrcData[uiDataIndex];
        for (uiIndex = 0; uiIndex < 8; uiIndex++)
        {
            if (0 != (ucCrc & 0x80))  /* 需区分最高位是否为1 */
            {
                /* CRC多项式:x^8 + x^2 + x^1 + 1 */
                ucCrc = (u8)((ucCrc << 1) ^ DRV_POWER_MCU_CRC_VAL);
            }
            else
            {
                ucCrc = (u8)(ucCrc << 1);
            }
        }
    }

    return ucCrc;
}

u8 McuWriteCrcCal(IN UINT uiPowerIndex, IN UINT uiInnerAddr, u8 *arr, u8 len)
{
    int i = 0;
    u8 ucCrc;
    UINT uiIndex;
    UINT uiDataIndex;
    UINT uiCrcDataLen;
    u8 ucMcuDevAddr;
    u8 aucCrcData[DRV_POWER_MCU_CRC_LEN];
    board_static_data *bd = bsp_get_board_data();

    ucMcuDevAddr = (u8)bd->i2c_addr_psu_pmbus[uiPowerIndex];
    aucCrcData[0] = (u8)((ucMcuDevAddr << 1) & 0xfe);
    aucCrcData[1] = (u8)uiInnerAddr;

    for(i = 0; i<len; i++)
    {
        aucCrcData[2+i] = arr[0+i];
    }

    uiCrcDataLen = DRV_POWER_MCU_WRITE_CRC_FRONT_LEN + len;

    ucCrc = 0;
    for(uiDataIndex = 0; uiDataIndex < uiCrcDataLen; uiDataIndex++)
    {
        ucCrc ^= aucCrcData[uiDataIndex];
        for (uiIndex = 0; uiIndex < 8; uiIndex++)
        {
            if (0 != (ucCrc & 0x80))  /* 需区分最高位是否为1 */
            {
                /* CRC多项式:x^8 + x^2 + x^1 + 1 */
                ucCrc = (u8)((ucCrc << 1) ^ DRV_POWER_MCU_CRC_VAL);
            }
            else
            {
                ucCrc = (u8)(ucCrc << 1);
            }
        }
    }

    return ucCrc;
}
EXPORT_SYMBOL(bsp_psu_get_status);
