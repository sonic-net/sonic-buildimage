/* register offset define */
#define PSU_STAT_REG            0xa0
#define BMC_PSU_STAT_REG        0xe8

#define PSU_1_VIN_REG           0xb0
#define PSU_1_IIN_REG           0xb1
#define PSU_1_VOUT_REG          0xb2
#define PSU_1_IOUT_REG          0xb3
#define PSU_1_TEMP_1_REG        0xb4
#define PSU_1_FAN_SPEED_REG     0xb5
#define PSU_1_POUT_REG          0xb6
#define PSU_1_PIN_REG           0xb7
#define PSU_1_MFR_MODEL_REG     0xfa
#define PSU_1_MFR_IOUT_MAX_REG  0xb9
#define PSU_1_VMODE_REG         0xd8

#define PSU_2_VIN_REG           0xba
#define PSU_2_IIN_REG           0xbb
#define PSU_2_VOUT_REG          0xbc
#define PSU_2_IOUT_REG          0xbd
#define PSU_2_TEMP_1_REG        0xbe
#define PSU_2_FAN_SPEED_REG     0xbf
#define PSU_2_POUT_REG          0xc0
#define PSU_2_PIN_REG           0xc1
#define PSU_2_MFR_MODEL_REG     0x9a
#define PSU_2_MFR_IOUT_MAX_REG  0xc3
#define PSU_2_VMODE_REG         0xd9

#define PSU_3_VIN_REG           0xc4
#define PSU_3_IIN_REG           0xc5
#define PSU_3_VOUT_REG          0xc6
#define PSU_3_IOUT_REG          0xc7
#define PSU_3_TEMP_1_REG        0xc8
#define PSU_3_FAN_SPEED_REG     0xc9
#define PSU_3_POUT_REG          0xca
#define PSU_3_PIN_REG           0xcb
#define PSU_3_MFR_MODEL_REG     0x9d
#define PSU_3_MFR_IOUT_MAX_REG  0xcd
#define PSU_3_VMODE_REG         0xda

#define PSU_4_VIN_REG           0xce
#define PSU_4_IIN_REG           0xcf
#define PSU_4_VOUT_REG          0xd0
#define PSU_4_IOUT_REG          0xd1
#define PSU_4_TEMP_1_REG        0xd2
#define PSU_4_FAN_SPEED_REG     0xd3
#define PSU_4_POUT_REG          0xd4
#define PSU_4_PIN_REG           0xd5
#define PSU_4_MFR_MODEL_REG     0xd6
#define PSU_4_MFR_IOUT_MAX_REG  0xd7
#define PSU_4_VMODE_REG         0xdb

#define DC_6E_P0_VOUT_REG       0x18
#define DC_70_P0_VOUT_REG       0x1b
#define DC_70_P1_VOUT_REG       0xf1

#define DC_6E_P0_IOUT_REG       0x19
#define DC_70_P0_IOUT_REG       0x1c
#define DC_70_P1_IOUT_REG       0xf2

#define DC_6E_P0_POUT_REG       0x1a
#define DC_70_P0_POUT_REG       0x1d
#define DC_70_P1_POUT_REG       0xf3

u8 dc_11_vout_table [9][2] = 
{
/*  Page0  Page1*/
    {0x00, 0x00},
    {0x30, 0x33},
    {0x40, 0x43},
    {0x50, 0x53},
    {0x60, 0x63},
    {0x70, 0x73},
    {0x80, 0x83},
    {0x90, 0x93},
    {0xa0, 0xa3},
};

u8 dc_12_vout_table [9][2] = 
{
/*  Page0  Page1*/
    {0x00, 0x00},
    {0x36, 0x3f},
    {0x46, 0x5f},
    {0x56, 0x7f},
    {0x66, 0x9f},
    {0x76, 0xdd},
    {0x86, 0x0d},
    {0x96, 0xf8},
    {0xa6, 0xfc},
};

u8 dc_13_vout_table [9][2] = 
{
/*  Page0  Page1*/
    {0x00, 0x00},
    {0x38, 0x3b},
    {0x48, 0x4b},
    {0x58, 0x5b},
    {0x68, 0x6b},
    {0x78, 0x7b},
    {0x88, 0x8b},
    {0x98, 0x9b},
    {0xa8, 0xab},
};

u8 dc_11_iout_table [9][2] = 
{
/*  Page0  Page1*/
    {0x00, 0x00},
    {0x31, 0x34},
    {0x41, 0x44},
    {0x51, 0x54},
    {0x61, 0x64},
    {0x71, 0x74},
    {0x81, 0x84},
    {0x91, 0x94},
    {0xa1, 0xa4},
};

u8 dc_12_iout_table [9][2] = 
{
/*  Page0  Page1*/
    {0x00, 0x00},
    {0x37, 0x4e},
    {0x47, 0x6e},
    {0x57, 0x8e},
    {0x67, 0xae},
    {0x77, 0xde},
    {0x87, 0x0e},
    {0x97, 0xf9},
    {0xa7, 0xfd},
};

u8 dc_13_iout_table [9][2] = 
{
/*  Page0  Page1*/
    {0x00, 0x00},
    {0x39, 0x3c},
    {0x49, 0x4c},
    {0x59, 0x5c},
    {0x69, 0x6c},
    {0x79, 0x7c},
    {0x89, 0x8c},
    {0x99, 0x9c},
    {0xa9, 0xac},
};
/* end of register offset define */