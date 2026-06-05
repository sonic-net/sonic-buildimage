local slash = require "slash"
local credo = require "credo"
local fort = require "fort"

slash.register_chip_command({"rsfec", "monitor"}, credo.FAMILY_BLACKHAWK, [[
Display rsfec counter and error rates.

Given that the counters are clear on read, the error rates can only be properly
computed when a duration is provided. Therefore we only show error rate values
when a duration is specified

Arguments:

    <ports>          (portlist default all)    List of ports to access
    -d,--duration    (optional time)           Capture over a period of time
    -f,--freeze                                Freeze monitor capture
]], function(slice, argt)

    assert(argt.duration == nil or argt.duration > 0, "Duration must be > 0ms")

    ---@type table<integer,credo.PortConfig>
    local port_configs = {}
    for port_id in iter(argt.ports) do
        local port_config = credo.port_query(slice, port_id)
        port_configs[port_id] = port_config
        if port_id ~= port_config.port_id or port_config.port_type ~= credo.PORT_GEARBOX then
            goto end_loop
        end

        if argt.duration ~= nil then
            credo.rsfec_reset_count(slice, port_id, credo.SIDE_HOST)
            credo.rsfec_reset_count(slice, port_id, credo.SIDE_LINE)
        end

        ::end_loop::
    end

    if argt.duration ~= nil then
        time.sleep(argt.duration)
    end

    local ftable = fort.new()
    ftable:write_ln("Port", "Dir", "Align", "CORR\nERR", "UNCORR\nERR", "TOTAL\nCODEWORDS", "CORR\nError Rate")
    ftable:add_separator()

    for port_id in iter(argt.ports) do
        local port_config = port_configs[port_id]
        if port_id ~= port_config.port_id or port_config.port_type ~= credo.PORT_GEARBOX then
            ftable:print_ln("%d" % port_id)
            goto end_loop
        end

        if argt.freeze then
            credo.rsfec_set_count_freeze(slice, port_id, credo.SIDE_HOST, true)
            credo.rsfec_set_count_freeze(slice, port_id, credo.SIDE_LINE, true)
        end

        local align_a = credo.rsfec_get_align_status(slice, port_id, credo.SIDE_HOST)
        -- local corr_bits_a = credo.rsfec_get_corrected_bits(slice, port_id, credo.SIDE_HOST)
        local corr_err_a = credo.rsfec_get_corrected_codeword(slice, port_id, credo.SIDE_HOST)
        local uncorr_err_a = credo.rsfec_get_uncorrected_codeword(slice, port_id, credo.SIDE_HOST)
        local total_word_a = credo.rsfec_get_total_codeword(slice, port_id, credo.SIDE_HOST)

        local align_b = credo.rsfec_get_align_status(slice, port_id, credo.SIDE_LINE)
        -- local corr_bits_b = credo.rsfec_get_corrected_bits(slice, port_id, credo.SIDE_LINE)
        local corr_err_b = credo.rsfec_get_corrected_codeword(slice, port_id, credo.SIDE_LINE)
        local uncorr_err_b = credo.rsfec_get_uncorrected_codeword(slice, port_id, credo.SIDE_LINE)
        local total_word_b = credo.rsfec_get_total_codeword(slice, port_id, credo.SIDE_LINE)

        if argt.freeze then
            credo.rsfec_set_count_freeze(slice, port_id, credo.SIDE_HOST, false)
            credo.rsfec_set_count_freeze(slice, port_id, credo.SIDE_LINE, false)
        end

        ftable:print("%d|A->B|%s" % {port_id, choose(align_a.fec_aligned, "yes", "no")})
        ftable:print("%d|%d|%.3g" % {corr_err_a, uncorr_err_a, total_word_a})
        ftable:print(choose(argt.duration ~= nil and total_word_a > 0, "%.2e" % {corr_err_a / total_word_a}, "-"))
        ftable:ln()

        ftable:print("|B->A|%s" % {choose(align_b.fec_aligned, "yes", "no")})

        local lfec_en = port_config.line_fec_type == credo.FEC_RS_528
        ftable:print(choose(lfec_en, "%d|%d|%.3g" % {corr_err_b, uncorr_err_b, total_word_b}, "-|-|-"))

        ftable:print(choose(lfec_en and argt.duration ~= nil and total_word_b > 0, "%.2g" % {corr_err_b / total_word_b},
                            "-"))
        ftable:ln()
        ftable:add_separator()

        ::end_loop::
    end
    print(ftable)
end)

slash.register_chip_command({"rsfec", "histogram"}, credo.FAMILY_BLACKHAWK, [[
Display rsfec counter histogram.

Arguments:

    <ports>          (portlist default all)    ports to capture histogram
    -c,--clear                                 clear the histogram

]], function(slice, argt)

    local ftable = fort.new()
    ftable:write("Symbol")

    ---@type table<integer, credo.PortConfig>
    local port_configs = {}

    for port_id in iter(argt.ports) do
        local port_config = credo.port_query(slice, port_id)
        if port_id ~= port_config.port_id or port_config.port_type ~= credo.PORT_GEARBOX then
            goto end_loop
        end
        port_configs[port_id] = port_config
        ftable:write("Port %d" % {port_id}, "")

        if argt.clear then
            credo.rsfec_reset_count(slice, port_id, credo.SIDE_HOST)
            credo.rsfec_reset_count(slice, port_id, credo.SIDE_LINE)
        end

        ::end_loop::
    end

    -- skip printing anything on clear command
    if argt.clear then
        return
    end

    ftable:ln()
    ftable:write("Bin")
    for port_id in iter(argt.ports) do
        local port_config = port_configs[port_id]
        if port_config == nil then
            goto end_loop
        end
        ftable:write("host (A)", "line (B)")
        ::end_loop::
    end
    ftable:ln()
    ftable:add_separator()
    for hist_bin = 0, 16 do
        ftable:print(">= %d" % {hist_bin})
        for port_id in iter(argt.ports) do
            local port_config = port_configs[port_id]
            if port_config == nil then
                goto end_loop
            end
            local hist_val_a = credo.rsfec_get_histogram(slice, port_id, credo.SIDE_HOST, hist_bin)

            ftable:print("%d" % {hist_val_a})
            if port_config.line_fec_type ~= credo.FEC_NONE then
                local hist_val_b = credo.rsfec_get_histogram(slice, port_id, credo.SIDE_LINE, hist_bin)
                ftable:print("%d" % {hist_val_b})
            else
                ftable:print("-")
            end
            ::end_loop::
        end
        ftable:ln()
    end

    -- make the port id span 2 columns
    for col = 1, ftable:col_count() // 2 do
        ftable:set_cell_span(1, col * 2, 2)
    end

    print(ftable)
end)

slash.register_chip_command({"rsfec", "clear"}, credo.FAMILY_BLACKHAWK, [[
Clear rsfec counters.

Arguments:

    <ports>    (portlist)    ports to clear rsfec counters

]], function(slice, argt)

    for port_id in iter(argt.ports) do
        local port_config = credo.port_query(slice, port_id)
        if port_id ~= port_config.port_id or port_config.port_type ~= credo.PORT_GEARBOX then
            goto end_loop
        end

        credo.rsfec_reset_count(slice, port_id, credo.SIDE_HOST)
        credo.rsfec_reset_count(slice, port_id, credo.SIDE_LINE)

        ::end_loop::
    end
end)

slash.register_chip_command({"rsfec", "status"}, credo.FAMILY_BLACKHAWK, [[
Display rsfec status.

This is an alias for the slash command `/port gearbox status -n`.
]], function(slice, argt)
    slash("port gearbox status -n")
end, slash.MULTISLICE)
