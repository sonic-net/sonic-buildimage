#include "dii.h"

#include <stdlib.h>
#include <string.h>
// Top Level

CredoError_t cr_lane_set_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_set_lane_config(slice, lane, lane_config));
}

CredoError_t cr_lane_get_config(CredoSlice_t* slice, int lane, CredoLaneConfig_t* lane_config) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_get_lane_config(slice, lane, lane_config));
}

// Lane Mode

CredoError_t cr_lane_get_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_get_lane_mode(slice, lane, mode));
}

CredoError_t cr_lane_get_tx_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_lane_get_tx_mode(slice, lane, mode));
}

CredoError_t cr_lane_set_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_set_lane_mode(slice, lane, mode));
}

CredoError_t cr_lane_update_mode(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_update_lane_mode(slice, lane));
}

CredoError_t cr_lane_enable(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_enable_lane(slice, lane));
}

CredoError_t cr_lane_disable(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_disable_lane(slice, lane));
}

CredoError_t cr_lane_get_count(CredoSlice_t* slice, int* host_lane, int* line_lane) {
    CALL_HAL(slice, hal_get_lane_count(slice, host_lane, line_lane));
}

CredoError_t cr_lane_set_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_set_lane_loopback_mode(slice, lane, mode));
}

CredoError_t cr_lane_get_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_get_lane_loopback_mode(slice, lane, mode));
}

// TX Control
CredoError_t cr_lane_tx_disable(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_tx_disable(slice, lane));
}

CredoError_t cr_lane_tx_no_disable(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_tx_no_disable(slice, lane));
}

CredoError_t cr_lane_tx_force_squelch(CredoSlice_t* slice, int lane, bool enable) {
    CHECK_LANE_VALID(slice, lane);
    if (enable) {
        CALL_HAL(slice, hal_tx_disable(slice, lane));
    } else {
        CALL_HAL(slice, hal_tx_no_disable(slice, lane));
    }
}

CredoError_t cr_lane_tx_get_status(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_lane_tx_status(slice, lane, status));
}

// RX Control
CredoError_t cr_lane_rx_disable(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_rx_disable(slice, lane));
}

CredoError_t cr_lane_rx_no_disable(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_rx_no_disable(slice, lane));
}

CredoError_t cr_lane_rx_force_squelch(CredoSlice_t* slice, int lane, bool enable) {
    CHECK_LANE_VALID(slice, lane);
    if (enable) {
        CALL_HAL(slice, hal_rx_disable(slice, lane));
    } else {
        CALL_HAL(slice, hal_rx_no_disable(slice, lane));
    }
}

// Resets
CredoError_t cr_lane_rx_reset(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_lane_rx_reset(slice, lane));
}

CredoError_t cr_lane_logic_reset(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_logic_reset_lane(slice, lane));
}

CredoError_t cr_lane_reg_reset(CredoSlice_t* slice, int lane) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_reg_reset_lane(slice, lane));
}

// Capabilities

CredoError_t cr_lane_get_option_count(CredoSlice_t* slice, int* count) {
    if (slice == NULL) return CR_INVALID_ARGS;
    *count = slice->device->desc->slice_capability->lane_option_count;
    return CR_OK;
}

// Options

CredoError_t cr_lane_index_option_list(CredoSlice_t* slice, int index, CredoLaneOption_t* option) {
    if (slice == NULL) return CR_INVALID_ARGS;
    int count = slice->device->desc->slice_capability->lane_option_count;
    if (index < 0 || index >= count) return CR_INVALID_ARGS;
    const LaneOption_t* option_list = slice->device->desc->slice_capability->lane_option_list;
    option->name = option_list[index].name;
    option->description = option_list[index].description;
    return CR_OK;
}

CredoError_t cr_lane_get_option_list(CredoSlice_t* slice, int* option_count, const CredoLaneOption_t** option_list) {
    if (slice == NULL) {
        return CR_INVALID_ARGS;
    }
    *option_count = 0;
    *option_list = NULL;
    return CR_OK;
}

static const LaneOption_t* find_option(CredoSlice_t* slice, const char* name) {
    int option_count = slice->device->desc->slice_capability->lane_option_count;
    const LaneOption_t* option_list = slice->device->desc->slice_capability->lane_option_list;
    if (option_list == NULL || option_count == 0) return NULL;
    for (int i = 0; i < option_count; i++) {
        if (strcmp(name, option_list[i].name) == 0) {
            return &option_list[i];
        }
    }
    return NULL;
}

CredoError_t cr_lane_is_option_supported(CredoSlice_t* slice, const char* option_name) {
    if (slice == NULL || option_name == NULL) {
        return CR_INVALID_ARGS;
    }

    return find_option(slice, option_name) != NULL ? CR_OK : CR_NOTIMPLEMENTED;
}

CredoError_t cr_lane_get_option(CredoSlice_t* slice, int lane, const char* option_name, int* value) {
    if (slice == NULL || option_name == NULL || value == NULL ||
        lane >= slice->device->desc->slice_capability->lane_count) {
        return CR_INVALID_ARGS;
    }

    const LaneOption_t* option = find_option(slice, option_name);

    if (option == NULL) {
        return CR_NOTIMPLEMENTED;
    } else {
        if (option->get_func_ptr != NULL) {
            return option->get_func_ptr(slice, lane, option_name, value);
        } else {
            return CR_NOTIMPLEMENTED;
        }
    }
}

CredoError_t cr_lane_set_option(CredoSlice_t* slice, int lane, const char* option_name, int value) {
    if (slice == NULL || option_name == NULL || lane >= slice->device->desc->slice_capability->lane_count) {
        return CR_INVALID_ARGS;
    }
    const LaneOption_t* option = find_option(slice, option_name);
    if (option == NULL) {
        return CR_NOTIMPLEMENTED;
    } else {
        if (option->set_func_ptr != NULL) {
            return option->set_func_ptr(slice, lane, option_name, value);
        } else {
            return CR_NOTIMPLEMENTED;
        }
    }
}

CredoError_t cr_lane_get_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_fw_get_lane_speed(slice, lane, speed_kbps));
}

CredoError_t cr_lane_get_tx_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    CHECK_LANE_VALID(slice, lane);
    CALL_HAL(slice, hal_lane_get_tx_speed(slice, lane, speed_kbps));
}

CredoError_t cr_lane_get_paramh(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data) {
    return cr_param_get_paramh(slice, PARAM_DOMAIN_LANE, name, index, data);
}

CredoError_t cr_lane_get_param(CredoSlice_t* slice, const char* name, int index, CredoParamData_t* data) {
    return cr_param_get_param(slice, PARAM_DOMAIN_LANE, name, index, data);
}

CredoError_t cr_lane_set_param(CredoSlice_t* slice, const char* name, int index, const CredoParamData_t* data) {
    return cr_param_set_param(slice, PARAM_DOMAIN_LANE, name, index, data);
}
