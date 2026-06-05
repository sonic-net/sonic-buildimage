local slash = require "slash"
local credo = require "credo"
local serdes = require "commands.common.serdes"
local zlib = require "zlib"
local base64 = require "base64"
local crutil = require "crutil"
local plotly = require "plotly"
local webbrowser = require "webbrowser"
local sram = require "chips.screaming_eagle.sram"

slash.register_chip_command({"serdes", "param"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display serdes paramter table.

Arguments:

    <lanes>      (lanelist default all)

Example:

    > serdes param           # dispaly all lanes
    > serdes param 0-2       # select lanes

]], function(slice, argt)
    local command = "serdes_param "
    credo.display_info_print(slice, "%s%s" % {command, list.join(argt.lanes, ",")})
end)

slash.register_chip_command({"serdes", "control"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display serdes control table.

]], function(slice, argt)
    credo.display_info_print(slice, "serdes_control")
end)

slash.register_chip_command({"serdes", "diag"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display serdes diagnostic information.

Arguments:

    <lanes>    (lanelist)

]], function(slice, argt)
    for lane in iter(argt.lanes) do
        credo.display_info_print(slice, "serdes_diag %d" % {lane})
    end
end)

serdes.register_tx_taps(credo.FAMILY_SCREAMING_EAGLE, {"pre3", "pre2", "pre1", "main", "post1", "post2", "post3"})

slash.register_chip_command({"serdes", "optlogs"}, credo.FAMILY_SCREAMING_EAGLE, [[
Capture SerDes Optimization logs.

Arguments:

    <lanes>       (lanelist)            lanes to capture optimization
    -f,--file     (optional file-out)   output to file
]], function(slice, argt)
    local DUMP_FW_LOG_BUFFER_LEN = 5001
    local DUMP_FW_LOG_RAM = 5100

    ---@type integer[]
    local lanes = argt.lanes

    ---@type file*|nil
    local file = argt.file

    for lane in iter(lanes) do
        local log_len = credo.firmware_debug_cmd(slice, lane, 1, DUMP_FW_LOG_BUFFER_LEN)
        ---@type string[]
        local log_ram = list.map(range(0, log_len - 1), function(i)
            local data = credo.firmware_debug_cmd(slice, lane, 1, i + DUMP_FW_LOG_RAM)
            return string.char(data & 0xFF, (data >> 8) & 0xFF)
        end)
        local output = table.concat(log_ram, "")
        output = zlib.compress(output)
        output = base64.encode(output)
        if file ~= nil then
            file:write("[slice %d][lane %d]\n" % {slice, lane})
            file:write(output)
        else
            print("[slice %d][lane %d]" % {slice, lane})
            print(output)
        end

    end

end)

---@param slice integer
---@param lanes integer[]
---@param data table<integer, number[]>
---@return plotly.figure
local function draw_isi(slice, lanes, data)
    local taps = range(-8, 58)

    local fig = plotly.figure('isi-plot')
    fig:update_layout({title = 'Slice %d ISI' % {slice}})
    for lane in iter(lanes) do
        local lane_data = data[lane]
        local lane_id = crutil.get_lane_id(slice, lane)
        fig:add_trace({type = 'scatter', x = taps, y = lane_data, name = "%s(%d)" % {lane_id, lane}})
    end
    return fig
end

slash.register_chip_command({"serdes", "isi"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display SerDes ISI plot.

Arguments:

    <lanes>      (lanelist)             lanes to capture isi
    -f,--file    (optional file-out)    file to put plot
]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes

    ---@type table<integer, number[]>
    local data = {}

    for lane in iter(lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        if not crutil.is_lane_configured(mode) then
            goto continue
        end
        local isi = credo.serdes_get_param(slice, 'rx_isi', lane) --[=[@as integer[]]=]
        local dfe_f0 = credo.serdes_get_param(slice, "rx_dfe_f0", lane) --[[@as integer]]
        -- local dtl_phase0 = credo.serdes_get_param(slice, "rx_dtl_phase0", lane)
        local isi_ph = list.map(isi, function(v)
            return 50 * v / dfe_f0
        end)

        data[lane] = isi_ph
        ::continue::
    end
    local fig = draw_isi(slice, lanes, data)
    local html = fig:tohtmlstring()
    if argt.file ~= nil then
        argt.file:write(html)
        if argt.open then
            webbrowser.open(argt.file_name)
        end
    else
        print(base64.encode(zlib.compress(html)))
    end
end)

local PHASE_NUM = 16
local ISI_NUM = 67

---@param slice integer
---@param lanes integer[]
---@param data table<integer, number[]>
---@return plotly.figure[]
local function draw_isi_phase(slice, lanes, data)
    local taps = range(-8, 58)

    ---@type plotly.figure[]
    local figs = {}

    for lane in iter(lanes) do
        local fig = plotly.figure('isi-plot-%d' % {lane})
        list.append(figs, fig)
        local lane_id = crutil.get_lane_id(slice, lane)
        fig:update_layout({title = 'Slice %d ISI Phase Lane %s(%d)' % {slice, lane_id, lane}})
        for phase in iter(range(0, PHASE_NUM - 1)) do
            local lane_data = list.slice(data[lane], (phase * ISI_NUM) + 1, (phase + 1) * ISI_NUM)
            fig:add_trace({type = 'scatter', x = taps, y = lane_data, name = "phase %d" % {phase}})
        end
    end
    return figs
end

slash.register_chip_command({"serdes", "isi_phase"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display SerDes ISI with phases plot.

Arguments:

    <lanes>      (lanelist)             lanes to capture isi
    -f,--file    (optional file-out)    file to put plot
    -o,--open                           open in webbrowser
]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes

    ---@type table<integer, number[]>
    local data = {}

    for lane in iter(lanes) do
        local mode = credo.lane_get_mode(slice, lane)
        if not crutil.is_lane_configured(mode) then
            goto continue
        end
        local isi = credo.serdes_get_param(slice, 'rx_isi_all', lane) --[=[@as integer[]]=]
        local dfe_f0 = credo.serdes_get_param(slice, "rx_dfe_f0", lane) --[[@as integer]]
        -- local dtl_phase0 = credo.serdes_get_param(slice, "rx_dtl_phase0", lane)
        local isi_ph = list.map(isi, function(v)
            return 50 * v / dfe_f0
        end)

        data[lane] = isi_ph
        ::continue::
    end
    local figs = draw_isi_phase(slice, lanes, data)
    local html = plotly.tohtml(figs)
    if argt.file ~= nil then
        argt.file:write(html)
        if argt.open then
            webbrowser.open(argt.file_name)
        end
    else
        print(base64.encode(zlib.compress(html)))
    end
end)

---@param slice integer
---@param lanes integer[]
---@param data table<integer, integer[]>
---@return plotly.figure
local function draw_adc_histogram(slice, lanes, data)
    local fig = plotly.figure('adc-plot')
    fig:update_layout({title = 'Slice %d ADC' % {slice}, barmode = "overlay"})

    for lane in iter(lanes) do
        local lane_data = data[lane]
        local lane_id = crutil.get_lane_id(slice, lane)
        fig:add_trace({
            type = 'histogram',
            x = lane_data,
            name = "%s(%d)" % {lane_id, lane},
            histnorm = "percent",
            xbins = {start = -64, ["end"] = 64, size = 1}
            -- autobinx = false
        })
    end
    return fig
end

---@param slice integer
---@param lanes integer[]
---@param data table<integer, integer[]>
---@return plotly.figure
local function draw_adc_scatter(slice, lanes, data)
    local fig = plotly.figure('adc-plot')
    fig:update_layout({title = 'Slice %d ADC' % {slice}})

    for lane in iter(lanes) do
        local lane_data = data[lane]
        local lane_id = crutil.get_lane_id(slice, lane)
        fig:add_trace({type = 'scatter', y = lane_data, mode = 'markers', name = "%s(%d)" % {lane_id, lane}})
    end
    return fig
end

slash.register_chip_command({"serdes", "adc"}, credo.FAMILY_SCREAMING_EAGLE, [[
Display ADC plot.

Arguments:

    <lanes>      (lanelist)             lanes to capture isi
    -f,--file    (optional file-out)    file to put plot
    -o,--open                           open in webbrowser
    --hist                              use histogram
]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes

    ---@type table<integer, integer[]>
    local data = {}
    local lane_count = #lanes
    for lane_idx, lane in ipairs(lanes) do
        print("Capturing Lane %d/%d" % {lane_idx, lane_count})
        local lane_data = sram.peek_adco(slice, lane)
        data[lane] = lane_data
    end
    local html
    if argt.hist then
        local fig = draw_adc_histogram(slice, lanes, data)
        html = fig:tohtmlstring()
    else
        local fig = draw_adc_scatter(slice, lanes, data)
        html = fig:tohtmlstring()
    end

    if argt.file ~= nil then
        ---@type file*
        local file = argt.file
        argt.file:write(html)
        file:flush()
        if argt.open then
            webbrowser.open(argt.file_name)
        end
    else
        print(base64.encode(zlib.compress(html)))
    end
end)
