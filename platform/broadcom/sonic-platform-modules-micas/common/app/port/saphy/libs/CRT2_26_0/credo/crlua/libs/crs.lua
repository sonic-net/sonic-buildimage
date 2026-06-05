local _crs = require "_crs"

---@classmod crs
---@overload fun (...:string): any
local crs = {}

---@diagnostic disable-next-line: param-type-mismatch
setmetatable(crs, crs)

---@function crs:__call
---Run crs command
---@vararg string
---@return any
function crs:__call(...)
    local varargs = {...}
    assert(#varargs > 0, "Must provide a command")
    if type(varargs[1]) == "string" then
        local commands = list.split(varargs[1])
        varargs = list.extend(commands, tablex.sub(varargs, 2))
    end

    local command = tablex.reduce(function(a, h)
        if type(h) == "string" then
            return a .. "'%s', " % {h}
        elseif type(h) == "number" then
            if types.is_integer(h) then
                return a .. "%d, " % {h}
            else
                return a .. "%f, " % {h}
            end
        else
            error("Invalid item")
        end
        return a .. tostring(h)
    end, varargs, "")
    command = "crs(%s)" % {string.sub(command, 1, -3)}
    _crs.run_command(command)

    local output_count = _crs.get_output_count()
    if output_count == 0 then
        return
    end

    local retval = {}
    for index = 0, output_count - 1 do
        local subcount = _crs.get_output_subcount(index)
        list.append(retval, {})
        for subi = 0, subcount - 1 do
            local value = _crs.get_output_item(index, subi)
            local num_value = tonumber(value)
            if num_value ~= nil then
                value = num_value
            end
            list.append(retval[index + 1], value)
        end
        if subcount == 1 then
            retval[index + 1] = retval[index + 1][1]
        end
    end
    if output_count == 1 then
        retval = retval[1]
    end
    return retval
end

return crs
