local slash = require "slash"
local credo = require "credo"

local opt_mode_map = {full = 0, bypass = 1, partial = 2}

slash.register_chip_command({"phymode", "config"}, credo.FAMILY_NUTCRACKER, [[
Configure lanes into phy mode

Enabling AN or LT will cause the lane to transmit PRBS when the modes are finished.

Arugments:

    <lanes>      (lanelist)
    <speed>      (speed)
    -m,--mode    (optional lanemode)    Lane mode to use, if not set it will determine base on speed
                                        (<50G means NRZ, otherwise PAM4)
    --lt                                Enable Link Training on line side
    --opt        (optional full|bypass|partial) Specify opt mode on host side
]], function(slice, argt)
    local mode = argt.mode
    if mode == nil then
        if argt.speed < 50000 then
            mode = credo.LMODE_NRZ
        else
            mode = credo.LMODE_PAM4
        end
    end
    for lane in iter(argt.lanes) do
        local flags = 0
        if lane >= 8 and argt.lt then
            flags = flags | credo.LFLAG_LT
        end
        if lane < 8 and argt.opt ~= nil then
            credo.lane_set_option(slice, lane, "opti_mode", opt_mode_map[argt.opt])
        end
        credo.phy_configure(slice, lane, mode, argt.speed, flags)
    end
end)

slash.register_chip_command({"phymode", "destroy"}, credo.FAMILY_NUTCRACKER, [[
Destroy phy mode for selected lanes.

Arugments:

    <lanes>    (lanelist)

]], ---@param slice integer
---@param argt {lanes: integer[]}
function(slice, argt)
    for lane in iter(argt.lanes) do
        credo.phy_destroy(slice, lane)
    end
end)
