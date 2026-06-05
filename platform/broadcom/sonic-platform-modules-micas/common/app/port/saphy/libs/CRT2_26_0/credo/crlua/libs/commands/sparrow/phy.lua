local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"phymode", "config"}, credo.FAMILY_SPARROW, [[
Configure lanes into phy mode

Enabling AN or LT will cause the lane to transmit PRBS when the modes are finished.

Arugments:

    <lanes>      (lanelist)
    <speed>      (speed)
    -m,--mode    (optional lanemode)    Lane mode to use, if not set it will determine base on speed
    --lb                                Enable Loopback
]], ---@param slice integer
---@param argt {lanes: integer[], mode:credo.LaneMode|nil, speed: integer, an: boolean, lt:boolean, lb: boolean}
function(slice, argt)
    local mode = argt.mode
    if mode == nil then
        if argt.speed < 50000 then
            mode = credo.LMODE_NRZ
        else
            mode = credo.LMODE_PAM4
        end
    end
    local flags = 0
    if argt.lb then
        flags = flags | credo.LFLAG_LOOPBACK
    end
    for lane in iter(argt.lanes) do
        credo.phy_configure(slice, lane, mode, argt.speed, flags)
    end
end)

slash.register_chip_command({"phymode", "destroy"}, credo.FAMILY_SPARROW, [[
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
