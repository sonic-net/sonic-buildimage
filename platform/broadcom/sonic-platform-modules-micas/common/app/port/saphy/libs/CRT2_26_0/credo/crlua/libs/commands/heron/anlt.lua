local slash = require "slash"
local credo = require "credo"
local crutil = require "crutil"
local fort = require "fort"

slash.register_chip_command({"anlt", "info"}, credo.FAMILY_HERON, [[
Display anlt information.

]], function(slice, argt)
    credo.display_info_print(slice, "anlt")
end)

slash.register_chip_command({"anlt", "status"}, credo.FAMILY_HERON, [[
Display ANLT status

    <lanes>    (lanelist default all)
]], function(slice, argt, argv)

    ---@type integer[]
    local lanes = argt.lanes

    local ftable = fort.create()
    ftable:print_ln("Lane|Mode|Main|AN State|LT State|LT Status")
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
        local is_main_lane = an_state > credo.AUTONEG_STATE_OFF and credo.firmware_debug_cmd(slice, lane, 7, 7) == lane
        ftable:print_ln("%2d(%2s)|%s|%s|%s|%s|%s" % {
            lane, crutil.get_lane_id(slice, lane), mode.display_name, choose(is_main_lane, "yes", ""),
            an_state.display_name, lt_state.display_name, lt_status.display_name
        })
        ::continue::
    end
    print(ftable)
end)
