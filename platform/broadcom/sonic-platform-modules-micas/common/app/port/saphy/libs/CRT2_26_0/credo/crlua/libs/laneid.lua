local crutil = require "crutil"

---@class LaneID
---@field public physical integer
---@field public physical_id string

---@param slice integer
---@param physical_lane integer
---@return LaneID
return function(slice, physical_lane)
    return {physical = physical_lane, physical_id = crutil.get_lane_id(slice, physical_lane, true)}
end
