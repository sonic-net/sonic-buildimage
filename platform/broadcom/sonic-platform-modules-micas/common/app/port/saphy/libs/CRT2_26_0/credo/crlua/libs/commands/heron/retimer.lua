local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"retimer", "config"}, credo.FAMILY_HERON, [[
Configure a retimer port.

Arguments:

    <port_id>       (port)
    <port_speed>    (speed)
    <host_lanes>    (lanelist)
    <line_lanes>    (lanelist)

    Options:
    --host-lt                     Enable host side LT
    --line-anlt                   Enable line side ANLT
    --line-an                     Enable line side AN
    --line-lt                     Enable line side LT
    --an-override                 Enable AN override

    Other:
    -n,--dry-run
    -b,--build-only

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
    setup.mode = credo.PORT_RETIMER
    setup.host_lanes = argt.host_lanes
    setup.line_lanes = argt.line_lanes
    setup.speed = argt.port_speed

    credo.port_build(slice, port_id, setup)

    if argt.host_lt then
        credo.port_set_option(slice, port_id, "host_lt", 1)
    end
    if argt.line_anlt then
        credo.port_set_option(slice, port_id, "line_anlt", 1)
    end
    if argt.line_lt then
        credo.port_set_option(slice, port_id, "line_lt", 1)
    end
    if argt.line_an then
        credo.port_set_option(slice, port_id, "line_an", 1)
    end

    if argt.build_only then
        return
    end

    credo.port_start(slice, port_id)
end)
