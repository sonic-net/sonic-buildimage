---@module 'shlex'
-- similar to python shlex library
local shlex = {}

--- custom argsplit that handles quotes + escaped quotes
--- naive support for escaping and quoting, but not a true posix splitter as it would require a PEG parser
---@param text string
---@return string[]
function shlex.split(text)
    local spat, epat, buf, quoted = [=[^(['"])]=], [=[(['"])$]=], nil, nil
    local args = {}
    for str in text:gmatch("%S+") do
        local squoted = str:match(spat)
        local equoted = str:match(epat)
        local escaped = str:match([=[(\*)['"]$]=])
        if squoted and not quoted and not equoted then
            buf, quoted = str, squoted
        elseif buf and equoted == quoted and #escaped % 2 == 0 then
            str, buf, quoted = buf .. ' ' .. str, nil, nil
        elseif buf then
            buf = buf .. ' ' .. str
        end
        if not buf then
            list.append(args, (str:gsub(spat, ""):gsub(epat, "")))
        end
    end
    if buf then
        error("Missing matching quote for " .. buf, 2)
    end
    return args
end

return shlex
