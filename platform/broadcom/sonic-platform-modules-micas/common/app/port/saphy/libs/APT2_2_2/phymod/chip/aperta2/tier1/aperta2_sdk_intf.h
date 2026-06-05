/*******************************************************************************
 *
 *  Broadcom Confidential/Proprietary.
 *  Copyright (C) 2022 Broadcom Inc. All rights reserved.
 *
 ******************************************************************************/

/**
 *  @file aperta2_sdk_intf.h - Constants and declaration shared by Aperta
 *                            firmware and Aperta SDK.
 */

#ifndef APERTA2_SDK_INTF_H_
#define APERTA2_SDK_INTF_H_

/*******************************************************************************
 *  Include files
 ******************************************************************************/

/*******************************************************************************
 *  Definitions
 ******************************************************************************/

/*! Maximum number of Slaves (Port Macro instances) */
#define APERTA2_MAX_NUM_SLAVES            (4)

/*! Maximum number of Ports */
#define APERTA2_MAX_NUM_PORTS             (16)

/*! Maximum number of Lanes */
#define APERTA2_MAX_NUM_LANES             (16)

/*! Number of Ports per octal */
#define APERTA2_NUM_PORTS_PER_OCTAL       (8)

/*! Number of Lanes per octal */
#define APERTA2_NUM_LANES_PER_OCTAL       (8)

/*! Number of sides */
#define APERTA2_NUM_SIDES                 (2)

/*! Number of Octals */
#define APERTA2_NUM_OCTALS                (2)

/*! Number of directions */
#define APERTA2_NUM_DIRS                  (2)

/*!
 * @enum E_SIDE_SEL
 * Constants to select Side (SYS/LINE)
 */
typedef enum APERTA2_SIDE_SEL_E {
    APERTA2_SIDE_SYS  = 0x00,   /*!< select System side */
    APERTA2_SIDE_LINE = 0x01,   /*!< select Line side   */
    APERTA2_SIDE_BOTH = 0x02   /*!< select Both sides (System and Line) */
} APERTA2_SIDE_SEL_T;


/*!
 * @enum E_PM_SEL
 * Constants to select PM slave
 */
typedef enum APERTA2_PM_SEL_E {
    APERTA2_PM_NONE          = 0x00,  /* select no PM*/
    APERTA2_PM_SYS_OCTAL0    = 0x01,  /* select System Side Octal0 PM*/
    APERTA2_PM_SYS_OCTAL1    = 0x02,  /* select System Side Octal1 PM*/
    APERTA2_PM_LINE_OCTAL0   = 0x04,  /* select Line Side Octal0 PM*/
    APERTA2_PM_LINE_OCTAL1   = 0x08,  /* select Line Side Octal1 PM*/
    APERTA2_PM_BROADCAST     = 0x0F  /* select Port Macro Broadcast*/
} APERTA2_PM_SEL_T;


/*!
 * @enum E_OCTAL_SEL
 * Constants to select Octal
 */
typedef enum APERTA2_OCTAL_SEL_E {
    APERTA2_OCTAL_NONE  = 0x00,   /*!< select Octal None */
    APERTA2_OCTAL0      = 0x01,   /*!< select Octal 0 */
    APERTA2_OCTAL1      = 0x02,   /*!< select Octal 1 */
    APERTA2_OCTAL_BOTH  = 0x03   /*!< select Octal Both */
} APERTA2_OCTAL_SEL_T;


/*!
 * @enum APERTA2_DIR_SEL
 * Constants to select Direction (INGRESS/EGRESS)
 */
typedef enum APERTA2_DIR_SEL_E {
    APERTA2_INGRESS = 0x00,   /*!< select Ingress direction */
    APERTA2_EGRESS  = 0x01   /*!< select Egress direction  */
} APERTA2_DIR_SEL_T;


/*!
 * @enum APERTA2_RXTX_SEL
 * Constants to select Rx/Tx (RX/TX)
 */
typedef enum APERTA2_RXTX_SEL_E {
    APERTA2_RX = 0x00,   /*!< select RX direction */
    APERTA2_TX = 0x01   /*!< select TX direction */
} APERTA2_RXTX_SEL_T;


/*!
 * @enum APERTA2_MACSEC_EIP_SEL
 * Constants to select Evora MacSec EIP block selection in some functions
 */
typedef enum APERTA2_MACSEC_EIP_SEL_E {
    APERTA2_MACSEC_EIP163 = 0,  /*!< select MacSec EIP block EIP-163 */
    APERTA2_MACSEC_EIP164 = 1,  /*!< select MacSec EIP block EIP-164 */
    APERTA2_MACSEC_EIP218 = 2  /*!< select MacSec EIP block EIP-218 */
} APERTA2_MACSEC_EIP_SEL_E;


/*!
 * @enum APERTA2_PM_ACCESSTYPE_SEL
 * Constants to select Port Macro AccessType selection in some functions
 */
typedef enum APERTA2_PM_ACCESSTYPE_SEL_E {
    APERTA2_PM_ATYPE_DC3PORT_GENREG   = 0x0,  /*!< select DC3PORT General Registers */
    APERTA2_PM_ATYPE_DC3MAC_GENREG    = 0x1,  /*!< select DC3MAC General Registers */
    APERTA2_PM_ATYPE_DC3PORT_LANEREG  = 0x4,  /*!< select PORT Lane-based Registers */
    APERTA2_PM_ATYPE_DC3MAC_LANEREG   = 0x5,  /*!< select DC3MAC Lane-based Registers */
    APERTA2_PM_ATYPE_TSC_MEMORY       = 0x7  /*!< select TSC Memory */
}APERTA2_PM_ACCESSTYPE_SEL_T;

/*!
 * @enum APERTA2_PM_MEMTYPE_SEL
 * Constants to select Port Macro MemoryType selection in some functions
 */
typedef enum APERTA2_PM_MEMTYPE_SEL {
    APERTA2_PM_MEM_MIB                  = 0x00,  /*!< select MIB Memory */
    APERTA2_PM_MEM_TSC_UCODE            = 0x01,  /*!< select TSC uCode Memory */
    APERTA2_PM_MEM_SPEED_ID_TABLE       = 0x02,  /*!< select Speed ID Table */
    APERTA2_PM_MEM_AM_TABLE             = 0x03,  /*!< select AM Table */
    APERTA2_PM_MEM_UM_TABLE             = 0x04,  /*!< select UM Table */
    APERTA2_PM_MEM_TX_LKUP1588_MPP0     = 0x05,  /*!< select TX LKUP 1588 MPP0 Memory */
    APERTA2_PM_MEM_TX_LKUP1588_MPP1     = 0x06,  /*!< select TX LKUP 1588 MPP1 Memory */
    APERTA2_PM_MEM_TX_LKUP1588_100G     = 0x07,  /*!< select TX LKUP 1588 100G Memory */
    APERTA2_PM_MEM_RX_LKUP1588_MPP0     = 0x08,  /*!< select RX LKUP 1588 MPP0 Memory */
    APERTA2_PM_MEM_RX_LKUP1588_MPP1     = 0x09,  /*!< select RX LKUP 1588 MPP1 Memory */
    APERTA2_PM_MEM_RSVD_A               = 0x0A,  /*!< Reserved */
    APERTA2_PM_MEM_SPEED_PRIOMAP_TABLE  = 0x0B,  /*!< select Speed Priority Map Table */
    APERTA2_PM_MEM_TX_TWOSTEP_1588_TS   = 0x0C,  /*!< select TX Two Step 1588 TS Memory */
    APERTA2_PM_MEM_FDR_MEMORY           = 0x0D,  /*!< select FDR Memory */
    APERTA2_PM_MEM_RSVD_E               = 0x0E,  /*!< Reserved */
    APERTA2_PM_MEM_RSFEC_SYMBERR_MIB    = 0x0F,  /*!< select RSFEC Symbol Error MIB Memory */
    APERTA2_PM_MEM_DC3PORT_INTR_MASK    = 0x10,  /*!< select DC3PORT Interrupt Mask */
    APERTA2_PM_MEM_DC3PORT_INTR_STATUS  = 0x11,  /*!< select DC3PORT Interrupt Status */
    APERTA2_PM_MEM_DC3PORT_INTR_SELECT  = 0x12  /*!< select DC3PORT Interrupt Select */
}APERTA2_PM_MEMTYPE_SEL_T;

/*!
 * @enum APERTA2_PORT_TYPE
 * Constants that define Port Type
 */
typedef enum APERTA2_PORT_TYPE_E {
    APERTA2_REPEATER    = 0,  /*!< Port Type - Repeater */
    APERTA2_GEARBOX     = 1,  /*!< Port Type - Gearbox  */
    APERTA2_REV_GEARBOX = 2  /*!< Port Type - Reverse-Gearbox */
}APERTA2_PORT_TYPE_T;


/*!
 * @enum APERTA2_PORT_MODE
 * Constants that define Port Mode (Regular / Failover MUX)
 */
typedef enum APERTA2_PORT_MODE_E
{
    APERTA2_REGULAR      = 0,   /*!< Port Mode - Regular (Repeater/Gearbox/Reverse-Gearbox) */
    APERTA2_FAILOVER_MUX = 1   /*!< Port Mode - Failover MUX Mode */
}APERTA2_PORT_MODE_T;


/*!
 * @enum APERTA2_PORT_SPEED
 * Constants that define Line-side Port Speed
 */
typedef enum APERTA2_PORT_SPEED_E {
    APERTA2_SPEED_10G_1x10G    = 0,    /*!< Port Speed : 10G 1x10G NRZ */
    APERTA2_SPEED_25G_1x25G    = 1,    /*!< Port Speed : 25G 1x25G NRZ */
    APERTA2_SPEED_50G_2x25G    = 2,    /*!< Port Speed : 50G 2x25G NRZ */
    APERTA2_SPEED_50G_1x50G    = 3,    /*!< Port Speed : 50G 1x50G PAM4 */
    APERTA2_SPEED_100G_4x25G   = 4,    /*!< Port Speed : 100G 4x25G NRZ */
    APERTA2_SPEED_100G_2x50G   = 5,    /*!< Port Speed : 100G 2x50G PAM4 */
    APERTA2_SPEED_100G_1x100G  = 6,    /*!< Port Speed : 100G 1x100G PAM4 */
    APERTA2_SPEED_200G_4x50G   = 7,    /*!< Port Speed : 200G 4x50G PAM4 */
    APERTA2_SPEED_200G_2x100G  = 8,    /*!< Port Speed : 200G 2x100G PAM4 */
    APERTA2_SPEED_400G_8x50G   = 9,    /*!< Port Speed : 400G 8x50G PAM4 */
    APERTA2_SPEED_400G_4x100G  = 10,   /*!< Port Speed : 400G 4x100G PAM4 */
    APERTA2_SPEED_800G_8x100G  = 11   /*!< Port Speed : 800G 8x100G PAM4 */
}APERTA2_PORT_SPEED_T;


/*!
 * @enum APERTA2_FAILOVER_PORT_SEL
 * Constants that define Failover Port Select (Primary / Secondary)
 */
typedef enum APERTA2_FAILOVER_PORT_SEL_E {
    APERTA2_PRIMARY_PORT   = 0,   /*!< Failover Port - Primary Port */
    APERTA2_SECONDARY_PORT = 1   /*!< Failover Port - Secondary Port */
}APERTA2_FAILOVER_PORT_SEL_T;


/*!
 * @enum APERTA2_OCTAL_CROSS_SEL
 * Constants that define Octal Cross Side
 */
enum APERTA2_OCTAL_CROSS_SEL {
    APERTA2_OCTAL_CROSS_NONE = 0,   /*!< No Octal Crossing */
    APERTA2_OCTAL_CROSS_SYS  = 1,   /*!< Octal Crossing on System-side */
    APERTA2_OCTAL_CROSS_LINE = 2   /*!< Octal Crossing on Line-side */
};


/*!
 * @enum APERTA2_PTP_OPTION_SEL
 * Constants that define PTP Options
 */
enum APERTA2_PTP_OPTION_SEL {
    APERTA2_PTP_STBYP      = 0,   /*! PTP disabled, static bypass. */
    APERTA2_PTP_NSE_NSE    = 1,   /*! PTP enabled, NSE-TS for Egr & Ing (packets encrypted) */
    APERTA2_PTP_NSE_PM     = 2,   /*! PTP enabled, NSE-TS for Egr, PM-TS for Ing (packets encrypted) */
    APERTA2_PTP_PM_PM      = 3,   /*! PTP enabled, PM-TS for Egr & Ing (packets not encrypted) */
    APERTA2_PTP_PM_NSE     = 4,   /*! PTP enabled, PM-TS for Egr & NSE-TS for Ing (packets not encrypted) */
    APERTA2_PTP_HYB_NSE    = 5,   /*! PTP enabled, Hybrid for Egr & NSE-TS for Ing (packets not encrypted) */
    APERTA2_PTP_HYB_PM     = 6,   /*! PTP enabled, Hybrid for Egr & PM-TS for Ing (packets encrypted) */
    APERTA2_PTP_RSVD       = 7    /*! Reserved */

};

/*!
 * @enum APERTA2_E_TX_DATA_ON_FAULT_SEL
 * Constants that define TX data mode on fault (PM TX data modes)
 */
enum APERTA2_E_TX_DATA_ON_FAULT_SEL
{
    APERTA2_TX_DATA_STALL_ON_FAULT    = 0,   /*!< Stall TX and send idles during faults */
    APERTA2_TX_DATA_DROP_ON_FAULT     = 1,   /*!< Drop TX and send idles on faults */
    APERTA2_TX_DATA_CONTINUE_ON_FAULT = 2   /*!< Continue TX on faults */
};


/*!
 * @enum APERTA2_CLKGEN_DIV_SEL
 * Constants to select Divider for Output Clock Generation
 */
enum APERTA2_CLKGEN_DIV_SEL {
    APERTA2_CLKGEN_DIV_20    = 0,  /*!< Line Rate % 20 */
    APERTA2_CLKGEN_DIV_40    = 1,  /*!< Line Rate % 40 */
    APERTA2_CLKGEN_DIV_80    = 2,  /*!< Line Rate % 80 */
    APERTA2_CLKGEN_DIV_160   = 3,  /*!< Line Rate % 160 */
    APERTA2_CLKGEN_DIV_400   = 4,  /*!< Line Rate % 400 */
    APERTA2_CLKGEN_DIV_1000  = 5,  /*!< Line Rate % 1000 */
    APERTA2_CLKGEN_DIV_UNDEF = 6  /*!< Line Rate % Undefined */
};


/*!
 * @enum APERTA2_CLKGEN_SQUELCH_MODAPERTA2_SEL
 * Constants to select Squelch Mode for Output Clock Generation
 */
enum APERTA2_CLKGEN_SQUELCH_MODE_SEL {
    APERTA2_CLKGEN_SQUELCH_NONE      = 0,  /*!< No Squelch requested */
    APERTA2_CLKGEN_SQUELCH_LOS       = 1,  /*!< Squelch Clock on Loss of Signal (LOS) */
    APERTA2_CLKGEN_SQUELCH_LOL       = 2  /*!< Squelch Clock on Loss of Lock (LOL) */
};


/*!
 * @enum APERTA2_PORT_INUSE
 * Constants that define Port InUse Status
 */
enum APERTA2_PORT_INUSE {
    APERTA2_PORT_NOT_INUSE     = 0,   /*!< Port Not in use */
    APERTA2_PORT_INUSE         = 1   /*!< Port in use */
};


/*!
 * @enum APERTA2_PORTID_INUSE
 * Constants that define PortID InUse Status
 */
enum APERTA2_PORTID_INUSE {
    APERTA2_PORTID_NOT_INUSE  = 0,   /*!< PortID Not in use */
    APERTA2_PORTID_INUSE      = 1   /*!< PortID in use */
};


/*!
 * @enum APERTA2_PBIST_SEL
 * Constants to select PBIST location
 */
enum APERTA2_PBIST_SEL {
    APERTA2_PBIST_ING_FC  = 0,  /*!< PBIST: Generate packets at ING_FC */
    APERTA2_PBIST_EGR_FC  = 1,  /*!< PBIST: Generate packets at EGR_FC */
    APERTA2_PBIST_EGR_SF  = 2  /*!< PBIST: Generate packets at EGR_SF */
};


/*!
 * @enum APERTA2_FOMUX_STATUS
 * Constants to report per port FOMUX result using register interface
 */
enum APERTA2_FOMUX_STATUS {
    APERTA2_FOMUX_NOT_REQUESTED  = 0x0,   /*!< FO MUX not requested */
    APERTA2_FOMUX_PARAM_ERROR    = 0x1,   /*!< FO MUX invalid request */
    APERTA2_FOMUX_SWITCH_ERROR   = 0x2,   /*!< FO MUX switch completed with errors */
    APERTA2_FOMUX_SWITCH_SUCCESS = 0x3   /*!< FO MUX switch completed with success */
};


/*******************************************************************************
 *  Message Interface related defines
 ******************************************************************************/

/*!
 * @enum APERTA2_MSG_STATUS
 * Constants to select Message status in the Message Interface module
 */
enum APERTA2_MSG_STATUS {
    APERTA2_STS_IDLE   = 0x0,   /*!< Message status idle */
    APERTA2_STS_SENT   = 0x1,   /*!< Message status sent */
    APERTA2_STS_RECD   = 0x2,   /*!< Message status received */
    APERTA2_STS_PROCD  = 0x3,   /*!< Message status processed */
    APERTA2_STS_RESET  = 0xF    /*!< Message status reset (chip reset) */
};


/*!
 * @enum APERTA2_MSG_OPERATION
 * Constants to select Message operation in the Message Interface module
 */
enum APERTA2_MSG_OPERATION {
    /*--- Commands ---*/
    APERTA2_OP_WRITE      = 0x0,  /*!< write operation (one/more consecutive entries) */
    APERTA2_OP_READ       = 0x1,  /*!< read  operation (one/more consecutive entries) */
    APERTA2_OP_WRITE_EXT  = 0x2,  /*!< write extended  (one/more non-consecutive entries) */
    APERTA2_OP_READ_EXT   = 0x3,  /*!< read  extended  (one/more non-consecutive entries) */

    APERTA2_OP_START        = 0x0,  /*!< start a firmware process (state machine) */
    APERTA2_OP_START_RESULT = 0x1,  /*!< get status a started firmware process */

    /*--- Responses ---*/
    APERTA2_OP_PROCESSING = 0xD,  /*!< operation in progress (processing) */
    APERTA2_OP_SUCCESS    = 0xE,  /*!< operation result: success */
    APERTA2_OP_ERROR      = 0xF   /*!< operation result: error */
};  


/*!
 * @enum APERTA2_MSG_FUNCTION
 * Constants to select Message function in the Message Interface module
 */
enum APERTA2_MSG_FUNCTION {
    /* Functions that respond with completion code (OP_RESULT/OP_ERROR) */
    APERTA2_FUNC_PM_REGS         = 0x10,  /*!< PM_REGS (Port Macro registers) */
    APERTA2_FUNC_TSC_REGS        = 0x11,  /*!< TSC_REGS (TSC registers) */
    APERTA2_FUNC_MACSEC_REGS     = 0x12,  /*!< MACSEC_REGS (MACSEC registers) */
    APERTA2_FUNC_CHIP_IND_REGS   = 0x13,  /*!< CHIP_IND_REGS (indirect chip registers) */
    APERTA2_FUNC_AHB_REGS        = 0x14,  /*!< AHB_REGS (AHB registers) */
    APERTA2_FUNC_CONFIG_POLSWAPS      = 0x15,  /*!< CONFIG_POLSWAPS (Polarity swap config) */
    APERTA2_FUNC_CONFIG_PHY      = 0x16,  /*!< CONFIG_PHY (Configure phy-level operating mode) */
    APERTA2_FUNC_CLOCK_GEN       = 0x17,  /*!< CLOCK_GEN (Clock Generation and Squelch) */
    APERTA2_FUNC_MACSEC_KEY      = 0x18,  /*!< MACSEC_KEY (MACSEC key) */
    APERTA2_FUNC_CONFIG_LANES    = 0x19,  /*!< CONFIG_LANES (Configure board lane maps */
    APERTA2_FUNC_CHIP_DIR_REGS   = 0x1A,  /*!< CHIP_DIR_REGS (direct chip registers) */
    APERTA2_FUNC_CONFIG_PORT     = 0x1C,  /*!< CONFIG_PORT (Configure a Port) */
    APERTA2_FUNC_PAUSE_PORT      = 0x1D,  /*!< PAUSE_PORT (Pause a Port) */
    APERTA2_FUNC_RESUME_PORT     = 0x1E,  /*!< RESUME_PORT (Resume a Port) */
    APERTA2_FUNC_CONFIG_PBIST    = 0x1F,  /*!< CONFIG_PBIST (Configure PBIST) */

    /* Functions that respond with OP_PROCESSING code immediately,
     * actual execution is done in the background loop */
    APERTA2_FUNC_ENABLE_PORT     = 0x20,  /*!< ENABLE_PORT (Enable a Port) */
    APERTA2_FUNC_DISABLE_PORT    = 0x21,  /*!< DISABLE_PORT (Disable a Port) */
    APERTA2_FUNC_FLUSH_PORT      = 0x22,  /*!< FLUSH_PORT (Flush a Port) */
    APERTA2_FUNC_SWITCH_MUX      = 0x23,  /*!< SWITCH_MUX (Switch MUX) */
    APERTA2_FUNC_ENABLE_PBIST    = 0x24,  /*!< ENABLE_PBIST (Enable PBIST) */
    APERTA2_FUNC_DISABLE_PBIST   = 0x25,  /*!< DISABLE_PBIST (Disable PBIST) */
    APERTA2_FUNC_INIT_EIP164     = 0x26,  /*!< INIT_EIP164 (Initialize EIP164 block) */
    APERTA2_FUNC_MACSEC_KEY_CLEAR  = 0x27,  /*!< MACSEC_KEY_CLEAR (Clear all MACSEC keys) */
    APERTA2_FUNC_SWITCH_DPCLK    = 0x28,  /*!< SWITCH_DPCLK (Switch DP clock) */

    /* Functions that respond with completion code (OP_RESULT/OP_ERROR)
     Common functions across multiple chips*/
    APERTA2_FUNC_CONFIG_SPIROM   = 0x30,  /*< CONFIG_SPIROM (Configure SPIROM interface)*/
    APERTA2_FUNC_ERASE_SPIROM    = 0x31,  /*< ERASE_SPIROM (Erase SPIROM Memory)*/
    APERTA2_FUNC_SPIROM_MEM      = 0x32,  /*< SPIROM_MEM (Read/Write SPIROM Memory)*/
    
    /* PTP Functions */
    APERTA2_FUNC_PTP_CONFIG      = 0x40,  /*!< PTP_CONFIG (Configure PTP Mode) */

    APERTA2_FUNC_RESERVED        = 0xFF   /*!< Reserved */
};


/*!
 * @enum APERTA2_MSG_ERROR
 * Constants for error codes returned in the Message Interface module
 */
enum APERTA2_MSG_ERROR
{
    APERTA2_ERR_FUNC_NOTAVAILABLE  = 0x10,  /*!< Function not available  */
    APERTA2_ERR_OPER_NOTAVAILABLE  = 0x11,  /*!< Operation not available */
    APERTA2_ERR_INCORRECT_LENGTH   = 0x12,  /*!< Incorrect length */
    APERTA2_ERR_BUSY               = 0x13,  /*!< HW/FW is busy */
    APERTA2_ERR_INCORRECT_OPERAND  = 0x14,  /*!< Incorrect operand */
    APERTA2_ERR_FUNC_SPECIFIC      = 0x1F,  /*!< Function specific */
    APERTA2_ERR_UNKNOWN            = 0xFF   /*!< Error Unknown / Unexpected */
};


/*******************************************************************************
 *  Firmware Registers Usage
 *
 *  Firmware registers used for Message Interface, Port and Lane Configuration
 ******************************************************************************/
/***********************************************************************
 *    MEMORY BANK 0: fwreg_000 - fwreg_3FF
 **********************************************************************/
/*! Port Configuration Registers (fwreg_000 - fwreg_0FF) */
#define APERTA2_PORT_CONFIG_REG_START_ADDR     FW_REGS_fwreg_000_Adr
#define APERTA2_PORT_CONFIG_REG_END_ADDR       FW_REGS_fwreg_0FF_Adr

/*! PBIST Configuration Registers (fwreg_100 - fwreg_1FF) */
#define APERTA2_PBIST_CONFIG_REG_START_ADDR    FW_REGS_fwreg_100_Adr
#define APERTA2_PBIST_CONFIG_REG_END_ADDR      FW_REGS_fwreg_1FF_Adr

/*! LANE Configuration Registers (fwreg_200 - fwreg_22F) */
#define APERTA2_LANE_CONFIG_REG_START_ADDR     FW_REGS_fwreg_200_Adr
#define APERTA2_LANE_CONFIG_REG_END_ADDR       FW_REGS_fwreg_22F_Adr

/*! RCLK Configuration Registers (fwreg_230 - fwreg_24F) */
#define APERTA2_RCLK_CONFIG_REG_START_ADDR     FW_REGS_fwreg_230_Adr
#define APERTA2_RCLK_CONFIG_REG_END_ADDR       FW_REGS_fwreg_24F_Adr

/*! Available Registers (fwreg_250 - fwreg_39F) */
#define APERTA2_AVAILABLE0_REG_START_ADDR       FW_REGS_fwreg_250_Adr
#define APERTA2_AVAILABLE0_REG_END_ADDR         FW_REGS_fwreg_39F_Adr

/*! AVS Configuration/Status Registers (fwreg_3A0 - fwreg_3BF) */
#define APERTA2_AVS_REG_START_ADDR             FW_REGS_fwreg_3A0_Adr
#define APERTA2_AVS_REG_END_ADDR               FW_REGS_fwreg_3BF_Adr

/*! Debug Registers (fwreg_3C0 - fwreg_3FF) */
#define APERTA2_DIAGNOSTIC_REG_START_ADDR      FW_REGS_fwreg_3C0_Adr
#define APERTA2_DIAGNOSTIC_REG_END_ADDR        FW_REGS_fwreg_3FF_Adr

/***********************************************************************
 *    MEMORY BANK 1: fwreg_400 - fwreg_7FF
 **********************************************************************/

/*! Message Interface 0 Registers (fwreg_400 - fwreg_4FF) */
#define APERTA2_MSG_INTF0_IN_BUFFER_ADDR    BCM_APERTA2_DIRECT_FWS_FWREG_400r  /*!< MSG_INTF0 buffer for incoming messages */
#define APERTA2_MSG_INTF0_OUT_BUFFER_ADDR   BCM_APERTA2_DIRECT_FWS_FWREG_480r  /*!< MSG_INTF0 buffer for outgoing messages */

/*! Available Registers (fwreg_500 - fwreg_7FF) */
#define APERTA2_AVAILABLE1_REG_START_ADDR      FW_REGS_fwreg_500_Adr
#define APERTA2_AVAILABLE1_REG_END_ADDR        FW_REGS_fwreg_7FF_Adr

/***********************************************************************
 *    MEMORY BANK 3: fwreg_800 - fwreg_BFF
 **********************************************************************/

/*! Message Interface 1 Registers (fwreg_800 - fwreg_BFF) */
#define APERTA2_MSG_INTF1_IN_BUFFER_ADDR    FW_REGS_fwreg_800_Adr  /*!< MSG_INTF1 buffer for incoming messages */
#define APERTA2_MSG_INTF1_OUT_BUFFER_ADDR   FW_REGS_fwreg_880_Adr  /*!< MSG_INTF1 buffer for outgoing messages */

/*! Available Registers (fwreg_500 - fwreg_7FF) */
#define APERTA2_AVAILABLE2_REG_START_ADDR      FW_REGS_fwreg_900_Adr
#define APERTA2_AVAILABLE2_REG_END_ADDR        FW_REGS_fwreg_BFF_Adr

/*! Message Interface IN Buffer Size (in bytes) */
#define APERTA2_MSG_INTF_IN_BUFFER_SIZE     0x0100                 /*!< in bytes (128 16-bit registers)  */
/*! Message Interface OUT Buffer Size (in bytes) */
#define APERTA2_MSG_INTF_OUT_BUFFER_SIZE    0x0100                 /*!< in bytes (128 16-bit registers)  */





/*******************************************************************************
 *  Diagnostic Registers
 ******************************************************************************/

/*! LMI error logging */
#define APERTA2_GPREG_lmi_debug_Adr             FW_REGS_fwreg_3E0_Adr
#define APERTA2_GPREG_lmi_cmd_Adr               FW_REGS_fwreg_3E1_Adr
#define APERTA2_GPREG_lmi_addr_Adr              FW_REGS_fwreg_3E2_Adr
#define APERTA2_GPREG_lmi_data_Adr              FW_REGS_fwreg_3E3_Adr
#define APERTA2_GPREG_lmi_cmd_ext_Adr           FW_REGS_fwreg_3E4_Adr
#define APERTA2_GPREG_lmi_cmd_ext2_Adr          FW_REGS_fwreg_3E5_Adr
#define APERTA2_GPREG_lmi_cmd_seq_Adr           FW_REGS_fwreg_3E6_Adr
#define APERTA2_GPREG_lmi_config_Adr            FW_REGS_fwreg_3E7_Adr
#define APERTA2_GPREG_lmi_slv_err_rdata_msb_Adr FW_REGS_fwreg_3E8_Adr
#define APERTA2_GPREG_lmi_slv_err_rdata_lsb_Adr FW_REGS_fwreg_3E9_Adr
#define APERTA2_GPREG_lmi_status_Adr            FW_REGS_fwreg_3EA_Adr

/* Available Registers: 3eb - 3F5 */

/*! Max process time for SWITCH_MUX in 10usec units */
#define APERTA2_FWREG_switch_mux_time_lsw_Adr  FW_REGS_fwreg_3F6_Adr
#define APERTA2_FWREG_switch_mux_time_msw_Adr  FW_REGS_fwreg_3F7_Adr

/*! Max process time for PTP servicing in 10usec units */
#define APERTA2_FWREG_ptp_proc_time_lsw_Adr    FW_REGS_fwreg_3F8_Adr
#define APERTA2_FWREG_ptp_proc_time_msw_Adr    FW_REGS_fwreg_3F9_Adr

/*! Firmware initialization time in 10usec units */
#define APERTA2_FWREG_init_time_lsw_Adr        FW_REGS_fwreg_3FA_Adr
#define APERTA2_FWREG_init_time_msw_Adr        FW_REGS_fwreg_3FB_Adr

/*! Minimum background loop time in 10usec units */
#define APERTA2_FWREG_min_loop_time_lsw_Adr    FW_REGS_fwreg_3FC_Adr
#define APERTA2_FWREG_min_loop_time_msw_Adr    FW_REGS_fwreg_3FD_Adr

/*! Maximum background loop time in 10usec units */
#define APERTA2_FWREG_max_loop_time_lsw_Adr    FW_REGS_fwreg_3FE_Adr
#define APERTA2_FWREG_max_loop_time_msw_Adr    FW_REGS_fwreg_3FF_Adr


/*******************************************************************************
 *  General Purpose Registers
 *
 *  Firmware registers for control, status and error fields.
 ******************************************************************************/

/* MSG_INTF0_msgin and msgout register */
#define APERTA2_MSG_INTF0_MSGIN_ADR              BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGINr
#define APERTA2_MSG_INTF0_MSGOUT_ADR             BCM_APERTA2_DIRECT_GEN_CNTRLS_MST_MSGOUTr

/* MSG_INTF1_msgin and msgout register */
#define APERTA2_MSG_INTF1_msgin_Adr              GEN_CNTRLS_mst_msgin2_Adr
#define APERTA2_MSG_INTF1_msgout_Adr             GEN_CNTRLS_mst_msgout2_Adr

/* Firmware control and status registers */
#define APERTA2_GPREG_fw_control_Adr             GEN_CNTRLS_gpreg_00_Adr
#define APERTA2_GPREG_fw_status0_Adr             GEN_CNTRLS_gpreg_01_Adr
#define APERTA2_GPREG_fw_status1_Adr             GEN_CNTRLS_gpreg_02_Adr
#define APERTA2_GPREG_fw_active_Adr              GEN_CNTRLS_gpreg_03_Adr
#define APERTA2_GPREG_fw_wdog_min_count_Adr      GEN_CNTRLS_gpreg_04_Adr
#define APERTA2_GPREG_fw_error_Adr               GEN_CNTRLS_gpreg_05_Adr
#define APERTA2_GPREG_fw_fatal_error_status_Adr  GEN_CNTRLS_gpreg_06_Adr
#define APERTA2_GPREG_fw_port_inuse_Adr          GEN_CNTRLS_gpreg_07_Adr
#define APERTA2_GPREG_fw_ing_dpport_inuse_Adr    GEN_CNTRLS_gpreg_08_Adr
#define APERTA2_GPREG_fw_egr_dpport_inuse_Adr    GEN_CNTRLS_gpreg_09_Adr
#define APERTA2_GPREG_fw_sys_portid_inuse_Adr    GEN_CNTRLS_gpreg_0A_Adr
#define APERTA2_GPREG_fw_line_portid_inuse_Adr   GEN_CNTRLS_gpreg_0B_Adr


#ifdef APERTA2_BITFIELD_NA
/*******************************************************************************
 *  Firmware register type definitions
 ******************************************************************************/

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
    uint16_t  fw_disable  : 1;   /*!<  disable firmware (idle loop) */
    uint16_t  reserved    : 13;  /*!<  Reserved bits */
    uint16_t  fw_clr_loop_times  : 1;   /*!<  Clear loop times (self clear bit) */
    uint16_t  fw_init_disable    : 1;   /*!<  disable firmware initialization */
  } fields;
} GPREG_fw_control_TYPE;

/*!
 * GPREG_fw_status0_TYPE
 *
 * Firmware Status Register that holds all top-level status bits. This register
 * is implemented using GEN_CNTRLS_gpreg_01.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  fw_init_done    : 1;   /*!<  Firmware initialization done */
    uint16_t  rsvd            : 15;  /*!<  Reserved bits                */
  } fields;
} GPREG_fw_status0_TYPE;

/*!
 * GPREG_fw_status1_TYPE
 *
 * Firmware Status1 Register that holds all top-level status bits. This register
 * is implemented using GEN_CNTRLS_gpreg_02.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  sys_oct0_pg_active   : 4;   /*!<  Sys-side Octal0 PG micro_uc_active[3:0]  */
    uint16_t  sys_oct1_pg_active   : 4;   /*!<  Sys-side Octal1 PG micro_uc_active[3:0]  */
    uint16_t  line_oct0_pg_active  : 4;   /*!<  Line-side Octal0 PG micro_uc_active[3:0] */
    uint16_t  line_oct1_pg_active  : 4;   /*!<  Line-side Octal1 PG micro_uc_active[3:0] */
  } fields;
} GPREG_fw_status1_TYPE;

/*!
 * GPREG_fw_active_TYPE
 *
 * Firmware Active Status Register that is set to 0x5555 by firmware in the background
 * loop. This register is implemented using GEN_CNTRLS_gpreg_03.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  active_val  : 16;   /*!<  Firmware active status */
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
    uint16_t  wdog_min_cnt  : 16;   /*!<  Watchdog minimum count */
  } fields;
} GPREG_fw_wdog_min_count_TYPE;

/*!
 * GPREG_fw_error_TYPE
 *
 * Firmware Error Register that holds all top-level status bits. This register
 * is implemented using GEN_CNTRLS_gpreg_05.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  lmi_xact_error   : 1;   /*!<  LMI transaction error */
    uint16_t  lmi_rd_error     : 1;   /*!<  LMI read error - RD FIFO !empty before a xact */
    uint16_t  msg_intf0_error  : 1;   /*!<  Message Interface 0 error */
    uint16_t  msg_intf1_error  : 1;   /*!<  Message Interface 1 error */
    uint16_t  fatal_error      : 1;   /*!<  Fatal Error (unexpected interrupt received) */
    uint16_t  wdt_expired      : 1;   /*!<  Watchdog timer expired */
    uint16_t  macsec_mem_error : 1;   /*!<  MACsec memory power up error */
    uint16_t  fc_mem_error     : 1;   /*!<  FC memory power up error */
    uint16_t  pmid_lock_error  : 1;   /*!<  PM Port ID lock error */
    uint16_t  port_rdmap_error : 1;   /*!<  Port Rd Map error */
    uint16_t  mem_buf_error    : 1;   /*!<  Memory buffer error */
    uint16_t  mem_user_error   : 1;   /*!<  Memory buffer user error */
    uint16_t  reserved         : 4;   /*!<  Reserved bits */
  } fields;
} GPREG_fw_error_TYPE;

/*!
 * GPREG_fw_fatal_error_status_TYPE
 *
 * Firmware Fatal Error Status Register that implements fatal errors seen by firmware
 * like unexpected interrupts. This register is implemented using GEN_CNTRLS_gpreg_06.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  NMI_err         : 1;   /*!<  NMI received error */
    uint16_t  HardFault_err   : 1;   /*!<  Hard Fault received error */
    uint16_t  SVC_err         : 1;   /*!<  SVC received error */
    uint16_t  PendSV_err      : 1;   /*!<  PendSV received error */
    uint16_t  SysTick_err     : 1;   /*!<  PendSV received error */
    uint16_t  IRQ00_err       : 1;   /*!<  IRQ00 received error */
    uint16_t  IRQ01_err       : 1;   /*!<  IRQ01 received error */
    uint16_t  IRQ12_err       : 1;   /*!<  IRQ12 received error */
    uint16_t  rsvd            : 7;   /*!<  Reserved bits */
    uint16_t  DefaultIRQ_err  : 1;   /*!<  Default IRQ error */
  } fields;
} GPREG_fw_fatal_error_status_TYPE;


/*!
 * GPREG_fw_port_inuse_TYPE
 *
 * Firmware Status Register that holds port in-use status. This register is
 * implemented using GEN_CNTRLS_gpreg_07.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  port0_sts  : 1;   /*!<  Port  0 in use status */
    uint16_t  port1_sts  : 1;   /*!<  Port  1 in use status */
    uint16_t  port2_sts  : 1;   /*!<  Port  2 in use status */
    uint16_t  port3_sts  : 1;   /*!<  Port  3 in use status */
    uint16_t  port4_sts  : 1;   /*!<  Port  4 in use status */
    uint16_t  port5_sts  : 1;   /*!<  Port  5 in use status */
    uint16_t  port6_sts  : 1;   /*!<  Port  6 in use status */
    uint16_t  port7_sts  : 1;   /*!<  Port  7 in use status */
    uint16_t  port8_sts  : 1;   /*!<  Port  8 in use status */
    uint16_t  port9_sts  : 1;   /*!<  Port  9 in use status */
    uint16_t  port10_sts : 1;   /*!<  Port 10 in use status */
    uint16_t  port11_sts : 1;   /*!<  Port 11 in use status */
    uint16_t  port12_sts : 1;   /*!<  Port 12 in use status */
    uint16_t  port13_sts : 1;   /*!<  Port 13 in use status */
    uint16_t  port14_sts : 1;   /*!<  Port 14 in use status */
    uint16_t  port15_sts : 1;   /*!<  Port 15 in use status */
  } fields;
} GPREG_fw_port_inuse_TYPE;


/*!
 * GPREG_fw_sys_portid_inuse_TYPE
 *
 * Firmware Status Register that holds sys portid in-use status. This register is
 * implemented using GEN_CNTRLS_gpreg_08.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  sys_portid0_sts   : 1;   /*!<  System-side PortID  0 in use status */
    uint16_t  sys_portid1_sts   : 1;   /*!<  System-side PortID  1 in use status */
    uint16_t  sys_portid2_sts   : 1;   /*!<  System-side PortID  2 in use status */
    uint16_t  sys_portid3_sts   : 1;   /*!<  System-side PortID  3 in use status */
    uint16_t  sys_portid4_sts   : 1;   /*!<  System-side PortID  4 in use status */
    uint16_t  sys_portid5_sts   : 1;   /*!<  System-side PortID  5 in use status */
    uint16_t  sys_portid6_sts   : 1;   /*!<  System-side PortID  6 in use status */
    uint16_t  sys_portid7_sts   : 1;   /*!<  System-side PortID  7 in use status */
    uint16_t  sys_portid8_sts   : 1;   /*!<  System-side PortID  8 in use status */
    uint16_t  sys_portid9_sts   : 1;   /*!<  System-side PortID  9 in use status */
    uint16_t  sys_portid10_sts  : 1;   /*!<  System-side PortID 10 in use status */
    uint16_t  sys_portid11_sts  : 1;   /*!<  System-side PortID 11 in use status */
    uint16_t  sys_portid12_sts  : 1;   /*!<  System-side PortID 12 in use status */
    uint16_t  sys_portid13_sts  : 1;   /*!<  System-side PortID 13 in use status */
    uint16_t  sys_portid14_sts  : 1;   /*!<  System-side PortID 14 in use status */
    uint16_t  sys_portid15_sts  : 1;   /*!<  System-side PortID 15 in use status */
  } fields;
} GPREG_fw_sys_portid_inuse_TYPE;


/*!
 * GPREG_fw_line_portid_inuse_TYPE
 *
 * Firmware Status Register that holds line portid in-use status. This register is
 * implemented using GEN_CNTRLS_gpreg_09.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  line_portid0_sts   : 1;   /*!<  Line-side PortID  0 in use status */
    uint16_t  line_portid1_sts   : 1;   /*!<  Line-side PortID  1 in use status */
    uint16_t  line_portid2_sts   : 1;   /*!<  Line-side PortID  2 in use status */
    uint16_t  line_portid3_sts   : 1;   /*!<  Line-side PortID  3 in use status */
    uint16_t  line_portid4_sts   : 1;   /*!<  Line-side PortID  4 in use status */
    uint16_t  line_portid5_sts   : 1;   /*!<  Line-side PortID  5 in use status */
    uint16_t  line_portid6_sts   : 1;   /*!<  Line-side PortID  6 in use status */
    uint16_t  line_portid7_sts   : 1;   /*!<  Line-side PortID  7 in use status */
    uint16_t  line_portid8_sts   : 1;   /*!<  Line-side PortID  8 in use status */
    uint16_t  line_portid9_sts   : 1;   /*!<  Line-side PortID  9 in use status */
    uint16_t  line_portid10_sts  : 1;   /*!<  Line-side PortID 10 in use status */
    uint16_t  line_portid11_sts  : 1;   /*!<  Line-side PortID 11 in use status */
    uint16_t  line_portid12_sts  : 1;   /*!<  Line-side PortID 12 in use status */
    uint16_t  line_portid13_sts  : 1;   /*!<  Line-side PortID 13 in use status */
    uint16_t  line_portid14_sts  : 1;   /*!<  Line-side PortID 14 in use status */
    uint16_t  line_portid15_sts  : 1;   /*!<  Line-side PortID 15 in use status */
  } fields;
} GPREG_fw_line_portid_inuse_TYPE;


/*!
 * GPREG_fw_fomux_request_TYPE
 *
 * Firmware Control Register to request FOMUX switch. This register is
 * implemented using GEN_CNTRLS_gpreg_10.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  rsvd          : 14;  /*!<  Reserved */
    uint16_t  switch_all    : 1;   /*!<  Switch all FO ports */
    uint16_t  switch_enable : 1;   /*!<  Switch enable control */
  } fields;
} GPREG_fw_fomux_request_TYPE;


/*!
 * GPREG_fw_fomux_port_select_TYPE
 *
 * Firmware Control Register to select ports that need FOMUX switch. This register is
 * implemented using GEN_CNTRLS_gpreg_11.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  port0_sel     : 1;   /*!<  Select/include Port 0 when switching */
    uint16_t  port1_sel     : 1;   /*!<  Select/include Port 1 when switching */
    uint16_t  port2_sel     : 1;   /*!<  Select/include Port 2 when switching */
    uint16_t  port3_sel     : 1;   /*!<  Select/include Port 3 when switching */
    uint16_t  port4_sel     : 1;   /*!<  Select/include Port 4 when switching */
    uint16_t  port5_sel     : 1;   /*!<  Select/include Port 5 when switching */
    uint16_t  port6_sel     : 1;   /*!<  Select/include Port 6 when switching */
    uint16_t  port7_sel     : 1;   /*!<  Select/include Port 7 when switching */
    uint16_t  port8_sel     : 1;   /*!<  Select/include Port 8 when switching */
    uint16_t  port9_sel     : 1;   /*!<  Select/include Port 9 when switching */
    uint16_t  port10_sel    : 1;   /*!<  Select/include Port 10 when switching */
    uint16_t  port11_sel    : 1;   /*!<  Select/include Port 11 when switching */
    uint16_t  port12_sel    : 1;   /*!<  Select/include Port 12 when switching */
    uint16_t  port13_sel    : 1;   /*!<  Select/include Port 13 when switching */
    uint16_t  port14_sel    : 1;   /*!<  Select/include Port 14 when switching */
    uint16_t  port15_sel    : 1;   /*!<  Select/include Port 15 when switching */
  } fields;
} GPREG_fw_fomux_port_select_TYPE;


/*!
 * GPREG_fw_fomux_result0_TYPE
 *
 * Firmware Status Register showing switch result on a per port basis. This
 * register is implemented using GEN_CNTRLS_gpreg_12.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  port0_sts : 2;   /*!<  Port 0 switch mux status */
    uint16_t  port1_sts : 2;   /*!<  Port 1 switch mux status */
    uint16_t  port2_sts : 2;   /*!<  Port 2 switch mux status */
    uint16_t  port3_sts : 2;   /*!<  Port 3 switch mux status */
    uint16_t  port4_sts : 2;   /*!<  Port 4 switch mux status */
    uint16_t  port5_sts : 2;   /*!<  Port 5 switch mux status */
    uint16_t  port6_sts : 2;   /*!<  Port 6 switch mux status */
    uint16_t  port7_sts : 2;   /*!<  Port 7 switch mux status */
  } fields;
} GPREG_fw_fomux_result0_TYPE;


/*!
 * GPREG_fw_fomux_result1_TYPE
 *
 * Firmware Status Register showing switch result on a per port basis. This
 * register is implemented using GEN_CNTRLS_gpreg_13.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  port8_sts  : 2;  /*!<  Port 8 switch mux status */
    uint16_t  port9_sts  : 2;  /*!<  Port 9 switch mux status */
    uint16_t  port10_sts : 2;  /*!<  Port 10 switch mux status */
    uint16_t  port11_sts : 2;  /*!<  Port 11 switch mux status */
    uint16_t  port12_sts : 2;  /*!<  Port 12 switch mux status */
    uint16_t  port13_sts : 2;  /*!<  Port 13 switch mux status */
    uint16_t  port14_sts : 2;  /*!<  Port 14 switch mux status */
    uint16_t  port15_sts : 2;  /*!<  Port 15 switch mux status */
  } fields;
} GPREG_fw_fomux_result1_TYPE;


/*!
 * GPREG_fw_fomux_gpio_control_TYPE
 *
 * Firmware Control Register to enable switch mux using GPIO. This register
 * is implemented using GEN_CNTRLS_gpreg_14.
 */
typedef union {
  uint16_t data;
  struct {
    uint16_t  gpio_num  : 8;   /*!<  GPIO pin number to use   */
    uint16_t  rsvd      : 7;   /*!<  Reserved */
    uint16_t  gpio_en   : 1;   /*!<  FO Mux enable using GPIO */
  } fields;
} GPREG_fw_fomux_gpio_control_TYPE;
#endif
#endif /* APERTA2_SDK_INTF_H_ */
