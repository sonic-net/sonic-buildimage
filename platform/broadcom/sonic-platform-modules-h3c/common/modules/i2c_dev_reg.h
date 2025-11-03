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

#ifndef __I2C_DEV_REGISTER_H
#define __I2C_DEV_REGISTER_H

#define REG_ADDR_LM75_TEMP        0x0
#define LM75_DEFAULT_HYST         75
//max6696
#define REG_ADDR_MAX6696_TEMP_LOCAL              0x0
#define REG_ADDR_MAX6696_TEMP_REMOTE             0x1
#define REG_ADDR_MAX6696_WRITE_CONFIG            0x9
#define REG_ADDR_MAX6696_READ_CONFIG             0x3
#define REG_ADDR_MAX6696_READ_ALERT_HI_LOCAL     0x5
#define REG_ADDR_MAX6696_READ_ALERT_LO_LOCAL     0x6
#define REG_ADDR_MAX6696_READ_ALERT_HI_REMOTE    0x7
#define REG_ADDR_MAX6696_READ_ALERT_LO_REMOTE    0x8
#define REG_ADDR_MAX6696_WRITE_ALERT_HI_LOCAL    0xB
#define REG_ADDR_MAX6696_WRITE_ALERT_LO_LOCAL    0xC

#define REG_ADDR_MAX6696_WRITE_ALERT_HI_REMOTE    0xD
#define REG_ADDR_MAX6696_WRITE_ALERT_LO_REMOTE    0xE

#define REG_ADDR_MAX6696_RW_OT2_LOCAL            0x17
#define REG_ADDR_MAX6696_RW_OT2_REMOTE           0x16

#define MAX6696_SPOT_SELECT_BIT             3

typedef enum
{
    MAX6696_LOCAL_SOPT_INDEX = 0,
    MAX6696_REMOTE_CHANNEL1_SOPT_INDEX,
    MAX6696_REMOTE_CHANNEL2_SOPT_INDEX,
    MAX6696_SPOT_NUM
} MAX6696_SPOT_INDEX;

typedef enum
{
    MAX6696_LOCAL_HIGH_ALERT = 0,
    MAX6696_LOCAL_LOW_ALERT,
    MAX6696_LOCAL_OT2_LIMIT,
    MAX6696_REMOTE_CHANNEL1_HIGH_ALERT,
    MAX6696_REMOTE_CHANNEL1_LOW_ALERT,
    MAX6696_REMOTE_CHANNEL1_OT2_LIMIT,
    MAX6696_REMOTE_CHANNEL2_HIGH_ALERT,
    MAX6696_REMOTE_CHANNEL2_LOW_ALERT,
    MAX6696_REMOTE_CHANNEL2_OT2_LIMIT,
    SET_MAX6696_LOCAL_HIGH_ALERT,
    SET_MAX6696_REMOTE_CHANNEL1_HIGH_ALERT,
    SET_MAX6696_REMOTE_CHANNEL2_HIGH_ALERT,
    SET_MAX6696_LOCAL_OT2_LIMIT,
    SET_MAX6696_REMOTE_CHANNEL1_OT2_LIMIT,
    SET_MAX6696_REMOTE_CHANNEL2_OT2_LIMIT,
    SET_MAX6696_LOCAL_LOW_ALERT,
    SET_MAX6696_REMOTE_CHANNEL1_LOW_ALERT,
    SET_MAX6696_REMOTE_CHANNEL2_LOW_ALERT,
    MAX6696_LIMIT_BUTT
} MAX6696_LIMIT_INDEX;

//power pmbus
#define REG_ADDR_PW650W_VOUT       0x8B       //电压输出
#define REG_ADDR_PW650W_IOUT       0x8C       //电流输出
//#define REG_ADDR_PW650W_VIN      0x88       //电压输入
//#define REG_ADDR_PW650W_IIN      0x89       //电流输出

#define REG_ADDR_MFR_ID            0x99
#define REG_ADDR_PW650W_WORDTATUS  0x79
#define REG_ADDR_PW650W_TSTATUS    0x7D       //温度状态输出
#define REG_ADDR_PW650W_TEMPER     0x8D       //当前温度输出
#define REG_ADDR_PW650W_FAN_1      0x90
#define REG_ADDR_PW650W_FAN_2      0x91
#define REG_ADDR_PW650W_SN         0xA0       //SN输出(从EE中去读)
#define REG_ADDR_PW650W_VENDOR     0x0F
#define REG_ADDR_PW650W_PRONUMB    0x15       //设备号输出(从EE中去读)
#define REG_ADDR_PW650W_PIN        0x97      //输入功率输出
#define REG_ADDR_PW650W_POUT       0x96      //输出功率输出
#define REG_ADDR_PW650W_HW_VER     0x9B
#define REG_ADDR_PW650W_FW_VER     0x9B

#define PW650_VOUT_BYTE_COUNT         2
#define PW650_IOUT_BYTE_COUNT         2
#define PW650_VIN_BYTE_COUNT          2
#define PW650_IIN_BYTE_COUNT          2
#define PW650_STAWORD_BYTE_COUNT      2
#define PW650_STATEMPURE_BYTE_COUNT   1
#define PW650_TEMPURE_BYTE_COUNT      2
#define PW650_FAN_BYTE_COUNT          2
#define PW650_SN_BYTE_COUNT           20
#define PW650_VENDOR_BYTE_COUNT       3
#define PW650_PRONUMB_BYTE_COUNT      12
#define PW650_PIN_BYTE_COUNT          2
#define PW650_POUT_BYTE_COUNT         2

#define REG_ADDR_PW1600W_ALARM_TEMP     0x7d    //电源的温度告警 bit6
#define REG_ADDR_PW1600W_ALARM_TEMP_MASK     0x40    //电源的温度告警 bit6
#define REG_ADDR_PW1600W_ALARM_FAN      0x81       //电源的风扇告警 fan1 bit4, fan2 bit5
#define REG_ADDR_PW1600W_ALARM_FAN_MASK      0x30      //电源的风扇告警 fan1 bit4, fan2 bit5
#define REG_ADDR_PW1600W_ALARM_STATUS_IOUT      0x7B       //Iout OC waring bit 5, Pout Op warning bit0
#define REG_ADDR_PW1600W_ALARM_STATUS_IOUT_MASK      0x21       //Iout OC waring bit 5, Pout Op warning bit0
#define REG_ADDR_PW1600W_ALARM_STATUS_INPUT      0x7c       //Vm UV waring bit 5, Im OC  warning bit1, Pm OP warning bit0
#define REG_ADDR_PW1600W_ALARM_STATUS_INPUT_MASK      0xff       //Vm UV waring bit 5, Im OC  warning bit1, Pm OP warning bit0


#define REG_ADDR_PW1600W_VOUT        0x8B
#define REG_ADDR_PW1600W_IOUT        0x8C
#define REG_ADDR_PW1600W_TEMPERATURE_2        0x8E
#define REG_ADDR_PW1600W_TEMPERATURE_3        0x8F
#define REG_ADDR_PW1600W_VIN         0x88
#define REG_ADDR_PW1600W_IIN         0x89
#define REG_ADDR_PW1600W_MFR_ID      0x99
#define REG_ADDR_PW1600W_HW_VER      0x9B
#define REG_ADDR_PW1600W_FW_VER      0xD9
#define REG_ADDR_PW1600W_BBOX        0xDF
#define REG_ADDR_PW1600W_BBOX_CLEAR  0xE0

#define REG_ADDR_PW1600W_STATUS_VOUT      0x7A
#define REG_ADDR_PW1600W_STATUS_IOUT      0x7B
#define REG_ADDR_PW1600W_STATUS_INPUT     0x7C
#define REG_ADDR_PW1600W_STATUS_FAN      0x81

//#define REG_ADDR_PW1600_SN           0x35
#define REG_ADDR_PW1600_SN          0x60
#define REG_ADDR_PW1600W_PDTNAME    0x40
#define REG_ADDR_PW1600W_FAN        0x90
#define REG_ADDR_PW1600W_TSTATUS    0x7D       //温度状态输出
#define REG_ADDR_PW1600W_TEMPER     0x8D       //当前温度输出
#define REG_ADDR_PW1600W_PIN        0x97      //输入功率输出
#define REG_ADDR_PW1600W_POUT       0x96      //输出功率输出
#define REG_ADDR_PW1600W_INVOL_TYPE 0x80
#define PW1600_VOUT_BYTE_COUNT        2
#define PW1600_IOUT_BYTE_COUNT        2
#define PW1600_VIN_BYTE_COUNT         2
#define PW1600_IIN_BYTE_COUNT         2
#define PW1600_VIN_TYPE_BYTE_COUNT    1
//#define PW1600_SN_BYTE_COUNT        19
#define PW1600_SN_BYTE_COUNT          20
#define PW1600_PRONUMB_BYTE_COUNT     33
#define PW1600_FAN_BYTE_COUNT         2
#define PW1600_STATEMPURE_BYTE_COUNT  1
#define PW1600_TEMPURE_BYTE_COUNT     2
#define PW1600_PIN_BYTE_COUNT         2
#define PW1600_POUT_BYTE_COUNT        2
#define PSU_MAX_SN_LEN            64
#define PSU_MAX_DATE_LEN            64
#define PW1600_ONE_BYTE_COUNT    1

#define PSU_MAX_PRODUCT_NUM_LEN   64
#define PSU_MAX_VENDOR_NAME_LEN      64
#define PSU_MAX_PRODUCT_NAME_LEN  64
#define PSU_MAX_MFR_ID_LEN        20
#define PSU_MAX_HW_VERSION_LEN    4
#define PSU_MAX_FW_VERSION_LEN    4
#define PSU_MAX_INFO_LEN          128
#define ADM11661_MAX_INFO_LEN          128
#define PART_NUMBER_OFFSET_LEN 2
#define PART_NUMBER_LEN 8

//fan
#define REG_ADDR_FAN_VENDOR_NAME    0xe8    //风扇本体厂商名称(从EE中去读)
#define REG_ADDR_FAN_SN             0x28    //SN输出(从EE中去读)
#define REG_ADDR_FAN_PDT_NAME       0x08   //设备号输出(从EE中去读)
#define REG_ADDR_FAN_HW_VER         0x0
#define FAN_VENDOR_NAME_BYTE_COUNT  32
#define FAN_SN_BYTE_COUNT           64
#define FAN_PDT_NAME_BYTE_COUNT     32
#define FAN_HW_VER_BYTE_COUNT       8
#define FAN_MAX_TYPE_COUNT          FAN_SN_BYTE_COUNT
//ISL68127
#define ISL68127_REG_VALUE_MAX_LEN   2

#define REG_ADDR_ISL68127_CMD_VOUT   0x21
#define REG_ADDR_ISL68127_CMD_PAGE   0x0
#define REG_ADDR_ISL68127_READ_VOUT   0x8b
#define REG_ADDR_ISL68127_READ_TEMPERATURE_1   0x8D
#define REG_ADDR_ISL68127_READ_TEMPERATURE_2   0x8E
#define REG_ADDR_ISL68127_READ_TEMPERATURE_3   0x8F
#define REG_ADDR_ISL68127_CMD_BLOCK_BOX        0xF7  //write 1095
#define REG_ADDR_ISL68127_CMD_BLOCK_BOX_DATA   0xF5  //read write 0x00 * 4
#define REG_ADDR_ISL68127_CMD_BLOCK_BOX_DATA_LINE 18
#define ISL68127_CMD_BLOCK_BOX_DATA_WRITE_LEN  2
#define ISL68127_CMD_BLOCK_BOX_DATA_READ_LEN  4


#define REG_ADDR_RA228_CMD_IC_DEVICE_ADDR       0xAE
#define REG_ADDR_RA228_CMD_BLOCK_BOX_ADDR       0xC7
#define REG_ADDR_RA228_CMD_BLOCK_BOX_DATA       0xC5
#define REG_ADDR_RA228_CMD_BLOCK_BOX_SEQUENTIAL 0xC6
#define RA228_CMD_BLOCK_BOX_DATA_READ_LEN_1  2
#define RA228_CMD_BLOCK_BOX_DATA_READ_LEN_2  4
#define RA228_CMD_BLOCK_BOX_DATA_READ_LEN_3  5
#define RA228_CMD_BLOCK_BOX_DATA_WRITE_LEN  2


#define REG_ADDR_ISL68127_CMD_BLOCK_BOX_SAVE   0xE7  //write 0001
#define REG_ADDR_RA228_CMD_VOUT   0x21
#define REG_ADDR_RA228_CMD_PAGE   0x0
#define REG_ADDR_RA228_READ_VOUT  0x8b
#define REG_ADDR_RA228_READ_TEMPERATURE   0x8E
#define REG_ADDR_RA228_READ_VERSION  0x9b

#define REG_ADDR_TX_DISABLE_8636   86
#define REG_ADDR_EEPROM_PAGE_8636  127

#define TX_DISABLE_BIT        0xf
#define TX1_DISABLE_BIT        1
#define TX2_DISABLE_BIT        2
#define TX3_DISABLE_BIT        4
#define TX4_DISABLE_BIT        8

#define REG_ADDR_TEMPERATURE_8636         22
#define REG_ADDR_VOLTAGE_8636             26
#define REG_ADDR_RX_POWER_8636            34
#define REG_ADDR_TX_BIAS_8636             42
#define REG_ADDR_TX_POWER_8636            50
#define REG_ADDR_MANUFACTURE_NAME_8636    148
#define REG_ADDR_MODEL_NAME_8636          168
#define REG_ADDR_CABLE_LENGTH_8636        142
#define REG_ADDR_CONNECTOR_8636           130
#define REG_ADDR_HW_VERSION_8636          184
#define REG_ADDR_INFO_BULK_8636           128
#define REG_ADDR_VENDOR_DATE_8636         212
#define REG_ADDR_VENDOR_OUI_8636          165
#define REG_ADDR_SERIAL_NUM_8636          196
#define REG_ADDR_TEMPERATURE_8472         96
#define REG_ADDR_VOLTAGE_8472             98
#define REG_ADDR_RX_POWER_8472            104
#define REG_ADDR_TX_BIAS_8472             100
#define REG_ADDR_TX_POWER_8472            102
#define REG_ADDR_MANUFACTURE_NAME_8472    20
#define REG_ADDR_MODEL_NAME_8472          40
#define REG_ADDR_SERIAL_NUM_8472          68
#define REG_ADDR_CONNECTOR_8472           2
#define REG_ADDR_HW_VERSION_8472          56
#define REG_ADDR_INFO_BULK_8472           0
#define REG_ADDR_VENDOR_DATE_8472         84
#define REG_ADDR_VENDOR_OUI_8472          37
#define REG_ADDR_CABLE_LENGTH_8472        18
//adm1166
#define REG_ADDR_DEV_ADM1166_BASE         0xa0
#define REG_ADDR_ADM1166_RRSEL1           0x80
#define REG_ADM1166_RRSEL1_VALUE          0xFF
#define REG_ADDR_ADM1166_RRSEL2           0x81
#define REG_ADM1166_RRSEL2_VALUE          0x1F
#define REG_ADDR_ADM1166_RRCTRL           0x82
#define REG_ADDR_ADM1166_UPDCFG           0x90
#define REG_ADDR_ADM1166_SECTRL           0x93
#define REG_ADDR_ADM1166_BBCTRL           0x9c
#define REG_ADDR_ADM1166_BBSEARCH         0xd9
#define REG_ADDR_ADM1166_VSCTRL           0x9c
#define ADM1166_RRCTRL_REG_RESET          0x00
#define REG_ADDR_ADM1166_UDOWNLD          0xd8

#ifdef _PDO
#define REG_ADDR_ADM1166_PDO1             0x07
#define REG_ADDR_ADM1166_PDO2             0x0F
#define REG_ADDR_ADM1166_PDO3             0x17
#define REG_ADDR_ADM1166_PDO4             0x1F
#define REG_ADDR_ADM1166_PDO5             0x27
#define REG_ADDR_ADM1166_PDO6             0x2F
#define REG_ADDR_ADM1166_PDO7             0x37
#define REG_ADDR_ADM1166_PDO8             0x3F
#define REG_ADDR_ADM1166_PDO9             0x47
#define REG_ADDR_ADM1166_PDO10            0x4F
#endif

#define ADM1166_RRCTRL_OPERATION_ENABLE      1
#define ADM1166_RRCTRL_OPERATION_STOPWRITE   2
#define ADM1166_RRCTRL_REG_GO            1<<0
#define ADM1166_RRCTRL_REG_ENABLE        1<<1
#define ADM1166_RRCTRL_REG_STOPWRITE     1<<3

#endif
