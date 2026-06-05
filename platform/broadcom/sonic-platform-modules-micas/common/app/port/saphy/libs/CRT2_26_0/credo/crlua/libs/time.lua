--- A simple implementation of time using a monotonic clock.
--- Do not use this for accurate dates. Only for measuring changes in time.
local _time = require "_time"

---@module 'time'
local time = {}

---Sleep for a duration of time
---@param secs number
function time.sleep(secs)
    _time.sleep(secs)
end

---Get the epoch time
---@return number
function time.time()
    return _time.time()
end

---Get the monotonic clock time (Not the epoch time)
---Use only for measuring changes in time, not dates.
---@return number
function time.monotonic()
    return _time.monotonic()
end

---Get the thread time
---Use only for measuring changes in time, not dates.
---@return number
function time.thread_time()
    return _time.thread_time()
end

---Get the process time
---Use only for measuring changes in time, not dates.
---@return number
function time.process_time()
    return _time.process_time()
end

return time
