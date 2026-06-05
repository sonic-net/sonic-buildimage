local inspect = require "inspect"

local strt = {}

local function update_struct_index(struct)
    -- would like not to have to warp __index and __newindex
    -- but SWIG checks the function is C before calling it (I guess as a protective measure).
    local struct_mt = getmetatable(struct)
    local inst = struct_mt[".instance"]
    -- already set
    if inst["__oldindex"] ~= nil then
        return
    end
    inst["__oldindex"] = inst["__index"]
    inst["__index"] = function(self, iparam)
        if type(inst[".get"][iparam]) == "function" then
            return inst[".get"][iparam](self)
        end
        return inst["__oldindex"](self, iparam)
    end
    inst["__oldnewindex"] = inst["__newindex"]
    inst["__newindex"] = function(self, iparam, val)
        if type(inst[".set"][iparam]) == "function" then
            return inst[".set"][iparam](self, val)
        end
        return inst["__oldnewindex"](self, iparam, val)
    end
end

---@param struct any
---@param member string
---@param enum enum.IntEnumMeta
---@param optional? boolean
function strt.enum(struct, member, enum, optional)
    if optional == nil then
        optional = false
    end
    update_struct_index(struct)

    local struct_mt = getmetatable(struct)
    local inst = struct_mt[".instance"]
    local getter = inst[".get"][member]
    local setter = nil
    if inst[".set"] then
        setter = inst[".set"][member]
    end

    if optional then
        if setter ~= nil then
            inst[".set"][member] = function(self, val)
                if val == nil then
                    setter(self, nil)
                else
                    setter(self, val.value)
                end
            end
        end
        inst[".get"][member] = function(self)
            local val = getter(self)
            if val == nil then
                return nil
            end
            return enum(val)
        end
    else
        if setter ~= nil then
            inst[".set"][member] = function(self, val)
                setter(self, val.value)
            end
        end
        inst[".get"][member] = function(self)
            return enum(getter(self))
        end
    end

end

local function userdata_tostring(self)
    local mt = getmetatable(self)
    local print_data = {}
    local print_type = (mt[".type"] or "UserData") .. " "
    if mt[".get"] ~= nil then
        for name, _ in pairs(mt[".get"]) do

            local val = self[name]
            local v_mt = getmetatable(val)
            if val == nil then
                val = "nil"
            elseif type(val) == "number" then
                if types.is_integer(val) then
                    val = int(val)
                end
            elseif type(v_mt) == "table" and v_mt[".get"] ~= nil then
                val = stringx.indent(tostring(val), 2):sub(3, -2)
            end
            print_data[name] = val
        end
    end
    return print_type .. inspect(print_data, {rawstring = true})
end

local function userdatastruct_tostring(self)
    local mt = getmetatable(self)
    local print_type = (mt[".instance"][".type"] or "UserData") .. " {"
    if mt[".instance"][".get"] ~= nil then
        print_type = print_type .. "\n"
        for name, _ in pairs(mt[".instance"][".get"]) do
            print_type = print_type .. "  %s,\n" % {name}
        end
    end
    return print_type .. "}"
end

function strt.set_struct_tostrings(mod)
    for _, v in pairs(mod) do
        if type(v) == "table" then
            local mt = getmetatable(v)
            if mt ~= nil and mt[".instance"] ~= nil then
                mt[".instance"].__tostring = userdata_tostring
                mt.__tostring = userdatastruct_tostring
            end
        end
    end
end

return strt
