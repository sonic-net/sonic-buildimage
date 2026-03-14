#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <linux/types.h>

/* advanced features control */
#define TRANSVR_INFO_DUMP_ENABLE        (1)
#define TRANSVR_INFO_CACHE_ENABLE       (1)
#define TRANSVR_UEVENT_ENABLE           (1)

/* Transceiver type define */
#define TRANSVR_TYPE_UNKNOW_1           (0x00)
#define TRANSVR_TYPE_UNKNOW_2           (0xff)
#define TRANSVR_TYPE_SFP                (0x03)  /* Define for SFP, SFP+, SFP28 */
#define TRANSVR_TYPE_QSFP               (0x0c)
#define TRANSVR_TYPE_QSFP_PLUS          (0x0d)
#define TRANSVR_TYPE_QSFP_28            (0x11)
#define TRANSVR_TYPE_UNPLUGGED          (0xfa)  /* Define for ERROR handle */
#define TRANSVR_TYPE_FAKE               (0xfc)  /* Define for ERROR handle */
#define TRANSVR_TYPE_INCONSISTENT       (0xfd)  /* Define for ERROR handle */
#define TRANSVR_TYPE_ERROR              (0xfe)  /* Define for ERROR handle */

/* Transceiver class for base info */
#define TRANSVR_CLASS_UNSPECIFIED       (0)
#define TRANSVR_CLASS_ERROR             (-26001)
#define TRANSVR_CLASS_1G                (26001)
#define TRANSVR_CLASS_10G               (26011)
#define TRANSVR_CLASS_25G               (26021)
#define TRANSVR_CLASS_40G               (26041)
#define TRANSVR_CLASS_100G              (26101)
#define TRANSVR_CLASS_NO_SPERARABLE     (26901)
#define TRANSVR_CLASS_EXTEND_COMP       (26902)
/* Transceiver class for Optical 1G */
#define TRANSVR_CLASS_OPTICAL           (27000)
#define TRANSVR_CLASS_OPTICAL_100       (27001)
#define TRANSVR_CLASS_OPTICAL_1G        (27002)
#define TRANSVR_CLASS_OPTICAL_1G_AOC    (27003)
#define TRANSVR_CLASS_OPTICAL_1G_SX     (27004)
#define TRANSVR_CLASS_OPTICAL_1G_LX     (27005)
#define TRANSVR_CLASS_OPTICAL_1G_EX     (27006)
/* Transceiver class for Optical 10G */
#define TRANSVR_CLASS_OPTICAL_10G       (27010)
#define TRANSVR_CLASS_OPTICAL_10G_S_AOC (27011)
#define TRANSVR_CLASS_OPTICAL_10G_S_SR  (27012)
#define TRANSVR_CLASS_OPTICAL_10G_S_LR  (27013)
#define TRANSVR_CLASS_OPTICAL_10G_S_ER  (27014)
#define TRANSVR_CLASS_OPTICAL_10G_Q_AOC (27015)
#define TRANSVR_CLASS_OPTICAL_10G_Q_SR  (27016)
#define TRANSVR_CLASS_OPTICAL_10G_Q_LR  (27017)
#define TRANSVR_CLASS_OPTICAL_10G_Q_ER  (27018)
/* Transceiver class for Optical 25G */
#define TRANSVR_CLASS_OPTICAL_25G       (27020)
#define TRANSVR_CLASS_OPTICAL_25G_AOC   (27021)
#define TRANSVR_CLASS_OPTICAL_25G_SR    (27022)
#define TRANSVR_CLASS_OPTICAL_25G_LR    (27023)
#define TRANSVR_CLASS_OPTICAL_25G_ER    (27024)
/* Transceiver class for Optical 40G */
#define TRANSVR_CLASS_OPTICAL_40G       (27040)
#define TRANSVR_CLASS_OPTICAL_40G_AOC   (27041)
#define TRANSVR_CLASS_OPTICAL_40G_SR4   (27042)
#define TRANSVR_CLASS_OPTICAL_40G_LR4   (27043)
#define TRANSVR_CLASS_OPTICAL_40G_ER4   (27044)
/* Transceiver class for Optical 100G */
#define TRANSVR_CLASS_OPTICAL_100G      (27100)
#define TRANSVR_CLASS_OPTICAL_100G_AOC  (27101)
#define TRANSVR_CLASS_OPTICAL_100G_SR4  (27102)
#define TRANSVR_CLASS_OPTICAL_100G_LR4  (27103)
#define TRANSVR_CLASS_OPTICAL_100G_ER4  (27104)
#define TRANSVR_CLASS_OPTICAL_100G_PSM4 (27105)
/* Transceiver class for Copper */
#define TRANSVR_CLASS_COPPER            (28000)
#define TRANSVR_CLASS_COPPER_L1_1G      (28001)
#define TRANSVR_CLASS_COPPER_L1_10G     (28011)
#define TRANSVR_CLASS_COPPER_L4_10G     (28012)
#define TRANSVR_CLASS_COPPER_L1_25G     (28021)
#define TRANSVR_CLASS_COPPER_L4_40G     (28041)
#define TRANSVR_CLASS_COPPER_L4_100G    (28101)
/* Transceiver class for Base-T */
#define TRANSVR_CLASS_BASE_T_1000       (29001)
#define TRANSVR_CLASS_BASE_T_1000_up    (29002)
/* For uevent message */
#define TRANSVR_UEVENT_KEY_IF           "IF_TYPE"
#define TRANSVR_UEVENT_KEY_SP           "IF_SPEED"
#define TRANSVR_UEVENT_KEY_LANE         "IF_LANE"
#define TRANSVR_UEVENT_UNKNOW           "UNKNOW"
#define TRANSVR_IF_KR                   "KR"
#define TRANSVR_IF_KR4                  "KR4"
#define TRANSVR_IF_SR                   "SR"
#define TRANSVR_IF_SR4                  "SR4"
#define TRANSVR_IF_SFI                  "SFI"
#define TRANSVR_IF_IF_GMII              "GMII"
#define TRANSVR_IF_IF_XGMII             "XGMII"
#define TRANSVR_IF_SP_100               "100"
#define TRANSVR_IF_SP_1G                "1000"
#define TRANSVR_IF_SP_10G               "10000"
#define TRANSVR_IF_SP_25G               "25000"
#define TRANSVR_IF_SP_40G               "40000"
#define TRANSVR_IF_SP_100G              "100000"

/* Transceiver mode define */
#define TRANSVR_MODE_DIRECT             (21000)
#define TRANSVR_MODE_POLLING            (21001)

/* Transceiver state define
 * [Note]
 *  1. State is used to represent the state of "Transceiver" and "Object".
 *  2. State for different target has different means. The description as following:
 */
#define STATE_TRANSVR_CONNECTED         (0)    /* [Transvr]:Be plugged in.  [Obj]:Link up,   and work normally.       */
#define STATE_TRANSVR_NEW               (-100) /* [Transvr]:(Not used)      [Obj]:Create                              */
#define STATE_TRANSVR_INIT              (-101) /* [Transvr]:Be plugged in.  [Obj]:Link up,   and in initial process.  */
#define STATE_TRANSVR_ISOLATED          (-102) /* [Transvr]:Be plugged in.  [Obj]:Isolate,   and not provide service. */
#define STATE_TRANSVR_SWAPPED           (-200) /* [Transvr]:Be plugged in.  [Obj]:(Not used)                          */
#define STATE_TRANSVR_DISCONNECTED      (-300) /* [Transvr]:Un-plugged.     [Obj]:Link down, and not provide service. */
#define STATE_TRANSVR_UNEXCEPTED        (-901) /* [Transvr]:Any             [Obj]:Any,       and not in expect case.  */

/* Task state define */
#define STATE_T_TASK_WAIT               (110)
#define STATE_T_TASK_DONE               (0)
#define STATE_T_TASK_INIT               (-110)
#define STATE_T_TASK_FAIL               (-410)


/* Event for task handling */
#define EVENT_TRANSVR_TASK_WAIT         (2101)
#define EVENT_TRANSVR_TASK_DONE         (0)
#define EVENT_TRANSVR_TASK_FAIL         (-2101)
/* Event for initial handling */
#define EVENT_TRANSVR_INIT_UP           (2201)
#define EVENT_TRANSVR_INIT_DOWN         (1)
#define EVENT_TRANSVR_INIT_REINIT       (-2201)
#define EVENT_TRANSVR_INIT_FAIL         (-2202)
/* Event for others */
#define EVENT_TRANSVR_RELOAD_FAIL       (-2301)
#define EVENT_TRANSVR_EXCEP_INIT        (-2401)
#define EVENT_TRANSVR_EXCEP_UP          (-2402)
#define EVENT_TRANSVR_EXCEP_DOWN        (-2403)
#define EVENT_TRANSVR_EXCEP_SWAP        (-2404)
#define EVENT_TRANSVR_EXCEP_EXCEP       (-2405)
#define EVENT_TRANSVR_EXCEP_ISOLATED    (-2406)
#define EVENT_TRANSVR_I2C_CRASH         (-2501)

/* Transceiver error code define */
#define ERR_TRANSVR_UNINIT              (-201)
#define ERR_TRANSVR_UNPLUGGED           (-202)
#define ERR_TRANSVR_ABNORMAL            (-203)
#define ERR_TRANSVR_NOSTATE             (-204)
#define ERR_TRANSVR_NOTSUPPORT          (-205)
#define ERR_TRANSVR_BADINPUT            (-206)
#define ERR_TRANSVR_UPDATE_FAIL         (-207)
#define ERR_TRANSVR_RELOAD_FAIL         (-208)
#define ERR_TRANSVR_INIT_FAIL           (-209)
#define ERR_TRANSVR_UNDEFINED           (-210)
#define ERR_TRANSVR_TASK_FAIL           (-211)
#define ERR_TRANSVR_TASK_BUSY           (-212)
#define ERR_TRANSVR_UEVENT_FAIL         (-213)
#define ERR_TRANSVR_FUNC_DISABLE        (-214)
#define ERR_TRANSVR_I2C_CRASH           (-297)
#define ERR_TRNASVR_BE_ISOLATED         (-298)
#define ERR_TRANSVR_UNEXCPT             (-299)

/* For debug */
#define DEBUG_TRANSVR_INT_VAL           (-99)
#define DEBUG_TRANSVR_HEX_VAL           (0xfe)
#define DEBUG_TRANSVR_STR_VAL           "ERROR"

/* For system internal */
#define VAL_TRANSVR_COMID_ARREESS       (0x50)
#define VAL_TRANSVR_COMID_OFFSET        (0x00)
#define VAL_TRANSVR_EXTPHY_ADDR_56      (0x56)
#define VAL_TRANSVR_8472_READY_ADDR     (0x51)
#define VAL_TRANSVR_8472_READY_PAGE     (-1)
#define VAL_TRANSVR_8472_READY_OFFSET   (110)
#define VAL_TRANSVR_8472_READY_BIT      (0)
#define VAL_TRANSVR_8472_READY_VALUE    (0)
#define VAL_TRANSVR_8472_READY_ABNORMAL (0xff)
#define VAL_TRANSVR_8436_READY_ADDR     (0x50)
#define VAL_TRANSVR_8436_READY_PAGE     (-1)
#define VAL_TRANSVR_8436_READY_OFFSET   (2)
#define VAL_TRANSVR_8436_READY_BIT      (0)
#define VAL_TRANSVR_8436_READY_VALUE    (0)
#define VAL_TRANSVR_8436_READY_ABNORMAL (0xff)
#define VAL_TRANSVR_8436_PWD_ADDR       (0x50)
#define VAL_TRANSVR_8436_PWD_PAGE       (-1)
#define VAL_TRANSVR_8436_PWD_OFFSET     (123)
#define VAL_TRANSVR_PAGE_FREE           (-99)
#define VAL_TRANSVR_PAGE_SELECT_OFFSET  (127)
#define VAL_TRANSVR_PAGE_SELECT_DELAY   (5)
#define VAL_TRANSVR_TASK_RETRY_FOREVER  (-999)
#define VAL_TRANSVR_FUNCTION_DISABLE    (-1)
#define STR_TRANSVR_SFP                 "SFP"
#define STR_TRANSVR_QSFP                "QSFP"
#define STR_TRANSVR_QSFP_PLUS           "QSFP+"
#define STR_TRANSVR_QSFP28              "QSFP28"

/* For transvr buf len */
#define LEN_TRANSVR_S_STR               (16)
#define LEN_TRANSVR_M_STR               (32)
#define LEN_TRANSVR_L_STR               (64)

/* Optical wavelength */
#define VAL_OPTICAL_WAVELENGTH_SR       (850)
#define VAL_OPTICAL_WAVELENGTH_LR       (1310)
#define VAL_OPTICAL_WAVELENGTH_ER       (1550)

/* chip type define */
#define CHIP_TYPE_MAGNOLIA              (31001)  /* Magnolia, Hudson32i, Spruce */
#define CHIP_TYPE_REDWOOD               (31002)  /* Redwood, Cypress, Sequoia */
#define CHIP_TYPE_MAPLE                 (31003)  /* Maple */

#define CHIP_TYPE_LAVENDER              (31011)  /* Lavender */

/* Info from transceiver EEPROM */
struct eeprom_map_s {
    int addr_br;           int page_br;           int offset_br;           int length_br;
    int addr_cdr;          int page_cdr;          int offset_cdr;          int length_cdr;
    int addr_comp_rev;     int page_comp_rev;     int offset_comp_rev;     int length_comp_rev;
    int addr_connector;    int page_connector;    int offset_connector;    int length_connector;
    int addr_diag_type;    int page_diag_type;    int offset_diag_type;    int length_diag_type;
    int addr_extbr;        int page_extbr;        int offset_extbr;        int length_extbr;
    int addr_ext_id;       int page_ext_id;       int offset_ext_id;       int length_ext_id;
    int addr_id;           int page_id;           int offset_id;           int length_id;
    int addr_len_sm;       int page_len_sm;       int offset_len_sm;       int length_len_sm;
    int addr_len_smf;      int page_len_smf;      int offset_len_smf;      int length_len_smf;
    int addr_len_om1;      int page_len_om1;      int offset_len_om1;      int length_len_om1;
    int addr_len_om2;      int page_len_om2;      int offset_len_om2;      int length_len_om2;
    int addr_len_om3;      int page_len_om3;      int offset_len_om3;      int length_len_om3;
    int addr_len_om4;      int page_len_om4;      int offset_len_om4;      int length_len_om4;
    int addr_option;       int page_option;       int offset_option;       int length_option;
    int addr_rate_id;      int page_rate_id;      int offset_rate_id;      int length_rate_id;
    int addr_rx_am;        int page_rx_am;        int offset_rx_am;        int length_rx_am;
    int addr_rx_em;        int page_rx_em;        int offset_rx_em;        int length_rx_em;
    int addr_rx_los;       int page_rx_los;       int offset_rx_los;       int length_rx_los;
    int addr_rx_power;     int page_rx_power;     int offset_rx_power;     int length_rx_power;
    int addr_soft_rs0;     int page_soft_rs0;     int offset_soft_rs0;     int length_soft_rs0;
    int addr_soft_rs1;     int page_soft_rs1;     int offset_soft_rs1;     int length_soft_rs1;
    int addr_temp;         int page_temp;         int offset_temp;         int length_temp;
    int addr_trancomp;     int page_trancomp;     int offset_trancomp;     int length_trancomp;
    int addr_trancomp_ext; int page_trancomp_ext; int offset_trancomp_ext; int length_trancomp_ext;
    int addr_tx_bias;      int page_tx_bias;      int offset_tx_bias;      int length_tx_bias;
    int addr_tx_disable;   int page_tx_disable;   int offset_tx_disable;   int length_tx_disable;
    int addr_tx_eq;        int page_tx_eq;        int offset_tx_eq;        int length_tx_eq;
    int addr_tx_fault;     int page_tx_fault;     int offset_tx_fault;     int length_tx_fault;
    int addr_tx_power;     int page_tx_power;     int offset_tx_power;     int length_tx_power;
    int addr_vendor_name;  int page_vendor_name;  int offset_vendor_name;  int length_vendor_name;
    int addr_vendor_pn;    int page_vendor_pn;    int offset_vendor_pn;    int length_vendor_pn;
    int addr_vendor_rev;   int page_vendor_rev;   int offset_vendor_rev;   int length_vendor_rev;
    int addr_vendor_sn;    int page_vendor_sn;    int offset_vendor_sn;    int length_vendor_sn;
    int addr_voltage;      int page_voltage;      int offset_voltage;      int length_voltage;
    int addr_wavelength;   int page_wavelength;   int offset_wavelength;   int length_wavelength;
};


struct transvr_worker_s;

/* Class of transceiver object */
struct transvr_obj_s {

    /* ========== Object private property ==========
     * [Prop]: id
     * [Desc]: Type of serial transceiver.
     * [Note]: SFP:03h / QSFP:0Ch / QSPF+:0Dh /QSFP28:11h
     */
    uint8_t id;

    /* [Prop]: connector
     * [Desc]: Connector type.
     * [Note]: SFP : A0h / 2
     *         QSFP: 00h / 130
     */
    uint8_t connector;

    /* [Prop]: transvr_comp
     * [Desc]: Transceiver compliance code.
     * [Note]: SFP: SFF-8472
     *         - Normal  : A0h / offset 3-10
     *         - Extended: A0h / offset 36
     *         QSFP: SFF-8436 & SFF-8636
     *         - Normal  : 00h / offset 131-138
     *         - Extended: 00h / offset 192
     */
    uint8_t transvr_comp[8];
    uint8_t transvr_comp_ext;

    /* [Prop]: vendor_name
     * [Desc]: SFP vendor name (ASCII 16 byte char).
     * [Note]: ex:FINISAR CORP.
     */
    char *vendor_name;

    /* [Prop]: vendor_pn
     * [Desc]: Part number provided by SFP vendor (ASCII 16 byte char).
     * [Note]:
     */
    char *vendor_pn;

    /* [Prop]: vendor_rev
     * [Desc]: Revision level for part number provided by vendor (ASCII 4 byte char).
     * [Note]:
     */
    char *vendor_rev;

    /* [Prop]: vendor_sn
     * [Desc]: Serial number provided by vendor (ASCII 16 byte char).
     * [Note]:
     */
    char *vendor_sn;

    /* [Prop]: Extended identifier
     * [Desc]: SFP:
     *         => None
     *
     *         QSFP:
     *         => This byte contained two information:
     *         (1) Power consumption class
     *         (2) CDR function present
     * [Note]: Bit description as below:
     *         [SFP]
     *           None
     *
     *         [QSFP]
     *         (1) Power consumption class:
     *             Class 1: 1.5W (Bit6-7 = 00:)
     *             Class 2: 2.0W (Bit6-7 = 01:)
     *             Class 3: 2.5W (Bit6-7 = 10:)
     *             Class 4: 3.5W (Bit6-7 = 11:)
     *             Class 5: 4.0W (Bit0-1 = 01:)
     *             Class 6: 4.5W (Bit0-1 = 10:)
     *             Class 7: 5.0W (Bit0-1 = 11:)
     *         (2) CDR function present:
     *             Bit2: 0 = No CDR in RX
     *                   1 = CDR present in RX
     *             Bit3: 0 = No CDR in TX
     *                   1 = CDR present in TX
     */
    uint8_t ext_id;

    /* [Prop]: br
     * [Desc]: Nominal bit rate, units of 100 MBits/sec.
     * [Note]: SFP:03h / QSFP:0Ch / QSPF+:0Dh
     *         has val: 0x67
     *         no val :
     */
    uint8_t br;

    /* [Prop]: extbr
     * [Desc]: Extended br (00h/222)
     * [Desc]: Nominal bit rate per channel, units of 250 Mbps.
     *         Complements. Byte 140. See Table 32A.
     */
    uint8_t extbr;

    /* [Prop]: len_sm
     * [Desc]: Length (single mode)-(100's)m
     * [Note]: This value specifies the link length that is supported by the transceiver
     *         while operating in compliance with the applicable standards using single mode
     *         fiber. The value is in units of 100 meters. A value of 255 means that the
     *         transceiver supports a link length greater than 25.4 km. A value of zero means
     *         that the transceiver does not support single mode fiber or that the length
     *         information must be determined from the transceiver technology.
     */
    int len_sm;

    /* [Prop]: len_smf
     * [Desc]: Length (single mode)-km
     * [Note]: Addition to EEPROM data from original GBIC definition. This value specifies
     *         the link length that is supported by the transceiver while operating in
     *         compliance with the applicable standards using single mode fiber. The value
     *         is in units of kilometers. A value of 255 means that the transceiver supports
     *         a link length greater than 254 km. A value of zero means that the transceiver
     *         does not support single mode fiber or that the length information must be
     *         determined from the transceiver technology.
     */
    int len_smf;

    /* [Prop]: len_om1
     * [Desc]: Link length supported for 62.5 um OM1 fiber, units of 10 m
     * [Note]: The value is in units of 10 meters. A value of 255 means that the
     *         transceiver supports a link length greater than 2.54 km. A value of
     *         zero means that the transceiver does not support 50 micron multi-mode
     *         fiber or that the length information must be determined from the transceiver
     *         technology.
     */
    int len_om1;

    /* [Prop]: len_om2
     * [Desc]: Link length supported for 50 um OM2 fiber, units of 10 m
     * [Note]: The value is in units of 10 meters. A value of 255 means that the
     *         transceiver supports a link length greater than 2.54 km. A value of
     *         zero means that the transceiver does not support 50 micron multi-mode
     *         fiber or that the length information must be determined from the transceiver
     *         technology.
     */
    int len_om2;

    /* [Prop]: len_om3
     * [Desc]: Length (50um, OM3)
     * [Note]: This value specifies link length that is supported by the transceiver while
     *         operating in compliance with applicable standards using 50 micron multimode
     *         OM3 [2000 MHz*km] fiber. The value is in units of 10 meters. A value of 255
     *         means that the transceiver supports a link length greater than 2.54 km. A value
     *         of zero means that the transceiver does not support 50 micron multimode fiber
     *         or that the length information must be determined from the transceiver technology.
     */
    int len_om3;

    /* [Prop]: len_om4
     * [Desc]: Length (50um, OM4) and Length (Active Cable or Copper)
     * [Note]: For optical links, this value specifies link length that is supported by the
     *         transceiver while operating in compliance with applicable standards using 50 micron
     *         multimode OM4 [4700 MHz*km] fiber. The value is in units of 10 meters. A value of
     *         255 means that the transceiver supports a link length greater than 2.54 km. A value
     *         of zero means that the transceiver does not support 50 micron multimode fiber or that
     *         the length information must be determined from the transceiver codes specified in Table 5-3.
     *
     *         For copper links, this value specifies minimum link length supported by the transceiver
     *         while operating in compliance with applicable standards using copper cable. For active
     *         cable, this value represents actual length. The value is in units of 1 meter. A value of 255
     *         means the transceiver supports a link length greater than 254 meters. A value of zero means
     *         the transceiver does not support copper or active cables or the length information must be
     *         determined from transceiver technology. Further information about cable design, equalization,
     *         and connectors is usually required to guarantee meeting a particular length requirement.
     */
    int len_om4;

    /* [Prop]: comp_rev
     * [Desc]: SFF spec revision compliance
     * [Note]: Indicates which revision of SFF SFF-8472 (SFP) / SFF-8636 (QSFP) the transceiver
     *         complies with. (unsigned integer)
     */
    uint8_t comp_rev;

    /* [Prop]: CDR
     * [Desc]: For transceivers with CDR capability, setting the CDR to ON engages the internal
     *         retiming function. Setting the CDR to OFF enables an internal bypassing mode ,which
     *         directs traffic around the internal CDR. (Reference: SFF-8636)
     * [Note]: value=0xff: ON.
     *         value=0x00: OFF.
     */
    uint8_t cdr;

    /* [Prop]: rate_id
     * [Desc]: Soft Rate Select 0(RX).
     * [Note]: 1. Addr: A0h / Offset: 13
     *         2. Value description:
     *            00h  Unspecified
     *            01h  SFF-8079 (4/2/1G Rate_Select & AS0/AS1)
     *            02h  SFF-8431 (8/4/2G Rx Rate_Select only)
     *            03h  Unspecified *
     *            04h  SFF-8431 (8/4/2G Tx Rate_Select only)
     *            05h  Unspecified *
     *            06h  SFF-8431 (8/4/2G Independent Rx & Tx Rate_select)
     *            07h  Unspecified *
     *            08h  FC-PI-5 (16/8/4G Rx Rate_select only) High=16G only, Low=8G/4G
     *            09h  Unspecified *
     *            0Ah  FC-PI-5 (16/8/4G Independent Rx, Tx Rate_select) High=16G only,
     *                 Low=8G/4G
     *            0Bh  Unspecified *
     *            0Ch  FC-PI-6 (32/16/8G Independent Rx, Tx Rate_Select)
     *                 High=32G only, Low = 16G/8G
     *            0Dh  Unspecified *
     *            0Eh  10/8G Rx and Tx Rate_Select controlling the operation or locking
     *                 modes of the internal signal conditioner, retimer or CDR, according
     *                 to the logic table defined in Table 10-2, High Bit Rate
     *                 (10G) =9.95-11.3 Gb/s; Low Bit Rate (8G) = 8.5 Gb/s. In this mode,
     *                 the default value of bit 110.3 (Soft Rate Select RS(0), Table 9-11)
     *                 and of bit 118.3 (Soft Rate Select RS(1), Table 10-1) is 1.
     *            0Fh  Unspecified *
     *            10h-FFh Unallocated
     */
    int rate_id;

    /* [Prop]: soft_rs0
     * [Desc]: Soft Rate Select 0(RX).
     * [Note]: 1. Writing '1' selects full bandwidth operation.
     *         2. This bit is "OR'd with the hard Rate_Select, AS(0) or RS(0) pin value.
     *         3. Default at power up is logic zero/low
     *         4. Addr: A2h / Offset: 110 / Bit: 3
     */
    uint8_t soft_rs0;

    /* [Prop]: soft_rs1
     * [Desc]: Soft Rate Select 1(TX).
     * [Note]: 1. Writing '1' selects full bandwidth TX operation.
     *         2. This bit is "OR'd with the hard Rate_Select, AS(1) or RS(1) pin value.
     *         3. Default at power up is logic zero/low
     *         4. Addr: A2h / Offset: 118 / Bit: 3
     */
    uint8_t soft_rs1;

    /* [Prop]: diag_type
     * [Desc]: DIAGNOSTIC MONITORING TYPE (A0h/92)
     * [Note]: Description in SFF-8472 as below:
     *         Bit7: Reserved for legacy diagnostic implementations. Must be '0' for compliance
     *               with this document.
     *         Bit6: Digital diagnostic monitoring implemented (described in this document).
     *               Must be '1' for compliance with this document.
     *         Bit5 Internally calibrated
     *         Bit4 Externally calibrated
     *         Bit3 Received power measurement type.0 = OMA, 1 = average power
     *         Bit2 Address change required see section above, "addressing modes"
     *         Bit1-0 Unallocated
     */
    uint8_t diag_type;

    /* [Prop]: curr_temp
     * [Desc]: Transceiver Current Temperature (A2h/96-97)
     * [Note]: 1. Dependent on diag_type.
     *         2. 96: High byte
     *         3. 97: Low byte
     *         4. This feature only for SFP
     */
    uint8_t curr_temp[2];

    /* [Prop]: curr_vol
     * [Desc]: Transceiver Current Voltage (SFP:A2h/108-109; QSFP:00h/22-23)
     * [Note]: 1. Dependent on diag_type.
     *         2. 98: High byte
     *         3. 99: Low byte
     *         4. This feature only for SFP
     *         5. Internally measured transceiver supply voltage. Represented
     *            as a 16 bit unsigned integer with the voltage defined as the
     *            full 16 bit value (0-65535) with LSB equal to 100 uVolt,
     *            yielding a total range of 0 to +6.55 Volts
     */
    uint8_t curr_voltage[2];

    /* [Prop]: curr_tx_bias
     * [Desc]: Transceiver TX Bias Current (SFP:A2h/100-101; QSFP:00h/26-27)
     * [Note]: 1. Dependent on diag_type.
     *         2. 100: High byte
     *         3. 101: Low byte
     *         4. This feature only for SFP
     *         5. Measured TX bias current in uA. Represented as a 16 bit unsigned
     *            integer with the current defined as the full 16 bit value (0-65535)
     *            with LSB equal to 2 uA, yielding a total range of 0 to 131 mA.
     *            Accuracy is vendor specific but must be better than 10% of the
     *            manufacturer's nominal value over specified operating temperature
     *            and voltage.
     */
    uint8_t curr_tx_bias[8];

    /* [Prop]: curr_tx_power
     * [Desc]: Transceiver TX Output Power (A2h/102-103)
     * [Note]: 1. Dependent on diag_type.
     *         2. 102: High byte
     *         3. 103: Low byte
     *         4. This feature only for SFP
     *         5. Measured TX output power in mW. Represented as a 16 bit unsigned
     *         integer with the power defined as the full 16 bit value (0-65535)
     *         with LSB equal to 0.1 uW, yielding a total range of 0 to 6.5535 mW
     *         (~ -40 to +8.2 dBm). Data is assumed to be based on measurement of
     *         laser monitor photodiode current. It is factory calibrated to absolute
     *         units using the most representative fiber output type. Accuracy is
     *         vendor specific but must be better than 3dB over specified temperature
     *          and voltage. Data is not valid when the transmitter is disabled.
     */
    uint8_t curr_tx_power[8];

    /* [Prop]: curr_tx_power
     * [Desc]: Transceiver TX Output Power (A2h/102-103)
     * [Note]: 1. Dependent on diag_type.
     *         2. 102: High byte
     *         3. 103: Low byte
     *         4. This feature only for SFP
     *         5. Measured RX received optical power in mW. Value can represent either
     *         average received power or OMA depending upon how bit 3 of byte 92 (A0h)
     *         is set. Represented as a 16 bit unsigned integer with the power defined
     *         as the full 16 bit value (0-65535) with LSB equal to 0.1 uW, yielding a
     *         total range of 0 to 6.5535 mW (~ -40 to +8.2 dBm). Absolute accuracy is
     *         dependent upon the exact optical wavelength. For the vendor specified
     *         wavelength, accuracy shall be better than 3dB over specified temperature
     *         and voltage.
     */
    uint8_t curr_rx_power[8];

    /* [Prop]: wavelength
     * [Desc]: Wavelength or Copper Cable Attenuation
     * [Note]: (Following is info from SFF-8636)
     *         For optical free side devices, this parameter identifies the nominal
     *         transmitter output wavelength at room temperature. This parameter is a
     *         16-bit hex value with Byte 186 as high order byte and Byte 187 as low
     *         order byte. The laser wavelength is equal to the 16-bit integer value
     *         divided by 20 in nm (units of 0.05 nm). This resolution should be adequate
     *         to cover all relevant wavelengths yet provide enough resolution for all
     *         expected DWDM applications. For accurate representation of controlled
     *         wavelength applications, this value should represent the center of the
     *         guaranteed wavelength range. If the free side device is identified as
     *         copper cable these registers will be used to define the cable attenuation.
     *         An indication of 0 dB attenuation refers to the case where the attenuation
     *         is not known or is unavailable.
     *         Byte 186 (00-FFh) is the copper cable attenuation at 2.5 GHz in units of 1 dB.
     *         Byte 187 (00-FFh) is the copper cable attenuation at 5.0 GHz in units of 1 dB.
     */
    uint8_t wavelength[2];

    /* [Prop]: Amplitude control
     * [Desc]: Amplitude control
     * [Note]: QSFP28  => SFF-8636 03H Byte-238/239
     */
    uint8_t rx_am[2];

    /* [Prop]: Emphasis control
     * [Desc]: Emphasis control
     * [Note]: SFP+/28 => SFF-8472 A2H Byte-115
     *         QSFP28  => SFF-8636 03H Byte-236/237
     */
    uint8_t rx_em[2];

    /* [Prop]: Soft Rx LOS
     * [Desc]: Soft Rx LOS which provide by transceiver
     * [Note]: (Following is info from SFF-8636)
     *         Byte 3:
     *          - Bit 0: L-Rx1 LOS
     *          - Bit 1: L-Rx2 LOS
     *          - Bit 2: L-Rx3 LOS
     *          - Bit 3: L-Rx4 LOS
     */
    uint8_t rx_los;

    /* [Prop]: Soft Tx Disable
     * [Desc]: Soft Tx Disable which provide by transceiver
     * [Note]: (Following is info from SFF-8636)
     *         Byte 86:
     *          - Bit 0: Tx1 Disable
     *          - Bit 1: Tx2 Disable
     *          - Bit 2: Tx3 Disable
     *          - Bit 3: Tx4 Disable
     */
    uint8_t tx_disable;

    /* [Prop]: Soft Tx Fault
     * [Desc]: Soft Tx Fault which provide by transceiver
     * [Note]: (Following is info from SFF-8636)
     *         Byte 86:
     *          - Bit 0: Tx1 Fault
     *          - Bit 1: Tx2 Fault
     *          - Bit 2: Tx3 Fault
     *          - Bit 3: Tx4 Fault
     */
    uint8_t tx_fault;

    /* [Prop]: Transceiver EQUALIZATION
     * [Desc]: Transceiver EQUALIZATION
     * [Note]: SFP+/28 => SFF-8472 A2H Byte-114
     *         QSFP28  => SFF-8636 03H Byte-234/235
     */
    uint8_t tx_eq[2];

    /* [Prop]: OPTION VALUES
     * [Desc]: The bits in the option field shall specify the options implemented in the transceiver.
     * [Note]: SFP+/28  => SFF-8472 A0H Byte-64/65
     *         QSFP+/28 => SFF-8636 00H Byte-193/195
     */
    uint8_t option[3];

    /* [Prop]: External PHY offset
     * [Desc]: It needs to be setup first if you want to access transceiver external phy.
     * [Note]: This feature dependent on transceiver.
     *         Currently, only 1G-RJ45 transceiver supported it.
     */
    uint8_t extphy_offset;

    /* ========== Object private property ==========
     */
    struct device       *transvr_dev_p;
    struct eeprom_map_s *eeprom_map_p;
    struct i2c_client   *i2c_client_p;
    struct ioexp_obj_s  *ioexp_obj_p;
    struct transvr_worker_s *worker_p;
    struct mutex lock;
    char swp_name[32];
    int auto_config;
    int auto_tx_disable;
    int chan_id;
    int chipset_type;
    int curr_page;
    int info;
    int ioexp_virt_offset;
    int lane_id[8];
    int layout;
    int mode;
    int retry;
    int state;
    int temp;
    int type;

    /* ========== Object public functions ==========
     */
    int  (*get_id)(struct transvr_obj_s *self);
    int  (*get_ext_id)(struct transvr_obj_s *self);
    int  (*get_connector)(struct transvr_obj_s *self);
    int  (*get_vendor_name)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_vendor_pn)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_vendor_rev)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_vendor_sn)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_power_cls)(struct transvr_obj_s *self);
    int  (*get_br)(struct transvr_obj_s *self);
    int  (*get_len_sm)(struct transvr_obj_s *self);
    int  (*get_len_smf)(struct transvr_obj_s *self);
    int  (*get_len_om1)(struct transvr_obj_s *self);
    int  (*get_len_om2)(struct transvr_obj_s *self);
    int  (*get_len_om3)(struct transvr_obj_s *self);
    int  (*get_len_om4)(struct transvr_obj_s *self);
    int  (*get_comp_rev)(struct transvr_obj_s *self);
    int  (*get_comp_eth_1)(struct transvr_obj_s *self);
    int  (*get_comp_eth_10)(struct transvr_obj_s *self);
    int  (*get_comp_eth_10_40)(struct transvr_obj_s *self);
    int  (*get_comp_extend)(struct transvr_obj_s *self);
    int  (*get_cdr)(struct transvr_obj_s *self);
    int  (*get_rate_id)(struct transvr_obj_s *self);
    int  (*get_soft_rs0)(struct transvr_obj_s *self);
    int  (*get_soft_rs1)(struct transvr_obj_s *self);
    int  (*get_info)(struct transvr_obj_s *self);
    int  (*get_if_type)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_if_speed)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_if_lane)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_curr_temp)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_curr_vol)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_soft_rx_los)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_soft_tx_disable)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_soft_tx_fault)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_auto_tx_disable)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_tx_bias)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_tx_power)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_rx_power)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_tx_eq)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_rx_am)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_rx_em)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_wavelength)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_extphy_offset)(struct transvr_obj_s *self, char *buf_p);
    int  (*get_extphy_reg)(struct transvr_obj_s *self, char *buf_p);
    int  (*set_cdr)(struct transvr_obj_s *self, int input_val);
    int  (*set_soft_rs0)(struct transvr_obj_s *self, int input_val);
    int  (*set_soft_rs1)(struct transvr_obj_s *self, int input_val);
    int  (*set_soft_tx_disable)(struct transvr_obj_s *self, int input_val);
    int  (*set_auto_tx_disable)(struct transvr_obj_s *self, int input_val);
    int  (*set_tx_eq)(struct transvr_obj_s *self, int input_val);
    int  (*set_rx_am)(struct transvr_obj_s *self, int input_val);
    int  (*set_rx_em)(struct transvr_obj_s *self, int input_val);
    int  (*set_extphy_offset)(struct transvr_obj_s *self, int input_val);
    int  (*set_extphy_reg)(struct transvr_obj_s *self, int input_val);

    /* ========== Object private functions ==========
     */
    int (*init)(struct transvr_obj_s *self);
    int (*clean)(struct transvr_obj_s *self);
    int (*check)(struct transvr_obj_s *self);
    int (*update_all)(struct transvr_obj_s *self, int show_err);
    int (*fsm_4_direct)(struct transvr_obj_s* self, char *caller_name);
    int (*fsm_4_polling)(struct transvr_obj_s* self, char *caller_name);
    int (*send_uevent)(struct transvr_obj_s* self, enum kobject_action u_action);
    int (*dump_all)(struct transvr_obj_s* self);
};


/* For AVL Mapping */
struct transvr_avl_s {
    char vendor_name[32];
    char vendor_pn[32];
    int (*init)(struct transvr_obj_s *self);
};


/* Worker for long term task of transceiver */
struct transvr_worker_s {
    /* Task Parameter */
    struct transvr_obj_s *transvr_p;
    struct transvr_worker_s *next_p;
    struct transvr_worker_s *pre_p;
    unsigned long trigger_time;
    char func_name[64];
    int retry;
    int state;

    /* Task private data */
    void *p_data;

    /* Call back function */
    int (*main_task)(struct transvr_worker_s *task);
    int (*post_task)(struct transvr_worker_s *task);
};


struct transvr_obj_s *
create_transvr_obj(char *swp_name,
                   int chan_id,
                   struct ioexp_obj_s *ioexp_obj_p,
                   int ioexp_virt_offset,
                   int transvr_type,
                   int chipset_type,
                   int run_mode);

void lock_transvr_obj(struct transvr_obj_s *self);
void unlock_transvr_obj(struct transvr_obj_s *self);
int isolate_transvr_obj(struct transvr_obj_s *self);

int resync_channel_tier_2(struct transvr_obj_s *self);

void alarm_msg_2_user(struct transvr_obj_s *self, char *emsg);

#endif /* TRANSCEIVER_H */







