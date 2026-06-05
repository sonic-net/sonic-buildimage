local json = require "cjson"
local webbrowser = require "webbrowser"

---@module 'plotly'
local plotly = {}

plotly.cdn_main = "<script src='https://cdn.plot.ly/plotly-2.12.1.min.js'></script>"
plotly.header = ""
plotly.body = ""
plotly.id_count = 1
plotly.sleep_time = 1

---Get the header for running plotly
---@return string
function plotly.get_header()
    return plotly.cdn_main
end

---Converts a set of figures to an HTML string
---@param figures plotly.figure[]
---@return string
function plotly.tohtml(figures)
    -- Create header tags
    local header = "<head>\n" .. plotly.cdn_main .. "\n" .. plotly.header .. "\n" .. "\n</head>\n"

    -- Create body tags
    local plots = ""
    for _, fig in pairs(figures) do
        plots = plots .. fig:toplotstring()
    end

    return header .. "<body>\n" .. plots .. "</body>"
end

---Saves a set of figures to filename
---@param filename string
---@param figures table
function plotly.tofile(filename, figures)
    local html_str = plotly.tohtml(figures)
    local file = io.open(filename, "w")
    if file == nil then
        error("unable to open file")
        return
    end
    file:write(html_str)
    file:close()
end

---Shows a set of figures in the browser
---@param figures table
function plotly.show(figures)
    local filename = "_temp.html"
    plotly.tofile(filename, figures)
    webbrowser.open(filename)
    if filename == "_temp.html" then
        time.sleep(plotly.sleep_time)
        os.remove(filename)
    end
end

---@class plotly.figure
---@field public data table<string ,any>[]
---@field public layout table<string, any>
---@field public config table<string, any>
---@field public id string
local figure = {}

---Adding a trace for the figure. All options can be found here: https://plotly.com/javascript/reference/index/
---Easy to call like: figure:add_trace{x=x, y=y, ...}
---@param trace table
function figure:add_trace(trace)
    table.insert(self.data, trace)
end

--[[Adding a trace for the figure with shorthand for common options (similar to matlab or matplotlib).
All js options can be found here: https://plotly.com/javascript/reference/index/
]]
---@param trace table<string, any>
function figure:plot(trace)
    self:add_trace(trace)
end

---Updates the plotly figure layout (options can be seen here: https://plotly.com/javascript/reference/layout/)
---@param layout table<string, any>
function figure:update_layout(layout)
    for name, val in pairs(layout) do
        self.layout[name] = val
    end
end

---Updates the plotly figure config (options can be seen here: https://plotly.com/javascript/configuration-options/)
---@param config table<string, any>
function figure:update_config(config)
    for name, val in pairs(config) do
        self.config[name] = val
    end
end

---convert figure to html plot string
---@return string
function figure:toplotstring()
    -- Converting input
    local data_str = json.encode(self.data)
    local layout_str = json.encode(self.layout)
    local config_str = json.encode(self.config)
    -- Creating string
    local plot = [[
<div id='$id'></div>
<script type="text/javascript">
    (function() {
        var data = $data
        var layout = $layout
        var config = $config
        Plotly.newPlot('$id', data, layout, config);
    })();

</script>
    ]] % {id = self.id, data = data_str, layout = layout_str, config = config_str}
    return plot
end

---html string (including header) for figure
---@return string
function figure:tohtmlstring()
    -- Create header tags
    local header = "<head>\n" .. plotly.cdn_main .. "\n" .. plotly.header .. "\n</head>\n"

    -- Create body tags
    local plot = self:toplotstring()

    return header .. "<body>\n" .. plot .. "</body>"
end

---Saves the figure to an HTML file with *filename*
---@param filename string
function figure:tofile(filename)
    local html_str = self:tohtmlstring()
    local file = io.open(filename, "w")
    if file == nil then
        error("unable to write to file")
        return
    end
    file:write(html_str)
    file:close()
    return self
end

---Opens/shows the plot in the browser
function figure:show()
    local filename = "_temp.html"
    self:tofile(filename)
    webbrowser.open(filename)
    if filename == "_temp.html" then
        time.sleep(plotly.sleep_time)
        os.remove(filename)
    end
end

-- Assigning functions
---@param id? string
---@return plotly.figure
function plotly.figure(id)
    if id == nil then
        id = "plotly" .. plotly.id_count
        plotly.id_count = plotly.id_count + 1
    else
        id = "plotly" .. id
    end
    local fig = {data = {}, layout = {}, config = {}, id = id}
    setmetatable(fig, {__index = figure})
    return fig
end

--[[Adding a trace for the figure with shorthand for common options (similar to matlab or matplotlib).
All js options can be found here: https://plotly.com/javascript/reference/index/
Easy to call like: plotly.plot{x, y, ...}
Shorthand options:
| key | explanation |
| :----: | :---------: |
| *1* | x-values  |
| *2* | y-values   |
| *ls* | line-style (options: "-", ".", "--")  |
| *lw* | line-width (numeric value - default 2) |
| *ms* | marker-size (numeric value - default 2) |
| *c* or *color* | sets color of line and marker |
| *mode* | shorter mode forms (options: "m"="markers", "l"="lines", "m+l" or "l+m"="markers+lines") |
| *title* | sets/updates the title of the figure |
| *xlabel* | sets/updates the xlabel of the figure |
| *ylabel* | sets/updates the ylabel of the figure |
]]
---@param trace table
---@return plotly.figure
function plotly.plot(trace)
    local fig = plotly.figure()
    fig:plot(trace)
    return fig
end

return plotly
