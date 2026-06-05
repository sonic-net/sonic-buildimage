local credo = require "credo"
local slash = require "slash"
local fort = require "fort"
local crutil = require "crutil"
local timedelta = require "timedelta"
local prbs = {}

---comments
---@param family credo.Family
---@param options? {check_lock?: boolean}
function prbs.register_prbs_berhist(family, options)
    options = options or {}

    local check_lock = options.check_lock or false

    slash.register_chip_command_multi({"prbs", "berhist"}, family, [[
Capture PRBS BER histogram for many slices and lanes

OFF indicates the lane is not configured.
UNRDY indicates that the PRBS value is not valid:
- lane not ready
- prbs checker not enabled
- prbs not locked

Arguments:
    <lanes>        (lanelist default all)
    -g,--group                               group by device instead of slice
    --id           (integer default 6)       show id locations if toal bin <= id
    -r,--reset                               reset the rx counters
    -b,--bin       (integer default 1)       bin size

    ]], function(slices, argt, argv)
        local bin = argt.bin ---@type number
        local bin_width = bin
        -- supporting bin size >= 10 is complicated
        assert(bin > 0 and bin < 10, "bin size must be 0 < bin < 10")
        -- capture prbs ber
        local data = {} ---@type table<integer, table<integer, number>>
        local post_fec_data = {} ---@type table<integer, number>
        local total_post_fec = 0
        local lanes = argt.lanes ---@type integer[]
        local group_device = argt.group ---@type boolean

        local prbs_min_duration_data = {} ---@type table<integer, number>
        local prbs_max_duration_data = {} ---@type table<integer, number>

        if argt.reset then
            for slice in iter(slices) do
                for lane in iter(lanes) do
                    local mode = credo.lane_get_mode(slice, lane)
                    if mode == credo.LMODE_OFF then
                        goto continue
                    end
                    local en = credo.prbs_get_rx_checker(slice, lane)
                    if en then
                        credo.prbs_reset_rx_count(slice, lane)
                        local fecana_en, _ = credo.fecana_query(slice, lane)
                        if fecana_en then
                            credo.fecana_reset(slice, lane)
                        end
                    end
                    ::continue::
                end

            end
            return
        end

        for slice in iter(slices) do
            data[slice] = {}
            post_fec_data[slice] = 0
            prbs_min_duration_data[slice] = math.huge
            prbs_max_duration_data[slice] = 0
            for lane in iter(lanes) do
                local mode = credo.lane_get_mode(slice, lane)
                if mode == credo.LMODE_OFF then
                    data[slice][lane] = -2
                    goto continue
                end
                local en = credo.prbs_get_rx_checker(slice, lane)
                local phy_ready = credo.serdes_get_phy_ready(slice, lane)
                local locked = true
                if check_lock then
                    locked = credo.prbs_get_rx_lock(slice, lane) == credo.PRBS_LOCK_YES
                end
                if en and phy_ready and locked then
                    data[slice][lane] = credo.prbs_get_rx_ber(slice, lane, 0)
                    local fecana_en, _ = credo.fecana_query(slice, lane)
                    local prbs_duration = credo.prbs_get_rx_duration(slice, lane)
                    prbs_max_duration_data[slice] = math.max(prbs_max_duration_data[slice], prbs_duration)
                    prbs_min_duration_data[slice] = math.min(prbs_min_duration_data[slice], prbs_duration)
                    if fecana_en then
                        local _, post_fec = credo.fecana_get_counter(slice, lane)
                        post_fec_data[slice] = post_fec_data[slice] + post_fec
                        total_post_fec = total_post_fec + post_fec
                    end
                else
                    data[slice][lane] = -1
                end
                ::continue::
            end
        end

        local ber_hist = {}
        local ber_hist_map = {} ---@type table<integer, string[]>
        local total_ber_hist = {}
        local min_group = 3 * bin_width
        local max_group = 12 * bin_width
        for slice in iter(slices) do
            ber_hist[slice] = {}
            for lane in iter(lanes) do
                local ber_data = data[slice][lane]
                local group
                if ber_data == 0 then
                    group = 0
                elseif ber_data == -1 then
                    group = -1
                elseif ber_data == -2 then
                    group = -2
                else
                    -- select base group
                    group = math.ceil(math.abs(math.log(data[slice][lane], 10))) * bin_width
                    group = group - math.floor(bin_width * data[slice][lane] * 10 ^ ((group // bin_width) - 1))
                    -- find the max group
                    max_group = math.max(max_group, group)
                    -- bound the minimum group
                    if group < min_group then
                        group = min_group
                    end
                end
                if ber_hist[slice][group] == nil then
                    ber_hist[slice][group] = 0
                end
                if total_ber_hist[group] == nil then
                    total_ber_hist[group] = 0
                end
                if ber_hist_map[group] == nil then
                    ber_hist_map[group] = {}
                end
                ber_hist[slice][group] = ber_hist[slice][group] + 1
                total_ber_hist[group] = total_ber_hist[group] + 1
                table.insert(ber_hist_map[group], "%d.%s" % {slice, crutil.get_lane_id(slice, lane)})
            end
        end

        ---@type integer[][]
        local slice_groups = list.map(slices, function(value)
            return {value}
        end)

        local max_id_count = argt.id ---@type integer

        -- determine slice groups into devices
        if group_device then
            slice_groups = {}
            local dev_map = {} ---@type table<lightuserdata, integer>
            local devices = 0
            for slice in iter(slices) do
                local dev = credo.slice_get_device(slice)
                if dev_map[dev] == nil then
                    devices = devices + 1
                    dev_map[dev] = devices
                    slice_groups[devices] = {}
                end
                local dev_index = dev_map[dev]
                table.insert(slice_groups[dev_index], slice)
            end
        end

        local ftable = fort.new()
        if group_device then
            ftable:print_ln("|Devices")
        else
            ftable:print_ln("|Slice")
        end

        ftable:print("BER Bin")
        for slice_group in iter(slice_groups) do
            ftable:print("%s" % {list.join(slice_group, "\n")})
        end
        ftable:print("All|Lane Ids")
        ftable:ln()
        ftable:add_separator()

        do
            local ber_count ---@type integer
            ftable:print("OFF")
            for slice_group in iter(slice_groups) do
                local total_ber_count = 0
                for slice in iter(slice_group) do
                    ber_count = ber_hist[slice][-2] or 0
                    total_ber_count = total_ber_count + ber_count
                end
                ftable:print("%d" % {total_ber_count})
            end
            ber_count = total_ber_hist[-2] or 0
            ftable:print("%d" % {ber_count})

            local ber_map = ber_hist_map[-2] or {}
            if #ber_map <= max_id_count then
                local ftable_id = fort.new()
                ftable_id:set_border_style(fort.EMPTY_STYLE)
                ftable_id:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
                for i, ber_loc in ipairs(ber_map) do
                    ftable_id:print(ber_loc)
                    if i % 8 == 0 then
                        ftable_id:ln()
                    end
                end
                ftable:write(stringx.strip(tostring(ftable_id), "\n"))
            end
            ftable:ln()

            ftable:print("UNRDY")
            for slice_group in iter(slice_groups) do
                local total_ber_count = 0
                for slice in iter(slice_group) do
                    ber_count = ber_hist[slice][-1] or 0
                    total_ber_count = total_ber_count + ber_count
                end
                ftable:print("%d" % {total_ber_count})
            end
            ber_count = total_ber_hist[-1] or 0
            ftable:print("%d" % {ber_count})

            ber_map = ber_hist_map[-1] or {}
            if #ber_map <= max_id_count then
                local ftable_id = fort.new()
                ftable_id:set_border_style(fort.EMPTY_STYLE)
                ftable_id:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
                for i, ber_loc in ipairs(ber_map) do
                    ftable_id:print(ber_loc)
                    if i % 8 == 0 then
                        ftable_id:ln()
                    end
                end
                ftable:write(stringx.strip(tostring(ftable_id), "\n"))
            end
            ftable:ln()

            ftable:print("Post FEC")
            for slice_group in iter(slice_groups) do
                local total_ber_count = 0
                for slice in iter(slice_group) do
                    ber_count = post_fec_data[slice] or 0
                    total_ber_count = total_ber_count + ber_count
                end
                ftable:print("%d" % {total_ber_count})
            end
            ftable:print("%d" % {total_post_fec})
            ftable:ln()
            ftable:add_separator()
        end
        for group = min_group, max_group do
            local sub_group = group % bin_width

            ftable:print(">%.2e" % {(bin_width - sub_group) / bin_width * 10 ^ (-(group // bin_width))})
            for slice_group in iter(slice_groups) do
                local total_ber_count = 0
                for slice in iter(slice_group) do
                    local ber_count = ber_hist[slice][group] or 0
                    total_ber_count = total_ber_count + ber_count
                end
                ftable:print("%d" % {total_ber_count})
            end
            local ber_count = total_ber_hist[group] or 0
            ftable:print("%d" % {ber_count})
            local ber_map = ber_hist_map[group] or {}
            if #ber_map <= max_id_count then
                local ftable_id = fort.new()
                ftable_id:set_border_style(fort.EMPTY_STYLE)
                ftable_id:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
                for i, ber_loc in ipairs(ber_map) do
                    ftable_id:print(ber_loc)
                    if i % 8 == 0 then
                        ftable_id:ln()
                    end
                end
                ftable:write(stringx.strip(tostring(ftable_id), "\n"))
            end

            ftable:ln()
        end
        ftable:print("=0e00")
        for slice_group in iter(slice_groups) do
            local total_ber_count = 0
            for slice in iter(slice_group) do
                local ber_count = ber_hist[slice][0] or 0
                total_ber_count = total_ber_count + ber_count
            end
            ftable:print("%d" % {total_ber_count})

        end
        local ber_count = total_ber_hist[0] or 0
        ftable:print("%d" % {ber_count})
        local ber_map = ber_hist_map[0] or {}
        if #ber_map <= max_id_count then
            local ftable_id = fort.new()
            ftable_id:set_border_style(fort.EMPTY_STYLE)
            ftable_id:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
            for i, ber_loc in ipairs(ber_map) do
                ftable_id:print(ber_loc)
                if i % 8 == 0 then
                    ftable_id:ln()
                end
            end
            ftable:write(stringx.strip(tostring(ftable_id), "\n"))
        end
        ftable:ln()
        ftable:add_separator()

        ftable:print("Total")
        for slice_group in iter(slice_groups) do
            ftable:print("%d" % {#slice_group * #lanes})
        end
        ftable:print("%d" % {#slices * #lanes})
        ftable:ln()
        ftable:add_separator()
        ftable:print("PRBS Dur\nRange")
        for slice_group in iter(slice_groups) do
            local prbs_min_duration = math.huge
            local prbs_max_duration = 0

            for slice in iter(slice_group) do
                local prbs_cur_min_duration = prbs_min_duration_data[slice] or math.huge
                local prbs_cur_max_duration = prbs_max_duration_data[slice] or 0
                prbs_min_duration = math.min(prbs_min_duration, prbs_cur_min_duration)
                prbs_max_duration = math.max(prbs_max_duration, prbs_cur_max_duration)
            end
            if prbs_min_duration == math.huge then
                prbs_min_duration = 0
            end
            ftable:print("%s\n%s" %
                             {
                    timedelta.pretty(prbs_min_duration / 1000, false), timedelta.pretty(prbs_max_duration / 1000, false)
                })
        end

        if #slice_groups > 1 then
            ftable:set_cell_span(1, 2, #slice_groups)
        end
        ftable:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_RIGHT)
        ftable:set_cell_prop(fort.ANY_ROW, -1, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_LEFT)
        ftable:set_cell_prop(1, 2, fort.CPROP_TEXT_ALIGN, fort.ALIGNED_LEFT)

        print(ftable)

    end)

end

---@param family credo.Family
function prbs.register_prbs_phase_error(family)
    slash.register_chip_command({"prbs", "phase_error"}, family, [[
Display Prbs phase error table.

Arguemnts:

    -d,--duration   (optional time)    Only whole seconds are used
    <lane>          (lane)

]], function(slice, argt)
        if argt.duration == nil then
            credo.display_info_print(slice, "prbs_phase_error %d" % {argt.lane})
        else
            credo.display_info_print(slice, "prbs_phase_error %d %d" % {argt.lane, math.floor(argt.duration * 1000)})
        end
    end)
end

return prbs
