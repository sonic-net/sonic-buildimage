local credo = require "credo"
local bitops = require "int"

local nh = {}

local FW_TIME_UNIT = 0.08589934592
local FW_STATE_TIME_UNIT = ((FW_TIME_UNIT * 1000) / 256)

local SE_INFO_ALL_EXIT_CODES = 100
local SE_INFO_ALL_ERROR_CODES = 120
local SE_TOP_INFO = 15

---@param slice integer
---@param lane integer
---@return integer[]
function nh.lane_get_exit_codes(slice, lane)
    return list.map(range(0, 15), function(index)
        return credo.firmware_debug_cmd(slice, lane, SE_TOP_INFO, SE_INFO_ALL_EXIT_CODES + index)
    end)
end

---@param slice integer
---@param lane integer
---@return integer[]
function nh.lane_get_error_codes(slice, lane)
    return list.map(range(0, 15), function(index)
        local val = credo.firmware_debug_cmd(slice, lane, SE_TOP_INFO, SE_INFO_ALL_ERROR_CODES + index) & 0xFF
        return bitops.bin_to_signed(val, 8)
    end)
end

local DUMP_EXIT_CODE_TIMESTAMP = 5800

---@param slice integer
---@param lane integer
---@return integer[]
function nh.lane_get_exit_code_timestamps(slice, lane)
    return list.map(range(0, 15), function(index)
        local lsb, msb = credo.firmware_debug_cmd_ex(slice, lane, SE_TOP_INFO, DUMP_EXIT_CODE_TIMESTAMP + index)
        return (((msb << 16) | lsb)) * FW_STATE_TIME_UNIT / 1000
    end)
end

return nh
