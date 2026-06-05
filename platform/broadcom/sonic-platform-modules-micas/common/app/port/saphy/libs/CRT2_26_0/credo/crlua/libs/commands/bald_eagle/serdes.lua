local slash = require "slash"
local credo = require "credo"

local serdes = require "commands.common.serdes"

slash.register_chip_command({"serdes", "param"}, credo.FAMILY_BALDEAGLE, [[
Display serdes paramter table.

Arguments:

    <lanes>      (lanelist default all)

Example:

    > serdes param           # dispaly all lanes
    > serdes param 0-2       # select lanes

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_param %s" % {list.join(argt.lanes, ",")})
end)

slash.register_chip_command({"serdes", "control"}, credo.FAMILY_BALDEAGLE, [[
Display serdes control table.

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_control")
end)

slash.register_chip_command({"serdes", "isi"}, credo.FAMILY_BALDEAGLE, [[
Display isi table.

Arguments:

    <lane> (lane)

]], function(slice, argt)
    credo.display_info_print(slice, "isi %s" % {argt.lane})
end)

serdes.register_tx_taps(credo.FAMILY_BALDEAGLE, {"pre2", "pre1", "main", "post1", "post2"})
