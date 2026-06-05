local credo = require "credo"

local eye = require "commands.common.eye"

eye.register_diagram(credo.FAMILY_OSPREY, {fast = 6, normal = 7, best = 8}, {fast = 10, normal = 15, best = 20})
eye.register_bathtub(credo.FAMILY_OSPREY, {fast = 6, normal = 7, best = 8}, {fast = 10, normal = 15, best = 20})
