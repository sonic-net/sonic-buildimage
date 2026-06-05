local credo = require "credo"

local eye = require "commands.common.eye"

local quality_mapping = {fast = 6, normal = 7, good = 8, best = 9}

local quality_mapping_duration = {fast = 5, normal = 10, good = 60, best = 600}

eye.register_bathtub(credo.FAMILY_SCREAMING_EAGLE, quality_mapping, quality_mapping_duration)
