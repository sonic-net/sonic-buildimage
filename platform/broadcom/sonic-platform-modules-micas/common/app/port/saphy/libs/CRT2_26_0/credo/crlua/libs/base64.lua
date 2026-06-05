local b64 = require "_b64"

---@module 'base64'
local base64 = {}

---Encode raw data to base64 string.
---@param data string
---@return string
function base64.encode(data)
    return b64.encode(data)
end

---Decode base64 string to raw data
---@param data string
---@return string
function base64.decode(data)
    return b64.decode(data)
end

return base64
