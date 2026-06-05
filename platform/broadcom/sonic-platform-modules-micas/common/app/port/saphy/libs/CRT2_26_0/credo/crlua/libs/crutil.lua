---@module 'crutil'
local crutil = {}

function crutil.is_lane_configured(mode)
    return not list.contains({credo.LMODE_DISABLE, credo.LMODE_OFF}, mode)
end

---allow users to select all lanes by sides for lanelist
---@param slice integer
---@param side "a"|"b"
---@return integer[]
function crutil.get_side_lanelist(slice, side)

    local device_type = credo.slice_get_device_type(slice)

    -- special cases with disabled lanes
    if list.contains({credo.DEV_OSPREY_800, credo.DEV_OWL_800}, device_type) then
        return choose(side == "a", range(0, 7), range(12, 19))
    end
    -- heron has a side and b side switched
    if list.contains({credo.DEV_HERON_1P0, credo.DEV_HERON_MR}, device_type) then
        return choose(side == "a", range(8, 15), range(0, 7))
    end
    if list.contains({credo.DEV_SPARROW_800}, device_type) then
        return choose(side == "a", range(0, 1), range(2, 5))
    end
    -- default case
    local host_count, line_count = credo.lane_get_count(slice)
    return choose(side == "a", range(0, host_count - 1), range(host_count, host_count + line_count - 1))
end

---@param slice integer
---@param lane integer
---@param safe? boolean  gives '-' on invalid id instead of error. Default true.
---@return string
function crutil.get_lane_id(slice, lane, safe)
    if safe == nil then
        safe = true
    end
    local alanes = crutil.get_side_lanelist(slice, "a")
    local blanes = crutil.get_side_lanelist(slice, "b")

    local id = list.index(alanes, lane)
    if id ~= nil then
        return "A%d" % {id - 1}
    end
    id = list.index(blanes, lane)

    if id ~= nil then
        return "B%d" % {id - 1}
    end
    assert(safe, "Invalid lane for identifier %d" % {lane})
    return "--"
end

function crutil.twos_to_int(twos, width)
    local mask = 1 << (width - 1)
    return (twos & (~mask)) - (twos & mask)
end

function crutil.int_to_twos(value, width)
    return ((1 << width) + value) & ((1 << width) - 1);
end

return crutil
