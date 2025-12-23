/* header file for i2c cpld driver of ufispace_s9321_64eo
 *
 * Copyright (C) 2017 UfiSpace Technology Corporation.
 * Wade He <wade.ce.he@ufispace.com>
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

#ifndef UFISPACE_S9321_64EO_CPLD_MAIN_H
#define UFISPACE_S9321_64EO_CPLD_MAIN_H

#include <linux/module.h>
#include <linux/i2c.h>
#include <dt-bindings/mux/mux.h>
#include <linux/i2c-mux.h>
#include <linux/version.h>

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3,
    fpga,
};

/* 
 *  Normally, the CPLD register range is 0x00-0xff.
 *  Therefore, we define the invalid address 0x100 as NONE_REG
 */

#define NONE_REG                                        0x100

/* CPLD Common */
#define CPLD_VERSION_REG                                0x02
#define CPLD_ID_REG                                     0x03
#define CPLD_BUILD_REG                                  0x04
#define CPLD_EVT_CTRL_REG                               0x3F

/* CPLD 1 registers */
#define CPLD_BOARD_ID_0_REG                             0x00
#define CPLD_BOARD_ID_1_REG                             0x01
// Interrupt status
#define CPLD_MAC_INTR_REG                               0x10
#define CPLD_PHY_INTR_REG                               0x13
#define CPLD_CPLDX_INTR_REG                             0x14
#define CPLD_THERMAL_INTR_REG                           0x17
#define CPLD_MISC_INTR_REG                              0x1B
#define CPLD_CPU_INTR_REG                               0x1C
// Interrupt mask
#define CPLD_MAC_MASK_REG                               0x20
#define CPLD_PHY_MASK_REG                               0x23
#define CPLD_CPLDX_MASK_REG                             0x24
#define CPLD_THERMAL_MASK_REG                           0x27
#define CPLD_MISC_MASK_REG                              0x2B
#define CPLD_CPU_MASK_REG                               0x2C
// Interrupt event
#define CPLD_MAC_EVT_REG                                0x30
#define CPLD_PHY_EVT_REG                                0x33
#define CPLD_CPLDX_EVT_REG                              0x34
#define CPLD_THERMAL_EVT_REG                            0x37
#define CPLD_MISC_EVT_REG                               0x3B
// Reset ctrl
#define CPLD_MAC_RESET_REG                              0x40
#define CPLD_BMC_RESET_REG                              0x43
#define CPLD_USB_RESET_REG                              0x44
#define CPLD_MISC_RESET_REG                             0x48
// Sys status
#define CPLD_BRD_PRESENT_REG                            0x50
#define CPLD_PSU_STATUS_REG                             0x51
#define CPLD_SYSTEM_PWR_REG                             0x52
#define CPLD_MAC_SYNCE_REG                              0x53
#define CPLD_MAC_ROV_REG                                0x54
// Mux ctrl
#define CPLD_MUX_CTRL_REG                               0x5C
// Led ctrl
#define CPLD_SYSTEM_LED_SYS_FAN_REG                     0x80
#define CPLD_SYSTEM_LED_PSU_REG                         0x81
#define CPLD_SYSTEM_LED_SYNC_ID_REG                     0x82
#define CPLD_SFP_PORT_0_1_LED_REG                       0x83
#define CPLD_PORT_LED_CLR_REG                           0x85
// Power Good Status
#define CPLD_MISC_PWR_REG                               0x92
// Interrupt debug
#define DBG_CPLD_MAC_INTR_REG                           0xE0
#define DBG_CPLD_CPLDX_INTR_REG                         0xE4
#define DBG_CPLD_THERMAL_INTR_REG                       0xE7
#define DBG_CPLD_MISC_INTR_REG                          0xEB

/* CPLD 2 and CPLD 3*/
// Interrupt status
#define CPLD_OSFP_PORT_0_7_16_23_INTR_REG             0x10
#define CPLD_OSFP_PORT_8_15_24_31_INTR_REG            0x11
#define CPLD_OSFP_PORT_32_39_48_55_INTR_REG           0x12
#define CPLD_OSFP_PORT_40_47_56_63_INTR_REG           0x13
#define CPLD_OSFP_PORT_0_7_16_23_PRES_REG             0x14
#define CPLD_OSFP_PORT_8_15_24_31_PRES_REG            0x15
#define CPLD_OSFP_PORT_32_39_48_55_PRES_REG           0x16
#define CPLD_OSFP_PORT_40_47_56_63_PRES_REG           0x17
#define CPLD_OSFP_PORT_0_15_16_31_FUSE_REG            0x18
#define CPLD_OSFP_PORT_32_47_48_63_FUSE_REG           0x19
#define CPLD_OSFP_PORT_0_7_16_23_STUCK_REG            0x1A
#define CPLD_OSFP_PORT_8_15_24_31_STUCK_REG           0x1B
#define CPLD_OSFP_PORT_32_39_48_55_STUCK_REG          0x1C
#define CPLD_OSFP_PORT_40_47_56_63_STUCK_REG          0x1D
// Interrupt mask
#define CPLD_OSFP_PORT_0_7_16_23_INTR_MASK_REG        0x20
#define CPLD_OSFP_PORT_8_15_24_31_INTR_MASK_REG       0x21
#define CPLD_OSFP_PORT_32_39_48_55_INTR_MASK_REG      0x22
#define CPLD_OSFP_PORT_40_47_56_63_INTR_MASK_REG      0x23
#define CPLD_OSFP_PORT_0_7_16_23_PRES_MASK_REG        0x24
#define CPLD_OSFP_PORT_8_15_24_31_PRES_MASK_REG       0x25
#define CPLD_OSFP_PORT_32_39_48_55_PRES_MASK_REG      0x26
#define CPLD_OSFP_PORT_40_47_56_63_PRES_MASK_REG      0x27
#define CPLD_OSFP_PORT_0_15_16_31_FUSE_MASK_REG       0x28
#define CPLD_OSFP_PORT_32_47_48_63_FUSE_MASK_REG      0x29
#define CPLD_OSFP_PORT_0_7_16_23_STUCK_MASK_REG       0x2A
#define CPLD_OSFP_PORT_8_15_24_31_STUCK_MASK_REG      0x2B
#define CPLD_OSFP_PORT_32_39_48_55_STUCK_MASK_REG     0x2C
#define CPLD_OSFP_PORT_40_47_56_63_STUCK_MASK_REG     0x2D
// Interrupt event
#define CPLD_OSFP_PORT_0_7_16_23_INTR_EVENT_REG       0x30
#define CPLD_OSFP_PORT_8_15_24_31_INTR_EVENT_REG      0x31
#define CPLD_OSFP_PORT_32_39_48_55_INTR_EVENT_REG     0x32
#define CPLD_OSFP_PORT_40_47_56_63_INTR_EVENT_REG     0x33
#define CPLD_OSFP_PORT_0_7_16_23_PRES_EVENT_REG       0x34
#define CPLD_OSFP_PORT_8_15_24_31_PRES_EVENT_REG      0x35
#define CPLD_OSFP_PORT_32_39_48_55_PRES_EVENT_REG     0x36
#define CPLD_OSFP_PORT_40_47_56_63_PRES_EVENT_REG     0x37
#define CPLD_OSFP_PORT_0_15_16_31_FUSE_EVENT_REG      0x38
#define CPLD_OSFP_PORT_32_47_48_63_FUSE_EVENT_REG     0x39
#define CPLD_OSFP_PORT_0_7_16_23_STUCK_EVENT_REG      0x3A
#define CPLD_OSFP_PORT_8_15_24_31_STUCK_EVENT_REG     0x3B
#define CPLD_OSFP_PORT_32_39_48_55_STUCK_EVENT_REG    0x3C
#define CPLD_OSFP_PORT_40_47_56_63_STUCK_EVENT_REG    0x3D
// Port ctrl
#define CPLD_OSFP_PORT_0_7_16_23_RST_REG              0x40
#define CPLD_OSFP_PORT_8_15_24_31_RST_REG             0x41
#define CPLD_OSFP_PORT_32_39_48_55_RST_REG            0x42
#define CPLD_OSFP_PORT_40_47_56_63_RST_REG            0x43
#define CPLD_OSFP_PORT_0_7_16_23_LPMODE_REG           0x44
#define CPLD_OSFP_PORT_8_15_24_31_LPMODE_REG          0x45
#define CPLD_OSFP_PORT_32_39_48_55_LPMODE_REG         0x46
#define CPLD_OSFP_PORT_40_47_56_63_LPMODE_REG         0x47

#define CPLD_I2C_CONTROL_REG                            0xA0
#define CPLD_I2C_RELAY_REG                              0xA5
// Interrupt debug
#define CPLD_DBG_OSFP_PORT_0_7_16_23_INTR_REG         0xE0
#define CPLD_DBG_OSFP_PORT_8_15_24_31_INTR_REG        0xE1
#define CPLD_DBG_OSFP_PORT_32_39_48_55_INTR_REG       0xE2
#define CPLD_DBG_OSFP_PORT_40_47_56_63_INTR_REG       0xE3
#define CPLD_DBG_OSFP_PORT_0_7_16_23_PRES_REG         0xE4
#define CPLD_DBG_OSFP_PORT_8_15_24_31_PRES_REG        0xE5
#define CPLD_DBG_OSFP_PORT_32_39_48_55_PRES_REG       0xE6
#define CPLD_DBG_OSFP_PORT_40_47_56_63_PRES_REG       0xE7
#define CPLD_DBG_OSFP_PORT_0_15_16_31_FUSE_REG        0xE8
#define CPLD_DBG_OSFP_PORT_32_47_48_63_FUSE_REG       0xE9

//FPGA
#define FPGA_VERSION_REG                                0x02
#define FPGA_BUILD_REG                                  0x04
#define FPGA_CHIP_REG                                   0x05
#define FPGA_MGMT_PORT_0_1_TX_RATE_SEL_REG              0x0A
#define FPGA_MGMT_PORT_0_1_RX_RATE_SEL_REG              0x0B
#define FPGA_MGMT_PORT_0_1_TX_DIS_REG                   0x0C
#define FPGA_MGMT_PORT_0_1_TX_FAULT_REG                 0x10
#define FPGA_MGMT_PORT_0_1_RX_LOS_REG                   0x11
#define FPGA_MGMT_PORT_0_1_PRES_REG                     0x12
#define FPGA_MGMT_PORT_0_1_STUCK_REG                    0x13
#define FPGA_MGMT_PORT_0_1_TX_FAULT_MASK_REG            0x20
#define FPGA_MGMT_PORT_0_1_RX_LOS_MASK_REG              0x21
#define FPGA_MGMT_PORT_0_1_PRES_MASK_REG                0x22
#define FPGA_MGMT_PORT_0_1_STUCK_MASK_REG               0x23
#define FPGA_MGMT_PORT_0_1_TX_FAULT_EVENT_REG           0x30
#define FPGA_MGMT_PORT_0_1_RX_LOS_EVENT_REG             0x31
#define FPGA_MGMT_PORT_0_1_PRES_EVENT_REG               0x32
#define FPGA_MGMT_PORT_0_1_STUCK_EVENT_REG              0x33
#define FPGA_EVT_CTRL_REG                               0x3F
#define FPGA_LAN_PORT_RELAY_REG                         0x40

//MASK
#define MASK_ALL                           (0xFF)
#define MASK_NONE                          (0x00)
#define MASK_0000_0001                     (0x01)
#define MASK_0000_0010                     (0x02)
#define MASK_0000_0011                     (0x03)
#define MASK_0000_0100                     (0x04)
#define MASK_0000_0111                     (0x07)
#define MASK_0000_1000                     (0x08)
#define MASK_0000_1101                     (0x0D)
#define MASK_0000_1111                     (0x0F)
#define MASK_0001_0000                     (0x10)
#define MASK_0001_1000                     (0x18)
#define MASK_0010_0000                     (0x20)
#define MASK_0011_1000                     (0x38)
#define MASK_0011_1111                     (0x3F)
#define MASK_0100_0000                     (0x40)
#define MASK_1000_0000                     (0x80)
#define MASK_1100_0000                     (0xC0)
#define MASK_1101_0000                     (0xD0)
#define MASK_1110_0000                     (0xE0)
#define MASK_1111_0000                     (0xF0)


// MUX
#define CPLD_MAX_NCHANS 32
#define CPLD_MUX_TIMEOUT                   1400
#define CPLD_MUX_RETRY_WAIT                200
#define CPLD_MUX_CHN_OFF                   (0x0)
#define FPGA_MUX_CHN_OFF                   (0x0)
#define CPLD_I2C_ENABLE_BRIDGE             MASK_1000_0000
#define CPLD_I2C_ENABLE_CHN_SEL            MASK_1000_0000
#define FPGA_LAN_PORT_RELAY_ENABLE         MASK_1000_0000

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)

struct cpld_data {
    int index;                  /* CPLD index */
    struct mutex access_lock;   /* mutex for cpld access */
    u8 access_reg;              /* register to access */

    const struct chip_desc *chip;
    u32 last_chan;       /* last register value */
    /* MUX_IDLE_AS_IS, MUX_IDLE_DISCONNECT or >= 0 for channel */
    s32 idle_state;

    struct i2c_client *client;
    raw_spinlock_t lock;
};

struct chip_desc {
    u8 nchans;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
    struct i2c_device_identity id;
#endif /* #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) */
};

/*
 *  Generally, the color bit for CPLD is 4 bits, and there are 16 color sets available.
 *  The color bit for GPIO is 2 bits (representing two GPIO pins), and there are 4 color sets.
 *  Therefore, we use the 16 color sets available for our application.
 */
#define COLOR_VAL_MAX           16

typedef enum {
    LED_COLOR_DARK,
    LED_COLOR_GREEN,
    LED_COLOR_YELLOW,
    LED_COLOR_RED,
    LED_COLOR_BLUE,
    LED_COLOR_GREEN_BLINK,
    LED_COLOR_YELLOW_BLINK,
    LED_COLOR_RED_BLINK,
    LED_COLOR_BLUE_BLINK,
    LED_COLOR_CYAN=100,
    LED_COLOR_MAGENTA,
    LED_COLOR_WHITE,
    LED_COLOR_CYAN_BLINK,
    LED_COLOR_MAGENTA_BLINK,
    LED_COLOR_WHITE_BLINK,
} s3ip_led_status_e;

typedef enum {
    TYPE_LED_UNNKOW = 0,
    // Blue
    TYPE_LED_1_SETS,

    // Green, Yellow
    TYPE_LED_2_SETS,

    // Red, Green, Blue, Yellow, Cyan, Magenta, white
    TYPE_LED_7_SETS,
    TYPE_LED_SETS_MAX,
} led_type_e;

typedef enum {
    PORT_NONE_BLOCK = 0,
    PORT_BLOCK      = 1,
} port_block_status_e;

typedef struct
{
    short int val;
    int status;
} color_obj_t;

typedef struct  {
    int type;
    u8 reg;
    u8 mask;
    u8 color_mask;
    u8 data_type;
    color_obj_t color_obj[COLOR_VAL_MAX];
} led_node_t;


u8 _mask_shift(u8 val, u8 mask);
int _cpld_reg_write(struct device *dev, u8 reg, u8 reg_val);
int _cpld_reg_read(struct device *dev, u8 reg, u8 mask);
int mux_select_chan(struct i2c_mux_core *muxc, u32 chan);
int mux_deselect_mux(struct i2c_mux_core *muxc, u32 chan);
ssize_t idle_state_show(struct device *dev,
            struct device_attribute *attr,
            char *buf);

ssize_t idle_state_store(struct device *dev,
            struct device_attribute *attr,
            const char *buf, size_t count);
int mux_init(struct device *dev);
void mux_cleanup(struct device *dev);
#endif
