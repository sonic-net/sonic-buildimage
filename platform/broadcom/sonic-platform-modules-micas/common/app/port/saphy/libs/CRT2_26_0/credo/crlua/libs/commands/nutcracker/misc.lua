local slash = require "slash"
local credo = require "credo"
local regmap = require "regmap"
local crutil = require "crutil"
local pretty = require "pl.pretty"

local gDebug = false
local gTimer = false

local function _print(f, lv, ...)
    if (type(f) == "boolean" and f == true) or
        (type(f) == "number" and f >= lv) then
        print(...)
    end
end

local function debug_print(...)
    _print(gDebug, 0, ...)
end

local function timer_print(...)
    _print(gTimer, 0, ...)
end

local function get_half_rate(slice, lane)
    local ncreg = regmap.slice(slice)
    if ncreg.pam100_v.reg_pam4_en:read(slice, lane) == 1 then
        return ncreg.pam100_v.reg_half_rate:read(slice, lane) == 1
    else
        return ncreg.nrz50.reg_halfrate_rx:read(slice, lane) == 1
    end
end

local function set_supercal(slice, lane, en)
    local ncreg = regmap.slice(slice)
    if en then
        ncreg.pam100_v.ow_mu_offset_q:write(slice, 3, lane)
        ncreg.pam100_v.owen_mu_offset_q:write(slice, 1, lane)
        ncreg.pam100_v.tosm_mu_offset_s6:write(slice, 1, lane)
        ncreg.pam100_v.tosm_mu_polarity:write(slice, 1, lane)
        local val0 = 0xdc1c -- phase margin counter
        local val1 = 0xe080 -- phase margin counter
        if get_half_rate(slice, lane) then
            val0 = 0xdbf0
            val1 = 0x0480
        end
        ncreg.pam100_v.cntr_sel0:write(slice, (val0 >> 13) & 0x7, lane)
        ncreg.pam100_v.cntr_sel1:write(slice, (val0 >> 10) & 0x7, lane)
        ncreg.pam100_v.cntr_sel2:write(slice, (val0 >> 7) & 0x7, lane)
        ncreg.pam100_v.cntr_sel3:write(slice, (val0 >> 4) & 0x7, lane)
        ncreg.pam100_v.cntr_sel4:write(slice, (val0 >> 1) & 0x7, lane)
        ncreg.pam100_v.cntr_sel5:write(slice, (val1 >> 13) & 0x7, lane)
        ncreg.pam100_v.cntr_sel6:write(slice, (val1 >> 10) & 0x7, lane)
        ncreg.pam100_v.cntr_sel7:write(slice, (val1 >> 7) & 0x7, lane)
    else
        ncreg.pam100_v.ow_mu_offset_q:write(slice, 0, lane)
        ncreg.pam100_v.owen_mu_offset_q:write(slice, 1, lane)
        ncreg.pam100_v.tosm_mu_offset_S6:write(slice, 0, lane)
    end
end

local function bp1(slice, lane, en, state)
    local ncreg = regmap.slice(slice)
    if ncreg.pam100_v.reg_pam4_en:read(slice, lane) == 1 then
        if state then
            ncreg.pam100_v.breakpoint_1_state:write(slice, state, lane)
        end
        if en then
            ncreg.pam100_v.breakpoint_1_en:write(slice, en, lane)
        end
    else
        if state then
            ncreg.nrz50.breakpoint_1_state:write(slice, state, lane)
        end
        if en then
            ncreg.nrz50.breakpoint_1_en:write(slice, en, lane)
        end
    end
end

local function sm_cont(slice, lane)
    local ncreg = regmap.slice(slice)
    ncreg.pam100_v.statemachine_continue:write(slice, 0, lane)
    time.sleep(0.001)
    ncreg.pam100_v.statemachine_continue:write(slice, 1, lane)
end

local function acal(slice, lane, dac_settle_time, timeout)
    dac_settle_time = choose(dac_settle_time, dac_settle_time, 7)
    timeout = choose(timeout, timeout, 0.06)
    local ncreg = regmap.slice(slice)
    ncreg.pam100_v.rg_adc_wait_sel_s:write(slice, dac_settle_time, lane)
    bp1(slice, lane, 1, 2)
    ncreg.pam100_v.sm_reset:write(slice, 1, lane)
    sm_cont(slice, lane)
    ncreg.pam100_v.sm_reset:write(slice, 0, lane)
    local start = time.time()
    while (time.time() - start) < timeout do
        if (gDebug and ncreg.pam100_v.adc_cal_done:read(slice, lane) == 1) or
           (not gDebug and ncreg.pam100_v.breakpoint_1_reached:read(slice, lane) == 1) then
            break
        end
    end
    local adc_cal_done = ncreg.pam100_v.adc_cal_done:read(slice, lane)
    if gDebug then
        timer_print("ADC_CAL_DONE %d time %.4fs" % {adc_cal_done, time.time() - start})
    else
        timer_print("bp1 state2 reach %d, time %.4fs"
            % {ncreg.pam100_v.breakpoint_1_reached:read(slice, lane), time.time() - start})
    end
    return adc_cal_done
end

local function read_sum(slice, lane, row)
    local ncreg = regmap.slice(slice)
    local dir_bits = 1
    -- local row_bits = 4
    local phase_bits = 1

    local diff_sum = 0
    for dir=0, 2^dir_bits-1 do
        for phase=0, 2^phase_bits-1 do
            ncreg.pam100_v.adr_comp:write(slice, (dir << 5) + (row << 2) + phase, lane)
            local calp_caln = ncreg.pam100_v.calp_caln:read(slice, lane)
            local calp = calp_caln >> 6
            local caln = calp_caln & 0x3F
            local diff = calp - caln
            diff_sum = diff_sum + diff
            debug_print("        dir->%d; row->%3d; phase->%2d; calp->%3d; caln->%3d; |diff|->%3d"
                % {dir,row,phase,calp,caln,diff})
        end
    end
    return diff_sum
end

local function tune_acal(slice, lane)
    local ncreg = regmap.slice(slice)
    local params = {"cth", "ths0", "ths1", "ths2", "eth0"}
    local res = {}
    local lane_str = "Lane %d" % {lane}
    res[lane_str] = {}

    set_supercal(slice, lane, 0)
    ncreg.pam100_v.pu_adccal_vref2com:write(slice, 0, lane)
    debug_print("\n"..lane_str)
    for row=1, #params do
        local param = params[row]
        debug_print(param, crutil.twos_to_int(ncreg.pam100_v["offset_"..param]:read(slice, lane), 7))
    end
    for row=1, #params do
        local param = params[row]
        debug_print("\n    row",row,"offset_%s"%{param})
        debug_print("        Stg1 acal")
        ncreg.pam100_v["offset_"..param]:write(slice, crutil.int_to_twos(0, 7), lane)
        ncreg.pam100_v.pu_stg2cal:write(slice, 0, lane)
        acal(slice, lane)
        local target = read_sum(slice, lane, row)
        debug_print("        Target=", target)
        debug_print("\n        Stg2 acal")
        ncreg.pam100_v.pu_stg2cal:write(slice, 1, lane)
        acal(slice, lane)
        local sum = read_sum(slice, lane, row)
        debug_print("        sum=",sum, "offset=",0)
        local margin = 6
        local k = 0
        local iter_count = 15
        for i=1, iter_count do
            if math.abs(sum - target) <= margin then
                break
            end
            local step = math.abs(sum - target) // 25
            step = choose(step == 0, 1, step)
            step = choose(sum > target, -step, step)
            k = k + step
            ncreg.pam100_v["offset_"..param]:write(slice, crutil.int_to_twos(k, 7), lane)
            time.sleep(0.00001)
            acal(slice, lane)
            sum = read_sum(slice, lane, row)
            debug_print("        sum=",sum, "offset=",k)
        end
        if math.abs(sum - target) <= margin then
            debug_print("        PASS %s"%{param}, " -  |Target-sum| < margin" , margin)
        else
            debug_print("\n        offset_%s" % {param},
                'timeout, sum',sum,'target',target, 'FAIL margin=', margin)
        end
        table.insert(res[lane_str], {param, k})
    end
    ncreg.pam100_v.pu_stg2cal:write(slice, 0, lane)
    bp1(slice, lane, 0)
    sm_cont(slice, lane)
    return res
end

local function read_acal(slice, lanes)
    local params = {"cth", "ths0", "ths1", "ths2", "eth0"}
    for lane in iter(lanes) do
        for row=1, #params do
            local sum = read_sum(slice, lane, row)
            print("    lane=", lane, "offset_" .. params[row], " sum=", sum)
        end
    end
end

slash.register_chip_command({"tune_acal"}, credo.FAMILY_NUTCRACKER, [[
Calibrate the offset_*['CTh','Ths0','Ths1','Ths2','ETh0'].
Arugments:
    <lane>       (integer)                      lane number
    -d,--debug                                  debug message
    -t,--timelog                                timer message
]], function(slice, argt)
    gDebug = argt.debug
    gTimer = argt.timelog
    local res = tune_acal(slice, argt.lane)
    pretty.dump(res)
    gDebug = false
    gTimer = false
end)

slash.register_chip_command({"read_acal"}, credo.FAMILY_NUTCRACKER, [[
Get the offset_*['CTh','Ths0','Ths1','Ths2','ETh0'].
Arugments:
    <lanes>        (lanelist default a*)        lanes to display isi
    -d,--debug                                  debug message
    -t,--timelog                                timer message
]], function(slice, argt)
    gDebug = argt.debug
    gTimer = argt.timelog
    read_acal(slice, argt.lanes)
    gDebug = false
    gTimer = false
end)
