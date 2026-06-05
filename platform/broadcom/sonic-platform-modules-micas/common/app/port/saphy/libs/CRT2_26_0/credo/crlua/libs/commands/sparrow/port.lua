local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"port", "info"}, credo.FAMILY_SPARROW, [[
Display a port information table.

]], function(slice, argt)
    credo.display_info_print(slice, "port_info")
end)
