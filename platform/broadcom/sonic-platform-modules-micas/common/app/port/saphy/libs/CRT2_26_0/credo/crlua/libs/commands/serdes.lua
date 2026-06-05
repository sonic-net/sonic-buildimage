local slash = require "slash"
local credo = require "credo"
local fort = require "fort"

local crutil = require "crutil"

---@param slice integer
---@param index_type credo.ParamIndex
---@return integer[]
local function compute_indices(slice, index_type)
    local indices = {}
    local limits = credo.slice_get_limits(slice)
    if index_type == credo.PARAM_INDEX_LANE then
        indices = range(0, limits.max_lanes - 1)
    elseif index_type == credo.PARAM_INDEX_TOP then
        indices = {0}
    elseif index_type == credo.PARAM_INDEX_SIDE then
        indices = {credo.SIDE_HOST.value, credo.SIDE_LINE.value}
    elseif index_type == credo.PARAM_INDEX_PORT then
        indices = range(0, limits.max_ports - 1)
    else
        error("Unknown index type %d" % {index_type.value})
    end
    return indices
end

slash.register_command({"serdes", "param"}, [[
Display serdes paramter table.

Arguments:

    <lanes>    (lanelist default all)

Example:

    > serdes param           # dispaly all lanes
    > serdes param 0-2       # select lanes

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_param %s" % {list.join(argt.lanes, ",")})
end)

slash.register_command({"serdes", "control"}, [[
Display serdes control table.

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_control")
end)

slash.register_command({"serdes", "config"}, [[
Set/Get a Parameter.

Arguments:

    -x,--hex                                   Show integers as hex
    -d,--definition                            Show the definition for the parameter
    -p,--partial       (optional intnum_map)   Set partial values
    -v,--values        (optional numbers)
    <name>             (optional string)       Parameter name to get/set
    <indices>          (optional indexlist)    Parameter indices to get/set

Examples:

    > /serdes config                            # list out all parameters w/ description
    > /serdes config tx_taps                    # list tx_taps for all indices
    > /serdes config tx_taps 0-7                # list tx_taps for indices 0-7
    > /serdes config tx_taps -d                 # show full definition for parameter
]], function(slice, argt)
    ---@type string
    local name = argt.name

    if name == nil then
        local param_deflist = credo.serdes_get_param_deflist(slice)
        local ftable = fort.create_table()
        fort.printf_ln(ftable, "Param|Description")
        ---@type credo.Param
        for param in iter(param_deflist) do
            ---@type string
            local desc = stringx.fill(param.description, 60)
            if #desc > 60 then
                fort.add_separator(ftable)
            end
            fort.printf_ln(ftable, "%s|%s", param.name, desc)
            if #desc > 60 then
                fort.add_separator(ftable)
            end
        end
        fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
        print(fort.to_string(ftable))
        return
    end

    local paramdef = credo.serdes_find_param_def(slice, name)

    assert(paramdef ~= nil, "Invalid serdes param %s" % {name})

    ---@type integer[]

    local index_bounds = compute_indices(slice, paramdef.index_type)
    local bounds_min, bounds_max = list.minmax(index_bounds)
    local indices = argt.indices or index_bounds
    assert(paramdef ~= nil, "Unable to find parameter %s" % {name})
    local min, max = list.minmax(indices)
    assert(min >= bounds_min, "Invalid index < %d bound" % {bounds_min})
    assert(max <= bounds_max, "Invalid index > %d bound " % {bounds_max})

    if argt.definition then
        print(paramdef)
        return
    end

    if argt.values ~= nil then
        ---@type number[]
        local values = argt.values
        assert(paramdef.has_setter, "Cannot set %s" % {paramdef.name})
        ---@type integer
        for index in iter(indices) do
            local val_count = paramdef.count
            assert(#values >= val_count, "Invalid value count %d for index %d (needs %d)" % {#values, index, val_count})
        end
        for index in iter(indices) do
            if paramdef.index_type == credo.PARAM_INDEX_LANE then
                local lmode = credo.lane_get_mode(slice, index)
                if lmode == credo.LMODE_DISABLE then
                    goto end_loop
                end
            end
            if paramdef.count == 1 then
                credo.serdes_set_param(slice, paramdef.name, index, values[1])
            else
                credo.serdes_set_param(slice, paramdef.name, index, values)
            end
            ::end_loop::
        end
    elseif argt.partial then
        ---@type table<integer, number[]>

        local map = argt.partial
        local map_min, map_max = list.minmax(tablex.keys(map))
        assert(map_min >= 0, "Partial value index bounds >= 0")
        assert(paramdef.count > 1, "No partial setting for single value parameters")
        for index in iter(indices) do
            local val_count = credo.serdes_get_param_val_count(slice, paramdef.name, index)
            assert(map_max < val_count,
                   "Invalid value index %d at param index %s (needs < %d)" % {map_max, index, val_count})
        end
        for index in iter(indices) do
            if paramdef.index_type == credo.PARAM_INDEX_LANE then
                local lmode = credo.lane_get_mode(slice, index)
                if lmode == credo.LMODE_DISABLE then
                    goto end_loop
                end
            end
            local values = credo.serdes_get_param(slice, paramdef.name, index)
            for idx, val in pairs(map) do
                values[idx + 1] = val
            end
            credo.serdes_set_param(slice, paramdef.name, index, values)
            ::end_loop::
        end
    else
        assert(paramdef.has_getter, "Cannot get %s" % {paramdef.name})
        local ftable = fort.create_table()
        local index_name = "Index"
        if paramdef.index_type == credo.PARAM_INDEX_TOP then
            index_name = "Top"
        elseif paramdef.index_type == credo.PARAM_INDEX_LANE then
            index_name = "Lanes"
        elseif paramdef.index_type == credo.PARAM_INDEX_SIDE then
            index_name = "Side"
        elseif paramdef.index_type == credo.PARAM_INDEX_PORT then
            index_name = "Port"
        end
        fort.printf_ln(ftable, "%s|Value%s", index_name, choose(paramdef.count > 1, "s", ""))
        ---@type integer

        for index in iter(indices) do
            fort.printf(ftable, "%d", index)
            if paramdef.index_type == credo.PARAM_INDEX_LANE then
                local lmode = credo.lane_get_mode(slice, index)
                if lmode == credo.LMODE_DISABLE then
                    fort.printf_ln(ftable, "-")
                    goto end_loop
                end
            end
            local vals = credo.serdes_get_param(slice, paramdef.name, index)
            if type(vals) == "table" then
                local count = 0
                if #vals >= 10 then
                    fort.add_separator(ftable)
                end
                for val in iter(vals) do
                    if count >= 10 and count % 10 == 0 then
                        fort.ln(ftable)
                        fort.printf(ftable, "") -- add space for index
                    end
                    count = count + 1
                    if type(val) == "number" then
                        if types.is_integer(val) then
                            if argt.hex then
                                fort.printf(ftable, "%s", hex(val))
                            else
                                fort.printf(ftable, "%d", val, val)
                            end

                        else
                            fort.printf(ftable, "%.2g", val)
                        end
                    else
                        fort.printf(ftable, "%s", tostring(val))
                    end

                end
                fort.ln(ftable)
                if count >= 10 then
                    fort.add_separator(ftable)
                end
            else
                if types.is_integer(vals) then
                    if argt.hex then
                        fort.printf_ln(ftable, "%s", hex(vals))
                    else
                        fort.printf_ln(ftable, "%d", vals, vals)
                    end
                elseif type(vals) == "number" then
                    fort.printf_ln(ftable, "%.02f", tostring(vals))
                else
                    fort.printf_ln(ftable, "%s", tostring(vals))
                end
            end
            ::end_loop::
        end
        if paramdef.count > 1 then
            fort.set_cell_span(ftable, 1, 2, math.min(paramdef.count, 10))
        end
        fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
        fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_LEFT)
        print(fort.to_string(ftable))
    end

end)

---@param slice integer
---@param lanes integer[]
local function print_polmap(slice, lanes)
    local ftable = fort.create_table()
    fort.printf_ln(ftable, "Lane|Tx Pol|Rx Pol")
    ---@type integer
    for lane in iter(lanes) do
        local tx_pol = credo.serdes_get_tx_polarity(slice, lane)
        local rx_pol = credo.serdes_get_rx_polarity(slice, lane)
        fort.printf_ln(ftable, "%s(%d)|%8s|%8s", crutil.get_lane_id(slice, lane), lane,
                       choose(tx_pol, "flip (1)", "norm (0)"), choose(rx_pol, "flip (1)", "norm (0)"))
    end
    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    print(fort.to_string(ftable))
end

slash.register_command({"serdes", "pol"}, [[
Set/Get Serdes polarity

Arguments:

    -d,--direction    (optional both|tx|rx)         Direction of polarities to set
    -f,--flip                                       Flip the polarities from current value
    -v,--value        (optional norm|flip|0|1)      Set the polarities value
    <lanes>           (lanelist default all)

Examples:

    > /serdes pol                            # show all serdes polarities
    > /serdes pol 0:7                        # show serdes polarities for 0-7
    > /serdes pol -d both -f                 # flip all serdes poliarities (norm->flip, flip->norm)
    > /serdes pol -d both -v flip            # set all serdes polarities to flip
    > /serdes pol 0,2,4,7 -d tx -v norm      # set 0,2,4,7 serdes tx polarities to normal

]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes
    ---@type 'both'|'tx'|'rx'|nil
    local direction = argt.direction
    if argt.flip then
        assert(direction ~= nil, "Must specify direction")
        for lane in iter(lanes) do
            if direction == "tx" or direction == "both" then
                local tx_pol = credo.serdes_get_tx_polarity(slice, lane)
                credo.serdes_set_tx_polarity(slice, lane, not tx_pol)
            end
            if direction == "rx" or direction == "both" then
                local rx_pol = credo.serdes_get_rx_polarity(slice, lane)
                credo.serdes_set_rx_polarity(slice, lane, not rx_pol)
            end
        end
    elseif argt.value then
        assert(direction ~= nil, "Must specify direction")
        local value_map = {norm = false, flip = true, ["0"] = false, ["1"] = true}
        local value = value_map[argt.value]
        for lane in iter(lanes) do
            if direction == "tx" or direction == "both" then
                credo.serdes_set_tx_polarity(slice, lane, value)
            end
            if direction == "rx" or direction == "both" then
                credo.serdes_set_rx_polarity(slice, lane, value)
            end
        end
    end
    print_polmap(slice, lanes)
end)
