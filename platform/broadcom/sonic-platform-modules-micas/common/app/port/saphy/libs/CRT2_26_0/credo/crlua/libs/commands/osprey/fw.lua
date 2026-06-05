local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"fw", "exit_codes"}, credo.FAMILY_OSPREY, [[
Display firmware exit codes.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_exit_codes")
end)

slash.register_chip_command({"fw", "reg", "dump"}, credo.FAMILY_OSPREY, [[
Display firmware register information.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_reg")
end)
