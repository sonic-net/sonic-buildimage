--- Fort Module
-- @pragma nostrip
local cfort = require "cfort"

---@class fort
local fort = {}

-- enable table object access
cfort.ftable_mt.__index = fort
setmetatable(fort, fort)

local function split(inputstr, sep)
    if sep == nil then
        sep = "%s"
    end
    local t = {}
    for str in string.gmatch(inputstr, "([^" .. sep .. "]*)") do
        table.insert(t, str)
    end
    return t
end

---Create a new formatted table
---@return fort new formatted table
function fort.create_table()
    return cfort.create_table()
end

---Alias of @{fort.create_table}
fort.new = fort.create_table

---Alias of @{fort.create_table}
fort.create = fort.create_table

---Alias of @{fort.create_table}
fort.__call = fort.new

---Control the default separator for @{printf} and @{printf_ln} functions.
---(Defaults to '|')
fort.default_separator = "|"

---Use a formatted string to write a row.
---
---Splits cells on fort.default_separator. Uses lua @{string.format} to
---format the text.
---@param row_format string row to write with formatting
---@vararg any format arguments
function fort:printf(row_format, ...)
    local formatted_text = string.format(row_format, ...)
    local row = split(formatted_text, fort.default_separator)
    fort.row_write(self, row)
end

---Use a formatted string to write a row and go to the next line.
---
---Splits cells on fort.default_separator. Uses lua @{string.format} to
---format the text.
---@param row_format string row to write with formatting
---@vararg any format arguments
function fort:printf_ln(row_format, ...)
    local formatted_text = string.format(row_format, ...)
    local row = split(formatted_text, fort.default_separator)
    fort.row_write_ln(self, row)
end

---Write a row.
---Use a single string to write a row. Allows using a custom cell separator
---without modifying the global separator.
---@param row_text string
---@param sep? string sep cell separator
function fort:print(row_text, sep)
    sep = sep or fort.default_separator
    local row = split(row_text, sep)
    fort.row_write(self, row)
end

---Write a row and go to the next line.
---Use a single string to write a row. Allows using a custom cell separator
---without modifying the global separator.
---@param row_text string
---@param sep? string sep cell separator
function fort:print_ln(row_text, sep)
    sep = sep or fort.default_separator
    local row = split(row_text, sep)
    fort.row_write_ln(self, row)
end

---Write a 2d array of strings to the ftable.
---@param data_table string[][] data_table 2d array of strings to write, can be jagged.
---@param colalign? boolean  align to the @{cur_col} at the start
function fort:table_write(data_table, colalign)
    colalign = colalign or false
    local rows = #data_table
    local cur_col = fort.cur_col(self)
    for row_index, data_row in ipairs(data_table) do
        fort.row_write(self, data_row)
        if row_index ~= rows then
            fort.ln(self)
            if colalign then
                local cur_row = fort.cur_row(self)
                fort.set_cur_cell(self, cur_row, cur_col)
            end
        end
    end
end

---Write a 2d array of strings to the ftable and go to next line.
---@param data_table string[][] 2d array of strings to write, can be jagged.
---@param colalign? boolean align to the @{cur_col} at the start
function fort:table_write_ln(data_table, colalign)
    local cur_col = fort.cur_col(self)
    colalign = colalign or false
    fort.table_write(self, data_table, colalign)
    fort.ln(self)
    if colalign then
        local cur_row = fort.cur_row(self)
        fort.set_cur_cell(self, cur_row, cur_col)
    end
end

---Add a dividing separtor line at the current row.
function fort:add_separator()
    cfort.add_separator(self)
end

---Completely Copy a table
---@return fort copied table
function fort:copy_table()
    return cfort.copy_table(self)
end

---Alias of @{fort.copy_table}
fort.copy = fort.copy_table

---Get the current column
---@return integer
function fort:cur_col()
    return cfort.cur_col(self)
end

---Get the current row
---@return integer
function fort:cur_row()
    return cfort.cur_row(self)
end

---Erase a rectangular range of data from the ftable
---@param top_left_row integer
---@param top_left_col integer
---@param bottom_right_row integer
---@param bottom_right_col integer
function fort:erase_range(top_left_row, top_left_col, bottom_right_row, bottom_right_col)
    if top_left_row < 0 then
        top_left_row = self:row_count() + top_left_row + 1
    end
    if top_left_col < 0 then
        top_left_col = self:col_count() + top_left_col + 1
    end
    if bottom_right_row < 0 then
        bottom_right_row = self:row_count() + bottom_right_row + 1
    end
    if bottom_right_col < 0 then
        bottom_right_col = self:col_count() + bottom_right_col + 1
    end
    return cfort.erase_range(self, top_left_row, top_left_col, bottom_right_row, bottom_right_col)
end

---Check if ftable is empty
---@return boolean
function fort:is_empty()
    return cfort.is_empty(self)
end

---Go to the next line (row)
function fort:ln()
    return cfort.ln(self)
end

---Get the number of rows in the ftable
---@return integer
function fort:row_count()
    return cfort.row_count(self)
end

---Get the number of columns in the ftable
---@return integer
function fort:col_count()
    return cfort.col_count(self)
end

---Write a row of data.
---@param row any[] row of data to write (uses tostring)
function fort:row_write(row)
    assert(type(row) == "table", "Expected table for arg 1")
    local stringified_row = {}
    for _, v in ipairs(row) do
        table.insert(stringified_row, tostring(v))
    end
    cfort.row_write(self, stringified_row)
end

---Write a row of data and go to the next line.
---@param row any[] row of data to write (uses tostring)
function fort:row_write_ln(row)
    assert(type(row) == "table", "Expected table for arg 1")
    local stringified_row = {}
    for _, v in ipairs(row) do
        table.insert(stringified_row, tostring(v))
    end
    cfort.row_write_ln(self, stringified_row)
end

---Write a row
---@vararg any data to write in the row (uses tostring)
function fort:write(...)
    fort.row_write(self, {...})
end
---Write a row and go to the next line
---@vararg any data to write in the row
function fort:write_ln(...)
    fort.row_write_ln(self, {...})
end

---Set the border style of the ftable.
---@param style fort.BorderStyle available styles @{BASIC_STYLE}
function fort:set_border_style(style)
    cfort.set_border_style(self, style)
end

---Set the cell property of the ftable.
---@param row integer the row to set, can also use @{ANY_ROW}/@{CUR_ROW}
---@param col integer column to set, can also use @{ANY_COLUMN}/@{CUR_COLUMN}
---@param property fort.CellProperty property to set
---@param value integer to set
function fort:set_cell_prop(row, col, property, value)

    if row < 0 then
        row = self:row_count() + row + 1
    end
    if col < 0 then
        col = self:col_count() + col + 1
    end

    cfort.set_cell_prop(self, row, col, property, value)
end

---Set a cell's horizontal span in the ftable.
---@param row integer the row to set. DO NOT USE @{ANY_ROW}/@{CUR_ROW}
---@param col integer the column to set. DO NOT USE @{ANY_COLUMN}/@{CUR_COLUMN}
---@param span integer how many columns the cell should span
function fort:set_cell_span(row, col, span)
    if row < 0 then
        row = self:row_count() + row + 1
    end
    if col < 0 then
        col = self:col_count() + col + 1
    end
    cfort.set_cell_span(self, row, col, span)
end

---Set the current cell position.
---@param row integer the row to set. DO NOT USE @{ANY_ROW}/@{CUR_ROW}
---@param col integer the column to set. DO NOT USE @{ANY_COLUMN}/@{CUR_COLUMN}
function fort:set_cur_cell(row, col)
    if row < 0 then
        row = self:row_count() + row + 1
    end
    if col < 0 then
        col = self:col_count() + col + 1
    end
    cfort.set_cur_cell(self, row, col)
end

---Set the default border style for new tables.
---@param style fort.BorderStyle available styles @{BASIC_STYLE}
function fort.set_default_border_style(style)
    cfort.set_default_border_style(style)
end

---Set a default cell property for new tables.
---@param property fort.CellProperty the property to set
---@param value integer value to set
function fort.set_default_cell_prop(property, value)
    cfort.set_default_cell_prop(property, value)
end

---Set a default table property for new tables.
---@param property fort.TableProperty the property to set
---@param value integer value to set
function fort.set_default_tbl_prop(property, value)
    cfort.set_default_tbl_prop(property, value)
end

---Set a table property.
---@param property fort.TableProperty the property to set
---@param value integer value to set
function fort:set_tbl_prop(property, value)
    cfort.set_tbl_prop(self, property, value)
end

---Generate the string version of the ftable.
---@return string formatted table string
function fort:to_string()
    return cfort.to_string(self)
end

---Cell Text Align.
-- Indicate the text alignment inside a cell.
--
-- Can be used with @{set_cell_prop}/@{set_default_cell_prop}
-- @section cell-text-align

---@type integer
fort.ALIGNED_CENTER = cfort.ALIGNED_CENTER
---@type integer
fort.ALIGNED_LEFT = cfort.ALIGNED_LEFT
---@type integer
fort.ALIGNED_RIGHT = cfort.ALIGNED_RIGHT

---Cell Selectors.
-- Special flags that select a cell
--
-- Can be used with @{set_cell_prop}
-- @section cell-selector

---@type integer select all columns
fort.ANY_COLUMN = cfort.ANY_COLUMN
---@type integer
fort.ANY_ROW = cfort.ANY_ROW
---@type integer
fort.CUR_COLUMN = cfort.CUR_COLUMN
---@type integer
fort.CUR_ROW = cfort.CUR_ROW

---Table Border Style.
-- Border styling of the table.
--
-- Refer to refer to
-- [this page](https://github.com/seleznevae/libfort/wiki/Border-styles-%28C-API%29#built-in-border-styles)
-- to see how each border style looks.
-- @section table-border-style

---@class fort.BorderStyle

---@type fort.BorderStyle
fort.BASIC_STYLE = cfort.BASIC_STYLE
---@type fort.BorderStyle
fort.BASIC2_STYLE = cfort.BASIC2_STYLE
---@type fort.BorderStyle
fort.BOLD_STYLE = cfort.BOLD_STYLE
---@type fort.BorderStyle
fort.BOLD2_STYLE = cfort.BOLD2_STYLE
---@type fort.BorderStyle
fort.DOT_STYLE = cfort.DOT_STYLE
---@type fort.BorderStyle
fort.DOUBLE_STYLE = cfort.DOUBLE_STYLE
---@type fort.BorderStyle
fort.DOUBLE2_STYLE = cfort.DOUBLE2_STYLE
---@type fort.BorderStyle
fort.EMPTY_STYLE = cfort.EMPTY_STYLE
---@type fort.BorderStyle
fort.EMPTY2_STYLE = cfort.EMPTY2_STYLE
---@type fort.BorderStyle
fort.FRAME_STYLE = cfort.FRAME_STYLE
---@type fort.BorderStyle
fort.NICE_STYLE = cfort.NICE_STYLE
---@type fort.BorderStyle
fort.PLAIN_STYLE = cfort.PLAIN_STYLE
---@type fort.BorderStyle
fort.SIMPLE_STYLE = cfort.SIMPLE_STYLE
---@type fort.BorderStyle
fort.SOLID_ROUND_STYLE = cfort.SOLID_ROUND_STYLE
---@type fort.BorderStyle
fort.SOLID_STYLE = cfort.SOLID_STYLE

---Cell Color.
-- Color used for cell/content properties.
-- @section cell-color

---@type integer
fort.COLOR_BLACK = cfort.COLOR_BLACK
---@type integer
fort.COLOR_BLUE = cfort.COLOR_BLUE
---@type integer
fort.COLOR_CYAN = cfort.COLOR_CYAN
---@type integer
fort.COLOR_DARK_GRAY = cfort.COLOR_DARK_GRAY
---@type integer
fort.COLOR_DEFAULT = cfort.COLOR_DEFAULT
---@type integer
fort.COLOR_GREEN = cfort.COLOR_GREEN
---@type integer
fort.COLOR_LIGHT_BLUE = cfort.COLOR_LIGHT_BLUE
---@type integer
fort.COLOR_LIGHT_CYAN = cfort.COLOR_LIGHT_CYAN
---@type integer
fort.COLOR_LIGHT_GRAY = cfort.COLOR_LIGHT_GRAY
---@type integer
fort.COLOR_LIGHT_GREEN = cfort.COLOR_LIGHT_GREEN
---@type integer
fort.COLOR_LIGHT_MAGENTA = cfort.COLOR_LIGHT_MAGENTA
---@type integer
fort.COLOR_LIGHT_RED = cfort.COLOR_LIGHT_RED
---@type integer
fort.COLOR_LIGHT_WHITE = cfort.COLOR_LIGHT_WHYTE
---@type integer
fort.COLOR_LIGHT_YELLOW = cfort.COLOR_LIGHT_YELLOW
---@type integer
fort.COLOR_MAGENTA = cfort.COLOR_MAGENTA
---@type integer
fort.COLOR_RED = cfort.COLOR_RED
---@type integer
fort.COLOR_YELLOW = cfort.COLOR_YELLOW

---@class fort.CellProperty : number

---@type fort.CellProperty
fort.CPROP_CELL_BG_COLOR = cfort.CPROP_CELL_BG_COLOR
---@type fort.CellProperty
fort.CPROP_CELL_TEXT_STYLE = cfort.CPROP_CELL_TEXT_STYLE
---@type fort.CellProperty
fort.CPROP_CONT_BG_COLOR = cfort.CPROP_CONT_BG_COLOR
---@type fort.CellProperty
fort.CPROP_CONT_FG_COLOR = cfort.CPROP_CONT_FG_COLOR
---@type fort.CellProperty
fort.CPROP_CONT_TEXT_STYLE = cfort.CPROP_CONT_TEXT_STYLE
---@type fort.CellProperty
fort.CPROP_EMPTY_STR_HEIGHT = cfort.CPROP_EMPTY_STR_HEIGHT
---@type fort.CellProperty
fort.CPROP_MIN_WIDTH = cfort.CPROP_MIN_WIDTH
---@type fort.CellProperty
fort.CPROP_ROW_TYPE = cfort.CPROP_ROW_TYPE
---@type fort.CellProperty
fort.CPROP_TEXT_ALIGN = cfort.CPROP_TEXT_ALIGN
---@type fort.CellProperty
fort.CPROP_TOP_PADDING = cfort.CPROP_TOP_PADDING
---@type fort.CellProperty
fort.CPROP_LEFT_PADDING = cfort.CPROP_LEFT_PADDING
---@type fort.CellProperty
fort.CPROP_BOTTOM_PADDING = cfort.CPROP_BOTTOM_PADDING
---@type fort.CellProperty
fort.CPROP_RIGHT_PADDING = cfort.CPROP_RIGHT_PADDING

---Row Type.
-- @section row-type

---@type integer
fort.ROW_COMMON = cfort.ROW_COMMON
---@type integer
fort.ROW_HEADER = cfort.ROW_HEADER

---Table Adding Strategy.
-- @section table-adding-strategy

---@type integer
---Insert new cells
fort.STRATEGY_INSERT = cfort.STRATEGY_INSERT
---@type integer
---Replace current cells
fort.STRATEGY_REPLACE = cfort.STRATEGY_REPLACE

---@class fort.TableProperty : number

---@type fort.TableProperty
fort.TPROP_ADDING_STRATEGY = cfort.TPROP_ADDING_STRATEGY
---@type fort.TableProperty
fort.TPROP_BOTTOM_MARGIN = cfort.TPROP_BOTTOM_MARGIN
---@type fort.TableProperty
fort.TPROP_LEFT_MARGIN = cfort.TPROP_LEFT_MARGIN
---@type fort.TableProperty
fort.TPROP_RIGHT_MARGIN = cfort.TPROP_RIGHT_MARGIN
---@type fort.TableProperty
fort.TPROP_TOP_MARGIN = cfort.TPROP_TOP_MARGIN

---Cell Text Style.
-- @section cell-text-style

---@type integer
fort.TSTYLE_BLINK = cfort.TSTYLE_BLINK
---@type integer
fort.TSTYLE_BOLD = cfort.TSTYLE_BOLD
---@type integer
fort.TSTYLE_DEFAULT = cfort.TSTYLE_DEFAULT
---@type integer
fort.TSTYLE_DIM = cfort.TSTYLE_DIM
---@type integer
fort.TSTYLE_HIDDEN = cfort.TSTYLE_HIDDEN
---@type integer
fort.TSTYLE_INVERTED = cfort.TSTYLE_INVERTED
---@type integer
fort.TSTYLE_ITALIC = cfort.TSTYLE_ITALIC
---@type integer
fort.TSTYLE_UNDERLINED = cfort.TSTYLE_UNDERLINED

return fort
