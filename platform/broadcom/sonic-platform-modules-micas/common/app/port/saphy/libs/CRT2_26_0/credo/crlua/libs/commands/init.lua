local credo = require "credo"
local shell = require "shell"

-- load common commands
require "commands.fecana"
require "commands.fw"
require "commands.lane"
require "commands.logger"
require "commands.meta"
require "commands.port"
require "commands.prbs"
require "commands.reg"
require "commands.serdes"
require "commands.slice"
require "commands.types"

---@type table<integer, fun():nil>
local chip_family_map = {
    [credo.FAMILY_BLACKHAWK.value] = function()
        require "commands.blackhawk"
    end,
    [credo.FAMILY_HERON.value] = function()
        require "commands.heron"
    end,
    [credo.FAMILY_OSPREY.value] = function()
        require "commands.osprey"
    end,
    [credo.FAMILY_NUTCRACKER.value] = function()
        require "commands.nutcracker"
    end,
    [credo.FAMILY_BALDEAGLE.value] = function()
        require "commands.bald_eagle"
    end,
    [credo.FAMILY_VICEROY.value] = function()
        require "commands.viceroy"
    end,
    [credo.FAMILY_SCREAMING_EAGLE.value] = function()
        require "commands.screaming_eagle"
    end,
    [credo.FAMILY_NIGHTHAWK.value] = function()
        require "commands.nighthawk"
    end,
    [credo.FAMILY_RAVEN.value] = function()
        require "commands.raven"
    end,
    [credo.FAMILY_SPARROW.value] = function()
        require "commands.sparrow"
    end,
    [credo.FAMILY_MONARCH2P1_TEST.value] = function()
        require "commands.monarch2p1_test"
    end,
    [credo.FAMILY_SKIPPER_TEST.value] = function()
        require "commands.skipper_test"
    end
}

-- lazy load chip commands to speed up the start up
for slice in iter(shell.get_slice_list()) do
    local slice_family = credo.slice_get_family(slice)
    local load_family_commands = chip_family_map[slice_family.value]
    if load_family_commands ~= nil then
        load_family_commands()
    end
end
