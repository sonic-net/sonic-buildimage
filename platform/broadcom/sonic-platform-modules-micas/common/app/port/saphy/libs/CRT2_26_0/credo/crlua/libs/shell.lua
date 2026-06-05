local _shell = require "_shell"

local fort = require "fort"

local om = require "pl.OrderedMap"

---@module 'shell'
local shell = {}

---@return integer[]
function shell.get_slice_list()
    return _shell.get_slice_list()
end

---@return boolean
function shell.in_shell()
    return _shell.in_shell()
end

---@return boolean
function shell.in_server()
    return _shell.in_server()
end

-- allow it to be default set on, useful for when writing new shell commands
---@type boolean
shell.stacktrace = (os.getenv("CREDO_LUA_STACKTRACE") ~= nil)

---register interactive functions
---@param completer fun (buf: string, lc: lightuserdata, pos: integer): nil
---@param mui fun (input: string) : string modify user input
---@param prompt fun (firstline: boolean): string
function shell.register_interactive(completer, mui, prompt)
    _shell.register_interactive(completer, mui, prompt)
end

---@return boolean
function shell.get_slash_mode()
    return _shell.get_slash_mode()
end

---whether color is allowed in display
---@return boolean
function shell.use_color()
    return shell.in_shell()
end

---@param enable boolean
function shell.set_slash_mode(enable)
    _shell.set_slash_mode(enable)
end

---@param slices integer[]
function shell.select_slices(slices)

    local slice_id_map = shell.get_slice_list()
    -- skip non-existant slices
    slices = list.filter(slices, function(value)
        return list.contains(slice_id_map, value)
    end)
    if #slices == 0 then
        error("must give nonzero number of slices")
    end

    local shared_family = credo.slice_get_family(slices[1])
    for slice in iter(slices) do
        local family = credo.slice_get_family(slice)
        assert(shared_family == family, "All slices must be of the same family")
    end

    local slice_selected_map = list.map(slice_id_map, function(slice_id)
        return list.contains(slices, slice_id)
    end)
    _shell.set_selected_slices(slice_selected_map)
end

--- get selected slices
---@return integer[]
function shell.get_selected_slices()
    return _shell.get_selected_slices()
end

---return the first slice of the selected slices
---useful for functions when the slice type is needed for the selected slices
---@return integer
function shell.get_selected_slice()
    return shell.get_selected_slices()[1]
end

function shell.print_device_tree()
    local dev_index = 0
    local ftable = fort.create()
    local selected_slices = shell.get_selected_slices()
    local dev_tree = om()
    for slice in iter(shell.get_slice_list()) do
        local device = credo.slice_get_device(slice)
        if dev_tree[device] == nil then
            dev_tree[device] = {}
        end
        table.insert(dev_tree[device], slice)
    end
    for dev, slices in pairs(dev_tree) do
        dev_index = dev_index + 1
        local first_slice = slices[1]
        local device_type = credo.slice_get_device_type(first_slice)
        for slice in iter(slices) do
            if slice == first_slice then
                ftable:print("%s|%s" % {string.lower(device_type.display_name), string.char((dev_index % 26) + 96)})
            else
                ftable:print("|")
            end
            ftable:print_ln("Slice %d%s" % {slice, choose(list.contains(selected_slices, slice), " [*]", "")})
        end
    end
    ftable:set_border_style(fort.EMPTY_STYLE)
    print(ftable)
end

---@param lc lightuserdata
---@param str string
---@param pos integer
function shell.add_completion(lc, str, pos)
    _shell.add_completion(lc, str, pos)
end

---Get user input
---@param prompt string|nil
---@return string
function shell.input(prompt)
    return _shell.input(prompt)
end

---@param str string
function shell.write(str)
    _shell.write(str)
end

---@param str string
function shell.write_error(str)
    _shell.write_error(str)
end

return shell
