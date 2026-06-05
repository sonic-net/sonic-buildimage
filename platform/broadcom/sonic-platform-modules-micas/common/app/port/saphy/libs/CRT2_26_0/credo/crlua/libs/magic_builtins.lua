-- builtin magic commands
local magic = require "magic"
local path = require "pl.path"
local shell = require "shell"
local slash = require "slash"
local stats = require "stats"
local sio = require "pl.stringio"
local pprint = require "pprint"
local environ = require "environ"
local lapp = require "pl.lapp"
local shlex = require "shlex"
local libs = require "libs"
local pf = require "pl.file"
local dir = require "pl.dir"

---@param file string
---@return unknown
local function shell_do_file(file)
    local func, res = loadfile(file, nil, _G.SHELLENV)
    if func == nil then
        error(res)
    end
    return func()
end

magic.register("sh", [[
Run alternative shell commands
]], function(argt, command, argv)
    if #argv == 1 and (argv[1] == "-h" or argv[1] == "--help") then
        print(command.description)
        return
    end
    slash.run(stringx.join(" ", argv))
end)

magic.register("env", [[

<name>        (optional string)
-v,--value    (optional string)
-u,--unset
]], function(argt)
    if argt.name == nil then
        for k, v in environ.enum() do
            print("%s=%s" % {k, v})
        end
    elseif argt.unset then
        environ.setenv(argt.name, nil)
    elseif argt.value == nil then
        local val = environ.getenv(argt.name)
        if val ~= nil then
            print("%s=%s" % {argt.name, val})
        else
            print("%s unset" % {argt.name})
        end
    else
        environ.setenv(argt.name, argt.value)
    end
end)

magic.register("cd", [[
Change Directory.

    <dir> (optional string)
]], function(argt)
    local cd_dir = argt.dir or ""
    local ok, err = path.chdir(cd_dir)
    if not ok then
        error(err)
    end
end)

magic.register("sys", [[
Run system commands

Examples:

    - !ls
    - !pwd
]], function(argt, command, argv, argstr)
    if shell.in_server() then
        error("cannot use command through crcli")
    end
    if #argv == 1 and (argv[1] == "-h" or argv[1] == "--help") then
        print(command.description)
        return
    end
    -- add override for cd so that it works in current process
    if #argv >= 1 and argv[1] == "cd" then
        local ok, err = path.chdir(stringx.removeprefix(argstr, "sys cd "))
        if not ok then
            error(err)
        end
        return
    end
    local cmd = stringx.removeprefix(argstr, "sys ")

    os.execute(cmd)
end, true)

magic.register("find", [[
Find symbol in credo module
    <name> (string) Substring of symbol you want to find
]], function(argt)
    ---@type string
    local command = argt.name
    local found = false
    print("Search results for %s" % {command})
    command = string.lower(command)
    for k, v in pairs(credo) do
        if k:sub(1, 1) ~= "_" and string.find(k:lower(), command, 0, true) then
            if type(v) == "function" then
                print("- credo." .. k .. "()")
            elseif type(v) == "table" then
                print("- credo." .. k .. "{}")
            else
                print("- credo." .. k .. " = " .. inspect(v))
            end
            found = true
        end
    end
    if not found then
        print("None found")
    end
end)

magic.register("edit", [[
Edit and execute a lua file
    -x                          Do not execute the file
    <file> (optional string)    Name of the lua file to create. Defaults to a temp folder lua file.
]], function(argt)

    if shell.in_server() then
        error("cannot use command through crcli")
    end
    if path.is_windows then
        error("%edit not supported on Windows")
    end
    ---@type boolean
    local no_execute = argt.x
    local file = argt.file or path.tmpname() .. ".lua"
    local editor = os.getenv("EDITOR")
    if editor == nil then
        editor = "/bin/vi"
    end
    local status, error_code = os.execute(editor .. " " .. file)
    if not status then
        error("Editor Error Code: %d" % {error_code})
    end

    if not no_execute then
        shell_do_file(file)
    end
end)

magic.register("run", [[
Run a file
    <file>         (string)    Name of the lua file to run.
    -g,--global                Run in the global namespace instead of shell namespace
]], function(argt)
    local file = argt.file
    if argt.global then
        dofile(file)
    else
        shell_do_file(file)
    end

end)

local description = [[
Crlua Shell
-----------
An interactive Lua shell with bindings for the csdk + special features.
C structs, c enums, and c macros are accessible via the credo module: credo.{c-name}
CredoSlice_t* slice are accessed using the slice_id which searches availble crshell slices

# Magic Commands

Magic commands are very much the same as IPython magic commands. Simply put %{magic} to run
a magic command.

    > %lsmagic        - list available magic commands

To see a description of the command simply run:

    > %{magic} -h
    > %find -h        -- gives information about the find command

# Features

    > quit                          -- exit the lua interpreter
    > credo.prbs_get_rx_checker?    -- provides help information about a symbol
    > !ls                           -- run an external shell (bash) command
    > /commands                     -- run a lua cli command (also called slash commands)
]]

magic.register("stacktrace", [[
Get/Set stacktrace on errors.

Enabling provides more useful information for why a function failed.

Arguments:

    -e,--enable     enable stacktrace
    -d,--disable    disable stacktrace

]], function(argt)
    if not argt.enable and not argt.disable then
        print("Stacktrace is currently %s" % {choose(shell.stacktrace, "Enabled", "Disabled")})
    elseif argt.enable then
        shell.stacktrace = true
        print("Stacktrace Enabled")
    elseif argt.disable then
        shell.stacktrace = false
        print("Stacktrace Disabled")
    end
end)

magic.register("pprint", [[
Provide prettier print output using the inspect module

Arguments:

    -e,--enable
    -d,--disable
    -l,--level (optional integer) set the level of pretty printing nested tables

]], function(argt)
    if argt.level then
        pprint.level = argt.level
        print("pprint level set to %d" % {pprint.level})
        return
    end
    if not argt.enable and not argt.disable then
        print("pprint is currently %s" % {choose(pprint.is_installed(), "Enabled", "Disabled")})
    elseif argt.enable then
        pprint.install()
        print("pprint Enabled")
    elseif argt.disable then
        pprint.uninstall()
        print("pprint Disabled")
    end
end)

magic.register("desc", description, function(argt)
    print(description)
end)

magic.register("lsmagic", [[
Show available magic commands.
]], function(_)
    local command_list = ""
    for command in iter(list.sorted(magic.commands())) do
        command_list = command_list .. "  %" .. command
    end
    print("Line Magic:")
    print(stringx.lstrip(command_list) .. "\n")

    local cell_command_list = ""
    for command in iter(list.sorted(magic.cell_commands())) do
        cell_command_list = cell_command_list .. "  %%" .. command
    end
    print("Cell Magic:")
    print(stringx.lstrip(cell_command_list) .. "\n")
end)

local monotonic = time.monotonic

local function compute_monotonic_delay()
    local remove_monotonic = monotonic()
    remove_monotonic = monotonic() - remove_monotonic
    if remove_monotonic < 0 then
        remove_monotonic = 0
    end
    return remove_monotonic
end

local function timeit(test_func, n, r, once)
    -- compute monotonic delay
    local remove_monotonic = compute_monotonic_delay()
    ---@param loops integer
    ---@return number duration
    local function capture_runtime(loops)
        loops = loops or 1
        local start_time = monotonic()
        for _ = 1, loops do
            test_func()
        end
        return monotonic() - start_time - remove_monotonic
    end

    local function pprint_duration(duration)
        if duration < 5e-7 then
            return "%.2f nsec" % {duration * 1e9}
        elseif duration < 5e-4 then
            return "%.2f usec" % {duration * 1e6}
        elseif duration < 0.5 then
            return "%.2f msec" % {duration * 1e3}
        else
            return "%.2f sec" % {duration}
        end
    end

    ---@param runtime number
    ---@param std_devtime? number
    local function print_runtime(runtime, std_devtime)
        local output = sio.create()
        output:write(pprint_duration(runtime))
        if std_devtime ~= nil then
            output:write(" +/- " .. pprint_duration(std_devtime))
            output:write(" (mean +/- std. dev. of %d run(s), %d loop(s) each)" % {r, n})
        end
        print(output)
    end

    if once then
        local runtime = capture_runtime(1)
        n = 1
        r = 1
        print_runtime(runtime)
        return
    end

    if n == nil then
        local runtime = capture_runtime(1)
        if runtime < 0.005 then
            n = 500
        elseif runtime < 0.05 then
            n = 50
        elseif runtime < 0.1 then
            n = 20
        elseif runtime < 0.5 then
            n = 10
        elseif runtime < 1 then
            n = 3
        else
            n = 1
        end
    end

    assert(n > 0, "n must be > 0")
    assert(r > 0, "r must be > 0")

    local results = list.map(list.range(r), function()
        return capture_runtime(n)
    end)
    -- remove the loop count
    results = list.map(results, function(a)
        return a / n
    end)

    ---@type number
    local avg_runtime = stats.mean(results)
    local std_devtime = stats.stddev(results)
    print_runtime(avg_runtime, std_devtime)
end

magic.register("timeit", [[
Run a function w/ no arguments and measure its runtime.

Arguments:

    -f,--file                           Indicate func is a file
    -n           (optional integer)     How many times to run in a loop
    -r           (integer default 7)    How many repeats to do
    -o,--once                           Override everything else, run as oneshot
    <func>       (string)               The function name

]], function(argt)
    local test_func
    if argt.file then
        local file<close> = assert(io.open(argt.func, "r"))
        local code = file:read("*all") --[[@as string]]
        code = "return function()\n%s\nend" % {code}
        test_func = load(code, "=stdin", nil, _G.SHELLENV)()
    else
        test_func = _G[argt.func]
    end
    assert(type(test_func) ~= "nil", "Unable to find function: " .. argt.func)
    assert(type(test_func) == "function", "Must be a function")
    timeit(test_func, argt.n, argt.r, argt.once)
end)

magic.register("time", [[
Run a function w/ no arguments and measure its runtime.

* Underneath calls %timeit with the `-o` flag set

Arguments:

    -f,--file                           Indicate func is a file
    <func>       (string)               The function name
]], function(argt)
    if argt.file then
        magic("timeit -o -f %s" % {argt.func})
    else
        magic("timeit -o %s" % {argt.func})
    end
end)

magic.register_cell("time", [[
Time a block of code to run once.
]], function(argt, cell)
    ---@type string
    cell = "return function()\n%s\nend" % {cell}
    local func, err = load(cell, "=stdin", nil, _G.SHELLENV)
    if func == nil then
        error(err)
    end
    timeit(func(), 1, 1, true)
end)

magic.register_cell("timeit", [[
Run a block of code and measure its runtime.

Arguments:

    -n           (optional integer)     How many times to run in a loop
    -r           (integer default 7)    How many repeats to do
    -o,--once                           Override everything else, run as oneshot

]], function(argt, cell)
    ---@type string
    cell = "return function()\n%s\nend" % {cell}
    local func, err = load(cell, "=stdin", nil, _G.SHELLENV)
    if func == nil then
        error(err)
    end
    timeit(func(), argt.n, argt.r, argt.once)
end)

magic.register_cell("debug", [[
Debug a block of code.

]], function(argt, cell)
    ---@type string
    cell = [[
local dbg = require "debugger"
dbg()
%s
]] % {cell}

    if shell.in_server() then
        error("cannot use command through crcli")
    end
    local file_path = path.tmpname() .. ".lua"
    local file<close> = io.open(file_path, "w")
    assert(file ~= nil, "Unable to open file: " .. file_path)
    file:write(cell)
    file:close()
    shell_do_file(file_path)
end)

magic.register("pwd", [[
Print current Working Directory

]], function(argt)
    local pwd = path.currentdir()
    print(pwd)
end)

magic.register_cell("writefile", [[
Write a block of text to a file.

Arguments:

    <file>         (string)
    -a,--append               append to the end of a file

]], function(argt, cell)
    ---@type string
    local file_path = argt.file
    local file<close> = io.open(file_path, choose(argt.append, "a", "w"))
    assert(file ~= nil, "Unable to open file: " .. file_path)
    file:write(cell)
    file:close()
end)

magic.register_cell("profile", [[
Profile a block of code.

Arguments:

    <file>         (optional string)    file to save profile results
    -q,--quiet                          do not print out profile results
]], function(argt, cell)
    cell = [[
return function(file, quiet)
    local profiler = require "profiler"
    if quiet then
        profiler.attachPrintFunction(nil)
    else
        profiler.attachPrintFunction(print)
    end

    profiler.start()
    %s
    profiler.stop()
    profiler.report(file)
end]] % {cell}
    local func, err = load(cell, "=stdin", nil, _G.SHELLENV)
    if func == nil then
        error(err)
    end
    func()(argt.file, argt.quiet)
end)

magic.register("restart", [[
Restart the shell.
]], function(argt)
    print("Can only call using %restart")
end)

local function clear_screen()
    print('\027[2J') -- clear screen
    print('\027[1;1H') -- goto position 1,1
end

local live_description = [[
Live Display information

Arguments:
    -d,--duration    (time default 10s)     how long to display live information
    -r,--refresh     (time default 0.5s)    refresh rate (data capture time included in refresh)

Example:
    > %live /serdes param
    > %live -d 10s /serdes param
]]

magic.register("live", live_description, function(_argt, command, _argv, argstr)
    local command_split = stringx.split(argstr, "/", 2)

    local argv = shlex.split(command_split[1])
    local argt = lapp.process_options_string(live_description, list.slice(argv, 2))
    -- only stop on help
    if argt == nil then
        return
    end

    if #command_split ~= 2 then
        error("No slash command provided")
    end

    local start_time = time.monotonic()
    while time.monotonic() < start_time + argt.duration do
        local capture_time = time.monotonic()
        local data, trace = credo.slash_command_catch(shell.get_selected_slices(), command_split[2])
        if trace ~= nil then
            shell.write_error(trace)
            return
        end
        local data_len = #data
        if data_len < 2 then -- \n may be rendered
            print("Stopping: No data to render")
            return
        end

        -- heuristic for multi-slice that data is printed (slice id is printed on multi slice which shouldnt count)
        local slice_count = #shell.get_selected_slices()
        if slice_count > 1 and data_len < slice_count * 10 then
            local remove_id = stringx.strip(string.gsub(data, "Slice %d+", ""), "%s\n")
            if #remove_id == 0 then
                print("Stopping: No data to render")
                return
            end
        end

        clear_screen()
        print(data)
        capture_time = time.monotonic() - capture_time
        if capture_time < argt.refresh then
            time.sleep((argt.refresh - capture_time))
        end
    end

end, true)

magic.register_cell("live", [[
Live Display information

Arguments:
    -d,--duration    (time default 10s)     how long to display live information
    -r,--refresh     (time default 0.5s)    refresh rate (data capture time included in refresh)

Examples:

> %%live
>> /serdes param
>> /prbs monitor
>> %%

]], function(argt, cell)

    local start_time = time.monotonic()
    while time.monotonic() < start_time + argt.duration do
        local capture_time = time.monotonic()
        local data, trace = credo.slash_command_catch(shell.get_selected_slices(), cell)
        if trace ~= nil then
            shell.write_error(trace)
            return
        end

        local data_len = #data
        if data_len < 2 then -- \n may be rendered
            print("Stopping: No data to render")
            return
        end

        -- heuristic for multi-slice that data is printed (slice id is printed on multi slice which shouldnt count)
        local slice_count = #shell.get_selected_slices()
        if slice_count > 1 and data_len < slice_count * 10 then
            local remove_id = stringx.strip(string.gsub(data, "Slice %d+", ""), "%s\n")
            if #remove_id == 0 then
                print("Stopping: No data to render")
                return
            end
        end

        clear_screen()
        print(data)
        capture_time = time.monotonic() - capture_time
        if capture_time < argt.refresh then
            time.sleep(argt.refresh - capture_time)
        end
    end
end)

magic.register("unbundle", [[
Dump the bundled lua code into the filesystem.

This can be done to make debug the lua code and make tweaks.

Arguments:
    <path>          (string)
    -w,--write                           actually write to the file
    -m,--modules    (optional string)    path of submodules to dump. Default is all.
]], function(argt)

    ---@type string
    local module_path = argt.modules or ""
    ---@type string
    local dump_path = argt.path

    assert(not path.exists(dump_path), "path already exists: '%s'" % {dump_path})
    local module_names = libs.get_module_list()
    if module_path ~= "" then
        -- filter out modules that do not start with submodule path
        module_names = list.filter(module_names, function(mod)
            return mod == module_path or string.match(mod, "^" .. module_path .. "%..+$")
        end)
    end

    for mod in iter(module_names) do
        local mod_filename = stringx.replace(mod, ".", path.sep) .. ".lua"
        local mod_path = path.normpath(path.join(dump_path, mod_filename))
        local code = libs.get_module(mod)
        if code == nil then
            goto continue
        end
        if argt.write then
            local mod_dir = path.dirname(mod_path)
            dir.makepath(mod_dir)
            local out = pf.write(mod_path, code)
            print(out)
            print("Written: %s" % {mod_path})
        else
            print("Would Write: %s" % {mod_path})
        end
        ::continue::
    end
end)
