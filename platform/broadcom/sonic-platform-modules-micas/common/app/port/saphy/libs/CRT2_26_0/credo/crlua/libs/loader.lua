do
    local _libs = require "_libs"
    local function load_module(module)
        -- remove .init in the search
        if module:sub(-5) == ".init" then
            module = module:sub(1, -6)
        end
        local chunkname = "=" .. module
        return _libs.load_module(module, chunkname)
    end
    table.insert(package.searchers, load_module)

    require "startup"
end
