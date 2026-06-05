local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"slice", "info"}, credo.FAMILY_BLACKHAWK, [[
Display top level slice information.

]], function(slice, argt)
    credo.display_info_print(slice, "slice_info")
end)

slash.register_chip_command({"slice", "clkout"}, credo.FAMILY_BLACKHAWK, [[
Display slice clock output information.

]], function(slice, argt)
    credo.display_info_print(slice, "clock_output")
end)

slash.register_chip_command({"slice", "vsensor"}, credo.FAMILY_BLACKHAWK, [[
Display slice vsensor information.

]], function(slice, argt)
    local vsensor = credo.slice_get_vsensor(slice)
    print("VSensor: %.3gV" % {vsensor})
end)
