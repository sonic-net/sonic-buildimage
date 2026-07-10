/* header file for i2c cpld driver of NETBERG_NBA750_64X
 *
 */

#ifndef NETBERG_NBA750_64X_I2C_CPLD_H
#define NETBERG_NBA750_64X_I2C_CPLD_H

/* CPLD device index value */
enum cpld_id {
    cpld1,
    cpld2,
    cpld3,
    cpld4,
    cpld5
};

enum LED_BLINK {
    BLINK,
    NOBLINK,
};

enum LED_YELLOW {
    YELLOW_OFF,
    YELLOW_ON,
};

enum LED_GREEN {
    GREEN_OFF,
    GREEN_ON,
};

/* port number on CPLD */
#define CPLD_1_PORT_NUM 12
#define CPLD_2_PORT_NUM 13

/* QSFP port number */
#define QSFP_MAX_PORT_NUM   64
#define QSFP_MIN_PORT_NUM   1

/* SFP+ port number */
#define SFP_MAX_PORT_NUM    2
#define SFP_MIN_PORT_NUM    1


/* CPLD registers */
#define CPLD_BOARD_TYPE_REG             0x0
#define CPLD_EXT_BOARD_TYPE_REG         0x7
#define CPLD_VERSION_REG                0x1
#define CPLD_ID_REG                     0x2
#define CPLD_QSFP_PORT_STATUS_BASE_REG  0x20
#define CPLD_QSFP_PORT_CONFIG_BASE_REG  0x30
#define CPLD_QSFP_PORT_INTERRUPT_REG    0x40
#define CPLD_SFP_PORT_STATUS_REG        0x2F
#define CPLD_SFP_PORT_CONFIG_REG        0x3F
#define CPLD_QSFP_PORT_INTERRUPT_REG    0x40
#define CPLD_10GMUX_CONFIG_REG          0x41
#define CPLD_BMC_STATUS_REG             0x42
#define CPLD_BMC_WATCHDOG_REG           0x43
#define CPLD_USB_STATUS_REG             0x44
#define CPLD_RESET_CONTROL_REG          0x4A
#define CPLD_SFP_LED_REG                0x80
#define CPLD_SFP_LED_BLINK_REG          0x90
#define CPLD_QSFP_LED_BASE_REG          0x80
#define CPLD_QSFP_LED_BLINK_BASE_REG    0x90
#define CPLD_RTMR_RESET_REG    0x4B
#define CPLD_ROV_STATUS_REG    0x4C

/* bit definition for register value */
enum CPLD_QSFP_PORT_STATUS_BITS {
    CPLD_QSFP_PORT_STATUS_INT_BIT,
    CPLD_QSFP_PORT_STATUS_ABS_BIT,
};
enum CPLD_QSFP_PORT_CONFIG_BITS {
    CPLD_QSFP_PORT_CONFIG_RESET_BIT,
    CPLD_QSFP_PORT_CONFIG_RESERVE_BIT,
    CPLD_QSFP_PORT_CONFIG_LPMODE_BIT,
};
enum CPLD_SFP_PORT_STATUS_BITS {
    CPLD_SFP0_PORT_STATUS_PRESENT_BIT,
    CPLD_SFP0_PORT_STATUS_TXFAULT_BIT,
    CPLD_SFP0_PORT_STATUS_RXLOS_BIT,
    CPLD_SFP_PORT_STATUS_DUMMY,
    CPLD_SFP1_PORT_STATUS_PRESENT_BIT,
    CPLD_SFP1_PORT_STATUS_TXFAULT_BIT,
    CPLD_SFP1_PORT_STATUS_RXLOS_BIT,
};
enum CPLD_SFP_PORT_CONFIG_BITS {
    CPLD_SFP0_PORT_CONFIG_TXDIS_BIT,
    CPLD_SFP0_PORT_CONFIG_RS_BIT,
    CPLD_SFP0_PORT_CONFIG_TS_BIT,
    CPLD_SFP_PORT_CONFIG_DUMMY,
    CPLD_SFP1_PORT_CONFIG_TXDIS_BIT,
    CPLD_SFP1_PORT_CONFIG_RS_BIT,
    CPLD_SFP1_PORT_CONFIG_TS_BIT,

};
enum CPLD_10GMUX_CONFIG_BITS {
    CPLD_10GMUX_CONFIG_ENSMB_BIT,
    CPLD_10GMUX_CONFIG_ENINPUT_BIT,
    CPLD_10GMUX_CONFIG_SEL1_BIT,
    CPLD_10GMUX_CONFIG_SEL0_BIT,
};
enum CPLD_BMC_WATCHDOG_BITS {
    CPLD_10GMUX_CONFIG_ENTIMER_BIT,
    CPLD_10GMUX_CONFIG_TIMEOUT_BIT,
};
enum CPLD_RESET_CONTROL_BITS {
    CPLD_RESET_CONTROL_SWRST_BIT,
    CPLD_RESET_CONTROL_CP2104RST_BIT,
    CPLD_RESET_CONTROL_82P33814RST_BIT,
    CPLD_RESET_CONTROL_BMCRST_BIT,
};
enum CPLD_SFP_LED_BITS {
    CPLD_SFP_LED_SFP0_GREEN_BIT,
    CPLD_SFP_LED_SFP0_YELLOW_BIT,
    CPLD_SFP_LED_SFP1_GREEN_BIT,
    CPLD_SFP_LED_SFP1_YELLOW_BIT,
};
enum CPLD_SFP_LED_BLINK_BITS {
    CPLD_SFP_LED_BLINK_SFP0_BIT,
    CPLD_SFP_LED_BLINK_SFP1_BIT,
};
enum CPLD_QSFP_LED_BITS {
    CPLD_QSFP_LED_CHAN_0_GREEN_BIT,
    CPLD_QSFP_LED_CHAN_0_YELLOW_BIT,
    CPLD_QSFP_LED_CHAN_1_GREEN_BIT,
    CPLD_QSFP_LED_CHAN_1_YELLOW_BIT,
    CPLD_QSFP_LED_CHAN_2_GREEN_BIT,
    CPLD_QSFP_LED_CHAN_2_YELLOW_BIT,
    CPLD_QSFP_LED_CHAN_3_GREEN_BIT,
    CPLD_QSFP_LED_CHAN_3_YELLOW_BIT,
    
};
enum CPLD_QSFP_LED_BLINK_BITS {
    CPLD_QSFP_LED_BLINK_X_CHAN0_BIT,
    CPLD_QSFP_LED_BLINK_X_CHAN1_BIT,
    CPLD_QSFP_LED_BLINK_X_CHAN2_BIT,
    CPLD_QSFP_LED_BLINK_X_CHAN3_BIT,
    CPLD_QSFP_LED_BLINK_XPLUS_CHAN0_BIT,
    CPLD_QSFP_LED_BLINK_XPLUS_CHAN1_BIT,
    CPLD_QSFP_LED_BLINK_XPLUS_CHAN2_BIT,
    CPLD_QSFP_LED_BLINK_XPLUS_CHAN3_BIT,
};

/* bit field structure for register value */
struct cpld_reg_board_type_t {
    u8 build_rev:2;
    u8 hw_rev:2;
    u8 board_id:4;
};

struct cpld_reg_version_t {
    u8 revision:6;
    u8 release:1;
    u8 reserve:1;
};

struct cpld_reg_id_t {
    u8 id:3;
    u8 release:5;
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
#define BOARD_TYPE_BUILD_REV_GET(val, res) \
    READ_BF(cpld_reg_board_type_t, val, build_rev, res)
#define BOARD_TYPE_HW_REV_GET(val, res) \
    READ_BF(cpld_reg_board_type_t, val, hw_rev, res)
#define BOARD_TYPE_BOARD_ID_GET(val, res) \
    READ_BF(cpld_reg_board_type_t, val, board_id, res)
#define CPLD_VERSION_REV_GET(val, res) \
    READ_BF(cpld_reg_version_t, val, revision, res)
#define CPLD_VERSION_REL_GET(val, res) \
    READ_BF(cpld_reg_version_t, val, release, res)
#define CPLD_ID_ID_GET(val, res) \
    READ_BF(cpld_reg_id_t, val, id, res)
#define CPLD_ID_REL_GET(val, res) \
    READ_BF(cpld_reg_id_t, val, release, res)
/* SFP/QSFP port led registers manipulation */
#define SFP_LED_TO_CPLD_IDX(sfp_port) cpld1
#define SFP_LED_REG(sfp_port) CPLD_SFP_LED_REG
#define SFP_LED_BLINK_REG(sfp_port) CPLD_SFP_LED_BLINK_REG
#define QSFP_LED_TO_CPLD_IDX(qsfp_port) \
    ((qsfp_port - 1) / 16 + 2)
#define QSFP_LED_REG(qsfp_port) \
    ((qsfp_port - 1) % 16 + CPLD_QSFP_LED_BASE_REG)
#define QSFP_LED_BLINK_REG(qsfp_port) \
    (((qsfp_port - 1) % 16) / 2 + CPLD_QSFP_LED_BLINK_BASE_REG)
/* QSFP/SFP port status registers manipulation */
#define QSFP_TO_CPLD_IDX(qsfp_port, cpld_index, cpld_port) \
{ \
    if (QSFP_MIN_PORT_NUM <= qsfp_port && qsfp_port <= CPLD_1_PORT_NUM) { \
        cpld_index = cpld1; \
        cpld_port = qsfp_port - 1; \
    } else if (CPLD_1_PORT_NUM < qsfp_port \
        && qsfp_port <= QSFP_MAX_PORT_NUM) { \
        cpld_index = cpld2 + (qsfp_port - 1 - CPLD_1_PORT_NUM) \
                / CPLD_2_PORT_NUM; \
        cpld_port = (qsfp_port - 1 - CPLD_1_PORT_NUM) % \
                CPLD_2_PORT_NUM; \
    } else { \
        cpld_index = 0; \
        cpld_port = 0; \
    } \
}
#define QSFP_PORT_STATUS_REG(cpld_port) \
    (CPLD_QSFP_PORT_STATUS_BASE_REG + cpld_port)
#define QSFP_PORT_CONFIG_REG(cpld_port) \
    (CPLD_QSFP_PORT_CONFIG_BASE_REG + cpld_port)
#define QSFP_PORT_INT_BIT_GET(port_status_value) \
    READ_BIT(port_status_value, CPLD_QSFP_PORT_STATUS_INT_BIT)
#define QSFP_PORT_ABS_BIT_GET(port_status_value) \
    READ_BIT(port_status_value, CPLD_QSFP_PORT_STATUS_ABS_BIT)
#define QSFP_PORT_RESET_BIT_GET(port_config_value) \
    READ_BIT(port_config_value, CPLD_QSFP_PORT_CONFIG_RESET_BIT)
#define QSFP_PORT_LPMODE_BIT_GET(port_config_value) \
    READ_BIT(port_config_value, CPLD_QSFP_PORT_CONFIG_LPMODE_BIT)
#define QSFP_PORT_RESET_BIT_SET(port_config_value) \
    SET_BIT(port_config_value, CPLD_QSFP_PORT_CONFIG_RESET_BIT)
#define QSFP_PORT_RESET_BIT_CLEAR(port_config_value) \
    CLEAR_BIT(port_config_value, CPLD_QSFP_PORT_CONFIG_RESET_BIT)
#define QSFP_PORT_LPMODE_BIT_SET(port_config_value) \
    SET_BIT(port_config_value, CPLD_QSFP_PORT_CONFIG_LPMODE_BIT)
#define QSFP_PORT_LPMODE_BIT_CLEAR(port_config_value) \
    CLEAR_BIT(port_config_value, CPLD_QSFP_PORT_CONFIG_LPMODE_BIT)
#define SFP_PORT_PRESENT_BIT_GET(sfp_port, port_status_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        READ_BIT(port_status_value, CPLD_SFP0_PORT_STATUS_PRESENT_BIT); \
    } else { \
        READ_BIT(port_status_value, CPLD_SFP1_PORT_STATUS_PRESENT_BIT); \
    }
#define SFP_PORT_TXFAULT_BIT_GET(sfp_port, port_status_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        READ_BIT(port_status_value, CPLD_SFP0_PORT_STATUS_TXFAULT_BIT); \
    } else { \
        READ_BIT(port_status_value, CPLD_SFP1_PORT_STATUS_TXFAULT_BIT); \
    }
#define SFP_PORT_RXLOS_BIT_GET(sfp_port, port_status_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        READ_BIT(port_status_value, CPLD_SFP0_PORT_STATUS_RXLOS_BIT); \
    } else { \
        READ_BIT(port_status_value, CPLD_SFP1_PORT_STATUS_RXLOS_BIT); \
    }
#define SFP_PORT_TXDIS_BIT_GET(sfp_port, port_config_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        READ_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_TXDIS_BIT); \
    } else { \
        READ_BIT(port_config_value, CPLD_SFP1_PORT_STATUS_RXLOS_BIT); \
    }
#define SFP_PORT_RS_BIT_GET(sfp_port, port_config_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        READ_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_RS_BIT); \
    } else { \
        READ_BIT(port_config_value, CPLD_SFP1_PORT_CONFIG_RS_BIT); \
    }
#define SFP_PORT_TS_BIT_GET(sfp_port, port_config_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        READ_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_TS_BIT); \
    } else { \
        READ_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_TS_BIT); \
    }
#define SFP_PORT_TXDIS_BIT_SET(sfp_port, port_config_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        SET_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_TXDIS_BIT); \
    } else { \
        SET_BIT(port_config_value, CPLD_SFP1_PORT_CONFIG_TXDIS_BIT); \
    }
#define SFP_PORT_TXDIS_BIT_CLEAR(sfp_port, port_config_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        CLEAR_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_TXDIS_BIT); \
    } else { \
        CLEAR_BIT(port_config_value, CPLD_SFP1_PORT_CONFIG_TXDIS_BIT); \
    }
#define SFP_PORT_RS_BIT_SET(sfp_port, port_config_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        SET_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_RS_BIT); \
    } else { \
        SET_BIT(port_config_value, CPLD_SFP1_PORT_CONFIG_RS_BIT); \
    }
#define SFP_PORT_RS_BIT_CLEAR(sfp_port, port_config_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        CLEAR_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_RS_BIT); \
    } else { \
        CLEAR_BIT(port_config_value, CPLD_SFP1_PORT_CONFIG_RS_BIT); \
    }
#define SFP_PORT_TS_BIT_SET(sfp_port, port_config_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        SET_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_TS_BIT); \
    } else { \
        SET_BIT(port_config_value, CPLD_SFP1_PORT_CONFIG_TS_BIT); \
    }
#define SFP_PORT_TS_BIT_CLEAR(sfp_port, port_config_value) \
    if (sfp_port == SFP_MIN_PORT_NUM) { \
        CLEAR_BIT(port_config_value, CPLD_SFP0_PORT_CONFIG_TS_BIT); \
    } else { \
        CLEAR_BIT(port_config_value, CPLD_SFP1_PORT_CONFIG_TS_BIT); \
    }

/* CPLD access functions */
extern int netberg_i2c_cpld_get_qsfp_port_status_val(u8 port_num);
extern int netberg_i2c_cpld_get_qsfp_port_config_val(u8 port_num);
extern int netberg_i2c_cpld_set_qsfp_port_config_val(u8 port_num, u8 reg_val);
extern int netberg_i2c_cpld_get_sfp_port_status_val(void);
extern int netberg_i2c_cpld_get_sfp_port_config_val(void);
extern int netberg_i2c_cpld_set_sfp_port_config_val(u8 reg_val);
extern u8 fp_port_to_phy_port(u8 fp_port);
#endif

