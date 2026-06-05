local se = require "chips.screaming_eagle"
local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local timedelta = require "timedelta"
local crutil = require "crutil"

local datacap = require "chips.screaming_eagle.datacap"

local state_mode_map = {[0x8] = "PAM4", [0x9] = "NRZ", [0x0] = ""}

local state_major_map = {
    [0x0] = "TOP",
    [0x1] = "OPT_STARTUP",
    [0x2] = "OPT_PRESET",
    [0x3] = "OPT_FFE_FLT",
    [0x4] = "OPT_CTLE_CP2",
    [0x5] = "OPT_F1_EYE",
    [0x6] = "OPT_CM1_EYE",
    [0x7] = "OPT_F0_EYE",
    [0x8] = "OPT_F0_SKEF_EYE",
    [0x9] = "OPT_GAIN",
    [0xA] = "ANLT",
    [0xB] = "LT",
    [0xF] = "OPT_LINKUP"
}

local state_minor_map = {
    [0x1] = "MODE_QUIET",
    [0x2] = "MODE_SQUELCH",
    [0x3] = "INIT_MODE",
    [0x4] = "INIT_NRZ",
    [0x5] = "INIT_PAM4",
    [0x6] = "INIT_AN",
    [0x7] = "START_PLL_CAL",
    [0x8] = "RX_DISABLE",
    [0x9] = "PLL_CAL_DONE",
    [0x10] = "START_NRZ",
    [0x11] = "START_PAM4",
    [0x12] = "START_AN",
    [0x13] = "START_NRZ_LT",
    [0x14] = "START_PAM4_LT",
    [0x15] = "SIGNAL_DETECT",
    [0x16] = "CHECK_CDR",
    [0x17] = "SD_RESET",
    [0x20] = "CH_EST",
    [0x21] = "INIT",
    [0x22] = "CAL_TABLE_RANGE",
    [0x23] = "ATTN_SEARCH",
    [0x24] = "DSP_STARTUP",
    [0x25] = "F0_ENV",
    [0x26] = "F0_VGA",
    [0x27] = "THDLY_ADAPT",
    [0x28] = "OPT_CM1",
    [0x29] = "CHECK_EYE_BALC",
    [0x2A] = "CHECK_CDR",
    [0x2B] = "CHECK_ANA_FE",
    [0x2C] = "CHECK_GAIN",
    [0x2D] = "GET_CANDIDATE",
    [0x2E] = "BEST_INDEX_BROKEN",
    [0x2F] = "OW",
    [0x30] = "ENTRY_SHORT_CH",
    [0x31] = "ENTRY_MIDDLE_CH",
    [0x32] = "ENTRY_LONG_CH",
    [0x33] = "CS_ADJ_PLUS",
    [0x34] = "CS_ADJ_MINUS",
    [0x35] = "THDLY_OW",
    [0x36] = "CS_GRP_SHRT_2_LNG",
    [0x37] = "CS_GRP_LNG_2_SHRT",
    [0x38] = "PPM_PATCH",
    [0x39] = "PPM_PATCH_CDR_UNLOCK",
    [0x40] = "FIND_CUR_IDX",
    [0x41] = "START_OPT",
    [0x42] = "OPT_F1_BY_CP1",
    [0x43] = "CP2_HIT_BOUNDARY",
    [0x44] = "CP2_DIR_CHANGE",
    [0x45] = "CP2_OUT_OF_RANGE",
    [0x46] = "F1_SATURATED",
    [0x47] = "GET_CANDIDATE",
    [0x48] = "PPM_PATCH_DONE",
    [0x49] = "PPM_PATCH_SIGNAL_LOST",
    [0x60] = "EYE_CHECK_RANGE",
    [0x61] = "EYE_START",
    [0x62] = "EYE_CAL_AVG",
    [0x63] = "EYE_CAL_MOVING_WINDOW",
    [0x64] = "EYE_DATA_ERROR",
    [0x65] = "EYE_BAD_RESULT",
    [0x70] = "ENV_ADJ_1",
    [0x71] = "ENV_ADJ_2",
    [0x72] = "VGA_PROTECTION",
    [0x73] = "CS_BY_ENV",
    [0x7F] = "FIN_ADJ",
    [0xB0] = "NRZ_START",
    [0xB1] = "NRZ_WAIT_FRAME_LOCK",
    [0xB2] = "NRZ_WAIT_REMOTE_LOCK",
    [0xB3] = "NRZ_WAIT_INITIAL_CMD",
    [0xB4] = "NRZ_ENTER_NRZ",
    [0xB5] = "NRZ_RESPONSE_NRZ_MOD",
    [0xB6] = "NRZ_TX_ADJUST",
    [0xB7] = "NRZ_END",
    [0xB8] = "NRZ_PHY_RDY",
    [0xB9] = "NRZ_LP_READY",
    [0xBA] = "NRZ_SWITCH_TO_PRBS",
    [0xC0] = "BP_C0",
    [0xC1] = "BP_C1",
    [0xC2] = "BP_C2",
    [0xC3] = "BP_C3",
    [0xC4] = "BP_C4",
    [0xF0] = "INIT",
    [0xF1] = "READ_EYE",
    [0xF2] = "READ_ISI",
    [0xF3] = "GET_FE_INFO",
    [0xF4] = "CHECK_SMALL_EYE",
    [0xF5] = "CHECK_EYE",
    [0xF6] = "TOP_RESET"
}

local OPT_DEBUG = 1
local OPT_DEBUG_STATE_CNT = 2000
local OPT_DEBUG_STATE_MAX_CNT = 2099
local OPT_DEBUG_STATE_REC = 2001
local OPT_DEBUG_STATE_CLEAR = 5002
local OPT_DEBUG_STATE_TIMESTAMP = 2100
local OPT_DEBUG_STATE = 0

local FW_TIME_UNIT = 0.08589934592
local FW_STATE_TIME_UNIT = ((FW_TIME_UNIT * 1000) / 256)

slash.register_chip_command({"lane", "statelogs"}, credo.FAMILY_NIGHTHAWK, [[
Display lane(s) state machine history logs.

    <lanes>    (lanelist)
    -c,--clear                  clear state logs (no printing)
]], function(slice, argt, argv)

    ---@type integer[]
    local lanes = argt.lanes

    if argt.clear then
        for lane in iter(lanes) do
            credo.firmware_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_CLEAR)
        end
        return
    end

    for lane in iter(lanes) do
        local state_cnt = credo.firmware_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_CNT)
        local state_max_cnt = credo.firmware_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_MAX_CNT)
        state_cnt = math.min(state_cnt, state_max_cnt) -- bound to maximum count
        ---@type number[]
        local state_time_stamp = list.map(range(0, state_cnt - 1), function(i)
            local msb = credo.firmware_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_TIMESTAMP + i * 2)
            local lsb = credo.firmware_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_TIMESTAMP + (i * 2) + 1)
            return ((msb << 16) | lsb) * FW_STATE_TIME_UNIT
        end)
        local state_id = list.map(range(0, state_cnt - 1), function(i)
            return credo.firmware_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_REC + i)
        end)
        local state_time = {}
        for i = 1, #state_time_stamp - 1 do
            state_time[i] = int(state_time_stamp[i + 1] - state_time_stamp[i])
        end
        state_time[#state_time_stamp] = 0

        local cumul_time = 0
        local ftable = fort.create()

        ftable:print_ln('Lane %s(%d) State|||||||Time|Cumulative|Time Stamp' % {crutil.get_lane_id(slice, lane), lane})
        ftable:print_ln("Idx|mode|ID|major|ID|minor|ID|(ms)|Duration|")
        ftable:add_separator()

        for i, id in ipairs(state_id) do
            local state_mode = (id >> 12) & 0xF
            local state_major = (id >> 8) & 0xF
            local state_minor = (id >> 0) & 0xFF
            local state_mode_str = state_mode_map[state_mode] or "?"
            local state_major_str = state_major_map[state_major] or "?"
            local state_minor_str = state_minor_map[state_minor] or "?"
            cumul_time = cumul_time + state_time[i]
            ftable:print_ln("%d|%s|%X|%s|%X|%s|%2X|%d|%s|%s" % {
                i, state_mode_str, state_mode, state_major_str, state_major, state_minor_str, state_minor,
                state_time[i], timedelta.pretty(cumul_time / 1000, true),
                timedelta.pretty(state_time_stamp[i] / 1000, true)
            })
        end

        ftable:set_cell_span(1, 1, 7)
        ftable:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        ftable:set_cell_prop(fort.ANY_ROW, 2, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_LEFT)
        ftable:set_cell_prop(fort.ANY_ROW, 4, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_LEFT)
        ftable:set_cell_prop(fort.ANY_ROW, 6, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_LEFT)
        ftable:set_cell_prop(1, 1, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_CENTER)

        -- get the current opt debug state
        local id = credo.firmware_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE)
        local state_mode = (id >> 12) & 0xF
        local state_major = (id >> 8) & 0xF
        local state_minor = (id >> 0) & 0xFF
        local state_mode_str = state_mode_map[state_mode] or "?"
        local state_major_str = state_major_map[state_major] or "?"
        local state_minor_str = state_minor_map[state_minor] or "?"

        local cur_time = credo.slice_get_param(slice, "sys_time", 0) --[[@as number]]
        ftable:add_separator()
        ftable:print_ln("now|%s|%X|%s|%X|%s|%2X|-|-|%s" % {
            state_mode_str, state_mode, state_major_str, state_major, state_minor_str, state_minor,
            timedelta.pretty(cur_time, true)
        })

        print(ftable)
    end
end)

slash.register_chip_command({"lane", "datalogs"}, credo.FAMILY_NIGHTHAWK, [[
Display lane(s) state machine history logs.

Arugments:

    <lanes>                    (lanelist)
    -f,--file                  (string)      file to output logs

]], function(slice, argt)
    local data = datacap.lane_data_logs(slice, argt)
    if data ~= nil then
        print(data)
    end
end)

local error_code_map = {
    [3] = "BYPASS",
    [2] = "F1_SATURATED",
    [1] = "SUCCESS",
    [0] = "FAIL",
    [-1] = "NO_SIGNAL",
    [-2] = "TIMEOUT_ERROR",
    [-3] = "PRESET_CAN_NOT_FOUND",
    [-4] = "SMALL_EYE_ERROR",
    [-5] = "RESET_ERROR",
    [-6] = "RESET_ISSUED",
    [-7] = "ADC_CAL_ERROR",
    [-8] = "OPT_CM1_ERROR",
    [-9] = "OPT_STACK_OVERFLOW",
    [-10] = "LINKUP_NO_SIGNAL",
    [-11] = "NO_VALID_PRESET",
    [-12] = "INVALID_PARAM",
    [-13] = "OUT_OF_RANGE",
    [-14] = "STARTUP_SIGNAL_LOSS",
    [-15] = "STARTUP_CDR_LOSS",
    [-16] = "OPT_F0_EYE_ERROR",
    [-17] = "LINKUP_TOP_RESET",
    [-18] = "OPT_ATTN_NOT_CONVERAGED",
    [-19] = "OPT_ENV_NOT_CONVERAGED",
    [-20] = "INVALID_VGA",
    [-21] = "CHL_EST_GLITCH",
    [-22] = "NO_IMP",
    [-23] = "LINKUP_CDR_LOSS",
    [-0xFF] = "LINK_DROP"
}

slash.register_chip_command({"lane", "exitlogs"}, credo.FAMILY_NIGHTHAWK, [[
Capture exit code information for a lane.

Exit codes are rendered from:

    newest
      |
      v
    oldest

Arguments:

    <lanes>        (lanelist default all)
]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes

    local ftable = fort.create()
    ftable:print_ln("Lane|Exit|Mode|Major|Minor|Raw|Error|Raw|Timestamp")

    for lane in iter(lanes) do
        local lane_id = crutil.get_lane_id(slice, lane)

        ftable:add_separator()
        local exit_codes = se.lane_get_exit_codes(slice, lane)
        list.reverse(exit_codes)

        local error_codes = se.lane_get_error_codes(slice, lane)
        list.reverse(error_codes)

        local exit_code_timestamps = se.lane_get_exit_code_timestamps(slice, lane)
        list.reverse(exit_code_timestamps)

        for idx, exit_code in ipairs(exit_codes) do
            local error_code = error_codes[idx] or 0
            local timestamp = exit_code_timestamps[idx] or 0

            ftable:print("%s(%2d)|%d" % {lane_id, lane, idx - 1})
            local state_mode = (exit_code >> 12) & 0xF
            local state_major = (exit_code >> 8) & 0xF
            local state_minor = (exit_code >> 0) & 0xFF
            local state_mode_str = state_mode_map[state_mode] or "?"
            local state_major_str = state_major_map[state_major] or "?"
            local state_minor_str = state_minor_map[state_minor] or "?"
            local error_str = error_code_map[error_code] or "?"

            if exit_code == 0x0 then
                state_mode_str, state_major_str, state_minor_str, error_str = "-", "-", "-", "-"
            end

            ftable:print("%s|%s|%s|0x%04X|%s|%d|%s" %
                             {
                    state_mode_str, state_major_str, state_minor_str, exit_code, error_str, error_code,
                    timedelta.pretty(timestamp, true)
                })
            ftable:ln()
        end
        ftable:add_separator()
    end

    ftable:set_cell_prop(fort.ANY_ROW, 9, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)

    print(ftable)

end)
