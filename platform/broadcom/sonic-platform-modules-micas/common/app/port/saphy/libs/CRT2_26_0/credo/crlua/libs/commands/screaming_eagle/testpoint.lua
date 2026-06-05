local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"

local tp_index_map = {20, 6, 32, 16, 5, 21, 13, 24, 7}

slash.register_chip_command({"testpoint", "read"}, credo.FAMILY_SCREAMING_EAGLE, [[
Read testpoints in the slice.


Arguments:

    <lanes>       (lanelist)

    -g,--groups   (intlist default 0)
    -m,--mode     (integer default 1)
    -i,--index    (optional intlist)
    -d,--div2                            use divider2 for saturated vsensor
    --delay       (time default 0.1s)


Note:

    When div2 is enabled, the output value will be doubled. Not all testpoints groups/modes support div2, so the value
    may be 2x expected.

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes
    local lane_count = #lanes
    ---@type integer[]
    local groups = argt.groups
    ---@type integer
    local mode = argt.mode
    ---@type integer[]

    ---@type boolean
    local div2 = argt.div2

    for group in iter(groups) do
        local ftable = fort.new()
        local indices = argt.index or range(tp_index_map[group + 1] - 1)

        print("Group=%d,Mode=%d,Div2=%s" % {group, mode, tostring(div2)})
        ftable:print_ln("|Lane")
        ftable:print("Index")
        for lane in iter(lanes) do
            ftable:print("(%s)%d" % {crutil.get_lane_id(slice, lane), lane})
        end

        ftable:ln()
        ftable:add_separator()

        for index in iter(indices) do
            ftable:print("%d" % {index})
            for lane in iter(lanes) do
                credo.testpoint_select(slice, lane, group, mode, index, div2, 0)
                time.sleep(argt.delay)
                local tp_val = credo.testpoint_read(slice)
                if div2 then
                    tp_val = 2 * tp_val
                end
                if tp_val < 0.1 then
                    ftable:print("%.3fmV" % {tp_val * 1000})
                else
                    ftable:print("%.3f V" % {tp_val})
                end
            end
            ftable:ln()
        end

        if lane_count > 1 then
            ftable:set_cell_span(1, 2, lane_count)
        end
        ftable:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)

        print(ftable)
    end

end)
