/*
*
* $Id: aperta2_msg_task.h,  $
*
*  *
*  *
  * $Copyright: (c) 2022 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
*  *
*  *
*
*/
#ifndef APERTA2_MSG_TASKS_H
#define APERTA2_MSG_TASKS_H

#include <phymod/phymod_acc.h>
#include <tier1/aperta2_sdk_intf.h>

#define APERTA2_FW_MSG_RETRY_CNT     100

#define APERTA2_OUTPUT_BUFFER_READ                           \
    addr = APERTA2_MSG_INTF0_OUT_BUFFER_ADDR;                \
    PHYMOD_IF_ERR_RETURN(                                   \
        PHYMOD_BUS_READ(&phy->access,  addr, &length));      \
    plp_aperta2_put_half_word (&rx_msg, length);                 \
    addr++;                                                  \
    length = (length + 1) / 2;                               \
    while (length) {                                         \
          PHYMOD_IF_ERR_RETURN(                              \
            PHYMOD_BUS_READ(&phy->access,  addr, &value));   \
        plp_aperta2_put_half_word (&rx_msg, value);              \
        addr++;                                              \
        length--;                                            \
    }

#define APERTA2_FW_FUN_SUPPORT_START_RESULT(FUN)    \
    (FUN == APERTA2_FUNC_ENABLE_PORT  ||            \
     FUN == APERTA2_FUNC_DISABLE_PORT ||            \
     FUN == APERTA2_FUNC_FLUSH_PORT   ||            \
     FUN == APERTA2_FUNC_INIT_EIP164 ||             \
     FUN == APERTA2_FUNC_SWITCH_MUX||               \
     FUN == APERTA2_FUNC_SWITCH_DPCLK)

typedef enum aperta2_tsc_clk_set_e {
    APERTA2_SYS_DP_CLK = 0,
    APERTA2_LINE_DP_CLK = 1
} aperta2_tsc_clk_set_t;

/* Interfacing structures between FW and SW*/
typedef struct aperta2_pm_reg_s {
    uint8_t pm_sel;                       /*Portmacro select*/ 
    uint8_t access_type;                  /* Access type to perform PM register read/write*/
    uint8_t datalen;                      /* Length of data*/
    uint32_t addr;                        /* Address to perform read/write*/
    uint64_t *data;                       /* Data for read/write*/
    uint32_t *mem_data;                   /* Data of PM memory*/
} aperta2_pm_reg_t;

typedef struct aperta2_tsc_reg_s {
    uint8_t pm_sel;                       /* Portmacro select */ 
    uint32_t addr;                        /* Address to perform TSC read/write*/
    uint32_t *data;                       /* Data for read/write*/
} aperta2_tsc_reg_t;

typedef struct aperta2_ind_reg_s {
    uint8_t oct_sel;                      /* Octal select */ 
    uint16_t addr;                        /* Address to perform Indirect read/write*/
    uint32_t *data;                       /* Data for read/write*/
} aperta2_ind_reg_t;

typedef struct aperta2_macsec_reg_s {
    uint8_t oct_sel;                      /* Octal select */ 
    uint32_t addr;                        /* Address to perform MACSEC read/write*/
    uint32_t *data;                       /* Data for read/write*/
} aperta2_macsec_reg_t;

typedef struct aperta2_ahb_reg_s {
    uint32_t addr;                        /* Address to perform MACSEC read/write*/
    uint32_t *data;                       /* Data for read/write*/
} aperta2_ahb_reg_t;

typedef struct aperta2_config_phy_s {
    uint8_t macsec_option;                /* bit0:3 Enable/Disable StaticBypass per octal Ingress/Egress*/
    uint8_t ptp_option;                   /* bit 0/1: Enable/Disable PTP Bypass, Bit2 PTP sync/async mode Bit3: PTP invert External ref clk*/
    uint8_t port_operation;               /* Set/get APERTA2_MSG_OPERATION*/
} aperta2_config_phy_t;

/* Structure that holds SWITCH_DPCLK message parameters */
typedef struct aperta2_switch_dpclk_s {
    uint8_t  OctalSel;                    /* Octal Select*/
    uint8_t  ClkSel;                      /* TSC Clock Select*/
} aperta2_switch_dpclk_t;

typedef struct aperta2_config_lane_s {
    uint8_t num_sys_lane;                /* Number of system side lanes*/
    uint8_t num_line_lane;               /* Number of line side lanes*/
    uint8_t sys_lane_list[16];           /* System Side lane list*/
    uint8_t line_lane_list[16];          /* Line side lane list*/
} aperta2_config_lane_t;

/*!
 *  Structure that holds CONFIG_PORT message parameters
 */
typedef struct aperta2_config_port_s
{
    uint8_t  PortNum;                     /* Hardware Port Number*/
    uint8_t  PortType;                    /* PortType (Repeater/Gearbox/Rev-Gearbox)*/
    uint8_t  PortMode;                    /* PortMode (Regular/FailoverMUX)*/
    uint8_t  SysPortSpeed;                /* PortSpeed*/
    uint8_t  LinePortSpeed;               /* PortSpeed*/
    uint8_t  SPMPortID;                   /* System-side Port Macro's Port ID*/
    uint8_t  LPMPortID;                   /* Line-side Port Macro's Port ID*/
    uint16_t PortOptions;                 /* Port configuration options*/
    uint16_t IngFixedLatency;             /* Fixed-Latency Value (Ingress)*/
    uint16_t EgrFixedLatency;             /* Fixed-Latency Value (Egress)*/
    uint8_t  FOOptions;                   /* Fail-over MUX Options*/
    uint8_t  FOPortID;                    /* Fail-over Port Macro Port ID*/
    uint8_t  port_operation;               /* Set/get APERTA2_MSG_OPERATION*/
}aperta2_config_port_t;

/*!
 *  Structure that holds PTP_CONFIG message parameters
 */
typedef struct aperta2_ptp_config_s
{
    uint8_t  PortNum;            /* Hardware Port Number*/
    uint8_t  PortOptions;        /* Port configuration options*/
    uint16_t IngFixedLatency;    /* Fixed-Latency Value (Ingress)*/
    uint16_t EgrFixedLatency;    /* Fixed-Latency Value (Egress)*/
} aperta2_ptp_config_t;

/*!
 *  Structure that holds CLOCK_GEN message parameters
 */
typedef struct aperta2_clock_gen_s
{
    uint8_t  RClkNum;      /* Recovered Clock Number */
    uint8_t  ClkGenEn;     /* Clock Generation Enable*/
    uint8_t  ClkPortNum;   /* Clock Port number*/
    uint8_t  ClkSide;      /* Recovered Clock Side*/
    uint8_t  ClkLane;      /* Recovered Clock Lane Number*/
    uint8_t  Divider;      /* Recovered Clock Divider*/
    uint8_t  SquelchMode;  /* Squelch Mode*/
} aperta2_clock_gen_t;

/* Aperta SyncE defines */
#define APERTA2_CLKGEN_DIS               0x0
#define APERTA2_CLKGEN_ENA               0x1

#define APERTA2_CLKGEN_CLKSIDE_SYS       0x0
#define APERTA2_CLKGEN_CLKSIDE_LINE      0x1

#define APERTA2_CLKGEN_OCT0_RCLKNUM_0    0x0
#define APERTA2_CLKGEN_OCT0_RCLKNUM_1    0x1
#define APERTA2_CLKGEN_OCT0_RCLKNUM_2    0x2
#define APERTA2_CLKGEN_OCT1_RCLKNUM_0    0x3
#define APERTA2_CLKGEN_OCT1_RCLKNUM_1    0x4
#define APERTA2_CLKGEN_OCT1_RCLKNUM_2    0x5
#define APERTA2_CLKGEN_RCLK_MAX          0x6

#define APERTA2_CLKGEN_NO_SQUELCH        0x0
#define APERTA2_CLKGEN_SQUELCH_LOS       0x1
#define APERTA2_CLKGEN_SQUELCH_LOL       0x2

/* Dividers for Differential Clock Ports (APERTA2_CLKGEN_RCLKNUM_0)*/
#define APERTA2_CLKGEN_DIVIDER_20         0x00	
#define APERTA2_CLKGEN_DIVIDER_40         0x01	
#define APERTA2_CLKGEN_DIVIDER_160        0x04	
#define APERTA2_CLKGEN_DIVIDER_320        0x06	
#define APERTA2_CLKGEN_DIVIDER_640        0x09	

/* Dividers for Single-ended and Differential Clock Ports (APERTA2_CLKGEN_RCLKNUM_0_1_2)*/
#define APERTA2_CLKGEN_DIVIDER_80         0x02	

/* Dividers for Single-ended Clock Ports (APERTA2_CLKGEN_RCLKNUM_1_2)*/
#define APERTA2_CLKGEN_DIVIDER_120        0x03	
#define APERTA2_CLKGEN_DIVIDER_240        0x05	
#define APERTA2_CLKGEN_DIVIDER_480        0x07	
#define APERTA2_CLKGEN_DIVIDER_520        0x08	
#define APERTA2_CLKGEN_DIVIDER_960        0x0A	
#define APERTA2_CLKGEN_DIVIDER_1000       0x0B	
#define APERTA2_CLKGEN_DIVIDER_1040       0x0C	
#define APERTA2_CLKGEN_DIVIDER_2000       0x0D	
#define APERTA2_CLKGEN_DIVIDER_2040       0x0E	
#define APERTA2_CLKGEN_DIVIDER_2080       0x0F	
#define APERTA2_CLKGEN_DIVIDER_4000       0x10	
#define APERTA2_CLKGEN_DIVIDER_4080       0x11	
#define APERTA2_CLKGEN_DIVIDER_8160       0x12	
#define APERTA2_CLKGEN_DIVIDER_16320      0x13	
#define APERTA2_CLKGEN_DIVIDER_32640      0x14	


int plp_aperta2_msg_send (const plp_aperta2_phymod_phy_access_t *phy,  uint8_t function, uint8_t operation,
                      uint8_t *tx_msg, uint8_t *rx_msg, uint8_t *result);
/* CONFIG_PHY functions*/
int plp_aperta2_fw_config_phy(const plp_aperta2_phymod_phy_access_t *phy,  aperta2_config_phy_t *phy_cfg);
int plp_aperta2_fw_mem_access(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_reg_t *mem, int write_en);
/* CONFIG_PORT functions*/
int plp_aperta2_fw_config_port(const plp_aperta2_phymod_phy_access_t *phy,  aperta2_config_port_t *port_cfg);
/* Port operation*/
int plp_aperta2_fw_port_op (const plp_aperta2_phymod_phy_access_t *phy,  uint8_t port_num, int function);
/* Switch dp clock */
int plp_aperta2_switch_dp_clock(const plp_aperta2_phymod_phy_access_t *phy, int operation, aperta2_switch_dpclk_t switch_dp_clk);

int plp_aperta2_fw_write_register (const plp_aperta2_phymod_phy_access_t *phy, int block, void *data);
int plp_aperta2_fw_read_register (const plp_aperta2_phymod_phy_access_t *phy, int block, void *data);
int plp_aperta2_fw_reg_access(const plp_aperta2_phymod_phy_access_t* phy, int write_en, int block, void *data) ;

int plp_aperta2_fw_config_ptp_write (const plp_aperta2_phymod_phy_access_t* phy, aperta2_ptp_config_t *ptp_cfg);
int plp_aperta2_fw_config_ptp_read (const plp_aperta2_phymod_phy_access_t* phy, aperta2_ptp_config_t *ptp_cfg);

int plp_aperta2_fw_config_lanes (const plp_aperta2_phymod_phy_access_t* phy, aperta2_config_lane_t *lane_cfg, int op);

/* FW MSG Helper functions */
void plp_aperta2_put_byte (uint8_t **ptr, uint8_t value);
uint8_t plp_aperta2_get_byte (uint8_t **ptr);
void plp_aperta2_put_half_word (uint8_t **ptr, uint16_t value);
uint16_t plp_aperta2_get_half_word (uint8_t **ptr);
uint32_t plp_aperta2_get_word (uint8_t **ptr);
void plp_aperta2_put_word (uint8_t **ptr, uint32_t value);
void plp_aperta2_put_24bits (uint8_t **ptr, uint32_t value);
int plp_aperta2_fw_switch_mux (const plp_aperta2_phymod_phy_access_t* phy, int port_num);
int plp_aperta2_fw_clock_gen (const plp_aperta2_phymod_phy_access_t *phy,  int op, aperta2_clock_gen_t *clk_cfg);
int plp_aperta2_fw_package_polarity_set (const plp_aperta2_phymod_phy_access_t* phy, int pm_sel);
#endif /*APERTA2_MSG_TASKS_H*/
