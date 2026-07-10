#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <linux/types.h>

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

/* Transceiver mode define */
#define TRANSVR_MODE_DIRECT             (21000)

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
#define STR_TRANSVR_QSFP                "QSFP"
#define STR_TRANSVR_QSFP_PLUS           "QSFP+"
#define STR_TRANSVR_QSFP28              "QSFP28"

/* BCM chip type define */
#define BCM_CHIP_TYPE_TOMAHAWK          (31002)  /* Redwood, Cypress */

/* Info from transceiver EEPROM */
struct eeprom_map_s {
    int addr_rx_los;       int page_rx_los;       int offset_rx_los;       int length_rx_los;
    int addr_tx_disable;   int page_tx_disable;   int offset_tx_disable;   int length_tx_disable;
    int addr_tx_fault;     int page_tx_fault;     int offset_tx_fault;     int length_tx_fault;
};

/* Class of transceiver object */
struct transvr_obj_s {
    /* ========== Object private property ==========
     */
    struct device       *transvr_dev_p;
    struct eeprom_map_s *eeprom_map_p;
    struct i2c_client   *i2c_client_p;
    struct ioexp_obj_s  *ioexp_obj_p;
    struct mutex lock;
    char swp_name[32];
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

    /* ========== Object private functions ==========
     */
    int (*init)(struct transvr_obj_s *self);
    int (*update_all)(struct transvr_obj_s *self, int show_err);
    int (*fsm_4_direct)(struct transvr_obj_s* self, char *caller_name);
};


/* For AVL Mapping */
struct transvr_avl_s {
    char vendor_name[32];
    char vendor_pn[32];
    int (*init)(struct transvr_obj_s *self);
};

struct transvr_obj_s *
create_transvr_obj(char *swp_name,
                   int chan_id,
                   struct ioexp_obj_s *ioexp_obj_p,
                   int ioexp_virt_offset,
                   int transvr_type,
                   int chipset_type,
                   int run_mode);

void alarm_msg_2_user(struct transvr_obj_s *self, char *emsg);

#endif /* TRANSCEIVER_H */




