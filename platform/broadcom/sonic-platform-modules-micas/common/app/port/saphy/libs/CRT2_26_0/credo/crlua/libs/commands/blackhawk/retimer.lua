local credo = require "credo"
local slash = require "slash"

slash.register_chip_command({"retimer", "config"}, credo.FAMILY_BLACKHAWK, [[
    Configure a Retimer port

    Simplifying the creation of a retimer with parameters that are more meaningful, and implicitly computing other values
    such as line lanes.

    Arguments:

        <speed>         (speed)                                 Speed of port
        <mapping>       (m1-1|m1x1|m2-2|m2x2|m4-4|m4x4|m8-8)    Mapping of retimer. 'x' means cross mode
        <host_lanes>    (lanelist)                              Host start lane(s) of the port(s)
        -g,--flags      (portflags default 0)                   Port flags
        -e,--fec        (fec default none)                      Port fec type
        -f,--force                                              Force configure even if lanes are used in another port
        -n,--dry-run                                            Do not configure the port, print out instead

    Examples:

        > retimer config 200g m4-4 0,4       # configure 2-200G retimers on host lanes 0,4
        > retimer config 200g m4x4 0,4       # same as above but in cross mode where it connects to different
                                                  # line lanes
        > retimer config 200g m4x4 0,4 -n    # dry run, displays the port configs it would run
        > retimer config 50g m1-1 0:7        # 8-50G retimer in cross mode
        > retimer config 400g m8-8 0         # 1-400G retimer
        > retimer config 400g m8-8 0 -f      # same as above but destroys other ports to get needed lanes

    Cross Mode:

    When the retimer maps to alternate line lanes:
        normal     cross
        0 ->  8    0 -> 12
        3 -> 11    3 -> 15
        4 -> 12    4 ->  8
        7 -> 15    7 -> 11
    ]], function(slice, argt)

    ---@type boolean
    local force = argt.force

    ---@param host_lane integer
    for index, host_lane in pairs(argt.host_lanes) do
        local host_lane_count = int(string.sub(argt.mapping, 2, 2))
        local port_config = credo.PortConfig()
        local cross = stringx.count(argt.mapping, "x") > 0
        port_config.port_id = credo.PORT_AUTO_ASSIGN_ID
        port_config.port_mode = credo.PMODE_SERDES
        port_config.port_type = credo.PORT_RETIMER
        port_config.speed = argt.speed
        port_config.host_start_lane = host_lane
        port_config.host_no_of_lanes = host_lane_count
        port_config.host_fec_type = argt.fec

        if cross then
            port_config.line_start_lane = host_lane + choose(host_lane < 4, 12, 4)
        else
            port_config.line_start_lane = host_lane + 8
        end
        port_config.line_no_of_lanes = host_lane_count
        port_config.line_fec_type = argt.fec
        port_config.flags = argt.flags

        if argt.dry_run then
            print("Config %d/%d" % {index, #argt.host_lanes})
            print(port_config)
        else
            credo.port_configure(slice, port_config, force)
        end

    end
end)

slash.register_chip_command_alias({"retimer", "config"}, {"port", "retimer", "config"}, credo.FAMILY_BLACKHAWK)

slash.register_chip_command({"retimer", "status"}, credo.FAMILY_BLACKHAWK, [[
Display Retimer status.

]], function(slice, argt)
    credo.display_info_print(slice, "retimer_status")
end)

slash.register_chip_command_alias({"retimer", "status"}, {"port", "retimer", "status"}, credo.FAMILY_BLACKHAWK)
