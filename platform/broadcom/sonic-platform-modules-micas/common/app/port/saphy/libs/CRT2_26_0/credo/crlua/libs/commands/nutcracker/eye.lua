local credo = require "credo"
local eye = require "commands.common.eye"

local quality_mapping = {fast = 6, normal = 7, best = 8}

local quality_mapping_timeout = {fast = 10, normal = 30, best = 60}

eye.register_diagram(credo.FAMILY_NUTCRACKER, quality_mapping, quality_mapping_timeout)
eye.register_bathtub(credo.FAMILY_NUTCRACKER, quality_mapping, quality_mapping_timeout)
