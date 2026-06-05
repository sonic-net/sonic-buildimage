local slash = require "slash"
local credo = require "credo"
local datacap = require "chips.screaming_eagle.datacap"
local crutil = require "crutil"
local fort = require "fort"

local json = require "dkjson"

slash.register_chip_command_multi({"port", "save"}, credo.FAMILY_SCREAMING_EAGLE, [[
Save port configurations to a file.

This file can then be used in conjunction with `/port load` to capture infomation

Arguments:

    <ports>    (portlist default all)
    -f,--file    (optional file-out)

]], function(slices, argt, argv)
    local saved_data = {}

    saved_data.slice_ids = slices
    saved_data.slices = {}
    local ports = argt.ports ---@type integer[]

    for slice in iter(slices) do
        local slice_data = {}
        slice_data.slice_id = slice
        slice_data.ports = {}
        for port_id in iter(ports) do
            local port_info = {}
            local _, setup = credo.port_get_setup(slice, port_id)
            if setup == nil then
                goto continue
            end
            port_info.host_lanes = setup.host_lanes
            port_info.line_lanes = setup.line_lanes
            port_info.mode = setup.mode.value
            port_info.mode_str = setup.mode.display_name:lower()
            port_info.speed = setup.speed
            port_info.options = {}
            port_info.port_id = port_id

            for option in iter({
                "line_lt", "line_an", "host_lt", "host_main_lane", "line_main_lane", "direction", "an_override",
                "isc_enable", "isc_an_agent", "isc_lane_map_a", "isc_lane_map_b", "isc_host_main_lane",
                "isc_line_main_lane", "50g_nrz_mode", "flexspeed_kbps"
            }) do
                port_info.options[option] = credo.port_get_option(slice, port_id, option)
            end
            table.insert(slice_data.ports, port_info)
            ::continue::
        end
        table.insert(saved_data.slices, slice_data)
    end

    local output = json.encode(saved_data, {indent = 4})
    if argt.file == nil then
        print(output)
    else
        argt.file:write(output)
    end
end)

slash.register_chip_command_multi({"port", "load"}, credo.FAMILY_SCREAMING_EAGLE, [[
Load port setup from file

    <file>    (file-in)    file to load setup
    -a,--all               apply first configuration for all slices
    -f,--force             destroy existing ports if needed
]], function(slices, argt, argv)
    local all = argt.all

    local file = argt.file ---@type file*

    local file_text = file:read("a")
    local config = json.decode(file_text, 1, json.null, nil, nil) ---@type any

    for slice in iter(slices) do
        for slice_config in iter(config.slices) do
            if not all and slice ~= slice_config.slice_id then
                goto continue
            end
            for port in iter(slice_config.ports) do
                local setup = credo.PortSetup()
                setup.host_lanes = port.host_lanes
                setup.line_lanes = port.line_lanes
                setup.mode = port.mode
                setup.speed = port.speed
                local port_id = port.port_id

                if argt.force then
                    credo.port_destroy(slice, port_id)
                end
                credo.port_build(slice, port_id, setup)
                for option_name, option_value in pairs(port.options) do
                    credo.port_set_option(slice, port_id, option_name, option_value)
                end
                credo.port_start(slice, port_id)
            end
            if all then
                break
            end
            ::continue::
        end
    end
end)

slash.register_chip_command({"port", "datalogs"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display lane(s) state machine history logs.

Arugments:

    <ports>      (portlist)
    -f,--file    (optional string)      file to output logs

]], function(slice, argt)
    local data = datacap.lane_data_logs(slice, argt)
    if data ~= nil then
        print(data)
    end
end)

slash.register_chip_command({"port", "info"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display a port information table.

Arguments:

    <ports>    (portlist default all)    ports to examine
    -o,--old                             use the old port info display

]], function(slice, argt)
    if argt.old then
        credo.display_info_print(slice, "port_info")
        return
    end
    local rm_slice = credo.slice_get_option(slice, "isc_slice_id")
    local isc_partnered = rm_slice ~= 0xFFFF

    local ftable = fort.create_table()
    ftable:print_ln("Port|||||Lane Mapping||ANLT|||ISC")

    ftable:print("ID|Mode|Speed|Direction|  Link\nHost Line|Host Lanes|Line Lanes|" ..
                     "Host\nLT|Line\nAN LT|AN\nOverride")
    if isc_partnered then
        ftable:print("Remote %d" % {rm_slice})
    else
        ftable:print("Disabled")
    end
    ftable:ln()
    ftable:add_separator()

    ---@type integer
    for port in iter(argt.ports) do
        local started, setup = credo.port_get_setup(slice, port)

        if setup == nil then
            ftable:print_ln("%d" % {port})
            goto continue
        end

        ftable:print("%d%s" % {port, choose(started, "", "*")})

        local mode = setup.mode.display_name
        local speed = setup.speed / 1000

        local line_an = credo.port_get_option(slice, port, "line_an") ~= 0
        local line_lt = credo.port_get_option(slice, port, "line_lt") ~= 0
        local host_lt = credo.port_get_option(slice, port, "host_lt") ~= 0
        local an_override = credo.port_get_option(slice, port, "an_override") ~= 0
        local direction = credo.port_get_option(slice, port, "direction")

        -- dont detail direction stuff in bidirectional to reduce info overload to read
        local direction_str = ""
        if direction == credo.PORT_DIR_INGRESS then
            direction_str = "LINE->HOST"
        elseif direction == credo.PORT_DIR_EGRESS then
            direction_str = "HOST->LINE"
        end

        local host_main_lane = credo.port_get_option(slice, port, "host_main_lane")
        local line_main_lane = credo.port_get_option(slice, port, "line_main_lane")

        local host_lane_speed = credo.lane_get_speed(slice, host_main_lane) / 1e6
        local line_lane_speed = credo.lane_get_speed(slice, line_main_lane) / 1e6

        local host_up = credo.port_is_link_up(slice, port, credo.SIDE_HOST)
        local line_up = credo.port_is_link_up(slice, port, credo.SIDE_LINE)

        local active_lanes = 0
        if setup.mode == credo.PORT_SWITCHOVER_RETIMER then
            active_lanes = credo.port_get_option(slice, port, "switchover_active_map")
        end

        local host_lanes_str = stringx.join(",", list.map(setup.host_lanes, function(lane)
            local is_main = lane == host_main_lane
            local is_active = (active_lanes & (1 << lane)) ~= 0
            local active_prefix = choose(is_active, "(", "") -- if switchover use parenthesis around active lanes
            local active_suffix = choose(is_active, ")", "")
            return choose(is_main, "[", "") .. active_prefix .. crutil.get_lane_id(slice, lane) .. active_suffix ..
                       choose(is_main, "]", "")
        end))
        local line_lanes_str = stringx.join(",", list.map(setup.line_lanes, function(lane)
            local is_main = lane == line_main_lane
            local is_active = (active_lanes & (1 << lane)) ~= 0
            local active_prefix = choose(is_active, "(", "") -- if switchover use parenthesis around active lanes
            local active_suffix = choose(is_active, ")", "")
            return choose(is_main, "[", "") .. active_prefix .. crutil.get_lane_id(slice, lane) .. active_suffix ..
                       choose(is_main, "]", "")
        end))

        -- core
        ftable:print("%s|%.2fG (%.2fG-%.2fG)|%s|%4s %4s" % {
            mode, speed, host_lane_speed, line_lane_speed, direction_str, choose(host_up, " UP ", "DOWN"),
            choose(line_up, " UP ", "DOWN")
        })
        -- lane mapping
        ftable:print("%s|%s" % {host_lanes_str, line_lanes_str})
        -- anlt
        ftable:print("%s|%s %s|%s" % {
            choose(host_lt, "ON", "OFF"), choose(line_an, "ON", "OFF"), choose(line_lt, "ON", "OFF"),
            choose(an_override, "ON", "OFF")
        })

        if isc_partnered then
            local isc_enabled = credo.port_get_option(slice, port, "isc_enable") ~= 0
            ftable:print("%s" % {choose(isc_enabled, "ON", "OFF")})
        end

        ftable:ln()
        ::continue::
    end
    ftable:set_cell_span(1, 1, 5)
    ftable:set_cell_span(1, 6, 2)
    ftable:set_cell_span(1, 8, 3)
    print(ftable)
end)

slash.register_chip_command_alias({"port", "info"}, {"port", "setup"}, credo.FAMILY_SCREAMING_EAGLE)
