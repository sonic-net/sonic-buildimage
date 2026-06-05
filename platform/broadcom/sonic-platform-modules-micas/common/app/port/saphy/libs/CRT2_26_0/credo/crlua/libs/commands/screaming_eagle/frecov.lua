local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"

slash.register_chip_command({"frecov", "config"}, credo.FAMILY_SCREAMING_EAGLE, [[
Configure Fast Recovery for lanes

Arugments:

    <lanes>          (lanelist)
    <duration>       (time)        Fast recovery timeout duration, 0 means to disable
]], ---@param slice integer
---@param argt {lanes: integer[], duration: number}
function(slice, argt)
    for lane in iter(argt.lanes) do
        credo.frecov_configure(slice, lane, int(argt.duration * 1000))
    end
end)

slash.register_chip_command({"frecov", "status"}, credo.FAMILY_SCREAMING_EAGLE, [[
Get Fast Recovery status information

Note that recover count is clear on read.

Arugments:

    <lanes>    (lanelist default all)
]], ---@param slice integer
---@param argt {lanes: integer[]}
function(slice, argt)

    local ftable = fort.create()
    ftable:print_ln("Lane|Mode|Status|Recover\nCount|Timeout\n(ms)")
    ftable:add_separator()
    for lane in iter(argt.lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        local lane_id = crutil.get_lane_id(slice, lane)
        local timeout_ms, status = credo.frecov_get_status(slice, lane)
        local recover_count = credo.frecov_get_recover_count(slice, lane)

        ftable:print_ln("%s (%2d)|%s|%s|%d|%d " %
                            {lane_id, lane, mode.display_name, status.display_name, recover_count, timeout_ms})
    end
    ftable:set_cell_prop(fort.ANY_ROW, 4, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    ftable:set_cell_prop(fort.ANY_ROW, 5, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    print(ftable)
end)
