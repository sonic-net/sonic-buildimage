local slash = require "slash"
local credo = require "credo"
local crutil = require "crutil"
local fort = require "fort"
local timedelta = require "timedelta"

local bitops = require "int"

slash.register_chip_command({"anlt", "pages"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display AN exchanged pages

    <lanes>    (lanelist)
]], function(slice, argt, argv)
    for lane in iter(argt.lanes) do
        credo.display_info_print(slice, "anlt %d" % {lane})
    end
end)

slash.register_chip_command({"anlt", "timers"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display ANLT Timers
]], function(slice, argt, argv)
    credo.display_info_print(slice, "anlt_timer")
end)

slash.register_chip_command({"anlt", "status"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display ANLT status

    <lanes>    (lanelist default all)
]], function(slice, argt, argv)

    ---@type integer[]
    local lanes = argt.lanes

    local ftable = fort.create()
    ftable:print_ln("Lane|Mode|Main|AN State|AN Restarts|LT State|Raw|LT Status|LT Restarts")
    ftable:add_separator()
    for lane in iter(lanes) do
        local mode = credo.lane_get_mode(slice, lane)

        if not crutil.is_lane_configured(mode) then
            ftable:print_ln("%2d(%2s)|%s" % {lane, crutil.get_lane_id(slice, lane), mode.display_name})
            goto continue
        end

        local an_state = credo.autoneg_get_state(slice, lane)
        local lt_state = credo.link_training_get_state(slice, lane)
        local lt_status = credo.link_training_get_status(slice, lane)
        local an_restarts = credo.autoneg_get_restart_count(slice, lane)
        local lt_restarts = credo.link_training_get_restart_count(slice, lane)

        local lt_state_raw = credo.firmware_debug_cmd(slice, lane, 13, 16)
        local is_main_lane = an_state > credo.AUTONEG_STATE_OFF and credo.firmware_debug_cmd(slice, lane, 13, 7) == lane
        ftable:print_ln("%2d(%2s)|%s|%s|%s|%d|%s|0x%X|%s|%d" % {
            lane, crutil.get_lane_id(slice, lane), mode.display_name, choose(is_main_lane, "yes", ""),
            an_state.display_name, an_restarts, lt_state.display_name, lt_state_raw, lt_status.display_name, lt_restarts
        })
        ::continue::
    end
    print(ftable)
end)

local FW_TIME_UNIT = 0.08589934592
local FW_STATE_TIME_UNIT = ((FW_TIME_UNIT) / 256)

local MOD_PREC_FLAG = 0x200
local PRESETS_FLAG = 0x100
local LD_REQ = 1
local LD_STS = 2
local LP_REQ = 3
local LP_STS = 4
local RX_TRAINED_FLAG = 0x200

local coeff_sel_list = {'CM3', 'CM2', 'CM1', 'C0', 'C1'}
local coeff_sts_list = {
    'NOT_UPDATED', 'UPDATED', 'COEFF_LIMIT', 'COEFF_NOSUPPORT', 'EQ_LIMIT', 'RESERVED', 'COEFF_EQ_LIMIT', 'RESERVED'
}

local nrz_tap_list = {'CM1', 'C0', 'C1'}
local nrz_sts_list = {'NOP', 'UPD', 'MIN', 'MAX'}

local mod_prec_list = {'PAM2', 'RESERVED', 'PAM4', 'PAM4_PREC'}

local lt_c162_presets = {{0, 0, 0, 63, 0}, {0, 0, 0, 32, 0}, {0, 0, -5, 47, 0}, {0, 3, -12, 47, 0}, {-2, 4, -16, 41, 0}}
local lt_c136_presets = {{0, 0, 0, 63, 0}, {0, 0, -9, 47, -6}, {0, 0, -15, 48, 0}}
local lt_cl72_preset = {0, 0, 0, 62, 0}
local lt_cl72_init = {0, 0, -4, 42, -16}

slash.register_chip_command({"anlt", "logs"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display ANLT logs for given lanes.

Arguments:

    <lanes>            (lanelist)               Lanes to display logs

    -t,--timestamp                              Display timestamp
    -s,--size          (integer default 300)    Size of logs

Limitations:

    The tx taps + mode are computed during post processing and should be treated as a best effort
    to determine settings at a given point in time. In unsuccessful interops they may become misleading.
    It is best to use the REQ/STS information as that is a non-speculative report of what the firmware sees.

    The tx taps reported for LP also are assuming same SERDES as the local device. Since tx tap values and ranges
    can differ amongst SERDES types, you may need to translate the tap settings into the LP SERDES values.

Note:

    REQ: Requests by the LP/LD to its partner. Denoted by `R:`
    STS: Response Status by the LP/LD to requests. Denoted by `S:`

]], function(slice, argt, argv)
    print("IMPORTANT: Read help (-h) for detailed description + limitations")
    ---@type integer[]
    local lanes = argt.lanes

    local start_time = credo.time_start(slice)
    for lane in iter(lanes) do
        local lane_mode = credo.lane_get_mode(slice, lane)
        if not crutil.is_lane_configured(lane_mode) then
            return
        end

        local is_pam4 = (lane_mode == credo.LMODE_PAM4)
        local lane_speed = int(credo.lane_get_speed(slice, lane) // 1e6)

        ---@type integer[]
        local tx_init_cond = list.map(range(390, 394), function(i)
            return bitops.bin_to_signed(credo.firmware_debug_cmd(slice, lane, 13, i), 16)
        end)

        local log_size = argt.size
        local lt_logs = list.map(range(400, 400 + log_size * 2 - 1), function(i)
            return credo.firmware_debug_cmd(slice, lane, 13, i)
        end)

        local lt_start_time = credo.firmware_debug_cmd(slice, lane, 13, 19)
        local lt_start_time_upper = 0
        local show_full_timestamp = false
        -- in case upper is not available
        pcall(function()
            lt_start_time_upper = credo.firmware_debug_cmd(slice, lane, 13, 18, true) << 16
            show_full_timestamp = true
        end)

        local ftable = fort.new()
        ftable:print_ln("Lane %s(%d) %dG\nInit Cond: %s" %
                            {crutil.get_lane_id(slice, lane), lane, lane_speed, stringx.join(",", tx_init_cond)})
        ftable:add_separator()
        ftable:print_ln("|||LD|||LP||")
        ftable:print_ln("|||      Tx Taps    ||||      Tx Taps    |")
        ftable:print_ln(
            "Idx|Timestamp|Timedelta| -3  -2  -1   M   1 |Mode|LD REQ/STS|LP REQ/STS| -3  -2  -1   M   1 |Mode")
        ftable:add_separator()

        --- 0 index (to match python)
        ---@param lst string[]
        ---@param i integer
        ---@return string
        local function lget(lst, i)
            local val = lst[i + 1]
            if val ~= nil then
                return val
            end
            return "UNKNOWN(%d)" % {i} -- add the -1 to map to 0 index value
        end

        local ld_state = {taps = tx_init_cond, mode = "PAM2", taps_confused = false}
        local lp_state = {taps = tablex.copy(tx_init_cond), mode = "PAM2", taps_confused = false}

        ---@type nil|fun():nil
        local lp_req = nil
        ---@type nil|fun():nil
        local ld_req = nil

        local idx = 1
        local function map_tx_taps(taps, confused)
            local taps_str = list.map(taps, function(tap)
                return "%3d" % {tap}
            end)
            return stringx.join(",", taps_str) .. choose(confused, " ?", "")
        end

        local function add_row(timestamp, row)
            if timestamp < lt_start_time then
                timestamp = timestamp + 0x10000
            end
            timestamp = timestamp + lt_start_time_upper
            -- prevent tap confusion breaking everything
            if ld_state.taps == nil then
                ld_state.taps = {0, 0, 0, 0, 0}
                ld_state.taps_confused = true
            end
            if lp_state.taps == nil then
                lp_state.taps = {0, 0, 0, 0, 0}
                lp_state.taps_confused = true
            end

            local full_timestamp = "-"
            if show_full_timestamp then
                full_timestamp = timedelta.timestamp_iso8601(timestamp * FW_STATE_TIME_UNIT + start_time)
            end

            ftable:print("%d|%s|%s" % {idx, full_timestamp, timedelta.pretty(timestamp * FW_STATE_TIME_UNIT, true)})
            idx = idx + 1
            ftable:write(map_tx_taps(ld_state.taps, ld_state.taps_confused), ld_state.mode)
            ftable:row_write(row)
            ftable:write(map_tx_taps(lp_state.taps, lp_state.taps_confused), lp_state.mode)
            ftable:ln()
        end

        for i = 1, log_size do
            ---@type integer
            local details = lt_logs[2 * i - 1]
            ---@type integer[]
            local timestamp = lt_logs[2 * i]
            ---@type integer
            local log_type = details >> 12
            ---@type integer
            local content = details & 0xFFF

            if log_type == 0 then
                goto continue
            elseif log_type == LD_REQ then

                if is_pam4 then
                    local coeff_req_list = {
                        'CM2 NOEQ', 'CM2 DEC', 'CM2 INC', 'CM1 NOEQ', 'CM1 DEC', 'CM1 INC', 'C1 NOEQ', 'C1 DEC',
                        'C1 INC', 'C0 NOEQ', 'C0 DEC', 'C0 INC'
                    }

                    local req = "?"
                    if content & MOD_PREC_FLAG ~= 0 then
                        req = lget(mod_prec_list, (content & 0xF))
                        lp_state.mode = req
                    elseif content & PRESETS_FLAG ~= 0 then
                        local preset = 16 - (content & 0xF)
                        req = "PRESET%d" % {preset}
                        ld_req = function()
                            if lane_speed == 106 then
                                lp_state.taps = tablex.copy(lt_c162_presets[preset])
                            else
                                lp_state.taps = tablex.copy(lt_c136_presets[preset])
                            end
                        end
                    else
                        req = lget(coeff_req_list, content)
                        ld_req = function()
                            local coeff_req_idx_list = {2, 2, 2, 3, 3, 3, 5, 5, 5, 4, 4, 4}
                            local coeff_sel = coeff_req_idx_list[content + 1]
                            local coeff_req = content % 3
                            local val = lp_state.taps[coeff_sel]
                            if coeff_req == 0 then
                                val = 0
                            elseif coeff_req == 1 then
                                val = val - 1
                            else
                                val = val + 1
                            end
                            lp_state.taps[coeff_sel] = val
                        end
                    end
                    add_row(timestamp, {"R:" .. req, ''})
                else
                    local coeff_req_list = {
                        'UNKNOWN', 'UNKNOWN', 'UNKNOWN', 'C1 DEC', 'C1 INC', 'CM1 DEC', 'CM1 INC', 'C0 DEC', 'C0 INC',
                        'INIT', 'PRESET', 'HOLD'
                    }
                    ld_req = function()
                        local req = content
                        if req == 3 then
                            lp_state.taps[5] = lp_state.taps[5] - 1
                        elseif req == 4 then
                            lp_state.taps[5] = lp_state.taps[5] + 1
                        elseif req == 5 then
                            lp_state.taps[3] = lp_state.taps[3] - 1
                        elseif req == 6 then
                            lp_state.taps[3] = lp_state.taps[3] + 1
                        elseif req == 7 then
                            lp_state.taps[4] = lp_state.taps[4] - 1
                        elseif req == 8 then
                            lp_state.taps[4] = lp_state.taps[4] + 1
                        elseif req == 9 then -- init
                            lp_state.taps = tablex.copy(lt_cl72_init)
                        elseif req == 10 then -- preset
                            lp_state.taps = tablex.copy(lt_cl72_preset)
                        end
                    end
                    local req = lget(coeff_req_list, content)
                    add_row(timestamp, {"R:" .. req, ''})
                end
            elseif list.contains({LP_STS, LD_STS}, log_type) then
                local sts = "?"
                if is_pam4 then
                    content = content & 0x3FF

                    local ic_sts = (content >> 8) & 1
                    local coeff_sel = bitops.bin_to_signed((content >> 3) & 7, 3)
                    local coeff_sts = (content >> 0) & 7

                    if content == RX_TRAINED_FLAG then
                        sts = choose(log_type == LP_STS, 'RECEIVER READY', 'LOCAL READY')
                    elseif ic_sts ~= 0 then
                        sts = 'IC UPDATED'
                        if log_type == LD_STS and lp_req ~= nil then
                            lp_req()
                            lp_req = nil
                        elseif log_type == LP_STS and ld_req ~= nil then
                            ld_req()
                            ld_req = nil
                        end

                    else
                        sts = "%s %s" % {lget(coeff_sel_list, coeff_sel + 3), lget(coeff_sts_list, coeff_sts)}

                        if log_type == LD_STS and lp_req ~= nil then
                            -- only update if coeff updated

                            if coeff_sts == 1 then
                                lp_req()
                            end
                            lp_req = nil
                        elseif log_type == LP_STS and ld_req ~= nil then
                            -- only update if coeff updated
                            if coeff_sts == 1 then
                                ld_req()
                            end
                            ld_req = nil
                        end
                    end
                else
                    if content == RX_TRAINED_FLAG then
                        sts = choose(log_type == LP_STS, 'RECEIVER READY', 'LOCAL READY')
                    else
                        content = content & 0x3F
                        local sts_all = {}
                        for tap = -1, 1 do
                            local sts_val = (content >> (2 * (tap + 1))) & 3
                            list.append(sts_all, "%s %s" % {lget(nrz_tap_list, tap + 1), lget(nrz_sts_list, sts_val)})
                        end
                        sts = stringx.join(',', sts_all)
                        if log_type == LD_STS and lp_req ~= nil then
                            lp_req()
                            lp_req = nil
                        elseif log_type == LP_STS and ld_req ~= nil then
                            ld_req()
                            ld_req = nil
                        end
                    end
                end
                if log_type == LP_STS then
                    add_row(timestamp, {"", "S:" .. sts})
                else
                    add_row(timestamp, {"S:" .. sts, ""})
                end
            elseif log_type == LP_REQ then
                local req = "?"
                if is_pam4 then

                    if content & MOD_PREC_FLAG ~= 0 then
                        req = mod_prec_list[(content & 0xF) + 1]
                        ld_state.mode = req
                    elseif content & PRESETS_FLAG ~= 0 then
                        local preset = content & 0xF
                        req = "PRESET%d" % {preset}
                        lp_req = function()
                            if lane_speed == 106 then
                                ld_state.taps = tablex.copy(lt_c162_presets[preset])
                            else
                                ld_state.taps = tablex.copy(lt_c136_presets[preset])
                            end
                        end
                    else
                        local coeff_sel = bitops.bin_to_signed((content >> 2) & 7, 3)
                        local coeff_req = (content >> 0) & 3
                        local coeff_req_list = {'HOLD', 'INC', 'DEC', 'NOEQ'}

                        req = "%s %s" % {lget(coeff_sel_list, coeff_sel + 3), lget(coeff_req_list, coeff_req)}
                        lp_req = function()
                            local val = ld_state.taps[coeff_sel + 4]
                            if coeff_req == 3 then
                                val = 0
                            elseif coeff_req == 1 then
                                val = val + 1
                            elseif coeff_req == 2 then
                                val = val - 1

                            end
                            ld_state.taps[coeff_sel + 4] = val
                        end
                    end
                else
                    local req_all = {}
                    if content & (1 << 7) ~= 0 then
                        list.append(req_all, 'PRESET')
                        lp_req = function()
                            ld_state.taps = tablex.copy(lt_cl72_preset)
                        end
                    elseif content & (1 << 6) ~= 0 then
                        list.append(req_all, "INIT")
                        lp_req = function()
                            ld_state.taps = tablex.copy(lt_cl72_init)
                        end
                    else
                        local coeff_req_list = {'HOLD', 'INC', 'DEC', 'RSVD'}
                        for tap = -1, 1 do
                            local req_val = (content >> (2 * (tap + 1))) & 3
                            if req_val ~= 0 then
                                list.append(req_all,
                                            "%s %s" % {lget(nrz_tap_list, tap + 1), lget(coeff_req_list, req_val)})
                            end
                        end
                        lp_req = function()
                            for tap = -1, 1 do
                                local req_val = (content >> (2 * (tap + 1))) & 3
                                if req_val ~= 0 then
                                    local tap_val = ld_state.taps[tap + 4]
                                    if req_val == 1 then
                                        ld_state.taps[tap + 4] = tap_val + 1
                                    elseif req_val == 2 then
                                        ld_state.taps[tap + 4] = tap_val - 1
                                    end
                                end
                            end
                        end
                    end
                    req = stringx.join(",", req_all)
                end
                add_row(timestamp, {'', "R:" .. req})
            else
                add_row(timestamp, {"UNKNOWN(%d)" % {details}, ''})
            end

            ::continue::
        end

        ftable:set_cell_span(1, 1, 9)
        ftable:set_cell_span(2, 4, 3)
        ftable:set_cell_span(2, 7, 3)
        print(ftable)
    end

end)
