local slash = require "slash"

slash.register_chip_command({"slice", "info"}, credo.FAMILY_VICEROY, [[
Display top level slice information.

]], function(slice, argt)
    credo.display_info_print(slice, "slice_info")
end)
