local time = require "time"
local itools = require "itertools"

local rsfec = {}
---Iterate through the rx pol, rx msblsb, rx precoder on the given port lanes until hopefully the FEC is able to align.
---It **should** be the rx polarity that should fix it, but we may as well be sure nothing weird is going on.
---@important If it does lock it does not necessarily mean these are the truly correct settings. The TX could be wrong.
---@param slice integer
---@param port integer
---@param side credo.Side
function rsfec.find_fec_align(slice, port, side)
    local port_config = credo.port_query(slice, port)

    assert(port_config.port_id ~= credo.PORT_UNCONFIGURED, "port is not configured")

    ---@type integer[]
    local lanes
    if side == credo.SIDE_HOST then
        lanes = range(port_config.host_start_lane, port_config.host_start_lane + port_config.host_no_of_lanes - 1)
    else
        lanes = range(port_config.line_start_lane, port_config.line_start_lane + port_config.line_no_of_lanes - 1)
    end

    local polmap_choices = {}
    for _ in iter(lanes) do
        list.append(polmap_choices, itools.values({0, 1}))
    end

    local initial_state = {pol = {}, msb = {}, pc = {}}

    for lane in iter(lanes) do
        initial_state.pol[lane] = credo.serdes_get_rx_polarity(slice, lane)
        initial_state.msb[lane] = credo.serdes_get_rx_msb(slice, lane)
        initial_state.pc[lane] = credo.serdes_get_rx_precoder(slice, lane)
    end

    for polarities in itools.product_table(table.unpack(polmap_choices)) do

        for idx, lane in ipairs(lanes) do
            credo.serdes_set_rx_polarity(slice, lane, polarities[idx])
        end
        for msb, pc in itools.product(itools.values({false, true}), itools.values({false, true})) do
            print("polarities:", polarities, "msblsb: %s" % {msb}, "precoder: %s" % {pc})
            for lane in iter(lanes) do
                credo.serdes_set_rx_msb(slice, lane, msb)
                credo.serdes_set_rx_precoder(slice, lane, pc)
            end
            time.sleep(0.5)
            local align = credo.rsfec_get_align_status(slice, port, side)
            print("aligned:", align.fec_aligned, "AM_locked", align.AM_locked)
            if align.fec_aligned then
                print("FEC is aligned!")
                return
            end
        end
    end
    print("FEC is not aligned")

    print("Restoring to initial state")
    for lane in iter(lanes) do
        credo.serdes_set_rx_polarity(slice, lane, initial_state.pol[lane])
        credo.serdes_set_rx_msb(slice, lane, initial_state.msb[lane])
        credo.serdes_set_rx_precoder(slice, lane, initial_state.pc[lane])
    end
end

return rsfec
