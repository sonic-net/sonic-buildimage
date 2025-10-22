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
#ifndef _PSU_H_
#define _PSU_H_


#define MODULE_NAME "psu"
#define DBG_ECHO(level, fmt, args...) DEBUG_PRINT(bsp_module_debug_level[BSP_PSU_MODULE], level, BSP_LOG_FILE, fmt, ##args)
#define DBG_ECHO_PSU(level, fmt, args...) DEBUG_PRINT(bsp_module_debug_level[BSP_PSU_MODULE], level, BSP_PSU_LOG_FILE, fmt, ##args)

#define ATTR_NAME_MAX_LEN    48
#define DRV_POWER_MCU_CRC_FRONT_LEN 3  /* 校验前三个字节:MCU地址低位0，MCU指令，MCU地址低位1 */
#define DRV_POWER_MCU_WRITE_CRC_FRONT_LEN 2  /* 校验前二个字节:MCU地址低位0，MCU指令 */
#define DRV_POWER_MCU_CRC_LEN 12
#define DRV_POWER_MCU_CRC_VAL 0x07  /* CRC多项式:x^8 + x^2 + x^1 + 1 */

#define DRV_POWER_BLACKBOX_MAX_LEN  156  /* 黑匣子记录信息长度 */
#define DRV_POWER_BLACKBOX_MAX_EVENT 5   /* 黑匣子记录事件数 */
#define DRV_POWER_BLACKBOX_EVENT_LEN 30  /* 黑匣子记录事件长度 */
#define DRV_POWER_BLACKBOX_EVENT_READ_LEN 32  /* 黑匣子记录事件读取长度 */

#define DRV_POWER_REG_Y                        0x07ff
#define DRV_POWER_REG_N                        0x7800
#define DRV_POWER_REG_Y_LEN                    11
#define DRV_POWER_REG_SYMBOL                   0X8000
#define DRV_POWER_REG_DEC_VALUE                125
#define DRV_POWER_REG_DEC_BIT                  3
#define DRV_POWER_REG_DEFAULT                  0x0001
#define DRV_POWER_REG_ENLARGE                  1000
#define DRV_POWER_REG_LEN                      8

typedef struct
{
    int status;
    int status_word;
    int vout;
    int iout;
    int vin;
    int iin;
    int pin;
    int pout;
    int tempinput;    //(当前温度)
    int fan_speed;    //(当前风速)
    int tempstatus;  //未使用
    int vin_type;    //未使用
    int alarm;
    int alarm_threshold_curr;
    int alarm_threshold_vol;
    int output_power;
    char sn_number[PSU_MAX_SN_LEN];    //未使用
    char pn_number[PSU_MAX_PRODUCT_NUM_LEN];  //未使用
    char product_name[PSU_MAX_PRODUCT_NAME_LEN];  //未使用
    char vendor_name[PSU_MAX_VENDOR_NAME_LEN];  //未使用
    char MFR_ID[PSU_MAX_MFR_ID_LEN + 1];  //未使用
    char hw_version[PSU_MAX_HW_VERSION_LEN + 1];  //未使用
    char fw_version[PSU_MAX_FW_VERSION_LEN + 1];  //未使用
    char date[PSU_MAX_DATE_LEN];  //未使用
    //struct attribute_group * psu_hwmon_attribute_group;
    struct device *parent_hwmon;
    struct kobject *customer_kobj;
} psu_info_st;

typedef enum
{
    DRV_POWER_INNER_IN_VOL = 0,
    DRV_POWER_INNER_IN_CUR,
    DRV_POWER_INNER_OUT_CUR,
    DRV_POWER_INNER_TEMP,
    DRV_POWER_INNER_FAN_SPEED1,
    DRV_POWER_INNER_IN_WATT,
    DRV_POWER_INNER_OUT_VOL
}DRV_POWERFAN_POWER_INNER_TYPE_E;

typedef struct tagDRV_POWERFAN_BlackboxInfo
{
    UINT uiPowerOnTime;
    UINT uiRealTime;
    u16 usStatusWord;
    u8 ucStatusIOut;
    u8 ucStatusInPut;
    u8 ucStatusTemperature;
    u8 ucStatusFan;
    u16 usReadVIn;
    u16 usReadIIn;
    u16 usReadIOut;
    u16 usReadTemFrist;
    u16 usReadTemSecond;
    u16 usReadFanSpeed;
    u16 usReadPIn;
    u16 usReadVOut;
} DRV_POWERFAN_BLACKBOX_INFO_S;

extern int bsp_psu_get_status(int psu_index);
extern char *bsp_psu_get_status_string(int status);
extern int bsp_psu_get_value(int command, int psu_index, u32 *value);

extern unsigned int bsp_psu_get_number(void);
extern ssize_t bsp_psu_get_vendor(unsigned int index, char *buf);
extern ssize_t bsp_psu_get_mfr_id(unsigned int index, char *buf);
extern ssize_t bsp_psu_get_model_name(unsigned int index, char *buf);
extern ssize_t bsp_psu_get_date(unsigned int index, char *buf);
extern ssize_t bsp_psu_get_hw_version(unsigned int index, char *buf);
extern ssize_t bsp_psu_get_fw_version(unsigned int index, char *buf);
extern ssize_t bsp_psu_get_sn(unsigned int index, char *buf);
extern ssize_t bsp_psu_get_pn(unsigned int index, char *buf);
extern bool bsp_psu_get_alarm(unsigned int index, int *alarm);
extern bool bsp_psu_get_alarm_threshold_curr(unsigned int index, int *alarm);
extern bool bsp_psu_get_alarm_threshold_vol(unsigned int index, int *alarm);
extern bool bsp_psu_get_max_output_power(unsigned int index, int *power);
extern ssize_t bsp_psu_get_data(unsigned int index, char *buf);
extern int bsp_psu_get_in_curr(unsigned int psu_index, int *value);
extern int bsp_psu_get_out_curr(unsigned int psu_index, int *value);
extern int bsp_psu_get_in_vol(unsigned int psu_index, int *value);
extern int bsp_psu_get_out_vol(unsigned int psu_index, int *value);
extern int bsp_psu_get_in_vol_type(unsigned int psu_index, char *buf);
extern int bsp_psu_get_status_word(unsigned int psu_index, int *value);
extern int bsp_psu_get_status_temperature(unsigned int psu_index, int *value);
extern int bsp_psu_get_temp_input(unsigned int psu_index, int *value);
extern int bsp_psu_get_fan_speed(unsigned int psu_index, int *value);
extern int bsp_psu_get_power_in(unsigned int psu_index, int *value);
extern int bsp_psu_get_power_out(unsigned int psu_index, int *value);
extern int bsp_psu_get_fan_status(unsigned int psu_index, int *value);
extern int bsp_psu_get_vout_status(unsigned int psu_index, int *value);
extern int bsp_psu_get_input_status(unsigned int psu_index, int *value);
extern bool bsp_psu_set_data(int psu_index, const char *buf, size_t count);
extern INT bsp_psu_set_bbox_status(IN UINT uiPowerIndex,u8 flag);
extern int bsp_psu_get_bbox_status(unsigned int psu_index, int *value);
extern INT drv_powerfan_ClearPowerBlackbox(IN UINT uiPowerIndex);
extern UINT drv_powerfan_DisplayPowerBlackbox(IN UINT uiPowerID,IN UINT uiBuffLength,INOUT UINT *puiUsedLength,OUT CHAR *pcHeader);
extern VOID drv_powerfan_DisplayPowerBlackboxByID(IN UINT uiPowerIndex,IN UINT uiBuffLength,INOUT UINT *puiUsedLength,OUT CHAR *pcHeader);
extern VOID drv_powerfan_DisplayBlackboxInfoByID(IN UINT uiPowerIndex,IN UINT uiBuffLength,INOUT UINT *puiUsedLength,OUT CHAR *pcHeader);
extern ULONG PDT_POWERFAN_BlackBoxEventGet(IN UINT uiPowerIndex,IN u8 ucEventId,OUT u8 *pucBlackbox);
extern ULONG pdt_powerfan_BlackBoxEventGetTD4(IN UINT uiPowerIndex,IN u8 ucEventId,OUT u8 *pucBlackbox);
extern UINT PDT_POWERFAN_PowerMcuValPrasing(IN DRV_POWERFAN_POWER_INNER_TYPE_E enParaType,IN UINT uiLen,IN const u8 *pucMcuVal);
extern UINT pdt_powerfan_PowerMcuValPrasingTD4(IN DRV_POWERFAN_POWER_INNER_TYPE_E enParaType,IN UINT uiLen,IN const u8 *pucMcuVal);
extern u8 pdt_powerfan_McuWriteCrcCal(IN UINT uiPowerIndex,IN UINT uiOffsetAddr,IN const u8 *pucCrc,IN UINT uiLen);
extern u8 McuWriteCrcCal(IN UINT uiPowerIndex, IN UINT uiInnerAddr, u8 *arr, u8 len);

#endif
