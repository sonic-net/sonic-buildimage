local credo = require "credo"

local eye = require "commands.common.eye"

local quality_mapping = {fast = 6, normal = 7, good = 8}

local quality_mapping_duration = {fast = 5, normal = 10, good = 60}

eye.register_bathtub(credo.FAMILY_NIGHTHAWK, quality_mapping, quality_mapping_duration)
