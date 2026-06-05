local miniz = require "miniz"

---@module 'zlib'
local zlib = {}

---@alias bytes string

---compress string (data will not be readable or printable)
---@param data string
---@param level? integer
---@param window_size? integer
---@return bytes bytes
function zlib.compress(data, level, window_size)
    return miniz.compress(data, level, window_size)
end

---decompress string (data will not be readable or printable)
---@param data string
---@param level? integer
---@param window_size? integer
---@return string data
function zlib.decompress(data, level, window_size)
    return miniz.decompress(data, level, window_size)
end

---Computes an Adler-32 checksum of data.
---@param data string
---@param prev? integer
---@return integer
function zlib.adler32(data, prev)
    return miniz.adler32(data, prev)
end

---Computes a CRC (Cyclic Redundancy Check) checksum of data.
---@param data string
---@param prev? integer
---@return integer
function zlib.crc32(data, prev)
    return miniz.crc32(data, prev)
end

return zlib
