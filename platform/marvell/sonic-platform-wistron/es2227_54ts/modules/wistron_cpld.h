
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _WISTRON_SWITCH_CPLD_H
#define _WISTRON_SWITCH_CPLD_H

//#define CPLD_REVISION_REG    	    0x0
#define CPLD_HW_REVISION_REG        0x1
#define CPLD_REVISION_REG           0x2
#define CPLD_BUILD_YEAR_REG         0x3
#define CPLD_BUILD_DATE_REG         0x4
#define CPLD_RW_TEST_REG            0x5
#define CPLD_PSU_CHECK_REG          0x6

#define CPLD_POWER_CTL1_REG         0x7
#define CPLD_POWER_CTL2_REG         0x8

#define CPLD_POWER_GOOD_REG         0x9
#define CPLD_POWER_GOOD2_REG        0xA

#define CPLD_FAN_CTL1_REG           0xB
#define CPLD_FAN_CTL2_REG           0xC


#define CPLD_1G_LED_CTL1_REG        0xD
#define CPLD_1G_LED_CTL2_REG        0xE
#define CPLD_1G_LED_CTL3_REG        0xF
#define CPLD_1G_LED_CTL4_REG        0x10
#define CPLD_1G_LED_CTL5_REG        0x11
#define CPLD_1G_LED_CTL6_REG        0x12
#define CPLD_1G_LED_CTL7_REG        0x13
#define CPLD_1G_LED_CTL8_REG        0x14
#define CPLD_1G_LED_CTL9_REG        0x15
#define CPLD_1G_LED_CTL10_REG       0x16
#define CPLD_1G_LED_CTL11_REG       0x17
#define CPLD_1G_LED_CTL12_REG       0x18
#define CPLD_1G_LED_CTL13_REG       0x19
#define CPLD_1G_LED_CTL14_REG       0x1A
#define CPLD_1G_LED_CTL15_REG       0x1B
#define CPLD_1G_LED_CTL16_REG       0x1C

#define CPLD_2_5G_LED_CTL1_REG      0x1D
#define CPLD_2_5G_LED_CTL2_REG      0x1E
#define CPLD_2_5G_LED_CTL3_REG      0x1F
#define CPLD_2_5G_LED_CTL4_REG      0x20
#define CPLD_2_5G_LED_CTL5_REG      0x21
#define CPLD_2_5G_LED_CTL6_REG      0x22
#define CPLD_2_5G_LED_CTL7_REG      0x23
#define CPLD_2_5G_LED_CTL8_REG      0x24

#define CPLD_POE_PRT_LED_CTL1_REG   0x25
#define CPLD_POE_PRT_LED_CTL2_REG   0x26
#define CPLD_POE_PRT_LED_CTL3_REG   0x27
#define CPLD_POE_PRT_LED_CTL4_REG   0x28
#define CPLD_POE_PRT_LED_CTL5_REG   0x29
#define CPLD_POE_PRT_LED_CTL6_REG   0x2A
#define CPLD_POE_PRT_LED_CTL7_REG   0x2B
#define CPLD_POE_PRT_LED_CTL8_REG   0x2C
#define CPLD_POE_PRT_LED_CTL9_REG   0x2D
#define CPLD_POE_PRT_LED_CTL10_REG  0x2E
#define CPLD_POE_PRT_LED_CTL11_REG  0x2F
#define CPLD_POE_PRT_LED_CTL12_REG  0x30

#define CPLD_SFP_LED_CTL1_REG       0x31
#define CPLD_SFP_LED_CTL2_REG       0x32
#define CPLD_SFP_LED_CTL3_REG       0x33


#define CPLD_FAN_LED_CTL_REG        0x34
#define CPLD_SYS_LED_CTL_REG        0x35

#define CPLD_RESET_CTL1_REG         0x36
#define CPLD_RESET_CTL2_REG         0x37

#define CPLD_INT_CTL1_REG           0x38
#define CPLD_INT_CTL2_REG           0x39
#define CPLD_INT_CTL3_REG           0x3A
#define CPLD_INT_CTL4_REG           0x3B
#define CPLD_MSCI_CTL_REG           0x3C
#define CPLD_PORT_LED_MANUAL_REG    0x3D
#define CPLD_RESET_CTL3_REG         0x3E
#define CPLD_MSCI_CTL2_REG          0x3F
#define CPLD_WATCHDOG_REG           0x40
#define CPLD_SYS_POWER_DOWN_REG     0x41

static char* power_ctrl_str1[] = {
    "P1V2_OOB_EN", // 0
    "P1V5_EN",
    "P1V8_CPU_EN",
    "P1V8",
    "P2V5_CPU_DDR4_EN",
    "P2V5_EN",
    "P3V3_EN",
    "P5V0_EN",   // 7
};

static char* power_ctrl_str2[] = {
    "PHY_P0V8_EN", // 0
    "P0V8_MAC_EN",
    "P0V85_CPU_AP_VDD_EN",
    "P1V15_EN",
    "P0V6_CPU_DDR4_VTT_EN",
    "P0V6_EN",
    "P1V2_CPU_DDR4_EN",
    "P1V2_EN",  // 7
};

static char* power_good_str1[] = {
    "PWRGD_VR_P1V2_OOB", // 0
    "PWRGD_P1V5",
    "PWRGD_P1V8_CPU",
    "PWRGD_P1V8",
    "PWRGD_P2V5_CPU_DDR4",
    "PWRGD_P2V5",
    "PWRGD_P3V3",
    "PWRGD_P5V0",   // 7
};

static char* power_good_str2[] = {
    "PWRGD_PHY_P0V8", // 0
    "PWRGD_VR_MAC_P0V8",
    "PWRGD_P0V85_CPU_AP_VDD",
    "PWRGD_P1V15",
    "PWRGD_VR_P0V6_CPU_DDR4_VTT",
    "PWRGD_VR_P0V6",
    "PWRGD_P1V2_CPU_DDR4",
    "PWRGD_P1V2",
};

enum psu_status_bit {
    PSU2_ON = 0,
    PSU1_ON,
    PSU2_AC_OK,
    PSU1_AC_OK,
    PSU2_PRESENT,
    PSU1_PRESENT,
    PSU2_PWRGD,
    PSU1_PWRGD,
};

enum fan_control_bit {
    FAN_FULL_SPEED = 0,
    FM_FAN3_EN = 1,
    FM_FAN2_EN = 2,
    FM_FAN1_EN = 3,
    FM_FAN3_TYPE = 4,
    FM_FAN2_TYPE = 5,
    FM_FAN1_TYPE = 6,
    PWRGD_VR_P12V_FAN3 = 7,
    PWRGD_VR_P12V_FAN2 = 8,
    PWRGD_VR_P12V_FAN1 = 9,
    CNFAN3_PRESENT = 10,
    CNFAN2_PRESENT = 11,
    CNFAN1_PRESENT = 12,
};

enum fan_id {
    FAN3 = 0,
    FAN2,
    FAN1,
};

enum port_id {
    PORT1 = 0,
    PORT2,
    PORT3,
    PORT4,
    PORT5,
    PORT6,
    PORT7,
    PORT8,
    PORT9,
    PORT10,
    PORT11,
    PORT12,
    PORT13,
    PORT14,
    PORT15,
    PORT16,
    PORT17,
    PORT18,
    PORT19,
    PORT20,
    PORT21,
    PORT22,
    PORT23,
    PORT24,
    PORT25,
    PORT26,
    PORT27,
    PORT28,
    PORT29,
    PORT30,
    PORT31,
    PORT32,
    PORT33,
    PORT34,
    PORT35,
    PORT36,
    PORT37,
    PORT38,
    PORT39,
    PORT40,
    PORT41,
    PORT42,
    PORT43,
    PORT44,
    PORT45,
    PORT46,
    PORT47,
    PORT48,
    PORT49,
    PORT50,
    PORT51,
    PORT52,
    PORT53,
    PORT54,
};

enum led_status_shift_bit {
    LOC_LED = 0,
    PSU_LED = 2,
    SYS_LED = 4,
    FAN_LED = 6,
};

enum rst_shift_bit {
    RST_MB_IOIEXP = 0,
    RST_PHY0_5 = 1,
    RST_POE_CTR = 2,
    CPLD_SYSRST_MAC_RST = 3,
    RST_CPU_SYSRST_OUT = 4,
    RST_GPHY_RST = 5,
    RST_I2C_MUX = 6,
    RST_MAC_JTAG = 7,
    RST_BUTTON_CPLD_CPU = 8,
    RST_BUTTON = 9,
    RST_CPLD_I2C_MUX = 10,
    RST_BUTTON_10S = 1,
};

enum int_shift_bit {
    INT_PHY0 = 0,
    INT_PHY1 = 1,
    INT_PHY2 = 2,
    INT_PHY3 = 3,
    INT_PHY4_0 = 4,
    INT_PHY4_1 = 5,
    INT_PHY5_0 = 6,
    INT_PHY5_1 = 7,

    INT_OOB_IOIEXP = 8,
    INT_MB_IOIEXP = 9,
    USB_OCP_ALERT = 10,
    TEMP_ALERT = 11,
    RTC_IRQ = 12,
    PSU2_INT_ALERT = 13,
    PSU1_INT_ALERT = 14,
    INT_PD69210 = 15,

    DEV_INIT_DONE = 16,
    FAN_CTL_FAIL = 17,
    MEMHOT = 18,

    MB_SYSTEM_INT = 19,
    MB_MOD_PRE_INT = 20,
};

enum msci_ctl2_shift_bit {
    PSU_VSENSE = 0,
    PSU_PWRGD = 1,
};

enum sysled_en_shift_bit {
    FRONT_PORT_LED_EN = 0,
    FRONT_SYS_LED_EN = 1,
    FRONT_PSU_LED_EN = 2,
    FRONT_FAN_LED_EN = 3,
    FRONT_LOC_LED_EN = 4,
};

enum watchdog_timer_def {
    WATCHDOG_180S = 0,
    WATCHDOG_210S = 1,
    WATCHDOG_240S = 2,
    WATCHDOG_270S = 3,
    WATCHDOG_300S = 4,
};

#endif /* _WISTRON_SWITCH_CPLD_H */
