local path = require "pl.path"

local datacap = {}

function datacap.lane_data_logs(slice, argt)
    local output = {}

    ---@param args table<string, any>
    local function data_capture(args)
        if argt.file ~= nil then
            credo.data_capture_file(slice, "lane_data", argt.file, args)
        else
            table.insert(output, credo.data_capture(slice, "lane_data", args))
        end
    end
    local header = argt.header
    --
    if argt.file ~= nil then
        local size = path.getsize(argt.file)
        if size == nil or size == 0 then
            header = true
        end
    end

    local args = {
        metadata = argt.metadata,
        serdes_control = argt.serdes_control,
        serdes_param = argt.serdes_param,
        isi = argt.isi,
        prbs = argt.prbs,
        prbs_duration = argt.prbs_duration,
        fecana = argt.fecana,
        fecana_duration = argt.fecana_duration,
        fecana_hist = argt.fecana_hist,
        fecana_hist_duration = argt.fecana_hist_duration,
        port_info = argt.port_info,
        bitmux = argt.bitmux,
        retimer = argt.retimer,
        exit_code = argt.exit_code
    }
    if header then
        data_capture(tablex.merge(args, {header = 1}, true))
    end

    if argt.ports ~= nil and #argt.ports > 0 then
        for port in iter(argt.ports) do
            local started, setup = credo.port_get_setup(slice, port)
            if started and setup ~= nil then
                for lane in iter(setup.host_lanes) do
                    data_capture(tablex.merge(args, {lane = lane}, true))
                end
                for lane in iter(setup.line_lanes) do
                    data_capture(tablex.merge(args, {lane = lane}, true))
                end
            end
        end
    else
        for lane in iter(argt.lanes) do
            data_capture(tablex.merge(args, {lane = lane}, true))
        end
    end

    if argt.file == nil then
        return table.concat(output)
    end
end

return datacap
