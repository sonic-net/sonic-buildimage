local lapp = require "pl.lapp"
local shlex = require "shlex"

local magic = {}

setmetatable(magic, magic)

---@alias magic.CommandRunner fun(argt: table<string, any>, command: magic.Command, argv: string[], argstr: string)
---@alias magic.CellCommandRunner fun(argt: table<string, any>, cell: string)
---@class magic.Command
---@field public description string
---@field public runner magic.CommandRunner
---@field public raw boolean

---@type table<string, magic.Command>
local commands = {}

---@class magic.CellCommand
---@field public description string
---@field public runner magic.CellCommandRunner

---@type table<string, magic.CellCommand>
local cell_commands = {}

---register a magic command
---@param prefix string
---@param description string
---@param runner magic.CommandRunner
---@param raw? boolean
function magic.register(prefix, description, runner, raw)
    raw = choose(raw ~= nil, raw, false)
    commands[prefix] = {description = description, runner = runner, raw = raw}
end

---register a magic cell command
---@param prefix string
---@param description string
---@param runner magic.CellCommandRunner
function magic.register_cell(prefix, description, runner)
    cell_commands[prefix] = {description = description, runner = runner}
end

---@param command string
function magic.run(command)
    ---@type string[]
    local argv = shlex.split(command)
    assert(#argv >= 1, "No magic command: " .. command)
    local prefix = argv[1]

    local command_handler = commands[prefix]

    assert(command_handler ~= nil, "No magic command: " .. prefix)

    -- on raw command skip string processing
    if command_handler.raw then
        command_handler.runner({}, command_handler, list.slice(argv, 2), command)
        return
    end

    local argt = lapp.process_options_string(command_handler.description, list.slice(argv, 2))
    -- help returns nil
    if argt ~= nil then
        command_handler.runner(argt, command_handler, list.slice(argv, 2), command)
    end
end

---@param command string
function magic:__call(command)
    magic.run(command)
end

function magic.run_cell(command, cell)
    ---@type string[]
    local argv = stringx.split(command)
    assert(#argv >= 1, "No magic command: " .. command)
    local prefix = argv[1]

    local command_handler = cell_commands[prefix]

    assert(command_handler ~= nil, "No magic command: " .. prefix)

    local argt = lapp.process_options_string(command_handler.description, list.slice(argv, 2))
    -- help returns nil
    if argt ~= nil then
        command_handler.runner(argt, cell)
    end
end

---@return string[]
function magic.commands()
    return tablex.keys(commands)
end

---@return string[]
function magic.cell_commands()
    return tablex.keys(cell_commands)
end

return magic
