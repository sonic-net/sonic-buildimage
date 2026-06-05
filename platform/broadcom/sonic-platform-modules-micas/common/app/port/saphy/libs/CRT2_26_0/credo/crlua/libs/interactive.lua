local slash = require "slash"
local magic = require "magic"
local shell = require "shell"

local interactive = {}

local function magic_cell_completions(buf, lc, pos)
    -- assumes a % prefix
    local start = stringx.rfind(buf, " ", 2, pos) or 2
    local end_pos = stringx.lfind(buf, " ", pos + 1) or #buf + 1

    -- not completing the command
    if start ~= 2 then
        return
    end

    local before = buf:sub(1, start)
    local after = buf:sub(end_pos)
    local search = buf:sub(start + 1, pos)
    ---@type table<string, {id:string, text: string}>
    local completions = {}
    ---@type string
    for magic_command in iter(magic.cell_commands()) do
        if stringx.startswith(magic_command, search) then
            completions[magic_command] = {text = before .. magic_command .. after, pos = start + #magic_command}
        end
    end
    for _, completion in tablex.sort(completions) do
        shell.add_completion(lc, completion.text, completion.pos)
    end
end

local function magic_completions(buf, lc, pos)
    -- assumes a % prefix
    local start = stringx.rfind(buf, " ", 1, pos) or 1
    local end_pos = stringx.lfind(buf, " ", pos + 1) or #buf + 1

    -- not completing the command
    if start ~= 1 then
        return
    end

    local before = buf:sub(1, start)
    local after = buf:sub(end_pos)
    local search = buf:sub(start + 1, pos)
    ---@type table<string, {id:string, text: string}>
    local completions = {}
    ---@type string
    for magic_command in iter(magic.commands()) do
        if stringx.startswith(magic_command, search) then
            completions[magic_command] = {text = before .. magic_command .. after, pos = start + #magic_command}
        end
    end
    for _, completion in tablex.sort(completions) do
        shell.add_completion(lc, completion.text, completion.pos)
    end
end

local function shell_completions(buf, lc, pos)
    local start = stringx.rfind(buf, " ", 1, pos) or 1
    local end_pos = stringx.lfind(buf, " ", pos + 1) or #buf + 1
    local before = buf:sub(1, start)
    local after = buf:sub(end_pos)
    local search = buf:sub(start + 1, pos)

    ---@type table<string, {id:string, text: string}>
    local completions = {}
    local before_argv = stringx.split(before:sub(2))
    for _, command in ipairs(slash.get_all_commands()) do
        -- if the before subpath match
        if #before_argv >= #command.path or list(before_argv) ~= list.slice(command.path, 1, #before_argv) then
            goto continue
        end
        local path_compl = command.path[#before_argv + 1]
        if stringx.startswith(path_compl, search) then
            completions[path_compl] = {text = before .. path_compl .. after, pos = start + #path_compl}
        end
        ::continue::
    end

    local in_slash_mode = shell.get_slash_mode()
    for _, completion in tablex.sort(completions) do
        -- easiest way to let the code work with slash mode
        if in_slash_mode then
            shell.add_completion(lc, string.sub(completion.text, 2), completion.pos - 1)
        else
            shell.add_completion(lc, completion.text, completion.pos)
        end
    end
end

local lua_keywords = {
    "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in", "local", "nil",
    "not", "or", "repeat", "return", "then", "true", "until", "while"
}

--- Determine all completions that work for current input
---@param buf string
---@param lc lightuserdata
---@param pos integer
local function get_completions(buf, lc, pos)
    -- use alternative completions for shell commands
    if stringx.startswith(buf, "/") then
        if shell.get_selected_slice() ~= nil then
            shell_completions(buf, lc, pos)
        end
        return
    end
    if shell.get_slash_mode() then
        if shell.get_selected_slice() ~= nil then
            shell_completions("/" .. buf, lc, pos + 1)
        end
        return
    end
    if stringx.startswith(buf, "%%") then
        magic_cell_completions(buf, lc, pos)
        return
    end
    if stringx.startswith(buf, "%") then
        magic_completions(buf, lc, pos)
        return
    end
    -- find the position that it will start replacing
    local start = pos
    for i = pos, 0, -1 do
        local c = stringx.at(buf, i)
        start = i
        if c == nil or string.find(c, "[%w._:]") == nil then
            break
        end
    end

    local before = buf:sub(1, start)
    local after = buf:sub(pos + 1)
    local search = buf:sub(start + 1, pos)
    -- if search is just . o : cancel
    if list.contains({".", ":"}, search) then
        return
    end
    -- treat : as . operator for searching
    local search_str = search:gsub(":", ".")
    ---@type string[]
    local search_path = stringx.split(search_str, ".")
    if #search_path == 0 then
        return
    end
    local search_key = list.pop(search_path)
    local obj = _G.SHELLENV
    for subpath in iter(search_path) do
        if obj[subpath] == nil then
            return
        end
        obj = obj[subpath]
    end
    if type(obj) == "userdata" then
        obj = getmetatable(obj)
        if obj == nil then
            return
        end
        if obj[".get"] ~= nil then
            obj = obj[".get"]
        elseif type(obj["__index"]) == "table" then
            obj = obj["__index"]
        end
    elseif type(obj) == "string" then
        obj = getmetatable(obj)["__index"]
    elseif type(obj) == "table" then
        obj = obj
    else
        return
    end
    local completions = {}

    for lua_kw in iter(lua_keywords) do
        if stringx.startswith(lua_kw, search) then
            completions[lua_kw] = {text = before .. lua_kw .. after, pos = start + #lua_kw}
        end
    end

    ---@param object  table<string, any>
    local function insert_pairs(object)
        for key, _ in pairs(object) do
            if type(key) ~= "string" then
                goto end_loop
            end
            -- dont show `_` prefixed keys unless the user explicitly adds `_`
            if #search_key == 0 and stringx.startswith(key, "_") then
                goto end_loop
            end
            if stringx.startswith(key, search_key) then
                local new_compl = key:sub(#search_key + 1)
                completions[search .. new_compl] = {
                    text = before .. search .. new_compl .. after,
                    pos = start + #search + #new_compl
                }
            end
            ::end_loop::
        end
    end

    insert_pairs(obj)
    local mt_obj = getmetatable(obj)
    if mt_obj ~= nil and type(mt_obj[".get"]) == "table" then
        insert_pairs(mt_obj[".get"])
    elseif mt_obj ~= nil and mt_obj[".get"] == nil then
        -- regular object
        insert_pairs(mt_obj)
    end
    for _, compl in tablex.sort(completions) do
        shell.add_completion(lc, compl.text, compl.pos)
    end
end

---@param buf string
---@param lc lightuserdata
---@param pos integer
function interactive.completions(buf, lc, pos)
    -- handle error printing
    local status, err = xpcall(get_completions, debug.traceback, buf, lc, pos)
    -- fail silently in production
    if shell.stacktrace and not status then
        print(err)
    end
end

local in_cell_mode = false
local cell_empty_lines = 0

---@param firstline boolean
---@param input string
---@return string
---@return boolean
function interactive.modify_user_input(firstline, input)
    local in_slash_mode = shell.get_slash_mode()
    if in_slash_mode then
        if input == "/" then
            shell.set_slash_mode(false)
            print("Exiting slash mode")
            return "", true
        end
        return "slash.run([===[%s]===])" % {input}, true
    end
    if in_cell_mode then
        if input == "%%" then
            -- user wants to complete cell
            in_cell_mode = false
            cell_empty_lines = 0
            return "]===])", true
        elseif input == "" then
            cell_empty_lines = cell_empty_lines + 1
            if cell_empty_lines >= 2 then
                -- user wants to complete cell
                in_cell_mode = false
                cell_empty_lines = 0
                return "]===])", true
            end
            return input, true
        else
            cell_empty_lines = 0
            return input, true
        end
    end
    if not firstline then
        return input, false
    end
    if input == "/" then
        shell.set_slash_mode(true)
        print("Entering slash mode -- type and run '/' to exit")
        return "", true
    end
    if input == "?" then
        magic.run('desc')
        return "", true
    end
    if stringx.startswith(input, "%%") and firstline then
        local full_command = input:sub(3)
        local command = stringx.split(full_command, " ")[1]
        assert(list.contains(magic.cell_commands(), command), "Invalid magic cell command: " .. full_command)
        -- just run if user is asking for help
        if stringx.count(full_command, " -h") == 1 or stringx.count(full_command, " --help") == 1 then
            return "magic.run_cell([===[%s]===], '')" % {full_command}, true
        end
        in_cell_mode = true
        return "magic.run_cell([===[%s]===], [===[" % {full_command}, true
    end
    if stringx.startswith(input, "%") then
        return "magic.run([===[%s]===])" % {input:sub(2)}, true
    end
    if stringx.startswith(input, "/") then
        return "slash.run([===[%s]===])" % {input}, true
    end
    if stringx.startswith(input, "!") then
        return 'magic.run("sys " .. [===[%s]===])' % {input:sub(2)}, true
    end
    if stringx.endswith(input, "?") then
        return "help(%s)" % {input:sub(1, -2)}, false
    end
    return input, false
end

function interactive.compute_prompt(firstline)
    local slice_list = stringx.join(',', shell.get_selected_slices())
    local arrows = choose(firstline, " > ", " >> ")
    local slash_mode = shell.get_slash_mode()
    if #slice_list > 40 then -- prevent issues if the prompt is too large
        print("\nSlice " .. slice_list)
        slice_list = "..."
    end
    return "crlua(slice %s)%s" % {slice_list, choose(slash_mode, " / ", arrows)}
end

return interactive
