---@module 'regmap'
local regmap = {}

local cjson = require "cjson"

local pf = require "pl.file"
local class = require "pl.class"
local intmod = require "int"

---@class RegisterSegment
---@field public address_offset integer
---@field public width integer
---@field public lsb integer
---@field public reg_lsb integer

---@class RegisterField:pl.Class
---@field public name string
---@field public segments RegisterSegment[]
local RegisterField = class()

regmap.RegisterField = RegisterField

local function bin_to_signed(val, width)
    return val - ((val << 1) & (1 << width))
end

function RegisterField:_init(name, field)
    ---@type string
    self.name = name
    ---@type integer[]
    self.base_addresses = field["bases"]
    ---@type boolean
    self.read_only = field["RO"]
    ---@type boolean
    self.signed = field["sign"]
    ---@type integer
    self.width = 0
    ---@type RegisterSegment[]
    self.segments = {}
    for segment in iter(range(field.parts)) do
        self.segments[segment] = {
            address_offset = field["addr"][segment],
            width = field["width"][segment],
            lsb = field["lsb"][segment],
            reg_lsb = field["reg_lsb"][segment]
        }
        self.width = self.width + field["width"][segment]
    end
end

function RegisterField:__tostring()
    return inspect({
        name = self.name,
        base_addresses = list.map(self.base_addresses, hex),
        read_only = self.read_only,
        segments = self.segments
    }, {rawstring = true})

end

---Read multiple indices at once
---@param slice integer
---@param indices? integer[] indices to read default is all
---@return integer[]
function RegisterField:read_all(slice, indices)
    if indices == nil then
        indices = range(0, #self.base_addresses - 1)
    end
    return list.map(indices, function(index)
        return self:read(slice, index)
    end)
end

---Write multiple indices at once
---@param slice integer
---@param value integer
---@param indices? integer[]
function RegisterField:write_all(slice, value, indices)
    if indices == nil then
        indices = range(0, #self.base_addresses - 1)
    end
    return list.foreach(indices, function(index)
        self:write(slice, value, index)
    end)
end

--- Read specific bits of field
---@note These are Field Bits not Segment Bits
---@param slice integer
---@param index integer
---@param msb? integer
---@param lsb? integer
---@return integer
function RegisterField:read_bits(slice, index, msb, lsb)
    local value = self:read(slice, index)
    lsb = lsb or msb or 0
    msb = msb or self.width
    if lsb > msb then
        msb, lsb = lsb, msb
    end
    return intmod.bitget(value, msb, lsb)
end

--- Write specific bits of the field
---@note These are Field Bits not Segment Bits
---@param slice integer
---@param index integer
---@param value integer
---@param msb? integer
---@param lsb? integer
---@return integer
function RegisterField:write_bits(slice, value, index, msb, lsb)
    lsb = lsb or msb or 0
    msb = msb or self.width
    if lsb > msb then
        msb, lsb = lsb, msb
    end

    if msb == self.width and lsb == 0 then
        self:write(slice, value, index)
    else
        local old_val = self:read(slice, index)
        self:write(slice, intmod.bitset(old_val, value, msb, lsb), index)
    end

    return (value >> lsb) & (((1 << (msb + 1)) - 1) >> lsb)
end

---@param slice integer
---@param index? integer
---@return integer
function RegisterField:read(slice, index)
    if index == nil then
        index = 0
    end
    local val = 0
    local base_address = self.base_addresses[index + 1]
    assert(base_address ~= nil, "Invalid Index %s" % {index})
    for segment in iter(self.segments) do
        local segval = credo.slice_read(slice, int(base_address + segment.address_offset),
                                        segment.lsb + segment.width - 1, segment.lsb)
        val = val | (segval << segment.reg_lsb)
    end
    if self.signed then
        val = bin_to_signed(val, self.width)
    end
    return val
end

---@param slice integer
---@param value integer
---@param index? integer
function RegisterField:write(slice, value, index)
    if index == nil then
        index = 0
    end
    assert(not self.read_only, "%s is read only" % {self.name})
    local base_address = self.base_addresses[index + 1]
    assert(base_address ~= nil, "Invalid Index %s" % {index})

    for segment in iter(self.segments) do
        local segval = (value >> segment.reg_lsb)
        credo.slice_write(slice, int(base_address + segment.address_offset), segval, segment.lsb + segment.width - 1,
                          segment.lsb)
    end
end

---@alias RegisterHive table<string, RegisterField>

---@class RegisterHiveClass: pl.Class
local RegisterHive = class()

---@param name string
---@param fields any
function RegisterHive:_init(name, fields)
    self.name = name
    for fieldname, field in pairs(fields) do
        fieldname = string.lower(fieldname)
        self[fieldname] = RegisterField(fieldname, field)
    end
end

---@alias RegisterMap table<string, RegisterHive>

---@class RegisterMapClass: pl.Class
local RegisterMap = class()

function RegisterMap:_init(filename, regmap_data)
    self.filename = filename
    for hive_name, hive in pairs(regmap_data) do
        hive_name = string.lower(hive_name)
        if type(hive) ~= "table" then
            goto continue
        end
        self[hive_name] = RegisterHive(hive_name, hive["regs"])
        ::continue::
    end
end

---@param file string
---@param is_module? boolean default false
---@return RegisterMap
function regmap.load(file, is_module)
    local data
    if is_module == true then
        local text = require(file).data()
        data = cjson.decode(text)
    else
        data = cjson.decode(pf.read(file))
    end
    return RegisterMap(file, data)
end

---@type table<string, RegisterMap>
local regmap_cache = {}

local function get_regmap_module(slice)
    local dev_type = credo.slice_get_device_type(slice)
    if list.contains({credo.DEV_BLACKHAWK_800, credo.DEV_BLACKHAWK_400}, dev_type) then
        return "regmap.blackhawk"
    elseif list.contains({credo.DEV_OWL_400, credo.DEV_OWL_800}, dev_type) then
        return "regmap.owl"
    elseif list.contains({credo.DEV_OSPREY_400, credo.DEV_OSPREY_800}, dev_type) then
        return "regmap.osprey"
    elseif list.contains({credo.DEV_HERON_1P0, credo.DEV_HERON_MR}, dev_type) then
        return "regmap.heron1p0"
    elseif list.contains({credo.DEV_NUTCRACKER_32}, dev_type) then
        return "regmap.nutcracker"
    elseif list.contains({credo.DEV_SCREAMING_EAGLE, credo.DEV_SCREAMING_EAGLE_AEC}, dev_type) then
        return "regmap.screaming_eagle"
    elseif list.contains({credo.DEV_NIGHTHAWK}, dev_type) then
        return "regmap.nighthawk"
    elseif list.contains({credo.DEV_NIGHTHAWK_1P0}, dev_type) then
        return "regmap.nighthawk1p0"
    elseif list.contains({credo.DEV_MONARCH2P1_TEST, credo.DEV_MONARCH2P1}, dev_type) then
        return "regmap.monarch2p1"
    elseif list.contains({credo.DEV_SKIPPER_TEST, credo.DEV_SKIPPER}, dev_type) then
        return "regmap.skipper"
    else
        return nil
    end
end

---Get register map for slice
---Uses a cache to make sure that a given register map is only loaded once
---@param slice integer
---@return RegisterMap
function regmap.slice(slice)
    local module = get_regmap_module(slice)
    if module == nil then
        error("No register map for slice %d" % {slice})
    end
    local cache_map = regmap_cache[module]
    if cache_map ~= nil then
        return cache_map
    end
    regmap_cache[module] = regmap.load(module, true)
    return regmap_cache[module]
end

return regmap
