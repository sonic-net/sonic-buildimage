local slash = require "slash"

slash.register_chip_command({"port", "info"}, credo.FAMILY_OSPREY, [[
    Display a port information table.

    ]], function(slice, argt)
    credo.display_info_print(slice, "port_info")
end)

slash.register_chip_command({"retimer", "config"}, credo.FAMILY_OSPREY, [[
Configure a Retimer port

Simplifying the creation of a retimer with parameters that are more meaningful, and implicitly computing other values
such as line lanes.

Arguments:

    <speed>         (speed)                       Speed of port
    <mapping>       (m1-1|m2-2|m4-4|m8-8)         Mapping of retimer
    <host_lanes>    (lanelist)                    Host start lane(s) of the port(s)
    -g,--flags      (portflags default 0)         Port flags
    -e,--fec        (fec default none)            Port fec type
    -f,--force                                    Force configure even if lanes are used in another port
    -n,--dry-run                                  Do not configure the port, print out instead
    -l,--layer      (portlayer default serdes)    Port layer to run

Examples:

    > port retimer config 200g m4-4 0,4       # configure 2-200G retimers on host lanes 0,4
    > port retimer config 200g m4-4 0,4 -n    # dry run, displays the port configs it would run
    > port retimer config 50g m1-1 0:7        # 8-50G retimer in cross mode
    > port retimer config 400g m8-8 0         # 1-400G retimer
    > port retimer config 400g m8-8 0 -f      # same as above but destroys other ports to get needed lanes

]], function(slice, argt)

    ---@type boolean
    local force = argt.force

    ---@param host_lane integer
    for index, host_lane in pairs(argt.host_lanes) do
        local host_lane_count = int(string.sub(argt.mapping, 2, 2))
        local port_config = credo.PortConfig()
        port_config.port_id = credo.PORT_AUTO_ASSIGN_ID
        port_config.port_mode = argt.layer
        port_config.port_type = credo.PORT_RETIMER
        port_config.speed = argt.speed
        port_config.host_start_lane = host_lane
        port_config.host_no_of_lanes = host_lane_count
        port_config.host_fec_type = argt.fec

        port_config.line_start_lane = host_lane + 12
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

slash.register_chip_command_alias({"retimer", "config"}, {"port", "retimer", "config"}, credo.FAMILY_OSPREY)

slash.register_chip_command({"gearbox", "config"}, credo.FAMILY_OSPREY, [[
Configure a Gearbox port


Simplifying the creation of a gearbox with parameters that are more meaningful, and implicitly computing other values
such as line lanes.

Arguments:

    <speed>            (speed)                  Speed of port
    <host_lanes>       (lanelist)               Host start lanes of the port(s)
    -g,--flags         (portflags default 0)    Port flags
    -e,--fec           (fec default none)       Port line side fec type
    -f,--force                                  Force configure even if lanes are used in another port
    -n,--dry-run                                Do not configure the port, print out instead
    -l,--layer         (portlayer default pcs)

Examples:

    > port gearbox config 100g 0,4                  # 2-100g gearbox on mapping m1
    > port gearbox config 50g 0:6:2                 # 4-50g gearbox on mapping m0
    > port gearbox config 50g 0:6:2 -n              # dry run of above
    > port gearbox config 50g 0:6:2 -g sopt|lopt    # add optical flags

]], function(slice, argt)
    assert(argt.speed == 50000 or argt.speed == 100000,
           "Invalid speed %.1fg, only 50G/100G supported" % {argt.speed / 1000})
    assert(argt.layer ~= credo.PMODE_SERDES, "Gearbox does not support serdes layer")
    ---@type boolean
    local force = argt.force
    local host_lane_count = choose(argt.speed == 50000, 1, 2)
    -- force rs528 on nrz lanes
    if argt.speed == 50000 then
        argt.fec = credo.FEC_RS_528
    end

    ---@param host_lane integer
    for host_idx, host_lane in pairs(argt.host_lanes) do
        ---@type integer
        local line_start_lane = 12 + (2 * host_lane)

        local port_config = credo.PortConfig()
        port_config.port_id = credo.PORT_AUTO_ASSIGN_ID
        port_config.port_mode = argt.layer
        port_config.port_type = credo.PORT_GEARBOX
        port_config.speed = argt.speed
        port_config.host_start_lane = host_lane
        port_config.host_no_of_lanes = host_lane_count
        port_config.host_fec_type = credo.FEC_RS_544
        port_config.line_start_lane = line_start_lane
        port_config.line_no_of_lanes = 2 * host_lane_count
        port_config.line_fec_type = argt.fec
        port_config.flags = argt.flags
        if argt.dry_run then
            print("Config %d/%d" % {host_idx, #argt.host_lanes})
            print(port_config)
        else
            credo.port_configure(slice, port_config, force)
        end
    end
end)

slash.register_chip_command_alias({"gearbox", "config"}, {"port", "gearbox", "config"}, credo.FAMILY_OSPREY)

slash.register_chip_command({"bitmux", "config"}, credo.FAMILY_OSPREY, [[
Configure a BitMux port

Simplifying the creation of a bitmux port with parameters that are more meaningful, and implicitly computing other values
such as line lanes.

Arguments:

    <speed>         (speed)                       Speed of port
    <mapping>       (m1-2|m2-1)
    <host_lanes>    (lanelist)                    Host start lanes of the port(s)
    -g,--flags      (portflags default 0)         Port flags
    -f,--force                                    Force configure even if lanes are used in another port
    -n,--dry-run                                  Do not configure the port, print out instead
    -l,--layer      (portlayer default serdes)

]], function(slice, argt)
    assert(list.contains({40000, 20000}, argt.speed), "Invalid speed %.1fg" % {argt.speed / 1000})
    ---@type boolean
    local force = argt.force
    ---@type string
    local mapping = argt.mapping
    local host_lane_count = int(mapping:sub(2, 2))

    ---@param host_lane integer
    for host_idx, host_lane in pairs(argt.host_lanes) do
        ---@type integer
        local line_start_lane = 12 + (2 * host_lane)

        local port_config = credo.PortConfig()
        port_config.port_id = credo.PORT_AUTO_ASSIGN_ID
        port_config.port_mode = argt.layer
        port_config.port_type = credo.PORT_BITMUX
        port_config.speed = argt.speed
        port_config.host_start_lane = host_lane
        port_config.host_no_of_lanes = host_lane_count
        port_config.host_fec_type = credo.FEC_NONE
        port_config.line_start_lane = line_start_lane
        port_config.line_no_of_lanes = 2 * host_lane_count
        port_config.line_fec_type = credo.FEC_NONE
        port_config.flags = argt.flags
        if argt.dry_run then
            print("Config %d/%d" % {host_idx, #argt.host_lanes})
            print(port_config)
        else
            credo.port_configure(slice, port_config, force)
        end
    end
end)

slash.register_chip_command_alias({"bitmux", "config"}, {"port", "bitmux", "config"}, credo.FAMILY_OSPREY)
