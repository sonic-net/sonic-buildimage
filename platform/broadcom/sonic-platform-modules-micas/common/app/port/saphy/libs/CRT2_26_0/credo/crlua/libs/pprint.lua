local inspect = require "inspect"

local pprint = {}

local function has_tostring(item)
    local mt = getmetatable(item)
    return mt ~= nil and type(mt.__tostring) == "function"
end

-- store the old print
local base_print = print
pprint.simple = print

pprint.level = 3

function pprint.print(...)
    -- bring default behavior of print nil if no values - not fully right
    local args = table.pack(...)

    local print_list = {}
    for i = 1, args.n do
        local v = args[i]
        if v == nil then
            print_list[i] = "nil"
        elseif type(v) == "table" then
            if has_tostring(v) then
                print_list[i] = v
            else
                print_list[i] = inspect(v, {depth = pprint.level})
            end
        else
            print_list[i] = v
        end
    end
    base_print(table.unpack(print_list))
end

function pprint.install()
    _G.SHELLENV.print = pprint.print
    _G.pprint = pprint.print
end

function pprint.uninstall()
    _G.SHELLENV.print = base_print
    _G.pprint = base_print
end

function pprint.is_installed()
    return _G.SHELLENV.print == pprint.print
end

return pprint
