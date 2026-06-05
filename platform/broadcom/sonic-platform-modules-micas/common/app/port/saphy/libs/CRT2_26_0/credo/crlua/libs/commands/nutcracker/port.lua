local slash = require "slash"

slash.register_chip_command({"port", "info"}, credo.FAMILY_NUTCRACKER, [[
Display a port information table.

]], function(slice, argt)
    credo.display_info_print(slice, "port_info")
end)

slash.register_chip_command({"retimer", "status"}, credo.FAMILY_NUTCRACKER, [[
Display retimer status.

]], function(slice, argt)
    credo.display_info_print(slice, "retimer_status")
end)

slash.register_chip_command_alias({"retimer", "status"}, {"port", "retimer", "status"}, credo.FAMILY_NUTCRACKER)
