-- enable debugger
if os.getenv("LOCAL_LUA_DEBUGGER_VSCODE") == "1" then
    local lpath = os.getenv("LUA_PATH") or ""
    package.path = package.path .. string.sub(lpath, 2)
    require("lldebugger").start()
end

-- allow us to use (20).value on integers for use with enums
debug.setmetatable(0, {
    __index = function(self, param)
        if param == "value" then
            return self
        end
        return nil
    end
})

-- allow python like string multiplication "3" * 4 -> "3333"
local sm = getmetatable("")
function sm.__mul(s, n)
    return string.rep(s, n)
end

-- allow python like adding of strings
function sm.__add(s1, s2)
    return s1 .. s2
end

-- add standalone modules to "builtins"
-- these dont rely on credo information to work
local list = require "pl.List"
local set = require "pl.Set"
local stringx = require "pl.stringx"
local tablex = require "pl.tablex"
local types = require "pl.types"
local utils = require "pl.utils"
local intmod = require "int"
local time = require "time"
local pprint = require "pprint"
local docstring = require "docstring"
local inspect = require "inspect"
local tabulate = require "tabulate"

stringx.import()
stringx.format_operator()

_G.time = time
_G.types = types
_G.list = list
_G.set = set
_G.stringx = stringx
_G.tablex = tablex
_G.inspect = inspect
_G.help = docstring.get_docs
_G.range = list.range
_G.zip = tablex.zip
---@generic T: table, V
---@param t T
---@return fun(table: V[], i?: integer): V
---@return T
---@return integer i
function _G.iter(t)
    ---@diagnostic disable-next-line: return-type-mismatch, missing-return-value
    return list.iter(t)
end
_G.choose = utils.choose
_G.assert_arg = utils.assert_arg
_G.gray_bin = intmod.gray_bin
_G.bin_gray = intmod.bin_gray
_G.int = intmod.int
_G.bin = intmod.bin
_G.hex = intmod.hex
_G.binu = intmod.binu
_G.hexu = intmod.hexu
_G.tabulate = tabulate

local credo = require "credo"
local slash = require "slash"
local interactive = require "interactive"
local shell = require "shell"
local magic = require "magic"

_G.slash = slash
_G.luash = slash -- for old name
_G.credo = credo
_G.magic = magic
_G.pprint = pprint.print
require "magic_builtins"
require "commands"

_G.SHELLENV = {}
if shell.in_shell() then
    shell.register_interactive(interactive.completions, interactive.modify_user_input, interactive.compute_prompt)
    _G.SHELLENV = tablex.copy(_G)
    pprint.install()
end
