local fort = require "fort"
local slash = require "slash"
local crutil = require "crutil"

local serdes = {}

---@param family credo.Family
---@param header string[]
function serdes.register_tx_taps(family, header)

    slash.register_chip_command({"serdes", "tx_taps"}, family, [[
Get/Set tx taps

Arguments:

    <lanes>        (lanelist default all)    lanes to get set
    -r,--reset                               reset the taps to default settings
    -f,--full      (optional intlist)        set all tx taps
    -p,--partial   (optional intmap)         set select tx taps

Examples:

    > /serdes tx_taps                       # display all tx taps
    > /serdes tx_taps 0-7                   # display tx taps for 0-7 lanes
    > /serdes tx_taps -r                    # reset all tx taps
    > /serdes tx_taps -f 0,1,2,3,4,5,6,7    # set all tx taps for all lanes
    > /serdes tx_taps -p 1=3,3=112          # set partial tx taps for all lanes {1=3,3=112}

]], function(slice, argt)

        ---@type integer[]
        local lanes = argt.lanes
        if argt.reset then
            for lane in iter(lanes) do
                credo.serdes_reset_tx_taps(slice, lane)
            end
            return
        end
        if argt.full ~= nil then
            ---@type integer[]
            local full = argt.full
            for lane in iter(lanes) do
                credo.serdes_set_tx_taps(slice, lane, full)
            end
            return
        end
        if argt.partial ~= nil then

            ---@type table<integer, integer>
            local partial = argt.partial

            local count = credo.serdes_get_tx_ffe_range(slice, lanes[1])

            local min, max = list.minmax(tablex.keys(partial))
            print(partial)
            assert(min >= 0, "Minimum partial key must be >= 0")
            assert(max < count, "Maximum partial must be < %d" % {count})

            for lane in iter(lanes) do
                local new_taps = credo.serdes_get_tx_taps(slice, lane)
                for index, tap in pairs(partial) do
                    new_taps[index + 1] = tap
                end
                credo.serdes_set_tx_taps(slice, lane, new_taps)
            end
            return
        end

        local ftable = fort.create_table()
        fort.write_ln(ftable, "Lane", "Taps")
        fort.write_ln(ftable, "", table.unpack(header))
        fort.add_separator(ftable)
        ---@type integer
        for lane in iter(lanes) do
            fort.printf(ftable, "%s(%d)", crutil.get_lane_id(slice, lane), lane)
            local taps = credo.serdes_get_tx_taps(slice, lane)
            fort.write_ln(ftable, table.unpack(taps))
        end
        fort.set_cell_prop(ftable, fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        fort.set_cell_prop(ftable, fort.ANY_ROW, 1, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_LEFT)
        fort.set_cell_prop(ftable, fort.ANY_ROW, 2, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_LEFT)
        fort.set_cell_span(ftable, 1, 2, #header)

        print(ftable)

    end)

    slash.register_chip_command({"serdes", "tx_taps", "preset"}, family, [[
Set tx taps given channel description to preset tx tap values.

Arguments:

    <lanes>         (lanelist)             lanes to get set
    <mode>          (optional lanemode)    lane mode of lanes (default to using the first lanes mode)
    <speed>         (optional speed)       lane speed of the lanes (default to using the first lanes speed)
    -l,--loss       (optional decibel)     channel loss of the datapath
    -o,--optical    (optional integer)     optical voltage swing, also indicates the channels are optical

Examples:

    > /serdes tx_taps preset a0-a7 -l 25db             # set a0-a7 lanes to tx_taps for 25db (lane speed )
    > /serdes tx_taps preset all -o 6                  # set all lanes to optical with vswing of 6
    > /serdes tx_taps preset a0-a7 nrz 25g -l 12db     # set all lanes to 12db loss where they will be in nrz 25g
]], function(slice, argt)
        local speed = argt.speed
        local mode = argt.mode
        local def_lane = argt.lanes[1]

        if speed == nil then
            speed = credo.lane_get_speed(slice, def_lane)
        end

        if mode == nil then
            mode = credo.lane_get_mode(slice, def_lane)
        end

        assert(crutil.is_lane_configured(mode), "Invalid lane mode given or lane.0 not configured")
        assert(speed > 0, "Invalid lane speed given or lane.0 not configured")

        local optical_vswing = argt.optical
        local loss = argt.loss
        local optical = argt.optical ~= nil
        assert(loss ~= nil or optical_vswing ~= nil, "Must provide loss or optical voltage swing")

        if optical_vswing == nil then
            optical_vswing = 0
        end

        if loss == nil then
            loss = 0
        end
        local desc = credo.ChannelDesc()
        desc.speed = speed
        desc.mode = mode
        desc.optical = optical
        desc.channel_loss = loss
        desc.opt_vswing = optical_vswing
        for lane in iter(argt.lanes) do
            credo.serdes_preset_tx_taps(slice, lane, desc)
        end
    end)
end

return serdes
