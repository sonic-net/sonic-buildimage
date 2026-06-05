local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"

local RETIMER_STATE_NAME = {
    [0] = "-",
    [1] = "START",
    [2] = "WAIT_PHY",
    [3] = "WAIT_LT",
    [4] = "ADJUST_FIFO",
    [5] = "TRACK",
    [6] = "RESET_FIFO",
    [32] = "SWITCH",
    [33] = "SWITCH_WAIT_PHY",
    [34] = "SWITCH_RESET_FIFO",
    [35] = "SWITCH_ADJUST_FIFO"
}

slash.register_chip_command({"switchover", "retimer", "status"}, credo.FAMILY_SCREAMING_EAGLE, [[
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
    ftable:print_ln("Port|Broadcast|Status|Mux||")
    ftable:print_ln("|Main State||Active State|Mux 0|Mux1")
    ftable:set_cell_span(1, 4, 3)
    ftable:add_separator()

    for port in iter(ports) do
        local _, setup = credo.port_get_setup(slice, port)
        if setup == nil or setup.mode ~= credo.PORT_SWITCHOVER_RETIMER then
            goto continue
        end

        local active_lane_map = credo.port_get_option(slice, port, "switchover_active_map")
        local active_group = credo.port_get_option(slice, port, "switchover_select")

        ---@type integer[]
        local broadcast_lanes = setup.host_lanes
        local mux_lanes = setup.line_lanes
        local is_aside_broadcast = #setup.host_lanes < #setup.line_lanes
        if not is_aside_broadcast then
            -- swap lanes
            broadcast_lanes = setup.line_lanes
            mux_lanes = setup.host_lanes
        end

        local ftable_fifo = ftable.new()
        ftable_fifo:print_ln("Broadcast||||Mux||")
        ftable_fifo:set_cell_span(1, 1, 3)
        ftable_fifo:set_cell_span(1, 5, 3)
        ftable_fifo:print_ln("Lane|# Heals|State|FIFO|State|# Heals|Lane")
        ftable_fifo:add_separator()
        ftable_fifo:set_cell_prop(fort.ANY_ROW, 1, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        for i = 1, #broadcast_lanes do
            local bcst_lane = broadcast_lanes[i]
            local dummy_lane = bcst_lane + choose(broadcast_lanes[i] % 2 == 0, 1, -1)
            local bcst_lane_id = crutil.get_lane_id(slice, bcst_lane)
            local bcst_lane_fifo = credo.port_get_param(slice, "retimer_fifo", bcst_lane)
            local dummy_lane_fifo = credo.port_get_param(slice, "retimer_fifo", dummy_lane)
            local bcst_lane_heal = credo.firmware_debug_cmd(slice, bcst_lane, 4, 7)
            local dummy_lane_heal = credo.firmware_debug_cmd(slice, dummy_lane, 4, 7)
            local bcst_state = credo.firmware_debug_cmd(slice, bcst_lane, 4, 0)
            local bcst_state_str = RETIMER_STATE_NAME[bcst_state] or '?'
            -- local line_state_str = RETIMER_STATE_NAME[line_state] or '?'

            ftable_fifo:print("%s(%2d) RX|%d|%s (%d)" %
                                  {bcst_lane_id, bcst_lane, bcst_lane_heal, bcst_state_str, bcst_state})
            ftable_fifo:print("-->[%d,%d]-->" % {bcst_lane_fifo[1], bcst_lane_fifo[2]})

            local mux_lane = mux_lanes[2 * i - 1]
            local mux_lane_id = crutil.get_lane_id(slice, mux_lane)
            local mux_lane_fifo = credo.port_get_param(slice, "retimer_fifo", mux_lane)
            local mux_lane_heal = credo.firmware_debug_cmd(slice, mux_lane, 4, 7)
            local mux_lane_is_active = (active_lane_map & (1 << mux_lane)) ~= 0
            local mux_state = credo.firmware_debug_cmd(slice, mux_lane, 4, 0)
            local mux_state_str = choose(mux_lane_is_active, RETIMER_STATE_NAME[mux_state] or '?', "STANDBY")

            ftable_fifo:print("||TX %s(%2d)" % {mux_lane_id, mux_lane})
            ftable_fifo:ln()
            ftable_fifo:print("TX||")
            ftable_fifo:print_ln("%s-[%d,%d]<-- |%s (%d)|%d|RX" %
                                     {
                    choose(mux_lane_is_active, "<-", " x"), mux_lane_fifo[1], mux_lane_fifo[2], mux_state_str,
                    mux_state, mux_lane_heal
                })

            mux_lane = mux_lanes[2 * i]
            mux_lane_id = crutil.get_lane_id(slice, mux_lane)
            mux_lane_fifo = credo.port_get_param(slice, "retimer_fifo", mux_lane)
            mux_lane_heal = credo.firmware_debug_cmd(slice, mux_lane, 4, 7)
            mux_lane_is_active = (active_lane_map & (1 << mux_lane)) ~= 0
            mux_state = credo.firmware_debug_cmd(slice, mux_lane, 4, 0)
            mux_state_str = choose(mux_lane_is_active, RETIMER_STATE_NAME[mux_state] or '?', "STANDBY")

            ftable_fifo:print("Dummy RX|%d|" % {dummy_lane_heal})
            ftable_fifo:print("-->[%d,%d]-->" % {dummy_lane_fifo[1], dummy_lane_fifo[2]})
            ftable_fifo:print("||TX %s(%2d)" % {mux_lane_id, mux_lane})
            ftable_fifo:ln()
            ftable_fifo:print("TX||")
            ftable_fifo:print_ln("%s-[%d,%d]<-- |%s (%d)|%d|RX" %
                                     {
                    choose(mux_lane_is_active, "<-", " x"), mux_lane_fifo[1], mux_lane_fifo[2], mux_state_str,
                    mux_state, mux_lane_heal
                })
            ftable_fifo:add_separator()
        end

        local bcst_main_state = credo.port_get_param(slice, 'retimer_host_state', port)
        local mux_main_state = credo.port_get_param(slice, 'retimer_line_state', port)

        if not is_aside_broadcast then
            -- swap lanes
            bcst_main_state, mux_main_state = mux_main_state, bcst_main_state
        end

        local bcst_main_state_str = RETIMER_STATE_NAME[bcst_main_state] or '?'
        local mux_main_state_str = RETIMER_STATE_NAME[mux_main_state] or '?'

        ftable:print("%d|%s (%d)" % {port, bcst_main_state_str, bcst_main_state})
        ftable:write(stringx.strip(tostring(ftable_fifo)))

        local mux_primed = credo.port_get_param(slice, "switchover_primed", port)
        local mux_primed_str = {}
        local mux0_group_name = choose(active_group == 0, "Active", "Standby")
        local mux1_group_name = choose(active_group == 1, "Active", "Standby")

        for i = 0, 1 do
            if active_group == i then
                mux_primed_str[i + 1] = choose(mux_primed[i + 1] ~= 0, "ACTIVE", "UNREADY")
            else
                mux_primed_str[i + 1] = choose(mux_primed[i + 1] ~= 0, "PRIMED", "UNREADY")
            end
        end

        ftable:print("%s (%d)|%s\n%s|%s\n%s" %
                         {
                mux_main_state_str, mux_main_state, mux0_group_name, mux_primed_str[1], mux1_group_name,
                mux_primed_str[2]
            })
        ftable:ln()

        -- local bitmux_host_state =
        ftable:add_separator()
        ::continue::
    end

    print(ftable)
end)

slash.register_chip_command({"switchover", "toggle"}, credo.FAMILY_SCREAMING_EAGLE, [[
Toggle switchover mux groups.

Arguments:

    <ports>    (portlist)
]], function(slice, argt)
    for port in iter(argt.ports) do
        local _, setup = credo.port_get_setup(slice, port)
        if setup == nil or setup.mode ~= credo.PORT_SWITCHOVER_RETIMER then
            goto continue
        end
        credo.port_set_option(slice, port, "switchover_switch", 0)
        ::continue::
    end
end)

slash.register_chip_command({"switchover", "select"}, credo.FAMILY_SCREAMING_EAGLE, [[
Select switchover mux groups.

Arguments:

    <ports>    (portlist)
    <value>    (mux0|mux1)
]], function(slice, argt)
    for port in iter(argt.ports) do
        local _, setup = credo.port_get_setup(slice, port)
        if setup == nil or setup.mode ~= credo.PORT_SWITCHOVER_RETIMER then
            goto continue
        end
        credo.port_set_option(slice, port, "switchover_select", choose(argt.value == "mux0", 0, 1))
        ::continue::
    end
end)

slash.register_chip_command({"switchover", "config"}, credo.FAMILY_SCREAMING_EAGLE, [[
Configure a switchover port.

Arguments:

    <port_id>       (port)
    <port_speed>    (speed)
    <host_lanes>    (lanelist)
    <line_lanes>    (lanelist)

    Options:
    --host-mlane    (optional lane)    Specify custom host main lane
    --line-mlane    (optional lane)    Specify custom line main lane
    --line-lt                          Enable line side LT
    --host-lt                          Enable host side LT

    -f,--flexspeed  (optional speedf)    Flexspeed configuration

    Other:
    -n,--dry-run       Print out the configuration that will be created
    -b,--build-only    Build the port, but do not start

Examples:

    > /switchover config 0 200g a0-a1 b0,b2,b1,b3
]], function(slice, argt)
    if argt.dry_run then
        print(argt)
        return
    end

    ---@type integer
    local port_id = argt.port_id
    local setup = credo.PortSetup()
    setup.mode = credo.PORT_SWITCHOVER_RETIMER
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
    if argt.line_lt then
        credo.port_set_option(slice, port_id, 'line_lt', 1)
    end
    if argt.flexspeed ~= nil then
        credo.port_set_option(slice, port_id, "flexspeed_kbps", int(argt.flexspeed / 1000))
    end

    if argt.build_only then
        return
    end

    credo.port_start(slice, port_id)
end)
