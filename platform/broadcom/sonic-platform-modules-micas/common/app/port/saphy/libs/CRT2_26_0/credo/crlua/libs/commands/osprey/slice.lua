local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"slice", "info"}, credo.FAMILY_OSPREY, [[
Display top level slice information.

]], function(slice, argt)
    credo.display_info_print(slice, "slice_info")
end)
