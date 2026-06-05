local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"clockout", "status"}, credo.FAMILY_BLACKHAWK, [[
Display clock output status
]], function(slice, argt)
    credo.display_info_print(slice, "clock_output")
end)

local clockout_map = {diff = 0, sg1 = 1, sg2 = 2}

slash.register_chip_command({"clockout", "enable"}, credo.FAMILY_BLACKHAWK, [[
Enable clock output

Arguments:

    <clock>          (diff|sg1|sg2)
    <source_lane>    (lane)
    <divider>        (32|64|128|256|512|1024|2048)
]], function(slice, argt)
    local clock = clockout_map[argt.clock]
    local source_lane = argt.source_lane
    local divider = int(argt.divider)
    credo.clockout_enable(slice, clock, source_lane, divider)
end)

slash.register_chip_command({"clockout", "disable"}, credo.FAMILY_BLACKHAWK, [[
Enable clock output

Arguments:

    <clock>          (diff|sg1|sg2|all)
]], function(slice, argt)

    if (argt.clock == "all") then
        credo.clockout_disable_all(slice)
        return
    end
    local clock = clockout_map[argt.clock]
    credo.clockout_disable(slice, clock)
end)
