local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"mac", "stats"}, credo.FAMILY_OSPREY, [[
Display mac statistics for the slice.
]], function(slice, argt)
    credo.display_info_print(slice, "mac_statistics")
end)

slash.register_chip_command({"mac", "stats", "clear"}, credo.FAMILY_OSPREY, [[
Clear mac statistics for the slice.

Arguments:

    <ports>      (portlist)
    -s,--side    (host|line|both default both)
]], function(slice, argt)

    for port in iter(argt.ports) do
        local port_config = credo.port_query(slice, port)
        if port_config.port_id == credo.PORT_UNCONFIGURED or port_config.port_mode == credo.PMODE_SERDES then
            print("Skipping port %d as it has no mac" % {port})
            goto continue
        end
        if list.contains({"both", "host"}, argt.side) then
            credo.mac_stats_clear_counters(slice, port, credo.SIDE_HOST)
        end
        if list.contains({"both", "line"}, argt.side) then
            credo.mac_stats_clear_counters(slice, port, credo.SIDE_LINE)
        end
        ::continue::
    end
end)
