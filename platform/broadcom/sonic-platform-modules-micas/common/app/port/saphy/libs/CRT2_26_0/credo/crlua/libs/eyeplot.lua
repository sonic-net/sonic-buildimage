local sio = require "pl.stringio"
local plotly = require "plotly"

local eyeplot = {}

local legend = {"@", "%", "#", "*", "+", "=", "-", ":", ".", " "}

local function generate_x_axis(vstep_side, extent_mv)
    local axis = sio.create()

    local vstep_full = vstep_side * 2 + 1
    axis:write("%6s  +" % {""})
    axis:write(string.rep("-", vstep_full))

    local ticks = sio.create()

    for i = 0, 10 do
        local mv = extent_mv * 2 * (i - 5) // 10;
        local width = (vstep_full * (i + 1)) // 10 - math.max(0, (vstep_full * (i)) // 10)
        ticks:write(stringx.center("%d" % {mv}, width))
    end
    local tick_offset = math.max(0, 9 - vstep_full // 20)
    axis:write("\n", string.rep(" ", tick_offset), ticks:value())
    return axis:value()
end

---@param vstep_side integer
---@param hstep_side integer
---@param extent_mv number
---@param data number[][]
---@return string
function eyeplot.diagram(vstep_side, hstep_side, extent_mv, data)
    local plot = sio.create()
    for i = 1, #data[1] do
        local phase = i - hstep_side - 1
        if phase == 0 then
            plot:write("    0   |")
        elseif math.abs(phase) % (hstep_side // 4) == 0 then
            plot:write("%5.2f   |" % {phase * 0.0232})
        else
            plot:write("        |")
        end
        for margin = 1, #data do
            local ber_exp = data[margin][i]
            if ber_exp == 0 then
                ber_exp = 255
            end
            ber_exp = math.ceil(ber_exp / 16)
            ber_exp = math.max(1, ber_exp)
            ber_exp = math.min(10, ber_exp)
            plot:write(legend[ber_exp])
        end
        plot:write("\n")
    end
    plot:write(generate_x_axis(vstep_side, extent_mv), "\n\n")

    plot:write("     Legend: \"@%#*+=-:. \"\n")
    plot:write("     First character '@' means 1e-1, last character ' ' means 1e-10\n")

    return plot:value()
end

---@param vstep_side integer
---@param hstep_side integer
---@param extent_mv integer
---@param data integer[]
---@return string
function eyeplot.bathtub(vstep_side, hstep_side, extent_mv, data)
    local plot = sio.create()

    local vstep_full = vstep_side * 2 + 1
    for i = 1, 18 do
        if i % 2 == 1 then
            plot:write("  1e-%d  |" % {i // 2})
        else
            plot:write("%8s|" % {" "})
        end

        local lower = i * 8 + 4
        local upper = i * 8 - 4
        if i == 1 then
            upper = 0
        end
        for m = 1, vstep_full do
            plot:write(choose(data[m] < lower and data[m] >= upper, "x", " "))
        end
        plot:write("\n")
    end

    plot:write(generate_x_axis(vstep_side, extent_mv), "\n")

    return plot:value()
end

---@param vstep_side integer
---@param hstep_side integer
---@param extent_mv number
---@param data number[][]
---@return plotly.figure
function eyeplot.diagram_plotly(vstep_side, hstep_side, extent_mv, data)
    local x_axis = range(0, hstep_side * 2)
    x_axis = list.map(x_axis, function(val)
        return (val - hstep_side) * 0.0232
    end)
    local y_axis = range(0, vstep_side * 2)
    y_axis = list.map(y_axis, function(val)
        return (val - vstep_side) * extent_mv / vstep_side
    end)

    local fig = plotly.figure()
    fig:plot{
        z = data,
        x = x_axis,
        y = y_axis,
        type = "heatmap",
        colorscale = "Jet",
        reversescale = true,
        zmax = 128,
        zmin = 0
    }
    fig:update_layout{
        width = 800,
        height = 600,
        xaxis = {title = {text = "Time/Phase [UI]"}, side = "bottom", zeroline = false},
        yaxis = {title = {text = "Eye Height (mV)"}, side = "left"}
    }
    return fig
end

---@param vstep_side integer
---@param hstep_side integer
---@param extent_mv number
---@param data number[][]
---@return plotly.figure
function eyeplot.bathtub_plotly(vstep_side, hstep_side, extent_mv, data)
    local x_axis = range(0, vstep_side * 2)
    local y_data = list.map(data, function(v)
        if v == 255 then
            return 0
        end
        return 10 ^ (-1 * v / 16.0)
    end)
    x_axis = list.map(x_axis, function(val)
        return (val - vstep_side) * extent_mv / vstep_side
    end)
    local fig = plotly.figure()
    fig:plot{x = x_axis, y = y_data, type = "lines"}
    fig:update_layout{
        width = 800,
        height = 600,
        xaxis = {title = {text = "Eye Height (mV)"}, side = "bottom", zeroline = false},
        yaxis = {type = 'log', autorange = true, exponentformat = 'e', showexponent = 'all', title = {text = "BER"}}
    }

    return fig
end

return eyeplot
