local credo = require "credo"
local slash = require "slash"
local fort = require "fort"

local lane_mapping_50g = {[1] = {0, 2, 4, 6}, [2] = {0, 1, 4, 5}, [4] = {0, 1, 2, 3}}
local lane_mapping_100g = {[2] = {0, 4}, [4] = {0, 2}}

slash.register_chip_command({"gearbox", "config"}, credo.FAMILY_BLACKHAWK, [[
    Configure a Gearbox port


    Simplifying the creation of a gearbox with parameters that are more meaningful, and implicitly computing other values
    such as line lanes.

    Arguments:

        <speed>            (speed)                  Speed of port
        <mapping>          (m1-2|m2-4|m4-8)         Mapping ID of the lanes (refer to section below)
        <host_lanes>       (lanelist)               Host start lanes of the port(s)
        -g,--flags         (portflags default 0)    Port flags
        -e,--fec           (fec default none)       Port line side fec type
        -f,--force                                  Force configure even if lanes are used in another port
        -n,--dry-run                                Do not configure the port, print out instead

    Examples:

        > gearbox config 100g m2-4 0,4                  # 2-100g gearbox on mapping m1
        > gearbox config 50g m1-2 0:6:2                 # 4-50g gearbox on mapping m0
        > gearbox config 50g m1-2 0:6:2 -n              # dry run of above
        > gearbox config 50g m1-2 0:6:2 -g sopt|lopt    # add optical flags

    Mapping:
                        Host Lanes
        Mapping ID | 50g           100g
        0: m1-2    | [0, 2, 4, 6]  INVALID
        1: m2-4    | [0, 1, 4, 5]  [0, 4]
        2: m4-8    | [0, 1, 2, 3]  [0, 2]
    ]], function(slice, argt)
    assert(argt.speed == 50000 or argt.speed == 100000,
           "Invalid speed %.1fg, only 50G/100G supported" % {argt.speed / 1000})
    ---@type boolean
    local force = argt.force

    local speed_lane_maps = choose(argt.speed == 50000, lane_mapping_50g, lane_mapping_100g)
    local host_lane_count = choose(argt.speed == 50000, 1, 2)
    -- force rs528 on nrz lanes
    if argt.speed == 50000 then
        argt.fec = credo.FEC_RS_528
    end

    local mapping = int(string.sub(argt.mapping, 2, 2))

    -- ensure the host lane is valid for the mapping
    local lane_map = speed_lane_maps[mapping]
    assert(lane_map ~= nil, "Invalid map %s for %dg" % {argt.mapping, argt.speed // 1000})
    for host_lane in iter(argt.host_lanes) do
        assert(list.index(lane_map, host_lane) ~= nil, "Invalid host lane %d for map %s" % {host_lane, argt.mapping})
    end
    ---@param host_lane integer
    for host_idx, host_lane in pairs(argt.host_lanes) do
        ---@type integer
        local line_start_lane = 8 + (2 * host_lane_count * (list.index(lane_map, host_lane) - 1))

        local port_config = credo.PortConfig()
        port_config.port_id = credo.PORT_AUTO_ASSIGN_ID
        port_config.port_mode = credo.PMODE_SERDES
        port_config.port_type = credo.PORT_GEARBOX
        port_config.speed = argt.speed
        port_config.host_start_lane = host_lane
        port_config.host_no_of_lanes = host_lane_count
        port_config.host_fec_type = credo.FEC_RS_544
        port_config.line_start_lane = line_start_lane
        port_config.line_no_of_lanes = 2 * host_lane_count
        port_config.line_fec_type = argt.fec
        port_config.flags = argt.flags
        if argt.dry_run then
            print("Config %d/%d" % {host_idx, #argt.host_lanes})
            print(port_config)
        else
            credo.port_configure(slice, port_config, force)
        end
    end
end)

slash.register_chip_command_alias({"gearbox", "config"}, {"port", "gearbox", "config"}, credo.FAMILY_BLACKHAWK)

slash.register_chip_command({"gearbox", "status"}, credo.FAMILY_BLACKHAWK, [[
    Display Retimer status.

    Arguments:

        -n,--new

    ]], function(slice, argt)
    if not argt.new then
        credo.display_info_print(slice, "gearbox_status")
        return
    end

    local ftable = fort.new()
    ftable:write_ln("Port", "Dir", "GB\nSt", "FEC\nAlign", "PCS\nAlign", "AM\nLocked", "CORR\nERR", "UNCORR\nERR",
                    "RX FIFO\n%4s [%4s %4s]" % {"Cur", "Min", "Max"}, "TX FIFO\n%4s [%4s %4s]" % {"Cur", "Min", "Max"})
    ftable:add_separator()
    for port_id = 0, 7 do

        local port_config = credo.port_query(slice, port_id)
        if port_id ~= port_config.port_id or port_config.port_type ~= credo.PORT_GEARBOX then
            ftable:print_ln("%d" % port_id)
            goto end_loop
        end

        ftable:add_separator()

        local gb_state_a = credo.firmware_debug_cmd(slice, port_config.host_start_lane, 5, 0)

        local fifo_a = credo.rsfec_get_fifo(slice, port_id, credo.SIDE_HOST)
        local align_a = credo.rsfec_get_align_status(slice, port_id, credo.SIDE_HOST)
        local corr_err_a = credo.rsfec_get_corrected_codeword(slice, port_id, credo.SIDE_HOST)
        local uncorr_err_a = credo.rsfec_get_uncorrected_codeword(slice, port_id, credo.SIDE_HOST)
        local am_locked_str_a = list.map(list.slice(align_a.AM_locked, 1, port_config.host_no_of_lanes),
                                         function(locked)
            return choose(locked, "1", "0")
        end)
        am_locked_str_a = stringx.join(",", am_locked_str_a)

        local gb_state_b = credo.firmware_debug_cmd(slice, port_config.line_start_lane, 5, 0)
        local fifo_b = credo.rsfec_get_fifo(slice, port_id, credo.SIDE_LINE)
        local align_b = credo.rsfec_get_align_status(slice, port_id, credo.SIDE_LINE)
        local corr_err_b = credo.rsfec_get_corrected_codeword(slice, port_id, credo.SIDE_LINE)
        local uncorr_err_b = credo.rsfec_get_uncorrected_codeword(slice, port_id, credo.SIDE_LINE)
        local am_locked_str_b = list.map(list.slice(align_b.AM_locked, 1, port_config.line_no_of_lanes),
                                         function(locked)
            return choose(locked, "1", "0")
        end)
        am_locked_str_b = stringx.join(",", am_locked_str_b)

        ftable:print("%d" % {port_id})
        ftable:print("A->B|%d" % {gb_state_a})
        ftable:print("%d|%d|%s|%d|%d|%4d [%4d %4d]" % {
            choose(align_a.fec_aligned, 1, 0), choose(align_a.pcs_aligned, 1, 0), am_locked_str_a, corr_err_a,
            uncorr_err_a, fifo_a.rx_cur, fifo_a.rx_min, fifo_a.rx_max
        })
        if port_config.line_fec_type ~= credo.FEC_NONE then
            ftable:print_ln("%4d [%4d %4d]" % {fifo_b.tx_cur, fifo_b.tx_min, fifo_b.tx_max})
        else
            ftable:print_ln("- - -")
        end

        ftable:print(" |B->A|%d" % {gb_state_b})

        if port_config.line_fec_type ~= credo.FEC_NONE then
            ftable:print("%d|%d|%s|%d|%d|%4d [%4d %4d]" % {
                choose(align_b.fec_aligned, 1, 0), choose(align_b.pcs_aligned, 1, 0), am_locked_str_b, corr_err_b,
                uncorr_err_b, fifo_b.rx_cur, fifo_b.rx_min, fifo_b.rx_max
            })
        else
            ftable:print(" |-|-|-|- - -")
        end
        ftable:print_ln("%4d [%4d %4d]" % {fifo_a.tx_cur, fifo_a.tx_min, fifo_a.tx_max})

        ftable:add_separator()

        ::end_loop::
    end
    print(ftable)
end)

slash.register_chip_command_alias({"gearbox", "status"}, {"port", "gearbox", "status"}, credo.FAMILY_BLACKHAWK)
