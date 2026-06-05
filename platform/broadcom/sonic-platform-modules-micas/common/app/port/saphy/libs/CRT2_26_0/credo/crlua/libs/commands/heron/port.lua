local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"

slash.register_chip_command({"retimer", "status"}, credo.FAMILY_HERON, [[
Display Retimer status.
]], function(slice, argt)
    credo.display_info_print(slice, "retimer_status")
end)

slash.register_chip_command_alias({"retimer", "status"}, {"port", "retimer", "status"}, credo.FAMILY_HERON)

slash.register_chip_command({"bitmux", "status"}, credo.FAMILY_HERON, [[
Display Bitmux status
]], function(slice, argt)
    credo.display_info_print(slice, "bitmux_status")
end)

slash.register_chip_command_alias({"bitmux", "status"}, {"port", "bitmux", "status"}, credo.FAMILY_HERON)

slash.register_chip_command({"port", "info"}, credo.FAMILY_HERON, [[
Get port setups.

Arguments:

    <ports>    (portlist default all)    Ports to get setup.

]], function(slice, argt)
    local ftable = fort.create_table()
    fort.printf_ln(ftable, "Port|Mode|Speed|  Link\nHost Line|Host Lanes|Line Lanes|" ..
                       "Host\nLT|Line\nAN LT|PRBS Traffic\nHost Line")
    ---@type integer
    for port in iter(argt.ports) do
        local _, setup = credo.port_get_setup(slice, port)

        if setup == nil then
            fort.printf_ln(ftable, "%d", port)
        else
            local mode = setup.mode.display_name
            local speed = setup.speed / 1000

            local line_an = credo.port_get_option(slice, port, "line_an") ~= 0
            local line_lt = credo.port_get_option(slice, port, "line_lt") ~= 0
            local host_lt = credo.port_get_option(slice, port, "host_lt") ~= 0
            local prbs_traffic = credo.port_get_option(slice, port, "prbs_traffic")
            local host_prbs_traffic = credo.port_get_option(slice, port, "host_prbs_traffic") ~= 0
            local line_prbs_traffic = credo.port_get_option(slice, port, "line_prbs_traffic") ~= 0
            local host_prbs_traffic_str, line_prbs_traffic_str = "off", "off"
            if host_prbs_traffic then
                host_prbs_traffic_str = credo.PrbsPattern(prbs_traffic).display_name
            end
            if line_prbs_traffic then
                line_prbs_traffic_str = credo.PrbsPattern(prbs_traffic).display_name
            end

            local host_main_lane = credo.port_get_option(slice, port, "host_main_lane")
            local line_main_lane = credo.port_get_option(slice, port, "line_main_lane")

            local host_up = credo.port_is_link_up(slice, port, credo.SIDE_HOST)
            local line_up = credo.port_is_link_up(slice, port, credo.SIDE_LINE)

            local host_lanes_str = stringx.join(",", list.map(setup.host_lanes, function(lane)
                local is_main = lane == host_main_lane
                return choose(is_main, "[", "") .. crutil.get_lane_id(slice, lane) .. choose(is_main, "]", "")
            end))
            local line_lanes_str = stringx.join(",", list.map(setup.line_lanes, function(lane)
                local is_main = lane == line_main_lane
                return choose(is_main, "[", "") .. crutil.get_lane_id(slice, lane) .. choose(is_main, "]", "")
            end))

            ftable:print_ln("%d|%s|%.1fG|%4s %4s|%s|%s|%s|%s %s|%s %s" % {
                port, mode, speed, choose(host_up, " Up ", "Down"), choose(line_up, " Up ", "Down"), host_lanes_str,
                line_lanes_str, choose(host_lt, "ON", "OFF"), choose(line_an, "ON", "OFF"),
                choose(line_lt, "ON", "OFF"), host_prbs_traffic_str, line_prbs_traffic_str
            })
        end
    end

    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    print(fort.to_string(ftable))
end)

slash.register_chip_command_alias({"port", "info"}, {"port", "setup"}, credo.FAMILY_HERON)
