#include "project.h"
#include "se_device.h"
#include "se_functions.h"
#include "se_option.h"

#include "common/options.h"

#include "utility.h"

#include <string.h>

CredoError_t se_option_get_toppll_mode(CredoSlice_t* slice, const char* name, int* value) {
    ERR_PROPS(se_fw_get_toppll_mode(slice, value));
    return CR_OK;
}

CredoError_t se_option_set_toppll_mode(CredoSlice_t* slice, const char* name, int value) {
    ERR_PROPS(se_fw_set_toppll_mode(slice, value));
    return CR_OK;
}

CredoError_t se_option_get_fw_em_vstep_side(CredoSlice_t* slice, const char* name, int* value) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    *value = se_slice->em_vstep_side;
    return CR_OK;
}

CredoError_t se_option_set_fw_em_vstep_side(CredoSlice_t* slice, const char* name, int value) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    se_slice->em_vstep_side = value;
    return CR_OK;
}

CredoError_t se_option_get_fw_isi_timeout(CredoSlice_t* slice, const char* name, int* value) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    *value = se_slice->fw_isi_timeout;
    return CR_OK;
}

CredoError_t se_option_set_fw_isi_timeout(CredoSlice_t* slice, const char* name, int value) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    se_slice->fw_isi_timeout = (value == 0) ? FW_ISI_TIMEOUT_PER_PHASE : value;
    return CR_OK;
}

CredoError_t se_option_get_isc_slice_id(CredoSlice_t* slice, const char* name, int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (seslice->isc_slice_id == 0xFFFF) {  // uninitialized
        hal_fw_reg_rd_internal(slice, FWREG_TOP_ISC_SLICE_ID, &seslice->isc_slice_id);
    }
    *value = seslice->isc_slice_id;
    return CR_OK;
}

CredoError_t se_option_set_isc_slice_id(CredoSlice_t* slice, const char* name, int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_ISC_SLICE_ID, value));
    seslice->isc_slice_id = value;
    return CR_OK;
}

// lane options
CredoError_t se_lane_option_get_phase_base(CredoSlice_t* slice, int lane, const char* name, int* value) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    *value = se_slice->em_info[lane].phase_base;
    return CR_OK;
}

CredoError_t se_lane_option_set_phase_base(CredoSlice_t* slice, int lane, const char* name, int value) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    se_slice->em_info[lane].phase_base = value & 0xffff;
    return CR_OK;
}

CredoError_t se_lane_option_get_fast_recover_timeout(CredoSlice_t* slice, int lane, const char* name, int* value) {
    return se_fw_get_fast_recover_timeout(slice, lane, value);
}

CredoError_t se_lane_option_set_fast_recover_timeout(CredoSlice_t* slice, int lane, const char* name, int value) {
    return se_fw_set_fast_recover_timeout(slice, lane, value);
}

CredoError_t se_lane_option_get_sd_delay(CredoSlice_t* slice, int lane, const char* name, int* value) {
    return se_fw_get_sd_delay(slice, lane, value);
}

CredoError_t se_lane_option_set_sd_delay(CredoSlice_t* slice, int lane, const char* name, int value) {
    return se_fw_set_sd_delay(slice, lane, value);
}

CredoError_t se_lane_option_set_oneshot(CredoSlice_t* slice, int lane, const char* name, int value) {
    unsigned regval = 0;
    ERR_PROPS(hal_fw_reg_rd(slice, 0, 0, &regval));  // reg val is really lane_continue, so we negate the value
    if (value == 0) {
        regval |= (1 << lane);
    } else {
        regval &= ~(1 << lane);
    }
    ERR_PROPS(hal_fw_reg_wr(slice, 0, 0, regval));
    return CR_OK;
}

CredoError_t se_lane_option_get_oneshot(CredoSlice_t* slice, int lane, const char* name, int* value) {
    unsigned regval = 0;
    ERR_PROPS(hal_fw_reg_rd(slice, 0, 0, &regval));
    *value = ((regval & (1 << lane)) != 0) ? 0 : 1;
    return CR_OK;
}

CredoError_t se_lane_option_set_lt_oneshot(CredoSlice_t* slice, int lane, const char* name, int value) {
    unsigned regval = 0;
    ERR_PROPS(hal_fw_reg_rd(slice, 4, 4, &regval));
    if (value == 0) {
        regval &= ~(1 << lane);
    } else {
        regval |= (1 << lane);
    }
    ERR_PROPS(hal_fw_reg_wr(slice, 4, 4, regval));
    return CR_OK;
}

CredoError_t se_lane_option_get_lt_oneshot(CredoSlice_t* slice, int lane, const char* name, int* value) {
    unsigned regval = 0;
    ERR_PROPS(hal_fw_reg_rd(slice, 4, 4, &regval));
    *value = ((regval & (1 << lane)) != 0) ? 1 : 0;
    return CR_OK;
}

CredoError_t se_slice_option_set_lt_timer(CredoSlice_t* slice, const char* name, int value) {
    if (strcmp(name, "anlt_holdoff_timer") == 0) {
        ERR_PROPS(hal_fw_reg_wr(slice, 8, 3, value));
    } else if (strcmp(name, "anlt_max_wait_timer") == 0) {
        ERR_PROPS(hal_fw_reg_wr(slice, 9, 3, value));
    } else if (strcmp(name, "anlt_wait_timer") == 0) {
        ERR_PROPS(hal_fw_reg_wr(slice, 10, 3, value));
    } else if (strcmp(name, "anlt_link_fail_inhibit_timer") == 0) {
        ERR_PROPS(hal_fw_reg_wr(slice, 11, 3, value));
    } else {
        return CR_INVALID_ARGS;
    }
    return CR_OK;
}

CredoError_t se_slice_option_get_lt_timer(CredoSlice_t* slice, const char* name, int* value) {
    if (strcmp(name, "anlt_holdoff_timer") == 0) {
        ERR_PROPS(hal_fw_reg_rd(slice, 8, 3, (unsigned*)value));
    } else if (strcmp(name, "anlt_max_wait_timer") == 0) {
        ERR_PROPS(hal_fw_reg_rd(slice, 9, 3, (unsigned*)value));
    } else if (strcmp(name, "anlt_wait_timer") == 0) {
        ERR_PROPS(hal_fw_reg_rd(slice, 10, 3, (unsigned*)value));
    } else if (strcmp(name, "anlt_link_fail_inhibit_timer") == 0) {
        ERR_PROPS(hal_fw_reg_rd(slice, 11, 3, (unsigned*)value));
    } else {
        return CR_INVALID_ARGS;
    }
    return CR_OK;
}

CredoError_t se_slice_option_set_vsensor_res(CredoSlice_t* slice, const char* name, int value) {
    int res[] = {14, 12, 10, 8};
    SeSlice_t* seslice = (SeSlice_t*)slice;

    if (value < 0 || value > 3) {
        LOGS_WARN("[Slice Option] setting out of range. Set to default.");
        value = FW_VSENSOR_DEFAULT_RESOLUTION;
    }

    seslice->vsensor_resolution = value;
    LOGS_INFO("[Slice Option] set vsensor to %u-bit resolution.", res[value]);
    return CR_OK;
}

CredoError_t se_slice_option_get_vsensor_res(CredoSlice_t* slice, const char* name, int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->vsensor_resolution;
    return CR_OK;
}

// port options
static CredoError_t se_port_option_get_direction(CredoSlice_t* slice, const char* option_name, int port_id,
                                                 int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].direction;
    return CR_OK;
}

static CredoError_t se_port_option_set_direction(CredoSlice_t* slice, const char* option_name, int port_id, int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (seslice->port_info[port_id].started) {
        LOGS_ERROR("[Port Option][%d] Port is already started, skip.", port_id);
        return CR_FAIL;
    }
    if (value > 2) {
        LOGS_ERROR("[Port Option][%d] invalid option value.", port_id);
        return CR_INVALID_ARGS;
    }
    seslice->port_info[port_id].direction = value;
    return CR_OK;
}

static CredoError_t se_port_option_get_50g_mode(CredoSlice_t* slice, const char* name, int port, int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = (seslice->port_info[port].is_50g_nrz_mode == true) ? 1 : 0;
    return CR_OK;
}

static CredoError_t se_port_option_set_50g_mode(CredoSlice_t* slice, const char* name, int port, int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (seslice->port_info[port].started) {
        LOGS_ERROR("[Port Option][%d] Port is already started.", port);
        return CR_FAIL;
    }
    seslice->port_info[port].is_50g_nrz_mode = (value != 0) ? true : false;
    return CR_OK;
}

static CredoError_t se_port_option_get_host_lt(CredoSlice_t* slice, const char* name, int port, int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port].host_lt;
    return CR_OK;
}

static CredoError_t se_port_option_set_host_lt(CredoSlice_t* slice, const char* name, int port, int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (seslice->port_info[port].started) {
        LOGS_ERROR("[Port Option][LT][%d] Port is already started.", port);
        return CR_FAIL;
    }
    seslice->port_info[port].host_lt = value;
    return CR_OK;
}

static CredoError_t se_port_option_get_line_lt(CredoSlice_t* slice, const char* name, int port, int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port].line_lt;
    return CR_OK;
}

static CredoError_t se_port_option_set_line_lt(CredoSlice_t* slice, const char* name, int port, int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (seslice->port_info[port].started) {
        LOGS_ERROR("[Port Option][LT][%d] Port is already started.", port);
        return CR_FAIL;
    }
    seslice->port_info[port].line_lt = value;
    return CR_OK;
}

static CredoError_t se_port_option_get_line_an(CredoSlice_t* slice, const char* name, int port, int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port].line_an;
    return CR_OK;
}

static CredoError_t se_port_option_set_line_an(CredoSlice_t* slice, const char* name, int port, int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (seslice->port_info[port].started) {
        LOGS_ERROR("[Port Option][AN][%d] Port is already started.", port);
        return CR_FAIL;
    }
    seslice->port_info[port].line_an = value;
    return CR_OK;
}

CredoError_t se_port_option_set_line_main_lane(CredoSlice_t* slice, const char* option_name, int port_id, int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (!seslice->port_info[port_id].built) {
        LOGS_ERROR("[Port Option][%d] Port must be built.", port_id);
        return CR_FAIL;
    }
    if (seslice->port_info[port_id].started) {
        LOGS_ERROR("[Port Option][%d] Port is already started, skip.", port_id);
        return CR_FAIL;
    }
    // check user gave valid main lane
    for (size_t i = 0; i < seslice->port_info[port_id].setup.line_count; i++) {
        if (seslice->port_info[port_id].setup.line_lanes[i] == value) {
            seslice->port_info[port_id].line_main_lane = value;
            return CR_OK;
        }
    }
    LOGS_ERROR("Line lane %d not in port setup", value);
    return CR_INVALID_ARGS;
}

static CredoError_t se_port_option_get_host_main_lane(CredoSlice_t* slice, const char* option_name, int port_id,
                                                      int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].host_main_lane;
    return CR_OK;
}

static CredoError_t se_port_option_get_line_main_lane(CredoSlice_t* slice, const char* option_name, int port_id,
                                                      int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].line_main_lane;
    return CR_OK;
}

CredoError_t se_port_option_set_host_main_lane(CredoSlice_t* slice, const char* option_name, int port_id, int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (!seslice->port_info[port_id].built) {
        LOGS_ERROR("[Port Option][%d] Port must be built.", port_id);
        return CR_FAIL;
    }
    if (seslice->port_info[port_id].started) {
        LOGS_ERROR("[Port Option][%d] Port is already started, skip.", port_id);
        return CR_FAIL;
    }
    // check user gave valid main lane
    for (size_t i = 0; i < seslice->port_info[port_id].setup.host_count; i++) {
        if (seslice->port_info[port_id].setup.host_lanes[i] == value) {
            seslice->port_info[port_id].host_main_lane = value;
            return CR_OK;
        }
    }
    LOGS_ERROR("Host lane %d not in port setup", value);
    return CR_INVALID_ARGS;
}

static CredoError_t se_port_option_set_switchover_switch(CredoSlice_t* slice, const char* option_name, int port_id,
                                                         int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (!seslice->port_info[port_id].built) {
        LOGS_WARN("[Port Option][%d] Port must be built.", port_id);
        return CR_OK;
    }
    if (!seslice->port_info[port_id].started) {
        LOGS_WARN("[Port Option][%d] Port is not started, skip.", port_id);
        return CR_OK;
    }
    if (seslice->port_info[port_id].setup.mode != CR_PORT_SWITCHOVER_RETIMER) {
        LOGS_WARN("[Port Option][%d] Port is not in switchover mode, skip.", port_id);
        return CR_OK;
    }

    if (strcmp(option_name, "switchover_select") == 0) {
        // only on change switch active standby, allowing absolute value
        if ((!value) != (!seslice->port_info[port_id].switchover_select)) {
            seslice->port_info[port_id].switchover_select = value;
            ERR_PROPS(se_port_switchover_switch(slice, port_id, 0xFF));
        }
    } else if (strcmp(option_name, "switchover_switch") == 0) {
        seslice->port_info[port_id].switchover_select = !seslice->port_info[port_id].switchover_select;
        ERR_PROPS(se_port_switchover_switch(slice, port_id, 0xFF));
    } else if (strcmp(option_name, "switchover_switch_lane") == 0) {
        ERR_PROPS(se_port_switchover_switch(slice, port_id, value));
    }
    return CR_OK;
}

static CredoError_t se_port_option_get_switchover_select(CredoSlice_t* slice, const char* option_name, int port_id,
                                                         int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (!seslice->port_info[port_id].built) {
        LOGS_WARN("[Port Option][%d] Port must be built.", port_id);
        return CR_OK;
    }
    if (!seslice->port_info[port_id].started) {
        LOGS_WARN("[Port Option][%d] Port is not started, skip.", port_id);
        return CR_OK;
    }
    if (seslice->port_info[port_id].setup.mode != CR_PORT_SWITCHOVER_RETIMER) {
        LOGS_WARN("[Port Option][%d] Port is not in switchover mode, skip.", port_id);
        return CR_OK;
    }
    *value = (int)seslice->port_info[port_id].switchover_select;
    return CR_OK;
}

static CredoError_t se_port_option_get_switchover_active_map(CredoSlice_t* slice, const char* option_name, int port_id,
                                                             int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (!seslice->port_info[port_id].built) {
        LOGS_WARN("[Port Option][%d] Port must be built.", port_id);
        return CR_OK;
    }
    if (!seslice->port_info[port_id].started) {
        LOGS_WARN("[Port Option][%d] Port is not started, skip.", port_id);
        return CR_OK;
    }
    if (seslice->port_info[port_id].setup.mode != CR_PORT_SWITCHOVER_RETIMER) {
        LOGS_WARN("[Port Option][%d] Port is not in switchover mode, skip.", port_id);
        return CR_OK;
    }
    *value = (int)seslice->port_info[port_id].active_lane_map;
    return CR_OK;
}

static CredoError_t se_port_option_get_isc_enable(CredoSlice_t* slice, const char* option_name, int port_id,
                                                  int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].isc.enable;
    return CR_OK;
}

static CredoError_t se_port_option_set_isc_enable(CredoSlice_t* slice, const char* option_name, int port_id,
                                                  int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    seslice->port_info[port_id].isc.enable = value ? true : false;
    return CR_OK;
}

static CredoError_t se_port_option_get_isc_an_agent(CredoSlice_t* slice, const char* option_name, int port_id,
                                                    int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].isc.an_agent;
    return CR_OK;
}

static CredoError_t se_port_option_set_isc_an_agent(CredoSlice_t* slice, const char* option_name, int port_id,
                                                    int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    seslice->port_info[port_id].isc.an_agent = value ? true : false;
    return CR_OK;
}

static CredoError_t se_port_option_get_isc_lane_map_a(CredoSlice_t* slice, const char* option_name, int port_id,
                                                      int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].isc.lane_map_a;
    return CR_OK;
}

static CredoError_t se_port_option_set_isc_lane_map_a(CredoSlice_t* slice, const char* option_name, int port_id,
                                                      int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    seslice->port_info[port_id].isc.lane_map_a = value;
    return CR_OK;
}

static CredoError_t se_port_option_get_isc_lane_map_b(CredoSlice_t* slice, const char* option_name, int port_id,
                                                      int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].isc.lane_map_b;
    return CR_OK;
}

static CredoError_t se_port_option_set_isc_lane_map_b(CredoSlice_t* slice, const char* option_name, int port_id,
                                                      int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    seslice->port_info[port_id].isc.lane_map_b = value;
    return CR_OK;
}

static CredoError_t se_port_get_lane_map(CredoSlice_t* slice, int port, int lane_map[2]) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (!seslice->port_info[port].built) {
        LOGS_ERROR("[Port Option][%d] Port must be built to get lane map", port);
        return CR_FAIL;
    }

    const CredoPortSetup_t* setup = &seslice->port_info[port].setup;

    int map_a = 0;
    int map_b = 0;

    if (setup->host_count == setup->line_count) {  // retimer
        for (size_t i = 0; i < setup->host_count; i++) {
            int host_lane = setup->host_lanes[i] & 0x7;
            int line_lane = setup->line_lanes[i] & 0x7;
            map_a |= (8 | line_lane) << (4 * host_lane);
            map_b |= (8 | host_lane) << (4 * line_lane);
        }
        lane_map[0] = map_a;
        lane_map[1] = map_b;
        return CR_OK;
    }
    const int* halfrate_lanes = NULL;
    const int* fullrate_lanes = NULL;
    size_t fullrate_lane_count = 0;
    bool is_a1b2 = false;
    if (setup->host_count * 2 == setup->line_count) {  // a1b2 bitmux
        fullrate_lanes = setup->host_lanes;
        halfrate_lanes = setup->line_lanes;
        fullrate_lane_count = setup->host_count;
        is_a1b2 = true;
    } else if (setup->host_count == setup->line_count * 2) {  // a2b1 bitmux
        fullrate_lanes = setup->line_lanes;
        halfrate_lanes = setup->host_lanes;
        fullrate_lane_count = setup->line_count;
        is_a1b2 = false;
    } else {  // invalid bitmux or port
        return CR_FAIL;
    }
    for (size_t i = 0; i < fullrate_lane_count; i++) {
        int halfrate_lane0 = halfrate_lanes[i * 2] & 0x7;
        int halfrate_lane1 = halfrate_lanes[(i * 2) + 1] & 0x7;
        int fullrate_lane = fullrate_lanes[i] & 0x7;

        map_a |= (8 | halfrate_lane0) << (4 * fullrate_lane);
        map_b |= (8 | fullrate_lane) << (4 * halfrate_lane0);
        map_b |= (8 | fullrate_lane) << (4 * halfrate_lane1);
    }
    if (is_a1b2) {
        lane_map[0] = map_a;
        lane_map[1] = map_b;
    } else {
        lane_map[0] = map_b;
        lane_map[1] = map_a;
    }
    return CR_OK;
}

CredoError_t se_port_get_lane_map_a(CredoSlice_t* slice, const char* option, int port, int* lane_map_a) {
    int lane_maps[2];
    ERR_PROPS(se_port_get_lane_map(slice, port, lane_maps));
    *lane_map_a = lane_maps[0];
    return CR_OK;
}

CredoError_t se_port_get_lane_map_b(CredoSlice_t* slice, const char* option, int port, int* lane_map_b) {
    int lane_maps[2];
    ERR_PROPS(se_port_get_lane_map(slice, port, lane_maps));
    *lane_map_b = lane_maps[1];
    return CR_OK;
}

static CredoError_t se_port_option_set_an_override(CredoSlice_t* slice, const char* option_name, int port_id,
                                                   int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (seslice->port_info[port_id].started) {
        LOGS_WARN("[Port Option][AN Override][%d] Port is already started, skip.", port_id);
        return CR_OK;
    }
    seslice->port_info[port_id].an_override = value;
    return CR_OK;
}

static CredoError_t se_port_option_get_an_override(CredoSlice_t* slice, const char* option_name, int port_id,
                                                   int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].an_override;
    return CR_OK;
}

static CredoError_t se_port_option_set_isc_host_main_lane(CredoSlice_t* slice, const char* option_name, int port_id,
                                                          int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    seslice->port_info[port_id].isc.host_main_lane = value;
    return CR_OK;
}

static CredoError_t se_port_option_get_isc_host_main_lane(CredoSlice_t* slice, const char* option_name, int port_id,
                                                          int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].isc.host_main_lane;
    return CR_OK;
}

static CredoError_t se_port_option_set_isc_line_main_lane(CredoSlice_t* slice, const char* option_name, int port_id,
                                                          int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    seslice->port_info[port_id].isc.line_main_lane = value;
    return CR_OK;
}

static CredoError_t se_port_option_get_isc_line_main_lane(CredoSlice_t* slice, const char* option_name, int port_id,
                                                          int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].isc.line_main_lane;
    return CR_OK;
}

static CredoError_t se_port_option_set_flexspeed_kbps(CredoSlice_t* slice, const char* option_name, int port_id,
                                                      int value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    if (seslice->port_info[port_id].started) {
        LOGS_WARN("[Port Option][Flexspeed][%d] Port is already started, skip.", port_id);
        return CR_OK;
    }
    double speed = value * 1e3;
    if (speed < 1e9 || speed > 112.5e9) {
        LOGS_ERROR("[Port Option][Flexspeed] Lane speed must be >=1G and <=112.5G");
        return CR_INVALID_ARGS;
    }
    seslice->port_info[port_id].flexspeed_kbps = value;
    return CR_OK;
}

static CredoError_t se_port_option_get_flexspeed_kbps(CredoSlice_t* slice, const char* option_name, int port_id,
                                                      int* value) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    *value = seslice->port_info[port_id].flexspeed_kbps;
    return CR_OK;
}

const OptionHandler_t option_port_list[] = {
    OPTION_DEF("direction",
               "Set link direction, only used in uni retimer. 0: bidirectional, 1: host to line, 2: line to host, "
               "default is 0 (bidirectional)",
               se_port_option_get_direction, se_port_option_set_direction),
    OPTION_DEF("line_an", "Line side AN option", se_port_option_get_line_an, se_port_option_set_line_an),
    OPTION_DEF("line_lt", "Line side LT option", se_port_option_get_line_lt, se_port_option_set_line_lt),
    OPTION_DEF("host_lt", "Hose side LT option", se_port_option_get_host_lt, se_port_option_set_host_lt),
    OPTION_DEF("an_override", "Auto-Negotiation enable override pages", se_port_option_get_an_override,
               se_port_option_set_an_override),
    OPTION_DEF("host_main_lane",
               "Set the host main lane. Defaults to the first host lane in {c:struct}`CredoPortSetup_t`",
               se_port_option_get_host_main_lane, se_port_option_set_host_main_lane),
    OPTION_DEF("line_main_lane",
               "Set the line main lane. Defaults to the first line lane in {c:struct}`CredoPortSetup_t`",
               se_port_option_get_line_main_lane, se_port_option_set_line_main_lane),
    OPTION_DEF("50g_nrz_mode", "Set 50g default speed mode. Default is pam4 mode.", se_port_option_get_50g_mode,
               se_port_option_set_50g_mode),
    OPTION_DEF("isc_enable", "Set isc enable, associate 2 slices in one port. Default is false.",
               se_port_option_get_isc_enable, se_port_option_set_isc_enable),
    OPTION_DEF("switchover_select", "Ability to select what mux group is active. 0= mux0 active, 1= mux1 active",
               se_port_option_get_switchover_select, se_port_option_set_switchover_switch),
    OPTION_DEF("switchover_switch", "Switch all current active lanes to standy. param value doesn't matter.", NULL,
               se_port_option_set_switchover_switch),
    OPTION_DEF("switchover_switch_lane",
               "Debug purpose option. Switch given lanes from active to standby, and vice versa.", NULL,
               se_port_option_set_switchover_switch),
    OPTION_DEF("switchover_active_map", "Debug purpose option. Get current active lanes for the switchover",
               se_port_option_get_switchover_active_map, NULL),
    OPTION_DEF("isc_an_agent", "Set isc an agent, Default is false.", se_port_option_get_isc_an_agent,
               se_port_option_set_isc_an_agent),
    OPTION_DEF("isc_lane_map_a", "Set isc lane map host side.", se_port_option_get_isc_lane_map_a,
               se_port_option_set_isc_lane_map_a),
    OPTION_DEF("isc_lane_map_b", "Set isc lane map line side.", se_port_option_get_isc_lane_map_b,
               se_port_option_set_isc_lane_map_b),
    OPTION_DEF("isc_host_main_lane", "Set isc host main lane.", se_port_option_get_isc_host_main_lane,
               se_port_option_set_isc_host_main_lane),
    OPTION_DEF("isc_line_main_lane", "Set isc host main lane.", se_port_option_get_isc_line_main_lane,
               se_port_option_set_isc_line_main_lane),
    OPTION_DEF("lane_map_a", "Get the lane mapping for a port. Used for isc_lane_map_a. Only has a getter",
               se_port_get_lane_map_a, NULL),
    OPTION_DEF("lane_map_b", "Get the lane mapping for a port. Used for isc_lane_map_b. Only has a getter",
               se_port_get_lane_map_b, NULL),
    OPTION_DEF("flexspeed_kbps", "Set flexspeed datarate for a port in kbps ", se_port_option_get_flexspeed_kbps,
               se_port_option_set_flexspeed_kbps),
};

const int option_port_count = COUNT_OF(option_port_list);
