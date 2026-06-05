local slash = require "slash"
local credo = require "credo"
local tabulate = require "tabulate"
local crutil = require "crutil"
local common_prbs = require "commands.common.prbs"

slash.register_chip_command({"prbs", "monitor"}, credo.FAMILY_NUTCRACKER, [[
Display prbs rx_monitor information

Can also be used to initialize the monitor. This includes
- Configuring prbs fec analyzer (if not set)
- Clearing fec analyzer counter and timer
- Clearing prbs counter and timer

Arguments:

    -i,--init                              Perform initialization
    -t,--time    (optional time)           Switch from global time to a window time
    <lanes>      (lanelist default all)    Lanes to display / initialize

Examples:

    > prbs monitor            # display prbs monitor all lanes global time
    > prbs monitor 0-7        # display prbs monitor certain lanes global time
    > prbs monitor -i         # init monitor all lanes
    > prbs monitor -i 0-7     # init monitor certain lanes
    > prbs monitor -t 1.5s    # display prbs monitor all lanes for a 1.5sec window
]], function(slice, argt)
    local command = "rx_monitor "
    if argt.init then
        command = command .. "init"
    end
    command = "%s %s" % {command, list.join(argt.lanes, ",")}

    if not argt.init and argt.time ~= nil then
        command = command .. " %d" % {argt.time * 1000}
    end

    credo.display_info_print(slice, command)
end)

slash.register_chip_command({"prbs", "phase_error"}, credo.FAMILY_NUTCRACKER, [[
Display Prbs phase error table.

Arguemnts:

    -d,--duration   (optional time)    Only whole seconds are used
    <lane>          (lane)

]], function(slice, argt)
    if argt.duration == nil then
        credo.display_info_print(slice, "prbs_phase_error %d" % {argt.lane})
    else
        credo.display_info_print(slice, "prbs_phase_error %d %d" % {argt.lane, math.floor(argt.duration)})
    end
end)

slash.register_chip_command({"prbs", "training", "status"}, credo.FAMILY_NUTCRACKER, [[
Display prbs training information

Arguments:

    <lanes>      (lanelist default a*)    Lanes to display
]], function(slice, argt)
    local lanes = argt.lanes ---@type integer[]

    assert(math.max(table.unpack(lanes)) < 8, "Only host lanes supported")

    local data = list.map(lanes, function(lane)
        local row = {}
        row.lane = "%s(%d)" % {crutil.get_lane_id(slice, lane), lane}
        local mode = credo.lane_get_mode(slice, lane)
        row.mode = mode.display_name
        if crutil.is_lane_configured(mode) then
            row.rx_status = credo.prbs_training_rx_get_status(slice, lane).display_name
            row.tx_enabled = choose(credo.prbs_training_tx_is_enabled(slice, lane), "TRAINING", "TRAFFIC")
        end
        return row
    end)

    print(tabulate(data, {
        column = {"lane", "mode", "tx_enabled", "rx_status"},
        header = {lane = "Lane", mode = "Mode", tx_enabled = "TX En", rx_status = "RX Status"}
    }))

end)

slash.register_chip_command({"prbs", "training", "relink"}, credo.FAMILY_NUTCRACKER, [[
Relink RX prbs training

Arguments:

    <lanes>      (lanelist)    Lanes to relink
]], function(slice, argt)
    local lanes = argt.lanes ---@type integer[]
    for lane in iter(lanes) do
        if lane >= 8 or not crutil.is_lane_configured(credo.lane_get_mode(slice, lane)) then
            goto continue
        end
        credo.prbs_training_rx_relink(slice, lane)
        ::continue::
    end
end)

slash.register_chip_command({"prbs", "training", "tx_enable"}, credo.FAMILY_NUTCRACKER, [[
Enable prbs training on TX

Arguments:

    <lanes>      (lanelist)    Lanes to disable
]], function(slice, argt)
    local lanes = argt.lanes ---@type integer[]
    for lane in iter(lanes) do
        if lane >= 8 or not crutil.is_lane_configured(credo.lane_get_mode(slice, lane)) then
            goto continue
        end
        credo.prbs_training_tx_enable(slice, lane, true)
        ::continue::
    end
end)

slash.register_chip_command({"prbs", "training", "tx_disable"}, credo.FAMILY_NUTCRACKER, [[
Disable prbs training on TX

Arguments:

    <lanes>      (lanelist)    Lanes to disable
]], function(slice, argt)
    local lanes = argt.lanes ---@type integer[]
    for lane in iter(lanes) do
        if lane >= 8 or not crutil.is_lane_configured(credo.lane_get_mode(slice, lane)) then
            goto continue
        end
        credo.prbs_training_tx_enable(slice, lane, false)
        ::continue::
    end
end)

common_prbs.register_prbs_berhist(credo.FAMILY_NUTCRACKER, {check_lock = false})
