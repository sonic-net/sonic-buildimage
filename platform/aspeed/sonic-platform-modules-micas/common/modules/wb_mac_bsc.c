/*
 *
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <wb_bsp_kernel_debug.h>

#define mem_clear(data, size) memset((data), 0, (size))

#define MAC_TEMP_INVALID    (99999999)
#define MAC_ID_REG          (0x02000000)

#define MAC_REG_ADDR_WIDTH (4)
#define MAC_REG_DATA_WIDTH (4)
#define MAC_BSC_MAX_TEMP_NUM (16)
#define MAC_BSC_MAX_READ_REG_STEP (6)
#define MAC_BSC_MAX_SETUP_NUM (16)
#define MAC_BSC_AVS_SYSFS_MAX_NUM (16)
#define MAC_BSC_AVS_REG_MAX_NUM (2)

#define MAC_BSC_MAX_RETRY (3)
#define MAC_BSC_RETRY_SLEEP_TIME   (10000)   /* 10ms */

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);
typedef enum{
    MAC_TYPE_START,
    TD4_X9 = 0xb780,
    TD4_X9_8 = 0xb788,
    TH3 = 0xb980,
    TD3 = 0xb870,
    TD3_X2 = 0xb274,
    TD4 = 0xb880,
    TH4 = 0xb990,
    TH5 = 0x8900,
    TH6 = 0x8910,
    MAC_TYPE_END,
} mac_id;

typedef enum {
    TYPE_VTMON = 0,
    TYPE_AVS,
} reg_type;

typedef enum {
    MAC_TEMP_START,
    MAC_TEMP_INDEX1,
    MAC_TEMP_INDEX2,
    MAC_TEMP_INDEX3,
    MAC_TEMP_INDEX4,
    MAC_TEMP_INDEX5,
    MAC_TEMP_INDEX6,
    MAC_TEMP_INDEX7,
    MAC_TEMP_INDEX8,
    MAC_TEMP_INDEX9,
    MAC_TEMP_INDEX10,
    MAC_TEMP_INDEX11,
    MAC_TEMP_INDEX12,
    MAC_TEMP_INDEX13,
    MAC_TEMP_INDEX14,
    MAC_TEMP_INDEX15,
    MAC_TEMP_END,
} mac_hwmon_index;

typedef enum action_e {
    I2C_WRITE,
    I2C_READ
} action_t;

typedef struct i2c_op_s {
    action_t op;
    uint32_t reg_addr;
    uint32_t reg_val;
    int read_back;
} i2c_op_t;

typedef struct avs_info_s {
    uint32_t avs_reg_addr;
    uint32_t avs_reg_offset;
    uint32_t avs_reg_mask;
} avs_info_t;

typedef struct dev_params_s {
    int mac_id;
    i2c_op_t sbus_setup[MAC_BSC_MAX_SETUP_NUM];
    i2c_op_t vtmon_read[MAC_BSC_MAX_READ_REG_STEP];
    i2c_op_t avs_read[MAC_BSC_MAX_READ_REG_STEP];
    uint32_t vtmon_reg_addrs[MAC_BSC_MAX_TEMP_NUM];
    uint32_t avs_reg_addrs[MAC_BSC_MAX_TEMP_NUM];
    uint8_t vtmon_instances;
    uint32_t vtmon_data_width;
    uint8_t avs_instances;
    avs_info_t avs_info[MAC_BSC_AVS_SYSFS_MAX_NUM][MAC_BSC_AVS_REG_MAX_NUM];
    uint8_t avs_info_instances;
    int vtmon_scalar;
    int vtmon_offset;
    uint8_t sbus_setup_ops;
    int vtmon_read_ops;
    int sbus_addr_op;
    int sbus_error_op;
    uint8_t avs_read_ops;
    uint32_t sbus_error_mask;
    bool not_support_vtmon;
} dev_params_t;

static dev_params_t mac_temp_conf[] = {
    {
        .mac_id = TD3_X2,
        /* CMIC_TOP_SBUS_RING_MAP_0_7 = 0x52222100 */
        .sbus_setup = {{I2C_WRITE, 0x1010000c, 0x52222100, 0}},
        .vtmon_read = {
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRL = 0x00000000 */
            {I2C_WRITE, 0x10110400, 0x00000000, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGE[0] = 0x2c380200 */
            {I2C_WRITE, 0x1011040c, 0x2c380200, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGE[1] = TOP_PVTMON_RESULT_0 */
            {I2C_WRITE, 0x10110410, 0x02005300, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRL = 0x00000001 */
            {I2C_WRITE, 0x10110400, 0x00000001, 0},
            /* CMIC_COMMON_POOL_SCHAN_CH4_ERR */
            {I2C_READ, 0x10110408},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGE[1] */
            {I2C_READ, 0x10110410}
        },
        .vtmon_reg_addrs = {
            0x02005300, 0x02005400, 0x02005500, 0x02005600, 0x02005700, 0x02005800
        },
        .vtmon_instances = 6,
        .vtmon_data_width = 10,
        .vtmon_scalar = -5570,
        .vtmon_offset = 4578289,
        .sbus_setup_ops = 1,
        .vtmon_read_ops = 6,
        .sbus_addr_op = 2,
        .sbus_error_op = 4,
        .sbus_error_mask = 0x00000041,
    },
    {
        .mac_id = TD3,  /* TD3_X7*/
        /* CMIC_TOP_SBUS_RING_MAP_0_7 = 0x52222100 */
        .sbus_setup = {{I2C_WRITE, 0x0320000c, 0x52222100, 0}},
        .vtmon_read = {
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRL = 0x00000000 */
            {I2C_WRITE, 0x03210400, 0x00000000, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGE[0] = 0x2c380200 */
            {I2C_WRITE, 0x0321040c, 0x2c380200, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGE[1] = TOP_PVTMON_RESULT_0 */
            {I2C_WRITE, 0x03210410, 0x02004700, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRL = 0x00000001 */
            {I2C_WRITE, 0x03210400, 0x00000001, 0},
            /* CMIC_COMMON_POOL_SCHAN_CH4_ERR */
            {I2C_READ, 0x03210408},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGE[1] */
            {I2C_READ, 0x03210410}
        },
        .vtmon_reg_addrs = {
            0x02004700, 0x02004800, 0x02004900, 0x02004a00, 0x02004b00, 0x02004c00,
            0x02004d00, 0x02004e00, 0x02005200, 0x02005100, 0x02005000, 0x02004f00
        },
        .vtmon_instances = 12,
        .vtmon_data_width = 10,
        .vtmon_scalar = -5350,
        .vtmon_offset = 4341000,
        .sbus_setup_ops = 0,
        .vtmon_read_ops = 6,
        .sbus_addr_op = 2,
        .sbus_error_op = 4,
        .sbus_error_mask = 0x00000041,
    },
    {
        .mac_id = TH3,
        .vtmon_read = {
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRL = 0x00000000 */
            {I2C_WRITE, 0x03210400, 0x00000000, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGE[0] = 0x2c400200 */
            {I2C_WRITE, 0x0321040c, 0x2c400200, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGE[1] = TOP_PVTMON_RESULT_0 */
            {I2C_WRITE, 0x03210410, 0x02004a00, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRL = 0x00000001 */
            {I2C_WRITE, 0x03210400, 0x00000001, 0},
            /* CMIC_COMMON_POOL_SCHAN_CH4_ERR */
            {I2C_READ, 0x03210408},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGE[1] */
            {I2C_READ, 0x03210410}
        },
        .vtmon_reg_addrs = {
            0x02004a00, 0x02004b00, 0x02004c00, 0x02004d00, 0x02004e00, 0x02004f00,
            0x02005000, 0x02005100, 0x02005200, 0x02005300, 0x02005400, 0x02005500,
            0x02005600, 0x02005700, 0x02005800
        },
        .vtmon_instances = 15,
        .vtmon_data_width = 10,
        .vtmon_scalar = -5350,
        .vtmon_offset = 4341000,
        .sbus_setup_ops = 0,
        .vtmon_read_ops = 6,
        .sbus_addr_op = 2,
        .sbus_error_op = -1,
    },
    {
        .mac_id = TD4_X9,
        /* CMIC_TOP_SBUS_RING_MAP_8_15r = 0x00000000 */
        .sbus_setup = {{I2C_WRITE, 0x03200010, 0x00000000, 0}},
        .vtmon_read = {
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRLr = 0x00000000 */
            {I2C_WRITE, 0x03210400, 0x00000000, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[0] = 0x2c400200 */
            {I2C_WRITE, 0x0321040c, 0x2c400200, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[1] = TOP_VTMON_0_RESULT_1r */
            {I2C_WRITE, 0x03210410, 0x02005a00, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRLr = 0x00000001 */
            {I2C_WRITE, 0x03210400, 0x00000001, 0},
            /* CMIC_COMMON_POOL_SCHAN_CH4_ERRr */
            {I2C_READ, 0x03210408},
             /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[1] */
            {I2C_READ, 0x03210410}
        },
        .vtmon_reg_addrs = {
            0x02005a00, 0x02005c00, 0x02005e00, 0x02006000, 0x02006200, 0x02006400,
            0x02006600, 0x02006800, 0x02006a00
        },
        .vtmon_instances = 9,
        .vtmon_data_width = 11,
        .vtmon_scalar = -2454,
        .vtmon_offset = 3668120,
        .sbus_setup_ops = 0,
        .vtmon_read_ops = 6,
        .sbus_addr_op = 2,
        .sbus_error_op = 4,
        .sbus_error_mask = 0x00000041,
    },
    {
        .mac_id = TD4_X9_8,
        /* CMIC_TOP_SBUS_RING_MAP_8_15r = 0x00000000 */
        .sbus_setup = {{I2C_WRITE, 0x03200010, 0x00000000, 0}},
        .vtmon_read = {
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRLr = 0x00000000 */
            {I2C_WRITE, 0x03210400, 0x00000000, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[0] = 0x2c400200 */
            {I2C_WRITE, 0x0321040c, 0x2c400200, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[1] = TOP_VTMON_0_RESULT_1r */
            {I2C_WRITE, 0x03210410, 0x02005a00, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRLr = 0x00000001 */
            {I2C_WRITE, 0x03210400, 0x00000001, 0},
            /* CMIC_COMMON_POOL_SCHAN_CH4_ERRr */
            {I2C_READ, 0x03210408},
             /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[1] */
            {I2C_READ, 0x03210410}
        },
        .vtmon_reg_addrs = {
            0x02005a00, 0x02005c00, 0x02005e00, 0x02006000, 0x02006200, 0x02006400,
            0x02006600, 0x02006800, 0x02006a00
        },
        .vtmon_instances = 9,
        .vtmon_data_width = 11,
        .vtmon_scalar = -2454,
        .vtmon_offset = 3668120,
        .sbus_setup_ops = 0,
        .vtmon_read_ops = 6,
        .sbus_addr_op = 2,
        .sbus_error_op = 4,
        .sbus_error_mask = 0x00000041,
    },
    {
        .mac_id = TD4,  /* TD4-X11 */
        /* CMIC_TOP_SBUS_RING_MAP_8_15r = 0x00000000 */
        .sbus_setup = {{I2C_WRITE, 0x03200010, 0x00000000, 0}},
        .vtmon_read = {
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRLr = 0x00000000 */
            {I2C_WRITE, 0x03210400, 0x00000000, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[0] = 0x2c400200 */
            {I2C_WRITE, 0x0321040c, 0x2c400200, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[1] = TOP_VTMON_0_RESULT_1r */
            {I2C_WRITE, 0x03210410, 0x02004900, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRLr = 0x00000001 */
            {I2C_WRITE, 0x03210400, 0x00000001, 0},
            /* CMIC_COMMON_POOL_SCHAN_CH4_ERRr */
            {I2C_READ,  0x03210408},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[1] */
            {I2C_READ,  0x03210410}
        },
        .vtmon_reg_addrs = {
            0x02004900, 0x02004b00, 0x02004d00, 0x02004f00, 0x02005100, 0x02005300,
            0x02005500, 0x02005700, 0x02005900, 0x02005b00, 0x02005d00, 0x02005f00,
            0x02006100, 0x02006300, 0x02006500
        },
        .vtmon_instances = 15,
        .vtmon_data_width = 11,
        .vtmon_scalar = -2454,
        .vtmon_offset = 3668120,
        .sbus_setup_ops = 0,
        .vtmon_read_ops = 6,
        .sbus_addr_op = 2,
        .sbus_error_op = 4,
        .sbus_error_mask = 0x00000041,
    },
    {
        .mac_id = TH4,
        .vtmon_read = {
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRLr = 0x00000000 */
            {I2C_WRITE, 0x03210400, 0x00000000, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[0] = 0x2c400200 */
            {I2C_WRITE, 0x0321040c, 0x2c400200, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[1] = TOP_VTMON_0_RESULT_1r */
            {I2C_WRITE, 0x03210410, 0x0201d800, 1},
            /* CMIC_COMMON_POOL_SCHAN_CH4_CTRLr = 0x00000001 */
            {I2C_WRITE, 0x03210400, 0x00000001, 0},
            /* CMIC_COMMON_POOL_SCHAN_CH4_ERRr */
            {I2C_READ,  0x03210408},
            /* CMIC_COMMON_POOL_SCHAN_CH4_MESSAGEr[1] */
            {I2C_READ,  0x03210410}
        },
        .vtmon_reg_addrs = {
            0x0201d800, 0x0201e000, 0x0201e800, 0x0201f000, 0x0201f800, 0x02020000,
            0x02020800, 0x02021000, 0x02021800, 0x02022000, 0x02022800, 0x02023000,
            0x02023800, 0x02024000, 0x02024800,
        },
        .vtmon_instances = 15,
        .vtmon_data_width = 11,
        .vtmon_scalar = -2454,
        .vtmon_offset = 3668120,
        .sbus_setup_ops = 0,
        .vtmon_read_ops = 6,
        .sbus_addr_op = 2,
        .sbus_error_op = -1,
    },
    {
        .mac_id = TH5,
        .sbus_setup = {{I2C_WRITE, 0x02879800, 0x00000000},     /* CMICX_M0_IDM_IDM_RESET_CONTROLr = 0x0 */
                       {I2C_WRITE, 0x0287b800, 0x00000000},     /* CMICX_M1_IDM_IDM_RESET_CONTROLr = 0x0 */
                       {I2C_WRITE, 0x0287a800, 0x00000000},     /* CMICX_M2_IDM_IDM_RESET_CONTROLr = 0x0 */
                       {I2C_WRITE, 0x02872800, 0x00000000},     /* CMICX_S0_IDM_IDM_RESET_CONTROLr = 0x0 */
                       {I2C_WRITE, 0x02810798, 0x02078000},     /* CMIC_CMNPL_SBUSPIO_CH15_CMPL_RING_BASE_ADDR_LOWERr = HOST_MEM_BASE */
                       {I2C_WRITE, 0x0281079c, 0x00000000},     /* CMIC_CMNPL_SBUSPIO_CH15_CMPL_RING_BASE_ADDR_HIGHr = 0 */
                       {I2C_WRITE, 0x028107a0, 0x00000000},     /* CMIC_CMNPL_SBUSPIO_CH15_CMPL_RING_SIZEr = 0x0 */
                       {I2C_WRITE, 0x028107a4, 0x02079000},     /* CMIC_CMNPL_SBUSPIO_CH15_POINTER_HOST_ADDR_LOWERr = POINTER_MEM_BASE */
                       {I2C_WRITE, 0x028107a8, 0x00000000},     /* CMIC_CMNPL_SBUSPIO_CH15_POINTER_HOST_ADDR_UPPERr = 0 */
                       {I2C_WRITE, 0x0280f800, 0x2c200040},     /* CMIC_CMNPL_SBUSPIO_CH15_COMMAND_RING_MEMORYr[0] = 0x2c200040 */
                       {I2C_WRITE, 0x028107ac, 0x00000008}},    /* CMIC_CMNPL_SBUSPIO_CH15_CMD_PRODUCER_INDEXr = 8 0x2c200040 */
        .vtmon_read = {{I2C_WRITE, 0x02810780, 0x00000200},     /* CMIC_CMNPL_SBUSPIO_CH15_CTRLr = 0x200 */
                       {I2C_WRITE, 0x0280f804, 0x02008800},     /* CMIC_CMNPL_SBUSPIO_CH15_COMMAND_RING_MEMORYr[1] = TOP_PVTMON_0_RESULT_1r */
                       {I2C_WRITE, 0x02810780, 0x00000201},     /* CMIC_CMNPL_SBUSPIO_CH15_CTRLr = 0x201 */
                       {I2C_READ,  0x02810794},                               /* CMIC_CMNPL_SBUSPIO_CH15_STATUSr */
                       {I2C_READ,  0x02078000}},                              /* Read HOST_MEM_BASE */
        .vtmon_reg_addrs = {0x02008800, 0x02009000, 0x02009800, 0x0200a000,
                            0x0200a800, 0x0200b000, 0x0200b800, 0x0200c000,
                            0x0200c800, 0x0200d000, 0x0200d800, 0x0200e000,
                            0x0200e800, 0x0200f000, 0x0200f800, 0x02010000},           /* "TOP_PVTMON0_RESULT_1" @ 0x02008800, ..... TOP_PVTMON15_RESULT_1 @  0x02010000 */
        .vtmon_instances = 16,              /* TH5 has 16 VTMONs mainly for customer check */
        .vtmon_data_width = 11,             /* check in regfile, TH5 TMON_DATA: 11bits */
        .vtmon_scalar = -3177,    //Tj= 476.359 - 0.317704xData
        .vtmon_offset = 4763590,     //Tj= 476.359 - 0.317704xData
        .sbus_setup_ops = 11,
        .vtmon_read_ops = 5,
        .sbus_addr_op = 1,
        .sbus_error_op = 3,
        .sbus_error_mask = 0x00000002,
    },
    {
        .mac_id = TH6,
        .sbus_setup = {{I2C_WRITE, 0x02879800, 0x00000000},     /* CMICX_M0_IDM_IDM_RESET_CONTROLr = 0x0 */
                       {I2C_WRITE, 0x0287b800, 0x00000000},     /* CMICX_M1_IDM_IDM_RESET_CONTROLr = 0x0 */
                       {I2C_WRITE, 0x0287a800, 0x00000000},     /* CMICX_M2_IDM_IDM_RESET_CONTROLr = 0x0 */
                       {I2C_WRITE, 0x02872800, 0x00000000},},    /* CMICX_S0_IDM_IDM_RESET_CONTROLr = 0x0 */
        .avs_read = {{I2C_READ,  0x02840060}},                /* Read DMU_MASTER_PCU_OTP_CONFIG_6r */
        .avs_reg_addrs = {0x02840060, 0x02840064, 0x02840068, 0x0284006c, 0x02840070},   /* DMU_MASTER_PCU_OTP_CONFIG_6r address offset=0x02840060; DMU_MASTER_PCU_OTP_CONFIG_6r:0x02840064; DMU_MASTER_PCU_OTP_CONFIG_6r:0x02840068... */
        .avs_instances = 5,               /* Core die and phytile avs status */
        .avs_info = {
                        {
                            {0x02840060, 6, 0xff}
                        },
                        {
                            {0x02840060, 14, 0xff}
                        },
                        {
                            {0x02840060, 22, 0xff}
                        },
                        {
                            {0x02840064, 0, 0x3f},
                            {0x02840060, 30, 0x3}
                        },
                        {
                            {0x02840064, 6, 0xff}
                        },
                        {
                            {0x02840064, 14, 0xff}
                        },
                        {
                            {0x02840068, 0, 0xff}
                        },
                        {
                            {0x02840068, 8, 0xff}
                        },
                        {
                            {0x02840068, 16, 0xff}
                        },
                    },
        .avs_info_instances = 9,
        .sbus_setup_ops = 4,
        .avs_read_ops = 1,
        .sbus_addr_op = 0,
        .sbus_error_op = 3,
        .sbus_error_mask = 0x00000002,
        .not_support_vtmon = 1,
    },
};

struct mac_data {
    struct i2c_client *client;
    struct device *hwmon_dev;
    struct mutex update_lock;
    dev_params_t dev_param;
};

static int bsc_i2c_read(struct i2c_client *client, uint32_t reg_addr, uint32_t *reg_val)
{
    int msgs_num, ret, i;
    uint8_t addr_buf[MAC_REG_ADDR_WIDTH];
    uint8_t data_buf[MAC_REG_DATA_WIDTH];
    uint32_t val;
    struct i2c_msg msgs[2];

    for (i = 0; i < MAC_REG_ADDR_WIDTH; i++) {
        addr_buf[i] = (reg_addr >> ((MAC_REG_ADDR_WIDTH -i -1) * 8)) & 0xff;
    }

    mem_clear(msgs, sizeof(msgs));
    msgs[0].addr = client->addr;
    msgs[0].flags = 0;
    msgs[0].len = MAC_REG_ADDR_WIDTH;
    msgs[0].buf = addr_buf;

    msgs[1].addr = client->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = MAC_REG_DATA_WIDTH;
    msgs[1].buf = data_buf;

    msgs_num = 2;
    ret = i2c_transfer(client->adapter, msgs, msgs_num);
    if (ret != msgs_num) {
        DEBUG_ERROR("i2c_transfer read failed, reg_addr: 0x%x, ret: %d\n", reg_addr, ret);
        return -EIO;
    }
    val = 0;
    for (i = 0; i < MAC_REG_DATA_WIDTH; i++) {
        val |= data_buf[i] << ((MAC_REG_DATA_WIDTH - i -1) * 8);
    }
    DEBUG_VERBOSE("bsc_i2c_read success, reg_addr: 0x%x, reg_val: 0x%x\n", reg_addr, val);
    *reg_val = val;
    return 0;
}

static int bsc_i2c_write(struct i2c_client *client, uint32_t reg_addr, uint32_t reg_val)
{
    int ret, i;
    uint8_t write_buf[MAC_REG_ADDR_WIDTH + MAC_REG_DATA_WIDTH];
    struct i2c_msg msgs[1];

    /* fill reg_addr first*/
    for (i = 0; i < MAC_REG_ADDR_WIDTH; i++) {
        write_buf[i] = (reg_addr >> ((MAC_REG_ADDR_WIDTH -i -1) * 8)) & 0xff;
    }
    for (i = 0; i < MAC_REG_DATA_WIDTH; i++) {
        write_buf[i + MAC_REG_ADDR_WIDTH] = (reg_val >> ((MAC_REG_DATA_WIDTH -i -1) * 8)) & 0xff;
    }

    mem_clear(msgs, sizeof(msgs));
    msgs[0].len = MAC_REG_ADDR_WIDTH + MAC_REG_DATA_WIDTH;
    msgs[0].buf = write_buf;
    msgs[0].addr = client->addr;
    msgs[0].flags = I2C_M_IGNORE_NAK;

    ret = i2c_transfer(client->adapter, msgs, 1);
    if (ret < 0) {
        DEBUG_VERBOSE("i2c_transfer write failed, reg_addr: 0x%x, reg_val: 0x%x, ret: %d\n",
            reg_addr, reg_val, ret);
        return ret;
    }
    DEBUG_VERBOSE("i2c_transfer write reg_addr: 0x%x, reg_val: 0x%x success\n",
        reg_addr, reg_val);
    return 0;
}

static int handle_operation_write(struct i2c_client *client, i2c_op_t *operation)
{
    int ret;
    uint32_t rd_back_val;

    ret = bsc_i2c_write(client, operation->reg_addr, operation->reg_val);
    DEBUG_VERBOSE("bsc_i2c_write reg_addr: 0x%x, set val: 0x%x, ret: %d\n",
        operation->reg_addr, operation->reg_val, ret);
    if (operation->read_back) {
        ret = bsc_i2c_read(client, operation->reg_addr, &rd_back_val);
        if (rd_back_val != operation->reg_val) {
            DEBUG_ERROR("bsc_i2c_write failed, reg_addr: 0x%x, set val: 0x%x, read back valu: 0x%x\n",
                operation->reg_addr, operation->reg_val, rd_back_val);
            return -1;
        }
         DEBUG_VERBOSE("bsc_i2c_write success, reg_addr: 0x%x, set val: 0x%x, read_back val: 0x%x\n",
             operation->reg_addr, operation->reg_val, rd_back_val);
    }
    return 0;
}

static int handle_operation(struct i2c_client *client, i2c_op_t *operation)
{
    int ret, i;

    if (operation->op == I2C_WRITE) {
        for (i = 0; i < MAC_BSC_MAX_RETRY; i++) {
            ret = handle_operation_write(client, operation);
            if (ret == 0) {
                DEBUG_VERBOSE("handle_operation_write success, retry: %d\n", i);
                return 0;
            }
            if ((i + 1) < MAC_BSC_MAX_RETRY) {
                usleep_range(MAC_BSC_RETRY_SLEEP_TIME, MAC_BSC_RETRY_SLEEP_TIME + 1);
            }
        }
        DEBUG_VERBOSE("handle_operation_write retry: %d failed, ret: %d, ignore it\n", i, ret);
        return 0;
    }

    if (operation->op == I2C_READ) {
        ret = bsc_i2c_read(client, operation->reg_addr, &operation->reg_val);
        DEBUG_VERBOSE("bsc_i2c_read reg_addr: 0x%x, get val: 0x%x, ret: %d\n",
            operation->reg_addr, operation->reg_val, ret);
        return ret;
    }

    DEBUG_ERROR("Unsupport operation type: %d\n", operation->op);
    return -EINVAL;
}

static int get_mac_reg(struct i2c_client *client, uint32_t reg_addr, uint32_t *reg_value, uint8_t type)
{
    int i, ret;
    i2c_op_t *op, *tmp_op;
    struct mac_data *data;
    dev_params_t *dev_params;
    uint32_t val_tmp;
    int ops;

    data = i2c_get_clientdata(client);
    dev_params = &data->dev_param;

    if (type == TYPE_AVS) {
        ops = dev_params->avs_read_ops;
        tmp_op = dev_params->avs_read;
    } else {
        ops = dev_params->vtmon_read_ops;
        tmp_op = dev_params->vtmon_read;
    }

    val_tmp = 0;
    for (i = 0; i < ops; i++) {
        op = tmp_op + i;
        if (i == dev_params->sbus_addr_op) {
            if (type == TYPE_AVS) {
                op->reg_addr = reg_addr;
            } else {
                op->reg_val = reg_addr;
            }
        }
        DEBUG_VERBOSE("Start to handle %s operation, step: %d, reg_addr: 0x%x, reg_value: 0x%x, read back flag: %d\n",
            op->op == I2C_READ ? "I2C_READ" : "I2C_WRITE", i, op->reg_addr, op->reg_val, op->read_back);
        ret = handle_operation(client, op);
        if (ret < 0) {
            DEBUG_ERROR("handle operation %d failed, ret: %d\n", i, ret);
            return ret;
        }
        if (op->op == I2C_READ) {
            val_tmp = op->reg_val;
        }

        if (i == dev_params->sbus_error_op) {
            if (val_tmp & dev_params->sbus_error_mask) {
                DEBUG_ERROR("SBUS error seen, status value: 0x%x\n", op->reg_val);
                return -EIO;
            }
            DEBUG_VERBOSE("Error status check ok, status: 0x%x, error_mask: 0x%x \n",
                val_tmp, dev_params->sbus_error_mask);
        }
    }

    if (val_tmp == reg_addr) {
        DEBUG_ERROR("get mac register error, register value: 0x%x equal to reg_addr: 0x%x\n",
            val_tmp, reg_addr);
        return -EIO;
    }

    *reg_value = val_tmp;
    DEBUG_VERBOSE("get_mac_reg success, reg_addr: 0x%x, reg_value: 0x%x", reg_addr, *reg_value);
    return 0;
}

static int read_vtmon(struct i2c_client *client, uint8_t vtmon, int *temp)
{
    struct mac_data *data;
    dev_params_t *dev_params;
    uint32_t reg_addr, reg_val;
    uint32_t vtmon_val;
    int ret;

    data = i2c_get_clientdata(client);
    dev_params = &data->dev_param;

    if (vtmon >= dev_params->vtmon_instances) {
        DEBUG_ERROR("VTMON index [%d] greater or equal to VTMON instance number: %d\n",
            vtmon, dev_params->vtmon_instances);
        return -1;
    }

    reg_addr = dev_params->vtmon_reg_addrs[vtmon];
    ret = get_mac_reg(client, reg_addr, &reg_val, TYPE_VTMON);
    if (ret < 0) {
        DEBUG_ERROR("Read VTMON[%d] failed, reg_addr: 0x%x, ret: %d\n",
            vtmon, reg_addr, ret);
        return ret;
    }

    vtmon_val = reg_val & ((1 << dev_params->vtmon_data_width) - 1);
    *temp = ((dev_params->vtmon_scalar * vtmon_val) + dev_params->vtmon_offset) / 10;

    if ((*temp / 1000 < -40) || (*temp / 1000 > 120)) {
        DEBUG_ERROR("MAC temp invalid, vtmon: %d, temp: %d\n", vtmon, *temp);
        return -EINVAL;
    }
    DEBUG_VERBOSE("Read mac temp success, index: %d, value: %d\n", vtmon + 1, *temp);
    return 0;
}

static int read_avs(struct i2c_client *client, uint8_t avs_index, uint32_t *avs_val)
{
    struct mac_data *data;
    dev_params_t *dev_params;
    avs_info_t *avs_info;
    int ret, i;
    uint32_t tmp_avs_val;
    uint32_t avs_reg_addr;
    uint32_t avs_reg_offset;
    uint32_t avs_reg_mask;
    bool avs_read_success = false;
    *avs_val = 0;

    data = i2c_get_clientdata(client);
    dev_params = &data->dev_param;

    if (avs_index >= dev_params->avs_info_instances) {
        DEBUG_ERROR("avs index [%d] greater or equal to avs instance number: %d\n",
            avs_index, dev_params->avs_info_instances);
        return -EINVAL;
    }
    
    for (i = 0; i < MAC_BSC_AVS_REG_MAX_NUM; i++) {
        avs_info = &dev_params->avs_info[avs_index][i];
        avs_reg_addr = avs_info->avs_reg_addr;
        avs_reg_offset = avs_info->avs_reg_offset;
        avs_reg_mask = avs_info->avs_reg_mask;

        if ((avs_reg_addr == 0) && (avs_reg_offset == 0) && (avs_reg_mask == 0)) {
            break;
        }
        ret = get_mac_reg(client, avs_reg_addr, &tmp_avs_val, TYPE_AVS);
        if (ret < 0) {
            DEBUG_ERROR("Read avs[%d] index[%d] failed, avs_reg_addr: 0x%x, ret: %d\n",
                avs_index, i, avs_reg_addr, ret);
            return ret;
        }
        DEBUG_VERBOSE("Read avs val success, avs%d, index: %d, value: %x\n", avs_index, i, tmp_avs_val);
        tmp_avs_val = (tmp_avs_val >> avs_reg_offset) & avs_reg_mask;
        *avs_val = (*avs_val << fls(avs_reg_mask)) | tmp_avs_val;
        avs_read_success = true;
    }

    if (!avs_read_success) {
        DEBUG_ERROR("Read avs val fail, avs config error\n");
        return -EINVAL;
    }

    DEBUG_VERBOSE("Read avs val success, index: %d, value: %x\n", avs_index + 1, *avs_val);
    return 0;
}

static ssize_t show_mac_temp(struct device *dev, struct device_attribute *da, char *buf)
{
    struct mac_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    u32 temp_index = to_sensor_dev_attr(da)->index;
    int ret, temp;

    mutex_lock(&data->update_lock);
    ret = read_vtmon(client, temp_index - 1, &temp);
    if (ret < 0) {
        temp = -MAC_TEMP_INVALID;
        DEBUG_ERROR("get_mactemp index: %d failed, ret = %d\n", temp_index, ret);
    }
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

static ssize_t show_mac_avs(struct device *dev, struct device_attribute *da, char *buf)
{
    struct mac_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    u32 avs_index = to_sensor_dev_attr(da)->index;
    int ret;
    u32 avs_val;

    mutex_lock(&data->update_lock);
    ret = read_avs(client, avs_index - 1, &avs_val);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("get_mac_avs index: %d failed, ret = %d\n", avs_index, ret);
        return ret;
    }
    return snprintf(buf, PAGE_SIZE, "0x%02x\n", avs_val);
}

static ssize_t show_mac_max_temp(struct device *dev, struct device_attribute *da, char *buf)
{
    struct mac_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    dev_params_t *dev_params;
    int i, ret;
    int tmp, temp;

    mutex_lock(&data->update_lock);

    dev_params = &data->dev_param;
    temp = -MAC_TEMP_INVALID;
    for (i = 0; i < dev_params->vtmon_instances ; i++) {
        ret = read_vtmon(client, i, &tmp);
        if (ret < 0) {
            DEBUG_ERROR("Get mactemp failed, temp index: %d, ret = %d\n",
                i, ret);
            tmp = -MAC_TEMP_INVALID;
        }

        temp = (temp > tmp) ? temp : tmp;
    }

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX1);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX2);
static SENSOR_DEVICE_ATTR(temp3_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX3);
static SENSOR_DEVICE_ATTR(temp4_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX4);
static SENSOR_DEVICE_ATTR(temp5_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX5);
static SENSOR_DEVICE_ATTR(temp6_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX6);
static SENSOR_DEVICE_ATTR(temp7_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX7);
static SENSOR_DEVICE_ATTR(temp8_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX8);
static SENSOR_DEVICE_ATTR(temp9_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX9);
static SENSOR_DEVICE_ATTR(temp10_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX10);
static SENSOR_DEVICE_ATTR(temp11_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX11);
static SENSOR_DEVICE_ATTR(temp12_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX12);
static SENSOR_DEVICE_ATTR(temp13_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX13);
static SENSOR_DEVICE_ATTR(temp14_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX14);
static SENSOR_DEVICE_ATTR(temp15_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX15);
static SENSOR_DEVICE_ATTR(temp99_input, S_IRUGO, show_mac_max_temp, NULL, 0);

static SENSOR_DEVICE_ATTR(avs1, S_IRUGO, show_mac_avs, NULL, 1);
static SENSOR_DEVICE_ATTR(avs2, S_IRUGO, show_mac_avs, NULL, 2);
static SENSOR_DEVICE_ATTR(avs3, S_IRUGO, show_mac_avs, NULL, 3);
static SENSOR_DEVICE_ATTR(avs4, S_IRUGO, show_mac_avs, NULL, 4);
static SENSOR_DEVICE_ATTR(avs5, S_IRUGO, show_mac_avs, NULL, 5);
static SENSOR_DEVICE_ATTR(avs6, S_IRUGO, show_mac_avs, NULL, 6);
static SENSOR_DEVICE_ATTR(avs7, S_IRUGO, show_mac_avs, NULL, 7);
static SENSOR_DEVICE_ATTR(avs8, S_IRUGO, show_mac_avs, NULL, 8);
static SENSOR_DEVICE_ATTR(avs9, S_IRUGO, show_mac_avs, NULL, 9);

static struct attribute *mac_hwmon_attrs[] = {
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp3_input.dev_attr.attr,
    &sensor_dev_attr_temp4_input.dev_attr.attr,
    &sensor_dev_attr_temp5_input.dev_attr.attr,
    &sensor_dev_attr_temp6_input.dev_attr.attr,
    &sensor_dev_attr_temp7_input.dev_attr.attr,
    &sensor_dev_attr_temp8_input.dev_attr.attr,
    &sensor_dev_attr_temp9_input.dev_attr.attr,
    &sensor_dev_attr_temp10_input.dev_attr.attr,
    &sensor_dev_attr_temp11_input.dev_attr.attr,
    &sensor_dev_attr_temp12_input.dev_attr.attr,
    &sensor_dev_attr_temp13_input.dev_attr.attr,
    &sensor_dev_attr_temp14_input.dev_attr.attr,
    &sensor_dev_attr_temp15_input.dev_attr.attr,
    &sensor_dev_attr_temp99_input.dev_attr.attr,
    &sensor_dev_attr_avs1.dev_attr.attr,
    &sensor_dev_attr_avs2.dev_attr.attr,
    &sensor_dev_attr_avs3.dev_attr.attr,
    &sensor_dev_attr_avs4.dev_attr.attr,
    &sensor_dev_attr_avs5.dev_attr.attr,
    &sensor_dev_attr_avs6.dev_attr.attr,
    &sensor_dev_attr_avs7.dev_attr.attr,
    &sensor_dev_attr_avs8.dev_attr.attr,
    &sensor_dev_attr_avs9.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(mac_hwmon);

static void mac_bsc_setup(struct i2c_client *client)
{
    int i, ret;
    struct mac_data *data;
    dev_params_t *dev_params;
    uint32_t reg_value;

    data = i2c_get_clientdata(client);
    dev_params = &data->dev_param;

    for (i = 0; i < dev_params->sbus_setup_ops; i++) {
        ret = bsc_i2c_read(client, dev_params->sbus_setup[i].reg_addr, &reg_value);
        if ((ret < 0) || (reg_value != dev_params->sbus_setup[i].reg_val)) {
            DEBUG_VERBOSE("bsc setup op%d, ret: %d, reg_addr: 0x%x, read value: 0x%x not equal to set value: 0x%x\n",
                i, ret, dev_params->sbus_setup[i].reg_addr, reg_value, dev_params->sbus_setup[i].reg_val);
            bsc_i2c_write(client, dev_params->sbus_setup[i].reg_addr, dev_params->sbus_setup[i].reg_val);
        } else {
            DEBUG_VERBOSE("bsc setup op%d, reg_addr: 0x%x, read value: 0x%x equal to set value: 0x%x\n",
                i, dev_params->sbus_setup[i].reg_addr, reg_value, dev_params->sbus_setup[i].reg_val);
        }
    }
    return;
}

static int mac_bsc_init(struct i2c_client *client)
{
    int ret, mac_id;
    uint32_t reg_value;

    ret = get_mac_reg(client, MAC_ID_REG, &reg_value, TYPE_VTMON);
    if (ret < 0) {
        DEBUG_ERROR("Get MAC ID failed, reg_addr: 0x%x, ret = %d\n",
            MAC_ID_REG, ret);
        return ret;
    }

    DEBUG_VERBOSE("Get MAC ID success, reg_addr: 0x%x, value: 0x%x \n",
        MAC_ID_REG, reg_value);
    mac_id = reg_value & 0xffff;
    return mac_id;
}

static int find_mac_config(int type, int *index)
{
    int i, size;

    size = ARRAY_SIZE(mac_temp_conf);
    for (i = 0; i < size; i++) {
        if (mac_temp_conf[i].mac_id == type) {
            *index = i;
            return 0;
        }
    }
    return -1;
}

static int mac_bsc_config_check(dev_params_t *dev_params)
{
    i2c_op_t *last_op;
    i2c_op_t *err_op;
    i2c_op_t *addr_op;

    if (dev_params->not_support_vtmon) {
        return 0;
    }

    /* vtmon_instances should not more than the MAC_BSC_MAX_TEMP_NUM */
    if ((dev_params->vtmon_instances > MAC_BSC_MAX_TEMP_NUM) ||
        (dev_params->vtmon_instances <= 0)) {
        DEBUG_ERROR("VTMON instance number %d more than the max number: %d\n",
            dev_params->vtmon_instances, MAC_BSC_MAX_TEMP_NUM);
        return -1;
    }

    /* vtmon read operation steps should not more than the MAC_BSC_MAX_READ_REG_STEP */
    if ((dev_params->vtmon_read_ops > MAC_BSC_MAX_READ_REG_STEP) ||
        (dev_params->vtmon_read_ops <=0)) {
        DEBUG_ERROR("VTMON read ops number %d more than the max step: %d\n",
            dev_params->vtmon_read_ops, MAC_BSC_MAX_READ_REG_STEP);
        return -1;
    }

    /* the last operation must be I2C_READ to get temperature register value */
    last_op = &dev_params->vtmon_read[dev_params->vtmon_read_ops - 1];
    if (last_op->op != I2C_READ) {
        DEBUG_ERROR("VTMON read ops config error, last operation not I2C_READ, last step: %d, op_code: %d\n",
            dev_params->vtmon_read_ops - 1, last_op->op);
        return -1;
    }

    /* the address operation steps should not more than the vtmon_read_ops and not the last step */
    if ((dev_params->sbus_addr_op >= (dev_params->vtmon_read_ops - 1)) ||
        (dev_params->sbus_addr_op < 0)) {
        DEBUG_ERROR("VTMON addr op step invalid, index %d, read ops: %d\n",
            dev_params->sbus_addr_op, dev_params->vtmon_read_ops);
        return -1;
    }

    /* the address operation must be I2C_WRITE to set temperature register address */
    addr_op = &dev_params->vtmon_read[dev_params->sbus_addr_op];
    if (addr_op->op != I2C_WRITE) {
        DEBUG_ERROR("VTMON addr op config error, addr operation not I2C_WRITE, addr op step: %d, op_code: %d\n",
            dev_params->sbus_addr_op, addr_op->op);
        return -1;
    }

    /* the error status operation steps should not more than the vtmon_read_ops and not the last step */
    if (dev_params->sbus_error_op >= (dev_params->vtmon_read_ops - 1)) {
        DEBUG_ERROR("VTMON error op step invalid, index %d, read ops: %d\n",
            dev_params->sbus_error_op, dev_params->vtmon_read_ops);
        return -1;
    }

    /* if error status operation exist, it must be I2C_READ to get error status register */
    if (dev_params->sbus_error_op >=0) {
        err_op = &dev_params->vtmon_read[dev_params->sbus_error_op];
        if (err_op->op != I2C_READ) {
            DEBUG_ERROR("VTMON error op config error, error operation not I2C_READ, error op step: %d, op_code: %d\n",
                dev_params->sbus_error_op, err_op->op);
            return -1;
        }
    }
    DEBUG_VERBOSE("dev_params check ok, instance number: %d, read_ops: %d, addr_op: %d, error_op: %d\n",
        dev_params->vtmon_instances, dev_params->vtmon_read_ops,
        dev_params->sbus_addr_op, dev_params->sbus_error_op);
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int mac_probe(struct i2c_client *client, const struct i2c_device_id *id)
#else
static int mac_probe(struct i2c_client *client)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    const struct i2c_device_id *id = i2c_client_get_device_id(client);
#endif
    struct mac_data *data;
    int ret, mac_id, index;

    DEBUG_VERBOSE("=========mac_probe(%d-%04x)===========\n",
        client->adapter->nr, client->addr);

    if (!client->adapter->algo->master_xfer) {
        dev_err(&client->adapter->dev, "I2C level transfers not supported\n");
        return -EOPNOTSUPP;
    }

    data = devm_kzalloc(&client->dev, sizeof(struct mac_data), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "Failed to devm_kzalloc.\n");
        return -ENOMEM;
    }

    data->client = client;
    i2c_set_clientdata(client, data);

    mac_id = id->driver_data;
    ret = find_mac_config(mac_id, &index);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to find mac config, mac id from driver_data: 0x%x\n", mac_id);
        return -EINVAL;
    }
    data->dev_param = mac_temp_conf[index];
    ret = mac_bsc_config_check(&data->dev_param);
    if (ret < 0) {
        dev_err(&client->dev, "Invalid config parameter, mac id: 0x%x, config index: %d\n",
            mac_id, index);
        return -EINVAL;
    }

    mac_bsc_setup(client);

    if (mac_id == TD4) {
        ret = mac_bsc_init(client);
        if (ret < 0) {
            dev_err(&client->dev, "Failed to get mac id, ret: %d\n", ret);
            return -EIO;
        }
        mac_id = ret;
        ret = find_mac_config(mac_id, &index);
        if (ret < 0) {
            dev_err(&client->dev, "Failed to find mac config, mac id from chip: 0x%x\n", mac_id);
            return -EINVAL;
        }
        data->dev_param = mac_temp_conf[index];
        ret = mac_bsc_config_check(&data->dev_param);
        if (ret < 0) {
            dev_err(&client->dev, "Invalid config parameter, mac id: 0x%x, config index: %d\n",
                mac_id, index);
            return -EINVAL;
        }
    }

    DEBUG_VERBOSE("mac_id: 0x%x, config index: %d\n", mac_id, index);

    mutex_init(&data->update_lock);
    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, data, mac_hwmon_groups);
    if (IS_ERR(data->hwmon_dev)) {
        dev_err(&client->dev, "Failed to register mac bsc hwmon\n");
        return PTR_ERR(data->hwmon_dev);
    }

    dev_info(&client->dev, "Register mac bsc %x with %d vtmon instance number success\n",
        mac_id, data->dev_param.vtmon_instances);
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int mac_remove(struct i2c_client *client)
#else
static void mac_remove(struct i2c_client *client)
#endif
{
    struct mac_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    return 0;
#endif
}

static const struct i2c_device_id mac_id_table[] = {
    { "wb_mac_bsc_td3", TD3 },
    { "wb_mac_bsc_td3_x2", TD3_X2 },
    { "wb_mac_bsc_td4", TD4 },
    { "wb_mac_bsc_th3", TH3 },
    { "wb_mac_bsc_th4", TH4 },
    { "wb_mac_bsc_th5", TH5 },
    { "wb_mac_bsc_th6", TH6 },
    {}
};
MODULE_DEVICE_TABLE(i2c, mac_id_table);

static struct i2c_driver wb_mac_bsc_driver = {
    .driver = {
        .name   = "wb_mac_bsc",
    },
    .probe      = mac_probe,
    .remove     = mac_remove,
    .id_table   = mac_id_table,
};

module_i2c_driver(wb_mac_bsc_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("mac bsc driver");
MODULE_LICENSE("GPL");
