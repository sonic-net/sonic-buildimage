local credo = require "credo"

local fecana = require "commands.common.fecana"

fecana.register_histogram(credo.FAMILY_BLACKHAWK, {
    valid_lane_mode = function(mode)
        return mode == credo.LMODE_PAM4
    end
})

fecana.register_histgroup(credo.FAMILY_BLACKHAWK)
