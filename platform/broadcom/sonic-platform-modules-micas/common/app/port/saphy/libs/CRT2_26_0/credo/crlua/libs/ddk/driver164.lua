local _driver164 = require "ddk._driver164"

---@module 'ddk.driver164'
local driver164 = {}

function driver164.Init()
    _driver164.Init()
end

function driver164.Exit()
    _driver164.Exit()
end

---@param index integer
---@param slice integer
---@param direction credo.MACsecDirection
---@param flags1? integer
---@param flags2? integer
function driver164.DataPath_Add(index, slice, direction, flags1, flags2)
    _driver164.DataPath_Add(index, slice, direction.value, flags1, flags2)
end

---@param index integer
function driver164.DataPath_Remove(index)
    _driver164.DataPath_Remove(index)
end

---@return integer
function driver164.DataPath_GetCount()
    return _driver164.DataPath_GetCount()
end

---@param index integer
---@return table<string, integer>
function driver164.DataPath_GetProperties(index)
    return _driver164.DataPath_GetProperties(index)
end

return driver164
