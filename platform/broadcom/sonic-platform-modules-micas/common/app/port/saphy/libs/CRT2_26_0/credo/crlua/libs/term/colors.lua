-- Copyright (c) 2009 Rob Hoelz <rob@hoelzro.net>
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
-- THE SOFTWARE.

local pairs = pairs
local tostring = tostring
local setmetatable = setmetatable
local schar = string.char

local colors = {}

local colormt = {}

function colormt:__tostring()
    return self.value
end

function colormt:__concat(other)
    return tostring(self) .. tostring(other)
end

function colormt:__call(s)
    return self .. s .. colors.reset
end

local function makecolor(value)
    return setmetatable({ value = schar(27) .. '[' .. tostring(value) .. 'm' }, colormt)
end

local colorvalues = {
    -- attributes
    reset      = 0,
    clear      = 0,
    default    = 0,
    bright     = 1,
    dim        = 2,
    underscore = 4,
    blink      = 5,
    reverse    = 7,
    hidden     = 8,

    -- foreground
    black   = 30,
    red     = 31,
    green   = 32,
    yellow  = 33,
    blue    = 34,
    magenta = 35,
    cyan    = 36,
    white   = 37,

    -- background
    onblack   = 40,
    onred     = 41,
    ongreen   = 42,
    onyellow  = 43,
    onblue    = 44,
    onmagenta = 45,
    oncyan    = 46,
    onwhite   = 47,
}

for c, v in pairs(colorvalues) do
    colors[c] = makecolor(v)
end

return colors
