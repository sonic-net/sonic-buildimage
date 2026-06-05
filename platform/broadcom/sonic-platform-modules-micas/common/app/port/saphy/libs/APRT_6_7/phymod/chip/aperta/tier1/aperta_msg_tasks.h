/*
*
* $Id: aperta_cfg_seq.c,  $
*
*  *
*  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
*  *
*  *
*
*/
#ifndef APERTA_MSG_TASKS_H
#define APERTA_MSG_TASKS_H

#include <phymod/phymod_acc.h>
#include <tier1/aperta_sdk_intf.h>
#include <tier1/aperta_reg_access.h>

#define APERTA_FW_MSG_RETRY_CNT     100
#define APERTA_FW_PORT_OP_ENABLE    1
#define APERTA_FW_PORT_OP_DISABLE   2
#define APERTA_FW_PORT_OP_FLUSH     3
#define APERTA_FW_PORT_OP_PAUSE     4
#define APERTA_FW_PORT_OP_RESUME    5

/* Aperta SyncE defines */
#define APERTA_CLKGEN_DIS               0x0
#define APERTA_CLKGEN_ENA               0x1

#define APERTA_CLKGEN_CLKSIDE_SYS       0x0
#define APERTA_CLKGEN_CLKSIDE_LINE      0x1

#define APERTA_CLKGEN_RCLKNUM_0         0x0
#define APERTA_CLKGEN_RCLKNUM_1         0x1
#define APERTA_CLKGEN_RCLKNUM_2         0x2
#define APERTA_CLKGEN_RCLK_MAX          0x3

#define APERTA_CLKGEN_NO_SQUELCH        0x0
#define APERTA_CLKGEN_SQUELCH_LOS       0x1
#define APERTA_CLKGEN_SQUELCH_LOL       0x2

/* Dividers for Differential Clock Ports (APERTA_CLKGEN_RCLKNUM_0)*/
#define APERTA_CLKGEN_DIVIDER_32        0x0
#define APERTA_CLKGEN_DIVIDER_64        0x1
#define APERTA_CLKGEN_DIVIDER_128       0x2
#define APERTA_CLKGEN_DIVIDER_256       0x3
#define APERTA_CLKGEN_DIVIDER_512       0x4
#define APERTA_CLKGEN_DIVIDER_1024      0x5
#define APERTA_CLKGEN_DIVIDER_2048      0x6
#define APERTA_CLKGEN_DIVIDER_4096      0x7

/* Dividers for Single Ended Clock Ports (APERTA_CLKGEN_RCLKNUM_1/2)*/
#define APERTA_CLKGEN_DIVIDER_80        0x1
#define APERTA_CLKGEN_DIVIDER_120       0x2
#define APERTA_CLKGEN_DIVIDER_240       0x3
#define APERTA_CLKGEN_DIVIDER_520       0x4
#define APERTA_CLKGEN_DIVIDER_1000      0x5
#define APERTA_CLKGEN_DIVIDER_2040      0x6
#define APERTA_CLKGEN_DIVIDER_4080      0x7

/*!
 *  Structure that holds CONFIG_PHY message parameters
 */
typedef struct aperta_config_phy_s
{
    uint8_t  MACsecOpt;    /* MACsec Bypass Option*/
    uint8_t  IOOpt;        /* I/O Option*/
} aperta_config_phy_t;

/*!
 *  Structure that holds CLOCK_GEN message parameters
 */
typedef struct aperta_clock_gen_s
{
    uint8_t  RClkNum;      /*Recovered Clock Number*/
    uint8_t  ClkGenEn;     /*Clock Generation Enable*/
    uint8_t  ClkSide;      /*Recovered Clock Side*/
    uint8_t  ClkLane;      /*Recovered Clock Lane Number*/
    uint8_t  Divider;      /*Recovered Clock Divider*/
    uint8_t  SquelchMode;  /*Squelch Mode*/
    uint8_t  PortLanes;    /*Port Lanes bitmap that lists lanes that belong to the Port*/
} aperta_clock_gen_t;

/*!
 *  Structure that holds CONFIG_PORT message parameters
 */
typedef struct aperta_config_port_s
{
    uint8_t  PortNum;      /*Hardware Port Number*/
    uint8_t  PortType;     /*PortType (Repeater/Gearbox/Rev-Gearbox)*/
    uint8_t  PortMode;     /*PortMode (Regular/FailoverMUX)*/
    uint8_t  PortSpeed;    /*PortSpeed*/
    uint8_t  SPMPortID;    /*System-side Port Macro's Port ID*/
    uint8_t  LPMPortID;    /*Line-side Port Macro's Port ID*/
    uint8_t  PortOptions;  /*Port configuration options*/
    uint16_t IngFixedLatency; /*ingress Fixed-Latency Value*/
    uint16_t EgrFixedLatency; /*egress Fixed-Latency Value*/
    uint8_t  FOOptions;    /*Fail-over MUX Options*/
    uint8_t  FOPortNum;    /*Fail-over Hardware Port Number*/
    uint8_t  FOPortID;     /*Fail-over Port Macro Port ID*/
    uint16_t EgrptpFixedLatency;
} aperta_config_port_t;

/*!
 *  Structure that holds PTP_CONFIG message parameters
 */
typedef struct aperta_ptp_config_s
{
    uint8_t  PortNum;            /* Hardware Port Number*/
    uint8_t  PortOptions;        /* Port configuration options*/
    uint16_t IngFixedLatency;    /* Fixed-Latency Value (Ingress)*/
    uint16_t EgrFixedLatency;    /* Fixed-Latency Value (Egress)*/
    uint16_t EgrPTPFixedLatency; /* PTP Fixed-Latency Value (Egress)*/
} aperta_ptp_config_t;
int plp_aperta_msg_send (const plp_aperta_phymod_phy_access_t *phy,  uint8_t function, uint8_t operation,
                      uint8_t *tx_msg, uint8_t *rx_msg, uint8_t *result);

/* Package Polarity functions*/
int plp_aperta_fw_dft_polarity_set (const plp_aperta_phymod_phy_access_t *phy);

/* CONFIG_PHY functions*/
int plp_aperta_fw_config_phy_set(const plp_aperta_phymod_phy_access_t *phy,  aperta_config_phy_t *phy_cfg);
int plp_aperta_fw_config_phy_get(const plp_aperta_phymod_phy_access_t *phy,  aperta_config_phy_t *phy_cfg);
/* CONFIG_PORT functions*/
int
plp_aperta_fw_config_port_set (const plp_aperta_phymod_phy_access_t *phy,  aperta_config_port_t *port_cfg);
int
plp_aperta_fw_config_port_get (const plp_aperta_phymod_phy_access_t *phy,  aperta_config_port_t *port_cfg);

/* Port operation*/
int plp_aperta_fw_port_op_start (const plp_aperta_phymod_phy_access_t *phy,  uint8_t port_num, int operation);
int plp_aperta_fw_port_op_result (const plp_aperta_phymod_phy_access_t *phy,  uint8_t port_num, int operation);
int plp_aperta_fw_port_op (const plp_aperta_phymod_phy_access_t *phy,  uint8_t port_num, int operation);

/* CLOCK_GEN functions*/
int plp_aperta_fw_configure_clock_gen (const plp_aperta_phymod_phy_access_t *phy,  aperta_clock_gen_t *clk_cfg);
int plp_aperta_fw_clock_gen_read (const plp_aperta_phymod_phy_access_t *phy,  aperta_clock_gen_t *clk_cfg);

/* PM reg */
typedef struct aperta_pm_reg_s {
    uint8_t access_type;
    uint8_t padding; /* Added for padding structure*/
    int datalen;
    uint32_t addr;
    uint64_t *data;
} aperta_pm_reg_t;

typedef struct aperta_tsc_reg_s {
    uint32_t addr;
    uint32_t *data;
} aperta_tsc_reg_t;

typedef struct aperta_ind_reg_s {
    uint16_t addr;
    uint32_t *data;
} aperta_ind_reg_t;

typedef struct aperta_macsec_reg_s {
    uint8_t direction;
    uint8_t block;
    uint32_t addr;
    uint32_t *data;
} aperta_macsec_reg_t;

typedef struct aperta_ptp_sopmem_s {
    uint8_t port;
    uint8_t addr;
    uint16_t data[14];
}aperta_ptp_sopmem_t;

#define APERTA_PTP_TOD_TYPE_48BIT     0
#define APERTA_PTP_TOD_TYPE_80BIT     1
/*!
 *  Structure that holds PTP_TOD80 message parameters
 */
typedef struct aperta_ptp_tod_s {
    uint8_t  PortNum;        /* Hardware Port Number*/
    uint8_t  TOD_TS [10];    /* TOD - 80/48 bits (6/10 bytes) in little-endian format*/
    uint8_t  TOD_DPLL [10];  /* TOD - 80/48 bits (6/10 bytes) in little-endian format*/
} aperta_ptp_tod_t;


#define APERTA_PHYMOD_FW_MEMTYPE(PM_MT,FW_MT)              \
    if (PM_MT == phymodMemSpeedIdTable) {                  \
        FW_MT = 2;                                         \
    } else if (PM_MT == phymodMemAMTable) {                \
        FW_MT = 3;                                         \
    } else if (PM_MT == phymodMemUMTable) {                \
        FW_MT = 4;                                         \
    } else if (PM_MT == phymodMemTxLkup1588Mpp0) {         \
        FW_MT = 5;                                         \
    } else if (PM_MT == phymodMemTxLkup1588Mpp1) {         \
        FW_MT = 6;                                         \
    } else if (PM_MT == phymodMemTxLkup1588400G) {         \
        FW_MT = 7;                                         \
    } else if (PM_MT == phymodMemRxLkup1588Mpp0) {         \
        FW_MT = 8;                                         \
    } else if (PM_MT == phymodMemRxLkup1588Mpp1) {         \
        FW_MT = 9;                                         \
    } else if (PM_MT == phymodMemRxLkup1588400G) {         \
        FW_MT = 0xA;                                       \
    } else if (PM_MT == phymodMemSpeedPriorityMapTable) {  \
        FW_MT = 0xB;                                       \
    } else if ( PM_MT == phymodMemPmMib) {                 \
        FW_MT = 0x0;                                       \
    } else {                                               \
        FW_MT = 1;                                         \
    }

int plp_aperta_fw_write_register (const plp_aperta_phymod_phy_access_t *phy, int block, void *data);
int plp_aperta_fw_read_register (const plp_aperta_phymod_phy_access_t *phy, int block, void *data);
int plp_aperta_fw_reg_access(const plp_aperta_phymod_phy_access_t* phy, uint16_t port_sel, uint16_t lane_sel, 
                        uint32_t reg_addr, uint64_t* data, int write_en, int pcs_pmd_addr);
int plp_aperta_fw_mem_access(const plp_aperta_phymod_phy_access_t* phy, APERTA_PM_MEM_ACCESS_T* mem_access);
int plp_aperta_nse_ptp_sopmem_read (const plp_aperta_phymod_phy_access_t *phy, aperta_ptp_sopmem_t *ptp_sopmem);
int plp_aperta_fw_switch_mux (const plp_aperta_phymod_phy_access_t* phy, int port_number);
int plp_aperta_fw_config_ptp_write (const plp_aperta_phymod_phy_access_t* phy, aperta_ptp_config_t *ptp_cfg);
int plp_aperta_fw_config_ptp_read (const plp_aperta_phymod_phy_access_t* phy, aperta_ptp_config_t *ptp_cfg);
int plp_aperta_fw_tod_config(const plp_aperta_phymod_phy_access_t* phy, int tod_type, aperta_ptp_tod_t *ptp_tod);

/* FW MSG Helper functions */
void plp_aperta_put_byte (uint8_t **ptr, uint8_t value);
uint8_t plp_aperta_get_byte (uint8_t **ptr);
void plp_aperta_put_half_word (uint8_t **ptr, uint16_t value);
uint16_t plp_aperta_get_half_word (uint8_t **ptr);
void plp_aperta_put_word (uint8_t **ptr, uint32_t value);

#endif /*APERTA_MSG_TASKS_H*/
