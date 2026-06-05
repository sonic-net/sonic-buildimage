local utils = require "pl.utils"
local class = require "pl.class"
local lapp = require "pl.lapp"
local shlex = require "shlex"

local credo = require "credo"
local shell = require "shell"

local slash = {}

lapp.show_usage_error = "throw"
lapp.slack = true

---@alias slash.CommandRunner fun(slice: integer, argt: table<string, any>, argv: string[])
---@alias slash.CommandRunnerMulti fun(slices: integer[], argt: table<string, any>, argv: string[])

-- Command Creation
---@class slash.Command
---@field public path string[]
---@field public description string
---@field public runner slash.CommandRunner
---@field public raw boolean
---@overload fun(path: string[], description: string, runner: slash.CommandRunner, raw?: integer): slash.Command
local Command = class()

slash.Command = Command

---Create the command
---@param path string[]
---@param description string
---@param runner slash.CommandRunner
---@param flags? integer
function Command:_init(path, description, runner, flags)
    self.path = path
    self.description = description
    self.runner = runner
    flags = flags or 0
    local raw = choose(flags & slash.RAW ~= 0, true, false)
    local multislice = choose(flags & slash.MULTISLICE ~= 0, true, false)
    self.raw = raw
    self.multislice = multislice
end

---Check that a provided path matches
---@param argv string[]
---@param subpath? boolean matches as a subpath
---@return boolean,integer
function Command:matches_path(argv, subpath)
    if subpath == nil then
        subpath = false
    end
    if not subpath and #argv < #self.path then
        return false, 0
    end
    for index, path_item in ipairs(self.path) do
        if subpath and argv[index] == nil then
            break
        end
        if argv[index] ~= path_item then
            return false, 0
        end
    end
    return true, #self.path
end

---@param path string[]
---@param verbose? boolean
function slash.print_possible_commands(path, verbose)
    local path_name = stringx.join(" ", path)
    local output = ""

    local slash_commands = slash.get_all_commands()

    table.sort(slash_commands, function(a, b)
        return stringx.join(" ", a.path) < stringx.join(" ", b.path)
    end)

    local sub_commands = list.filter(slash_commands, function(command)
        local command_name = stringx.join(" ", command.path)
        return #path == 0 or stringx.startswith(command_name, path_name)
    end)
    local sub_commands_count = #sub_commands

    if sub_commands_count == 0 then
        error("Invalid command path: " .. path_name)
    end

    ---@param command slash.Command
    for command in iter(sub_commands) do
        local command_name = stringx.join(" ", command.path)
        if not verbose and sub_commands_count > 1 then
            output = output .. command_name .. "\n"
        else
            output = "%s%s\n%s\n%s\n" % {output, command_name, string.gsub(command_name, ".", "-"), command.description}
        end
    end
    print(output)
end

---@param argv string[]
---@return integer
local function get_subcommand_count(argv)
    local match_count = 0
    for _, slash_command in ipairs(slash.get_all_commands()) do
        local cmd_matches, cmd_match_count = slash_command:matches_path(argv, true)
        if cmd_matches and cmd_match_count > #argv then
            match_count = match_count + 1
        end
    end
    return match_count
end

---Run the shell command
---@param argv string[]
function Command:run(argv)

    local sub_argv = list.slice(argv, #self.path + 1)
    if list.contains(sub_argv, "?") then
        print(self.description)
        local match_count = get_subcommand_count(self.path)
        if match_count > 1 then
            print("Sub Commands\n------------")
            slash.print_possible_commands(self.path)
        end
        return
    end

    local slice_list_selected = shell.get_selected_slices()
    local function runner(slice)
        -- cannot lapp.process on raw command
        if self.raw then
            return self.runner(slice, {}, argv)
        end
        local argt = lapp.process_options_string(self.description, sub_argv)
        -- help returns nil
        if argt ~= nil then
            return self.runner(slice, argt, argv)
        else
            return "help"
        end
    end

    if self.multislice then
        runner(slice_list_selected)
    else
        for slice in iter(slice_list_selected) do
            if #slice_list_selected > 1 then
                print("Slice %d" % {slice})
            end
            local res = runner(slice)
            if res == "help" then
                break
            end
        end
    end
end

-- Command Creation
---@class slash.Type
---@field public name string
---@field public description string
---@field public parser fun(raw: string):any
slash.Type = class()

---Create the command
---@param name string
---@param description string
---@param parser fun(raw: string):any
function slash.Type:_init(name, description, parser)
    self.name = name
    self.description = description
    self.parser = parser
end

-- Command Registration

---@type slash.Command[]
slash.commands = {}

---@type table<string, slash.Type>
slash.types = {}

---@type table<credo.Family, slash.Command[]>
slash.chip_commands = {}

---@return slash.Command[]
function slash.get_all_commands(slice)
    slice = slice or shell.get_selected_slice()
    local slice_family = credo.slice_get_family(slice)
    local chip_commands = slash.chip_commands[slice_family.value]
    if chip_commands == nil then
        return list.new(slash.commands)
    end
    local commands = list()
    list.extend(commands, chip_commands)
    -- Allow chip commands to override/shadow builtin commands
    ---@param command slash.Command
    for command in iter(slash.commands) do
        if #list.filter(commands, function(chip_command)
            local match = command:matches_path(chip_command.path)
            return match and #chip_command.path == #command.path
        end) == 0 then
            list.append(commands, command)
        end
    end

    return commands
end

---Get all types
---@return table<string,slash.Type>
function slash.get_all_types()
    return tablex.copy(slash.types)
end

slash.RAW = 0x1
slash.MULTISLICE = 0x2

---register a command
---@param path string[]
---@param description string
---@param runner slash.CommandRunner
---@param flags? integer
function slash.register_command(path, description, runner, flags)
    assert(type(path) == "table")
    table.insert(slash.commands, 1, slash.Command(path, description, runner, flags))
end

---register a lapp type
---@param name string
---@param description string
---@param parser fun(raw: string):any
function slash.register_type(name, description, parser)
    assert(type(name) == "string")
    slash.types[name] = slash.Type(name, description, parser)
    lapp.add_type(name, parser)
end

---register a chip command
---@param path string[]
---@param family credo.Family
---@param description string
---@param runner slash.CommandRunner
function slash.register_chip_command(path, family, description, runner, flags)
    if slash.chip_commands[family.value] == nil then
        slash.chip_commands[family.value] = {}
    end
    table.insert(slash.chip_commands[family.value], 1, slash.Command(path, description, runner, flags))
end

---register a multi chip command
---@param path string[]
---@param family credo.Family
---@param description string
---@param runner slash.CommandRunnerMulti
function slash.register_chip_command_multi(path, family, description, runner)
    slash.register_chip_command(path, family, description, runner, slash.MULTISLICE)
end

---register a chip alias command
---@param base_path string[]
---@param alias_path string[]
---@param family credo.Family
function slash.register_chip_command_alias(base_path, alias_path, family)

    ---@type slash.Command[]
    local chip_commands = slash.chip_commands[family.value]
    assert(chip_commands ~= nil, "Invalid family for alias: %s" % {family})

    for chip_command in iter(chip_commands) do
        if list(base_path) == list(chip_command.path) then
            list.append(chip_commands, slash.Command(alias_path, chip_command.description, chip_command.runner, choose(
                                                         chip_command.multislice, slash.MULTISLICE, 0) |
                                                         choose(chip_command.raw, slash.RAW, 0)))
            return
        end
    end
    error("Unable to find base path %s to alias", inspect(base_path))
end

-- Command Parsing / Running

---run a basic command
---@param command string
local function run_command(command)
    local base_command = command
    ---@type string[] split into command vector (argv like)
    local argv = shlex.split(command)
    -- empty line or just full of only spaces
    if #argv == 0 then
        return
    end
    ---@type slash.Command
    local selected_command = nil
    local selected_match_count = 0
    for _, slash_command in ipairs(slash.get_all_commands()) do
        local cmd_matches, cmd_match_count = slash_command:matches_path(argv)
        if cmd_matches and cmd_match_count > selected_match_count then
            selected_command = slash_command
            selected_match_count = cmd_match_count
        end
    end

    if selected_command ~= nil then
        selected_command:run(argv)
    elseif #argv > 0 and stringx.endswith(argv[#argv], "?") then
        argv[#argv] = stringx.removesuffix(argv[#argv], "?")
        if argv[#argv] == "" then
            argv = list.slice(argv, 1, -1)
        end
        slash.print_possible_commands(argv, false)
    else
        error("Invalid command %s" % {base_command})
    end
end

---Run a commandset which can be a 1 or many slash.commands
---@param commandset string
function slash.run(commandset)
    ---@type string[]
    local commandset_list = utils.split(commandset, "[;\n]")
    for command in list.iterate(commandset_list) do
        command = stringx.strip(command) -- remove whitespace
        command = stringx.removeprefix(command, "/") -- remove '/' prefix
        -- ignore any comments in the line
        if stringx.count(command, "#") > 0 then
            command = stringx.partition(command, "#")
        end
        if #command > 0 then
            run_command(command)
        end
    end
end

setmetatable(slash, slash)

---@param commandset string
---@param slice? integer|nil
function slash:__call(commandset, slice)
    if slice ~= nil then
        slash.run("slice %d" % {slice})
    end
    slash.run(commandset)
end

return slash
