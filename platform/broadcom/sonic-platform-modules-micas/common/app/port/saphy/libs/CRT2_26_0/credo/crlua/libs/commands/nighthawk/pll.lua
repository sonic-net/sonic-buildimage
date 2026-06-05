local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local crutil = require "crutil"
local regmap = require "regmap"

local SE_INFO = 15
local SE_INFO_TOP_PLL_VCOCAP_DEBUG = 200
local SE_INFO_TOP_PLL_LOCK_CLKGATE = 350
local SE_INFO_TOP_PLL_REFCLK_CNT_LO = 351
local SE_INFO_TOP_PLL_TARGET_MARGIN = 352
local SE_INFO_TOP_PLL_TARGET_CNT = 353

local function reftable_insert(tbl, key, content)
    table.insert(tbl, key)
    tbl[key] = content
end

local function get_top_pll_status(slice, vcocap_length)
    local sereg = regmap.slice(slice)
    local status = {}
    local target_marin = credo.firmware_debug_cmd(slice, 0, SE_INFO, SE_INFO_TOP_PLL_TARGET_MARGIN)
    local target_cnt = credo.firmware_debug_cmd(slice, 0, SE_INFO, SE_INFO_TOP_PLL_TARGET_CNT)
    local ret_val = credo.firmware_reg_rd(slice, 19)
    local lock_clkgate = credo.firmware_debug_cmd(slice, 0, SE_INFO, SE_INFO_TOP_PLL_LOCK_CLKGATE)
    local refclk_cnt_lo = credo.firmware_debug_cmd(slice, 0, SE_INFO, SE_INFO_TOP_PLL_REFCLK_CNT_LO)
    local vcocap_final = sereg.pll_fsyn_reg.lcvcocap:read(slice, 0)

    reftable_insert(status, "VCOCAP_FINAL", "%d" % {vcocap_final})
    reftable_insert(status, "RET_VAL", "0x%04X" % {ret_val})
    reftable_insert(status, "TARGET_MARGIN", "%d" % {target_marin})
    reftable_insert(status, "TARGET_CNT", "%d" % {target_cnt})
    reftable_insert(status, "LOCK_CLKGATE", "%d" % {lock_clkgate})
    reftable_insert(status, "REFCLK_CNT_LO", "0x%04X" % {refclk_cnt_lo})
    local dbg_status = {}
    for i = 0, vcocap_length - 1 do
        table.insert(dbg_status, credo.firmware_debug_cmd(slice, 0, SE_INFO, SE_INFO_TOP_PLL_VCOCAP_DEBUG + i))
    end
    return status, dbg_status
end

slash.register_chip_command({"pll", "top"}, credo.FAMILY_NIGHTHAWK, [[
Get top pll status.
Arugments:
    -l,--len     (optional integer)             vcocap length
    -d,--debug   (optional integer default 0)   debug level 0,1,2... to show more in table
]], function(slice, argt)
    local len = choose(argt.debug > 0, argt.len or 64, 0)
    local status, dbg_status = get_top_pll_status(slice, len)
    local ftable = fort.create_table()

    for name in iter(status) do
        ftable:print_ln("%s|%s" % {name, status[name]})
    end
    ftable:add_separator()
    if argt.debug > 0 then
        ftable:print("DEBUG")
        local target_margin = tonumber(status["TARGET_MARGIN"])
        local vcocap_final = tonumber(status["VCOCAP_FINAL"])
        local status_str = ""
        for idx, val in ipairs(dbg_status) do
            local flag = "   "
            if (idx ~= 1) and (idx % 8) == 1 then
                status_str = status_str .. "\n"
            end
            if 0 < val and val < target_margin then
                flag = choose((idx - 1) == vcocap_final, " S ", " L ")
            end
            status_str = status_str .. "%4d%s" % {(idx - 1), flag}
            if argt.debug > 1 then
                status_str = status_str .. "[%6d]" % {val}
            end
        end
        ftable:print_ln(status_str)
    end
    print(ftable)
end)

local OPT_DEBUG = 1
local OPT_DEBUG_VCOCAP_FINAL_RX = 350
local OPT_DEBUG_VCOCAP_FINAL_TX = 351
local OPT_DEBUG_VCOCAP_RET_RX = 352
local OPT_DEBUG_VCOCAP_RET_TX = 353
local OPT_DEBUG_VCOCAP_MIN_RX = 354
local OPT_DEBUG_VCOCAP_MIN_TX = 355
local OPT_DEBUG_VCOCAP_MARGIN_CNT_RX = 356
local OPT_DEBUG_VCOCAP_MARGIN_CNT_TX = 632
local OPT_DEBUG_VCOCAP_TRGT_CNT_RX = 357
local OPT_DEBUG_VCOCAP_TRGT_CNT_TX = 633
local OPT_DEBUG_VCOCAP_DEBUG_RX = 370
local OPT_DEBUG_VCOCAP_DEBUG_TX = 500
local OPT_DEBUG_VCOCAP_LOCK_CNT_RX = 630
local OPT_DEBUG_VCOCAP_LOCK_CNT_TX = 631
local OPT_DEBUG_PLL_SEARCH_RANGE_RX = 3010
local OPT_DEBUG_PLL_SEARCH_RANGE_TX = 3025

local function get_fw_lane_status(slice, lanes, indexes, ex)
    local status = {}
    if type(indexes) == "number" then
        indexes = {indexes}
    end
    if type(lanes) == "number" then
        lanes = {lanes}
    end

    for lane in iter(lanes) do
        local lane_status = {}
        for index in iter(indexes) do
            local value
            if ex then
                local lsb, msb = credo.firmware_debug_cmd_ex(slice, lane, OPT_DEBUG, index)
                value = msb << 16 | lsb
            else
                value = credo.firmware_debug_cmd(slice, lane, OPT_DEBUG, index)
            end
            table.insert(lane_status, value)
        end
        table.insert(status, lane_status)
    end
    return status
end

local function get_fw_lane_status_by_fmt(slice, lanes, index, fmt, ex)
    local status = get_fw_lane_status(slice, lanes, index, ex)
    return list.map(status, function(value_table)
        return list.map(value_table, function(value)
            if type(fmt) == "function" then
                return fmt(value)
            else
                return fmt % {value}
            end
        end)
    end)
end

local function get_lanes_pll_status(slice, lanes, direction, debug_len)
    local status = {}
    local function abs_twos_int(value)
        return math.abs(crutil.twos_to_int(value, 16))
    end
    local idx_tables = {
        ["RX"] = {
            ["VCOCAP_FINAL_RX"] = {OPT_DEBUG_VCOCAP_FINAL_RX, "%d"},
            ["RET_RX"] = {OPT_DEBUG_VCOCAP_RET_RX, "0x%04X"},
            ["MIN_RX"] = {OPT_DEBUG_VCOCAP_MIN_RX, "%d"},
            ["MARGIN_CNT_RX"] = {OPT_DEBUG_VCOCAP_MARGIN_CNT_RX, "%d"},
            ["TRGT_CNT_RX"] = {OPT_DEBUG_VCOCAP_TRGT_CNT_RX, "%d", 1}, -- fw_debug_cmd_ex
            ["LOCK_CNT_RX"] = {OPT_DEBUG_VCOCAP_LOCK_CNT_RX, "%d"},
            ["SEARCH_RANGE_RX"] = {{OPT_DEBUG_PLL_SEARCH_RANGE_RX, OPT_DEBUG_PLL_SEARCH_RANGE_RX + 1}, "%d"},
            ["DEBUG_RX"] = {range(OPT_DEBUG_VCOCAP_DEBUG_RX, OPT_DEBUG_VCOCAP_DEBUG_RX + debug_len - 1), abs_twos_int}
        },
        ["TX"] = {
            ["VCOCAP_FINAL_TX"] = {OPT_DEBUG_VCOCAP_FINAL_TX, "%d"},
            ["RET_TX"] = {OPT_DEBUG_VCOCAP_RET_TX, "0x%04X"},
            ["MIN_TX"] = {OPT_DEBUG_VCOCAP_MIN_TX, "%d"},
            ["MARGIN_CNT_TX"] = {OPT_DEBUG_VCOCAP_MARGIN_CNT_TX, "%d"},
            ["TRGT_CNT_TX"] = {OPT_DEBUG_VCOCAP_TRGT_CNT_TX, "%d", 1}, -- fw_debug_cmd_ex
            ["LOCK_CNT_TX"] = {OPT_DEBUG_VCOCAP_LOCK_CNT_TX, "%d"},
            ["SEARCH_RANGE_TX"] = {{OPT_DEBUG_PLL_SEARCH_RANGE_TX, OPT_DEBUG_PLL_SEARCH_RANGE_TX + 1}, "%d"},
            ["DEBUG_TX"] = {range(OPT_DEBUG_VCOCAP_DEBUG_TX, OPT_DEBUG_VCOCAP_DEBUG_TX + debug_len - 1), abs_twos_int}
        }
    }
    local idx_table = idx_tables[direction]
    if idx_table == nil then
        print("[Error] idx_table is nil")
    end
    local function reftable_insert_lanes(tbl, prefix_name)
        local name = prefix_name .. "_" .. direction
        local index = idx_table[name][1]
        local fmt = idx_table[name][2]
        local ex = idx_table[name][3]
        reftable_insert(tbl, name, get_fw_lane_status_by_fmt(slice, lanes, index, fmt, ex))
    end
    reftable_insert_lanes(status, "VCOCAP_FINAL")
    reftable_insert_lanes(status, "RET")
    reftable_insert_lanes(status, "MIN")
    reftable_insert_lanes(status, "MARGIN_CNT")
    reftable_insert_lanes(status, "TRGT_CNT")
    reftable_insert_lanes(status, "LOCK_CNT")
    local dbg_status = {}
    if debug_len > 0 then
        reftable_insert_lanes(dbg_status, "SEARCH_RANGE")
        reftable_insert_lanes(dbg_status, "DEBUG")
    end
    return status, dbg_status
end

slash.register_chip_command({"pll", "lane"}, credo.FAMILY_NIGHTHAWK, [[
Get vcocap status.

Arugments:

    <lanes>      (lanelist default all)
    -l,--len     (optional integer)              vcocap length
    -t,--tx                                      show tx
    -r,--rx                                      show rx
    -d,--debug   (optional integer default 0)    debug lever 0,1,2... to show more in table
]], function(slice, argt)
    local ftable = fort.create_table()
    local lanes = argt.lanes
    local len = choose(argt.debug > 0, argt.len or 72, 0)

    local function print_lanes_status(status, dbg_status, direction)
        ftable:print("Item/Lane")
        for lane in iter(lanes) do
            local lane_id = crutil.get_lane_id(slice, lane)
            ftable:print("%s(%2d)" % {lane_id, lane})
        end
        ftable:ln()
        ftable:add_separator()
        local function print_item(tbl, name)
            ftable:print("%s" % {name})
            for items in iter(tbl[name]) do
                local str = ""
                for value in iter(items) do
                    str = str .. choose(#str > 0, ", ", "") .. value
                end
                ftable:print("%s" % {str})
            end
            ftable:ln()
        end
        for name in iter(status) do
            print_item(status, name)
        end
        ftable:add_separator()
        if argt.debug > 0 then
            print_item(dbg_status, "SEARCH_RANGE_" .. direction)
            ftable:add_separator()
            local debug_lv = argt.debug
            local name = "DEBUG_" .. direction
            ftable:print(name)
            for lane, lane_status in ipairs(dbg_status[name]) do
                local margin_cnt = tonumber(status["MARGIN_CNT_" .. direction][lane][1])
                local vcocap_final = tonumber(status["VCOCAP_FINAL_" .. direction][lane][1])
                local start, final, sel = nil, nil, false
                local unmatch_count = 0
                local search_res = {}
                for idx, val in ipairs(lane_status) do
                    local flag = "   "
                    if 0 < val and val < margin_cnt then
                        start = start or idx
                        final = idx
                        sel = sel or (idx - 1) == vcocap_final
                        flag = choose((idx - 1) == vcocap_final, " S ", " L ")
                        unmatch_count = 0
                    else
                        unmatch_count = unmatch_count + 1
                        if unmatch_count >= 3 and sel == false then
                            start = nil
                            final = nil
                        end
                    end
                    search_res[idx] = {value = val, flag = flag}
                end
                -- print(search_res)
                local search_str = ""
                for idx, res in ipairs(search_res) do
                    if sel and debug_lv < 3 and ((idx < start - 3) or (idx > final + 3)) then
                        goto continue
                    end
                    search_str = search_str .. "%2d%s" % {(idx - 1), res.flag}
                    if sel == false or debug_lv >= 2 then
                        search_str = search_str .. "[%d]" % {(res.value & 0xFFFF)}
                    end
                    search_str = search_str .. "\n"
                    ::continue::
                end
                ftable:print(search_str)
            end
            ftable:ln()
            ftable:add_separator()
        end
    end

    local rx_en = choose(argt.tx == argt.rx, true, argt.rx)
    local tx_en = choose(argt.tx == argt.rx, true, argt.tx)

    if rx_en then
        local rx_status, rx_dbg_status = get_lanes_pll_status(slice, lanes, "RX", len)
        print_lanes_status(rx_status, rx_dbg_status, "RX")
    end
    if tx_en then
        local tx_status, tx_dbg_status = get_lanes_pll_status(slice, lanes, "TX", len)
        print_lanes_status(tx_status, tx_dbg_status, "TX")
    end

    print(ftable)
end)
