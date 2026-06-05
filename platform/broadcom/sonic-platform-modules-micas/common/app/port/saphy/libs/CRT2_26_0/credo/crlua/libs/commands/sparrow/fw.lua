local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"fw", "regdump"}, credo.FAMILY_SPARROW, [[
Display firmware register information.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_reg")
end)
