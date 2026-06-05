local credo = require "credo"
local slash = require "slash"

slash.register_chip_command({"gearbox", "status"}, credo.FAMILY_SPARROW, [[
Display Gearbox status.
]], function(slice, argt)
    credo.display_info_print(slice, "gearbox_status")
end)

slash.register_chip_command({"gearbox", "config"}, credo.FAMILY_SPARROW, [[
Configure a Gearbox port

Arguments:

    -i,--id            (optional integer)           Set custom port id
    -e,--fec           (fec default none)       Port line side fec type
    -f,--force                                  Force configure even if lanes are used in another port
    -n,--dry-run                                Do not configure the port, print out instead

Examples:

    > gearbox config 100g m2-4 0,4                  # 2-100g gearbox on mapping m1
    > gearbox config 50g m1-2 0:6:2                 # 4-50g gearbox on mapping m0
    > gearbox config 50g m1-2 0:6:2 -n              # dry run of above
    > gearbox config 50g m1-2 0:6:2 -g sopt|lopt    # add optical flags

]], function(slice, argt)
    ---@type boolean
    local force = argt.force
    local port_config = credo.PortConfig()
    port_config.port_id = argt.id or credo.PORT_AUTO_ASSIGN_ID
    port_config.port_mode = credo.PMODE_SERDES
    port_config.port_type = credo.PORT_GEARBOX
    port_config.speed = 106000
    port_config.host_start_lane = 0
    port_config.host_no_of_lanes = 4
    port_config.host_fec_type = credo.FEC_RS_544
    port_config.line_start_lane = 5
    port_config.line_no_of_lanes = 1
    port_config.line_fec_type = argt.fec
    port_config.flags = 0x0
    if argt.dry_run then
        print(port_config)
    else
        credo.port_configure(slice, port_config, force)
    end
end)
