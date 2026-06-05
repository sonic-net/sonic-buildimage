local fort = require "fort"

---@class tabulate
---@overload fun(data: tabulate.Data, options?: tabulate.Options): string
local tabulate = {}

---@diagnostic disable-next-line: param-type-mismatch
setmetatable(tabulate, tabulate)

---@alias tabulate.Data table<string, any>[]|table<string, any[]>|any[][]

---@alias tabulate.ColumnKey any

---@alias tabulate.Frame
--- |'basic'
--- |'basic2'
--- |'bold'
--- |'bold2'
--- |'dot'
--- |'double'
--- |'double2'
--- |'empty'
--- |'empty2'
--- |'frame'
--- |'nice'
--- |'plain'
--- |'simple'
--- |'solid_round'
--- |'solid'

---@alias tabulate.Align 'left'|'center'|'right'

---@alias tabulate.Formatter  any
---@alias tabulate.Wrapper fun (value: any, row: integer, col_name: tabulate.ColumnKey): string[]

---@class tabulate.Padding
---@field public top? integer
---@field public bottom? integer
---@field public left? integer
---@field public right? integer

---@alias tabulate.Margin tabulate.Padding

---@class tabulate.Options
---@field public column? tabulate.ColumnKey[] default to determine columns, but order of columns not guaranteed
---@field public header? table<tabulate.ColumnKey, string>
---@field public align? table<tabulate.ColumnKey, tabulate.Align>
---@field public row_separator? integer[]|integer
---@field public cell_span? table<integer, table<tabulate.ColumnKey, integer>>
---@field public frame? tabulate.Frame
---@field public margin? tabulate.Margin
---@field public col_padding? table<tabulate.ColumnKey, tabulate.Padding>
---@field public padding? tabulate.Padding
---@field public show_header? boolean  defaults to true
---@field public format? tabulate.Formatter
---@field public wrap? tabulate.Wrapper
---@field public footer? table<tabulate.ColumnKey, any>
---@field public footer_column? tabulate.ColumnKey[]
---@field public footer_span? table<tabulate.ColumnKey, integer>
---@field public footer_separator? boolean  defaults to true when footer enabled
---@field public footer_align? table<tabulate.ColumnKey, tabulate.Align>
---@field public sort? fun(row1: table<tabulate.ColumnKey, any>, row2: table<tabulate.ColumnKey, any>):boolean
---@field public filter? fun(row: table<tabulate.ColumnKey, any>):boolean

---@type table<tabulate.Frame, fort.BorderStyle>
local border_style_mapping = {
    basic = fort.BASIC_STYLE,
    basic2 = fort.BASIC2_STYLE,
    bold = fort.BOLD_STYLE,
    bold2 = fort.BOLD2_STYLE,
    dot = fort.DOT_STYLE,
    double = fort.DOUBLE_STYLE,
    double2 = fort.DOUBLE2_STYLE,
    empty = fort.EMPTY_STYLE,
    empty2 = fort.EMPTY2_STYLE,
    frame = fort.FRAME_STYLE,
    nice = fort.NICE_STYLE,
    plain = fort.PLAIN_STYLE,
    simple = fort.SIMPLE_STYLE,
    solid_round = fort.SOLID_ROUND_STYLE,
    solid = fort.SOLID_STYLE
}

---@type table<tabulate.Align, integer>
local text_align_mapping = {left = fort.ALIGNED_LEFT, center = fort.ALIGNED_CENTER, right = fort.ALIGNED_RIGHT}

---@generic IT
---@param data IT[]
---@return table<IT, integer>
local function indexify(data)
    local res = {}
    for i, val in ipairs(data) do
        res[val] = i
    end
    return res
end

---@generic HT
---@param data HT[]
---@return table<HT, true>
local function hashmapify(data)
    local res = {}
    for _, val in ipairs(data) do
        res[val] = true
    end
    return res
end

---@param data tabulate.Data
---@return boolean
local function is_list(data)
    local i = 0
    for _ in pairs(data) do
        i = i + 1
        if data[i] == nil then
            return false
        end
    end
    return true
end

---@param data table<tabulate.ColumnKey, any[]>
---@return table<tabulate.ColumnKey, any>[]
local function listdict_to_dictlist(data)
    local table_list = {}
    for col_name, col in pairs(data) do
        for row, value in ipairs(col) do
            if table_list[row] == nil then
                table_list[row] = {}
            end
            table_list[row][col_name] = value
        end
    end
    return table_list
end

---@param data table<tabulate.ColumnKey, any>[]
---@return string[]
local function get_column_keys(data)
    ---@type string[]
    local keys = {}
    local key_map = {}
    for _, row in ipairs(data) do
        for col_name, _ in pairs(row) do
            if key_map[col_name] == nil then
                key_map[col_name] = true
                table.insert(keys, col_name)
            end
        end
    end
    table.sort(keys, function(a, b)
        if type(a) == type(b) then
            return a < b
        end
        return tostring(a) < tostring(b)
    end)
    return keys
end

---@param data table<string, any>[]
---@return table<string, any>[]
local function shallow_list_copy(data)
    local copy = {}
    for k, v in ipairs(data) do
        copy[k] = v
    end
    return copy
end

---@generic FT
---@param data FT[]
---@param filter fun(val: any): boolean
---@return FT[]
local function filter_list(data, filter)
    local res = {}
    for _, v in ipairs(data) do
        if filter(v) then
            table.insert(res, v)
        end
    end
    return res
end

---@param table_data tabulate.Data
---@param options? tabulate.Options
---@return fort
function tabulate.tabulate(table_data, options)
    options = options or {}
    local ftable = fort.create()

    ---@type table<tabulate.ColumnKey, any>[]
    local data
    if is_list(table_data) then
        data = shallow_list_copy(table_data)
    else
        data = listdict_to_dictlist(table_data)
    end

    if options.sort then
        table.sort(data, options.sort)
    end
    if options.filter then
        data = filter_list(data, options.filter)
    end

    local column = options.column or get_column_keys(data)

    local base_row_separator = options.row_separator or {}
    local row_separator
    if type(base_row_separator) == "number" then
        row_separator = base_row_separator
        assert(row_separator > 0, "tabulate: row_separator must be > 0")
    elseif type(base_row_separator) == "table" then
        row_separator = hashmapify(base_row_separator)
    else
        error("Invalid row separator type:" .. type(base_row_separator))
    end

    local header_row_offset = 0
    local footer_row_offset = 0
    local show_header = options.show_header == nil or options.show_header
    -- add
    if options.footer then
        footer_row_offset = -1
    end
    if show_header then
        header_row_offset = 1
    end
    local column_index_map = indexify(column)
    local footer_index_map = indexify(options.footer_column or column)

    local header = options.header or {}

    if show_header then
        for _, col_name in ipairs(column) do
            local name = header[col_name] or col_name
            ftable:write(name)
        end
        ftable:ln()
        ftable:set_cell_prop(1, fort.ANY_COLUMN, fort.CPROP_ROW_TYPE, fort.ROW_HEADER)
    end

    -- add data to table
    for row_index, row in ipairs(data) do
        for _, col_name in ipairs(column) do
            local value = row[col_name]
            if options.format then
                if type(options.format) == "table" then
                    local col_formatter = options.format[col_name]
                    if col_formatter ~= nil then
                        value = col_formatter(value, row_index)
                    end
                elseif type(options.format) == "function" then
                    value = options.format(value, row_index, col_name)
                end

            end
            if options.wrap then
                -- allow for penlight text wrap or other func
                value = table.concat(options.wrap(value, row_index, col_name), "\n")
            end
            value = value or "" -- show empty for nil (user can use format to change this)
            ftable:write(value)
        end
        ftable:ln()
        if type(row_separator) == "number" then
            if row_index % row_separator == 0 then
                ftable:add_separator()
            end
        elseif row_separator[row_index] then
            ftable:add_separator()
        end

    end

    if options.footer then
        if options.footer_separator == nil or options.footer_separator then
            ftable:add_separator()
        end
        local footer_column = options.footer_column or column
        for _, col_name in ipairs(footer_column) do
            local value = options.footer[col_name]
            if options.format then
                -- pass -1 for footer as special index
                value = options.format(value, -1, col_name)
            end
            if options.wrap then
                -- allow for penlight text wrap or other func
                value = table.concat(options.wrap(value, -1, col_name), "\n")
            end
            value = value or "" -- show empty for nil (user can use format to change this)
            ftable:write(value)
        end
        ftable:ln()
    end

    -- add table margins
    if options.margin then
        if options.margin.top then
            ftable:set_tbl_prop(fort.TPROP_TOP_MARGIN, options.margin.top)
        end
        if options.margin.left then
            ftable:set_tbl_prop(fort.TPROP_LEFT_MARGIN, options.margin.left)
        end
        if options.margin.bottom then
            ftable:set_tbl_prop(fort.TPROP_BOTTOM_MARGIN, options.margin.bottom)
        end
        if options.margin.right then
            ftable:set_tbl_prop(fort.TPROP_RIGHT_MARGIN, options.margin.right)
        end
    end

    --- set text alignment
    if options.align then
        for col_name, text_align in pairs(options.align) do
            assert(text_align_mapping[text_align], "tabulate: invalid text align:" .. tostring(text_align))
            ftable:set_cell_prop(fort.ANY_ROW, column_index_map[col_name], fort.CPROP_TEXT_ALIGN,
                                 text_align_mapping[text_align])
        end
    end

    --- set footer alignment
    if options.footer_align then
        for col_name, text_align in pairs(options.footer_align) do
            assert(text_align_mapping[text_align], "tabulate: invalid footer align:" .. tostring(text_align))
            ftable:set_cell_prop(-1, footer_index_map[col_name], fort.CPROP_TEXT_ALIGN, text_align_mapping[text_align])
        end
    end

    -- set generic padding
    local tbl_padding = options.padding or {}
    if tbl_padding.top ~= nil then
        ftable:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_TOP_PADDING, tbl_padding.top)
    end
    if tbl_padding.left ~= nil then
        ftable:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_LEFT_PADDING, tbl_padding.left)
    end
    if tbl_padding.bottom ~= nil then
        ftable:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_BOTTOM_PADDING, tbl_padding.bottom)
    end
    if tbl_padding.right ~= nil then
        ftable:set_cell_prop(fort.ANY_ROW, fort.ANY_COLUMN, fort.CPROP_RIGHT_PADDING, tbl_padding.right)
    end

    --- set column padding, after generic padding to override
    if options.col_padding then
        for col_name, padding in pairs(options.col_padding) do
            if padding.top ~= nil then
                ftable:set_cell_prop(fort.ANY_ROW, column_index_map[col_name], fort.CPROP_TOP_PADDING, padding.top)
            end
            if padding.left ~= nil then
                ftable:set_cell_prop(fort.ANY_ROW, column_index_map[col_name], fort.CPROP_LEFT_PADDING, padding.left)
            end
            if padding.bottom ~= nil then
                ftable:set_cell_prop(fort.ANY_ROW, column_index_map[col_name], fort.CPROP_BOTTOM_PADDING, padding.bottom)
            end
            if padding.right ~= nil then
                ftable:set_cell_prop(fort.ANY_ROW, column_index_map[col_name], fort.CPROP_RIGHT_PADDING, padding.right)
            end
        end
    end

    -- add cell spanning
    if options.cell_span then
        for row_index, cell_span in pairs(options.cell_span) do
            for col_name, span in pairs(cell_span) do
                -- only add header offset when positive indices
                if row_index > 0 then
                    row_index = row_index + header_row_offset
                else
                    row_index = row_index + footer_row_offset
                end
                ftable:set_cell_span(row_index, column_index_map[col_name], span)
            end
        end
    end

    --- add footer spanning
    if options.footer and options.footer_span then
        for col_name, span in pairs(options.footer_span) do
            -- will be the last index
            ftable:set_cell_span(-1, footer_index_map[col_name], span)
        end
    end

    if options.frame ~= nil then
        assert(border_style_mapping[options.frame], "tabulate: invalid border style")
        ftable:set_border_style(border_style_mapping[options.frame])
    end

    return ftable
end

---convenience call to tabulate.tabulate
---@param data tabulate.Data
---@param options tabulate.Options
---@return fort
function tabulate:__call(data, options)
    return self.tabulate(data, options)
end

---@class tabulate.GridOptions
---@field public padding? tabulate.Padding
---@field public frame? tabulate.Frame defaults to 'empty'
---@field public align? table<integer, table<integer, tabulate.Align>>

--- Create a grid of tables
---@param table_grid string[][]
---@param options? tabulate.GridOptions
---@return fort
function tabulate.grid(table_grid, options)
    options = options or {}

    options.frame = options.frame or 'empty'

    ---@type table<string, string>>[]
    local table_map = {}
    local table_cols = {}
    for row, table_row in ipairs(table_grid) do
        table_map[row] = {}
        for col, dtable in ipairs(table_row) do
            table_map[row][tostring(col)] = dtable
            if #table_cols < col then
                table.insert(table_cols, tostring(col))
            end
        end

    end
    return tabulate.tabulate(table_map, {
        column = table_cols,
        ---@diagnostic disable-next-line: assign-type-mismatch
        align = options.align,
        col_padding = options.padding,
        frame = options.frame,
        show_header = false
    })
end

return tabulate
