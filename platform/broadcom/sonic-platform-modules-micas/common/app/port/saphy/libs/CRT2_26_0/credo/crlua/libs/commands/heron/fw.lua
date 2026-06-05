local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local timedelta = require "timedelta"
local crutil = require "crutil"

slash.register_chip_command({"fw", "exit_codes"}, credo.FAMILY_HERON, [[
Display firmware exit codes.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_exit_codes")
end)

slash.register_chip_command({"fw", "reg", "dump"}, credo.FAMILY_HERON, [[
Display firmware register information.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_reg")
end)

slash.register_chip_command({"fw", "cmdlogs"}, credo.FAMILY_HERON, [[
Display the firmware command logs.
]], function(slice, argt)
    local INFO_TOP = 9
    local DUMP_TOP_FW_CMD_LOG_CNT = 9
    local DUMP_TOP_FW_CMD_LOG = 5000
    local DUMP_TOP_FW_CMD_TIME = 6000
    local DUMP_TOP_FW_CMD_SEC = 6200

    local count = credo.firmware_debug_cmd(slice, 0, INFO_TOP, DUMP_TOP_FW_CMD_LOG_CNT)
    local cmd_time = list.map(range(DUMP_TOP_FW_CMD_TIME, DUMP_TOP_FW_CMD_TIME + count - 1), function(i)
        return credo.firmware_debug_cmd(slice, 0, INFO_TOP, i)
    end)
    local cmd_sec = list.map(range(DUMP_TOP_FW_CMD_SEC, DUMP_TOP_FW_CMD_SEC + count - 1), function(i)
        return credo.firmware_debug_cmd(slice, 0, INFO_TOP, i)
    end)
    local ftable = fort.create()
    ftable:print_ln("id|cmd|det|det2|ext0|ext1|ext2|ext3|timestamp")
    ftable:add_separator()
    for i = 0, count - 1 do
        local vals = list.map(range(DUMP_TOP_FW_CMD_LOG + i * 7, DUMP_TOP_FW_CMD_LOG + i * 7 + 7), function(j)
            return credo.firmware_debug_cmd(slice, 0, INFO_TOP, j)
        end)
        local ms = (cmd_time[i + 1] + cmd_sec[i + 1] * 2980) // 2.98
        ftable:print_ln("%-4d|%04x|%04x|%04x|%04x|%04x|%04x|%04x|%s" %
                            {
                i, vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], timedelta.pretty(ms / 1000, true)
            })

    end
    ftable:set_cell_prop(fort.ANY_ROW, 9, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    ftable:set_cell_prop(fort.ANY_ROW, 10, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    print(ftable)
end)

local DUMP_FW_OPT_STATE_CNT = 4
local DUMP_FW_OPT_STATE_CNT_CLEAR = 6
local DUMP_FW_OPT_STATE_TIME = 300
local DUMP_FW_OPT_STATE_TIME_SEC = 500
local DUMP_FW_OPT_STATE_ID = 350
-- local DUMP_TOP_SYS_UP_TIME = 2

local states = {
    VULTURE_STATE_END = 19,
    VULTURE_STATE_MODE_CHANGED = 0,
    VULTURE_STATE_WAIT_SIGNAL_DETECTED = 1,
    VULTURE_STATE_INIT_SETUP = 2,
    VULTURE_STATE_LINKUP = 13,
    VULTURE_STATE_BACKGROUND_START = 15,
    VULTURE_STATE_FIFO_ADJUST = 16,
    VULTURE_STATE_TX_ENABLE = 17,
    VULTURE_STATE_LINK_DROPPED = 18,

    VULTURE_STATE_WAIT_ANLT_DONE = 20,
    VULTURE_STATE_AN = 21,
    VULTURE_STATE_DO_AN = 22,
    VULTURE_STATE_LT = 23,

    TX_STATE_POWER_DOWN = 0x4F,
    TX_STATE_NORMAL_QUIET = 0x40,
    TX_STATE_NORMAL_PRBS_PAM4 = 0x41,
    TX_STATE_NORMAL_PRBS_NRZ = 0x42,
    TX_STATE_NORMAL_DATA = 0x43,
    TX_STATE_NORMAL_USER_PATTERN = 0x44,
    TX_STATE_FORCE_QUIET = 0x48,
    TX_STATE_FORCE_PRBS_PAM4 = 0x49,
    TX_STATE_FORCE_PRBS_NRZ = 0x4A,
    TX_STATE_FORCE_DATA = 0x4B,
    TX_STATE_FORCE_USER_PATTERN = 0x4C,

    STATE_LT_INITIALIZE = 0xAF,
    STATE_LT_START = 0xB0,
    STATE_LT_PAM2_START = 0xC0,
    STATE_LT_PAM2_OPT_ENV = 0xC2,
    STATE_LT_PAM2_OPT_ENV1 = 0xC3,
    STATE_LT_PAM2_LOAD_SETTING = 0xC4,
    STATE_LT_PAM2_WAIT_FRAME_LOCK = 0xD0,
    STATE_LT_PAM2_WAIT_REMOTE_LOCK = 0xD4,
    STATE_LT_PAM2_WAIT_INITIAL_CMD = 0xD8,
    STATE_LT_PAM2_WAIT_LINK = 0xE8,

    STATE_LT_PAM4_START = 0x70,
    STATE_LT_PAM4_WAIT_FRAME_LOCK = 0x7C,
    STATE_LT_PAM4_WAIT_PAM4_FRAME = 0x80,
    STATE_LT_PAM4_ENTER_PAM4 = 0x82,
    STATE_LT_PAM4_RESPONSE_PAM4_MOD = 0x83,
    STATE_LT_PAM4_TX_ADJUST = 0x88,
    STATE_LT_PAM4_WAIT_DATA_READY = 0x8e,
    STATE_LT_PAM4_TX_ADJUST_DONE = 0x8C,
    STATE_LT_PAM4_WAIT_LP_READY = 0x90,
    STATE_LT_PAM4_RX_TRAINED = 0x91,
    STATE_LT_PAM4_LP_READY = 0x92,
    STATE_LT_PAM4_SWITCH_TO_PRBS = 0x93,
    STATE_LT_END = 0x9F,

    BITMUX_STATE_TO_WAIT_PHY = 0x30,
    BITMUX_STATE_TO_WAIT_LT = 0x31,
    BITMUX_STATE_TO_RELEASE_FIFO = 0x32,
    BITMUX_STATE_TO_FUNCTIONAL = 0x33,
    BITMUX_STATE_TO_DONE = 0x34,
    BITMUX_STATE_TO_RESET = 0x36
}

local states_str = {
    [states.VULTURE_STATE_MODE_CHANGED] = 'mode_changed',
    [states.VULTURE_STATE_WAIT_SIGNAL_DETECTED] = 'wait_signal_detect',
    [states.VULTURE_STATE_INIT_SETUP] = 'init_setup',

    [states.VULTURE_STATE_LINKUP] = 'linkup',
    [states.VULTURE_STATE_BACKGROUND_START] = 'background_start',
    [states.VULTURE_STATE_FIFO_ADJUST] = 'fifo_adjust',
    [states.VULTURE_STATE_TX_ENABLE] = 'tx_enable',
    [states.VULTURE_STATE_LINK_DROPPED] = 'Link dropped',

    [states.VULTURE_STATE_WAIT_ANLT_DONE] = 'ANLT done',
    [states.VULTURE_STATE_AN] = 'AN',
    [states.VULTURE_STATE_DO_AN] = 'Do AN',
    [states.VULTURE_STATE_LT] = 'LT',

    [states.TX_STATE_POWER_DOWN] = 'tx power down',
    [states.TX_STATE_NORMAL_QUIET] = 'tx normal quiet',
    [states.TX_STATE_NORMAL_PRBS_PAM4] = 'tx normal prbs pam4',
    [states.TX_STATE_NORMAL_PRBS_NRZ] = 'tx normal prbs nrz',
    [states.TX_STATE_NORMAL_DATA] = 'tx normal data',
    [states.TX_STATE_NORMAL_USER_PATTERN] = 'tx normal user patter',
    [states.TX_STATE_FORCE_QUIET] = 'tx force quiet',
    [states.TX_STATE_FORCE_PRBS_PAM4] = 'tx force prbs pam4',
    [states.TX_STATE_FORCE_PRBS_NRZ] = 'tx force prbs nrz',
    [states.TX_STATE_FORCE_DATA] = 'tx force data',
    [states.TX_STATE_FORCE_USER_PATTERN] = 'tx force prbs pam4',

    [states.STATE_LT_INITIALIZE] = 'LT Initialize',
    [states.STATE_LT_START] = 'LT start',
    [states.STATE_LT_PAM2_START] = 'LT pam2 start',
    [states.STATE_LT_PAM2_OPT_ENV] = 'LT pam2 opt env',
    [states.STATE_LT_PAM2_OPT_ENV1] = 'LT pam2 opt env1',
    [states.STATE_LT_PAM2_LOAD_SETTING] = 'LT pam2 load setting',

    [states.STATE_LT_PAM2_WAIT_FRAME_LOCK] = 'LT pam2 wait frame lock',
    [states.STATE_LT_PAM2_WAIT_REMOTE_LOCK] = 'LT pam2 wait remote lock',
    [states.STATE_LT_PAM2_WAIT_INITIAL_CMD] = 'LT pam2 wait initial cmd',

    [states.STATE_LT_PAM4_START] = 'LT pam4 start',
    [states.STATE_LT_PAM4_WAIT_FRAME_LOCK] = 'LT pam4 wait frame lock',
    [states.STATE_LT_PAM4_WAIT_PAM4_FRAME] = 'LT pam4 wait frame',
    [states.STATE_LT_PAM4_ENTER_PAM4] = 'LT pam4 enter pam4',
    [states.STATE_LT_PAM4_RESPONSE_PAM4_MOD] = 'LT response mode_req',
    [states.STATE_LT_PAM4_WAIT_DATA_READY] = 'LT wait data ready',
    [states.STATE_LT_PAM4_TX_ADJUST] = 'LT pam4 tx adjust',
    [states.STATE_LT_PAM4_TX_ADJUST_DONE] = 'LT pam4 tx adjust done',
    [states.STATE_LT_PAM4_WAIT_LP_READY] = 'LT wait LP ready',
    [states.STATE_LT_PAM4_RX_TRAINED] = 'LT Rx trained',
    [states.STATE_LT_PAM4_LP_READY] = 'LT LP ready',
    [states.STATE_LT_PAM4_SWITCH_TO_PRBS] = 'LT switch to PAM4 PRBS',
    [states.STATE_LT_END] = 'LT Exit',

    [states.BITMUX_STATE_TO_WAIT_PHY] = 'bm to wait phy',
    [states.BITMUX_STATE_TO_WAIT_LT] = 'bm to wait LT',
    [states.BITMUX_STATE_TO_RELEASE_FIFO] = 'bm to release fifo',
    [states.BITMUX_STATE_TO_FUNCTIONAL] = 'bm to functional',
    [states.BITMUX_STATE_TO_DONE] = 'bm to done',
    [states.BITMUX_STATE_TO_RESET] = 'bm to reset'
}

slash.register_chip_command({"fw", "statelogs"}, credo.FAMILY_HERON, [[
Display firmware state machine history logs.

    <lanes>    (lanelist)
    -c,--clear                  clear state logs (no printing)
]], function(slice, argt, argv)

    ---@type integer[]
    local lanes = argt.lanes

    if argt.clear then
        for lane in iter(lanes) do
            credo.firmware_debug_cmd(slice, lane, 0, DUMP_FW_OPT_STATE_CNT_CLEAR)
        end
        return
    end

    for lane in iter(lanes) do
        local state_cnt = credo.firmware_debug_cmd(slice, lane, 0, DUMP_FW_OPT_STATE_CNT)
        local state_time_stamp = list.map(range(0, state_cnt - 1), function(i)
            return credo.firmware_debug_cmd(slice, lane, 0, DUMP_FW_OPT_STATE_TIME + i)
        end)
        local state_id = list.map(range(0, state_cnt - 1), function(i)
            return credo.firmware_debug_cmd(slice, lane, 0, DUMP_FW_OPT_STATE_ID + i)
        end)
        local state_time_sec = list.map(range(0, state_cnt - 1), function(i)
            return credo.firmware_debug_cmd(slice, lane, 0, DUMP_FW_OPT_STATE_TIME_SEC + i)
        end)
        local state_time = {}
        for i = 1, #state_time_stamp do
            state_time_stamp[i] = state_time_stamp[i] + (state_time_sec[i] * 2980)
        end
        for i = 1, #state_time_stamp - 1 do
            if state_time_stamp[i + 1] == 0 and state_id[i + 1] == 0 then
                break
            end
            local t = state_time_stamp[i + 1] - state_time_stamp[i]
            state_time[i] = t // 2.9
        end
        state_time[#state_time_stamp] = 0

        local cumul_time = 0
        local ftable = fort.create()

        ftable:print_ln('State|ID|Time (ms)|Cumul. (ms)|Time Stamp')
        ftable:add_separator()

        for i, id in ipairs(state_id) do
            local state_str = states_str[id] or "?"
            cumul_time = cumul_time + state_time[i]
            ftable:print_ln("%s|0x%02X|%d|%d|%s" %
                                {
                    state_str, id, state_time[i], cumul_time, timedelta.pretty(state_time_stamp[i] / 2980, true)
                })
        end

        ftable:set_cell_prop(fort.ANY_ROW, 3, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        ftable:set_cell_prop(fort.ANY_ROW, 4, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        ftable:set_cell_prop(fort.ANY_ROW, 5, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        print("Lane %s (%d)" % {crutil.get_lane_id(slice, lane), lane})
        print(ftable)
    end
end)
