local slash = require "slash"
local crutil = require "crutil"
local fort = require "fort"
local time = require "time"

local fecana = {}

---@class FecanaHistogramConfig
---@field public valid_lane_mode fun(mode: credo.LaneMode): boolean

---@param family credo.Family
---@param config? FecanaHistogramConfig
function fecana.register_histogram(family, config)
    config = config or {}
    local valid_lane_mode = config.valid_lane_mode
    if valid_lane_mode == nil then
        valid_lane_mode = crutil.is_lane_configured
    end

    slash.register_chip_command({"fecana", "histogram"}, family, [[
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
            if not valid_lane_mode(mode) then
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

end

---@param family credo.Family
function fecana.register_histgroup(family)
    slash.register_chip_command({"fecana", "histgroup"}, family, [[
Get a PRBS fec analyzer histogram at a specific group.

Since it only captures 1 group, it can return instantly and capture for a global duration

Arguments:

    <lanes>          (lanelist default all)    lanes to capture errors
    -g,--group       (optional integer)        set the hostogram bin group to analyze
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

        if argt.group ~= nil then
            assert(argt.group >= 0, "group must be >= 0")
            assert(argt.group <= 3, "group must be <= 3")
            for lane in iter(fec_lanes) do
                credo.fecana_set_hist_group(slice, lane, argt.group)
            end
            print("Set to group %d" % {argt.group})
            return
        end

        for lane in iter(fec_lanes) do
            local counters = credo.fecana_get_hist_counter(slice, lane)
            if data[lane] == nil then
                data[lane] = counters
            else
                list.extend(data[lane], counters)
            end
        end

        local ftable = fort.create()

        ftable:print("SEI")
        for lane in iter(lanes) do
            ftable:print("%s(%02d)" % {crutil.get_lane_id(slice, lane), lane})
        end
        ftable:ln()
        ftable:add_separator()
        for bin = 0, 3 do
            ftable:print(">= Group Bin %d" % {bin})
            for lane in iter(lanes) do
                if data[lane] == nil then
                    ftable:print("-")
                    goto continue
                end
                local count = data[lane][bin + 1]
                if count > 1e6 and not argt.precise then
                    ftable:print("%.3g" % {count})
                else
                    ftable:print("%d" % {count})
                end
                ::continue::
            end
            ftable:ln()
        end
        ftable:add_separator()
        ftable:print("Group #")
        for lane in iter(lanes) do
            if data[lane] == nil then
                ftable:print("-")
                goto continue
            end
            ftable:print("%d" % {credo.fecana_get_hist_group(slice, lane)})
            ::continue::
        end
        ftable:ln()

        ftable:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        print(ftable)
    end)

end

return fecana
