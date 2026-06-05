local file = require "pl.file"
local libs = require "libs"

local docstring = {}

---@param source string|nil
---@param info debuginfo
local function get_docs_from_source(source, info)
    if source == nil then
        print("No source")
        return
    end
    local lines = stringx.split(source, "\n")
    local start_docs_line = info.linedefined - 1
    while true do
        if start_docs_line < 1 or lines[start_docs_line] == nil then
            break
        end
        if not stringx.startswith(stringx.lstrip(lines[start_docs_line]), "---") then
            break
        end
        start_docs_line = start_docs_line - 1
    end
    start_docs_line = start_docs_line + 1
    local output = ""
    for ln = start_docs_line, info.lastlinedefined do
        local line = lines[ln]
        if line ~= nil then
            output = output .. lines[ln] .. "\n"
        end
    end
    print(output)
end

---@param func function
function docstring.get_docs(func)
    if type(func) ~= "function" then
        print(func)
        return
    end

    local info = debug.getinfo(func)
    if info.source == "stdin" then
        print("From stdin")
    elseif info.source == "=[C]" then
        print("C Function")
    elseif info.lastlinedefined <= 0 or info.linedefined <= 0 then
        print("Unknown Function")
    elseif stringx.startswith(info.source, "@") and stringx.endswith(info.source, ".lua") then
        local source = file.read(info.source:sub(2))
        get_docs_from_source(source, info)
    elseif stringx.startswith(info.source, "=") then
        local source = libs.get_module(info.source:sub(2))
        get_docs_from_source(source, info)
    else
        get_docs_from_source(info.source, info)
    end
end

return docstring
