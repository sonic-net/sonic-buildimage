#include "project.h"
#include "be_device.h"
#include "be_functions.h"

#include "common/common_firmware.h"

#include <string.h>

static unsigned BE_PORT_SUPPORT_FLAGS =
    CR_PFLAG_LINE_SIDE_ANLT | CR_PFLAG_AUTONEG_OVERRIDE | CR_PFLAG_LINE_SIDE_OPTICAL;

typedef struct {
    uint8_t lanes;
    uint8_t host_lane;
    uint8_t line_lane;
    uint8_t code;
} config_table;

static const config_table* be_match_config(const CredoPortConfig_t* port_config, const config_table* table) {
    const config_table* entry = table;
    int match = 0;
    while (entry->lanes) {
        do {
            if (port_config->line_no_of_lanes != entry->lanes) break;
            if (port_config->host_start_lane != entry->host_lane) break;
            if (port_config->line_start_lane != entry->line_lane) break;
            match = 1;
        } while (0);
        if (match) return entry;
        entry++;
    }
    return NULL;
}

static CredoError_t be_port_check_flag(CredoSlice_t* slice, unsigned flags) {
    if (flags & ~BE_PORT_SUPPORT_FLAGS) {
        LOGS_ERROR("[Port config] Incorrect flags setting %u", flags);
        return CR_INVALID_ARGS;
    }

    if ((flags & CR_PFLAG_LINE_SIDE_ANLT) && (flags & CR_PFLAG_LINE_SIDE_OPTICAL)) {
        LOGS_ERROR("[Port config] line side ANLT/optical flags can't use simultaneously");
        return CR_INVALID_ARGS;
    }

    return CR_OK;
}

static CredoError_t be_port_send_config(CredoSlice_t* slice, const CredoPortConfig_t* port_config, unsigned cmd,
                                        unsigned config_detail, CredoLaneMode_t A_mode, CredoLaneMode_t B_mode,
                                        int force) {
    unsigned response, mask;
    int lane;
    uint32_t port_id = port_config->port_id;

    BeSlice_t* be_slice = (BeSlice_t*)slice;
    BePortInfo_t* port_info = &be_slice->port_info[port_id];

    if (port_info->configured == true && !force) {
        LOGS_ERROR("[Port config] Port %d is already taken. Deconfig first or use force option.", port_id);
        return CR_INVALID_ARGS;
    }
    if (force) {
        /* Deconfig port if applicable */
        ERR_PROPS(be_port_teardown(slice, port_id));
        ERR_PROPS(be_fw_deconfig_cmd(slice, cmd));
    }

    if (be_fw_set_nrz_optical_mode(slice, port_config->line_start_lane, port_config->line_no_of_lanes,
                                   port_config->flags & CR_PFLAG_LINE_SIDE_OPTICAL) != CR_OK)
        return CR_FAIL;

    if (port_config->flags & CR_PFLAG_LINE_SIDE_ANLT) {
        config_detail |= FW_OPTION_LINE_SIDE_ANLT_ENABLE;
    }

    if (port_config->flags & CR_PFLAG_AUTONEG_OVERRIDE) {
        config_detail |= FW_OPTION_AUTONEG_OVERRIDE;
    }

    /* Now config */
    ERR_PROPS(common_fw_cmd(slice, cmd + FW_CMD_CONFIG_MODE, config_detail, &response, &mask));
    /* Bookkeeping */
    for (lane = 0; lane < LANES; lane++) {
        if (mask & (1 << lane)) {
            be_slice->lane_mode[lane] = (lane < LANES / 2) ? A_mode : B_mode;
        }
    }
    port_info->configured = true;
    memcpy(&port_info->port_config, port_config, sizeof(CredoPortConfig_t));
    return CR_OK;
}

static const config_table be_retimer_config_table[] = {
    /* One pair */
    {1, 0, 8, 0},
    {1, 1, 9, 1},
    {1, 2, 10, 2},
    {1, 3, 11, 3},
    {1, 4, 12, 4},
    {1, 5, 13, 5},
    {1, 6, 14, 6},
    {1, 7, 15, 7},
    {1, 0, 12, 0},  // retimer cross
    {1, 1, 13, 1},
    {1, 2, 14, 2},
    {1, 3, 15, 3},
    {1, 4, 8, 4},
    {1, 5, 9, 5},
    {1, 6, 10, 6},
    {1, 7, 11, 7},
    /* Two pairs */
    {2, 0, 8, 8},
    {2, 2, 10, 9},
    {2, 4, 12, 10},
    {2, 6, 14, 11},
    {2, 0, 12, 8},  // retimer cross
    {2, 2, 14, 9},
    {2, 4, 8, 10},
    {2, 6, 10, 11},
    /* Four pairs */
    {4, 0, 8, 12},
    {4, 4, 12, 13},
    {4, 0, 12, 12},  // retimer cross
    {4, 4, 8, 13},
    /* Eight pairs */
    {8, 0, 8, 14},
    /* End */
    {0, 0, 0, 0},
};

CredoError_t be_fw_config_retimer(CredoSlice_t* slice, const CredoPortConfig_t* port_config, int force) {
    uint32_t speed = port_config->speed;
    int lanes = port_config->line_no_of_lanes;
    uint32_t lane_speed = speed / lanes;
    FirmwareSpeed_t lane_speed_code;
    CredoLaneMode_t mode = CR_LMODE_NRZ;
    /* Calculate the command and index */
    const config_table* entry = be_match_config(port_config, be_retimer_config_table);
    if (!entry || (port_config->line_no_of_lanes != port_config->host_no_of_lanes)) {
        LOGS_ERROR("[Port config][Retimer] Unknown combination of host lane %d, line lane %d, and speed %d",
                   port_config->host_start_lane, port_config->line_start_lane, speed);
        return CR_INVALID_ARGS;
    }
    unsigned fw_speed, cmd;

    /* All the valid retimer speeds are listed here. */
    switch (lane_speed) {
        case CONFIG_10G:
            lane_speed_code = SPEED_10G;
            break;
        case CONFIG_20G:
            lane_speed_code = SPEED_20G;
            break;
        case CONFIG_25G:
            lane_speed_code = SPEED_25G;
            break;
        case CONFIG_26G:
            lane_speed_code = SPEED_26G;
            break;
        case CONFIG_50G:
        case CONFIG_53G:
            lane_speed_code = SPEED_53G;
            mode = CR_LMODE_PAM4;
            break;
        default:
            goto INVALID;
    }
    fw_speed = lane_speed_code * 0x11;  // retimer always use the same speed

    FirmwareMode_t firmware_mode = MODE_RETIMER_NRZ;
    if (mode == CR_LMODE_PAM4) {
        firmware_mode = MODE_RETIMER_PAM4;
    }

    // retimer cross
    if (entry->line_lane - entry->host_lane != LINE_LANES) {
        firmware_mode += MODE_RETIMER_CROSS_NRZ;
    }

    cmd = entry->code + ((int)(firmware_mode) << 4);

    return be_port_send_config(slice, port_config, cmd, fw_speed, mode, mode, force);
INVALID:
    LOGS_ERROR("[Port config][Retimer] Unknown combination of speed=%d and lanes=%d", speed, lanes);
    return CR_INVALID_ARGS;
}

static const config_table be_bitmux_config_table[] = {
    /* 2->4 */
    {4, 0, 8, 6},
    {4, 2, 12, 7},
    {4, 4, 12, 8},
    /* 1->2 */
    {2, 0, 8, 0},
    {2, 1, 10, 1},
    {2, 2, 12, 2},
    {2, 3, 14, 3},
    {2, 4, 12, 4},
    {2, 5, 14, 5},
    /* End */
    {0, 0, 0, 0},
};

CredoError_t be_fw_config_bitmux(CredoSlice_t* slice, const CredoPortConfig_t* port_config, int force) {
    Speed_t speed = port_config->speed;
    int lanes = port_config->line_no_of_lanes;
    unsigned fw_speed, cmd, A_mode;
    /* Calculate the command and index */
    const config_table* entry = be_match_config(port_config, be_bitmux_config_table);
    if (!entry) {
        LOGS_ERROR("[Port config][Bitmux] Unknown combination of host lane %d, line lane %d and speed %d",
                   port_config->host_start_lane, port_config->line_start_lane, speed);
        return CR_INVALID_ARGS;
    }
    /* All the valid bitmux speeds are listed here. */
    switch (speed) {
        case CONFIG_40G:
            /* Must be 20Gx2 to 10Gx4 */
            if (lanes != 4) goto INVALID;
            fw_speed = DEFSPEED(20G, 10G);
            cmd = (int)MODE_BITMUX_A1B2_NRZ;
            A_mode = CR_LMODE_NRZ;
            break;
        case CONFIG_50G:
            /* Must be 53Gx1 to 26Gx2 */
            if (lanes != 2) goto INVALID;
            fw_speed = DEFSPEED(53G, 26G);
            cmd = (int)MODE_BITMUX_A1B2_PAM4;
            A_mode = CR_LMODE_PAM4;
            break;
        case CONFIG_100G:
            /* Must be 53Gx2 to 26Gx2 */
            if (lanes != 4) goto INVALID;
            fw_speed = DEFSPEED(53G, 26G);
            cmd = (int)MODE_BITMUX_A1B2_PAM4;
            A_mode = CR_LMODE_PAM4;
            break;
        default:
            goto INVALID;
    }
    cmd = (cmd << 4) + entry->code;
    return be_port_send_config(slice, port_config, cmd, fw_speed, A_mode, CR_LMODE_NRZ, force);
INVALID:
    LOGS_ERROR("[Port config][Bitmux] Unknown combination of speed=%d and lanes=%d", speed, lanes);
    return CR_INVALID_ARGS;
}

static const config_table be_gearbox_config_table[] = {
    /* 2->4 */
    {4, 0, 8, 0},
    {4, 2, 12, 1},
    {4, 4, 12, 2},
    /* 1->2 */
    {2, 0, 8, 0},
    {2, 1, 10, 1},
    {2, 2, 12, 2},
    {2, 3, 14, 3},
    {2, 4, 12, 4},
    {2, 5, 14, 5},
    {2, 2, 10, 6},
    {2, 6, 14, 7},
    /* End */
    {0, 0, 0, 0},
};

CredoError_t be_fw_config_gearbox(CredoSlice_t* slice, const CredoPortConfig_t* port_config, int force) {
    Speed_t speed = port_config->speed;
    int lanes = port_config->line_no_of_lanes;
    unsigned fw_speed = DEFSPEED(53G, 25G);  // All 53G->25G
    unsigned cmd;
    /* Calculate the command and index */
    const config_table* entry = be_match_config(port_config, be_gearbox_config_table);
    if (!entry) {
        LOGS_ERROR("[Port config][GearBox] Unknown combination of host lane %d, line lane %d, and speed %d",
                   port_config->host_start_lane, port_config->line_start_lane, speed);
        return CR_INVALID_ARGS;
    }
    /* All the valid gearbox speeds are listed here */
    switch (lanes) {
        case 2:  // 1->2
            if (speed != CONFIG_50G) goto INVALID;
            cmd = (int)MODE_GEARBOX_50G_PAM4;
            if (port_config->line_fec_type != CR_FEC_RS_528) {
                LOGS_ERROR("[Port config][GearBox] 50G line side FEC only support RS528");
                return CR_INVALID_ARGS;
            }
            break;
        case 4:  // 2->4
            if (speed != CONFIG_100G) goto INVALID;
            cmd = (int)MODE_GEARBOX_100G_PAM4;
            if (port_config->line_fec_type == CR_FEC_NONE) {
                fw_speed |= CONFIG_GB_NOFEC << 8;
            }
            break;
        default:
            goto INVALID;
    }
    cmd = (cmd << 4) + entry->code;
    return be_port_send_config(slice, port_config, cmd, fw_speed, CR_LMODE_PAM4, CR_LMODE_NRZ, force);
INVALID:
    LOGS_ERROR("[Port config][GearBox] Unknown combination of speed = %d and lanes = %d", speed, lanes);
    return CR_INVALID_ARGS;
}

CredoError_t be_port_config(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force) {
    if (port_config->line_no_of_lanes == 0) {
        LOGS_ERROR("[Port config] Incorrect configuration with 0 line_no_of_lanes");
        return CR_INVALID_ARGS;
    }
    if (port_config->port_id >= BE_MAX_PORT) {
        LOGS_ERROR("[Port config] Incorrect port_id %d (valid port_id is 0-%d)", port_config->port_id,
                   slice->desc->port_count - 1);
        return CR_INVALID_ARGS;
    }

    ERR_PROP(be_port_check_flag(slice, port_config->flags));

    switch (port_config->connection_mode) {
        case CR_PORT_RETIMER:
            ERR_PROPS(be_fw_config_retimer(slice, port_config, force));
            break;
        case CR_PORT_BITMUX:
            ERR_PROPS(be_fw_config_bitmux(slice, port_config, force));
            break;
        case CR_PORT_GEARBOX:
            ERR_PROPS(be_fw_config_gearbox(slice, port_config, force));
            break;
        default:
            return CR_UNSUPPORTED;
    }

    return CR_OK;
}

/**
 * @brief Destroy one port to BaldEagle device. No-op if the port_id is not configured.
 * @param slice BaldEagle slice handle
 * @param port The port to destroy
 * @return Error Code
 */
CredoError_t be_port_teardown(CredoSlice_t* slice, uint32_t port_id) {
    BeSlice_t* be_slice = (BeSlice_t*)slice;
    BePortInfo_t* port_info = &be_slice->port_info[port_id];
    CredoPortConfig_t* port_config = &port_info->port_config;

    if (port_info->configured == false) return CR_OK;

    // reset nrz optical flag
    unsigned line_start_lane = port_info->port_config.line_start_lane;
    unsigned line_no_of_lanes = port_info->port_config.line_no_of_lanes;
    ERR_PROPS(be_fw_set_nrz_optical_mode(slice, line_start_lane, line_no_of_lanes, 0));

    ERR_PROPS(be_fw_deconfig_lane(slice, port_config->host_start_lane));
    port_info->configured = false;
    return CR_OK;
}

CredoError_t be_port_teardown_all(CredoSlice_t* slice) {
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        ERR_PROPS(be_port_teardown(slice, port_id));
    }
    return CR_OK;
}

CredoError_t be_port_query(CredoSlice_t* slice, uint32_t port_id, CredoPortConfig_t* port_config) {
    BeSlice_t* be_slice = (BeSlice_t*)slice;
    BePortInfo_t* port_info = &be_slice->port_info[port_id];

    if (port_info->configured == false) {
        port_config->port_id = CR_PORT_UNCONFIGURED;
        return CR_OK;
    }

    memcpy(port_config, &port_info->port_config, sizeof(CredoPortConfig_t));
    return CR_OK;
}
