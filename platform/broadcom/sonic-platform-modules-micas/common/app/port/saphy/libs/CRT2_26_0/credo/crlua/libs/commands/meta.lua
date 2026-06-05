local slash = require "slash"
local inspect = require "inspect"

slash.register_command({"commands"}, [[
List Available Commands

Arugments:

    -v,--verbose                         show command descriptions
    <path...>       (optional string)    subpath of search

]], function(slice, argt)
    ---@type boolean
    local verbose = argt.verbose
    ---@type string[]
    local path = argt.path

    slash.print_possible_commands(path, verbose)
end)

slash.register_command({"run"}, [[
Run a slash command file

Arguments:

    <file>    (file-in)    File to run

]], function(slices, argt)
    ---@type file*
    local runfile = argt.file
    slash(runfile:read("a"))
end, slash.MULTISLICE)

slash.register_command({"type"}, [[
Get information about a shell type.

Arguments:

    <type>     (optional string)    Type to show information
    <value>    (optional string)    Validate the value

]], function(slice, argt)
    local shell_types = slash.get_all_types()
    if argt.type == nil then
        for shell_type in iter(list.sort(tablex.keys(shell_types))) do
            print("- " .. shell_type)
        end
        return
    end
    local shell_type = shell_types[argt.type]
    assert(shell_type ~= nil, "Invalid type name: " .. argt.type)

    if argt.value == nil then
        print(shell_type.description)
    else
        local ok, err = pcall(shell_type.parser, argt.value)
        if not ok then
            print(err)
        else
            print("Valid: " .. inspect(err))
        end
    end
end)

slash.register_command({"display"}, [[
Display slice information.

Uses cr_slice_display_info underneath.

]], function(slice, _, argv)
    credo.display_info_print(slice, stringx.join(" ", list.slice(argv, 2)))
end, slash.RAW)

slash.register_command({"sleep"}, [[
Sleep for a given amount of time

Arguments:
    <duration>    (time)
]], function(slices, argt)
    time.sleep(argt.duration)
end, slash.MULTISLICE) --  multi slice so it doesnt sleep for every slice separately
