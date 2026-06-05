local slash = require "slash"
local credo = require "credo"
local serdes = require "commands.common.serdes"
local regmap = require "regmap"
local fort = require "fort"
local crutil = require "crutil"

local itt = require "itertools"

slash.register_chip_command({"serdes", "param"}, credo.FAMILY_BLACKHAWK, [[
Display serdes paramter table.

Arguments:

    -s,--split
    <lanes>      (lanelist default all)

Example:

    > serdes param           # dispaly all lanes
    > serdes param 0-2       # select lanes
    > serdes param 0-2 -s    # split

]], function(slice, argt)
    local command = "serdes_param "
    if argt.split then
        command = command .. " S"
    end
    credo.display_info_print(slice, "%s%s" % {command, list.join(argt.lanes, ",")})
end)

slash.register_chip_command({"serdes", "control"}, credo.FAMILY_BLACKHAWK, [[
Display serdes control table.

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_control")
end)

slash.register_chip_command({"serdes", "isi"}, credo.FAMILY_BLACKHAWK, [[
Display isi table.

Arguments:

    <lanes> (lanelist default all)

]], function(slice, argt)
    local command = "isi "
    command = command .. list.join(argt.lanes, ",")
    credo.display_info_print(slice, command)
end)

slash.register_chip_command({"serdes", "ffe"}, credo.FAMILY_BLACKHAWK, [[
Display ffe table.

Arguments:

    <lane>    (lane)

]], function(slice, argt)
    credo.display_info_print(slice, "ffe_info %d" % {argt.lane})
end)

slash.register_chip_command({"serdes", "debug"}, credo.FAMILY_BLACKHAWK, [[
Display serdes debug information.

Arguments:

    <lane>    (lane)

]], function(slice, argt)
    local mode = credo.lane_get_mode(slice, argt.lane)
    if mode == credo.LMODE_PAM4 then
        credo.display_info_print(slice, "pam4_debug %d" % {argt.lane})
    elseif mode == credo.LMODE_NRZ then
        credo.display_info_print(slice, "nrz_debug %d" % {argt.lane})
    else
        print("No debug information for mode %s" % {mode.display_name})
    end
end)

slash.register_chip_command({"serdes", "pll_info"}, credo.FAMILY_BLACKHAWK, [[
Display pll information.

Arguments:

    <lane>    (lane)

]], function(slice, argt)
    credo.display_info_print(slice, "nrz_debug %d" % {argt.lane})
end)

serdes.register_tx_taps(credo.FAMILY_BLACKHAWK, {"pre3", "pre2", "pre1", "main", "post1", "post2", "post3"})

slash.register_chip_command({"serdes", "agccal"}, credo.FAMILY_BLACKHAWK, [[
Check agccal information

    <lanes>    (lanelist)   lanes to check agccal

]], function(slice, argt)
    local bhreg = regmap.slice(slice)

    ---@type integer[]
    local lanes = argt.lanes

    local ftable = fort.create()
    ftable:print("||")
    for lane in iter(lanes) do
        ftable:print("Ln %s(%d)" % {crutil.get_lane_id(slice, lane), lane})
    end
    ftable:ln()

    ftable:print("Dir|Phase|Addr") -- add separators
    for lane in iter(lanes) do
        ftable:print(" P  N   D")
    end
    ftable:ln()
    ftable:add_separator()

    ---@param dir integer
    ---@param phase integer
    ---@param addr integer
    for dir, phase, addr in itt.product(itt.values(range(0, 1)), itt.values(range(0, 1)), itt.values(range(1, 15))) do
        ftable:print("%d|%d|%d" % {dir, phase, addr})
        for lane in iter(lanes) do
            bhreg.pll.adr_comp:write_bits(slice, dir, lane, 6)
            bhreg.pll.adr_comp:write_bits(slice, phase, lane, 1, 0)
            bhreg.pll.adr_comp:write_bits(slice, addr, lane, 5, 2)

            local calp = bhreg.pll.calp_caln:read_bits(slice, lane, 11, 6)
            local caln = bhreg.pll.calp_caln:read_bits(slice, lane, 5, 0)
            ftable:print("%2d %2d %3d" % {calp, caln, calp - caln})
        end
        ftable:ln()
    end
    ftable:set_cell_span(1, 1, 3)
    ftable:set_cell_prop(1, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    ftable:set_cell_prop(fort.ANY_ROW, 1, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    ftable:set_cell_prop(fort.ANY_ROW, 2, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    ftable:set_cell_prop(fort.ANY_ROW, 3, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)

    print(ftable)

end)
