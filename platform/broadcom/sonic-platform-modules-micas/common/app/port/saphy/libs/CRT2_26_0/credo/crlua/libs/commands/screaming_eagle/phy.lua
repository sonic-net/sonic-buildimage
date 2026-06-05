local slash = require "slash"
local credo = require "credo"

---@class se.phymode.config.argt
---@field lanes integer[]
---@field mode credo.LaneMode|nil
---@field speed number
---@field an boolean
---@field lt boolean
---@field lb boolean
---@field flex boolean

slash.register_chip_command({"phymode", "config"}, credo.FAMILY_SCREAMING_EAGLE, [[
Configure lanes into phy mode

Enabling AN or LT will cause the lane to transmit PRBS when the modes are finished.

Arugments:

    <lanes>      (lanelist)
    <speed>      (speedf)
    -m,--mode    (optional lanemode)    Lane mode to use, if not set it will determine base on speed
                                        (<50G means NRZ, otherwise PAM4)
    --an                                Enable Auto Negotiation
    --lt                                Enable Link Training
    --lb                                Enable Loopback
    -f,--flex                           Enable flexspeed
]], ---@param slice integer
---@param argt se.phymode.config.argt
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
    if argt.an then
        flags = flags | credo.LFLAG_AN
    end
    if argt.lt then
        flags = flags | credo.LFLAG_LT
    end
    if argt.lb then
        flags = flags | credo.LFLAG_LOOPBACK
    end
    if argt.flex then
        flags = flags | credo.LFLAG_FLEXSPEED
    end
    for lane in iter(argt.lanes) do
        credo.phy_configure(slice, lane, mode, argt.speed, flags)
    end
end)

slash.register_chip_command({"phymode", "destroy"}, credo.FAMILY_SCREAMING_EAGLE, [[
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
