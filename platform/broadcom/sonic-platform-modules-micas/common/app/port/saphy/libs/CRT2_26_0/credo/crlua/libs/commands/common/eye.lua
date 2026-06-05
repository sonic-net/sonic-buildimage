local slash = require "slash"
local credo = require "credo"
local crutil = require "crutil"
local eyeplot = require "eyeplot"
local plotly = require "plotly"
local webbrowser = require "webbrowser"

local eye = {}

---@param family credo.Family
---@param quality_mapping {fast:number, normal: number, best: number}
---@param quality_mapping_timeout {fast:number, normal: number, best: number}
function eye.register_diagram(family, quality_mapping, quality_mapping_timeout)

    slash.register_chip_command({"eye", "diagram"}, family, [[
Display eye diagram.

Can do a desctructive eye which gives more information, but will cause errors in the signal.
Otherwise it uses a heuristic to determine the cut-off of when it should stop capturing an
eye to prevent degredation in performance.

Arguments:

    <lane>              (lane)                               Lane to run
    -q,--quality        (fast|normal|best default normal)    How fast to capture eye (quality vs speed)
    -d,--destructive                                         Perform a destructive eye
    -p,--plotly         (optional string)                    Generate plotly diagram instead of ascii (html filename)
    -o,--open                                                Open plotly html file
    -t,--timeout        (optional time)                      Timeout duration on no progress update
    -s,--silent                                              Dont display progress update
Examples:

    > eye diagram 0             # eye monitor lane 0
    > eye diagram -d 0         # eye monitor lane 0 destructive
    > eye diagram -q best 0    # eye monitor lane 0 quality=best
]], function(slice, argt)
        ---@type integer
        local lane = argt.lane
        ---@type integer
        local quality = quality_mapping[argt.quality]

        local timeout_duration = argt.timeout or quality_mapping_timeout[argt.quality]
        local lane_mode = credo.lane_get_mode(slice, lane)

        assert(crutil.is_lane_configured(lane_mode), "Lane must be configured into a mode")

        credo.eye_monitor_start(slice, lane, quality, choose(argt.destructive, credo.EYE_MONITOR_DESTRUCTIVE, 0))
        local progress = 0

        local start_time = time.monotonic()
        local last_progress_time = start_time
        print("Capturing eye diagram on lane %s (%d)" % {crutil.get_lane_id(slice, lane), lane})
        while progress < 100 do
            local new_progress = credo.eye_monitor_get_progress(slice, lane)
            if new_progress ~= progress then
                progress = new_progress
                last_progress_time = time.monotonic()
                if not argt.silent then
                    print("progress %d%%" % {progress})
                end

            elseif time.monotonic() > last_progress_time + timeout_duration then
                credo.eye_monitor_stop(slice, lane)
                error("Eye capture timeout")
            end
        end

        local vstep_side, hstep_side = credo.eye_monitor_get_range(slice, lane)
        local data, extent_mv = credo.eye_monitor_get_data(slice, lane)

        if argt.plotly then
            local eye_fig = eyeplot.diagram_plotly(vstep_side, hstep_side, extent_mv, data)
            eye_fig:update_layout{
                title = {
                    text = "Eye Diagram: Lane $lane_id ($lane)" %
                        {lane_id = crutil.get_lane_id(slice, lane), lane = lane}
                }
            }
            print("Writing to file: %s" % {argt.plotly})
            plotly.tofile(argt.plotly, {eye_fig})
            if argt.open then
                webbrowser.open(argt.plotly)
            end
        else
            print(eyeplot.diagram(vstep_side, hstep_side, extent_mv, data))
        end
        print("Capture Duration %.1fsec" % {time.monotonic() - start_time})
    end)
end

---@param family credo.Family
---@param quality_mapping {fast:number, normal: number, best: number}
---@param quality_mapping_timeout {fast:number, normal: number, best: number}
function eye.register_bathtub(family, quality_mapping, quality_mapping_timeout)
    slash.register_chip_command({"eye", "bathtub"}, family, [[
Display eye bathtub plot.

Arguments:

    <lane>         (lane)                              Lane to run
    -q,--quality   (fast|normal|best default normal)   Quality to capture bathtub
    -p,--plotly    (optional string)                   Generate plotly diagram instead of ascii (html filename)
    -o,--open                                          Open plotly html file
    -t,--timeout   (optional time)                     Custom timeout duration
    -s,--silent                                        Dont display progress update
]], function(slice, argt)
        ---@type integer
        local lane = argt.lane
        ---@type integer
        local quality = quality_mapping[argt.quality]

        local timeout_duration = argt.timeout or quality_mapping_timeout[argt.quality]
        local lane_mode = credo.lane_get_mode(slice, lane)

        assert(crutil.is_lane_configured(lane_mode), "Lane must be configured into a mode")

        credo.eye_monitor_start(slice, lane, quality, credo.EYE_MONITOR_BATHTUB)
        local progress = 0

        local start_time = time.monotonic()
        local last_progress_time = start_time
        while progress < 100 do
            local new_progress = credo.eye_monitor_get_progress(slice, lane)
            if new_progress ~= progress then
                progress = new_progress
                last_progress_time = time.monotonic()
                if not argt.silent then
                    print("progress %d%%" % {progress})
                end
            elseif time.monotonic() > last_progress_time + timeout_duration then
                credo.eye_monitor_stop(slice, lane)
                error("bathtub capture timeout")
            end
        end

        local vstep_side, hstep_side = credo.eye_monitor_get_range(slice, lane)

        local data, extent_mv = credo.eye_monitor_get_data(slice, lane)

        local bathtub_data = list.map(data, function(v)
            return v[1]
        end)
        if argt.plotly then
            local bathtub_fig = eyeplot.bathtub_plotly(vstep_side, hstep_side, extent_mv, bathtub_data)
            bathtub_fig:update_layout{
                title = {
                    text = "Bathtub Plot: Lane $lane_id ($lane)" %
                        {lane_id = crutil.get_lane_id(slice, lane), lane = lane}
                }
            }
            print("Writing to file: %s" % {argt.plotly})
            plotly.tofile(argt.plotly, {bathtub_fig})
            if argt.open then
                webbrowser.open(argt.plotly)
            end
        else
            print(eyeplot.bathtub(vstep_side, hstep_side, extent_mv, bathtub_data))
        end
        print("Capture Duration %.1fsec" % {time.monotonic() - start_time})
    end)
end

return eye
