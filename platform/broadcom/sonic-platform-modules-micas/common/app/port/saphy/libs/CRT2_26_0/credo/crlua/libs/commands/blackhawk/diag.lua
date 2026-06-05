local slash = require "slash"
local credo = require "credo"
local webbrowser = require "webbrowser"
local template = require "resty.template"

slash.register_chip_command({"diag", "report"}, credo.FAMILY_BLACKHAWK, [[
Generate a diagnostic report for the slice to send to credo for debugging

Arguments:

    <file>       (file-out default bh-diag-report.html)
    -o,--open    open the report in your webbrowser once completed
]], function(slice, argt)

    print("Generating report")
    ---@type file*
    local file = argt.file
    ---@type string
    local file_name = argt.file_name

    -- capture data

    local report = template.new("bh_diag_report.html", "layout.html")

    print("Capturing `serdes param`")

    local serdes_param = credo.slash_command(slice, "serdes param")

    print("Capturing `serdes control`")
    local serdes_control = credo.slash_command(slice, "serdes control")

    print("Capturing `prbs monitor` (global)")
    local prbs_monitor = credo.slash_command(slice, "prbs monitor")

    print("Capturing `prbs monitor` (3 seconds)")
    local prbs_monitor_3s = credo.slash_command(slice, "prbs monitor -t 3s")

    print("Capturing `fecana histogram` (12 seconds)")
    local fecana_hist = credo.slash_command(slice, "fecana histogram -d 3s")

    print("Capturing `fw exit_codes`")

    local fw_exit_codes = credo.slash_command(slice, "fw exit_codes")

    print("Capturing `port info`")

    local port_info = credo.slash_command(slice, "port info")

    print("Capturing `port retimer status`")
    local retimer_status = credo.slash_command(slice, "port retimer status")

    print("Capturing `port gearbox status`")
    local gearbox_status = credo.slash_command(slice, "port gearbox status")

    print("Capturing `port bitmux status`")
    local bitmux_status = credo.slash_command(slice, "port bitmux status")

    local output = report.process {
        title = "Blackhawk Diagnostic Report",
        slice_id = slice,
        date = os.date("!%c UTC"),
        data = {
            {title = "SerDes Parameters", output = serdes_param}, {title = "SerDes Control", output = serdes_control},
            {title = "PRBS Monitor (Global)", output = prbs_monitor},
            {title = "PRBS Monitor (3s)", output = prbs_monitor_3s},
            {title = "FEC Analyzer Histogram", output = fecana_hist},
            {title = "Firmware Exit Codes", output = fw_exit_codes}, {title = "Port Info", output = port_info},
            {title = "Retimer Status", output = retimer_status}, {title = "GearBox Status", output = gearbox_status},
            {title = "Bitmux Status", output = bitmux_status}
        }
    }

    file:write(output)
    file:flush()

    print("Report finished: %s" % {file_name})
    if argt.open then
        webbrowser.open(file_name)
    end
end)
