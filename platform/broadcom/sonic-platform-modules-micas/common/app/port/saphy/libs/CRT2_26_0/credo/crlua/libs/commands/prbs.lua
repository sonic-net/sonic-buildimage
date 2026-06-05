local slash = require "slash"
local credo = require "credo"
local fort = require "fort"

local crutil = require "crutil"

slash.register_command({"prbs", "config"}, [[
Set prbs configuration.

Allows setting both the tx generator and rx checker or just one of them.

Arguments:

    -d, --direction    (prbs_direction default both)    Set the tx gen/rx checker/both directions
    <lanes>            (lanelist)                       Lanes to set
    <pattern>          (prbs_pattern)                   Prbs pattern to configure
    --tx-checker                                        Configure tx checker

Examples:

    > prbs config all prbs31        # configure all lanes prbs31 (tx gen, rx check)
    > prbs config all -d tx prbs    # configure all lanes prbs31 (tx gen only)

]], function(slice, argt)
    ---@type credo.PrbsPattern
    local pattern = argt.pattern
    ---@type integer[]
    local lanes = argt.lanes
    ---@type credo.PrbsDirection
    local direction = argt.direction

    for lane in iter(lanes) do
        local lmode = credo.lane_get_mode(slice, lane)
        if not crutil.is_lane_configured(lmode) then
            goto end_loop
        end
        if direction == "both" or direction == "tx" then
            credo.prbs_set_tx_generator(slice, lane, true, pattern)
        end
        if direction == "both" or direction == "rx" then
            credo.prbs_set_rx_checker(slice, lane, true, pattern)
        end
        if argt.tx_checker then
            credo.prbs_set_tx_checker(slice, lane, true, pattern)
        end
        ::end_loop::
    end
end)

slash.register_command({"prbs", "destroy"}, [[
Destroy PRBS configuration.

Allows destroying both the tx generator and rx checker or just one of them.

Arguments:

    <lanes>            (lanelist)                       Lanes to destroy
    -d, --direction    (prbs_direction default both)    Destroy the tx gen/rx checker/both directions
    --tx-checker                                        Destroy tx checker

Examples:

    > prbs destroy all          # destroy all lanes (tx gen, rx check)
    > prbs destroy all -d tx    # destroy all lanes (tx gen only)

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes
    ---@type credo.PrbsDirection
    local direction = argt.direction

    ---@type integer
    for lane in iter(lanes) do
        if direction == "both" or direction == "tx" then
            credo.prbs_set_tx_generator(slice, lane, false, credo.PRBS31)
        end
        if direction == "both" or direction == "rx" then
            credo.prbs_set_rx_checker(slice, lane, false, credo.PRBS31)
        end

        if argt.tx_checker then
            credo.prbs_set_tx_checker(slice, lane, false, credo.PRBS31)
        end
    end
end)

slash.register_command({"prbs", "reset"}, [[
Reset prbs timer and error count.

Arguments:

    <lanes>    (lanelist)    lanes to reset
    --tx-checker             reset tx checker
]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes
    ---@type integer
    for lane in iter(lanes) do
        local lmode = credo.lane_get_mode(slice, lane)
        if crutil.is_lane_configured(lmode) then
            credo.prbs_reset_rx_count(slice, lane)
        end
        if argt.tx_checker then
            credo.prbs_reset_tx_count(slice, lane)
        end
    end
end)

slash.register_command({"prbs", "generror"}, [[
Generate an error on the prbs tx generator.

Arguments:

    <lanes>       (lanelist)             lanes to generate error
    -c,--count    (integer default 1)    number of errors to generate
]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes
    ---@type integer
    assert(argt.count > 0, "Count must be greater than 0")
    for lane in iter(lanes) do
        local lmode = credo.lane_get_mode(slice, lane)
        if crutil.is_lane_configured(lmode) then
            for _ = 1, argt.count do
                credo.prbs_generate_tx_error(slice, lane)
            end
        end
    end
end)

slash.register_command({"prbs", "status"}, [[
Get PRBS status information.

Arguments:

    -t, --time    (time default 0s)         Duration to capture info, default is global
    <lanes>       (lanelist default all)    Lanes to get

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes
    ---@type integer
    local duration = argt.time

    ---@class PrbsInfoData
    ---@field public lane_mode credo.LaneMode
    ---@field public speed number
    ---@field public tx_mode? credo.TestPatternMode
    ---@field public rx_mode? credo.TestPatternMode
    ---@field public error_count integer|nil
    ---@field public error_duration number|nil
    ---@field public rx_lock credo.PrbsLockStatus|nil
    ---@field public rx_link boolean
    ---@type PrbsInfoData[]
    local prbs_data = {}
    for lane in iter(lanes) do
        ---@type PrbsInfoData
        local lane_data = {
            lane_mode = credo.lane_get_mode(slice, lane),
            speed = credo.lane_get_speed(slice, lane),
            rx_link = credo.serdes_get_phy_ready(slice, lane)
        }
        local lane_enable = crutil.is_lane_configured(lane_data.lane_mode)

        local rx_enable = false
        if lane_enable then
            local rx_mode
            local tx_enable, tx_mode = credo.prbs_get_tx_generator(slice, lane)
            lane_data.tx_mode = choose(tx_enable, tx_mode, false)
            rx_enable, rx_mode = credo.prbs_get_rx_checker(slice, lane)
            lane_data.rx_mode = choose(rx_enable, rx_mode, false)
        end
        -- show only
        if lane_enable and lane_data.rx_link then
            lane_data.error_count = choose(rx_enable, credo.prbs_get_rx_count(slice, lane), false)
            lane_data.error_duration = choose(rx_enable, credo.prbs_get_rx_duration(slice, lane) / 1000, false)
            lane_data.rx_lock = nil
            -- not all chips support prbs lock
            if rx_enable then
                local ok, rx_lock = pcall(function()
                    return credo.prbs_get_rx_lock(slice, lane)
                end)
                if ok then
                    lane_data.rx_lock = rx_lock
                end
            end
        else
            lane_data.error_count = nil
            lane_data.error_duration = nil
        end
        list.append(prbs_data, lane_data)
    end

    if duration > 0 then
        time.sleep(duration)
        for full_data in iter(zip(lanes, prbs_data)) do
            ---@type integer
            local lane = full_data[1]
            ---@type PrbsInfoData
            local lane_data = full_data[2]
            ---@type PrbsInfoData

            if lane_data.error_duration then
                lane_data.error_duration = duration
            end
            if lane_data.error_count then
                lane_data.error_count = credo.prbs_get_rx_count(slice, lane) - lane_data.error_count
                if lane_data.error_count < 0 then
                    lane_data.error_count = lane_data.error_count + (1 << 32)
                end
            end
        end
    end
    local ftable = fort.create_table()

    fort.printf_ln(ftable, "Lane|Mode|Tx Gen|Rx Chk|Rx Link|Rx Lock|Err Dur|Err Count|BER")
    for full_data in iter(zip(lanes, prbs_data)) do
        ---@type integer
        local lane = full_data[1]
        ---@type PrbsInfoData
        local lane_data = full_data[2]

        local tx_mode_str = "OFF"
        if lane_data.tx_mode then
            tx_mode_str = lane_data.tx_mode.display_name
        end
        local rx_mode_str = "OFF"
        if lane_data.rx_mode then
            rx_mode_str = lane_data.rx_mode.display_name
        end
        local err_count_str = "-"
        if lane_data.error_count then
            err_count_str = "%d" % {lane_data.error_count}
        end
        local rx_lock_str = "-"
        if lane_data.rx_lock and lane_data.rx_lock ~= credo.PRBS_LOCK_INVALID then
            rx_lock_str = lane_data.rx_lock.display_name
        end
        local err_duration_str = "-"
        if lane_data.error_duration then
            err_duration_str = "%.1f" % {lane_data.error_duration}
        end
        local ber_str = "-"
        if lane_data.error_duration then
            local ber_val = lane_data.error_count / (lane_data.speed * 1e3 * lane_data.error_duration)
            ber_str = "%.3e" % {ber_val}
        end
        local lmode_str = lane_data.lane_mode.display_name

        fort.printf_ln(ftable, "%s(%d)|%s|%s|%s|%s|%s|%s|%s|%s", crutil.get_lane_id(slice, lane), lane, lmode_str,
                       tx_mode_str, rx_mode_str, choose(lane_data.rx_link, "UP", "DOWN"), rx_lock_str, err_duration_str,
                       err_count_str, ber_str)
    end
    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    --- error count align right
    fort.set_cell_prop(ftable, fort.ANY_ROW, 7, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    fort.set_cell_prop(ftable, fort.ANY_ROW, 8, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    fort.set_cell_prop(ftable, fort.ANY_ROW, 9, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)

    print(ftable)
end)
