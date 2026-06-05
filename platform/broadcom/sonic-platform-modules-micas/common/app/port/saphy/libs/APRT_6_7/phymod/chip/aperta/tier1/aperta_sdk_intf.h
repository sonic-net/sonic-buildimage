/*
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
 */

/**
 *  @file aperta_sdk_intf.h - Constants and declaration shared by Aperta
 *                            firmware and Aperta SDK.
 */

#ifndef APERTA_SDK_INTF_H_
#define APERTA_SDK_INTF_H_

#include <phymod/phymod_acc.h>
#include <tier1/bcmi_aperta_d_defs.h>


#define MAX_NUM_PORTS             (8)

/*! Maximum number of Lanes*/
#define MAX_NUM_LANES             (8)

/* Number of sides*/
#define NUM_SIDES                 (2)

#define NUM_DIRS                  (2)

/*!
 * @enum E_PM_ACCESSTYPE_SEL
 * Constants to select Port Macro AccessType selection in some functions
 */
enum E_PM_ACCESSTYPE_SEL
{
    PM_ATYPE_PORT_GENREG    = 0x0,  /* select PORT General Registers     */
    PM_ATYPE_CDMAC0_GENREG  = 0x1,  /* select CDMAC0 General Registers      */
    PM_ATYPE_CDMAC1_GENREG  = 0x2,  /* select CDMAC1 General Registers      */
    PM_ATYPE_PORT_LANEREG   = 0x8,  /* select PORT Lane-based Registers      */
    PM_ATYPE_CDMAC0_LANEREG = 0x9,  /* select CDMAC0 Lane-based Registers*/
    PM_ATYPE_CDMAC1_LANEREG = 0xA,  /* select CDMAC0 Lane-based Registers*/
};


/*!
 * @enum E_PORT_TYPE
 * Constants that define Port Type
 */
enum E_PORT_TYPE
{
    REPEATER    = 0,  /* Port Type - Repeater*/
    GEARBOX     = 1,  /* Port Type - Gearbox*/
    REV_GEARBOX = 2,  /* Port Type - Reverse-Gearbox*/
};


/*!
 * @enum E_PORT_MODE
 * Constants that define Port Mode (Regular / Failover MUX)
 */
enum E_PORT_MODE
{
    REGULAR      = 0,   /* Port Mode - Regular (Repeater/Gearbox/Reverse-Gearbox) */
    FAILOVER_MUX = 1,   /* Port Mode - Failover MUX Mode*/
};


/*!
 * @enum E_PORT_SPEED
 * Constants that define Line-side Port Speed
 */
enum E_PORT_SPEED
{
    SPEED_10G       = 0,   /* Port Speed : 10G          */
    SPEED_25G       = 1,   /* Port Speed : 25G          */
    SPEED_40G       = 2,   /* Port Speed : 40G          */
    SPEED_50G_NRZ   = 3,   /* Port Speed : 50G NRZ     */
    SPEED_50G_PAM4  = 4,   /* Port Speed : 50G PAM4     */
    SPEED_100G_NRZ  = 5,   /* Port Speed : 100G NRZ     */
    SPEED_100G_PAM4 = 6,   /* Port Speed : 100G PAM4*/
    SPEED_200G_PAM4 = 7,   /* Port Speed : 200G PAM4*/
    SPEED_400G_PAM4 = 8,   /* Port Speed : 400G PAM4*/
};


/*!
 * @enum E_FAILOVER_PORT_SEL
 * Constants that define Failover Port Select (Primary / Secondary)
 */
enum E_FAILOVER_PORT_SEL
{
    PRIMARY_PORT   = 0,   /* Failover Port - Primary Port     */
    SECONDARY_PORT = 1,   /* Failover Port - Secondary Port */
};


/*!
 * @enum E_PTP_OPTION_SEL
 * Constants that define PTP Options
 */
enum E_PTP_OPTION_SEL
{
    PTP_STBYP         = 0,   /* PTP disabled, static bypass.                                             */
    PTP_INTBYP         = 1,   /* PTP disable, internal/legacy bypass.                                    */
    PTP_LEGACY        = 2,   /* PTP enabled, NSE-TS for Egr & Ing, packets encrypted.               */
    PTP_LEGACY_LVC = 3,       /* PTP enabled, NSE-TS for Egr/Ing, for Ing@PM, packets encrypted.*/
    PTP_HP             = 4,   /* PTP enabled, NSE-TS for Egr, PM-TS for Ing, packets encrypted.     */
    PTP_PM             = 5,   /* PTP enabled, PM-TS for Egr & Ing, packets not encrypted.       */
};


/*!
 * @enum E_CLKGEN_DIV_SEL
 * Constants to select Divider for Output Clock Generation
 */
enum E_CLKGEN_DIV_SEL
{
    CLKGEN_DIV_20    = 0,  /* Line Rate % 20          */
    CLKGEN_DIV_40    = 1,  /* Line Rate % 40          */
    CLKGEN_DIV_80    = 2,  /* Line Rate % 80          */
    CLKGEN_DIV_160   = 3,  /* Line Rate % 160          */
    CLKGEN_DIV_400   = 4,  /* Line Rate % 400          */
    CLKGEN_DIV_1000  = 5,  /* Line Rate % 1000          */
    CLKGEN_DIV_UNDEF = 6,  /* Line Rate % Undefined */
};


/*!
 * @enum E_CLKGEN_SQUELCH_MODE_SEL
 * Constants to select Squelch Mode for Output Clock Generation
 */
enum E_CLKGEN_SQUELCH_MODE_SEL
{
    CLKGEN_SQUELCH_NONE      = 0,  /* No Squelch requested                       */
    CLKGEN_SQUELCH_LOS       = 1,  /* Squelch Clock on Loss of Signal (LOS)*/
    CLKGEN_SQUELCH_LOL       = 2,  /* Squelch Clock on Loss of Lock (LOL)  */
    CLKGEN_SQUELCH_LINK_DOWN = 3,  /* Squelch Clock on Link Down             */
};


/*!
 * @enum E_PORT_INUSE
 * Constants that define Port InUse Status
 */
enum E_PORT_INUSE
{
    PORT_NOT_INUSE     = 0,   /* Port Not in use                          */
    PORT_REGULAR       = 1,   /* Port in use for Regualr Port           */
    PORT_FO_PRIMARY    = 2,   /* Port in use for Failover (Primary)      */
    PORT_FO_SECONDARY  = 3,   /* Port in use for Failover (Secondary)*/
};



/* Message Interface related defines*/


/*!
 * MSG_msgin_TYPE MSGIN register fields
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t status     : 4;   /*  MSG_msgin_TYPE[03:00]*/
    uint16_t operation  : 4;   /*  MSG_msgin_TYPE[07:04]*/
    uint16_t function   : 8;   /*  MSG_msgin_TYPE[15:08]*/
  } fields;
} MSG_msgin_TYPE;


/*!
 * MSG_msgout_TYPE MSGOUT register fields
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t status     : 4;   /*  MSG_msgout_TYPE[03:00]*/
    uint16_t operation  : 4;   /*  MSG_msgout_TYPE[07:04]*/
    uint16_t function   : 8;   /*  MSG_msgout_TYPE[15:08]*/
  } fields;
} MSG_msgout_TYPE;


/*!
 * @enum E_MSG_STATUS
 * Constants to select Message status in the Message Interface module
 */
enum E_MSG_STATUS
{
    STS_IDLE   = 0x0,   /* Message status idle                    */
    STS_SENT   = 0x1,   /* Message status sent                    */
    STS_RECD   = 0x2,   /* Message status received               */
    STS_PROCD  = 0x3,   /* Message status processed               */
    STS_RESET  = 0xF    /* Message status reset (chip reset)*/
};


/*!
 * @enum E_MSG_OPERATION
 * Constants to select Message operation in the Message Interface module
 */
enum E_MSG_OPERATION
{
    APERTA_OP_WRITE      = 0x0,      /* write operation (one/more consecutive entries)        */
    APERTA_OP_READ       = 0x1,      /* read  operation (one/more consecutive entries)        */
    APERTA_OP_WRITE_EXT  = 0x2,      /* write extended  (one/more non-consecutive entries)*/
    APERTA_OP_READ_EXT   = 0x3,      /* read  extended  (one/more non-consecutive entries)*/
    APERTA_OP_START        = 0x0,    /* start a firmware process (state machine)             */
    APERTA_OP_START_RESULT = 0x1,    /* get status a started firmware process                  */
    APERTA_OP_PROCESSING = 0xD,      /* operation in progress (processing)                       */
    APERTA_OP_SUCCESS    = 0xE,      /* operation result: success                                 */
    APERTA_OP_ERROR      = 0xF       /* operation result: error                                 */
};


/*!
 * @enum E_MSG_FUNCTION
 * Constants to select Message function in the Message Interface module
 */
enum E_MSG_FUNCTION
{

    APERTA_FUNC_PM_REGS         = 0x10,  /* PM_REGS (Port Macro registers)                         */
    APERTA_FUNC_TSC_REGS        = 0x11,  /* TSC_REGS (TSC registers)                              */
    APERTA_FUNC_MACSEC_REGS     = 0x12,  /* MACSEC_REGS (MACSEC registers)                         */
    APERTA_FUNC_CHIP_IND_REGS   = 0x13,  /* CHIP_IND_REGS (indirect chip registers)          */
    APERTA_FUNC_MACSEC_KEY      = 0x14,  /* MACSEC_KEY (MACSEC key)                              */
    APERTA_FUNC_PKG_CFG         = 0x15,  /* PKG_CFG (Lane map and polarity swap config)     */
    APERTA_FUNC_CLOCK_GEN       = 0x16,  /* CLOCK_GEN (Clock Generation and Squelch)          */
    APERTA_FUNC_CONFIG_PHY      = 0x17,  /* CONFIG_PHY (Configure phy-level operating mode)*/
    APERTA_FUNC_CONFIG_PORT     = 0x18,  /* CONFIG_PORT (Configure a Port)                */
    APERTA_FUNC_PAUSE_PORT      = 0x19,  /* PAUSE_PORT (Pause a Port)                     */
    APERTA_FUNC_RESUME_PORT     = 0x1A,  /* RESUME_PORT (Resume a Port)                */
    APERTA_FUNC_PTP_SOPMEM      = 0x1C,  /* PTP_SOPMEM (PTP SOP Memory read) */
    APERTA_FUNC_PTP_TOD48       = 0x1D,  /* PTP_TOD48 (PTP TOD48 write) */
    APERTA_FUNC_PTP_CONFIG      = 0x1E,  /* PTP_CONFIG  */
    APERTA_FUNC_PTP_TOD80       = 0x1F,  /* PTP_TOD80 (PTP TOD80 write) */
    APERTA_FUNC_ENABLE_PORT     = 0x20,  /* ENABLE_PORT (Enable a Port)                */
    APERTA_FUNC_DISABLE_PORT    = 0x21,  /* DISABLE_PORT (Disable a Port)                */
    APERTA_FUNC_FLUSH_PORT      = 0x22,  /* FLUSH_PORT (Flush a Port)                     */
    APERTA_FUNC_MACSEC_KEY_CLEAR  = 0x23,/* MACSEC_KEY_CLEAR (Clear all MACSEC keys)*/
    APERTA_FUNC_SWITCH_MUX      = 0x26, 
    APERTA_FUNC_MACSEC_INIT_EIP164 = 0x27,/* MACSEC_EIP164_INIT (Initialize SecY/EIP164 Device)*/
    APERTA_FUNC_RESERVED        = 0xFF   /* Reserved*/
};


/*!
 * @enum E_MSG_ERROR
 * Constants for error codes returned in the Message Interface module
 */
enum E_MSG_ERROR
{
    ERR_FUNC_NOTAVAILABLE  = 0x10,  /* Function not available      */
    ERR_OPER_NOTAVAILABLE  = 0x11,  /* Operation not available      */
    ERR_INCORRECT_LENGTH   = 0x12,  /* Incorrect length                */
    ERR_BUSY               = 0x13,  /* HW/FW is busy                */
    ERR_INCORRECT_OPERAND  = 0x14,  /* Incorrect operand           */
    ERR_FUNC_SPECIFIC      = 0x1F,  /* Function specific           */
    ERR_UNKNOWN            = 0xFF   /* Error Unknown / Unexpected*/
};

#define MSG_IN_BUFFER_ADDR    BCMI_APERTA_D_FWS_FWREG_000r   /* MSG buffer for incoming messages*/
#define MSG_IN_BUFFER_SIZE    0x0100                         /* in bytes (128 16-bit registers)     */
#define MSG_OUT_BUFFER_ADDR   BCMI_APERTA_D_FWS_FWREG_080r   /* MSG buffer for outgoing messages*/
#define MSG_OUT_BUFFER_SIZE   0x0100                         /* in bytes (128 16-bit registers)*/

/*Port Configuration Registers (fwreg_100 - fwreg_17F)*/
#define PORT_CONFIG_REG_START_ADDR     BCMI_APERTA_D_FWS_FWREG_100r
#define PORT_CONFIG_REG_END_ADDR       BCMI_APERTA_D_FWS_FWREG_17Fr

/* RCLK Configuration Registers (fwreg_180 - fwreg_18F)*/
#define RCLK_CONFIG_REG_START_ADDR     BCMI_APERTA_D_FWS_FWREG_180r
#define RCLK_CONFIG_REG_END_ADDR       BCMI_APERTA_D_FWS_FWREG_18Fr

/* PBIST Configuration Registers (fwreg_190 - fwreg_20F)*/
#define PBIST_CONFIG_REG_START_ADDR    BCMI_APERTA_D_FWS_FWREG_190r
#define PBIST_CONFIG_REG_END_ADDR      BCMI_APERTA_D_FWS_FWREG_20Fr

/* Available Registers (fwreg_210 - fwreg_2FF)*/
#define AVAILABLE_REG_START_ADDR       BCMI_APERTA_D_FWS_FWREG_210r
#define AVAILABLE_REG_END_ADDR         BCMI_APERTA_D_FWS_FWREG_2FFr

/* ATE Firmware Registers (fwreg_300 - fwreg_3EF)*/
#define ATE_FW_REG_START_ADDR          BCMI_APERTA_D_FWS_FWREG_300r
#define ATE_FW_REG_END_ADDR            BCMI_APERTA_D_FWS_FWREG_3EFr

/* Debug Registers (fwreg_3F0 - fwreg_3FF)*/
#define DIAGNOSTIC_REG_START_ADDR      BCMI_APERTA_D_FWS_FWREG_3F0r
#define DIAGNOSTIC_REG_END_ADDR        BCMI_APERTA_D_FWS_FWREG_3FFr

/* LMI error logging*/
#define GPREG_lmi_rd_error_Adr         BCMI_APERTA_D_FWS_FWREG_3F0r
#define GPREG_lmi_xact_error_Adr       BCMI_APERTA_D_FWS_FWREG_3F1r
#define GPREG_lmi_debug_Adr            BCMI_APERTA_D_FWS_FWREG_3F2r

#define FWREG_init_time_lsw_Adr        BCMI_APERTA_D_FWS_FWREG_3FAr
#define FWREG_init_time_msw_Adr        BCMI_APERTA_D_FWS_FWREG_3FBr

#define FWREG_min_loop_time_lsw_Adr    BCMI_APERTA_D_FWS_FWREG_3FCr
#define FWREG_min_loop_time_msw_Adr    BCMI_APERTA_D_FWS_FWREG_3FDr

#define FWREG_max_loop_time_lsw_Adr    BCMI_APERTA_D_FWS_FWREG_3FEr
#define FWREG_max_loop_time_msw_Adr    BCMI_APERTA_D_FWS_FWREG_3FFr

/*------------------------------------------------------------------------------
 General Purpose Registers

 Firmware registers for control, status and error fields.
------------------------------------------------------------------------------*/
#define GPREG_fw_control_Adr             BCMI_APERTA_D_GEN_CNTRLS_GPREG_00r
#define GPREG_fw_status_Adr              BCMI_APERTA_D_GEN_CNTRLS_GPREG_01r
#define GPREG_fw_error_Adr               BCMI_APERTA_D_GEN_CNTRLS_GPREG_02r
#define GPREG_fw_active_Adr              BCMI_APERTA_D_GEN_CNTRLS_GPREG_03r
#define GPREG_fw_wdog_min_count_Adr      BCMI_APERTA_D_GEN_CNTRLS_GPREG_04r
#define GPREG_fw_fatal_error_status_Adr  BCMI_APERTA_D_GEN_CNTRLS_GPREG_05r
#define GPREG_fw_port_inuse_status_Adr   BCMI_APERTA_D_GEN_CNTRLS_GPREG_06r

/* Boot Loader Use Only (used to skip the PLL lock check during boot process)*/
#define GPREG_bl_skip_pll_lock_Adr       BCMI_APERTA_D_GEN_CNTRLS_GPREG_1Fr

/*!
 * GPREG_fw_control_TYPE
 *
 * Firmware Control Register that holds all top-level control bits which will
 * will change the firmware behavior. This register is implemented using
 * GEN_CNTRLS_gpreg_00.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  fw_disable  : 1;         /*  disable firmware (idle loop)       */
    uint16_t  reserved    : 14;        /*  Reserved bits                      */
    uint16_t  fw_init_disable  : 1;    /*  disable firmware initialization*/
  } fields;
} GPREG_fw_control_TYPE;

/*!
 * GPREG_fw_status_TYPE
 *
 * Firmware Status Register that holds all top-level status bits. This register
 * is implemented using GEN_CNTRLS_gpreg_01.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  fw_init_done    : 1;   /*  Firmware initialization done                       */
    uint16_t  rsvd            : 6;   /*  Reserved bits                                           */
    uint16_t  gpio_mode       : 1;   /*  GPIO Mode (0: Low Voltage; 1: 3.3V compatible)*/
    uint16_t  sys_bh_active   : 4;   /*  Sys-side Blackhawk micro_uc_active[3:0] */
    uint16_t  line_bh_active  : 4;   /*  Line-side Blackhawk micro_uc_active[3:0]*/
  } fields;
} GPREG_fw_status_TYPE;

/*!
 * GPREG_fw_error_TYPE
 *
 * Firmware Error Register that holds all top-level status bits. This register
 * is implemented using GEN_CNTRLS_gpreg_02.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  lmi_xact_error  : 1;   /*  LMI transaction error*/
    uint16_t  lmi_rd_error    : 1;   /*  LMI read error - RD FIFO !empty before a xact*/
    uint16_t  msg_intf_error  : 1;   /*  Message Interface error                           */
    uint16_t  fatal_error     : 1;   /*  Fatal Error (unexpected interrupt received)  */
    uint16_t  wdt_expired     : 1;   /*  Watchdog timer expired                                */
    uint16_t  macsec_mem_error: 1;   /*  MACsec memory power up error                      */
    uint16_t  fc_mem_error    : 1;   /*  FC memory power up error                           */
    uint16_t  pmid_lock_error : 1;   /*  PM Port ID lock error                                */
    uint16_t  reserved        : 8;   /*  Reserved bits                                          */
  } fields;
} GPREG_fw_error_TYPE;

/*!
 * GPREG_fw_active_TYPE
 *
 * Firmware Active Status Register that is set to 0x5555 by firmware in the background
 * loop. This register is implemented using GEN_CNTRLS_gpreg_03.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  active_val  : 16;   /*  Firmware active status*/
  } fields;
} GPREG_fw_active_TYPE;

/*!
 * GPREG_fw_wdog_min_count_TYPE
 *
 * Firmware Watchdog Minimum Count (Live) Register that is set by firmware with the
 * live watchdog count before resetting the watchdog timer. This register is
 * implemented using GEN_CNTRLS_gpreg_04.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  wdog_min_cnt  : 16;   /* Watchdog minimum count*/
  } fields;
} GPREG_fw_wdog_min_count_TYPE;

/*!
 * GPREG_fw_fatal_error_status_TYPE
 *
 * Firmware Fatal Error Status Register that implements fatal errors seen by firmware
 * like unexpected interrupts. This register is implemented using GEN_CNTRLS_gpreg_05.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  NMI_err         : 1;   /*  NMI received error*/
    uint16_t  HardFault_err   : 1;   /*  Hard Fault received error*/
    uint16_t  SVC_err         : 1;   /*  SVC received error            */
    uint16_t  PendSV_err      : 1;   /*  PendSV received error       */
    uint16_t  SysTick_err     : 1;   /*  PendSV received error       */
    uint16_t  IRQ00_err       : 1;   /*  IRQ00 received error       */
    uint16_t  IRQ01_err       : 1;   /*  IRQ01 received error       */
    uint16_t  IRQ12_err       : 1;   /*  IRQ12 received error       */
    uint16_t  rsvd            : 7;   /*  Reserved bits                 */
    uint16_t  DefaultIRQ_err  : 1;   /*  Default IRQ error            */
  } fields;
} GPREG_fw_fatal_error_status_TYPE;

/*!
 * GPREG_fw_port_inuse_status_TYPE
 *
 * Firmware Status Register that holds port in-use status. This register is
 * implemented using GEN_CNTRLS_gpreg_06.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  port0_sts : 2;   /*  Port 0 in use status*/
    uint16_t  port1_sts : 2;   /*  Port 1 in use status*/
    uint16_t  port2_sts : 2;   /*  Port 2 in use status*/
    uint16_t  port3_sts : 2;   /*  Port 3 in use status*/
    uint16_t  port4_sts : 2;   /*  Port 4 in use status*/
    uint16_t  port5_sts : 2;   /*  Port 5 in use status*/
    uint16_t  port6_sts : 2;   /*  Port 6 in use status*/
    uint16_t  port7_sts : 2;   /*  Port 7 in use status*/
  } fields;
} GPREG_fw_port_inuse_status_TYPE;

#endif
