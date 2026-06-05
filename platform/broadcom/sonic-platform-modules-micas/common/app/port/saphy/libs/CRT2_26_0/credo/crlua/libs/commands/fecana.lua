local slash = require "slash"
local credo = require "credo"
local fort = require "fort"

local crutil = require "crutil"

slash.register_command({"fecana", "config"}, [[
Set prbs fec analyzer configuration.

It is best to not set codeword flag so it can choose based upon lane mode type.

It will skip lanes not configured and who do not have the rx prbs checker configured.

Arguments:

    -e,--error_type    (fec_error default frame)    Error type
    -c,--codeword      (optional integer)           DEBUG: Codeword size, default nrz=5280 pam4=5440.
    -t,--threshold     (optional integer)           Threshold for errors, default nrz=7 pam4=15
    -s,--symbol        (integer default 10)         Symbol size
    <lanes>            (lanelist)                   Lanes to set

Examples:

    > fecana config all                 # configure all to default settings based on if nrz or pam4
    > fecana config 0-7 -e bit -t 12    # configure lanes 0-8 for bit fec errors with 12 threshold
    > fecana config 0-7 -e bit -t 12    # configure lanes 0-8

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes

    for lane in iter(lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        if mode == credo.LMODE_OFF then
            goto continue
        end
        local prbs_en, _ = credo.prbs_get_rx_checker(slice, lane)
        -- if prbs rx checker is not configured, we will skip it
        if not prbs_en then
            goto continue
        end
        if mode == credo.LMODE_PAM4 then
            local config = credo.FecAnalyzerConfig()
            config.error_type = argt.error_type
            config.codeword_size = argt.codeword or 5440
            config.symbol_size = argt.symbol
            config.threshold = argt.threshold or 15
            credo.fecana_configure(slice, lane, true, config)
        elseif mode == credo.LMODE_NRZ then
            local config = credo.FecAnalyzerConfig()
            config.error_type = argt.error_type
            config.codeword_size = argt.codeword or 5280
            config.symbol_size = argt.symbol
            config.threshold = argt.threshold or 7
            credo.fecana_configure(slice, lane, true, config)
        end
        ::continue::
    end
end)

slash.register_command({"fecana", "destroy"}, [[
Destroy prbs fec analyzer configuration.

Arguments:

    <lanes>    (lanelist)    Lanes to destroy fec analy/prbszer

Example

    > fecana destroy all
    > fecana destroy 0-7

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes

    for lane in iter(lanes) do
        local config = credo.FecAnalyzerConfig()
        credo.fecana_configure(slice, lane, false, config)
    end
end)

slash.register_command({"fecana", "reset"}, [[
Reset prbs fec analyzer configuration.

- Clear counters
- Reset timers

Arguments:

    <lanes>    (lanelist)    Lanes to reset

Examples:

    > /fecana reset all
    > /fecana reset 0-7

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes

    for lane in iter(lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        if crutil.is_lane_configured(mode) then
            local enable, _ = credo.fecana_query(slice, lane)
            if enable then
                credo.fecana_reset(slice, lane)
            end
        end
    end
end)

slash.register_command({"fecana", "info"}, [[
Get fecana configuration.

Arguments:

    <lanes>    (lanelist default all)    Lanes to check

Examples:

    > fecana info
    > fecana info 0-7

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes
    local ftable = fort.create_table()
    fort.printf_ln(ftable, "Lane|Mode|Enabled|Error Type|Threshold|Codeword|Symbol")
    for lane in iter(lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        local mode_name = mode.display_name
        if not crutil.is_lane_configured(mode) then
            fort.printf_ln(ftable, "%d|%s", lane, mode_name)
        else
            local enable, config = credo.fecana_query(slice, lane)
            if not enable then
                fort.printf_ln(ftable, "%d|%s|NO", lane, mode_name)
            else
                fort.printf_ln(ftable, "%d|%s|YES|%s|%d|%d|%d", lane, mode_name, config.error_type.display_name,
                               config.threshold, config.codeword_size, config.symbol_size)
            end
        end
    end
    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    print(ftable)
end)

slash.register_command({"fecana", "status"}, [[
Get fecana status.

Arguments:

    <lanes>    (lanelist default all)   Lanes to get status

Examples:

    > fecana status
    > fecana status 0-7

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes
    local ftable = fort.create_table()
    fort.printf_ln(ftable, "Lane|Mode|FEC\nEn|Timer|Pre-FEC\nCount|Post-FEC\nCount|Pre-FEC\nBER|Post-FEC\nBER")
    for lane in iter(lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        local mode_name = mode.display_name
        if not crutil.is_lane_configured(mode) then
            fort.printf_ln(ftable, "%d|%s", lane, mode_name)
        else
            local enable = credo.fecana_query(slice, lane)
            if not enable then
                fort.printf_ln(ftable, "%d|%s|NO", lane, mode_name)
            else
                local duration = credo.fecana_get_duration(slice, lane)
                local pre_fec, post_fec = credo.fecana_get_counter(slice, lane)
                local pre_fec_ber = credo.fecana_get_error_rate(slice, lane, 4, 0)
                local post_fec_ber = credo.fecana_get_error_rate(slice, lane, 6, 0)
                fort.printf_ln(ftable, "%d|%s|YES|%.1f|%d|%d|%.3e|%.3e", lane, mode_name, duration / 1000, pre_fec,
                               post_fec, pre_fec_ber, post_fec_ber)
            end
        end
    end
    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    fort.set_cell_prop(ftable, fort.ANY_ROW, 4, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    fort.set_cell_prop(ftable, fort.ANY_ROW, 5, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    fort.set_cell_prop(ftable, fort.ANY_ROW, 6, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    print(ftable)
end)

slash.register_command({"fecana", "histogram"}, [[
Get a PRBS fec analyzer histogram.

This is an imperfect capture as we can only capture errors 4 at a time per lane. This means the error captures are from
different periods of time.

Arguments:

    <lanes>          (lanelist default all)    lanes to capture errors
    -d,--duration    (time default 1s)         how long to capture symbol errors per group
    -p,--precise                               show full counters even if very large

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes

    ---@type table<integer, integer[]>
    local data = {}

    ---@type integer[]
    local fec_lanes = {}
    for lane in iter(lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        if not crutil.is_lane_configured(mode) then
            goto continue
        end
        local enabled = credo.fecana_query(slice, lane)
        if not enabled then
            goto continue
        end
        list.append(fec_lanes, lane)
        ::continue::
    end

    for group = 0, 3 do
        for lane in iter(fec_lanes) do
            credo.fecana_set_hist_group(slice, lane, group)
        end
        time.sleep(argt.duration)
        for lane in iter(fec_lanes) do
            local counters = credo.fecana_get_hist_counter(slice, lane)
            if data[lane] == nil then
                data[lane] = counters
            else
                list.extend(data[lane], counters)
            end
        end
    end

    local ftable = fort.create()

    ftable:print("SEI")
    for lane in iter(lanes) do
        ftable:print("%s(%02d)" % {crutil.get_lane_id(slice, lane), lane})
    end
    ftable:ln()
    ftable:add_separator()
    for error_count = 0, 15 do
        ftable:print(">= %d" % {error_count})
        for lane in iter(lanes) do
            if data[lane] == nil then
                ftable:print("-")
                goto continue
            end
            local count = data[lane][error_count + 1]
            if count > 1e6 and not argt.precise then
                ftable:print("%.3g" % {count})
            else
                ftable:print("%d" % {count})
            end
            ::continue::
        end
        ftable:ln()
    end

    ftable:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    print(ftable)
    print("Duration: %.1f sec" % {argt.duration * 4})
end)
