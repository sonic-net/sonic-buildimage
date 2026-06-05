local slash = require "slash"
local credo = require "credo"
local crutil = require "crutil"
local fort = require "fort"

local lb_fifo_bitmap = {{15, 13}, {12, 10}, {9, 7}, {6, 4}}

slash.register_chip_command({"lane", "loopback", "status"}, credo.FAMILY_BLACKHAWK, [[
Get the loopback fifo status.

Arguments:

    <lanes>    (lanelist default all)    lanes to get fifo status

    ]], function(slice, argt)

    local ftable = fort.create_table()
    fort.printf_ln(ftable, "Lane|Mode|LB Mode|LB Status")
    ---@type integer

    for lane in iter(argt.lanes) do
        local mode = credo.lane_get_mode(slice, lane)

        if not crutil.is_lane_configured(mode) then
            fort.printf_ln(ftable, "%d|-", lane)
            goto end_loop
        end

        local lb_mode = credo.lane_get_loopback_mode(slice, lane)
        if lb_mode == credo.LB_DISABLED then
            fort.printf_ln(ftable, "%d|%s|%s", lane, mode.display_name, lb_mode.display_name)
            goto end_loop
        end
        local loopback_status_a, loopback_status_b = true, true
        local fifo_addr = 0x9814 + (lane % 8) * 8
        local fifo1 = gray_bin(credo.slice_read(slice, fifo_addr, lb_fifo_bitmap[1][1], lb_fifo_bitmap[1][2]))
        local fifo2 = gray_bin(credo.slice_read(slice, fifo_addr, lb_fifo_bitmap[2][1], lb_fifo_bitmap[2][2]))
        local fifo3 = gray_bin(credo.slice_read(slice, fifo_addr, lb_fifo_bitmap[3][1], lb_fifo_bitmap[3][2]))
        local fifo4 = gray_bin(credo.slice_read(slice, fifo_addr, lb_fifo_bitmap[4][1], lb_fifo_bitmap[4][2]))

        if fifo1 < 4 or fifo2 < 4 or fifo1 > 6 or fifo2 > 6 then
            loopback_status_a = false
        end
        if fifo3 < 4 or fifo4 < 4 or fifo3 > 6 or fifo4 > 6 then
            loopback_status_b = false
        end

        fort.printf(ftable, "%d|%s|%s", lane, mode.display_name, lb_mode.display_name)

        if lb_mode == credo.LB_RX_TO_TX then
            local status_symbol = choose(loopback_status_a, " ", "*")
            fort.printf_ln(ftable, "RX%s[%d] -> [%d]%sTX", status_symbol, fifo1, fifo2, status_symbol)
        else
            local status_symbol = choose(loopback_status_b, " ", "*")
            fort.printf_ln(ftable, "TX%s[%d] -> [%d]%sRX", status_symbol, fifo4, fifo3, status_symbol)
        end

        ::end_loop::
    end

    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)

    print(fort.to_string(ftable))
end)

slash.register_chip_command({"lane", "recovery"}, credo.FAMILY_BLACKHAWK, [[
Get/Set the fast recovery information for a lane.

Arguments:

    <lanes>         (lanelist default all)    lanes to get fast recovery status
    -t,--timeout    (optional time)           set fast recover timeout (0 means disable)
]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes
    if argt.timeout ~= nil then
        for lane in iter(lanes) do
            credo.lane_set_option(slice, lane, "fast_recover_timeout", int(argt.timeout * 1000))
        end
        return
    end

    local ftable = fort.create()
    ftable:print_ln("Lane|Mode|Timeout|Armed|Count")
    for lane in iter(argt.lanes) do
        local lmode = credo.lane_get_mode(slice, lane)
        local lmode_name = lmode.display_name
        ftable:print("%s(%02d)|%s" % {crutil.get_lane_id(slice, lane), lane, lmode_name:lower()})
        local timeout = credo.lane_get_option(slice, lane, "fast_recover_timeout")

        if timeout == 0 or not crutil.is_lane_configured(lmode) then
            ftable:print_ln("off|-|-")
        elseif lmode == credo.LMODE_PAM4 then
            ftable:print("%d ms" % {timeout})
            local armed = credo.firmware_debug_cmd(slice, lane, 2, 27)
            local count = credo.firmware_debug_cmd(slice, lane, 2, 23)
            ftable:print_ln("%s|%d" % {choose(armed ~= 0, "yes", "no"), count})
        else
            ftable:print("%d ms" % {timeout})
            local armed = credo.firmware_debug_cmd(slice, lane, 1, 32)
            local count = credo.firmware_debug_cmd(slice, lane, 1, 23)
            ftable:print_ln("%s|%d" % {choose(armed ~= 0, "yes", "no"), count})
        end
    end

    fort.set_cell_prop(ftable, 1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)

    print(fort.to_string(ftable))
end)
