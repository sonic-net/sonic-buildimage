local regmap = require "regmap"
local a2d = require "pl.array2d"

local sram = {}

---@param slice integer
---@param lane integer
local function dump_adc_output(slice, lane)
    local seregmap = regmap.slice(slice)

    local dbg_sram = seregmap.debug_sram_reg
    local rx_dig = seregmap.rx_dig_reg
    local ana = seregmap.ana_top_reg

    local dsp_source_sel = regmap.RegisterField("dsp_source_sel", {
        bases = range(0x1180, 0x9000, 0x800),
        RO = false,
        sign = false,
        addr = {0xE},
        width = {2},
        lsb = {14},
        reg_lsb = {0},
        parts = 1
    })

    local dsp_clear_sram = regmap.RegisterField("dsp_clear_sram", {
        bases = range(0x1180, 0x9000, 0x800),
        RO = false,
        sign = false,
        addr = {0xE},
        width = {1},
        lsb = {2},
        reg_lsb = {0},
        parts = 1
    })

    rx_dig.reg_phase_sel:write(slice, 0, lane)
    rx_dig.reg_adc_ds_sel:write(slice, 0, lane)
    rx_dig.reg_fec_512_sel:write(slice, 0, lane)
    dbg_sram.fec_clk_sel:write(slice, 1, lane)
    ana.fec_clkb_en:write(slice, 1, lane)
    dsp_clear_sram:write(slice, 1, lane)
    time.sleep(0.001)
    dsp_clear_sram:write(slice, 0, lane)
    dsp_source_sel:write(slice, 1, lane)

    dbg_sram.clear_trigger:write(slice, 1, lane)
    dbg_sram.set_mem_wren:write(slice, 1, lane)
    ana.fec_clkb_en:write(slice, 0, lane)
    dbg_sram.reg_mem2_wr_en:write(slice, 1, lane)
    dbg_sram.reg_mem1_wr_en:write(slice, 1, lane)
    ana.fec_clkb_en:write(slice, 1, lane)
    time.sleep(0.001)
    ana.fec_clkb_en:write(slice, 0, lane)
    dbg_sram.reg_mem2_wr_en:write(slice, 0, lane)
    dbg_sram.reg_mem1_wr_en:write(slice, 0, lane)
    ana.fec_clkb_en:write(slice, 1, lane)
end

---@param slice integer
---@param lane integer
---@return integer[][]
---@return integer[][]
local function sram_regbus_readout(slice, lane)
    local seregmap = regmap.slice(slice)
    local dbg_sram = seregmap.debug_sram_reg

    local dsp_source_sel = regmap.RegisterField("dsp_source_sel", {
        bases = range(0x1180, 0x9000, 0x800),
        RO = false,
        sign = false,
        parts = 1,
        addr = {0xE},
        width = {2},
        lsb = {14},
        reg_lsb = {0}
    })

    local dsp_dump_trigger = regmap.RegisterField("dsp_dump_trigger", {
        bases = range(0x1180, 0x9000, 0x800),
        RO = false,
        sign = false,
        parts = 1,
        addr = {0xE},
        width = {1},
        lsb = {0},
        reg_lsb = {0}
    })

    dbg_sram.fec_clk_sel:write(slice, 1, lane)
    dbg_sram.mem2_data_sel:write(slice, 0, lane)

    -- dbg_sram.dsp_cfg = 0x4001
    dsp_source_sel:write(slice, 1, lane)
    dsp_dump_trigger:write(slice, 1, lane)

    dbg_sram.read_sel:write(slice, 20, lane)
    local mem1_last_addr = dbg_sram.read_data:read(slice, lane)
    dbg_sram.read_sel:write(slice, 21, lane)
    local mem2_last_addr = dbg_sram.read_data:read(slice, lane)

    -- dbg_sram.dsp_cfg = 0x0000
    dsp_dump_trigger:write(slice, 0, lane)
    dsp_source_sel:write(slice, 0, lane)

    ---@type integer[][]
    local mem1_data = a2d.new(128, 16, 0)
    ---@type integer[][]
    local mem2_data = a2d.new(128, 16, 0)

    for i = 0, 127 do
        for m = 0, 7 do
            dbg_sram.mem1_raddr:write(slice, ((mem1_last_addr + i + 1) % 128 << 3) + m, lane)
            dbg_sram.mem2_raddr:write(slice, ((mem2_last_addr + i + 1) % 128 << 3) + m, lane)

            dbg_sram.read_sel:write(slice, 8, lane)
            local mem1_rdata_l = dbg_sram.read_data:read(slice, lane)
            dbg_sram.read_sel:write(slice, 9, lane)
            local mem1_rdata_h = dbg_sram.read_data:read(slice, lane)
            dbg_sram.read_sel:write(slice, 10, lane)
            local mem2_rdata_l = dbg_sram.read_data:read(slice, lane)
            dbg_sram.read_sel:write(slice, 11, lane)
            local mem2_rdata_h = dbg_sram.read_data:read(slice, lane)

            mem1_data[(127 - i) + 1][(15 - m * 2) + 1] = mem1_rdata_l
            mem1_data[(127 - i) + 1][(14 - m * 2) + 1] = mem1_rdata_h
            mem2_data[(127 - i) + 1][(15 - m * 2) + 1] = mem2_rdata_l
            mem2_data[(127 - i) + 1][(14 - m * 2) + 1] = mem2_rdata_h
        end
    end
    return mem1_data, mem2_data
end

---@param slice integer
---@param lane integer
---@param bytes integer?
---@param signed boolean?
---@return integer[]
local function dump_sram(slice, lane, bytes, signed)
    bytes = bytes or 1
    if signed == nil then
        signed = true
    end
    local mem1, mem2 = sram_regbus_readout(slice, lane)

    ---@type integer[]
    local dump = {}
    local frame_vals = 32 // bytes

    for i = 0, 127 do
        ---@type string[]
        local tmp_hs = {}
        ---@type string[]
        local tmp_ls = {}
        for j = 0, 15 do
            tmp_ls = list.append(tmp_ls, string.pack("I2", mem1[128 - i][16 - j]))
            tmp_hs = list.append(tmp_hs, string.pack("I2", mem2[128 - i][16 - j]))
        end
        local tmp_l = table.concat(tmp_ls)
        local tmp_h = table.concat(tmp_hs)

        local fmt = "%s%d" % {choose(signed, "i", "I"), bytes}
        for j = 0, frame_vals - 1 do
            ---@type integer
            local t1 = string.unpack(fmt, tmp_l, j * bytes + 1)
            list.append(dump, t1)
        end
        for j = 0, frame_vals - 1 do
            local t2 = string.unpack(fmt, tmp_h, j * bytes + 1)
            list.append(dump, t2)
        end
    end
    return dump
end

---@param slice integer
---@param lane integer
---@return integer[]
function sram.peek_adco(slice, lane)
    local seregmap = regmap.slice(slice)
    local dbg_sram = seregmap.debug_sram_reg

    dbg_sram.cnt_clr:write(slice, 1, lane)
    dump_adc_output(slice, lane)
    local data = dump_sram(slice, lane, 1)
    dbg_sram.cnt_clr:write(slice, 0, lane)
    return data
end

return sram
