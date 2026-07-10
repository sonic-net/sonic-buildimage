#include <linux/slab.h>
#include <linux/i2c.h>
#include "io_expander.h"

static struct ioexp_obj_s *ioexp_head_p = NULL;
static struct ioexp_obj_s *ioexp_tail_p = NULL;

struct ioexp_map_s ioexp_map_cypress_nabc = {

    .chip_amount = 3,
    .data_width  = 2,

    .map_present    = { {0, 0, 4}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {0, 0, 5}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {0, 0, 6}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {0, 0, 7}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {1, 0, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {1, 0, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
                        {1, 0, 6}, /* map_present[6] = MOD_ABS_PORT(X+6) */
                        {1, 0, 7}, /* map_present[7] = MOD_ABS_PORT(X+7) */
    },
    .map_tx_disable = { {0, 1, 0}, /* map_tx_disable[0] = TXDISABLE_SFP+_P(X)   */
                        {0, 1, 1}, /* map_tx_disable[1] = TXDISABLE_SFP+_P(X+1) */
                        {0, 1, 2}, /* map_tx_disable[2] = TXDISABLE_SFP+_P(X+2) */
                        {0, 1, 3}, /* map_tx_disable[3] = TXDISABLE_SFP+_P(X+3) */
                        {1, 1, 0}, /* map_tx_disable[4] = TXDISABLE_SFP+_P(X+4) */
                        {1, 1, 1}, /* map_tx_disable[5] = TXDISABLE_SFP+_P(X+5) */
                        {1, 1, 2}, /* map_tx_disable[6] = TXDISABLE_SFP+_P(X+6) */
                        {1, 1, 3}, /* map_tx_disable[7] = TXDISABLE_SFP+_P(X+7) */
    },
    .map_tx_fault   = { {0, 0, 0}, /* map_tx_fault[0] = TXFAULT_SFP+_P(X)   */
                        {0, 0, 1}, /* map_tx_fault[1] = TXFAULT_SFP+_P(X+1) */
                        {0, 0, 2}, /* map_tx_fault[2] = TXFAULT_SFP+_P(X+2) */
                        {0, 0, 3}, /* map_tx_fault[3] = TXFAULT_SFP+_P(X+3) */
                        {1, 0, 0}, /* map_tx_fault[4] = TXFAULT_SFP+_P(X+4) */
                        {1, 0, 1}, /* map_tx_fault[5] = TXFAULT_SFP+_P(X+5) */
                        {1, 0, 2}, /* map_tx_fault[6] = TXFAULT_SFP+_P(X+6) */
                        {1, 0, 3}, /* map_tx_fault[7] = TXFAULT_SFP+_P(X+7) */
    },
    .map_rxlos      = { {0, 1, 4}, /* map_rxlos[0] = OPRXLOS_PORT(X)   */
                        {0, 1, 5}, /* map_rxlos[1] = OPRXLOS_PORT(X+1) */
                        {0, 1, 6}, /* map_rxlos[2] = OPRXLOS_PORT(X+2) */
                        {0, 1, 7}, /* map_rxlos[3] = OPRXLOS_PORT(X+3) */
                        {1, 1, 4}, /* map_rxlos[4] = OPRXLOS_PORT(X+4) */
                        {1, 1, 5}, /* map_rxlos[5] = OPRXLOS_PORT(X+5) */
                        {1, 1, 6}, /* map_rxlos[6] = OPRXLOS_PORT(X+6) */
                        {1, 1, 7}, /* map_rxlos[7] = OPRXLOS_PORT(X+7) */
    },
};

struct ioexp_map_s ioexp_map_cypress_7abc = {

    .chip_amount    = 3,
    .data_width     = 2,

    .map_present    = { {2, 0, 0}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {2, 0, 1}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {2, 0, 2}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {2, 0, 3}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {2, 0, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {2, 0, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
    },
    .map_reset      = { {0, 0, 0}, /* map_reset[0] = QRESET_QSFP_N_P(X)   */
                        {0, 0, 1}, /* map_reset[1] = QRESET_QSFP_N_P(X+1) */
                        {0, 0, 2}, /* map_reset[2] = QRESET_QSFP_N_P(X+2) */
                        {0, 0, 3}, /* map_reset[3] = QRESET_QSFP_N_P(X+3) */
                        {0, 0, 4}, /* map_reset[4] = QRESET_QSFP_N_P(X+4) */
                        {0, 0, 5}, /* map_reset[5] = QRESET_QSFP_N_P(X+5) */
    },
    .map_lpmod      = { {0, 1, 0}, /* map_lpmod[0] = LPMODE_QSFP_P(X)   */
                        {0, 1, 1}, /* map_lpmod[1] = LPMODE_QSFP_P(X+1) */
                        {0, 1, 2}, /* map_lpmod[2] = LPMODE_QSFP_P(X+2) */
                        {0, 1, 3}, /* map_lpmod[3] = LPMODE_QSFP_P(X+3) */
                        {0, 1, 4}, /* map_lpmod[4] = LPMODE_QSFP_P(X+4) */
                        {0, 1, 5}, /* map_lpmod[5] = LPMODE_QSFP_P(X+5) */
    },
    .map_modsel     = { {1, 1, 0}, /* map_modsel[0] = MODSEL_QSFP_N_P(X)   */
                        {1, 1, 1}, /* map_modsel[1] = MODSEL_QSFP_N_P(X+1) */
                        {1, 1, 2}, /* map_modsel[2] = MODSEL_QSFP_N_P(X+2) */
                        {1, 1, 3}, /* map_modsel[3] = MODSEL_QSFP_N_P(X+3) */
                        {1, 1, 4}, /* map_modsel[4] = MODSEL_QSFP_N_P(X+4) */
                        {1, 1, 5}, /* map_modsel[5] = MODSEL_QSFP_N_P(X+5) */
    },
};



/* ========== Private functions ==========
 */
int check_channel_tier_1(void);

struct i2c_client *
_get_i2c_client(struct ioexp_obj_s *self,
                int chip_id){

    struct ioexp_i2c_s *i2c_curr_p = self->i2c_head_p;

    if (!(i2c_curr_p)){
        SWPS_ERR("%s: i2c_curr_p is NULL\n", __func__);
        return NULL;
    }
    while (i2c_curr_p){
        if ((i2c_curr_p->chip_id) == chip_id){
            return i2c_curr_p->i2c_client_p;
        }
        i2c_curr_p = i2c_curr_p->next;
    }
    SWPS_ERR("%s: not exist! <chip_id>:%d\n", __func__, chip_id);
    return NULL;
}


static int
_common_ioexp_update_one(struct ioexp_obj_s *self,
                         struct ioexp_addr_s *ioexp_addr,
                         int chip_id,
                         int data_width,
                         int show_err,
                         char *caller_name) {
    int buf      = 0;
    int err      = 0;
    int data_id  = 0;
    int r_offset = 0;

    for(data_id=0; data_id<data_width; data_id++){
        /* Read from IOEXP */
        r_offset = ioexp_addr->read_offset[data_id];
        buf = i2c_smbus_read_byte_data(_get_i2c_client(self, chip_id), r_offset);
        /* Check error */
        if (buf < 0) {
            err = 1;
            if (show_err) {
                SWPS_INFO("IOEXP-%d read fail! <err>:%d \n", self->ioexp_id, buf);
                SWPS_INFO("Dump: <chan>:%d <addr>:0x%02x <offset>:%d, <caller>:%s\n",
                          ioexp_addr->chan_id, ioexp_addr->chip_addr,
                          ioexp_addr->read_offset[data_id], caller_name);
            }
            continue;
        }
        /* Update IOEXP object */
        self->chip_data[chip_id].data[data_id] = (uint8_t)buf;
    }
    if (err) {
        return ERR_IOEXP_UNEXCPT;
    }
    return 0;
}


static int
common_ioexp_update_all(struct ioexp_obj_s *self,
                        int show_err,
                        char *caller_name){

    int err     = 0;
    int chip_id = 0;
    int chip_amount = self->ioexp_map_p->chip_amount;

    for (chip_id=0; chip_id<chip_amount; chip_id++){
        if (_common_ioexp_update_one(self,
                                     &(self->ioexp_map_p->map_addr[chip_id]),
                                     chip_id,
                                     self->ioexp_map_p->data_width,
                                     show_err,
                                     caller_name) < 0) {
            err = 1;
        }
    }
    if (err) {
        return ERR_IOEXP_UNEXCPT;
    }
    return 0;
}

static int
_common_get_bit(struct ioexp_obj_s *self,
                struct ioexp_bitmap_s *bitmap_obj_p,
                char *func_mane){
    uint8_t buf;
    int err_code;

    /* Get address */
    err_code = self->fsm_4_direct(self);
    if (err_code < 0){
            return err_code;
        }

    if (!bitmap_obj_p){
        SWPS_ERR("Layout config incorrect! <ioexp_id>:%d <func>:%s\n",
                 self->ioexp_id, func_mane);
        return ERR_IOEXP_BADCONF;
    }
    /* Get data form cache */
    buf = self->chip_data[bitmap_obj_p->chip_id].data[bitmap_obj_p->ioexp_voffset];
    return (int)(buf >> bitmap_obj_p->bit_shift & 0x01);
}


static int
_common_set_bit(struct ioexp_obj_s *self,
                struct ioexp_bitmap_s *bitmap_obj_p,
                int input_val,
                char *func_mane){
    int err_code, target_offset;
    uint8_t origin_byte;
    uint8_t modify_byte;

    /* Get address */
    err_code = self->fsm_4_direct(self);
    if (err_code < 0){
            return err_code;
        }
    if (!bitmap_obj_p){
        SWPS_ERR("Layout config incorrect! <ioexp>:%d <func>:%s\n",
                 self->ioexp_id, func_mane);
        return ERR_IOEXP_BADCONF;
    }
    /* Prepare write date */
    origin_byte = self->chip_data[bitmap_obj_p->chip_id].data[bitmap_obj_p->ioexp_voffset];
    switch (input_val) {
        case 0:
            modify_byte = origin_byte;
            SWP_BIT_CLEAR(modify_byte, bitmap_obj_p->bit_shift);
            break;
        case 1:
            modify_byte = origin_byte;
            SWP_BIT_SET(modify_byte, bitmap_obj_p->bit_shift);
            break;
        default:
            SWPS_ERR("Input value incorrect! <val>:%d <ioexp>:%d <func>:%s\n",
                     input_val, self->ioexp_id, func_mane);
            return ERR_IOEXP_BADINPUT;
    }
    /* Setup i2c client */
    target_offset = self->ioexp_map_p->map_addr[bitmap_obj_p->chip_id].write_offset[bitmap_obj_p->ioexp_voffset];
    /* Write byte to chip via I2C */
    err_code = i2c_smbus_write_byte_data(_get_i2c_client(self, bitmap_obj_p->chip_id),
                                         target_offset,
                                         modify_byte);
    /* Update or bollback object */
    if (err_code < 0){
        self->chip_data[bitmap_obj_p->chip_id].data[bitmap_obj_p->ioexp_voffset] = origin_byte;
        SWPS_ERR("I2C write fail! <input>:%d <ioexp>:%d <func>:%s <err>:%d\n",
                 input_val, self->ioexp_id, func_mane, err_code);
        return err_code;
    }
    self->chip_data[bitmap_obj_p->chip_id].data[bitmap_obj_p->ioexp_voffset] = modify_byte;
    return 0;
}


/* ========== Object public functions ==========
 */
int
common_get_present(struct ioexp_obj_s *self,
                   int virt_offset){

    int UNPLUG = 1;
    int retval = ERR_IOEXP_UNEXCPT;

    retval = _common_get_bit(self,
                             &(self->ioexp_map_p->map_present[virt_offset]),
                             "common_get_present");
    if (retval < 0) {
        /* [Note]
         * => Transceiver object does not need to handle IOEXP layer issues.
         */
        return UNPLUG;
    }
    return retval;
}

int
common_get_tx_fault(struct ioexp_obj_s *self,
                    int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_tx_fault[virt_offset]),
                           "common_get_tx_fault");
}

int
common_get_rxlos(struct ioexp_obj_s *self,
                 int virt_offset){
    /* [Receiver Loss of Signal (Rx_LOS)]
     * The post-amplification IC also includes transition detection circuitry
     * which monitors the ac level of incoming optical signals and provides a
     * TTL/CMOS compatible status signal to the host (pin 8). An adequate optical
     * input results in a low Rx_LOS output while a high Rx_LOS output indicates
     * an unusable optical input. The Rx_LOS thresholds are factory set so that
     * a high output indicates a definite optical fault has occurred. Rx_LOS can
     * also be monitored via the two-wire serial interface
     * (address A2h, byte 110, bit 1).
     *
     * 0: Normal
     * 1: Abnormal
     */
    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_rxlos[virt_offset]),
                           "common_get_rxlos");
}


int
common_get_tx_disable(struct ioexp_obj_s *self,
                      int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_tx_disable[virt_offset]),
                           "common_get_tx_disable");
}

int
common_get_reset(struct ioexp_obj_s *self,
                 int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_reset[virt_offset]),
                           "common_get_reset");
}


int
common_get_lpmod(struct ioexp_obj_s *self,
                 int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_lpmod[virt_offset]),
                           "common_get_lpmod");
}


int
common_get_modsel(struct ioexp_obj_s *self,
                  int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_modsel[virt_offset]),
                           "common_get_modsel");
}

int
common_set_tx_disable(struct ioexp_obj_s *self,
                      int virt_offset,
                      int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_tx_disable[virt_offset]),
                           input_val,
                           "common_set_tx_disable");
}

int
common_set_reset(struct ioexp_obj_s *self,
                 int virt_offset,
                 int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_reset[virt_offset]),
                           input_val,
                           "common_set_reset");
}


int
common_set_lpmod(struct ioexp_obj_s *self,
                 int virt_offset,
                 int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_lpmod[virt_offset]),
                           input_val,
                           "common_set_lpmod");
}


int
common_set_modsel(struct ioexp_obj_s *self,
                  int virt_offset,
                  int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_modsel[virt_offset]),
                           input_val,
                           "common_set_modsel");
}

int
ioexp_get_not_support(struct ioexp_obj_s *self,
                      int virt_offset){
    return ERR_IOEXP_NOTSUPPORT;
}


int
ioexp_set_not_support(struct ioexp_obj_s *self,
                      int virt_offset,
                      int input_val){
    return ERR_IOEXP_NOTSUPPORT;
}

/* ========== Initial functions for IO Expander ==========
 */
int
common_ioexp_init(struct ioexp_obj_s *self) {

    int chip_id, offset, err_code;
    struct ioexp_addr_s *addr_p;

    if (self->mode == IOEXP_MODE_DIRECT) { ///important
        goto update_common_ioexp_init;
    }
    /* Setup default value to each physical IO Expander */
    for (chip_id=0; chip_id<(self->ioexp_map_p->chip_amount); chip_id++){
        /* Get address mapping */
        addr_p = &(self->ioexp_map_p->map_addr[chip_id]);
        if (!addr_p){
            SWPS_ERR("%s: IOEXP config incorrect! <chip_id>:%d \n",
                     __func__, chip_id);
            return -1;
        }
        /* Setup default value */
        for (offset=0; offset<(self->ioexp_map_p->data_width); offset++){
            err_code = i2c_smbus_write_byte_data(_get_i2c_client(self, chip_id),
                                                 addr_p->write_offset[offset],
                                                 addr_p->data_default[offset]);
            if (err_code < 0){
                SWPS_ERR("%s: set default fail! <error>:%d \n",
                         __func__, err_code);
                return ERR_IOEXP_UNEXCPT;
            }
        }
    }

update_common_ioexp_init:
    /* Check and update info to object */
    err_code = self->update_all(self, 1, "common_ioexp_init");
    if (err_code < 0) {
        SWPS_ERR("%s: update_all() fail! <error>:%d \n",
                __func__, err_code);
        return ERR_IOEXP_UNEXCPT;
    }
    return 0;
}


/* ========== Object functions for Final State Machine ==========
 */
int
_is_channel_ready(struct ioexp_obj_s *self){

    int buf     = 0;
    int chip_id = 0;  /* Use first chip which be registered */
    int data_id = 0;  /* Use first byte which be registered */
    struct ioexp_addr_s *ioexp_addr = NULL;

    ioexp_addr = &(self->ioexp_map_p->map_addr[chip_id]);
    if (!ioexp_addr){
        SWPS_ERR("%s: config incorrect!\n", __func__);
        return ERR_IOEXP_UNEXCPT;
    }
    buf = i2c_smbus_read_byte_data(_get_i2c_client(self, chip_id),
                                   ioexp_addr->read_offset[data_id]);
    if (buf >= 0){
        return 1;
    }
    return 0;
}

int
_ioexp_init_handler(struct ioexp_obj_s *self){

    int return_val;

    switch (self->mode) {
        case IOEXP_MODE_DIRECT:
            return_val = self->init(self);
            if (return_val < 0){
                self->state = STATE_IOEXP_ABNORMAL;
            } else {
                self->state = STATE_IOEXP_NORMAL;
            }
            return return_val;
        default:
            break;
    }
    SWPS_ERR("%s: exception occur <mode>:%d\n", __func__, self->mode);
    return ERR_IOEXP_UNEXCPT;
}


int
common_ioexp_fsm_4_direct(struct ioexp_obj_s *self){

    int result_val;
    int show_err = 1;
    char *func_mane = "common_ioexp_fsm_4_direct";

    switch (self->state){
        case STATE_IOEXP_INIT:
            result_val = _ioexp_init_handler(self);
            /* Exception case: terminate initial procedure */
            if(result_val < 0){
                /* Initial fail */
                return result_val;
            }
            if(self->state == STATE_IOEXP_INIT){
                /* Keep in INIT state, and return error */
                return ERR_IOEXP_UNINIT;
            }
            /* Case: Initial done */
            return 0;

        case STATE_IOEXP_NORMAL:
            result_val = self->update_all(self, show_err, func_mane);
            if (result_val < 0){
                SWPS_INFO("%s: NORMAL -> ABNORMAL <err>:%d\n",
                           __func__, result_val);
                self->state = STATE_IOEXP_ABNORMAL;
                return result_val;
            }
            self->state = STATE_IOEXP_NORMAL;
            return 0;

        case STATE_IOEXP_ABNORMAL:
            result_val = self->update_all(self, show_err, func_mane);
            if (result_val < 0){
                self->state = STATE_IOEXP_ABNORMAL;
                return result_val;
            }
            SWPS_DEBUG("%s: ABNORMAL -> NORMAL <err>:%d\n",
                       __func__, result_val);
            self->state = STATE_IOEXP_NORMAL;
            return 0;

        default:
            break;
    }
    SWPS_ERR("%s: Exception occurs <state>:%d\n",
            __func__, self->state);
    return ERR_IOEXP_UNEXCPT;
}

/* ========== Functions for Factory pattern ==========
 */
static struct ioexp_map_s *
get_ioexp_map(int ioexp_type){
    switch (ioexp_type){ 
        case IOEXP_TYPE_CYPRESS_NABC:
            return &ioexp_map_cypress_nabc;
        case IOEXP_TYPE_CYPRESS_7ABC:
            return &ioexp_map_cypress_7abc;
        default:
            return NULL;
    }
}


int
setup_ioexp_ssize_attr(struct ioexp_obj_s *self,
                       struct ioexp_map_s *ioexp_map_p,
                       int ioexp_id,
                       int ioexp_type,
                       int run_mode){
    switch (run_mode){
        case IOEXP_MODE_DIRECT:   /* Direct access device mode */
            self->mode = run_mode;
            break;
        default:
            SWPS_ERR("%s: non-defined run_mode:%d\n",
                     __func__, run_mode);
            self->mode = ERR_IOEXP_UNEXCPT;
            return ERR_IOEXP_UNEXCPT;
    }
    self->ioexp_id    = ioexp_id;
    self->ioexp_type  = ioexp_type;
    self->ioexp_map_p = ioexp_map_p;
    self->state       = STATE_IOEXP_INIT;
    mutex_init(&self->lock);
    return 0;
}


static int
setup_addr_mapping(struct ioexp_obj_s *self,
                   struct ioexp_addr_s *addr_map_p){
    if (!addr_map_p){
        SWPS_ERR("%s: map is null\n", __func__);
        return -1;
    }
    self->ioexp_map_p->map_addr = addr_map_p;
    return 0;
}


static int
setup_ioexp_public_cb(struct ioexp_obj_s *self,
                      int ioexp_type){
    switch (ioexp_type){
        case IOEXP_TYPE_CYPRESS_NABC:
            self->get_present    = common_get_present;
            self->get_tx_fault   = common_get_tx_fault;
            self->get_rxlos      = common_get_rxlos;
            self->get_tx_disable = common_get_tx_disable;
            self->get_reset      = ioexp_get_not_support;
            self->get_lpmod      = ioexp_get_not_support;
            self->get_modsel     = ioexp_get_not_support;
            self->set_tx_disable = common_set_tx_disable;
            self->set_reset      = ioexp_set_not_support;
            self->set_lpmod      = ioexp_set_not_support;
            self->set_modsel     = ioexp_set_not_support;
            return 0;
        case IOEXP_TYPE_CYPRESS_7ABC:
            self->get_present    = common_get_present;
            self->get_tx_fault   = ioexp_get_not_support;
            self->get_rxlos      = ioexp_get_not_support;
            self->get_tx_disable = ioexp_get_not_support;
            self->get_reset      = common_get_reset;
            self->get_lpmod      = common_get_lpmod;
            self->get_modsel     = common_get_modsel;
            self->set_tx_disable = ioexp_set_not_support;
            self->set_reset      = common_set_reset;
            self->set_lpmod      = common_set_lpmod;
            self->set_modsel     = common_set_modsel;
            return 0;

        default:
            SWPS_ERR("%s: type:%d incorrect!\n", __func__, ioexp_type);
            break;
    }
    return ERR_IOEXP_UNEXCPT;
}


static int
setup_ioexp_private_cb(struct ioexp_obj_s *self,
                       int ioexp_type){

    switch (ioexp_type){
        case IOEXP_TYPE_CYPRESS_NABC:
        case IOEXP_TYPE_CYPRESS_7ABC:
            self->init           = common_ioexp_init;
            self->update_all     = common_ioexp_update_all;
            self->fsm_4_direct   = common_ioexp_fsm_4_direct;
            return 0;

        default:
            SWPS_ERR("%s: type:%d incorrect!\n", __func__, ioexp_type);
            break;
    }
    return ERR_IOEXP_UNEXCPT;
}


static int
setup_i2c_client_one(struct ioexp_obj_s *self,
                     int chip_id){

    char *err_msg = "ERROR";
    struct i2c_adapter *adap    = NULL;
    struct i2c_client  *client  = NULL;
    struct ioexp_i2c_s *i2c_obj_p  = NULL;
    struct ioexp_i2c_s *i2c_curr_p = NULL;

    int chan_id = self->ioexp_map_p->map_addr[chip_id].chan_id;
    adap = i2c_get_adapter(chan_id);
    if(!adap){
        err_msg = "Can not get adap!";
        goto err_ioexp_setup_i2c_1;
    }
    client = kzalloc(sizeof(*client), GFP_KERNEL);
    if (!client){
        err_msg = "Can not kzalloc client!";
        goto err_ioexp_setup_i2c_1;
    }
    i2c_obj_p = kzalloc(sizeof(*i2c_obj_p), GFP_KERNEL);
    if (!i2c_obj_p){
        err_msg = "Can not kzalloc i2c_obj_p!";
        goto err_ioexp_setup_i2c_2;
    }
    client->adapter = adap;
    client->addr = self->ioexp_map_p->map_addr[chip_id].chip_addr;
    i2c_obj_p->i2c_client_p = client;
    i2c_obj_p->chip_id = chip_id;
    i2c_obj_p->next = NULL;
    if (!self->i2c_head_p){
        self->i2c_head_p = i2c_obj_p;
    } else {
        i2c_curr_p = self->i2c_head_p;
        while (i2c_curr_p->next){
            i2c_curr_p = i2c_curr_p->next;
        }
        i2c_curr_p->next = i2c_obj_p;
    }
    return 0;

err_ioexp_setup_i2c_2:
    kfree(client);
err_ioexp_setup_i2c_1:
    SWPS_ERR("%s: %s <chanID>:%d\n", __func__, err_msg, chan_id);
    return -1;
}


static int
setup_i2c_client(struct ioexp_obj_s* self){

    int result;
    int chip_id = 0;

    for (chip_id=0; chip_id<(self->ioexp_map_p->chip_amount); chip_id++){
        result  = setup_i2c_client_one(self, chip_id);
        if (result < 0){
            SWPS_ERR("%s fail! <chip_id>:%d\n", __func__, chip_id);
            return -1;
        }
    }
    return 0;
}

static int
setup_ioexp_config(struct ioexp_obj_s *self) {

    int chip_id, offset, err_code;
    struct ioexp_addr_s *addr_p;

    for (chip_id=0; chip_id<(self->ioexp_map_p->chip_amount); chip_id++){
        addr_p = &(self->ioexp_map_p->map_addr[chip_id]);
        if (!addr_p){
            SWPS_ERR("IOEXP config incorrect! <chip_id>:%d \n",chip_id);
            return -1;
        }
        for (offset=0; offset<(self->ioexp_map_p->data_width); offset++){

            err_code = i2c_smbus_write_byte_data(_get_i2c_client(self, chip_id),
                                                 addr_p->conf_offset[offset],
                                                 addr_p->conf_default[offset]);

            if (err_code < 0){
                SWPS_INFO("%s: set conf fail! <err>:%d \n", __func__, err_code);
                return -2;
            }
        }
    }
    return 0;
}

struct ioexp_obj_s *
_create_ioexp_obj(int ioexp_id,
                  int ioexp_type,
                  struct ioexp_addr_s *addr_map_p,
                  int run_mode){

    struct ioexp_map_s* ioexp_map_p;
    struct ioexp_obj_s* result_p;
    struct ioexp_i2c_s *i2c_curr_p;
    struct ioexp_i2c_s *i2c_next_p;

    /* Get layout */
    ioexp_map_p = get_ioexp_map(ioexp_type);
    if (!ioexp_map_p){
        SWPS_ERR("%s: Invalid ioexp_type\n", __func__);
        goto err_create_ioexp_fail;
    }
    /* Prepare IOEXP object */
    result_p = kzalloc(sizeof(*result_p), GFP_KERNEL);
    if (!result_p){
        SWPS_ERR("%s: kzalloc failure!\n", __func__);
        goto err_create_ioexp_fail;
    }
    /* Prepare static size attributes */
    if (setup_ioexp_ssize_attr(result_p,
                               ioexp_map_p,
                               ioexp_id,
                               ioexp_type,
                               run_mode) < 0){
        goto err_create_ioexp_setup_attr_fail;
    }
    /* Prepare address mapping */
    if (setup_addr_mapping(result_p, addr_map_p) < 0){
        goto err_create_ioexp_setup_attr_fail;
    }
    if (setup_i2c_client(result_p) < 0){
        goto err_create_ioexp_setup_i2c_fail;
    }
    /* Prepare call back functions of object */
    if (setup_ioexp_public_cb(result_p, ioexp_type) < 0){
        goto err_create_ioexp_setup_i2c_fail;
    }
    if (setup_ioexp_private_cb(result_p, ioexp_type) < 0){
        goto err_create_ioexp_setup_i2c_fail;
    }
    return result_p;

err_create_ioexp_setup_i2c_fail:
    i2c_curr_p = result_p->i2c_head_p;
    i2c_next_p = result_p->i2c_head_p;
    while (i2c_curr_p){
        i2c_next_p = i2c_curr_p->next;
        kfree(i2c_curr_p->i2c_client_p);
        kfree(i2c_curr_p);
        i2c_curr_p = i2c_next_p;
    }
err_create_ioexp_setup_attr_fail:
    kfree(result_p);
err_create_ioexp_fail:
    SWPS_ERR("%s: fail! <id>:%d <type>:%d \n",
             __func__, ioexp_id, ioexp_type);
    return NULL;
}


int
create_ioexp_obj(int ioexp_id,
                 int ioexp_type,
                 struct ioexp_addr_s *addr_map_p,
                 int run_mode){

    struct ioexp_obj_s *ioexp_p  = NULL;

    ioexp_p = _create_ioexp_obj(ioexp_id, ioexp_type,
                                addr_map_p, run_mode);
    if (!ioexp_p){
        return -1;
    }
    if (ioexp_head_p == NULL){
        ioexp_head_p = ioexp_p;
        ioexp_tail_p = ioexp_p;
        return 0;
    }
    ioexp_tail_p->next = ioexp_p;
    ioexp_tail_p = ioexp_p;
    return 0;
}

static int
_init_ioexp_obj(struct ioexp_obj_s* self) {

    char *err_msg   = "ERR";
    char *func_name = "_init_ioexp_obj";

    /* Setup IOEXP configure byte */
    if (setup_ioexp_config(self) < 0){
        err_msg = "setup_ioexp_config fail";
        goto err_init_ioexp_obj;
    }
    /* Setup default data */
    if (_ioexp_init_handler(self) < 0){
        err_msg = "_ioexp_init_handler fail";
        goto err_init_ioexp_obj;
    }
    /* Update all */
    if (self->state == STATE_IOEXP_NORMAL){
        if (self->update_all(self, 1, func_name) < 0){
            err_msg = "update_all() fail";
            goto err_init_ioexp_obj;
        }
    }
    return 0;

err_init_ioexp_obj:
    SWPS_DEBUG("%s: %s\n", __func__, err_msg);
    return -1;
}

int
init_ioexp_objs(void){
    /* Return value:
     *   0: Success
     *  -1: Detect topology error
     *  -2: SWPS internal error
     */

    struct ioexp_obj_s *curr_p  = ioexp_head_p;

    if (!curr_p) {
        SWPS_ERR("%s: ioexp_head_p is NULL\n", __func__);
        return -2;
    }
    while (curr_p) {
        if (_init_ioexp_obj(curr_p) < 0) {
            SWPS_DEBUG("%s: _init_ioexp_obj() fail\n", __func__);
            return -1;
        }
        curr_p = curr_p->next;
    }
    SWPS_DEBUG("%s: done.\n", __func__);
    return 0;
}

void
clean_ioexp_objs(void){

    struct ioexp_i2c_s *i2c_curr_p   = NULL;
    struct ioexp_i2c_s *i2c_next_p   = NULL;
    struct ioexp_obj_s *ioexp_next_p = NULL;
    struct ioexp_obj_s *ioexp_curr_p = ioexp_head_p;

    if (ioexp_head_p == NULL){
        ioexp_tail_p = NULL;
        return;
    }
    while(ioexp_curr_p){
        ioexp_next_p = ioexp_curr_p->next;
        i2c_curr_p = ioexp_curr_p->i2c_head_p;
        while (i2c_curr_p) {
            i2c_next_p = i2c_curr_p->next;
            kfree(i2c_curr_p->i2c_client_p);
            kfree(i2c_curr_p);
            i2c_curr_p = i2c_next_p;
        }
        kfree(ioexp_curr_p);
        ioexp_curr_p = ioexp_next_p;
    }
    ioexp_tail_p = NULL;
    SWPS_DEBUG("%s: done.\n", __func__);
}

struct ioexp_obj_s *
get_ioexp_obj(int ioexp_id){

    struct ioexp_obj_s *result_p = NULL;
    struct ioexp_obj_s *ioexp_curr_p = ioexp_head_p;

    while(ioexp_curr_p){
        if (ioexp_curr_p->ioexp_id == ioexp_id){
            result_p = ioexp_curr_p;
            break;
        }
        ioexp_curr_p = ioexp_curr_p->next;
    }
    return result_p;
}
int
check_channel_tier_1(void) {

    if ( (!_is_channel_ready(ioexp_head_p)) &&
         (!_is_channel_ready(ioexp_tail_p)) ){
        return -1;
    }
    return 0;
}


