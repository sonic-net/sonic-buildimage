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
#include <linux/platform_device.h>

//This macro only used by developmet test
#define DEBUG_VERSION  0
#define  FT_TEST 1
#define INOUT
#define VOID void
#define CHAR char
#define IN
#define OUT
#define ERROR_SUCCESS 0
#define ERROR_FAILED 1
#define DEBUG_DBG    0x4
#define DEBUG_ERR    0x2
#define DEBUG_INFO   0x1
#define DEBUG_NO     0x0
#define DBG_STR0x4       "DEBUG"
#define DBG_STR0x2       "ERROR"
#define DBG_STR0x1       "INFO"

#define LOG_FILE_PATH_0         "/var/log/h3c_bsp_log1.txt"
#define LOG_FILE_PATH_1         "/var/log/h3c_bsp_log2.txt"
#define LOG_FILE_PATH_2         "/var/log/h3c_psu_log1.txt" //for psu
#define LOG_FILE_PATH_3         "/var/log/h3c_psu_log2.txt" //for psu

#define LOG_FILE_WDT_PATH1      "/var/log/h3c_wdt_log1.txt"  //for watchdog
#define LOG_FILE_WDT_PATH2      "/var/log/h3c_wdt_log2.txt"  //for watchdog

#define LOG_FILE_I2C_PATH1      "/var/log/h3c_i2c_log1.txt"  //for i2c
#define LOG_FILE_I2C_PATH2      "/var/log/h3c_i2c_log2.txt"  //for i2c

#define LOG_FILE_SIZE           5 * 1024 * 1024
#define LOG_PSU_FILE_SIZE       1 * 1024 * 1024

#define LOG_WDT_I2C_FILE_SIZE   2 * 1024 * 1024

#define LOG_STRING_LEN   256
#define LOG_LEVEL_DEBUG   0
#define LOG_LEVEL_INFO    1
#define LOG_LEVEL_ERR     2
#define LOG_LEVEL_CRIT    3

#define TEMP_STR_MAX_LEN    128

#define BSP_DEBUG_TIP_CAT       "For more information about sysfs nodes, please use 'cat_sysfs_node.py'."

#define MAX_HWMON_NAME_LEN  64
#define MAX_FILE_NAME_LEN 255
typedef enum
{
    BSP_LOG_FILE = 0,
    BSP_PSU_LOG_FILE,
    BSP_I2C_LOG_FILE,
    BSP_WDT_LOG_FILE,
    LOG_BUTT
} LOG_TYPE;

enum led_color
{
    LED_COLOR_DARK = 0,
    LED_COLOR_GREEN,
    LED_COLOR_YELLOW,
    LED_COLOR_RED,
    LED_COLOR_GREEN_FLASH,
    LED_COLOR_YELLOW_FLASH,
    LED_COLOR_RED_FLASH,
    LED_COLOR_BLUE
};

enum psu_status
{
    PSU_STATUS_ABSENT = 0,
    PSU_STATUS_OK,
    PSU_STATUS_NOT_OK
};

enum fan_status
{
    FAN_STATUS_ABSENT = 0,
    FAN_STATUS_OK,
    FAN_STATUS_NOT_OK
};

enum slot_status
{
    SLOT_STATUS_ABSENT = 0,
    SLOT_STATUS_NORMAL,
    SLOT_STATUS_FAULT,
    SLOT_STATUS_UNKNOWN
};
enum psu_alarm
{
    PSU_ALARM_TEMP_SHIFT = 0,
    PSU_ALARM_FAN_SHIFT,
    PSU_ALARM_VOL_SHIFT
};

#define ENABLE      1
#define DISABLE     0
#ifndef TRUE
#define TRUE        1
#endif
#ifndef FALSE
#define FALSE       0
#endif
#define MAX_SLOT_NUM                  5                                        //设备最大slot 数量, 对于有插卡的设备有意义
#define MAX_OPTIC_PER_SLOT            64                                       //单个子卡/主板最大光模块数量
#define MAX_OPTIC_COUNT               (MAX_OPTIC_PER_SLOT * (MAX_SLOT_NUM + 1))  //整个设备最大光模块数量, 主板有可能也有光模块，算到此数量里
#define MAX_PSU_NUM                   5              //最大电源个数
#define MAX_INA219_NUM                MAX_PSU_NUM    //INA219数量
#define MAX_FAN_NUM                   8
#define MAX_FAN_MOTER_NUM             4              //一个风扇最多有几个马达
#define MAX_EEPROM_PER_SLOT           4
#define MAX_EEPROM_NUM                (MAX_EEPROM_PER_SLOT * (MAX_SLOT_NUM + 1))  //设备最多几个eeprom
#define MAX_LM75_NUM_PER_SLOT         2
#define MAX_LM75_NUM                  (MAX_LM75_NUM_PER_SLOT * (MAX_SLOT_NUM + 1)) //设备最大LM75数量
#define MAX_MAX6696_NUM_PER_SLOT      4
#define MAX_MAX6696_NUM               (MAX_MAX6696_NUM_PER_SLOT * (MAX_SLOT_NUM + 1)) //每板卡最大6696数量
#define MAX_PHY_NUM_PER_SLOT          16              //每子卡最多多少个外部PHY
#define MAX_CPLD_NUM_PER_SLOT         8
#define MAX_CPLD_NUM                  (MAX_CPLD_NUM_PER_SLOT * (MAX_SLOT_NUM + 1))
#define MAX_ISL68127_NUM             4
#define MAX_RA228_NUM                1
#define MAX_TPS53659_NUM             1
#define MAX_TPS53622_NUM             1
#define MAX_ADM1166_NUM              2
#define MAX_PCA9545_NUM              16
#define MAX_PCA9548_NUM              16
#define INVALID                      -1
#define CPU_CPLD_INDEX                0
#define PSU_1600W_MANU_ADDR  1024  /*　0-1023  reserved  for GRE/FSP ***/

#define TLV_CODE_PRODUCT_NAME        0x21
#define TLV_CODE_PART_NUMBER         0x22
#define TLV_CODE_SERIAL_NUMBER       0x23
#define TLV_CODE_MAC_BASE                0x24
#define TLV_CODE_MANUF_DATE           0x25
#define TLV_CODE_DEVICE_VERSION      0x26
#define TLV_CODE_LABEL_REVISION      0x27
#define TLV_CODE_PLATFORM_NAME       0x28
#define TLV_CODE_ONIE_VERSION        0x29
#define TLV_CODE_MAC_SIZE            0x2A
#define TLV_CODE_MANUF_NAME          0x2B
#define TLV_CODE_MANUF_COUNTRY       0x2C
#define TLV_CODE_VENDOR_NAME         0x2D
#define TLV_CODE_DIAG_VERSION        0x2E
#define TLV_CODE_SERVICE_TAG         0x2F
#define TLV_CODE_SVENDOR             0x30
//#define TLV_CODE_CUSTOMER_CODE     0x31  comware   商业机用
//#define TLV_CODE_TRACKING_NUM      0x32  comware 商业机用
//#define TLV_CODE_MAC_COUNT         0x33  comware 商业机用
#define TLV_CODE_HW_VERSION          0x34
#define TLV_CODE_VENDOR_EXT          0xFD
#define TLV_CODE_CRC_32              0xFE

enum product_type
{
    PDT_TYPE_TCS83_120F_4U = 1,
    PDT_TYPE_TCS81_120F_1U,
    PDT_TYPE_TCS83_120F_32H_SUBCARD,
    PDT_TYPE_LS_9855_48CD8D_W1,
    PDT_TYPE_LS_9825_64D_W1,
    PDT_TYPE_LS_9855_32CDF_W1,
    PDT_TYPE_BUTT
};

typedef enum enuSubSlot
{
    MAIN_BOARD,           /*main*/
    FIRST_EXP_BOARD,      /*subslot1*/
    SECOND_EXP_BOARD,     /*subslot2*/
    THIRD_EXP_BOARD,      /*subslot3*/
    FOURTH_EXP_BOARD,     /*subslot4*/
    FIFTH_EXP_BOARD,      /*subslot5*/
    SIXTH_EXP_BOARD,      /*subslot6*/
    SEVENTH_EXP_BOARD,    /*subslot7*/
    EIGHTH_EXP_BOARD,
    SUBSLOT_BUTT
} SUBSLOT_TYPE_E;

//#define REG_WRITE  I2C_SMBUS_WRITE
//#define REG_READ   I2C_SMBUS_READ

/**************************************
*
*用于i2c选路，包括子卡和主设备的I2C设备索引
***************************************/

//I2C device index
typedef enum
{
    /*光模块索引开始*/
    I2C_DEV_OPTIC_IDX_START = 1,                                                     //主板光模块开始  1
    I2C_DEV_SLOT0_OPTIC_IDX_START = I2C_DEV_OPTIC_IDX_START,     //子卡光模块开始 65
    I2C_DEV_OPTIC_BUTT = I2C_DEV_OPTIC_IDX_START + MAX_OPTIC_COUNT,   //光模块索引结束 1+64*6=385

    /*除光模块外的其他器件起始*/
    I2C_DEV_EEPROM,  //386
    /*
                I2C_DEV_CPUBMC_EEPROM, it a eeprom shared in between cpu and bmc
                I2C_DEV_BMC_EEPROM, reserved for bmc
    */
    I2C_DEV_CPUBMC_EEPROM,  //387
    I2C_DEV_BMC_EEPROM,    //388
    I2C_DEV_SLOT0_EEPROM = I2C_DEV_EEPROM,              //slot 0 eeprom开始位置386+4=390
    I2C_DEV_EEPROM_BUTT =  I2C_DEV_EEPROM + MAX_EEPROM_NUM,  //386+4*6=410
    I2C_DEV_LM75,    //411
    I2C_DEV_SLOT0_LM75 = I2C_DEV_LM75,       //slot 0 lm75开始位置 413
    I2C_DEV_LM75_BUTT =  I2C_DEV_LM75 + MAX_LM75_NUM,    //411+2*6=423
    I2C_DEV_MAX6696,         //424
    I2C_DEV_MAX6696_BUTT = I2C_DEV_MAX6696 + MAX_MAX6696_NUM,   //424+4*6=448
    I2C_DEV_PSU,             //449
    I2C_DEV_PSU_BUTT = I2C_DEV_PSU + MAX_PSU_NUM,   // 449+5=454
    I2C_DEV_INA219,    //455
    I2C_DEV_INA219_BUTT = I2C_DEV_INA219 + MAX_INA219_NUM,   //455+5=460
    I2C_DEV_I350,   //461
    I2C_DEV_FAN,    //462
    I2C_DEV_FAN_BUTT = I2C_DEV_FAN + MAX_FAN_NUM,  //462+8=470
    I2C_DEV_ISL68127,   //471
    I2C_DEV_ISL68127_BUTT = I2C_DEV_ISL68127 + MAX_ISL68127_NUM,  //471+2=473
    I2C_DEV_RA228,
    I2C_DEV_RA228_BUTT = I2C_DEV_RA228 + MAX_RA228_NUM,
    I2C_DEV_TPS53659,
    I2C_DEV_TPS53659_BUTT = I2C_DEV_TPS53659 + MAX_TPS53659_NUM,
    I2C_DEV_TPS53622,
    I2C_DEV_TPS53622_BUTT = I2C_DEV_TPS53622 + MAX_TPS53622_NUM,
    I2C_DEV_ADM1166,   //474
    I2C_DEV_ADM1166_BUTT = I2C_DEV_ADM1166 + MAX_ADM1166_NUM,  //474+2=476
    I2C_DEV_N287,     //477
    I2C_DEV_RTC_4337,   //478
    I2C_DEV_BUTT
} I2C_DEVICE_E;

#define GET_I2C_DEV_OPTIC_IDX_START_SLOT(slot_index)               (I2C_DEV_SLOT0_OPTIC_IDX_START + (slot_index) * MAX_OPTIC_PER_SLOT)                            //子卡光模块索引开始位置
#define GET_I2C_DEV_EEPROM_IDX_START_SLOT(slot_index)              (I2C_DEV_SLOT0_EEPROM + (slot_index) * MAX_EEPROM_PER_SLOT)                                    //子卡EEPROM索引开始位置
#define GET_I2C_DEV_LM75_IDX_START_SLOT(slot_index)                (I2C_DEV_SLOT0_LM75 + (slot_index) * MAX_LM75_NUM_PER_SLOT)                                    //子卡LM75索引开始位置
#define GET_SLOT_INDEX_BY_I2C_OPTIC_DEV_IDX(i2c_optic_dev_index)   ((i2c_optic_dev_index) < I2C_DEV_SLOT0_OPTIC_IDX_START ? -1 : (((i2c_optic_dev_index) - I2C_DEV_SLOT0_OPTIC_IDX_START) / MAX_OPTIC_PER_SLOT ))                   //获取端口所在槽位

enum BSPMODULE_INDEX_E
{
    BSP_BASE_MODULE,
    BSP_CPLD_MODULE,
    BSP_FAN_MODULE,
    BSP_PSU_MODULE,
    BSP_SENSOR_MODULE,
    BSP_SYSEEPROM_MODULE,
    BSP_SYSLED_MODULE,
    BSP_XCVR_MODULE,
    BSP_WATCHDOG_MODULE,
    BSP_FPGA_MODULE,
    BSP_FT_MODULE,
    BSP_MODULE_MAX
};
/* module_name_string的定义在bsp_base_sysfs.c中，要和enum BSPMODULE_INDEX_E一一对应，不要漏！  */
extern char *module_name_string[BSP_MODULE_MAX];

/*
*function macro defination
*/
#define DEBUG_PRINT(switch, level, fno, fmt, args...) \
    do {\
        if(((level) & (switch)) != DEBUG_NO) {\
            char * ___temp_str = strrchr(__FILE__, '/');\
            ___temp_str = (___temp_str == NULL) ? __FILE__ : ___temp_str + 1;\
            if ((level) == DEBUG_NO) {\
                printk(KERN_ERR"BSP %s: "fmt, MODULE_NAME, ##args);\
                printk("\n");}\
            else {\
                char ___temp_buf[LOG_STRING_LEN] = {0};\
                int ___s = scnprintf(___temp_buf, LOG_STRING_LEN, "BSP %s: "fmt, MODULE_NAME, ##args);\
                if (bsp_h3c_localmsg_to_file(___temp_buf, (long)___s, level, ___temp_str, __LINE__, fno) != ERROR_SUCCESS)\
                    {printk(KERN_ERR"BSP %s: "fmt, MODULE_NAME, ##args);\
                    printk(KERN_ERR"\n");}}\
            }\
    } while(0);

#define INIT_PRINT(fmt, args...) \
    do {\
        char * ___temp_str = strrchr(__FILE__, '/');\
        ___temp_str = (___temp_str == NULL) ? __FILE__ : ___temp_str + 1;\
        printk(KERN_INFO"BSP %s: "fmt, MODULE_NAME, ##args);\
        printk(KERN_INFO"\n");\
        } while(0);

#if 0
//no need SYSLOG
#define SYSLOG(level, fmt, args...) \
    do {\
        char ___temp_buf[LOG_STRING_LEN] = {0};\
        int ___s = scnprintf(___temp_buf, LOG_STRING_LEN, "BSP: [SYSLOG-%d]:"fmt, level, ##args);\
        bsp_h3c_localmsg_to_file(___temp_buf, (long)___s, level, "SYSLOG", __LINE__);\
    } while(0);

#endif
#define SYSLOG(level, fmt, args...)

//just a attribute without rw
#define SYSFS_ATTR_DEF(field) \
    static struct kobj_attribute field = __ATTR(field, S_IRUGO, NULL, NULL);

//read write sysfs
#define SYSFS_RW_ATTR_DEF(field, _read, _write) \
    static struct kobj_attribute field = __ATTR(field, S_IRUGO|S_IWUSR, _read, _write);

//read only sysfs
#define SYSFS_RO_ATTR_DEF(field, _read) \
    static struct kobj_attribute field = __ATTR(field, S_IRUGO, _read, NULL);

//just a attribute without rw
#define SYSFS_PREFIX_ATTR_DEF(prefix, _field) \
    static struct kobj_attribute prefix##_##_field = __ATTR(_field, S_IRUGO, NULL, NULL);

//read write sysfs
#define SYSFS_PREFIX_RW_ATTR_DEF(prefix, _field, _read, _write) \
    static struct kobj_attribute prefix##_##_field = __ATTR(_field, S_IRUGO|S_IWUSR, _read, _write);

//read only sysfs
#define SYSFS_PREFIX_RO_ATTR_DEF(prefix, _field, _read) \
    static struct kobj_attribute prefix##_##_field = __ATTR(_field, S_IRUGO, _read, NULL);

//write only sysfs
#define SYSFS_PREFIX_WO_ATTR_DEF(prefix, _field, _write) \
    static struct kobj_attribute prefix##_##_field = __ATTR(_field, S_IWUSR, NULL, _write);

//create attribute node, goto label 'exit' if failed.
#define CHECK_CREATE_SYSFS_FILE(kobj_name, attribute, result) \
    (result) = sysfs_create_file((kobj_name), &((attribute).attr));\
    if (ERROR_SUCCESS != result) \
    {DBG_ECHO(DEBUG_ERR, "sysfs create attribute %s failed!", (&((attribute).attr))->name); \
    goto exit;}

#if 0
#define BSPMODULE_DEBUG_ATTR_DEF(_name, _nodeindex) \
    static ssize_t BSPMODULE_DEBUG_SYSFS_READ_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, char *buf) { \
        return bsp_sysfs_debug_access_get(kobjs, ka, buf, _nodeindex); \
    } \
    SYSFS_PREFIX_RO_ATTR_DEF(bspmodule, _name, BSPMODULE_DEBUG_SYSFS_READ_##_nodeindex)
#else
#define BSPMODULE_DEBUG_ATTR_DEF(_name, _nodeindex) \
    static ssize_t BSPMODULE_DEBUG_SYSFS_READ_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, char *buf) { \
        return bsp_sysfs_debug_access_get(kobjs, ka, buf, _nodeindex); \
    } \
    static ssize_t BSPMODULE_DEBUG_SYSFS_WRITE_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, const char *buf, size_t count) { \
        return bsp_sysfs_debug_access_set(kobjs, ka, buf, count, _nodeindex); \
    } \
    SYSFS_PREFIX_RW_ATTR_DEF(bspmodule, _name, BSPMODULE_DEBUG_SYSFS_READ_##_nodeindex, BSPMODULE_DEBUG_SYSFS_WRITE_##_nodeindex)

#endif

#define BSPMODULE_DEBUG_RW_ATTR_DEF(_name, _nodeindex) \
    static ssize_t BSPMODULE_DEBUG_LEVEL_SYSFS_READ_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, char *buf) { \
        return bsp_sysfs_debug_loglevel_get(kobjs, ka, buf, _nodeindex); \
    } \
    static ssize_t BSPMODULE_DEBUG_LEVEL_SYSFS_WRITE_##_nodeindex(struct kobject *kobjs, struct kobj_attribute *ka, const char *buf, size_t count) { \
        return bsp_sysfs_debug_loglevel_set(kobjs, ka, buf, count, _nodeindex); \
    } \
    SYSFS_PREFIX_RW_ATTR_DEF(bspmodule, _name, BSPMODULE_DEBUG_LEVEL_SYSFS_READ_##_nodeindex, BSPMODULE_DEBUG_LEVEL_SYSFS_WRITE_##_nodeindex)

#define CHECK_IF_ERROR_GOTO_EXIT(ret, fmt, args...) \
    do {\
        if((ret) != ERROR_SUCCESS) {\
            char * ___temp_str = strrchr(__FILE__, '/');\
            char ___temp_buf[LOG_STRING_LEN] = {0};\
            int ___s = 0;\
            ___temp_str = (___temp_str == NULL) ? __FILE__ : ___temp_str + 1;\
            ___s = scnprintf(___temp_buf, LOG_STRING_LEN, "BSP %s: "fmt, MODULE_NAME, ##args);\
            if (bsp_h3c_localmsg_to_file(___temp_buf, (long)___s, DEBUG_ERR, ___temp_str, __LINE__, BSP_LOG_FILE) != ERROR_SUCCESS)\
                {printk(KERN_ERR"BSP %s: "fmt, MODULE_NAME, ##args);\
                printk("\n");}\
            goto exit;}\
        } while(0);

#define CHECK_IF_NULL_GOTO_EXIT(err, ret, value, fmt, args...) \
        do {\
            if((value) == NULL) {\
                char * ___temp_str = strrchr(__FILE__, '/');\
                char ___temp_buf[LOG_STRING_LEN] = {0};\
                int ___s = 0;\
                ___temp_str = (___temp_str == NULL) ? __FILE__ : ___temp_str + 1;\
                ___s = scnprintf(___temp_buf, LOG_STRING_LEN, "BSP %s: "fmt, MODULE_NAME, ##args);\
                if (bsp_h3c_localmsg_to_file(___temp_buf, (long)___s, DEBUG_ERR, ___temp_str, __LINE__, BSP_LOG_FILE) != ERROR_SUCCESS)\
                    {printk(KERN_ERR"BSP %s: "fmt, MODULE_NAME, ##args);\
                    printk("\n");}\
                ret = err;\
                goto exit;}\
            } while(0);

#define CHECK_IF_ZERO_GOTO_EXIT(err, ret, value, fmt, args...) \
            do {\
                if((value) == 0) {\
                    char * ___temp_str = strrchr(__FILE__, '/');\
                    char ___temp_buf[LOG_STRING_LEN] = {0};\
                    int ___s = 0;\
                    ___temp_str = (___temp_str == NULL) ? __FILE__ : ___temp_str + 1;\
                    ___s = scnprintf(___temp_buf, LOG_STRING_LEN, "BSP %s: "fmt, MODULE_NAME, ##args);\
                    if (bsp_h3c_localmsg_to_file(___temp_buf, (long)___s, DEBUG_ERR, ___temp_str, __LINE__, BSP_LOG_FILE) != ERROR_SUCCESS)\
                        {printk(KERN_ERR"BSP %s: "fmt, MODULE_NAME, ##args);\
                        printk("\n");}\
                    ret = err;\
                    goto exit;}\
                } while(0);


#define CHECK_IF_ERROR(ret, fmt, args...) \
                    do {\
                        if((ret) != ERROR_SUCCESS) {\
                            char * ___temp_str = strrchr(__FILE__, '/');\
                            char ___temp_buf[LOG_STRING_LEN] = {0};\
                            int ___s = 0;\
                            ___temp_str = (___temp_str == NULL) ? __FILE__ : ___temp_str + 1;\
                            ___s = scnprintf(___temp_buf, LOG_STRING_LEN, "BSP %s: "fmt, MODULE_NAME, ##args);\
                            if (bsp_h3c_localmsg_to_file(___temp_buf, (long)___s, DEBUG_ERR, ___temp_str, __LINE__, BSP_LOG_FILE) != ERROR_SUCCESS)\
                                {printk(KERN_ERR"BSP %s: "fmt, MODULE_NAME, ##args);\
                                printk("\n");}\
                                }\
                        } while(0);

