local utils = require "environ.utils"
local core = require "environ.core"

---@module 'environ'
local env = {}

-- Note 1 about `*_win` functions.
-- This functions use WinAPI and ignores any CRT.
-- So it allows get variables set in differend module
-- even if this module use different or static CRT.
-- But it also means that `os.getenv` may returns different
-- result and some C mudules will use this value.
-- Also it means that use `setenv_win` is quite useless
-- because this value will be invisiable for any C mudule
-- unless it also uses WinAPI.
--
-- Note 2 about `*_win` functions.
-- Windows hase environment variables started with `=` e.g.
-- `=Exitcode`, `=C:`. CRT versions does not returns such values.
--
-- Note about expand function.
-- There exists `expand_win` but I think better use
-- custom function so it can be used same syntax on all
-- platforms.
--

---@param key string
---@return string|nil
function env.getenv(key)
    return core.get(key)
end

local expenv = utils.build_expand(env.getenv)

---@param str string
---@return string
function env.expand(str)
    return expenv(str)
end

---@param key string
---@param value string|nil nil indicates to unset
---@param expand? boolean
function env.setenv(key, value, expand)
    if value and expand then
        value = expenv(value)
    end
    return core.set(key, value)
end
local environ_ = core.environ

---get environment variable table
---@param upper? boolean uppercase keys
---@return table<string, string>
function env.environ(upper)
    assert(environ_, "environ not supported")

    local t, r = environ_(), {}

    for _, str in ipairs(t) do
        local k, v = utils.split_first(str, '=', true, 2)
        if upper then
            k = string.upper(k)
        end
        r[k] = v
    end

    return r
end

---Enumerate across all environment variable
---@return fun(): string, string
---@return table<string, string>
function env.enum()
    return next, env.environ()
end

---@class environ.ENV
---environment variable class that can allow modifying process variables without using setenv()/getenv()
---similar to python os.environ
env.ENV = utils.make_env_map(env)

return env
