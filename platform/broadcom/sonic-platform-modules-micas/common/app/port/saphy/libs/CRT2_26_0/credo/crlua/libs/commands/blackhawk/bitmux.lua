local credo = require "credo"
local slash = require "slash"

local bitmux_config_table = {
    ["m1-2"] = {{0, 8}, {1, 10}, {2, 12}, {3, 14}},
    ["m1-2a"] = {{0, 8}, {1, 10}, {4, 12}, {5, 14}},
    ["m2-1"] = {{0, 8}, {2, 9}, {4, 10}, {6, 11}},
    -- ["m2-1a"] = {{0, 8}, {2, 9}, {4, 12}, {6, 13}},
    ["m2-4"] = {{0, 8}, {2, 12}},
    ["m2-4a"] = {{0, 8}, {4, 12}},
    ["m4-2"] = {{0, 8}, {4, 10}}
    -- ["m4-2a"] = {{0, 8}, {4, 12}}
}

slash.register_chip_command({"bitmux", "config"}, credo.FAMILY_BLACKHAWK, [[
Configure a bitmux port

Simplifying the creation of a gearbox with parameters that are more meaningful, and implicitly computing other values
such as line lanes.

Arguments:

    <speed>              (speed)                  Speed of port
    <mapping>            (m2-1|m1-2|m1-2a|m4-2|m2-4|m2-4a)    lane count: host - line
    <host_lanes>         (lanelist)               Host start lanes of the port(s)
    -g,--flags           (portflags default 0)    Port flags
    -f,--force                                    Force configure even if lanes are used in another port
    -n,--dry-run                                  Do not configure ports, print out configurations instead

Examples:

    > bitmux config 40g m4-2 0,4           # configure 2-40g (10g-20g) bitmux
    > bitmux config 40g m2-4 0,2           # configure 2-40g (20g-10g) bitmux
    > bitmux config 40g m2-4 0,2 -f        # force configuration and destroy other ports that have necessary lanes

]], function(slice, argt)

    local host_lane_count_raw, line_lane_count_raw = string.match(argt.mapping, "m(%d)-(%d)a?")
    local host_lane_count = int(host_lane_count_raw)
    local line_lane_count = int(line_lane_count_raw)

    for lane in iter(argt.host_lanes) do
        assert(lane % host_lane_count == 0, "Invalid host lane %d" % {lane})
    end

    local line_lanes = list.map(argt.host_lanes, function(host_lane)
        local line_lane_index = tablex.find_if(bitmux_config_table[argt.mapping], function(config)
            return config[1] == host_lane
        end)
        assert(line_lane_index ~= nil, "Invalid host lane %d for mapping %s" % {host_lane, argt.mapping})
        return bitmux_config_table[argt.mapping][line_lane_index][2]
    end)
    ---@type boolean
    local force = argt.force

    for index, lanes in pairs(zip(argt.host_lanes, line_lanes)) do
        ---@type integer,integer
        local host_lane, line_lane = table.unpack(lanes)
        local port_config = credo.PortConfig()
        port_config.port_id = credo.PORT_AUTO_ASSIGN_ID
        port_config.port_mode = credo.PMODE_SERDES
        port_config.port_type = credo.PORT_BITMUX
        port_config.speed = argt.speed
        port_config.host_start_lane = host_lane
        port_config.host_no_of_lanes = host_lane_count
        port_config.host_fec_type = credo.FEC_NONE
        port_config.line_start_lane = line_lane
        port_config.line_no_of_lanes = line_lane_count
        port_config.line_fec_type = credo.FEC_NONE
        port_config.flags = argt.flags
        if argt.dry_run then
            print("Config %d/%d" % {index, #argt.host_lanes})
            print(port_config)
        else
            credo.port_configure(slice, port_config, force)
        end
    end
end)

slash.register_chip_command_alias({"bitmux", "config"}, {"port", "bitmux", "config"}, credo.FAMILY_BLACKHAWK)

slash.register_chip_command({"bitmux", "status"}, credo.FAMILY_BLACKHAWK, [[
Display Retimer status.

]], function(slice, argt)
    credo.display_info_print(slice, "bitmux_status")
end)

slash.register_chip_command_alias({"bitmux", "status"}, {"port", "bitmux", "status"}, credo.FAMILY_BLACKHAWK)
