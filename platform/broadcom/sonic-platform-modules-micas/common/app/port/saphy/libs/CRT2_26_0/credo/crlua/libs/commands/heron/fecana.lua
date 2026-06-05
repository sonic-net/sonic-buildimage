local fecana = require "commands.common.fecana"

fecana.register_histgroup(credo.FAMILY_HERON)

fecana.register_histogram(credo.FAMILY_HERON, {
    valid_lane_mode = function(mode)
        return mode == credo.LMODE_PAM4
    end
})
