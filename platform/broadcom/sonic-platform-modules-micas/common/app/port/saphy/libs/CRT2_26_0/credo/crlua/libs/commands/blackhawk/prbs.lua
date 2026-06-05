local slash = require "slash"
local credo = require "credo"

local prbs = require "toolbox.prbs"

slash.register_chip_command({"prbs", "monitor"}, credo.FAMILY_BLACKHAWK, [[
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

slash.register_chip_command({"prbs", "phase_error"}, credo.FAMILY_BLACKHAWK, [[
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

slash.register_chip_command({"prbs", "auto_lock"}, credo.FAMILY_BLACKHAWK, [[
Auto Lock PRBS trafic by tweaking serdes RX controls

Modifies the following until it finds a lock or runs out of options:
- rx polarity
- rx msblsb
- rx precoder

Note: The values found may not indicate what they should truly be as the link partner tx might need changing.

Arguments:

    <lanes>           (lanelist)
    -t,--threshold    (number default 1e-5)    prbs threshold to consider it locked
]], function(slice, argt)
    prbs.find_rx_lock(slice, argt.lanes, argt.threshold)
end)
