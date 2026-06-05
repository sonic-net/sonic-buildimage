local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"

local RETIMER_STATE_NAME = {
    [0] = "-",
    [1] = "START",
    [2] = "WAIT_PHYMODE",
    [3] = "WAIT_LT",
    [4] = "ADJUST_FIFO",
    [5] = "TRACK",
    [6] = "RESET_FIFO"
}

slash.register_chip_command({"retimer", "status"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display Retimer Status.

Arugments:

    <ports>    (portlist default all)    ports to display
    -o,--old                             use old display method
]], function(slice, argt)

    if argt.old then
        credo.display_info_print(slice, "retimer_status")
        return
    end

    ---@type integer[]
    local ports = argt.ports

    local ftable = fort.create()
    ftable:print_ln("Port|Host\nState|Host\nLane|Host\n# Heals|FIFO|Line\n# Heals|Line\nLane|Line\nState")
    ftable:add_separator()

    for port in iter(ports) do
        local _, setup = credo.port_get_setup(slice, port)
        if setup == nil or setup.mode ~= credo.PORT_RETIMER then
            goto continue
        end

        local direction = credo.port_get_option(slice, port, "direction")
        local host_state = credo.port_get_param(slice, 'retimer_host_state', port)
        local line_state = credo.port_get_param(slice, 'retimer_line_state', port)

        local host_state_str = RETIMER_STATE_NAME[host_state] or '?'
        local line_state_str = RETIMER_STATE_NAME[line_state] or '?'

        for i = 1, #setup.host_lanes do
            if i == 1 then
                ftable:print("%d|%s (%d)" % {port, host_state_str, host_state})
            else
                ftable:print("|")
            end

            local host_lane = setup.host_lanes[i]
            local host_lane_id = crutil.get_lane_id(slice, host_lane)
            local host_lane_fifo = credo.port_get_param(slice, "retimer_fifo", host_lane)
            local host_lane_heal = credo.firmware_debug_cmd(slice, host_lane, 4, 7)

            ftable:print("%s(%2d) RX|%d" % {host_lane_id, host_lane, host_lane_heal})
            if direction == 2 then
                ftable:print("-----X---->")
            else
                ftable:print("-->[%d,%d]-->" % {host_lane_fifo[1], host_lane_fifo[2]})
            end

            local line_lane = setup.line_lanes[i]
            local line_lane_id = crutil.get_lane_id(slice, line_lane)
            local line_lane_fifo = credo.port_get_param(slice, "retimer_fifo", line_lane)
            local line_lane_heal = credo.firmware_debug_cmd(slice, line_lane, 4, 7)

            ftable:print("%d|TX %s(%2d)" % {line_lane_heal, line_lane_id, line_lane})

            if i == 1 then
                ftable:print("%s (%d)" % {line_state_str, line_state})
            end
            ftable:ln()
            ftable:print("||   TX|")
            if direction == 1 then
                ftable:print_ln(" <----X----- ||RX")
            else
                ftable:print_ln(" <--[%d,%d]<-- ||RX" % {line_lane_fifo[1], line_lane_fifo[2]})
            end
        end
        ftable:set_cell_prop(fort.ANY_ROW, 3, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        -- local bitmux_host_state =
        ftable:add_separator()
        ::continue::
    end

    print(ftable)
end)

slash.register_chip_command({"retimer", "config"}, credo.FAMILY_SCREAMING_EAGLE, [[
Configure a retimer port.

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

    -d,--direction  (optional egress|ingress)   Uni-retimer, egress means host to line
    -f,--flexspeed  (optional speedf)           Flexspeed configuration


    Other:
    -n,--dry-run       Print out the configuration that will be created
    -b,--build-only    Build the port, but do not start

Examples:

    > /retimer config 0 200g a0-a1 b0-b1                 # 200g retimer port_id=0 mapping=(a0,a1 <-> b0,b1)
    > /retimer config 0 200g a0-a1 b0-b1 --line-anlt     # add line side anlt
]], function(slice, argt)
    if argt.dry_run then
        print(argt)
        return
    end

    ---@type integer
    local port_id = argt.port_id
    local setup = credo.PortSetup()
    setup.mode = credo.PORT_RETIMER
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
    if argt.direction == "egress" then
        credo.port_set_option(slice, port_id, 'direction', 1)
    end
    if argt.direction == "ingress" then
        credo.port_set_option(slice, port_id, 'direction', 2)
    end
    if argt.flexspeed ~= nil then
        credo.port_set_option(slice, port_id, "flexspeed_kbps", int(argt.flexspeed / 1000))
    end

    if argt.build_only then
        return
    end

    credo.port_start(slice, port_id)
end)
