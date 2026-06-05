local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"anlt", "info"}, credo.FAMILY_BALDEAGLE, [[
Display anlt information.
]], function(slice, argt)
    credo.display_info_print(slice, "anlt")
end)
