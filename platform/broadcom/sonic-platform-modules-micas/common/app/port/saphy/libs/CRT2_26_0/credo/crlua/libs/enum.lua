local class = require "pl.class"

---@module 'enum'
local enum = {}

local BaseEnum = class()

---@class enum.IntEnum
---@field public value integer
---@field public name string|nil
---@field public display_name string
---@field public type string

---@class enum.IntEnumMeta
---@field public keys table<string, integer>
---@field public name string

---@param name string
---@param keys table<string, integer>
---@param flag? boolean default false
---@return any
function enum.IntEnum(name, keys, flag)
    local IntEnumMeta = class({keys = keys, name = name}, {name = name})

    local IntEnum = class(BaseEnum)

    flag = flag or false

    ---@param value integer
    function IntEnum:_init(value)
        self.value = value.value
        assert(type(self.value) == "number")
        self.display_name = "?"
        self.type = name
        for k, v in pairs(keys) do
            if self.value == v then
                self.key = k
                self.name = k

                self.display_name = k
            end
        end
    end
    if not flag then
        function IntEnum:__tostring()
            local enum_str = "<" .. name .. "."
            for k, v in pairs(keys) do
                if self.value == v then
                    enum_str = enum_str .. k .. "|"
                end
            end
            return stringx.rstrip(enum_str, "|.") .. ":" .. "%d" % {self.value} .. ">"
        end
    else
        function IntEnum:__tostring()
            local enum_str = "<" .. name .. "."
            for k, v in pairs(keys) do
                if (v == 0 and self.value == 0) or (v ~= 0 and (self.value & v) == v) then
                    enum_str = enum_str .. k .. "|"
                end
            end
            return stringx.rstrip(enum_str, "|.") .. ":" .. "0x%X" % {self.value} .. ">"
        end
    end

    function IntEnum:__unm()
        return IntEnum(-self.value)
    end
    function IntEnum:__add(other)
        return IntEnum(self.value + other.value)
    end
    function IntEnum:__sub(other)
        return IntEnum(self.value - other.value)
    end
    function IntEnum:__band(other)
        return IntEnum(self.value & other.value)
    end
    function IntEnum:__bor(other)
        return IntEnum(self.value | other.value)
    end
    function IntEnum:__bxor(other)
        return IntEnum(self.value ^ other.value)
    end
    function IntEnum:__bnot()
        return IntEnum(~self.value)
    end
    function IntEnum:__shl(other)
        return IntEnum(self.value << other.value)
    end
    function IntEnum:__shr(other)
        return IntEnum(self.value >> other.value)
    end
    function IntEnum:__eq(other)
        return self.value == other.value
    end
    -- custom with the power patch
    function IntEnum.__eqval(o1, o2)
        if type(o1) == "number" then
            return o2.value == o1
        else
            return o1.value == o2
        end
    end
    function IntEnum:__lt(other)

        return self.value < other.value
    end
    function IntEnum:__le(other)
        return self.value <= other.value
    end

    for k, v in pairs(keys) do
        IntEnumMeta[k] = IntEnum(v)
    end

    setmetatable(IntEnumMeta, {
        __call = function(self, value)
            return IntEnum(value)
        end,
        __tostring = function()
            local base = "IntEnum"
            if flag then
                base = "IntEnumFlag"
            end
            local output = ""
            for key, val in pairs(keys) do
                if flag then
                    output = "%s\n    %s = 0x%X" % {output, key, val}
                else
                    output = "%s\n    %s = %d" % {output, key, val}
                end
            end
            return "<%s(%s) %s\n/>" % {name, base, output}
        end
    })

    return IntEnumMeta
end

return enum
