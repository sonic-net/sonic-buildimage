local zlib = require "zlib"
local b64 = require "base64"
local pf = require "pl.file"

local gen = {}

-- make sure that ci/customer_regmap.py is used first before generating the register maps

---@param path string
---@param chip string
function gen.generate(path, chip)

    local regmap = pf.read(path)

    ---@cast regmap string|nil
    assert(regmap ~= nil, "Regmap not found")

    local regmap_comp = zlib.compress(regmap)
    local regmap_comp_ascii = b64.encode(regmap_comp)
    regmap_comp_ascii = string.gsub(regmap_comp_ascii, "\n", [[\z]] .. "\n")
    regmap_comp_ascii = regmap_comp_ascii:sub(1, -4)

    local module = [==[
local zlib = require "zlib"
local b64 = require "base64"
local data = "\z
%s"

local decompressed = zlib.decompress(b64.decode(data))
local regmap = {}
---@return string
function regmap.data()
    return decompressed
end
return regmap
]==] % {regmap_comp_ascii}

    pf.write("%s.lua" % {chip}, module)
end

return gen
