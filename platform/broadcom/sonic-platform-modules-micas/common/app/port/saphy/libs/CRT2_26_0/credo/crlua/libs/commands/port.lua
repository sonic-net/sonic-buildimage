local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"

slash.register_command({"port", "config"}, [[
Configure a Port fully setting all parameters.

Arguments:

    <mode>          (portmode)                    Port mode of operation
    <speed>         (speed)                       Speed of port
    <host_lanes>    (lanespan)                    Host lane(s) of the port
    <line_lanes>    (lanespan)                    Line lane(s) of the port
    -i,--id         (port default auto)           Port identifier, defaults to be auto configured
    -g,--flags      (portflags default 0)         Port flags
    -l,--layer      (portlayer default serdes)    Port sublayer
    -hf,--hfec      (fec default none)            Port host fec type
    -lf,--lfec      (fec default none)            Port line fec type
    -f,--force                                    Force configure even if lanes are used in another port
    -n,--dry-run                                  Do not configure, just show configuration

Examples:

    > port config retimer 400g 0-7 8-15                        # 400g retimer
    > port config gearbox 100g 0-1 8-11 -hf rs544 -lf rs528    # 100g gearbox
    > port config gearbox 400g 0-7 8-15 -g line_optical        # 400g retimer w/ line side optical

]], function(slice, argt)
    local port_config = credo.PortConfig()
    port_config.port_id = argt.id
    port_config.port_mode = argt.layer
    port_config.port_type = argt.mode
    port_config.speed = argt.speed
    port_config.host_start_lane = argt.host_lanes.start
    port_config.host_no_of_lanes = argt.host_lanes.stop - argt.host_lanes.start + 1
    port_config.host_fec_type = argt.hfec
    port_config.line_start_lane = argt.line_lanes.start
    port_config.line_no_of_lanes = argt.line_lanes.stop - argt.line_lanes.start + 1
    port_config.line_fec_type = argt.lfec
    port_config.flags = argt.flags

    if argt.dry_run then
        print(port_config)
    else
        ---@type boolean
        local force = argt.force
        port_config = credo.port_configure(slice, port_config, force)

        -- print out port configuration
        slash.run("port setup %d" % {port_config.port_id.value})
    end

end)

slash.register_command({"port", "destroy"}, [[
Destroy a Port(s) by id.

Arguments:

    <ids>    (portlist)

]], function(slice, argt)
    local port_ids = argt.ids
    for port_id in iter(port_ids) do
        credo.port_destroy(slice, port_id)
    end
end)

slash.register_command({"port", "setup"}, [[
Get port setups.

Arguments:

    <ports>    (portlist default all)    Ports to get setup.

]], function(slice, argt)
    local ftable = fort.create_table()
    fort.printf_ln(ftable, "Port|Layer|Mode|Speed|Host\nLanes|Line\nLanes|Host\nFEC|Line\nFEC|Flags")
    ---@type integer
    for port in iter(argt.ports) do
        local port_config = credo.port_query(slice, port)

        if port_config.port_id == credo.PORT_UNCONFIGURED then
            fort.printf_ln(ftable, "%d", port)
        else
            local layer = port_config.port_mode.display_name
            local mode = port_config.port_type.display_name
            local speed = port_config.speed / 1000
            local host_fec = port_config.host_fec_type.display_name
            local line_fec = port_config.line_fec_type.display_name
            ---@type string[]

            local flags = ""
            for k, v in pairs(credo.PortFlags.keys) do
                if v.value & port_config.flags ~= 0 then
                    flags = flags .. "," .. k
                end
            end
            if #flags == 0 then
                flags = "NONE"
            else
                -- remove first comma
                flags = flags:sub(2)
            end

            local host_start_lane_id = crutil.get_lane_id(slice, port_config.host_start_lane)
            local host_end_lane_id = crutil.get_lane_id(slice,
                                                        port_config.host_start_lane + port_config.host_no_of_lanes - 1)
            local line_start_lane_id = crutil.get_lane_id(slice, port_config.line_start_lane)
            local line_end_lane_id = crutil.get_lane_id(slice,
                                                        port_config.line_start_lane + port_config.line_no_of_lanes - 1)

            fort.printf(ftable, "%d|%s|%s|%.1fG", port, layer, mode, speed)
            if port_config.host_no_of_lanes == 1 then
                fort.printf(ftable, "%s(%d)", host_start_lane_id, port_config.host_start_lane)
            else
                fort.printf(ftable, "%s-%s(%d-%d)", host_start_lane_id, host_end_lane_id, port_config.host_start_lane,
                            port_config.host_start_lane + port_config.host_no_of_lanes - 1)
            end
            if port_config.line_no_of_lanes == 1 then
                fort.printf(ftable, "%s(%d)", line_start_lane_id, port_config.line_start_lane)
            else
                fort.printf(ftable, "%s-%s(%d-%d)", line_start_lane_id, line_end_lane_id, port_config.line_start_lane,
                            port_config.line_start_lane + port_config.line_no_of_lanes - 1)
            end
            fort.printf(ftable, "%s|%s|%s", host_fec, line_fec, flags)
            fort.ln(ftable)
        end
    end

    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    print(fort.to_string(ftable))
end)
