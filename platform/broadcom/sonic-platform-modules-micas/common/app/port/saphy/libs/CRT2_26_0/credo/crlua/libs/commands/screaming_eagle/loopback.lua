local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"
local regmap = require "regmap"

local LB_STATE_NAME = {[0] = "-", [0xe0] = "START", [0xe1] = "RESET_FIFO", [0xe3] = "FIFO_ADJUST", [0xe4] = "TRACK"}

slash.register_chip_command({"loopback", "status"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display lane(s) state machine history logs.

Arugments:

    <lanes>    (lanelist default all)

]], function(slice, argt)

    local sereg = regmap.slice(slice)

    local ftable = fort.create()

    ftable:print_ln("Lane||Loopback||||||Crossbar")
    ftable:print_ln("ID|Mode|Mode|State|Adjust #|FIFO|Heal #|FIFO Diff|FIFO Diff")
    ftable:set_cell_span(1, 1, 2)
    ftable:set_cell_span(1, 3, 6)
    ftable:add_separator()

    ---@type integer[]
    local lanes = argt.lanes
    for lane in iter(lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        local lane_id = crutil.get_lane_id(slice, lane)
        local lbmode = credo.lane_get_loopback_mode(slice, lane)

        ftable:print("%s (%2d)" % {lane_id, lane})
        ftable:print("%s" % {mode.display_name})
        if not crutil.is_lane_configured(mode) then
            goto continue
        end

        ftable:print("%s" % {lbmode.display_name})

        do
            local loopback_state = credo.firmware_debug_cmd(slice, lane, 6, 0)
            local loopback_state_str = LB_STATE_NAME[loopback_state] or "?"
            local fifo_adj_count = credo.firmware_debug_cmd(slice, lane, 6, 2)
            local lb_fifo2 = credo.firmware_debug_cmd(slice, lane, 6, 1)
            local fifo_heal_count = credo.firmware_debug_cmd(slice, lane, 6, 6)
            ftable:print("%s (%X)|%d|%d|%d" %
                             {loopback_state_str, loopback_state, fifo_adj_count, lb_fifo2, fifo_heal_count})
        end

        ftable:print("%d,%d" % {
            gray_bin(sereg.reg_quail_one_lane.adr_diff_lb_0:read(slice, lane)),
            gray_bin(sereg.reg_quail_one_lane.adr_diff_lb_1:read(slice, lane))
        })
        ftable:print("%d,%d" % {
            gray_bin(sereg.reg_quail_one_lane.adr_diff_0:read(slice, lane)),
            gray_bin(sereg.reg_quail_one_lane.adr_diff_1:read(slice, lane))
        })

        ::continue::
        ftable:ln()
    end

    print(ftable)
end)
