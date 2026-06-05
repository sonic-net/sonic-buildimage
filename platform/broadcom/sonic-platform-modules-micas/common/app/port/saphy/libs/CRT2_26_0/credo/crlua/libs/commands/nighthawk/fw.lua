local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"fw", "exit_codes"}, credo.FAMILY_NIGHTHAWK, [[
Display firmware exit codes.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_exit_codes")
end)

slash.register_chip_command({"fw", "error_codes"}, credo.FAMILY_NIGHTHAWK, [[
Display firmware error codes.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_error_codes")
end)

slash.register_chip_command({"fw", "cmdlogs"}, credo.FAMILY_NIGHTHAWK, [[
Display firmware command logs.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_cmd_log")
end)

slash.register_chip_command({"fw", "state_timer"}, credo.FAMILY_NIGHTHAWK, [[
Display firmware state timer.

    <lanes>    (lanelist)   lanes to display

]], function(slice, argt)
    for lane in iter(argt.lanes) do
        credo.display_info_print(slice, "fw_state_timer %d" % {lane})
    end

end)

slash.register_chip_command({"fw", "regdump"}, credo.FAMILY_NIGHTHAWK, [[
Display firmware register information.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_reg")
end)
