local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"

local BITMUX_STATE_NAME = {
    [0] = "START",
    [1] = "WAIT_PHYMODE",
    [2] = "WAIT_LT",
    [3] = "RELEASE_FIFO",
    [4] = "CHECK_FIFO",
    [5] = "FUNCTIONAL",
    [6] = "DONE"
}

slash.register_chip_command({"bitmux", "status"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display Bitmux Status.

Arugments:

    <ports>    (portlist default all)    ports to display
    -o,--old                             use old display method
]], function(slice, argt)

    if argt.old then
        credo.display_info_print(slice, "bitmux_status")
        return
    end

    ---@type integer[]
    local ports = argt.ports

    local ftable = fort.create()
    ftable:print_ln("Port|Host\nState|Host\nLn|Host\n# Heal|Host\nFIFO|Line\nFIFO|Line\n# Heal|Line\nln|Line\nState")
    ftable:add_separator()

    for port in iter(ports) do
        local _, setup = credo.port_get_setup(slice, port)
        if setup == nil or setup.mode ~= credo.PORT_BITMUX then
            goto continue
        end

        local host_state = credo.port_get_param(slice, 'bitmux_host_state', port)
        local line_state = credo.port_get_param(slice, 'bitmux_line_state', port)

        local host_state_str = BITMUX_STATE_NAME[host_state] or '?'
        local line_state_str = BITMUX_STATE_NAME[line_state] or '?'

        local is_a2b1 = #setup.host_lanes > #setup.line_lanes
        local max_lanes = math.max(#setup.host_lanes, #setup.line_lanes)

        for i = 1, max_lanes do
            if i == 1 then
                ftable:print("%d|%s (%d)" % {port, host_state_str, host_state})
            else
                ftable:print("|")
            end

            local host_lane_index = choose(is_a2b1, i, (i + 1) // 2)
            local line_lane_index = choose(is_a2b1, (i + 1) // 2, i)
            local host_lane = setup.host_lanes[host_lane_index]
            local line_lane = setup.line_lanes[line_lane_index]

            if is_a2b1 or i % 2 == 1 then

                local host_lane_id = crutil.get_lane_id(slice, host_lane)
                local host_lane_fifo = credo.port_get_param(slice, "bitmux_fifo", host_lane)
                local host_heal_count = credo.firmware_debug_cmd(slice, host_lane, 5, 7)

                ftable:print("%s(%2d)|%d|%d,%d" %
                                 {host_lane_id, host_lane, host_heal_count, host_lane_fifo[1], host_lane_fifo[2]})
            else
                ftable:print("||")
            end

            if not is_a2b1 or i % 2 == 1 then

                local line_lane_id = crutil.get_lane_id(slice, line_lane)
                local line_lane_fifo = credo.port_get_param(slice, "bitmux_fifo", line_lane)
                local line_heal_count = credo.firmware_debug_cmd(slice, line_lane, 5, 7)

                ftable:print("%d,%d|%d|%s(%2d)" %
                                 {line_lane_fifo[1], line_lane_fifo[2], line_heal_count, line_lane_id, line_lane})
            else
                ftable:print("||")
            end

            if i == 1 then
                ftable:print("%s (%d)" % {line_state_str, line_state})
            end
            ftable:ln()
        end

        -- local bitmux_host_state =
        ftable:add_separator()
        ::continue::
    end
    print(ftable)
end)

slash.register_chip_command({"bitmux", "config"}, credo.FAMILY_SCREAMING_EAGLE, [[
Configure a bitmux port.

Arguments:

    <port_id>       (port)
    <port_speed>    (speed)
    <host_lanes>    (lanelist)
    <line_lanes>    (lanelist)

    Options:
    --host-mlane    (optional lane)    Specify custom host main lane
    --line-mlane    (optional lane)    Specify custom line main lane
    --line-an                          Enable line side AN
    --line-lt                          Enable line side LT
    --host-lt                          Enable host side LT
    --nrz-mode                         50G default mode will be NRZ

    -f,--flexspeed    (optional speedf)    Flexspeed configuration

    Other:
    -n,--dry-run       Print out the configuration that will be created
    -b,--build-only    Build the port, but do not start

Examples:

    > /bitmux config 0 200g a0-a1 b0-b3                 # 200g bitmux port_id=0 mapping=a0,a1 <-> b0,b1,b2,b3
    > /bitmux config 0 200g a0-a1 b0-b3 --line-anlt     # add line side anlt
]], function(slice, argt)
    if argt.dry_run then
        print(argt)
        return
    end

    ---@type integer
    local port_id = argt.port_id
    local setup = credo.PortSetup()
    setup.mode = credo.PORT_BITMUX
    setup.host_lanes = argt.host_lanes
    setup.line_lanes = argt.line_lanes
    setup.speed = argt.port_speed

    credo.port_build(slice, port_id, setup)

    if argt.host_mlane then
        credo.port_set_option(slice, port_id, 'host_main_lane', argt.host_mlane)
    end
    if argt.line_mlane then
        credo.port_set_option(slice, port_id, 'line_main_lane', argt.line_mlane)
    end
    if argt.host_lt then
        credo.port_set_option(slice, port_id, 'host_lt', 1)
    end
    if argt.line_an then
        credo.port_set_option(slice, port_id, 'line_an', 1)
    end
    if argt.line_lt then
        credo.port_set_option(slice, port_id, 'line_lt', 1)
    end
    if argt.nrz_mode then
        credo.port_set_option(slice, port_id, '50g_nrz_mode', 1)
    end
    if argt.flexspeed ~= nil then
        credo.port_set_option(slice, port_id, "flexspeed_kbps", int(argt.flexspeed / 1000))
    end

    if argt.build_only then
        return
    end

    credo.port_start(slice, port_id)
end)
