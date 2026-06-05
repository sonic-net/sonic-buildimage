local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local intmod = require "int"

slash.register_command({"fw", "version"}, [[
Display firmware version.

]], function(slice, argt)
    print(credo.firmware_version_str(slice), "0x%X" % {credo.firmware_hash(slice)})
end)

slash.register_command({"fw", "status"}, [[
Display firmware status.

]], function(slice, argt)
    if credo.firmware_get_status(slice) == 1 then
        print("Firmware: RUNNING")
    else
        print("Firmware: NOT RUNNING")
    end
end)

slash.register_command({"fw", "load"}, [[
Load firmware. (DEBUG)

Prefer using `> slice reinit`

Arguments:

    -f,--force
    <path>        (string)    firmware path

]], function(slice, argt)
    credo.firmware_load(slice, argt.path, argt.force)
end)

slash.register_command({"fw", "unload"}, [[
Unload firmware. (DEBUG)

]], function(slice, argt)
    credo.firmware_unload(slice)
end)

slash.register_command({"fw", "cmd"}, [[
Run a firmware command. (DEBUG)

Arguments:

    <command>    (integer)
    <param>      (integer)

]], function(slice, argt)
    local response, response_param = credo.firmware_cmd(slice, argt.command, argt.param)
    print("0x%X, 0x%X" % {response, response_param})
end)

slash.register_command({"fw", "cmdex"}, [[
Run a firmware command. (DEBUG)

Arguments:

    <command>    (integer)
    <param1>     (integer)
    <param2>     (integer)

]], function(slice, argt)
    local res, rparam1, rparam2 = credo.firmware_cmd_ex(slice, argt.command, argt.param1, argt.param2)
    print("0x%X, 0x%X, 0x%X" % {res, rparam1, rparam2})
end)

local function do_fw_debug_cmd(slice, argt, extra)
    ---@type integer[]
    local lane_list = argt.lane_list
    ---@type integer
    local section = argt.section
    ---@type integer[]
    local index_list = argt.index_list

    local ftable = fort.create_table()
    fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_CENTER)
    fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_LEFT_PADDING, 0)
    fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_RIGHT_PADDING, 0)

    local limits = credo.slice_get_limits(slice)
    local num_lane = limits.max_lanes

    -- fwdebug_header
    fort.printf(ftable, "index")
    for lane in iter(lane_list) do
        if lane >= num_lane then
            break
        end
        fort.printf(ftable, "Ln%2d", lane)
    end
    fort.ln(ftable)
    fort.add_separator(ftable)

    local api_func
    if extra == true then
        api_func = credo.firmware_debug_cmd_ex
    else
        api_func = credo.firmware_debug_cmd
    end

    for index in iter(index_list) do
        fort.printf(ftable, "%4d", index)
        for lane in iter(lane_list) do
            if lane >= num_lane then
                break
            end
            local ret, res, res1 = pcall(api_func, slice, lane, section, index)
            if ret ~= true then
                fort.printf(ftable, " - ")
            elseif res1 ~= nil then
                fort.printf(ftable, "%04X,%04X", res, res1)
            else
                fort.printf(ftable, "%04X", res)
            end
        end
        fort.ln(ftable)
    end

    print("section", section)
    print(fort.to_string(ftable))
end

slash.register_command({"fw", "debug_cmd"}, [[
Run a firmware debug command. (DEBUG)

Arguments:

    <lane_list>     (lanelist)
    <section>       (integer)
    <index_list>    (intlist)

]], function(slice, argt)
    do_fw_debug_cmd(slice, argt, false)
end)

slash.register_command({"fw", "debug_cmdex"}, [[
Run a firmware debug command. (DEBUG)

Arguments:

    <lane_list>     (lanelist)
    <section>       (integer)
    <index_list>    (intlist)

]], function(slice, argt)
    do_fw_debug_cmd(slice, argt, true)
end)

slash.register_command({"fw", "reg"}, [[
Get/set firmware registers

Arguments:

    <addresses>     (intlist)
    -v,--value      (optional integer)
    -b,--bits       (optional regbits)
    -s,--section    (optional integer)    extended registers
]], function(slice, argt)

    ---@type integer | nil
    local section = argt.section
    ---@type RegisterBits|nil
    local bits = argt.bits

    local function fw_regread(address)
        if section ~= nil then
            return credo.firmware_reg_rd_ex(slice, address, section)
        else
            return credo.firmware_reg_rd(slice, address)
        end
    end

    if argt.value ~= nil then
        ---@type integer
        local value = argt.value
        for address in iter(argt.addresses) do
            local regvalue = value
            if bits ~= nil then
                local curvalue = fw_regread(address)
                value = intmod.bitset(curvalue, regvalue, bits.msb, bits.lsb)
            end

            if section ~= nil then
                credo.firmware_reg_wr_ex(slice, address, section, value)
            else
                credo.firmware_reg_wr(slice, address, value)
            end
        end
    end

    local ftable = fort.create_table()
    if section ~= nil then
        ftable:print_ln("Section %d" % {section})
    end
    ftable:print_ln("Address|Value")
    ftable:add_separator()

    for address in iter(argt.addresses) do
        local regval = fw_regread(address)
        if bits ~= nil then
            regval = intmod.bitget(regval, bits.msb, bits.lsb)
        end
        local regbit_str = choose(bits ~= nil, "." .. tostring(bits), "")
        ftable:print_ln("%s%s|%s" % {hex(address), regbit_str, hex(regval)})
    end
    print(ftable)
end)
