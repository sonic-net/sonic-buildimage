#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include "io_expander.h"
#include "transceiver.h"


/* ========== Register EEPROM address mapping ==========
 */
struct eeprom_map_s eeprom_map_qsfp = {
    .addr_rx_los       =0x50,  .page_rx_los       =-1,  .offset_rx_los       =3,    .length_rx_los       =1,
    .addr_tx_disable   =0x50,  .page_tx_disable   =-1,  .offset_tx_disable   =86,   .length_tx_disable   =1,
    .addr_tx_fault     =0x50,  .page_tx_fault     =-1,  .offset_tx_fault     =4,    .length_tx_fault     =1,
};

struct eeprom_map_s eeprom_map_qsfp28 = {
    .addr_rx_los       =0x50,  .page_rx_los       =-1,  .offset_rx_los       =3,    .length_rx_los       =1,
    .addr_tx_disable   =0x50,  .page_tx_disable   =-1,  .offset_tx_disable   =86,   .length_tx_disable   =1,
    .addr_tx_fault     =0x50,  .page_tx_fault     =-1,  .offset_tx_fault     =4,    .length_tx_fault     =1,
};


/* ========== Utility Functions ==========
 */
void
alarm_msg_2_user(struct transvr_obj_s *self,
                 char *emsg) {

    SWPS_ERR("%s on %s.\n", emsg, self->swp_name);
}


/* ========== Private functions ==========
 */
static int
_reload_transvr_obj(struct transvr_obj_s *self,int new_type);

static int
reload_transvr_obj(struct transvr_obj_s *self,int new_type);

static int
_transvr_init_handler(struct transvr_obj_s *self);

static void
_transvr_clean_retry(struct transvr_obj_s *self) {
    self->retry = 0;
}


static int
_transvr_handle_retry(struct transvr_obj_s *self, int retry) {
    /* Return: 0: keep retry
     *        -1: stop retry
     */
    if (self->retry == 0) {
        self->retry = retry;
    }
    self->retry -= 1;
    if (self->retry <= 0) {
        _transvr_clean_retry(self);
        return -1;
    }
    return 0;
}

static int
_common_setup_page(struct transvr_obj_s *self,
                   int addr,
                   int page,
                   int offset,
                   int len,
                   int show_e) {
    /* return:
     *    0 : OK
     *   -1 : EEPROM settings incorrect
     *   -2 : I2C R/W failure
     *   -3 : Undefined case
     */
    int retval = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;

    /* Check */
    if ((addr < 0) || (offset < 0) || (len < 0)) {
        emsg   = "EEPROM settings incorrect";
        retval = -1;
        goto err_common_setup_page;
    }
    /* Case1: continue access */
    if ((self->i2c_client_p->addr == addr) &&
        (self->curr_page == page)) {
        return 0;
    }
    self->i2c_client_p->addr = addr;
    /* Case2: select lower page */
    if (page == -1) {
        self->curr_page = page;
        return 0;
    }
    /* Case3: select upper page */
    if (page >= 0) {
        goto upper_common_setup_page;
    }
    /* Unexpected case */
    show_e = 1;
    emsg   = "Unexpected case";
    retval = -3;
    goto err_common_setup_page;

upper_common_setup_page:
    if (i2c_smbus_write_byte_data(self->i2c_client_p,
                                  VAL_TRANSVR_PAGE_SELECT_OFFSET,
                                  page) < 0) {
        emsg   = "I2C R/W failure";
        retval = -2;
        goto err_common_setup_page;
    }
    self->curr_page = page;
    mdelay(VAL_TRANSVR_PAGE_SELECT_DELAY);
    return 0;

err_common_setup_page:
    if (show_e) {
        SWPS_INFO("%s: %s", __func__, emsg);
        SWPS_INFO("%s: <addr>:0x%02x <page>:%d <offs>:%d <len>:%d\n",
                __func__, addr, page, offset, len);
    }
    return retval;
}

/* ========== Object functions for Final State Machine ==========
 */
int
is_plugged(struct transvr_obj_s *self){

    int  limit    = 63;
    int  present  = DEBUG_TRANSVR_INT_VAL;
    char emsg[64] = DEBUG_TRANSVR_STR_VAL;
    struct ioexp_obj_s *ioexp_p = self->ioexp_obj_p;

    if (!ioexp_p) {
        snprintf(emsg, limit, "ioexp_p is null!");
        goto err_is_plugged_1;
    }
    present = ioexp_p->get_present(ioexp_p, self->ioexp_virt_offset);
    switch (present){
        case 0:
            return 1;
        case 1:
            return 0;
        case ERR_IOEXP_UNINIT:
            snprintf(emsg, limit, "ioexp_p not ready!");
            goto err_is_plugged_1;
        default:
            if (ioexp_p->state == STATE_IOEXP_INIT){
                snprintf(emsg, limit, "ioexp_p not ready!");
                goto err_is_plugged_1;
            }
            break;
    }
    SWPS_INFO("%s: Exception case! <pres>:%d <istate>:%d\n",
              __func__, present, ioexp_p->state);
    return 0;

err_is_plugged_1:
    SWPS_DEBUG("%s: %s\n", __func__, emsg);
    return 0;
}


static int
detect_transvr_type(struct transvr_obj_s* self){

    int type = TRANSVR_TYPE_ERROR;

    self->i2c_client_p->addr = VAL_TRANSVR_COMID_ARREESS;
    type = i2c_smbus_read_byte_data(self->i2c_client_p,
                                    VAL_TRANSVR_COMID_OFFSET);

    /* Case: 1. Wait transceiver I2C module.
     *       2. Transceiver I2C module failure.
     * Note: 1. SFF allow maximum transceiver initial time is 2 second. So, there
     *          are exist some case that we need to wait transceiver.
     *          For these case, we keeps status on "TRANSVR_TYPE_UNPLUGGED", than
     *          state machine will keep trace with it.
     *       2. There exist some I2C failure case we need to handle. Such as user
     *          insert the failure transceiver, or any reason cause it abnormal.
     */
    if (type < 0){
        switch (type) {
            case -EIO:
                SWPS_DEBUG("%s: %s smbus return:-5 (I/O error)\n",
                        __func__, self->swp_name);
                return TRANSVR_TYPE_UNPLUGGED;
            case -ENXIO:
                SWPS_DEBUG("%s: %s smbus return:-6 (No such device or address)\n",
                        __func__, self->swp_name);
                return TRANSVR_TYPE_UNPLUGGED;
            default:
                break;
        }
        SWPS_INFO("%s: %s unexpected smbus return:%d \n",
                __func__, self->swp_name, type);
        return TRANSVR_TYPE_ERROR;
    }    
    /* Identify valid transceiver type */
    switch (type){
        case TRANSVR_TYPE_SFP:
        case TRANSVR_TYPE_QSFP:
        case TRANSVR_TYPE_QSFP_PLUS:
        case TRANSVR_TYPE_QSFP_28:
            break;
        case TRANSVR_TYPE_UNKNOW_1:
        case TRANSVR_TYPE_UNKNOW_2:
            type = TRANSVR_TYPE_UNKNOW_2;
            break;
        default:
            SWPS_DEBUG("%s: unknow type:0x%02x \n", __func__, type);
            type = TRANSVR_TYPE_ERROR;
            break;
    }
    return type;
}


static int
detect_transvr_state(struct transvr_obj_s *self,
                     int result[2]){
    /* [return]                  [result-0]                  [result-1]
     *  0                        STATE_TRANSVR_CONNECTED     TRANSVR_TYPE_FAKE
     *  0                        STATE_TRANSVR_DISCONNECTED  TRANSVR_TYPE_UNPLUGGED
     *  0                        STATE_TRANSVR_ISOLATED      TRANSVR_TYPE_ERROR
     *  0                        STATE_TRANSVR_INIT          <NEW_TYPE>/<OLD_TYPE>
     *  0                        STATE_TRANSVR_SWAPPED       <NEW_TYPE>
     *  0                        STATE_TRANSVR_CONNECTED     <OLD_TYPE>
     *  ERR_TRNASVR_BE_ISOLATED  STATE_TRANSVR_ISOLATED      TRANSVR_TYPE_ERROR  <Isolated>
     *  ERR_TRANSVR_I2C_CRASH    STATE_TRANSVR_UNEXCEPTED    TRANSVR_TYPE_ERROR  <New event>
     *  ERR_TRANSVR_UNEXCPT      STATE_TRANSVR_UNEXCEPTED    TRANSVR_TYPE_UNKNOW_1/2
     */
    result[0] = STATE_TRANSVR_UNEXCEPTED;  /* For return state */
    result[1] = TRANSVR_TYPE_ERROR;        /* For return type  */

    /* Case1: Fake type */
    if (self->type == TRANSVR_TYPE_FAKE){
        result[0] = STATE_TRANSVR_CONNECTED;
        result[1] = TRANSVR_TYPE_FAKE;
        return 0;
    }
    /* Case2: Transceiver unplugged */
    if (!is_plugged(self)){
        result[0] = STATE_TRANSVR_DISCONNECTED;
        result[1] = TRANSVR_TYPE_UNPLUGGED;
        return 0;
    }
    /* Case3: Transceiver be isolated */
    if (self->state == STATE_TRANSVR_ISOLATED){
        result[0] = STATE_TRANSVR_ISOLATED;
        result[1] = TRANSVR_TYPE_ERROR;
        return ERR_TRNASVR_BE_ISOLATED;
    }
    /* Case4: Transceiver plugged */
    result[1] = detect_transvr_type(self);
    /* Case4.1: I2C topology crash
     * Note   : There are some I2C issues cause by transceiver/cables.
     *          We need to check topology status when user insert it.
     *          But in this step, we can't not ensure this is the issues
     *          port. So, it return the ERR_TRANSVR_I2C_CRASH, then upper
     *          layer will diagnostic I2C topology.
     */
    if (check_channel_tier_1() < 0) {
        SWPS_INFO("%s: %s detect I2C crash <obj-state>:%d\n",
                __func__, self->swp_name, self->state);
        result[0] = STATE_TRANSVR_UNEXCEPTED;
        result[1] = TRANSVR_TYPE_ERROR;
        return ERR_TRANSVR_I2C_CRASH;
    }
    /* Case4.2: System initial not ready,
     * Note   : Sometime i2c channel or transceiver EEPROM will delay that will
     *          cause system in inconsistent state between EEPROM and IOEXP.
     *          In this case, SWP transceiver object keep state at LINK_DOWN
     *          to wait system ready.
     *          By the way, State Machine will handle these case.
     */
    if (result[1] == TRANSVR_TYPE_UNPLUGGED){
        result[0] = STATE_TRANSVR_DISCONNECTED;
        return 0;
    }
    /* Case4.3: Error transceiver type */
    if (result[1] == TRANSVR_TYPE_ERROR){
        result[0] = STATE_TRANSVR_ISOLATED;
        SWPS_INFO("%s: %s detect error type\n", __func__, self->swp_name);
        alarm_msg_2_user(self, "detected transceiver/cables not meet SFF standard!");
        return ERR_TRNASVR_BE_ISOLATED;
    }
    /* Case3.3: Unknow transceiver type */
    if ((result[1] == TRANSVR_TYPE_UNKNOW_1) ||
        (result[1] == TRANSVR_TYPE_UNKNOW_2) ){
        result[0] = STATE_TRANSVR_UNEXCEPTED;
        return ERR_TRANSVR_UNEXCPT;
    }
    /* Case3.4: During initial process */
    if (self->state == STATE_TRANSVR_INIT){
        result[0] = STATE_TRANSVR_INIT;
        return 0;
    }
    /* Case3.5: Transceiver be swapped */
    if (self->type != result[1]){
        result[0] = STATE_TRANSVR_SWAPPED;
        return 0;
    }
    /* Case3.6: Link up state */
    result[0] = STATE_TRANSVR_CONNECTED;
    return 0;
}
int
common_fsm_4_direct_mode(struct transvr_obj_s* self,
                         char *caller_name){

    int err;
    int detect_result[2];
    int current_state = STATE_TRANSVR_UNEXCEPTED;
    int current_type  = TRANSVR_TYPE_ERROR;

    if (self->state == STATE_TRANSVR_NEW) {
        if (_transvr_init_handler(self) < 0){
            return ERR_TRANSVR_INIT_FAIL;
        }
    }
    err = detect_transvr_state(self, detect_result);
    if (err < 0) {
        return err;
    }
    /* In Direct mode, driver only detect transceiver when user call driver interface
     * which on sysfs. So it only need consider the state of Transceiver.
     */
    current_state = detect_result[0];
    current_type  = detect_result[1];

    switch (current_state){

        case STATE_TRANSVR_DISCONNECTED:   /* Transceiver is not plugged */
            self->state = current_state;
            self->type  = current_type;
            return ERR_TRANSVR_UNPLUGGED;

        case STATE_TRANSVR_INIT:           /* Transceiver is plugged, system not ready */
            return ERR_TRANSVR_UNINIT;

        case STATE_TRANSVR_ISOLATED:       /* Transceiver is plugged, but has some issues */
            return ERR_TRNASVR_BE_ISOLATED;

        case STATE_TRANSVR_CONNECTED:      /* Transceiver is plugged, system is ready */
            self->state = current_state;
            self->type  = current_type;
            return 0;

        case STATE_TRANSVR_SWAPPED:        /* Transceiver is plugged, system detect user changed */
            self->type = current_type;
            if (reload_transvr_obj(self, current_type) < 0){
                self->state = STATE_TRANSVR_UNEXCEPTED;
                return ERR_TRANSVR_UNEXCPT;
            }
            self->state = current_state;
            return 0;

        case STATE_TRANSVR_UNEXCEPTED:     /* Transceiver type or state is unexpected case */
            self->state = STATE_TRANSVR_UNEXCEPTED;
            self->type  = TRANSVR_TYPE_ERROR;
            return ERR_TRANSVR_UNEXCPT;

        default:
            SWPS_INFO("%s: state:%d not in define.\n", __func__, current_state);
            break;
    }
    return ERR_TRANSVR_UNEXCPT;
}

int
fake_fsm_4_direct_mode(struct transvr_obj_s* self,
                       char *caller_name){
    self->state = STATE_TRANSVR_CONNECTED;
    self->type  = TRANSVR_TYPE_FAKE;
    return 0;
}

/* ========== Object Initial handler ==========
 */
static int
_is_transvr_valid(struct transvr_obj_s *self,
                  int type,
                  int state) {
    /* [Return]
     *   0                        : OK, inserted
     *   EVENT_TRANSVR_INIT_DOWN  : OK, removed
     *   EVENT_TRANSVR_INIT_FAIL  : Outside error, type doesn't supported
     *   EVENT_TRANSVR_EXCEP_INIT : Internal error, state undefined
     */
    switch (type) {
        case TRANSVR_TYPE_SFP:
        case TRANSVR_TYPE_QSFP:
        case TRANSVR_TYPE_QSFP_PLUS:
        case TRANSVR_TYPE_QSFP_28:
        case TRANSVR_TYPE_UNPLUGGED:
        case TRANSVR_TYPE_FAKE:
            break;
        default:
            SWPS_INFO("detect undefined type:0x%02x on %s\n",
                      type, self->swp_name);
            return EVENT_TRANSVR_INIT_FAIL;
    }
    switch (state) {
        case STATE_TRANSVR_DISCONNECTED:
            return EVENT_TRANSVR_INIT_DOWN;
        case STATE_TRANSVR_INIT:
        case STATE_TRANSVR_CONNECTED:
        case STATE_TRANSVR_SWAPPED:
            break;
        default:
            SWPS_INFO("detect undefined state:%d on %s\n",
                      state, self->swp_name);
            return EVENT_TRANSVR_EXCEP_INIT;
    }
    return 0;
}


static int
_is_transvr_hw_ready(struct transvr_obj_s *self,
                     int type){
    /* [Return]
     *   EVENT_TRANSVR_TASK_DONE : Ready
     *   EVENT_TRANSVR_TASK_WAIT : Not ready
     *   EVENT_TRANSVR_INIT_FAIL : Error
     */
    int addr   = DEBUG_TRANSVR_INT_VAL;
    int page   = DEBUG_TRANSVR_INT_VAL;
    int offs   = DEBUG_TRANSVR_INT_VAL;
    int bit    = DEBUG_TRANSVR_INT_VAL;
    int ready  = DEBUG_TRANSVR_INT_VAL;
    int err    = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;
    uint8_t ab_val = DEBUG_TRANSVR_HEX_VAL;

    switch (type) {
        case TRANSVR_TYPE_SFP:
            addr   = VAL_TRANSVR_8472_READY_ADDR;
            page   = VAL_TRANSVR_8472_READY_PAGE;
            offs   = VAL_TRANSVR_8472_READY_OFFSET;
            bit    = VAL_TRANSVR_8472_READY_BIT;
            ready  = VAL_TRANSVR_8472_READY_VALUE;
            ab_val = VAL_TRANSVR_8472_READY_ABNORMAL;
            break;

        case TRANSVR_TYPE_QSFP:
        case TRANSVR_TYPE_QSFP_PLUS:
        case TRANSVR_TYPE_QSFP_28:
            addr   = VAL_TRANSVR_8436_READY_ADDR;
            page   = VAL_TRANSVR_8436_READY_PAGE;
            offs   = VAL_TRANSVR_8436_READY_OFFSET;
            bit    = VAL_TRANSVR_8436_READY_BIT;
            ready  = VAL_TRANSVR_8436_READY_VALUE;
            ab_val = VAL_TRANSVR_8436_READY_ABNORMAL;
            break;

        case TRANSVR_TYPE_UNPLUGGED:
        case TRANSVR_TYPE_FAKE:
            return EVENT_TRANSVR_TASK_DONE;

        default:
            emsg = "unexpected case";
            goto err_is_transvr_hw_ready;
    }
    /* Select target page */
    err = _common_setup_page(self, addr, page, offs, 1, 0);
    if (err < 0) {
        emsg = "setup page fail";
        goto err_is_transvr_hw_ready;
    }
    /* Check feature supported
     * [Note]
     *   Some of transceiver/cables doesn't support "Status Indicators"
     *   (ex:DAC, RJ45 copper SFP ...etc). In these case, we bypass the
     *   step of checking Status Indicators, then state machine will take
     *   the following handle procedure.
     */
    err = i2c_smbus_read_byte_data(self->i2c_client_p,
                                   VAL_TRANSVR_COMID_OFFSET);
    if (err < 0) {
        emsg = "doesn't support Status Indicators";
        goto bypass_is_transvr_hw_ready;
    }
    /* Filter abnormal case */
    if (err == ab_val) {
        emsg = "detect using unusual definition.";
        goto bypass_is_transvr_hw_ready;
    }
    /* Get Status Indicators */
    err = i2c_smbus_read_byte_data(self->i2c_client_p, offs);
    if (err < 0) {
        emsg = "detect current value fail";
        goto err_is_transvr_hw_ready;
    }
    if ((err & (1<<bit)) == ready) {
        return EVENT_TRANSVR_TASK_DONE;
    }
    return EVENT_TRANSVR_TASK_WAIT;

bypass_is_transvr_hw_ready:
    SWPS_DEBUG("%s: %s <type>:%d\n", __func__, emsg, type);
    return EVENT_TRANSVR_TASK_DONE;

err_is_transvr_hw_ready:
    SWPS_DEBUG("%s: %s <type>:%d\n", __func__, emsg, type);
    return EVENT_TRANSVR_INIT_FAIL;
}

static int
_transvr_init_handler(struct transvr_obj_s *self){

    int detect[2];
    int d_state   = STATE_TRANSVR_UNEXCEPTED;
    int d_type    = TRANSVR_TYPE_ERROR;
    int result    = ERR_TRANSVR_UNINIT;
    int retry     = 6;  /* (6+1) x 0.3 = 2.1s > spec:2.0s */
    int elimit    = 63;
    char emsg[64] = DEBUG_TRANSVR_STR_VAL;

    /* Clean and check callback */
    self->state = STATE_TRANSVR_INIT;
    if (self->init == NULL) {
        snprintf(emsg, elimit, "init() is null");
        goto initer_err_case_unexcept_0;
    }
    /* Detect transceiver information */
    result = detect_transvr_state(self, detect);
    if (result < 0) {
        snprintf(emsg, elimit, "detect_transvr_state() fail");
        switch (result) {
            case ERR_TRANSVR_I2C_CRASH:
                goto initer_err_case_i2c_ceash;
            case ERR_TRNASVR_BE_ISOLATED:
                goto initer_err_case_be_isolated;

            case ERR_TRANSVR_UNEXCPT:
            default:
                break;
        }
        goto initer_err_case_retry_1;
    }
    d_state = detect[0];
    d_type  = detect[1];

    /* Verify transceiver type and state */
    switch (_is_transvr_valid(self, d_type, d_state)) {
        case 0:
            break;
        case EVENT_TRANSVR_INIT_DOWN:
            goto initer_ok_case_down;;
        case EVENT_TRANSVR_INIT_FAIL:
            snprintf(emsg, elimit, "transceiver type doesn't support");
            goto initer_err_case_alarm_to_user;
        case EVENT_TRANSVR_EXCEP_INIT:
        default:
           	goto initer_err_case_unexcept_0;
    }

    /* Handle reload case */
    if (self->type != d_type){
        /* This is the protect mechanism. Normally, This case will not happen.
         * When State machine detect swap event during initial, It will trigger
         * reload function to ensure type correct. */
        if (_reload_transvr_obj(self, d_type) < 0){
            snprintf(emsg, elimit, "reload object fail");
            goto initer_err_case_unexcept_0;
        }
    }

    /* Check transceiver HW initial ready */
    switch (_is_transvr_hw_ready(self, d_type)) {
        case EVENT_TRANSVR_TASK_DONE:
            break;
        case EVENT_TRANSVR_TASK_WAIT:
            goto initer_err_case_retry_1;
        case EVENT_TRANSVR_INIT_FAIL:
        default:
            goto initer_err_case_unexcept_0;
    }

    /* Try to update all and check */
    if (self->update_all(self, 1) < 0){
        /* For some transceiver, EEPROME has lag issues during initial stage.
         * In this case, we set status back to STATE_TRANSVR_NEW, than it will
         * be checked in next polling cycle. */
        goto initer_err_case_retry_1;
    }

    /* Execute init() call back */
    result = self->init(self);
    switch (result) {
        case EVENT_TRANSVR_TASK_DONE:
            break;
        case EVENT_TRANSVR_TASK_WAIT:
            goto initer_ok_case_wait;

        default:
            snprintf(emsg, elimit, "undefined init() return:%d\n", result);
            goto initer_err_case_unexcept_0;
    }
    goto initer_ok_case_up;


initer_ok_case_wait:
    return EVENT_TRANSVR_TASK_WAIT;

initer_ok_case_up:
    self->state = STATE_TRANSVR_CONNECTED;
    self->temp  = 0;
    return EVENT_TRANSVR_INIT_UP;

initer_ok_case_down:
    self->temp  = 0;
    self->state = STATE_TRANSVR_DISCONNECTED;
    return EVENT_TRANSVR_INIT_DOWN;

initer_err_case_i2c_ceash:
    SWPS_DEBUG("%s: %s <port>:%s <case>:I2C crash\n",
               __func__, emsg, self->swp_name);
    self->state = STATE_TRANSVR_UNEXCEPTED;
    return EVENT_TRANSVR_I2C_CRASH;

initer_err_case_be_isolated:
    SWPS_DEBUG("%s: %s <port>:%s <case>:isolated\n",
               __func__, emsg, self->swp_name);
    self->state = STATE_TRANSVR_ISOLATED;
    return EVENT_TRANSVR_EXCEP_ISOLATED;

initer_err_case_retry_1:
    SWPS_DEBUG("%s: %s <port>:%s <case>:retry\n",
               __func__, emsg, self->swp_name);
    if (_transvr_handle_retry(self, retry) == 0) {
        self->state = STATE_TRANSVR_NEW;
        return EVENT_TRANSVR_INIT_REINIT;
    }
    goto initer_err_case_alarm_to_user;

initer_err_case_unexcept_0:
    self->state = STATE_TRANSVR_UNEXCEPTED;
    return EVENT_TRANSVR_INIT_FAIL;

initer_err_case_alarm_to_user:
    SWPS_DEBUG("%s: %s <port>:%s <case>:alarm_to_user\n",
               __func__, emsg, self->swp_name);
    self->state = STATE_TRANSVR_UNEXCEPTED;
    alarm_msg_2_user(self, "detected transceiver/cables not meet SFF standard");
    return EVENT_TRANSVR_INIT_FAIL;
}

static int
setup_transvr_private_cb(struct transvr_obj_s *self,
                         int transvr_type){
    switch (transvr_type){
        case TRANSVR_TYPE_SFP:
            self->fsm_4_direct = common_fsm_4_direct_mode;
            return 0;

        case TRANSVR_TYPE_QSFP:
        case TRANSVR_TYPE_QSFP_PLUS:
            self->fsm_4_direct = common_fsm_4_direct_mode;
            return 0;

        case TRANSVR_TYPE_QSFP_28:
            self->fsm_4_direct = common_fsm_4_direct_mode;
            return 0;

        case TRANSVR_TYPE_FAKE:
            self->fsm_4_direct = fake_fsm_4_direct_mode;
            return 0;

        default:
            break;
    }
    SWPS_WARN("%s: Detect non-defined type:%d\n", __func__, transvr_type);
    return ERR_TRANSVR_UNEXCPT;
}


static struct eeprom_map_s *
get_eeprom_map(int transvr_type){

    switch (transvr_type){
        case TRANSVR_TYPE_QSFP:
        case TRANSVR_TYPE_QSFP_PLUS:
            return &eeprom_map_qsfp;
        case TRANSVR_TYPE_QSFP_28:
            return &eeprom_map_qsfp28;

        default:
            break;
    }
    SWPS_WARN("%s: Detect non-defined type:%d\n", __func__, transvr_type);
    return NULL;
}


static int
setup_transvr_ssize_attr(char *swp_name,
                         struct transvr_obj_s *self,
                         struct eeprom_map_s  *map_p,
                         struct ioexp_obj_s   *ioexp_obj_p,
                         int ioexp_virt_offset,
                         int transvr_type,
                         int chipset_type,
                         int chan_id,
                         int run_mode){
    switch (run_mode){
        case TRANSVR_MODE_DIRECT:  /* Direct access device mode */
            self->mode = run_mode;
            break;
        default:
            SWPS_ERR("%s: non-defined run_mode:%d\n",
                    __func__, run_mode);
            self->mode = DEBUG_TRANSVR_INT_VAL;
            return -1;
    }
    self->eeprom_map_p      = map_p;
    self->ioexp_obj_p       = ioexp_obj_p;
    self->ioexp_virt_offset = ioexp_virt_offset;
    self->chan_id           = chan_id;
    self->layout            = transvr_type;
    self->type              = transvr_type;
    self->chipset_type      = chipset_type;
    self->state             = STATE_TRANSVR_NEW;
    self->info              = STATE_TRANSVR_NEW;
    self->auto_tx_disable   = VAL_TRANSVR_FUNCTION_DISABLE;
    strncpy(self->swp_name, swp_name, 32);
    mutex_init(&self->lock);
    return 0;
}



static int
setup_i2c_client(struct transvr_obj_s *self){

    struct i2c_adapter *adap  = NULL;
    struct i2c_client *client = NULL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    adap = i2c_get_adapter(self->chan_id);
    if(!adap){
        snprintf(err_msg, sizeof(err_msg),
                "can not get adap:%d", self->chan_id);
        goto err_setup_i2c_client;
    }
    client = kzalloc(sizeof(*client), GFP_KERNEL);
    if (!client){
        snprintf(err_msg, sizeof(err_msg),
                "can not kzalloc client:%d", self->chan_id);
        goto err_setup_i2c_client;
    }
    client->adapter = adap;
    self->i2c_client_p = client;
    self->i2c_client_p->addr = VAL_TRANSVR_COMID_ARREESS;
    return 0;

err_setup_i2c_client:
    SWPS_ERR("%s: %s\n", __func__, err_msg);
    return ERR_TRANSVR_UNEXCPT;
}


struct transvr_obj_s *
create_transvr_obj(char *swp_name,
                   int chan_id,
                   struct ioexp_obj_s *ioexp_obj_p,
                   int ioexp_virt_offset,
                   int transvr_type,
                   int chipset_type,
                   int run_mode){

    struct transvr_obj_s *result_p;
    struct eeprom_map_s  *map_p;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    /* Allocate transceiver object */
    map_p = get_eeprom_map(transvr_type);
    if (!map_p){
        snprintf(err_msg, sizeof(err_msg),
                "Invalid transvr_type:%d", transvr_type);
        goto err_create_transvr_fail;
    }
    result_p = kzalloc(sizeof(*result_p), GFP_KERNEL);
    if (!result_p){
        snprintf(err_msg, sizeof(err_msg), "kzalloc fail");
        goto err_create_transvr_fail;
    }
    /* Prepare static size attributes */
    if (setup_transvr_ssize_attr(swp_name,
                                 result_p,
                                 map_p,
                                 ioexp_obj_p,
                                 ioexp_virt_offset,
                                 transvr_type,
                                 chipset_type,
                                 chan_id,
                                 run_mode) < 0){
        goto err_create_transvr_sattr_fail;
    }

    /* Prepare call back functions of object */
    if (setup_transvr_private_cb(result_p, transvr_type) < 0){
    	goto err_create_transvr_sattr_fail;
    }
    /* Prepare i2c client object */
    if (setup_i2c_client(result_p) < 0){
    	goto err_create_transvr_sattr_fail;
    }
    return result_p;
err_create_transvr_sattr_fail:
    kfree(result_p);
err_create_transvr_fail:
    SWPS_ERR("%s: %s <chan>:%d <voff>:%d <type>:%d\n",
            __func__, err_msg, chan_id, ioexp_virt_offset, transvr_type);
    return NULL;
}


static int
_reload_transvr_obj(struct transvr_obj_s *self,
                    int new_type){

    struct eeprom_map_s *new_map_p;
    struct eeprom_map_s *old_map_p = self->eeprom_map_p;
    struct i2c_client   *old_i2c_p = self->i2c_client_p;
    int old_type = self->type;

    /* Change state to STATE_TRANSVR_INIT */
    self->state = STATE_TRANSVR_INIT;
    self->type  = new_type;
    /* Replace EEPROME map */
    new_map_p = get_eeprom_map(new_type);
    if (!new_map_p){
        goto err_private_reload_func_1;
    }
    self->eeprom_map_p = new_map_p;
    /* Reload i2c client */
    if (setup_i2c_client(self) < 0){
        goto err_private_reload_func_2;
    }
    if (setup_transvr_private_cb(self, new_type) < 0){
        goto err_private_reload_func_3;
    }
    kfree(old_i2c_p);
    return 0;

err_private_reload_func_3:
    SWPS_INFO("%s: init() fail!\n", __func__);
    kfree(old_i2c_p);
    self->state = STATE_TRANSVR_UNEXCEPTED;
    self->type  = TRANSVR_TYPE_ERROR;
    return -2;

err_private_reload_func_2:
    self->eeprom_map_p = old_map_p;
    self->i2c_client_p = old_i2c_p;
err_private_reload_func_1:
    self->state = STATE_TRANSVR_UNEXCEPTED;
    self->type  = old_type;
    SWPS_INFO("%s fail! <type>:0x%02x\n", __func__, new_type);
    return -1;
}


static int
reload_transvr_obj(struct transvr_obj_s *self,
                   int new_type){

    int result_val = ERR_TRANSVR_UNEXCPT;

    /* Reload phase */
    result_val = _reload_transvr_obj(self, new_type);
    if (result_val < 0){
        SWPS_INFO("%s: reload phase fail! <err>:%d\n",
                  __func__, result_val);
        return EVENT_TRANSVR_RELOAD_FAIL;
    }
    /* Initial phase */
    result_val = _transvr_init_handler(self);
    if (result_val < 0){
        SWPS_INFO("%s: initial phase fail! <err>:%d\n",
                  __func__, result_val);
    }
    return result_val;
}




