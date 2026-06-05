local credo = require "credo"

local fecana = require "commands.common.fecana"

fecana.register_histgroup(credo.FAMILY_OSPREY)
fecana.register_histogram(credo.FAMILY_OSPREY, {
    valid_lane_mode = function(mode)
        return mode == credo.LMODE_PAM4
    end
})
