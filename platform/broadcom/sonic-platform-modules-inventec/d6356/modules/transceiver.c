#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include "io_expander.h"
#include "transceiver.h"

/* For build single module using (Ex: ONL platform) */
#include <linux/module.h>
//#include <linux/inventec/d5254/io_expander.h>
//#include <linux/inventec/d5254/transceiver.h>

extern int io_no_init;
/* ========== Register EEPROM address mapping ==========
 */
struct eeprom_map_s eeprom_map_sfp = {
    .addr_br           =0x50,  .page_br           =-1,  .offset_br           =12,   .length_br           =1,
    .addr_cdr          =-1,    .page_cdr          =-1,  .offset_cdr          =-1,   .length_cdr          =-1,
    .addr_comp_rev     =0x50,  .page_comp_rev     =-1,  .offset_comp_rev     =94,   .length_comp_rev     =1,
    .addr_connector    =0x50,  .page_connector    =-1,  .offset_connector    =2,    .length_connector    =1,
    .addr_diag_type    =0x50,  .page_diag_type    =-1,  .offset_diag_type    =92 ,  .length_diag_type    =1,
    .addr_extbr        =-1,    .page_extbr        =-1,  .offset_extbr        =-1,   .length_extbr        =-1,
    .addr_ext_id       =0x50,  .page_ext_id       =-1,  .offset_ext_id       =1,    .length_ext_id       =1,
    .addr_id           =0x50,  .page_id           =-1,  .offset_id           =0,    .length_id           =1,
    .addr_len_sm       =0x50,  .page_len_sm       =-1,  .offset_len_sm       =15,   .length_len_sm       =1,
    .addr_len_smf      =0x50,  .page_len_smf      =-1,  .offset_len_smf      =14,   .length_len_smf      =1,
    .addr_len_om1      =0x50,  .page_len_om1      =-1,  .offset_len_om1      =17,   .length_len_om1      =1,
    .addr_len_om2      =0x50,  .page_len_om2      =-1,  .offset_len_om2      =16,   .length_len_om2      =1,
    .addr_len_om3      =0x50,  .page_len_om3      =-1,  .offset_len_om3      =19,   .length_len_om3      =1,
    .addr_len_om4      =0x50,  .page_len_om4      =-1,  .offset_len_om4      =18,   .length_len_om4      =1,
    .addr_option       =0x50,  .page_option       =-1,  .offset_option       =64,   .length_option       =2,
    .addr_rate_id      =0x50,  .page_rate_id      =-1,  .offset_rate_id      =13,   .length_rate_id      =1,
    .addr_rx_am        =-1,    .page_rx_am        =-1,  .offset_rx_am        =-1,   .length_rx_am        =-1,
    .addr_rx_em        =0x51,  .page_rx_em        =-1,  .offset_rx_em        =115,  .length_rx_em        =1,
    .addr_rx_los       =-1,    .page_rx_los       =-1,  .offset_rx_los       =-1,   .length_rx_los       =-1,
    .addr_rx_power     =0x51,  .page_rx_power     =-1,  .offset_rx_power     =104,  .length_rx_power     =2,
    .addr_soft_rs0     =0x51,  .page_soft_rs0     =-1,  .offset_soft_rs0     =110,  .length_soft_rs0     =1,
    .addr_soft_rs1     =0x51,  .page_soft_rs1     =-1,  .offset_soft_rs1     =118,  .length_soft_rs0     =1,
    .addr_temp         =0x51,  .page_temp         =-1,  .offset_temp         =96,   .length_temp         =2,
    .addr_trancomp     =0x50,  .page_trancomp     =-1,  .offset_trancomp     =3,    .length_trancomp     =8,
    .addr_trancomp_ext =0x50,  .page_trancomp_ext =-1,  .offset_trancomp_ext =36,   .length_trancomp_ext =1,
    .addr_tx_bias      =0x51,  .page_tx_bias      =-1,  .offset_tx_bias      =100,  .length_tx_bias      =2,
    .addr_tx_disable   =-1,    .page_tx_disable   =-1,  .offset_tx_disable   =-1,   .length_tx_disable   =-1,
    .addr_tx_eq        =0x51,  .page_tx_eq        =-1,  .offset_tx_eq        =114,  .length_tx_eq        =1,
    .addr_tx_fault     =-1,    .page_tx_fault     =-1,  .offset_tx_fault     =-1,   .length_tx_fault     =-1,
    .addr_tx_power     =0x51,  .page_tx_power     =-1,  .offset_tx_power     =102,  .length_tx_power     =2,
    .addr_vendor_name  =0x50,  .page_vendor_name  =-1,  .offset_vendor_name  =20,   .length_vendor_name  =16,
    .addr_vendor_pn    =0x50,  .page_vendor_pn    =-1,  .offset_vendor_pn    =40,   .length_vendor_pn    =16,
    .addr_vendor_rev   =0x50,  .page_vendor_rev   =-1,  .offset_vendor_rev   =56,   .length_vendor_rev   =4,
    .addr_vendor_sn    =0x50,  .page_vendor_sn    =-1,  .offset_vendor_sn    =68,   .length_vendor_sn    =16,
    .addr_voltage      =0x51,  .page_voltage      =-1,  .offset_voltage      =98,   .length_voltage      =2,
    .addr_wavelength   =0x50,  .page_wavelength   =-1,  .offset_wavelength   =60,   .length_wavelength   =2,
};

struct eeprom_map_s eeprom_map_qsfp = {
    .addr_br           =0x50,  .page_br           =0,   .offset_br           =140,  .length_br           =1,
    .addr_cdr          =-1,    .page_cdr          =-1,  .offset_cdr          =-1,   .length_cdr          =-1,
    .addr_comp_rev     =0x50,  .page_comp_rev     =-1,  .offset_comp_rev     =1,    .length_comp_rev     =1,
    .addr_connector    =0x50,  .page_connector    =0,   .offset_connector    =130,  .length_connector    =1,
    .addr_diag_type    =0x50,  .page_diag_type    =0,   .offset_diag_type    =220,  .length_diag_type    =1,
    .addr_extbr        =0x50,  .page_extbr        =0,   .offset_extbr        =222,  .length_extbr        =1,
    .addr_ext_id       =0x50,  .page_ext_id       =0,   .offset_ext_id       =129,  .length_ext_id       =1,
    .addr_id           =0x50,  .page_id           =0,   .offset_id           =128,  .length_id           =1,
    .addr_len_sm       =-1,    .page_len_sm       =-1,  .offset_len_sm       =-1,   .length_len_sm       =-1,
    .addr_len_smf      =0x50,  .page_len_smf      =0,   .offset_len_smf      =142,  .length_len_smf      =1,
    .addr_len_om1      =0x50,  .page_len_om1      =0,   .offset_len_om1      =145,  .length_len_om1      =1,
    .addr_len_om2      =0x50,  .page_len_om2      =0,   .offset_len_om2      =144,  .length_len_om2      =1,
    .addr_len_om3      =0x50,  .page_len_om3      =0,   .offset_len_om3      =143,  .length_len_om3      =1,
    .addr_len_om4      =0x50,  .page_len_om4      =0,   .offset_len_om4      =146,  .length_len_om4      =1,
    .addr_option       =0x50,  .page_option       =0,   .offset_option       =193,  .length_option       =3,
    .addr_rate_id      =-1,    .page_rate_id      =-1,  .offset_rate_id      =-1,   .length_rate_id      =-1,
    .addr_rx_am        =-1,    .page_rx_am        =-1,  .offset_rx_am        =-1,   .length_rx_am        =-1,
    .addr_rx_em        =-1,    .page_rx_em        =-1,  .offset_rx_em        =-1,   .length_rx_em        =-1,
    .addr_rx_los       =0x50,  .page_rx_los       =-1,  .offset_rx_los       =3,    .length_rx_los       =1,
    .addr_rx_power     =0x50,  .page_rx_power     =-1,  .offset_rx_power     =34,   .length_rx_power     =8,
    .addr_soft_rs0     =-1,    .page_soft_rs0     =-1,  .offset_soft_rs0     =-1,   .length_soft_rs0     =-1,
    .addr_soft_rs1     =-1,    .page_soft_rs1     =-1,  .offset_soft_rs1     =-1,   .length_soft_rs0     =-1,
    .addr_temp         =0x50,  .page_temp         =-1,  .offset_temp         =22,   .length_temp         =2,
    .addr_trancomp     =0x50,  .page_trancomp     =0,   .offset_trancomp     =131,  .length_trancomp     =8,
    .addr_trancomp_ext =0x50,  .page_trancomp_ext =0,   .offset_trancomp_ext =192,  .length_trancomp_ext =1,
    .addr_tx_bias      =0x50,  .page_tx_bias      =-1,  .offset_tx_bias      =42,   .length_tx_bias      =8,
    .addr_tx_disable   =0x50,  .page_tx_disable   =-1,  .offset_tx_disable   =86,   .length_tx_disable   =1,
    .addr_tx_eq        =-1,    .page_tx_eq        =-1,  .offset_tx_eq        =-1,   .length_tx_eq        =-1,
    .addr_tx_fault     =0x50,  .page_tx_fault     =-1,  .offset_tx_fault     =4,    .length_tx_fault     =1,
    .addr_tx_power     =0x50,  .page_tx_power     =-1,  .offset_tx_power     =50,   .length_tx_power     =8,
    .addr_vendor_name  =0x50,  .page_vendor_name  =0,   .offset_vendor_name  =148,  .length_vendor_name  =16,
    .addr_vendor_pn    =0x50,  .page_vendor_pn    =0,   .offset_vendor_pn    =168,  .length_vendor_pn    =16,
    .addr_vendor_rev   =0x50,  .page_vendor_rev   =0,   .offset_vendor_rev   =184,  .length_vendor_rev   =2,
    .addr_vendor_sn    =0x50,  .page_vendor_sn    =0,   .offset_vendor_sn    =196,  .length_vendor_sn    =16,
    .addr_voltage      =0x50,  .page_voltage      =-1,  .offset_voltage      =26,   .length_voltage      =2,
    .addr_wavelength   =0x50,  .page_wavelength   =0,   .offset_wavelength   =186,  .length_wavelength   =2,
};

struct eeprom_map_s eeprom_map_qsfp28 = {
    .addr_br           =0x50,  .page_br           =0,   .offset_br           =140,  .length_br           =1,
    .addr_cdr          =0x50,  .page_cdr          =-1,  .offset_cdr          =98,   .length_cdr          =1,
    .addr_comp_rev     =0x50,  .page_comp_rev     =-1,  .offset_comp_rev     =1,    .length_comp_rev     =1,
    .addr_connector    =0x50,  .page_connector    =0,   .offset_connector    =130,  .length_connector    =1,
    .addr_diag_type    =0x50,  .page_diag_type    =0,   .offset_diag_type    =220,  .length_diag_type    =1,
    .addr_extbr        =0x50,  .page_extbr        =0,   .offset_extbr        =222,  .length_extbr        =1,
    .addr_ext_id       =0x50,  .page_ext_id       =0,   .offset_ext_id       =129,  .length_ext_id       =1,
    .addr_id           =0x50,  .page_id           =0,   .offset_id           =128,  .length_id           =1,
    .addr_len_sm       =-1,    .page_len_sm       =-1,  .offset_len_sm       =-1,   .length_len_sm       =-1,
    .addr_len_smf      =0x50,  .page_len_smf      =0,   .offset_len_smf      =142,  .length_len_smf      =1,
    .addr_len_om1      =0x50,  .page_len_om1      =0,   .offset_len_om1      =145,  .length_len_om1      =1,
    .addr_len_om2      =0x50,  .page_len_om2      =0,   .offset_len_om2      =144,  .length_len_om2      =1,
    .addr_len_om3      =0x50,  .page_len_om3      =0,   .offset_len_om3      =143,  .length_len_om3      =1,
    .addr_len_om4      =0x50,  .page_len_om4      =0,   .offset_len_om4      =146,  .length_len_om4      =1,
    .addr_option       =0x50,  .page_option       =0,   .offset_option       =193,  .length_option       =3,
    .addr_rate_id      =-1,    .page_rate_id      =-1,  .offset_rate_id      =-1,   .length_rate_id      =-1,
    .addr_rx_am        =0x50,  .page_rx_am        =3,   .offset_rx_am        =238,  .length_rx_am        =2,
    .addr_rx_em        =0x50,  .page_rx_em        =3,   .offset_rx_em        =236,  .length_rx_em        =2,
    .addr_rx_los       =0x50,  .page_rx_los       =-1,  .offset_rx_los       =3,    .length_rx_los       =1,
    .addr_rx_power     =0x50,  .page_rx_power     =-1,  .offset_rx_power     =34,   .length_rx_power     =8,
    .addr_soft_rs0     =-1,    .page_soft_rs0     =-1,  .offset_soft_rs0     =-1,   .length_soft_rs0     =-1,
    .addr_soft_rs1     =-1,    .page_soft_rs1     =-1,  .offset_soft_rs1     =-1,   .length_soft_rs0     =-1,
    .addr_temp         =0x50,  .page_temp         =-1,  .offset_temp         =22,   .length_temp         =2,
    .addr_trancomp     =0x50,  .page_trancomp     =0,   .offset_trancomp     =131,  .length_trancomp     =8,
    .addr_trancomp_ext =0x50,  .page_trancomp_ext =0,   .offset_trancomp_ext =192,  .length_trancomp_ext =1,
    .addr_tx_bias      =0x50,  .page_tx_bias      =-1,  .offset_tx_bias      =42,   .length_tx_bias      =8,
    .addr_tx_disable   =0x50,  .page_tx_disable   =-1,  .offset_tx_disable   =86,   .length_tx_disable   =1,
    .addr_tx_eq        =0x50,  .page_tx_eq        =3,   .offset_tx_eq        =234,  .length_tx_eq        =2,
    .addr_tx_fault     =0x50,  .page_tx_fault     =-1,  .offset_tx_fault     =4,    .length_tx_fault     =1,
    .addr_tx_power     =0x50,  .page_tx_power     =-1,  .offset_tx_power     =50,   .length_tx_power     =8,
    .addr_vendor_name  =0x50,  .page_vendor_name  =0,   .offset_vendor_name  =148,  .length_vendor_name  =16,
    .addr_vendor_pn    =0x50,  .page_vendor_pn    =0,   .offset_vendor_pn    =168,  .length_vendor_pn    =16,
    .addr_vendor_rev   =0x50,  .page_vendor_rev   =0,   .offset_vendor_rev   =184,  .length_vendor_rev   =2,
    .addr_vendor_sn    =0x50,  .page_vendor_sn    =0,   .offset_vendor_sn    =196,  .length_vendor_sn    =16,
    .addr_voltage      =0x50,  .page_voltage      =-1,  .offset_voltage      =26,   .length_voltage      =2,
    .addr_wavelength   =0x50,  .page_wavelength   =0,   .offset_wavelength   =186,  .length_wavelength   =2,
};


/* ========== Utility Functions ==========
 */
static int
get_bit(uint8_t origin_byte, int bit_shift) {
    return (int)((origin_byte >> bit_shift) & 0x1);
}

static int
transform_word_to_int(uint8_t hight_byte,
                      uint8_t low_byte) {
    return ((((int)hight_byte) << 8) + (int)low_byte);
}

void
alarm_msg_2_user(struct transvr_obj_s *self,
                 char *emsg) {

    SWPS_ERR("%s on %s.\n", emsg, self->swp_name);
}
EXPORT_SYMBOL(alarm_msg_2_user);

/* ========== Private functions ==========
 */
static int
_reload_transvr_obj(struct transvr_obj_s *self,int new_type);

static int
reload_transvr_obj(struct transvr_obj_s *self,int new_type);

static int
_is_transvr_support_ctle(struct transvr_obj_s *self);

static int
_transvr_init_handler(struct transvr_obj_s *self);

int
_transvr_clean_handler(struct transvr_obj_s *self);

int
_sfp_detect_class_by_1g_ethernet(struct transvr_obj_s* self);


void
lock_transvr_obj(struct transvr_obj_s *self) {

    mutex_lock(&self->lock);
    self->curr_page = VAL_TRANSVR_PAGE_FREE;
}
EXPORT_SYMBOL(lock_transvr_obj);


void
unlock_transvr_obj(struct transvr_obj_s *self) {

    self->curr_page = VAL_TRANSVR_PAGE_FREE;
    mutex_unlock(&self->lock);
}
EXPORT_SYMBOL(unlock_transvr_obj);


static int
_check_by_mode(struct transvr_obj_s *self,
               int  (*attr_update_func)(struct transvr_obj_s *self, int show_err),
               char *caller_name){

    int return_val = ERR_TRANSVR_UNEXCPT;

    switch (self->mode){
        case TRANSVR_MODE_POLLING:
            switch (self->state){
                case STATE_TRANSVR_CONNECTED:
                    goto ok_check_by_mode_1;
                case STATE_TRANSVR_NEW:
                case STATE_TRANSVR_INIT:
                    return ERR_TRANSVR_UNINIT;
                case STATE_TRANSVR_DISCONNECTED:
                    return ERR_TRANSVR_UNPLUGGED;
                case STATE_TRANSVR_UNEXCEPTED:
                    return ERR_TRANSVR_ABNORMAL;
                case STATE_TRANSVR_ISOLATED:
                    return ERR_TRNASVR_BE_ISOLATED;
                default:
                    goto err_check_by_mode_1;
            }
            goto ok_check_by_mode_1;

        case TRANSVR_MODE_DIRECT:
            return_val = self->fsm_4_direct(self, caller_name);
            if (return_val < 0){
                return return_val;
            }
            goto ok_check_by_mode_1;

        default:
            goto err_check_by_mode_1;
    }
    goto ok_check_by_mode_1;

ok_check_by_mode_1:
    return attr_update_func(self, 0);

err_check_by_mode_1:
    SWPS_INFO("_check_by_mode: mode:%d state:%d\n", self->mode, self->state);
    return ERR_TRANSVR_UNEXCPT;
}


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

/*
static int
_common_setup_password(struct transvr_obj_s *self,
                       int addr,
                       int page,
                       int offs,
                       uint8_t pwd[4],
                       int show_e) {
    int   i    = 0;
    int   err  = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;

    err = _common_setup_page(self, addr, page, offs, 4, show_e);
    if (err < 0){
        emsg = "setup EEPROM page fail";
        goto err_common_setup_password;
    }
    for (i=0; i<4; i++) {
        err = i2c_smbus_write_byte_data(self->i2c_client_p,
                                        (offs + i),
                                        pwd[i]);
        if (err < 0){
            emsg = "I2C R/W fail!";
            goto err_common_setup_password;
        }
    }
    return 0;

err_common_setup_password:
    if (show_e) {
        SWPS_INFO("%s: %s <err>:%d\n", __func__, emsg,  err);
    }
    return ERR_TRANSVR_UPDATE_FAIL;
}
*/

static int
_common_update_uint8_attr(struct transvr_obj_s *self,
                          int addr,
                          int page,
                          int offset,
                          int len,
                          uint8_t *buf,
                          char *caller,
                          int show_e){

    int   i;
    int   err  = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;

    err = _common_setup_page(self, addr, page, offset, len, show_e);
    if (err < 0){
        emsg = "setup EEPROM page fail";
        goto err_common_update_uint8_attr;
    }
    for (i=0; i<len; i++) {
        err = i2c_smbus_read_byte_data(self->i2c_client_p, (offset + i));
        if (err < 0){
            emsg = "I2C R/W fail!";
            goto err_common_update_uint8_attr;
        }
        buf[i] = err;
    }
    return 0;

err_common_update_uint8_attr:
    if (show_e) {
        SWPS_INFO("%s: %s <caller>:%s <err>:%d\n",
                  __func__, emsg, caller, err);
    }
    buf[0] = DEBUG_TRANSVR_HEX_VAL;
    return ERR_TRANSVR_UPDATE_FAIL;
}


static int
_common_update_int_attr(struct transvr_obj_s *self,
                        int addr,
                        int page,
                        int offset,
                        int len,
                        int *buf,
                        char *caller,
                        int show_e){

    int   i;
    int   err  = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;

    err = _common_setup_page(self, addr, page, offset, len, show_e);
    if (err < 0){
        emsg = "setup EEPROM page fail";
        goto err_common_update_int_attr;
    }
    for (i=0; i<len; i++) {
        err = i2c_smbus_read_byte_data(self->i2c_client_p, (offset + i));
        if (err < 0){
            emsg = "I2C R/W fail!";
            goto err_common_update_int_attr;
        }
        buf[i] = (int)err;
    }
    return 0;

err_common_update_int_attr:
    if (show_e) {
        SWPS_INFO("%s: %s <caller>:%s <err>:%d\n",
                  __func__, emsg, caller, err);
    }
    buf[0] = DEBUG_TRANSVR_INT_VAL;
    return ERR_TRANSVR_UPDATE_FAIL;
}


static int
_common_update_string_attr(struct transvr_obj_s *self,
                           int addr,
                           int page,
                           int offset,
                           int len,
                           char buf[],
                           char *caller,
                           int show_e){

    int   i;
    int   err  = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;

    err = _common_setup_page(self, addr, page, offset, len, show_e);
    if (err < 0){
        emsg = "setup EEPROM page fail";
        goto err_common_update_string_attr;
    }
    for (i=0; i<len; i++) {
        err = i2c_smbus_read_byte_data(self->i2c_client_p, (offset + i));
        if (err < 0){
            emsg = "I2C R/W fail!";
            goto err_common_update_string_attr;
        }
        buf[i] = (char)err;
    }
    return 0;

err_common_update_string_attr:
    if (show_e) {
        SWPS_INFO("%s: %s <caller>:%s <err>:%d\n",
                  __func__, emsg, caller, err);
    }
    buf[0] = 'e';
    return ERR_TRANSVR_UPDATE_FAIL;
}


static int
_common_set_uint8_attr(struct transvr_obj_s *self,
                       int addr,
                       int page,
                       int offset,
                       uint8_t update,
                       uint8_t *buf,
                       char *caller,
                       int show_e){
    int   len  = 1;
    int   err  = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;

    if ((*buf) == update){
        return 0;
    }
    err = _common_setup_page(self, addr, page, offset, len, show_e);
    if (err < 0){
        emsg = "setup EEPROM page fail";
        goto err_common_set_uint8_attr_1;
    }
    err = i2c_smbus_write_byte_data(self->i2c_client_p,
                                    offset,
                                    update);
    if (err < 0){
        emsg = "I2C R/W fail!";
        goto err_common_set_uint8_attr_1;
    }
    (*buf) = update;
    return 0;

err_common_set_uint8_attr_1:
    if (show_e) {
        SWPS_INFO("%s: %s <caller>:%s <err>:%d\n",
                  __func__, emsg, caller, err);
    }
    return ERR_TRANSVR_UPDATE_FAIL;
}


static int
_common_set_uint8_array(struct transvr_obj_s *self,
                       int addr,
                       int page,
                       int offs,
                       int len,
                       uint8_t update[],
                       uint8_t buf[],
                       char *caller,
                       int show_e){
    int   i    = 0;
    int   err  = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;

    err = _common_setup_page(self, addr, page, offs, len, show_e);
    if (err < 0){
        emsg = "setup EEPROM page fail";
        goto err_common_set_uint8_attr_1;
    }
    for (i=0; i<len; i++) {
        if (buf[i] == update[i]){
            continue;
        }
        err = i2c_smbus_write_byte_data(self->i2c_client_p,
                                        (offs + i),
                                        update[i]);
        if (err < 0){
            emsg = "I2C R/W fail!";
            goto err_common_set_uint8_attr_1;
        }
        buf[i] = update[i];
    }
    return 0;

err_common_set_uint8_attr_1:
    if (show_e) {
        SWPS_INFO("%s: %s <caller>:%s <err>:%d <i>:%d\n",
                  __func__, emsg, caller, err, i);
    }
    return ERR_TRANSVR_UPDATE_FAIL;
}


static int
_common_update_attr_id(struct transvr_obj_s *self,
                       int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_id,
                                     self->eeprom_map_p->page_id,
                                     self->eeprom_map_p->offset_id,
                                     self->eeprom_map_p->length_id,
                                     &(self->id),
                                     "_common_update_attr_id",
                                     show_err);
}


static int
_common_update_attr_extended_id(struct transvr_obj_s *self,
                                int show_err){
    return  _common_update_uint8_attr(self,
                                      self->eeprom_map_p->addr_ext_id,
                                      self->eeprom_map_p->page_ext_id,
                                      self->eeprom_map_p->offset_ext_id,
                                      self->eeprom_map_p->length_ext_id,
                                      &(self->ext_id),
                                      "_common_update_attr_extended_id",
                                      show_err);
}


static int
_common_update_attr_connector(struct transvr_obj_s *self,
                              int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_connector,
                                     self->eeprom_map_p->page_connector,
                                     self->eeprom_map_p->offset_connector,
                                     self->eeprom_map_p->length_connector,
                                     &(self->connector),
                                     "_common_update_attr_connector",
                                     show_err);
}


static int
_common_update_attr_transvr_comp(struct transvr_obj_s *self,
                                 int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_trancomp,
                                     self->eeprom_map_p->page_trancomp,
                                     self->eeprom_map_p->offset_trancomp,
                                     self->eeprom_map_p->length_trancomp,
                                     self->transvr_comp,
                                     "_common_update_attr_transvr_comp",
                                     show_err);
}


static int
_common_update_attr_transvr_comp_ext(struct transvr_obj_s *self,
                                     int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_trancomp_ext,
                                     self->eeprom_map_p->page_trancomp_ext,
                                     self->eeprom_map_p->offset_trancomp_ext,
                                     self->eeprom_map_p->length_trancomp_ext,
                                     &(self->transvr_comp_ext),
                                     "_common_update_attr_transvr_comp_ext",
                                     show_err);
}


static int
_common_update_attr_vendor_name(struct transvr_obj_s *self,
                                int show_err){
    return _common_update_string_attr(self,
                                      self->eeprom_map_p->addr_vendor_name,
                                      self->eeprom_map_p->page_vendor_name,
                                      self->eeprom_map_p->offset_vendor_name,
                                      self->eeprom_map_p->length_vendor_name,
                                      self->vendor_name,
                                      "_common_update_attr_vendor_name",
                                      show_err);
}


static int
_common_update_attr_vendor_pn(struct transvr_obj_s *self,
                              int show_err){
    return _common_update_string_attr(self,
                                      self->eeprom_map_p->addr_vendor_pn,
                                      self->eeprom_map_p->page_vendor_pn,
                                      self->eeprom_map_p->offset_vendor_pn,
                                      self->eeprom_map_p->length_vendor_pn,
                                      self->vendor_pn,
                                      "_common_update_attr_vendor_pn",
                                      show_err);
}


static int
_common_update_attr_vendor_rev(struct transvr_obj_s *self,
                               int show_err){
    return _common_update_string_attr(self,
                                      self->eeprom_map_p->addr_vendor_rev,
                                      self->eeprom_map_p->page_vendor_rev,
                                      self->eeprom_map_p->offset_vendor_rev,
                                      self->eeprom_map_p->length_vendor_rev,
                                      self->vendor_rev,
                                      "_common_update_attr_vendor_rev",
                                      show_err);
}


static int
_common_update_attr_vendor_sn(struct transvr_obj_s *self,
                              int show_err){
    return _common_update_string_attr(self,
                                      self->eeprom_map_p->addr_vendor_sn,
                                      self->eeprom_map_p->page_vendor_sn,
                                      self->eeprom_map_p->offset_vendor_sn,
                                      self->eeprom_map_p->length_vendor_sn,
                                      self->vendor_sn,
                                      "_common_update_attr_vendor_sn",
                                      show_err);
}


static int
_common_update_attr_br(struct transvr_obj_s *self,
                       int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_br,
                                     self->eeprom_map_p->page_br,
                                     self->eeprom_map_p->offset_br,
                                     self->eeprom_map_p->length_br,
                                     &(self->br),
                                     "_common_update_attr_br",
                                     show_err);
}


static int
_common_update_attr_len_smf(struct transvr_obj_s *self,
                            int show_err){
    return _common_update_int_attr(self,
                                   self->eeprom_map_p->addr_len_smf,
                                   self->eeprom_map_p->page_len_smf,
                                   self->eeprom_map_p->offset_len_smf,
                                   self->eeprom_map_p->length_len_smf,
                                   &(self->len_smf),
                                   "_common_update_attr_len_smf",
                                   show_err);
}


static int
_common_update_attr_len_om1(struct transvr_obj_s *self,
                            int show_err){
    return _common_update_int_attr(self,
                                   self->eeprom_map_p->addr_len_om1,
                                   self->eeprom_map_p->page_len_om1,
                                   self->eeprom_map_p->offset_len_om1,
                                   self->eeprom_map_p->length_len_om1,
                                   &(self->len_om1),
                                   "_common_update_attr_len_om1",
                                   show_err);
}

static int
_common_update_attr_len_om2(struct transvr_obj_s *self,
                            int show_err){
    return _common_update_int_attr(self,
                                   self->eeprom_map_p->addr_len_om2,
                                   self->eeprom_map_p->page_len_om2,
                                   self->eeprom_map_p->offset_len_om2,
                                   self->eeprom_map_p->length_len_om2,
                                   &(self->len_om2),
                                   "_common_update_attr_len_om2",
                                   show_err);
}

static int
_common_update_attr_len_om3(struct transvr_obj_s *self,
                            int show_err){
    return _common_update_int_attr(self,
                                   self->eeprom_map_p->addr_len_om3,
                                   self->eeprom_map_p->page_len_om3,
                                   self->eeprom_map_p->offset_len_om3,
                                   self->eeprom_map_p->length_len_om3,
                                   &(self->len_om3),
                                   "_common_update_attr_len_om3",
                                   show_err);
}


static int
_common_update_attr_len_om4(struct transvr_obj_s *self,
                            int show_err){
    return _common_update_int_attr(self,
                                   self->eeprom_map_p->addr_len_om4,
                                   self->eeprom_map_p->page_len_om4,
                                   self->eeprom_map_p->offset_len_om4,
                                   self->eeprom_map_p->length_len_om4,
                                   &(self->len_om4),
                                   "_common_update_attr_len_om4",
                                   show_err);
}


static int
_common_update_attr_option(struct transvr_obj_s *self,
                           int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_option,
                                     self->eeprom_map_p->page_option,
                                     self->eeprom_map_p->offset_option,
                                     self->eeprom_map_p->length_option,
                                     self->option,
                                     "_common_update_attr_option",
                                     show_err);
}


static int
_common_update_attr_comp_rev(struct transvr_obj_s *self,
                             int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_comp_rev,
                                     self->eeprom_map_p->page_comp_rev,
                                     self->eeprom_map_p->offset_comp_rev,
                                     self->eeprom_map_p->length_comp_rev,
                                     &(self->comp_rev),
                                     "_common_update_attr_comp_rev",
                                     show_err);
}


static int
_common_update_attr_diag_type(struct transvr_obj_s *self,
                              int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_diag_type,
                                     self->eeprom_map_p->page_diag_type,
                                     self->eeprom_map_p->offset_diag_type,
                                     self->eeprom_map_p->length_diag_type,
                                     &(self->diag_type),
                                     "_common_update_attr_diag_type",
                                     show_err);
}


static int
_common_update_attr_wavelength(struct transvr_obj_s *self,
                               int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_wavelength,
                                     self->eeprom_map_p->page_wavelength,
                                     self->eeprom_map_p->offset_wavelength,
                                     self->eeprom_map_p->length_wavelength,
                                     self->wavelength,
                                     "_common_update_attr_wavelength",
                                     show_err);
}


int
_common_get_option_value(struct transvr_obj_s *self,
                         int offset,
                         int bit_shift) {
    /* SFP:
     *  - option[0] = A0h / 64
     *  - option[1] = A0h / 65
     * QSFP:
     *  - option[0] = 00h / 193
     *  - option[1] = 00h / 194
     *  - option[2] = 00h / 195
     */
    return (self->option[offset] & (1 << bit_shift));
}


static int
_sfp_update_attr_len_sm(struct transvr_obj_s *self,
                        int show_err){
    return _common_update_int_attr(self,
                                   self->eeprom_map_p->addr_len_sm,
                                   self->eeprom_map_p->page_len_sm,
                                   self->eeprom_map_p->offset_len_sm,
                                   self->eeprom_map_p->length_len_sm,
                                   &(self->len_sm),
                                   "_common_update_attr_len_sm",
                                   show_err);
}


static int
_sfp_update_attr_rate_id(struct transvr_obj_s *self,
                         int show_err){
    return _common_update_int_attr(self,
                                   self->eeprom_map_p->addr_rate_id,
                                   self->eeprom_map_p->page_rate_id,
                                   self->eeprom_map_p->offset_rate_id,
                                   self->eeprom_map_p->length_rate_id,
                                   &(self->rate_id),
                                   "_sfp_update_attr_rate_id",
                                   show_err);
}


static int
_sfp_update_attr_soft_rs0(struct transvr_obj_s *self,
                          int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_soft_rs0,
                                     self->eeprom_map_p->page_soft_rs0,
                                     self->eeprom_map_p->offset_soft_rs0,
                                     self->eeprom_map_p->length_soft_rs0,
                                     &(self->soft_rs0),
                                     "_sfp_update_attr_soft_rs0",
                                     show_err);
}


static int
_sfp_update_attr_soft_rs1(struct transvr_obj_s *self,
                          int show_err){
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_soft_rs1,
                                     self->eeprom_map_p->page_soft_rs1,
                                     self->eeprom_map_p->offset_soft_rs1,
                                     self->eeprom_map_p->length_soft_rs1,
                                     &(self->soft_rs1),
                                     "_sfp_update_attr_soft_rs1",
                                     show_err);
}


int
_sfp_is_diag_support(struct transvr_obj_s *self){

    uint8_t bit_mask = 0xC0; /* 1100 0000 */
    uint8_t en_val   = 0x40; /* 0100 0000 */
    uint8_t checkval = (self->diag_type & bit_mask);

    if (checkval == en_val) {
        return 1;
    }
    return 0;
}


static int
_sfp_update_attr_curr_temp(struct transvr_obj_s *self,
                           int show_err){

    if (!(_sfp_is_diag_support(self))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_temp,
                                     self->eeprom_map_p->page_temp,
                                     self->eeprom_map_p->offset_temp,
                                     self->eeprom_map_p->length_temp,
                                     self->curr_temp,
                                     "_sfp_update_attr_curr_temp",
                                     show_err);
}


static int
_sfp_update_attr_curr_voltage(struct transvr_obj_s *self,
                              int show_err){

    if (!(_sfp_is_diag_support(self))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_voltage,
                                     self->eeprom_map_p->page_voltage,
                                     self->eeprom_map_p->offset_voltage,
                                     self->eeprom_map_p->length_voltage,
                                     self->curr_voltage,
                                     "_sfp_update_attr_curr_voltage",
                                     show_err);
}


static int
_sfp_update_attr_curr_tx_bias(struct transvr_obj_s *self,
                              int show_err){

    if (!(_sfp_is_diag_support(self))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_tx_bias,
                                     self->eeprom_map_p->page_tx_bias,
                                     self->eeprom_map_p->offset_tx_bias,
                                     self->eeprom_map_p->length_tx_bias,
                                     self->curr_tx_bias,
                                     "_sfp_update_attr_curr_tx_bias",
                                     show_err);
}


static int
_sfp_update_attr_curr_tx_power(struct transvr_obj_s *self,
                               int show_err){

    if (!(_sfp_is_diag_support(self))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_tx_power,
                                     self->eeprom_map_p->page_tx_power,
                                     self->eeprom_map_p->offset_tx_power,
                                     self->eeprom_map_p->length_tx_power,
                                     self->curr_tx_power,
                                     "_sfp_update_attr_curr_tx_power",
                                     show_err);
}


static int
_sfp_update_attr_curr_rx_power(struct transvr_obj_s *self,
                               int show_err){

    if (!(_sfp_is_diag_support(self))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_rx_power,
                                     self->eeprom_map_p->page_rx_power,
                                     self->eeprom_map_p->offset_rx_power,
                                     self->eeprom_map_p->length_rx_power,
                                     self->curr_rx_power,
                                     "_sfp_update_attr_curr_rx_power",
                                     show_err);
}


static int
_sfp_update_attr_rx_em(struct transvr_obj_s *self,
                       int show_err){

    if (!_is_transvr_support_ctle(self)) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_rx_em,
                                     self->eeprom_map_p->page_rx_em,
                                     self->eeprom_map_p->offset_rx_em,
                                     self->eeprom_map_p->length_rx_em,
                                     self->rx_em,
                                     "_sfp_update_attr_rx_em",
                                     show_err);
}


static int
_sfp_update_attr_tx_eq(struct transvr_obj_s *self,
                       int show_err){

    if (!_is_transvr_support_ctle(self)) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_tx_eq,
                                     self->eeprom_map_p->page_tx_eq,
                                     self->eeprom_map_p->offset_tx_eq,
                                     self->eeprom_map_p->length_tx_eq,
                                     self->tx_eq,
                                     "_sfp_update_attr_tx_eq",
                                     show_err);
}


static int
_qsfp_update_attr_cdr(struct transvr_obj_s *self,
                      int show_err){
    if (self->type != TRANSVR_TYPE_QSFP_28){
        self->cdr = DEBUG_TRANSVR_HEX_VAL;
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_cdr,
                                     self->eeprom_map_p->page_cdr,
                                     self->eeprom_map_p->offset_cdr,
                                     self->eeprom_map_p->length_cdr,
                                     &(self->cdr),
                                     "_common_update_attr_cdr",
                                     show_err);
}


static int
_qsfg_update_attr_extbr(struct transvr_obj_s *self,
                        int show_err) {
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_extbr,
                                     self->eeprom_map_p->page_extbr,
                                     self->eeprom_map_p->offset_extbr,
                                     self->eeprom_map_p->length_extbr,
                                     &(self->extbr),
                                     "_common_update_attr_extbr",
                                     show_err);
}


static int
_qsfp_is_diag_support(struct transvr_obj_s *self,
                      int diag_type) {
    /* Input Parm: diag_type
     * => 1 : temperature
     * => 2 : voltage
     * => 3 : tx relate
     * => 4 : rx relate
     */
    uint8_t mask_b2 = 0x04; /* 0000 0100 */
    uint8_t mask_b3 = 0x08; /* 0000 1000 */

    switch (diag_type) {
        case 1: /* temperature */
        case 2: /* voltage */
            /* Direct access target, because of spec not defined */
            return 1;
        case 3:
        case 4:
            /* [Note]
             *   Due to lot of transceiver vendor defined it not rigorously and
             *   consider of general support, we seem it as supported if there
             *   are bit-2 OR bit-3 defined by transceiver vendor.
             */
            if ( ((self->diag_type & mask_b2) == mask_b2 ) ||
                 ((self->diag_type & mask_b3) == mask_b3 ) ){
                return 1;
            }
            return 0;
        default:
            SWPS_INFO("%s: undefined diag_type:%d\n",
                      __func__, diag_type);
            break;
    }
    return 0;
}


int
_qsfp_is_implement_tx_disable(struct transvr_obj_s *self) {
    /*
     * 00h / Byte-195 / Bit-4
     */
    int byte = 2;
    int bit  = 4;
    return _common_get_option_value(self, byte, bit);
}


int
_qsfp_is_implement_tx_fault(struct transvr_obj_s *self) {
    /*
     * 00h / Byte-195 / Bit-3
     */
    int byte = 2;
    int bit  = 3;
    return _common_get_option_value(self, byte, bit);
}


static int
_qsfp_update_attr_curr_temp(struct transvr_obj_s *self,
                            int show_err){
    int diag_type = 1;

    if (!(_qsfp_is_diag_support(self, diag_type))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_temp,
                                     self->eeprom_map_p->page_temp,
                                     self->eeprom_map_p->offset_temp,
                                     self->eeprom_map_p->length_temp,
                                     self->curr_temp,
                                     "_qsfp_update_attr_curr_temp",
                                     show_err);
}


static int
_qsfp_update_attr_curr_voltage(struct transvr_obj_s *self,
                               int show_err){
    int diag_type = 2;

    if (!(_qsfp_is_diag_support(self, diag_type))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_voltage,
                                     self->eeprom_map_p->page_voltage,
                                     self->eeprom_map_p->offset_voltage,
                                     self->eeprom_map_p->length_voltage,
                                     self->curr_voltage,
                                     "_qsfp_update_attr_curr_voltage",
                                     show_err);
}


static int
_qsfp_update_attr_curr_tx_bias(struct transvr_obj_s *self,
                               int show_err){
    int diag_type = 3;

    if (!(_qsfp_is_diag_support(self, diag_type))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_tx_bias,
                                     self->eeprom_map_p->page_tx_bias,
                                     self->eeprom_map_p->offset_tx_bias,
                                     self->eeprom_map_p->length_tx_bias,
                                     self->curr_tx_bias,
                                     "_qsfp_update_attr_curr_tx_bias",
                                     show_err);
}


static int
_qsfp_update_attr_curr_tx_power(struct transvr_obj_s *self,
                                int show_err){
    int diag_type = 3;

    if (!(_qsfp_is_diag_support(self, diag_type))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_tx_power,
                                     self->eeprom_map_p->page_tx_power,
                                     self->eeprom_map_p->offset_tx_power,
                                     self->eeprom_map_p->length_tx_power,
                                     self->curr_tx_power,
                                     "_qsfp_update_attr_curr_tx_power",
                                     show_err);
}


static int
_qsfp_update_attr_curr_rx_power(struct transvr_obj_s *self,
                                int show_err){
    int diag_type = 4;

    if (!(_qsfp_is_diag_support(self, diag_type))) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_rx_power,
                                     self->eeprom_map_p->page_rx_power,
                                     self->eeprom_map_p->offset_rx_power,
                                     self->eeprom_map_p->length_rx_power,
                                     self->curr_rx_power,
                                     "_qsfp_update_attr_curr_rx_power",
                                     show_err);
}


static int
_qsfp_update_attr_soft_rx_los(struct transvr_obj_s *self,
                              int show_err){

    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_rx_los,
                                     self->eeprom_map_p->page_rx_los,
                                     self->eeprom_map_p->offset_rx_los,
                                     self->eeprom_map_p->length_rx_los,
                                     &(self->rx_los),
                                     "_qsfp_update_attr_soft_rx_los",
                                     show_err);
}


static int
_qsfp_update_attr_soft_tx_disable(struct transvr_obj_s *self,
                                  int show_err){

    if (!_qsfp_is_implement_tx_disable(self)) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_tx_disable,
                                     self->eeprom_map_p->page_tx_disable,
                                     self->eeprom_map_p->offset_tx_disable,
                                     self->eeprom_map_p->length_tx_disable,
                                     &(self->tx_disable),
                                     "_qsfp_update_attr_soft_tx_disable",
                                     show_err);
}


static int
_qsfp_update_attr_soft_tx_fault(struct transvr_obj_s *self,
                                int show_err){

    if (!_qsfp_is_implement_tx_fault(self)) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_tx_fault,
                                     self->eeprom_map_p->page_tx_fault,
                                     self->eeprom_map_p->offset_tx_fault,
                                     self->eeprom_map_p->length_tx_fault,
                                     &(self->tx_fault),
                                     "_qsfp_update_attr_soft_tx_fault",
                                     show_err);
}


static int
_qsfp_update_attr_tx_eq(struct transvr_obj_s *self,
                        int show_err){

    if (!_is_transvr_support_ctle(self)) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_tx_eq,
                                     self->eeprom_map_p->page_tx_eq,
                                     self->eeprom_map_p->offset_tx_eq,
                                     self->eeprom_map_p->length_tx_eq,
                                     self->tx_eq,
                                     "_qsfp_update_attr_tx_eq",
                                     show_err);
}


static int
_qsfp_update_attr_rx_am(struct transvr_obj_s *self,
                        int show_err){

    if (!_is_transvr_support_ctle(self)) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_rx_am,
                                     self->eeprom_map_p->page_rx_am,
                                     self->eeprom_map_p->offset_rx_am,
                                     self->eeprom_map_p->length_rx_am,
                                     self->rx_am,
                                     "_qsfp_update_attr_rx_am",
                                     show_err);
}


static int
_qsfp_update_attr_rx_em(struct transvr_obj_s *self,
                        int show_err){

    if (!_is_transvr_support_ctle(self)) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return _common_update_uint8_attr(self,
                                     self->eeprom_map_p->addr_rx_em,
                                     self->eeprom_map_p->page_rx_em,
                                     self->eeprom_map_p->offset_rx_em,
                                     self->eeprom_map_p->length_rx_em,
                                     self->rx_em,
                                     "_qsfp_update_attr_rx_em",
                                     show_err);
}


int
_common_update_attr_all(struct transvr_obj_s *self,
                        int show_err){

    char *err_str = "err";

    if (_common_update_attr_id(self, show_err) < 0) {
        err_str = "_common_update_attr_id";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_extended_id(self, show_err) < 0) {
        err_str = "_common_update_attr_extended_id";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_connector(self, show_err) < 0) {
        err_str = "_common_update_attr_connector";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_transvr_comp(self, show_err) < 0) {
        err_str = "_common_update_attr_transvr_comp";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_transvr_comp_ext(self, show_err) < 0) {
        err_str = "_common_update_attr_transvr_comp_ext";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_vendor_name(self, show_err) < 0) {
        err_str = "_common_update_attr_vendor_name";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_vendor_pn(self, show_err) < 0) {
        err_str = "_common_update_attr_vendor_pn";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_vendor_rev(self, show_err) < 0) {
        err_str = "_common_update_attr_vendor_rev";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_vendor_sn(self, show_err) < 0) {
        err_str = "_common_update_attr_vendor_sn";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_br(self, show_err) < 0) {
        err_str = "_common_update_attr_br";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_len_smf(self, show_err) < 0) {
        err_str = "_common_update_attr_len_smf";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_len_om1(self, show_err) < 0) {
        err_str = "_common_update_attr_len_om1";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_len_om2(self, show_err) < 0) {
        err_str = "_common_update_attr_len_om2";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_len_om3(self, show_err) < 0) {
        err_str = "_common_update_attr_len_om3";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_len_om4(self, show_err) < 0) {
        err_str = "_common_update_attr_len_om4";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_option(self, show_err) < 0) {
        err_str = "_common_update_attr_option";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_comp_rev(self, show_err) < 0) {
        err_str = "_common_update_attr_comp_rev";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_diag_type(self, show_err) < 0) {
        err_str = "_common_update_attr_diag_type";
        goto err_common_update_attr_all;
    }
    if (_common_update_attr_wavelength(self, show_err) < 0) {
        err_str = "_common_update_attr_wavelength";
        goto err_common_update_attr_all;
    }
    return 0;

err_common_update_attr_all:
    if (show_err){
        SWPS_INFO("%s: fail at:%s <swp>:%s\n", __func__, err_str, self->swp_name);
    }
    return -1;
}


int
_sfp_update_attr_all(struct transvr_obj_s *self,
                     int show_err){

    char *err_str  = DEBUG_TRANSVR_STR_VAL;

    if (_common_update_attr_all(self, show_err) < 0){
        err_str = "_common_update_attr_all";
        goto err_sfp_update_attr_all;
    }
    if (_sfp_update_attr_len_sm(self, show_err) < 0) {
        err_str = "_sfp_update_attr_len_sm";
        goto err_sfp_update_attr_all;
    }
    if (_sfp_update_attr_rate_id(self, show_err) < 0) {
        err_str = "_sfp_update_attr_rate_id";
        goto err_sfp_update_attr_all;
    }
    if ((self->rate_id) > 0) {
        if (_sfp_update_attr_soft_rs0(self, show_err) < 0) {
            err_str = "_sfp_update_attr_soft_rs0";
            goto err_sfp_update_attr_all;
        }
        if (_sfp_update_attr_soft_rs1(self, show_err) < 0) {
            err_str = "_sfp_update_attr_soft_rs1";
            goto err_sfp_update_attr_all;
        }
    }
    return 0;

err_sfp_update_attr_all:
    if (show_err){
        SWPS_INFO("%s: fail at:%s <swp>:%s\n", __func__, err_str, self->swp_name);
    }
    return -1;
}


int
_qsfp_update_attr_all(struct transvr_obj_s *self,
                      int show_err){

    char *err_str  = DEBUG_TRANSVR_STR_VAL;

    if (_common_update_attr_all(self, show_err) < 0){
        err_str = "_common_update_attr_all";
        goto err_qsfp_update_attr_all;
    }
    if (_qsfg_update_attr_extbr(self, show_err) < 0) {
        err_str = "_qsfg_update_attr_extbr";
        goto err_qsfp_update_attr_all;
    }
    if (self->type == TRANSVR_TYPE_QSFP_28) {
        if (_qsfp_update_attr_cdr(self, 1) < 0) {
            err_str = "_qsfp_update_attr_cdr";
            goto err_qsfp_update_attr_all;
        }
    }
    return 0;

err_qsfp_update_attr_all:
    if (show_err){
        SWPS_INFO("%s: fail at:%s <swp>:%s\n", __func__, err_str, self->swp_name);
    }
    return -1;
}


/* ========== Object functions for common type ==========
 */
int
_common_count_temp(uint8_t high_byte,
                   uint8_t low_byte,
                   char *buf_p) {
    int sign = 0;
    int high = 0;
    int low  = 0;
    int lmax = 8;

    /* Count high */
    sign = get_bit(high_byte,7);
    SWP_BIT_CLEAR(high_byte, 7);
    high = (int)high_byte;
    if (sign == 1) {
        high = 0 - high;
    }
    /* Count low */
    low  = (get_bit(low_byte, 7) * 500);
    low += (get_bit(low_byte, 6) * 250);
    low += (get_bit(low_byte, 5) * 125);
    low += (get_bit(low_byte, 4) *  62);
    low  = (low / 100);
    /* Integrate High and Low */
    return snprintf(buf_p, lmax, "%d.%d\n", high, low);
}


int
_common_count_voltage(uint8_t high_byte,
                      uint8_t low_byte,
                      char *buf_p) {
    /* [Note]:
     *   Internally measured transceiver supply voltage. Represented
     *   as a 16 bit unsigned integer with the voltage defined as the
     *   full 16 bit value (0-65535) with LSB equal to 100 uVolt,
     *   yielding a total range of 0 to +6.55 Volts. Practical
     *   considerations to be defined by transceiver manufacturer will
     *   tend to limit the actual bounds of the supply voltage measurement.
     *   Accuracy is vendor specific but must be better than 3% of the
     *   manufacturer's nominal value over specified operating temperature
     *   and voltage. Note that in some transceivers, transmitter supply
     *   voltage and receiver supply voltage are isolated. In that case,
     *   only one supply is monitored. Refer to the device specification
     *   for more detail.
     */
    int total = 0;
    int lmax  = 8;
    int val_i = 0;
    int val_f = 0;
    /* unit: 100 uV (1mV=1000uV) */
    total = transform_word_to_int(high_byte, low_byte);
    val_i = ((total/10) / 1000);
    val_f = ((total/10) - (val_i*1000));
    /* Return Unit: 1 Volt */
    return snprintf(buf_p, lmax, "%d.%03d\n", val_i, val_f);
}


int
_common_count_tx_bias(uint8_t high_byte,
                      uint8_t low_byte,
                      char *buf_p) {
    /* [Note]
     *   Measured TX bias current in uA. Represented as a 16 bit unsigned
     *   integer with the current defined as the full 16 bit value (0-65535)
     *   with LSB equal to 2 uA, yielding a total range of 0 to 131 mA.
     *   Accuracy is vendor specific but must be better than 10% of the
     *   manufacturer's nominal value over specified operating temperature
     *   and voltage.
     */
    int total = 0;
    int lmax  = 8;
    int val_i = 0;
    int val_f = 0;
    /* unit: 2 uA (1mA=1000uA) */
    total = transform_word_to_int(high_byte, low_byte);
    val_i = ((total*2) / 1000);
    val_f = (((total*2) - (val_i*1000)) / 100);
    /* Return Unit: 1 mA */
    return snprintf(buf_p, lmax, "%d.%01d\n", val_i, val_f);
}


int
_common_count_tx_power(uint8_t high_byte,
                       uint8_t low_byte,
                       char *buf_p) {
    /* [Note]
     *   Measured TX output power in mW. Represented as a 16 bit unsigned
     *   integer with the power defined as the full 16 bit value (0-65535)
     *   with LSB equal to 0.1 uW, yielding a total range of 0 to 6.5535 mW
     *   (~ -40 to +8.2 dBm). Data is assumed to be based on measurement of
     *   laser monitor photodiode current. It is factory calibrated to absolute
     *   units using the most representative fiber output type. Accuracy is
     *   vendor specific but must be better than 3dB over specified temperature
     *   and voltage. Data is not valid when the transmitter is disabled.
     */
    int total = 0;
    int lmax  = 8;
    int val_i = 0;
    int val_f = 0;
    /* unit: 0.1 uW (1mW=1000uW) */
    total = transform_word_to_int(high_byte, low_byte);
    val_i = ((total/10) / 1000);
    val_f = ((total/10) - (val_i*1000));
    /* Return Unit: 1 mW */
    return snprintf(buf_p, lmax, "%d.%03d\n", val_i, val_f);
}


int
_common_count_rx_power(uint8_t high_byte,
                       uint8_t low_byte,
                       char *buf_p) {
    /* [Note]
     *   Measured RX received optical power in mW. Value can represent either
     *   average received power or OMA depending upon how bit 3 of byte 92 (A0h)
     *   is set. Represented as a 16 bit unsigned integer with the power defined
     *   as the full 16 bit value (0-65535) with LSB equal to 0.1 uW, yielding a
     *   total range of 0 to 6.5535 mW (~ -40 to +8.2 dBm). Absolute accuracy is
     *   dependent upon the exact optical wavelength. For the vendor specified
     *   wavelength, accuracy shall be better than 3dB over specified temperature
     *   and voltage.
     */
    int total = 0;
    int lmax  = 8;
    int val_i = 0;
    int val_f = 0;
    /* unit: 0.1 uW (1mW=1000uW) */
    total = transform_word_to_int(high_byte, low_byte);
    val_i = ((total/10) / 1000);
    val_f = ((total/10) - (val_i*1000));
    /* Return Unit: 1 mW */
    return snprintf(buf_p, lmax, "%d.%03d\n", val_i, val_f);
}


int
_common_count_wavelength(struct transvr_obj_s *self,
                         uint8_t high_byte,
                         uint8_t low_byte) {
    /* [Note]
     *  SFP : uint 1 um.
     *  QSFP: unit 0.05 um.
     */
    int total = 0;

    total = transform_word_to_int(high_byte, low_byte);
    switch (self->type) {
        case TRANSVR_TYPE_SFP:
            return total;

        case TRANSVR_TYPE_QSFP:
        case TRANSVR_TYPE_QSFP_PLUS:
        case TRANSVR_TYPE_QSFP_28:
            return (total/20);

        default:
            break;
    }
    return ERR_TRANSVR_UNDEFINED;
}


int
common_get_id(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_common_update_attr_id,
                                  "common_get_id");
    if (err_code < 0){
        return err_code;
    }
    /* Transform to INT to show error case */
    return (int)self->id;
}


int
common_get_ext_id(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_common_update_attr_extended_id,
                                  "common_get_ext_id");
    if (err_code < 0){
        return err_code;
    }
    /* Transform to INT to show error case */
    return (int)self->ext_id;
}


int
common_get_connector(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_common_update_attr_connector,
                                  "common_get_connector");
    if (err_code < 0){
        return err_code;
    }
    /* Transform to INT to show error case */
    return (int)self->connector;
}


int
common_get_vendor_name(struct transvr_obj_s *self, char *buf){

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return snprintf(buf, LEN_TRANSVR_M_STR, "%s\n", self->vendor_name);
    }
    err = _check_by_mode(self,
                         &_common_update_attr_vendor_name,
                         "common_get_vendor_name");
    memset(buf, 0, LEN_TRANSVR_M_STR);
    if (err < 0){
        return snprintf(buf, LEN_TRANSVR_M_STR, "%d\n", err);
    }
    return snprintf(buf, LEN_TRANSVR_M_STR, "%s\n", self->vendor_name);
}


int
common_get_vendor_pn(struct transvr_obj_s *self, char *buf) {

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return snprintf(buf, LEN_TRANSVR_M_STR, "%s\n", self->vendor_pn);
    }
    err = _check_by_mode(self,
                         &_common_update_attr_vendor_pn,
                         "common_get_vendor_pn");
    memset(buf, 0, LEN_TRANSVR_M_STR);
    if (err < 0){
        return snprintf(buf, LEN_TRANSVR_M_STR, "%d\n", err);
    }
    return snprintf(buf, LEN_TRANSVR_M_STR, "%s\n", self->vendor_pn);
}


int
common_get_vendor_rev(struct transvr_obj_s *self, char *buf) {

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return snprintf(buf, LEN_TRANSVR_M_STR, "%s\n", self->vendor_rev);
    }
    err = _check_by_mode(self,
                         &_common_update_attr_vendor_rev,
                         "common_get_vendor_rev");
    memset(buf, 0, LEN_TRANSVR_M_STR);
    if (err < 0){
        return snprintf(buf, LEN_TRANSVR_M_STR, "%d\n", err);
    }
    return snprintf(buf, LEN_TRANSVR_M_STR, "%s\n", self->vendor_rev);
}


int
common_get_vendor_sn(struct transvr_obj_s *self, char *buf) {

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return snprintf(buf, LEN_TRANSVR_M_STR, "%s\n", self->vendor_sn);
    }
    err = _check_by_mode(self,
                         &_common_update_attr_vendor_sn,
                         "common_get_vendor_sn");
    memset(buf, 0, LEN_TRANSVR_M_STR);
    if (err < 0){
        return snprintf(buf, LEN_TRANSVR_M_STR, "%d\n", err);
    }
    return snprintf(buf, LEN_TRANSVR_M_STR, "%s\n", self->vendor_sn);
}


int
common_get_br(struct transvr_obj_s *self){

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return (int)self->br;
    }
    err = _check_by_mode(self,
                         &_common_update_attr_br,
                         "common_get_br");
    if (err < 0){
        return err;
    }
    /* Transform to INT to show error case */
    return (int)self->br;
}


int
common_get_len_smf(struct transvr_obj_s *self){

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return self->len_smf;
    }
    err = _check_by_mode(self,
                         &_common_update_attr_len_smf,
                         "common_get_len_smf");
    if (err < 0){
        return err;
    }
    return self->len_smf;
}


int
common_get_len_om1(struct transvr_obj_s *self){

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return self->len_om1;
    }
    err = _check_by_mode(self,
                         &_common_update_attr_len_om1,
                         "common_get_len_om1");
    if (err < 0){
        return err;
    }
    return self->len_om1;
}


int
common_get_len_om2(struct transvr_obj_s *self){

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return self->len_om2;
    }

    err = _check_by_mode(self,
                         &_common_update_attr_len_om2,
                         "common_get_len_om2");
    if (err < 0){
        return err;
    }
    return self->len_om2;
}


int
common_get_len_om3(struct transvr_obj_s *self){

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return self->len_om3;
    }

    err = _check_by_mode(self,
                         &_common_update_attr_len_om3,
                         "common_get_len_om3");
    if (err < 0){
        return err;
    }
    return self->len_om3;
}


int
common_get_len_om4(struct transvr_obj_s *self){

    int err = DEBUG_TRANSVR_INT_VAL;

    if (self->state == STATE_TRANSVR_CONNECTED &&
        self->mode == TRANSVR_MODE_POLLING &&
        TRANSVR_INFO_CACHE_ENABLE) {
        return self->len_om4;
    }
    err = _check_by_mode(self,
                         &_common_update_attr_len_om4,
                         "common_get_len_om4");
    if (err < 0){
        return err;
    }
    return self->len_om4;
}


int
common_get_comp_extended(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_common_update_attr_transvr_comp_ext,
                                  "common_get_comp_extended");
    if (err_code < 0){
        return err_code;
    }
    return self->transvr_comp_ext;
}


int
common_get_comp_rev(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_common_update_attr_comp_rev,
                                  "common_get_comp_rev");
    if (err_code < 0){
        return err_code;
    }
    return self->comp_rev;
}


int
common_get_info(struct transvr_obj_s *self){

    if (self->state != STATE_TRANSVR_CONNECTED) {
        return self->state;
    }
    return self->info;
}


int
_common_get_if_lane(struct transvr_obj_s *self,
                    char *result){
    int  i = 0;
    int  tmp_val = 0;
    char tmp_str[LEN_TRANSVR_M_STR] = DEBUG_TRANSVR_STR_VAL;

    memset(result, 0, LEN_TRANSVR_M_STR);

    for (i=0; i<ARRAY_SIZE(self->lane_id); i++) {
        tmp_val = self->lane_id[i];
        if (tmp_val < 1) {
            break;
        }
        memset(tmp_str, 0, LEN_TRANSVR_M_STR);
        if (i == 0) {
            snprintf(tmp_str, LEN_TRANSVR_M_STR, "%d", tmp_val);
        } else {
            snprintf(tmp_str, LEN_TRANSVR_M_STR, ",%d", tmp_val);
        }
        strncat(result, tmp_str, LEN_TRANSVR_M_STR);
    }
    if (i == 0) {
        return EVENT_TRANSVR_TASK_FAIL;
    }
    return 0;
}


int
common_get_if_lane(struct transvr_obj_s *self,
                   char *buf_p){

    char tmp_str[LEN_TRANSVR_M_STR] = DEBUG_TRANSVR_STR_VAL;

    if (self->ioexp_obj_p->state != STATE_IOEXP_NORMAL) {
        return snprintf(buf_p, LEN_TRANSVR_M_STR, "%d\n", ERR_TRANSVR_ABNORMAL);
    }
    if (_common_get_if_lane(self, tmp_str) < 0) {
        return snprintf(buf_p, LEN_TRANSVR_M_STR, "%d\n" ,ERR_TRANSVR_ABNORMAL);
    }
    return snprintf(buf_p, LEN_TRANSVR_M_STR, "%s\n" ,tmp_str);
}


int
sfp_get_len_sm(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_sfp_update_attr_len_sm,
                                  "sfp_get_len_sm");
    if (err_code < 0){
        return err_code;
    }
    return self->len_sm;
}


int
sfp_get_rate_id(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_sfp_update_attr_rate_id,
                                  "sfp_get_rate_id");
    if (err_code < 0){
        return err_code;
    }
    return self->rate_id;
}


int
sfp_get_soft_rs0(struct transvr_obj_s *self){
    /* Note:
     *   SFP Soft Rate_Select Select [aka. "RS(0)"] address
     *   A2h, offset: 110, bit 3 (begin form 0)
     */
    int err_code    = DEBUG_TRANSVR_INT_VAL;
    int bit_shift   = 3;
    uint8_t result  = 0x00;
    uint8_t bitmask = (1 << bit_shift);

    /* Check rate identifier is supported */
    err_code = self->get_rate_id(self);
    if (err_code <= 0) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    /* Update and check */
    err_code = _check_by_mode(self,
                              &_sfp_update_attr_soft_rs0,
                              "sfp_get_soft_rs0");
    if (err_code <0){
        return err_code;
    }
    result = (self->soft_rs0 & bitmask);
    if (result == bitmask) {
        return 1;
    }
    if (result == 0) {
        return 0;
    }
    return ERR_TRANSVR_UNEXCPT;
}


int
sfp_get_soft_rs1(struct transvr_obj_s *self){
    /* Note:
     *   SFP Soft RS(1) Select address
     *   A2h, offset: 118, bit 3 (begin form 0)
     */
    int err_code = DEBUG_TRANSVR_INT_VAL;
    int bit_shift = 3;
    uint8_t result = 0x00;
    uint8_t bitmask = (1 << bit_shift);

    /* Check rate identifier is supported */
    err_code = self->get_rate_id(self);
    if (err_code <= 0) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    /* Update and check */
    err_code = _check_by_mode(self,
                              &_sfp_update_attr_soft_rs1,
                              "sfp_get_soft_rs1");
    if (err_code <0){
        return err_code;
    }
    result = (self->soft_rs1 & bitmask);
    if (result == bitmask) {
        return 1;
    }
    if (result == 0) {
        return 0;
    }
    return ERR_TRANSVR_UNEXCPT;
}


int
sfp_get_transvr_temp(struct transvr_obj_s *self,
                     char *buf_p) {

    int lmax = 8;
    int err_code = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_sfp_update_attr_curr_temp,
                              "sfp_get_transvr_temp");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_temp[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    return _common_count_temp(self->curr_temp[0],
                              self->curr_temp[1],
                              buf_p);
}


int
sfp_get_transvr_voltage(struct transvr_obj_s *self,
                        char *buf_p) {

    int lmax = 8;
    int err_code = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_sfp_update_attr_curr_voltage,
                              "sfp_get_transvr_voltage");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_voltage[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* Return Unit: 1 Volt */
    return _common_count_voltage(self->curr_voltage[0],
                                 self->curr_voltage[1],
                                 buf_p);
}


int
sfp_get_transvr_tx_bias(struct transvr_obj_s *self,
                        char *buf_p) {

    int lmax = 8;
    int err_code = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_sfp_update_attr_curr_tx_bias,
                              "sfp_get_transvr_tx_bias");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_tx_bias[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* Return Unit: 1 mA */
    return _common_count_tx_bias(self->curr_tx_bias[0],
                                 self->curr_tx_bias[1],
                                 buf_p);
}


int
sfp_get_transvr_tx_power(struct transvr_obj_s *self,
                         char *buf_p) {

    int lmax = 8;
    int err_code = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_sfp_update_attr_curr_tx_power,
                              "sfp_get_transvr_tx_power");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_tx_bias[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* Return Unit: 1 mW */
    return _common_count_tx_power(self->curr_tx_power[0],
                                  self->curr_tx_power[1],
                                  buf_p);
}


int
sfp_get_transvr_rx_power(struct transvr_obj_s *self,
                         char *buf_p) {

    int lmax = 8;
    int err_code = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_sfp_update_attr_curr_rx_power,
                              "sfp_get_transvr_rx_power");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_tx_bias[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* Return Unit: 1 mW */
    return _common_count_rx_power(self->curr_rx_power[0],
                                  self->curr_rx_power[0],
                                  buf_p);
}


int
sfp_get_transvr_rx_em(struct transvr_obj_s *self,
                      char *buf_p) {

    int limt = 8;
    int err  = DEBUG_TRANSVR_INT_VAL;

    err = _check_by_mode(self,
                         &_sfp_update_attr_rx_em,
                         "sfp_get_transvr_rx_em");
    if (err < 0) {
        return snprintf(buf_p, limt, "%d\n", err);
    }
    if ((self->tx_eq[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, limt, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    return snprintf(buf_p, limt, "0x%02x\n", self->rx_em[0]);
}


int
sfp_get_transvr_tx_eq(struct transvr_obj_s *self,
                      char *buf_p) {

    int limt = 8;
    int err  = DEBUG_TRANSVR_INT_VAL;

    err = _check_by_mode(self,
                         &_sfp_update_attr_tx_eq,
                         "sfp_get_transvr_tx_eq");
    if (err < 0) {
        return snprintf(buf_p, limt, "%d\n", err);
    }
    if ((self->tx_eq[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, limt, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    return snprintf(buf_p, limt, "0x%02x\n", self->tx_eq[0]);
}


int
_sfp_get_comp_extended(struct transvr_obj_s *self) {
    /* Address: A0h / 36
     * Reference: SFF-8024 TABLE 4-4
     */
    if ((self->state == STATE_TRANSVR_CONNECTED) ||
        (self->state == STATE_TRANSVR_INIT) ) {
        return (int)(self->transvr_comp_ext);
    }
    return ERR_TRANSVR_ABNORMAL;
}


int
__sfp_get_comp_attr(struct transvr_obj_s *self,
                     int array_offset) {
    /* SFP Specification Compliance: A0h / 3-10
     * transvr_comp[0-7] = 3 - 10
     */
    if ((self->state == STATE_TRANSVR_CONNECTED) ||
        (self->state == STATE_TRANSVR_INIT) ) {
        return (int)(self->transvr_comp[array_offset]);
    }
    return ERR_TRANSVR_ABNORMAL;
}


int
_sfp_get_comp_10g_eth_comp(struct transvr_obj_s *self) {
    /* transvr_comp[0] = address A0h / 3
     *
     * 3 7: 10G Base-ER
     * 3 6: 10GBASE-LRM
     * 3 5: 10GBASE-LR
     * 3 4: 10GBASE-SR
     */
    int bitmask = 0xf0; /* 11110000 */
    return (__sfp_get_comp_attr(self, 0) & bitmask);
}


int
_sfp_get_comp_1g_eth_comp(struct transvr_obj_s *self) {
    /* transvr_comp[3] = address A0h / 6
     *
     * 6 7: BASE-PX *3
     * 6 6: BASE-BX10 *3
     * 6 5: 100BASE-FX
     * 6 4: 100BASE-LX/LX10
     * 6 3: 1000BASE-T
     * 6 2: 1000BASE-CX
     * 6 1: 1000BASE-LX *3
     * 6 0: 1000BASE-SX
     */
    return __sfp_get_comp_attr(self, 3);
}


int
_sfp_get_cable_tech(struct transvr_obj_s *self) {
    /* transvr_comp[5] = address A0h / 8
     *
     * 8 3: Active Cable *8
     * 8 2: Passive Cable *8
     */
    int bitmask = 0x0c; /* 00001100 */
    return (__sfp_get_comp_attr(self, 5) & bitmask);
}


int
sfp_get_comp_eth_1(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_common_update_attr_transvr_comp,
                                  "sfp_get_comp_eth_1");
    if (err_code < 0){
        return err_code;
    }
    return _sfp_get_comp_1g_eth_comp(self);
}


int
sfp_get_comp_eth_10(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_common_update_attr_transvr_comp,
                                  "sfp_get_comp_eth_10");
    if (err_code < 0){
        return err_code;
    }
    return _sfp_get_comp_10g_eth_comp(self);
}


int
_sfp_get_connector_type(struct transvr_obj_s *self) {
    /* Address: A0h / 2
     * Reference: SFF-8024 TABLE 4-3
     */
    if ((self->state == STATE_TRANSVR_CONNECTED) ||
        (self->state == STATE_TRANSVR_INIT) ) {
        return (int)(self->connector);
    }
    return ERR_TRANSVR_ABNORMAL;
}


int
sfp_get_wavelength(struct transvr_obj_s *self,
                   char *buf_p) {
    /* [Note] Optical and Cable Variants Specification Compliance (SFF-8472)
     * [Addr] A0h, Bytes 60-61
     * [Note] For optical variants, as defined by having zero's in A0h Byte 8
     *        bits 2 and 3, Bytes 60 and 61 denote nominal transmitter output
     *        wavelength at room temperature. 16 bit value with byte 60 as high
     *        order byte and byte 61 as low order byte. The laser wavelength is
     *        equal to the 16 bit integer value in nm. This field allows the user
     *        to read the laser wavelength directly, so it is not necessary to
     *        infer it from the Transceiver Codes A0h Bytes 3 to 10 (see Table
     *        5-3). This also allows specification of wavelengths not covered
     *        in the Transceiver Codes, such as those used in coarse WDM systems.
     *
     *        For passive and active cable variants, a value of 00h for both A0h
     *        Byte 60 and Byte 61 denotes laser wavelength or cable specification
     *        compliance is unspecified.
     */

    int lmax = 8;
    int err_code = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_common_update_attr_wavelength,
                              "common_get_wavelength");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->wavelength[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* unit: 1 um */
    return snprintf(buf_p, lmax, "%d\n",
            _common_count_wavelength(self,
                                     self->wavelength[0],
                                     self->wavelength[1]));
}


int
sfp_get_1g_rj45_extphy_offset(struct transvr_obj_s *self, char *buf) {

    if (self->state != STATE_TRANSVR_CONNECTED) {
        return ERR_TRANSVR_UNPLUGGED;
    }
    if ((self->info != TRANSVR_CLASS_BASE_T_1000) &&
        (self->info != TRANSVR_CLASS_BASE_T_1000_up) ){
        return ERR_TRANSVR_NOTSUPPORT;
    }
    return snprintf(buf, LEN_TRANSVR_S_STR, "0x%02x\n", self->extphy_offset);
}


int
sfp_get_1g_rj45_extphy_reg(struct transvr_obj_s *self, char *buf) {

    int i      = 0;
    int ret    = 0;
    int retry  = 3;
    int delay  = 200;

    if (self->state != STATE_TRANSVR_CONNECTED) {
        return ERR_TRANSVR_UNPLUGGED;
    }
    if ((self->info != TRANSVR_CLASS_BASE_T_1000) &&
        (self->info != TRANSVR_CLASS_BASE_T_1000_up) ){
        return ERR_TRANSVR_NOTSUPPORT;
    }
    if (_common_setup_page(self, VAL_TRANSVR_EXTPHY_ADDR_56,
                           -1, self->extphy_offset, 1, 0) < 0) {
        return -EIO;
    }
    for (i=0; i<retry; i++) {
        ret = i2c_smbus_read_word_data(self->i2c_client_p, self->extphy_offset);
        if (ret >=0) {
            goto ok_sfp_get_1g_rj45_extphy_reg;
        }
        msleep(delay);
    }
    SWPS_INFO("%s: retry:%d fail <port>:%s <offset>:0x%02x\n",
              __func__, retry, self->swp_name, self->extphy_offset);
    return -EIO;

ok_sfp_get_1g_rj45_extphy_reg:
    ret = ((ret & 0x00ff) << 8) | ((ret & 0xff00) >> 8);
    return snprintf(buf, LEN_TRANSVR_S_STR, "0x%04x\n", ret);
}


int
__qsfp_get_power_cls(struct transvr_obj_s *self,
                     int direct_access){

    int err_code;
    uint8_t detect_val;

    /* Detect and Update power class attribute */
    if (direct_access){
        err_code = _check_by_mode(self,
                                  &_common_update_attr_extended_id,
                                  "__qsfp_get_power_cls");
    } else {
        err_code = self->ext_id;
    }
    if (err_code <0){
        return err_code;
    }
    if (err_code == DEBUG_TRANSVR_HEX_VAL){
        return ERR_TRANSVR_UPDATE_FAIL;
    }
    /* Clean data */
    detect_val = self->ext_id;
    SWP_BIT_CLEAR(detect_val, 2); /* Bit2: CDR RX present */
    SWP_BIT_CLEAR(detect_val, 3); /* Bit3: CDR TX present */
    SWP_BIT_CLEAR(detect_val, 4); /* Bit4: CLEI present   */
    SWP_BIT_CLEAR(detect_val, 5); /* Bit5: reserved       */
    /* Identify power class */
    switch (detect_val) {
        case 0:   /* Class_1: 00000000 */
            return 1;
        case 64:  /* Class_2: 01000000 */
            return 2;
        case 128: /* Class_3: 10000000 */
            return 3;
        case 192: /* Class_4: 11000000 */
            return 4;
        case 1:   /* Class_5: 00000001 */
        case 193: /* Class_5: 11000001 */
            return 5;
        case 2:   /* Class_6: 00000010 */
        case 194: /* Class_6: 11000010 */
            return 6;
        case 3:   /* Class_7: 00000011 */
        case 195: /* Class_7: 11000011 */
            return 7;
        default:
            break;
    }
    SWPS_INFO("%s: Detect undefined power class:%d\n", __func__, detect_val);
    return ERR_TRANSVR_UNDEFINED;
}


int
qsfp_get_power_cls(struct transvr_obj_s *self) {
    return __qsfp_get_power_cls(self, 1);
}


int
__qsfp_get_cdr_present(struct transvr_obj_s *self,
                       int direct_access){

    int retval;
    int BIT_SHIFT = 2;
    int BIT_MASK  = 0x3;

    /* Detect and Update power class attribute */
    if (direct_access) {
        retval = _check_by_mode(self,
                                &_common_update_attr_extended_id,
                                "__qsfp_get_cdr_present");
        if (retval < 0){
            return retval;
        }
    }
    retval = self->ext_id;
    if (retval == DEBUG_TRANSVR_HEX_VAL){
        return ERR_TRANSVR_UPDATE_FAIL;
    }
    /* Clean data and return */
    return (int)(retval >> BIT_SHIFT & BIT_MASK);
}


int
qsfp_get_cdr_present(struct transvr_obj_s *self) {
    return __qsfp_get_cdr_present(self, 1);
}


int
qsfp_get_cdr(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_qsfp_update_attr_cdr,
                                  "qsfp_get_cdr");
    if (err_code <0){
        return err_code;
    }

    return self->cdr;
}


int
__qsfp_get_comp_attr(struct transvr_obj_s *self,
                     int array_offset) {
    /* QSFP Specification Compliance: 00h / 131-138
     * transvr_comp[0-7] = 131 - 138
     */
    if ((self->state == STATE_TRANSVR_CONNECTED) ||
        (self->state == STATE_TRANSVR_INIT) ) {
        return (int)(self->transvr_comp[array_offset]);
    }
    return ERR_TRANSVR_ABNORMAL;
}


int
_qsfp_get_comp_10_40_100_ethernet(struct transvr_obj_s *self) {
    /* transvr_comp[0] = address 00h / 131
     *
     * 131 7: Extended: See section 6.3.23. The Extended Specification Compliance
     *        Codes are maintained in the Transceiver Management section of SFF-
     *        8024.
     * 131 6: 10GBASE-LRM
     * 131 5: 10GBASE-LR
     * 131 4: 10GBASE-SR
     * 131 3: 40GBASE-CR4
     * 131 2: 40GBASE-SR4
     * 131 1: 40GBASE-LR4
     * 131 0: 40G Active Cable (XLPPI)
     */
    return __qsfp_get_comp_attr(self, 0);
}


int
_qsfp_get_comp_sonet(struct transvr_obj_s *self) {
    /* transvr_comp[1] = address 00h / 132
     *
     * 132 7-3: Reserved
     * 132 2: OC 48, long reach
     * 132 1: OC 48, intermediate reach
     * 132 0: OC 48 short reach
     */
    return __qsfp_get_comp_attr(self, 1);
}


int
_qsfp_get_comp_sas_sata(struct transvr_obj_s *self) {
    /* transvr_comp[1] = address 00h / 132
     *
     * 133 7: SAS 24.0 Gb/s
     * 133 6: SAS 12.0 Gb/s
     * 133 5: SAS 6.0 Gb/s
     * 133 4: SAS 3.0 Gb/s
     * 133 3-0: Reserved
     */
    return __qsfp_get_comp_attr(self, 2);
}


int
_qsfp_get_comp_ethernet(struct transvr_obj_s *self) {
    /* transvr_comp[1] = address 00h / 132
     *
     * 134 7-4: Reserved
     * 134 3: 1000BASE-T
     * 134 2: 1000BASE-CX
     * 134 1: 1000BASE-LX
     * 134 0: 1000BASE-SX
     */
    return __qsfp_get_comp_attr(self, 3);
}


int
_qsfp_get_comp_fc_link_length(struct transvr_obj_s *self) {
    /* transvr_comp[1] = address 00h / 132
     *
     * 135 7: Very long distance (V)
     * 135 6: Short distance (S)
     * 135 5: Intermediate distance (I)
     * 135 4: Long distance (L)
     * 135 3: Medium (M)
     */
    int mask = 0xFC; /* 11111100 */
    return (__qsfp_get_comp_attr(self, 4) & mask);
}


int
_qsfp_get_comp_fc_trans_tech(struct transvr_obj_s *self) {
    /* transvr_comp[1] = address 00h / 132
     *
     * 135 2: Reserved
     * 135 1: Longwave laser (LC)
     * 135 0: Electrical inter-enclosure (EL)
     *
     * 136 7: Electrical intra-enclosure
     * 136 6: Shortwave laser w/o OFC (SN)
     * 136 5: Shortwave laser w OFC (SL)
     * 136 4: Longwave Laser (LL)
     * 136 3-0: Reserved
     *
     * return value = [bit 8-15:addr 135][bit 0-7:addr 136]
     */
    int mask_135 = 7; /* 00000111 */
    int val_135  = (__qsfp_get_comp_attr(self, 4) & mask_135);
    int val_136  = __qsfp_get_comp_attr(self, 5);
    return ((val_135 << 7) + val_136);
}


int
_qsfp_get_comp_fc_trans_media(struct transvr_obj_s *self) {
    /* transvr_comp[1] = address 00h / 132
     *
     * 137 7: Twin Axial Pair (TW)
     * 137 6: Shielded Twisted Pair (TP)
     * 137 5: Miniature Coax (MI)
     * 137 4: Video Coax (TV)
     * 137 3: Multi-mode 62.5 m (M6)
     * 137 2: Multi-mode 50 m (M5)
     * 137 1: Multi-mode 50 um (OM3)
     * 137 0: Single Mode (SM)
     */
    return __qsfp_get_comp_attr(self, 6);
}


int
_qsfp_get_comp_fc_speed(struct transvr_obj_s *self) {
    /* transvr_comp[1] = address 00h / 132
     *
     * 138 7: 1200 MBps (per channel)
     * 138 6: 800 MBps
     * 138 5: 1600 MBps (per channel)
     * 138 4: 400 MBps
     * 138 3: 3200 MBps (per channel)
     * 138 2: 200 MBps
     * 138 1: Extended: See section 6.3.23. The Extended Specification
     *        Compliance Codes are maintained in the Transceiver Management
     *        section of SFF-8024.
     * 138 0: 100 MBps
     */
    return __qsfp_get_comp_attr(self, 7);
}


int
_qsfp_get_comp_extended(struct transvr_obj_s *self) {
    /* Address: 00h / 192
     * Reference: SFF-8024 TABLE 4-4
     */
    if ((self->state == STATE_TRANSVR_CONNECTED) ||
        (self->state == STATE_TRANSVR_INIT) ) {
        return (int)(self->transvr_comp_ext);
    }
    return ERR_TRANSVR_ABNORMAL;
}


int
qsfp_get_comp_eth(struct transvr_obj_s *self){

    int err_code = _check_by_mode(self,
                                  &_common_update_attr_transvr_comp,
                                  "qsfp_get_comp_eth");
    if (err_code < 0){
        return err_code;
    }
    return _qsfp_get_comp_ethernet(self);
}


int
qsfp_get_comp_10_40(struct transvr_obj_s *self) {

    int err_code = _check_by_mode(self,
                                  &_common_update_attr_transvr_comp,
                                  "qsfp_get_comp_10_40");
    if (err_code < 0){
        return err_code;
    }
    return _qsfp_get_comp_10_40_100_ethernet(self);
}


int
_qsfp_get_connector_type(struct transvr_obj_s *self) {
    /* Address: 00h / 130
     * Reference: SFF-8024 TABLE 4-3
     */
    if ((self->state == STATE_TRANSVR_CONNECTED) ||
        (self->state == STATE_TRANSVR_INIT) ) {
        return (int)(self->connector);
    }
    return ERR_TRANSVR_ABNORMAL;
}


int
qsfp_get_transvr_temp(struct transvr_obj_s *self,
                     char *buf_p) {

    int lmax = 8;
    int err_code = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_qsfp_update_attr_curr_temp,
                              "qsfp_get_transvr_temp");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_temp[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    return _common_count_temp(self->curr_temp[0],
                              self->curr_temp[1],
                              buf_p);
}


int
qsfp_get_transvr_voltage(struct transvr_obj_s *self,
                         char *buf_p) {

    int lmax = 8;
    int err_code = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_qsfp_update_attr_curr_voltage,
                              "qsfp_get_transvr_voltage");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_voltage[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* Return Unit: 1 Volt */
    return _common_count_voltage(self->curr_voltage[0],
                                 self->curr_voltage[1],
                                 buf_p);
}


int
qsfp_get_transvr_tx_eq(struct transvr_obj_s *self,
                       char *buf_p) {

    int limt = 8;
    int err  = DEBUG_TRANSVR_INT_VAL;

    err = _check_by_mode(self,
                         &_qsfp_update_attr_tx_eq,
                         "qsfp_get_transvr_tx_eq");
    if (err < 0) {
        return snprintf(buf_p, limt, "%d\n", err);
    }
    if ((self->tx_eq[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, limt, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    return snprintf(buf_p, limt, "0x%02x%02x\n",
                    self->tx_eq[0], self->tx_eq[1]);
}


int
qsfp_get_transvr_rx_am(struct transvr_obj_s *self,
                       char *buf_p) {

    int limt = 8;
    int err  = DEBUG_TRANSVR_INT_VAL;

    err = _check_by_mode(self,
                         &_qsfp_update_attr_rx_am,
                         "qsfp_get_transvr_rx_am");
    if (err < 0) {
        return snprintf(buf_p, limt, "%d\n", err);
    }
    if ((self->rx_am[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, limt, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    return snprintf(buf_p, limt, "0x%02x%02x\n",
                    self->rx_am[0], self->rx_am[1]);
}


int
qsfp_get_transvr_rx_em(struct transvr_obj_s *self,
                       char *buf_p) {

    int limt = 8;
    int err  = DEBUG_TRANSVR_INT_VAL;

    err = _check_by_mode(self,
                         &_qsfp_update_attr_rx_em,
                         "qsfp_get_transvr_rx_em");
    if (err < 0) {
        return snprintf(buf_p, limt, "%d\n", err);
    }
    if ((self->rx_em[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, limt, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    return snprintf(buf_p, limt, "0x%02x%02x\n",
                    self->rx_em[0], self->rx_em[1]);
}


int
_qsfp_get_channel_diag(uint8_t *data_array,
                       int (*count_func)(uint8_t high_byte, uint8_t low_byte, char *buf_p),
                       char *ch_name,
                       char *result_p) {
    int i, high, low;
    int len_max = 128;
    char ch_buf[4][16] = { DEBUG_TRANSVR_STR_VAL,
                           DEBUG_TRANSVR_STR_VAL,
                           DEBUG_TRANSVR_STR_VAL,
                           DEBUG_TRANSVR_STR_VAL };

    for (i=0; i<4; i++) {
        high = (i*2);
        low  = ((i*2) + 1);
        count_func(data_array[high], data_array[low], ch_buf[i]);
    }
    return snprintf(result_p, len_max,
                    "%s-%d:%s%s-%d:%s%s-%d:%s%s-%d:%s",
                    ch_name, 1, ch_buf[0],
                    ch_name, 2, ch_buf[1],
                    ch_name, 3, ch_buf[2],
                    ch_name, 4, ch_buf[3]);
}


int
qsfp_get_soft_rx_los(struct transvr_obj_s *self,
                     char *buf_p) {

    int lmax      = 8;
    int mask      = 0x0f; /* Bit 0 ~ Bit 3 */
    int err_code  = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_qsfp_update_attr_soft_rx_los,
                              "qsfp_get_soft_rx_los");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    return snprintf(buf_p, lmax, "0x%02x\n", (self->rx_los & mask));
}


int
qsfp_get_soft_tx_disable(struct transvr_obj_s *self,
                         char *buf_p) {

    int lmax      = 8;
    int mask      = 0x0f; /* Bit 0 ~ Bit 3 */
    int err_code  = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_qsfp_update_attr_soft_tx_disable,
                              "qsfp_get_soft_tx_disable");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    return snprintf(buf_p, lmax, "0x%02x\n", (self->tx_disable & mask));
}


int
qsfp_get_soft_tx_fault(struct transvr_obj_s *self,
                       char *buf_p) {

    int lmax      = 8;
    int mask      = 0x0f; /* Bit 0 ~ Bit 3 */
    int err_code  = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_qsfp_update_attr_soft_tx_fault,
                              "qsfp_get_soft_tx_fault");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    return snprintf(buf_p, lmax, "0x%02x\n", (self->tx_fault & mask));
}


int
qsfp_get_auto_tx_disable(struct transvr_obj_s *self,
                         char *buf_p) {

    if (self->auto_tx_disable == VAL_TRANSVR_FUNCTION_DISABLE) {
        return snprintf(buf_p, LEN_TRANSVR_S_STR,
                        "%d\n", ERR_TRANSVR_FUNC_DISABLE);
    }
    return snprintf(buf_p, LEN_TRANSVR_S_STR,
                    "0x%02x\n", self->auto_tx_disable);
}


int
qsfp_get_transvr_tx_bias(struct transvr_obj_s *self,
                         char *buf_p) {

    int lmax      = 8;
    int err_code  = DEBUG_TRANSVR_INT_VAL;
    char *ch_name = "TX";

    err_code = _check_by_mode(self,
                              &_qsfp_update_attr_curr_tx_bias,
                              "qsfp_get_transvr_tx_bias");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_tx_bias[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* Return Unit: 1 mA */
    return _qsfp_get_channel_diag(self->curr_tx_bias,
                                  _common_count_tx_bias,
                                  ch_name,
                                  buf_p);
}


int
qsfp_get_transvr_tx_power(struct transvr_obj_s *self,
                          char *buf_p) {

    int lmax      = 8;
    int err_code  = DEBUG_TRANSVR_INT_VAL;
    char *ch_name = "TX";

    err_code = _check_by_mode(self,
                              &_qsfp_update_attr_curr_tx_power,
                              "qsfp_get_transvr_tx_power");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_tx_power[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* Return Unit: 1 mW */
    return _qsfp_get_channel_diag(self->curr_tx_power,
                                  _common_count_tx_power,
                                  ch_name,
                                  buf_p);
}


int
qsfp_get_transvr_rx_power(struct transvr_obj_s *self,
                          char *buf_p) {

    int lmax      = 8;
    int err_code  = DEBUG_TRANSVR_INT_VAL;
    char *ch_name = "RX";

    err_code = _check_by_mode(self,
                              &_qsfp_update_attr_curr_rx_power,
                              "qsfp_get_transvr_rx_power");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->curr_tx_power[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* Return Unit: 1 mW */
    return _qsfp_get_channel_diag(self->curr_rx_power,
                                  _common_count_rx_power,
                                  ch_name,
                                  buf_p);
}


int
qsfp_get_wavelength(struct transvr_obj_s *self,
                    char *buf_p) {
    /* [Desc] Wavelength or Copper Cable Attenuation (SFF-8636)
     * [Addr] 00h 186-187
     * [Note]
     *  For optical free side devices, this parameter identifies the nominal
     *  transmitter output wavelength at room temperature. This parameter is
     *  a 16-bit hex value with Byte 186 as high order byte and Byte 187 as
     *  low order byte. The laser wavelength is equal to the 16-bit integer value
     *  divided by 20 in nm (units of 0.05 nm). This resolution should be adequate
     *  to cover all relevant wavelengths yet provide enough resolution for all
     *  expected DWDM applications. For accurate representation of controlled
     *  wavelength applications, this value should represent the center of the
     *  guaranteed wavelength range.
     *  If the free side device is identified as copper cable these registers will
     *  be used to define the cable attenuation. An indication of 0 dB attenuation
     *  refers to the case where the attenuation is not known or is unavailable.
     *  Byte 186 (00-FFh) is the copper cable attenuation at 2.5 GHz in units of 1 dB.
     *  Byte 187 (00-FFh) is the copper cable attenuation at 5.0 GHz in units of 1 dB.
     */
    int lmax     = 8;
    int err_code = DEBUG_TRANSVR_INT_VAL;

    err_code = _check_by_mode(self,
                              &_common_update_attr_wavelength,
                              "common_get_wavelength");
    if (err_code < 0) {
        return snprintf(buf_p, lmax, "%d\n", err_code);
    }
    if ((self->wavelength[0]) == DEBUG_TRANSVR_HEX_VAL) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_UPDATE_FAIL);
    }
    /* unit: 1 um */
    return snprintf(buf_p, lmax, "%d\n",
            _common_count_wavelength(self,
                                     self->wavelength[0],
                                     self->wavelength[1]));
}


/*  Public Function for Setup Features
 */
static int
__sfp_set_soft_rs(struct transvr_obj_s *self,
                  int input_val,
                  int address,
                  int page,
                  int offset,
                  int bit_shift,
                  uint8_t *attr_p,
                  char *caller,
                  int show_err) {

    int   retval   = ERR_TRANSVR_UNEXCPT;
    int   err_code = ERR_TRANSVR_UNEXCPT;
    char *err_msg  = DEBUG_TRANSVR_STR_VAL;
    uint8_t update_val = (*attr_p);

    switch (input_val) {
       case 0:
           SWP_BIT_CLEAR(update_val, bit_shift);
           break;
       case 1:
           SWP_BIT_SET(update_val, bit_shift);
           break;
       default:
           retval   = ERR_TRANSVR_UNEXCPT;
           err_code = ERR_TRANSVR_UNEXCPT;
           err_msg  = "Exception occurs";
           goto err_private_sfp_set_soft_rs_1;
    }
    err_code = _common_set_uint8_attr(self,
                                      address,
                                      page,
                                      offset,
                                      update_val,
                                      attr_p,
                                      caller,
                                      show_err);
    if (err_code < 0) {
        retval   = err_code;
        err_msg  = "Write data via i2c fail!";
        goto err_private_sfp_set_soft_rs_1;
    }
    (*attr_p) = update_val;
    return 0;

err_private_sfp_set_soft_rs_1:
    if (show_err) {
        SWPS_INFO("%s: %s <ERR>:%d <Port>:%s\n <Input>:%d\n",
                   __func__, err_msg, err_code, self->swp_name, input_val);
    }
    return retval;
}


static int
_sfp_set_soft_rs(struct transvr_obj_s *self,
                 int input_val,
                 int address,
                 int page,
                 int offset,
                 int bit_shift,
                 int (*attr_update_func)(struct transvr_obj_s *self, int show_err),
                 uint8_t *attr_p,
                 char *caller,
                 int show_err) {

    int retval    = ERR_TRANSVR_UNEXCPT;
    int err_code  = ERR_TRANSVR_UNEXCPT;
    char *err_msg = DEBUG_TRANSVR_STR_VAL;

    /* Check input value */
    if ((input_val != 0) && (input_val != 1)){
        retval   = ERR_TRANSVR_BADINPUT;
        err_code = ERR_TRANSVR_BADINPUT;
        err_msg  = "Input range incorrect!";
        goto err_common_sfp_set_soft_rs_1;
    }
    /* Check rate identifier is supported */
    err_code = self->get_rate_id(self);
    if (err_code <= 0) {
        switch (err_code) {
            case 0:
                retval  = ERR_TRANSVR_NOTSUPPORT;
                err_msg = "Not support this feature";
                break;
            case ERR_TRANSVR_UNINIT:
                retval  = ERR_TRANSVR_UNINIT;
                err_msg = "Check CDR present fail!";
                break;
            case ERR_TRANSVR_UNPLUGGED:
                retval  = ERR_TRANSVR_UNPLUGGED;
                err_msg = "Transceiver unplugged!";
                break;
            default:
                retval  = err_code;
                err_msg = "Check Rate_ID fail!";
                break;
        }
        goto err_common_sfp_set_soft_rs_1;
    }
    /* Check and update */
    err_code = _check_by_mode(self,
                              attr_update_func,
                              caller);
    if ( (err_code < 0) ||
         ((*attr_p) == DEBUG_TRANSVR_HEX_VAL) ){
        retval  = err_code;
        err_msg = "Get current value fail!";
        goto err_common_sfp_set_soft_rs_1;
    }
    /* Generate and update value */
    return __sfp_set_soft_rs(self,
                             input_val,
                             address,
                             page,
                             offset,
                             bit_shift,
                             attr_p,
                             caller,
                             show_err);

err_common_sfp_set_soft_rs_1:
    if (show_err) {
        SWPS_INFO("%s: %s <ERR>:%d <Port>:%s\n <Input>:%d\n",
                  __func__, err_msg, err_code, self->swp_name, input_val);
    }
    return retval;
}


int
sfp_set_soft_rs0(struct transvr_obj_s *self,
                 int input_val) {
    /* Note:
     *   SFP Soft Rate_Select Select RX ["RS(0)"] address
     *   A2h, offset: 110, bit 3
     */
    int bit_shift = 3;
    int show_err  = 1;
    return _sfp_set_soft_rs(self,
                            input_val,
                            self->eeprom_map_p->addr_soft_rs0,
                            self->eeprom_map_p->page_soft_rs0,
                            self->eeprom_map_p->offset_soft_rs0,
                            bit_shift,
                            &_sfp_update_attr_soft_rs0,
                            &(self->soft_rs0),
                            "sfp_set_soft_rs0",
                            show_err);
}


int
sfp_set_soft_rs1(struct transvr_obj_s *self,
                 int input_val) {
    /* Note:
     *   SFP Soft Rate_Select Select RX ["RS(1)"] address
     *   A2h, offset: 118, bit 3
     */
    int bit_shift = 3;
    int show_err  = 1;
    return _sfp_set_soft_rs(self,
                            input_val,
                            self->eeprom_map_p->addr_soft_rs1,
                            self->eeprom_map_p->page_soft_rs1,
                            self->eeprom_map_p->offset_soft_rs1,
                            bit_shift,
                            &_sfp_update_attr_soft_rs1,
                            &(self->soft_rs1),
                            "sfp_set_soft_rs1",
                            show_err);
}


int
__sfp_set_tx_eq(struct transvr_obj_s *self,
                int input,
                int show_e) {

    int     err  = DEBUG_TRANSVR_INT_VAL;
    char   *emsg = DEBUG_TRANSVR_STR_VAL;
    uint8_t setv = DEBUG_TRANSVR_HEX_VAL;

    if ((input < 0) || (input > 0xFF)) {
        emsg = "input incorrect";
        err = ERR_TRANSVR_BADINPUT;
        goto err_sfp_set_tx_eq;
    }
    setv = (uint8_t)input;
    if (self->tx_eq[0] == setv) {
        return 0;
    }
    err = _common_set_uint8_attr(self,
                                 self->eeprom_map_p->addr_tx_eq,
                                 self->eeprom_map_p->page_tx_eq,
                                 self->eeprom_map_p->offset_tx_eq,
                                 setv,
                                 &(self->tx_eq[0]),
                                 "_sfp_set_tx_eq",
                                 show_e);
    if (err < 0) {
        emsg = "set_uint8_attr fail";
        goto err_sfp_set_tx_eq;
    }
    return 0;

err_sfp_set_tx_eq:
    if (show_e) {
        SWPS_INFO("%s: %s <input>:%d\n", __func__, emsg, input);
    }
    return err;
}


int
_sfp_set_tx_eq(struct transvr_obj_s *self,
               int input,
               int show_e) {

    uint8_t tmp;
    int i = 0;
    int retry = 3;

    for (i=0; i<retry; i++) {
        if (__sfp_set_tx_eq(self, input, show_e) < 0){
            continue;
        }
        tmp = self->tx_eq[0];
        if (_sfp_update_attr_tx_eq(self, show_e) < 0){
            continue;
        }
        if (self->tx_eq[0] == tmp){
            return 0;
        }
    }
    return ERR_TRANSVR_UPDATE_FAIL;
}


int
sfp_set_tx_eq(struct transvr_obj_s *self,
              int input) {

    int err = _check_by_mode(self,
                             &_sfp_update_attr_tx_eq,
                             "sfp_set_tx_eq");
    if (err < 0) {
        SWPS_DEBUG("%s: check fail <err>:%d\n", __func__, err);
        return err;
    }
    return _sfp_set_tx_eq(self, input, 1);
}


int
__sfp_set_rx_em(struct transvr_obj_s *self,
                int input,
                int show_e) {

    int     err  = DEBUG_TRANSVR_INT_VAL;
    char   *emsg = DEBUG_TRANSVR_STR_VAL;
    uint8_t setv = DEBUG_TRANSVR_HEX_VAL;

    if ((input < 0) || (input > 0xFF)) {
        emsg = "input incorrect";
        err = ERR_TRANSVR_BADINPUT;
        goto err_sfp_set_rx_em;
    }
    setv = (uint8_t)input;
    if (self->rx_em[0] == setv) {
        return 0;
    }
    err = _common_set_uint8_attr(self,
                                 self->eeprom_map_p->addr_rx_em,
                                 self->eeprom_map_p->page_rx_em,
                                 self->eeprom_map_p->offset_rx_em,
                                 setv,
                                 &(self->rx_em[0]),
                                 "_sfp_set_rx_em",
                                 show_e);
    if (err < 0) {
        emsg = "set_uint8_attr fail";
        goto err_sfp_set_rx_em;
    }
    return 0;

err_sfp_set_rx_em:
    if (show_e) {
        SWPS_INFO("%s: %s <input>:%d\n", __func__, emsg, input);
    }
    return err;
}


int
_sfp_set_rx_em(struct transvr_obj_s *self,
               int input,
               int show_e) {

    uint8_t tmp;
    int i = 0;
    int retry = 3;

    for (i=0; i<retry; i++) {
        if (__sfp_set_rx_em(self, input, show_e) < 0){
            continue;
        }
        tmp = self->rx_em[0];
        if (_sfp_update_attr_rx_em(self, show_e) < 0){
            continue;
        }
        if (self->rx_em[0] == tmp){
            return 0;
        }
    }
    return -1;
}


int
sfp_set_rx_em(struct transvr_obj_s *self,
              int input) {

    int err = _check_by_mode(self,
                             &_sfp_update_attr_rx_em,
                             "sfp_set_rx_em");
    if (err < 0) {
        SWPS_DEBUG("%s: check fail <err>:%d\n", __func__, err);
        return err;
    }
    return _sfp_set_rx_em(self, input, 1);
}


int
sfp_set_1g_rj45_extphy_offset(struct transvr_obj_s *self,
                              int input) {

    if (self->state != STATE_TRANSVR_CONNECTED) {
        return ERR_TRANSVR_UNPLUGGED;
    }
    if ((self->info != TRANSVR_CLASS_BASE_T_1000) &&
        (self->info != TRANSVR_CLASS_BASE_T_1000_up) ){
        return ERR_TRANSVR_NOTSUPPORT;
    }
    if ((input < 0) || (input > 0xff)) {
        return ERR_TRANSVR_BADINPUT;
    }
    self->extphy_offset = (uint8_t)input;
    return 0;
}


int
sfp_set_1g_rj45_extphy_reg(struct transvr_obj_s *self,
                           int input) {

    int i      = 0;
    int retry  = 3;
    int delay  = 200;
    uint16_t tmp = 0;

    if (self->state != STATE_TRANSVR_CONNECTED) {
        return ERR_TRANSVR_UNPLUGGED;
    }
    if ((self->info != TRANSVR_CLASS_BASE_T_1000) &&
        (self->info != TRANSVR_CLASS_BASE_T_1000_up) ){
        return ERR_TRANSVR_NOTSUPPORT;
    }
    if ((input < 0) || (input > 0xffff)) {
        return ERR_TRANSVR_BADINPUT;
    }
    tmp = ((input & 0x00ff) << 8) | ((input & 0xff00) >> 8);
    if (_common_setup_page(self, VAL_TRANSVR_EXTPHY_ADDR_56,
                           -1, self->extphy_offset, 1, 0) < 0) {
        return -EIO;
    }
    for (i=0; i<=retry; i++) {
        if (i2c_smbus_write_word_data(self->i2c_client_p,
                                      self->extphy_offset,
                                      tmp) >= 0) {
            return 0;
        }
        msleep(delay);
    }
    SWPS_INFO("%s: retry:%d fail <port>:%s <offset>:0x%02x\n",
              __func__, retry, self->swp_name, self->extphy_offset);
    return -EIO;
}


static int
__qsfp_set_cdr(struct transvr_obj_s *self,
               int input_val,
               int show_err) {

    uint8_t update_val;
    int CDR_FEATURE_SUPPORTED = 0x3;
    int retval    = ERR_TRANSVR_UNEXCPT;
    int err_code  = ERR_TRANSVR_UNEXCPT;
    char *err_msg = DEBUG_TRANSVR_STR_VAL;
    char *func_name = "__qsfp_set_cdr";

    /* Check input value */
    if ((input_val < 0) || (input_val > 0xff)){
        retval   = ERR_TRANSVR_BADINPUT;
        err_code = ERR_TRANSVR_BADINPUT;
        err_msg  = "Input range incorrect!";
        goto err_qsfp_set_cdr_1;
    }
    update_val = (uint8_t)input_val;
    /* Check CDR supported by transceiver */
    err_code = qsfp_get_cdr_present(self);
    if (err_code < 0) {
        retval   = err_code;
        switch (err_code) {
            case ERR_TRANSVR_UNINIT:
                err_msg  = "Check CDR present fail!";
                break;
            case ERR_TRANSVR_UNPLUGGED:
                err_msg  = "Transceiver unplugged!";
                break;
            default:
                err_msg  = "Check CDR present fail!";
                break;
        }
        goto err_qsfp_set_cdr_1;
    }
    if (err_code != CDR_FEATURE_SUPPORTED) {
        retval   = ERR_TRANSVR_NOTSUPPORT;
        err_msg  = "This transceiver not support CDR!";
        goto err_qsfp_set_cdr_1;
    }
    /* Check and update */
    err_code = _check_by_mode(self,
                              &_qsfp_update_attr_cdr,
                              func_name);
    if ( (err_code < 0) ||
         (self->cdr == DEBUG_TRANSVR_HEX_VAL) ){
        retval   = err_code;
        err_msg  = "Get current value fail!";
        goto err_qsfp_set_cdr_1;
    }
    /* Write input value to transceiver */
    return _common_set_uint8_attr(self,
                                  self->eeprom_map_p->addr_cdr,
                                  self->eeprom_map_p->page_cdr,
                                  self->eeprom_map_p->offset_cdr,
                                  update_val,
                                  &(self->cdr),
                                  func_name,
                                  show_err);

err_qsfp_set_cdr_1:
    if (show_err) {
        SWPS_INFO("%s: %s <ERR>:%d <Port>:%s\n <Input>:%d\n",
                  __func__, err_msg, err_code, self->swp_name, input_val);
    }
    return retval;
}


int
qsfp_set_cdr(struct transvr_obj_s *self,
             int input_val) {
    return __qsfp_set_cdr(self, input_val, 1);
}


int
qsfp_set_soft_tx_disable(struct transvr_obj_s *self,
                         int input_val) {

    int show_err     = 1;
    int in_max       = 0xf; /* 1111 */
    int in_min       = 0x0; /* 0000 */
    int retval       = DEBUG_TRANSVR_INT_VAL;
    int update_val   = DEBUG_TRANSVR_INT_VAL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    retval = _check_by_mode(self,
                            &_qsfp_update_attr_soft_tx_disable,
                            "qsfp_set_soft_tx_disable");
    if (retval < 0) {
        snprintf(err_msg, 63, "Not ready. err:%d", retval);
        goto err_qsfp_set_soft_tx_disable;
    }
    if ((input_val > in_max) ||
        (input_val < in_min) ){
        retval = ERR_TRANSVR_BADINPUT;
        snprintf(err_msg, 63, "Input value:%d incorrect!", input_val);
        goto err_qsfp_set_soft_tx_disable;
    }
    if ((self->tx_disable & 0x0f) == input_val) {
        return 0;
    }
    update_val = ((self->tx_disable & 0xf0) & input_val);
    retval = _common_set_uint8_attr(self,
                                    self->eeprom_map_p->addr_tx_disable,
                                    self->eeprom_map_p->page_tx_disable,
                                    self->eeprom_map_p->offset_tx_disable,
                                    input_val,
                                    &(self->tx_disable),
                                    "qsfp_set_tx_disable",
                                    show_err);
    if (retval < 0) {
        snprintf(err_msg, 63, "_common_set_uint8_attr:%d fail!", retval);
        goto err_qsfp_set_soft_tx_disable;
    }
    return 0;

err_qsfp_set_soft_tx_disable:
    SWPS_INFO("%s: %s <port>:%s\n", __func__, err_msg, self->swp_name);
    return retval;
}


int
_qsfp_set_auto_tx_disable(struct transvr_obj_s *self,
                          uint8_t update) {

    uint8_t tx_enable = 0x0;
    int show_e = 1;
    int err    = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;

    /* Handle timing issues */
    if (update != tx_enable) {
        /* Note:
         *  Because there are some txvr has timing issues,
         *  therefore we need to execute reset cycle first.
         *  (enable -> other settings)
          */
        err = _common_set_uint8_attr(self,
                                     self->eeprom_map_p->addr_tx_disable,
                                     self->eeprom_map_p->page_tx_disable,
                                     self->eeprom_map_p->offset_tx_disable,
                                     tx_enable,
                                     &(self->tx_disable),
                                     "_qsfp_set_auto_tx_disable",
                                     show_e);
        if (err < 0) {
            emsg = "I2C set reset value fail";
            goto err_qsfp_set_auto_tx_disable;
        }
        mdelay(10);
    }
    /* Setup target value */
    err = _common_set_uint8_attr(self,
                                 self->eeprom_map_p->addr_tx_disable,
                                 self->eeprom_map_p->page_tx_disable,
                                 self->eeprom_map_p->offset_tx_disable,
                                 self->auto_tx_disable,
                                 &(self->tx_disable),
                                 "_qsfp_set_auto_tx_disable",
                                 show_e);
    if (err < 0) {
        emsg = "I2C set target value fail";
        goto err_qsfp_set_auto_tx_disable;
    }
    /* Check and update */
    err = _common_update_uint8_attr(self,
                                    self->eeprom_map_p->addr_tx_disable,
                                    self->eeprom_map_p->page_tx_disable,
                                    self->eeprom_map_p->offset_tx_disable,
                                    self->eeprom_map_p->length_tx_disable,
                                    &(self->tx_disable),
                                    "_qsfp_set_auto_tx_disable",
                                    show_e);
    if (err < 0) {
        emsg = "I2C get value fail";
        goto err_qsfp_set_auto_tx_disable;
    }
    if (self->tx_disable != update) {
        emsg = "data not become effective";
        goto err_qsfp_set_auto_tx_disable;
    }
    return 0;

err_qsfp_set_auto_tx_disable:
    SWPS_DEBUG("%s: %s <port>:%s\n",
               __func__, emsg, self->swp_name);
    return ERR_TRANSVR_UPDATE_FAIL;
}


int
qsfp_set_auto_tx_disable(struct transvr_obj_s *self,
                         int input_val) {

    int in_max = 0xf; /* 1111 */
    int in_min = 0x0; /* 0000 */
    int retval = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;

    /* Update settings*/
    if (input_val == VAL_TRANSVR_FUNCTION_DISABLE) {
        emsg = "User disable auto tx_disable";
        self->auto_tx_disable = VAL_TRANSVR_FUNCTION_DISABLE;
        goto out_qsfp_set_auto_tx_disable;
    }
    if ((input_val > in_max) || (input_val < in_min) ){
        SWPS_INFO("%s: Input value:%d incorrect! <port>:%s\n",
                  __func__, input_val, self->swp_name);
        return ERR_TRANSVR_BADINPUT;
    }
    self->auto_tx_disable = input_val;
    /* Check current soft tx_disable */
    retval = _check_by_mode(self,
                            &_qsfp_update_attr_soft_tx_disable,
                            "qsfp_set_auto_tx_disable");
    switch (retval) {
        case 0:
            break;
        case ERR_TRANSVR_UNPLUGGED:
            emsg = "Doesn't need to update";
            goto out_qsfp_set_auto_tx_disable;
        default:
            SWPS_INFO("%s: setup fail <err>:%d <port>:%s\n",
                      __func__, retval, self->swp_name);
            return retval;
    }
    return _qsfp_set_auto_tx_disable(self, input_val);

out_qsfp_set_auto_tx_disable:
    SWPS_DEBUG("%s: %s <port>:%s <input>:%d\n <ret>:%d",
               __func__, emsg, self->swp_name, input_val, retval);
    return 0;
}


int
__qsfp_set_tx_eq(struct transvr_obj_s *self,
                 int input,
                 int show_e) {
    /* [Note]
     *   0x<CH-1><CH-2><CH-3><CH-4>
     */
    int   err  = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;
    uint8_t setv[2] = {0x00, 0x00};

    if ((input < 0) || (input > 0xFFFF)) {
        emsg = "input incorrect";
        err = ERR_TRANSVR_BADINPUT;
        goto err_qsfp_set_tx_eq;
    }
    setv[0] = (uint8_t)((input & 0xFF00) >> 8);
    setv[1] = (uint8_t)(input & 0xFF);
    if ((self->tx_eq[0] == setv[0]) &&
        (self->tx_eq[1] == setv[1]) ) {
        return 0;
    }
    err = _common_set_uint8_array(self,
                                  self->eeprom_map_p->addr_tx_eq,
                                  self->eeprom_map_p->page_tx_eq,
                                  self->eeprom_map_p->offset_tx_eq,
                                  self->eeprom_map_p->length_tx_eq,
                                  setv,
                                  self->tx_eq,
                                  "_qsfp_set_tx_eq",
                                  show_e);
    if (err < 0) {
        emsg = "set_uint8_array fail";
        goto err_qsfp_set_tx_eq;
    }
    return 0;

err_qsfp_set_tx_eq:
    if (show_e) {
        SWPS_INFO("%s: %s <input>:%d\n", __func__, emsg, input);
    }
    return err;
}


int
_qsfp_set_tx_eq(struct transvr_obj_s *self,
                int input,
                int show_e) {

    int i = 0;
    int retry = 3;
    uint8_t tmp[2];

    for (i=0; i<retry; i++) {
        if (__qsfp_set_tx_eq(self, input, show_e) < 0){
            continue;
        }
        tmp[0] = self->tx_eq[0];
        tmp[1] = self->tx_eq[1];
        if (_qsfp_update_attr_tx_eq(self, show_e) < 0){
            continue;
        }
        if ((self->tx_eq[0] == tmp[0]) &&
            (self->tx_eq[1] == tmp[1]) ){
            return 0;
        }
    }
    return -1;
}


int
qsfp_set_tx_eq(struct transvr_obj_s *self,
               int input) {

    int err = _check_by_mode(self,
                             &_qsfp_update_attr_tx_eq,
                             "qsfp_set_tx_eq");
    if (err < 0) {
        SWPS_DEBUG("%s: check fail <err>:%d\n", __func__, err);
        return err;
    }
    return _qsfp_set_tx_eq(self, input, 1);
}


int
__qsfp_set_rx_am(struct transvr_obj_s *self,
                 int input,
                 int show_e) {
    /* [Note]
     *   0x<CH-1><CH-2><CH-3><CH-4>
     */
    int   err  = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;
    uint8_t setv[2] = {0x00, 0x00};

    if ((input < 0) || (input > 0xFFFF)) {
        emsg = "input incorrect";
        err = ERR_TRANSVR_BADINPUT;
        goto err_qsfp_set_rx_am;
    }
    setv[0] = (uint8_t)((input & 0xFF00) >> 8);
    setv[1] = (uint8_t)(input & 0xFF);
    if ((self->rx_am[0] == setv[0]) &&
        (self->rx_am[1] == setv[1]) ) {
        return 0;
    }
    err = _common_set_uint8_array(self,
                                  self->eeprom_map_p->addr_rx_am,
                                  self->eeprom_map_p->page_rx_am,
                                  self->eeprom_map_p->offset_rx_am,
                                  self->eeprom_map_p->length_rx_am,
                                  setv,
                                  self->rx_am,
                                  "_qsfp_set_rx_am",
                                  show_e);
    if (err < 0) {
        emsg = "set_uint8_array fail";
        goto err_qsfp_set_rx_am;
    }
    return 0;

err_qsfp_set_rx_am:
    if (show_e) {
        SWPS_INFO("%s: %s <input>:%d\n", __func__, emsg, input);
    }
    return err;
}


int
_qsfp_set_rx_am(struct transvr_obj_s *self,
                int input,
                int show_e) {

    int i = 0;
    int retry = 3;
    uint8_t tmp[2];

    for (i=0; i<retry; i++) {
        if (__qsfp_set_rx_am(self, input, show_e) < 0){
            continue;
        }
        tmp[0] = self->rx_am[0];
        tmp[1] = self->rx_am[1];
        if (_qsfp_update_attr_rx_am(self, show_e) < 0){
            continue;
        }
        if ((self->rx_am[0] == tmp[0]) &&
            (self->rx_am[1] == tmp[1]) ){
            return 0;
        }
    }
    return -1;
}


int
qsfp_set_rx_am(struct transvr_obj_s *self,
               int input) {

    int err = _check_by_mode(self,
                             &_qsfp_update_attr_rx_am,
                             "qsfp_set_rx_am");
    if (err < 0) {
        SWPS_DEBUG("%s: check fail <err>:%d\n", __func__, err);
        return err;
    }
    return _qsfp_set_rx_am(self, input, 1);
}


int
__qsfp_set_rx_em(struct transvr_obj_s *self,
                 int input,
                 int show_e) {
    /* [Note]
     *   0x<CH-1><CH-2><CH-3><CH-4>
     */
    int   err  = DEBUG_TRANSVR_INT_VAL;
    char *emsg = DEBUG_TRANSVR_STR_VAL;
    uint8_t setv[2] = {0x00, 0x00};

    if ((input < 0) || (input > 0xFFFF)) {
        emsg = "input incorrect";
        err = ERR_TRANSVR_BADINPUT;
        goto err_qsfp_set_rx_em;
    }
    setv[0] = (uint8_t)((input & 0xFF00) >> 8);
    setv[1] = (uint8_t)(input & 0xFF);
    if ((self->rx_em[0] == setv[0]) &&
        (self->rx_em[1] == setv[1]) ) {
        return 0;
    }
    err = _common_set_uint8_array(self,
                                  self->eeprom_map_p->addr_rx_em,
                                  self->eeprom_map_p->page_rx_em,
                                  self->eeprom_map_p->offset_rx_em,
                                  self->eeprom_map_p->length_rx_em,
                                  setv,
                                  self->rx_em,
                                  "_qsfp_set_rx_em",
                                  show_e);
    if (err < 0) {
        emsg = "set_uint8_array fail";
        goto err_qsfp_set_rx_em;
    }
    return 0;

err_qsfp_set_rx_em:
    if (show_e) {
        SWPS_INFO("%s: %s <input>:%d\n", __func__, emsg, input);
    }
    return err;
}


int
_qsfp_set_rx_em(struct transvr_obj_s *self,
                int input,
                int show_e) {

    int i = 0;
    int retry = 3;
    uint8_t tmp[2];

    for (i=0; i<retry; i++) {
        if (__qsfp_set_rx_em(self, input, show_e) < 0){
            continue;
        }
        tmp[0] = self->rx_em[0];
        tmp[1] = self->rx_em[1];
        if (_qsfp_update_attr_rx_em(self, show_e) < 0){
            continue;
        }
        if ((self->rx_em[0] == tmp[0]) &&
            (self->rx_em[1] == tmp[1]) ){
            return 0;
        }
    }
    return -1;
}


int
qsfp_set_rx_em(struct transvr_obj_s *self,
               int input) {

    int err = _check_by_mode(self,
                             &_qsfp_update_attr_rx_em,
                             "qsfp_set_rx_em");
    if (err < 0) {
        SWPS_DEBUG("%s: check fail <err>:%d\n", __func__, err);
        return err;
    }
    return _qsfp_set_rx_em(self, input, 1);
}


int
common_transvr_dump(struct transvr_obj_s* self){

    char *type_name = "Undefined";

    if (TRANSVR_INFO_DUMP_ENABLE != 1) {
        return 0;
    }
    switch (self->type) {
        case TRANSVR_TYPE_SFP:
            type_name = STR_TRANSVR_SFP;
            break;
        case TRANSVR_TYPE_QSFP:
            type_name = STR_TRANSVR_QSFP;
            break;
        case TRANSVR_TYPE_QSFP_PLUS:
            type_name = STR_TRANSVR_QSFP_PLUS;
            break;
        case TRANSVR_TYPE_QSFP_28:
            type_name = STR_TRANSVR_QSFP28;
            break;
        case TRANSVR_TYPE_FAKE:
            type_name = "FAKE";
            goto ok_common_transvr_dump;
        case TRANSVR_TYPE_UNPLUGGED:
            type_name = "UNPLUGGED";
            goto err_common_transvr_dump;
        case TRANSVR_TYPE_INCONSISTENT:
            type_name = "INCONSISTENT";
            goto err_common_transvr_dump;
        case TRANSVR_TYPE_ERROR:
            type_name = "ERROR";
            goto err_common_transvr_dump;

        default:
            type_name = "UNEXPECTED";
            goto err_common_transvr_dump;
    }
    printk(KERN_INFO "[SWPS] Dump %s information:\n", self->swp_name);
    printk(KERN_INFO "       |- <Type>:%s\n", type_name);
    printk(KERN_INFO "       |- <VenderName>:%s\n", self->vendor_name);
    printk(KERN_INFO "       |- <VenderPN>:%s\n", self->vendor_pn);
    printk(KERN_INFO "       |- <VenderREV>:%s\n", self->vendor_rev);
    printk(KERN_INFO "       |- <VenderSN>:%s\n", self->vendor_sn);
    printk(KERN_INFO "       |- <BitRate>:0x%02x\n", self->br);
    printk(KERN_INFO "       |- <RevComp>:0x%02x\n", self->comp_rev);
    printk(KERN_INFO "       |- <LenOM1>:%d\n", self->len_om1);
    printk(KERN_INFO "       |- <LenOM2>:%d\n", self->len_om2);
    printk(KERN_INFO "       |- <LenOM3>:%d\n", self->len_om3);
    printk(KERN_INFO "       |- <LenOM4>:%d\n", self->len_om4);
    return 0;

ok_common_transvr_dump:
    SWPS_INFO("%s: %s is %s\n", __func__, self->swp_name, type_name);
    return 0;

err_common_transvr_dump:
    SWPS_INFO("%s: %s is %s\n", __func__, self->swp_name, type_name);
    return -1;
}


int
sfp_transvr_dump(struct transvr_obj_s* self) {

    if (TRANSVR_INFO_DUMP_ENABLE != 1) {
        return 0;
    }
    if (common_transvr_dump(self) < 0) {
        return -1;
    }
    printk(KERN_INFO "       |- <LenSM>:%d\n", self->len_sm);
    printk(KERN_INFO "       |- <LenSMF>:%d\n", self->len_smf);
    printk(KERN_INFO "       '- <RateID>:0x%02x\n", self->rate_id);
    return 0;
}


int
qsfp_transvr_dump(struct transvr_obj_s* self) {

    if (TRANSVR_INFO_DUMP_ENABLE != 1) {
        return 0;
    }
    if (common_transvr_dump(self) < 0) {
        return -1;
    }
    printk(KERN_INFO "       |- <LenSMF>:%d\n", self->len_smf);
    printk(KERN_INFO "       '- <PowerClass>:Class_%d\n", __qsfp_get_power_cls(self, 0));
    return 0;
}


int
fake_transvr_dump(struct transvr_obj_s* self) {

    printk(KERN_INFO "[SWPS] Dump transceiver information: %s\n", self->swp_name);
    printk(KERN_INFO "       |- <Type>:FAKE\n");
    printk(KERN_INFO "       |- <VenderName>:FAKE_VENDER_NAME\n");
    printk(KERN_INFO "       |- <VenderPN>:FAKE_VENDER_PN\n");
    printk(KERN_INFO "       |- <VenderREV>:FAKE_VENDER_REV\n");
    printk(KERN_INFO "       |- <VenderSN>:FAKE_VENDER_SN\n");
    printk(KERN_INFO "       |- <BitRate>:0x99\n");
    printk(KERN_INFO "       |- <LenOM1>:99\n");
    printk(KERN_INFO "       |- <LenOM2>:99\n");
    printk(KERN_INFO "       |- <LenOM3>:99\n");
    printk(KERN_INFO "       |- <LenOM4>:99\n");
    printk(KERN_INFO "       |- <LenSM>:99\n");
    printk(KERN_INFO "       |- <LenSMF>:99\n");
    printk(KERN_INFO "       '- <RevComp>:0x99\n");
    return 0;
}


/* ========== Object functions for fake type ==========
 */
int
fake_transvr_update(struct transvr_obj_s *self,
                    int show_err){
    self->state = STATE_TRANSVR_CONNECTED;
    return 0;
}


int
fake_get_binary(struct transvr_obj_s *self){
    return 1;
}

int
fake_get_int(struct transvr_obj_s *self){
    return 99;
}


int
fake_get_hex(struct transvr_obj_s *self){
    return 0x0f;
}


int
fake_get_str(struct transvr_obj_s *self, char *buf) {
    return snprintf(buf, 16, "fake_get_str\n");
}


int
fake_set_int(struct transvr_obj_s *self, int input){
    SWPS_INFO("%s: %d\n", __func__, input);
    return 0;
}


int
fake_set_hex(struct transvr_obj_s *self, int input){
    SWPS_INFO("%s: 0x%02x\n", __func__, input);
    return 0;
}


/* ========== Object functions for unsupported ==========
 */
int
unsupported_get_func(struct transvr_obj_s *self){
    return ERR_TRANSVR_NOTSUPPORT;
}


int
unsupported_get_func2(struct transvr_obj_s *self,
                      char *buf_p) {
    int len = snprintf(buf_p, 8, "%d\n", ERR_TRANSVR_NOTSUPPORT);
    return len;
}


int
unsupported_set_func(struct transvr_obj_s *self,
                     int input_val){
    return ERR_TRANSVR_NOTSUPPORT;
}



/* ========== Object functions for long term task ==========
 *
 * [Note]
 *   SWPS transceiver worker is likely the green-thread (coroutine).
 *   Due to resource and performance considerations. SWPS run all
 *   features in one kthread at the same time, and handle by it self.
 */

/* For Transceiver Task Handling
 */
static struct transvr_worker_s *
transvr_task_get(struct transvr_obj_s *self,
                 char *func_name) {

    struct transvr_worker_s *curr_p = self->worker_p;

    while(curr_p != NULL){
        if (strcmp((curr_p->func_name), func_name) == 0 ) {
            return curr_p;
        }
        curr_p = curr_p->next_p;
    }
    return NULL;
}


static struct transvr_worker_s*
transvr_task_creat(struct transvr_obj_s *self,
                   int (*main_task)(struct transvr_worker_s *task),
                   int (*post_task)(struct transvr_worker_s *task),
                   char *caller) {

    struct transvr_worker_s *task_p = NULL;
    struct transvr_worker_s *curr_p = NULL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    /* Check task not exist */
    task_p = transvr_task_get(self, caller);
    if (task_p) {
        snprintf(err_msg, sizeof(err_msg), "Task already created!");
        goto err_transvr_task_creat;
    }
    /* Create task worker */
    task_p = kzalloc(sizeof(struct transvr_worker_s), GFP_KERNEL);
    if (!task_p){
        snprintf(err_msg, sizeof(err_msg), "kzalloc fail");
        goto err_transvr_task_creat;
    }
    /* Setup task data */
    task_p->transvr_p    = self;
    task_p->next_p       = NULL;
    task_p->trigger_time = 0;
    task_p->retry        = 1;
    task_p->state        = STATE_T_TASK_INIT;
    task_p->main_task    = main_task;
    task_p->post_task    = post_task;
    task_p->p_data       = NULL;
    snprintf(task_p->func_name, sizeof(task_p->func_name), "%s", caller);
    /* Setup Link List */
    if (self->worker_p) {
        curr_p = self->worker_p;
        while(curr_p->next_p != NULL) {
            curr_p = curr_p->next_p;
        }
        curr_p->next_p = task_p;
        task_p->pre_p  = curr_p;
    } else {
        self->worker_p = task_p;
        task_p->pre_p  = NULL;
    }
    return task_p;

err_transvr_task_creat:
    SWPS_ERR("%s: %s <caller>:%s <port>:%s\n",
            __func__, err_msg, caller, self->swp_name);
    return NULL;
}


static void
transvr_task_free_one(struct transvr_worker_s *task_p){

    struct transvr_worker_s *pre_p  = NULL;
    struct transvr_worker_s *next_p = NULL;

    if (task_p) {
        pre_p  = task_p->pre_p;
        next_p = task_p->next_p;

        if ((pre_p) && (next_p)) {
            pre_p->next_p = next_p;
            next_p->pre_p = pre_p;

        } else if ((!pre_p) && (next_p)) {
            next_p->pre_p = NULL;

        } else if ((pre_p) && (!next_p)) {
            pre_p->next_p = NULL;

        } else if ((!pre_p) && (!next_p)) {
            task_p->transvr_p->worker_p = NULL;
        } else {
            SWPS_ERR("%s: Unexcept case!\n <port>:%s",
                    __func__, task_p->transvr_p->swp_name);
        }
        kfree(task_p->p_data);
        kfree(task_p);
    }
}


static void
transvr_task_free_all(struct transvr_obj_s *self) {

    struct transvr_worker_s *curr_p = NULL;
    struct transvr_worker_s *next_p = NULL;

    if (self->worker_p) {
        curr_p = self->worker_p;
        while(curr_p) {
            next_p = curr_p->next_p;
            transvr_task_free_one(curr_p);
            curr_p = next_p;
        }
        self->worker_p = NULL;
    }
}


static void
transvr_cache_free_all(struct transvr_obj_s *self) {

    memset(self->vendor_name, 0, (LEN_TRANSVR_M_STR * sizeof(char)) );
    memset(self->vendor_rev,  0, (LEN_TRANSVR_M_STR * sizeof(char)) );
    memset(self->vendor_pn,   0, (LEN_TRANSVR_M_STR * sizeof(char)) );
    memset(self->vendor_sn,   0, (LEN_TRANSVR_M_STR * sizeof(char)) );
    self->extphy_offset = 0;
}

static int
_transvr_task_run_main(struct transvr_worker_s *task_p) {

    int retval = DEBUG_TRANSVR_INT_VAL;
    char err_msg[64]  = DEBUG_TRANSVR_STR_VAL;

    if (!task_p){
        snprintf(err_msg, sizeof(err_msg), "main_task is NULL!");
        goto main_transvr_task_err;
    }
    if ((task_p->trigger_time) == 0){
        goto main_transvr_task_run;
    }
    if (time_before(jiffies, task_p->trigger_time)){
        goto main_transvr_task_wait;
    }
    goto main_transvr_task_run;

main_transvr_task_run:
    if (task_p->retry != VAL_TRANSVR_TASK_RETRY_FOREVER) {
        task_p->retry -= 1;
    }
    retval = task_p->main_task(task_p);
    if (retval < 0) {
        if (task_p->retry > 0) {
            task_p->state = STATE_T_TASK_WAIT;
            return EVENT_TRANSVR_TASK_WAIT;
        }
        snprintf(err_msg, sizeof(err_msg), "Run main_task fail!");
        goto main_transvr_task_err;
    }
    goto main_transvr_task_identify;

main_transvr_task_identify:
    switch (retval) {
        case EVENT_TRANSVR_TASK_WAIT:
            task_p->state = STATE_T_TASK_WAIT;
            return EVENT_TRANSVR_TASK_WAIT;

        case EVENT_TRANSVR_TASK_DONE:
            task_p->state = STATE_T_TASK_DONE;
            return EVENT_TRANSVR_TASK_DONE;

    default:
        break;
    }
    snprintf(err_msg, sizeof(err_msg), "Run main_task fail!");
    goto main_transvr_task_err;

main_transvr_task_wait:
    task_p->state = STATE_T_TASK_WAIT;
    return EVENT_TRANSVR_TASK_WAIT;

main_transvr_task_err:
    task_p->state = STATE_T_TASK_FAIL;
    SWPS_INFO("%s: %s <rval>:%d <port>:%s\n",
            __func__, err_msg, retval, task_p->transvr_p->swp_name);
    return EVENT_TRANSVR_INIT_FAIL;
}


static int
_transvr_task_run_post(struct transvr_worker_s *task_p) {

    char err_msg[64]  = DEBUG_TRANSVR_STR_VAL;

    if ((task_p->post_task) == NULL) {
        return EVENT_TRANSVR_TASK_DONE;
    }
    switch (task_p->state) {
        case STATE_T_TASK_WAIT:
        case STATE_T_TASK_INIT:
            goto post_transvr_task_wait;

        case STATE_T_TASK_DONE:
        case STATE_T_TASK_FAIL:
            goto post_transvr_task_run;

    default:
        break;
    }
    snprintf(err_msg, sizeof(err_msg), "Unexcept task state");
    goto post_transvr_task_err;

post_transvr_task_run:
    task_p->post_task(task_p);
    return EVENT_TRANSVR_TASK_DONE;

post_transvr_task_wait:
    return EVENT_TRANSVR_TASK_WAIT;

post_transvr_task_err:
    SWPS_INFO("%s: %s <state>:%d <port>:%s\n",
              __func__, err_msg, task_p->state, task_p->transvr_p->swp_name);
    return EVENT_TRANSVR_TASK_FAIL;
}


static int
transvr_task_run_one(struct transvr_worker_s *task_p) {

    int retval_main  = DEBUG_TRANSVR_INT_VAL;
    int retval_post  = DEBUG_TRANSVR_INT_VAL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    retval_main = _transvr_task_run_main(task_p);
    if (retval_main < 0) {
        snprintf(err_msg, sizeof(err_msg), "Execute main_task fail!");
        goto err_transvr_task_run_one;
    }
    retval_post = _transvr_task_run_post(task_p);
    if (retval_post < 0) {
        snprintf(err_msg, sizeof(err_msg), "Execute post_task fail!");
        goto err_transvr_task_run_one;
    }
    return retval_main;

err_transvr_task_run_one:
    SWPS_INFO("%s: %s <main>:%d <post>:%d <caller>:%s <port>:%s\n",
              __func__, err_msg, retval_main, retval_post,
              task_p->func_name, task_p->transvr_p->swp_name);
    return EVENT_TRANSVR_TASK_FAIL;
}


static int
transvr_task_run_all(struct transvr_obj_s *self) {

    int haserr = 0;
    int retval = DEBUG_TRANSVR_INT_VAL;
    struct transvr_worker_s *curr_p = NULL;
    struct transvr_worker_s *next_p = NULL;

    if ((self->worker_p) == NULL) {
        return EVENT_TRANSVR_TASK_DONE;
    }
    curr_p = self->worker_p;
    while (curr_p != NULL) {
        next_p = curr_p->next_p;
        retval = transvr_task_run_one(curr_p);
        if (curr_p->retry == VAL_TRANSVR_TASK_RETRY_FOREVER) {
            curr_p = next_p;
            continue;
        }
        switch (retval) {
            case EVENT_TRANSVR_TASK_WAIT:
                break;
            case EVENT_TRANSVR_TASK_DONE:
                transvr_task_free_one(curr_p);
                break;
            case EVENT_TRANSVR_TASK_FAIL:

            default:
                haserr = 1;
                transvr_task_free_one(curr_p);
                break;
        }
        curr_p = next_p;
    }
    if (haserr) {
        return EVENT_TRANSVR_TASK_FAIL;
    }
    return EVENT_TRANSVR_TASK_DONE;
}


static void
transvr_task_set_delay(struct transvr_worker_s *task_p,
                       unsigned long delay_msec) {

    task_p->trigger_time = (jiffies + (delay_msec * (HZ/1000)));
}


static void
transvr_task_set_retry(struct transvr_worker_s *task_p,
                       unsigned long retry_times) {

    task_p->retry = retry_times;
}


/* For Transceiver Post Task
 */
int
taskfunc_post_do_nothing(struct transvr_worker_s *task_p) {

    return EVENT_TRANSVR_TASK_DONE;
}


int
taskfunc_post_handle_task_state(struct transvr_worker_s *task_p) {

    struct transvr_obj_s* tp = task_p->transvr_p;

    switch (task_p->state) {
        case STATE_T_TASK_INIT:
        case STATE_T_TASK_WAIT:
            return EVENT_TRANSVR_TASK_WAIT;

        case STATE_T_TASK_DONE:
            tp->state = STATE_TRANSVR_CONNECTED;
            tp->send_uevent(tp, KOBJ_ADD);
            return EVENT_TRANSVR_TASK_DONE;

        case STATE_T_TASK_FAIL:
            tp->state = STATE_TRANSVR_UNEXCEPTED;
            return EVENT_TRANSVR_TASK_FAIL;

        default:
            break;
    }
    return EVENT_TRANSVR_TASK_FAIL;
}


/* For Transceiver Main Task
 */
int
_taskfunc_sfp_setup_soft_rs(struct transvr_worker_s *task_p,
                            int input_val,
                            int address,
                            int page,
                            int offset,
                            int bit_shift,
                            uint8_t *attr_p,
                            char *caller) {

    int show_err = 0;
    int err_code = DEBUG_TRANSVR_INT_VAL;
    char *err_str  = DEBUG_TRANSVR_STR_VAL;
    char *func_str = "_taskfunc_sfp_setup_soft_rs";

    err_code = _sfp_update_attr_soft_rs0(task_p->transvr_p, 0);
    if (err_code < 0) {
        err_str = "Get current soft_rs0 fail!";
        goto err_taskfunc_sfp_setup_soft_rs_1;
    }
    err_code = __sfp_set_soft_rs(task_p->transvr_p,
                                 input_val,
                                 address,
                                 page,
                                 offset,
                                 bit_shift,
                                 attr_p,
                                 caller,
                                 show_err);
    if (err_code < 0) {
        err_str = "Get current soft_rs0 fail!";
        goto err_taskfunc_sfp_setup_soft_rs_1;
    }
    return EVENT_TRANSVR_TASK_DONE;

err_taskfunc_sfp_setup_soft_rs_1:
    if ((task_p->retry) == 0) {
        SWPS_INFO("%s: %s <port>:%s <input>:%d <err>:%d\n",
                  func_str, err_str, task_p->transvr_p->swp_name, input_val, err_code);
    }
    return EVENT_TRANSVR_TASK_FAIL;
}


int
__taskfunc_sfp_setup_hard_rs(struct transvr_worker_s *task_p,
                             int input_val,
                             int (*get_func)(struct ioexp_obj_s *self, int virt_offset),
                             int (*set_func)(struct ioexp_obj_s *self, int virt_offset, int input_val)) {

    int err_val    = EVENT_TRANSVR_EXCEP_EXCEP;
    char *err_str  = DEBUG_TRANSVR_STR_VAL;

    err_val = get_func(task_p->transvr_p->ioexp_obj_p,
                       task_p->transvr_p->ioexp_virt_offset);

    if (err_val < 0) {
        if (err_val == ERR_IOEXP_NOTSUPPORT) {
            return EVENT_TRANSVR_TASK_DONE;
        }
        err_str = "Get current hard_rs fail!";
        goto err_p_taskfunc_sfp_setup_hard_rs_1;
    }
    if (err_val == input_val) {
        return EVENT_TRANSVR_TASK_DONE;
    }
    err_val = set_func(task_p->transvr_p->ioexp_obj_p,
                       task_p->transvr_p->ioexp_virt_offset,
                       input_val);
    if (err_val < 0) {
        err_str = "Setup hard_rs fail!";
        goto err_p_taskfunc_sfp_setup_hard_rs_1;
    }
    return EVENT_TRANSVR_TASK_DONE;

err_p_taskfunc_sfp_setup_hard_rs_1:
    if ((task_p->retry) == 0) {
        SWPS_INFO("%s: %s <port>:%s <input>:%d <err>:%d\n",
                  __func__, err_str, task_p->transvr_p->swp_name, input_val, err_val);
    }
    return EVENT_TRANSVR_TASK_FAIL;
}


int
_taskfunc_sfp_setup_hard_rs0(struct transvr_worker_s *task_p,
                             int input_val) {

    return __taskfunc_sfp_setup_hard_rs(task_p,
                                        input_val,
                                        task_p->transvr_p->ioexp_obj_p->get_hard_rs0,
                                        task_p->transvr_p->ioexp_obj_p->set_hard_rs0);
}


int
_taskfunc_sfp_setup_hard_rs1(struct transvr_worker_s *task_p,
                             int input_val) {

    return __taskfunc_sfp_setup_hard_rs(task_p,
                                        input_val,
                                        task_p->transvr_p->ioexp_obj_p->get_hard_rs1,
                                        task_p->transvr_p->ioexp_obj_p->set_hard_rs1);
}


int
_taskfunc_sfp_setup_rs0(struct transvr_worker_s *task_p,
                        int input_val) {

    int  bit_shift = 3;
    int  old_val   = DEBUG_TRANSVR_INT_VAL;
    int  err_val   = EVENT_TRANSVR_EXCEP_EXCEP;
    char *err_str  = DEBUG_TRANSVR_STR_VAL;
    char *func_str = "_taskfunc_sfp_setup_rs0";

    err_val = _taskfunc_sfp_setup_hard_rs0(task_p,
                                           input_val);
    if (err_val < 0) {
        err_str = "Setup hard_rs0 fail!";
        goto err_private_taskfunc_sfp_setup_rs0_1;
    }
    old_val = err_val;
    err_val = _taskfunc_sfp_setup_soft_rs(task_p,
                                          input_val,
                                          task_p->transvr_p->eeprom_map_p->addr_soft_rs0,
                                          task_p->transvr_p->eeprom_map_p->page_soft_rs0,
                                          task_p->transvr_p->eeprom_map_p->offset_soft_rs0,
                                          bit_shift,
                                          &(task_p->transvr_p->soft_rs0),
                                          func_str);
    if (err_val < 0) {
        err_str = "Setup soft_rs0 fail!";
        goto err_private_taskfunc_sfp_setup_rs0_1;
    }
    return EVENT_TRANSVR_TASK_DONE;

err_private_taskfunc_sfp_setup_rs0_1:
    if ((task_p->retry) == 0) {
        SWPS_INFO("%s: %s <port>:%s <input>:%d <err>:%d\n",
                  func_str, err_str, task_p->transvr_p->swp_name, input_val, err_val);
    }
    _taskfunc_sfp_setup_hard_rs0(task_p, old_val);
    return EVENT_TRANSVR_TASK_FAIL;
}


int
_taskfunc_sfp_setup_rs1(struct transvr_worker_s *task_p,
                        int input_val) {

    int  bit_shift = 3;
    int  old_val   = DEBUG_TRANSVR_INT_VAL;
    int  err_val   = EVENT_TRANSVR_EXCEP_EXCEP;
    char *err_str  = DEBUG_TRANSVR_STR_VAL;
    char *func_str = "_taskfunc_sfp_setup_rs1";

    err_val = _taskfunc_sfp_setup_hard_rs1(task_p,
                                           input_val);
    if (err_val < 0) {
        err_str = "Setup hard_rs1 fail!";
        goto err_private_taskfunc_sfp_setup_rs1_1;
    }
    old_val = err_val;
    err_val = _taskfunc_sfp_setup_soft_rs(task_p,
                                          input_val,
                                          task_p->transvr_p->eeprom_map_p->addr_soft_rs1,
                                          task_p->transvr_p->eeprom_map_p->page_soft_rs1,
                                          task_p->transvr_p->eeprom_map_p->offset_soft_rs1,
                                          bit_shift,
                                          &(task_p->transvr_p->soft_rs1),
                                          func_str);
    if (err_val < 0) {
        err_str = "Setup soft_rs1 fail!";
        goto err_private_taskfunc_sfp_setup_rs1_1;
    }
    return EVENT_TRANSVR_TASK_DONE;

err_private_taskfunc_sfp_setup_rs1_1:
    if ((task_p->retry) == 0) {
        SWPS_INFO("%s: %s <port>:%s <input>:%d <err>:%d\n",
                  func_str, err_str, task_p->transvr_p->swp_name, input_val, err_val);
    }
    _taskfunc_sfp_setup_hard_rs1(task_p, old_val);
    return EVENT_TRANSVR_TASK_FAIL;
}


int
taskfunc_sfp_setup_SFF8431_case1(struct transvr_worker_s *task_p) {
    /* SFF-8431 (8/4/2G Rx Rate_Select only) */
    int update_val = 1;

    return _taskfunc_sfp_setup_rs0(task_p, update_val);
}



int
taskfunc_sfp_setup_SFF8431_case2(struct transvr_worker_s *task_p) {
    /* SFF-8431 (8/4/2G Tx Rate_Select only) */
    int update_val = 1;

    return _taskfunc_sfp_setup_rs1(task_p, update_val);
}


int
taskfunc_sfp_setup_SFF8431_case3(struct transvr_worker_s *task_p) {
    /* SFF-8431 (8/4/2G Independent Rx & Tx Rate_select) */
    int update_rs0 = 1;
    int update_rs1 = 1;
    int err_code   = DEBUG_TRANSVR_INT_VAL;

    err_code = _taskfunc_sfp_setup_rs0(task_p, update_rs0);
    if (err_code < 0) {
        return err_code;
    }
    return _taskfunc_sfp_setup_rs1(task_p, update_rs1);
}


int
taskfunc_sfp_handle_1g_rj45(struct transvr_worker_s *task_p) {

    /* Not all of platform support 0x56 for transceiver
     * external PHY, Support list as below:
     * => 1. Magnolia-PVT (PS: EVT & DVT not ready)
     */
    int ext_phy_addr   = 0x56;
    int ext_phy_page   = -1;
    int ext_phy_offs   = 0x11;
    int ext_phy_len    = 1;
    int lstate_mask    = 0x04; /* 00000100 */
    int show_err       = 0;
    int fail_retry     = 5;
    int fail_delay     = 1000; /* msec */
    int err_code       = DEBUG_TRANSVR_INT_VAL;
    uint8_t detect_val = DEBUG_TRANSVR_HEX_VAL;
    char err_str[64]   = DEBUG_TRANSVR_STR_VAL;
    int *tmp_p         = NULL;
    char *func_name    = "taskfunc_sfp_handle_1g_rj45";

    if (task_p->transvr_p->state != STATE_TRANSVR_CONNECTED) {
        return EVENT_TRANSVR_TASK_DONE;
    }
    if ( (task_p->transvr_p->info != TRANSVR_CLASS_BASE_T_1000) &&
         (task_p->transvr_p->info != TRANSVR_CLASS_BASE_T_1000_up) ) {
        goto err_taskfunc_sfp_handle_1g_rj45_1;
    }
    err_code = _common_update_uint8_attr(task_p->transvr_p,
                                         ext_phy_addr,
                                         ext_phy_page,
                                         ext_phy_offs,
                                         ext_phy_len,
                                         &detect_val,
                                         func_name,
                                         show_err);
    if ( (err_code < 0) ||
         (detect_val == DEBUG_TRANSVR_HEX_VAL) ) {
        snprintf(err_str, sizeof(err_str), "Detect external link status fail");
        goto err_taskfunc_sfp_handle_1g_rj45_2;
    }
    if ((detect_val & lstate_mask) == lstate_mask) {
        goto ok_taskfunc_sfp_handle_1g_rj45_link_up;
    }
    goto ok_taskfunc_sfp_handle_1g_rj45_link_down;

ok_taskfunc_sfp_handle_1g_rj45_link_up:
    /* Filter out noise */
    if (!(task_p->p_data)) {
        tmp_p = kzalloc(sizeof(int), GFP_KERNEL);
        if (!tmp_p) {
            snprintf(err_str, sizeof(err_str), "kzalloc p_data fail");
            goto err_taskfunc_sfp_handle_1g_rj45_2;
        }
        *tmp_p = TRANSVR_CLASS_BASE_T_1000_up;
        task_p->p_data = tmp_p;
        goto ok_taskfunc_sfp_handle_1g_rj45_done;
    }
    if ((*(int *)(task_p->p_data)) != TRANSVR_CLASS_BASE_T_1000_up) {
        kfree(task_p->p_data);
        task_p->p_data = NULL;
        snprintf(err_str, sizeof(err_str), "Internal error");
        goto err_taskfunc_sfp_handle_1g_rj45_2;
    }
    task_p->transvr_p->info = TRANSVR_CLASS_BASE_T_1000_up;
    kfree(task_p->p_data);
    task_p->p_data = NULL;
    goto ok_taskfunc_sfp_handle_1g_rj45_done;

ok_taskfunc_sfp_handle_1g_rj45_link_down:
    if (task_p->p_data) {
        kfree(task_p->p_data);
        task_p->p_data = NULL;
    }
    task_p->transvr_p->info = TRANSVR_CLASS_BASE_T_1000;
    goto ok_taskfunc_sfp_handle_1g_rj45_done;

ok_taskfunc_sfp_handle_1g_rj45_done:
    if (task_p->retry != VAL_TRANSVR_TASK_RETRY_FOREVER) {
        transvr_task_set_retry(task_p, VAL_TRANSVR_TASK_RETRY_FOREVER);
    }
    return EVENT_TRANSVR_TASK_DONE;

err_taskfunc_sfp_handle_1g_rj45_1:
    snprintf(err_str, sizeof(err_str), "Detect transceiver:%d not Base-T, remove task.",
            task_p->transvr_p->info);
    SWPS_INFO("%s: %s <port>:%s\n", __func__, err_str, task_p->transvr_p->swp_name);
    transvr_task_set_retry(task_p, 0);
    return EVENT_TRANSVR_TASK_DONE;

err_taskfunc_sfp_handle_1g_rj45_2:
    if (task_p->retry == VAL_TRANSVR_TASK_RETRY_FOREVER) {
        transvr_task_set_retry(task_p, fail_retry);
    }
    if ((task_p->retry) == 0) {
        /* Error case:
         * => In this case, SWPS will stop external Link state monitor features
         *    and keeps transvr_p->info on TRANSVR_CLASS_BASE_T_1000_up.
         *    Upper layer will see it always Linkup that because of these type of
         *    transceiver has external phy, BCM chip see it as Loopback transceiver.
         */
        SWPS_WARN("%s can not access external PHY of Base-T SFP transceiver\n",
                task_p->transvr_p->swp_name);
        task_p->transvr_p->info = TRANSVR_CLASS_BASE_T_1000_up;
        return EVENT_TRANSVR_TASK_DONE;
    } else {
        transvr_task_set_delay(task_p, fail_delay);
    }
    return EVENT_TRANSVR_TASK_FAIL;
}


int
_taskfunc_qsfp_setup_power_mod(struct transvr_obj_s *self,
                               int setup_val) {

    int curr_val   = DEBUG_TRANSVR_INT_VAL;
    int err_val    = DEBUG_TRANSVR_INT_VAL;
    char *err_msg  = DEBUG_TRANSVR_STR_VAL;
    if (io_no_init) {

        SWPS_INFO("%s no_io_init\n",__func__);
        return EVENT_TRANSVR_TASK_DONE;
    }

    curr_val = self->ioexp_obj_p->get_lpmod(self->ioexp_obj_p,
                                            self->ioexp_virt_offset);
    if (curr_val < 0){
        err_msg = "Get current value fail!";
        goto err_private_taskfunc_qsfp_setup_power_mod_1;
    }
    if (curr_val == setup_val){
        return EVENT_TRANSVR_TASK_DONE;
    }
    err_val = self->ioexp_obj_p->set_lpmod(self->ioexp_obj_p,
                                           self->ioexp_virt_offset,
                                           setup_val);
    if (err_val < 0){
        err_msg = "Setup power mode fail!";
        goto err_private_taskfunc_qsfp_setup_power_mod_1;
    }
    return EVENT_TRANSVR_TASK_DONE;

err_private_taskfunc_qsfp_setup_power_mod_1:
    SWPS_INFO("%s: %s <err>:%d <curr>:%d <input>:%d\n",
              __func__, err_msg, err_val, curr_val, setup_val);
    return EVENT_TRANSVR_TASK_FAIL;
}


int
taskfunc_qsfp_handle_tx_disable(struct transvr_worker_s *task_p) {

    int i = 0;
    int retry = 5;
    int delay_ms = 100;

    if (task_p->transvr_p->auto_tx_disable == VAL_TRANSVR_FUNCTION_DISABLE) {
        return EVENT_TRANSVR_TASK_DONE;
    }
    if (!_qsfp_is_implement_tx_disable(task_p->transvr_p)) {
        return EVENT_TRANSVR_TASK_DONE;
    }
    for (i=0; i<retry; i++) {
        if (_qsfp_set_auto_tx_disable(task_p->transvr_p,
                                      task_p->transvr_p->auto_tx_disable)
                                      == EVENT_TRANSVR_TASK_DONE) {
            goto ok_taskfunc_qsfp_handle_tx_disable;
        }
        mdelay(delay_ms);
    }
    SWPS_INFO("%s auto setup tx_disable:0x%02x fail.\n",
              task_p->transvr_p->swp_name,
              task_p->transvr_p->auto_tx_disable);
    return EVENT_TRANSVR_INIT_FAIL;

ok_taskfunc_qsfp_handle_tx_disable:
    SWPS_INFO("%s auto setup tx_disable:0x%02x ok.\n",
              task_p->transvr_p->swp_name,
              task_p->transvr_p->auto_tx_disable);
    return EVENT_TRANSVR_TASK_DONE;
}


int
taskfunc_qsfp_set_hpmod(struct transvr_worker_s *task_p) {

    int err = DEBUG_TRANSVR_INT_VAL;
    int HIGH_POWER_MODE = 0;

    /* Handle power mode */
    err = _taskfunc_qsfp_setup_power_mod(task_p->transvr_p,
                                         HIGH_POWER_MODE);
    if (err < 0) {
        SWPS_INFO("%s: setup hpmod fail <err>:%d <port>:%s\n",
                  __func__, err, task_p->transvr_p->swp_name);
        return err;
    }
    /* Handle auto tx_disable
     * [Note]
     * => Because there are some transceiver have timing issues or
     *    setup sequence issues, therefore we handle auto tx_disable
     *    after handle power mode.
     */
    mdelay(100);
    return taskfunc_qsfp_handle_tx_disable(task_p);
}


int
taskfunc_qsfp_set_lpmod(struct transvr_worker_s *task_p) {

    int LOW_POWER_MODE = 1;
    return _taskfunc_qsfp_setup_power_mod(task_p->transvr_p,
                                          LOW_POWER_MODE);
}


static int
initfunc_sfp_handle_multi_rate_mode(struct transvr_obj_s *self) {

    int task_retry = 3;
    int err_code   = DEBUG_TRANSVR_INT_VAL;
    char *err_str  = DEBUG_TRANSVR_STR_VAL;
    char *func_str = "sfp_handle_multi_rate_mode";
    struct transvr_worker_s *task_p = NULL;

    switch (self->rate_id) {
        case 0x00: /* Unspecified */
        case 0x03: /* Unspecified */
        case 0x05: /* Unspecified */
        case 0x07: /* Unspecified */
        case 0x09: /* Unspecified */
        case 0x0B: /* Unspecified */
        case 0x0D: /* Unspecified */
        case 0x0F: /* Unspecified */
            goto sfp_handle_multi_rate_mode_4_unspecified;

        case 0x02: /* SFF-8431 (8/4/2G Rx Rate_Select only) */
            task_p = transvr_task_creat(self,
                                        taskfunc_sfp_setup_SFF8431_case1,
                                        taskfunc_post_handle_task_state,
                                        func_str);
            goto sfp_handle_multi_rate_mode_4_sff8431;

        case 0x04: /* SFF-8431 (8/4/2G Tx Rate_Select only) */
            task_p = transvr_task_creat(self,
                                        taskfunc_sfp_setup_SFF8431_case2,
                                        taskfunc_post_handle_task_state,
                                        func_str);
            goto sfp_handle_multi_rate_mode_4_sff8431;

        case 0x06: /* SFF-8431 (8/4/2G Independent Rx & Tx Rate_select) */
            task_p = transvr_task_creat(self,
                                        taskfunc_sfp_setup_SFF8431_case3,
                                        taskfunc_post_handle_task_state,
                                        func_str);
            goto sfp_handle_multi_rate_mode_4_sff8431;

        case 0x01: /* SFF-8079 (4/2/1G Rate_Select & AS0/AS1) */
            err_str = "SFF-8079 (4/2/1G Rate_Select & AS0/AS1)";
            goto sfp_handle_multi_rate_mode_4_not_support;

        case 0x08: /* FC-PI-5 (16/8/4G Rx Rate_select only)
                    * High=16G only, Low=8G/4G
                    */
            err_str = "FC-PI-5 (16/8/4G Rx Rate_select only)";
            goto sfp_handle_multi_rate_mode_4_not_support;

        case 0x0A: /* FC-PI-5 (16/8/4G Independent Rx, Tx Rate_select)
                    * High=16G only, Low=8G/4G
                    */
            err_str = "FC-PI-5 (16/8/4G Independent Rx, Tx Rate_select)";
            goto sfp_handle_multi_rate_mode_4_not_support;

        case 0x0C: /* FC-PI-6 (32/16/8G Independent Rx, Tx Rate_Select)
                    * High=32G only, Low = 16G/8G
                    */
            err_str = "FC-PI-6 (32/16/8G Independent Rx, Tx Rate_Select)";
            goto sfp_handle_multi_rate_mode_4_not_support;

        case 0x0E: /* 10/8G Rx and Tx Rate_Select controlling the operation or
                    * locking modes of the internal signal conditioner, retimer
                    * or CDR, according to the logic table defined in Table 10-2,
                    * High Bit Rate (10G) =9.95-11.3 Gb/s; Low Bit Rate (8G) =
                    * 8.5 Gb/s. In this mode, the default value of bit 110.3 (Soft
                    * Rate Select RS(0), Table 9-11) and of bit 118.3 (Soft Rate
                    * Select RS(1), Table 10-1) is 1.
                    */
            err_str = "cable type: 0x0E";
            goto sfp_handle_multi_rate_mode_4_not_support;

        default:
            err_str = "cable type: UNKNOW";
            goto sfp_handle_multi_rate_mode_4_not_support;
    }

sfp_handle_multi_rate_mode_4_sff8431:
    if (!task_p) {
        err_str = "Create task fail!";
        goto sfp_handle_multi_rate_mode_4_fail_1;
    }
    transvr_task_set_retry(task_p, task_retry);
    return EVENT_TRANSVR_TASK_WAIT;

sfp_handle_multi_rate_mode_4_unspecified:
    return EVENT_TRANSVR_TASK_DONE;

sfp_handle_multi_rate_mode_4_not_support:
    SWPS_INFO("%s: Does not support %s <port>:%s <type>:0x%02x\n",
            func_str, err_str, self->swp_name, self->rate_id);
    return EVENT_TRANSVR_TASK_DONE;

sfp_handle_multi_rate_mode_4_fail_1:
    SWPS_INFO("%s: %s <port>:%s <type>:0x%02x, <err>:%d\n",
              func_str, err_str, self->swp_name, self->rate_id, err_code);
    return EVENT_TRANSVR_INIT_FAIL;
}


static int
initfunc_sfp_handle_1g_rj45(struct transvr_obj_s *self) {

    struct transvr_worker_s *task_p = NULL;
    int detect_cls   = DEBUG_TRANSVR_INT_VAL;
    char err_str[64] = DEBUG_TRANSVR_STR_VAL;
    char *func_str   = "initfunc_sfp_handle_1g_rj45";


    if (self->info == TRANSVR_CLASS_BASE_T_1000) {
        task_p = transvr_task_creat(self,
                                    taskfunc_sfp_handle_1g_rj45,
                                    taskfunc_post_do_nothing,
                                    func_str);
        if (!task_p) {
            snprintf(err_str, sizeof(err_str), "Create task fail");
            goto err_initfunc_sfp_handle_1g_rj45;
        }
        transvr_task_set_retry(task_p, VAL_TRANSVR_TASK_RETRY_FOREVER);
    }
    return EVENT_TRANSVR_TASK_DONE;

err_initfunc_sfp_handle_1g_rj45:
    SWPS_INFO("%s: %s <port>:%s <type>:%d\n",
              __func__, err_str, self->swp_name, detect_cls);
    return EVENT_TRANSVR_TASK_FAIL;
}


static int
initfunc_qsfp_handle_power_mode(struct transvr_obj_s *self) {

    int err_code      = EVENT_TRANSVR_EXCEP_INIT;
    int power_class   = DEBUG_TRANSVR_INT_VAL;
    int hpmod_retry   = 3;
    int lpower_config = 1;
    char err_msg[64]  = DEBUG_TRANSVR_STR_VAL;
    unsigned long hpmod_delay = 500; /* msec */
    struct transvr_worker_s *task_p = NULL;

    /* Handle power mode for IOEXP */
    power_class = __qsfp_get_power_cls(self, 0);
    switch (power_class) {
        case 1: /* Case: Low power mode (Class = 1) */
            err_code = _taskfunc_qsfp_setup_power_mod(self, lpower_config);
            if (err_code < 0){
                snprintf(err_msg, sizeof(err_msg), "Setup lpmod fail <ERR>:%d", err_code);
                goto err_initfunc_qsfp_handle_power_mode;
            }
            return EVENT_TRANSVR_TASK_DONE;

        case 2: /* Case: High power mode (Class > 1) */
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            task_p = transvr_task_creat(self,
                                        taskfunc_qsfp_set_hpmod,
                                        taskfunc_post_handle_task_state,
                                        "transvr_init_qsfp");
            if (!task_p) {
                snprintf(err_msg, sizeof(err_msg), "Setup lpmod fail <ERR>:%d", err_code);
                goto err_initfunc_qsfp_handle_power_mode;
            }
            transvr_task_set_retry(task_p, hpmod_retry);
            transvr_task_set_delay(task_p, hpmod_delay);
            return EVENT_TRANSVR_TASK_WAIT;

        default:
            break;
    }
    snprintf(err_msg, sizeof(err_msg), "Exception case");
    goto err_initfunc_qsfp_handle_power_mode;

err_initfunc_qsfp_handle_power_mode:
    SWPS_INFO("%s: %s <port>:%s \n", __func__, err_msg, self->swp_name);
    return EVENT_TRANSVR_INIT_FAIL;
}


int
initfunc_qsfp28_handle_cdr(struct transvr_obj_s *self) {

    uint8_t DEFAULT_VAL_CDR = 0xff;
    int CDR_FUNC_EXISTED    = 0x3;
    int show_err   = 1;
    int err_val    = EVENT_TRANSVR_TASK_FAIL;
    char *err_msg  = DEBUG_TRANSVR_STR_VAL;
    char *func_str = "initfunc_qsfp28_handle_cdr";

    err_val = __qsfp_get_cdr_present(self, 0);
    if ( (err_val < 0) ||
         (err_val == DEBUG_TRANSVR_HEX_VAL) ) {
        err_msg = "detect cdr_present fail!";
        goto err_taskfunc_qsfp_handle_cdr_1;
    }
    if (err_val == CDR_FUNC_EXISTED) {
        err_val = _common_set_uint8_attr(self,
                                         self->eeprom_map_p->addr_cdr,
                                         self->eeprom_map_p->addr_cdr,
                                         self->eeprom_map_p->offset_cdr,
                                         DEFAULT_VAL_CDR,
                                         &(self->cdr),
                                         func_str,
                                         show_err);
        if (err_val < 0) {
            err_msg = "set CDR fail!";
            goto err_taskfunc_qsfp_handle_cdr_1;
        }
    }
    return EVENT_TRANSVR_TASK_DONE;

err_taskfunc_qsfp_handle_cdr_1:
    SWPS_INFO("%s: %s <err>:%d <port>:%s\n",
              func_str, err_msg, err_val, self->swp_name);
    return EVENT_TRANSVR_INIT_FAIL;
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
_sfp_detect_class_by_extend_comp(struct transvr_obj_s* self) {
    /* Reference: SFF-8024 (v3.8)
     */
    int detect_val = _sfp_get_comp_extended(self);

    switch(detect_val) {
        case 0x00: /* Unspecified */
            return TRANSVR_CLASS_UNSPECIFIED;

        case 0x01: /* 100G AOC (Active Optical Cable) or 25GAUI C2M */
        case 0x18: /* 100G AOC or 25GAUI C2M AOC. */
            return TRANSVR_CLASS_OPTICAL_25G_AOC;

        case 0x02: /* 100GBASE-SR4 or 25GBASE-SR */
            return TRANSVR_CLASS_OPTICAL_25G_SR;

        case 0x03: /* 100GBASE-LR4 or 25GBASE-LR */
            return TRANSVR_CLASS_OPTICAL_25G_LR;

        case 0x04: /* 100GBASE-ER4 or 25GBASE-ER */
            return TRANSVR_CLASS_OPTICAL_25G_ER;

        case 0x08: /* 100G ACC (Active Copper Cable) or 25GAUI C2M ACC. */
        case 0x0b: /* 100GBASE-CR4 or 25GBASE-CR CA-L */
        case 0x0c: /* 25GBASE-CR CA-S */
        case 0x0d: /* 25GBASE-CR CA-N */
        case 0x19: /* 100G ACC or 25GAUI C2M ACC. */
            return TRANSVR_CLASS_COPPER_L1_25G;

        default:
            break;
    }
    SWPS_INFO("%s: Unexcept value:0x%02x\n <port>:%s",
              __func__, detect_val, self->swp_name);
    return TRANSVR_CLASS_ERROR;
}


int
_sfp_detect_class_by_10_ethernet(struct transvr_obj_s* self) {
    /* Reference: SFF-8472 (v12.2)
     */
    int detect_val = DEBUG_TRANSVR_INT_VAL;

    detect_val = _sfp_get_comp_10g_eth_comp(self);
    /* Case: Unspecified */
    if (detect_val == 0x00) {
        return TRANSVR_CLASS_UNSPECIFIED;
    }
    /* Case: 10G Optical (x1) */
    if ((detect_val & 0x10) == 0x10) {    /* 00010000 : 10GBASE-SR  */
        return TRANSVR_CLASS_OPTICAL_10G_S_SR;
    }
    if ( ((detect_val & 0x20) == 0x20) || /* 00100000 : 10GBASE-LR  */
         ((detect_val & 0x40) == 0x40) ){ /* 01000000 : 10GBASE-LRM */
        return TRANSVR_CLASS_OPTICAL_10G_S_LR;
    }
    if ((detect_val & 0x80) == 0x80) {    /* 10000000 : 10GBASE-ER  */
        return TRANSVR_CLASS_OPTICAL_10G_S_ER;
    }
    /* Case: ERROR */
    SWPS_INFO("%s: Unexcept value:0x%02x\n <port>:%s",
              __func__, detect_val, self->swp_name);
    return TRANSVR_CLASS_ERROR;
}


int
_sfp_detect_if_sp_by_br(struct transvr_obj_s* self) {

    int lower_bound_1g  = 0x0b;
    int upper_bound_1g  = 0x1A;
    int lower_bound_10g = 0x60;
    int upper_bound_10g = 0x75;
    int lower_bound_25g = 0xf0;
    int upper_bound_25g = 0xff;
    int notmal_br = DEBUG_TRANSVR_INT_VAL;

    notmal_br = (int)(self->br); /* updated by update_all() */
    /* Check 25G */
    if ((notmal_br >= lower_bound_25g) &&
        (notmal_br <= upper_bound_25g) ) {
        return TRANSVR_CLASS_25G;
    }
    /* Check 10G */
    if ((notmal_br >= lower_bound_10g) &&
        (notmal_br <= upper_bound_10g) ) {
        return TRANSVR_CLASS_10G;
    }
    /* Check 1G */
    if ((notmal_br >= lower_bound_1g) &&
        (notmal_br <= upper_bound_1g) ) {
        return TRANSVR_CLASS_1G;
    }
    return TRANSVR_CLASS_UNSPECIFIED;
}


int
_sfp_detect_class_by_1g_ethernet(struct transvr_obj_s* self) {
    /* Reference: SFF-8472 (v12.2)
     */
    int detect_val   = DEBUG_TRANSVR_INT_VAL;
    int speed_br     = DEBUG_TRANSVR_INT_VAL;
    int speed_tmp    = DEBUG_TRANSVR_INT_VAL;
    char err_str[64] = DEBUG_TRANSVR_STR_VAL;

    speed_br   = _sfp_detect_if_sp_by_br(self);
    detect_val = _sfp_get_comp_1g_eth_comp(self);

    if (detect_val < 0) {
        snprintf(err_str, sizeof(err_str), "Detect abnormal value:%d", detect_val);
        goto err_p_sfp_detect_class_by_1g_ethernet;
    }
    /* Case: Unspecified */
    if (detect_val == 0x00) {
        return TRANSVR_CLASS_UNSPECIFIED;
    }
    /* Case: 1G (x1) */
    if ((detect_val & 0x01) == 0x01) {    /* 00000001 : 1000BASE-SX    */
        speed_tmp = TRANSVR_CLASS_OPTICAL_1G_SX;
        goto ok_sfp_detect_class_by_1g_ethernet_4_check_br_10g;
    }
    if ((detect_val & 0x02) == 0x02) {    /* 00000010 : 1000BASE-LX *3 */
        speed_tmp = TRANSVR_CLASS_OPTICAL_1G_LX;
        goto ok_sfp_detect_class_by_1g_ethernet_4_check_br_10g;
    }
    if ((detect_val & 0x04) == 0x04) {    /* 00000100 : 1000BASE-CX    */
        speed_tmp = TRANSVR_CLASS_COPPER_L1_1G;
        goto ok_sfp_detect_class_by_1g_ethernet_4_check_br_10g;
    }
    /* Case: 1000 Base-T (x1) */
    if ((detect_val & 0x08) == 0x08) {    /* 00001000 : 1000BASE-T     */
        return TRANSVR_CLASS_BASE_T_1000;
    }
    /* Case: 100 Base */
    if ( ((detect_val & 0x10) == 0x10) || /* 00010000 : 100BASE-LX/LX10  */
         ((detect_val & 0x20) == 0x20) || /* 00100000 : 100BASE-FX  */
         ((detect_val & 0x40) == 0x40) || /* 01000000 : BASE-BX10 *3 */
         ((detect_val & 0x80) == 0x80) ){ /* 10000000 : BASE-PX *3  */
        return TRANSVR_CLASS_OPTICAL_100;
    }
    /* Case: ERROR */
    snprintf(err_str, sizeof(err_str), "Case:ERROR, value:%d", detect_val);
    goto err_p_sfp_detect_class_by_1g_ethernet;

ok_sfp_detect_class_by_1g_ethernet_4_check_br_10g:
    switch (speed_br) {
        case TRANSVR_CLASS_UNSPECIFIED:
        case TRANSVR_CLASS_1G:
            return speed_tmp;
        case TRANSVR_CLASS_10G:
            goto ok_sfp_detect_class_by_1g_ethernet_4_transfer_10G;
    }

ok_sfp_detect_class_by_1g_ethernet_4_transfer_10G:
    switch (speed_tmp) {
        case TRANSVR_CLASS_OPTICAL_1G_SX:
            return TRANSVR_CLASS_OPTICAL_10G_S_SR;
        case TRANSVR_CLASS_OPTICAL_1G_LX:
            return TRANSVR_CLASS_OPTICAL_10G_S_LR;
        case TRANSVR_CLASS_COPPER_L1_1G:
            return TRANSVR_CLASS_COPPER_L1_10G;
        default:
            break;
    }
    snprintf(err_str, sizeof(err_str), "transfer_1to10 fail, speed:%d", speed_tmp);
    goto err_p_sfp_detect_class_by_1g_ethernet;

err_p_sfp_detect_class_by_1g_ethernet:
    SWPS_INFO("%s: %s <port>:%s", __func__, err_str, self->swp_name);
    return TRANSVR_CLASS_ERROR;
}


int
_sfp_detect_class_by_feature(struct transvr_obj_s* self) {
    /* Reference: SFF-8024 (v3.8)
     */
    int is_active    = 0;
    int conn_val     = DEBUG_TRANSVR_INT_VAL;
    int check_val    = DEBUG_TRANSVR_INT_VAL;
    int wave_len     = DEBUG_TRANSVR_INT_VAL;
    int speed_val    = DEBUG_TRANSVR_INT_VAL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    speed_val = _sfp_detect_if_sp_by_br(self);
    conn_val  = _sfp_get_connector_type(self);

    switch(conn_val) {
        case 0x00: /* Unspecified */
            goto ok_sfp_detect_class_by_feature_4_check_active_passive;
        case 0x07: /* LC (Lucent Connector) */
        case 0x0b: /* Optical Pigtail */
        case 0x0c: /* MPO 1x12 */
        case 0x0d: /* MPO 2x16 */
            /*
             * ToDo: Need verify Optical Pigtail
             */
            goto ok_sfp_detect_class_by_feature_4_optiocal;
        case 0x21: /* Copper pigtail */
            /*
             * ToDo: Need check ACC use case
             */
            goto ok_sfp_detect_class_by_feature_4_check_active_passive;
        case 0x23: /* No separable connector */
            /*
             * ToDo: Standard not clear, not all transceiver vendor
             *       have the same defined
             */
            goto ok_sfp_detect_class_by_feature_4_check_active_passive;
        default:
            break;
    }
    goto ok_sfp_detect_class_by_feature_4_unknow;

ok_sfp_detect_class_by_feature_4_check_active_passive:
    check_val = _sfp_get_cable_tech(self);
    switch(check_val) {
        case 0x00: /* Unspecified */
            goto ok_sfp_detect_class_by_feature_4_unknow;
        case 0x04: /* Passive */
            goto ok_sfp_detect_class_by_feature_4_copper;
        case 0x08: /* Active  */
            is_active = 1;
            goto ok_sfp_detect_class_by_feature_4_aoc;
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "_sfp_get_cable_tech return Non define value:%d",
                     check_val);
            break;
    }
    goto err_sfp_detect_class_by_feature_1;

ok_sfp_detect_class_by_feature_4_optiocal:
    wave_len  = _common_count_wavelength(self,
                                         self->wavelength[0],
                                         self->wavelength[1]);
    switch(speed_val) {
        case TRANSVR_CLASS_25G:
            switch (wave_len) {
                case VAL_OPTICAL_WAVELENGTH_SR:
                    return TRANSVR_CLASS_OPTICAL_25G_SR;
                case VAL_OPTICAL_WAVELENGTH_LR:
                    return TRANSVR_CLASS_OPTICAL_25G_LR;
                case VAL_OPTICAL_WAVELENGTH_ER:
                    return TRANSVR_CLASS_OPTICAL_25G_ER;
                default:
                    break;
            }
            return TRANSVR_CLASS_OPTICAL_25G;

        case TRANSVR_CLASS_10G:
            switch (wave_len) {
                case VAL_OPTICAL_WAVELENGTH_SR:
                    return TRANSVR_CLASS_OPTICAL_10G_S_SR;
                case VAL_OPTICAL_WAVELENGTH_LR:
                    return TRANSVR_CLASS_OPTICAL_10G_S_LR;
                case VAL_OPTICAL_WAVELENGTH_ER:
                    return TRANSVR_CLASS_OPTICAL_10G_S_ER;
                default:
                    break;
            }
            return TRANSVR_CLASS_OPTICAL_10G;

        case TRANSVR_CLASS_1G:
            switch (wave_len) {
                case VAL_OPTICAL_WAVELENGTH_SR:
                    return TRANSVR_CLASS_OPTICAL_1G_SX;
                case VAL_OPTICAL_WAVELENGTH_LR:
                    return TRANSVR_CLASS_OPTICAL_1G_LX;
                case VAL_OPTICAL_WAVELENGTH_ER:
                    return TRANSVR_CLASS_OPTICAL_1G_EX;
                default:
                    break;
            }
            return TRANSVR_CLASS_OPTICAL_1G;

        default:
            return TRANSVR_CLASS_OPTICAL;
    }

ok_sfp_detect_class_by_feature_4_aoc:
    switch(speed_val) {
        case TRANSVR_CLASS_25G:
            return TRANSVR_CLASS_OPTICAL_25G_AOC;
        case TRANSVR_CLASS_10G:
            return TRANSVR_CLASS_OPTICAL_10G_S_AOC;
        case TRANSVR_CLASS_1G:
            return TRANSVR_CLASS_OPTICAL_1G_AOC;
        default:
            break;
    }
    goto ok_sfp_detect_class_by_feature_4_unknow;

ok_sfp_detect_class_by_feature_4_copper:
    switch(speed_val) {
        case TRANSVR_CLASS_25G:
            return TRANSVR_CLASS_COPPER_L1_25G;
        case TRANSVR_CLASS_10G:
            return TRANSVR_CLASS_COPPER_L1_10G;
        case TRANSVR_CLASS_1G:
            return TRANSVR_CLASS_COPPER_L1_1G;
        default:
            return TRANSVR_CLASS_COPPER;
    }

ok_sfp_detect_class_by_feature_4_unknow:
    return TRANSVR_CLASS_UNSPECIFIED;

err_sfp_detect_class_by_feature_1:
    SWPS_INFO("%s: %s\n <port>:%s", __func__, err_msg, self->swp_name);
    return TRANSVR_CLASS_ERROR;
}


int
sft_detect_transvr_class(struct transvr_obj_s* self) {

    int detect_val   = DEBUG_TRANSVR_INT_VAL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    /* Check Extended Compliance */
    detect_val = _sfp_detect_class_by_extend_comp(self);
    switch(detect_val) {
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        case TRANSVR_CLASS_OPTICAL_25G_AOC:
        case TRANSVR_CLASS_OPTICAL_25G_SR:
        case TRANSVR_CLASS_OPTICAL_25G_LR:
        case TRANSVR_CLASS_OPTICAL_25G_ER:
        case TRANSVR_CLASS_COPPER_L1_25G:
            return detect_val;
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined extend_comp:%d",
                     detect_val);
            goto err_sft_detect_transceiver_class_1;
    }
    /* Check 10G Compliance */
    detect_val = _sfp_detect_class_by_10_ethernet(self);
    switch(detect_val) {
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        case TRANSVR_CLASS_OPTICAL_10G_S_SR:
        case TRANSVR_CLASS_OPTICAL_10G_S_LR:
        case TRANSVR_CLASS_OPTICAL_10G_S_ER:
            return detect_val;
        default:
            snprintf(err_msg, sizeof(err_msg),
                    "Detect undefined 10G_eth:%d",
                     detect_val);
            goto err_sft_detect_transceiver_class_1;
    }
    /* Check 1G Compliance */
    detect_val = _sfp_detect_class_by_1g_ethernet(self);
    switch(detect_val) {
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        case TRANSVR_CLASS_OPTICAL_1G_SX:
        case TRANSVR_CLASS_OPTICAL_1G_LX:
        case TRANSVR_CLASS_COPPER_L1_1G:
        case TRANSVR_CLASS_BASE_T_1000:
        case TRANSVR_CLASS_OPTICAL_100:
            /*
             * ToDo: Need Check 0.1G
             */
        case TRANSVR_CLASS_OPTICAL_10G_S_SR:
        case TRANSVR_CLASS_OPTICAL_10G_S_LR:
        case TRANSVR_CLASS_COPPER_L1_10G:
            /* Transfer speed case
             * => Example: Raycom 10G DAC
             */
            return detect_val;
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined 1G_eth:%d",
                     detect_val);
            goto err_sft_detect_transceiver_class_1;
    }
    /* Check by connector, br, wavelength */
    detect_val = _sfp_detect_class_by_feature(self);
    switch(detect_val) {
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        case TRANSVR_CLASS_OPTICAL:
        case TRANSVR_CLASS_OPTICAL_1G:
        case TRANSVR_CLASS_OPTICAL_1G_SX:
        case TRANSVR_CLASS_OPTICAL_1G_LX:
        case TRANSVR_CLASS_OPTICAL_1G_EX:
        case TRANSVR_CLASS_OPTICAL_1G_AOC:
        case TRANSVR_CLASS_OPTICAL_10G:
        case TRANSVR_CLASS_OPTICAL_10G_S_SR:
        case TRANSVR_CLASS_OPTICAL_10G_S_LR:
        case TRANSVR_CLASS_OPTICAL_10G_S_ER:
        case TRANSVR_CLASS_OPTICAL_10G_S_AOC:
        case TRANSVR_CLASS_OPTICAL_25G:
        case TRANSVR_CLASS_OPTICAL_25G_SR:
        case TRANSVR_CLASS_OPTICAL_25G_LR:
        case TRANSVR_CLASS_OPTICAL_25G_ER:
        case TRANSVR_CLASS_OPTICAL_25G_AOC:
        case TRANSVR_CLASS_COPPER:
        case TRANSVR_CLASS_COPPER_L1_1G:
        case TRANSVR_CLASS_COPPER_L1_10G:
        case TRANSVR_CLASS_COPPER_L1_25G:
            return detect_val;
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined get_connector:%d",
                     detect_val);
            goto err_sft_detect_transceiver_class_1;
    }
    /* Exception case: Can't verify */
    snprintf(err_msg, sizeof(err_msg), "Can not identify!");
    goto err_sft_detect_transceiver_class_1;

err_sft_detect_transceiver_class_1:
    SWPS_INFO("%s: %s <port>:%s\n", __func__,  err_msg, self->swp_name);
    return TRANSVR_CLASS_ERROR;
}


int
_sfp_set_trident2_if_type(struct transvr_obj_s* self,
                          int transvr_cls,
                          char *result){

    int lmax = 8;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    switch(transvr_cls) {
        case TRANSVR_CLASS_ERROR:
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        /* 25G OPTICAL */
        case TRANSVR_CLASS_OPTICAL_25G_AOC:
        case TRANSVR_CLASS_OPTICAL_25G_SR:
        case TRANSVR_CLASS_OPTICAL_25G_LR:
        case TRANSVR_CLASS_OPTICAL_25G_ER:
        case TRANSVR_CLASS_OPTICAL_25G:
            return snprintf(result, lmax, TRANSVR_IF_SFI);
        /* 25G COPPER */
        case TRANSVR_CLASS_COPPER_L1_25G:
            return snprintf(result, lmax, TRANSVR_IF_SFI);
        /* 10G OPTICAL */
        case TRANSVR_CLASS_OPTICAL_10G_S_AOC:
        case TRANSVR_CLASS_OPTICAL_10G_S_SR:
        case TRANSVR_CLASS_OPTICAL_10G_S_LR:
        case TRANSVR_CLASS_OPTICAL_10G_S_ER:
        case TRANSVR_CLASS_OPTICAL_10G:
            return snprintf(result, lmax, TRANSVR_IF_SFI);
        /* 10G COPPER */
        case TRANSVR_CLASS_COPPER_L1_10G:
            return snprintf(result, lmax, TRANSVR_IF_SFI);
        /* 1G OPTICAL */
        case TRANSVR_CLASS_OPTICAL_1G_AOC:
        case TRANSVR_CLASS_OPTICAL_1G_SX:
        case TRANSVR_CLASS_OPTICAL_1G_LX:
        case TRANSVR_CLASS_OPTICAL_1G_EX:
        case TRANSVR_CLASS_OPTICAL_1G:
            return snprintf(result, lmax, TRANSVR_IF_IF_GMII);
        /* 1G COPPER */
        case TRANSVR_CLASS_COPPER_L1_1G:
            return snprintf(result, lmax, TRANSVR_IF_IF_GMII);
        /* 1G BASE_T */
        case TRANSVR_CLASS_BASE_T_1000:
            return snprintf(result, lmax, TRANSVR_IF_IF_GMII);
        /* 100 Base */
        case TRANSVR_CLASS_OPTICAL_100:
            return snprintf(result, lmax, TRANSVR_IF_IF_GMII);
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined value:%d",
                     transvr_cls);
            goto err_sfp_set_trident2_if_type_1;
    }
    /* Exception case: Can't verify */
    snprintf(err_msg, sizeof(err_msg), "Can not identify!");
    goto err_sfp_set_trident2_if_type_1;

err_sfp_set_trident2_if_type_1:
    snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
    SWPS_INFO("%s: %s <port>:%s\n", __func__, err_msg, self->swp_name);
    return ERR_TRANSVR_ABNORMAL;
}


int
_sfp_set_tomahawk_if_type(struct transvr_obj_s* self,
                          int transvr_cls,
                          char *result) {

    int lmax = 8;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    switch(transvr_cls) {
        case TRANSVR_CLASS_ERROR:
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        /* 25G OPTICAL */
        case TRANSVR_CLASS_OPTICAL_25G_AOC:
        case TRANSVR_CLASS_OPTICAL_25G_SR:
        case TRANSVR_CLASS_OPTICAL_25G_LR:
        case TRANSVR_CLASS_OPTICAL_25G_ER:
        case TRANSVR_CLASS_OPTICAL_25G:
            return snprintf(result, lmax, TRANSVR_IF_SR);
        /* 25G COPPER */
        case TRANSVR_CLASS_COPPER_L1_25G:
            return snprintf(result, lmax, TRANSVR_IF_KR);
        /* 10G OPTICAL */
        case TRANSVR_CLASS_OPTICAL_10G_S_AOC:
        case TRANSVR_CLASS_OPTICAL_10G_S_SR:
        case TRANSVR_CLASS_OPTICAL_10G_S_LR:
        case TRANSVR_CLASS_OPTICAL_10G_S_ER:
        case TRANSVR_CLASS_OPTICAL_10G:
            return snprintf(result, lmax, TRANSVR_IF_SFI);
        /* 10G COPPER */
        case TRANSVR_CLASS_COPPER_L1_10G:
            return snprintf(result, lmax, TRANSVR_IF_SFI);
        /* 1G OPTICAL */
        case TRANSVR_CLASS_OPTICAL_1G_AOC:
        case TRANSVR_CLASS_OPTICAL_1G_SX:
        case TRANSVR_CLASS_OPTICAL_1G_LX:
        case TRANSVR_CLASS_OPTICAL_1G_EX:
        case TRANSVR_CLASS_OPTICAL_1G:
            return snprintf(result, lmax, TRANSVR_IF_IF_GMII);
        /* 1G COPPER */
        case TRANSVR_CLASS_COPPER_L1_1G:
            return snprintf(result, lmax, TRANSVR_IF_IF_GMII);
        /* 1G BASE_T */
        case TRANSVR_CLASS_BASE_T_1000:
            return snprintf(result, lmax, TRANSVR_IF_IF_GMII);
        /* 100 Base */
        case TRANSVR_CLASS_OPTICAL_100:
            return snprintf(result, lmax, TRANSVR_IF_IF_GMII);
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined value:%d",
                     transvr_cls);
            goto err_sfp_set_tomahawk_if_type_1;
    }
    /* Exception case: Can't verify */
    snprintf(err_msg, sizeof(err_msg), "Can not identify!");
    goto err_sfp_set_tomahawk_if_type_1;

err_sfp_set_tomahawk_if_type_1:
    snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
    SWPS_INFO("%s: %s\n <port>:%s", __func__, err_msg, self->swp_name);
    return ERR_TRANSVR_ABNORMAL;
}


int
_sfp_set_bf_tofino_if_type(struct transvr_obj_s* self,
                           int transvr_cls,
                           char *result) {
    /* (TBD)
     *  Due to BF looks like doesn't have interface type.
     *  We bypass it currently.
     */
    int lmax = 8;    
    return snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
}


int
_sfp_detect_if_type(struct transvr_obj_s* self,
                    char *result){

    int lmax = 8;
    int detect_cls = DEBUG_TRANSVR_INT_VAL;

    detect_cls = sft_detect_transvr_class(self);
    switch (self->chipset_type) {
        case BCM_CHIP_TYPE_TRIDENT_2:
            return _sfp_set_trident2_if_type(self, detect_cls, result);
        
        case BCM_CHIP_TYPE_TRIDENT_3:
        case BCM_CHIP_TYPE_TOMAHAWK:
            return _sfp_set_tomahawk_if_type(self, detect_cls, result);

        case BF_CHIP_TYPE_TOFINO:
            return _sfp_set_bf_tofino_if_type(self, detect_cls, result);
            
        default:
            SWPS_INFO("%s: non-defined chipset_type:%d <port>:%s\n",
                      __func__, self->chipset_type, self->swp_name);
            break;
    }
    snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
    return ERR_TRANSVR_ABNORMAL;
}


int
sfp_get_if_type(struct transvr_obj_s *self,
                char *buf_p){

    int lmax = 16;
    char tmp_result[16] = DEBUG_TRANSVR_STR_VAL;

    if (self->state != STATE_TRANSVR_CONNECTED) {
        return snprintf(buf_p, lmax, "%d\n", self->state);
    }
    if (_sfp_detect_if_type(self, tmp_result) < 0) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_ABNORMAL);
    }
    return snprintf(buf_p, lmax, "%s\n", tmp_result);
}


int
_sfp_detect_if_speed(struct transvr_obj_s* self,
                     char *result){

    int lmax = 16;
    int detect_val   = DEBUG_TRANSVR_INT_VAL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    detect_val = sft_detect_transvr_class(self);
    switch(detect_val) {
        case TRANSVR_CLASS_ERROR:
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        /* 25G OPTICAL */
        case TRANSVR_CLASS_OPTICAL_25G_AOC:
        case TRANSVR_CLASS_OPTICAL_25G_SR:
        case TRANSVR_CLASS_OPTICAL_25G_LR:
        case TRANSVR_CLASS_OPTICAL_25G_ER:
        case TRANSVR_CLASS_OPTICAL_25G:
            return snprintf(result, lmax, TRANSVR_IF_SP_25G);
        /* 25G COPPER */
        case TRANSVR_CLASS_COPPER_L1_25G:
            return snprintf(result, lmax, TRANSVR_IF_SP_25G);
        /* 10G OPTICAL */
        case TRANSVR_CLASS_OPTICAL_10G_S_AOC:
        case TRANSVR_CLASS_OPTICAL_10G_S_SR:
        case TRANSVR_CLASS_OPTICAL_10G_S_LR:
        case TRANSVR_CLASS_OPTICAL_10G_S_ER:
        case TRANSVR_CLASS_OPTICAL_10G:
            return snprintf(result, lmax, TRANSVR_IF_SP_10G);
        /* 10G COPPER */
        case TRANSVR_CLASS_COPPER_L1_10G:
            return snprintf(result, lmax, TRANSVR_IF_SP_10G);
        /* 1G OPTICAL */
        case TRANSVR_CLASS_OPTICAL_1G_AOC:
        case TRANSVR_CLASS_OPTICAL_1G_SX:
        case TRANSVR_CLASS_OPTICAL_1G_LX:
        case TRANSVR_CLASS_OPTICAL_1G_EX:
        case TRANSVR_CLASS_OPTICAL_1G:
            return snprintf(result, lmax, TRANSVR_IF_SP_1G);
        /* 1G COPPER */
        case TRANSVR_CLASS_COPPER_L1_1G:
            return snprintf(result, lmax, TRANSVR_IF_SP_1G);
        /* 1G BASE_T */
        case TRANSVR_CLASS_BASE_T_1000:
            return snprintf(result, lmax, TRANSVR_IF_SP_1G);
        /* 100 Base */
        case TRANSVR_CLASS_OPTICAL_100:
            return snprintf(result, lmax, TRANSVR_IF_SP_100);
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined value:%d",
                     detect_val);
            goto err_sfp_detect_if_speed_1;
    }
    /* Check by BR */
    detect_val = _sfp_detect_if_sp_by_br(self);
    switch (detect_val) {
        case TRANSVR_CLASS_25G:
            return snprintf(result, lmax, TRANSVR_IF_SP_25G);
        case TRANSVR_CLASS_10G:
            return snprintf(result, lmax, TRANSVR_IF_SP_10G);
        case TRANSVR_CLASS_1G:
            return snprintf(result, lmax, TRANSVR_IF_SP_1G);
        default:
            break;
    }
    /* Exception case: Can't verify */
    snprintf(err_msg, sizeof(err_msg), "Can not identify!");
    goto err_sfp_detect_if_speed_1;

err_sfp_detect_if_speed_1:
    snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
    SWPS_INFO("%s %s\n <port>:%s", __func__, err_msg, self->swp_name);
    return ERR_TRANSVR_ABNORMAL;
}


int
sfp_get_if_speed(struct transvr_obj_s *self,
                 char *buf_p){

    int lmax = 16;
    char tmp_result[16] = DEBUG_TRANSVR_STR_VAL;

    if (self->state != STATE_TRANSVR_CONNECTED) {
        return snprintf(buf_p, lmax, "%d\n", self->state);
    }
    if (_sfp_detect_if_speed(self, tmp_result) < 0) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_ABNORMAL);
    }
    return snprintf(buf_p, lmax, "%s\n", tmp_result);
}


int
_qsfp_detect_class_by_extend_comp(struct transvr_obj_s* self) {
    /* Reference: SFF-8024 (v3.8)
     */
    int detect_val = DEBUG_TRANSVR_INT_VAL;

    detect_val = _qsfp_get_comp_extended(self);
    switch(detect_val) {
        case 0x00: /* Unspecified */
            return TRANSVR_CLASS_UNSPECIFIED;

        case 0x01: /* 100G AOC (Active Optical Cable) or 25GAUI C2M */
        case 0x18: /* 100G AOC or 25GAUI C2M AOC. */
            return TRANSVR_CLASS_OPTICAL_100G_AOC;

        case 0x06: /* 100G CWDM4 */
        case 0x09: /* Obsolete (assigned before 100G CWDM4 MSA required FEC) */
        case 0x17: /* 100G CLR4 */
        case 0x1A: /* 100GE-DWDM2 */
            return TRANSVR_CLASS_OPTICAL_100G;

        case 0x02: /* 100GBASE-SR4 or 25GBASE-SR */
            return TRANSVR_CLASS_OPTICAL_100G_SR4;

        case 0x03: /* 100GBASE-LR4 or 25GBASE-LR */
            return TRANSVR_CLASS_OPTICAL_100G_LR4;

        case 0x04: /* 100GBASE-ER4 or 25GBASE-ER */
            return TRANSVR_CLASS_OPTICAL_100G_ER4;

        case 0x07: /* 100G PSM4 Parallel SMF */
            return TRANSVR_CLASS_OPTICAL_100G_PSM4;

        case 0x12: /* 40G PSM4 Parallel SMF */
            return TRANSVR_CLASS_OPTICAL_40G;

        case 0x11: /* 4 x 10GBASE-SR */
            return TRANSVR_CLASS_OPTICAL_40G_SR4;

        case 0x10: /* 40GBASE-ER4 */
            return TRANSVR_CLASS_OPTICAL_40G_ER4;

        case 0x08: /* 100G ACC (Active Copper Cable) or 25GAUI C2M ACC. */
        case 0x0b: /* 100GBASE-CR4 or 25GBASE-CR CA-L */
        case 0x19: /* 100G ACC or 25GAUI C2M ACC. */
            return TRANSVR_CLASS_COPPER_L4_100G;

        default:
            break;
    }
    SWPS_INFO("%s: Unexcept value:0x%02x\n <port>:%s",
              __func__, detect_val, self->swp_name);
    return TRANSVR_CLASS_ERROR;
}


int
_qsfp_detect_class_by_10_40_100_ethernet(struct transvr_obj_s* self) {
    /* Reference: SFF-8472 (v12.2)
     */
    int detect_val = DEBUG_TRANSVR_INT_VAL;

    detect_val = _qsfp_get_comp_10_40_100_ethernet(self);
    /* Case: Unspecified */
    if (detect_val == 0x00) {
        return TRANSVR_CLASS_UNSPECIFIED;
    }
    /* Case: 40G Optical */
    if ((detect_val & 0x01) == 0x01) {    /* 00000001 : 40G Active Cable (XLPPI) */
        return TRANSVR_CLASS_OPTICAL_40G_AOC;
    }
    if ((detect_val & 0x04) == 0x04) {    /* 00000100 : 40GBASE-SR4 */
        return TRANSVR_CLASS_OPTICAL_40G_SR4;
    }
    if ( (detect_val & 0x02) == 0x02) {   /* 00000010 : 40GBASE-LR4 */
        return TRANSVR_CLASS_OPTICAL_40G_LR4;
    }
    if ( (detect_val & 0x08) == 0x08) {   /* 00001000 : 40GBASE-CR4 */
        return TRANSVR_CLASS_COPPER_L4_40G;
    }
    /* Case: 10G Optical */
    if ( (detect_val & 0x10) == 0x10) {   /* 00010000 : 10GBASE-SR  */
        return TRANSVR_CLASS_OPTICAL_10G_Q_SR;
    }
    if ( ((detect_val & 0x20) == 0x20) || /* 00100000 : 10GBASE-LR  */
         ((detect_val & 0x40) == 0x40) ){ /* 01000000 : 10GBASE-LRM */
        return TRANSVR_CLASS_OPTICAL_10G_Q_LR;
    }
    /* Case: Extend Compliance */
    if ( ((detect_val & 0x80) == 0x80) ){ /* 10000000 : Use Extend Compliance */
        return TRANSVR_CLASS_EXTEND_COMP;
    }
    /* Case: ERROR */
    SWPS_INFO("%s: Unexcept value:0x%02x\n <port>:%s",
              __func__, detect_val, self->swp_name);
    return TRANSVR_CLASS_ERROR;
}


int
_qsfp_detect_if_sp_by_br(struct transvr_obj_s* self) {

    int lower_bound_10g  = 0x10;
    int upper_bound_10g  = 0x25;
    int lower_bound_40g  = 0x60;
    int upper_bound_40g  = 0x75;
    int lower_bound_100g = 0x60;
    int upper_bound_100g = 0x75;
    int used_extend_br   = 0xff;
    int notmal_br = DEBUG_TRANSVR_INT_VAL;
    int extend_br = DEBUG_TRANSVR_INT_VAL;

    notmal_br = (int)(self->br); /* updated by update_all() */
    /* Check 40G */
    if ((notmal_br >= lower_bound_40g) &&
        (notmal_br <= upper_bound_40g) ) {
        return TRANSVR_CLASS_40G;
    }
    /* Check 100G */
    if (notmal_br == used_extend_br) {
        extend_br = (int)(self->extbr); /* updated by update_all() */
        if ((extend_br >= lower_bound_100g) &&
            (extend_br <= upper_bound_100g) ) {
            return TRANSVR_CLASS_100G;
        }
    }
    /* Check 10G */
    if ((notmal_br >= lower_bound_10g) &&
        (notmal_br <= upper_bound_10g) ) {
        return TRANSVR_CLASS_10G;
    }
    return TRANSVR_CLASS_UNSPECIFIED;
}


int
_qsfp_detect_class_by_feature(struct transvr_obj_s* self) {
    /* Reference: SFF-8024 (v3.8)
     */
    int conn_val     = DEBUG_TRANSVR_INT_VAL;
    int wave_len     = DEBUG_TRANSVR_INT_VAL;
    int speed_val    = DEBUG_TRANSVR_INT_VAL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    speed_val = _qsfp_detect_if_sp_by_br(self);
    conn_val  = _qsfp_get_connector_type(self);

    switch(conn_val) {
        case 0x00: /* Unspecified */
            return TRANSVR_CLASS_UNSPECIFIED;
        case 0x07: /* LC (Lucent Connector) */
        case 0x0b: /* Optical Pigtail */
        case 0x0c: /* MPO 1x12 (Multifiber Parallel Optic) */
        case 0x0d: /* MPO 2x16 */
            goto ok_qsfp_detect_class_by_feature_4_optiocal;
        case 0x21: /* Copper pigtail */
            goto ok_qsfp_detect_class_by_feature_4_copper;
        case 0x23: /* No separable connector */
            if ((_qsfp_get_comp_fc_link_length(self) > 0) ||
                (_qsfp_get_comp_fc_trans_tech(self) > 0)  ||
                (_qsfp_get_comp_fc_trans_media(self) > 0) ||
                (_qsfp_get_comp_fc_speed(self) > 0) ) {
                goto ok_qsfp_detect_class_by_feature_4_aoc;
            }
            goto ok_qsfp_detect_class_by_feature_4_copper;
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "_qsfp_get_connector_type return Non define value:%d",
                     conn_val);
            goto err_qsfp_detect_class_by_feature_1;
    }
    return TRANSVR_CLASS_UNSPECIFIED;

ok_qsfp_detect_class_by_feature_4_optiocal:
    wave_len  = _common_count_wavelength(self,
                                         self->wavelength[0],
                                         self->wavelength[1]);
    switch(speed_val) {
        case TRANSVR_CLASS_100G:
            switch (wave_len) {
                case VAL_OPTICAL_WAVELENGTH_SR:
                    return TRANSVR_CLASS_OPTICAL_100G_SR4;
                case VAL_OPTICAL_WAVELENGTH_LR:
                    return TRANSVR_CLASS_OPTICAL_100G_LR4;
                case VAL_OPTICAL_WAVELENGTH_ER:
                    return TRANSVR_CLASS_OPTICAL_100G_ER4;
                default:
                    break;
            }
            return TRANSVR_CLASS_OPTICAL_100G;

        case TRANSVR_CLASS_40G:
            switch (wave_len) {
                case VAL_OPTICAL_WAVELENGTH_SR:
                    return TRANSVR_CLASS_OPTICAL_40G_SR4;
                case VAL_OPTICAL_WAVELENGTH_LR:
                    return TRANSVR_CLASS_OPTICAL_40G_LR4;
                case VAL_OPTICAL_WAVELENGTH_ER:
                    return TRANSVR_CLASS_OPTICAL_40G_ER4;
                default:
                    break;
            }
            return TRANSVR_CLASS_OPTICAL_40G;

        case TRANSVR_CLASS_10G:
            switch (wave_len) {
                case VAL_OPTICAL_WAVELENGTH_SR:
                    return TRANSVR_CLASS_OPTICAL_10G_Q_SR;
                case VAL_OPTICAL_WAVELENGTH_LR:
                    return TRANSVR_CLASS_OPTICAL_10G_Q_LR;
                case VAL_OPTICAL_WAVELENGTH_ER:
                    return TRANSVR_CLASS_OPTICAL_10G_Q_ER;
                default:
                    break;
            }
            return TRANSVR_CLASS_OPTICAL_10G;

        default:
            return TRANSVR_CLASS_OPTICAL;
    }

ok_qsfp_detect_class_by_feature_4_aoc:
    switch(speed_val) {
        case TRANSVR_CLASS_100G:
            return TRANSVR_CLASS_OPTICAL_100G_AOC;
        case TRANSVR_CLASS_40G:
            return TRANSVR_CLASS_OPTICAL_40G_AOC;
        case TRANSVR_CLASS_10G:
            return TRANSVR_CLASS_OPTICAL_10G_Q_AOC;
        default:
            return TRANSVR_CLASS_OPTICAL;
    }

ok_qsfp_detect_class_by_feature_4_copper:
    switch(speed_val) {
        case TRANSVR_CLASS_100G:
            return TRANSVR_CLASS_COPPER_L4_100G;
        case TRANSVR_CLASS_40G:
            return TRANSVR_CLASS_COPPER_L4_40G;
        case TRANSVR_CLASS_10G:
            return TRANSVR_CLASS_COPPER_L4_10G;
        default:
            return TRANSVR_CLASS_COPPER;
    }

err_qsfp_detect_class_by_feature_1:
    SWPS_INFO("%s: %s\n <port>:%s",
              __func__, err_msg, self->swp_name);
    return TRANSVR_CLASS_ERROR;
}


int
qsft_detect_transvr_class(struct transvr_obj_s* self) {

    int detect_val   = DEBUG_TRANSVR_INT_VAL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    /* Check Extended Compliance */
    detect_val = _qsfp_detect_class_by_extend_comp(self);
    switch (detect_val) {
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        case TRANSVR_CLASS_OPTICAL_100G:
        case TRANSVR_CLASS_OPTICAL_100G_AOC:
        case TRANSVR_CLASS_OPTICAL_100G_SR4:
        case TRANSVR_CLASS_OPTICAL_100G_LR4:
        case TRANSVR_CLASS_OPTICAL_100G_ER4:
        case TRANSVR_CLASS_OPTICAL_100G_PSM4:
        case TRANSVR_CLASS_OPTICAL_40G:
        case TRANSVR_CLASS_OPTICAL_40G_AOC:
        case TRANSVR_CLASS_OPTICAL_40G_SR4:
        case TRANSVR_CLASS_OPTICAL_40G_LR4:
        case TRANSVR_CLASS_OPTICAL_40G_ER4:
        case TRANSVR_CLASS_COPPER_L4_100G:
            return detect_val;
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined extend_comp:%d",
                     detect_val);
            goto err_qsft_detect_transvr_class_1;
    }
    /* Check 10/40G/100G Ethernet Compliance */
    detect_val = _qsfp_detect_class_by_10_40_100_ethernet(self);
    switch(detect_val) {
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        case TRANSVR_CLASS_OPTICAL_40G_AOC:
        case TRANSVR_CLASS_OPTICAL_40G_SR4:
        case TRANSVR_CLASS_OPTICAL_40G_LR4:
        case TRANSVR_CLASS_OPTICAL_10G_Q_SR: /* Need Check: SR4 or SR */
        case TRANSVR_CLASS_OPTICAL_10G_Q_LR: /* Need Check: SR4 or SR */
        case TRANSVR_CLASS_COPPER_L4_40G:
            return detect_val;
        case TRANSVR_CLASS_EXTEND_COMP:
            /* Format incorrect case (We already checked the Extend
             * Compliance is 0
             */
            snprintf(err_msg, sizeof(err_msg),
                    "Transceiver format incorrect");
            goto err_qsft_detect_transvr_class_1;
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined 10/40/100:%d",
                     detect_val);
            goto err_qsft_detect_transvr_class_1;
    }
    /* Check by Connector type, BR and wavelength */
    detect_val = _qsfp_detect_class_by_feature(self);
    switch (detect_val) {
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        case TRANSVR_CLASS_OPTICAL_100G_ER4:
        case TRANSVR_CLASS_OPTICAL_100G_LR4:
        case TRANSVR_CLASS_OPTICAL_100G_SR4:
        case TRANSVR_CLASS_OPTICAL_100G_AOC:
        case TRANSVR_CLASS_OPTICAL_100G:
        case TRANSVR_CLASS_OPTICAL_40G_ER4:
        case TRANSVR_CLASS_OPTICAL_40G_LR4:
        case TRANSVR_CLASS_OPTICAL_40G_SR4:
        case TRANSVR_CLASS_OPTICAL_40G_AOC:
        case TRANSVR_CLASS_OPTICAL_40G:
        case TRANSVR_CLASS_OPTICAL_10G_Q_ER:
        case TRANSVR_CLASS_OPTICAL_10G_Q_LR:
        case TRANSVR_CLASS_OPTICAL_10G_Q_SR:
        case TRANSVR_CLASS_OPTICAL_10G_Q_AOC:
        case TRANSVR_CLASS_OPTICAL_10G:
        case TRANSVR_CLASS_OPTICAL:
        case TRANSVR_CLASS_COPPER_L4_100G:
        case TRANSVR_CLASS_COPPER_L4_40G:
        case TRANSVR_CLASS_COPPER_L4_10G:
        case TRANSVR_CLASS_COPPER:
            return detect_val;
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined connector:%d",
                     detect_val);
            goto err_qsft_detect_transvr_class_1;
    }
    /* Exception case: Can't verify */
    snprintf(err_msg, sizeof(err_msg),
            "Can not identify!");
    goto err_qsft_detect_transvr_class_1;

err_qsft_detect_transvr_class_1:
    SWPS_INFO("%s: %s\n <port>:%s", __func__, err_msg, self->swp_name);
    return TRANSVR_CLASS_ERROR;
}


int
_qsfp_set_trident2_if_type(struct transvr_obj_s* self,
                           int transvr_cls,
                           char *result){

    int lmax = 8;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    switch (transvr_cls) {
        case TRANSVR_CLASS_UNSPECIFIED:
        case TRANSVR_CLASS_ERROR:
            break;
        /* 100G Optical */
        case TRANSVR_CLASS_OPTICAL_100G:
        case TRANSVR_CLASS_OPTICAL_100G_AOC:
        case TRANSVR_CLASS_OPTICAL_100G_SR4:
        case TRANSVR_CLASS_OPTICAL_100G_LR4:
        case TRANSVR_CLASS_OPTICAL_100G_ER4:
        case TRANSVR_CLASS_OPTICAL_100G_PSM4:
            return snprintf(result, lmax, TRANSVR_IF_SR4);
        /* 100G Copper */
        case TRANSVR_CLASS_COPPER_L4_100G:
            return snprintf(result, lmax, TRANSVR_IF_KR4);
        /* 40G Optical */
        case TRANSVR_CLASS_OPTICAL_40G:
        case TRANSVR_CLASS_OPTICAL_40G_AOC:
        case TRANSVR_CLASS_OPTICAL_40G_SR4:
        case TRANSVR_CLASS_OPTICAL_40G_LR4:
        case TRANSVR_CLASS_OPTICAL_40G_ER4:
            return snprintf(result, lmax, TRANSVR_IF_IF_XGMII);
        /* 40G Copper */
        case TRANSVR_CLASS_COPPER_L4_40G:
            return snprintf(result, lmax, TRANSVR_IF_IF_XGMII);
        /* 10G Optical */
        case TRANSVR_CLASS_OPTICAL_10G:
        case TRANSVR_CLASS_OPTICAL_10G_Q_AOC:
        case TRANSVR_CLASS_OPTICAL_10G_Q_SR: /* Need Check: SR4 or SR */
        case TRANSVR_CLASS_OPTICAL_10G_Q_LR: /* Need Check: LR4 or LR */
        case TRANSVR_CLASS_OPTICAL_10G_Q_ER: /* Need Check: ER4 or ER */
            return snprintf(result, lmax, TRANSVR_IF_IF_XGMII);
        /* Optical */
        case TRANSVR_CLASS_OPTICAL:
            return snprintf(result, lmax, TRANSVR_IF_IF_XGMII);
        /* Copper */
        case TRANSVR_CLASS_COPPER:
            return snprintf(result, lmax, TRANSVR_IF_IF_XGMII);
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined value:%d",
                     transvr_cls);
            goto err_qsfp_set_trident2_if_type_1;
    }
    /* Exception case: Can't verify */
    snprintf(err_msg, sizeof(err_msg), "Can not identify!");
    goto err_qsfp_set_trident2_if_type_1;

err_qsfp_set_trident2_if_type_1:
    snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
    SWPS_INFO("%s: %s\n <port>:%s", __func__, err_msg, self->swp_name);
    return ERR_TRANSVR_ABNORMAL;
}


int
_qsfp_set_tomahawk_if_type(struct transvr_obj_s* self,
                           int transvr_cls,
                           char *result){

    int lmax = 8;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    switch (transvr_cls) {
        case TRANSVR_CLASS_UNSPECIFIED:
        case TRANSVR_CLASS_ERROR:
            break;
        /* 100G Optical */
        case TRANSVR_CLASS_OPTICAL_100G:
        case TRANSVR_CLASS_OPTICAL_100G_AOC:
        case TRANSVR_CLASS_OPTICAL_100G_SR4:
        case TRANSVR_CLASS_OPTICAL_100G_LR4:
        case TRANSVR_CLASS_OPTICAL_100G_ER4:
        case TRANSVR_CLASS_OPTICAL_100G_PSM4:
            return snprintf(result, lmax, TRANSVR_IF_SR4);
        /* 100G Copper */
        case TRANSVR_CLASS_COPPER_L4_100G:
            return snprintf(result, lmax, TRANSVR_IF_KR4);
        /* 40G Optical */
        case TRANSVR_CLASS_OPTICAL_40G:
        case TRANSVR_CLASS_OPTICAL_40G_AOC:
        case TRANSVR_CLASS_OPTICAL_40G_SR4:
        case TRANSVR_CLASS_OPTICAL_40G_LR4:
        case TRANSVR_CLASS_OPTICAL_40G_ER4:
            return snprintf(result, lmax, TRANSVR_IF_SR4);
        /* 40G Copper */
        case TRANSVR_CLASS_COPPER_L4_40G:
            return snprintf(result, lmax, TRANSVR_IF_KR4);
        /* 10G Optical */
        case TRANSVR_CLASS_OPTICAL_10G:
        case TRANSVR_CLASS_OPTICAL_10G_Q_AOC:
        case TRANSVR_CLASS_OPTICAL_10G_Q_SR: /* Need Check: SR4 or SR */
        case TRANSVR_CLASS_OPTICAL_10G_Q_LR: /* Need Check: SR4 or SR */
        case TRANSVR_CLASS_OPTICAL_10G_Q_ER:
            return snprintf(result, lmax, TRANSVR_IF_SR4);
        /* Optical */
        case TRANSVR_CLASS_OPTICAL:
            return snprintf(result, lmax, TRANSVR_IF_SR4);
        /* Copper */
        case TRANSVR_CLASS_COPPER:
            return snprintf(result, lmax, TRANSVR_IF_KR4);
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined value:%d",
                     transvr_cls);
            goto err_qsfp_set_trident2_if_type_1;
    }
    /* Exception case: Can't verify */
    snprintf(err_msg, sizeof(err_msg), "Can not identify!");
    goto err_qsfp_set_trident2_if_type_1;

err_qsfp_set_trident2_if_type_1:
    snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
    SWPS_INFO("%s: %s\n <port>:%s", __func__, err_msg, self->swp_name);
    return ERR_TRANSVR_ABNORMAL;
}


int
_qsfp_set_bf_tofino_if_type(struct transvr_obj_s* self,
                            int transvr_cls,
                            char *result) {
    /* (TBD)
     *  Due to BF looks like doesn't have interface type.
     *  We bypass it currently.
     */
    int lmax = 8;    
    return snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
}


int
_qsfp_detect_if_type(struct transvr_obj_s* self,
                     char *result){

    int lmax = 8;
    int detect_cls = DEBUG_TRANSVR_INT_VAL;

    detect_cls = qsft_detect_transvr_class(self);
    switch (self->chipset_type) {
        case BCM_CHIP_TYPE_TRIDENT_2:
            return _qsfp_set_trident2_if_type(self, detect_cls, result);
        
        case BCM_CHIP_TYPE_TRIDENT_3:
        case BCM_CHIP_TYPE_TOMAHAWK:
            return _qsfp_set_tomahawk_if_type(self, detect_cls, result);

        case BF_CHIP_TYPE_TOFINO:
            return _qsfp_set_bf_tofino_if_type(self, detect_cls, result);
            
        default:
            SWPS_INFO("%s: non-defined chipset_type:%d <port>:%s\n",
                      __func__, self->chipset_type, self->swp_name);
            break;
    }
    snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
    return ERR_TRANSVR_ABNORMAL;
}


int
qsfp_get_if_type(struct transvr_obj_s *self,
                 char *buf_p){

    int lmax = 8;
    char tmp_result[8] = DEBUG_TRANSVR_STR_VAL;

    if (self->state != STATE_TRANSVR_CONNECTED) {
        return snprintf(buf_p, lmax, "%d\n", self->state);
    }
    if (_qsfp_detect_if_type(self, tmp_result) < 0) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_ABNORMAL);
    }
    return snprintf(buf_p, lmax, "%s\n", tmp_result);
}


int
_qsfp_detect_if_speed(struct transvr_obj_s* self,
                      char *result){
    int lmax = 16;
    int detect_val   = DEBUG_TRANSVR_INT_VAL;
    char err_msg[64] = DEBUG_TRANSVR_STR_VAL;

    detect_val = qsft_detect_transvr_class(self);
    switch (detect_val) {
        case TRANSVR_CLASS_UNSPECIFIED:
        case TRANSVR_CLASS_ERROR:
            break;
        /* 100G Optical */
        case TRANSVR_CLASS_OPTICAL_100G:
        case TRANSVR_CLASS_OPTICAL_100G_AOC:
        case TRANSVR_CLASS_OPTICAL_100G_SR4:
        case TRANSVR_CLASS_OPTICAL_100G_LR4:
        case TRANSVR_CLASS_OPTICAL_100G_ER4:
        case TRANSVR_CLASS_OPTICAL_100G_PSM4:
            return snprintf(result, lmax, TRANSVR_IF_SP_100G);
        /* 100G Copper */
        case TRANSVR_CLASS_COPPER_L4_100G:
            return snprintf(result, lmax, TRANSVR_IF_SP_100G);
        /* 40G Optical */
        case TRANSVR_CLASS_OPTICAL_40G:
        case TRANSVR_CLASS_OPTICAL_40G_AOC:
        case TRANSVR_CLASS_OPTICAL_40G_SR4:
        case TRANSVR_CLASS_OPTICAL_40G_LR4:
        case TRANSVR_CLASS_OPTICAL_40G_ER4:
            return snprintf(result, lmax, TRANSVR_IF_SP_40G);
        /* 40G Copper */
        case TRANSVR_CLASS_COPPER_L4_40G:
            return snprintf(result, lmax, TRANSVR_IF_SP_40G);
        /* 10G Optical */
        case TRANSVR_CLASS_OPTICAL_10G:
        case TRANSVR_CLASS_OPTICAL_10G_Q_AOC:
        case TRANSVR_CLASS_OPTICAL_10G_Q_SR: /* Need Check: SR4 or SR */
        case TRANSVR_CLASS_OPTICAL_10G_Q_LR: /* Need Check: SR4 or SR */
        case TRANSVR_CLASS_OPTICAL_10G_Q_ER:
            return snprintf(result, lmax, TRANSVR_IF_SP_10G);
        /* 10G Copper */
        case TRANSVR_CLASS_COPPER_L4_10G:
            return snprintf(result, lmax, TRANSVR_IF_SP_10G);
        /* Optical */
        case TRANSVR_CLASS_OPTICAL:
            break;
        /* Copper */
        case TRANSVR_CLASS_COPPER:
            break;
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined class case:%d",
                     detect_val);
            goto err_qsfp_detect_if_speed_1;
    }
    /* Check br and extbr */
    detect_val = _qsfp_detect_if_sp_by_br(self);
    switch(detect_val) {
        case TRANSVR_CLASS_UNSPECIFIED:
            break;
        case TRANSVR_CLASS_10G:
            return snprintf(result, lmax, TRANSVR_IF_SP_10G);
        case TRANSVR_CLASS_40G:
            return snprintf(result, lmax, TRANSVR_IF_SP_40G);
        case TRANSVR_CLASS_100G:
            return snprintf(result, lmax, TRANSVR_IF_SP_100G);
        default:
            snprintf(err_msg, sizeof(err_msg),
                     "Detect undefined BR case:%d",
                     detect_val);
            goto err_qsfp_detect_if_speed_1;
    }
    /* Exception case: Can't verify */
    snprintf(err_msg, sizeof(err_msg), "Can not identify!");
    goto err_qsfp_detect_if_speed_1;

err_qsfp_detect_if_speed_1:
    snprintf(result, lmax, TRANSVR_UEVENT_UNKNOW);
    SWPS_INFO("%s: %s <port>:%s\n", __func__, err_msg, self->swp_name);
    return ERR_TRANSVR_ABNORMAL;
}


int
qsfp_get_if_speed(struct transvr_obj_s *self,
                 char *buf_p){

    int lmax = 16;
    char tmp_result[16] = DEBUG_TRANSVR_STR_VAL;

    if (self->state != STATE_TRANSVR_CONNECTED) {
        return snprintf(buf_p, lmax, "%d\n", self->state);
    }
    if (_qsfp_detect_if_speed(self, tmp_result) < 0) {
        return snprintf(buf_p, lmax, "%d\n", ERR_TRANSVR_ABNORMAL);
    }
    return snprintf(buf_p, lmax, "%s\n", tmp_result);
}


int
_common_set_lane_map_str(struct transvr_obj_s* self,
                         char *result) {
    int  i = 0;
    int  tmp_val = 0;
    char tmp_str[LEN_TRANSVR_L_STR] = DEBUG_TRANSVR_STR_VAL;
    char err_msg[LEN_TRANSVR_L_STR] = DEBUG_TRANSVR_STR_VAL;

    memset(result, 0, LEN_TRANSVR_L_STR);
    snprintf(result, LEN_TRANSVR_L_STR, "%s=", TRANSVR_UEVENT_KEY_LANE);

    for (i=0; i<ARRAY_SIZE(self->lane_id); i++) {
        tmp_val = self->lane_id[i];
        if (tmp_val < 1) {
            break;
        }
        if (tmp_val > 256) {
            snprintf(err_msg, sizeof(err_msg),
                    "detect abnormal value:%d", tmp_val);
            goto err_common_set_lane_map_str_1;
        }
        memset(tmp_str, 0, sizeof(tmp_str));
        if (i == 0) {
            snprintf(tmp_str, LEN_TRANSVR_L_STR, "%d", tmp_val);
        } else {
            snprintf(tmp_str, LEN_TRANSVR_L_STR, ",%d", tmp_val);
        }
        strncat(result, tmp_str, LEN_TRANSVR_L_STR);
    }
    if (i == 0) {
        goto err_common_set_lane_map_str_2;
    }
    return 0;

err_common_set_lane_map_str_1:
    SWPS_INFO("%s: %s", __func__, err_msg);
err_common_set_lane_map_str_2:
    snprintf(result, LEN_TRANSVR_L_STR, "%s=%s", TRANSVR_UEVENT_KEY_LANE, TRANSVR_UEVENT_UNKNOW);
    return EVENT_TRANSVR_TASK_FAIL;
}


int
_common_send_uevent(struct transvr_obj_s* self,
                    enum kobject_action u_action,
                    int (*detect_if_type)(struct transvr_obj_s *self, char *result),
                    int (*detect_if_speed)(struct transvr_obj_s *self, char *result),
                    int send_anyway) {

    char *uevent_envp[4];
    char err_msg[64]   = DEBUG_TRANSVR_STR_VAL;
    char tmp_str[32]   = DEBUG_TRANSVR_STR_VAL;
    char tmp_str_1[32] = DEBUG_TRANSVR_STR_VAL;
    char tmp_str_2[32] = DEBUG_TRANSVR_STR_VAL;
    char tmp_str_3[64] = DEBUG_TRANSVR_STR_VAL;

    if (TRANSVR_UEVENT_ENABLE != 1) {
        return ERR_TRANSVR_NOTSUPPORT;
    }
    if (_common_get_if_lane(self, tmp_str) < 0) {
        snprintf(tmp_str_3, sizeof(tmp_str_3),
                "%s=%s", TRANSVR_UEVENT_KEY_LANE, TRANSVR_UEVENT_UNKNOW);
    } else {
        snprintf(tmp_str_3, sizeof(tmp_str_3),
                "%s=%s", TRANSVR_UEVENT_KEY_LANE, tmp_str);
    }
    switch (u_action) {
        case KOBJ_ADD:
            /* Detect type */
            if (detect_if_type(self, tmp_str) < 0) {
                snprintf(err_msg, sizeof(err_msg), "%s", "Detect interface type fail!");
                snprintf(tmp_str_1, sizeof(tmp_str_1), "%s=%s", TRANSVR_UEVENT_KEY_IF, TRANSVR_UEVENT_UNKNOW);
                snprintf(tmp_str_2, sizeof(tmp_str_2), "%s=%s", TRANSVR_UEVENT_KEY_SP, TRANSVR_UEVENT_UNKNOW);
                uevent_envp[0] = tmp_str_1;
                uevent_envp[1] = tmp_str_2;
                uevent_envp[2] = tmp_str_3;
                uevent_envp[3] = NULL;
                goto private_common_send_uevent_4_fail;
            }
            snprintf(tmp_str_1, sizeof(tmp_str_1), "%s=%s", TRANSVR_UEVENT_KEY_IF, tmp_str);
            uevent_envp[0] = tmp_str_1;
            /* Detect speed */
            if (detect_if_speed(self, tmp_str) < 0) {
                snprintf(err_msg, sizeof(err_msg), "%s", "Detect interface speed fail!");
                snprintf(tmp_str_2, sizeof(tmp_str_2), "%s=%s", TRANSVR_UEVENT_KEY_SP, TRANSVR_UEVENT_UNKNOW);
                uevent_envp[1] = tmp_str_2;
                uevent_envp[2] = tmp_str_3;
                uevent_envp[3] = NULL;
                goto private_common_send_uevent_4_fail;
            }
            snprintf(tmp_str_2, sizeof(tmp_str_2), "%s=%s", TRANSVR_UEVENT_KEY_SP, tmp_str);
            uevent_envp[1] = tmp_str_2;
            uevent_envp[2] = tmp_str_3;
            uevent_envp[3] = NULL;
            goto private_common_send_uevent_4_send;

        case KOBJ_REMOVE:
            snprintf(tmp_str_1, sizeof(tmp_str_1), "%s=%s", TRANSVR_UEVENT_KEY_IF, TRANSVR_UEVENT_UNKNOW);
            snprintf(tmp_str_2, sizeof(tmp_str_2), "%s=%s", TRANSVR_UEVENT_KEY_SP, TRANSVR_UEVENT_UNKNOW);
            uevent_envp[0] = tmp_str_1;
            uevent_envp[1] = tmp_str_2;
            uevent_envp[2] = tmp_str_3;
            uevent_envp[3] = NULL;
            goto private_common_send_uevent_4_send;

        default:
            snprintf(err_msg, sizeof(err_msg), "kobject_action:%d not support", u_action);
            goto private_common_send_uevent_4_fail;
    }
    snprintf(err_msg, sizeof(err_msg), "%s", "Exception case");
    goto private_common_send_uevent_4_fail;

private_common_send_uevent_4_fail:
    SWPS_INFO("%s: %s <port>:%s\n", __func__, err_msg, self->swp_name);
    if (send_anyway) {
        goto private_common_send_uevent_4_send;
    }
    return ERR_TRANSVR_UEVENT_FAIL;

private_common_send_uevent_4_send:
    return kobject_uevent_env(&(self->transvr_dev_p->kobj),
                              u_action,
                              uevent_envp);
}

int
sfp_send_uevent(struct transvr_obj_s* self,
                enum kobject_action u_action) {
    int send_anyway = 1;
    return _common_send_uevent(self,
                               u_action,
                               &_sfp_detect_if_type,
                               &_sfp_detect_if_speed,
                               send_anyway);
}


int
qsfp_send_uevent(struct transvr_obj_s* self,
                enum kobject_action u_action) {
    int send_anyway = 1;
    return _common_send_uevent(self,
                               u_action,
                               &_qsfp_detect_if_type,
                               &_qsfp_detect_if_speed,
                               send_anyway);
}


int
fake_send_uevent(struct transvr_obj_s* self,
                 enum kobject_action u_action) {
    return EVENT_TRANSVR_TASK_DONE;
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


static int
_is_except_happened_4_pmode(struct transvr_obj_s* self,
                            int new_state) {

    int event_chk = 0;

    if (self->temp == 0){
        return 0;
    }
    switch (new_state) {
        case STATE_TRANSVR_INIT:
            event_chk = EVENT_TRANSVR_EXCEP_INIT;
            goto check_event_happened_4_pmode;

        case STATE_TRANSVR_CONNECTED:
            event_chk = EVENT_TRANSVR_EXCEP_UP;
            goto check_event_happened_4_pmode;

        case STATE_TRANSVR_DISCONNECTED:
            event_chk = EVENT_TRANSVR_EXCEP_DOWN;
            goto check_event_happened_4_pmode;

        case STATE_TRANSVR_SWAPPED:
            event_chk = EVENT_TRANSVR_EXCEP_SWAP;
            goto check_event_happened_4_pmode;

        case STATE_TRANSVR_UNEXCEPTED:
            event_chk = EVENT_TRANSVR_EXCEP_EXCEP;
            goto check_event_happened_4_pmode;

        case STATE_TRANSVR_ISOLATED:
            event_chk = EVENT_TRANSVR_EXCEP_ISOLATED;
            goto check_event_happened_4_pmode;

        default:
            SWPS_INFO("%s: unexcepted case:%d\n", __func__, new_state);
            break;
    }
    return 0;

check_event_happened_4_pmode:
    if (self->temp == event_chk){
        return 1;
    }
    return 0;
}


int
common_fsm_4_polling_mode(struct transvr_obj_s* self,
                          char *caller_name){
    /* [Return Value]:
     *   ERR_TRANSVR_UNINIT      : (1) Initial not ready
     *   ERR_TRANSVR_UNPLUGGED   : (1) Any -> Down
     *   ERR_TRANSVR_TASK_BUSY   : (1) Wait Initial task
     *   ERR_TRANSVR_UNEXCPT     : (1) Initial fail
     *                             (2) Task fail
     *                             (3) Reload fail
     *   ERR_TRNASVR_BE_ISOLATED : (1) Already be isolated
     *   OK Case (return 0)      : (1) action_4_connected
     *                             (2) action_4_nothing (initial retry)
     */
    int curr_state[2];
    int old_state  = self->state;
    int old_type   = self->type;
    int new_state  = STATE_TRANSVR_UNEXCEPTED;
    int new_type   = TRANSVR_TYPE_ERROR;
    int return_val = ERR_TRANSVR_UNEXCPT;

    /* Never initial */
    if (self->state == STATE_TRANSVR_NEW) {
        goto comfsm_action_4_reinit_obj;
    }
    /* Detect current state */
    switch (detect_transvr_state(self, curr_state)) {
      case 0:
          new_state = curr_state[0];
          new_type  = curr_state[1];
          break;

      case ERR_TRNASVR_BE_ISOLATED:
          new_state = STATE_TRANSVR_ISOLATED;
          new_type  = old_type;
          break;

      case ERR_TRANSVR_I2C_CRASH:
          goto comfsm_action_4_report_i2c_crash;

      case ERR_TRANSVR_UNEXCPT:
      default:
          new_state = STATE_TRANSVR_UNEXCEPTED;
          new_type  = old_type;
    }
    /* State handling  */
    switch (old_state) {
       case STATE_TRANSVR_INIT:                  /* INIT -> <ANY> */
           return_val = ERR_TRANSVR_UNINIT;
           goto comfsm_action_4_keep_state;

       case STATE_TRANSVR_CONNECTED:
           switch (new_state) {
               case STATE_TRANSVR_INIT:          /* Case 1-1: UP -> INIT */
                   SWPS_INFO("Detect %s is present. <case>:1-1\n",self->swp_name);
                   return_val = ERR_TRANSVR_UNINIT;
                   goto comfsm_action_4_keep_state;

               case STATE_TRANSVR_CONNECTED:     /* Case 1-2: UP -> UP */
                   return_val = 0;
                   goto comfsm_action_4_keep_state;

               case STATE_TRANSVR_DISCONNECTED:  /* Case 1-3: UP -> DOWN */
                   SWPS_INFO("Detect %s is removed. <case>:1-3\n",self->swp_name);
                   goto comfsm_action_4_disconnected;

               case STATE_TRANSVR_SWAPPED:       /* Case 1-4: UP -> SWAP */
                   SWPS_INFO("Detect %s is swapped. <case>:1-4\n",self->swp_name);
                   goto comfsm_action_4_reload_obj;

               case STATE_TRANSVR_UNEXCEPTED:    /* Case 1-5: UP -> UNEXPET */
                   SWPS_INFO("Detect %s has error. <case>:1-5\n",self->swp_name);
                   goto comfsm_action_4_unexpected;

               case STATE_TRANSVR_ISOLATED:      /* Case 1-6: UP -> ISOLATE */
                   SWPS_INFO("Detect %s be isolated. <case>:1-6\n",self->swp_name);
                   goto comfsm_action_4_isolate_obj;

               default:
                   break;
           }
           goto comfsm_action_4_unexpected;

       case STATE_TRANSVR_DISCONNECTED:
           switch (new_state) {
               case STATE_TRANSVR_INIT:          /* Case 2-1: DOWN -> INIT */
                   SWPS_INFO("Detect %s is present. <case>:2-1\n",self->swp_name);
                   return_val   = ERR_TRANSVR_UNINIT;
                   goto comfsm_action_4_keep_state;

               case STATE_TRANSVR_CONNECTED:     /* Case 2-2: DOWN -> UP */
                   SWPS_INFO("Detect %s is present. <case>:2-2\n",self->swp_name);
                   goto comfsm_action_4_reinit_obj;

               case STATE_TRANSVR_DISCONNECTED:  /* Case 2-3: DOWN -> DOWN */
                   return_val = ERR_TRANSVR_UNPLUGGED;
                   goto comfsm_action_4_keep_state;

               case STATE_TRANSVR_SWAPPED:       /* Case 2-4: DOWN -> SWAP */
                   SWPS_INFO("Detect %s is swapped. <case>:2-4\n",self->swp_name);
                   goto comfsm_action_4_reload_obj;

               case STATE_TRANSVR_UNEXCEPTED:    /* Case 2-5: DOWN -> UNEXPET */
                   SWPS_INFO("Detect %s has error. <case>:2-5\n",self->swp_name);
                   goto comfsm_action_4_unexpected;

               case STATE_TRANSVR_ISOLATED:      /* Case 2-6: DOWN -> ISOLATE */
                   SWPS_INFO("Detect %s be isolated. <case>:2-6\n",self->swp_name);
                   goto comfsm_action_4_isolate_obj;

               default:
                   break;
           }
           goto comfsm_action_4_unexpected;

       case STATE_TRANSVR_UNEXCEPTED:
           /* Filter out re-action */
           if (_is_except_happened_4_pmode(self, new_state)) {
               goto comfsm_action_4_keep_state;
           }
           /* First action */
           switch (new_state) {
               case STATE_TRANSVR_INIT:          /* Case 3-1: UNEXPET -> INIT */
                   SWPS_INFO("Detect %s is present. <case>:3-1\n",self->swp_name);
                   self->temp = EVENT_TRANSVR_EXCEP_INIT;
                   return_val = ERR_TRANSVR_UNINIT;
                   goto comfsm_action_4_keep_state;

               case STATE_TRANSVR_CONNECTED:     /* Case 3-2: UNEXPET -> UP */
                   SWPS_INFO("Detect %s is present. <case>:3-2\n",self->swp_name);
                   self->temp = EVENT_TRANSVR_EXCEP_UP;
                   goto comfsm_action_4_reload_obj;

               case STATE_TRANSVR_DISCONNECTED:  /* Case 3-3: UNEXPET -> DOWN */
                   SWPS_INFO("Detect %s is removed. <case>:3-3\n",self->swp_name);
                   goto comfsm_action_4_disconnected;

               case STATE_TRANSVR_SWAPPED:       /* Case 3-4: UNEXPET -> SWAP */
                   SWPS_INFO("Detect %s is swapped. <case>:3-4\n",self->swp_name);
                   self->temp = EVENT_TRANSVR_EXCEP_SWAP;
                   goto comfsm_action_4_reload_obj;

               case STATE_TRANSVR_UNEXCEPTED:    /* Case 3-5: UNEXPET -> UNEXPET */
                   self->temp = EVENT_TRANSVR_EXCEP_EXCEP;
                   return_val = ERR_TRANSVR_UNEXCPT;
                   goto comfsm_action_4_keep_state;

               case STATE_TRANSVR_ISOLATED:      /* Case 3-6: UNEXPET -> ISOLATE */
                   SWPS_INFO("Detect %s be isolated. <case>:3-6\n",self->swp_name);
                   goto comfsm_action_4_isolate_obj;

               default:
                   break;
           }
           goto comfsm_action_4_unexpected;

        case STATE_TRANSVR_ISOLATED:
            /* Filter out re-action */
            if (_is_except_happened_4_pmode(self, new_state)) {
                goto comfsm_action_4_keep_state;
            }
            /* First action */
            switch (new_state) {
                case STATE_TRANSVR_INIT:          /* Case 4-1: ISOLATE -> INIT */
                    SWPS_INFO("Detect %s internal error. <case>:4-1\n",self->swp_name);
                    self->temp = EVENT_TRANSVR_EXCEP_INIT;
                    return_val = ERR_TRNASVR_BE_ISOLATED;
                    goto comfsm_action_4_keep_state;

                case STATE_TRANSVR_CONNECTED:     /* Case 4-2: ISOLATE -> UP */
                    SWPS_INFO("Detect %s internal error. <case>:4-2\n",self->swp_name);
                    self->temp = EVENT_TRANSVR_EXCEP_UP;
                    return_val = ERR_TRNASVR_BE_ISOLATED;
                    goto comfsm_action_4_keep_state;

                case STATE_TRANSVR_DISCONNECTED:  /* Case 4-3: ISOLATE -> DOWN */
                    SWPS_INFO("Detect %s is removed. <case>:4-3\n",self->swp_name);
                    goto comfsm_action_4_disconnected;

                case STATE_TRANSVR_SWAPPED:       /* Case 4-4: ISOLATE -> SWAP */
                    SWPS_INFO("Detect %s internal error. <case>:4-4\n",self->swp_name);
                    self->temp = EVENT_TRANSVR_EXCEP_SWAP;
                    return_val = ERR_TRNASVR_BE_ISOLATED;
                    goto comfsm_action_4_keep_state;

                case STATE_TRANSVR_UNEXCEPTED:    /* Case 4-5: ISOLATE -> UNEXPET */
                    SWPS_INFO("Detect %s internal error. <case>:4-5\n",self->swp_name);
                    self->temp = EVENT_TRANSVR_EXCEP_EXCEP;
                    return_val = ERR_TRNASVR_BE_ISOLATED;
                    goto comfsm_action_4_keep_state;

                case STATE_TRANSVR_ISOLATED:      /* Case 4-6: ISOLATE -> ISOLATE */
                    return_val = ERR_TRNASVR_BE_ISOLATED;
                    goto comfsm_action_4_keep_state;

                default:
                    break;
            }
            goto comfsm_action_4_unexpected;

       default:
           break;
    }
    goto comfsm_action_4_unexpected;


comfsm_action_4_keep_state:
    return return_val;

comfsm_action_4_reinit_obj:
    SWPS_DEBUG("FSM action: %s re-initial.\n", self->swp_name);
    return_val = _transvr_init_handler(self);
    goto comfsm_action_4_identify_event;

comfsm_action_4_reload_obj:
    SWPS_DEBUG("FSM action: %s reload.\n", self->swp_name);
    self->type = new_type;
    return_val = reload_transvr_obj(self, new_type);
    goto comfsm_action_4_identify_event;

comfsm_action_4_identify_event:
    switch (return_val) {
        case EVENT_TRANSVR_INIT_UP:
        case EVENT_TRANSVR_TASK_DONE:
            goto comfsm_action_4_connected;

        case EVENT_TRANSVR_INIT_DOWN:
            goto comfsm_action_4_disconnected;

        case EVENT_TRANSVR_INIT_REINIT:
            goto comfsm_action_4_nothing;

        case EVENT_TRANSVR_TASK_WAIT:
            self->state = STATE_TRANSVR_INIT;
            return ERR_TRANSVR_TASK_BUSY;

        case EVENT_TRANSVR_TASK_FAIL:
            SWPS_INFO("%s detect EVENT_TRANSVR_TASK_FAIL.\n", self->swp_name);
            goto comfsm_action_4_unexpected;

        case EVENT_TRANSVR_INIT_FAIL:
            SWPS_INFO("%s detect EVENT_TRANSVR_INIT_FAIL.\n", self->swp_name);
            goto comfsm_action_4_unexpected;

        case EVENT_TRANSVR_RELOAD_FAIL:
            SWPS_INFO("%s detect EVENT_TRANSVR_RELOAD_FAIL.\n", self->swp_name);
            goto comfsm_action_4_unexpected;

        case EVENT_TRANSVR_I2C_CRASH:
            goto comfsm_action_4_report_i2c_crash;

        case EVENT_TRANSVR_EXCEP_ISOLATED:
            goto comfsm_action_4_isolate_obj;

        default:
            SWPS_INFO("%s detect undefined event:%d.\n", self->swp_name, return_val);
            goto comfsm_action_4_unexpected;
    }

comfsm_action_4_nothing:
    SWPS_DEBUG("FSM action: %s do nothing.\n", self->swp_name);
    return 0;

comfsm_action_4_connected:
    SWPS_DEBUG("FSM action: %s Connected.\n", self->swp_name);
    self->state = STATE_TRANSVR_CONNECTED;
    self->type  = new_type;
    self->send_uevent(self, KOBJ_ADD);
    _transvr_clean_retry(self);
    return 0;

comfsm_action_4_disconnected:
    SWPS_DEBUG("FSM action: %s Disconnected. \n", self->swp_name);
    self->state = STATE_TRANSVR_DISCONNECTED;
    self->temp  = EVENT_TRANSVR_TASK_DONE;
    self->send_uevent(self, KOBJ_REMOVE);
    _transvr_clean_retry(self);
    _transvr_clean_handler(self);
    return ERR_TRANSVR_UNPLUGGED;

comfsm_action_4_report_i2c_crash:
    SWPS_DEBUG("FSM action: %s report I2C crash.\n", self->swp_name);
    self->state = STATE_TRANSVR_UNEXCEPTED;
    return ERR_TRANSVR_I2C_CRASH;

comfsm_action_4_isolate_obj:
    SWPS_DEBUG("FSM action: %s isolate.\n", self->swp_name);
    self->state = STATE_TRANSVR_ISOLATED;
    return ERR_TRNASVR_BE_ISOLATED;

comfsm_action_4_unexpected:
    SWPS_INFO("FSM action: %s unexpected.\n", self->swp_name);
    SWPS_INFO("Dump: <os>:%d <ot>:0x%02x <ns>:%d <nt>:0x%02x\n",
              old_state, old_type, new_state, new_type);
    self->state = STATE_TRANSVR_UNEXCEPTED;
    self->send_uevent(self, KOBJ_REMOVE);
    _transvr_clean_handler(self);
    return ERR_TRANSVR_UNEXCPT;
}


int
fake_fsm_4_direct_mode(struct transvr_obj_s* self,
                       char *caller_name){
    self->state = STATE_TRANSVR_CONNECTED;
    self->type  = TRANSVR_TYPE_FAKE;
    return 0;
}


int
fake_fsm_4_polling_mode(struct transvr_obj_s* self,
                        char *caller_name){
    self->state = STATE_TRANSVR_CONNECTED;
    self->type  = TRANSVR_TYPE_FAKE;
    return 0;
}


/* ========== Object functions for Initial procedure ==========
 */
int
transvr_init_common(struct transvr_obj_s *self){
    /* Nothing to update */
    return EVENT_TRANSVR_TASK_DONE;
}


int
transvr_init_fake(struct transvr_obj_s *self){
    return EVENT_TRANSVR_TASK_DONE;
}


int
transvr_init_sfp(struct transvr_obj_s *self){

    int  tmp_val  = DEBUG_TRANSVR_INT_VAL;
    int  err_code = DEBUG_TRANSVR_INT_VAL;
    char *err_msg = "ERR";

    self->info = sft_detect_transvr_class(self);
    /* Disable auto_config */
    if (!self->auto_config) {
        return EVENT_TRANSVR_TASK_DONE;
    }
    /* Handle multi-rate */
    err_code = initfunc_sfp_handle_multi_rate_mode(self);
    if (err_code < 0) {
        err_msg = "initfunc_sfp_handle_multi_rate_mode fail!";
        goto err_transvr_init_sfp_1;
    }
    /* Handle 1G- RJ45 */
    tmp_val  = err_code;
    err_code = initfunc_sfp_handle_1g_rj45(self);
    if (err_code < 0) {
        err_msg = "initfunc_sfp_handle_1g_rj45 fail!";
        goto err_transvr_init_sfp_1;
    }
    tmp_val = (tmp_val > err_code ? tmp_val : err_code);
    if (tmp_val > EVENT_TRANSVR_TASK_DONE) {
        return tmp_val;
    }
    return EVENT_TRANSVR_TASK_DONE;

err_transvr_init_sfp_1:
    SWPS_INFO("%s: %s <err>:%d <port>:%s\n",
              __func__, err_msg, err_code, self->swp_name);
    return EVENT_TRANSVR_INIT_FAIL;
}


int
transvr_init_qsfp(struct transvr_obj_s *self){

    int  err = EVENT_TRANSVR_EXCEP_EXCEP;
    char *emsg = "ERR";

    self->info = qsft_detect_transvr_class(self);
    if (!self->auto_config) {
        return EVENT_TRANSVR_TASK_DONE;
    }
    err = initfunc_qsfp_handle_power_mode(self);
    if (err < 0){
        emsg = "initfunc_qsfp_handle_tx_disable fail!";
        goto err_transvr_init_qsfp;
    }
    return EVENT_TRANSVR_TASK_DONE;

err_transvr_init_qsfp:
    SWPS_INFO("%s: %s <err>:%d <port>:%s\n",
              __func__, emsg, err, self->swp_name);
    return EVENT_TRANSVR_INIT_FAIL;
}


int
transvr_init_qsfp28(struct transvr_obj_s *self){

    int  tmp_val  = EVENT_TRANSVR_EXCEP_EXCEP;
    int  err_val  = EVENT_TRANSVR_EXCEP_EXCEP;
    char *err_msg = "ERR";

    /* Handle QSFP common */
    err_val = transvr_init_qsfp(self);
    if (err_val < 0){
        err_msg = "transvr_init_qsfp fail!";
        goto err_transvr_init_qsfp28_1;
    }
    /* Disable auto_config */
    if (!self->auto_config) {
        return err_val;
    }
    /* Handle CDR */
    tmp_val = err_val;
    err_val = initfunc_qsfp28_handle_cdr(self);
    if (err_val < 0){
        err_msg = "Handle CDR fail!";
        goto err_transvr_init_qsfp28_1;
    }
    tmp_val = (tmp_val > err_val ? tmp_val : err_val);
    if (tmp_val > EVENT_TRANSVR_TASK_DONE) {
        return tmp_val;
    }
    return EVENT_TRANSVR_TASK_DONE;

err_transvr_init_qsfp28_1:
    SWPS_INFO("%s: %s <err>:%d <port>:%s\n",
              __func__, err_msg, err_val, self->swp_name);
    return EVENT_TRANSVR_INIT_FAIL;
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
_is_transvr_support_ctle(struct transvr_obj_s *self) {

    switch (self->info) {
        case TRANSVR_CLASS_OPTICAL_25G:
        case TRANSVR_CLASS_OPTICAL_25G_AOC:
        case TRANSVR_CLASS_OPTICAL_25G_SR:
        case TRANSVR_CLASS_OPTICAL_25G_LR:
        case TRANSVR_CLASS_OPTICAL_25G_ER:
        case TRANSVR_CLASS_OPTICAL_100G:
        case TRANSVR_CLASS_OPTICAL_100G_AOC:
        case TRANSVR_CLASS_OPTICAL_100G_SR4:
        case TRANSVR_CLASS_OPTICAL_100G_LR4:
        case TRANSVR_CLASS_OPTICAL_100G_ER4:
        case TRANSVR_CLASS_OPTICAL_100G_PSM4:
            return 1;
        default:
            break;
    }
    return 0;
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
    if (self->clean == NULL) {
        snprintf(emsg, elimit, "clean() is null");
        goto initer_err_case_unexcept_0;
    }
    self->clean(self);

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
            goto initer_err_case_unexcept_1;
    }

    /* Handle reload case */
    if (self->type != d_type){
        /* This is the protect mechanism. Normally, This case will not happen.
         * When State machine detect swap event during initial, It will trigger
         * reload function to ensure type correct. */
        if (_reload_transvr_obj(self, d_type) < 0){
            snprintf(emsg, elimit, "reload object fail");
            goto initer_err_case_unexcept_1;
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
            goto initer_err_case_unexcept_1;
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
            goto initer_err_case_unexcept_1;
    }
    goto initer_ok_case_up;


initer_ok_case_wait:
    self->dump_all(self);
    return EVENT_TRANSVR_TASK_WAIT;

initer_ok_case_up:
    self->state = STATE_TRANSVR_CONNECTED;
    self->temp  = 0;
    self->dump_all(self);
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

initer_err_case_unexcept_1:
    self->clean(self);
initer_err_case_unexcept_0:
    self->state = STATE_TRANSVR_UNEXCEPTED;
    if (_is_except_happened_4_pmode(self, d_state) &&
        (self->mode == TRANSVR_MODE_POLLING) ){
        SWPS_INFO("%s: %s <port>:%s\n", __func__, emsg, self->swp_name);
        SWPS_INFO("Dump: <os>:%d <ot>:%d <ns>:%d <nt>:%d\n",
                   self->state, self->type, d_state, d_type);
    }
    return EVENT_TRANSVR_INIT_FAIL;

initer_err_case_alarm_to_user:
    SWPS_DEBUG("%s: %s <port>:%s <case>:alarm_to_user\n",
               __func__, emsg, self->swp_name);
    self->state = STATE_TRANSVR_UNEXCEPTED;
    alarm_msg_2_user(self, "detected transceiver/cables not meet SFF standard");
    return EVENT_TRANSVR_INIT_FAIL;
}


/* ========== Object functions for Clean procedure ==========
 */
int
_transvr_clean_handler(struct transvr_obj_s *self){

    int retval = DEBUG_TRANSVR_INT_VAL;

    if (!self->clean) {
        SWPS_ERR("%s: %s clean() is NULL.\n",
                __func__, self->swp_name);
        return EVENT_TRANSVR_TASK_FAIL;
    }
    retval = self->clean(self);
    if (retval != EVENT_TRANSVR_TASK_DONE){
        SWPS_ERR("%s: %s clean() fail. [ERR]:%d\n",
                __func__, self->swp_name, retval);
        return retval;
    }
    return EVENT_TRANSVR_TASK_DONE;
}


int
common_transvr_clean(struct transvr_obj_s *self){

    transvr_task_free_all(self);
    transvr_cache_free_all(self);
    return EVENT_TRANSVR_TASK_DONE;
}


int
qsfp_transvr_clean(struct transvr_obj_s *self){

    int retval;
    int lpower_config = 1;

    retval = _taskfunc_qsfp_setup_power_mod(self, lpower_config);
    if (retval < 0){
        SWPS_ERR("%s: Set lpmod fail! <err>:%d\n",
                __func__, retval);
        return retval;
    }
    retval = common_transvr_clean(self);
    if (retval < 0){
        SWPS_ERR("%s: common_transvr_clean fail! <err>:%d\n",
                __func__, retval);
        return retval;
    }
    return EVENT_TRANSVR_TASK_DONE;
}


int
fake_transvr_clean(struct transvr_obj_s *self){

    return EVENT_TRANSVR_TASK_DONE;
}


/* ========== Object functions for check and update ==========
 */
int
common_transvr_check(struct transvr_obj_s *self){

    char fun_str[32]  = "common_transvr_check";

    if (self->mode != TRANSVR_MODE_POLLING) {
        SWPS_ERR("%s: mode:%d is not TRANSVR_MODE_POLLING\n",
                fun_str, self->mode);
        return ERR_TRANSVR_UNEXCPT;
    }
    /* Trigger delay task */
    transvr_task_run_all(self);
    /* Trigger state machine to check and update */
    return self->fsm_4_polling(self, fun_str);
}


int
fake_transvr_check(struct transvr_obj_s *self){
    return 0;
}


/* ========== Functions for Factory pattern ==========
 */
static int
setup_transvr_public_cb(struct transvr_obj_s *self,
                        int transvr_type){
    switch (transvr_type){
        case TRANSVR_TYPE_SFP:
            self->get_id              = common_get_id;
            self->get_ext_id          = common_get_ext_id;
            self->get_connector       = common_get_connector;
            self->get_vendor_name     = common_get_vendor_name;
            self->get_vendor_pn       = common_get_vendor_pn;
            self->get_vendor_rev      = common_get_vendor_rev;
            self->get_vendor_sn       = common_get_vendor_sn;
            self->get_power_cls       = unsupported_get_func;
            self->get_br              = common_get_br;
            self->get_len_sm          = sfp_get_len_sm;
            self->get_len_smf         = common_get_len_smf;
            self->get_len_om1         = common_get_len_om1;
            self->get_len_om2         = common_get_len_om2;
            self->get_len_om3         = common_get_len_om3;
            self->get_len_om4         = common_get_len_om4;
            self->get_comp_rev        = common_get_comp_rev;
            self->get_comp_eth_1      = sfp_get_comp_eth_1;
            self->get_comp_eth_10     = sfp_get_comp_eth_10;
            self->get_comp_eth_10_40  = unsupported_get_func;
            self->get_comp_extend     = common_get_comp_extended;
            self->get_cdr             = unsupported_get_func;
            self->get_rate_id         = sfp_get_rate_id;
            self->get_soft_rs0        = sfp_get_soft_rs0;
            self->get_soft_rs1        = sfp_get_soft_rs1;
            self->get_info            = common_get_info;
            self->get_if_type         = sfp_get_if_type;
            self->get_if_speed        = sfp_get_if_speed;
            self->get_if_lane         = common_get_if_lane;
            self->get_curr_temp       = sfp_get_transvr_temp;
            self->get_curr_vol        = sfp_get_transvr_voltage;
            self->get_soft_rx_los     = unsupported_get_func2;
            self->get_soft_tx_disable = unsupported_get_func2;
            self->get_soft_tx_fault   = unsupported_get_func2;
            self->get_auto_tx_disable = unsupported_get_func2;
            self->get_tx_bias         = sfp_get_transvr_tx_bias;
            self->get_tx_power        = sfp_get_transvr_tx_power;
            self->get_rx_power        = sfp_get_transvr_rx_power;
            self->get_tx_eq           = sfp_get_transvr_tx_eq;
            self->get_rx_am           = unsupported_get_func2;
            self->get_rx_em           = sfp_get_transvr_rx_em;
            self->get_wavelength      = sfp_get_wavelength;
            self->get_extphy_offset   = sfp_get_1g_rj45_extphy_offset;
            self->get_extphy_reg      = sfp_get_1g_rj45_extphy_reg;
            self->set_cdr             = unsupported_set_func;
            self->set_soft_rs0        = sfp_set_soft_rs0;
            self->set_soft_rs1        = sfp_set_soft_rs1;
            self->set_soft_tx_disable = unsupported_set_func;
            self->set_auto_tx_disable = unsupported_set_func;
            self->set_tx_eq           = sfp_set_tx_eq;
            self->set_rx_am           = unsupported_set_func;
            self->set_rx_em           = sfp_set_rx_em;
            self->set_extphy_offset   = sfp_set_1g_rj45_extphy_offset;
            self->set_extphy_reg      = sfp_set_1g_rj45_extphy_reg;
            return 0;

        case TRANSVR_TYPE_QSFP:
        case TRANSVR_TYPE_QSFP_PLUS:
            self->get_id              = common_get_id;
            self->get_ext_id          = common_get_ext_id;
            self->get_connector       = common_get_connector;
            self->get_vendor_name     = common_get_vendor_name;
            self->get_vendor_pn       = common_get_vendor_pn;
            self->get_vendor_rev      = common_get_vendor_rev;
            self->get_vendor_sn       = common_get_vendor_sn;
            self->get_power_cls       = qsfp_get_power_cls;
            self->get_br              = common_get_br;
            self->get_len_sm          = unsupported_get_func;
            self->get_len_smf         = common_get_len_smf;
            self->get_len_om1         = common_get_len_om1;
            self->get_len_om2         = common_get_len_om2;
            self->get_len_om3         = common_get_len_om3;
            self->get_len_om4         = common_get_len_om4;
            self->get_comp_rev        = common_get_comp_rev;
            self->get_comp_eth_1      = qsfp_get_comp_eth;
            self->get_comp_eth_10     = unsupported_get_func;
            self->get_comp_eth_10_40  = qsfp_get_comp_10_40;
            self->get_comp_extend     = common_get_comp_extended;
            self->get_cdr             = unsupported_get_func;
            self->get_rate_id         = unsupported_get_func;
            self->get_soft_rs0        = unsupported_get_func; /* TBD */
            self->get_soft_rs1        = unsupported_get_func; /* TBD */
            self->get_info            = common_get_info;
            self->get_if_type         = qsfp_get_if_type;
            self->get_if_speed        = qsfp_get_if_speed;
            self->get_if_lane         = common_get_if_lane;
            self->get_curr_temp       = qsfp_get_transvr_temp;
            self->get_curr_vol        = qsfp_get_transvr_voltage;
            self->get_soft_rx_los     = qsfp_get_soft_rx_los;
            self->get_soft_tx_disable = qsfp_get_soft_tx_disable;
            self->get_soft_tx_fault   = qsfp_get_soft_tx_fault;
            self->get_auto_tx_disable = qsfp_get_auto_tx_disable;
            self->get_tx_bias         = qsfp_get_transvr_tx_bias;
            self->get_tx_power        = qsfp_get_transvr_tx_power;
            self->get_rx_power        = qsfp_get_transvr_rx_power;
            self->get_tx_eq           = unsupported_get_func2;
            self->get_rx_am           = unsupported_get_func2;
            self->get_rx_em           = unsupported_get_func2;
            self->get_wavelength      = qsfp_get_wavelength;
            self->get_extphy_offset   = unsupported_get_func2;
            self->get_extphy_reg      = unsupported_get_func2;
            self->set_cdr             = unsupported_set_func;
            self->set_soft_rs0        = unsupported_set_func; /* TBD */
            self->set_soft_rs1        = unsupported_set_func; /* TBD */
            self->set_soft_tx_disable = qsfp_set_soft_tx_disable;
            self->set_auto_tx_disable = qsfp_set_auto_tx_disable;
            self->set_tx_eq           = unsupported_set_func;
            self->set_rx_am           = unsupported_set_func;
            self->set_rx_em           = unsupported_set_func;
            self->set_extphy_offset   = unsupported_set_func;
            self->set_extphy_reg      = unsupported_set_func;
            return 0;

        case TRANSVR_TYPE_QSFP_28:
            self->get_id              = common_get_id;
            self->get_ext_id          = common_get_ext_id;
            self->get_connector       = common_get_connector;
            self->get_vendor_name     = common_get_vendor_name;
            self->get_vendor_pn       = common_get_vendor_pn;
            self->get_vendor_rev      = common_get_vendor_rev;
            self->get_vendor_sn       = common_get_vendor_sn;
            self->get_power_cls       = qsfp_get_power_cls;
            self->get_br              = common_get_br;
            self->get_len_sm          = unsupported_get_func;
            self->get_len_smf         = common_get_len_smf;
            self->get_len_om1         = common_get_len_om1;
            self->get_len_om2         = common_get_len_om2;
            self->get_len_om3         = common_get_len_om3;
            self->get_len_om4         = common_get_len_om4;
            self->get_comp_rev        = common_get_comp_rev;
            self->get_comp_eth_1      = qsfp_get_comp_eth;
            self->get_comp_eth_10     = unsupported_get_func;
            self->get_comp_eth_10_40  = qsfp_get_comp_10_40;
            self->get_comp_extend     = common_get_comp_extended;
            self->get_cdr             = qsfp_get_cdr;
            self->get_rate_id         = unsupported_get_func;
            self->get_soft_rs0        = unsupported_get_func; /* TBD */
            self->get_soft_rs1        = unsupported_get_func; /* TBD */
            self->get_info            = common_get_info;
            self->get_if_type         = qsfp_get_if_type;
            self->get_if_speed        = qsfp_get_if_speed;
            self->get_if_lane         = common_get_if_lane;
            self->get_curr_temp       = qsfp_get_transvr_temp;
            self->get_curr_vol        = qsfp_get_transvr_voltage;
            self->get_soft_rx_los     = qsfp_get_soft_rx_los;
            self->get_soft_tx_disable = qsfp_get_soft_tx_disable;
            self->get_soft_tx_fault   = qsfp_get_soft_tx_fault;
            self->get_auto_tx_disable = qsfp_get_auto_tx_disable;
            self->get_tx_bias         = qsfp_get_transvr_tx_bias;
            self->get_tx_power        = qsfp_get_transvr_tx_power;
            self->get_rx_power        = qsfp_get_transvr_rx_power;
            self->get_tx_eq           = qsfp_get_transvr_tx_eq;
            self->get_rx_am           = qsfp_get_transvr_rx_am;
            self->get_rx_em           = qsfp_get_transvr_rx_em;
            self->get_wavelength      = qsfp_get_wavelength;
            self->get_extphy_offset   = unsupported_get_func2;
            self->get_extphy_reg      = unsupported_get_func2;
            self->set_cdr             = qsfp_set_cdr;
            self->set_soft_rs0        = unsupported_set_func; /* TBD */
            self->set_soft_rs1        = unsupported_set_func; /* TBD */
            self->set_soft_tx_disable = qsfp_set_soft_tx_disable;
            self->set_auto_tx_disable = qsfp_set_auto_tx_disable;
            self->set_tx_eq           = qsfp_set_tx_eq;
            self->set_rx_am           = qsfp_set_rx_am;
            self->set_rx_em           = qsfp_set_rx_em;
            self->set_extphy_offset   = unsupported_set_func;
            self->set_extphy_reg      = unsupported_set_func;
            return 0;

        case TRANSVR_TYPE_FAKE:
            self->get_id              = fake_get_hex;
            self->get_ext_id          = fake_get_hex;
            self->get_connector       = fake_get_hex;
            self->get_vendor_name     = fake_get_str;
            self->get_vendor_pn       = fake_get_str;
            self->get_vendor_rev      = fake_get_str;
            self->get_vendor_sn       = fake_get_str;
            self->get_power_cls       = fake_get_int;
            self->get_br              = fake_get_hex;
            self->get_len_sm          = fake_get_int;
            self->get_len_smf         = fake_get_int;
            self->get_len_om1         = fake_get_int;
            self->get_len_om2         = fake_get_int;
            self->get_len_om3         = fake_get_int;
            self->get_len_om4         = fake_get_int;
            self->get_comp_rev        = fake_get_hex;
            self->get_comp_eth_1      = fake_get_hex;
            self->get_comp_eth_10     = fake_get_hex;
            self->get_comp_eth_10_40  = fake_get_hex;
            self->get_comp_extend     = fake_get_hex;
            self->get_cdr             = fake_get_hex;
            self->get_rate_id         = fake_get_hex;
            self->get_soft_rs0        = fake_get_binary;
            self->get_soft_rs1        = fake_get_binary;
            self->get_info            = fake_get_int;
            self->get_if_type         = fake_get_str;
            self->get_if_speed        = fake_get_str;
            self->get_if_lane         = fake_get_str;
            self->get_curr_temp       = fake_get_str;
            self->get_curr_vol        = fake_get_str;
            self->get_soft_rx_los     = fake_get_str;
            self->get_soft_tx_disable = fake_get_str;
            self->get_soft_tx_fault   = fake_get_str;
            self->get_auto_tx_disable = fake_get_str;
            self->get_tx_bias         = fake_get_str;
            self->get_tx_power        = fake_get_str;
            self->get_rx_power        = fake_get_str;
            self->get_tx_eq           = fake_get_str;
            self->get_rx_am           = fake_get_str;
            self->get_rx_em           = fake_get_str;
            self->get_wavelength      = fake_get_str;
            self->get_extphy_offset   = fake_get_str;
            self->get_extphy_reg      = fake_get_str;
            self->set_cdr             = fake_set_hex;
            self->set_soft_rs0        = fake_set_int;
            self->set_soft_rs1        = fake_set_int;
            self->set_soft_tx_disable = fake_set_int;
            self->set_auto_tx_disable = fake_set_int;
            self->set_tx_eq           = fake_set_int;
            self->set_rx_am           = fake_set_int;
            self->set_rx_em           = fake_set_int;
            self->set_extphy_offset   = fake_set_hex;
            self->set_extphy_reg      = fake_set_hex;
            return 0;

        default:
            break;
    }
    SWPS_WARN("%s: Detect non-defined type:%d\n", __func__, transvr_type);
    return ERR_TRANSVR_UNEXCPT;
}


static int
setup_transvr_private_cb(struct transvr_obj_s *self,
                         int transvr_type){
    switch (transvr_type){
        case TRANSVR_TYPE_SFP:
            self->init  = transvr_init_sfp;
            self->clean = common_transvr_clean;
            self->check = common_transvr_check;
            self->update_all = _sfp_update_attr_all;
            self->fsm_4_direct = common_fsm_4_direct_mode;
            self->fsm_4_polling = common_fsm_4_polling_mode;
            self->send_uevent = sfp_send_uevent;
            self->dump_all = sfp_transvr_dump;
            return 0;

        case TRANSVR_TYPE_QSFP:
        case TRANSVR_TYPE_QSFP_PLUS:
            self->init  = transvr_init_qsfp;
            self->clean = qsfp_transvr_clean;
            self->check = common_transvr_check;
            self->update_all = _qsfp_update_attr_all;
            self->fsm_4_direct = common_fsm_4_direct_mode;
            self->fsm_4_polling = common_fsm_4_polling_mode;
            self->send_uevent = qsfp_send_uevent;
            self->dump_all = qsfp_transvr_dump;
            return 0;

        case TRANSVR_TYPE_QSFP_28:
            self->init  = transvr_init_qsfp28;
            self->clean = qsfp_transvr_clean;
            self->check = common_transvr_check;
            self->update_all = _qsfp_update_attr_all;
            self->fsm_4_direct = common_fsm_4_direct_mode;
            self->fsm_4_polling = common_fsm_4_polling_mode;
            self->send_uevent = qsfp_send_uevent;
            self->dump_all = qsfp_transvr_dump;
            return 0;

        case TRANSVR_TYPE_FAKE:
            self->init  = transvr_init_fake;
            self->clean = fake_transvr_clean;
            self->check = fake_transvr_check;
            self->update_all = fake_transvr_update;
            self->fsm_4_direct = fake_fsm_4_direct_mode;
            self->fsm_4_polling = fake_fsm_4_polling_mode;
            self->send_uevent = fake_send_uevent;
            self->dump_all = fake_transvr_dump;
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
        case TRANSVR_TYPE_SFP:
            return &eeprom_map_sfp;
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
        case TRANSVR_MODE_POLLING: /* Polling mode, read from cache */
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
setup_transvr_dsize_attr(struct transvr_obj_s *self){

    char *emsg = DEBUG_TRANSVR_STR_VAL;

    self->vendor_name = kzalloc((LEN_TRANSVR_M_STR * sizeof(char)), GFP_KERNEL);
    if (!self->vendor_name){
        emsg = "vendor_name";
        goto err_setup_d_attr;
    }
    self->vendor_pn = kzalloc((LEN_TRANSVR_M_STR * sizeof(char)), GFP_KERNEL);
    if (!self->vendor_pn){
        emsg = "vendor_pn";
        goto err_setup_d_attr;
    }
    self->vendor_rev = kzalloc((LEN_TRANSVR_M_STR * sizeof(char)), GFP_KERNEL);
    if (!self->vendor_rev){
        emsg = "vendor_rev";
        goto err_setup_d_attr;
    }
    self->vendor_sn = kzalloc((LEN_TRANSVR_M_STR * sizeof(char)), GFP_KERNEL);
    if (!self->vendor_sn){
        emsg = "vendor_sn";
        goto err_setup_d_attr;
    }
    self->worker_p = NULL;
    return 0;

err_setup_d_attr:
    SWPS_ERR("%s: %s kzalloc fail!", __func__, emsg);
    return ERR_TRANSVR_UNEXCPT;
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
    /* Prepare dynamic size attributes */
    if (setup_transvr_dsize_attr(result_p) < 0){
        goto err_create_transvr_dattr_fail;
    }
    /* Prepare call back functions of object */
    if (setup_transvr_public_cb(result_p, transvr_type) < 0){
        goto err_create_transvr_dattr_fail;
    }
    /* Prepare call back functions of object */
    if (setup_transvr_private_cb(result_p, transvr_type) < 0){
        goto err_create_transvr_dattr_fail;
    }
    /* Prepare i2c client object */
    if (setup_i2c_client(result_p) < 0){
        goto err_create_transvr_dattr_fail;
    }
    return result_p;

err_create_transvr_dattr_fail:
    kfree(result_p->vendor_sn);
    kfree(result_p->vendor_rev);
    kfree(result_p->vendor_pn);
    kfree(result_p->vendor_name);
err_create_transvr_sattr_fail:
    kfree(result_p);
err_create_transvr_fail:
    SWPS_ERR("%s: %s <chan>:%d <voff>:%d <type>:%d\n",
            __func__, err_msg, chan_id, ioexp_virt_offset, transvr_type);
    return NULL;
}
EXPORT_SYMBOL(create_transvr_obj);


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
    /* Replace call back functions */
    if (setup_transvr_public_cb(self, new_type) < 0){
        goto err_private_reload_func_3;
    }
    if (setup_transvr_private_cb(self, new_type) < 0){
        goto err_private_reload_func_3;
    }
    if(old_i2c_p){
        i2c_put_adapter(old_i2c_p->adapter);
    }
    kfree(old_i2c_p);
    return 0;

err_private_reload_func_3:
    SWPS_INFO("%s: init() fail!\n", __func__);
    if(old_i2c_p){
        i2c_put_adapter(old_i2c_p->adapter);
    }
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


int
isolate_transvr_obj(struct transvr_obj_s *self) {

    self->state = STATE_TRANSVR_ISOLATED;
    SWPS_INFO("%s: %s be isolated\n", __func__, self->swp_name);
    return 0;
}
EXPORT_SYMBOL(isolate_transvr_obj);


int
resync_channel_tier_2(struct transvr_obj_s *self) {

    int val = TRANSVR_TYPE_ERROR;

    if (self->state == STATE_TRANSVR_ISOLATED) {
        return 0;
    }
    self->i2c_client_p->addr = VAL_TRANSVR_COMID_ARREESS;
    val = i2c_smbus_read_byte_data(self->i2c_client_p,
                                   VAL_TRANSVR_COMID_OFFSET);
    if (val < 0) {
        return -1;
    }
    return 0;
}
EXPORT_SYMBOL(resync_channel_tier_2);

/* For build single module using (Ex: ONL platform) */
MODULE_LICENSE("GPL");


/* -----------------------------------------
 *   ToDo List
 * -----------------------------------------
 * 1. _sfp_detect_class_by_feature()
 *    => Need check ACC use case.
 * 2. _sfp_detect_class_by_1g_ethernet()
 *    => Need check 0.1G use case.
 * 3. Loopback transceiver use case.
 *    => Less much data
 * 4. _qsfp_detect_class_by_extend_comp()
 *    => Verify 100G CWDM4
 *    => Verify Obsolete (assigned before 100G CWDM4 MSA required FEC)
 *    => Verify 100G CLR4
 *    => Verify 100GE-DWDM2
 *    => Verify 40G PSM4 Parallel SMF
 *    => Verify 100G ACC (Active Copper Cable) or 25GAUI C2M ACC.
 *    => Verify 100G ACC or 25GAUI C2M ACC.
 *    => Verify 25GBASE-LR
 *    => Verify 40G Active Cable (XLPPI)
 */








