local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"fw", "exit_codes"}, credo.FAMILY_BLACKHAWK, [[
Display firmware exit codes.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_exit_codes")
end)

slash.register_chip_command({"fw", "reg", "dump"}, credo.FAMILY_BLACKHAWK, [[
Display firmware register information.

]], function(slice, argt)
    credo.display_info_print(slice, "fw_reg")
end)

slash.register_chip_command({"fw", "ram", "dump"}, credo.FAMILY_BLACKHAWK, [[
Dump RAM information of firmware

    -s,--size    (integer default 112000)    upperbound size of decompressed firmware
    -f,--file    (optional file-out)         write to file instead of stdout
]], function(slice, argt)
    local count = argt.size // 16

    for i = 0, count - 1 do
        local line = "%06x: " % {i * 16}
        for j = 0, 3 do
            local _, b, c = credo.firmware_cmd_ex(slice, 0xc000, i * 4 + j, 0)
            line = line .. "%02x %02x %02x %02x " % {c >> 8, c & 0xff, b >> 8, b & 0xff}
        end
        if argt.file then
            ---@type file*
            argt.file:write(line .. "\n")
            if i % (count // 20) == 0 then
                print("%0.1f%%" % {i / (count / 100)})
            end
        else
            print(line)
        end
    end
    if argt.file then
        print("Written to %s" % {argt.file_name})
    end
end)
