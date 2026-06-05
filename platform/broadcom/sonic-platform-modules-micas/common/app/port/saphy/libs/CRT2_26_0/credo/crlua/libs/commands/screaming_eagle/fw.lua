local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"fw", "exit_codes"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display firmware exit codes.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_exit_codes")
end)

slash.register_chip_command({"fw", "error_codes"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display firmware error codes.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_error_codes")
end)

slash.register_chip_command({"fw", "cmdlogs"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display firmware command logs.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_cmd_log")
end)

slash.register_chip_command({"fw", "opt_trace"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display firmware optimization trace.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_opt_trace")
end)

slash.register_chip_command({"fw", "state_timer"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display firmware state timer.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_state_timer")
end)

slash.register_chip_command({"fw", "reg", "dump"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display firmware register information.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_reg")
end)
