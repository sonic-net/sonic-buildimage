local slash = require "slash"
local credo = require "credo"
local regmap = require "regmap"
local intmod = require "int"
local crutil = require "crutil"
local pretty = require "pl.pretty"

local gDebug = false
local gTimer = false

local function get_clk_phase(slice, lane)
    local skpreg = regmap.slice(slice)
    return {
        gray_bin(skpreg.pam100.vdacclkphase1:read(slice, lane)),
        gray_bin(skpreg.pam100.vdacclkphase2:read(slice, lane)),
        gray_bin(skpreg.pam100.vdacclkphase0:read(slice, lane)),
        gray_bin(skpreg.pam100.vdacclkphase1_rx:read(slice, lane)),
        gray_bin(skpreg.pam100.vdacclkphase2_rx:read(slice, lane)),
        gray_bin(skpreg.pam100.vdacclkphase3:read(slice, lane)),
    }
end

local function set_rx_phase(slice, lane, idx, value)
    assert(0 <= idx and idx <= 3, "unknown index %d to set rx phase" % {idx})
    local skpreg = regmap.slice(slice)
    value = intmod.bin_gray(value)
    if idx == 0 then
        skpreg.pam100.vdacclkphase0:write(slice, value, lane)
    elseif idx == 1 then
        skpreg.pam100.vdacclkphase1_rx:write(slice, value, lane)
    elseif idx == 2 then
        skpreg.pam100.vdacclkphase2_rx:write(slice, value, lane)
    elseif idx == 3 then
        skpreg.pam100.vdacclkphase3:write(slice, value, lane)
    end
    time.sleep(0.00002)
end

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

local function scan_rx_clk_phase(slice, lane, iter_count, step, t, debug)
    local rx_phase123 = {table.unpack(get_clk_phase(slice, lane), 4, 6)}
    local ph = 2 -- start at phase2
    local best_ber = credo.prbs_get_rx_ber(slice, lane, t)
    local log_func = choose(debug, print, io.write)
    if debug then
        print("------------+------------------+----------+--------")
        print(" Lane %2d" % {lane})
        print(" %10s | %16s | %-8s | step" % {"interation", "rx_clk_phase", " ber"})
        print("------------+------------------+----------+--------")
    end
    log_func(" %10s | %3d , %3d , %3d  | %.2e |" % {"inital", rx_phase123[1], rx_phase123[2], rx_phase123[3], best_ber})

    for i = 1, iter_count do
        local best_phase = rx_phase123[ph]
        local step_count = 0
        for n = 1, 2 do -- positive, negative
            local sign = choose(n == 1, 1, -1)
            for j = 1, 6 do -- maximum 6 steps
                rx_phase123[ph] = rx_phase123[ph] + (sign * step)
                set_rx_phase(slice, lane, ph, rx_phase123[ph])
                step_count = j
                local ber_val = credo.prbs_get_rx_ber(slice, lane, t)

                local phase_str = ""
                for p=1, 3 do
                    if #phase_str > 0 then
                        phase_str = phase_str .. ", "
                    end
                    if p == ph then
                        phase_str = phase_str .. "["
                    end
                    phase_str = phase_str .. "%3d" % {rx_phase123[p]}
                    if p == ph then
                        phase_str = phase_str .. "]"
                    end
                end
                io.write("\r %10d | %16s | %.2e | %d" % { i, phase_str, ber_val, (sign * j)})
                if ber_val < best_ber then
                    best_ber = ber_val
                    best_phase = rx_phase123[ph]
                end
                if ber_val > (3 * best_ber) then
                    break
                end
            end
            if n == 2 then break end
            for j = 1, step_count do -- return to initial value
                rx_phase123[ph] = rx_phase123[ph] - (sign * step)
                set_rx_phase(slice, lane, ph, rx_phase123[ph])
            end
        end
        for val=rx_phase123[ph], best_phase do
            set_rx_phase(slice, lane, ph, val)
        end
        rx_phase123[ph] = best_phase

        local phase_str = ""
        for p=1, 3 do
            if #phase_str > 0 then
                phase_str = phase_str .. ", "
            end
            if p == ph then
                phase_str = phase_str .. "["
            end
            phase_str = phase_str .. "%3d" % {rx_phase123[p]}
            if p == ph then
                phase_str = phase_str .. "]"
            end
        end
        log_func("\r %10d | %16s | %.2e |              " % { i, phase_str, best_ber})
        ph = choose(ph == 2, 1, choose(ph == 1, 3, 2)) -- change phase
    end
    if debug then
        print("------------+------------------+----------+--------")
    end
    if true then
        rx_phase123 = {table.unpack(get_clk_phase(slice, lane), 4, 6)}
        local ber_val = credo.prbs_get_rx_ber(slice, lane, t)
        print("\rlane %d, final phases: %3d, %3d, %3d, ber %.2e" %
            {lane, rx_phase123[1], rx_phase123[2], rx_phase123[3], ber_val})
    end
end

slash.register_chip_command({"scan_rx_clk_phase"}, credo.FAMILY_SKIPPER_TEST, [[
Get top pll status.
Arugments:
    <lane>       (integer)                      lane number
    -c,--count   (optional integer default 10)  iteration count
    -s,--step    (optional integer default 2)   step of phase
    -t           (optional time default 0.5s)   ber calculate time
    -d,--debug                                  debug message
]], function(slice, argt)
    scan_rx_clk_phase(slice, argt.lane, argt.count, argt.step, argt.t, argt.debug)
end)

local function get_half_rate(slice, lane)
    local skpreg = regmap.slice(slice)
    if skpreg.pam100.reg_pam4_en:read(slice, lane) == 1 then
        return skpreg.pam100.reg_half_rate:read(slice, lane) == 1
    else
        return skpreg.nrz50.reg_halfrate_rx:read(slice, lane) == 1
    end
end

local function set_supercal(slice, lane, en)
    local skpreg = regmap.slice(slice)
    if en then
        skpreg.pam100.ow_mu_offset_q:write(slice, 3, lane)
        skpreg.pam100.owen_mu_offset_q:write(slice, 1, lane)
        skpreg.pam100.tosm_mu_offset_s6:write(slice, 1, lane)
        skpreg.pam100.tosm_mu_polarity:write(slice, 1, lane)
        local val0 = 0xdc1c -- phase margin counter
        local val1 = 0xe080 -- phase margin counter
        if get_half_rate(slice, lane) then
            val0 = 0xdbf0
            val1 = 0x0480
        end
        skpreg.pam100.cntr_sel0:write(slice, (val0 >> 13) & 0x7, lane)
        skpreg.pam100.cntr_sel1:write(slice, (val0 >> 10) & 0x7, lane)
        skpreg.pam100.cntr_sel2:write(slice, (val0 >> 7) & 0x7, lane)
        skpreg.pam100.cntr_sel3:write(slice, (val0 >> 4) & 0x7, lane)
        skpreg.pam100.cntr_sel4:write(slice, (val0 >> 1) & 0x7, lane)
        skpreg.pam100.cntr_sel5:write(slice, (val1 >> 13) & 0x7, lane)
        skpreg.pam100.cntr_sel6:write(slice, (val1 >> 10) & 0x7, lane)
        skpreg.pam100.cntr_sel7:write(slice, (val1 >> 7) & 0x7, lane)
    else
        skpreg.pam100.ow_mu_offset_q:write(slice, 0, lane)
        skpreg.pam100.owen_mu_offset_q:write(slice, 1, lane)
        skpreg.pam100.tosm_mu_offset_S6:write(slice, 0, lane)
    end
end

local function bp1(slice, lane, en, state)
    local skpreg = regmap.slice(slice)
    if skpreg.pam100.reg_pam4_en:read(slice, lane) == 1 then
        if state then
            skpreg.pam100.breakpoint_1_state:write(slice, state, lane)
        end
        if en then
            skpreg.pam100.breakpoint_1_en:write(slice, en, lane)
        end
    else
        if state then
            skpreg.nrz50.breakpoint_1_state:write(slice, state, lane)
        end
        if en then
            skpreg.nrz50.breakpoint_1_en:write(slice, en, lane)
        end
    end
end

local function sm_cont(slice, lane)
    local skpreg = regmap.slice(slice)
    skpreg.pam100.statemachine_continue:write(slice, 0, lane)
    time.sleep(0.001)
    skpreg.pam100.statemachine_continue:write(slice, 1, lane)
end

local function acal(slice, lane, dac_settle_time, timeout)
    dac_settle_time = choose(dac_settle_time, dac_settle_time, 7)
    timeout = choose(timeout, timeout, 0.06)
    local skpreg = regmap.slice(slice)
    skpreg.pam100.rg_adc_wait_sel_s:write(slice, dac_settle_time, lane)
    bp1(slice, lane, 1, 2)
    skpreg.pam100.sm_reset:write(slice, 1, lane)
    sm_cont(slice, lane)
    skpreg.pam100.sm_reset:write(slice, 0, lane)
    local start = time.time()
    while (time.time() - start) < timeout do
        if (gDebug and skpreg.pam100.adc_cal_done:read(slice, lane) == 1) or
           (not gDebug and skpreg.pam100.breakpoint_1_reached:read(slice, lane) == 1) then
            break
        end
    end
    local adc_cal_done = skpreg.pam100.adc_cal_done:read(slice, lane)
    if gDebug then
        timer_print("ADC_CAL_DONE %d time %.4fs" % {adc_cal_done, time.time() - start})
    else
        timer_print("bp1 state2 reach %d, time %.4fs"
            % {skpreg.pam100.breakpoint_1_reached:read(slice, lane), time.time() - start})
    end
    return adc_cal_done
end

local function read_sum(slice, lane, row)
    local skpreg = regmap.slice(slice)
    local dir_bits = 1
    -- local row_bits = 4
    local phase_bits = 1

    local diff_sum = 0
    for dir=0, 2^dir_bits-1 do
        for phase=0, 2^phase_bits-1 do
            skpreg.pam100.adr_comp:write(slice, (dir << 5) + (row << 2) + phase, lane)
            local calp_caln = skpreg.pam100.calp_caln:read(slice, lane)
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
    local skpreg = regmap.slice(slice)
    local params = {"cth", "ths0", "ths1", "ths2", "eth0"}
    local res = {}
    local lane_str = "Lane %d" % {lane}
    res[lane_str] = {}

    set_supercal(slice, lane, 0)
    skpreg.pam100.pu_adccal_vref2com:write(slice, 0, lane)
    debug_print("\n"..lane_str)
    for row=1, #params do
        local param = params[row]
        debug_print(param, crutil.twos_to_int(skpreg.pam100["offset_"..param]:read(slice, lane), 7))
    end
    for row=1, #params do
        local param = params[row]
        debug_print("\n    row",row,"offset_%s"%{param})
        debug_print("        Stg1 acal")
        skpreg.pam100["offset_"..param]:write(slice, crutil.int_to_twos(0, 7), lane)
        skpreg.pam100.pu_stg2cal:write(slice, 0, lane)
        acal(slice, lane)
        local target = read_sum(slice, lane, row)
        debug_print("        Target=", target)
        debug_print("\n        Stg2 acal")
        skpreg.pam100.pu_stg2cal:write(slice, 1, lane)
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
            skpreg.pam100["offset_"..param]:write(slice, crutil.int_to_twos(k, 7), lane)
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
    skpreg.pam100.pu_stg2cal:write(slice, 0, lane)
    bp1(slice, lane, 0)
    sm_cont(slice, lane)
    return res
end

slash.register_chip_command({"tune_acal"}, credo.FAMILY_SKIPPER_TEST, [[
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
