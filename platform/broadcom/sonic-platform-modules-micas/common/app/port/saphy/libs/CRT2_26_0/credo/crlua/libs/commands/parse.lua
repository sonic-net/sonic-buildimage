local shell = require "shell"

local credo = require "credo"
local crutil = require "crutil"

local parse = {}

---@param raw string
---@return integer
function parse.int(raw)
    local parsed_int
    -- treat a/b prefix as indicating a lane integer
    if stringx.startswith(raw:lower(), {"a", "b"}) then
        local slice = shell.get_selected_slice()
        local lanelist = crutil.get_side_lanelist(slice, raw:sub(1, 1):lower())
        local lane_index = tonumber(raw:sub(2))
        assert(lane_index ~= nil, "Invalid lane id %s" % {raw})
        parsed_int = lanelist[lane_index + 1]
        assert(parsed_int ~= nil, "Invalid lane id index %s" % {raw})
    elseif stringx.startswith(raw, "0b") then
        parsed_int = tonumber(raw:sub(3), 2)
        assert(parsed_int ~= nil, "Invalid Integer:" .. raw)
    else
        parsed_int = tonumber(raw)
        assert(parsed_int ~= nil, "Invalid Integer:" .. raw)
    end

    assert(math.ceil(parsed_int) == parsed_int, "not an integer!")
    ---@cast parsed_int integer
    return parsed_int
end

---@param raw string
---@return string,string
function parse.keyval(raw)
    ---@type string|nil, string|nil
    local key, val = string.match(raw, "^([^=]+)=([^=]+)$")
    assert(key ~= nil or val ~= nil, "Invalid map: %s" % {raw})
    ---@cast key string assert check
    ---@cast val string assert check
    return key, val
end

---@param raw string
---@return table<string, string>
function parse.map(raw)
    local raw_list = stringx.split(raw, ",")
    ---@type table<string, string>
    local map = {}
    for raw_keyval in iter(raw_list) do
        local key, val = parse.keyval(raw_keyval)
        map[key] = val
    end
    return map
end

---@param raw string
---@return table<integer, integer>
function parse.intmap(raw)
    local raw_list = stringx.split(raw, ",")
    ---@type table<integer, integer>
    local intmap = {}
    for raw_keyval in iter(raw_list) do
        local key, val = parse.keyval(raw_keyval)
        local intkey = parse.int(key)
        local intval = parse.int(val)
        intmap[intkey] = intval
    end
    return intmap
end

---@param raw string
---@return table<integer, number>
function parse.int_to_num_map(raw)
    local raw_list = stringx.split(raw, ",")
    ---@type table<integer, number>
    local intmap = {}
    for raw_keyval in iter(raw_list) do
        local key, val = parse.keyval(raw_keyval)
        local intkey = parse.int(key)
        local numval = tonumber(val)

        assert(numval ~= nil, "Invalid number %s" % {val})
        intmap[intkey] = numval
    end
    return intmap
end

-- ---@param raw string
-- ---@return table<integer, string>
-- function parse.int_str_map(raw)
--     local map = parse.map(raw)
--     ---@type table<integer, string>
--     local int_str_map = {}
--     ---@type table<integer, string>
--     for k, v in pairs(map) do
--         int_str_map[parse.int(k)] = v
--     end
--     return int_str_map
-- end

---@param raw string
---@return boolean
local function is_lane_id(raw)
    return list.contains({"a", "A", "b", "B"}, string.sub(raw, 1, 1))
end

---@param raw string
---@return integer[]
function parse.intlist(raw)
    ---@type integer[]
    local reglist = {}
    ---@type string[]
    local reglist_raw = stringx.split(raw, ",")
    for _, regraw in ipairs(reglist_raw) do
        -- enable int list using hyphens
        local start, stop = string.match(regraw, "^(-?[^:-]+)-(-?[^:-]+)")
        if start ~= nil then
            local reg_start = parse.int(start)
            local reg_end = parse.int(stop)
            if is_lane_id(start) and not is_lane_id(stop) then
                reg_end = reg_end + parse.int(string.sub(start, 1, 1) .. "0")
            end
            local reg_step = 1
            if reg_start > reg_end then
                reg_step = -1
            end
            list.extend(reglist, list.range(reg_start, reg_end, reg_step))
            goto end_loop
        end
        local regrange = stringx.split(regraw, ":")
        if #regrange == 1 then
            list.append(reglist, parse.int(regraw))
        elseif #regrange == 2 then
            local reg_start = parse.int(regrange[1])
            local reg_end = parse.int(regrange[2])
            if is_lane_id(regrange[1]) and not is_lane_id(regrange[2]) then
                reg_end = reg_end + parse.int(string.sub(regrange[1], 1, 1) .. "0")
            end
            local reg_step = 1
            if reg_start > reg_end then
                reg_step = -1
            end
            list.extend(reglist, list.range(reg_start, reg_end, reg_step))
        elseif #regrange == 3 then
            local reg_start = parse.int(regrange[1])
            local reg_end = parse.int(regrange[2])
            if is_lane_id(regrange[1]) and not is_lane_id(regrange[2]) then
                reg_end = reg_end + parse.int(string.sub(regrange[1], 1, 1) .. "0")
            end
            local reg_step = parse.int(regrange[3])
            assert(math.abs(reg_end - reg_start) + 1 >= math.abs(reg_step), "Invalid range step size: " .. raw)
            list.extend(reglist, list.range(reg_start, reg_end, reg_step))
        else
            error("Invalid reglist" .. raw)
        end
        ::end_loop::
    end
    return reglist
end

---@param raw string
---@return number[]
function parse.numbers(raw)
    local numlist_raw = stringx.split(raw, ",")

    ---@type number[]
    local numbers = {}
    for num_raw in iter(numlist_raw) do
        local new_num = tonumber(num_raw)
        assert(new_num ~= nil, "Invalid Number: " % {num_raw})
        list.append(numbers, new_num)
    end

    return numbers
end

---@class IntSpan
---@field public start integer
---@field public stop integer

---@param raw string
---@return IntSpan
function parse.intspan(raw)
    ---@type IntSpan
    local int_range = {start = 0, stop = 0}
    setmetatable(int_range, {
        __tostring = function(self)
            if self.start == self.stop then
                return "%d" % {self.start}
            end
            return "%d:%d" % {self.start, self.stop}
        end,
        as_list = function(self)
            return list.range(self.start, self.stop)
        end
    })
    -- allow hypens in intspan
    local start, stop = string.match(raw, "^(-?[^:-]+)-(-?[^:-]+)")
    if start ~= nil then
        int_range.start = parse.int(start)
        int_range.stop = parse.int(stop)
    elseif stringx.count(raw, ":") == 1 then
        local raw_bits = stringx.split(raw, ":")
        int_range.start = parse.int(raw_bits[1])
        int_range.stop = parse.int(raw_bits[2])
    elseif stringx.count(raw, ":") == 0 then
        int_range.stop = parse.int(raw)
        int_range.start = parse.int(raw)
    else
        error("Invalid Integer Range: " .. raw)
    end
    if int_range.stop < int_range.start then
        local tmp = int_range.stop
        int_range.stop = int_range.start
        int_range.start = tmp
    end
    return int_range
end

---@class RegisterBits
---@field public msb integer
---@field public lsb integer

function parse.regbits(raw)
    ---@type RegisterBits
    local regfieldbits = {msb = 15, lsb = 0}

    setmetatable(regfieldbits, {
        __tostring = function(self)
            if self.msb == self.lsb then
                return "%d" % {self.msb}
            end
            return "%d:%d" % {self.msb, self.lsb}
        end
    })

    local intspan = parse.intspan(raw)
    regfieldbits.lsb = intspan.start
    regfieldbits.msb = intspan.stop

    assert(regfieldbits.msb < 16 and regfieldbits.lsb >= 0, "Invalid Register Field Bits:" .. raw)
    return regfieldbits
end

function parse.reg32bits(raw)
    ---@type RegisterBits
    local regfieldbits = {msb = 15, lsb = 0}

    setmetatable(regfieldbits, {
        __tostring = function(self)
            if self.msb == self.lsb then
                return "%d" % {self.msb}
            end
            return "%d:%d" % {self.msb, self.lsb}
        end
    })

    local intspan = parse.intspan(raw)
    regfieldbits.lsb = intspan.start
    regfieldbits.msb = intspan.stop

    assert(regfieldbits.msb < 32 and regfieldbits.lsb >= 0, "Invalid Register Field 32 Bits:" .. raw)
    return regfieldbits
end

---@param raw string
---@return IntSpan
function parse.lanespan(raw)
    local span = parse.intspan(raw)
    local slice = shell.get_selected_slice()
    local limits = credo.slice_get_limits(slice)
    assert(span.start >= 0 and span.stop < limits.max_lanes, "Invalid lane bounds: %s" % {raw})
    return span
end

---@param name string
---@param enum enum.IntEnumMeta
---@param enum_map table<string, any>
---@return fun(raw:string):enum.IntEnum
---@return string
function parse.enum_creator(name, enum, enum_map)
    local enum_desc = enum.name .. ": Enumeration (Choose One)\n" ..
                          stringx.join("\n", list.map(tablex.keys(enum_map), function(k)
            return "- " .. k
        end)) .. "\n"

    return function(raw)
        -- allow integer values
        local ok, res = pcall(parse.int, raw)
        if ok then
            return enum(res)
        end
        local map_key = string.lower(raw)
        if enum_map[map_key] == nil then
            local enum_literal = stringx.join("|", tablex.keys(enum_map))
            error("Invalid %s type: %s. Expected %s or integer" % {name, map_key, enum_literal})
        end
        return enum_map[map_key]
    end, enum_desc
end

---
---@param name string
---@param enum_map table<string, any>
---@return fun(raw:string):integer
function parse.enumflag_creator(name, enum, enum_map)
    return function(raw)
        -- allow integer values
        local flag_list = {raw}
        if stringx.count(raw, "|") > 0 then
            flag_list = stringx.split(raw, "|")
        end

        local val = enum(0)
        for flag in iter(flag_list) do
            local ok, res = pcall(parse.int, flag)
            if ok then
                val = val | enum(res)
            else
                local map_key = string.lower(flag)
                if enum_map[map_key] == nil then
                    local enum_literal = stringx.join("|", tablex.keys(enum_map))
                    error("Invalid %s type: %s. Expected %s or integer" % {name, map_key, enum_literal})
                end
                val = val | enum_map[map_key]
            end
        end
        return val
    end
end

parse.fec, parse.fec_desc = parse.enum_creator("fec", credo.FecType, {
    rs544 = credo.FEC_RS_544,
    rs528 = credo.FEC_RS_528,
    none = credo.FEC_NONE,
    firecode = credo.FEC_FIRE_CODE
})

parse.portlayer, parse.portlayer_desc = parse.enum_creator("portlayer", credo.PortMode, {
    serdes = credo.PMODE_SERDES,
    pcs = credo.PMODE_PCS,
    macsec = credo.PMODE_MACSEC,
    macsec_bypass = credo.PMODE_PCS
})

parse.portside, parse.portside_desc = parse.enum_creator("portside", credo.PortMode, {
    host = credo.SIDE_HOST,
    line = credo.SIDE_LINE,
    system = credo.SIDE_HOST,
    a = credo.SIDE_HOST,
    b = credo.SIDE_LINE
})

parse.portmode, parse.portmode_desc = parse.enum_creator("portmode", credo.PortType, {
    bitmux = credo.PORT_BITMUX,
    retimer = credo.PORT_RETIMER,
    gearbox = credo.PORT_GEARBOX,
    switchover = credo.PORT_SWITCHOVER_RETIMER
})

parse.lanemode, parse.lanemode_desc = parse.enum_creator("lanemode", credo.LaneMode, {
    an = credo.LMODE_AN,
    disabled = credo.LMODE_DISABLE,
    nrz = credo.LMODE_NRZ,
    off = credo.LMODE_OFF,
    pam4 = credo.LMODE_PAM4
})

parse.lbmode, parse.lbmode_desc = parse.enum_creator("lbmode", credo.LaneLoopbackMode, {
    disabled = credo.LB_DISABLED,
    ["tx_to_rx"] = credo.LB_TX_TO_RX,
    ["rx_to_tx"] = credo.LB_RX_TO_TX
})

parse.portflags = parse.enumflag_creator("portflags", credo.PortFlags, {
    line_optical = credo.PFLAG_LINE_SIDE_OPTICAL,
    line_anlt = credo.PFLAG_LINE_SIDE_ANLT,
    line_an = credo.PFLAG_LINE_SIDE_AN,
    line_lt = credo.PFLAG_LINE_SIDE_LT,
    sys_optical = credo.PFLAG_SYS_SIDE_OPTICAL,
    sys_anlt = credo.PFLAG_SYS_SIDE_ANLT,
    sys_lt = credo.PFLAG_SYS_SIDE_LT,
    autoneg_override = credo.PFLAG_AUTONEG_OVERRIDE,
    autoneg_disable = credo.PFLAG_AUTONEG_DISABLE,
    enable_double_crc = credo.PFLAG_ENABLE_DOUBLE_CRC,
    -- shorthand
    lopt = credo.PFLAG_LINE_SIDE_OPTICAL,
    lanlt = credo.PFLAG_LINE_SIDE_ANLT,
    lan = credo.PFLAG_LINE_SIDE_AN,
    llt = credo.PFLAG_LINE_SIDE_LT,
    sopt = credo.PFLAG_SYS_SIDE_OPTICAL,
    sanlt = credo.PFLAG_SYS_SIDE_ANLT,
    slt = credo.PFLAG_SYS_SIDE_LT,
    anegovr = credo.PFLAG_AUTONEG_OVERRIDE,
    anegdis = credo.PFLAG_AUTONEG_DISABLE,
    dblcrc = credo.PFLAG_ENABLE_DOUBLE_CRC
})

parse.init_type, parse.init_type_desc = parse.enum_creator("init", credo.SliceInitType, {
    full = credo.INIT_FULL,
    none = credo.INIT_NONE,
    no_fw = credo.INIT_NO_FIRMWARE,
    warm = credo.INIT_WARM
})

parse.param_index, parse.param_index_desc = parse.enum_creator("param_index", credo.ParamIndex, {
    lane = credo.PARAM_INDEX_LANE,
    port = credo.PARAM_INDEX_PORT,
    side = credo.PARAM_INDEX_SIDE,
    top = credo.PARAM_INDEX_TOP
})

parse.prbs_pattern, parse.prbs_patt_desc = parse.enum_creator("prbs_pattern", credo.PrbsPattern, {
    prbs7 = credo.PRBS7,
    prbs9 = credo.PRBS9,
    prbs11 = credo.PRBS11,
    prbs13 = credo.PRBS13,
    prbs15 = credo.PRBS15,
    prbs23 = credo.PRBS23,
    prbs31 = credo.PRBS31,
    unknown = credo.PRBS_UNKNOWN
})

parse.fec_error, parse.fec_error_desc = parse.enum_creator("fec_error", credo.FecErrorType, {
    bit = credo.FEC_ERROR_BIT,
    symbol = credo.FEC_ERROR_SYMBOL,
    frame = credo.FEC_ERROR_FRAME
})

parse.log_io, parse.log_io_desc = parse.enum_creator("log_io", credo.LogIO, {
    off = credo.LOG_IO_OFF,
    on = credo.LOG_IO_ON,
    READ = credo.LOG_IO_READ,
    WRITE = credo.LOG_IO_WRITE
})

parse.log_level, parse.log_level_desc = parse.enum_creator("log_io", credo.LogIO, {
    trace = credo.LOG_TRACE,
    debug = credo.LOG_DEBUG,
    error = credo.LOG_ERROR,
    warn = credo.LOG_WARN,
    info = credo.LOG_INFO
})

local prbs_directions = {"both", "rx", "tx"}

---@alias credo.PrbsDirection "'both'" |"'tx'"|"'rx'"

---@param raw string
---@return credo.PrbsDirection
function parse.prbs_direction(raw)
    if not list.contains(prbs_directions, raw) then
        error("Invalid PRBS direction  expected one of %s" % {tostring(prbs_directions)})
    end
    return raw
end

-- have lanes that are disabled not be shown by default
---@param slice integer
---@return integer[]
local function get_all_lanelist(slice)
    return list.extend(crutil.get_side_lanelist(slice, "a"), crutil.get_side_lanelist(slice, "b"))
end

---parse a lane list
---@param raw string
---@return integer[]
function parse.lanelist(raw)
    local slice = shell.get_selected_slice()
    raw = raw:lower()
    if raw == "all" then
        return get_all_lanelist(slice)
    elseif list.contains({"a*", "b*"}, raw:lower()) then
        return crutil.get_side_lanelist(slice, raw:sub(1, 1))
    end
    raw = stringx.replace(raw, "l", "")

    local intlist = parse.intlist(raw)
    local min, max = list.minmax(intlist)
    local limits = credo.slice_get_limits(slice)
    assert(min >= 0 and max < limits.max_lanes, "Invalid lanelist bounds: %s" % {raw})
    return intlist
end

---parse a lane list
---@param raw string
---@return integer[]|nil
function parse.indexlist(raw)
    if raw == "all" then
        return nil
    end
    local intlist = parse.intlist(raw)
    local min = list.minmax(intlist)
    assert(min >= 0)
    return intlist
end

---parse a lane
---@param raw string
---@return integer
function parse.lane(raw)
    local slice = shell.get_selected_slice()
    local limits = credo.slice_get_limits(slice)
    raw = raw:lower()
    raw = stringx.replace(raw, "l", "")
    local intval = parse.int(raw)
    assert(intval >= 0 and intval < limits.max_lanes, "Invalid lane bounds: %s" % {raw})
    return intval
end

---parse a port
---@param raw string
---@return integer
function parse.port(raw)
    raw = raw:lower()
    if raw == "auto" then
        return credo.PORT_AUTO_ASSIGN_ID.value
    end
    raw = stringx.replace(raw, "p", "")
    local slice = shell.get_selected_slice()
    local limits = credo.slice_get_limits(slice)
    local intval = parse.int(raw)
    assert(intval >= 0 and intval < limits.max_ports, "Invalid port bounds: %s" % {raw})
    return intval
end

---parse a port list
---@param raw string
---@return integer[]
function parse.portlist(raw)
    local slice = shell.get_selected_slice()
    local limits = credo.slice_get_limits(slice)
    raw = raw:lower()
    raw = stringx.replace(raw, "p", "") -- allow p0-p7 for readability if the user would like
    if raw == "all" then
        return list.range(0, limits.max_ports - 1)
    end
    local intlist = parse.intlist(raw)
    local min, max = list.minmax(intlist)
    assert(min >= 0 and max < limits.max_ports, "Invalid portlist bounds: %s" % {raw})
    return intlist
end

---parse a speed
---@param raw string
---@return integer
function parse.speed(raw)
    if stringx.endswith(raw, {"g", "G"}) then
        local val = tonumber(raw:sub(1, -2))
        assert(val ~= nil, "Invalid speed number: " .. raw)
        return int(val * 1000)
    elseif stringx.endswith(raw, {"m", "M"}) then
        local val = tonumber(raw:sub(1, -2))
        assert(val ~= nil, "Invalid speed number: " .. raw)
        return int(val)
    else
        error("Invalid speed, must use speed specifier G,g,M,m: " .. raw)
    end
end

---parse a speed float
---@param raw string
---@return number
function parse.speedf(raw)
    if stringx.endswith(raw, {"g", "G"}) then
        local val = tonumber(raw:sub(1, -2))
        assert(val ~= nil, "Invalid speed number: " .. raw)
        return val * 1e9
    elseif stringx.endswith(raw, {"m", "M"}) then
        local val = tonumber(raw:sub(1, -2))
        assert(val ~= nil, "Invalid speed number: " .. raw)
        return val * 1e6
    else
        error("Invalid speed, must use speed specifier G,g,M,m: " .. raw)
    end
end

---parse a decibel value
---@param raw string
---@return integer
function parse.decibel(raw)
    if stringx.endswith(raw, {"db", "dB"}) then
        return int(raw:sub(1, -3))
    else
        return parse.int(raw)
    end
end

---parse a time duration
---@param raw string
---@return number
function parse.time(raw)
    local time_dur
    if stringx.endswith(raw, {"ms"}) then
        time_dur = tonumber(raw:sub(1, -3)) / 1000
    elseif stringx.endswith(raw, {"us"}) then
        time_dur = tonumber(raw:sub(1, -3)) / 1000000
    elseif stringx.endswith(raw, {"sec"}) then
        time_dur = tonumber(raw:sub(1, -4))
    elseif stringx.endswith(raw, {"s"}) then
        time_dur = tonumber(raw:sub(1, -2))
    else
        error("Invalid time specifier, must use ms,us,sec,s: " .. raw)
    end
    assert(time_dur ~= nil, "Invalid number: %s" % {raw})
    assert(time_dur >= 0, "Time duration must be >= 0")
    return time_dur
end

return parse
