local itools = require "itertools"

local prbs = {}

---Determine RX settings that would lock for PRBS.
---@important locks here does not mean it has found the "correct" settings. TX may need changes for general usage.
---@param slice integer
---@param lanes integer[]
---@param threshold? number default 1e-5
function prbs.find_rx_lock(slice, lanes, threshold)
    if threshold == nil then
        threshold = 1e-5
    end
    local polmap_choices = {}
    for _ in iter(lanes) do
        list.append(polmap_choices, itools.values({0, 1}))
    end
    local failed_lanes = {}
    for lane in iter(lanes) do
        print("Locking Lane %d" % {lane})

        local initial_state = {
            pol = credo.serdes_get_rx_polarity(slice, lane),
            msb = credo.serdes_get_rx_msb(slice, lane),
            pc = credo.serdes_get_rx_precoder(slice, lane)
        }

        for pol, msb, pc in itools.product(itools.values({0, 1}), itools.values({false, true}),
                                           itools.values({false, true})) do
            print("polarities:", pol, "msblsb: %s" % {msb}, "precoder: %s" % {pc})
            credo.serdes_set_rx_polarity(slice, lane, pol)
            credo.serdes_set_rx_msb(slice, lane, msb)
            credo.serdes_set_rx_precoder(slice, lane, pc)
            -- need to trigger an auto sync on the prbs
            credo.prbs_reset_rx_count(slice, lane)
            -- we could use rx_lock, but not all chips / lane modes support it
            local ber = credo.prbs_get_rx_ber(slice, lane, 0.1)
            if ber < threshold then
                print("Lane locked!")
                goto next_lane
            end
        end
        list.append(failed_lanes, lane)
        print("Failed to lock -- Restoring to initial state")
        credo.serdes_set_rx_polarity(slice, lane, initial_state.pol)
        credo.serdes_set_rx_msb(slice, lane, initial_state.msb)
        credo.serdes_set_rx_precoder(slice, lane, initial_state.pc)
        ::next_lane::
    end

    if #failed_lanes > 0 then
        pprint("Failed to lock on the lanes", failed_lanes)
        return
    end
    print("Successfully locked on all lanes")
end

return prbs
