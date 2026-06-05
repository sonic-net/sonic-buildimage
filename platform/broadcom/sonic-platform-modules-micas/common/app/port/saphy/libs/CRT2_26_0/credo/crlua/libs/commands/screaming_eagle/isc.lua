local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"

local ISC_DEBUG = 15

local ISC_DEBUG_REMOTE_LINK_STATUS = 500
local ISC_DEBUG_LOCAL_PHY_READY = 501
local ISC_DEBUG_LOCAL_DATA_READY = 502
local ISC_DEBUG_LOCAL_AN_STATUS = 503
local ISC_DEBUG_LOCAL_LT_STATUS = 504
local ISC_DEBUG_REMOTE_PHY_READY = 505
local ISC_DEBUG_REMOTE_DATA_READY = 506
local ISC_DEBUG_REMOTE_AN_STATUS = 507
local ISC_DEBUG_REMOTE_LT_STATUS = 508

---Convert tx mapping to lane mask
---@param tx_map integer
local function compute_lane_mask(tx_map)
    local mask = 0
    for lane = 0, 7 do
        local lane_map = (tx_map >> (4 * lane)) & 0xF
        -- print("%X" % {lane_map})
        if lane_map >= 0x8 then
            mask = mask | (1 << lane)
        end
    end
    return mask
end

--- will not handle msblsb swapping
---@param lane_map_a integer
---@param lane_map_b integer
---@return integer[]
---@return integer[]
local function convert_lane_map(lane_map_a, lane_map_b)
    local a_cnt = 0
    local b_cnt = 0

    for lane = 0, 7 do
        if (lane_map_a >> (4 * lane)) & 0x8 == 0x8 then
            a_cnt = a_cnt + 1
        end
        if (lane_map_b >> (4 * lane)) & 0x8 == 0x8 then
            b_cnt = b_cnt + 1
        end
    end

    local lane_map = lane_map_a
    if b_cnt > a_cnt then
        -- switch what to use if a2b1
        lane_map = lane_map_b
    end

    local host_lanes = {}
    local line_lanes = {}
    for lane = 0, 7 do

        if (lane_map >> (4 * lane)) & 0x8 == 0x8 then
            list.append(host_lanes, lane)
            local lane2 = (lane_map >> (4 * lane)) & 0x7
            if not list.contains(line_lanes, lane2) then
                list.append(line_lanes, lane2)
            end
        end
    end

    if b_cnt > a_cnt then
        -- swap if a1b2
        host_lanes, line_lanes = line_lanes, host_lanes
    end
    line_lanes = list.map(line_lanes, function(lane)
        return lane + 8
    end)
    return host_lanes, line_lanes
end

slash.register_chip_command({"isc", "info"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display port isc info

    <ports>    (portlist default all)    list of ports to see status

]], function(slice, argt)

    ---@type integer[]
    local ports = argt.ports

    -- check slice actually has an ISC partner
    local partner_slice = credo.slice_get_option(slice, "isc_slice_id")
    if partner_slice == 0xFFFF then
        print("Slice does not have ISC Enabled")
        return
    end

    local ftable = fort.create()
    ftable:print_ln("Port|Mode|Speed|Side|Local (Slice %d)|||Remote (Slice %d)||" % {slice, partner_slice})
    ftable:print("||||Lanes|AN|LT")
    ftable:print_ln("Lanes|AN|LT")
    ftable:add_separator()
    for port in iter(ports) do
        -- skip invalid ports or ports not in ISC
        local started, setup = credo.port_get_setup(slice, port)
        if not started or setup == nil then
            ftable:print_ln("%d" % {port})
            goto continue
        end
        local isc_enabled = credo.port_get_option(slice, port, "isc_enable")
        if isc_enabled == 0 then
            ftable:print_ln("%d" % {port})
            goto continue
        end

        local host_main_lane = credo.port_get_option(slice, port, "host_main_lane")
        local line_main_lane = credo.port_get_option(slice, port, "line_main_lane")
        -- indicate if the ISC port has it enabled
        local line_lt = credo.port_get_option(slice, port, "line_lt") ~= 0
        local lc_an = credo.port_get_option(slice, port, "line_an") ~= 0
        local rm_an = credo.port_get_option(slice, port, "isc_an_agent") ~= 0

        local rm_lane_map_a = credo.port_get_option(slice, port, "isc_lane_map_a")
        local rm_lane_map_b = credo.port_get_option(slice, port, "isc_lane_map_b")

        local rm_host_lanes, rm_line_lanes = convert_lane_map(rm_lane_map_a, rm_lane_map_b)

        local rm_host_main_lane = credo.port_get_option(slice, port, "isc_host_main_lane")
        local rm_line_main_lane = credo.port_get_option(slice, port, "isc_line_main_lane")

        local lc_host_lanes_str = stringx.join(",", list.map(setup.host_lanes, function(lane)
            local is_main = lane == host_main_lane
            return choose(is_main, "[", "") .. crutil.get_lane_id(slice, lane) .. choose(is_main, "]", "")
        end))
        local lc_line_lanes_str = stringx.join(",", list.map(setup.line_lanes, function(lane)
            local is_main = lane == line_main_lane
            return choose(is_main, "[", "") .. crutil.get_lane_id(slice, lane) .. choose(is_main, "]", "")
        end))

        local rm_host_lanes_str = stringx.join(",", list.map(rm_host_lanes, function(lane)
            local is_main = lane == rm_host_main_lane
            return choose(is_main, "[", "") .. crutil.get_lane_id(slice, lane) .. choose(is_main, "]", "")
        end))
        local rm_line_lanes_str = stringx.join(",", list.map(rm_line_lanes, function(lane)
            local is_main = lane == rm_line_main_lane
            return choose(is_main, "[", "") .. crutil.get_lane_id(slice, lane) .. choose(is_main, "]", "")
        end))

        ftable:write(port, setup.mode.display_name, "%.1fG" % {setup.speed / 1000}, "host", lc_host_lanes_str, "-", "-",
                     rm_host_lanes_str, "-", "-")

        ftable:ln()

        local lc_an_str = "DIS"
        local rm_an_str = "DIS"
        if lc_an then
            lc_an_str = "EN"
            rm_an_str = "AGENT"
        elseif rm_an then
            lc_an_str = "AGENT"
            rm_an_str = "EN"
        end

        ftable:write("", "", "", "line", lc_line_lanes_str, lc_an_str, choose(line_lt, "EN", "DIS"), rm_line_lanes_str,
                     rm_an_str, choose(line_lt, "EN", "DIS"))

        ftable:ln()
        ftable:add_separator()
        ::continue::
    end

    ftable:set_cell_span(1, 5, 3)
    ftable:set_cell_span(1, 8, 3)
    print(ftable)

end)

slash.register_chip_command({"isc", "status"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display port isc status

    <ports>    (portlist default all)    list of ports to see status

]], function(slice, argt)

    ---@type integer[]
    local ports = argt.ports

    -- check slice actually has an ISC partner
    local partner_slice = credo.slice_get_option(slice, "isc_slice_id")
    if partner_slice == 0xFFFF then
        print("Slice does not have ISC Enabled")
        return
    end

    local ftable = fort.create()
    ftable:print_ln("Port|Side|Status|Local (Slice %d)|||||Remote (Slice %d)|||||" % {slice, partner_slice})
    ftable:print("|||Lane\nMask     (Main)|PHY\nReady|Data\nReady|AN\nStatus|LT\nStatus")
    ftable:print_ln("Lane\nMask     (Main)|Lane\nLink|PHY\nReady|Data\nReady|AN\nStatus|LT\nStatus")
    ftable:add_separator()
    for port in iter(ports) do
        -- skip invalid ports or ports not in ISC
        local started, setup = credo.port_get_setup(slice, port)
        if not started or setup == nil then
            ftable:print_ln("%d" % {port})
            goto continue
        end
        local isc_enabled = credo.port_get_option(slice, port, "isc_enable")
        if isc_enabled == 0 then
            ftable:print_ln("%d" % {port})
            goto continue
        end

        local host_up = credo.port_is_link_up(slice, port, credo.SIDE_HOST)
        local line_up = credo.port_is_link_up(slice, port, credo.SIDE_LINE)

        local host_main_lane = credo.port_get_option(slice, port, "host_main_lane")
        local line_main_lane = credo.port_get_option(slice, port, "line_main_lane")
        local host_main_lane_mask = 1 << host_main_lane
        local line_main_lane_mask = 1 << line_main_lane

        -- indicate if the ISC port has it enabled
        local line_lt = credo.port_get_option(slice, port, "line_lt") ~= 0
        local lc_an = credo.port_get_option(slice, port, "line_an") ~= 0
        local rm_an = credo.port_get_option(slice, port, "isc_an_agent") ~= 0

        local lc_host_lane_mask = compute_lane_mask(credo.port_get_option(slice, port, "lane_map_a"))
        local lc_line_lane_mask = compute_lane_mask(credo.port_get_option(slice, port, "lane_map_b")) << 8

        local rm_host_lane_mask = compute_lane_mask(credo.port_get_option(slice, port, "isc_lane_map_a"))
        local rm_line_lane_mask = compute_lane_mask(credo.port_get_option(slice, port, "isc_lane_map_b")) << 8

        local rm_host_main_lane = credo.port_get_option(slice, port, "isc_host_main_lane")
        local rm_line_main_lane = credo.port_get_option(slice, port, "isc_line_main_lane")
        local rm_host_main_lane_mask = 1 << rm_host_main_lane
        local rm_line_main_lane_mask = 1 << rm_line_main_lane

        for lane in iter({host_main_lane, line_main_lane}) do

            local is_host = lane == host_main_lane
            local lc_side_lane_mask = choose(is_host, lc_host_lane_mask, lc_line_lane_mask)
            local rm_side_lane_mask = choose(is_host, rm_host_lane_mask, rm_line_lane_mask)

            local lc_phy_ready = credo.firmware_debug_cmd(slice, lane, ISC_DEBUG, ISC_DEBUG_LOCAL_PHY_READY)
            local lc_side_phy_ready = choose((lc_phy_ready & lc_side_lane_mask) == lc_side_lane_mask, "YES", "NO")
            local lc_data_ready = credo.firmware_debug_cmd(slice, lane, ISC_DEBUG, ISC_DEBUG_LOCAL_DATA_READY)
            local lc_side_data_ready = choose((lc_data_ready & lc_side_lane_mask) == lc_side_lane_mask, "YES", "NO")
            local lc_an_status = credo.firmware_debug_cmd(slice, lane, ISC_DEBUG, ISC_DEBUG_LOCAL_AN_STATUS)

            local lc_an_status_str = "OFF"
            if is_host then
                lc_an_status_str = " - "
            elseif lc_an then
                lc_an_status_str = choose((lc_an_status & line_main_lane_mask) == line_main_lane_mask, "DONE", "EN")
            elseif rm_an then
                lc_an_status_str = "AGENT"
            end
            local lc_lt_status = credo.firmware_debug_cmd(slice, lane, ISC_DEBUG, ISC_DEBUG_LOCAL_LT_STATUS)
            local lc_lt_status_str = "OFF"
            if is_host then
                lc_lt_status_str = " - "
            elseif line_lt then
                lc_lt_status_str = choose((lc_lt_status & lc_side_lane_mask) == lc_side_lane_mask, "DONE", "EN")
            end

            local rm_link_status = credo.firmware_debug_cmd(slice, lane, ISC_DEBUG, ISC_DEBUG_REMOTE_LINK_STATUS)
            local rm_phy_ready = credo.firmware_debug_cmd(slice, lane, ISC_DEBUG, ISC_DEBUG_REMOTE_PHY_READY)
            local rm_side_phy_ready = choose((rm_phy_ready & rm_side_lane_mask) == rm_side_lane_mask, "YES", "NO")
            local rm_data_ready = credo.firmware_debug_cmd(slice, lane, ISC_DEBUG, ISC_DEBUG_REMOTE_DATA_READY)
            local rm_side_data_ready = choose((rm_data_ready & rm_side_lane_mask) == rm_side_lane_mask, "YES", "NO")
            local rm_an_status = credo.firmware_debug_cmd(slice, lane, ISC_DEBUG, ISC_DEBUG_REMOTE_AN_STATUS)
            local rm_an_status_str = "OFF"
            if is_host then
                rm_an_status_str = " - "
            elseif lc_an then
                rm_an_status_str = "AGENT"
            elseif rm_an then
                rm_an_status_str = choose((rm_an_status & rm_line_main_lane_mask) == rm_line_main_lane_mask, "DONE",
                                          "EN")
            end
            local rm_lt_status = credo.firmware_debug_cmd(slice, lane, ISC_DEBUG, ISC_DEBUG_REMOTE_LT_STATUS)
            local rm_lt_status_str = "OFF"
            if is_host then
                rm_lt_status_str = " - "
            elseif line_lt then
                rm_lt_status_str = choose((rm_lt_status & rm_side_lane_mask) == rm_side_lane_mask, "DONE", "EN")
            end
            if is_host then
                ftable:print("%d|host|%s" % {port, choose(host_up, "UP", "DOWN")})
            else
                ftable:print("|line|%s" % {choose(line_up, "UP", "DOWN")})
            end
            local lc_side_main_mask = choose(is_host, host_main_lane_mask, line_main_lane_mask)
            ftable:print("0x%04X (0x%04X)|%3s (0x%04X)|%3s (0x%04X)|%s (0x%X)|%s (0x%X)" % {
                lc_side_lane_mask, lc_side_main_mask, lc_side_phy_ready, lc_phy_ready, lc_side_data_ready,
                lc_data_ready, lc_an_status_str, lc_an_status, lc_lt_status_str, lc_lt_status
            })
            local rm_side_main_mask = choose(is_host, rm_host_main_lane_mask, rm_line_main_lane_mask)
            ftable:print("0x%04X (0x%04X)|0x%04X|%3s (0x%04X)|%3s (0x%04X)|%s (0x%X)|%s (0x%X)" % {
                rm_side_lane_mask, rm_side_main_mask, rm_link_status, rm_side_phy_ready, rm_phy_ready,
                rm_side_data_ready, rm_data_ready, rm_an_status_str, rm_an_status, rm_lt_status_str, rm_lt_status
            })
            ftable:ln()

        end
        ftable:add_separator()
        ::continue::
    end

    ftable:set_cell_span(1, 4, 5)
    ftable:set_cell_span(1, 9, 6)
    print(ftable)
end)

slash.register_chip_command({"isc", "partner"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display info about ISC partner.

    -p,--pair    (optional integer)    slice id of partner slice

]], function(slice, argt)

    if argt.pair then
        credo.slice_set_option(slice, "isc_slice_id", argt.pair)

        local ok = pcall(function()
            credo.slice_set_option(slice, "isc_slice_id", argt.pair)
        end)
        if not ok then
            print("Unable to call partner slice")
        end
        return
    end

    local remote_slice_id = credo.slice_get_option(slice, "isc_slice_id")

    if remote_slice_id == 0xFFFF then
        print("Slice is not partnered")
    else
        print("Local Slice %d <-> Remote Slice %d" % {slice, remote_slice_id})
    end
end)

slash.register_chip_command({"uart", "status"}, credo.FAMILY_SCREAMING_EAGLE, [[
Dump information about ISC UART status.

    <lanes>    (lanelist default all)

]], function(slice, argt)

    local ftable_top = fort.new()

    ftable_top:print_ln("Baudrate|%d Hz" %
                            {(credo.firmware_reg_rd_ex(slice, 32, 0) << 16) | credo.firmware_reg_rd_ex(slice, 33, 0)})
    ftable_top:print_ln("Cut Frame Time|%d us" %
                            {(credo.firmware_reg_rd_ex(slice, 34, 0) << 16) | credo.firmware_reg_rd_ex(slice, 35, 0)})
    ftable_top:print_ln("TX Interval Time|%d us" %
                            {(credo.firmware_reg_rd_ex(slice, 36, 0) << 16) | credo.firmware_reg_rd_ex(slice, 37, 0)})
    ftable_top:print_ln("RX Interval Time|%d us" %
                            {(credo.firmware_reg_rd_ex(slice, 38, 0) << 16) | credo.firmware_reg_rd_ex(slice, 39, 0)})
    print(ftable_top)

    local lanes = argt.lanes ---@type integer[]
    local ftable = fort.new()

    ftable:print("Status")
    ftable:row_write_ln(list.map(lanes, function(lane)
        return "%s (%d)" % {crutil.get_lane_id(slice, lane), lane}
    end))
    ftable:add_separator()

    local function dump_item(name, index)
        ftable:print(name)
        ftable:row_write_ln(list.map(lanes, function(lane)
            return credo.firmware_debug_cmd(slice, lane, 11, index)
        end))
    end
    local function dump_item_ex(name, index)
        ftable:print(name)
        ftable:row_write_ln(list.map(lanes, function(lane)
            local val1, val2 = credo.firmware_debug_cmd(slice, lane, 11, index),
                               credo.firmware_debug_cmd(slice, lane, 11, index + 1)
            return (val1 << 16) | val2
        end))
    end

    dump_item("intp_count", 1)
    dump_item("rx_valid_count", 2)
    dump_item("tx_valid_count", 3)
    dump_item("rx_frame_count", 4)
    dump_item("rx_error_count", 5)
    dump_item_ex("rx_frame_time", 6)
    dump_item_ex("tx_frame_time", 8)
    dump_item("tx_buffer_idx", 10)
    dump_item("rx_buffer_idx", 11)
    dump_item_ex("rx_intp_time", 14)
    dump_item_ex("tx_intp_time", 16)
    dump_item("rx_timeout_count", 18)
    dump_item("check_error_idx", 12)

    print(ftable)
    local ftable_errors = ftable.new()
    ftable_errors:print_ln("Lane|Captured Error Frames")
    ftable_errors:add_separator()
    for lane in iter(lanes) do
        ftable_errors:print("%s (%d)" % {crutil.get_lane_id(slice, lane), lane})
        ftable_errors:print(list.join(list.map(range(0, 9), function(i)
            return "%d 0x: %s" % {
                i, list.join(list.map(range(20 + 30 * i, 49 + 30 * i), function(byte_index)
                    local v1 = credo.firmware_debug_cmd(slice, lane, 11, byte_index)
                    return choose(byte_index % 10 == 9, "%02X " % {v1}, "%02X" % {v1})
                end), "")
            }
        end), "\n"))
        ftable_errors:ln()
        ftable_errors:add_separator()
    end
    print(ftable_errors)

end)
