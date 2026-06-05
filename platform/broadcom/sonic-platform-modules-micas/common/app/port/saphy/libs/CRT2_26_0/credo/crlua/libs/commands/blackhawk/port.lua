local slash = require "slash"
local credo = require "credo"
local fort = require "fort"

slash.register_chip_command({"port", "status"}, credo.FAMILY_BLACKHAWK, [[
Display port status.

Arguments:

    <mode>    (optional portmode)    Type of port status to examine.

]], function(slice, argt)
    if argt.mode == nil then
        credo.display_info_print(slice, "retimer_status")
        credo.display_info_print(slice, "bitmux_status")
        credo.display_info_print(slice, "gearbox_status")
    elseif argt.mode == credo.PORT_RETIMER then
        credo.display_info_print(slice, "retimer_status")
    elseif argt.mode == credo.PORT_BITMUX then
        credo.display_info_print(slice, "bitmux_status")
    else
        credo.display_info_print(slice, "gearbox_status")
    end
end)

slash.register_chip_command({"port", "info"}, credo.FAMILY_BLACKHAWK, [[
Display a port information table.

]], function(slice, argt)
    credo.display_info_print(slice, "port_info")
end)

slash.register_chip_command({"port", "link", "wait"}, credo.FAMILY_BLACKHAWK, [[
Wait for a link to come up.

Loop through all the ports checking if the link came up, and how long it took
since the start of the command. This should be called right after the port has
been configured to give the full link-up time.

Arguments:

    <ports>           (portlist)                       ports to check
    <wait>            (time)                           allow a wait timeout to wait until links to come up
    -d,--direction    (host|line|both default both)    check port up in a specific diection
    -e,--error                                         error if wait timed out
    --silent                                           do not print out anything
]], function(slice, argt)
    ---@type table<integer, number>
    local host_port_durations = {}
    ---@type table<integer, number>
    local line_port_durations = {}
    ---@type integer[]
    local configured_ports = {}
    ---@type number
    local wait = argt.wait
    ---@type 'both'|'host'|'line'
    local direction = argt.direction

    for port_id in iter(argt.ports) do
        local port_config = credo.port_query(slice, port_id)
        if port_config.port_id ~= credo.PORT_UNCONFIGURED then
            list.append(configured_ports, port_id)
        end
    end
    local total_ports = #configured_ports

    local start = time.monotonic()

    local function get_duration()
        return time.monotonic() - start
    end

    local finished_ports = 0
    while time.monotonic() - start < wait and finished_ports ~= total_ports do
        for port_id in iter(configured_ports) do
            if direction == "host" and host_port_durations[port_id] == nil then
                local link_up = credo.port_is_link_up(slice, port_id, credo.SIDE_HOST)
                if link_up then
                    finished_ports = finished_ports + 1
                    host_port_durations[port_id] = get_duration()
                end
            elseif direction == "line" and line_port_durations[port_id] ~= nil then
                local link_up = credo.port_is_link_up(slice, port_id, credo.SIDE_LINE)
                if link_up then
                    finished_ports = finished_ports + 1
                    line_port_durations[port_id] = get_duration()
                end
            elseif direction == "both" then
                local new_link = false
                if host_port_durations[port_id] == nil then
                    local link_up = credo.port_is_link_up(slice, port_id, credo.SIDE_HOST)
                    if link_up then
                        new_link = true
                        host_port_durations[port_id] = get_duration()
                    end
                end
                if line_port_durations[port_id] == nil then
                    local link_up = credo.port_is_link_up(slice, port_id, credo.SIDE_LINE)
                    if link_up then
                        new_link = true
                        line_port_durations[port_id] = get_duration()
                    end
                end
                if new_link and line_port_durations[port_id] ~= nil and host_port_durations[port_id] ~= nil then
                    finished_ports = finished_ports + 1
                end
            end
        end
    end
    if finished_ports ~= total_ports and argt.error then
        error("Timeout waiting for ports to link up")
    end

    if argt.silent then
        return
    end

    local ftable = fort.create()
    ftable:print_ln("Port|Host|Line")
    ftable:add_separator()
    for port_id in iter(argt.ports) do
        ftable:print("%d" % {port_id})
        local port_config = credo.port_query(slice, port_id)
        if port_config.port_id == credo.PORT_UNCONFIGURED then
            ftable:print_ln("-|-")
            goto continue
        end

        local host_duration = host_port_durations[port_id]
        local line_duration = line_port_durations[port_id]

        ftable:print("%s" % {choose(host_duration ~= nil, "%.1f" % {host_duration}, "DOWN")})
        ftable:print("%s" % {choose(line_duration ~= nil, "%.1f" % {line_duration}, "DOWN")})
        ftable:ln()
        ::continue::
    end
    print(ftable)

end)

slash.register_chip_command({"port", "link"}, credo.FAMILY_BLACKHAWK, [[
Display port link information.

Arguments:

    <ports>           (portlist default all)           ports to check
]], function(slice, argt)
    local ftable = fort.create()
    ftable:print_ln("Port|Host|Line")
    ftable:add_separator()
    for port_id in iter(argt.ports) do
        ftable:print("%d" % {port_id})
        local port_config = credo.port_query(slice, port_id)
        if port_config.port_id == credo.PORT_UNCONFIGURED then
            ftable:print_ln("-|-")
            goto continue
        end

        local host_up = credo.port_is_link_up(slice, port_id, credo.SIDE_HOST)
        local line_up = credo.port_is_link_up(slice, port_id, credo.SIDE_LINE)

        ftable:print("%s" % {choose(host_up, "UP", "DOWN")})
        ftable:print("%s" % {choose(line_up, "UP", "DOWN")})
        ftable:ln()
        ::continue::
    end
    print(ftable)
end)

slash.register_chip_command({"port", "recovery"}, credo.FAMILY_BLACKHAWK, [[
Get/Set the fast recovery for ports.

Arguments:

    <ports>         (portlist default all)           ports to get fast recovery status
    -s,--side       (host|line|both default both)    side to set timeout for
    -t,--timeout    (optional time)                  duration of fast recovery timeout
]], function(slice, argt)
    ---@type integer[]
    local ports = argt.ports

    if argt.timeout ~= nil then
        ---@type integer
        local timeout = int(argt.timeout * 1000)
        ---@type 'host'|'line'|'both'
        local side = argt.side
        for port_id in iter(ports) do
            local port = credo.port_query(slice, port_id)

            if port.port_id == credo.PORT_UNCONFIGURED then
                goto continue
            end

            local host_lanes = range(port.host_start_lane, port.host_start_lane + port.host_no_of_lanes - 1)
            local line_lanes = range(port.line_start_lane, port.line_start_lane + port.line_no_of_lanes - 1)
            if list.contains({"host", "both"}, side) then
                for host_lane in iter(host_lanes) do
                    credo.lane_set_option(slice, host_lane, "fast_recover_timeout", timeout)
                end
            end
            if list.contains({"line", "both"}, side) then
                for line_lane in iter(line_lanes) do
                    credo.lane_set_option(slice, line_lane, "fast_recover_timeout", timeout)
                end
            end
            ::continue::
        end
        return
    end

    local ftable = fort.create()
    ftable:print_ln("Port|Mode|Timeout\nHost Line|Armed\nHost Line|Count\nHost Line")
    for port_id in iter(ports) do
        local port = credo.port_query(slice, port_id)

        ftable:print("%d" % {port_id})
        if port.port_id == credo.PORT_UNCONFIGURED then
            ftable:print_ln("-")
            goto continue
        end

        local pmode_name = port.port_type.display_name:lower()
        ftable:print(pmode_name)

        local host_start_lane = port.host_start_lane
        local line_start_lane = port.line_start_lane
        local host_lmode = credo.lane_get_mode(slice, host_start_lane)
        local line_lmode = credo.lane_get_mode(slice, line_start_lane)

        -- we will lazily assume all fast recovery are properly set by the user for each lane
        local host_timeout = credo.lane_get_option(slice, host_start_lane, "fast_recover_timeout")
        local line_timeout = credo.lane_get_option(slice, line_start_lane, "fast_recover_timeout")

        ftable:print("%-4s %4s" %
                         {
                choose(host_timeout == 0, "off", tostring(host_timeout)),
                choose(host_timeout == 0, "off", tostring(line_timeout))
            })
        local host_armed
        if host_lmode == credo.LMODE_PAM4 then
            host_armed = credo.firmware_debug_cmd(slice, host_start_lane, 2, 27)
        else
            host_armed = credo.firmware_debug_cmd(slice, host_start_lane, 1, 32)
        end

        local line_armed
        if line_lmode == credo.LMODE_PAM4 then
            line_armed = credo.firmware_debug_cmd(slice, line_start_lane, 2, 27)
        else
            line_armed = credo.firmware_debug_cmd(slice, line_start_lane, 1, 32)
        end
        ftable:print("%-4s %4s" % {choose(host_armed == 0, "off", "on"), choose(line_armed == 0, "off", "on")})

        local host_count, line_count
        if port.port_type == credo.PORT_RETIMER then
            host_count = credo.firmware_debug_cmd(slice, host_start_lane, 3, 10)
            line_count = credo.firmware_debug_cmd(slice, line_start_lane, 3, 10)
        elseif port.port_type == credo.PORT_GEARBOX then
            host_count = credo.firmware_debug_cmd(slice, host_start_lane, 5, 10)
            line_count = credo.firmware_debug_cmd(slice, line_start_lane, 5, 10)
        else
            host_count = credo.firmware_debug_cmd(slice, host_start_lane, 4, 10)
            line_count = credo.firmware_debug_cmd(slice, line_start_lane, 4, 10)
        end

        ftable:print("%4d %4d" % {host_count, line_count})

        ftable:ln()
        ::continue::
    end

    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)

    print(fort.to_string(ftable))
end)
