local slash = require "slash"
local credo = require "credo"
local crutil = require "crutil"
local fort = require "fort"
local cjson = require "cjson"

local opt_mode_map = {[0] = "full", [1] = "bypass", [2] = "partial"}
local opt_mode_map_str = {full = 0, bypass = 1, partial = 2}

slash.register_chip_command({"opti", "info"}, credo.FAMILY_NUTCRACKER, [[
Display opti

Arguments:

    <lanes>      (lanelist default a*)    Lanes to display
]], function(slice, argt)
    ---@type integer[]
    local lanes = argt.lanes

    assert(math.max(table.unpack(lanes)) < 8, "Only host lanes supported")

    local ftable = fort.create()
    ftable:print_ln("Lane|Opti Mode")
    ftable:add_separator()

    for lane in iter(lanes) do
        ftable:print("%s (%2d)" % {crutil.get_lane_id(slice, lane), lane})

        local opti_mode = credo.lane_get_option(slice, lane, "opti_mode")
        ftable:print_ln("%s" % {opt_mode_map[opti_mode]})
    end
    print(ftable)
end)

slash.register_chip_command({"opti", "capture"}, credo.FAMILY_NUTCRACKER, [[
Display prbs training information

Arguments:

    <lanes>      (lanelist default a*)  Lanes to capture
    -f,--file    (optional file-out)    Store output to json file
]], function(slices, argt)

    ---@type integer[]
    slices = slices

    ---@type integer[]
    local lanes = argt.lanes

    assert(math.max(table.unpack(lanes)) < 8, "Only host lanes supported")

    local data = {}

    local ftable = fort.create()

    ftable:print_ln("Slice|Opti Capture Data")
    ftable:print("")
    for lane in iter(argt.lanes) do
        ftable:print("Ln %s (%2d)" % {crutil.get_lane_id(slices[1], lane), lane})
    end
    ftable:ln()
    ftable:add_separator()

    local lane_count = #argt.lanes
    if lane_count > 1 then
        ftable:set_cell_span(1, 2, lane_count)
    end

    for slice in iter(slices) do
        ftable:print("%d" % {slice})
        for lane in iter(argt.lanes) do
            local lane_mode = credo.lane_get_mode(slice, lane)
            if not crutil.is_lane_configured(lane_mode) then
                ftable:print("OFF")
                goto continue
            end
            local status = credo.prbs_training_rx_get_status(slice, lane)
            if status ~= credo.PRBS_TRAINING_LINKED then
                ftable:print("PRBS Training not finished")
                goto continue
            end
            local lane_data = {
                slice = slice,
                lane = lane,
                data = credo.data_capture(slice, "opti_capture", {lane = lane})
            }
            list.append(data, lane_data)
            ftable:print("%s" % {lane_data.data})
            ::continue::
        end
        ftable:ln()
        ftable:add_separator()
    end

    if argt.file == nil then
        print(ftable)
    else
        local json_data = cjson.encode(data)
        argt.file:write(json_data)
    end

end, slash.MULTISLICE)

slash.register_chip_command({"opti", "load"}, credo.FAMILY_NUTCRACKER, [[
Load optimization settings from json file.

Arguments:

    <file>       (file-in)     optimization load file
    <lanes>      (lanelist)    Lanes to load
    --opt       (full|bypass|partial default partial)    Specify opt mode
]], function(slices, argt)

    ---@type integer[]
    slices = slices

    ---@type integer[]
    local lanes = argt.lanes

    assert(math.max(table.unpack(lanes)) < 8, "Only host lanes supported")

    local data_str = argt.file:read("*all")
    local full_data = cjson.decode(data_str)

    local mapped_data = {}

    for lane_data in iter(full_data) do
        local slice = lane_data.slice
        local lane = lane_data.lane
        local data = lane_data.data

        assert(type(slice) == "number", "Invalid slice")
        assert(type(lane) == "number", "Invalid lane")
        assert(type(data) == "string", "Invalid data")

        if mapped_data[slice] == nil then
            mapped_data[slice] = {}
        end

        mapped_data[slice][lane] = data

    end

    for slice in iter(slices) do
        local slice_data = mapped_data[slice]
        if slice_data == nil then
            print("WARN: slice %d missing opti data. skipping." % {slice})
            goto next_slice
        end
        for lane in iter(lanes) do
            local lane_data = slice_data[lane]
            if lane_data == nil then
                print("WARN: slice %d, lane %d missing opti data. skipping.")
                goto next_lane
            end
            credo.lane_set_option(slice, lane, "opti_mode", opt_mode_map_str[argt.opt]) -- set to mode
            credo.slice_load_setup(slice, lane_data .. "\n")
            ::next_lane::
        end
        ::next_slice::
    end

end, slash.MULTISLICE)
