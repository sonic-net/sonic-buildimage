---@module 'int'
local intmod = {}

--- add hex function for convenience from python
---@param val integer
---@return string
function intmod.hex(val)
    if (val >= 0) then
        return "0x%X" % {val}
    else
        return "-0x%X" % {math.abs(val)}
    end
end

---@param val integer
---@return string
function intmod.bin(val)
    local val_str = ""
    local index = 0
    local neg = ""
    if val < 0 then
        val = math.abs(val)
        neg = "-"
    end
    if val == 0 then
        val_str = "0"
    end
    while (val >> index) > 0 do
        val_str = tostring((val >> index) & 0x1) .. val_str
        index = index + 1
    end
    return neg .. "0b" .. val_str
end

---@param val integer
---@return string
function intmod.hexu(val)
    return "0x%X" % {val}
end

---@param val integer
---@return string
function intmod.binu(val)
    local val_str = ""
    local index = 0
    if val == 0 then
        val_str = "0"
    end
    while (val >> index) ~= 0 do
        val_str = tostring((val >> index) & 0x1) .. val_str
        index = index + 1
    end
    return "0b" .. val_str
end

---@param bin integer
---@return integer
function intmod.bin_gray(bin)
    return bin ~ (bin >> 1)
end

---@param gray integer
---@return integer
function intmod.gray_bin(gray)
    local mask = gray
    while mask ~= 0 do
        mask = mask >> 1
        gray = gray ~ mask
    end
    return gray
end

---@param val integer|number|string|boolean
---@return integer
function intmod.int(val)
    if type(val) == "string" then
        local newval = tonumber(val)
        assert(newval ~= nil, "Invalid literal %s" % {val})
        return math.floor(newval)
    elseif type(val) == "boolean" then
        return choose(val, 1, 0)
    elseif type(val) == "number" then
        local negative = choose(val >= 0, 1, -1)
        return negative * math.floor(math.abs(val))
    else
        error("Cannot convert type to integer: %s" % {type(val)})
    end
end

---@param val integer
---@param msb integer
---@param lsb integer
---@return integer
function intmod.bitget(val, msb, lsb)
    return (val >> lsb) & (((1 << (msb + 1)) - 1) >> lsb)
end

---@param val integer
---@param newval integer
---@param msb integer
---@param lsb integer
---@return integer
function intmod.bitset(val, newval, msb, lsb)
    local mask = ((1 << (msb + 1)) - 1) - ((1 << lsb) - 1)
    val = (val & ~mask) | ((newval << lsb) & mask)
    return val
end

---@param val integer
---@param width integer
---@return integer
function intmod.bin_to_signed(val, width)
    return val - ((val << 1) & (1 << width))
end

return intmod
