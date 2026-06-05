local lpeg = require "lpeg"

local IS_WINDOWS = (package.config:sub(1, 1) == '\\')

local D_WIN = function(s)
    return '%' .. s .. '%'
end
local D_PSX = function(s)
    return '${' .. s .. '}'
end
local D = IS_WINDOWS and D_WIN or D_PSX

local P, C, Cs, S, R = lpeg.P, lpeg.C, lpeg.Cs, lpeg.S, lpeg.R

local any = P(1)
local sym = R 'AZ' + R 'az' + R '09' + S '_'
local sym2 = any - S ':;${}%()'
local esc = (P '%%' / '%%') + (P '$$' / '$')
local var = (P '%' * C(sym2 ^ 1) * '%') + (P '${' * C(sym2 ^ 1) * '}') + (P '$' * C(sym ^ 1))

local function MakeSubPattern(fn)
    return Cs((esc + (var / fn) + any) ^ 0)
end

local function BuildExpand(getenv)
    local function subst(k)
        local v = getenv(k)
        if v then
            return v
        end
    end

    local pattern = MakeSubPattern(subst)

    return function(str)
        return pattern:match(str)
    end
end

local Normalize
do

    local psx_pattern = MakeSubPattern(D_PSX)
    local win_pattern = MakeSubPattern(D_WIN)
    local sys_pattern = MakeSubPattern(D)

    Normalize = function(str, mode)
        local pattern
        if mode == nil then
            pattern = sys_pattern
        elseif (mode == '$') or (mode == false) then
            pattern = psx_pattern
        elseif (mode == '%') or (mode == true) then
            pattern = win_pattern
        else
            error('unsupported mode')
        end
        return pattern:match(str)
    end

end

local function split_first(str, sep, plain, pos)
    local e, e2 = string.find(str, sep, pos, plain)
    if e then
        return string.sub(str, 1, e - 1), string.sub(str, e2 + 1)
    end
    return str
end

local function make_map(mod)
    local env = setmetatable({}, {
        __index = function(_, k)
            return mod.getenv(k)
        end,
        __newindex = function(_, k, v)
            return mod.setenv(k, v, true)
        end,
        __call = function(_, upper)
            return mod.environ(upper)
        end,
        __pairs = function()
            return next, mod.environ()
        end
    })
    return env
end

return {
    IS_WINDOWS = IS_WINDOWS,
    build_expand = BuildExpand,
    normalize = Normalize,
    split_first = split_first,
    make_env_map = make_map
}
