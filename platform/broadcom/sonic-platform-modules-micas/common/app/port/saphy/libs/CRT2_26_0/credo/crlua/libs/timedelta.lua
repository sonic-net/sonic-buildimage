local timedelta = {}

local function divmod(a, b)
    local q = a // b
    local r = a % b
    return q, r
end

---Pretty print time delta as dd:hh:mm:ss.MMM?
---@param duration_sec number
---@param millisecond? boolean display milliseconds
function timedelta.pretty(duration_sec, millisecond)
    local sign_str = choose(duration_sec < 0, '-', '')
    local seconds = math.floor(math.abs(duration_sec))

    local days, hours, minutes

    days, seconds = divmod(seconds, 86400)
    hours, seconds = divmod(seconds, 3600)
    minutes, seconds = divmod(seconds, 60)

    local output = {}

    table.insert(output, sign_str)
    if days > 0 then
        table.insert(output, "%dd" % {days})
    end
    if days > 0 or hours > 0 then
        table.insert(output, "%02d:" % {hours})
    end
    table.insert(output, "%02d:%02d" % {minutes, seconds})
    if millisecond then
        local _, m = math.modf(duration_sec)
        table.insert(output, string.sub("%.3f" % {math.abs(m)}, 2))
    end
    return table.concat(output)
end

function timedelta.datetime_iso8601(timestamp)
    local base_time, milliseconds = math.modf(timestamp)
    return os.date("%Y-%m-%dT%H:%M:%S", base_time) .. string.sub(("%.3f") % {milliseconds}, 2) ..
               os.date("%z", base_time)
end

function timedelta.timestamp_iso8601(timestamp)
    local base_time, milliseconds = math.modf(timestamp)
    return os.date("%H:%M:%S", base_time) .. string.sub(("%.3f") % {milliseconds}, 2) .. os.date("%z", base_time)
end

return timedelta
