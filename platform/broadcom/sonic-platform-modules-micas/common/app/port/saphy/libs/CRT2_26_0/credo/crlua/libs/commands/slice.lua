local slash = require "slash"
local credo = require "credo"
local fort = require "fort"
local shell = require "shell"

slash.register_command({"slice"}, [[
Display the slice tree or select slice by ids from the slice tree.

All slices selected must be of the same exact slice type. This is to ensure they all have the same possible slash
commands.

Arguments:

    <slice_list>    (optional intlist)    Set the selected slice(s)
    -a,--add                              Add slices (if slice_list is empty then it adds all slices)
    -r,--remove                           Remove slices

]], function(_, argt, _)
    -- print slice tree
    if argt.add then
        if argt.slice_list == nil then
            shell.select_slices(shell.get_slice_list())
        else
            shell.select_slices(list.extend(shell.get_selected_slices(), argt.slice_list))
        end
        return
    end

    if argt.remove then
        if argt.slice_list == nil then
            -- no-op
            print("No slices removed")
        else
            local filter_selected_slices = list.filter(shell.get_selected_slices(), function(value)
                return not list.contains(argt.slice_list, value)
            end)
            shell.select_slices(filter_selected_slices)
        end
        return
    end

    if argt.slice_list == nil then
        shell.print_device_tree()
        return
    end
    shell.select_slices(argt.slice_list)
end, slash.MULTISLICE)

slash.register_command({"slice", "reinit"}, [[
Reinitialize the slice.

Arguments:
    -f,--firmware    (optional string)    firmware to load if the initialization type needs it
    <init>           (init)               type of reinitialization to do

Examples:

    > slice reinit full -f fw.bin    # full reinit with firmware
    > slice reinit no_fw             # full reinit without firmware

]], function(slice, argt)
    credo.slice_reinit(slice, argt.init, argt.firmware)
end)

slash.register_command({"slice", "temp"}, [[
Display Slice temperature.

]], function(slice, argt)
    local temp = credo.slice_get_temperature(slice)
    print("%.1fC" % {temp})
end)

slash.register_command({"slice", "option"}, [[
Get/Set slice options.

If no arguments given, it displays a table of all options.

Arguments:

    <options...>        (optional string)     Option(s) to set/get
    -v,--value          (optional integer)    Value to set option(s)
    -d,--description                          Display Descriptions only

Examples:

    > slice option                 # display all slice options
    > lane option spare0 spare1    # select certain options
    > lane option spare0 -v 200    # set option

]], function(slice, argt)
    ---@type string[]
    local options = argt.options
    ---@type integer|nil
    local value = argt.value
    ---@type boolean
    local description_only = argt.description or false

    local function print_option_table(option_list)
        local ftable = fort.create_table()

        if description_only then
            fort.printf_ln(ftable, "Option|Description")
        else
            fort.printf_ln(ftable, "Option|Value")
        end

        fort.add_separator(ftable)
        ---@type credo.SliceOption
        for option_item in iter(option_list) do
            -- format description text so it is not too wide
            local description = stringx.join("\n", stringx.wrap(option_item.description, 60))
            if description_only then
                fort.printf_ln(ftable, "%s|%s", option_item.name, description)
                fort.add_separator(ftable)
            else
                local ok, val = pcall(function()
                    return credo.slice_get_option(slice, option_item.name)
                end)
                if ok then
                    fort.printf_ln(ftable, "%s|%d (0x%X)", option_item.name, val, val)
                else
                    fort.printf_ln(ftable, "%s|-", option_item.name, description)
                end
            end
        end
        print(fort.to_string(ftable))
    end

    if #options == 0 then
        -- dont allow setting all options at once
        if value ~= nil then
            error("Cannot set all slice options at once")
        end
        local option_list = credo.slice_get_option_list(slice)
        print_option_table(option_list)
    elseif value == nil then
        local option_list = credo.slice_get_option_list(slice)
        for option in iter(options) do
            if not credo.slice_is_option_supported(slice, option) then
                print("WARN: Invalid option %s" % {option})
            end
        end
        option_list = list.filter(option_list, function(option_item)
            return list.index(options, option_item.name) ~= nil
        end)
        print_option_table(option_list)
    else
        for option in iter(options) do
            if not credo.slice_is_option_supported(slice, option) then
                -- more strict if user is trying to set an invalid option
                error("Invalid option %s" % {option})
            end
        end
        for option in iter(options) do
            credo.slice_set_option(slice, option, value)
        end
    end
end)

slash.register_command({"slice", "setup"}, [[
Save or load a slice setup

Arguments:

    -s,--save                save slice setup
    <file>       (string)    file to save or load setup

Examples:

    > slice setup -s new_setup.txt    # save setup to file
    > slice setup  old_setup.txt      # load setup from file

]], function(slice, argt)
    if argt.save then
        credo.slice_save_setup(slice, argt.file)
    else
        credo.slice_load_setup(slice, argt.file)
    end
end)
