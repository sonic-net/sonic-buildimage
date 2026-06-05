local slash = require "slash"
local reg = require "commands.common.reg"

slash.register_command({"reg"}, reg.list_desc, reg.create_reg_list(), slash.MULTISLICE)

slash.register_command({"reg", "table"}, reg.table_desc, reg.create_reg_talbe(), slash.MULTISLICE)

slash.register_command({"reg", "hive"}, reg.hive_desc, reg.create_reg_hive(), slash.MULTISLICE)

slash.register_command({"reg", "hive", "info"}, reg.hive_info_desc, reg.create_reg_hive_info())

slash.register_command({"reg", "lane"}, reg.lane_desc, reg.create_reg_lane(), slash.MULTISLICE)

slash.register_command({"reg", "verify"}, [[
Verify that register access is working correctly. Also provides information on performance.

Performs Write-Read operations to frame registers and verifies they are correct.

Performance readings may be slightly degraded by lua runtime.

Arguments:

    -d,--duration     (time default 20s)    how long to verify register access
    -b,--burst                              use burst register access
    -r,--registers    (optional intspan)    what registers to use, defaults to use firmware frame registers
]], function(slice, argt)

    ---@type IntSpan|nil
    local regspan = argt.registers
    ---@type boolean
    local burst = argt.burst
    ---@type integer
    local duration = argt.duration

    local config = credo.RegVerifyConfig()
    if regspan ~= nil then
        config.address = regspan.start
        config.burst_width = regspan.stop - regspan.start + 1
        config.flags = config.flags | credo.RegVerifyFlags.BURST | credo.RegVerifyFlags.OVERRIDE_ADDRESS
    end
    if burst then
        config.flags = config.flags | credo.RegVerifyFlags.BURST
    end
    config.flags = config.flags | credo.RegVerifyFlags.USE_DURATION
    config.duration_sec = duration

    local stats = credo.reg_verify(slice, config)
    local fails = stats.fail_count
    local rdwrs = stats.reg_count
    duration = stats.duration_sec
    if fails == 0 then
        print("Register Access is clean after %.1f seconds" % {duration})
    else
        print("Register Access is FAILING after %.1f seconds: %d errors (%.3g%% error rate)" %
                  {duration, fails, fails / rdwrs * 100})
    end

    print("%.4g Write-Reads/sec" % {rdwrs / duration})
end)
