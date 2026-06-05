local credo = require "credo"
local slash = require "slash"

local opt_mode_map = {full = 0, bypass = 1, partial = 2}

slash.register_chip_command({"retimer", "config"}, credo.FAMILY_NUTCRACKER, [[
Configure a retimer port.

Arguments:

    <port_speed>    (speed)
    <host_lanes>    (lanelist)

    Options:
    --line-lt                                         Enable line side LT
    --host-opt      (optional full|bypass|partial)    Specify host side opti mode (default )

    Other:
    -n,--dry-run    Print out the configuration that will be created

]], function(slice, argt)

    assert(math.max(table.unpack(argt.host_lanes)) < 8, "Invalid host lane provided")

    for host_lane in iter(argt.host_lanes) do
        local config = credo.PortConfig()
        config.port_type = credo.PORT_RETIMER
        config.port_mode = credo.PMODE_SERDES
        config.host_no_of_lanes = 1
        config.line_no_of_lanes = 1
        config.host_start_lane = host_lane
        config.line_start_lane = host_lane + 8
        config.host_fec_type = credo.FEC_NONE
        config.line_fec_type = credo.FEC_NONE
        config.speed = argt.port_speed
        config.port_id = host_lane
        if argt.line_lt then
            config.flags = credo.PFLAG_LINE_SIDE_LT
        end
        if argt.dry_run then
            print(inspect(config))
        else
            if argt.host_opt ~= nil then
                credo.lane_set_option(slice, host_lane, "opti_mode", opt_mode_map[argt.host_opt])
            end
            credo.port_configure(slice, config, false)
        end
    end

end)
