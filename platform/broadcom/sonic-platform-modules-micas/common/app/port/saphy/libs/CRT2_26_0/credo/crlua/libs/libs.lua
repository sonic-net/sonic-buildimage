local _libs = require "_libs"

local libs = {}

---Get bundled lua module code
---@param mod string
---@return string|nil
function libs.get_module(mod)
    return _libs.get_module(mod)
end

---Get bundle lua module names
---@return string[]
function libs.get_module_list()
    return _libs.get_module_list()
end

return libs
