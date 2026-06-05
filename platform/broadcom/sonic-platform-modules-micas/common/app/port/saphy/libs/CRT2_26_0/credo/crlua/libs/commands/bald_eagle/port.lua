local slash = require "slash"

slash.register_chip_command({"port", "info"}, credo.FAMILY_BALDEAGLE, [[
Display a port information table.

]], function(slice, argt)
    credo.display_info_print(slice, "port_info")
end)

slash.register_chip_command({"retimer", "status"}, credo.FAMILY_BALDEAGLE, [[
Display retimer status.

]], function(slice, argt)
    credo.display_info_print(slice, "retimer_status")
end)

slash.register_chip_command_alias({"retimer", "status"}, {"port", "retimer", "status"}, credo.FAMILY_BALDEAGLE)

slash.register_chip_command({"bitmux", "status"}, credo.FAMILY_BALDEAGLE, [[
Display bitmux status.

]], function(slice, argt)
    credo.display_info_print(slice, "bitmux_status")
end)

slash.register_chip_command_alias({"bitmux", "status"}, {"port", "bitmux", "status"}, credo.FAMILY_BALDEAGLE)

slash.register_chip_command({"gearbox", "status"}, credo.FAMILY_BALDEAGLE, [[
Display gearbox status.
]], function(slice, argt)
    credo.display_info_print(slice, "gearbox_status")
end)

slash.register_chip_command_alias({"gearbox", "status"}, {"port", "gearbox", "status"}, credo.FAMILY_BALDEAGLE)
