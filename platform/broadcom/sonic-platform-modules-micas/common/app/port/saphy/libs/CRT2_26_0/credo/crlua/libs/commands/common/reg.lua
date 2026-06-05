--- Higher order functions for generating reg commands based upon the chip.
local fort = require "fort"

local reg = {}

---@param slice integer
---@param addr integer
---@param bits RegisterBits|nil
---@param value integer|nil
local function access_reg(slice, addr, bits, value)
    local msb = nil
    local lsb = nil
    if bits ~= nil then
        msb = bits.msb
        lsb = bits.lsb
    end
    if value ~= nil then
        credo.slice_write(slice, addr, value, msb, lsb)
        return value
    else
        return credo.slice_read(slice, addr, msb, lsb)
    end
end

---@param slice integer
---@param addr_list integer[]
---@param bits RegisterBits|nil
---@param value integer|nil
local function access_reg_list(slice, addr_list, bits, value)
    local result = {}
    for i = 1, #addr_list do
        result[i] = access_reg(slice, addr_list[i], bits, value)
    end
    return result
end

---@param sliceS integer[]|integer
---@return integer[]
local function to_slice_list(sliceS)
    if type(sliceS) == "number" then
        return {sliceS}
    elseif type(sliceS) == "table" then
        return sliceS
    else
        error("invalid slice list")
    end
end

local function display_reg_listS(sliceS, reg_list, bits, resultS)
    local ftable = fort.create_table()

    assert(#reg_list <= 0x200, "Too many registers")

    if #sliceS > 1 then
        fort.printf(ftable, "Slice")
        for slice in iter(sliceS) do
            fort.printf(ftable, "%d", slice)
        end
        fort.ln(ftable)
    end

    fort.printf(ftable, "Register")
    for i = 1, #sliceS do
        fort.printf(ftable, "Value")
    end
    fort.ln(ftable)
    fort.add_separator(ftable)

    local bits_string = ""
    if bits then
        bits_string = "." .. tostring(bits)
    end

    ---@type integer
    for ri, regv in pairs(reg_list) do
        fort.printf(ftable, "%04X%s", regv, bits_string)

        for si = 1, #sliceS do
            fort.printf(ftable, "%04X", resultS[si][ri])
        end
        fort.ln(ftable)
    end
    print(fort.to_string(ftable))
end

---@param sliceS integer|integer[]
---@param reg_list integer[]
---@param bits RegisterBits|nil
---@param value integer|nil
local function do_reg_list(sliceS, reg_list, bits, value)
    local resultS = {}
    sliceS = to_slice_list(sliceS)

    for i, slice in pairs(sliceS) do
        resultS[i] = access_reg_list(slice, reg_list, bits, value)
    end

    -- no printing on write
    if value ~= nil then
        return
    end

    display_reg_listS(sliceS, reg_list, bits, resultS)
end

reg.list_desc = [[
Read or write a register or list of registers.

Arguments:

    -b,--bits     (optional regbits)    Bits to read for all registers
    -v,--value    (optional integer)    Value to set for all registers, default is just a read
    <reglist>     (intlist)             List of registers to read

Examples:

    > reg 0x8000                   # read a single register
    > reg 0x9000,0x9001,0x9002     # read the 3 registers
    > reg 0x9000:0x9010            # read registers in range [0x9000, 0x9010]
    > reg 0x8000:0x9000:0x200      # read registers [0x8000,0x9000] with steps of 0x200
    > reg 0x9000:0x9010 -b 12:2    # read register list bits 12:2
    > reg 0x8000 -v 0x1            # write register 0x8000 to value 0x1
    > reg 0x9000:0x9010 -v 0x2     # write register list to value 0x2

    +----------+-------+
    | Register | Value |
    +----------+-------+
    |  HEX     |  HEX  |
    |  HEX     |  HEX  |
    |  HEX     |  HEX  |
    +----------+-------+
]]

function reg.create_reg_list()
    return function(sliceS, argt)
        ---@type integer[]
        local reg_list = argt.reglist
        ---@type RegisterBits|nil
        local bits = argt.bits
        ---@type integer|nil
        local value = argt.value

        do_reg_list(sliceS, reg_list, bits, value)
    end
end

---@param slice integer
---@param base_list integer[]
---@param offset_list integer[]
---@param bits RegisterBits|nil
---@param value integer|nil
local function access_reg_table(slice, base_list, offset_list, bits, value)
    local result = {}
    for i = 1, #base_list do
        local addr_list = list.map(offset_list, function(offset)
            return base_list[i] + offset
        end)
        result[i] = access_reg_list(slice, addr_list, bits, value)
    end
    return result
end

local function display_reg_tableS(sliceS, indicesS, base_listS, offset_list, bits, resultS)
    local ftable = fort.create_table()
    fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_LEFT_PADDING, 0)
    fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_RIGHT_PADDING, 0)
    local function add_header(slice_list)
        fort.add_separator(ftable)
        if #slice_list > 1 then
            fort.printf(ftable, "Slice")
            for si = 1, #slice_list do
                local indices = indicesS[si]
                for i = 1, #indices do
                    if i == 1 then
                        fort.printf(ftable, "%d", slice_list[si])
                    else
                        fort.printf(ftable, "")
                    end
                end
            end
            fort.ln(ftable)
            fort.add_separator(ftable)
        end
        fort.printf(ftable, "Index")
        for si = 1, #slice_list do
            local indices = indicesS[si]
            for idx in iter(indices) do
                fort.printf(ftable, "%d", idx)
            end
        end
        fort.ln(ftable)

        fort.printf(ftable, "Offset")
        for si = 1, #slice_list do
            local base_list = base_listS[si]
            for base in iter(base_list) do
                fort.printf(ftable, "%04X", base)
            end
        end
        fort.ln(ftable)
        fort.add_separator(ftable)
    end

    local bits_string = ""
    if bits then
        bits_string = "." .. tostring(bits)
    end

    for oi, offset in pairs(offset_list) do
        if oi % 16 == 1 then
            add_header(sliceS)
        end
        fort.printf(ftable, "%04X%s", offset, bits_string)
        for si = 1, #sliceS do
            local result = resultS[si]
            local base_list = base_listS[si]
            for bi = 1, #base_list do
                fort.printf(ftable, "%04X", result[bi][oi])
            end
        end
        fort.ln(ftable)
    end
    print(fort.to_string(ftable))
end

---@param sliceS integer|integer[]
---@param indices integer[]|"all"
---@param base_list integer[]
---@param offset_list integer[]
---@param bits RegisterBits|nil
---@param value integer|nil
local function do_reg_table(sliceS, indices, base_list, offset_list, bits, value)
    local resultS = {}
    local indicesS = {}
    local base_listS = {}

    sliceS = to_slice_list(sliceS)

    if indices == "all" then
        indices = range(0, #base_list - 1)
    end

    for i, slice in pairs(sliceS) do
        resultS[i] = access_reg_table(slice, base_list, offset_list, bits, value)
        base_listS[i] = base_list
        indicesS[i] = indices
    end

    -- no printing on setting values
    if value ~= nil then
        return
    end

    display_reg_tableS(sliceS, indicesS, base_listS, offset_list, bits, resultS)

end

reg.table_desc = [[
Read/Write a table of register bases + offsets.

Address Computation:
register_address = base_list[column] + offset_list[row]

It is best to make sure base_list is the smaller of the lists, as the table
can grow vertically better than horizontally.

Arguments:

    -b,--bits        (optional regbits)    Register bits
    -s,--size        (optional integer)    How many register
    -v,--value       (optional integer)    Register value to write
    <base_list>      (intlist)             Base register locations
    <offset_list>    (optional intlist)    Offset from base register location

Examples:

    > reg table 0x8000:0x9000:0x200 0x20

        Read at steps of 0x200 between [0x8000,0x9000] (so 0x8000,0x8200...0x9000).
        Read at each step just register 0x20.

    > reg table 0x8000:0x9000:0x200 -s 0x20           # read 0x0-0x19 at each step in the base list.
    > reg table 0x8000:0x9000:0x200 -s 0x20 -v 0x2    # write the register for each step + offset.

      +------+----+----+----+
      |Index |0   |1   |2   |
      |Offset| HEX| HEX| HEX|
      +------+----+----+----+
      |  HEX | HEX| HEX| HEX|
      |  HEX | HEX| HEX| HEX|
      |  HEX | HEX| HEX| HEX|
      +------+----+----+----+
]]

function reg.create_reg_talbe()
    return function(sliceS, argt)
        ---@type integer[]
        local base_list = argt.base_list
        ---@type integer[]|nil
        local offset_list = argt.offset_list
        ---@type integer|nil
        local size = argt.size
        ---@type integer|nil
        local value = argt.value
        ---@type RegisterBits|nil
        local bits = argt.bits

        -- one value is shorthand for range, otherwise they would use `reg readtable`
        assert(size ~= nil or offset_list ~= nil, "Must provide a size or offset list")
        if offset_list == nil then
            offset_list = list.range(0, size - 1)
        end

        assert(#base_list <= 24, "Too many base list args")
        assert(#offset_list <= 0x400, "Too many offsets provided")

        local indices = list.range(0, #base_list - 1)

        do_reg_table(sliceS, indices, base_list, offset_list, bits, value)
    end
end

local function access_reg_hive(slice, indices, hivename, offset_list, bits, value)
    local hives = credo.reghive_get_map(slice)
    local hive = hives[hivename]

    assert(hive ~= nil, "Not found hive `%s` in slice %d" % {hivename, slice})
    if indices == "all" then
        indices = range(0, #hive - 1)
    else
        indices = list.filter(indices, function(v)
            return v < #hive
        end)
    end

    local base_list = list.map(indices, function(v)
        return hive[v + 1]
    end)

    return access_reg_table(slice, base_list, offset_list, bits, value), indices, base_list
end

---@param sliceS integer|integer[]
---@param indices integer[]|"all"
---@param hivename string
---@param offset_list integer[]
---@param bits RegisterBits|nil
---@param value integer|nil
local function do_reg_hive(sliceS, indices, hivename, offset_list, bits, value)
    local resultS = {}
    local indicesS = {}
    local base_listS = {}

    sliceS = to_slice_list(sliceS)

    for i, slice in pairs(sliceS) do
        resultS[i], indicesS[i], base_listS[i] = access_reg_hive(slice, indices, hivename, offset_list, bits, value)
    end

    if #resultS == 0 or #resultS[1] == 0 then
        return
    end

    -- no printing on write
    if value ~= nil then
        return
    end

    display_reg_tableS(sliceS, indicesS, base_listS, offset_list, bits, resultS)

end

reg.hive_desc = [[
Read/Write a register hive.

A hive is multiple groups of registers that have the same functionality, but
can show up multiple times at different offsets. The multiple groups usually
indicates that they are

Either offsets or the size flag must be set.

See `> reg hive info` for list of available hives.

Arguments:

    -b,--bits     (optional regbits)         bits of the registers to read
    -v,--value    (optional integer)         set register values
    <hive>        (string)                   hive to read/write
    <indices>     (indexlist default all)    index of hive to read/write
    <offsets>     (intlist)                  offset from the hive to read

Examples:

    > reg hive serdes_rx all 0-0x50
    > reg hive serdes_rx 0-8 0x20
    > reg hive serdes_rx 0-8 0x20 -v 0x10

      +------+----+----+----+
      |Index |0   |1   |2   |
      |Offset| HEX| HEX| HEX|
      +------+----+----+----+
      |  HEX | HEX| HEX| HEX|
      |  HEX | HEX| HEX| HEX|
      |  HEX | HEX| HEX| HEX|
      +------+----+----+----+
]]

---@param hives? table<string, integer[]>
---@return fun (sliceS: integer[], argt: table<string, any>)
function reg.create_reg_hive(hives)
    return function(sliceS, argt)
        ---@type integer[]|"all"
        local indices = argt.indices or "all"
        ---@type string
        local hive = argt.hive
        ---@type integer[]
        local offset_list = argt.offsets
        ---@type integer|nil
        local value = argt.value
        ---@type RegisterBits|nil
        local bits = argt.bits

        assert(#offset_list < 0x400, "Too many offsets provided")

        if hives ~= nil then
            local hive_base_list = hives[hive]
            assert(hive_base_list ~= nil, "Invalid Hive: " .. hive)
            do_reg_table(sliceS, indices, hive_base_list, offset_list, bits, value)
        else
            do_reg_hive(sliceS, indices, hive, offset_list, bits, value)
        end
    end
end

reg.hive_info_desc = [[
Show available register hives.

]]

---Create a reg hive for a chip
---@param hives? table<string,integer[]>
---@return fun (slice:integer, argt: table<string, any>)
function reg.create_reg_hive_info(hives)
    return function(slice, argt)
        hives = hives or credo.reghive_get_map(slice)
        local ftable = fort.create_table()
        fort.printf_ln(ftable, "Hive|Base Registers")
        for hive, values in tablex.sort(hives) do
            local values_str = stringx.join(", ", list.map(values, function(val)
                return "0x%04X" % {val}
            end))
            values_str = stringx.join("\n", stringx.wrap(values_str, 60))
            fort.printf_ln(ftable, "%s|%s", hive, values_str)
        end
        fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
        fort.set_border_style(ftable, fort.BASIC2_STYLE)
        print(fort.to_string(ftable))
    end
end

local function do_reg_lane_from_hive(sliceS, indices, offset_list, bits, value)
    local func_reg_hive = reg.create_reg_hive()
    local slice = sliceS[1]
    local hives = credo.reghive_get_map(slice)
    local argt = {indices = indices, offsets = offset_list, bits = bits, value = value}

    if hives["serdes"] ~= nil then
        argt.hive = "serdes"
        print(argt.hive)
        func_reg_hive(sliceS, argt)
    end
    if hives["serdes_rx"] ~= nil then
        argt.hive = "serdes_rx"
        print(argt.hive)
        func_reg_hive(sliceS, argt)
    end
    if hives["serdes_tx"] ~= nil then
        local rx = hives["serdes_rx"]
        local tx = hives["serdes_tx"]
        if tablex.compare(rx, tx, "==") ~= true then
            argt.hive = "serdes_tx"
            print(argt.hive)
            func_reg_hive(sliceS, argt)
        end
    end

    local host_count = credo.lane_get_count(slice)
    argt.indices = list.filter(argt.indices, function(v)
        return v >= host_count
    end)

    if hives["serdes_bs"] ~= nil then
        argt.hive = "serdes_bs"
        print(argt.hive)
        func_reg_hive(sliceS, argt)
    end
    if hives["serdes_rx_bs"] ~= nil then
        argt.hive = "serdes_rx_bs"
        print(argt.hive)
        func_reg_hive(sliceS, argt)
    end
    if hives["serdes_tx_bs"] ~= nil then
        local rx = hives["serdes_rx_bs"]
        local tx = hives["serdes_tx_bs"]
        if tablex.compare(rx, tx, "==") ~= true then
            argt.hive = "serdes_tx_bs"
            print(argt.hive)
            func_reg_hive(sliceS, argt)
        end
    end
end

reg.lane_desc = [[
Read/Write to slice lanes.

Arguments:

    -b,--bits     (optional regbits)    Bits to access
    -v,--value    (optional integer)    Balue to write to registers
    <lanes>       (lanelist)            Lanes to access
    <offsets>     (intlist)             Offset from lane to access

Examples:

> reg lane all 0
> reg lane all 0-0x20
> reg lane 0-7 0

      +------+----+----+----+
      |Index |0   |1   |2   |
      |Offset| HEX| HEX| HEX|
      +------+----+----+----+
      |  HEX | HEX| HEX| HEX|
      |  HEX | HEX| HEX| HEX|
      |  HEX | HEX| HEX| HEX|
      +------+----+----+----+
]]

---@param hive? integer[]
---@return fun (sliceS: integer[], argt: table<string, any>)
function reg.create_reg_lane(hive)
    return function(sliceS, argt)
        ---@type integer[]
        local indices = argt.lanes
        ---@type integer[]
        local offset_list = argt.offsets
        ---@type integer|nil
        local value = argt.value
        ---@type RegisterBits|nil
        local bits = argt.bits

        if hive ~= nil then
            local hive_base_list = hive
            assert(#offset_list <= 0x400, "Too many offsets provided")

            do_reg_table(sliceS, indices, hive_base_list, offset_list, bits, value)
        else
            sliceS = to_slice_list(sliceS)
            local pre_dev_type = ""
            local slice_group = {}
            for slice in iter(sliceS) do
                local dev_type = credo.slice_get_device_type(slice).name:lower()
                if pre_dev_type ~= dev_type then
                    if #slice_group > 0 then
                        do_reg_lane_from_hive(slice_group, indices, offset_list, bits, value)
                    end
                    slice_group = {slice}
                else
                    table.insert(slice_group, slice)
                end
                pre_dev_type = dev_type
            end
            if #slice_group > 0 then
                do_reg_lane_from_hive(slice_group, indices, offset_list, bits, value)
            end
        end
    end
end

return reg
