local credo = require "credo"
local slash = require "slash"
local fort = require "fort"
local tcm = {}
local intmod = require "int"

---@param family credo.Family
function tcm.register_access_ops(family)

    slash.register_chip_command({"tcm"}, family, [[
Read or write a tcm register or list of tcm regiters.

Arguments:

    <reglist>     (intlist)               List of registers to read
    -b,--bits     (optional reg32bits)    Bits to read for all registers
    -v,--value    (optional integer)      Value to set for all registers, default is just a read
]], function(slice, argt)

        ---@type integer[]
        local reglist = argt.reglist
        ---@type RegisterBits|nil
        local bits = argt.bits

        if argt.value ~= nil then
            ---@type integer
            local newval = argt.value
            for reg in iter(reglist) do
                if bits ~= nil then
                    local val = credo.tcm_read(slice, reg)
                    val = intmod.bitset(val, newval, bits.msb, bits.lsb)
                    credo.tcm_write(slice, reg, val)
                else
                    credo.tcm_write(slice, reg, newval)
                end
            end
            return
        end

        local ftable = fort.create()
        ftable:print_ln("Reg|Value")
        ftable:add_separator()
        for reg in iter(reglist) do
            local val = credo.tcm_read(slice, reg)
            if bits ~= nil then
                val = intmod.bitget(val, bits.msb, bits.lsb)
                ftable:print_ln("0x%X.%s|0x%X" % {reg, bits, val, val})
            else
                ftable:print_ln("0x%X|0x%X" % {reg, val, val})
            end
        end
        print(ftable)
    end)

    slash.register_chip_command({"tcm", "table"}, family, [[
Read/Write a table of tcm register bases + offsets.

Address Computation:
register_address = base_list[column] + offset_list[row]

It is best to make sure base_list is the smaller of the lists, as the table
can grow vertically better than horizontally.

Arguments:

    <base_list>      (intlist)               Base register locations
    <offset_list>    (intlist)               Offset from base register location
    -b,--bits        (optional reg32bits)    Register bits
    -v,--value       (optional integer)      Register value to write

]], function(slice, argt)
        ---@type integer[]
        local base_list = argt.base_list
        ---@type integer[]
        local offset_list = argt.offset_list
        ---@type RegisterBits|nil
        local bits = argt.bits
        if argt.value ~= nil then
            ---@type integer
            local newval = argt.value
            for offset in iter(offset_list) do
                for base in iter(base_list) do
                    if bits ~= nil then
                        local val = credo.tcm_read(slice, base + offset)
                        val = intmod.bitset(val, newval, bits.msb, bits.lsb)
                        credo.tcm_write(slice, base + offset, val)
                    else
                        credo.tcm_write(slice, base + offset, newval)
                    end
                end
            end
        end

        local ftable = fort.create()
        ftable:print("Reg")
        for base in iter(base_list) do
            ftable:print("0x%X" % {base})
        end
        ftable:ln()
        ftable:add_separator()
        for idx, offset in ipairs(offset_list) do

            if bits ~= nil then
                ftable:print("0x%X.%s" % {offset, bits})
            else
                ftable:print("0x%X" % {offset})
            end
            for base in iter(base_list) do
                local val = credo.tcm_read(slice, base + offset)
                if bits ~= nil then
                    val = intmod.bitget(val, bits.msb, bits.lsb)
                end
                ftable:print("0x%X" % {val})
            end
            ftable:ln()
            if idx % 0x10 == 0 then
                ftable:add_separator()
            end
        end
        print(ftable)
    end)

end

return tcm
