/* header file for i2c cpld driver of ufispace_s9300_32d
 *
 * Copyright (C) 2017 UfiSpace Technology Corporation.
 * Leo Lin <leo.yt.lin@ufispace.com>
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

#ifndef UFISPACE_S9300_I2C_CPLD_H
#define UFISPACE_S9300_I2C_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3,
};

enum LED_BLINK {
    NOBLINK,
    BLINK,
};

enum LED_BLINK_SPEED {
    BLINK_1X,  // 0.5hz
    BLINK_4X,  // 2hz
};

enum LED_STATUS {
    OFF,
    ON,
};

enum LED_YELLOW {
    YELLOW_OFF,
    YELLOW_ON,
};

enum LED_GREEN {
    GREEN_OFF,
    GREEN_ON,
};

/* QSFPDD port number */
#define QSFPDD_MAX_PORT_NUM   32
#define QSFPDD_MIN_PORT_NUM   1

/* SFP+ port number */
#define SFP_MAX_PORT_NUM    4
#define SFP_MIN_PORT_NUM    1


/* CPLD registers */
/* CPLD 1 */
#define CPLD_SKU_ID_REG                     0x00
#define CPLD_HW_REV_REG	                    0x01
#define CPLD_VERSION_REG                    0x02
#define CPLD_ID_REG                         0x03
#define CPLD_BUILD_VER_REG                  0x04
// Interrupt status
#define CPLD_MAC_INTR_REG                   0x10
#define CPLD_10G_PHY_INTR_REG               0x13
#define CPLD_CPLD_FRU_INTR_REG              0x14
#define CPLD_THERMAL_ALERT_INTR_REG         0x16
#define CPLD_MISC_INTR_REG                  0x1B
#define CPLD_SYSTEM_INTR_REG                0x1D
// Interrupt mask
#define CPLD_MAC_INTR_MASK_REG              0x20
#define CPLD_10G_PHY_INTR_MASK_REG          0x23
#define CPLD_CPLD_FRU_INTR_MASK_REG         0x24
#define CPLD_THERMAL_ALERT_INTR_MASK_REG    0x26
#define CPLD_MISC_INTR_MASK_REG             0x2B
// Interrupt event
#define CPLD_MAC_INTR_EVENT_REG             0x30
#define CPLD_10G_PHY_INTR_EVENT_REG         0x33
#define CPLD_CPLD_FRU_INTR_EVENT_REG        0x14
#define CPLD_THERMAL_ALERT_INTR_EVENT_REG   0x16
#define CPLD_MISC_INTR_EVENT_REG            0x1B
// Reset ctrl
#define CPLD_MAC_RST_REG                    0x40
#define CPLD_10G_PHY_RST_REG                0x42
#define CPLD_BMC_RST_REG                    0x43
#define CPLD_USB_RST_REG                    0x44
#define CPLD_MUX_RST_REG                    0x46
#define CPLD_MISC_RST_REG                   0x48
#define CPLD_BMC_WATCHDOG_REG               0x4D
// Sys status
#define CPLD_DAU_BD_PRES_REG                0x50
#define CPLD_PSU_STATUS_REG                 0x51
#define CPLD_SYS_PW_STATUS_REG              0x52
#define CPLD_MISC_REG                       0x5B
// Mux ctrl
#define CPLD_MUX_CTRL_REG                   0x5C
#define CPLD_MAC_QSFP_SEL_CTRL_REG          0x5F
// Led ctrl
#define CPLD_SYS_LED_CTRL_1_REG             0x80
#define CPLD_SYS_LED_CTRL_2_REG             0x81
#define CPLD_BEACON_LED_CTRL_REG            0x84
#define CPLD_PORT_LED_CLR_CTRL_REG          0x85
// Event Detect Ctrl
#define CPLD_EVENT_DETECT_CTRL_REG          0x5D

/* CPLD 2 */
/*  G0 - port 0 ~ 7
    G1 - port 8 ~ 15
    G2 - port 16 ~ 23
    G3 - port 24 ~ 31
  */
// Interrupt status
#define CPLD_QSFPDD_MOD_INT_G0_REG	    	0x10
#define CPLD_QSFPDD_MOD_INT_G1_REG	    	0x11
#define CPLD_QSFPDD_MOD_INT_G2_REG	    	0x12
#define CPLD_QSFPDD_MOD_INT_G3_REG	    	0x13
#define CPLD_QSFPDD_PRES_G0_REG			    0x14
#define CPLD_QSFPDD_PRES_G1_REG			    0x15
#define CPLD_QSFPDD_PRES_G2_REG		    	0x16
#define CPLD_QSFPDD_PRES_G3_REG		    	0x17
#define CPLD_QSFPDD_FUSE_INT_G0_REG	    	0x18
#define CPLD_QSFPDD_FUSE_INT_G1_REG	    	0x19
#define CPLD_QSFPDD_FUSE_INT_G2_REG	    	0x1A
#define CPLD_QSFPDD_FUSE_INT_G3_REG	    	0x1B
#define CPLD_SFP_TXFAULT_REG			    0x1D
#define CPLD_SFP_ABS_REG				    0x1E
#define CPLD_SFP_RXLOS_REG			    	0x1F
// Interrupt mask
#define CPLD_QSFPDD_MOD_INT_MASK_G0_REG		0x20
#define CPLD_QSFPDD_MOD_INT_MASK_G1_REG		0x21
#define CPLD_QSFPDD_MOD_INT_MASK_G2_REG		0x22
#define CPLD_QSFPDD_MOD_INT_MASK_G3_REG		0x23
#define CPLD_QSFPDD_PRES_MASK_G0_REG		0x24
#define CPLD_QSFPDD_PRES_MASK_G1_REG		0x25
#define CPLD_QSFPDD_PRES_MASK_G2_REG		0x26
#define CPLD_QSFPDD_PRES_MASK_G3_REG		0x27
#define CPLD_QSFPDD_FUSE_INT_MASK_G0_REG	0x28
#define CPLD_QSFPDD_FUSE_INT_MASK_G1_REG	0x29
#define CPLD_QSFPDD_FUSE_INT_MASK_G2_REG	0x2A
#define CPLD_QSFPDD_FUSE_INT_MASK_G3_REG	0x2B
#define CPLD_SFP_TXFAULT_MASK_REG			0x2D
#define CPLD_SFP_ABS_MASK_REG				0x2E
#define CPLD_SFP_RXLOS_MASK_REG				0x2F
// Interrupt event
#define CPLD_QSFPDD_MOD_INT_EVENT_G0_REG    0x30
#define CPLD_QSFPDD_MOD_INT_EVENT_G1_REG	0x31
#define CPLD_QSFPDD_MOD_INT_EVENT_G2_REG	0x32
#define CPLD_QSFPDD_MOD_INT_EVENT_G3_REG	0x33
#define CPLD_QSFPDD_PRES_EVENT_G0_REG		0x34
#define CPLD_QSFPDD_PRES_EVENT_G1_REG		0x35
#define CPLD_QSFPDD_PRES_EVENT_G2_REG		0x36
#define CPLD_QSFPDD_PRES_EVENT_G3_REG		0x37
#define CPLD_QSFPDD_FUSE_INT_EVENT_G0_REG	0x38
#define CPLD_QSFPDD_FUSE_INT_EVENT_G1_REG	0x39
#define CPLD_QSFPDD_FUSE_INT_EVENT_G2_REG	0x3A
#define CPLD_QSFPDD_FUSE_INT_EVENT_G3_REG	0x3B
#define CPLD_SFP_TXFAULT_EVENT_REG			0x3D
#define CPLD_SFP_ABS_EVENT_REG				0x3E
#define CPLD_SFP_RXLOS_EVENT_REG			0x3F
// Port ctrl
#define CPLD_QSFPDD_RESET_CTRL_G0_REG	    0x40
#define CPLD_QSFPDD_RESET_CTRL_G1_REG	    0x41
#define CPLD_QSFPDD_RESET_CTRL_G2_REG	    0x42
#define CPLD_QSFPDD_RESET_CTRL_G3_REG	    0x43
#define CPLD_QSFPDD_LP_MODE_G0_REG		    0x44
#define CPLD_QSFPDD_LP_MODE_G1_REG		    0x45
#define CPLD_QSFPDD_LP_MODE_G2_REG		    0x46
#define CPLD_QSFPDD_LP_MODE_G3_REG		    0x47
#define CPLD_SFP_TX_DIS_REG				    0x55
#define CPLD_SFP_RS_REG					    0x56
#define CPLD_SFP_TS_REG					    0x57
// Port status
#define CPLD_PORT_INT_STATUS_REG            0x58


/* bit field structure for register value */
struct cpld_reg_sku_id_t {
    u8 model_id:8;
};

struct cpld_reg_hw_rev_t {
    u8 hw_rev:2;
    u8 deph_rev:1;
    u8 build_rev:3;
    u8 reserved:1;
    u8 id_type:1;
};

struct cpld_reg_version_t {
    u8 minor:6;
    u8 major:2;
};

struct cpld_reg_id_t {
    u8 id:3;
    u8 release:5;
};

struct cpld_reg_beacon_led_ctrl_t {
    u8 reserve:5;
    u8 speed:1;
    u8 blink:1;
    u8 onoff:1;
};

/* common manipulation */
#define INVALID(i, min, max)    ((i < min) || (i > max) ? 1u : 0u)
#define READ_BIT(val, bit)      ((0u == (val & (1<<bit))) ? 0u : 1u)
#define SET_BIT(val, bit)       (val |= (1 << bit))
#define CLEAR_BIT(val, bit)     (val &= ~(1 << bit))
#define TOGGLE_BIT(val, bit)    (val ^= (1 << bit))
#define _BIT(n)                 (1<<(n))
#define _BIT_MASK(len)          (BIT(len)-1)

/* bitfield of register manipulation */
#define READ_BF(bf_struct, val, bf_name, bf_value) \
    (bf_value = ((struct bf_struct *)&val)->bf_name)
#define READ_BF_1(bf_struct, val, bf_name, bf_value) \
    bf_struct bf; \
    bf.data = val; \
    bf_value = bf.bf_name
#define HW_REV_GET(val, res) \
    READ_BF(cpld_reg_hw_rev_t, val, hw_rev, res)
#define DEPH_REV_GET(val, res) \
    READ_BF(cpld_reg_hw_rev_t, val, deph_rev, res)
#define BUILD_REV_GET(val, res) \
    READ_BF(cpld_reg_hw_rev_t, val, build_rev, res)
#define ID_TYPE_GET(val, res) \
    READ_BF(cpld_reg_hw_rev_t, val, id_type, res)
#define CPLD_MAJOR_VERSION_GET(val, res) \
    READ_BF(cpld_reg_version_t, val, major, res)
#define CPLD_MINOR_VERSION_GET(val, res) \
    READ_BF(cpld_reg_version_t, val, minor, res)
#define CPLD_ID_ID_GET(val, res) \
    READ_BF(cpld_reg_id_t, val, id, res)

/* CPLD access functions */
extern int s9300_cpld_read(u8 cpld_idx, u8 reg);
extern int s9300_cpld_write(u8 cpld_idx, u8 reg, u8 value);

#endif

