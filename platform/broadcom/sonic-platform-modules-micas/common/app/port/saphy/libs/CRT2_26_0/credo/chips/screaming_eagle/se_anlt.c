#include "screaming_eagle.h"
#include "se_functions.h"

#include "anlt/anlt.h"

CredoError_t se_autoneg_get_exchanged_pages(CredoSlice_t* slice, int lane, int* page_count,
                                            uint64_t transmitted_pages[9], uint64_t received_pages[9]) {
    unsigned fw_status;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));
    if (fw_status == 0) {
        ERR_PROPS(common_get_autoneg_exchanged_pages(slice, lane, page_count, transmitted_pages, received_pages));
    } else {
        ERR_PROP(se_fw_get_an_pages(slice, lane, transmitted_pages, received_pages));
        *page_count = 3;
    }

    return CR_OK;
}

CredoError_t se_an_get_restart_count(CredoSlice_t* slice, int lane, unsigned* val) {
    return hal_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_AN_RESTART_COUNT, (unsigned*)val);
}

CredoError_t se_lt_get_restart_count(CredoSlice_t* slice, int lane, unsigned* val) {
    return hal_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_LT_RESTART_COUNT, (unsigned*)val);
}

CredoError_t se_an_get_state(CredoSlice_t* slice, int lane, CredoAutoNegState_t* state) {
    unsigned raw_state, an_on = 0;
    ERR_PROPS(se_fw_get_anlt(slice, lane, &an_on, NULL));
    if (an_on == 0) {
        *state = CR_AUTONEG_STATE_OFF;
        return CR_OK;
    }
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_AN_HW_STATE, &raw_state));
    *state = (raw_state < 9) ? raw_state + 1 : CR_AUTONEG_STATE_UNKNOWN;
    return CR_OK;
}

CredoError_t se_get_lt_status(CredoSlice_t* slice, int lane, CredoLinkTrainingStatus_t* lt_status) {
    *lt_status = CR_LT_STATUS_IDLE;

    CredoLinkTrainingState_t lt_state = CR_LT_STATE_OFF;
    ERR_PROP(se_get_lt_state(slice, lane, &lt_state));

    switch (lt_state) {
        case CR_LT_STATE_OFF:
            *lt_status = CR_LT_STATUS_OFF;
            break;
        case CR_LT_STATE_IDLE:
            *lt_status = CR_LT_STATUS_IDLE;
            break;
        case CR_LT_STATE_LINK_UP:
            *lt_status = CR_LT_STATUS_FINISHED;
            break;
        case CR_LT_STATE_EXIT:
            *lt_status = CR_LT_STATUS_FAILED;
            break;
        default:
            *lt_status = CR_LT_STATUS_TRAINING;
            break;
    }
    return CR_OK;
}

CredoError_t se_get_lt_state(CredoSlice_t* slice, int lane, CredoLinkTrainingState_t* lt_state) {
    *lt_state = CR_LT_STATE_OFF;

    unsigned lt_on = 0;
    ERR_PROPS(se_fw_get_anlt(slice, lane, NULL, &lt_on));
    if (lt_on == 0) {
        *lt_state = CR_LT_STATE_OFF;
        return CR_OK;
    }

    unsigned lt_status = 0;
    ERR_PROPS(se_fw_get_lt_state(slice, lane, &lt_status));

    ERR_PROPS(se_fw_translate_state_reverse(lt_status, lt_state));
    return CR_OK;
}
