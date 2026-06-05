local fort = require "fort"
local crutil = require "crutil"
local slash = require "slash"
local serdes = require "commands.common.serdes"

slash.register_chip_command({"serdes", "param"}, credo.FAMILY_VICEROY, [[
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

slash.register_chip_command({"serdes", "control"}, credo.FAMILY_VICEROY, [[
Display serdes control table.

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_control")
end)

slash.register_chip_command({"serdes", "isi"}, credo.FAMILY_VICEROY, [[
Display serdes isi table.

Arguments:

    <lanes>    (lanelist default all)    lanes to display isi
]], function(slice, argt)
    local ftable = fort.create_table()

    fort.printf_ln(ftable, "isi|nrz|fm1|f4|f5|f6|f7|f8|f9|f10|f11|f12|f13|f14|f15|f16|f17|f18")
    fort.printf_ln(ftable, "lane|pam4|fm3|fm2|fm1|f2|f3|f4|f5|f6|f7|f8|f9|f10|f11|f12|f13|f14")

    ---@type integer
    for lane in iter(argt.lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        if not crutil.is_lane_configured(mode) then
            fort.printf_ln(ftable, "%d|-", lane)
            goto end_loop
        end
        local isi = credo.serdes_get_param(slice, "rx_isi", lane)
        fort.print(ftable, "%2s(%02d)" % {crutil.get_lane_id(slice, lane), lane})
        fort.printf(ftable, "%s", choose(mode == credo.LMODE_NRZ, "nrz", "pam4"))
        for isi_val in iter(isi) do
            fort.printf(ftable, "%d", isi_val)
        end
        fort.ln(ftable)
        ::end_loop::
    end

    fort.set_cell_prop(ftable, 2, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
    print(fort.to_string(ftable))
end)

serdes.register_tx_taps(credo.FAMILY_VICEROY, {"pre3", "pre2", "pre1", "main", "post1", "post2", "post3"})
