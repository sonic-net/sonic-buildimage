local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"anlt", "info"}, credo.FAMILY_BLACKHAWK, [[
Display anlt information.

Arguments:

    <mode>    (raw|detail|both default both)

Examples:

    > anlt info
    > anlt info raw
    > anlt info detail

]], function(slice, argt)
    if argt.mode == "both" then
        credo.display_info_print(slice, "anlt")
    elseif argt.mode == "raw" then
        credo.display_info_print(slice, "anlt raw")
    else
        credo.display_info_print(slice, "anlt detail")
    end
end)
