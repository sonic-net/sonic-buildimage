#include "project.h"
#include "bh_device.h"
#include "bh_functions.h"

#include "common/common_firmware.h"

#include <string.h>

static unsigned BH_PORT_SUPPORT_FLAGS = CR_PFLAG_LINE_SIDE_ANLT | CR_PFLAG_AUTONEG_DISABLE | CR_PFLAG_AUTONEG_OVERRIDE |
                                        CR_PFLAG_SYS_SIDE_OPTICAL | CR_PFLAG_LINE_SIDE_OPTICAL;

typedef struct {
    uint8_t host_start_lane;
    uint8_t line_start_lane;
    uint8_t host_no_of_lanes;
    uint8_t line_no_of_lanes;
} config_table;

static const config_table* bh_match_config(const CredoPortConfig_t* port_config, const config_table* table) {
    const config_table* entry = table;
    int match = 0;
    while (entry->line_start_lane) {
        do {
            if (port_config->host_start_lane != entry->host_start_lane) break;
            if (port_config->line_start_lane != entry->line_start_lane) break;
            if (port_config->host_no_of_lanes != entry->host_no_of_lanes) break;
            if (port_config->line_no_of_lanes != entry->line_no_of_lanes) break;
            match = 1;
        } while (0);
        if (match) return entry;
        entry++;
    }
    return NULL;
}

static CredoError_t bh_port_check_flag(CredoSlice_t* slice, unsigned flags) {
    if (flags & ~BH_PORT_SUPPORT_FLAGS) {
        LOGS_ERROR("[Port config] Incorrect flags setting %u", flags);
        return CR_INVALID_ARGS;
    }

    if ((flags & CR_PFLAG_LINE_SIDE_ANLT) && (flags & CR_PFLAG_LINE_SIDE_OPTICAL)) {
        LOGS_ERROR("[Port config] line side ANLT/optical flags can't use simultaneously");
        return CR_INVALID_ARGS;
    }

    if ((flags & CR_PFLAG_AUTONEG_OVERRIDE) && !(flags & CR_PFLAG_LINE_SIDE_ANLT)) {
        LOGS_WARN("[Port config] autoneg override flag can't use alone, skip it");
    }

    return CR_OK;
}

static CredoError_t bh_port_send_config(CredoSlice_t* slice, CredoPortConfig_t* port_config, unsigned cmd,
                                        unsigned detail1, int force) {
    uint32_t detail2 = 0, response = 0;
    uint32_t port_id = port_config->port_id;
    uint32_t flags = port_config->flags;
    uint32_t host_start_lane = port_config->host_start_lane;
    uint32_t host_no_of_lanes = port_config->host_no_of_lanes;
    uint32_t line_start_lane = port_config->line_start_lane;
    uint32_t line_no_of_lanes = port_config->line_no_of_lanes;

    if (port_config->port_id != CR_PORT_AUTO_ASSIGN_ID) {
        unsigned port_config_mask = 0;
        ERR_PROPS(bh_fw_port_config_mask(slice, &port_config_mask));

        CredoPortConfig_t old_port_config = {.port_id = CR_PORT_UNCONFIGURED};
        if (port_config_mask & (1 << port_id)) {
            ERR_PROPS(bh_port_query(slice, port_config->port_id, &old_port_config));
        }

        if (old_port_config.port_id != CR_PORT_UNCONFIGURED && !force) {
            LOGS_ERROR("[Port config] Port %d is already taken. Deconfig first or use force option.", port_id);
            return CR_INVALID_ARGS;
        }
    }

    detail2 = DEFLANE_INDEX_2(port_config->host_start_lane, port_config->line_start_lane);
    if (port_id != CR_PORT_AUTO_ASSIGN_ID) detail2 |= ((port_id + 1) << 12);

    if (force) {
        FirmwareMode_t fw_mode = (cmd >> 4) & 0xF;
        ERR_PROPS(bh_fw_deconfig_cmd(slice, fw_mode, detail1, detail2));
    }

    if ((flags & CR_PFLAG_LINE_SIDE_LT) == CR_PFLAG_LINE_SIDE_LT) {
        detail2 |= FW_OPTION_LINE_SIDE_LT_ENABLE;
    } else if (flags & CR_PFLAG_LINE_SIDE_ANLT) {
        detail1 |= FW_OPTION_LINE_SIDE_ANLT_ENABLE;
    }

    if (flags & CR_PFLAG_SYS_SIDE_LT) {
        detail1 |= FW_OPTION_SYS_SIDE_LT_ENABLE;
    }

    if (flags & CR_PFLAG_AUTONEG_OVERRIDE) {
        detail1 |= FW_OPTION_AUTONEG_OVERRIDE;
    }

    ERR_PROP(bh_fw_set_optical_mode(slice, host_start_lane, host_no_of_lanes, flags & CR_PFLAG_SYS_SIDE_OPTICAL));
    ERR_PROP(bh_fw_set_optical_mode(slice, line_start_lane, line_no_of_lanes, flags & CR_PFLAG_LINE_SIDE_OPTICAL));

    LOGS_DEBUG("[Port config] Cmd 0x%04X, detail1 0x%04X, detail2 0x%04X", cmd, detail1, detail2);
    ERR_PROP(common_fw_cmd_ex(slice, cmd, detail1, detail2, &response, NULL, NULL));

    if (port_id == CR_PORT_AUTO_ASSIGN_ID) {
        /* Take firmware assigned port ID from response */
        port_id = response & 0xF;
        port_config->port_id = port_id;
    }

    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    bh_slice->port_info[port_id].configured = true;
    bh_slice->port_info[port_id].port_config = *port_config;

    return CR_OK;
}

static CredoError_t bh_port_config_retimer(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force) {
    if (port_config->line_no_of_lanes != port_config->host_no_of_lanes) {
        LOGS_ERROR("[Port config][Retimer] Host/Line lane number mismatch.");
        return CR_INVALID_ARGS;
    }

    if (port_config->host_fec_type != CR_FEC_NONE) {
        LOGS_WARN("[Port config][Retimer] Set host FEC type on retimer mode, force to NOFEC.");
        port_config->host_fec_type = CR_FEC_NONE;
    }
    if (port_config->line_fec_type != CR_FEC_NONE) {
        LOGS_WARN("[Port config][Retimer] Set line FEC type on retimer mode, force to NOFEC.");
        port_config->line_fec_type = CR_FEC_NONE;
    }

    CredoLaneMode_t lane_mode = CR_LMODE_NRZ;
    unsigned speed = port_config->speed / port_config->line_no_of_lanes;
    unsigned detail1;
    switch (speed) {
        case CONFIG_1G:
            detail1 = DEFSPEED(1G);
            break;
        case CONFIG_10G:
            detail1 = DEFSPEED(10G);
            break;
        case CONFIG_20G:
            detail1 = DEFSPEED(20G);
            break;
        case CONFIG_25G:
            detail1 = DEFSPEED(25G);
            break;
        case CONFIG_26G:
            detail1 = DEFSPEED(26G);
            break;
        case CONFIG_28G:
            detail1 = DEFSPEED(28G);
            break;
        case CONFIG_50G:
        case CONFIG_53G:
            detail1 = DEFSPEED(53G);
            lane_mode = CR_LMODE_PAM4;
            break;
        case CONFIG_55G:
            detail1 = DEFSPEED(55G);
            lane_mode = CR_LMODE_PAM4;
            break;
        case CONFIG_56_15G:
            detail1 = DEFSPEED(56_15G);
            lane_mode = CR_LMODE_PAM4;
            break;
        case CONFIG_56G:
        case CONFIG_56_25G:
            detail1 = DEFSPEED(56G);
            lane_mode = CR_LMODE_PAM4;
            break;
        default:
            // FIXME: need refactor
            if (speed >= CONFIG_55_9G && speed < CONFIG_56G) {
                detail1 = DEFSPEED(55_9G);
                lane_mode = CR_LMODE_PAM4;
            } else if (speed >= CONFIG_27G && speed < 28000) {
                detail1 = DEFSPEED(27_9G);
            } else {
                LOGS_ERROR("[Port config][Retimer] Not supported Speed: %d", speed);
                return CR_UNSUPPORTED;
            }
    }

    /* translate multilane */
    unsigned multilane_code = port_config->line_no_of_lanes;
    ERR_PROPS(bh_fw_translate_multilane(multilane_code, &multilane_code));
    detail1 |= multilane_code << 8;

    unsigned cmd = FW_CMD_CONFIG_MODE + (int)(MODE_RETIMER << 4);
    ERR_PROPS(bh_port_send_config(slice, port_config, cmd, detail1, force));

    for (int i = 0; i < port_config->host_no_of_lanes; i++) {
        ERR_PROPS(bh_set_lane_mode(slice, i + port_config->host_start_lane, lane_mode));
        ERR_PROPS(bh_set_lane_mode(slice, i + port_config->line_start_lane, lane_mode));
    }

    return CR_OK;
}

static const config_table bh_bitmux_config_table[] = {
    /* {host_start_lane, line_start_lane, host_no_of_lanes, line_no_of_lanes} */
    /* 1->2 */
    {0, 8, 1, 2},
    {1, 10, 1, 2},
    {2, 12, 1, 2},
    {3, 14, 1, 2},
    {4, 12, 1, 2},
    {5, 14, 1, 2},
    {0, 8, 2, 1},
    {2, 9, 2, 1},
    {4, 10, 2, 1},
    {6, 11, 2, 1},
    {4, 12, 2, 1},
    {6, 13, 2, 1},
    /* 2->4 */
    {0, 8, 2, 4},
    {2, 12, 2, 4},
    {4, 12, 2, 4},
    {0, 8, 4, 2},
    {4, 10, 4, 2},
    {4, 12, 4, 2},
    /* End */
    {0, 0, 0, 0},
};

static CredoError_t bh_port_config_bitmux(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force) {
    uint32_t speed = port_config->speed;
    int line_lanes = port_config->line_no_of_lanes;
    int host_lanes = port_config->host_no_of_lanes;
    unsigned cmd, detail1, multilane_code;
    CredoLaneMode_t A_mode, B_mode;
    FirmwareMode_t fw_mode;

    /* Calculate the command and index */
    if (bh_match_config(port_config, bh_bitmux_config_table) == NULL) goto INVALID;

    /* All the valid bitmux speeds are listed here. */
    switch (speed) {
        case CONFIG_56_25G:
            if (line_lanes != 2 && host_lanes != 2) goto INVALID;
            fw_mode = (line_lanes == 2) ? MODE_BITMUX_A1B2 : MODE_BITMUX_A2B1;
            detail1 = (line_lanes == 2) ? (DEFSPEED(56G, 28_125G)) : (DEFSPEED(28_125G, 56G));
            A_mode = (line_lanes == 2) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
            B_mode = (line_lanes == 2) ? CR_LMODE_NRZ : CR_LMODE_PAM4;
            multilane_code = 1;
            break;
        case (CONFIG_56_15G * 2):
        case (CONFIG_28G * 4):
            if (line_lanes != 4 && host_lanes != 4) goto INVALID;
            fw_mode = (line_lanes == 4) ? MODE_BITMUX_A1B2 : MODE_BITMUX_A2B1;
            detail1 = (line_lanes == 4) ? (DEFSPEED(56_15G, 28G)) : (DEFSPEED(28G, 56_15G));
            A_mode = (line_lanes == 4) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
            B_mode = (line_lanes == 4) ? CR_LMODE_NRZ : CR_LMODE_PAM4;
            multilane_code = 2;
            break;
        case (CONFIG_55_9G * 2):
            if (line_lanes != 4 && host_lanes != 4) goto INVALID;
            fw_mode = (line_lanes == 4) ? MODE_BITMUX_A1B2 : MODE_BITMUX_A2B1;
            detail1 = (line_lanes == 4) ? (DEFSPEED(55_9G, 27_9G)) : (DEFSPEED(27_9G, 55_9G));
            A_mode = (line_lanes == 4) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
            B_mode = (line_lanes == 4) ? CR_LMODE_NRZ : CR_LMODE_PAM4;
            multilane_code = 2;
            break;
        case CONFIG_20G:
            if (line_lanes != 2 && host_lanes != 2) goto INVALID;
            fw_mode = (line_lanes == 2) ? MODE_BITMUX_A1B2 : MODE_BITMUX_A2B1;
            detail1 = (line_lanes == 2) ? (DEFSPEED(20G, 10G)) : (DEFSPEED(10G, 20G));
            A_mode = B_mode = CR_LMODE_NRZ;
            multilane_code = 1;
            break;
        case CONFIG_40G:
            /* Must be 20Gx2 to 10Gx4 or 10Gx4 to 20Gx2*/
            if (line_lanes != 4 && host_lanes != 4) goto INVALID;
            fw_mode = (line_lanes == 4) ? MODE_BITMUX_A1B2 : MODE_BITMUX_A2B1;
            detail1 = (line_lanes == 4) ? (DEFSPEED(20G, 10G)) : (DEFSPEED(10G, 20G));
            A_mode = B_mode = CR_LMODE_NRZ;
            multilane_code = 2;
            break;
        case CONFIG_50G:
        case CONFIG_53G:
            /* Must be 53Gx1 to 26Gx2 or 26Gx2 to 53Gx1*/
            if (line_lanes != 1 && host_lanes != 1) goto INVALID;
            fw_mode = (line_lanes == 2) ? MODE_BITMUX_A1B2 : MODE_BITMUX_A2B1;
            detail1 = (line_lanes == 2) ? (DEFSPEED(53G, 26G)) : (DEFSPEED(26G, 53G));
            A_mode = (line_lanes == 2) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
            B_mode = (line_lanes == 2) ? CR_LMODE_NRZ : CR_LMODE_PAM4;
            multilane_code = 1;
            break;
        case CONFIG_100G:
        case CONFIG_53G * 2:
            /* Must be 53Gx1 to 26Gx2 or 26Gx2 to 53Gx1*/
            if (line_lanes != 2 && host_lanes != 2) goto INVALID;
            fw_mode = (line_lanes == 4) ? MODE_BITMUX_A1B2 : MODE_BITMUX_A2B1;
            detail1 = (line_lanes == 4) ? (DEFSPEED(53G, 26G)) : (DEFSPEED(26G, 53G));
            A_mode = (line_lanes == 4) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
            B_mode = (line_lanes == 4) ? CR_LMODE_NRZ : CR_LMODE_PAM4;
            multilane_code = 2;
            break;
        default:
            goto INVALID;
    }

    /* translate multilane */
    ERR_PROPS(bh_fw_translate_multilane(multilane_code, &multilane_code));
    detail1 |= multilane_code << 8;

    cmd = FW_CMD_CONFIG_MODE + (int)(fw_mode << 4);
    ERR_PROPS(bh_port_send_config(slice, port_config, cmd, detail1, force));

    for (int i = 0; i < port_config->host_no_of_lanes; i++) {
        ERR_PROPS(bh_set_lane_mode(slice, i + port_config->host_start_lane, A_mode));
    }

    for (int i = 0; i < port_config->line_no_of_lanes; i++) {
        ERR_PROPS(bh_set_lane_mode(slice, i + port_config->line_start_lane, B_mode));
    }

    return CR_OK;

INVALID:
    LOGS_ERROR("[Port config][Bitmux] Unknown combination of host start lane %d, line start lane %d and speed %d",
               host_lanes, line_lanes, speed);
    return CR_INVALID_ARGS;
}

static const config_table bh_gearbox_config_table[] = {
    // {host_start_lane, line_start_lane, host_no_of_lanes, line_no_of_lanes},
    /* 1->2 */
    {0, 8, 1, 2},
    {1, 10, 1, 2},
    {2, 10, 1, 2},
    {2, 12, 1, 2},
    {3, 14, 1, 2},
    {4, 12, 1, 2},
    {5, 14, 1, 2},
    {6, 14, 1, 2},
    //{4, 12, 4},
    //{5, 14, 5},
    //{6, 10, 6},
    //{7, 14, 7},
    /* 2->4 */
    {0, 8, 2, 4},
    {2, 12, 2, 4},
    {4, 12, 2, 4},
    //{4, 4, 12, 2},
    /* End */
    {0, 0, 0, 0},
};

static CredoError_t bh_port_config_gearbox(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force) {
    unsigned cmd;
    unsigned host_fec_type = port_config->host_fec_type;
    unsigned line_fec_type = port_config->line_fec_type;
    Speed_t speed = port_config->speed;

    /* Calculate the command and index */
    if (bh_match_config(port_config, bh_gearbox_config_table) == NULL) {
        LOGS_ERROR("[Port config][GearBox] Unknown combination of host start lane %d, line start lane %d,and speed %d",
                   port_config->host_start_lane, port_config->line_start_lane, speed);
        return CR_INVALID_ARGS;
    }

    /* All the valid gearbox speeds are listed here
     * 100G gearbox mode:
     *   100G-KP2 on A side to 100G-KR4 on B side
     *   100G-KP2 on A side to 100G AUI (CAUI, no FEC) on B side
     * 50G gearbox mode:
     *   50G-KP on A side to 50G-KR2 on B side
     */
    if (host_fec_type != CR_FEC_RS_544) {
        LOGS_ERROR("[Port config][GearBox] Host FEC type only supports RS544");
        return CR_INVALID_ARGS;
    }
    if (line_fec_type != CR_FEC_RS_528 && line_fec_type != CR_FEC_NONE) {
        LOGS_ERROR("[Port config][GearBox] Line FEC type only supports RS528 or NONE");
        return CR_INVALID_ARGS;
    }

    switch (speed) {
        case CONFIG_50G:
        case CONFIG_53G:
            if (line_fec_type != CR_FEC_RS_528) {
                LOGS_ERROR("[Port config][GearBox] 50G line side FEC type only support RS528");
                return CR_INVALID_ARGS;
            }
            cmd = FW_CMD_CONFIG_MODE + (int)(MODE_GEARBOX_50G << 4);
            break;
        case CONFIG_100G:
        case CONFIG_106G:
            cmd = FW_CMD_CONFIG_MODE + (int)(MODE_GEARBOX_100G << 4);
            break;
        default:
            LOGS_ERROR("[Port config][GearBox] Unknown port speed=%d", speed);
            return CR_INVALID_ARGS;
    }

    /* translate FEC */
    unsigned fw_fec;
    bh_fw_translate_FEC(line_fec_type, &fw_fec);
    cmd |= (fw_fec << 0);

    bh_fw_translate_FEC(host_fec_type, &fw_fec);
    cmd |= (fw_fec << 2);

    unsigned detail1 = DEFSPEED(53G, 25G);  // All 53G->25G
    ERR_PROPS(bh_port_send_config(slice, port_config, cmd, detail1, force));

    for (int i = 0; i < port_config->host_no_of_lanes; i++) {
        ERR_PROPS(bh_set_lane_mode(slice, i + port_config->host_start_lane, CR_LMODE_PAM4));
    }
    for (int i = 0; i < port_config->line_no_of_lanes; i++) {
        ERR_PROPS(bh_set_lane_mode(slice, i + port_config->line_start_lane, CR_LMODE_NRZ));
    }

    return CR_OK;
}

CredoError_t bh_port_config(CredoSlice_t* slice, CredoPortConfig_t* port_config, int force) {
    uint32_t port_id = port_config->port_id;

    if (port_id != CR_PORT_AUTO_ASSIGN_ID && port_id >= slice->desc->port_count) {
        LOGS_ERROR("[Port config] Incorrect port_id %d (valid port_id is 0-%d)", port_id, slice->desc->port_count - 1);
        return CR_INVALID_ARGS;
    }

    ERR_PROP(bh_port_check_flag(slice, port_config->flags));

    switch (port_config->connection_mode) {
        case CR_PORT_RETIMER:
            ERR_PROPS(bh_port_config_retimer(slice, port_config, force));
            break;
        case CR_PORT_BITMUX:
            ERR_PROPS(bh_port_config_bitmux(slice, port_config, force));
            break;
        case CR_PORT_GEARBOX:
            ERR_PROPS(bh_port_config_gearbox(slice, port_config, force));
            break;
        default:
            return CR_UNSUPPORTED;
    }

    if (port_id == CR_PORT_AUTO_ASSIGN_ID) {
        /* Take firmware assigned port ID from response */
        port_id = port_config->port_id;
        LOGS_INFO("[Port config] Firmware assigning port_id: %d", port_id);
    }

    return CR_OK;
}

CredoError_t bh_port_teardown(CredoSlice_t* slice, unsigned port_id) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    if (port_config.port_id == CR_PORT_UNCONFIGURED) return CR_OK;

    // reset optical flag
    unsigned host_start_lane = port_config.host_start_lane;
    unsigned host_no_of_lanes = port_config.host_no_of_lanes;
    unsigned line_start_lane = port_config.line_start_lane;
    unsigned line_no_of_lanes = port_config.line_no_of_lanes;
    ERR_PROPS(bh_fw_set_optical_mode(slice, host_start_lane, host_no_of_lanes, 0));
    ERR_PROPS(bh_fw_set_optical_mode(slice, line_start_lane, line_no_of_lanes, 0));

    return bh_fw_deconfig_lane(slice, port_config.host_start_lane);
}

CredoError_t bh_port_teardown_all(CredoSlice_t* slice) {
    unsigned port_config_mask = 0;
    ERR_PROPS(bh_fw_port_config_mask(slice, &port_config_mask));
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        if (port_config_mask & (1 << port_id)) {
            ERR_PROPS(bh_port_teardown(slice, port_id));
        }
    }
    return CR_OK;
}

CredoError_t bh_port_query(CredoSlice_t* slice, unsigned port_id, CredoPortConfig_t* port_config) {
    if (port_id < 0 || port_id > BH_MAX_PORT) {
        return CR_INVALID_ARGS;
    }
    BhSlice_t* bh_slice = (BhSlice_t*)slice;

    if (!bh_slice->port_info[port_id].configured) {
        CredoPortConfig_t unconfig_port = {.port_id = CR_PORT_UNCONFIGURED, 0};
        *port_config = unconfig_port;
        return CR_OK;
    }
    *port_config = bh_slice->port_info[port_id].port_config;
    return CR_OK;
}

CredoError_t bh_port_info(CredoSlice_t* slice, unsigned port_id, CredoPortConfig_t* port_config, bool update) {
    if (!update) {
        return bh_port_query(slice, port_id, port_config);
    }
    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    unsigned detail1 = 0, detail2 = 0, response = 0;

    // TODO: move to bh_fw
    ERR_PROPS(common_fw_cmd_ex(slice, FW_CMD_PORT_QUERY + port_id, 0, 0, &response, &detail1, &detail2));
    LOGS_DEBUG("[Port query] Response 0x%04X, detail1 0x%04X, detail2 0x%04X", response, detail1, detail2);

    /* Extract the fields of port query */
    unsigned fw_mode = (response >> 4) & 0x0f;
    unsigned char fec = (response >> 0) & 0x0f;
    unsigned multilane = (detail1 >> 8) & 0x0f;
    unsigned char host_fw_speed = (detail1 >> 4) & 0x0f;
    unsigned char line_fw_speed = (detail1 >> 0) & 0x0f;
    unsigned char host_start_lane = (detail2 >> 6) & 0x1f;
    unsigned char line_start_lane = (detail2 >> 0) & 0x1f;
    unsigned host_speed = 0, line_speed = 0;

    /* unconfigured */
    if (host_start_lane == 0x1f) {
        bh_slice->port_info[port_id].configured = false;
        port_config->port_id = CR_PORT_UNCONFIGURED;
        return CR_OK;
    }

    ERR_PROP_LOG(bh_fw_translate_config_mode_reverse(fw_mode, &port_config->connection_mode),
                 LOGS_ERROR("[Port query] Unknown fw mode %d", fw_mode));

    ERR_PROP_LOG(bh_fw_translate_lane_speed_reverse((FirmwareSpeed_t)host_fw_speed, &host_speed),
                 LOGS_ERROR("[Port query] Unknown host side fw lane speed %d", host_fw_speed));

    ERR_PROP_LOG(bh_fw_translate_lane_speed_reverse((FirmwareSpeed_t)line_fw_speed, &line_speed),
                 LOGS_ERROR("[Port query] Unknown line side fw lane speed %d", line_fw_speed));

    ERR_PROP_LOG(bh_fw_translate_multilane_reverse(multilane, &multilane),
                 LOGS_ERROR("[Port query] Unknown fw multilane code %d", multilane));

    FirmwareFecType_t fw_fec;
    fw_fec = (fec >> 2) & 3;
    ERR_PROP_LOG(bh_fw_translate_FEC_reverse(fw_fec, &port_config->host_fec_type),
                 LOGS_ERROR("[Port query] Unknown host side fw fec type%d", fw_fec));

    fw_fec = (fec >> 0) & 3;
    ERR_PROP_LOG(bh_fw_translate_FEC_reverse(fw_fec, &port_config->line_fec_type),
                 LOGS_ERROR("[Port query] Unknown line side fw fec type%d", fw_fec));

    port_config->port_id = port_id;
    port_config->flags = 0;
    port_config->host_start_lane = host_start_lane;
    port_config->line_start_lane = line_start_lane;
    port_config->port_mode = CR_PMODE_SERDES;

    if (fw_mode == MODE_GEARBOX_50G) {
        port_config->host_no_of_lanes = 1;
        port_config->line_no_of_lanes = 2;
    } else if (fw_mode == MODE_GEARBOX_100G) {
        port_config->host_no_of_lanes = 2;
        port_config->line_no_of_lanes = 4;
    } else if (fw_mode == MODE_BITMUX_A2B1) {
        port_config->host_no_of_lanes = 2 * multilane;
        port_config->line_no_of_lanes = multilane;
    } else if (fw_mode == MODE_BITMUX_A1B2) {
        port_config->host_no_of_lanes = multilane;
        port_config->line_no_of_lanes = 2 * multilane;
    } else {
        port_config->host_no_of_lanes = port_config->line_no_of_lanes = multilane;
    }

    port_config->speed = host_speed * port_config->host_no_of_lanes;

    unsigned optical_mask = 0;
    ERR_PROP(readReg(slice, REG_FW_OPTICAL, &optical_mask));
    if (optical_mask & (1 << line_start_lane)) {
        port_config->flags |= CR_PFLAG_LINE_SIDE_OPTICAL;
    }
    if (optical_mask & (1 << host_start_lane)) {
        port_config->flags |= CR_PFLAG_SYS_SIDE_OPTICAL;
    }

    unsigned lt_on = 0, an_on = 0;
    ERR_PROP(bh_fw_get_anlt(slice, line_start_lane, &an_on, &lt_on));
    if (lt_on && an_on) {
        port_config->flags |= CR_PFLAG_LINE_SIDE_ANLT;
    } else if (lt_on) {
        port_config->flags |= CR_PFLAG_LINE_SIDE_LT;
    } else if (an_on) {
        port_config->flags |= CR_PFLAG_LINE_SIDE_AN;
    }

    unsigned an_mode = 0;
    ERR_PROP(bh_fw_get_an_mode(slice, line_start_lane, &an_mode));
    if (an_on && an_mode == MODE_AN_D_OVERRIDEN) {
        port_config->flags |= CR_PFLAG_AUTONEG_OVERRIDE;
    }
    bh_slice->port_info[port_id].configured = true;
    bh_slice->port_info[port_id].port_config = *port_config;
    return CR_OK;
}

CredoError_t bh_port_link_state_internal(CredoSlice_t* slice, const CredoPortConfig_t* port_config, unsigned phy_rdy,
                                         CredoSide_t side, int* link) {
    *link = 1;
    unsigned start_lane = (side == CR_SIDE_HOST) ? port_config->host_start_lane : port_config->line_start_lane;
    unsigned lane_count = (side == CR_SIDE_HOST) ? port_config->host_no_of_lanes : port_config->line_no_of_lanes;
    for (int i = 0; i < lane_count; i++) {
        if (((phy_rdy >> (start_lane + i)) & 0x1) == 0) {
            *link = 0;
            return CR_OK;
        }
    }

    unsigned state = 0;
    if (port_config->connection_mode == CR_PORT_GEARBOX) {
        ERR_PROPS(bh_fw_get_gearbox_state(slice, start_lane, &state));
        if (state != FW_GB_STATE_DONE) *link = 0;
    } else if (port_config->connection_mode == CR_PORT_RETIMER) {
        ERR_PROPS(bh_fw_get_retimer_state(slice, start_lane, &state));
        if (state != FW_RT_STATE_TRACK) *link = 0;
    } else if (port_config->connection_mode == CR_PORT_BITMUX) {
        ERR_PROPS(bh_fw_get_bitmux_state(slice, start_lane, &state));
        if (state != FW_BM_STATE_DONE) *link = 0;
    } else {
        *link = 0;
        LOGS_ERROR("[Port link] Internal ERROR");
        return CR_FAIL;
    }

    return CR_OK;
}

CredoError_t bh_port_link_state(CredoSlice_t* slice, unsigned port_id, int link[]) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));
    if (port_config.port_id == CR_PORT_UNCONFIGURED) {
        link[0] = link[1] = 0;
        return CR_OK;
    }

    unsigned rdy = 0;
    ERR_PROPS(bh_fw_phy_ready(slice, &rdy));

    ERR_PROPS(bh_port_link_state_internal(slice, &port_config, rdy, CR_SIDE_HOST, link + 0));
    ERR_PROPS(bh_port_link_state_internal(slice, &port_config, rdy, CR_SIDE_LINE, link + 1));

    return CR_OK;
}

CredoError_t bh_port_link_state_host(CredoSlice_t* slice, unsigned port_id, int* link) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));
    if (port_config.port_id == CR_PORT_UNCONFIGURED) {
        *link = 0;
        return CR_OK;
    }

    unsigned rdy = 0;
    ERR_PROPS(bh_fw_phy_ready(slice, &rdy));

    ERR_PROPS(bh_port_link_state_internal(slice, &port_config, rdy, CR_SIDE_HOST, link));

    return CR_OK;
}

CredoError_t bh_port_link_state_line(CredoSlice_t* slice, unsigned port_id, int* link) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));
    if (port_config.port_id == CR_PORT_UNCONFIGURED) {
        *link = 0;
        return CR_OK;
    }

    unsigned rdy = 0;
    ERR_PROPS(bh_fw_phy_ready(slice, &rdy));

    ERR_PROPS(bh_port_link_state_internal(slice, &port_config, rdy, CR_SIDE_LINE, link));

    return CR_OK;
}

CredoError_t bh_port_is_link_up(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* up) {
    int link;
    if (side == CR_SIDE_HOST) {
        ERR_PROPS(bh_port_link_state_host(slice, port_id, &link));
        *up = (link != 0);
    } else {
        ERR_PROPS(bh_port_link_state_line(slice, port_id, &link));
        *up = (link != 0);
    }
    return CR_OK;
}
