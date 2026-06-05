local slash = require "slash"
local credo = require "credo"
local zlib = require "zlib"
local serdes = require "commands.common.serdes"
local base64 = require "base64"

slash.register_chip_command({"serdes", "param"}, credo.FAMILY_HERON, [[
Display serdes paramter table.

Arguments:

    <lanes>    (lanelist default all)

Example:

    > serdes param           # dispaly all lanes
    > serdes param 0-2       # select lanes

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_param %s" % {list.join(argt.lanes, ",")})
end)

slash.register_chip_command({"serdes", "control"}, credo.FAMILY_HERON, [[
Display serdes control table.

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_control")
end)

slash.register_chip_command({"serdes", "pll_info"}, credo.FAMILY_HERON, [[
Display pll information.

Arguments:

    <lane>    (optional lane)    specific information for a lane

]], function(slice, argt)
    if argt.lane ~= nil then
        credo.display_info_print(slice, "pll_info %d" % {argt.lane})
    else
        credo.display_info_print(slice, "pll_info" % {argt.lane})
    end

end)

serdes.register_tx_taps(credo.FAMILY_HERON, {"pre3", "pre2", "pre1", "main", "post1", "post2", "post3"})

slash.register_chip_command({"serdes", "diag"}, credo.FAMILY_HERON, [[
Display serdes diagnostic information.

Arguments:

    <lane>    (lane)

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_diag %d" % {argt.lane})
end)

slash.register_chip_command({"serdes", "optlogs"}, credo.FAMILY_HERON, [[
Capture SerDes Optimization logs.

Arguments:

    <lanes>       (lanelist)            lanes to capture optimization
    -f,--file     (optional file-out)   output to file
    -c,--clear                          clear information
]], function(slice, argt)
    local DUMP_FW_LOG_BUFFER_LEN = 7
    local DUMP_FW_LOG_BUFFER_READ_CLEAR = 8
    local DUMP_FW_LOG_RAM = 1000

    ---@type integer[]
    local lanes = argt.lanes

    if argt.clear then
        for lane in iter(lanes) do
            credo.firmware_debug_cmd(slice, lane, 0, DUMP_FW_LOG_BUFFER_READ_CLEAR)
        end
        return
    end

    for lane in iter(lanes) do
        local log_len = credo.firmware_debug_cmd(slice, lane, 0, DUMP_FW_LOG_BUFFER_LEN)
        ---@type string[]
        local log_ram = list.map(range(0, log_len - 1), function(i)
            local data = credo.firmware_debug_cmd(slice, lane, 0, i + DUMP_FW_LOG_RAM)
            return string.char(data & 0xFF, (data >> 8) & 0xFF)
        end)
        local output = table.concat(log_ram, "")
        output = zlib.compress(output)
        output = base64.encode(output)
        if argt.file ~= nil then
            ---@type file*
            local file = argt.file
            file:write(output)
            return
        end
        print("[slice %d][lane %d]" % {slice, lane})
        print(output)
    end

end)
