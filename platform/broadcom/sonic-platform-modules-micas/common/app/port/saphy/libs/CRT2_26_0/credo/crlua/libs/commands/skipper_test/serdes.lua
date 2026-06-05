local slash = require "slash"
local fort = require "fort"
local crutil = require "crutil"

local serdes = require "commands.common.serdes"

slash.register_chip_command({"serdes", "param"}, credo.FAMILY_SKIPPER_TEST, [[
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

slash.register_chip_command({"serdes", "control"}, credo.FAMILY_SKIPPER_TEST, [[
Display serdes control table.

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_control")
end)

slash.register_chip_command({"serdes", "isi"}, credo.FAMILY_SKIPPER_TEST, [[
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
        local isi = credo.serdes_get_param(slice, "rx_isi", lane) ---@type integer[]
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

serdes.register_tx_taps(credo.FAMILY_SKIPPER_TEST, {"pre1", "main", "post1", "post2"})

slash.register_chip_command({"serdes", "tx_taps_nl"}, credo.FAMILY_SKIPPER_TEST, [[
Get/Set tx taps nonlinear

Arguments:

    <lanes>        (lanelist default all)        lanes to get set
    -f,--full      (optional intlist)            set all tx taps
    -p,--partial   (optional intmap)             set select tx taps
    -lv,--level    (optional intlist default 0,0,0,0)  set level, level_bar, level_3, level_bar_3

Examples:

    > /serdes tx_taps_nl                       # display all tx taps
    > /serdes tx_taps_nl 0-7                   # display tx taps for 0-7 lanes
    > /serdes tx_taps_nl a*                    # display tx taps for lanes on host side
    > /serdes tx_taps_nl -f 0,1,2,3,4,5,6,7    # set all tx taps for all lanes
    > /serdes tx_taps_nl -p 1=3,3=112          # set partial tx taps for all lanes {1=3,3=112}

]], function(slice, argt)
    local lanes = argt.lanes
    if argt.full ~= nil or argt.partial ~= nil then
        local partial = argt.partial
        if partial then
            local count = credo.serdes_get_tx_ffe_range(slice, lanes[1])
            local min, max = list.minmax(tablex.keys(partial))
            -- print("partial:", pairs(partial))
            assert(min >= 0, "Minimum partial key must be >= 0")
            assert(max < count, "Maximum partial must be < %d" % {count})
        end
        for lane in iter(lanes) do
            local val
            if argt.full then
                val = {table.unpack(argt.full)}
            else
                val = credo.serdes_get_tx_taps(slice, lane)
            end
            if partial then
                for index, tap in pairs(partial) do
                    val[index + 1] = tap
                end
            end
            for v in iter(argt.level) do
                table.insert(val, v)
            end
            print("lane %d set tx_taps_nl {%d,%d,%d,%d} {%d,%d,%d,%d}" % {lane, table.unpack(val)})
            credo.serdes_set_param(slice, 'tx_taps_nl', lane, val)
        end
        return
    end

    local function get_table_range(tbl, si, ei)
        local new_tbl = {}
        for i = si, ei do
            table.insert(new_tbl, tbl[i])
        end
        return new_tbl
    end

    local skpreg = require"regmap".slice(slice)
    local ftable = fort.create_table()
    fort.write_ln(ftable, "", "auto", "Taps", "", "", "Taps_bar", "", "", "Taps_3", "", "", "Taps_bar_3", "", "")
    fort.write_ln(ftable, "Lane", "select", "-1 M +1 +2", "Scale", "Sum", "-1 M +1 +2", "Scale", "Sum", "-1 M +1 +2",
                  "Scale", "Sum", "-1 M +1 +2", "Scale", "Sum")
    fort.add_separator(ftable)
    for lane in iter(argt.lanes) do
        local taps = credo.serdes_get_param(slice, 'tx_taps_nl', lane)
        fort.printf(ftable, "%s(%d)", crutil.get_lane_id(slice, lane), lane)
        local auto_sel = ""
        auto_sel = auto_sel .. skpreg.pam100.reg_tx_coef_4_auto_sel:read(slice, lane)
        auto_sel = auto_sel .. skpreg.pam100.reg_tx_coef_3_auto_sel:read(slice, lane)
        auto_sel = auto_sel .. skpreg.pam100.reg_tx_coef_2_auto_sel:read(slice, lane)
        auto_sel = auto_sel .. skpreg.pam100.reg_tx_coef_1_auto_sel:read(slice, lane)
        fort.printf(ftable, "%s" % {auto_sel})
        local pr1, m, po1, po2, scale, sum = table.unpack(get_table_range(taps, 1, 6))
        fort.printf(ftable, "%d  %d  %d  %d" % {pr1, m, po1, po2})
        fort.printf(ftable, "%d|%d" % {scale, sum})
        pr1, m, po1, po2, scale, sum = table.unpack(get_table_range(taps, 7, 12))
        fort.printf(ftable, "%d  %d  %d  %d" % {pr1, m, po1, po2})
        fort.printf(ftable, "%d|%d" % {scale, sum})
        pr1, m, po1, po2, scale, sum = table.unpack(get_table_range(taps, 13, 18))
        fort.printf(ftable, "%d  %d  %d  %d" % {pr1, m, po1, po2})
        fort.printf(ftable, "%d|%d" % {scale, sum})
        pr1, m, po1, po2, scale, sum = table.unpack(get_table_range(taps, 19, 24))
        fort.printf(ftable, "%d  %d  %d  %d" % {pr1, m, po1, po2})
        fort.printf(ftable, "%d|%d" % {scale, sum})
        fort.ln(ftable)
    end
    print(ftable)
end)

slash.register_chip_command({"serdes", "tx_6taps"}, credo.FAMILY_SKIPPER_TEST, [[
Get/Set tx 6-taps

Arguments:

    <lanes>        (lanelist default all)        lanes to get set
    -f,--full      (optional intlist)            set all tx taps
    -p,--partial   (optional intmap)             set select tx taps
    -s,--scale     (optional integer default 1)  set full scale

Examples:

    > /serdes tx_6taps                       # display all tx taps
    > /serdes tx_6taps 0-7                   # display tx taps for 0-7 lanes
    > /serdes tx_6taps a*                    # display tx taps for lanes on host side
    > /serdes tx_6taps -f 0,1,2,3,4,5        # set all tx taps for all lanes
    > /serdes tx_6taps -p 1=3,3=112          # set partial tx taps for all lanes {1=3,3=112}

]], function(slice, argt)
    local lanes = argt.lanes

    local function get_table_range(tbl, si, ei)
        local new_tbl = {}
        for i = si, ei do
            table.insert(new_tbl, tbl[i])
        end
        return new_tbl
    end

    if argt.full ~= nil or argt.partial ~= nil then
        local partial = argt.partial
        if partial then
            local count = 6
            local min, max = list.minmax(tablex.keys(partial))
            -- print("partial:", pairs(partial))
            assert(min >= 0, "Minimum partial key must be >= 0")
            assert(max < count, "Maximum partial must be < %d" % {count})
        end
        for lane in iter(lanes) do
            local val
            if argt.full then
                val = {table.unpack(argt.full)}
            else
                val = get_table_range(credo.serdes_get_param(slice, 'tx_6taps', lane), 1, 6)
            end
            if partial then
                for index, tap in pairs(partial) do
                    val[index + 1] = tap
                end
            end
            local scale_str = choose(argt.scale == 1, "full", "half")
            print("lane %d set tx_6taps %s-scale {%d,%d,%d,%d,%d,%d}" % {lane, scale_str, table.unpack(val)})
            table.insert(val, argt.scale)
            credo.serdes_set_param(slice, 'tx_6taps', lane, val)
        end
        return
    end

    local skpreg = require"regmap".slice(slice)
    local ftable = fort.create_table()
    fort.write_ln(ftable, "", "auto", "6-Taps NRZ", "")
    fort.write_ln(ftable, "Lane", "select", "-1 M +1 +2 +3 +4", "Scale")
    fort.add_separator(ftable)
    for lane in iter(argt.lanes) do
        fort.printf(ftable, "%s(%d)", crutil.get_lane_id(slice, lane), lane)
        local auto_sel = ""
        auto_sel = auto_sel .. skpreg.nrz50.reg_tx_coef_4_auto_sel:read(slice, lane)
        auto_sel = auto_sel .. skpreg.nrz50.reg_tx_coef_3_auto_sel:read(slice, lane)
        auto_sel = auto_sel .. skpreg.nrz50.reg_tx_coef_2_auto_sel:read(slice, lane)
        auto_sel = auto_sel .. skpreg.nrz50.reg_tx_coef_1_auto_sel:read(slice, lane)
        fort.printf(ftable, "%s" % {auto_sel})
        local pr1, m, po1, po2, po3, po4, scale = table.unpack(credo.serdes_get_param(slice, 'tx_6taps', lane))
        fort.printf(ftable, "%d  %d  %d  %d  %d  %d" % {pr1, m, po1, po2, po3, po4})
        local scale_str = choose(scale == 1, "full", "half")
        fort.printf(ftable, "%s-scale" % {scale_str})
        fort.ln(ftable)
    end
    print(ftable)
end)
