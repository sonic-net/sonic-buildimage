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
//#include <linux/kobject.h>
//#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/file.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/kernel.h>
/*私有文件*/
#include "i2c_dev_reg.h"
#include "pub.h"
#include "bsp_base.h"

#define MODULE_NAME "bsp_base"
#define MAX_SMBUS_NUM                 5      //初始化时probe的smbus数量
#define EEPROM_WRITE_MAX_DELAY_MS     10     //E2PROM写操作一个字节delay时间
#define I2C_RW_MAX_RETRY_COUNT        3
#define I2C_RW_MAX_RETRY_DELAY_MS     5
#define COMMON_EEPROM_BLOCK_READ      1      //eeprom读多个数据时，按块读。还是一个个读
#define SFP_EEPROM_BLOCK_READ         COMMON_EEPROM_BLOCK_READ       //读eeprom时采用按块读
#define DEFAULT_CPLD_BOARD_TYPE_OFFSET    0x2202       //默认的板卡CPLD偏移地址
#define DEFAULT_SUBCARD_BOARD_TYPE_OFFSET 0x2
#define DEFAULT_CPLD_BASE_ADDR_IOREMAP    0xdffe0000   //使用IOREMAP方式访问时，默认的基址
#define DIRECT_IO_SUPPORT                 0  //使用inb/outb访问cpld

/******************struct*************************/

/*使用内存方式时才使用*/
#define DBG_ECHO(level, fmt, args...) DEBUG_PRINT(bsp_module_debug_level[BSP_BASE_MODULE], level, BSP_LOG_FILE, fmt, ##args)
#define DBG_ECHO_I2C(level, fmt, args...) DEBUG_PRINT(bsp_module_debug_level[BSP_BASE_MODULE], level, BSP_I2C_LOG_FILE, fmt, ##args)

/****************data**********************/
struct mutex bsp_i2c_path_lock;
struct mutex bsp_cpld_lock;
struct mutex bsp_slot_cpld_lock[MAX_SLOT_NUM];
struct mutex bsp_logfile_lock;
struct mutex bsp_psu_logfile_lock;
struct mutex bsp_i2c_logfile_lock;
struct mutex bsp_wdt_logfile_lock;

struct mutex bsp_recent_log_lock;
struct mutex bsp_fan_speed_lock;
struct mutex bsp_mac_inner_temp_lock;
struct mutex bsp_mac_width_temp_lock;
struct mutex bsp_manu_lock;

struct mutex psu_table_lock[MAX_PSU_NUM];
struct mutex fan_table_lock[MAX_FAN_NUM];
struct mutex syseeprom_table_lock[1];

char *curr_h3c_log_file = LOG_FILE_PATH_0;
char *curr_h3c_psu_log_file = LOG_FILE_PATH_2;
char *curr_h3c_i2c_log_file = LOG_FILE_I2C_PATH1;
char *curr_h3c_wdt_log_file = LOG_FILE_WDT_PATH1;

struct i2c_adapter *smbus = NULL;     //使用的i2c smbus

static int bsp_product_type = PDT_TYPE_BUTT;
volatile void *g_u64Cpld_addr = NULL;
volatile void *g_u64CpuCpld_addr = NULL;
volatile void *g_u64SlotCpld_addr[MAX_SLOT_NUM] = {0};
volatile void *g_u64BiosCpld_addr = NULL;

//板卡形态相关的静态数据.无子卡设备是整个设备数据。有子卡设备表示主板数据
board_static_data main_board_data = {0};

//子卡形态相关静态数据，简化设计使用全局变量，不使用动态分配
board_static_data sub_board_data[MAX_SLOT_NUM];

struct i2c_diag_records i2c_diag_info = {0};
int current_i2c_path_id = 0;

struct bsp_log_filter bsp_recent_log;
bool log_to_private_file = TRUE;
bool log_filter_to_dmesg = TRUE;
int bsp_dmesg_log_level = DEBUG_DBG | DEBUG_ERR | DEBUG_INFO;
int bsp_module_debug_level[BSP_MODULE_MAX] = {DEBUG_ERR, DEBUG_ERR, DEBUG_ERR, DEBUG_ERR, DEBUG_ERR, DEBUG_ERR, DEBUG_ERR, DEBUG_ERR, DEBUG_ERR};

u8 *p_dom_remap = NULL;
extern int g_i2crst_time;//us
extern int g_i2c_select_time;//ms
extern int g_psuretry_count;
EXPORT_SYMBOL(p_dom_remap);


inline board_static_data *bsp_get_board_data(void)
{
    return &main_board_data;
}

board_static_data *bsp_get_slot_data(int slot_index)
{
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_get_slot_data get bd failed");
        return NULL;
    }

    if (slot_index == MAIN_BOARD)
    {
        return bd;
    }
    else if ((slot_index > MAIN_BOARD) && (slot_index <= bd->slot_num) && (slot_index < MAX_SLOT_NUM))
    {
        return bd->sub_slot_info[slot_index];
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "get_slot_data of slot index %d (0~%d expected) is invalid!", slot_index, (int)bd->slot_num);
        return NULL;
    }
}

#define CPLD_REG_ASSIGN(func, addr, mask, offset) \
    bd->cpld_addr_##func = addr;\
    bd->cpld_mask_##func = mask;\
    bd->cpld_offs_##func = offset;

#define CPLD_TRANS_VALUE_WITH_MASK_AND_OFFSET(value, func) \
    (value) = (((value) & bd->cpld_mask_##func) >> bd->cpld_offs_##func)

#define STEP(type, addr, value)   (type),(addr),(value)
#define STEP_OVER                 (OP_TYPE_NONE),0,0
#define STEP_CNT(n)               (n)

int i2c_select_steps_init(i2c_select_operation_steps *i2c_steps, int step_count, ...)
{
    int i;
    int temp_value;
    int ret = 0;
    va_list steps;
    va_start(steps, step_count);
    i2c_steps->valid = 0;
    if (step_count > MAX_I2C_SEL_OP_STEPS_COUNT)
    {
        DBG_ECHO(DEBUG_ERR, "param err: i2c select steps %d (> %d).", step_count,  MAX_I2C_SEL_OP_STEPS_COUNT);
        ret = -EINVAL;
        goto exit;
    }
    for (i = 0; i < step_count; i++)
    {
        temp_value = va_arg(steps, int);
        i2c_steps->step[i].op_type = temp_value;
        if ((u32)temp_value >= OP_TYPE_BUTT)
        {
            DBG_ECHO(DEBUG_ERR, "i2c table init op_type %d error", temp_value);
            ret = -EINVAL;
            goto exit;
        }
        temp_value = va_arg(steps, int);
        if (i2c_steps->step[i].op_type == OP_TYPE_WR_CPLD)
        {
            i2c_steps->step[i].cpld_offset_addr = (u16)temp_value;
        }
        else
        {
            i2c_steps->step[i].i2c_dev_addr = (u16)temp_value;
        }
        i2c_steps->step[i].op_value = (u8)(va_arg(steps, int));
    }

    i2c_steps->valid = I2C_SELECT_STEPS_VALID;

exit:
    va_end(steps);
    return ret;
}

//打印i2c选通表的内容
size_t bsp_print_i2c_select_table(char *buf)
{

    int i = 0;
    int j = 0;
    int len = 0;
    int per_page = 30;
    static int last_from_index = 0;
    int printed = 0;
    int start_from = last_from_index;

    //board_static_data * bd = &main_board_data;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_print_i2c_select_table get bd failed");
        return -EINVAL;
    }

    for (i = last_from_index; i < I2C_DEV_BUTT; i++)
    {
        if (bd->i2c_select_table[i].valid == I2C_SELECT_STEPS_VALID)
        {
            len += scnprintf(buf + len, PAGE_SIZE - len, "i2c idx %3d:", i);
            for (j = 0; j < MAX_I2C_SEL_OP_STEPS_COUNT; j++)
            {
                if (bd->i2c_select_table[i].step[j].op_type != OP_TYPE_NONE)
                    len += scnprintf(buf + len, PAGE_SIZE - len, "{t:%d a:0x%02x v:0x%02x}->", bd->i2c_select_table[i].step[j].op_type,  bd->i2c_select_table[i].step[j].i2c_dev_addr, bd->i2c_select_table[i].step[j].op_value);
            }
            len += scnprintf(buf + len, PAGE_SIZE - len, "None\n");
            if (++printed >= per_page)
            {
                last_from_index = i + 1;
                break;
            }
        }
    }
    if (printed < per_page)
    {
        last_from_index = 0;
    }
    len += scnprintf(buf + len, PAGE_SIZE - len, "\n*t:op_type a:op_address v:value\n");
    len += scnprintf(buf + len, PAGE_SIZE - len, "op_type %d:%s\n", OP_TYPE_WR_9545, __stringify(OP_TYPE_WR_9545));
    len += scnprintf(buf + len, PAGE_SIZE - len, "op_type %d:%s\n", OP_TYPE_WR_9548, __stringify(OP_TYPE_WR_9548));
    len += scnprintf(buf + len, PAGE_SIZE - len, "op_type %d:%s\n", OP_TYPE_WR_CPLD, __stringify(OP_TYPE_WR_CPLD));
    len += scnprintf(buf + len, PAGE_SIZE - len, "%d Bytes\nshow %d~%d\n ", len, start_from, last_from_index);

    return len;
}

/*
    d11168: PDT_TYPE_LS_9855_48CD8D_W1
*/
int board_static_data_init_LS_9855_48CD8D_W1(board_static_data *board_data)
{
    int ret = ERROR_SUCCESS;
    int i = 0;
    board_static_data *bd = board_data;
    u8 dom_remap[DOM_PORT_MAX_NUM] =
    {
        0, 1, 2,  3, 4, 5, 6, 7,        /* 00-07 */
        8, 9, 10, 11, 12, 13, 14, 15,   /* 08-15 */
        16, 17, 18, 19, 20, 21, 22, 23, /* 16-23 */
        24, 25, 26, 27, 28, 29, 30, 31, /* 24-31 */
        32, 33, 34, 35, 36, 37, 38, 39, /* 32-39 */
        40, 41, 42, 43, 44, 45, 46, 47, /* 40-47 */
        48, 49, 52, 53, 56, 57, 60, 61  /* 48-55 */
    };

    second_power_item_s ft_second_power1_item[] =
    {
        {3300000, 3400000, 3150000, "VP1", 0},
        {1800000, 1890000, 1710000, "VP2", 1},
        {2500000, 2700000, 1300000, "VP3", 2},
        {1800000, 1890000, 1710000, "VP4", 3},
        {12000000, 13200000, 10800000, "VH", 4},
        {1200000, 1260000, 1140000, "VX1", 5},
        {1150000, 1210000, 1090000, "VX2", 6},
        {1000000, 1050000, 950000, "VX3", 7},
    };

    second_power_item_s ft_second_power2_item[] =
    {
        {3300000, 3400000, 3150000, "VP1", 0},
        {3300000, 3465000, 3200000, "VP2", 1},
        {1200000, 1250000, 1160000, "VP3", 2},
        {1200000, 1250000, 1160000, "VP4", 3},
        {12000000, 13200000, 10800000, "VH", 4},
        {1000000, 1050000, 950000, "VX1", 5},
        {750000, 800000, 720000, "VX2", 6},
        {800000, 950000, 700000, "VX3", 7},
    };

    //无关数据全写0
    memset(bd, 0, sizeof(board_static_data));

    bd->slot_index = MAIN_BOARD;
    bd->product_type = PDT_TYPE_LS_9855_48CD8D_W1;
    bd->mainboard = NULL;

    bd->cpld_num        = 5;        //5 个cpld
    bd->fan_num         = 6;        //风扇个数
    bd->motors_per_fan    = 2;        //每风扇2个马达
    bd->fan_speed_coef    = 11718750; //风扇转速转换系数, 9820是 30000000
    bd->fan_max_speed    = 20000;
    bd->fan_delta_max_speed[0] = 27000;
    bd->fan_delta_max_speed[1] = 23000;
    bd->fan_foxconn_max_speed[0] = 28500;
    bd->fan_foxconn_max_speed[1] = 29000;
    bd->fan_default_max_speed[0] = 23000;
    bd->fan_default_max_speed[1] = 27000;
    bd->fan_min_speed    = 5500;
    bd->fan_min_speed_pwm = 20;
    bd->fan_max_speed_pwm = 100;    //最大转速时对应的pwm值
    bd->fan_min_pwm_speed_percentage = 20;
    bd->fan_max_pwm_speed_percentage = 100;
    bd->fan_temp_low    = 30;
    bd->fan_temp_high    = 70;
    bd->cpld_fan_good_flag = 0x3;

    bd->isl68127_num = 1;
    bd->tps53622_num = 1;
    bd->tps53659_num = 1;

    bd->adm1166_num     = 2;
    bd->psu_num         = 2;        //电源个数
    bd->psu_num_temp_sensors = 1;
    /*1600W power supply unit*/
    bd->psu_type        = PSU_TYPE_1600W_TD4;
    bd->slot_num        = 0;        //子卡个数, 无子卡写0
    bd->smbus_use_index = 0;        //使用的smbus的索引
    bd->lm75_num        = 0;
    /*2 6696, 1 on control board and 1 on port board */
    bd->max6696_num     = 3;        //max6696数量 增加u68
    bd->optic_modlue_num = 56;        //可插光模块数量
    bd->eeprom_used_size = 512;        //简化设计eeprom只使用前边512字节, 不够再扩展

    bd->mac_rov_min_voltage     = 698;
    bd->mac_rov_max_voltage     = 927;
    bd->mac_rov_default_voltage = 800;

    bd->fan_target_speed_coef0[0] = 6524500;          //front fan 4 coeffcient of the polynomial
    bd->fan_target_speed_coef1[0] = -112170;
    bd->fan_target_speed_coef2[0] = 4567;
    bd->fan_target_speed_coef3[0] = -22;

    bd->fan_target_speed_coef0[1] = 5400100;          //rear fan
    bd->fan_target_speed_coef1[1] = -57783;
    bd->fan_target_speed_coef2[1] = 3296;
    bd->fan_target_speed_coef3[1] = -15;
    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->cage_type[i]  = CAGE_TYPE_QSFP;
        bd->port_speed[i] = i < 48 ? SPEED_100G : SPEED_400G;
    }

    bd->i2c_addr_isl68127[0] = 0x5c;
    bd->isl68127_describe[0] = "VR outlet to Mac chip";

    bd->i2c_addr_tps53622 = 0x66;
    bd->i2c_addr_tps53659 = 0x61;
    /*2 eeproms , 1 for cpu and the other one shared in between cpu and bmc*/
    bd->i2c_addr_eeprom    = 0x50;    //板卡eerom的i2c地址
    /*2 6696 ,Description TODO*/
    /* identify: 主控板6696 */
    bd->i2c_addr_max6696[0] = 0x18;   //max6696 i2c地址
    bd->max6696_describe[0][0] = "Switch Air Outlet";
    bd->max6696_describe[0][1] = "Switch Air Inlet";
    bd->max6696_describe[0][2] = "Switch PCB Near NS FPGA";


    // identify: 端口板第一片6696
    bd->i2c_addr_max6696[1] = 0x18;
    bd->max6696_describe[1][0] = "Switch PCB Near MAC";
    bd->max6696_describe[1][1] = "Switch PWR2 Inlet";
    bd->max6696_describe[1][2] = "Switch PCB Near Port49";


    // identify: 端口板第二片6696
    bd->i2c_addr_max6696[2] = 0x18;
    bd->max6696_describe[2][0] = "Switch PCB U68-0";
    bd->max6696_describe[2][1] = "Switch MAC U68-1";
    bd->max6696_describe[2][2] = "Switch MAC U68-2";

    /*  no 9548 or 9545 but using i2c fpga to access front port i2c
        bd->i2c_addr_pca9548    = 0x70;        //这里假设所有9545/9548使用的i2c地址都一样
        bd->i2c_addr_pca9548_2    = 0x77;
    */

    bd->i2c_addr_psu[0] = 0x50;
    bd->i2c_addr_psu[1] = 0x50;
    bd->i2c_addr_psu_pmbus[0] = 0x58;     //与电源i2c地址配对, +0x08
    bd->i2c_addr_psu_pmbus[1] = 0x58;

    bd->i2c_addr_ina219[0] = 0x44;
    bd->i2c_addr_ina219[1] = 0x44;

    bd->i2c_addr_adm1166[0]  = 0x34;
    bd->i2c_addr_adm1166[1]  = 0x34;
    bd->i2c_addr_n287  = 0x6d;
    bd->i2c_addr_rtc_4337  = 0x68;

    bd->i2c_addr_fan[0] = 0x50;
    bd->i2c_addr_fan[1] = 0x50;
    bd->i2c_addr_fan[2] = 0x50;
    bd->i2c_addr_fan[3] = 0x50;
    bd->i2c_addr_fan[4] = 0x50;
    bd->i2c_addr_fan[5] = 0x50;

    //光模块i2c地址初始化
    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->i2c_addr_optic_eeprom[i] = 0x50;
        bd->i2c_addr_optic_eeprom_dom[i] = (bd->cage_type[i] == CAGE_TYPE_SFP) ? 0x51 : 0x50;
    }

    bd->cpld_access_type   = IO_REMAP;
    bd->cpld_base_address  = 0xdffe0000;    //由OM提供的地址
    bd->cpld_hw_addr_board = 0x2200;        //由硬件设计提供的地址
    bd->cpld_size_board    = 256;           //按字节数
    bd->cpld_hw_addr_cpu   = 0x2000;        //CPU扣板CPLD
    bd->cpld_size_cpu      = 512;           //CPU扣板CPLD
    bd->cpld_hw_addr_bios  = 0x0000;        //bios cpld
    bd->cpld_size_bios     = 512;

    CPLD_REG_ASSIGN(pcb_type, 0x02, 0xff, 0);

    /*board version*/
    CPLD_REG_ASSIGN(pcb_ver[0], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[1], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[2], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[3], 0x09, 0x78, 3);
    CPLD_REG_ASSIGN(pcb_ver[4], 0x09, 0x78, 3);

    /*HW_VERSION*/
    CPLD_REG_ASSIGN(cpld_ver[0], 0x02, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[1], 0x01, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[2], 0x02, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[3], 0x03, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[4], 0x40, 0x0f, 0);

    bd->cpld_type_describe[0] = "LCMXO3LF-6900C-6BG400CAM1 ";
    bd->cpld_type_describe[1] = "LCMXO3LF-6900C-5BG400CAM1 ";
    bd->cpld_type_describe[2] = "LCMXO3LF-6900C-5BG400CAM1 ";
    bd->cpld_type_describe[3] = "LCMXO3LF-6900C-5BG400CAM1 ";
    bd->cpld_type_describe[4] = "LCMXO3LF-6900C-5BG400CAM1 ";
    bd->cpld_type_describe[5] = "Xlinx_Memory_Controller";

    /*  bd->cpld_location_describe[0] = "1st-JTAG-Chain";
        bd->cpld_location_describe[1] = "2nd-JTAG-Chain";
        bd->cpld_location_describe[2] = "3rd-JTAG-Chain";
        bd->cpld_location_describe[3] = "4th-JTAG-Chain";*/

    bd->cpld_location_describe[0] = "cpu_cpld";
    bd->cpld_location_describe[1] = "board_cpld_0";
    bd->cpld_location_describe[2] = "board_cpld_1";
    bd->cpld_location_describe[3] = "board_cpld_2";
    bd->cpld_location_describe[4] = "board_cpld_3";
    bd->cpld_location_describe[5] = "dom_fpga";

    bd->dom_exist = 1;
    bd->mac_rov_device = I2C_DEV_ISL68127;

    memcpy((bd->dom_remap), dom_remap, DOM_PORT_MAX_NUM);
    p_dom_remap = bd->dom_remap;
    memcpy((bd->ft_second_power1_item), ft_second_power1_item, sizeof(ft_second_power1_item));
    memcpy((bd->ft_second_power2_item), ft_second_power2_item, sizeof(ft_second_power2_item));
    CPLD_REG_ASSIGN(max6696_rst[0],  0x15, 0x10, 4);
    CPLD_REG_ASSIGN(max6696_rst[1],  0x15, 0x40, 6);
    CPLD_REG_ASSIGN(max6696_rst[2],  0x15, 0x80, 7);

    /*
                eeprom write protect register
                EN_WP_reg[3]: Netstream FPGA模块eeprom写保护
                EN_WP_reg[2]: CPU和BMC共享eeprom写保护
                EN_WP_reg[1]: BMC独享eeprom写保护
                EN_WP_reg[0]: CPU独享eeprom写保护

    */
    CPLD_REG_ASSIGN(eeprom_write_protect, 0x55, 0x01, 0); /*默认写保护*/

    //mac 初始完成可点端口灯
    CPLD_REG_ASSIGN(mac_init_ok, 0x0b, 0x01, 0);

    //mac核心电压设置
    CPLD_REG_ASSIGN(mac_rov,     0x3d, 0xff, 0);

    //面板上的系统指示灯
    /*
        CPLD_REG_ASSIGN(pannel_sys_led_green, 0x6b, 0x10, 4);
        CPLD_REG_ASSIGN(pannel_sys_led_red,   0x6b, 0x20, 5);
    */
    CPLD_REG_ASSIGN(pannel_sys_led_ctrl,  0x6c, 0x0f, 0);
    CPLD_REG_ASSIGN(pannel_psu_led_green, 0x6b, 0x40, 6);
    CPLD_REG_ASSIGN(pannel_psu_led_red,   0x6b, 0x80, 7);
    CPLD_REG_ASSIGN(pannel_fan_led_green, 0x6b, 0x04, 2);
    CPLD_REG_ASSIGN(pannel_fan_led_red,   0x6b, 0x08, 3);
    //CPLD_REG_ASSIGN(pannel_bmc_led_green, 0x6a, 0x02, 1);
    //CPLD_REG_ASSIGN(pannel_bmc_led_red,   0x6a, 0x04, 2);
    //CPLD_REG_ASSIGN(pannel_id_led_blue,   0x6a, 0x01, 0);

    bd->cpld_tx_dis_num = 6;
    CPLD_REG_ASSIGN(cpld_tx_dis[0], 0x95, 0xff, 0);
    CPLD_REG_ASSIGN(cpld_tx_dis[1], 0x96, 0xff, 0);
    CPLD_REG_ASSIGN(cpld_tx_dis[2], 0x97, 0xff, 0);
    CPLD_REG_ASSIGN(cpld_tx_dis[3], 0x98, 0xff, 0);
    CPLD_REG_ASSIGN(cpld_tx_dis[4], 0x99, 0xff, 0);
    CPLD_REG_ASSIGN(cpld_tx_dis[5], 0x9a, 0xff, 0);

    //cpld setting for sysled led color
    /***
    0000:4Hz绿灯闪
    0001:2Hz绿灯闪
    0010:1Hz绿灯闪
    0011:0.5Hz绿灯闪
    0100:绿灯常亮
    0101:4Hz红灯闪
    0110:2Hz红灯闪
    0111:1Hz红灯闪
    1000:0.5Hz红灯闪
    1001:红灯常亮
    1010:4Hz黄灯闪
    1011:2Hz黄灯闪
    1100:1Hz黄灯闪
    1101:0.5Hz黄灯闪
    1110:黄灯常亮
    1111:灯灭
    **/
    bd->cpld_value_sys_led_code_green  = 0xf4;
    bd->cpld_value_sys_led_code_red    = 0xf9;
    bd->cpld_value_sys_led_code_yellow = 0xfe;
    bd->cpld_value_sys_led_code_dark   = 0xff;
    bd->cpld_value_sys_led_code_green_flash = 0xf0;
    bd->cpld_value_sys_led_code_red_flash = 0xf5;
    bd->cpld_value_sys_led_code_yellow_flash = 0xfa;

    /* reboot cause register on cpu cpld: 0x20表示没有进行clr_reset操作前本设备所有曾经发生过的复位原因; 0x38为上次重启原因*/
#if 0
    CPLD_REG_ASSIGN(reset_type_cpu_thermal,    0x20, 0x80, 7);
    CPLD_REG_ASSIGN(reset_type_cold,           0x20, 0x40, 6);
    CPLD_REG_ASSIGN(reset_type_power_en,       0x20, 0x10, 4);
    CPLD_REG_ASSIGN(reset_type_boot_sw,        0x20, 0x08, 3);
    CPLD_REG_ASSIGN(reset_type_soft,           0x20, 0x04, 2);
    CPLD_REG_ASSIGN(reset_type_wdt,            0x20, 0x02, 1);
    CPLD_REG_ASSIGN(reset_type_mlb,            0x20, 0x01, 0);
#endif
    CPLD_REG_ASSIGN(reset_type,           0x20, 0xff, 0);
    CPLD_REG_ASSIGN(last_reset_type,      0x38, 0xff, 0);
    /*0x21寄存器清除的是0x20的内容*/
    CPLD_REG_ASSIGN(clr_rst,     0x21, 0x02, 1);

    //watchdog cpld 相关数据
    CPLD_REG_ASSIGN(wd_feed,      0x30, 0xff, 0);
    CPLD_REG_ASSIGN(wd_disfeed,   0x31, 0xff, 0);
    CPLD_REG_ASSIGN(wd_timeout,   0x32, 0xff, 0);
    CPLD_REG_ASSIGN(wd_enable,      0x33, 0x01, 0);

    //电源相关寄存器
    CPLD_REG_ASSIGN(psu_absent[0], 0x35, 0x01, 0);
    CPLD_REG_ASSIGN(psu_absent[1], 0x35, 0x02, 1);

    CPLD_REG_ASSIGN(psu_good[0],  0x34, 0x01, 0);
    CPLD_REG_ASSIGN(psu_good[1],  0x34, 0x02, 1);

    //这里开始是寄存器定义
    //风扇相关寄存器定义

    CPLD_REG_ASSIGN(fan_num,      0x70, 0x0f, 0);
    CPLD_REG_ASSIGN(fan_select,   0x70, 0xf0, 4);
    CPLD_REG_ASSIGN(fan_pwm,      0x71, 0xff, 0);

    /*2 bits for each fan*/
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_LOW_REG_INDEX],  0x72, 0xff, 0);
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_HIGH_REG_INDEX], 0x73, 0xff, 0);

    /*reg[5:0]对应6个风扇，写0关闭，写1打开*/
    CPLD_REG_ASSIGN(fan_eeprom_write_protect, 0x7c, 0x00, 0);

    /*   0:power2port
        1:port2power*/

    CPLD_REG_ASSIGN(fan_direction[5], 0x74, 0x20, 5);
    CPLD_REG_ASSIGN(fan_direction[4], 0x74, 0x10, 4);
    CPLD_REG_ASSIGN(fan_direction[3], 0x74, 0x08, 3);
    CPLD_REG_ASSIGN(fan_direction[2], 0x74, 0x04, 2);
    CPLD_REG_ASSIGN(fan_direction[1], 0x74, 0x02, 1);
    CPLD_REG_ASSIGN(fan_direction[0], 0x74, 0x01, 0);

    CPLD_REG_ASSIGN(fan_enable[5], 0x75, 0x20, 5);
    CPLD_REG_ASSIGN(fan_enable[4], 0x75, 0x10, 4);
    CPLD_REG_ASSIGN(fan_enable[3], 0x75, 0x08, 3);
    CPLD_REG_ASSIGN(fan_enable[2], 0x75, 0x04, 2);
    CPLD_REG_ASSIGN(fan_enable[1], 0x75, 0x02, 1);
    CPLD_REG_ASSIGN(fan_enable[0], 0x75, 0x01, 0);

    CPLD_REG_ASSIGN(fan_led_red[5], 0x76, 0x20, 5);
    CPLD_REG_ASSIGN(fan_led_red[4], 0x76, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_red[3], 0x76, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_red[2], 0x76, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_red[1], 0x76, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_red[0], 0x76, 0x01, 0);

    CPLD_REG_ASSIGN(fan_absent[5], 0x77, 0x20, 5);
    CPLD_REG_ASSIGN(fan_absent[4], 0x77, 0x10, 4);
    CPLD_REG_ASSIGN(fan_absent[3], 0x77, 0x08, 3);
    CPLD_REG_ASSIGN(fan_absent[2], 0x77, 0x04, 2);
    CPLD_REG_ASSIGN(fan_absent[1], 0x77, 0x02, 1);
    CPLD_REG_ASSIGN(fan_absent[0], 0x77, 0x01, 0);

    CPLD_REG_ASSIGN(fan_led_green[5], 0x7d, 0x20, 5);
    CPLD_REG_ASSIGN(fan_led_green[4], 0x7d, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_green[3], 0x7d, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_green[2], 0x7d, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_green[1], 0x7d, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_green[0], 0x7d, 0x01, 0);

    CPLD_REG_ASSIGN(fan_status[5], 0x79, 0x30, 4);
    CPLD_REG_ASSIGN(fan_status[4], 0x79, 0x0c, 2);
    CPLD_REG_ASSIGN(fan_status[3], 0x79, 0x03, 0);
    CPLD_REG_ASSIGN(fan_status[2], 0x78, 0x30, 4);
    CPLD_REG_ASSIGN(fan_status[1], 0x78, 0x0c, 2);
    CPLD_REG_ASSIGN(fan_status[0], 0x78, 0x03, 0);

    //光模块控制相关寄存器 , 按端口索引排续
    CPLD_REG_ASSIGN(cage_power_on[0], 0xE8, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[1], 0xE8, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[2], 0xE8, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[3], 0xE8, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[4], 0xE8, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[5], 0xE8, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[6], 0xE8, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[7], 0xE8, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[8], 0xE9, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[9], 0xE9, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[10], 0xE9, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[11], 0xE9, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[12], 0xE9, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[13], 0xE9, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[14], 0xE9, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[15], 0xE9, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[16], 0xEA, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[17], 0xEA, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[18], 0xEA, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[19], 0xEA, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[20], 0xEA, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[21], 0xEA, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[22], 0xEA, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[23], 0xEA, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[24], 0xEB, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[25], 0xEB, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[26], 0xEB, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[27], 0xEB, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[28], 0xEB, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[29], 0xEB, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[30], 0xEB, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[31], 0xEB, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[32], 0xEC, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[33], 0xEC, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[34], 0xEC, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[35], 0xEC, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[36], 0xEC, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[37], 0xEC, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[38], 0xEC, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[39], 0xEC, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[40], 0xED, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[41], 0xED, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[42], 0xED, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[43], 0xED, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[44], 0xED, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[45], 0xED, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[46], 0xED, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[47], 0xED, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[48], 0xEE, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[49], 0xEE, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[50], 0xEE, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[51], 0xEE, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[52], 0xEE, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[53], 0xEE, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[54], 0xEE, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[55], 0xEE, 0x80, 7);

    bd->cpld_num_cages_power_on = bd->optic_modlue_num / 8;

    CPLD_REG_ASSIGN(cages_power_on[0], 0xE8, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[1], 0xE9, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[2], 0xEA, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[3], 0xEB, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[4], 0xEC, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[5], 0xED, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[6], 0xEE, 0xff, 0);

    //在位信号
    CPLD_REG_ASSIGN(qsfp_present[0], 0x89, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[1], 0x89, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[2], 0x89, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[3], 0x89, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[4], 0x89, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[5], 0x89, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[6], 0x89, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[7], 0x89, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[8], 0x8a, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[9], 0x8a, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[10], 0x8a, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[11], 0x8a, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[12], 0x8a, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[13], 0x8a, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[14], 0x8a, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[15], 0x8a, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[16], 0x8b, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[17], 0x8b, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[18], 0x8b, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[19], 0x8b, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[20], 0x8b, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[21], 0x8b, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[22], 0x8b, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[23], 0x8b, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[24], 0x8c, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[25], 0x8c, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[26], 0x8c, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[27], 0x8c, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[28], 0x8c, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[29], 0x8c, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[30], 0x8c, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[31], 0x8c, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[32], 0x8d, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[33], 0x8d, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[34], 0x8d, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[35], 0x8d, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[36], 0x8d, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[37], 0x8d, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[38], 0x8d, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[39], 0x8d, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[40], 0x8e, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[41], 0x8e, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[42], 0x8e, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[43], 0x8e, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[44], 0x8e, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[45], 0x8e, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[46], 0x8e, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[47], 0x8e, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[48], 0xb1, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[49], 0xb1, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[50], 0xb1, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[51], 0xb1, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[52], 0xb1, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[53], 0xb1, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[54], 0xb1, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[55], 0xb1, 0x80, 7);

    CPLD_REG_ASSIGN(qsfp_lpmode[0], 0x8f, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[1], 0x8f, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[2], 0x8f, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[3], 0x8f, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[4], 0x8f, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[5], 0x8f, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[6], 0x8f, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[7], 0x8f, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[8], 0x90, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[9], 0x90, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[10], 0x90, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[11], 0x90, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[12], 0x90, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[13], 0x90, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[14], 0x90, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[15], 0x90, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[16], 0x91, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[17], 0x91, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[18], 0x91, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[19], 0x91, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[20], 0x91, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[21], 0x91, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[22], 0x91, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[23], 0x91, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[24], 0x92, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[25], 0x92, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[26], 0x92, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[27], 0x92, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[28], 0x92, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[29], 0x92, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[30], 0x92, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[31], 0x92, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[32], 0x93, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[33], 0x93, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[34], 0x93, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[35], 0x93, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[36], 0x93, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[37], 0x93, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[38], 0x93, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[39], 0x93, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[40], 0x94, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[41], 0x94, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[42], 0x94, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[43], 0x94, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[44], 0x94, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[45], 0x94, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[46], 0x94, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[47], 0x94, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[48], 0xb9, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[49], 0xb9, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[50], 0xb9, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[51], 0xb9, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[52], 0xb9, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[53], 0xb9, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[54], 0xb9, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[55], 0xb9, 0x80, 7);

    CPLD_REG_ASSIGN(qsfp_reset[0], 0xa1, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[1], 0xa1, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[2], 0xa1, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[3], 0xa1, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[4], 0xa1, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[5], 0xa1, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[6], 0xa1, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[7], 0xa1, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[8], 0xa2, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[9], 0xa2, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[10], 0xa2, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[11], 0xa2, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[12], 0xa2, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[13], 0xa2, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[14], 0xa2, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[15], 0xa2, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[16], 0xa3, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[17], 0xa3, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[18], 0xa3, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[19], 0xa3, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[20], 0xa3, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[21], 0xa3, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[22], 0xa3, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[23], 0xa3, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[24], 0xa4, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[25], 0xa4, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[26], 0xa4, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[27], 0xa4, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[28], 0xa4, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[29], 0xa4, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[30], 0xa4, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[31], 0xa4, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[32], 0xa5, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[33], 0xa5, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[34], 0xa5, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[35], 0xa5, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[36], 0xa5, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[37], 0xa5, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[38], 0xa5, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[39], 0xa5, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[40], 0xa6, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[41], 0xa6, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[42], 0xa6, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[43], 0xa6, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[44], 0xa6, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[45], 0xa6, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[46], 0xa6, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[47], 0xa6, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[48], 0xc1, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[49], 0xc1, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[50], 0xc1, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[51], 0xc1, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[52], 0xc1, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[53], 0xc1, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[54], 0xc1, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[55], 0xc1, 0x80, 7);

    CPLD_REG_ASSIGN(qsfp_interrupt[0], 0x9b, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[1], 0x9b, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[2], 0x9b, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[3], 0x9b, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[4], 0x9b, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[5], 0x9b, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[6], 0x9b, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[7], 0x9b, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[8], 0x9c, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[9], 0x9c, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[10], 0x9c, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[11], 0x9c, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[12], 0x9c, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[13], 0x9c, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[14], 0x9c, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[15], 0x9c, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[16], 0x9d, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[17], 0x9d, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[18], 0x9d, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[19], 0x9d, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[20], 0x9d, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[21], 0x9d, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[22], 0x9d, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[23], 0x9d, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[24], 0x9e, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[25], 0x9e, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[26], 0x9e, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[27], 0x9e, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[28], 0x9e, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[29], 0x9e, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[30], 0x9e, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[31], 0x9e, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[32], 0x9f, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[33], 0x9f, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[34], 0x9f, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[35], 0x9f, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[36], 0x9f, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[37], 0x9f, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[38], 0x9f, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[39], 0x9f, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[40], 0xa0, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[41], 0xa0, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[42], 0xa0, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[43], 0xa0, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[44], 0xa0, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[45], 0xa0, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[46], 0xa0, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[47], 0xa0, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[48], 0xc9, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[49], 0xc9, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[50], 0xc9, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[51], 0xc9, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[52], 0xc9, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[53], 0xc9, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[54], 0xc9, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[55], 0xc9, 0x80, 7);

    //i2c选通寄存器
    CPLD_REG_ASSIGN(main_i2c_sel, 0x48, 0xff, 0);      //主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[0],   0x49, 0xff, 0);      //非主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[1],   0x4a, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[2],   0x4b, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[3],   0x4c, 0xff, 0);
    // 9548 reset寄存器 -----TCS84没有
    //重置cpu寄存器
    CPLD_REG_ASSIGN(cpu_rst, 0x15, 0x08, 3);

    //CPLD_REG_ASSIGN(i2c_wdt_ctrl, 0x32, 0x0f, 0);
    CPLD_REG_ASSIGN(cpu_init_ok,  0xb,    0x80, 7);
    //CPLD_REG_ASSIGN(i2c_wdt_feed, 0x33, 0x01, 0);
    CPLD_REG_ASSIGN(i2c_sel[4],   0x4d, 0xff, 0);
    /*
            add board i2c channel select register 0x50:
            0x050    I2C通道选择寄存器8    1    PWR_I2C_S1    PWR_I2C_S0    PWR_I2C_OE    1    MAX6696_I2C_S1    MAX6696_I2C_S0    MAX6696_I2C_OE
            MAX6696_I2C_S1    MAX6696_I2C_S0    MAX6696_I2C_OE
            6696 1 : 000
            6696 2 : 010
            PWR_I2C_S1    PWR_I2C_S0    PWR_I2C_OE (select 68127 6696)
            68127 : 000
            1166  : 010
    */
    CPLD_REG_ASSIGN(i2c_sel[5], 0x50, 0xff, 0);

    CPLD_REG_ASSIGN(gpio_i2c_1, 0x41, 0x01, 0);
    CPLD_REG_ASSIGN(gpio_i2c_0, 0x41, 0x02, 1);

    CPLD_REG_ASSIGN(bios_set,  0x03, 0x80, 7);
    CPLD_REG_ASSIGN(bios_get,  0x03, 0x08, 3);
    CPLD_REG_ASSIGN(cpudown,   0x62, 0x01, 0);
    CPLD_REG_ASSIGN(n287_reset, 0x15, 0x20, 5);

    //SSD 下电寄存器
    CPLD_REG_ASSIGN(ssd_pwr_down, 0x37, 0x01, 0);

    /*
               : 6696 select ,
                0x15 写了0xff 后续再细改；
                bd->cpld_addr_i2c_sel[5]寄存器粗暴地只关心低三位，没有顾及bit [4:6]是68127和1166相关bit，不过I2C同时只能一路访问，所以没影响，后续细改。
    */
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xa3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 1]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xa1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 2]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xa2), STEP_OVER);
    //    单比特操作不成功啊，后续找硬件调
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_EEPROM]),
                                 STEP_CNT(2),
                                 STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x80),
                                 STEP_OVER);
    /*
                I2C_DEV_CPUBMC_EEPROM, eeprom shared in between cpu and bmc
                I2C_DEV_BMC_EEPROM, reserved for bmc, no need to access on cpu side
    */
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_CPUBMC_EEPROM]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 80), STEP_OVER);
    /* ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_BMC_EEPROM]),        STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 80), STEP_OVER);
    */
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 0]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 1]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 2]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 3]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 4]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 5]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd5), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 0]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x90), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 1]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x91), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_INA219 + 0]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x92), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_INA219 + 1]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x93), STEP_OVER);

    //ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x04), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x04), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[5], 0x00), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc1),  STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_TPS53659]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_TPS53622]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ADM1166]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ADM1166 + 1]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc2), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_N287]),         STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xb3), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_RTC_4337]),  STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xa4), STEP_OVER);

    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "board static data init failed: ret=%d!", ret);
        bd->initialized = 0;
    }
    else
    {
        bd->initialized = 1;
    }
    return ret;
}
int board_static_data_init_LS_9825_64D_W1(board_static_data *board_data)
{
    int ret = ERROR_SUCCESS;
    int i = 0;
    board_static_data *bd = board_data;

    u8 dom_remap[DOM_PORT_MAX_NUM] =
    {
        32, 33,  0,  1, 34, 35,  2,  3, /* 00-07 */
        36, 37,  4,  5, 38, 39,  6,  7, /* 08-15 */
        40, 41,  8,  9, 42, 43, 10, 11, /* 16-23 */
        44, 45, 12, 13, 46, 47, 14, 15, /* 24-31 */
        48, 49, 16, 17, 50, 51, 18, 19, /* 32-39 */
        52, 53, 20, 21, 54, 55, 22, 23, /* 40-47 */
        56, 57, 24, 25, 58, 59, 26, 27, /* 48-55 */
        60, 61, 28, 29, 62, 63, 30, 31 /* 56-63 */
    };

    second_power_item_s ft_second_power1_item[] =
    {
        {3300000, 3400000, 3150000, "VP1", 0},
        {1800000, 1890000, 1710000, "VP2", 1},
        {2500000, 2700000, 1300000, "VP3", 2},
        {1800000, 1890000, 1710000, "VP4", 3},
        {12000000, 13200000, 10800000, "VH", 4},
        {1200000, 1260000, 1140000, "VX1", 5},
        {1150000, 1210000, 1090000, "VX2", 6},
        {1000000, 1050000, 950000, "VX3", 7},
    };


    second_power_item_s ft_second_power2_item[] =
    {
        {3300000, 3400000, 3150000, "VP1", 0},
        {1800000, 1890000, 1710000, "VP2", 1},
        {1200000, 1250000, 1160000, "VP3", 2},
        {1200000, 1250000, 1160000, "VP4", 3},
        {12000000, 13200000, 10800000, "VH", 4},
        {1000000, 1050000, 950000, "VX1", 5},
        {750000, 800000, 720000, "VX2", 6},
        {800000, 950000, 700000, "VX3", 7},
    };

    //无关数据全写0
    memset(bd, 0, sizeof(board_static_data));

    bd->slot_index = MAIN_BOARD;
    bd->product_type = PDT_TYPE_LS_9825_64D_W1;
    bd->mainboard = NULL;

    bd->cpld_num        = 6;        //6 个cpld
    bd->fan_num         = 6;        //风扇个数
    bd->motors_per_fan  = 2;        //每风扇2个马达

    bd->fan_speed_coef  = 30000000; //风扇转速转换系数, 9820是 30000000
    bd->fan_max_speed   = 12000;
    bd->fan_min_speed   = 1500;
    bd->fan_delta_max_speed[0] = 11000;
    bd->fan_delta_max_speed[1] = 12500;
    bd->fan_foxconn_max_speed[0] = 11200;
    bd->fan_foxconn_max_speed[1] = 12800;
    bd->fan_default_max_speed[0] = 11000;
    bd->fan_default_max_speed[1] = 12000;
    bd->fan_min_speed_pwm = 20;
    bd->fan_max_speed_pwm = 100;    //最大转速时对应的pwm值

    //后续二次确认
    bd->fan_min_pwm_speed_percentage = 20;
    bd->fan_max_pwm_speed_percentage = 100;
    bd->fan_temp_low    = 30;
    bd->fan_temp_high   = 60;

    bd->isl68127_num    = 4;

    bd->ra228_num    = 1;
    bd->tps53622_num = 1;
    bd->tps53659_num = 1;

    bd->adm1166_num     = 2;
    bd->psu_num         = 4;        //电源个数
    bd->psu_num_temp_sensors = 1;
    bd->psu_type        = PSU_TYPE_1600W_TD4;
    bd->slot_num        = 0;        //子卡个数,无子卡写0
    bd->smbus_use_index = 0;        //使用的smbus的索引
    bd->lm75_num        = 0;
    bd->max6696_num     = 4;        //max6696数量
    bd->optic_modlue_num = 64;      //可插光模块数量
    bd->eeprom_used_size = 512;     //简化设计eeprom只使用前边512字节,不够再扩展

    bd->mac_rov_min_voltage     = 648;
    bd->mac_rov_max_voltage     = 960;
    bd->mac_rov_default_voltage = 800;

    bd->port_left_rov_min_voltage      = 2002;
    bd->port_left_rov_max_voltage      = 2448;
    bd->port_left_rov_default_voltage  = 2225;
    bd->port_right_rov_min_voltage     = 2002;
    bd->port_right_rov_max_voltage     = 2448;
    bd->port_right_rov_default_voltage = 2225;
    bd->port_down_rov_min_voltage      = 1980;
    bd->port_down_rov_max_voltage      = 2420;
    bd->port_down_rov_default_voltage  = 2200;

    bd->ra228_mac_rov_min_voltage     = 640;
    bd->ra228_mac_rov_max_voltage     = 960;
    bd->ra228_mac_rov_default_voltage = 800;

    bd->fan_target_speed_coef0[0] = 2338000;          //front fan 4 coeffcient of the polynomial
    bd->fan_target_speed_coef1[0] = -13067;
    bd->fan_target_speed_coef2[0] = 2433;
    bd->fan_target_speed_coef3[0] = -13;

    bd->fan_target_speed_coef0[1] = 2123300;          //rear fan
    bd->fan_target_speed_coef1[1] = -9874;
    bd->fan_target_speed_coef2[1] = 2083;
    bd->fan_target_speed_coef3[1] = -11;
    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->cage_type[i]  = CAGE_TYPE_QSFP;
        bd->port_speed[i] = SPEED_400G;
    }
    bd->i2c_addr_isl68127[0] = 0x5C;
    bd->i2c_addr_isl68127[1] = 0x50;
    bd->i2c_addr_isl68127[2] = 0x5C;
    bd->i2c_addr_isl68127[3] = 0x54;
    bd->isl68127_describe[0] = "VR outlet to Mac TRVDD";
    bd->isl68127_describe[1] = "VR outlet to port left PORT";
    bd->isl68127_describe[2] = "VR outlet to port right PORT";
    bd->isl68127_describe[3] = "VR outlet to port down PORT";

    bd->i2c_addr_ra228    = 0x60;
    bd->ra228_describe    = "VR outlet to Mac AVS";
    bd->i2c_addr_tps53622 = 0x66;
    bd->i2c_addr_tps53659 = 0x61;

    bd->i2c_addr_eeprom    = 0x50;    //板卡eerom的i2c地址
    /* identify: 主控板-整机第一路6696 */
    bd->i2c_addr_max6696[0] = 0x18;   //max6696 i2c地址
    bd->max6696_describe[0][0]  = "Switch Outflow";
    bd->max6696_describe[0][1]  = "Switch Netstream";
    bd->max6696_describe[0][2]  = "Switch MB Inflow";

    // identify: 交换板-整机第二路6696
    bd->i2c_addr_max6696[1] = 0x18;
    bd->max6696_describe[1][0]  = "Switch Inflow1";
    bd->max6696_describe[1][1]  = "Switch MAC Left";
    bd->max6696_describe[1][2]  = "Switch MAC Up";

    // identify: 交换板-整机第三路6696
    bd->i2c_addr_max6696[2] = 0x18;
    bd->max6696_describe[2][0]  = "Switch MAC Right";
    bd->max6696_describe[2][1]  = "Switch MAC Inner1";
    bd->max6696_describe[2][2]  = "Switch MAC Inner2";

    // identify: 下端口板-整机第四路6696
    bd->i2c_addr_max6696[3] = 0x18;
    bd->max6696_describe[3][0]  = "Switch Inflow2";
    bd->max6696_describe[3][1]  = "Switch CDB 68127";
    bd->max6696_describe[3][2]  = "Switch CDB Jtag";


    /*  no 9548 or 9545 but using i2c fpga to access front port i2c
        bd->i2c_addr_pca9548    = 0x70;        //这里假设所有9545/9548使用的i2c地址都一样
        bd->i2c_addr_pca9548_2    = 0x77;
    */

    bd->i2c_addr_psu[0] = 0x50;
    bd->i2c_addr_psu[1] = 0x50;
    bd->i2c_addr_psu[2] = 0x50;
    bd->i2c_addr_psu[3] = 0x50;
    bd->i2c_addr_psu_pmbus[0] = 0x58;       //与电源i2c地址配对, +0x08
    bd->i2c_addr_psu_pmbus[1] = 0x58;
    bd->i2c_addr_psu_pmbus[2] = 0x58;   //与电源i2c地址配对, +0x08
    bd->i2c_addr_psu_pmbus[3] = 0x58;

    bd->i2c_addr_adm1166[0]  = 0x34;
    bd->i2c_addr_adm1166[1]  = 0x34;
    bd->i2c_addr_n287  = 0x6d;
    bd->i2c_addr_rtc_4337  = 0x68;

    bd->i2c_addr_fan[0] = 0x50;
    bd->i2c_addr_fan[1] = 0x50;
    bd->i2c_addr_fan[2] = 0x50;
    bd->i2c_addr_fan[3] = 0x50;
    bd->i2c_addr_fan[4] = 0x50;
    bd->i2c_addr_fan[5] = 0x50;

    //光模块i2c地址初始化
    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->i2c_addr_optic_eeprom[i] = 0x50;
        bd->i2c_addr_optic_eeprom_dom[i] = 0x50;
    }

    bd->cpld_access_type   = IO_REMAP;
    bd->cpld_base_address  = 0xdffe0000;   //由OM提供的地址
    bd->cpld_hw_addr_board = 0x2200;       //由硬件设计提供的地址
    bd->cpld_size_board    = 256;           //按字节数
    bd->cpld_hw_addr_cpu   = 0x2000;       //CPU扣板CPLD
    bd->cpld_size_cpu      = 512;          //CPU扣板CPLD
    bd->cpld_hw_addr_bios  = 0x0000;       //bios cpld
    bd->cpld_size_bios     = 512;
    bd->cpld_fan_good_flag = 0x1;

    CPLD_REG_ASSIGN(pcb_type, 0x02, 0xff, 0);

    /*board version*/
    CPLD_REG_ASSIGN(pcb_ver[0], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[1], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[2], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[3], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[4], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[5], 0x09, 0x0f, 0);

    /*HW_VERSION*/
    CPLD_REG_ASSIGN(cpld_ver[0], 0x02, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[1], 0x01, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[2], 0x02, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[3], 0x03, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[4], 0x40, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[5], 0x41, 0x0f, 0);

    bd->cpld_type_describe[0] = "LCMXO3LF_6900C_5BG400C ";
    bd->cpld_type_describe[1] = "LCMXO3LF_6900C_5BG400C ";
    bd->cpld_type_describe[2] = "LCMXO3LF_6900C_5BG400C ";
    bd->cpld_type_describe[3] = "LCMXO3LF_6900C_5BG400C ";
    bd->cpld_type_describe[4] = "LCMXO3LF_6900C_5BG400C ";
    bd->cpld_type_describe[5] = "LCMXO3LF_6900C_5BG400C ";
    bd->cpld_type_describe[6] = "Xlinx_Memory_Controller";

    bd->cpld_location_describe[0] = "cpu_cpld";
    bd->cpld_location_describe[1] = "board_cpld_0";
    bd->cpld_location_describe[2] = "board_cpld_1";
    bd->cpld_location_describe[3] = "board_cpld_2";
    bd->cpld_location_describe[4] = "board_cpld_3";
    bd->cpld_location_describe[5] = "board_cpld_4";
    bd->cpld_location_describe[6] = "dom_fpga";

    bd->dom_exist = 1;
    bd->mac_rov_device = I2C_DEV_RA228;
    memcpy((bd->dom_remap), dom_remap, DOM_PORT_MAX_NUM);
    p_dom_remap = bd->dom_remap;
    memcpy((bd->ft_second_power1_item), ft_second_power1_item, sizeof(ft_second_power1_item));

    memcpy((bd->ft_second_power2_item), ft_second_power2_item, sizeof(ft_second_power2_item));
    //0x15 bit4 is for QSPI reset, max6696 reset moved to cpu cpld
    CPLD_REG_ASSIGN(max6696_rst[0], 0x15, 0x08, 3);
    CPLD_REG_ASSIGN(max6696_rst[1], 0x15, 0x10, 4);
    CPLD_REG_ASSIGN(max6696_rst[2], 0x15, 0x20, 5);
    CPLD_REG_ASSIGN(max6696_rst[3], 0x15, 0x40, 6);
    CPLD_REG_ASSIGN(eeprom_write_protect, 0x54, 0x01, 0);

    //mac 初始完成可点端口灯
    CPLD_REG_ASSIGN(mac_init_ok, 0x0b, 0x01, 0);

    CPLD_REG_ASSIGN(cpu_init_ok, 0xb, 0x80, 7);

    //mac核心电压设置
    CPLD_REG_ASSIGN(mac_rov, 0x3d, 0xff, 0);

    //面板上的系统指示灯

    CPLD_REG_ASSIGN(pannel_sys_led_ctrl, 0x6c, 0x0f, 0);
    /*
    CPLD_REG_ASSIGN(pannel_sys_led_green, 0x6b, 0x10, 4);
    CPLD_REG_ASSIGN(pannel_sys_led_red, 0x6b, 0x20, 5);
    */
    CPLD_REG_ASSIGN(pannel_psu_led_green, 0x6a, 0x04, 2);
    CPLD_REG_ASSIGN(pannel_psu_led_red,   0x6a, 0x08, 3);
    CPLD_REG_ASSIGN(pannel_fan_led_green, 0x6a, 0x01, 0);
    CPLD_REG_ASSIGN(pannel_fan_led_red,   0x6a, 0x02, 1);
    CPLD_REG_ASSIGN(pannel_bmc_led_green, 0x6a, 0x10, 4);
    CPLD_REG_ASSIGN(pannel_bmc_led_red,   0x6a, 0x20, 5);
    CPLD_REG_ASSIGN(pannel_id_led_blue,   0x6b, 0x00, 0);

    //cpld setting for sysled led color
    /***
    0000:4Hz绿灯闪
    0001:2Hz绿灯闪
    0010:1Hz绿灯闪
    0011:0.5Hz绿灯闪
    0100:绿灯常亮
    0101:4Hz红灯闪
    0110:2Hz红灯闪
    0111:1Hz红灯闪
    1000:0.5Hz红灯闪
    1001:红灯常亮
    1010:4Hz黄灯闪
    1011:2Hz黄灯闪
    1100:1Hz黄灯闪
    1101:0.5Hz黄灯闪
    1110:黄灯常亮
    1111:灯灭
    **/
    bd->cpld_value_sys_led_code_green  = 0xf4;
    bd->cpld_value_sys_led_code_red    = 0xf9;
    bd->cpld_value_sys_led_code_yellow = 0xfe;
    bd->cpld_value_sys_led_code_dark   = 0xff;
    bd->cpld_value_sys_led_code_green_flash = 0xf0;
    bd->cpld_value_sys_led_code_red_flash = 0xf5;
    bd->cpld_value_sys_led_code_yellow_flash = 0xfa;

    CPLD_REG_ASSIGN(reset_type, 0x20, 0xff, 0);
    CPLD_REG_ASSIGN(last_reset_type, 0x38, 0xff, 0);
    CPLD_REG_ASSIGN(clr_rst, 0x21, 0x02, 1);

    //watchdog cpld 相关数据
    CPLD_REG_ASSIGN(wd_feed,      0x30, 0xff, 0);
    CPLD_REG_ASSIGN(wd_disfeed,   0x31, 0xff, 0);
    CPLD_REG_ASSIGN(wd_timeout,   0x32, 0xff, 0);
    CPLD_REG_ASSIGN(wd_enable,    0x33, 0x01, 0);

    //电源相关寄存器
    CPLD_REG_ASSIGN(psu_absent[0], 0x33, 0x01, 0);
    CPLD_REG_ASSIGN(psu_absent[1], 0x34, 0x01, 0);
    CPLD_REG_ASSIGN(psu_absent[2], 0x35, 0x01, 0);
    CPLD_REG_ASSIGN(psu_absent[3], 0x36, 0x01, 0);

    CPLD_REG_ASSIGN(psu_good[0],  0x33, 0x04, 2);
    CPLD_REG_ASSIGN(psu_good[1],  0x34, 0x04, 2);
    CPLD_REG_ASSIGN(psu_good[2],  0x35, 0x04, 2);
    CPLD_REG_ASSIGN(psu_good[3],  0x36, 0x04, 2);

    //这里开始是寄存器定义
    //风扇相关寄存器定义
    CPLD_REG_ASSIGN(fan_num,      0x70, 0x0f, 0);
    CPLD_REG_ASSIGN(fan_select,   0x70, 0xf0, 4);
    CPLD_REG_ASSIGN(fan_pwm,      0x71, 0xff, 0);
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_LOW_REG_INDEX], 0x72, 0xff, 0);
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_HIGH_REG_INDEX], 0x73, 0xff, 0);

    CPLD_REG_ASSIGN(fan_eeprom_write_protect, 0x7c, 0x00, 0);
    CPLD_REG_ASSIGN(fan_direction[5], 0x74, 0x20, 5);
    CPLD_REG_ASSIGN(fan_direction[4], 0x74, 0x10, 4);
    CPLD_REG_ASSIGN(fan_direction[3], 0x74, 0x08, 3);
    CPLD_REG_ASSIGN(fan_direction[2], 0x74, 0x04, 2);
    CPLD_REG_ASSIGN(fan_direction[1], 0x74, 0x02, 1);
    CPLD_REG_ASSIGN(fan_direction[0], 0x74, 0x01, 0);

    CPLD_REG_ASSIGN(fan_led_red[5], 0x7d, 0x20, 5);
    CPLD_REG_ASSIGN(fan_led_red[4], 0x7d, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_red[3], 0x7d, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_red[2], 0x7d, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_red[1], 0x7d, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_red[0], 0x7d, 0x01, 0);

    CPLD_REG_ASSIGN(fan_led_green[5], 0x76, 0x20, 5);
    CPLD_REG_ASSIGN(fan_led_green[4], 0x76, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_green[3], 0x76, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_green[2], 0x76, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_green[1], 0x76, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_green[0], 0x76, 0x01, 0);

    CPLD_REG_ASSIGN(fan_absent[5], 0x77, 0x20, 5);
    CPLD_REG_ASSIGN(fan_absent[4], 0x77, 0x10, 4);
    CPLD_REG_ASSIGN(fan_absent[3], 0x77, 0x08, 3);
    CPLD_REG_ASSIGN(fan_absent[2], 0x77, 0x04, 2);
    CPLD_REG_ASSIGN(fan_absent[1], 0x77, 0x02, 1);
    CPLD_REG_ASSIGN(fan_absent[0], 0x77, 0x01, 0);

    CPLD_REG_ASSIGN(fan_status[0], 0x78, 0x01, 0);
    CPLD_REG_ASSIGN(fan_status[1], 0x78, 0x02, 1);
    CPLD_REG_ASSIGN(fan_status[2], 0x78, 0x04, 2);
    CPLD_REG_ASSIGN(fan_status[3], 0x78, 0x08, 3);
    CPLD_REG_ASSIGN(fan_status[4], 0x78, 0x10, 4);
    CPLD_REG_ASSIGN(fan_status[5], 0x78, 0x20, 5);


    //光模块控制相关寄存器 sfp, 按端口索引排续
    /*
    CPLD_REG_ASSIGN(cage_power_on, 0xDF, 0xff, 0); //所有cage上电
    CPLD_REG_ASSIGN(cage_power_on, 0xE0, 0xff, 0);
    CPLD_REG_ASSIGN(cage_power_on, 0xE1, 0xff, 0);
    CPLD_REG_ASSIGN(cage_power_on, 0xE2, 0xff, 0);
    CPLD_REG_ASSIGN(cage_power_on, 0xE3, 0xff, 0);
    CPLD_REG_ASSIGN(cage_power_on, 0xE4, 0xff, 0);
    CPLD_REG_ASSIGN(cage_power_on, 0xE5, 0xff, 0);
    CPLD_REG_ASSIGN(cage_power_on, 0xE6, 0xff, 0);
    */
    CPLD_REG_ASSIGN(cage_power_on[0], 0xDF, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[1], 0xDF, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[2], 0xDF, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[3], 0xDF, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[4], 0xDF, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[5], 0xDF, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[6], 0xDF, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[7], 0xDF, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[8], 0xE0, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[9], 0xE0, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[10], 0xE0, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[11], 0xE0, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[12], 0xE0, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[13], 0xE0, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[14], 0xE0, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[15], 0xE0, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[16], 0xE1, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[17], 0xE1, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[18], 0xE1, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[19], 0xE1, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[20], 0xE1, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[21], 0xE1, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[22], 0xE1, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[23], 0xE1, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[24], 0xE2, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[25], 0xE2, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[26], 0xE2, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[27], 0xE2, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[28], 0xE2, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[29], 0xE2, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[30], 0xE2, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[31], 0xE2, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[32], 0xE3, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[33], 0xE3, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[34], 0xE3, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[35], 0xE3, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[36], 0xE3, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[37], 0xE3, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[38], 0xE3, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[39], 0xE3, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[40], 0xE4, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[41], 0xE4, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[42], 0xE4, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[43], 0xE4, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[44], 0xE4, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[45], 0xE4, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[46], 0xE4, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[47], 0xE4, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[48], 0xE5, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[49], 0xE5, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[50], 0xE5, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[51], 0xE5, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[52], 0xE5, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[53], 0xE5, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[54], 0xE5, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[55], 0xE5, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[56], 0xE6, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[57], 0xE6, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[58], 0xE6, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[59], 0xE6, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[60], 0xE6, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[61], 0xE6, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[62], 0xE6, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[63], 0xE6, 0x80, 7);

    bd->cpld_num_cages_power_on = bd->optic_modlue_num / 8;

    CPLD_REG_ASSIGN(cages_power_on[0], 0xDF, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[1], 0xE0, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[2], 0xE1, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[3], 0xE2, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[4], 0xE3, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[5], 0xE4, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[6], 0xE5, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[7], 0xE6, 0xff, 0);

    //在位信号
    //qsfp, 从0开始排续. 数组索引与端口索引一致
    CPLD_REG_ASSIGN(qsfp_present[0], 0xb1, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[1], 0xb1, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[2], 0xb1, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[3], 0xb1, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[4], 0xb1, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[5], 0xb1, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[6], 0xb1, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[7], 0xb1, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[8], 0xb2, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[9], 0xb2, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[10], 0xb2, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[11], 0xb2, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[12], 0xb2, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[13], 0xb2, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[14], 0xb2, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[15], 0xb2, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[16], 0xb3, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[17], 0xb3, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[18], 0xb3, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[19], 0xb3, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[20], 0xb3, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[21], 0xb3, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[22], 0xb3, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[23], 0xb3, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[24], 0xb4, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[25], 0xb4, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[26], 0xb4, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[27], 0xb4, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[28], 0xb4, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[29], 0xb4, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[30], 0xb4, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[31], 0xb4, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[32], 0xb5, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[33], 0xb5, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[34], 0xb5, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[35], 0xb5, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[36], 0xb5, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[37], 0xb5, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[38], 0xb5, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[39], 0xb5, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[40], 0xb6, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[41], 0xb6, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[42], 0xb6, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[43], 0xb6, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[44], 0xb6, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[45], 0xb6, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[46], 0xb6, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[47], 0xb6, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[48], 0xb7, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[49], 0xb7, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[50], 0xb7, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[51], 0xb7, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[52], 0xb7, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[53], 0xb7, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[54], 0xb7, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[55], 0xb7, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[56], 0xb8, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[57], 0xb8, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[58], 0xb8, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[59], 0xb8, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[60], 0xb8, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[61], 0xb8, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[62], 0xb8, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[63], 0xb8, 0x80, 7);

    CPLD_REG_ASSIGN(qsfp_lpmode[0], 0xb9, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[1], 0xb9, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[2], 0xb9, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[3], 0xb9, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[4], 0xb9, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[5], 0xb9, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[6], 0xb9, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[7], 0xb9, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[8], 0xba, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[9], 0xba, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[10], 0xba, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[11], 0xba, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[12], 0xba, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[13], 0xba, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[14], 0xba, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[15], 0xba, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[16], 0xbb, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[17], 0xbb, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[18], 0xbb, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[19], 0xbb, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[20], 0xbb, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[21], 0xbb, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[22], 0xbb, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[23], 0xbb, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[24], 0xbc, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[25], 0xbc, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[26], 0xbc, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[27], 0xbc, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[28], 0xbc, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[29], 0xbc, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[30], 0xbc, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[31], 0xbc, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[32], 0xbd, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[33], 0xbd, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[34], 0xbd, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[35], 0xbd, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[36], 0xbd, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[37], 0xbd, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[38], 0xbd, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[39], 0xbd, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[40], 0xbe, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[41], 0xbe, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[42], 0xbe, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[43], 0xbe, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[44], 0xbe, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[45], 0xbe, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[46], 0xbe, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[47], 0xbe, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[48], 0xbf, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[49], 0xbf, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[50], 0xbf, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[51], 0xbf, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[52], 0xbf, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[53], 0xbf, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[54], 0xbf, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[55], 0xbf, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[56], 0xc0, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[57], 0xc0, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[58], 0xc0, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[59], 0xc0, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[60], 0xc0, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[61], 0xc0, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[62], 0xc0, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[63], 0xc0, 0x80, 7);

    CPLD_REG_ASSIGN(qsfp_reset[0], 0xc1, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[1], 0xc1, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[2], 0xc1, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[3], 0xc1, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[4], 0xc1, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[5], 0xc1, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[6], 0xc1, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[7], 0xc1, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[8], 0xc2, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[9], 0xc2, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[10], 0xc2, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[11], 0xc2, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[12], 0xc2, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[13], 0xc2, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[14], 0xc2, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[15], 0xc2, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[16], 0xc3, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[17], 0xc3, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[18], 0xc3, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[19], 0xc3, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[20], 0xc3, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[21], 0xc3, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[22], 0xc3, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[23], 0xc3, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[24], 0xc4, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[25], 0xc4, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[26], 0xc4, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[27], 0xc4, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[28], 0xc4, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[29], 0xc4, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[30], 0xc4, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[31], 0xc4, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[32], 0xc5, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[33], 0xc5, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[34], 0xc5, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[35], 0xc5, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[36], 0xc5, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[37], 0xc5, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[38], 0xc5, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[39], 0xc5, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[40], 0xc6, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[41], 0xc6, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[42], 0xc6, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[43], 0xc6, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[44], 0xc6, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[45], 0xc6, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[46], 0xc6, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[47], 0xc6, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[48], 0xc7, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[49], 0xc7, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[50], 0xc7, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[51], 0xc7, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[52], 0xc7, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[53], 0xc7, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[54], 0xc7, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[55], 0xc7, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[56], 0xc8, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[57], 0xc8, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[58], 0xc8, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[59], 0xc8, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[60], 0xc8, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[61], 0xc8, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[62], 0xc8, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[63], 0xc8, 0x80, 7);

    CPLD_REG_ASSIGN(qsfp_interrupt[0], 0xc9, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[1], 0xc9, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[2], 0xc9, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[3], 0xc9, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[4], 0xc9, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[5], 0xc9, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[6], 0xc9, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[7], 0xc9, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[8], 0xca, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[9], 0xca, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[10], 0xca, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[11], 0xca, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[12], 0xca, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[13], 0xca, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[14], 0xca, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[15], 0xca, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[16], 0xcb, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[17], 0xcb, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[18], 0xcb, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[19], 0xcb, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[20], 0xcb, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[21], 0xcb, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[22], 0xcb, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[23], 0xcb, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[24], 0xcc, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[25], 0xcc, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[26], 0xcc, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[27], 0xcc, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[28], 0xcc, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[29], 0xcc, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[30], 0xcc, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[31], 0xcc, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[32], 0xcd, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[33], 0xcd, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[34], 0xcd, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[35], 0xcd, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[36], 0xcd, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[37], 0xcd, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[38], 0xcd, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[39], 0xcd, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[40], 0xce, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[41], 0xce, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[42], 0xce, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[43], 0xce, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[44], 0xce, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[45], 0xce, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[46], 0xce, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[47], 0xce, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[48], 0xcf, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[49], 0xcf, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[50], 0xcf, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[51], 0xcf, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[52], 0xcf, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[53], 0xcf, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[54], 0xcf, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[55], 0xcf, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[56], 0xd0, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[57], 0xd0, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[58], 0xd0, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[59], 0xd0, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[60], 0xd0, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[61], 0xd0, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[62], 0xd0, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[63], 0xd0, 0x80, 7);

    //重置cpu寄存器
    CPLD_REG_ASSIGN(cpu_rst, 0x15, 0x01, 0);

    //For I2C 9clock Reset
    CPLD_REG_ASSIGN(gpio_i2c_1, 0x41, 0x01, 0);
    CPLD_REG_ASSIGN(gpio_i2c_0, 0x41, 0x02, 1);

    //i2c选通寄存器
    CPLD_REG_ASSIGN(main_i2c_sel, 0x48, 0xff, 0);     //主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[0],   0x48, 0xff, 0);     //非主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[1],   0x48, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[2],   0x48, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[3],   0x48, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[4],   0x48, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[5],   0x48, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[6],   0x48, 0xff, 0);

    CPLD_REG_ASSIGN(bios_set,  0x03, 0x80, 7);
    CPLD_REG_ASSIGN(bios_get,  0x03, 0x08, 3);
    CPLD_REG_ASSIGN(cpudown,   0x62, 0x01, 0);
    CPLD_REG_ASSIGN(n287_reset, 0x16, 0x04, 2);

    //SSD 下电寄存器
    CPLD_REG_ASSIGN(ssd_pwr_down, 0x37, 0x01, 0);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 0]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x21), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 1]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x22), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 2]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x23), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 3]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x24), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_EEPROM]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x0), STEP_OVER);
    /*
                I2C_DEV_CPUBMC_EEPROM, eeprom shared in between cpu and bmc
                I2C_DEV_BMC_EEPROM, reserved for bmc, no need to access on cpu side
    */
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_CPUBMC_EEPROM]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0), STEP_OVER);
    /* ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_BMC_EEPROM]),        STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 80), STEP_OVER);
    */
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 0]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x52), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 1]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x56), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 2]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x51), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 3]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x55), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 4]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x50), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 5]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x54), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU]),      STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x10), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 1]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x11), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 2]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x12), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 3]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x13), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x41),  STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127 + 1]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x43),  STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127 + 2]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x45),  STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127 + 3]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x47),  STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_RA228]),      STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0X44),  STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_TPS53659]),   STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x42), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_TPS53622]),   STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x42), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ADM1166]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x40), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ADM1166 + 1]),  STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x46), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_N287]),       STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x32), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_RTC_4337]),   STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x20), STEP_OVER);

    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "board static data init failed: ret=%d!", ret);
        bd->initialized = 0;
    }
    else
    {
        bd->initialized = 1;
    }
    return ret;
}
int board_static_data_init_TCS83_120F(board_static_data *board_data)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = board_data;
    //无关数据全写0
    memset(bd, 0, sizeof(board_static_data));
    bd->slot_index = MAIN_BOARD;
    bd->product_type = PDT_TYPE_TCS83_120F_4U;
    bd->mainboard = NULL;
    bd->cpld_num        = 3;
    bd->isl68127_num    = 1;        //设置MAC核心电压
    bd->adm1166_num     = 2;
    bd->fan_num         = 6;        //风扇个数
    bd->motors_per_fan  = 2;        //每风扇2个马达
    bd->fan_speed_coef  = 30000000; //风扇转速转换系数, 9820是 30000000
    bd->fan_max_speed   = 12000;
    bd->fan_min_speed   = 1500;
    bd->fan_min_speed_pwm = 0xa;
    bd->fan_max_speed_pwm = 0x64;   //最大转速时对应的pwm值
    bd->fan_min_pwm_speed_percentage = 20;
    bd->fan_max_pwm_speed_percentage = 100;
    bd->fan_temp_low    = 30;
    bd->fan_temp_high   = 60;

    bd->fan_target_speed_coef0[0] = 2338000;          //front fan 4 coeffcient of the polynomial
    bd->fan_target_speed_coef1[0] = -13067;
    bd->fan_target_speed_coef2[0] = 2433;
    bd->fan_target_speed_coef3[0] = -13;

    bd->fan_target_speed_coef0[1] = 2123300;          //rear fan
    bd->fan_target_speed_coef1[1] = -9874;
    bd->fan_target_speed_coef2[1] = 2083;
    bd->fan_target_speed_coef3[1] = -11;

    bd->psu_num         = 4;        //电源个数
    bd->psu_type        = PSU_TYPE_1600W;
    bd->slot_num        = 4;        //子卡个数, 无子卡写0
    bd->smbus_use_index = 0;        //使用的smbus的索引
    bd->max6696_num     = 2;        //max6696数量
    bd->lm75_num        = 0;
    bd->optic_modlue_num = 0;       //光模块数量, 端口在子卡上
    bd->eeprom_used_size = 512;
    bd->mac_rov_min_voltage     = 750;    //th3芯片适用
    bd->mac_rov_max_voltage     = 900;
    bd->mac_rov_default_voltage = 893;
    bd->ext_phy_num = 0;    //外部phy数量，用于解复位

#if 0
    no sfp on mainboard
    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->cage_type[i]  = i < 48 ? CAGE_TYPE_SFP : CAGE_TYPE_QSFP;
        bd->port_speed[i] = i < 48 ? SPEED_25G : SPEED_100G;
    }
#endif

    bd->i2c_addr_isl68127[0] = 0x5c;
    bd->isl68127_describe[0] = "VR outlet to Mac chip";
    bd->i2c_addr_adm1166[0]  = 0x34;
    bd->i2c_addr_adm1166[1]  = 0x35;

    bd->i2c_addr_eeprom    = 0x50;  //板卡eerom的i2c地址

    bd->i2c_addr_max6696[0] = 0x18; //max6696 i2c地址
    bd->i2c_addr_max6696[1] = 0x18;
    bd->max6696_describe[0][0] = "U49_Local";
    bd->max6696_describe[0][1] = "ASIC_Spot1";
    bd->max6696_describe[0][2] = "ASIC_Spot2";            //left 指从前面板方向
    bd->max6696_describe[1][0] = "U50_Local";
    bd->max6696_describe[1][1] = "ASIC_PCB_Back";
    bd->max6696_describe[1][2] = "ASIC_PCB_Front";
    bd->i2c_addr_psu[0] = 0x50;
    bd->i2c_addr_psu[1] = 0x50;
    bd->i2c_addr_psu[2] = 0x50;
    bd->i2c_addr_psu[3] = 0x50;
    bd->i2c_addr_psu_pmbus[0] = 0x58;    //与电源i2c地址配对, +0x08
    bd->i2c_addr_psu_pmbus[1] = 0x58;
    bd->i2c_addr_psu_pmbus[2] = 0x58;    //与电源i2c地址配对, +0x08
    bd->i2c_addr_psu_pmbus[3] = 0x58;

    bd->i2c_addr_fan[0] = 0x50;
    bd->i2c_addr_fan[1] = 0x50;
    bd->i2c_addr_fan[2] = 0x50;
    bd->i2c_addr_fan[3] = 0x50;
    bd->i2c_addr_fan[4] = 0x50;
    bd->i2c_addr_fan[5] = 0x50;
    bd->cpld_access_type   = IO_REMAP;
    bd->cpld_base_address  = 0xdffe0000;   //由OM提供的地址
    bd->cpld_hw_addr_board = 0x2200;       //由硬件设计提供的地址
    bd->cpld_size_board    = 256;          //按字节数
    bd->cpld_hw_addr_cpu   = 0x2000;       //CPU扣板CPLD
    bd->cpld_size_cpu      = 256;          //CPU扣板CPLD

    //子卡cpld 相关数据
    bd->cpld_hw_addr_slot[FIRST_EXP_BOARD] = 0x2400;       //slot 1
    bd->cpld_hw_addr_slot[SECOND_EXP_BOARD] = 0x2480;       //slot 2
    bd->cpld_hw_addr_slot[THIRD_EXP_BOARD] = 0x2600;
    bd->cpld_hw_addr_slot[FOURTH_EXP_BOARD] = 0x2680;

    bd->cpld_size_slot[FIRST_EXP_BOARD] = 0x80;
    bd->cpld_size_slot[SECOND_EXP_BOARD] = 0x80;
    bd->cpld_size_slot[THIRD_EXP_BOARD] = 0x80;
    bd->cpld_size_slot[FOURTH_EXP_BOARD] = 0x80;
    //bd->cpld_size_slot[4] = 0x80;            //test 0

    CPLD_REG_ASSIGN(pcb_type, 0x02, 0xff, 0);
    CPLD_REG_ASSIGN(pcb_ver[0], 0x00, 0x0f, 0);

#if 0
    CPLD_REG_ASSIGN(reset_type_cpu_thermal,    0x20, 0x80, 7);
    CPLD_REG_ASSIGN(reset_type_cold,           0x20, 0x40, 6);
    CPLD_REG_ASSIGN(reset_type_power_en,       0x20, 0x10, 4);
    CPLD_REG_ASSIGN(reset_type_boot_sw,        0x20, 0x08, 3);
    CPLD_REG_ASSIGN(reset_type_soft,           0x20, 0x04, 2);
    CPLD_REG_ASSIGN(reset_type_wdt,            0x20, 0x02, 1);
    CPLD_REG_ASSIGN(reset_type_mlb,            0x20, 0x01, 0);
#endif
    CPLD_REG_ASSIGN(reset_type,           0x20, 0xff, 0);
    CPLD_REG_ASSIGN(last_reset_type,      0x38, 0xff, 0);
    CPLD_REG_ASSIGN(clr_rst,     0x21, 0x02, 1);

    //watchdog cpld 相关数据
    CPLD_REG_ASSIGN(wd_feed,      0x30, 0xff, 0);
    CPLD_REG_ASSIGN(wd_disfeed,   0x31, 0xff, 0);
    CPLD_REG_ASSIGN(wd_timeout,   0x32, 0xff, 0);
    CPLD_REG_ASSIGN(wd_enable,    0x33, 0x01, 0);

    bd->cpld_type_describe[0] = "XO2 1200";
    bd->cpld_type_describe[1] = "XO3 6900";
    bd->cpld_type_describe[2] = "XO3 6900";

    bd->cpld_location_describe[0] = "1st-JTAG-Chain";
    bd->cpld_location_describe[1] = "2nd-JTAG-Chain";
    bd->cpld_location_describe[2] = "3rd-JTAG-Chain";

    CPLD_REG_ASSIGN(max6696_rst[1],  0x15, 0x20, 5);
    CPLD_REG_ASSIGN(max6696_rst[0],  0x15, 0x10, 4);

    //重置cpu寄存器
    CPLD_REG_ASSIGN(cpu_rst, 0x15, 0x08, 3);

    //mac 初始完成可点端口灯
    CPLD_REG_ASSIGN(mac_init_ok,  0x0b, 0x01, 0);

    //mac核心电压设置
    CPLD_REG_ASSIGN(mac_rov,     0x3d, 0xff, 0);

    //面板上的系统指示灯
    /*
      CPLD_REG_ASSIGN(pannel_sys_led_yellow, 0x6c, 0x07, 1);
      CPLD_REG_ASSIGN(pannel_sys_led_green,  0x6c, 0x04, 2);
      CPLD_REG_ASSIGN(pannel_sys_led_red,    0x6c, 0x09, 0);
      */
    CPLD_REG_ASSIGN(pannel_sys_led_ctrl,   0x6c, 0x0f, 0);
    CPLD_REG_ASSIGN(pannel_psu_led_green,  0x6b, 0x40, 6);
    CPLD_REG_ASSIGN(pannel_psu_led_red,    0x6b, 0x80, 7);
    CPLD_REG_ASSIGN(pannel_fan_led_green,  0x6b, 0x04, 2);
    CPLD_REG_ASSIGN(pannel_fan_led_red,    0x6b, 0x08, 3);
    CPLD_REG_ASSIGN(pannel_bmc_led_green,  0x6a, 0x02, 1);
    CPLD_REG_ASSIGN(pannel_bmc_led_red,    0x6a, 0x04, 2);
    CPLD_REG_ASSIGN(pannel_id_led_blue,    0x6a, 0x01, 0);

    //子卡sysled灯
    CPLD_REG_ASSIGN(slot_sysled[0],   0x58, 0x0f, 0);
    CPLD_REG_ASSIGN(slot_sysled[1],   0x59, 0x0f, 0);
    CPLD_REG_ASSIGN(slot_sysled[2],   0x5a, 0x0f, 0);
    CPLD_REG_ASSIGN(slot_sysled[3],   0x5b, 0x0f, 0);

    //cpld setting for sysled led color
    bd->cpld_value_sys_led_code_green  = 0xf4;
    bd->cpld_value_sys_led_code_red    = 0xf9;
    bd->cpld_value_sys_led_code_yellow = 0xfe;
    bd->cpld_value_sys_led_code_dark   = 0xff;

    bd->mac_rov_device = I2C_DEV_ISL68127;
    CPLD_REG_ASSIGN(cpld_ver[0], 0x01, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[1], 0x03, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[2], 0x07, 0x0f, 0);

    //电源相关寄存器
    CPLD_REG_ASSIGN(psu_absent[0], 0x35, 0x01, 0);
    CPLD_REG_ASSIGN(psu_absent[1], 0x35, 0x02, 1);
    CPLD_REG_ASSIGN(psu_absent[2], 0x35, 0x04, 2);
    CPLD_REG_ASSIGN(psu_absent[3], 0x35, 0x08, 3);
    CPLD_REG_ASSIGN(psu_good[0],  0x34, 0x01, 0);
    CPLD_REG_ASSIGN(psu_good[1],  0x34, 0x02, 1);
    CPLD_REG_ASSIGN(psu_good[2],  0x34, 0x04, 2);
    CPLD_REG_ASSIGN(psu_good[3],  0x34, 0x08, 3);

    //风扇相关寄存器定义
    CPLD_REG_ASSIGN(fan_num,         0x70, 0x0f, 0);
    CPLD_REG_ASSIGN(fan_select,      0x70, 0xf0, 4);
    CPLD_REG_ASSIGN(fan_pwm,         0x71, 0xff, 0);
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_LOW_REG_INDEX],     0x72, 0xff, 0);
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_HIGH_REG_INDEX],    0x73, 0xff, 0);
    CPLD_REG_ASSIGN(fan_direction[0], 0x74, 0x01, 0);
    CPLD_REG_ASSIGN(fan_direction[1], 0x74, 0x02, 1);
    CPLD_REG_ASSIGN(fan_direction[2], 0x74, 0x04, 2);
    CPLD_REG_ASSIGN(fan_direction[3], 0x74, 0x08, 3);
    CPLD_REG_ASSIGN(fan_direction[4], 0x74, 0x10, 4);
    CPLD_REG_ASSIGN(fan_direction[5], 0x74, 0x20, 5);

    CPLD_REG_ASSIGN(fan_enable[0],   0x75, 0x01, 0);
    CPLD_REG_ASSIGN(fan_enable[1],   0x75, 0x02, 1);
    CPLD_REG_ASSIGN(fan_enable[2],   0x75, 0x04, 2);
    CPLD_REG_ASSIGN(fan_enable[3],   0x75, 0x08, 3);
    CPLD_REG_ASSIGN(fan_enable[4],   0x75, 0x10, 4);
    CPLD_REG_ASSIGN(fan_enable[5],   0x75, 0x20, 5);

    CPLD_REG_ASSIGN(fan_led_green[0],  0x76, 0x01, 0);
    CPLD_REG_ASSIGN(fan_led_green[1],  0x76, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_green[2],  0x76, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_green[3],  0x76, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_green[4],  0x76, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_green[5],  0x76, 0x20, 5);

    CPLD_REG_ASSIGN(fan_led_red[0],   0x7d, 0x01, 0);
    CPLD_REG_ASSIGN(fan_led_red[1],   0x7d, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_red[2],   0x7d, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_red[3],   0x7d, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_red[4],   0x7d, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_red[5],   0x7d, 0x20, 5);

    CPLD_REG_ASSIGN(fan_absent[0],   0x77, 0x01, 0);
    CPLD_REG_ASSIGN(fan_absent[1],   0x77, 0x02, 1);
    CPLD_REG_ASSIGN(fan_absent[2],   0x77, 0x04, 2);
    CPLD_REG_ASSIGN(fan_absent[3],   0x77, 0x08, 3);
    CPLD_REG_ASSIGN(fan_absent[4],   0x77, 0x10, 4);
    CPLD_REG_ASSIGN(fan_absent[5],   0x77, 0x20, 5);

    CPLD_REG_ASSIGN(fan_status[5],   0x79, 0x30, 4);
    CPLD_REG_ASSIGN(fan_status[4],   0x79, 0x0c, 2);
    CPLD_REG_ASSIGN(fan_status[3],   0x79, 0x03, 0);
    CPLD_REG_ASSIGN(fan_status[2],   0x78, 0x30, 4);
    CPLD_REG_ASSIGN(fan_status[1],   0x78, 0x0c, 2);
    CPLD_REG_ASSIGN(fan_status[0],   0x78, 0x03, 0);

    //子卡相关寄存器
    CPLD_REG_ASSIGN(slot_absent[0], 0x39, 0x01, 0);
    CPLD_REG_ASSIGN(slot_absent[1], 0x39, 0x02, 1);
    CPLD_REG_ASSIGN(slot_absent[2], 0x39, 0x04, 2);
    CPLD_REG_ASSIGN(slot_absent[3], 0x39, 0x08, 3);

    CPLD_REG_ASSIGN(slot_power_en[0], 0x3a, 0x01, 0);
    CPLD_REG_ASSIGN(slot_power_en[1], 0x3a, 0x02, 1);
    CPLD_REG_ASSIGN(slot_power_en[2], 0x3a, 0x04, 2);
    CPLD_REG_ASSIGN(slot_power_en[3], 0x3a, 0x08, 3);

    CPLD_REG_ASSIGN(slot_reset[0], 0x17, 0x01, 0);
    CPLD_REG_ASSIGN(slot_reset[1], 0x17, 0x02, 1);
    CPLD_REG_ASSIGN(slot_reset[2], 0x17, 0x04, 2);
    CPLD_REG_ASSIGN(slot_reset[3], 0x17, 0x08, 3);

    CPLD_REG_ASSIGN(slot_buff_oe1[0], 0x52, 0x03, 0);
    CPLD_REG_ASSIGN(slot_buff_oe1[1], 0x52, 0x0c, 2);
    CPLD_REG_ASSIGN(slot_buff_oe1[2], 0x52, 0x30, 4);
    CPLD_REG_ASSIGN(slot_buff_oe1[3], 0x52, 0xc0, 6);

    CPLD_REG_ASSIGN(slot_buff_oe2[0], 0x53, 0x02, 1);
    CPLD_REG_ASSIGN(slot_buff_oe2[1], 0x53, 0x08, 3);
    CPLD_REG_ASSIGN(slot_buff_oe2[2], 0x53, 0x20, 5);
    CPLD_REG_ASSIGN(slot_buff_oe2[3], 0x53, 0x80, 7);

    CPLD_REG_ASSIGN(card_power_ok[0], 0x3b, 0x01, 0);
    CPLD_REG_ASSIGN(card_power_ok[1], 0x3b, 0x02, 1);
    CPLD_REG_ASSIGN(card_power_ok[2], 0x3b, 0x04, 2);
    CPLD_REG_ASSIGN(card_power_ok[3], 0x3b, 0x08, 3);

    CPLD_REG_ASSIGN(miim_enable,     0x56, 0x80, 7);

    CPLD_REG_ASSIGN(i2c_wdt_ctrl, 0x32, 0x0f, 0);
    CPLD_REG_ASSIGN(cpu_init_ok,  0xb,  0x80, 7);
    CPLD_REG_ASSIGN(i2c_wdt_feed, 0x33, 0x01, 0);

    CPLD_REG_ASSIGN(cpld_smb_sck_reg, 0x55, 0x10, 4);
    CPLD_REG_ASSIGN(cpld_buf_enable,  0x55, 0x08, 3);

    CPLD_REG_ASSIGN(gpio_i2c_1, 0x41, 0x01, 0);
    CPLD_REG_ASSIGN(gpio_i2c_0, 0x41, 0x02, 1);

    //i2c选通寄存器
    CPLD_REG_ASSIGN(main_i2c_sel, 0x48, 0xff, 0);     //主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[0],   0x49, 0xff, 0);     //temp fan
    CPLD_REG_ASSIGN(i2c_sel[1],   0x4a, 0xff, 0);     //pwr   vr
    CPLD_REG_ASSIGN(i2c_sel[2],   0x4b, 0xff, 0);     //mgt eep
    CPLD_REG_ASSIGN(i2c_sel[3],   0x4d, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[4],   0x4f, 0xff, 0);     //slot eep

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_EEPROM + 0]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x0), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[2], 0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_EEPROM + 1]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x0), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[2], 1), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 0]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 1]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 2]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 3]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 4]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x5), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 5]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x0), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 0]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x1), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x00), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 1]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x1), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x40), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 2]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x1), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x80), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 3]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x1), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0xc0), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 0]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x00), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 1]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x20), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127 + 0]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x4), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ADM1166 + 0]),  STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ADM1166 + 1]),  STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x0), STEP_OVER);

#if 0

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_LM75]),   STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x2), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x2), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);

    //光模块选通表
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 0]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 1]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 1), STEP_OVER);
#endif

    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "board static data init failed: ret=%d!", ret);
    }
    else
    {
        bd->initialized = 1;
    }

    return ret;
}

int board_static_data_init_TCS83_120F_32H_subcard(int slot_index, board_static_data *mainboard)
{
    int ret = ERROR_SUCCESS;
    int i = 0;
    board_static_data *bd  = mainboard->sub_slot_info[slot_index];

    int current_slot_optic_start_index  = GET_I2C_DEV_OPTIC_IDX_START_SLOT(slot_index);
    int current_slot_eeprom_start_index = GET_I2C_DEV_EEPROM_IDX_START_SLOT(slot_index);
    int current_slot_lm75_start_index   = GET_I2C_DEV_LM75_IDX_START_SLOT(slot_index);

    //无关数据全写0
    memset(bd, 0, sizeof(board_static_data));

    bd->slot_index = slot_index;
    bd->product_type = PDT_TYPE_TCS83_120F_32H_SUBCARD;
    bd->mainboard = mainboard;

    bd->lm75_num         = 2;
    bd->optic_modlue_num = 32;       //光模块数量
    bd->ext_phy_num = 8;

    bd->i2c_addr_pca9545   = 0x73;        //这里假设所有9545/9548使用的i2c地址都一样
    bd->i2c_addr_pca9548   = 0x77;

    bd->i2c_addr_eeprom    = 0x50;  //子卡eerom的i2c地址
    bd->eeprom_used_size   = 512;

    bd->i2c_addr_lm75[0]   = 0x48;  //lm75  i2c地址
    bd->i2c_addr_lm75[1]   = 0x49;  //lm75  i2c地址

    bd->lm75_describe[0]   = "PHY_left";             //left right 指从前面板方向看
    bd->lm75_describe[1]   = "PHY_right";

    bd->mac_rov_device = I2C_DEV_ISL68127;
    //光模块i2c地址初始化
    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->i2c_addr_optic_eeprom[i] = 0x50;
        bd->i2c_addr_optic_eeprom_dom[i] = 0x50;
        bd->cage_type[i]  = CAGE_TYPE_QSFP;
        bd->port_speed[i] = SPEED_100G;
    }

    CPLD_REG_ASSIGN(pcb_type, 0x02, 0xff, 0);
    CPLD_REG_ASSIGN(pcb_ver[0],  0x00, 0x07, 0);

    //phy复位寄存器
    CPLD_REG_ASSIGN(phy_reset[0], 0xd, 0x01, 0);
    CPLD_REG_ASSIGN(phy_reset[1], 0xd, 0x02, 1);
    CPLD_REG_ASSIGN(phy_reset[2], 0xd, 0x04, 2);
    CPLD_REG_ASSIGN(phy_reset[3], 0xd, 0x08, 3);
    CPLD_REG_ASSIGN(phy_reset[4], 0xd, 0x10, 4);
    CPLD_REG_ASSIGN(phy_reset[5], 0xd, 0x20, 5);
    CPLD_REG_ASSIGN(phy_reset[6], 0xd, 0x40, 6);
    CPLD_REG_ASSIGN(phy_reset[7], 0xd, 0x80, 7);

    bd->cpld_num_cages_power_on = 1;
    CPLD_REG_ASSIGN(cages_power_on[0], 0x25, 0x01, 0);

    //i2c选通寄存器
    CPLD_REG_ASSIGN(main_i2c_sel, 0x47, 0xff, 0);     //主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[0],   0x48, 0xff, 0);     //非主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[1],   0x49, 0xff, 0);

    //9545 reset寄存器
    CPLD_REG_ASSIGN(9545_rst[0], 0x0f, 0x01, 0);
    CPLD_REG_ASSIGN(9545_rst[1], 0x0f, 0x02, 1);
    CPLD_REG_ASSIGN(9545_rst[2], 0x0f, 0x04, 2);
    CPLD_REG_ASSIGN(9545_rst[3], 0x0f, 0x08, 3);
    CPLD_REG_ASSIGN(9545_rst[4], 0x0f, 0x10, 4);
    CPLD_REG_ASSIGN(9545_rst[5], 0x0f, 0x20, 5);
    CPLD_REG_ASSIGN(9545_rst[6], 0x0f, 0x40, 6);
    CPLD_REG_ASSIGN(9545_rst[7], 0x0f, 0x80, 7);
    //eeprom select
    CPLD_REG_ASSIGN(9545_rst[8], 0x11, 0x01, 0);
    //9548 reset寄存器
    CPLD_REG_ASSIGN(9548_rst[0], 0x11, 0x02, 1);

    //在位信号
    //qsfp, 从0开始
    CPLD_REG_ASSIGN(qsfp_present[0], 0x3b, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[1], 0x3b, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[2], 0x3b, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[3], 0x3b, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[4], 0x3b, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[5], 0x3b, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[6], 0x3b, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[7], 0x3b, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[8], 0x3c, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[9], 0x3c, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[10], 0x3c, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[11], 0x3c, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[12], 0x3c, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[13], 0x3c, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[14], 0x3c, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[15], 0x3c, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[16], 0x3d, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[17], 0x3d, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[18], 0x3d, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[19], 0x3d, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[20], 0x3d, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[21], 0x3d, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[22], 0x3d, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[23], 0x3d, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[24], 0x3e, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[25], 0x3e, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[26], 0x3e, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[27], 0x3e, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[28], 0x3e, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[29], 0x3e, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[30], 0x3e, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[31], 0x3e, 0x40, 6);

    //qsfp, 从48开始排续. 数组索引与端口索引一致
    CPLD_REG_ASSIGN(qsfp_interrupt[0], 0x47, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[1], 0x47, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[2], 0x47, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[3], 0x47, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[4], 0x47, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[5], 0x47, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[6], 0x47, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[7], 0x47, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[8], 0x48, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[9], 0x48, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[10], 0x48, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[11], 0x48, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[12], 0x48, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[13], 0x48, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[14], 0x48, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[15], 0x48, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[16], 0x49, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[17], 0x49, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[18], 0x49, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[19], 0x49, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[20], 0x49, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[21], 0x49, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[22], 0x49, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[23], 0x49, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[24], 0x4a, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[25], 0x4a, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[26], 0x4a, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[27], 0x4a, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[28], 0x4a, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[29], 0x4a, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[30], 0x4a, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[31], 0x4a, 0x40, 6);

    CPLD_REG_ASSIGN(qsfp_lpmode[0], 0x3f, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[1], 0x3f, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[2], 0x3f, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[3], 0x3f, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[4], 0x3f, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[5], 0x3f, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[6], 0x3f, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[7], 0x3f, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[8], 0x40, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[9], 0x40, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[10], 0x40, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[11], 0x40, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[12], 0x40, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[13], 0x40, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[14], 0x40, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[15], 0x40, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[16], 0x41, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[17], 0x41, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[18], 0x41, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[19], 0x41, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[20], 0x41, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[21], 0x41, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[22], 0x41, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[23], 0x41, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[24], 0x42, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[25], 0x42, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[26], 0x42, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[27], 0x42, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[28], 0x42, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[29], 0x42, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[30], 0x42, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[31], 0x42, 0x40, 6);

    CPLD_REG_ASSIGN(qsfp_reset[0], 0x43, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[1], 0x43, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[2], 0x43, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[3], 0x43, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[4], 0x43, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[5], 0x43, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[6], 0x43, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[7], 0x43, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[8], 0x44, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[9], 0x44, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[10], 0x44, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[11], 0x44, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[12], 0x44, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[13], 0x44, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[14], 0x44, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[15], 0x44, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[16], 0x45, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[17], 0x45, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[18], 0x45, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[19], 0x45, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[20], 0x45, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[21], 0x45, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[22], 0x45, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[23], 0x45, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[24], 0x46, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[25], 0x46, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[26], 0x46, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[27], 0x46, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[28], 0x46, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[29], 0x46, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[30], 0x46, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[31], 0x46, 0x40, 6);

    //i2c选通表全放到主板数据结构里, 使用mainboard读写主板信息
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_eeprom_start_index + 0]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x0), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[2], (0x2 + slot_index)), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);

    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_lm75_start_index + 0]),   STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x2), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[0], (0x2 + slot_index) << 5), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_lm75_start_index + 1]),   STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x2), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[0], (0x2 + slot_index) << 5), STEP_OVER);

    //光模块选通表, 注意主板上器件使用mainboard操作，子卡上器件使用bd操作
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 0]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 1]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 2]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 3]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 4]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 5]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 6]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 7]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 8]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 9]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 10]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 11]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 12]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 13]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 14]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 15]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 16]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 17]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 18]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 19]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 20]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 21]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 22]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 23]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 24]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 25]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 26]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 27]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 28]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 7), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 29]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 7), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 30]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 7), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(mainboard->i2c_select_table[current_slot_optic_start_index + 31]), STEP_CNT(5), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_CPLD, mainboard->cpld_addr_i2c_sel[3], 0x4 * slot_index), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 7), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1 << 2), STEP_OVER);

    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "board static data init failed: ret=%d!", ret);
        bd->initialized = 0;
    }
    else
    {
        bd->initialized = 1;
        //mbd->sub_slot_info[slot_index] = bd;
    }
    return ret;
}

int board_static_data_init_TCS81_120F(board_static_data *board_data)
{
    int ret = ERROR_SUCCESS;
    int i = 0;
    board_static_data *bd = board_data;
    //无关数据全写0
    memset(bd, 0, sizeof(board_static_data));

    bd->slot_index = MAIN_BOARD;
    bd->product_type = PDT_TYPE_TCS81_120F_1U;
    bd->mainboard = NULL;

    bd->cpld_num        = 3;
    bd->fan_num         = 5;        //风扇个数
    bd->motors_per_fan  = 2;        //每风扇2个马达
    bd->fan_speed_coef  = 11718750; //风扇转速转换系数, 9820是 30000000
    bd->fan_max_speed   = 21500;
    bd->fan_min_speed   = 3000;
    bd->fan_min_speed_pwm = 0x27;
    bd->fan_max_speed_pwm = 0xc2;   //最大转速时对应的pwm值
    bd->fan_min_pwm_speed_percentage = 20;
    bd->fan_max_pwm_speed_percentage = 100;
    bd->fan_temp_low    = 30;
    bd->fan_temp_high   = 70;

    bd->isl68127_num    = 1;
    bd->adm1166_num     = 1;
    bd->psu_num         = 2;        //电源个数
    bd->psu_type        = PSU_TYPE_650W;
    bd->slot_num        = 0;        //子卡个数, 无子卡写0
    bd->smbus_use_index = 0;        //使用的smbus的索引
    bd->lm75_num        = 0;
    bd->max6696_num     = 1;        //max6696数量
    bd->optic_modlue_num = 56;      //可插光模块数量
    bd->eeprom_used_size = 512;     //简化设计eeprom只使用前边512字节, 不够再扩展

    bd->mac_rov_min_voltage     = 750;    //td3芯片适用
    bd->mac_rov_max_voltage     = 1000;
    bd->mac_rov_default_voltage = 890;

    bd->fan_target_speed_coef0[0] = 6524500;          //front fan 4 coeffcient of the polynomial
    bd->fan_target_speed_coef1[0] = -112170;
    bd->fan_target_speed_coef2[0] = 4567;
    bd->fan_target_speed_coef3[0] = -22;

    bd->fan_target_speed_coef0[1] = 5400100;          //rear fan
    bd->fan_target_speed_coef1[1] = -57783;
    bd->fan_target_speed_coef2[1] = 3296;
    bd->fan_target_speed_coef3[1] = -15;

    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->cage_type[i]  = i < 48 ? CAGE_TYPE_SFP : CAGE_TYPE_QSFP;    //前48口是sfp 25G, //后8个口是qsfp 100G
        bd->port_speed[i] = i < 48 ? SPEED_25G : SPEED_100G;
    }

    bd->i2c_addr_isl68127[0] = 0x5c;
    bd->isl68127_describe[0] = "VR outlet to Mac chip";
    bd->i2c_addr_eeprom    = 0x50;  //板卡eerom的i2c地址
    //bd->i2c_addr_lm75[0]   = 0x48;  //lm75 i2c地址
    bd->i2c_addr_max6696[0] = 0x18; //max6696 i2c地址
    bd->max6696_describe[0][0] = "DeviceEnv";
    bd->max6696_describe[0][1] = "ASIC_Front";
    bd->max6696_describe[0][2] = "ASIC_Back";

    bd->i2c_addr_pca9548    = 0x70;        //这里假设所有9545/9548使用的i2c地址都一样
    bd->i2c_addr_pca9548_2  = 0x77;

    bd->i2c_addr_psu[0] = 0x51;
    bd->i2c_addr_psu[1] = 0x50;
    bd->i2c_addr_psu_pmbus[0] = 0x59;    //与电源i2c地址配对, +0x08
    bd->i2c_addr_psu_pmbus[1] = 0x58;

    bd->i2c_addr_ina219[0] = 0x44;
    bd->i2c_addr_ina219[1] = 0x44;

    bd->i2c_addr_adm1166[0]  = 0x34;

    bd->i2c_addr_fan[0] = 0x50;
    bd->i2c_addr_fan[1] = 0x50;
    bd->i2c_addr_fan[2] = 0x50;
    bd->i2c_addr_fan[3] = 0x50;
    bd->i2c_addr_fan[4] = 0x50;

    //光模块i2c地址初始化
    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->i2c_addr_optic_eeprom[i] = 0x50;
        bd->i2c_addr_optic_eeprom_dom[i] = (bd->cage_type[i] == CAGE_TYPE_SFP) ? 0x51 : 0x50;
    }

    bd->cpld_access_type   = IO_REMAP;
    bd->cpld_base_address  = 0xdffe0000;   //由OM提供的地址
    bd->cpld_hw_addr_board = 0x2200;       //由硬件设计提供的地址
    bd->cpld_size_board    = 256;          //按字节数
    bd->cpld_hw_addr_cpu   = 0x2000;       //CPU扣板CPLD
    bd->cpld_size_cpu      = 256;          //CPU扣板CPLD

    CPLD_REG_ASSIGN(pcb_type, 0x02, 0xff, 0);
    CPLD_REG_ASSIGN(pcb_ver[0], 0x00, 0x0f, 0);

    bd->cpld_type_describe[0] = "LCMXO3LF_6900C_5BG400C ";
    bd->cpld_type_describe[1] = "LCMXO2_1200UHC_4FTG256C ";
    bd->cpld_type_describe[2] = "LCMXO2_1200UHC_4FTG256C ";

    bd->cpld_location_describe[0] = "1st-JTAG-Chain";
    bd->cpld_location_describe[1] = "2nd-JTAG-Chain";
    bd->cpld_location_describe[2] = "3rd-JTAG-Chain";

    //0x15 bit4 is for QSPI reset, max6696 reset moved to cpu cpld
    //CPLD_REG_ASSIGN(max6696_rst[0],  0x15, 0x10, 4);
    CPLD_REG_ASSIGN(eeprom_write_protect, 0x55, 0x03, 0);

    //mac 初始完成可点端口灯
    CPLD_REG_ASSIGN(mac_init_ok, 0x0b, 0x01, 0);
    CPLD_REG_ASSIGN(cpu_init_ok,  0xb,  0x80, 7);

    //mac核心电压设置
    CPLD_REG_ASSIGN(mac_rov,     0x3c, 0xff, 0);

    //面板上的系统指示灯

    CPLD_REG_ASSIGN(pannel_sys_led_ctrl,  0x6c, 0x0f, 0);
    /*
      CPLD_REG_ASSIGN(pannel_sys_led_green, 0x6b, 0x10, 4);
      CPLD_REG_ASSIGN(pannel_sys_led_red,   0x6b, 0x20, 5);
      */
    CPLD_REG_ASSIGN(pannel_psu_led_green, 0x6b, 0x40, 6);
    CPLD_REG_ASSIGN(pannel_psu_led_red,   0x6b, 0x80, 7);
    CPLD_REG_ASSIGN(pannel_fan_led_green, 0x6b, 0x04, 2);
    CPLD_REG_ASSIGN(pannel_fan_led_red,   0x6b, 0x08, 3);
    CPLD_REG_ASSIGN(pannel_bmc_led_green, 0x6a, 0x02, 1);
    CPLD_REG_ASSIGN(pannel_bmc_led_red,   0x6a, 0x04, 2);
    CPLD_REG_ASSIGN(pannel_id_led_blue,   0x6a, 0x01, 0);

    //cpld setting for sysled led color
    bd->cpld_value_sys_led_code_green  = 0xf4;
    bd->cpld_value_sys_led_code_red    = 0xf9;
    bd->cpld_value_sys_led_code_yellow = 0xfe;
    bd->cpld_value_sys_led_code_dark   = 0xff;

    bd->mac_rov_device = I2C_DEV_ISL68127;
    //cpld版本
    CPLD_REG_ASSIGN(cpld_ver[0], 0x01, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[1], 0x03, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[2], 0x07, 0x0f, 0);

#if 0
    CPLD_REG_ASSIGN(reset_type_cpu_thermal,    0x20, 0x80, 7);
    CPLD_REG_ASSIGN(reset_type_cold,           0x20, 0x40, 6);
    CPLD_REG_ASSIGN(reset_type_power_en,       0x20, 0x10, 4);
    CPLD_REG_ASSIGN(reset_type_boot_sw,        0x20, 0x08, 3);
    CPLD_REG_ASSIGN(reset_type_soft,           0x20, 0x04, 2);
    CPLD_REG_ASSIGN(reset_type_wdt,            0x20, 0x02, 1);
    CPLD_REG_ASSIGN(reset_type_mlb,            0x20, 0x01, 0);
#endif
    CPLD_REG_ASSIGN(reset_type,           0x20, 0xff, 0);
    CPLD_REG_ASSIGN(last_reset_type,           0x38, 0xff, 0);
    CPLD_REG_ASSIGN(clr_rst,              0x21, 0x02, 1);

    //watchdog cpld 相关数据
    CPLD_REG_ASSIGN(wd_feed,      0x30, 0xff, 0);
    CPLD_REG_ASSIGN(wd_disfeed,   0x31, 0xff, 0);
    CPLD_REG_ASSIGN(wd_timeout,   0x32, 0xff, 0);
    CPLD_REG_ASSIGN(wd_enable,    0x33, 0x01, 0);

    //电源相关寄存器
    CPLD_REG_ASSIGN(psu_absent[0], 0x35, 0x02, 1);
    CPLD_REG_ASSIGN(psu_absent[1], 0x35, 0x01, 0);

    CPLD_REG_ASSIGN(psu_good[0],  0x34, 0x02, 1);
    CPLD_REG_ASSIGN(psu_good[1],  0x34, 0x01, 0);

    //这里开始是寄存器定义
    //风扇相关寄存器定义
    CPLD_REG_ASSIGN(fan_num,      0x70, 0x0f, 0);
    CPLD_REG_ASSIGN(fan_select,   0x70, 0xf0, 4);
    CPLD_REG_ASSIGN(fan_pwm,      0x71, 0xff, 0);
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_LOW_REG_INDEX],  0x72, 0xff, 0);
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_HIGH_REG_INDEX], 0x73, 0xff, 0);

    CPLD_REG_ASSIGN(fan_direction[0], 0x74, 0x10, 4);
    CPLD_REG_ASSIGN(fan_direction[1], 0x74, 0x08, 3);
    CPLD_REG_ASSIGN(fan_direction[2], 0x74, 0x04, 2);
    CPLD_REG_ASSIGN(fan_direction[3], 0x74, 0x02, 1);
    CPLD_REG_ASSIGN(fan_direction[4], 0x74, 0x01, 0);

    CPLD_REG_ASSIGN(fan_enable[0],   0x75, 0x10, 4);
    CPLD_REG_ASSIGN(fan_enable[1],   0x75, 0x08, 3);
    CPLD_REG_ASSIGN(fan_enable[2],   0x75, 0x04, 2);
    CPLD_REG_ASSIGN(fan_enable[3],   0x75, 0x02, 1);
    CPLD_REG_ASSIGN(fan_enable[4],   0x75, 0x01, 0);

    CPLD_REG_ASSIGN(fan_led_red[0],   0x76, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_red[1],   0x76, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_red[2],   0x76, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_red[3],   0x76, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_red[4],   0x76, 0x01, 0);

    CPLD_REG_ASSIGN(fan_led_green[0],   0x7d, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_green[1],   0x7d, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_green[2],   0x7d, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_green[3],   0x7d, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_green[4],   0x7d, 0x01, 0);

    CPLD_REG_ASSIGN(fan_absent[0],   0x77, 0x10, 4);
    CPLD_REG_ASSIGN(fan_absent[1],   0x77, 0x08, 3);
    CPLD_REG_ASSIGN(fan_absent[2],   0x77, 0x04, 2);
    CPLD_REG_ASSIGN(fan_absent[3],   0x77, 0x02, 1);
    CPLD_REG_ASSIGN(fan_absent[4],   0x77, 0x01, 0);

    CPLD_REG_ASSIGN(fan_status[0], 0x79, 0x0c, 2);
    CPLD_REG_ASSIGN(fan_status[1], 0x79, 0x03, 0);
    CPLD_REG_ASSIGN(fan_status[2], 0x78, 0x30, 4);
    CPLD_REG_ASSIGN(fan_status[3], 0x78, 0x0c, 2);
    CPLD_REG_ASSIGN(fan_status[4], 0x78, 0x03, 0);

    //光模块控制相关寄存器 sfp, 按端口索引排续
    bd->cpld_num_cages_power_on = 1;
    CPLD_REG_ASSIGN(cages_power_on[0], 0x37, 0x20, 5); //所有cage上电
    //在位信号
    CPLD_REG_ASSIGN(sfp_present[0], 0x89, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_present[1], 0x89, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_present[2], 0x89, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_present[3], 0x89, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_present[4], 0x89, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_present[5], 0x89, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_present[6], 0x89, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_present[7], 0x89, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_present[8], 0x8a, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_present[9], 0x8a, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_present[10], 0x8a, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_present[11], 0x8a, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_present[12], 0x8a, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_present[13], 0x8a, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_present[14], 0x8a, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_present[15], 0x8a, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_present[16], 0x8b, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_present[17], 0x8b, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_present[18], 0x8b, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_present[19], 0x8b, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_present[20], 0x8b, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_present[21], 0x8b, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_present[22], 0x8b, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_present[23], 0x8b, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_present[24], 0x8c, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_present[25], 0x8c, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_present[26], 0x8c, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_present[27], 0x8c, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_present[28], 0x8c, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_present[29], 0x8c, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_present[30], 0x8c, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_present[31], 0x8c, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_present[32], 0x8d, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_present[33], 0x8d, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_present[34], 0x8d, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_present[35], 0x8d, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_present[36], 0x8d, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_present[37], 0x8d, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_present[38], 0x8d, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_present[39], 0x8d, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_present[40], 0x8e, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_present[41], 0x8e, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_present[42], 0x8e, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_present[43], 0x8e, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_present[44], 0x8e, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_present[45], 0x8e, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_present[46], 0x8e, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_present[47], 0x8e, 0x40, 6);

    //sfp
    CPLD_REG_ASSIGN(sfp_tx_dis[0], 0x91, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_dis[1], 0x91, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_dis[2], 0x91, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_dis[3], 0x91, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_dis[4], 0x91, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_dis[5], 0x91, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_dis[6], 0x91, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_dis[7], 0x91, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_dis[8], 0x92, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_dis[9], 0x92, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_dis[10], 0x92, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_dis[11], 0x92, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_dis[12], 0x92, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_dis[13], 0x92, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_dis[14], 0x92, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_dis[15], 0x92, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_dis[16], 0x93, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_dis[17], 0x93, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_dis[18], 0x93, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_dis[19], 0x93, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_dis[20], 0x93, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_dis[21], 0x93, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_dis[22], 0x93, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_dis[23], 0x93, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_dis[24], 0x94, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_dis[25], 0x94, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_dis[26], 0x94, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_dis[27], 0x94, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_dis[28], 0x94, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_dis[29], 0x94, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_dis[30], 0x94, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_dis[31], 0x94, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_dis[32], 0x95, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_dis[33], 0x95, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_dis[34], 0x95, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_dis[35], 0x95, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_dis[36], 0x95, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_dis[37], 0x95, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_dis[38], 0x95, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_dis[39], 0x95, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_dis[40], 0x96, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_dis[41], 0x96, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_dis[42], 0x96, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_dis[43], 0x96, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_dis[44], 0x96, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_dis[45], 0x96, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_dis[46], 0x96, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_dis[47], 0x96, 0x40, 6);

    //rx_los
    CPLD_REG_ASSIGN(sfp_rx_los[0], 0xa1, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_rx_los[1], 0xa1, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_rx_los[2], 0xa1, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_rx_los[3], 0xa1, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_rx_los[4], 0xa1, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_rx_los[5], 0xa1, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_rx_los[6], 0xa1, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_rx_los[7], 0xa1, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_rx_los[8], 0xa2, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_rx_los[9], 0xa2, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_rx_los[10], 0xa2, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_rx_los[11], 0xa2, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_rx_los[12], 0xa2, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_rx_los[13], 0xa2, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_rx_los[14], 0xa2, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_rx_los[15], 0xa2, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_rx_los[16], 0xa3, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_rx_los[17], 0xa3, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_rx_los[18], 0xa3, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_rx_los[19], 0xa3, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_rx_los[20], 0xa3, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_rx_los[21], 0xa3, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_rx_los[22], 0xa3, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_rx_los[23], 0xa3, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_rx_los[24], 0xa4, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_rx_los[25], 0xa4, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_rx_los[26], 0xa4, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_rx_los[27], 0xa4, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_rx_los[28], 0xa4, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_rx_los[29], 0xa4, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_rx_los[30], 0xa4, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_rx_los[31], 0xa4, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_rx_los[32], 0xa5, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_rx_los[33], 0xa5, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_rx_los[34], 0xa5, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_rx_los[35], 0xa5, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_rx_los[36], 0xa5, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_rx_los[37], 0xa5, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_rx_los[38], 0xa5, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_rx_los[39], 0xa5, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_rx_los[40], 0xa6, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_rx_los[41], 0xa6, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_rx_los[42], 0xa6, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_rx_los[43], 0xa6, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_rx_los[44], 0xa6, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_rx_los[45], 0xa6, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_rx_los[46], 0xa6, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_rx_los[47], 0xa6, 0x40, 6);

    //tx_fault
    CPLD_REG_ASSIGN(sfp_tx_fault[0], 0x99, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_fault[1], 0x99, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_fault[2], 0x99, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_fault[3], 0x99, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_fault[4], 0x99, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_fault[5], 0x99, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_fault[6], 0x99, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_fault[7], 0x99, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_fault[8], 0x9a, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_fault[9], 0x9a, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_fault[10], 0x9a, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_fault[11], 0x9a, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_fault[12], 0x9a, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_fault[13], 0x9a, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_fault[14], 0x9a, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_fault[15], 0x9a, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_fault[16], 0x9b, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_fault[17], 0x9b, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_fault[18], 0x9b, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_fault[19], 0x9b, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_fault[20], 0x9b, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_fault[21], 0x9b, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_fault[22], 0x9b, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_fault[23], 0x9b, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_fault[24], 0x9c, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_fault[25], 0x9c, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_fault[26], 0x9c, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_fault[27], 0x9c, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_fault[28], 0x9c, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_fault[29], 0x9c, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_fault[30], 0x9c, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_fault[31], 0x9c, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_fault[32], 0x9d, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_fault[33], 0x9d, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_fault[34], 0x9d, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_fault[35], 0x9d, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_fault[36], 0x9d, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_fault[37], 0x9d, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_fault[38], 0x9d, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_fault[39], 0x9d, 0x40, 6);
    CPLD_REG_ASSIGN(sfp_tx_fault[40], 0x9e, 0x02, 1);
    CPLD_REG_ASSIGN(sfp_tx_fault[41], 0x9e, 0x01, 0);
    CPLD_REG_ASSIGN(sfp_tx_fault[42], 0x9e, 0x08, 3);
    CPLD_REG_ASSIGN(sfp_tx_fault[43], 0x9e, 0x04, 2);
    CPLD_REG_ASSIGN(sfp_tx_fault[44], 0x9e, 0x20, 5);
    CPLD_REG_ASSIGN(sfp_tx_fault[45], 0x9e, 0x10, 4);
    CPLD_REG_ASSIGN(sfp_tx_fault[46], 0x9e, 0x80, 7);
    CPLD_REG_ASSIGN(sfp_tx_fault[47], 0x9e, 0x40, 6);

    //qsfp, 从48开始排续. 数组索引与端口索引一致
    CPLD_REG_ASSIGN(qsfp_present[48], 0xb1, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[49], 0xb1, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[50], 0xb1, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[51], 0xb1, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[52], 0xb1, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[53], 0xb1, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[54], 0xb1, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[55], 0xb1, 0x40, 6);

    CPLD_REG_ASSIGN(qsfp_lpmode[48], 0xb9, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[49], 0xb9, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[50], 0xb9, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[51], 0xb9, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[52], 0xb9, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[53], 0xb9, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[54], 0xb9, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[55], 0xb9, 0x40, 6);

    CPLD_REG_ASSIGN(qsfp_reset[48], 0xc1, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[49], 0xc1, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[50], 0xc1, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[51], 0xc1, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[52], 0xc1, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[53], 0xc1, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[54], 0xc1, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[55], 0xc1, 0x40, 6);

    CPLD_REG_ASSIGN(qsfp_interrupt[48], 0xc9, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[49], 0xc9, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[50], 0xc9, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[51], 0xc9, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[52], 0xc9, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[53], 0xc9, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[54], 0xc9, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[55], 0xc9, 0x40, 6);

    //9548 reset寄存器
    CPLD_REG_ASSIGN(9548_rst[0], 0x16, 0x01, 0);
    CPLD_REG_ASSIGN(9548_rst[1], 0x16, 0x02, 1);
    CPLD_REG_ASSIGN(9548_rst[2], 0x16, 0x04, 2);
    CPLD_REG_ASSIGN(9548_rst[3], 0x16, 0x08, 3);
    CPLD_REG_ASSIGN(9548_rst[4], 0x16, 0x10, 4);
    CPLD_REG_ASSIGN(9548_rst[5], 0x16, 0x20, 5);
    CPLD_REG_ASSIGN(9548_rst[6], 0x16, 0x40, 6);
    CPLD_REG_ASSIGN(9548_rst[7], 0x16, 0x80, 7);

    //重置cpu寄存器
    CPLD_REG_ASSIGN(cpu_rst, 0x15, 0x08, 3);
    //For I2C 9clock Reset
    CPLD_REG_ASSIGN(gpio_i2c_1, 0x41, 0x01, 0);
    CPLD_REG_ASSIGN(gpio_i2c_0, 0x41, 0x02, 1);
    //i2c选通寄存器
    CPLD_REG_ASSIGN(main_i2c_sel, 0x48, 0xff, 0);     //主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[0],   0x49, 0xff, 0);     //非主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[1],   0x4a, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[2],   0x4b, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[3],   0x4c, 0xff, 0);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696]),  STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 1]),  STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 2), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_EEPROM]),   STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0), STEP_OVER);
    //ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_LM75]),   STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 2), STEP(OP_TYPE_WR_9545, bd->i2c_addr_pca9545, 1<<1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 0]),    STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x04), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 1]),    STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x03), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 2]),    STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x02), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 3]),    STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x01), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 4]),    STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 5), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[0], 0x00), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 0]),    STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 1), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x6F), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 1]),    STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 1), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x2F), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_INA219 + 0]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 1), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0xAF), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_INA219 + 1]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 1), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0xEF), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127]), STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x04), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ADM1166]),  STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x00), STEP_OVER);

    //I2C DEV
    //ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_I350]),     STEP_CNT(3), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 6), STEP(OP_TYPE_WR_CPLD, 0x4b, 0xee), STEP_OVER);

    //光模块选通表
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 0]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 1]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 2]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 3]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 4]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 5), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 5]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 6]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 7), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 7]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 0), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 6), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 8]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 9]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 10]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 11]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 12]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 5), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 13]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 14]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 7), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 15]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 1), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 6), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 16]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 17]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 18]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 19]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 20]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 5), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 21]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 22]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 7), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 23]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 2), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 6), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 24]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 25]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 26]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 27]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 28]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 5), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 29]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 30]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 7), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 31]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 3), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 6), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 32]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 33]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 34]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 35]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 36]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 5), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 37]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 38]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 7), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 39]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 4), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 6), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 40]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 41]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 42]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 43]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 44]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 5), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 45]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 46]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 7), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 47]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 5), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 6), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 48]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 49]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 50]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 51]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 52]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 5), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 53]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 54]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 7), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_OPTIC_IDX_START + 55]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x7), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548, 1 << 6), STEP(OP_TYPE_WR_9548, bd->i2c_addr_pca9548_2, 1 << 6), STEP_OVER);

    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "board static data init failed: ret=%d!", ret);
        bd->initialized = 0;
    }
    else
    {
        bd->initialized = 1;
    }
    return ret;
}

int board_static_data_init_LS_9855_32CDF_W1(board_static_data *board_data)
{
    int ret = ERROR_SUCCESS;
    int i = 0;
    board_static_data *bd = board_data;
    u8 dom_remap[DOM_PORT_MAX_NUM] =
    {
        0,  1,  4,  5,  8,  9,  12, 13,  /* 00-07 */
        16, 17, 20, 21, 24, 25, 28, 29,  /* 08-15 */
        32, 33, 36, 37, 40, 41, 44, 45,  /* 16-23 */
        48, 49, 52, 53, 56, 57, 60, 61   /* 24-31 */
    };
    second_power_item_s ft_second_power1_item[] =
    {
        {3300000, 3400000, 3150000, "VP1", 0},
        {1800000, 1890000, 1710000, "VP2", 1},
        {2500000, 2700000, 1300000, "VP3", 2},
        {1800000, 1890000, 1710000, "VP4", 3},
        {12000000, 13200000, 10800000, "VH", 4},
        {1200000, 1260000, 1140000, "VX1", 5},
        {1150000, 1210000, 1090000, "VX2", 6},
        {1000000, 1050000, 950000,  "VX3", 7},
    };

    second_power_item_s ft_second_power2_item[] =
    {
        {3300000, 3400000, 3150000, "VP1", 0},
        {3300000, 3465000, 3200000, "VP2", 1},
        {1200000, 1250000, 1160000, "VP3", 2},
        {1200000, 1250000, 1160000, "VP4", 3},
        {12000000, 13200000, 10800000, "VH", 4},
        {1000000, 1050000, 950000,  "VX1", 5},
        {750000, 800000, 720000,    "VX2", 6},
        {800000, 950000, 700000,    "VX3", 7},
    };

    //无关数据全写0
    memset(bd, 0, sizeof(board_static_data));

    bd->slot_index = MAIN_BOARD;
    bd->product_type = PDT_TYPE_LS_9855_32CDF_W1;
    bd->mainboard = NULL;

    bd->cpld_num        = 5;        //5 个cpld
    bd->fan_num         = 6;        //风扇个数
    bd->motors_per_fan    = 2;        //每风扇2个马达
    bd->fan_speed_coef    = 11718750; //风扇转速转换系数, 9820是 30000000
    bd->fan_max_speed     = 20000;
    bd->fan_min_speed     = 5500;
    bd->fan_min_speed_pwm = 20;
    bd->fan_max_speed_pwm = 100;    //最大转速时对应的pwm值
    bd->fan_min_pwm_speed_percentage = 20;
    bd->fan_max_pwm_speed_percentage = 100;
    bd->fan_temp_low    = 30;
    bd->fan_temp_high    = 70;
    bd->cpld_fan_good_flag = 0x3;

    bd->isl68127_num    = 1;
    bd->adm1166_num     = 2;
    bd->psu_num         = 2;        //电源个数
    bd->tps53622_num = 1;
    bd->tps53659_num = 1;
    bd->psu_num_temp_sensors = 1;
    /*1600W power supply unit*/
    bd->psu_type        =  PSU_TYPE_1600W_TD4;
    bd->slot_num        = 0;        //子卡个数, 无子卡写0
    bd->smbus_use_index = 0;        //使用的smbus的索引
    bd->lm75_num        = 0;
    /*2 6696, 1 on control board and 1 on port board */
    bd->max6696_num     = 3;          //max6696数量 增加u68
    bd->optic_modlue_num = 32;        //可插光模块数量
    bd->eeprom_used_size = 512;       //简化设计eeprom只使用前边512字节, 不够再扩展

    bd->mac_rov_min_voltage     = 698;
    bd->mac_rov_max_voltage     = 927;
    bd->mac_rov_default_voltage = 800;

    bd->fan_target_speed_coef0[0] = 6524500;          //front fan 4 coeffcient of the polynomial
    bd->fan_target_speed_coef1[0] = -112170;
    bd->fan_target_speed_coef2[0] = 4567;
    bd->fan_target_speed_coef3[0] = -22;

    bd->fan_target_speed_coef0[1] = 5400100;          //rear fan
    bd->fan_target_speed_coef1[1] = -57783;
    bd->fan_target_speed_coef2[1] = 3296;
    bd->fan_target_speed_coef3[1] = -15;
    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->cage_type[i]  = CAGE_TYPE_QSFP;
        bd->port_speed[i] = i < 24 ? SPEED_200G : SPEED_400G;
    }
    bd->i2c_addr_isl68127[0] = 0x5c;
    bd->isl68127_describe[0] = "VR outlet to Mac chip";
    /*2 eeproms , 1 for cpu and the other one shared in between cpu and bmc*/
    bd->i2c_addr_eeprom    = 0x50;    //板卡eerom的i2c地址
    /*2 6696 ,Description TODO*/
    /* identify: 主控板6696 */
    bd->i2c_addr_max6696[0] = 0x18;   //max6696 i2c地址
    bd->max6696_describe[0][0] = "Switch Air Outlet";
    bd->max6696_describe[0][1] = "Switch Air Inlet";
    bd->max6696_describe[0][2] = "Switch PCB Near NS FPGA";
    // identify: 端口板第一片6696
    bd->i2c_addr_max6696[1] = 0x18;
    bd->max6696_describe[1][0] = "Switch PCB Near MAC";
    bd->max6696_describe[1][1] = "Switch PWR2 Inlet";
    bd->max6696_describe[1][2] = "Switch PCB Near Port49";

    // identify: 端口板第二片6696
    bd->i2c_addr_max6696[2] = 0x18;
    bd->max6696_describe[2][0] = "Switch PCB U68-0";
    bd->max6696_describe[2][1] = "Switch MAC U68-1";
    bd->max6696_describe[2][2] = "Switch MAC U68-2";

    /*  no 9548 or 9545 but using i2c fpga to access front port i2c
        bd->i2c_addr_pca9548      = 0x70;        //这里假设所有9545/9548使用的i2c地址都一样
        bd->i2c_addr_pca9548_2    = 0x77;
    */
    bd->i2c_addr_psu[0] = 0x50;
    bd->i2c_addr_psu[1] = 0x50;
    bd->i2c_addr_psu_pmbus[0] = 0x58;        //与电源i2c地址配对, +0x08
    bd->i2c_addr_psu_pmbus[1] = 0x58;

    bd->i2c_addr_ina219[0] = 0x44;
    bd->i2c_addr_ina219[1] = 0x44;

    bd->i2c_addr_adm1166[0]  = 0x34;
    bd->i2c_addr_adm1166[1]  = 0x34;

    bd->i2c_addr_tps53622 = 0x66;
    bd->i2c_addr_tps53659 = 0x61;

    bd->i2c_addr_n287      = 0x6d;
    bd->i2c_addr_rtc_4337  = 0x68;

    bd->i2c_addr_fan[0] = 0x50;
    bd->i2c_addr_fan[1] = 0x50;
    bd->i2c_addr_fan[2] = 0x50;
    bd->i2c_addr_fan[3] = 0x50;
    bd->i2c_addr_fan[4] = 0x50;
    bd->i2c_addr_fan[5] = 0x50;

    //光模块i2c地址初始化
    for (i = 0; i < bd->optic_modlue_num; i++)
    {
        bd->i2c_addr_optic_eeprom[i] = 0x50;
        bd->i2c_addr_optic_eeprom_dom[i] = (bd->cage_type[i] == CAGE_TYPE_SFP) ? 0x51 : 0x50;
    }

    bd->cpld_access_type   = IO_REMAP;
    bd->cpld_base_address  = 0xdffe0000;   //由OM提供的地址
    bd->cpld_hw_addr_board = 0x2200;       //由硬件设计提供的地址
    bd->cpld_size_board    = 256;           //按字节数
    bd->cpld_hw_addr_cpu   = 0x2000;       //CPU扣板CPLD
    bd->cpld_size_cpu      = 512;           //CPU扣板CPLD
    bd->cpld_hw_addr_bios  = 0x0000;       //bios cpld
    bd->cpld_size_bios     = 512;

    CPLD_REG_ASSIGN(pcb_type, 0x02, 0xff, 0);

    /*board version*/
    CPLD_REG_ASSIGN(pcb_ver[0], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[1], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[2], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[3], 0x00, 0x0f, 0);
    CPLD_REG_ASSIGN(pcb_ver[4], 0x00, 0x0f, 0);

    /*HW_VERSION*/
    CPLD_REG_ASSIGN(cpld_ver[0], 0x02, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[1], 0x01, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[2], 0x02, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[3], 0x03, 0x0f, 0);
    CPLD_REG_ASSIGN(cpld_ver[4], 0x40, 0x0f, 0);

    bd->cpld_type_describe[0] = "LCMXO3LF-6900C-6BG400CAM1 ";
    bd->cpld_type_describe[1] = "LCMXO3LF-6900C-5BG400CAM1 ";
    bd->cpld_type_describe[2] = "LCMXO3LF-6900C-5BG400CAM1 ";
    bd->cpld_type_describe[3] = "LCMXO3LF-6900C-5BG400CAM1 ";
    bd->cpld_type_describe[4] = "LCMXO3LF-6900C-5BG400CAM1 ";
    bd->cpld_type_describe[5] = "Xlinx_Memory_Controller";

    /*  bd->cpld_location_describe[0] = "1st-JTAG-Chain";
        bd->cpld_location_describe[1] = "2nd-JTAG-Chain";
        bd->cpld_location_describe[2] = "3rd-JTAG-Chain";
        bd->cpld_location_describe[3] = "4th-JTAG-Chain";*/

    bd->cpld_location_describe[0] = "cpu_cpld";
    bd->cpld_location_describe[1] = "board_cpld_0";
    bd->cpld_location_describe[2] = "board_cpld_1";
    bd->cpld_location_describe[3] = "board_cpld_2";
    bd->cpld_location_describe[4] = "board_cpld_3";

    bd->dom_exist = 1;
    bd->mac_rov_device = I2C_DEV_ISL68127;
    memcpy((bd->dom_remap), dom_remap, DOM_PORT_MAX_NUM);
    p_dom_remap = bd->dom_remap;
    memcpy((bd->ft_second_power1_item), ft_second_power1_item, sizeof(ft_second_power1_item));
    memcpy((bd->ft_second_power2_item), ft_second_power2_item, sizeof(ft_second_power2_item));
    CPLD_REG_ASSIGN(max6696_rst[0], 0x13, 0x01,  0);
    CPLD_REG_ASSIGN(max6696_rst[1], 0x12, 0x04,  2);
    CPLD_REG_ASSIGN(max6696_rst[2], 0x12, 0x02,  1);

    /*
    eeprom write protect register
    EN_WP_reg[3]: Netstream FPGA模块eeprom写保护
    EN_WP_reg[2]: CPU和BMC共享eeprom写保护
    EN_WP_reg[1]: BMC独享eeprom写保护
    EN_WP_reg[0]: CPU独享eeprom写保护
    */
    CPLD_REG_ASSIGN(eeprom_write_protect, 0x52, 0x01, 0); /*默认写保护*/

    //mac 初始完成可点端口灯
    CPLD_REG_ASSIGN(mac_init_ok, 0x0b, 0x01, 0);

    //mac核心电压设置
    CPLD_REG_ASSIGN(mac_rov,     0x3d, 0xff, 0);

    //面板上的系统指示灯
    /*
        CPLD_REG_ASSIGN(pannel_sys_led_green, 0x6b, 0x10, 4);
        CPLD_REG_ASSIGN(pannel_sys_led_red,   0x6b, 0x20, 5);
    */
    CPLD_REG_ASSIGN(pannel_sys_led_ctrl,  0x6c, 0x0f, 0);
    CPLD_REG_ASSIGN(pannel_psu_led_green, 0x6b, 0x04, 2);
    CPLD_REG_ASSIGN(pannel_psu_led_red,   0x6b, 0x08, 3);
    CPLD_REG_ASSIGN(pannel_fan_led_green, 0x6b, 0x01, 0);
    CPLD_REG_ASSIGN(pannel_fan_led_red,   0x6b, 0x02, 1);
    //CPLD_REG_ASSIGN(pannel_bmc_led_green, 0x6a, 0x02, 1);
    //CPLD_REG_ASSIGN(pannel_bmc_led_red,   0x6a, 0x04, 2);
    //CPLD_REG_ASSIGN(pannel_id_led_blue,   0x6a, 0x01, 0);

    //cpld setting for sysled led color
    /***
    0000:4Hz绿灯闪
    0001:2Hz绿灯闪
    0010:1Hz绿灯闪
    0011:0.5Hz绿灯闪
    0100:绿灯常亮
    0101:4Hz红灯闪
    0110:2Hz红灯闪
    0111:1Hz红灯闪
    1000:0.5Hz红灯闪
    1001:红灯常亮
    1010:4Hz黄灯闪
    1011:2Hz黄灯闪
    1100:1Hz黄灯闪
    1101:0.5Hz黄灯闪
    1110:黄灯常亮
    1111:灯灭
    **/
    bd->cpld_value_sys_led_code_green  = 0xf4;
    bd->cpld_value_sys_led_code_red    = 0xf9;
    bd->cpld_value_sys_led_code_yellow = 0xfe;
    bd->cpld_value_sys_led_code_dark   = 0xff;
    bd->cpld_value_sys_led_code_green_flash  = 0xf0;
    bd->cpld_value_sys_led_code_red_flash    = 0xf5;
    bd->cpld_value_sys_led_code_yellow_flash = 0xfa;

    /* reboot cause register on cpu cpld: 0x20表示没有进行clr_reset操作前本设备所有曾经发生过的复位原因; 0x38为上次重启原因*/
#if 0
    CPLD_REG_ASSIGN(reset_type_cpu_thermal,    0x20, 0x80, 7);
    CPLD_REG_ASSIGN(reset_type_cold,           0x20, 0x40, 6);
    CPLD_REG_ASSIGN(reset_type_power_en,       0x20, 0x10, 4);
    CPLD_REG_ASSIGN(reset_type_boot_sw,        0x20, 0x08, 3);
    CPLD_REG_ASSIGN(reset_type_soft,           0x20, 0x04, 2);
    CPLD_REG_ASSIGN(reset_type_wdt,            0x20, 0x02, 1);
    CPLD_REG_ASSIGN(reset_type_mlb,            0x20, 0x01, 0);
#endif
    //CPLD_REG_ASSIGN(reset_type,           0x20, 0xff, 0);
    CPLD_REG_ASSIGN(last_reset_type,      0x38, 0xff, 0);
    /*0x21寄存器清除的是0x20的内容*/
    //CPLD_REG_ASSIGN(clr_rst,     0x21, 0x02, 1);

    //watchdog cpld 相关数据
    //CPLD_REG_ASSIGN(wd_feed,      0x30, 0xff, 0);
    //CPLD_REG_ASSIGN(wd_disfeed,   0x31, 0xff, 0);
    //CPLD_REG_ASSIGN(wd_timeout,   0x32, 0xff, 0);
    //CPLD_REG_ASSIGN(wd_enable,      0x33, 0x01, 0);

    //电源相关寄存器
    CPLD_REG_ASSIGN(psu_absent[0], 0x35, 0x01, 0);
    CPLD_REG_ASSIGN(psu_absent[1], 0x35, 0x02, 1);

    CPLD_REG_ASSIGN(psu_good[0],  0x34, 0x01, 0);
    CPLD_REG_ASSIGN(psu_good[1],  0x34, 0x02, 1);

    //这里开始是寄存器定义
    //风扇相关寄存器定义

    //CPLD_REG_ASSIGN(fan_num,      0x70, 0x0f, 0);
    CPLD_REG_ASSIGN(fan_select,   0x70, 0x0f, 0);
    CPLD_REG_ASSIGN(fan_pwm,      0x71, 0xff, 0);

    /*2 bits for each fan*/
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_LOW_REG_INDEX],  0x72, 0xff, 0);
    CPLD_REG_ASSIGN(fan_speed[CPLD_FAN_SPEED_HIGH_REG_INDEX], 0x73, 0xff, 0);

    /*reg[5:0]对应6个风扇，写0关闭，写1打开*/
    CPLD_REG_ASSIGN(fan_eeprom_write_protect, 0x76, 0x00, 0);

    /* 0:power2port 1:port2power */

    CPLD_REG_ASSIGN(fan_direction[5], 0x74, 0x20, 5);
    CPLD_REG_ASSIGN(fan_direction[4], 0x74, 0x10, 4);
    CPLD_REG_ASSIGN(fan_direction[3], 0x74, 0x08, 3);
    CPLD_REG_ASSIGN(fan_direction[2], 0x74, 0x04, 2);
    CPLD_REG_ASSIGN(fan_direction[1], 0x74, 0x02, 1);
    CPLD_REG_ASSIGN(fan_direction[0], 0x74, 0x01, 0);

    CPLD_REG_ASSIGN(fan_enable[5], 0x75, 0x20, 5);
    CPLD_REG_ASSIGN(fan_enable[4], 0x75, 0x10, 4);
    CPLD_REG_ASSIGN(fan_enable[3], 0x75, 0x08, 3);
    CPLD_REG_ASSIGN(fan_enable[2], 0x75, 0x04, 2);
    CPLD_REG_ASSIGN(fan_enable[1], 0x75, 0x02, 1);
    CPLD_REG_ASSIGN(fan_enable[0], 0x75, 0x01, 0);

    CPLD_REG_ASSIGN(fan_led_red[5], 0x7c, 0x20, 5);
    CPLD_REG_ASSIGN(fan_led_red[4], 0x7c, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_red[3], 0x7c, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_red[2], 0x7c, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_red[1], 0x7c, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_red[0], 0x7c, 0x01, 0);

    CPLD_REG_ASSIGN(fan_absent[5], 0x77, 0x20, 5);
    CPLD_REG_ASSIGN(fan_absent[4], 0x77, 0x10, 4);
    CPLD_REG_ASSIGN(fan_absent[3], 0x77, 0x08, 3);
    CPLD_REG_ASSIGN(fan_absent[2], 0x77, 0x04, 2);
    CPLD_REG_ASSIGN(fan_absent[1], 0x77, 0x02, 1);
    CPLD_REG_ASSIGN(fan_absent[0], 0x77, 0x01, 0);

    CPLD_REG_ASSIGN(fan_led_green[5], 0x7d, 0x20, 5);
    CPLD_REG_ASSIGN(fan_led_green[4], 0x7d, 0x10, 4);
    CPLD_REG_ASSIGN(fan_led_green[3], 0x7d, 0x08, 3);
    CPLD_REG_ASSIGN(fan_led_green[2], 0x7d, 0x04, 2);
    CPLD_REG_ASSIGN(fan_led_green[1], 0x7d, 0x02, 1);
    CPLD_REG_ASSIGN(fan_led_green[0], 0x7d, 0x01, 0);

    CPLD_REG_ASSIGN(fan_status[5], 0x79, 0x30, 4);
    CPLD_REG_ASSIGN(fan_status[4], 0x79, 0x0c, 2);
    CPLD_REG_ASSIGN(fan_status[3], 0x79, 0x03, 0);
    CPLD_REG_ASSIGN(fan_status[2], 0x78, 0x30, 4);
    CPLD_REG_ASSIGN(fan_status[1], 0x78, 0x0c, 2);
    CPLD_REG_ASSIGN(fan_status[0], 0x78, 0x03, 0);

    //光模块控制相关寄存器 , 按端口索引排续
    CPLD_REG_ASSIGN(cage_power_on[0], 0xdf, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[1], 0xdf, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[2], 0xdf, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[3], 0xdf, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[4], 0xdf, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[5], 0xdf, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[6], 0xdf, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[7], 0xdf, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[8], 0xe0, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[9], 0xe0, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[10], 0xe0, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[11], 0xe0, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[12], 0xe0, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[13], 0xe0, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[14], 0xe0, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[15], 0xe0, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[16], 0xe1, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[17], 0xe1, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[18], 0xe1, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[19], 0xe1, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[20], 0xe1, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[21], 0xe1, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[22], 0xe1, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[23], 0xe1, 0x80, 7);
    CPLD_REG_ASSIGN(cage_power_on[24], 0xe2, 0x01, 0);
    CPLD_REG_ASSIGN(cage_power_on[25], 0xe2, 0x02, 1);
    CPLD_REG_ASSIGN(cage_power_on[26], 0xe2, 0x04, 2);
    CPLD_REG_ASSIGN(cage_power_on[27], 0xe2, 0x08, 3);
    CPLD_REG_ASSIGN(cage_power_on[28], 0xe2, 0x10, 4);
    CPLD_REG_ASSIGN(cage_power_on[29], 0xe2, 0x20, 5);
    CPLD_REG_ASSIGN(cage_power_on[30], 0xe2, 0x40, 6);
    CPLD_REG_ASSIGN(cage_power_on[31], 0xe2, 0x80, 7);

    bd->cpld_num_cages_power_on = bd->optic_modlue_num / 8;

    CPLD_REG_ASSIGN(cages_power_on[0], 0xdf, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[1], 0xe0, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[2], 0xe1, 0xff, 0);
    CPLD_REG_ASSIGN(cages_power_on[3], 0xe2, 0xff, 0);

    //在位信号
    CPLD_REG_ASSIGN(qsfp_present[0], 0xb1, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[1], 0xb1, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[2], 0xb1, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[3], 0xb1, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[4], 0xb1, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[5], 0xb1, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[6], 0xb1, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[7], 0xb1, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[8], 0xb2, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[9], 0xb2, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[10], 0xb2, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[11], 0xb2, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[12], 0xb2, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[13], 0xb2, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[14], 0xb2, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[15], 0xb2, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[16], 0xb3, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[17], 0xb3, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[18], 0xb3, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[19], 0xb3, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[20], 0xb3, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[21], 0xb3, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[22], 0xb3, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[23], 0xb3, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_present[24], 0xb4, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_present[25], 0xb4, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_present[26], 0xb4, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_present[27], 0xb4, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_present[28], 0xb4, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_present[29], 0xb4, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_present[30], 0xb4, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_present[31], 0xb4, 0x80, 7);

    CPLD_REG_ASSIGN(qsfp_lpmode[0], 0xb9, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[1], 0xb9, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[2], 0xb9, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[3], 0xb9, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[4], 0xb9, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[5], 0xb9, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[6], 0xb9, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[7], 0xb9, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[8], 0xba, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[9], 0xba, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[10], 0xba, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[11], 0xba, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[12], 0xba, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[13], 0xba, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[14], 0xba, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[15], 0xba, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[16], 0xbb, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[17], 0xbb, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[18], 0xbb, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[19], 0xbb, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[20], 0xbb, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[21], 0xbb, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[22], 0xbb, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[23], 0xbb, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_lpmode[24], 0xbc, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_lpmode[25], 0xbc, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_lpmode[26], 0xbc, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_lpmode[27], 0xbc, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_lpmode[28], 0xbc, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_lpmode[29], 0xbc, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_lpmode[30], 0xbc, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_lpmode[31], 0xbc, 0x80, 7);

    CPLD_REG_ASSIGN(qsfp_reset[0], 0xc1, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[1], 0xc1, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[2], 0xc1, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[3], 0xc1, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[4], 0xc1, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[5], 0xc1, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[6], 0xc1, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[7], 0xc1, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[8], 0xc2, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[9], 0xc2, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[10], 0xc2, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[11], 0xc2, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[12], 0xc2, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[13], 0xc2, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[14], 0xc2, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[15], 0xc2, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[16], 0xc3, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[17], 0xc3, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[18], 0xc3, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[19], 0xc3, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[20], 0xc3, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[21], 0xc3, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[22], 0xc3, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[23], 0xc3, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_reset[24], 0xc4, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_reset[25], 0xc4, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_reset[26], 0xc4, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_reset[27], 0xc4, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_reset[28], 0xc4, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_reset[29], 0xc4, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_reset[30], 0xc4, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_reset[31], 0xc4, 0x80, 7);

    CPLD_REG_ASSIGN(qsfp_interrupt[0], 0xc9, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[1], 0xc9, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[2], 0xc9, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[3], 0xc9, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[4], 0xc9, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[5], 0xc9, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[6], 0xc9, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[7], 0xc9, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[8], 0xca, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[9], 0xca, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[10], 0xca, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[11], 0xca, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[12], 0xca, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[13], 0xca, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[14], 0xca, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[15], 0xca, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[16], 0xcb, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[17], 0xcb, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[18], 0xcb, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[19], 0xcb, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[20], 0xcb, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[21], 0xcb, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[22], 0xcb, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[23], 0xcb, 0x80, 7);
    CPLD_REG_ASSIGN(qsfp_interrupt[24], 0xcc, 0x01, 0);
    CPLD_REG_ASSIGN(qsfp_interrupt[25], 0xcc, 0x02, 1);
    CPLD_REG_ASSIGN(qsfp_interrupt[26], 0xcc, 0x04, 2);
    CPLD_REG_ASSIGN(qsfp_interrupt[27], 0xcc, 0x08, 3);
    CPLD_REG_ASSIGN(qsfp_interrupt[28], 0xcc, 0x10, 4);
    CPLD_REG_ASSIGN(qsfp_interrupt[29], 0xcc, 0x20, 5);
    CPLD_REG_ASSIGN(qsfp_interrupt[30], 0xcc, 0x40, 6);
    CPLD_REG_ASSIGN(qsfp_interrupt[31], 0xcc, 0x80, 7);

    //i2c选通寄存器
    CPLD_REG_ASSIGN(main_i2c_sel, 0x48, 0xff, 0);      //主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[0],   0x49, 0xff, 0);      //非主通道选择对应的cpld寄存器
    CPLD_REG_ASSIGN(i2c_sel[1],   0x4a, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[2],   0x4b, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[3],   0x4c, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[4],   0x4d, 0xff, 0);
    CPLD_REG_ASSIGN(i2c_sel[5],   0x50, 0xff, 0);
    // 9548 reset寄存器 -----TCS84没有
    //重置cpu寄存器
    CPLD_REG_ASSIGN(cpu_rst, 0x15, 0x08, 3);
    CPLD_REG_ASSIGN(n287_reset, 0x14, 0x02, 1);
    CPLD_REG_ASSIGN(cpu_init_ok, 0x8, 0x80, 7);

    //CPLD_REG_ASSIGN(i2c_wdt_ctrl, 0x32, 0x0f, 0);
    //CPLD_REG_ASSIGN(i2c_wdt_feed, 0x33, 0x01, 0);
    /*
    add board i2c channel select register 0x50:
    0x050    I2C通道选择寄存器8    1    PWR_I2C_S1    PWR_I2C_S0    PWR_I2C_OE    1    MAX6696_I2C_S1    MAX6696_I2C_S0    MAX6696_I2C_OE
    MAX6696_I2C_S1    MAX6696_I2C_S0    MAX6696_I2C_OE
    6696 1 : 000
    6696 2 : 010
    PWR_I2C_S1    PWR_I2C_S0    PWR_I2C_OE (select 68127 6696)
    68127 : 000
    1166  : 010
    */
    //CPLD_REG_ASSIGN(gpio_i2c_1, 0x41, 0x01, 0);
    //CPLD_REG_ASSIGN(gpio_i2c_0, 0x41, 0x02, 1);

    //CPLD_REG_ASSIGN(bios_set,   0x03, 0x80, 7);
    //CPLD_REG_ASSIGN(bios_get, 0x03, 0x08, 3);
    //CPLD_REG_ASSIGN(cpudown,   0x62, 0x01, 0);

    /*
       : 6696 select ,
        0x15 写了0xff 后续再细改；
        bd->cpld_addr_i2c_sel[5]寄存器粗暴地只关心低三位，没有顾及bit [4:6]是68127和1166相关bit，不过I2C同时只能一路访问，所以没影响，后续细改。
    */
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xa4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 1]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xa2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_MAX6696 + 2]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xa3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_EEPROM]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x80), STEP_OVER);
    /*
        I2C_DEV_CPUBMC_EEPROM, eeprom shared in between cpu and bmc
        I2C_DEV_BMC_EEPROM, reserved for bmc, no need to access on cpu side
    */
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_CPUBMC_EEPROM]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 80), STEP_OVER);
    /* ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_BMC_EEPROM]),        STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 80), STEP_OVER);
    */
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 0]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 1]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd1), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 2]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd2), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 3]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 4]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd4), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_FAN + 5]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xd5), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 0]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x90), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_PSU + 1]),    STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x91), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_INA219 + 0]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x92), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_INA219 + 1]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x93), STEP_OVER);

    //ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127]), STEP_CNT(4), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0x04), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[1], 0x04), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_i2c_sel[5], 0x00), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ISL68127]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc1),  STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ADM1166]),   STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc0), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_ADM1166 + 1]), STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc2), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_TPS53659]),  STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_TPS53622]),  STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xc3), STEP_OVER);
    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_N287]),      STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xb3), STEP_OVER);

    ret += i2c_select_steps_init(&(bd->i2c_select_table[I2C_DEV_RTC_4337]),  STEP_CNT(2), STEP(OP_TYPE_WR_CPLD, bd->cpld_addr_main_i2c_sel, 0xa0), STEP_OVER);

    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "board static data init failed: ret=%d!", ret);
        bd->initialized = 0;
    }
    else
    {
        bd->initialized = 1;
    }
    return ret;
}

/**************************************/
//cpld on switch board
size_t bsp_cpld_get_size(void)
{
    return bsp_get_board_data()->cpld_size_board;
    //return main_board_data.cpld_size_board;
}
size_t bsp_get_bios_cpld_size(void)
{
    return bsp_get_board_data()->cpld_size_bios;
}

//sub slot cpld size, slot_index from 1
size_t bsp_cpld_get_slot_size(int slot_index)
{
    if (slot_index >= MAX_SLOT_NUM)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld_get_slot_size slot_index %d (>= %d)", slot_index, MAX_SLOT_NUM);
        return 0;
    }
    return bsp_get_board_data()->cpld_size_slot[slot_index];
    //return main_board_data.cpld_size_slot[slot_index];
}

//cpld on cpu board
size_t bsp_get_cpu_cpld_size(void)
{
    return bsp_get_board_data()->cpld_size_cpu;
    //return main_board_data.cpld_size_cpu;
}

//只能由 read_byte/write_byte/read_part/write_part/set_bit/get_bit调用，不能由其它上层调用。
//read_write_byte和read_write_part是同一级函数
static u8 bsp_cpld_read_absolue_address(volatile void *addr)
{
    if (bsp_get_board_data()->cpld_access_type == IO_REMAP)
    {
        return readb(addr);
    }
    else
    {
#if DIRECT_IO_SUPPORT
        return inb((long)addr);
#endif
    }
    return (u8)0;
}

static void bsp_cpld_write_absolue_address(volatile void *addr, u8 value)
{
    if (bsp_get_board_data()->cpld_access_type == IO_REMAP)
    {
        writeb(value, addr);
    }
    else
    {
#if DIRECT_IO_SUPPORT
        outb(value, (unsigned long)addr);
#endif
    }
    return;
}

//access board cpld, param offset is the offset bytes which based on 0x2200.
int bsp_cpld_read_byte(OUT u8 *value, IN u16 offset)
{
    if (offset > bsp_get_board_data()->cpld_size_board)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_board);
        return -EINVAL;
    }
    mutex_lock(&bsp_cpld_lock);
    *value = bsp_cpld_read_absolue_address(offset + g_u64Cpld_addr);
    mutex_unlock(&bsp_cpld_lock);
    return ERROR_SUCCESS;
}

//write board cpld
int bsp_cpld_write_byte(IN u8 value, IN u16 offset)
{
    if (offset > bsp_get_board_data()->cpld_size_board)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_board);
        return -EINVAL;
    }

    mutex_lock(&bsp_cpld_lock);
    bsp_cpld_write_absolue_address(offset + g_u64Cpld_addr, value);
    mutex_unlock(&bsp_cpld_lock);

    return ERROR_SUCCESS;
}

//读取cpld寄存器部分域，value = (v & mask) >> shift_bits
int bsp_cpld_read_part(OUT u8 *value, IN u16 offset, IN u8 mask, IN u8 shift_bits)
{
    if (offset > bsp_get_board_data()->cpld_size_board)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_board);
        return -EINVAL;
    }
    mutex_lock(&bsp_cpld_lock);
    *value = bsp_cpld_read_absolue_address(offset + g_u64Cpld_addr);
    *value = (*value & mask) >> shift_bits;
    mutex_unlock(&bsp_cpld_lock);
    return ERROR_SUCCESS;
}

//写cpld寄存器部分域
int bsp_cpld_write_part(IN u8 value, IN u16 offset, IN u8 mask, IN u8 shift_bits)
{
    u8 temp = 0;
    int ret = ERROR_SUCCESS;

    if (offset > bsp_get_board_data()->cpld_size_board || shift_bits > 7)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x shift_bits %d", offset, shift_bits);
        ret = -EINVAL;
        goto exit_no_lock;
    }
    mutex_lock(&bsp_cpld_lock);

    temp = bsp_cpld_read_absolue_address(offset + g_u64Cpld_addr);
    temp &= 0xff;
    temp &= (~mask);
    temp |= (u8)((u32)value << shift_bits);
    bsp_cpld_write_absolue_address(offset + g_u64Cpld_addr, temp);
    mutex_unlock(&bsp_cpld_lock);

exit_no_lock:
    return ret;
}


int bsp_cpu_cpld_read_part(OUT u8 *value, IN u16 offset, IN u8 mask, IN u8 shift_bits)
{
    if (offset > bsp_get_board_data()->cpld_size_cpu)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_cpu);
        return -EINVAL;
    }
    mutex_lock(&bsp_cpld_lock);
    *value = bsp_cpld_read_absolue_address(offset + g_u64CpuCpld_addr);
    *value = (*value & mask) >> shift_bits;
    mutex_unlock(&bsp_cpld_lock);
    return ERROR_SUCCESS;
}

int bsp_cpu_cpld_write_part(IN u8 value, IN u16 offset, IN u8 mask, IN u8 shift_bits)
{
    u8 temp = 0;
    int ret = ERROR_SUCCESS;

    if (offset > bsp_get_board_data()->cpld_size_cpu || shift_bits > 7)
    {
        DBG_ECHO(DEBUG_ERR, "param err: : cpld offset 0x%x, shift_bits %d", offset, shift_bits);
        ret = -EINVAL;
        goto exit_no_lock;
    }
    mutex_lock(&bsp_cpld_lock);

    temp = bsp_cpld_read_absolue_address(offset + g_u64CpuCpld_addr);
    temp &= 0xff;
    temp &= (~mask);
    temp |= (u8)((u32)value << shift_bits);
    bsp_cpld_write_absolue_address(offset + g_u64CpuCpld_addr, temp);

    mutex_unlock(&bsp_cpld_lock);

exit_no_lock:

    return ret;
}



// board cpld
int bsp_cpld_get_bit(u16 cpld_offset, u8 bit, u8 *value)
{
    int ret = ERROR_SUCCESS;

    if (bit > 7)
    {
        DBG_ECHO(DEBUG_DBG, "param err: offset 0x%x bit %d > 7", cpld_offset, bit);
        return -EINVAL;
    }

    ret = bsp_cpld_read_part(value, cpld_offset, 1 << bit, bit);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "cpld_get_bit failed");

exit:
    return ret;
}

//bsp_cpld_set_bit调用bsp_cpld_write_part，不用加锁
int bsp_cpld_set_bit(u16 cpld_offset, u8 bit, u8 value)
{
    //u8 val = 0;
    int ret = ERROR_SUCCESS;

    if (((0 != value) && (1 != value)) || (bit > 7))
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld_offset=0x%x bit=%d value=0x%x", cpld_offset, bit, value);
        return -EINVAL;
    }
    ret = bsp_cpld_write_part(value, cpld_offset, 1 << bit, bit);

    return ret;
}

//read cpu card cpld space
int bsp_cpu_cpld_read_byte(OUT u8 *value, IN u16 offset)
{
    if (offset > bsp_get_board_data()->cpld_size_cpu)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_cpu);
        return -EINVAL;
    }
    mutex_lock(&bsp_cpld_lock);
    *value = bsp_cpld_read_absolue_address(offset + g_u64CpuCpld_addr);
    mutex_unlock(&bsp_cpld_lock);
    //DBG_ECHO(DEBUG_DBG, "cpu cpld read offset 0x%x value 0x%x", offset, *value);
    return ERROR_SUCCESS;
}
//write cpu card cpld space
int bsp_cpu_cpld_write_byte(IN  u8 value, IN u16 offset)
{
    if (offset > bsp_get_board_data()->cpld_size_cpu)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_cpu);
        return -EINVAL;
    }
    //DBG_ECHO(DEBUG_DBG, "cpu cpld write offset 0x%x value 0x%x", offset, value);
    mutex_lock(&bsp_cpld_lock);
    bsp_cpld_write_absolue_address(offset + g_u64CpuCpld_addr, value);
    mutex_unlock(&bsp_cpld_lock);
    return ERROR_SUCCESS;
}

//read bios cpld space
int bsp_bios_cpld_read_byte(OUT u8 *value, IN u16 offset)
{
    if (offset > bsp_get_board_data()->cpld_size_bios)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_cpu);
        return -EINVAL;
    }
    mutex_lock(&bsp_cpld_lock);
    *value = bsp_cpld_read_absolue_address(offset + g_u64BiosCpld_addr);
    mutex_unlock(&bsp_cpld_lock);
    //DBG_ECHO(DEBUG_DBG, "cpu cpld read offset 0x%x value 0x%x", offset, *value);
    return ERROR_SUCCESS;
}
//write bios cpld space
int bsp_bios_cpld_write_byte(IN  u8 value, IN u16 offset)
{
    if (offset > bsp_get_board_data()->cpld_size_bios)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_cpu);
        return -EINVAL;
    }
    //DBG_ECHO(DEBUG_DBG, "cpu cpld write offset 0x%x value 0x%x", offset, value);
    mutex_lock(&bsp_cpld_lock);
    bsp_cpld_write_absolue_address(offset + g_u64BiosCpld_addr, value);
    mutex_unlock(&bsp_cpld_lock);
    return ERROR_SUCCESS;
}

//读取cpld寄存器部分域，value = (v & mask) >> shift_bits
int bsp_bios_cpld_read_part(OUT u8 *value, IN u16 offset, IN u8 mask, IN u8 shift_bits)
{
    if (offset > bsp_get_board_data()->cpld_size_board)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_board);
        return -EINVAL;
    }
    mutex_lock(&bsp_cpld_lock);
    *value = bsp_cpld_read_absolue_address(offset + g_u64BiosCpld_addr);
    *value = (*value & mask) >> shift_bits;
    mutex_unlock(&bsp_cpld_lock);
    return ERROR_SUCCESS;
}

//写cpld寄存器部分域
int bsp_bios_cpld_write_part(IN u8 value, IN u16 offset, IN u8 mask, IN u8 shift_bits)
{
    u8 temp = 0;

    if (offset > bsp_get_board_data()->cpld_size_board || shift_bits > 7)
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x shift_bits %d", offset, shift_bits);
        return -EINVAL;
    }
    mutex_lock(&bsp_cpld_lock);
    temp = bsp_cpld_read_absolue_address(offset + g_u64BiosCpld_addr);
    temp &= 0xff;
    temp &= (~mask);
    temp |= (u8)((u32)value << shift_bits);
    bsp_cpld_write_absolue_address(offset + g_u64BiosCpld_addr, temp);
    mutex_unlock(&bsp_cpld_lock);


    return ERROR_SUCCESS;
}



//read subcard cpld space, slot_index from 1
int bsp_slot_cpld_read_byte(int slot_index, OUT u8 *value, IN u16 offset)
{
    if (offset > bsp_get_board_data()->cpld_size_slot[slot_index])
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_slot[slot_index]);
        return -EINVAL;
    }
    mutex_lock(&bsp_cpld_lock);
    *value = bsp_cpld_read_absolue_address(offset + g_u64SlotCpld_addr[slot_index]);
    mutex_unlock(&bsp_cpld_lock);
    //DBG_ECHO(DEBUG_DBG, "slot %d cpld read offset 0x%x value 0x%x", slot_index + 1, offset, *value);
    return ERROR_SUCCESS;
}

//write subcard cpld space,  slot_index from 1
int bsp_slot_cpld_write_byte(int slot_index, IN  u8 value, IN u16 offset)
{
    if (offset > bsp_get_board_data()->cpld_size_slot[slot_index])
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld offset 0x%x (> 0x%x)", offset, bsp_get_board_data()->cpld_size_slot[slot_index]);
        return -EINVAL;
    }
    //DBG_ECHO(DEBUG_DBG, "slot %d cpld write base/offset 0x%x/0x%x value 0x%x", slot_index + 1,  g_u64SlotCpld_addr[slot_index], offset, value);
    mutex_lock(&bsp_cpld_lock);
    bsp_cpld_write_absolue_address(offset + g_u64SlotCpld_addr[slot_index], value);
    mutex_unlock(&bsp_cpld_lock);
    return ERROR_SUCCESS;
}

// board cpld
int bsp_slot_cpld_get_bit(int slot_index, u16 cpld_offset, u8 bit, u8 *value)
{
    int ret = ERROR_SUCCESS;

    if (bit > 7)
    {
        DBG_ECHO(DEBUG_DBG, "param err: offset 0x%x bit %d > 7", cpld_offset, bit);
        return -EINVAL;
    }

    ret = bsp_slot_cpld_read_byte(slot_index, value, cpld_offset);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "read slot %d cpld offset 0x%x failed", slot_index, cpld_offset);
    *value = (*value >> bit) & 0x1;

exit:
    return ret;
}

//bsp_cpld_set_bit调用bsp_cpld_write_part，不用加锁
int bsp_slot_cpld_set_bit(int slot_index, u16 cpld_offset, u8 bit, u8 value)
{
    u8 temp_value = 0;
    int ret = ERROR_SUCCESS;

    if (((0 != value) && (1 != value)) || (bit > 7))
    {
        DBG_ECHO(DEBUG_ERR, "param err: cpld_offset=0x%x bit=%d value=0x%x", cpld_offset, bit, value);
        return -EINVAL;
    }

    mutex_lock(&bsp_slot_cpld_lock[slot_index]);
    ret = bsp_slot_cpld_read_byte(slot_index, &temp_value, cpld_offset);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "read slot %d cpld offset 0x%x failed", slot_index, cpld_offset);
    temp_value = (value == 1) ? ((u8)(1 << bit) | temp_value) : ((~((u8)(1 << bit))) & temp_value);

    ret = bsp_slot_cpld_write_byte(slot_index, temp_value, cpld_offset);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "write slot %d cpld offset 0x%x failed", slot_index, cpld_offset);
exit:
    mutex_unlock(&bsp_slot_cpld_lock[slot_index]);
    return ret;
}

//子卡cpld初始化, slot  是子卡索引，从1开始
int bsp_slot_cpld_init(int slot_index)
{
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_slot_cpld_init get bd failed");
        return -EINVAL;
    }
    if (slot_index >= MAX_SLOT_NUM)
    {
        DBG_ECHO(DEBUG_ERR, "param err: invalid slot index %d (>= %d)", slot_index, MAX_SLOT_NUM);
        return -EINVAL;
    }
    if (bd->cpld_size_slot[slot_index] <= 0)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: cpld size for slot %d is %d, invalid size.", slot_index, bd->cpld_size_slot[slot_index]);
        return -EINVAL;
    }

    mutex_init(&bsp_slot_cpld_lock[slot_index]);

    if (bd->cpld_access_type == IO_REMAP)
    {
        g_u64SlotCpld_addr[slot_index] = ioremap(bd->cpld_base_address + bd->cpld_hw_addr_slot[slot_index], bd->cpld_size_slot[slot_index]);
    }
    else
    {
#if DIRECT_IO_SUPPORT
        g_u64SlotCpld_addr[slot_index] = (void *)bd->cpld_hw_addr_slot[slot_index];
#endif
    }
    DBG_ECHO(DEBUG_INFO, "slot %d cpld_address set to %p~%p\n", slot_index + 1, g_u64SlotCpld_addr[slot_index], g_u64SlotCpld_addr[slot_index] + bd->cpld_size_slot[slot_index]);

    return ERROR_SUCCESS;
}

int bsp_cpld_init(void)
{
    int slot_index = 0;
    char *access_type = NULL;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_cpld_init get bd failed");
        return -EINVAL;
    }

    mutex_init(&bsp_cpld_lock);
    mutex_init(&bsp_mac_inner_temp_lock);
    mutex_init(&bsp_mac_width_temp_lock);
    mutex_init(&bsp_manu_lock);

    if (bd->cpld_access_type == IO_REMAP)
    {
        g_u64Cpld_addr = ioremap(bd->cpld_base_address + bd->cpld_hw_addr_board, bd->cpld_size_board);
        g_u64CpuCpld_addr = ioremap(bd->cpld_base_address + bd->cpld_hw_addr_cpu, bd->cpld_size_cpu);
        DBG_ECHO(DEBUG_INFO, "bios cpld %#lx\n", (bd->cpld_base_address + bd->cpld_hw_addr_bios));
        g_u64BiosCpld_addr = ioremap(bd->cpld_base_address + bd->cpld_hw_addr_bios, bd->cpld_size_bios);
        access_type = "ioremap";
    }
    else
    {
#if DIRECT_IO_SUPPORT
        g_u64Cpld_addr = (void *)bd->cpld_hw_addr_board;
        g_u64CpuCpld_addr = (void *)bd->cpld_hw_addr_cpu;
        access_type = "inout set";
#endif
    }

    DBG_ECHO(DEBUG_INFO, "  cpu cpld %s to %p~%p\n", access_type, g_u64CpuCpld_addr, g_u64CpuCpld_addr + bd->cpld_size_cpu);
    DBG_ECHO(DEBUG_INFO, "board cpld %s to %p~%p\n", access_type, g_u64Cpld_addr, g_u64Cpld_addr + bd->cpld_size_board);
    DBG_ECHO(DEBUG_INFO, "bios  cpld %s to %p~%p\n", access_type, g_u64BiosCpld_addr, g_u64BiosCpld_addr + bd->cpld_size_bios);
    //每子卡上的cpld初始化
    for (slot_index = FIRST_EXP_BOARD; slot_index <= bd->slot_num; slot_index++)
    {
        ret = bsp_slot_cpld_init(slot_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "slot %d cpld init failed!", slot_index);
    }

exit:
    return ret;
}

int bsp_cpld_deinit(void)
{
    int slot_index;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_cpld_deinit get bd failed");
        return -EINVAL;
    }

    if (bd->cpld_access_type == IO_REMAP)
    {
        iounmap(g_u64Cpld_addr);
        iounmap(g_u64CpuCpld_addr);
        iounmap(g_u64BiosCpld_addr);

        DBG_ECHO(DEBUG_INFO, "board cpld iounmap!");

        for (slot_index = FIRST_EXP_BOARD; slot_index <= bd->slot_num; slot_index++)
        {
            if (g_u64SlotCpld_addr[slot_index] != NULL)
            {
                iounmap(g_u64SlotCpld_addr[slot_index]);
                DBG_ECHO(DEBUG_INFO, "slot %d cpld iounmap done!", slot_index);
            }
        }
    }

    return ERROR_SUCCESS;
}

//cpld 还没有初始化时，尝试从0x2202位置获取板卡PCB类型数据
static int bsp_try_get_product_type_from_cpld(OUT u8 *pcb_type)
{
    u8 value_io_remap = 0x0;
    void *temp_remap = NULL;

#if DIRECT_IO_SUPPORT
    u8 value_io_inout = 0x0;
    value_io_inout = inb(DEFAULT_CPLD_BOARD_TYPE_OFFSET);
    if ((value_io_inout != 0) && (value_io_inout != 0xff))
    {
        *pcb_type = value_io_inout;
        DBG_ECHO(DEBUG_INFO, "get product type success by inb(), pcb_type=0x%x", value_io_inout)
        return ERROR_SUCCESS;
    }
#endif

    temp_remap = ioremap(DEFAULT_CPLD_BASE_ADDR_IOREMAP + DEFAULT_CPLD_BOARD_TYPE_OFFSET, 0x2);
    value_io_remap = readb(temp_remap);
    iounmap(temp_remap);
    if ((value_io_remap != 0) && (value_io_remap != 0xff))
    {
        *pcb_type = value_io_remap;
        DBG_ECHO(DEBUG_INFO, "get product type success by ioremap, pcb_type=0x%x", value_io_remap)
        return ERROR_SUCCESS;
    }
    DBG_ECHO(DEBUG_ERR, "try to get product type failed!")

    return -ENXIO;
}

//get product type
int bsp_get_product_type(OUT int *pdt_type)
{
    int ret = ERROR_SUCCESS;
    u8 cpld_pcb_type = 0;

    if (bsp_product_type == PDT_TYPE_BUTT)
    {
        //cpld还没有初始化, 尝试获取板卡PCB类型
        ret = bsp_try_get_product_type_from_cpld(&cpld_pcb_type);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "try to get product type from cpld failed!");
        DBG_ECHO(DEBUG_INFO, "got cpld pcb type 0x%x successed!", cpld_pcb_type);
        switch (cpld_pcb_type)
        {
            case 0x1:
                bsp_product_type = PDT_TYPE_TCS81_120F_1U;
                break;
            case 0x2:
                bsp_product_type = PDT_TYPE_TCS83_120F_4U;
                break;
            case 0x7:
            case 0xa:
                bsp_product_type = PDT_TYPE_LS_9855_48CD8D_W1;
                break;
            case 0x8:
            case 0xb:
                bsp_product_type = PDT_TYPE_LS_9825_64D_W1;
                break;
            case 0x4:
                bsp_product_type = PDT_TYPE_LS_9855_32CDF_W1;
                break;
            default:
                bsp_product_type = PDT_TYPE_TCS81_120F_1U;
                break;
        }
    }
    * pdt_type = bsp_product_type;
exit:
    return ret;
}

int optoe_port_index_convert(int optoe_port_index)
{
    int port_num = 0;
    int switch_type;
    if (bsp_get_product_type(&switch_type) != ERROR_SUCCESS)
    {
        goto exit;
    }

    switch (switch_type)
    {
        case PDT_TYPE_TCS81_120F_1U:
            if (optoe_port_index > 104) /* 48x2 + 8 */
            {
                goto exit;
            }
            if (96 >= optoe_port_index)
            {
                port_num = (0 == optoe_port_index % 2) ? (optoe_port_index) : (optoe_port_index + 1);
                return (port_num / 2);
            }
            else
            {
                port_num = optoe_port_index - 96 + 48;
                return port_num;
            }
            break;
        default:
            return optoe_port_index;
    }
exit:
    return -ENODEV;
}

//获取子卡类型
int bsp_get_card_product_type(OUT int *card_pdt_type, int slot_index)
{
    int ret = ERROR_SUCCESS;
    u8 cpld_pdt_type = 0;

    //board_static_data *bd = bsp_get_board_data();
    ret = bsp_slot_cpld_read_byte(slot_index, &cpld_pdt_type, DEFAULT_SUBCARD_BOARD_TYPE_OFFSET);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "get card product type failed for slot index %d", slot_index);
        return ret;
    }
    switch (cpld_pdt_type)
    {
        case 0x0:
            *card_pdt_type = PDT_TYPE_TCS83_120F_32H_SUBCARD;
            ret = ERROR_SUCCESS;
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "unknown card type %d for slot index %d", cpld_pdt_type, slot_index);
            ret = -EINVAL;
            break;
    }
    return ret;
}

/***
char * bsp_get_product_name_string(int product_type)
{
    char * pdt_name = NULL;

    switch (product_type)
    {
        case PDT_TYPE_TCS81_120F_1U:
        {
            pdt_name = "TCS81-120F";
            break;
        }
        case PDT_TYPE_TCS83_120F_4U:
        {
            pdt_name = "TCS83-120F";
            break;
        }
        case PDT_TYPE_TCS83_120F_32H_SUBCARD:
        {
            pdt_name = "TCS83-120-32CQ";
            break;
        }
        default:
        {
            pdt_name = "Unknown";
            break;
        }
    }
    return pdt_name;
}***/


/*******************************
start:
basic i2c device read/write method
eeprom, 954x, lm75, max669x, sfp...

********************************/
//包装i2c_smbus_xfer，用于记录i2c访问记录
typedef enum
{
    i2c_flag_BYTE = 0,
    i2c_flag_BYTE_DATA,
    i2c_flag_WORD_DATA,
    i2c_flag_BLOCK_DATA,
    i2c_flag_24LC128_eeprom,
    i2c_flag_eeprom_rw_byte,
    i2c_flag_LM75_get_temp,
    i2c_flag_ADM116x_rw_SecV,/*当前仅用到读，后续需要可增加支持写*/
    i2c_flag_ADM116x_rw_VerS,/*当前仅用到读，后续需要可增加支持写*/
    i2c_flag_ADM116x_rw_FauC,
    i2c_flag_ADM116x_rw_SE,
    i2c_flag_ISL68127_rw_select1,/*当前硬件设计仅用到page1，后续需要可以扩展*/
    i2c_flag_Max6696_rw_select0,
    i2c_flag_Max6696_rw_select1,
    i2c_flag_Max6696_rw_select2,
    i2c_flag_psu_eeprom_r_byte,
    i2c_flag_RA228_rw_select1,/* use page 0*/
    i2c_flag_2addr_eeprom_rw_byte,
    i2c_flag_BUTT
} I2C_FLAG_E;

/*
 *当内核操作失败时，尝试
 *1.对slave进行复位操作（用cpld模拟9个时钟脉冲）
 *2.对CPU侧进行软复位
 *注意：需要调用者进行锁的处理
 */
static void bsp_i2c_reset(void)
{
    int i = 0;
    int ret = 0;
    u8 temp_value = 0;
    struct pci_dev *pdev = pci_get_domain_bus_and_slot(0, 0x00, PCI_DEVFN(0x1f, 0x03));
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_i2c_reset get bd failed");
        return;
    }

    udelay(g_i2crst_time);
    //CPU侧SMBUS软复位
    ret = pci_read_config_byte(pdev, 0x40, &temp_value);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "i2c_reset: pci_read_config_byte failed ret=%d", ret);
        //return; 是否需要返回？
    }
    temp_value |= 8;

    ret = pci_write_config_byte(pdev, 0x40, temp_value);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "i2c_reset: pci_write_config_byte value=%d failed ret=%d", temp_value, ret);
        //return;  是否需要返回
    }

    //slave复位
    for (i = 0; i < 9; i++)
    {
        //write high
        bsp_cpu_cpld_read_byte(&temp_value, bd->cpld_addr_gpio_i2c_0);
        temp_value = (1 << bd->cpld_offs_gpio_i2c_0) | temp_value;
        bsp_cpu_cpld_write_byte(temp_value, bd->cpld_addr_gpio_i2c_0);
        udelay(5);
        //write low
        bsp_cpu_cpld_read_byte(&temp_value, bd->cpld_addr_gpio_i2c_0);
        temp_value = (~(1 << bd->cpld_offs_gpio_i2c_0)) & temp_value;
        bsp_cpu_cpld_write_byte(temp_value, bd->cpld_addr_gpio_i2c_0);
        udelay(5);
    }

    //udelay(1000);
    udelay(g_i2crst_time);
    DBG_ECHO_I2C(DEBUG_NO, "bsp i2c reset");
}

/*
 *基本的i2c操作函数
 * 本函数包装i2c_smbus_xfer，在失败的时候进行CPU侧和Slave的复位操作，同时记录失败的i2c访问操作
 * 注意：需要调用者进行锁的处理
 */
static int bsp_i2c_smbus_xfer(struct i2c_adapter *adapter, u16 addr, unsigned short flags, char read_write, u8 command, int protocol, union i2c_smbus_data *data)
{
    int ret = ERROR_SUCCESS;
    int ret_bak = ERROR_SUCCESS;
    int curr_index = 0;
    int try_count = 0;
    int count_temp = 3;

    if ((adapter == NULL) || (data == NULL))
    {
        DBG_ECHO(DEBUG_ERR, "param err: adapter == NULL or data == NULL");
        return -EINVAL;
    }

    if ((current_i2c_path_id >= I2C_DEV_PSU) && (current_i2c_path_id < I2C_DEV_PSU_BUTT))
    {
        count_temp = g_psuretry_count;
    }

    while (try_count < count_temp)
    {
        ret = i2c_smbus_xfer(adapter, addr, flags, read_write, command, protocol, data);
        if (ret == ERROR_SUCCESS)
        {
            break;
        }
        else
        {
            ret_bak = ret;
            //reset复位尝试恢复
            bsp_i2c_reset();
            DBG_ECHO(DEBUG_INFO, "i2c reset try_count=%d failed(%d) addr=0x%x flags=%d, read_write=%d, command=0x%x, protocol=%d", try_count, ret, addr, flags, read_write, command, protocol);
            //DBG_ECHO(DEBUG_ERR, "i2c_smbus_xfer failed(%d), retrys=%d addr=0x%x flags=%d, read_write=%d, command=0x%x, protocol=%d", ret, retrys, addr, flags, read_write, command, protocol);
            //msleep(I2C_RW_MAX_RETRY_DELAY_MS);
        }
        try_count++;
    }

    if (try_count != 0)
    {
        curr_index = i2c_diag_info.curr_index;
        i2c_diag_info.record[curr_index].error_code = ret_bak;
        i2c_diag_info.record[curr_index].i2c_addr = addr;
        i2c_diag_info.record[curr_index].path_id = current_i2c_path_id;
        i2c_diag_info.record[curr_index].inner_addr = command;
        i2c_diag_info.record[curr_index].protocol = protocol;
        i2c_diag_info.record[curr_index].read_write = read_write;
        i2c_diag_info.record[curr_index].retry_times = try_count;
        i2c_diag_info.record[curr_index].time_sec = ktime_get_real_seconds();
        i2c_diag_info.rec_count = (i2c_diag_info.rec_count < MAX_I2C_DIAG_RECORD_COUNT) ? (i2c_diag_info.rec_count + 1) : MAX_I2C_DIAG_RECORD_COUNT;
        i2c_diag_info.curr_index = (curr_index  < MAX_I2C_DIAG_RECORD_COUNT - 1) ? (curr_index + 1) : 0;
        i2c_diag_info.is_valid = TRUE;

        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "i2c_smbus_xfer failed(%d) try_count=%d dev index=0x%x addr=0x%x flags=%d, read_write=%d, command=0x%x, protocol=%d", ret, try_count, current_i2c_path_id, addr, flags, read_write, command, protocol);
        }
    }
    return ret;
}

int bsp_enable_slot_all_9548(int slot_index)
{
    int ret = ERROR_SUCCESS;
    int i = 0;
    board_static_data *bd = bsp_get_slot_data(slot_index);
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_enable_slot_all_9548 get bd failed");
        return -EINVAL;
    }
    for (i = 0; i < MAX_PCA9548_NUM; i++)
    {
        if (bd->cpld_addr_9548_rst[i] == 0)
        {
            continue;
        }
        if (slot_index == MAIN_BOARD)
        {
            ret = bsp_cpld_set_bit(bd->cpld_addr_9548_rst[i], bd->cpld_offs_9548_rst[i], 0);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "reset 9548[%d] failed!", i);
        }
        else
        {
            ret = bsp_slot_cpld_set_bit(slot_index, bd->cpld_addr_9548_rst[i], bd->cpld_offs_9548_rst[i], 0);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "reset slot %d 9548[%d] failed!", slot_index, i);
        }
    }

    udelay(1000);

    for (i = 0; i < MAX_PCA9548_NUM; i++)
    {
        if (bd->cpld_addr_9548_rst[i] == 0)
        {
            continue;
        }
        if (slot_index == MAIN_BOARD)
        {
            ret = bsp_cpld_set_bit(bd->cpld_addr_9548_rst[i], bd->cpld_offs_9548_rst[i], 1);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "reset 9548[%d] failed!", i);
        }
        else
        {
            ret = bsp_slot_cpld_set_bit(slot_index, bd->cpld_addr_9548_rst[i], bd->cpld_offs_9548_rst[i], 1);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "reset slot %d 9548[%d] failed!", slot_index, i);
        }
    }

    udelay(1000);

exit:
    return ret;
}

int bsp_enable_slot_all_9545(int slot_index)
{
    int ret = ERROR_SUCCESS;
    int i = 0;
    board_static_data *bd = bsp_get_slot_data(slot_index);
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: %s get bd failed", __FUNCTION__);
        return -EINVAL;
    }

    for (i = 0; i < MAX_PCA9545_NUM; i++)
    {
        if (bd->cpld_addr_9545_rst[i] == 0)
        {
            continue;
        }
        if (slot_index == MAIN_BOARD)
        {
            ret = bsp_cpld_set_bit(bd->cpld_addr_9545_rst[i], bd->cpld_offs_9545_rst[i], 0);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "reset 9545[%d] failed!", i);
        }
        else
        {
            ret = bsp_slot_cpld_set_bit(slot_index, bd->cpld_addr_9545_rst[i], bd->cpld_offs_9545_rst[i], 0);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "reset slot %d 9545[%d] failed!", slot_index, i);
        }
    }

    udelay(1000);

    for (i = 0; i < MAX_PCA9545_NUM; i++)
    {
        if (bd->cpld_addr_9545_rst[i] == 0)
        {
            continue;
        }
        if (slot_index == MAIN_BOARD)
        {
            ret = bsp_cpld_set_bit(bd->cpld_addr_9545_rst[i], bd->cpld_offs_9545_rst[i], 1);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "reset 9545[%d] failed!", i);
        }
        else
        {
            ret = bsp_slot_cpld_set_bit(slot_index, bd->cpld_addr_9545_rst[i], bd->cpld_offs_9545_rst[i], 1);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "reset slot %d 9545[%d] failed!", slot_index, i);
        }
    }

    udelay(1000);

exit:
    return ret;
}


/*9545选通
 *在基于器件的I2C操作之前调用，
 *由调用者保证相关的锁处理
 */
static int bsp_i2c_9545_write_byte(u16 dev_i2c_address, u16 inner_address, u8 value)
{
    union i2c_smbus_data temp_data;
    int status;
    (void)inner_address;

    status = bsp_i2c_smbus_xfer(smbus, dev_i2c_address, 0, I2C_SMBUS_WRITE, value, I2C_SMBUS_BYTE, &temp_data);

    CHECK_IF_ERROR(status, "9545 write address 0x%x value 0x%x failed", dev_i2c_address, value);

    return status;
}

/*9548选通
 *在基于器件的I2C操作之前调用，
 *由调用者保证相关的锁处理
 */
static int bsp_i2c_9548_write_byte(u16 dev_i2c_address, u16 inner_address, u8 value)
{
    union i2c_smbus_data temp_data;
    int status;
    (void)inner_address;

    status = bsp_i2c_smbus_xfer(smbus, dev_i2c_address, 0, I2C_SMBUS_WRITE, value, I2C_SMBUS_BYTE, &temp_data);

    CHECK_IF_ERROR(status, "9548 write address 0x%x value 0x%x failed", dev_i2c_address, value);
    return status;
}

static int bsp_select_i2c_device_with_device_table(I2C_DEVICE_E i2c_device_index)
{
    int step_count = 0;
    i2c_select_op_step *temp_step;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_select_i2c_device_with_device_table get bd failed");
        return -EINVAL;
    }

    if (i2c_device_index >= I2C_DEV_BUTT)
    {
        DBG_ECHO(DEBUG_ERR, "param err: i2c device index %d (>= %d)", i2c_device_index, I2C_DEV_BUTT);
        ret = -EINVAL;
        goto exit;
    }
    if (bd->i2c_select_table[i2c_device_index].valid != I2C_SELECT_STEPS_VALID)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: i2c select device %d steps is invalid!", i2c_device_index);
        ret = -EINVAL;
        goto exit;
    }

    temp_step = bd->i2c_select_table[i2c_device_index].step;

    for (step_count = 0; step_count < MAX_I2C_SEL_OP_STEPS_COUNT; step_count++)
    {
        switch (temp_step[step_count].op_type)
        {
            case OP_TYPE_WR_9545:
                ret = bsp_i2c_9545_write_byte(temp_step[step_count].i2c_dev_addr, 0, temp_step[step_count].op_value);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "operate 9545 failed, ret=%d", ret);
                break;
            case OP_TYPE_WR_9548:
                ret = bsp_i2c_9548_write_byte(temp_step[step_count].i2c_dev_addr, 0, temp_step[step_count].op_value);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "operate 9548 failed, ret=%d", ret);
                break;
            case OP_TYPE_WR_CPLD:
                ret = bsp_cpld_write_byte(temp_step[step_count].op_value, temp_step[step_count].cpld_offset_addr);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "operate cpld failed, ret=%d", ret);
                break;
            case OP_TYPE_WR_BIOS_CPLD:
                ret = bsp_bios_cpld_write_byte(temp_step[step_count].op_value, temp_step[step_count].cpld_offset_addr);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "operate bios cpld failed, ret=%d", ret);
                break;
            case OP_TYPE_WR_CPU_CPLD:
                ret = bsp_cpu_cpld_write_byte(temp_step[step_count].op_value, temp_step[step_count].cpld_offset_addr);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "operate cpu cpld failed, ret=%d", ret);
                break;
            case OP_TYPE_NONE:
                goto exit;
                break;
            default:
                DBG_ECHO(DEBUG_ERR, "i2c select op_type %d is invalid", temp_step[step_count].op_type);
                goto exit;
                break;
        }
    }

exit:
    if (ret != ERROR_SUCCESS)
    {
        u8 main_temp, t9548_rst, temp0, temp1, temp2, temp3;
        int ret;
        ret = bsp_cpld_read_byte(&t9548_rst, bd->cpld_addr_9548_rst[0]);
        ret = bsp_cpld_read_byte(&main_temp, bd->cpld_addr_main_i2c_sel);
        ret = bsp_cpld_read_byte(&temp0, bd->cpld_addr_i2c_sel[0]);
        ret = bsp_cpld_read_byte(&temp1, bd->cpld_addr_i2c_sel[1]);
        ret = bsp_cpld_read_byte(&temp2, bd->cpld_addr_i2c_sel[2]);
        ret = bsp_cpld_read_byte(&temp3, bd->cpld_addr_i2c_sel[3]);

        DBG_ECHO(DEBUG_ERR, "i2c select device %d failed, ret=%d 9548_rst=0x%02x main_se=0x%02x sel[0]=0x%02x sel[1]=0x%02x sel[2]=0x%02x sel[3]=0x%02x",
                 i2c_device_index, ret,
                 t9548_rst,
                 main_temp,
                 temp0,
                 temp1,
                 temp2,
                 temp3
                );

    }

    return ret;
}

/*lock i2c path, when access i2c
 *这里本身不考虑锁，认为调用者统一考虑了
 */
static int i2c_select(I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    int slot_index = MAIN_BOARD;

    if (I2C_DEV_BUTT == i2c_device_index)
    {
        return ret;
    }

    msleep(g_i2c_select_time);

    //reset for optic i2c hang up
    if ((i2c_device_index >= I2C_DEV_OPTIC_IDX_START) && (i2c_device_index < I2C_DEV_OPTIC_BUTT))
    {
        if (bsp_enable_slot_all_9545(slot_index) != ERROR_SUCCESS)
        {

            DBG_ECHO(DEBUG_ERR, "reset 9545 for i2c dev %d failed", i2c_device_index);
        }
        if (bsp_enable_slot_all_9548(slot_index) != ERROR_SUCCESS)
        {

            DBG_ECHO(DEBUG_ERR, "reset 9548 for i2c dev %d failed", i2c_device_index);
        }
    }

    ret = bsp_select_i2c_device_with_device_table(i2c_device_index);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "i2c path switching to device index=%d failed", i2c_device_index);
    }

    msleep(g_i2c_select_time);

    return ret;
}


/*器件的I2C操作函数
 * 对bsp_i2c_smbus_xfer函数的加锁操作
 * 同时与选通处理进行配合
 * 支持的操作：
 * I2C_SMBUS_BYTE_DATA  单/多字节读写
 * I2C_SMBUS_WORD_DATA  单word读写
 * I2C_SMBUS_I2C_BLOCK_DATA 读写
 */
#define  I2C_SMBUS_PROTOCOL_BUTT 255
int bsp_h3c_i2c_smbus_xfer(u16 addr, unsigned short flags, char read_write, u16 command, int byte_count, void *dataval, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    int protocol = I2C_SMBUS_PROTOCOL_BUTT;  //无效数
    union i2c_smbus_data temp_data = {0};
    u8 valuetmp = 0;
    int loop_count;
    int mod;
    int temp_read_bytes = 0;
    u16 temp_from_address;
    u8 *data_pointer;
    int retry = 0;
    u8 config = 0;
    int i, uiTry;
    unsigned short usEraseAddr = 0;
    u8 temp_value = 0;
    u8 data[128] = {0};
    u8 ucBuf[2] = {0};
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_h3c_i2c_smbus_xfer get bd failed");
        return -EINVAL;
    }
    if ((dataval == NULL))
    {
        DBG_ECHO(DEBUG_ERR, "param err: dataval is NULL");
        return -EINVAL;
    }

    if ((i2c_device_index >= I2C_DEV_OPTIC_IDX_START) && (i2c_device_index < I2C_DEV_OPTIC_BUTT))
    {
        if (1 == bd->dom_exist)   //因为涉及锁操作，所以建议调用这个select之前先判断，需要的话再调用。这个可以增加到函数调用说明中
        {
            DBG_ECHO(DEBUG_ERR, "check fail: access by dom rather than i2c_select");
            return -EPERM;
        }
    }

    mutex_lock(&bsp_i2c_path_lock);
    current_i2c_path_id = i2c_device_index;

    ret = i2c_select(i2c_device_index);

    CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c select dev index 0x%x failed", i2c_device_index);

    switch (flags)
    {
        case i2c_flag_BYTE:
            protocol = I2C_SMBUS_BYTE;
            break;
        case i2c_flag_BYTE_DATA:
            protocol = I2C_SMBUS_BYTE_DATA;
            break;
        case i2c_flag_WORD_DATA:
            protocol = I2C_SMBUS_WORD_DATA;
            break;
        case i2c_flag_BLOCK_DATA:
        case i2c_flag_2addr_eeprom_rw_byte:
            protocol = I2C_SMBUS_I2C_BLOCK_DATA;
            break;
        case i2c_flag_24LC128_eeprom:
            if (I2C_SMBUS_WRITE == read_write)
            {
                valuetmp = *(u8 *)dataval;
                temp_data.word = (((u16)valuetmp) << 8) | (command & 0xff);
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command >> 8), I2C_SMBUS_WORD_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "eeprom 24LC128 write addr:0x%x, command:0x%x, dataval:0x%x failed", addr, command, valuetmp);

#if 0
                /*下面的操作，根据资料应该不需要，跟yinlonghui确认也是可以去掉的*/
                temp_data.byte = 0;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command >> 8), I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "eeprom 4LC128 write failed, addr:0x%x, command:0x%x, dataval:0x%x", addr, command, valuetmp);
#endif
            }
            else
            {
                temp_data.byte = (u8)(command & 0xff);  //for 24LC128, the inner address is combined by 2 bytes(not 1). temp_data contains the lower address byte.
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command >> 8), I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "eeprom 24LC128 read addr:0x%x, command:0x%x failed", addr, command);

                protocol = I2C_SMBUS_BYTE;
                //command = 0;
            }
            break;
        case i2c_flag_eeprom_rw_byte:
        case i2c_flag_psu_eeprom_r_byte:
            if (I2C_SMBUS_WRITE == read_write)
            {
                temp_data.byte = *(u8 *)dataval;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command), I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "eeprom write addr:0x%x, command:0x%x, dataval:0x%x failed", addr, command, *(u8 *)dataval);

#if 0
                /*下面的操作，根据资料应该不需要，跟yinlonghui确认也是可以去掉的*/
                temp_data.byte    = 0;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command >> 8), I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "eeprom write failed, addr:0x%x, command:0x%x, dataval:0x%x", addr, command, *(u8 *)dataval);
#endif
            }
            else
            {
                protocol = I2C_SMBUS_I2C_BLOCK_DATA;
            }
            break;
        case i2c_flag_ADM116x_rw_SecV:
            /*select channel*/
            temp_data.byte = REG_ADM1166_RRSEL1_VALUE;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_RRSEL1, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "select channel 1 addr:0x%x, command:0x%x failed", addr, command);
            temp_data.byte = REG_ADM1166_RRSEL2_VALUE;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_RRSEL2, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "select channel 2 addr:0x%x, command:0x%x failed", addr, command);

            if (7 >= command)
            {
                temp_data.byte = ~(1 << command);
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_RRSEL1, I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "failed i2c write dev_addr:0x%x reg_addr:0x%x", addr, REG_ADDR_ADM1166_RRSEL1);
            }
            else
            {
                //假设后续的电压channel在一个枚举中
                temp_data.byte = ~(1 << (command - 8));
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_RRSEL2, I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "failed i2c write dev_addr:0x%x reg_addr:0x%x", addr, REG_ADDR_ADM1166_RRSEL2);
            }

            temp_value = ADM1166_RRCTRL_REG_GO;
            temp_data.byte = temp_value;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_RRCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x read ctrl addr:0x%x, command:0x%x failed", addr, command);
            /*loop read 0x82=0x00*/
            do
            {
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, REG_ADDR_ADM1166_RRCTRL, I2C_SMBUS_BYTE, &temp_data);
                data[0] = temp_data.byte;
                CHECK_IF_ERROR_GOTO_EXIT(ret, "failed i2c write dev_addr:0x%x reg_addr:0x%x", addr, REG_ADDR_ADM1166_RRCTRL);
                if (ADM1166_RRCTRL_REG_RESET == data[0])
                {
                    break;
                }
                mdelay(1);
            } while (retry++ < 500);
            CHECK_IF_ERROR_GOTO_EXIT(data[0], "loop read 0x82=0 faild, retry:%d", retry);

            temp_value = ADM1166_RRCTRL_REG_STOPWRITE;
            temp_data.byte = temp_value;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_RRCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);

            CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write ctrl addr:0x%x, command:0x%x failed", addr, command);
            protocol = I2C_SMBUS_I2C_BLOCK_DATA;
            break;
        case i2c_flag_ADM116x_rw_VerS:
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, REG_ADDR_ADM1166_VSCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);

            CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x read ctrl addr:0x%x, command:0x%x failed", addr, command);

            temp_value = temp_data.byte;
            temp_data.byte = 0x01;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_VSCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write ctrl addr:0x%x, command:0x%x failed", addr, command);

            temp_data.byte = (u8)(command & 0xff);
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, command >> 8, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write ctrl addr:0x%x, command:0x%x failed", addr, command);
            protocol = I2C_SMBUS_I2C_BLOCK_DATA;
            break;
        case i2c_flag_ADM116x_rw_FauC:
            if (I2C_SMBUS_READ == read_write)
            {
                //stopwrite
                temp_value = 0x01;
                temp_data.byte = temp_value;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_BBCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write REG_ADDR_ADM1166_BBCTRL addr:0x%x, command:0x%x failed", addr, command);
                protocol = I2C_SMBUS_I2C_BLOCK_DATA;
            }
            else
            {
                temp_data.byte = 0x01;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_UPDCFG,  I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x first write REG_ADDR_ADM1166_UPDCFG addr:0x%x, command:0x%x failed", addr, command);

                //STOPWRITE to 0x93
                temp_data.byte = 0x01;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_SECTRL,  I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write REG_ADDR_ADM1166_SECTRL addr:0x%x, command:0x%x failed", addr, command);

                //STOPWRITE to 0x9c
                temp_data.byte = 0x01;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_BBCTRL,  I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x first write REG_ADDR_ADM1166_BBCTRL addr:0x%x, command:0x%x failed", addr, command);

                temp_data.byte = 0x05;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_UPDCFG,  I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x second write REG_ADDR_ADM1166_UPDCFG addr:0x%x, command:0x%x failed", addr, command);

                //clear fault record
                for (i = 0; i < 4; i++)
                {
                    temp_data.byte = (command & 0xff);
                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command >> 8), I2C_SMBUS_BYTE_DATA, &temp_data);
                    CHECK_IF_ERROR_GOTO_EXIT(ret, "block clear addr:0x%x, command:0x%x failed", addr, command);

                    //send 0xFE
                    temp_data.byte = 0;
                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, 0xFE, I2C_SMBUS_BYTE, &temp_data);
                    CHECK_IF_ERROR_GOTO_EXIT(ret, "send block addr:0x%x, command:0x%x failed", addr, command);

                    mdelay(100);
                    command += 32;
                }
                protocol = I2C_SMBUS_PROTOCOL_BUTT;
            }
            break;
        case i2c_flag_ADM116x_rw_SE:
#if 0
            //STOPWRITE to 0x90
            temp_value = 0x01;
            temp_data.byte = temp_value;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_UPDCFG, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write REG_ADDR_ADM1166_UPDCFG addr:0x%x, command:0x90 failed", addr);
            mdelay(1);
#endif
            //STOPWRITE to 0x9c
            temp_value = 0x01;
            temp_data.byte = temp_value;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_BBCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write REG_ADDR_ADM1166_BBCTRL addr:0x%x, command:0x9c failed", addr);
            mdelay(1);
            
            //STOPWRITE to 0x93
            temp_data.byte = 0x01;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_SECTRL,  I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x first write REG_ADDR_ADM1166_BBCTRL addr:0x%x, command:0x9C failed", addr);
            mdelay(1);

            if (I2C_SMBUS_EraseEeprom == read_write)
            {
                //STOPWRITE to 0x90 enable earse
                temp_data.byte = 0x05;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_UPDCFG,  I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x second write REG_ADDR_ADM1166_UPDCFG addr:0x%x, command:0x90 failed", addr);

                for(usEraseAddr = command; usEraseAddr <= 0xFBFF; usEraseAddr += 0x20)
                {
                    if(usEraseAddr >= 0xF8A0 && usEraseAddr <= 0xF8FF)              /* reserve */
                    {
                        continue;
                    }

                    if(usEraseAddr >= 0xFA00)
                    {
                        //STOPWRITE to 0x93
                        temp_data.byte = 0x01;
                        ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_SECTRL,  I2C_SMBUS_BYTE_DATA, &temp_data);
                        CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x first write REG_ADDR_ADM1166_BBCTRL addr:0x%x, command:0x9C failed", addr);
                        mdelay(2);
                    }

                    temp_data.byte = (usEraseAddr & 0xff);
                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(usEraseAddr >> 8), I2C_SMBUS_BYTE_DATA, &temp_data);
                    CHECK_IF_ERROR_GOTO_EXIT(ret, "adm1166 read/erase addr:0x%x, send register addr:0x%04x failed", addr, usEraseAddr);

                    //send 0xFE
                    temp_data.byte = 0x0;
                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, 0xFE, I2C_SMBUS_BYTE, &temp_data);
                    CHECK_IF_ERROR_GOTO_EXIT(ret, "adm1166 erase block addr:0x%x, command:0x%x send 0xFE failed", addr, usEraseAddr);
                    mdelay(100);
                }

                //STOPWRITE to 0xd9
                temp_data.byte = 0x01;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_BBSEARCH, I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR(ret, "ADM116x write REG_ADDR_ADM1166_BBSEARCH addr:0x%x, value:0x%x failed", REG_ADDR_ADM1166_BBSEARCH, temp_data.byte);

                //STOPWRITE to 0x90
                temp_value = 0x01;
                temp_data.byte = temp_value;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_UPDCFG, I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write REG_ADDR_ADM1166_UPDCFG addr:0x%x, command:0x90 failed", addr);
                protocol = I2C_SMBUS_PROTOCOL_BUTT;
            }
            else
            {
                protocol = I2C_SMBUS_I2C_BLOCK_DATA;
            }
            
            break;
        case i2c_flag_ISL68127_rw_select1:
            /*根据硬件设计，当前仅特殊处理select page 1*/
            temp_data.byte = 1;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ISL68127_CMD_PAGE, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "ISL68127 write cmdpage addr:0x%x, command:0x%x, dataval:0x%x failed", addr, command, *(u8 *)dataval);
            if (byte_count == 2)
            {
                protocol = I2C_SMBUS_WORD_DATA;
            }
            else
            {
                protocol = I2C_SMBUS_BYTE_DATA;
            }
            break;
        case i2c_flag_RA228_rw_select1:
            /*根据硬件设计，当前仅特殊处理select page 1*/
            temp_data.byte = 0;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_RA228_CMD_PAGE, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "RA228 write cmdpage addr:0x%x, command:0x%x, dataval:0x%x failed", addr, command, *(u8 *)dataval);
            if (byte_count == 2)
            {
                protocol = I2C_SMBUS_WORD_DATA;
            }
            else
            {
                protocol = I2C_SMBUS_BYTE_DATA;
            }
            break;
        case i2c_flag_Max6696_rw_select1:
        case i2c_flag_Max6696_rw_select2:
            /*进行6696的channel选通处理*/
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, REG_ADDR_MAX6696_READ_CONFIG, I2C_SMBUS_BYTE_DATA, &temp_data);
            if (ERROR_SUCCESS == ret)
            {
                config = temp_data.byte;
                config = flags == i2c_flag_Max6696_rw_select1 ? (config & (~((u8)1 << MAX6696_SPOT_SELECT_BIT))) : (config | ((u8)1 << MAX6696_SPOT_SELECT_BIT));
                temp_data.byte = config;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_MAX6696_WRITE_CONFIG, I2C_SMBUS_BYTE_DATA, &temp_data);
            }
            CHECK_IF_ERROR_GOTO_EXIT(ret, "max6696 op addr:0x%x, channel_index:0x%x failed", addr, flags - i2c_flag_Max6696_rw_select0);
            break;
        case i2c_flag_Max6696_rw_select0:
            protocol = I2C_SMBUS_BYTE_DATA;
#if 0
            if (read_write == REG_WRITE)
            {
                temp_data.byte = *(u8 *)dataval;
            }
#endif
            break;
        default:
            ret = -EINVAL;
            CHECK_IF_ERROR_GOTO_EXIT(ret, "unsupported i2c op %d", flags);
    }

    switch (protocol)
    {
        case I2C_SMBUS_BYTE_DATA:
        case I2C_SMBUS_BYTE:
            data_pointer = (u8 *)dataval;
            temp_from_address = command;
            for (i = 0; i < byte_count; i++)
            {
                if (read_write == I2C_SMBUS_WRITE)
                {
                    temp_data.byte = *(u8 *)data_pointer;
                }
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, read_write, (u8)temp_from_address, protocol, &temp_data);

                if (0 == ret)
                {
                    if (read_write == I2C_SMBUS_READ)
                    {
                        *(u8 *)data_pointer = temp_data.byte;
                    }
                    temp_from_address ++;
                    data_pointer ++;
                }
                else
                {
                    break;
                }
            }

            CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c byte op addr:0x%x, command:0x%x, dev index:0x%x failed", addr, command, i2c_device_index);
            break;
        case I2C_SMBUS_WORD_DATA://仅支持单次操作
            if (read_write == I2C_SMBUS_WRITE)
            {
                temp_data.word = *(u16 *)dataval;
            }
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, read_write, (u8)command, protocol, &temp_data);
            CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c word op addr:0x%x, command:0x%x, dev index:0x%x failed", addr, command, i2c_device_index);

            if (read_write == I2C_SMBUS_READ)
            {
                *(u16 *)dataval = temp_data.word;
            }
            break;
        case I2C_SMBUS_I2C_BLOCK_DATA:
            if (i2c_flag_ADM116x_rw_SecV == flags)
            {
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, (u8)REG_ADDR_DEV_ADM1166_BASE + command * 2, I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c byte op addr:0x%x, command:0x%x, dev index:0x%x failed", addr, command, i2c_device_index);
                data[0] = (u8)temp_data.byte;
                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, (u8)(REG_ADDR_DEV_ADM1166_BASE + command * 2) + 1, I2C_SMBUS_BYTE_DATA, &temp_data);
                CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c byte op addr:0x%x, command:0x%x, dev index:0x%x failed", addr, command, i2c_device_index);
                data[1] = (u8)temp_data.byte;
                *(int *)dataval = ((((u32)(data[0])) << 8) + ((u32)(data[1])));
            }
            else if (i2c_flag_ADM116x_rw_VerS == flags)
            {
                for (i = 0; i <= byte_count; i++)
                {
                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &temp_data);
                    CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c byte op addr:0x%x, command:0x%x, dev index:0x%x failed", addr, command, i2c_device_index);
                    data[i] = (u8)temp_data.byte;
                }
                *(int *)dataval = ((((u32)(data[0])) << 8) + ((u32)(data[1])));
            }
            else if (i2c_flag_ADM116x_rw_FauC == flags)
            {
                for (i = 0; i < byte_count; i++)
                {
                    temp_data.byte = (u8)(command & 0xff);
                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command >> 8), I2C_SMBUS_BYTE_DATA, &temp_data);
                    CHECK_IF_ERROR_GOTO_EXIT(ret, "fault_record write addr:0x%x, command:0x%x failed", addr, command);

                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &temp_data);
                    ((u8 *)dataval)[i] = temp_data.byte;
                    CHECK_IF_ERROR_GOTO_EXIT(ret, "fault_record read addr:0x%x, command:0x%x failed", addr, command);
                    command++;
                }
            }
            else if (i2c_flag_ADM116x_rw_SE == flags)
            {
                if (I2C_SMBUS_READ == read_write)
                {
                    for (i = 0; i < byte_count; i++)
                    {
                        if(command >= 0xF8A0 && command <= 0xF8FF)/* reserve */
                        {
                            ((u8 *)dataval)[i] = 0x00;
                        }
                        else
                        {
                            temp_data.byte = (u8)(command & 0xff);
                            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command >> 8), I2C_SMBUS_BYTE_DATA, &temp_data);
                            CHECK_IF_ERROR_GOTO_EXIT(ret, "1166 SE write addr:0x%x, command:0x%x failed at reading SE process", addr, command);

                            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &temp_data);
                            ((u8 *)dataval)[i] = temp_data.byte;
                            CHECK_IF_ERROR_GOTO_EXIT(ret, "1166 SE read addr:0x%x, command:0x%x failed at reading SE process", addr, command);
                        }
                        command++;
                    }
                }
                else
                {
                    for(i = 0; i < byte_count; i++)
                    {
                        for(uiTry = 0; uiTry < 3; uiTry++)
                        {
                            if(command >= 0xF8A0 && command <= 0xF8FF)/* reserve */
                            {
                                command ++;
                                break;
                            }

                            if(command >= 0xFA00)
                            {
                                //STOPWRITE to 0x93
                                temp_data.byte = 0x01;
                                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_SECTRL,  I2C_SMBUS_BYTE_DATA, &temp_data);
                                CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write REG_ADDR_ADM1166_SECTRL addr:0x%x, command:0x%x failed", addr, command);
                            }

                            valuetmp = *((u8 *)dataval + i);
                            temp_data.word = (((u16)valuetmp) << 8) | (command & 0xff);
                            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command >> 8), I2C_SMBUS_WORD_DATA, &temp_data);
                            CHECK_IF_ERROR_GOTO_EXIT(ret, "1166 SE write addr:0x%x, command:0x%x failed at writing SE process", addr, command);
                            msleep(1);

                            if(command >= 0xFA00)
                            {
                                //STOPWRITE to 0x93
                                temp_data.byte = 0x01;
                                ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_SECTRL,  I2C_SMBUS_BYTE_DATA, &temp_data);
                                CHECK_IF_ERROR_GOTO_EXIT(ret, "ADM116x write REG_ADDR_ADM1166_SECTRL addr:0x%x, command:0x%x failed", addr, command);
                             }

                            /*read back*/
                            temp_data.word = 0;
                            temp_data.byte = (u8)(command & 0xff);
                            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, (u8)(command >> 8), I2C_SMBUS_BYTE_DATA, &temp_data);
                            CHECK_IF_ERROR_GOTO_EXIT(ret, "eeprom adm1166 read back addr:0x%x, command:0x%x failed at writing SE process", addr, command);

                            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &temp_data);
                            msleep(1);

                            if(temp_data.byte != valuetmp)
                            {
                                continue;
                            }
                            else
                            {
                                command ++;
                                break;
                            }
                        }

                        if(3 == uiTry)
                        {
                            DBG_ECHO(DEBUG_ERR, "bsp_i2c_adm116x_write_reg try_count=%d failed(%d) addr=0x%04x.\n", uiTry, ret, command);
                            goto exit;
                        }
                    }
                }
            }
            else if (i2c_flag_psu_eeprom_r_byte == flags)
            {
                data_pointer = (u8 *)dataval;
                for (i = 0; i < byte_count; i++)
                {
                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_READ, (u8)command + i, I2C_SMBUS_BYTE_DATA, &temp_data);
                    CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c byte op addr:0x%x, command:0x%x, dev index:0x%x failed", addr, command, i2c_device_index);
                    data_pointer[i] = (u8)temp_data.byte;
                }
            }
            else if (i2c_flag_2addr_eeprom_rw_byte == flags)
            {
                loop_count = byte_count / 30;//N287 USE 30,refer comware
                mod = byte_count % 30;
                temp_read_bytes = 0;
                temp_from_address = command;
                data_pointer = (u8 *)dataval;
                for (i = 0; i <= loop_count; i++)
                {
                    ucBuf[0] = ((temp_from_address) >> 8) & 0x0ff;
                    ucBuf[1] = temp_from_address & 0x0ff;
                    DBG_ECHO(DEBUG_ERR, "comt 2 bytes write  data_pointer %x addr %x,%x\r\n ", *data_pointer, ucBuf[0], ucBuf[1]);
                    temp_read_bytes = (i == loop_count) ? mod : 30;
                    if (0 == temp_read_bytes)
                    {
                        break;
                    }
                    temp_data.block[0] = temp_read_bytes + 1;
                    temp_data.block[1] = ucBuf[1];
                    if (read_write == I2C_SMBUS_WRITE)
                    {
                        memcpy(&temp_data.block[2], data_pointer, temp_read_bytes);
                    }
                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, read_write, ucBuf[0], I2C_SMBUS_I2C_BLOCK_DATA, &temp_data);
                    mdelay(10);
                    if (0 == ret)
                    {
                        if (read_write == I2C_SMBUS_READ)
                        {
                            memcpy(data_pointer, &temp_data.block[1], temp_read_bytes);
                        }
                        temp_from_address += temp_read_bytes;
                        data_pointer += temp_read_bytes;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            else
            {
                loop_count = byte_count / I2C_SMBUS_BLOCK_MAX;         //每次只能读block max个字节，32个
                mod = byte_count % I2C_SMBUS_BLOCK_MAX;
                temp_read_bytes = 0;
                temp_from_address = command;
                data_pointer = (u8 *)dataval;

                for (i = 0; i <= loop_count; i++)
                {
                    temp_read_bytes = (i == loop_count) ? mod : I2C_SMBUS_BLOCK_MAX;
                    if (0 == temp_read_bytes)
                    {
                        break;
                    }
                    temp_data.block[0] = temp_read_bytes;

                    if (read_write == I2C_SMBUS_WRITE)
                    {
                        memcpy(&temp_data.block[1], data_pointer, temp_read_bytes);
                    }

                    ret = bsp_i2c_smbus_xfer(smbus, addr, 0, read_write, temp_from_address, I2C_SMBUS_I2C_BLOCK_DATA, &temp_data);
                    if (0 == ret)
                    {
                        if (read_write == I2C_SMBUS_READ)
                        {
                            memcpy(data_pointer, &temp_data.block[1], temp_read_bytes);
                        }
                        temp_from_address += temp_read_bytes;
                        data_pointer += temp_read_bytes;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c block BLOCK op addr:0x%x, command:0x%x, dev index:0x%x failed", addr, command, i2c_device_index);
            break;
        case I2C_SMBUS_PROTOCOL_BUTT://表示已经前面已经处理完成了
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "i2c op is not support, addr=0x%x flags=%d, read_write=%d, command=0x%x, protocol=%d, dev index:0x%x", addr, flags, read_write, command, protocol, i2c_device_index);
            break;
    }

exit:
    if (flags == i2c_flag_ADM116x_rw_SecV)
    {
        temp_value = ADM1166_RRCTRL_REG_RESET;
        temp_data.byte = temp_value;
        ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_RRCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);
        CHECK_IF_ERROR(ret, "ADM116x write ctrl failed, addr:0x%x, command:0x%x", addr, command);
    }
    else if (flags == i2c_flag_ADM116x_rw_VerS)
    {
        temp_data.byte = temp_value;
        ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_VSCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);
        CHECK_IF_ERROR(ret, "ADM116x write ctrl failed, addr:0x%x, command:0x%x", addr, command);
    }
    else if (flags == i2c_flag_ADM116x_rw_FauC)
    {
        if (read_write == I2C_SMBUS_READ)
        {
            temp_value = 0x00;
            temp_data.byte = temp_value;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_BBCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR(ret, "ADM116x write ctrl failed, addr:0x%x, command:0x%x", REG_ADDR_ADM1166_BBCTRL, command);
        }
        else if (read_write == I2C_SMBUS_WRITE)
        {
            //STOPWRITE to 0xd9
            temp_data.byte = 0x01;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_BBSEARCH, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR(ret, "ADM116x write REG_ADDR_ADM1166_BBSEARCH addr:0x%x, value:0x%x failed", REG_ADDR_ADM1166_BBSEARCH, temp_data.byte);

            //ENABLE to 0x9c
            temp_data.byte = 0x00;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_BBCTRL, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR(ret, "ADM116x second write REG_ADDR_ADM1166_BBCTRL addr:0x%x, value:0x%x failed", REG_ADDR_ADM1166_BBCTRL, temp_data.byte);

            //STOPWRITE to 0x90
            temp_data.byte = 0x01;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_UPDCFG, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR(ret, "ADM116x third write REG_ADDR_ADM1166_UPDCFG addr:0x%x, value:0x%x failed", REG_ADDR_ADM1166_UPDCFG, temp_data.byte);

            //enbale to 0x93
            temp_data.byte = 0x00;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_SECTRL, I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR(ret, "ADM116x write REG_ADDR_ADM1166_SECTRL addr:0x%x, value:0x%x failed", REG_ADDR_ADM1166_SECTRL, temp_data.byte);
        }
    }
    else if (flags == i2c_flag_ADM116x_rw_SE)
    {
        if (command > 0xFBFF && read_write == I2C_SMBUS_WRITE)
        {
            //STOPWRITE to 0xD8 Restore the sequential control engine
            temp_data.byte = 0x01;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_UDOWNLD,  I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR(ret, "ADM116x write REG_ADDR_ADM1166_UDOWNLD addr:0x%x, command:0xD8 failed", addr);
        }

        if (I2C_SMBUS_EraseEeprom != read_write)
        {
            //STOPWRITE to 0x93 Restore the sequential control engine
            temp_data.byte = 0x00;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_SECTRL,  I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR(ret, "ADM116x write REG_ADDR_ADM1166_SECTRL addr:0x%x, command:0x93 failed", addr);
            
            /* The following commands are recovery status */
            //STOPWRITE to 0x9C ENABLE BLACK BOX
            temp_data.byte = 0x00;
            ret = bsp_i2c_smbus_xfer(smbus, addr, 0, I2C_SMBUS_WRITE, REG_ADDR_ADM1166_BBCTRL,  I2C_SMBUS_BYTE_DATA, &temp_data);
            CHECK_IF_ERROR(ret, "ADM116x first write REG_ADDR_ADM1166_BBCTRL addr:0x%x, command:0x9C failed", addr);
        }
    }

    current_i2c_path_id = current_i2c_path_id + 10000;
    mutex_unlock(&bsp_i2c_path_lock);

    return ret;
}

/* psu eeprom */
int bsp_i2c_pmbus_eeprom_read_bytes(u16 dev_i2c_address, u16 from_inner_address, size_t byte_count, OUT u8 * data, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_BLOCK_DATA, I2C_SMBUS_READ, from_inner_address, byte_count, (void*)data, i2c_device_index);
    CHECK_IF_ERROR(ret, "read eeprom address 0x%x inner 0x%x failed", dev_i2c_address, from_inner_address);
    return ret;
}


int bsp_i2c_pmbus_eeprom_write_bytes(u16 dev_i2c_address, u16 from_inner_address, size_t byte_count, u8 * data, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_BLOCK_DATA, I2C_SMBUS_WRITE, from_inner_address, byte_count, (void *)data, i2c_device_index);
    CHECK_IF_ERROR(ret, "write eeprom address 0x%x inner 0x%x failed", dev_i2c_address, from_inner_address);
    return ret;
}
#ifdef _PDO
int bsp_adm1166_reg_read_byte(u16 dev_i2c_address, u16 from_inner_address, OUT u8 *data, I2C_DEVICE_E i2c_device_index)
{
    union i2c_smbus_data temp_data = {0};
    int status = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_adm1166_reg_read_byte get bd failed");
        return -EINVAL;
    }

    if ((i2c_device_index >= I2C_DEV_OPTIC_IDX_START) && (i2c_device_index < I2C_DEV_OPTIC_BUTT))
    {
        if (1 == bd->dom_exist)   //因为涉及锁操作，所以建议调用这个select之前先判断，需要的话再调用。这个可以增加到函数调用说明中
        {
            DBG_ECHO(DEBUG_ERR, "check fail: access by dom rather than i2c_select");
            return -EPERM;
        }
    }

    mutex_lock(&bsp_i2c_path_lock);
    current_i2c_path_id = i2c_device_index;
    status = bsp_i2c_smbus_xfer(smbus, dev_i2c_address, 0, I2C_SMBUS_READ, (u8)from_inner_address, I2C_SMBUS_BYTE, &temp_data);
    if (status == ERROR_SUCCESS)
    {
        *data = temp_data.byte;
    }
    else
    {
        DBG_ECHO(DEBUG_INFO, "adm1166 read byte failed ret=%d", status);
    }
    current_i2c_path_id = current_i2c_path_id + 10000;
    mutex_unlock(&bsp_i2c_path_lock);
    return ERROR_SUCCESS;
}

int bsp_adm1166_reg_write_byte(u16 dev_i2c_address, u8 inner_address, u8 data, I2C_DEVICE_E i2c_device_index)
{
    union i2c_smbus_data temp_data = {0};
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_adm1166_reg_write_byte get bd failed");
        return -EPERM;
    }
    temp_data.byte = data;

    if ((i2c_device_index >= I2C_DEV_OPTIC_IDX_START) && (i2c_device_index < I2C_DEV_OPTIC_BUTT))
    {
        if (1 == bd->dom_exist)   //因为涉及锁操作，所以建议调用这个select之前先判断，需要的话再调用。这个可以增加到函数调用说明中
        {
            DBG_ECHO(DEBUG_ERR, "check fail: access by dom rather than i2c_select");
            return -EPERM;
        }
    }

    mutex_lock(&bsp_i2c_path_lock);
    current_i2c_path_id = i2c_device_index;

    ret = i2c_select(i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c select dev index 0x%x failed", i2c_device_index);

    ret = bsp_i2c_smbus_xfer(smbus, dev_i2c_address, 0, I2C_SMBUS_WRITE, inner_address, I2C_SMBUS_BYTE_DATA, &temp_data);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "addr %#x write value %#x failed", inner_address, data);

    current_i2c_path_id = current_i2c_path_id + 10000;
    mutex_unlock(&bsp_i2c_path_lock);
exit:
    return ret;
}
#endif

//从eeprom设备读取内容。 dev_i2c_address, i2c总线地址, from_inner
//适用于 类似24LC128  的eeprom
int bsp_i2c_24LC128_eeprom_read_bytes(u16 dev_i2c_address, u16 from_inner_address, size_t byte_count, OUT u8 *data, I2C_DEVICE_E i2c_device_index)
{
    int status;
    status = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_24LC128_eeprom, I2C_SMBUS_READ, from_inner_address, byte_count, (void *)data, i2c_device_index);
    CHECK_IF_ERROR(status, "read eeprom address 0x%x inner 0x%x failed", dev_i2c_address, from_inner_address);
    return status;
}

//eeprom for 24LC128,  地址采用2字节编址，读写需要特殊处理
int bsp_i2c_24LC128_eeprom_write_byte(u16 dev_i2c_address, u16 inner_address, u8 value, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_24LC128_eeprom, I2C_SMBUS_WRITE, inner_address, 1, (void *)&value, i2c_device_index);
    CHECK_IF_ERROR(ret, "write eeprom address 0x%x inner 0x%x failed", dev_i2c_address, inner_address);
    return ret;
}

//eeprom for 24LC128,  地址采用2字节编址，读写需要特殊处理
int bsp_i2c_24LC128_eeprom_write_bytes(u16 dev_i2c_address, u16 inner_address, u8 *value, size_t byte_count, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_2addr_eeprom_rw_byte, I2C_SMBUS_WRITE, inner_address, byte_count, (void *)value, i2c_device_index);
    CHECK_IF_ERROR(ret, "write eeprom address 0x%x inner 0x%x failed", dev_i2c_address, inner_address);
    return ret;
}

//一般的eeprom读, eeprom内部地址采用一字节
int bsp_i2c_common_eeprom_read_bytes(u16 dev_i2c_address, u16 from_inner_address, size_t byte_count, OUT u8 *data, I2C_DEVICE_E i2c_device_index)
{
    int status = ERROR_SUCCESS;
    status = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_eeprom_rw_byte, I2C_SMBUS_READ, from_inner_address, byte_count, (void *)data, i2c_device_index);
    CHECK_IF_ERROR(status, "eeprom read address 0x%x inner 0x%x failed", dev_i2c_address, from_inner_address);
    return status;
}
int bsp_i2c_psu_eeprom_read_bytes(u16 dev_i2c_address, u16 from_inner_address, size_t byte_count, OUT u8 *data, I2C_DEVICE_E i2c_device_index)
{
    int status = ERROR_SUCCESS;
    status = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_psu_eeprom_r_byte, I2C_SMBUS_READ, from_inner_address, byte_count, (void *)data, i2c_device_index);
    CHECK_IF_ERROR(status, "eeprom read address 0x%x inner 0x%x failed", dev_i2c_address, from_inner_address);
    return status;
}

int bsp_i2c_common_eeprom_write_byte(u16 dev_i2c_address, u16 inner_address, u8 data, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_eeprom_rw_byte, I2C_SMBUS_WRITE, inner_address, 1, (void *)&data, i2c_device_index);
    CHECK_IF_ERROR(ret, "eeprom write address 0x%x inner 0x%x failed", dev_i2c_address, inner_address);
    return ret;
}

//SFP eeprom
int bsp_i2c_SFP_read_bytes(int sfp_index, u16 dev_i2c_address, u16 from_inner_address, size_t byte_count, OUT u8 *data, I2C_DEVICE_E i2c_device_index)
{
    u16 count;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_i2c_SFP_write_byte get bd failed");
        return -EINVAL;
    }
    if (bd->dom_exist)
    {
        //TODO
        (void)dev_i2c_address;
        count = byte_count;
        ret = dom_rw((u8)sfp_index, 0, 0, from_inner_address, &count, data, 0);
        CHECK_IF_ERROR(ret, "SFP read opt %d offset 0x%x failed!", sfp_index + 1, from_inner_address);
    }
    else
    {
        return bsp_i2c_common_eeprom_read_bytes(dev_i2c_address, from_inner_address, byte_count, data, i2c_device_index);
    }
    return ret;
}

//SFP eeprom
int bsp_i2c_SFP_write_byte(int sfp_index, u16 dev_i2c_address, u16 from_inner_address, u8 data, I2C_DEVICE_E i2c_device_index)
{
    u16 count;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_i2c_SFP_write_byte get bd failed");
        return -EINVAL;
    }
    if (bd->dom_exist)
    {
        (void)dev_i2c_address;
        count = 1;
        ret = dom_rw((u8)sfp_index, 0, 0, from_inner_address, &count, &data, 1);
        CHECK_IF_ERROR(ret, "SFP write opt %d offset 0x%x failed!", sfp_index + 1, from_inner_address);
    }
    else
    {
        return bsp_i2c_common_eeprom_write_byte(dev_i2c_address, from_inner_address, data, i2c_device_index);
    }

    return ret;
}

//max 6696 读其中spot index的温度点寄存器值，spot index = [0,1,2]
int bsp_i2c_Max6696_get_temp(u16 dev_i2c_address, MAX6696_SPOT_INDEX spot_index, s8 *value, I2C_DEVICE_E i2c_device_index)
{
    int status;
    union i2c_smbus_data temp_data;
    u16 inner_address = 0;
    I2C_FLAG_E opflags = i2c_flag_Max6696_rw_select0;

    switch (spot_index)
    {
        case MAX6696_LOCAL_SOPT_INDEX:
            inner_address = REG_ADDR_MAX6696_TEMP_LOCAL;
            break;
        case MAX6696_REMOTE_CHANNEL1_SOPT_INDEX:
        case MAX6696_REMOTE_CHANNEL2_SOPT_INDEX:
            opflags = (spot_index == MAX6696_REMOTE_CHANNEL1_SOPT_INDEX) ? i2c_flag_Max6696_rw_select1 : i2c_flag_Max6696_rw_select2;
            inner_address = REG_ADDR_MAX6696_TEMP_REMOTE;
            break;
        default:
            status = -EINVAL;
            CHECK_IF_ERROR_GOTO_EXIT(status, "max6696 has only %d spot, spot index %d is invalid", MAX6696_SPOT_NUM, spot_index);
    }
    status = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, opflags, I2C_SMBUS_READ, inner_address, I2C_SMBUS_BYTE_DATA, (void *)&temp_data, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(status, "Max6696 get temp for dev_add 0x%x inner_addr 0x%x failed", dev_i2c_address, inner_address);
    *value = temp_data.byte;
exit:
    return status;
}

//读写max6696的温度门限寄存器
int bsp_i2c_Max6696_limit_rw(int read_write, u16 dev_i2c_address, MAX6696_LIMIT_INDEX limit_index, s8 *value, I2C_DEVICE_E i2c_device_index)
{
    int status = ERROR_SUCCESS;
    u8 inner_addr = 0;
    I2C_FLAG_E opflags = i2c_flag_Max6696_rw_select0;
    switch (limit_index)
    {
        case MAX6696_LOCAL_HIGH_ALERT: inner_addr = REG_ADDR_MAX6696_READ_ALERT_HI_LOCAL;
            break;
        case MAX6696_LOCAL_LOW_ALERT:  inner_addr = REG_ADDR_MAX6696_READ_ALERT_LO_LOCAL;
            break;
        //case MAX6696_LOCAL_OT2_LIMIT:  inner_addr = REG_ADDR_MAX6696_RW_OT2_LOCAL; break;
        case MAX6696_LOCAL_OT2_LIMIT:
        case SET_MAX6696_LOCAL_OT2_LIMIT:
            inner_addr = REG_ADDR_MAX6696_RW_OT2_LOCAL;
            break;
        case MAX6696_REMOTE_CHANNEL1_HIGH_ALERT:
        case MAX6696_REMOTE_CHANNEL2_HIGH_ALERT:
            opflags = (limit_index == MAX6696_REMOTE_CHANNEL1_HIGH_ALERT) ? i2c_flag_Max6696_rw_select1 : i2c_flag_Max6696_rw_select2;
            inner_addr = REG_ADDR_MAX6696_READ_ALERT_HI_REMOTE;
            break;
        case MAX6696_REMOTE_CHANNEL1_LOW_ALERT:
        case MAX6696_REMOTE_CHANNEL2_LOW_ALERT:
            opflags = (limit_index == MAX6696_REMOTE_CHANNEL1_LOW_ALERT) ? i2c_flag_Max6696_rw_select1 : i2c_flag_Max6696_rw_select2;
            inner_addr = REG_ADDR_MAX6696_READ_ALERT_LO_REMOTE;
            break;
        case MAX6696_REMOTE_CHANNEL1_OT2_LIMIT:
        case MAX6696_REMOTE_CHANNEL2_OT2_LIMIT:
            opflags = (limit_index == MAX6696_REMOTE_CHANNEL1_OT2_LIMIT) ? i2c_flag_Max6696_rw_select1 : i2c_flag_Max6696_rw_select2;
            inner_addr = REG_ADDR_MAX6696_RW_OT2_REMOTE;
            break;
        case SET_MAX6696_REMOTE_CHANNEL1_OT2_LIMIT:
        case SET_MAX6696_REMOTE_CHANNEL2_OT2_LIMIT:
            opflags = (limit_index == SET_MAX6696_REMOTE_CHANNEL1_OT2_LIMIT) ? i2c_flag_Max6696_rw_select1 : i2c_flag_Max6696_rw_select2;
            inner_addr = REG_ADDR_MAX6696_RW_OT2_REMOTE;
            break;
        case SET_MAX6696_LOCAL_HIGH_ALERT:
            inner_addr = REG_ADDR_MAX6696_WRITE_ALERT_HI_LOCAL;
            break;
        case SET_MAX6696_LOCAL_LOW_ALERT:
            inner_addr = REG_ADDR_MAX6696_WRITE_ALERT_LO_LOCAL;
            break;
        case SET_MAX6696_REMOTE_CHANNEL1_HIGH_ALERT:
        case SET_MAX6696_REMOTE_CHANNEL2_HIGH_ALERT:
            opflags = (limit_index == SET_MAX6696_REMOTE_CHANNEL1_HIGH_ALERT) ? i2c_flag_Max6696_rw_select1 : i2c_flag_Max6696_rw_select2;
            inner_addr = REG_ADDR_MAX6696_WRITE_ALERT_HI_REMOTE;
            break;
        case SET_MAX6696_REMOTE_CHANNEL1_LOW_ALERT:
        case SET_MAX6696_REMOTE_CHANNEL2_LOW_ALERT:
            opflags = (limit_index == SET_MAX6696_REMOTE_CHANNEL1_LOW_ALERT) ? i2c_flag_Max6696_rw_select1 : i2c_flag_Max6696_rw_select2;
            inner_addr = REG_ADDR_MAX6696_WRITE_ALERT_LO_REMOTE;
            break;
        default:
            status = -EINVAL;
            CHECK_IF_ERROR_GOTO_EXIT(status, "Max6696 limit index %d is invalid", limit_index);
    }

    status = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, opflags, read_write, inner_addr, 1, (void *)value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(status, "Max6696 limit rw for dev_add 0x%x inner_addr 0x%x failed", dev_i2c_address, inner_addr);
exit:
    return status;
}

int bsp_i2c_power_reg_read(u16 dev_i2c_address, u16 from_inner_address, size_t byte_count, u8 *value, I2C_DEVICE_E i2c_device_index)
{
    return bsp_i2c_common_eeprom_read_bytes(dev_i2c_address, from_inner_address, byte_count, value, i2c_device_index);
}

/*
 *获取adm1166的电压值，
 * 涉及到多次I2C的操作，进行了器件级别的锁保护
 */
int bsp_get_secondary_voltage_value(u16 dev_i2c_address, int uiChanNo, int *data, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_ADM116x_rw_SecV, I2C_SMBUS_READ, uiChanNo, 2, (void *)data, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "adm1166 read dev_addr 0x%x reg_addr 0x%x failed", dev_i2c_address, REG_ADDR_ADM1166_RRCTRL);
exit:
    return ret;
}

int bsp_get_adm116x_get_hwversion_reg(u16 dev_i2c_address, int uiChanNo, int *data, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_ADM116x_rw_VerS, I2C_SMBUS_READ, 0xf900, 8, (void *)data, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "adm1166 read dev_addr 0x%x reg_addr 0x%x failed", dev_i2c_address, REG_ADDR_ADM1166_RRCTRL);
exit:
    return ret;
}
int bsp_get_adm116x_fault_record(u16 dev_i2c_address, int uiChanNo, u8 *data, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_ADM116x_rw_FauC, I2C_SMBUS_READ, 0xf980, 128, data, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "adm1166 read dev_addr 0x%x reg_addr 0x%x failed", dev_i2c_address, REG_ADDR_ADM1166_RRCTRL);
exit:
    return ret;
}

int bsp_set_adm116x_fault_record(u16 dev_i2c_address, int uiChanNo, u8 *data, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_ADM116x_rw_FauC, I2C_SMBUS_WRITE, 0xf980, 128, (void *)data, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "adm1166 read dev_addr 0x%x reg_addr 0x%x failed", dev_i2c_address, REG_ADDR_ADM1166_RRCTRL);
exit:
    return ret;
}
/***　read adm116x  ***/
int bsp_i2c_adm116x_read_reg(u16 dev_i2c_address, u16 command_code, u8 *value, int read_byte_count, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_ADM116x_rw_SE, I2C_SMBUS_READ, command_code, read_byte_count, value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "adm116x read dev_addr 0x%x se_eeprom_addr 0xF800 failed", dev_i2c_address);
exit:
    return ret;
}

/*** erase adm116x eeprom  ***/
int bsp_i2c_adm116x_erase_reg(u16 dev_i2c_address, u16 command_code, u8 *value, int erase_byte_count, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_ADM116x_rw_SE, I2C_SMBUS_EraseEeprom, command_code, erase_byte_count, value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "adm116x erase i2c_addr 0x%x se_eeprom_addr 0xF800 failed", dev_i2c_address);
exit:
    return ret;
}

/*** write adm116x ***/
int bsp_i2c_adm116x_write_reg(u16 dev_i2c_address, u16 command_code, u8 *value, int write_byte_count, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_ADM116x_rw_SE, I2C_SMBUS_WRITE, command_code, write_byte_count, value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "adm116x write i2c_addr 0x%x se_eeprom_addr 0x%x failed", dev_i2c_address, command_code);
exit:
    return ret;
}
/*没有调用 jzq*/
int bsp_i2c_ina219_read_reg(u16 dev_i2c_address, u16 inner_address, u16 *value, I2C_DEVICE_E i2c_device_index)
{
    int status;
    status = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_WORD_DATA, I2C_SMBUS_READ, inner_address, 1, (void *)value, i2c_device_index);
    CHECK_IF_ERROR(status, "ina219 read dev_addr 0x%x dev_index %d failed", dev_i2c_address, i2c_device_index);
    return status;
}

static void bsp_i2c_is168127_to_complement_code(u8 tvalue[], int16_t *value)
{
    u16 temp_value = 0;
    
    temp_value = tvalue[0] | (tvalue[1] << 8);
    if (tvalue[1] == 128 && tvalue[0] == 0)
    {
        *value = 0;
        return;
    }
    else if ((tvalue[1] >> 7) != 1)
    {
        *value = (int16_t)temp_value;
        return;
    }

    *value = - ((int16_t)(~temp_value) + 1);
    return;
}

//读isl68127
int bsp_i2c_isl68127_read_reg(u16 dev_i2c_address, u16 command_code, u16 *value, int read_byte_count, I2C_DEVICE_E i2c_device_index)
{
    u8 temp_value[ISL68127_REG_VALUE_MAX_LEN] = {0};
    int ret = ERROR_SUCCESS;
    I2C_FLAG_E opflags = i2c_flag_ISL68127_rw_select1;
    if (read_byte_count == 0 || read_byte_count > ISL68127_REG_VALUE_MAX_LEN)
    {
        DBG_ECHO(DEBUG_ERR, "param err: isl68127 read byte %d", read_byte_count);
        return -EINVAL;
    }
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, opflags, I2C_SMBUS_READ, command_code, read_byte_count, (void *)temp_value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "isl68127 read i2c_addr 0x%x command 0x%x byte count %d failed", dev_i2c_address, command_code, read_byte_count);
    if (read_byte_count == 2)
    {
        bsp_i2c_is168127_to_complement_code(temp_value, value);
    }
    else
    {
        *value = temp_value[0];
    }
exit:
    return ret;
}

//写isl68127
int bsp_i2c_isl68127_write_reg(u16 dev_i2c_address, u16 command_code, u16 value, int write_byte_count, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    I2C_FLAG_E opflags = i2c_flag_ISL68127_rw_select1;
    if ((write_byte_count < 1) || (write_byte_count > 2))
    {
        DBG_ECHO(DEBUG_ERR, "param err: isl68127 write byte %d", write_byte_count);
        return -EINVAL;
    }
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, opflags, I2C_SMBUS_WRITE, command_code, write_byte_count, (void *)&value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "isl68127 write i2c_addr 0x%x command 0x%x byte count %d failed", dev_i2c_address, command_code, write_byte_count);
exit:
    return ret;
}

/***　read ra228  ***/
int bsp_i2c_ra228_read_reg(u16 dev_i2c_address, u16 command_code, u16 *value, int read_byte_count, I2C_DEVICE_E i2c_device_index)
{
    u8 temp_value[ISL68127_REG_VALUE_MAX_LEN] = {0};
    int ret = ERROR_SUCCESS;
    I2C_FLAG_E opflags = i2c_flag_RA228_rw_select1;
    if (read_byte_count == 0 || read_byte_count > 2)
    {
        DBG_ECHO(DEBUG_ERR, "param err: ra228 read byte %d", read_byte_count);
        return -EINVAL;
    }
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, opflags, I2C_SMBUS_READ, command_code, read_byte_count, (void *)temp_value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "ra228 read i2c_addr 0x%x command 0x%x byte count %d failed", dev_i2c_address, command_code, read_byte_count);
    if (read_byte_count == 2)
    {
        bsp_i2c_is168127_to_complement_code(temp_value, value);
    }
    else
    {
        *value = temp_value[0];
    }
exit:
    return ret;
}

/*** write ra2288 ***/
int bsp_i2c_ra228_write_reg(u16 dev_i2c_address, u16 command_code, u16 value, int write_byte_count, I2C_DEVICE_E i2c_device_index)
{
    int ret = ERROR_SUCCESS;
    I2C_FLAG_E opflags = i2c_flag_RA228_rw_select1;
    if ((write_byte_count < 1) || (write_byte_count > 2))
    {
        DBG_ECHO(DEBUG_ERR, "param err: ra228 write byte %d", write_byte_count);
        return -EINVAL;
    }
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, opflags, I2C_SMBUS_WRITE, command_code, write_byte_count, (void *)&value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "ra228 write i2c_addr 0x%x command 0x%x byte count %d failed", dev_i2c_address, command_code, write_byte_count);
exit:
    return ret;
}

int dom_rw(u8 opt_idx, u8 bank, u16 page_id, u8 offset, u16 *count, u8 *data, u8 op)
{
    pr_warn("dom_rw stub called !\n");
    return -ENOSYS;  
}
EXPORT_SYMBOL(dom_rw); 

//设置mac电压
int bsp_set_mac_rov(void)
{
#define RETRY_COUNT 3
    int i = 0;
    int ret = ERROR_SUCCESS;
    u8 mac_rov_cpld_value = 0;
    u16 voltage_to_set = 0;
    int iIndex = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_set_mac_rov get bd failed");
        return -EINVAL;
    }

    for (i = 0; i < RETRY_COUNT; i ++)
    {
        ret = bsp_cpld_read_byte(&mac_rov_cpld_value, bd->cpld_addr_mac_rov);
        if (ret != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "cpld read mac rov (0x%x) failed", bd->cpld_addr_mac_rov);
            continue;
        }
        voltage_to_set = (u16)((160000 - (mac_rov_cpld_value - 2) * 625) / 100);
        if ((voltage_to_set > bd->mac_rov_max_voltage) || (voltage_to_set < bd->mac_rov_min_voltage))
        {
            //从cpld里读出数值不在合理范围，设置成默认值
            DBG_ECHO(DEBUG_ERR, "cpld mac rov calc voltage = %d, set to default %d", voltage_to_set, bd->cpld_addr_mac_rov);
            voltage_to_set = bd->mac_rov_default_voltage;
        }
        else
        {
            break;
        }
    }
    if ((bd->mac_rov_device >= I2C_DEV_RA228) && (bd->mac_rov_device < I2C_DEV_RA228_BUTT))
    {
        if ((ret = bsp_i2c_ra228_write_reg(bd->i2c_addr_ra228, REG_ADDR_RA228_CMD_VOUT, voltage_to_set, 2, bd->mac_rov_device)) == ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_INFO, "RA228 write voltage 0x%x success! i=%d", (int)voltage_to_set, i);
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "RA228 write voltage 0x%x failed! i=%d", (int)voltage_to_set, i);
        }
    }
    else if ((bd->mac_rov_device >= I2C_DEV_ISL68127) && (bd->mac_rov_device < I2C_DEV_ISL68127_BUTT))
    {
        iIndex = bd->mac_rov_device - I2C_DEV_ISL68127;
        if ((ret = bsp_i2c_isl68127_write_reg(bd->i2c_addr_isl68127[iIndex], REG_ADDR_ISL68127_CMD_VOUT, voltage_to_set, 2, bd->mac_rov_device)) == ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_INFO, "isl68127 write voltage 0x%x success! i=%d", (int)voltage_to_set, i);
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "isl68127 write voltage 0x%x failed! i=%d", (int)voltage_to_set, i);
        }
    }
    else
    {
        /*** to compatible 81,82,83  and not set mac_rov_device***/
        DBG_ECHO(DEBUG_ERR, "not set mac_rov_device in  board data");
        if ((ret = bsp_i2c_isl68127_write_reg(bd->i2c_addr_isl68127[0], REG_ADDR_ISL68127_CMD_VOUT, voltage_to_set, 2, I2C_DEV_ISL68127)) == ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_INFO, "isl68127 write voltage 0x%x success! i=%d", (int)voltage_to_set, i);
        }
        else
        {
            DBG_ECHO(DEBUG_ERR, "isl68127 write voltage 0x%x failed! i=%d", (int)voltage_to_set, i);
        }
    }
    return ret;
#undef RETRY_COUNT
}
//EXPORT_SYMBOL(bsp_set_mac_rov);

int bsp_set_cpu_init_ok(u8 bit)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_set_cpu_init_ok get bd failed");
        return -EINVAL;
    }
    CHECK_IF_ZERO_GOTO_EXIT(-ENODEV, ret, bd->cpld_addr_cpu_init_ok, "mainboard cpu_init_ok reg is not defined!");
    ret = bsp_cpld_set_bit(bd->cpld_addr_cpu_init_ok, bd->cpld_offs_cpu_init_ok, bit);
exit:
    return ret;
}

ssize_t bsp_cpld_read_mac_inner_temp(u16 *temp_val)
{
    int ret = ERROR_SUCCESS;
    u8 mac_temp_lo = 0;
    u8 mac_temp_hi = 0;
    u16 mac_temp = 0;
    u8 retry = 0;
    u8 temp = 0;
    bool result = 0;

    mutex_lock(&bsp_mac_inner_temp_lock);
    ret = bsp_cpld_write_byte(0x01, MAC_TEMP_ADDR_CTRL);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "write mac inner temp flag failed!");

    do
    {
        ret = bsp_cpld_read_byte(&temp, MAC_TEMP_ADDR_CTRL);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "mac inner temp read phase 0 failed!");
        result = (MAC_TEMP_OK_MASK != (temp & MAC_TEMP_OK_MASK));
        if (result)
            msleep(10);
        else
            break;
    } while (retry++ < MAC_TEMP_MAX_RETRIES);

    CHECK_IF_ERROR_GOTO_EXIT(result, "mac inner temp read phase 1 failed with %#x!", temp);

    ret = bsp_cpld_read_byte(&mac_temp_lo, MAC_TEMP_ADDR_LO);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "mac inner temp read phase 2 failed!");
    ret = bsp_cpld_read_byte(&mac_temp_hi, MAC_TEMP_ADDR_HI);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "mac inner temp read phase 3 failed!");
    mac_temp = mac_temp_hi;
    mac_temp <<= 8;
    mac_temp |= mac_temp_lo;
    ret = bsp_cpld_write_byte(0x00, 0x63);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "mac inner temp read phase 4 failed!");

    *temp_val = mac_temp;

exit:
    mutex_unlock(&bsp_mac_inner_temp_lock);
    return ret;
}

ssize_t bsp_cpld_read_mac_width_temp(u16 *temp_val)
{
    int ret = ERROR_SUCCESS;
    u8 mac_temp_lo = 0;
    u8 mac_temp_hi = 0;
    u16 mac_temp = 0;
    u8 retry = 0;
    u8 temp = 0;
    bool result = 0;

    mutex_lock(&bsp_mac_width_temp_lock);
    ret = bsp_cpld_write_byte(0x01, MAC_TEMP_ADDR_CTRL);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "write mac width temp flag failed!");

    do
    {
        ret = bsp_cpld_read_byte(&temp, MAC_TEMP_ADDR_CTRL);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "mac width temp read phase 0 failed!");
        result = (MAC_TEMP_OK_MASK != (temp & MAC_TEMP_OK_MASK));
        if (result)
            msleep(10);
        else
            break;
    } while (retry++ < MAC_TEMP_MAX_RETRIES);

    CHECK_IF_ERROR_GOTO_EXIT(result, "mac width temp read phase 1 failed with %#x!", temp);

    ret = bsp_cpld_read_byte(&mac_temp_lo, MAC_WIDTH_TEMP_ADDR_LO);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "mac width temp read phase 2 failed!");
    ret = bsp_cpld_read_byte(&mac_temp_hi, MAC_WIDTH_TEMP_ADDR_HI);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "mac width temp read phase 3 failed!");
    mac_temp = mac_temp_hi;
    mac_temp <<= 8;
    mac_temp |= mac_temp_lo;
    ret = bsp_cpld_write_byte(0x00, 0x63);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "mac width temp read phase 4 failed!");

    *temp_val = mac_temp;

exit:
    mutex_unlock(&bsp_mac_width_temp_lock);
    return ret;
}
//start for cpld device method
int bsp_cpld_get_fan_pwm_reg(OUT u8 *pwm)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_fan_pwm_reg get bd failed");
        return ERROR_FAILED;
    }
    ret = bsp_cpld_read_byte(pwm, bd->cpld_addr_fan_pwm);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "pwm read error");
    CPLD_TRANS_VALUE_WITH_MASK_AND_OFFSET(*pwm, fan_pwm);
    //DBG_ECHO(DEBUG_DBG, "pwm = 0x%x", *pwm);
exit:
    return ret;
}

int bsp_cpld_set_fan_pwm_reg(IN u8 pwm)
{
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_set_fan_pwm_reg get bd failed");
        return -EINVAL;
    }
    return bsp_cpld_write_byte(pwm, bd->cpld_addr_fan_pwm);
}

//获取风扇对应的马达转速
int bsp_cpld_get_fan_speed(OUT u16 *speed, int fan_index, int moter_index)
{
    u8 speed_low = 0;
    u8 speed_high = 0;
    u16 temp_speed = 0;
    int moter_abs_index = 0;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_fan_speed get bd failed");
        return -EINVAL;
    }

    if (bd->product_type == PDT_TYPE_TCS81_120F_1U)
    {
        //cpld高位对应风扇小编号
        moter_abs_index = (bd->fan_num - fan_index - 1) * bd->motors_per_fan + moter_index;
    }
    else
    {
        moter_abs_index = fan_index * bd->motors_per_fan + moter_index;
    }
    mutex_lock(&bsp_fan_speed_lock);

    //选风扇
    bsp_cpld_write_part(moter_abs_index, bd->cpld_addr_fan_select, bd->cpld_mask_fan_select, bd->cpld_offs_fan_select);

    ret = bsp_cpld_read_byte(&speed_low, bd->cpld_addr_fan_speed[CPLD_FAN_SPEED_LOW_REG_INDEX]);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "fan speed_low read failed");
    ret = bsp_cpld_read_byte(&speed_high, bd->cpld_addr_fan_speed[CPLD_FAN_SPEED_HIGH_REG_INDEX]);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "fan speed_high read failed");
    CPLD_TRANS_VALUE_WITH_MASK_AND_OFFSET(speed_low, fan_speed[CPLD_FAN_SPEED_LOW_REG_INDEX]);
    CPLD_TRANS_VALUE_WITH_MASK_AND_OFFSET(speed_high, fan_speed[CPLD_FAN_SPEED_HIGH_REG_INDEX]);

    if ((speed_high == CODE_FAN_MOTER_STOP) && (speed_low == CODE_FAN_MOTER_STOP))
    {
        *speed = 0;
    }
    else
    {
        temp_speed = ((((u16) speed_high) << 8) | ((u16)speed_low));
        *speed = temp_speed == 0 ? 0 : bd->fan_speed_coef / temp_speed;
    }
exit:

    mutex_unlock(&bsp_fan_speed_lock);
    return ret;
}

int bsp_cpld_get_fan_enable(OUT u8 *enable, int fan_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_fan_enable get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_bit(bd->cpld_addr_fan_enable[fan_index], bd->cpld_offs_fan_enable[fan_index], enable);
    return ret;
}

int bsp_cpld_set_fan_enable(IN u8 enable, int fan_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_set_fan_enable get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_set_bit(bd->cpld_addr_fan_enable[fan_index], bd->cpld_offs_fan_enable[fan_index], enable);
    return ret;
}

int bsp_cpld_get_fan_led_red(OUT u8 *led, int fan_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_fan_led_red get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_bit(bd->cpld_addr_fan_led_red[fan_index], bd->cpld_offs_fan_led_red[fan_index], led);
    return ret;
}

int bsp_cpld_set_fan_led_red(IN u8 led, int fan_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_set_fan_led_red get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_set_bit(bd->cpld_addr_fan_led_red[fan_index], bd->cpld_offs_fan_led_red[fan_index], led);
    return ret;
}

int bsp_cpld_get_fan_led_green(OUT u8 *led, int fan_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_fan_led_green get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_bit(bd->cpld_addr_fan_led_green[fan_index], bd->cpld_offs_fan_led_green[fan_index], led);
    return ret;
}

int bsp_cpld_set_fan_led_green(IN u8 led, int fan_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_set_fan_led_green get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_set_bit(bd->cpld_addr_fan_led_green[fan_index], bd->cpld_offs_fan_led_green[fan_index], led);
    return ret;
}

int bsp_cpld_get_fan_absent(OUT u8 *absent, int fan_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_fan_absent get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_get_bit(bd->cpld_addr_fan_absent[fan_index], bd->cpld_offs_fan_absent[fan_index], absent);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "get_fan_absent failed");

exit:
    return ret;
}

int bsp_cpld_get_fan_direction(OUT u8 *direction, int fan_index, int moter_index)
{
    int ret = ERROR_SUCCESS;
    //cpld高位对应风扇小编号
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_fan_direction get bd failed");
        return -EINVAL;
    }
    //选风扇
    //bsp_cpld_write_part(moter_abs_index, bd->cpld_addr_fan_select, bd->cpld_mask_fan_select, bd->cpld_offs_fan_select);
    ret = bsp_cpld_get_bit(bd->cpld_addr_fan_direction[fan_index], bd->cpld_offs_fan_direction[fan_index], direction);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "get_fan_direction failed");

exit:
    return ret;
}
//只支持2个状态寄存器,返回2个寄存器的合成值
int bsp_cpld_get_fan_status(OUT u8 *status, int fan_index)
{
    u8 status_0 = 0;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_fan_status get bd failed");
        return -EINVAL;
    }
    ret = bsp_cpld_read_byte(&status_0, bd->cpld_addr_fan_status[fan_index]);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "get fan status reg failed");
    CPLD_TRANS_VALUE_WITH_MASK_AND_OFFSET(status_0, fan_status[fan_index]);

    *status = status_0;
exit:
    return ret;
}

//电源
int bsp_cpld_get_psu_absent(OUT u8 *absent, int psu_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_psu_absent get bd failed");
        return -EINVAL;
    }

    if (psu_index >= bd->psu_num)
    {
        DBG_ECHO(DEBUG_ERR, "param err: psu_index %d (>= %d)", psu_index, (int)bd->psu_num);
        return -EINVAL;
    }
    ret = bsp_cpld_get_bit(bd->cpld_addr_psu_absent[psu_index], bd->cpld_offs_psu_absent[psu_index], absent);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "get psu absent bit failed!");

    DBG_ECHO(DEBUG_DBG, "psu %d absent=%d", psu_index + 1, *absent);

exit:
    return ret;
}

int bsp_cpld_get_psu_good(OUT u8 *good, int psu_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_psu_good get bd failed");
        return -EINVAL;
    }

    if (psu_index >= bd->psu_num)
    {
        DBG_ECHO(DEBUG_ERR, "param err: psu_index %d (>= %d)", psu_index, (int)bd->psu_num);
        return -EINVAL;
    }
    ret = bsp_cpld_get_bit(bd->cpld_addr_psu_good[psu_index], bd->cpld_offs_psu_good[psu_index], good);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "get psu good bit failed!");

    DBG_ECHO(DEBUG_DBG, "psu %d good=%d", psu_index + 1, *good);
exit:
    return ret;
}

/****************sub slot ***************************/

int bsp_cpld_get_slot_absent(OUT u8 *absent, int slot_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_slot_absent get bd failed");
        return -EINVAL;
    }

    if (slot_index == MAIN_BOARD)
    {
        *absent = 0;
        return ERROR_SUCCESS;
    }

    if (slot_index > bd->slot_num)
    {
        DBG_ECHO(DEBUG_ERR, "param err: slot_index %d (> %d)", slot_index, (int)bd->slot_num);
        return -EINVAL;
    }
    ret = bsp_cpld_get_bit(bd->cpld_addr_slot_absent[slot_index], bd->cpld_offs_slot_absent[slot_index], absent);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "get slot %d absent bit failed!", slot_index);

    DBG_ECHO(DEBUG_DBG, "slot %d absent=%d", slot_index, *absent);

exit:
    return ret;
}

//power_on = TRUE, power on; power_on = FALSE, power off
int bsp_cpld_slot_power_enable(int slot_index, int power_on)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_slot_power_enable get bd failed");
        return -EINVAL;
    }

    if (slot_index > bd->slot_num)
    {
        DBG_ECHO(DEBUG_ERR, "param err: slot_index %d (> %d)", slot_index, (int)bd->slot_num);
        return -EINVAL;
    }
    ret = bsp_cpld_set_bit(bd->cpld_addr_slot_power_en[slot_index], bd->cpld_offs_slot_power_en[slot_index], power_on);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "set slot %d power_on bit failed!", slot_index);

exit:
    return ret;
}

int bsp_cpld_get_card_power_ok(OUT u8 *power_ok, int slot_index)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_get_card_power_ok get bd failed");
        return -EINVAL;
    }

    if (slot_index == MAIN_BOARD)
    {
        *power_ok = 1;
        return ERROR_SUCCESS;
    }

    if (slot_index > bd->slot_num)
    {
        DBG_ECHO(DEBUG_ERR, "param err: slot_index %d (> %d)", slot_index, (int)bd->slot_num);
        return -EINVAL;
    }
    ret = bsp_cpld_get_bit(bd->cpld_addr_card_power_ok[slot_index], bd->cpld_offs_card_power_ok[slot_index], power_ok);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "get slot power_ok bit failed!");

    DBG_ECHO(DEBUG_DBG, "slot %d get power_ok=%d", slot_index, *power_ok);

exit:
    return ret;
}

//enable 255buffer, enable=TRUE, open; enable=FALSE, closed
int bsp_cpld_slot_buffer_enable(int slot_index, int enable)
{
    int ret = ERROR_SUCCESS;
    u8 enable1 = 0;
    u8 enable2 = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_set_slot_reset get bd failed");
        return -EINVAL;
    }

    if (slot_index > bd->slot_num)
    {
        DBG_ECHO(DEBUG_ERR, "param err: slot_index %d (>= %d)", slot_index, (int)bd->slot_num);
        return -EINVAL;
    }
    enable1 = (enable == TRUE) ? 0 : 0x3;
    enable2 = (enable == TRUE) ? 0 : 0x1;

    ret = bsp_cpld_write_part(enable1, bd->cpld_addr_slot_buff_oe1[slot_index], bd->cpld_mask_slot_buff_oe1[slot_index], bd->cpld_offs_slot_buff_oe1[slot_index]);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "set slot %d buffer1 enable bit failed!", slot_index);

    ret = bsp_cpld_set_bit(bd->cpld_addr_slot_buff_oe2[slot_index], bd->cpld_offs_slot_buff_oe2[slot_index], enable2);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "set slot %d buffer2 enable bit failed!", slot_index);

exit:
    return ret;
}

int bsp_cpld_miim_enable(int enable)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    ret = bsp_cpld_set_bit(bd->cpld_addr_miim_enable, bd->cpld_offs_miim_enable, enable);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "miim enable val %d failed!", enable);
exit:
    return ret;
}

//子卡复位，写1解复位，0复位
int bsp_cpld_set_slot_reset(int slot_index, int reset)
{
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_set_slot_reset get bd failed");
        return -EINVAL;
    }
    if (bd->cpld_addr_slot_reset[slot_index] != 0)
    {
        ret = bsp_cpld_set_bit(bd->cpld_addr_slot_reset[slot_index], bd->cpld_offs_slot_reset[slot_index], reset);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "slot index %d set reset %d failed", slot_index, reset);
    }
exit:
    return ret;
}

void bsp_cpld_reset_max6696(int max_6696_index)
{
    u8 reset_value_a = 1;
    u8 reset_value_b = 0;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_cpld_reset_max6696 get bd failed");
        return ;
    }

    if ((max_6696_index >= 0) && (max_6696_index < bd->max6696_num))
    {
        if (bd->cpld_addr_max6696_rst[max_6696_index] != 0)
        {
            bsp_cpld_set_bit(bd->cpld_addr_max6696_rst[max_6696_index], bd->cpld_offs_max6696_rst[max_6696_index], reset_value_a);
            mdelay(100);
            bsp_cpld_set_bit(bd->cpld_addr_max6696_rst[max_6696_index], bd->cpld_offs_max6696_rst[max_6696_index], reset_value_b);
            mdelay(100);
        }
        else
        {
            DBG_ECHO(DEBUG_INFO, "max6696 %d reset reg not defined!", max_6696_index);
        }
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "max6696 index %d error!", max_6696_index);
    }
    return;
}

/*************************************
end for cpld device method
**************************************/

/*初始化i2c总线*/
int i2c_init(void)
{
    u8 i = 0;
    int ret = -ENODEV;
    int use_smbus_index = bsp_get_board_data()->smbus_use_index;

    memset(&i2c_diag_info, 0, sizeof(i2c_diag_info));

    DBG_ECHO(DEBUG_INFO, "I2C SMBus init started...")

    for (i = 0; i < MAX_SMBUS_NUM; i++)
    {
        smbus = i2c_get_adapter(i);
        if ((NULL != smbus) && (NULL != strstr(smbus->name, "SMBus")))
        {
            DBG_ECHO(DEBUG_INFO, "I2C adapter[%d] %s (ptr=0x%lx)", i, smbus->name, (unsigned long)smbus);
            break;
        }
        if (NULL != smbus)
        {
            i2c_put_adapter(smbus);
            smbus = NULL;
        }
    }

    if (NULL != smbus)
    {
        DBG_ECHO(DEBUG_INFO, "use smbus %d = %s", use_smbus_index, smbus->name);
        ret = ERROR_SUCCESS;
    }
    else
    {
        DBG_ECHO(DEBUG_INFO, "I2C SMBus[%d] is NULL", use_smbus_index);
        ret = -ENODEV;
    }

    /*把内核的retry 去掉*/
    smbus->retries = 0;
    mutex_init(&bsp_i2c_path_lock);

    DBG_ECHO(DEBUG_INFO, "I2C SMBus init end. ret = %d", ret)
    return ret;
}

int i2c_deinit(void)
{
    i2c_put_adapter(smbus);
    return ERROR_SUCCESS;
}


//打印in_buf的内容，整理后的字符中存在out_buf中
size_t bsp_print_memory(u8 *in_buf, ssize_t in_buf_len, s8 *out_string_buf, ssize_t out_string_buf_len, unsigned long start_address, unsigned char addr_print_len)
{
#define BYTES_PER_LINE      16
#define MAX_OUT_BUF_LEN     PAGE_SIZE      //最大一页4096
#define TEMP_BUF_LEN        MAX_OUT_BUF_LEN
#define LOW_MASK            (BYTES_PER_LINE-1)
#define MAX_LEN_PERLINE     256

    s32 i, j, len = 0, len_temp = 0;
    unsigned long temp_start_address = 0;
    unsigned long blank_count = 0;
    unsigned long blank_count_tail = 0;
    u8 temp_char = '\0';
    u8 *temp_buffer = NULL;
    u8 address_format[50] = {0};
    u8 temp2[MAX_LEN_PERLINE + 4] = {0};

    if (out_string_buf_len > MAX_OUT_BUF_LEN)
    {
        DBG_ECHO(DEBUG_ERR, "param err: out_string_buf_len(%d) larger than MAX_OUT_BUF_LEN(%ld)", (int)out_string_buf_len, MAX_OUT_BUF_LEN);
        return 0;
    }
    temp_buffer = (s8 *)kmalloc(TEMP_BUF_LEN, GFP_KERNEL);
    if (NULL == temp_buffer)
    {
        DBG_ECHO(DEBUG_ERR, "kmalloc for temp_buffer failed");
        return 0;
    }
    memset(temp_buffer, 0, TEMP_BUF_LEN);

    temp_start_address = start_address & (~LOW_MASK);
    blank_count = start_address & LOW_MASK;
    blank_count_tail = LOW_MASK - ((start_address + in_buf_len) & LOW_MASK);
    //一行全是空格，不用再显示这行
    blank_count_tail = blank_count_tail == LOW_MASK ? 0 : blank_count_tail;

    //地址显示格式
    scnprintf(address_format, sizeof(address_format), "%c%02dx: ", '%', addr_print_len);

    for (i = 0; i < in_buf_len + blank_count + blank_count_tail; i += BYTES_PER_LINE)
    {
        len += scnprintf(temp_buffer + len, TEMP_BUF_LEN - len, address_format, i + temp_start_address);
        len_temp = 0;
        for (j = 0; j < BYTES_PER_LINE; j++)
        {
            if ((i + j) < blank_count || i + j >= in_buf_len + blank_count)
            {
                temp_char = ' ';

                len += scnprintf(temp_buffer + len, TEMP_BUF_LEN - len, "   ");
            }
            else
            {
                temp_char = in_buf[i + j - blank_count];
                len += scnprintf(temp_buffer + len, TEMP_BUF_LEN - len, "%02x ", temp_char);
            }
            if (j == BYTES_PER_LINE / 2 - 1)
            {
                len += scnprintf(temp_buffer + len, TEMP_BUF_LEN - len, " ");
            }
            len_temp += scnprintf(temp2 + len_temp, sizeof(temp2) - len_temp, "%c", (temp_char >= ' ' && temp_char <= '~') ? temp_char : '.');

            if (len_temp >= MAX_LEN_PERLINE)
            {
                DBG_ECHO(DEBUG_INFO, "current line string %d reaches max %d, break line.", len_temp, MAX_LEN_PERLINE);
                break;
            }
        }

        temp2[len_temp] = '\0';
        len += scnprintf(temp_buffer + len, TEMP_BUF_LEN - len, " * %s *\n", temp2);
    }

    len_temp = len >= (out_string_buf_len - 1) ? out_string_buf_len - 1 : len;
    memcpy(out_string_buf, temp_buffer, len_temp);
    out_string_buf[len_temp] = '\0';

    kfree(temp_buffer);

    return len_temp + 1;

#undef BYTES_PER_LINE
#undef MAX_OUT_BUF_LEN
#undef TEMP_BUF_LEN
#undef LOW_MASK
#undef MAX_LEN_PERLINE

}


void bsp_i2c_reset_smbus_host(void)
{
    int ret = ERROR_SUCCESS;
    u8 temp_value = 0;

    struct pci_dev *pdev = pci_get_domain_bus_and_slot(0, 0x00, PCI_DEVFN(0x1f, 0x03));

    ret = pci_read_config_byte(pdev, 0x40, &temp_value);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "pci_read_config_byte failed ret=%d", ret);
        return;
    }

    temp_value |= 8;

    ret = pci_write_config_byte(pdev, 0x40, temp_value);
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "pci_write_config_byte value=%d failed ret=%d", temp_value, ret);
        return;
    }

    udelay(1000);

    return;
}


/*用cpld模拟9个时钟脉冲来复位slave
 *这个接口提供给sysfs调试使用
 */
int bsp_reset_smbus_slave(int i2c_device_id)
{
    int i = 0;
    u8 temp_value = 0;
    int ret = ERROR_SUCCESS;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_reset_smbus_slave get bd failed");
        return -EINVAL;
    }

    mutex_lock(&bsp_i2c_path_lock);
    current_i2c_path_id = i2c_device_id;
    for (i = 0; i < 9; i++)
    {
        //write high
        bsp_cpu_cpld_read_byte(&temp_value, bd->cpld_addr_gpio_i2c_0);
        temp_value = (1 << bd->cpld_offs_gpio_i2c_0) | temp_value;
        bsp_cpu_cpld_write_byte(temp_value, bd->cpld_addr_gpio_i2c_0);
        udelay(5);
        //write low
        bsp_cpu_cpld_read_byte(&temp_value, bd->cpld_addr_gpio_i2c_0);
        temp_value = (~(1 << bd->cpld_offs_gpio_i2c_0)) & temp_value;
        bsp_cpu_cpld_write_byte(temp_value, bd->cpld_addr_gpio_i2c_0);
        udelay(5);
    }

    current_i2c_path_id = current_i2c_path_id + 10000;
    mutex_unlock(&bsp_i2c_path_lock);
    udelay(1000);

    return ret;
}


void bsp_h3c_open_init_log(void)
{
    log_to_private_file = TRUE;
}

void bsp_h3c_close_init_log(void)
{
    log_to_private_file = FALSE;
}

void bsp_h3c_dmesg_filter_on(void)
{
    log_filter_to_dmesg = TRUE;
}

void bsp_h3c_dmesg_filter_off(void)
{
    log_filter_to_dmesg = FALSE;
}

static int bsp_init_subcard(board_static_data *mainboard)
{
    int slot_index = FIRST_EXP_BOARD;
    int ret = ERROR_SUCCESS;
    int slot_num = mainboard->slot_num;
    u8 absent = TRUE;
    int card_pdt_type = PDT_TYPE_BUTT;
    //int slot_type = PDT_TYPE_BUTT;

    memset(&sub_board_data, 0, sizeof(sub_board_data));

    for (slot_index = FIRST_EXP_BOARD; slot_index <= slot_num; slot_index ++)
    {
        //子卡数据通过initialized=TRUE判断有效性，不通过NULL判断
        mainboard->sub_slot_info[slot_index] = &sub_board_data[slot_index];

        if (bsp_cpld_get_slot_absent(&absent, slot_index) != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "get slot %d absent status from cpld failed!", slot_index);
            continue;
        }
        msleep(10);

        if (absent == TRUE)
        {
            //子卡不在位，直接过
            DBG_ECHO(DEBUG_INFO, "slot %d is absent.", slot_index);
            continue;
        }
        else
        {
            DBG_ECHO(DEBUG_INFO, "detect %d inserted!", slot_index);
        }

        if (bsp_cpld_slot_power_enable(slot_index, TRUE) != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "set slot %d power on failed!", slot_index);
            continue;
        }
        msleep(10);

        //解复位子卡
        if ((ret = bsp_cpld_set_slot_reset(slot_index, 1)) != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "slot %d reset failed", slot_index);
            continue;
        }
        msleep(10);

        if (bsp_cpld_slot_buffer_enable(slot_index, TRUE) != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "set slot %d buffer open failed!", slot_index);
            continue;
        }
        msleep(10);
        if (bsp_get_card_product_type(&card_pdt_type, slot_index) != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "get card product type for slot index %d failed!", slot_index);
            continue;
        }
        msleep(10);
        //主板寄存器
        if (bsp_cpld_miim_enable(TRUE) != ERROR_SUCCESS)
        {
            DBG_ECHO(DEBUG_ERR, "set miim enable failed!");
        }
        msleep(10);

        //slot is present, initialize the data

        switch (card_pdt_type)
        {
            case PDT_TYPE_TCS83_120F_32H_SUBCARD:
                if ((ret = board_static_data_init_TCS83_120F_32H_subcard(slot_index, mainboard)) != ERROR_SUCCESS)
                {
                    DBG_ECHO(DEBUG_ERR, "initialize slot %d card type %d failed! ret=%d", slot_index, card_pdt_type, ret);
                    continue;
                }
                else
                {
                    DBG_ECHO(DEBUG_INFO, "slot %d card type %d static data initialized!", slot_index, card_pdt_type);
                }
                break;
            default:
                DBG_ECHO(DEBUG_ERR, "Not support slot %d card type %d !", slot_index, card_pdt_type);
                break;
        }

    }
    return ERROR_SUCCESS;
}

int bsp_h3c_get_file_size(char *filename, int *file_size)
{
    struct file *pfile = NULL;
    pfile = filp_open(filename, O_RDONLY, 0644);
    if (IS_ERR(pfile))
    {
        //printk (KERN_ERR "BSP: bsp_h3c_get_file_size open file %s failed\n", filename);
        return -EIO;
    }
    *file_size = pfile->f_inode->i_size;
    fput(pfile);

    return ERROR_SUCCESS;
}

int bsp_h3c_localmsg_init(void)
{

    int ret0 = ERROR_SUCCESS;
    int ret1 = ERROR_SUCCESS;
    int file_size_0 = 0;
    int file_size_1 = 0;

    mutex_init(&bsp_logfile_lock);
    mutex_init(&bsp_recent_log_lock);
    mutex_init(&bsp_psu_logfile_lock);
    mutex_init(&bsp_i2c_logfile_lock);
    mutex_init(&bsp_wdt_logfile_lock);

    memset(&bsp_recent_log, 0, sizeof(bsp_recent_log));

    ret0 = bsp_h3c_get_file_size(LOG_FILE_PATH_0, &file_size_0);
    ret1 = bsp_h3c_get_file_size(LOG_FILE_PATH_1, &file_size_1);
    if (file_size_0 < LOG_FILE_SIZE && ret0 == ERROR_SUCCESS)
    {
        curr_h3c_log_file = LOG_FILE_PATH_0;
    }
    else if (file_size_1 < LOG_FILE_SIZE && ret1 == ERROR_SUCCESS)
    {
        curr_h3c_log_file = LOG_FILE_PATH_1;
    }
    else
    {
        curr_h3c_log_file = LOG_FILE_PATH_0;
    }

    ret0 = bsp_h3c_get_file_size(LOG_FILE_PATH_2, &file_size_0);
    ret1 = bsp_h3c_get_file_size(LOG_FILE_PATH_3, &file_size_1);
    if (file_size_0 < LOG_PSU_FILE_SIZE && ret0 == ERROR_SUCCESS)
    {
        curr_h3c_psu_log_file = LOG_FILE_PATH_2;
    }
    else if (file_size_1 < LOG_PSU_FILE_SIZE && ret1 == ERROR_SUCCESS)
    {
        curr_h3c_psu_log_file = LOG_FILE_PATH_3;
    }
    else
    {
        curr_h3c_psu_log_file = LOG_FILE_PATH_2;
    }

    ret0 = bsp_h3c_get_file_size(LOG_FILE_I2C_PATH1, &file_size_0);
    ret1 = bsp_h3c_get_file_size(LOG_FILE_I2C_PATH2, &file_size_1);
    if (file_size_0 < LOG_WDT_I2C_FILE_SIZE && ret0 == ERROR_SUCCESS)
    {
        curr_h3c_i2c_log_file = LOG_FILE_I2C_PATH1;
    }
    else if (file_size_1 < LOG_WDT_I2C_FILE_SIZE && ret1 == ERROR_SUCCESS)
    {
        curr_h3c_i2c_log_file = LOG_FILE_I2C_PATH2;
    }
    else
    {
        curr_h3c_i2c_log_file = LOG_FILE_I2C_PATH1;
    }

    ret0 = bsp_h3c_get_file_size(LOG_FILE_WDT_PATH1, &file_size_0);
    ret1 = bsp_h3c_get_file_size(LOG_FILE_WDT_PATH2, &file_size_1);
    if (file_size_0 < LOG_WDT_I2C_FILE_SIZE && ret0 == ERROR_SUCCESS)
    {
        curr_h3c_wdt_log_file = LOG_FILE_WDT_PATH1;
    }
    else if (file_size_1 < LOG_WDT_I2C_FILE_SIZE && ret1 == ERROR_SUCCESS)
    {
        curr_h3c_wdt_log_file = LOG_FILE_WDT_PATH2;
    }
    else
    {
        curr_h3c_wdt_log_file = LOG_FILE_WDT_PATH1;
    }
    //printk (KERN_INFO "BSP: %s size=%d %s size=%d set logfile to %s\n", LOG_FILE_PATH_0, file_size_0, LOG_FILE_PATH_1, file_size_1, curr_h3c_log_file);

    return ERROR_SUCCESS;

}

int bsp_h3c_check_log_file(char *log_file_path, int type)
{
    int file_size = 0;
    int file_size_max = 0;
    struct file *pfile = NULL;
    int ret = ERROR_SUCCESS;

    int temp_ret = bsp_h3c_get_file_size(log_file_path, &file_size);

    if (BSP_PSU_LOG_FILE == type)
    {
        file_size_max = LOG_PSU_FILE_SIZE;
    }
    else if (BSP_I2C_LOG_FILE == type)
    {
        file_size_max = LOG_WDT_I2C_FILE_SIZE;
    }
    else if (BSP_WDT_LOG_FILE == type)
    {
        file_size_max = LOG_WDT_I2C_FILE_SIZE;
    }
    else
    {
        file_size_max = LOG_FILE_SIZE;
    }

    if (file_size >= file_size_max && temp_ret == ERROR_SUCCESS)
    {
        if (BSP_PSU_LOG_FILE == type)
        {
            curr_h3c_psu_log_file = log_file_path = (strcmp(log_file_path, LOG_FILE_PATH_2) == 0) ? LOG_FILE_PATH_3 : LOG_FILE_PATH_2;
        }
        else if (BSP_I2C_LOG_FILE == type)
        {
            curr_h3c_i2c_log_file = log_file_path = (strcmp(log_file_path, LOG_FILE_I2C_PATH1) == 0) ? LOG_FILE_I2C_PATH2 : LOG_FILE_I2C_PATH1;
        }
        else if (BSP_WDT_LOG_FILE == type)
        {
            curr_h3c_wdt_log_file = log_file_path = (strcmp(log_file_path, LOG_FILE_WDT_PATH1) == 0) ? LOG_FILE_WDT_PATH2 : LOG_FILE_WDT_PATH1;
        }
        else
        {
            curr_h3c_log_file = log_file_path = (strcmp(log_file_path, LOG_FILE_PATH_0) == 0) ? LOG_FILE_PATH_1 : LOG_FILE_PATH_0;
        }

        pfile = filp_open(log_file_path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (IS_ERR(pfile))
        {
            printk(KERN_ERR "BSP: open logfile %s failed!\n", log_file_path);
            ret = -EIO;
        }
        else
        {
            fput(pfile);
            ret = ERROR_SUCCESS;
        }
    }

    return ret;
}

int bsp_h3c_log_filter_to_dmesg(int log_level, const char *log_str, const char *src_file_name, unsigned int line_no)
{
    int i = 0;
    int found_hit = 0;
    int curr_index = 0;

    if ((log_level & bsp_dmesg_log_level) == 0)
        return ERROR_SUCCESS;

    if (log_str == NULL || src_file_name == NULL)
    {
        printk(KERN_ERR "BSP: param error, log_str=%p src_file=%p\n", log_str, src_file_name);
        return -EINVAL;
    }

    mutex_lock(&bsp_recent_log_lock);

    for (i = 0; i < BSP_LOG_FILETER_RECENT_LOG_COUNT; i++)
    {
        if (bsp_recent_log.used[i] == 1 && bsp_recent_log.line_no[i] == line_no)
        {
            //There is the log recently, skip
            if (/*bsp_recent_log.filename[i] != NULL &&*/ strcmp(bsp_recent_log.filename[i], src_file_name) == 0)
            {
                bsp_recent_log.hit_count[i]++;
                found_hit = 1;
                break;
            }
        }
    }

    if (found_hit != 1)
    {
        curr_index = bsp_recent_log.curr_record_index;
        bsp_recent_log.used[curr_index] = 1;
        bsp_recent_log.line_no[curr_index] = line_no;
        bsp_recent_log.hit_count[curr_index] = 1;
        strlcpy(bsp_recent_log.filename[curr_index], src_file_name, sizeof(bsp_recent_log.filename[curr_index]));
        bsp_recent_log.curr_record_index = (curr_index >= BSP_LOG_FILETER_RECENT_LOG_COUNT - 1) ? 0 : (curr_index + 1);
        printk(KERN_ERR"%s\n", log_str);
    }
    else if (log_filter_to_dmesg == FALSE)
    {
        printk(KERN_ERR"%s\n", log_str);
    }

    mutex_unlock(&bsp_recent_log_lock);

    return ERROR_SUCCESS;
}

//src_file and line_no is the key for log filter
int bsp_h3c_localmsg_to_file(char *buf, long len, int log_level, const char *src_file, unsigned int line_no, LOG_TYPE type)
{
    int ret = ERROR_SUCCESS;
    struct file *pfile = NULL;
    long len_s = 0;
    loff_t pos = 0;
    bool log_to_private_file_tmp = TRUE;
    char *log_file_path;

    if (NULL == buf)
    {
        printk(KERN_ERR "function params buf = %p, LOG_TYPE = %d is invalid.\n", buf, type);
        ret = -EINVAL;
        goto exit_no_lock;
    }
    if (BSP_PSU_LOG_FILE == type)
    {
        if (DEBUG_ERR != log_level)
        {
            ret = -EINVAL;
            goto exit_no_lock;
        }
        log_file_path = curr_h3c_psu_log_file;
        mutex_lock(&bsp_psu_logfile_lock);

    }
    else if (BSP_I2C_LOG_FILE == type)
    {
        log_file_path = curr_h3c_i2c_log_file;
        mutex_lock(&bsp_i2c_logfile_lock);
    }
    else if (BSP_WDT_LOG_FILE == type)
    {
        log_file_path = curr_h3c_wdt_log_file;
        mutex_lock(&bsp_wdt_logfile_lock);
    }
    else
    {
        log_file_path = curr_h3c_log_file;
        mutex_lock(&bsp_logfile_lock);
        bsp_h3c_log_filter_to_dmesg(log_level, buf, src_file, line_no);
        log_to_private_file_tmp = log_to_private_file;
    }

    if (TRUE == log_to_private_file_tmp)
    {
        if (bsp_h3c_check_log_file(log_file_path, type) < 0)
        {
            printk(KERN_ERR "BSP: bsp_h3c_check_log_file failed!\n");
        }
        pfile = filp_open(log_file_path, O_RDWR | O_CREAT | O_APPEND, 0644);
        if (IS_ERR(pfile))
        {
            printk(KERN_ERR "file %s filp_open error.\n", log_file_path);
            ret = -EIO;
            goto exit;
        }
        else
        {
            char log_str[LOG_STRING_LEN + 64] = {0};
            struct rtc_time tm = {0};
            u64 timezone_sec_diff = (u64)sys_tz.tz_minuteswest * 60;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
            rtc_time_to_tm(get_seconds() - timezone_sec_diff, &tm);
#else
            rtc_time64_to_tm(ktime_get_real_seconds() - timezone_sec_diff, &tm);
#endif
            if (BSP_I2C_LOG_FILE == type)
            {
                len_s = scnprintf(log_str, sizeof(log_str), "%04d-%02d-%02d %02d:%02d:%02d jiffies:%ld, pid:%d, pid_name:%s, current_i2c_path_id:%d, operation:%s\n",
                                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, jiffies, current->pid, current->comm, current_i2c_path_id, buf);
            }
            else
            {
                len_s = scnprintf(log_str, sizeof(log_str), "%04d-%02d-%02d %02d:%02d:%02d %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, buf);
            }
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
            //4.9
            ret = kernel_write(pfile, log_str, len_s, pos);
#else
            //4.19
            ret = kernel_write(pfile, log_str, len_s, &pos);
#endif
            if (0 > ret)
            {
                printk(KERN_ERR "Error writing local msg file: %s.\n", log_file_path);
            }

            fput(pfile);
        }
    }

exit:
    if (BSP_PSU_LOG_FILE == type)
    {
        mutex_unlock(&bsp_psu_logfile_lock);
    }
    else if (BSP_I2C_LOG_FILE == type)
    {
        mutex_unlock(&bsp_i2c_logfile_lock);
    }
    else if (BSP_WDT_LOG_FILE == type)
    {
        mutex_unlock(&bsp_wdt_logfile_lock);
    }
    else
    {
        mutex_unlock(&bsp_logfile_lock);
    }
exit_no_lock:

    return (ret < 0) ? -EIO : ERROR_SUCCESS;
}

int parameter_int_to_float_deal(int value, int *integer_value, int *float_value)
{
    int temp_value = 0;
    *integer_value = value / 100;
    temp_value = abs(value) % 100;
    *float_value = temp_value * 10;
    return 0;
}

int drv_rtc_Read(RTC_DS1337_REG_E enAddr, UINT uilen, UCHAR *upcData)
{
    int ret = 0;
    ret = bsp_i2c_common_eeprom_read_bytes(0x68, enAddr, uilen, upcData, I2C_DEV_RTC_4337);
    return ret;
}

int drv_rtc_Write(RTC_DS1337_REG_E enAddr, UINT uilen, UCHAR *upcData)
{
    int ret = 0;
    int i  = 0;
    for (i = 0; i < uilen; i++)
    {
        ret = bsp_i2c_common_eeprom_write_byte(0x68, enAddr + i, *(upcData + i), I2C_DEV_RTC_4337);
    }
    return ret;
}

ULONG drv_rtc_SetDs1337Time(IN DRV_RTC_TIME_S *pstTime)
{
    ULONG ulRet;
    DRV_RTC_TIME_S *pstTm = NULL;
    volatile UCHAR ucYear;
    UCHAR ucRegValue;
    UCHAR ucRegVal[RTC_DS1337_BUTT] = {0};
    pstTm = pstTime;
    if ((pstTm->iYear < 0) || (pstTm->iYear > 9999) ||
        (pstTm->iMonth < 1) || (pstTm->iMonth > 12) ||
        (pstTm->iDate < 1) || (pstTm->iDate > 31) ||
        (pstTm->iWeek < 0) || (pstTm->iWeek > 6))
    {
        DBG_ECHO(DEBUG_ERR, "Set rtc time failed, time is out of range.");
        DBG_ECHO(DEBUG_ERR, "year: %ld, month: %ld, date: %ld, week: %ld.",
                 pstTm->iYear, pstTm->iMonth, pstTm->iDate, pstTm->iWeek);
        return ERROR_FAILED;
    }
    if ((pstTm->iHour < 0) || (pstTm->iHour > 23) ||
        (pstTm->iMinute < 0) || (pstTm->iMinute > 59) ||
        (pstTm->iSecond < 0) || (pstTm->iSecond > 59))
    {
        DBG_ECHO(DEBUG_ERR, "Set rtc time failed, time is out of range.");
        DBG_ECHO(DEBUG_ERR, "Hour: %ld, Minute: %ld, Second: %ld.",
                 pstTm->iHour, pstTm->iMinute, pstTm->iSecond);
        return ERROR_FAILED;
    }

    ucRegVal[RTC_DS1337_SECONDS] = (((UCHAR)(pstTm->iSecond / 10) << RTC_TIME_HIGH_SHIFT) |
                                    (UCHAR)(pstTm->iSecond % 10));
    ucRegVal[RTC_DS1337_MINUTES] = (((UCHAR)(pstTm->iMinute / 10) << RTC_TIME_HIGH_SHIFT) |
                                    (UCHAR)(pstTm->iMinute % 10));
    ucRegVal[RTC_DS1337_HOURS] = (((UCHAR)(pstTm->iHour / 10) << RTC_TIME_HIGH_SHIFT) |
                                  (UCHAR)(pstTm->iHour % 10));
    ucRegVal[RTC_DS1337_DATE] = (((UCHAR)(pstTm->iDate / 10) << RTC_TIME_HIGH_SHIFT) |
                                 (UCHAR)(pstTm->iDate % 10));
    ucRegVal[RTC_DS1337_MONTH] = (((UCHAR)(pstTm->iMonth / 10) << RTC_TIME_HIGH_SHIFT) |
                                  (UCHAR)(pstTm->iMonth % 10));
    ucYear = (UCHAR)(pstTm->iYear % 100);
    ucRegVal[RTC_DS1337_YEAR] = ((ucYear / 10) << RTC_TIME_HIGH_SHIFT) | (ucYear % 10);
    if (0 == pstTm->iWeek)
    {
        ucRegVal[RTC_DS1337_DAY] = RTC_DS1337_SUNDAY;
    }
    else
    {
        ucRegVal[RTC_DS1337_DAY] = (UCHAR)(pstTm->iWeek);
    }
    ucRegVal[RTC_DS1337_DAY] = 2;
    ucRegValue = 0;
    ulRet = drv_rtc_Write(0, 1, &ucRegValue);
    if (ERROR_SUCCESS != ulRet)
    {
        DBG_ECHO(DEBUG_ERR, "Set rtc time failed.");
        return ERROR_FAILED;
    }
    ulRet = drv_rtc_Write(RTC_DS1337_MINUTES, RTC_DS1337_TIME_REG_NUM - 1, ucRegVal + 1);
    if (ERROR_SUCCESS != ulRet)
    {
        DBG_ECHO(DEBUG_ERR, "Set rtc time failed.");
        return ERROR_FAILED;
    }
    ulRet = drv_rtc_Write(0, 1, &ucRegVal[RTC_DS1337_SECONDS]);
    if (ERROR_SUCCESS != ulRet)
    {
        DBG_ECHO(DEBUG_ERR, "Set rtc time failed.");
        return ERROR_FAILED;
    }
    return ERROR_SUCCESS;
}

ULONG drv_rtc_GetDs1337Time(OUT DRV_RTC_TIME_S *pstTime)
{
    ULONG ulRet;
    UCHAR ucRegVal[RTC_DS1337_BUTT] = {0};
    UCHAR ucTmp;
    ulRet = drv_rtc_Read(RTC_DS1337_SECONDS, RTC_DS1337_TIME_REG_NUM, ucRegVal);
    if (ERROR_SUCCESS != ulRet)
    {
        DBG_ECHO(DEBUG_ERR, "Get rtc time failed.");
        return ERROR_FAILED;
    }
    memset(pstTime, 0, sizeof(DRV_RTC_TIME_S));

    ucTmp = ucRegVal[RTC_DS1337_YEAR];
    pstTime->iYear = (ucTmp >>  RTC_TIME_HIGH_SHIFT) * 10 +
                     (ucTmp & RTC_TIME_LOW_MASK) + RTC_YEAR_BASE;
    ucTmp = ucRegVal[RTC_DS1337_MONTH];
    pstTime->iMonth = ((ucTmp >> RTC_TIME_HIGH_SHIFT) & RTC_MONTH_HIGH_MASK) * 10 +
                      (ucTmp & RTC_TIME_LOW_MASK);
    ucTmp = ucRegVal[RTC_DS1337_DATE];
    pstTime->iDate = ((ucTmp >> RTC_TIME_HIGH_SHIFT) & RTC_DATE_HIGH_MASK) * 10 +
                     (ucTmp & RTC_TIME_LOW_MASK);
    ucTmp = ucRegVal[RTC_DS1337_HOURS];
    pstTime->iHour = ((ucTmp >> RTC_TIME_HIGH_SHIFT) & RTC_HOURS_HIGH_MASK) * 10 +
                     (ucTmp & RTC_TIME_LOW_MASK);
    ucTmp = ucRegVal[RTC_DS1337_MINUTES];
    pstTime->iMinute = ((ucTmp >> RTC_TIME_HIGH_SHIFT) & RTC_MINUTES_HIGH_MASK) * 10 +
                       (ucTmp & RTC_TIME_LOW_MASK);
    ucTmp = ucRegVal[RTC_DS1337_SECONDS];
    pstTime->iSecond = ((ucTmp >> RTC_TIME_HIGH_SHIFT) & RTC_SECONDS_HIGH_MASK) * 10 +
                       (ucTmp & RTC_TIME_LOW_MASK);
    ucTmp = ucRegVal[RTC_DS1337_DAY] & RTC_TIME_LOW_MASK;
    if (RTC_DS1337_SUNDAY == ucTmp)
    {
        pstTime->iWeek = 0;
    }
    else
    {
        pstTime->iWeek = ucTmp;
    }
    return ERROR_SUCCESS;
}
int bsp_base_init(void)
{
    int i = 0;
    int ret = ERROR_SUCCESS;
    int pdt_type = PDT_TYPE_BUTT;
    board_static_data *bd = bsp_get_board_data();
    if (bd == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "check fail: bsp_base_init get bd failed");
        return -EINVAL;
    }

    bsp_h3c_localmsg_init();
    mutex_init(&bsp_fan_speed_lock);

    //必须先初始化核心数据
    ret = bsp_get_product_type(&pdt_type);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "bsp get product type failed!");
    //pdt_type = PDT_TYPE_TCS81_120F_1U;

    switch (pdt_type)
    {
        case PDT_TYPE_TCS81_120F_1U:
            ret = board_static_data_init_TCS81_120F(bd);
            break;
        case PDT_TYPE_TCS83_120F_4U:
            ret = board_static_data_init_TCS83_120F(bd);
            break;
        case PDT_TYPE_LS_9855_48CD8D_W1:
            ret = board_static_data_init_LS_9855_48CD8D_W1(bd);
            break;
        case PDT_TYPE_LS_9825_64D_W1:
            ret = board_static_data_init_LS_9825_64D_W1(bd);
            break;
        case PDT_TYPE_LS_9855_32CDF_W1:
            ret = board_static_data_init_LS_9855_32CDF_W1(bd);
            break;
        default:
            DBG_ECHO(DEBUG_ERR, "pdt_type=0x%x, not supported!\n", pdt_type);
            ret = -EINVAL;
            goto exit;
            break;
    }
    CHECK_IF_ERROR_GOTO_EXIT(ret, "static data init failed!");

    ret = bsp_cpld_init();
    CHECK_IF_ERROR_GOTO_EXIT(ret, "cpld init failed!");

    ret = i2c_init();
    CHECK_IF_ERROR_GOTO_EXIT(ret, "i2c init failed!");

    mutex_init(&syseeprom_table_lock[0]);
    for (i = 0; i < bd->fan_num; i++)
    {
        mutex_init(&fan_table_lock[i]);
    }
    for (i = 0; i < bd->psu_num; i++)
    {
        mutex_init(&psu_table_lock[i]);
    }
    // if (bsp_set_cpu_init_ok(1) != ERROR_SUCCESS)
    // {
    //     DBG_ECHO(DEBUG_ERR, "CPU init ok bit set failed!");
    // }
    if (bsp_set_mac_rov() != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "MAC voltage set failed!");
    }
    if (bd->slot_num > 0)
    {
        //子卡初始化
        bsp_init_subcard(bd);
    }

exit:
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_base_init failed, ret = %d!", ret);
    }
    return ret;
}

s32 bsp_i2c_isl68127_read_black_box(u16 dev_i2c_address, u32 *data, I2C_DEVICE_E i2c_device_index)
{
    s32 ret = ERROR_SUCCESS;
    u32 *pblack_box_addr_getter = (u32*)kmalloc(sizeof(u32), GFP_KERNEL);
    u32 black_box_addr = 0x0;
    u16 i = 0;
    u32 black_box_dma_address = 0x1088;

    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_WORD_DATA, I2C_SMBUS_WRITE, REG_ADDR_ISL68127_CMD_BLOCK_BOX, ISL68127_CMD_BLOCK_BOX_DATA_WRITE_LEN, (void *)&black_box_dma_address, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "isl68127 write black box cmd set REG_ADDR_ISL68127_CMD_BLOCK_BOX 1088 failed");

    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_BLOCK_DATA, I2C_SMBUS_READ, REG_ADDR_ISL68127_CMD_BLOCK_BOX_DATA, ISL68127_CMD_BLOCK_BOX_DATA_READ_LEN, (void *)pblack_box_addr_getter, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "isl68127 read black box cmd get black_box_addr_getter_addr failed");

    black_box_addr = (*pblack_box_addr_getter + 3800)/4 + 0x1000;

    for (i = black_box_addr; i < black_box_addr + 18; i++)
    {
        ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_WORD_DATA, I2C_SMBUS_WRITE, REG_ADDR_ISL68127_CMD_BLOCK_BOX, ISL68127_CMD_BLOCK_BOX_DATA_WRITE_LEN, (void *)&i, i2c_device_index);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "isl68127 read black box cmd set REG_ADDR_ISL68127_CMD_BLOCK_BOX failed");

        ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_BLOCK_DATA, I2C_SMBUS_READ, REG_ADDR_ISL68127_CMD_BLOCK_BOX_DATA, ISL68127_CMD_BLOCK_BOX_DATA_READ_LEN, (void *)data, i2c_device_index);
        data ++;
        msleep(200);
        CHECK_IF_ERROR_GOTO_EXIT(ret, "isl68127 read black box cmd set REG_ADDR_ISL68127_CMD_BLOCK_BOX failed");
    }
exit:
    kfree(pblack_box_addr_getter);
    return ret;
}


/*read ram*/
s32 bsp_i2c_ra228_read_black_box(u16 dev_i2c_address, u32 *data, I2C_DEVICE_E i2c_device_index)
{
    s32 ret = ERROR_SUCCESS;
    u64 device_revision = 0;
    u32 *pblack_box_addr_getter = (u32*)kmalloc(sizeof(u32),GFP_KERNEL);
    u32 black_box_addr = 0x0;
    u16 i = 0;
    u16 err_count = 0;
    u32 temp_value = 0xff;
    u16 black_box_dma_address = 0xC7;
    u32 addr_end = 0;

    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_BLOCK_DATA, I2C_SMBUS_READ, REG_ADDR_RA228_CMD_IC_DEVICE_ADDR, RA228_CMD_BLOCK_BOX_DATA_READ_LEN_3, (void *)&device_revision, i2c_device_index);

    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_WORD_DATA, I2C_SMBUS_WRITE, REG_ADDR_RA228_CMD_BLOCK_BOX_ADDR, RA228_CMD_BLOCK_BOX_DATA_WRITE_LEN, (void *)&black_box_dma_address, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "ra228 write black box cmd set 0xC7 failed");

    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_BLOCK_DATA, I2C_SMBUS_READ, REG_ADDR_RA228_CMD_BLOCK_BOX_DATA, RA228_CMD_BLOCK_BOX_DATA_READ_LEN_2, (void *)pblack_box_addr_getter, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "ra228 get black box start address cmd 0xC5 failed");
    black_box_addr = (*pblack_box_addr_getter)/4;

    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_WORD_DATA, I2C_SMBUS_WRITE, REG_ADDR_RA228_CMD_BLOCK_BOX_ADDR, RA228_CMD_BLOCK_BOX_DATA_WRITE_LEN, (void *)&black_box_addr, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "ra228 write black box cmd 0xC7 failed");

    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_BLOCK_DATA, I2C_SMBUS_READ, REG_ADDR_RA228_CMD_BLOCK_BOX_DATA, RA228_CMD_BLOCK_BOX_DATA_READ_LEN_2, (void *)&temp_value, i2c_device_index);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "ra228 read black box cmd 0xC5 failed");

    if( 0 == temp_value )
    {
        for (err_count = 0; err_count < 20; err_count++)
        {
            ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_BLOCK_DATA, I2C_SMBUS_READ, REG_ADDR_RA228_CMD_BLOCK_BOX_DATA, RA228_CMD_BLOCK_BOX_DATA_READ_LEN_2, (void *)&temp_value, i2c_device_index);
            msleep(200);
        }
    }

    for (i = 0; i < 46; i++)
    {
        ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_BLOCK_DATA, I2C_SMBUS_READ, REG_ADDR_RA228_CMD_BLOCK_BOX_SEQUENTIAL, RA228_CMD_BLOCK_BOX_DATA_READ_LEN_2, (void *)data, i2c_device_index);
        data ++;
        CHECK_IF_ERROR_GOTO_EXIT(ret, "ra228 read black box data cmd failed");
    }
    ret = bsp_h3c_i2c_smbus_xfer(dev_i2c_address, i2c_flag_WORD_DATA, I2C_SMBUS_READ, REG_ADDR_RA228_CMD_BLOCK_BOX_ADDR, RA228_CMD_BLOCK_BOX_DATA_READ_LEN_1, (void *)&addr_end, i2c_device_index);
    if (addr_end != (black_box_addr + 46))
    {
        ret = ERROR_FAILED;
        CHECK_IF_ERROR_GOTO_EXIT(ret, "ra228 read black box data length error, please check");
    }
exit:
    kfree(pblack_box_addr_getter);
    return ret;
}
EXPORT_SYMBOL(kobj_switch);
EXPORT_SYMBOL(kobj_debug);
EXPORT_SYMBOL(bsp_module_debug_level);
EXPORT_SYMBOL(bsp_get_board_data);
EXPORT_SYMBOL(bsp_get_slot_data);
EXPORT_SYMBOL(bsp_cpld_read_byte);
EXPORT_SYMBOL(bsp_cpld_write_byte);
EXPORT_SYMBOL(bsp_cpld_read_part);
EXPORT_SYMBOL(bsp_cpld_write_part);
EXPORT_SYMBOL(bsp_cpld_set_bit);
EXPORT_SYMBOL(bsp_cpld_get_bit);
EXPORT_SYMBOL(bsp_cpu_cpld_read_part);
EXPORT_SYMBOL(bsp_cpu_cpld_write_part);
EXPORT_SYMBOL(bsp_cpu_cpld_read_byte);
EXPORT_SYMBOL(bsp_cpu_cpld_write_byte);
EXPORT_SYMBOL(bsp_slot_cpld_read_byte);
EXPORT_SYMBOL(bsp_slot_cpld_write_byte);
EXPORT_SYMBOL(bsp_slot_cpld_set_bit);
EXPORT_SYMBOL(bsp_slot_cpld_get_bit);
EXPORT_SYMBOL(bsp_cpld_get_size);
EXPORT_SYMBOL(bsp_get_cpu_cpld_size);
EXPORT_SYMBOL(bsp_cpld_get_slot_size);
EXPORT_SYMBOL(bsp_get_product_type);
//EXPORT_SYMBOL(bsp_get_product_name_string);
EXPORT_SYMBOL(bsp_i2c_pmbus_eeprom_read_bytes);
EXPORT_SYMBOL(bsp_i2c_pmbus_eeprom_write_bytes);
EXPORT_SYMBOL(bsp_i2c_24LC128_eeprom_read_bytes);
EXPORT_SYMBOL(bsp_i2c_24LC128_eeprom_write_byte);
EXPORT_SYMBOL(bsp_i2c_24LC128_eeprom_write_bytes);
EXPORT_SYMBOL(bsp_i2c_SFP_read_bytes);
EXPORT_SYMBOL(bsp_i2c_SFP_write_byte);
EXPORT_SYMBOL(bsp_i2c_Max6696_get_temp);
EXPORT_SYMBOL(bsp_cpld_read_mac_inner_temp);
EXPORT_SYMBOL(bsp_cpld_read_mac_width_temp);
EXPORT_SYMBOL(bsp_i2c_Max6696_limit_rw);
EXPORT_SYMBOL(bsp_cpld_reset_max6696);
EXPORT_SYMBOL(bsp_enable_slot_all_9548);
EXPORT_SYMBOL(bsp_enable_slot_all_9545);
#ifdef _PDO
EXPORT_SYMBOL(bsp_adm1166_reg_read_byte);
EXPORT_SYMBOL(bsp_adm1166_reg_write_byte);
#endif
EXPORT_SYMBOL(bsp_i2c_common_eeprom_read_bytes);
EXPORT_SYMBOL(bsp_i2c_common_eeprom_write_byte);
EXPORT_SYMBOL(bsp_i2c_psu_eeprom_read_bytes);
EXPORT_SYMBOL(bsp_i2c_power_reg_read);
EXPORT_SYMBOL(bsp_i2c_ina219_read_reg);
EXPORT_SYMBOL(bsp_i2c_isl68127_read_reg);
EXPORT_SYMBOL(bsp_i2c_isl68127_write_reg);
EXPORT_SYMBOL(bsp_i2c_isl68127_read_black_box);
EXPORT_SYMBOL(bsp_i2c_ra228_read_reg);
EXPORT_SYMBOL(bsp_i2c_ra228_read_black_box);
EXPORT_SYMBOL(bsp_cpld_set_fan_pwm_reg);
EXPORT_SYMBOL(bsp_cpld_get_fan_pwm_reg);
EXPORT_SYMBOL(bsp_cpld_get_fan_speed);
EXPORT_SYMBOL(bsp_cpld_get_fan_enable);
EXPORT_SYMBOL(bsp_cpld_set_fan_enable);
EXPORT_SYMBOL(bsp_cpld_get_fan_absent);
EXPORT_SYMBOL(bsp_cpld_get_fan_status);
EXPORT_SYMBOL(bsp_cpld_set_fan_led_red);
EXPORT_SYMBOL(bsp_cpld_get_fan_led_red);
EXPORT_SYMBOL(bsp_cpld_set_fan_led_green);
EXPORT_SYMBOL(bsp_cpld_get_fan_led_green);
EXPORT_SYMBOL(bsp_cpld_get_fan_direction);
EXPORT_SYMBOL(bsp_cpld_get_psu_absent);
EXPORT_SYMBOL(bsp_cpld_get_psu_good);
EXPORT_SYMBOL(bsp_cpld_get_slot_absent);
EXPORT_SYMBOL(bsp_cpld_get_card_power_ok);
EXPORT_SYMBOL(bsp_print_memory);
EXPORT_SYMBOL(bsp_get_secondary_voltage_value);
EXPORT_SYMBOL(bsp_get_adm116x_get_hwversion_reg);
EXPORT_SYMBOL(bsp_bios_cpld_write_byte);
EXPORT_SYMBOL(bsp_bios_cpld_read_byte);
EXPORT_SYMBOL(bsp_bios_cpld_read_part);
EXPORT_SYMBOL(bsp_bios_cpld_write_part);
EXPORT_SYMBOL(bsp_h3c_localmsg_to_file);
EXPORT_SYMBOL(bsp_get_bios_cpld_size);
EXPORT_SYMBOL(bsp_h3c_open_init_log);
EXPORT_SYMBOL(bsp_h3c_close_init_log);
EXPORT_SYMBOL(bsp_reset_smbus_slave);
EXPORT_SYMBOL(optoe_port_index_convert);
EXPORT_SYMBOL(bsp_sysfs_debug_access_get);
EXPORT_SYMBOL(bsp_sysfs_debug_access_set);
EXPORT_SYMBOL(bsp_sysfs_debug_loglevel_get);
EXPORT_SYMBOL(bsp_sysfs_debug_loglevel_set);
EXPORT_SYMBOL(bsp_mac_inner_temp_lock);
EXPORT_SYMBOL(bsp_mac_width_temp_lock);
EXPORT_SYMBOL(bsp_manu_lock);
EXPORT_SYMBOL(parameter_int_to_float_deal);
EXPORT_SYMBOL(g_u64BiosCpld_addr);
EXPORT_SYMBOL(bsp_h3c_get_file_size);
EXPORT_SYMBOL(psu_table_lock);
EXPORT_SYMBOL(fan_table_lock);
EXPORT_SYMBOL(syseeprom_table_lock);
EXPORT_SYMBOL(bsp_get_adm116x_fault_record);
EXPORT_SYMBOL(bsp_set_adm116x_fault_record);
EXPORT_SYMBOL(bsp_i2c_adm116x_read_reg);
EXPORT_SYMBOL(bsp_i2c_adm116x_erase_reg);
EXPORT_SYMBOL(bsp_i2c_adm116x_write_reg);
