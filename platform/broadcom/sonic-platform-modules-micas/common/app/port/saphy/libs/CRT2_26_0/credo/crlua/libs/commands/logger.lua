local slash = require "slash"
local fort = require "fort"

slash.register_command({"logger", "feat"}, [[
Set/Get logger features

    -a,--api    (optional log_io)
    -r,--reg    (optional log_io)
    -t,--tcm    (optional log_io)
]], function(slice, argt)

    local print_out = argt.api == nil and argt.reg == nil and argt.tcm == nil

    if print_out then
        local ftable = fort.new()
        local api_state = credo.logger_get_feature(credo.LOG_FEAT_API)
        local reg_state = credo.logger_get_feature(credo.LOG_FEAT_REG)
        local tcm_state = credo.logger_get_feature(credo.LOG_FEAT_TCM)

        ftable:print_ln("Feature|Mode")
        ftable:add_separator()
        ftable:print_ln("API|%s" % {api_state.display_name})
        ftable:print_ln("REG|%s" % {reg_state.display_name})
        ftable:print_ln("TCM|%s" % {tcm_state.display_name})
        print(ftable)
        return
    end
    if argt.api ~= nil then
        credo.logger_set_feature(credo.LOG_FEAT_API, argt.api)
    end
    if argt.reg ~= nil then
        credo.logger_set_feature(credo.LOG_FEAT_REG, argt.reg)
    end
    if argt.tcm ~= nil then
        credo.logger_set_feature(credo.LOG_FEAT_TCM, argt.tcm)
    end
end, slash.MULTISLICE)

slash.register_command({"logger", "level"}, [[
Set/Get logger level

    <level>    (optional log_level)
]], function(slice, argt)

    if argt.level ~= nil then
        credo.logger_set_level(argt.level)
        return
    end
    local level = credo.logger_get_level()
    print("Level: %s" % {level.display_name})

end, slash.MULTISLICE)
