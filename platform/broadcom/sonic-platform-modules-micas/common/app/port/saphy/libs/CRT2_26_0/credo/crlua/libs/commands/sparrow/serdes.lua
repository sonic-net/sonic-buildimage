local slash = require "slash"
local credo = require "credo"

slash.register_chip_command({"serdes", "param"}, credo.FAMILY_SPARROW, [[
Display serdes paramter table.

Arguments:

    <lanes>      (lanelist default all)

Example:

    > serdes param           # dispaly all lanes
    > serdes param 0-2       # select lanes

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_param %s" % {list.join(argt.lanes, ",")})
end)

slash.register_chip_command({"serdes", "control"}, credo.FAMILY_SPARROW, [[
Display serdes control table.

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_control")
end)

slash.register_chip_command({"serdes", "diag"}, credo.FAMILY_SPARROW, [[
Display serdes diagnostic table.

    <lanes>    (lanelist)

]], function(slice, argt)
    for _, lane in ipairs(argt.lanes) do
        if lane <= 2 then
            credo.display_info_print(slice, "serdes_diag %d" % {lane})
        end
    end
end)

slash.register_chip_command({"serdes", "ffe"}, credo.FAMILY_SPARROW, [[
Display serdes diagnostic table.

    <lanes>    (lanelist)

]], function(slice, argt)
    for _, lane in ipairs(argt.lanes) do
        print("Lane %d" % {lane})
        credo.display_info_print(slice, "ffe_info %d" % {lane})
    end
end)

slash.register_chip_command({"serdes", "isi"}, credo.FAMILY_SPARROW, [[
Display serdes isi table.

Arguments:

    <lanes>      (lanelist default all)
]], function(slice, argt)
    credo.display_info_print(slice, "isi %s" % {list.join(argt.lanes, ",")})
end)
