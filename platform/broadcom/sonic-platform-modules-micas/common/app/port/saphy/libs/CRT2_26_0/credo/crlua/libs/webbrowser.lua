local compat = require "pl.compat"

---@module 'webbrowser'
local webbrowser = {}

-- From: https://stackoverflow.com/questions/11163748/open-web-browser-using-lua-in-a-vlc-extension#18864453
-- Attempts to open a given URL in the system default browser, regardless of Operating System.
local open_cmd = function(url)
    -- we will assume linux if check fails
    os.execute('xdg-open "%s"' % {url})
end -- this needs to stay outside the function, or it'll re-sniff every time...

---open webbrowser to url or file
---@param url string
function webbrowser.open(url)
    open_cmd(url)
end

pcall(function()
    if compat.is_windows then -- windows
        open_cmd = function(url)
            -- Should work on anything since (and including) win'95
            os.execute('start "%s"' % {url})
        end
    else -- that ought to only leave Linux
        open_cmd = function(url)
            -- should work on X-based distros.
            os.execute('xdg-open "%s"' % {url})
        end
    end
end)

return webbrowser
