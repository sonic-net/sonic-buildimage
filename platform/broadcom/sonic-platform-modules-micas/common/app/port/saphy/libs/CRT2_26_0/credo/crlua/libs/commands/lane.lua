local slash = require "slash"
local credo = require "credo"
local fort = require "fort"

local crutil = require "crutil"

slash.register_command({"lane", "mode", "config"}, [[
Configure a lane into a pure phy mode.

Arguments:


    <lane_list>      (lanelist)    Lanes to configure
    <mode>           (lanemode)    Lane mode to configure nrz|pam4
    <speed>          (speed)       Speed of the lane
    -l,--loopback                  Enable shallow rx_to_tx loopback
    -f,--force                     Destroy lane if configured

Examples:

    > lane mode config all pam4 50g       # configure all lanes to pam4 50g
    > lane mode config all pam4 50g -l    # above + loopback enabled
    > lane mode config 0-8 nrz 25g        # configure lanes 0-8 to nrz 25g

]], function(slice, argt)
    ---@type credo.LaneMode
    local mode = argt.mode
    ---@type number
    local speed = argt.speed
    ---@type integer[]
    local lane_list = argt.lane_list
    for lane in iter(lane_list) do
        local lmode = credo.lane_get_mode(slice, lane)
        if not list.contains({credo.LMODE_OFF, credo.LMODE_DISABLE}, lmode) then
            assert(argt.force, "Lane %d is already configured in %s" % {lane, lmode.display_name})
            credo.lane_destroy_mode(slice, lane)
        end
    end

    for lane in iter(lane_list) do
        local lmode = credo.lane_get_mode(slice, lane)
        if lmode == credo.LMODE_DISABLE then
            goto end_loop
        end
        if argt.loopback then
            credo.lane_configure_mode_loopback(slice, lane, int(speed))
        else
            credo.lane_configure_mode(slice, lane, mode, int(speed))
        end
        ::end_loop::
    end

end)

slash.register_command({"lane", "mode", "destroy"}, [[
Destroy a lane mode configuration

Arguments:

    <lane_list>    (lanelist)

]], function(slice, argt)
    ---@type integer[]
    local lane_list = argt.lane_list
    for lane in iter(lane_list) do
        credo.lane_destroy_mode(slice, lane)
    end
end)

slash.register_command({"lane", "mode", "info"}, [[
Get information about lanes mode configured.

Arguments:

    <lane_list>    (lanelist default all)

]], function(slice, argt)
    ---@type integer[]
    local lane_list = argt.lane_list
    local ftable = fort.create_table()

    fort.printf_ln(ftable, "Lane|Mode|Speed|LB Mode")

    for lane in iter(lane_list) do
        local mode = credo.lane_get_mode(slice, lane)
        local speed_str = "-"
        local lbmode_str = "-"
        if crutil.is_lane_configured(mode) then
            local speed = credo.lane_get_speed(slice, lane)
            speed_str = "%.3fG" % {speed / 1000000}
            lbmode_str = credo.lane_get_loopback_mode(slice, lane).display_name
        end

        fort.printf_ln(ftable, "%s(%d)|%s|%s|%s", crutil.get_lane_id(slice, lane), lane, mode.display_name, speed_str,
                       lbmode_str)
    end
    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_CENTER)
    print(fort.to_string(ftable))
end)

slash.register_command({"lane", "status"}, [[
Get information about lanes state.

Arguments:

    <lane_list>    (lanelist default all)

]], function(slice, argt)
    ---@type integer[]
    local lane_list = argt.lane_list
    local ftable = fort.create_table()

    fort.printf_ln(ftable, "Lane|Mode|Speed|LB Mode|Tx State| Rx State\nSD RDY ADP")

    for lane in iter(lane_list) do
        local mode = credo.lane_get_mode(slice, lane)
        local speed_str = "-"
        local lbmode_str = "-"
        local tx_state_str = "-"
        local rx_state_str = "-"
        if crutil.is_lane_configured(mode) then
            local speed = credo.lane_get_speed(slice, lane)
            speed_str = "%.3fG" % {speed / 1000000}
            lbmode_str = credo.lane_get_loopback_mode(slice, lane).display_name
            tx_state_str = credo.lane_tx_get_status(slice, lane).display_name
            local sd = credo.serdes_get_rx_signal_detect(slice, lane)
            local rdy = credo.serdes_get_rx_ready(slice, lane)
            local adp = credo.serdes_get_phy_ready(slice, lane)
            rx_state_str = "%d  %d  %d" % {choose(sd, 1, 0), choose(rdy, 1, 0), choose(adp, 1, 0)}
        end

        fort.printf_ln(ftable, "%s(%d)|%s|%s|%s|%s|%s", crutil.get_lane_id(slice, lane), lane, mode.display_name,
                       speed_str, lbmode_str, tx_state_str, rx_state_str)
    end
    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_CENTER)
    print(fort.to_string(ftable))
end)

slash.register_command({"lane", "loopback"}, [[
Get/Set loopback mode for lanes

Arguments:

    -v, --value    (optional lbmode)
    <lane_list>    (lanelist default all)

Examples:

    > lane loopback                    # get loopback mode all lanes
    > lane loopback 0-8                # get loopback mode certain lanes
    > lane loopback 0-8 -v rx_to_tx    # set loopback mode

]], function(slice, argt)
    ---@type credo.LaneLoopbackMode|nil
    local value = argt.value
    ---@type integer[]
    local lane_list = argt.lane_list

    if value == nil then
        local ftable = fort.create_table()
        fort.printf_ln(ftable, "Lane|LB Mode")
        fort.add_separator(ftable)
        for lane in iter(lane_list) do
            local lbmode = credo.lane_get_loopback_mode(slice, lane)
            fort.printf_ln(ftable, "%s(%d)|%s", crutil.get_lane_id(slice, lane), lane, lbmode.display_name)
        end
        print(fort.to_string(ftable))
    else
        for lane in iter(lane_list) do
            credo.lane_set_loopback_mode(slice, lane, value)
        end
    end
end)

slash.register_command({"lane", "option"}, [[
Get/Set lane options.

*If no arguments given, it displays a table of all options.

Arguments:

    <lanes>             (lanelist default all)      Lanes to display
    <options...>        (optional string)           Option(s) to set/get
    -v,--value          (optional integer)          Value to set option(s)
    -d,--description                                Display descriptions only

Examples:

    > lane option                              # display all lane options for all lanes
    > lane option 0-7                          # display certain lanes
    > lane option 0-7 sd_delay optical_mode    # select certain options
    > lane option 0-7 sd_delay -v 200          # set option

]], function(slice, argt)
    ---@type string[]
    local options = argt.options
    ---@type integer|nil
    local value = argt.value
    ---@type boolean
    local description_only = argt.description or false
    ---@type integer[]
    local lanes = argt.lanes

    local function print_option_table(option_list)
        local ftable = fort.create_table()

        fort.printf(ftable, "Option")

        if not description_only then
            ---@type integer

            for lane in iter(lanes) do
                fort.printf(ftable, "Ln %s(%d)", crutil.get_lane_id(slice, lane), lane)
            end
        else
            fort.printf(ftable, "Description")
        end

        fort.ln(ftable)
        fort.add_separator(ftable)
        ---@type credo.SliceOption
        for option_item in iter(option_list) do
            -- format description text so it is not too wide
            local description = stringx.join("\n", stringx.wrap(option_item.description, 60))
            if description_only then
                fort.printf_ln(ftable, "%s|%s", option_item.name, description)
                fort.add_separator(ftable)
            else
                fort.printf(ftable, "%s", option_item.name)
                for lane in iter(lanes) do
                    local lmode = credo.lane_get_mode(slice, lane)
                    if lmode ~= credo.LMODE_DISABLE then
                        local ok, val = pcall(function()
                            return credo.lane_get_option(slice, lane, option_item.name)
                        end)
                        if not ok then
                            fort.printf(ftable, "-")
                        else
                            fort.printf(ftable, "%d 0x%X", val, val)
                        end
                    else
                        fort.printf(ftable, "-")
                    end

                end
                fort.ln(ftable)
            end
        end
        print(fort.to_string(ftable))
    end

    if #options == 0 then
        -- dont allow setting all options at once
        if value ~= nil then
            error("Cannot set all slice options at once")
        end
        local option_list = credo.lane_get_option_list(slice)
        print_option_table(option_list)
    elseif value == nil then
        local option_list = credo.lane_get_option_list(slice)
        for option in iter(options) do
            if not credo.lane_is_option_supported(slice, option) then
                print("WARN: Invalid option %s" % {option})
            end
        end
        option_list = list.filter(option_list, function(option_item)
            return list.index(options, option_item.name) ~= nil
        end)
        print_option_table(option_list)
    else
        for option in iter(options) do
            if not credo.lane_is_option_supported(slice, option) then
                -- more strict if user is trying to set an invalid option
                error("Invalid option %s" % {option})
            end
        end

        for option in iter(options) do
            for lane in iter(lanes) do
                local lmode = credo.lane_get_mode(slice, lane)
                if lmode ~= credo.LMODE_DISABLE then
                    credo.lane_set_option(slice, lane, option, value)
                end
            end
        end
    end
end)

slash.register_command({"lane", "rx_reset"}, [[
Perform a rx reset on specified lanes.

This will issue a SerDes firmware readaptation.

Arguments:

    <lane_list>    (lanelist)

]], function(slice, argt)
    ---@type integer[]
    local lane_list = argt.lane_list
    for lane in iter(lane_list) do
        local lmode = credo.lane_get_mode(slice, lane)
        if crutil.is_lane_configured(lmode) then
            credo.lane_rx_reset(slice, lane)
        end
    end
end)

slash.register_command({"lane", "rx_disable"}, [[
Enable/Disable rx on specified lanes.

Arguments:

    <lane_list>    (lanelist)
    <disable>      (force|unforce)

]], function(slice, argt)
    ---@type integer[]
    local lane_list = argt.lane_list
    for lane in iter(lane_list) do
        local lmode = credo.lane_get_mode(slice, lane)
        if crutil.is_lane_configured(lmode) then
            if argt.disable == "force" then
                credo.lane_rx_disable(slice, lane)
            else
                credo.lane_rx_no_disable(slice, lane)
            end
        end
    end
end)

slash.register_command({"lane", "tx_squelch"}, [[
Control Forcing Transmitter Squelch.

State will be lost if you are in a mode other than normal TRAFFIC.

Arguments:

    <lanes>         (lanelist)
    <squelch>       (force|unforce)

]], function(slice, argt)

    if argt.squelch ~= nil then
        ---@type integer
        for lane in iter(argt.lanes) do
            local lmode = credo.lane_get_mode(slice, lane)
            if crutil.is_lane_configured(lmode) then
                credo.lane_tx_force_squelch(slice, lane, argt.squelch == "force")
            end
        end
    end

end)
