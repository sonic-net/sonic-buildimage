#include "lauxlib.h"
#include "lua.h"

#include "fort.h"

#if LUA_VERSION_NUM >= 502
#define new_lib(L, l)       (luaL_newlib(L, l))
#define lua_tbl_len(L, arg) (lua_rawlen(L, arg))
#else
#define new_lib(L, l)       (lua_newtable(L), luaL_register(L, NULL, l))
#define lua_tbl_len(L, arg) (lua_objlen(L, arg))
#endif

// use integer in newer lua for response values
#if LUA_VERSION_NUM < 503
#define lua_pushinteger(L, n) lua_pushinteger(L, n)
#endif

#define FTABLEMETA   "fort.ftable"
#define FBORDERSTYLE "fort.border_style"

#define ERR_CHECK(func)                                                                    \
    do {                                                                                   \
        int _error_code = func;                                                            \
        if (_error_code != 0) {                                                            \
            return luaL_error(L, "fort: %s (%d) ", ft_strerror(_error_code), _error_code); \
        }                                                                                  \
    } while (0);

static int ftable_destroy(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    if (table != NULL) {
        ft_destroy_table(*table);
    }
    // The critical step that will prevent us from allowing
    // a call into a dead object.
    *table = NULL;
    return 0;
}

static void register_ftable(lua_State *L, ft_table_t *table) {
    ft_table_t **data = lua_newuserdata(L, sizeof(ft_table_t *));
    *data = table;
    // add ftable
    luaL_getmetatable(L, FTABLEMETA);
    lua_setmetatable(L, -2);
}

static int lft_create_table(lua_State *L) {
    ft_table_t *table = ft_create_table();
    ERR_CHECK(table != NULL ? FT_SUCCESS : FT_MEMORY_ERROR);

    register_ftable(L, table);
    return 1;
}

static int lft_copy_table(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    ft_table_t *new_table = ft_copy_table(*table);
    ERR_CHECK((new_table != NULL) ? FT_SUCCESS : FT_GEN_ERROR);

    register_ftable(L, new_table);
    return 1;
}

static int lft_ln(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    ERR_CHECK(ft_ln(*table));
    return 0;
}

static int lft_cur_row(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);

    int cur_row = ft_cur_row(*table);
    lua_pushinteger(L, cur_row + 1);
    return 1;
}

static int lft_cur_col(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    int cur_col = ft_cur_col(*table);
    lua_pushinteger(L, cur_col + 1);
    return 1;
}

static int lft_set_cur_cell(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    size_t row = luaL_checknumber(L, 2) - 1;
    size_t col = luaL_checknumber(L, 3) - 1;

    luaL_argcheck(L, lua_tonumber(L, 2) > 0, 2, "Invalid row index");
    luaL_argcheck(L, lua_tonumber(L, 3) > 0, 3, "Invalid column index");

    ft_set_cur_cell(*table, row, col);

    return 0;
}

static int lft_is_empty(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);

    int is_empty = ft_is_empty(*table);
    lua_pushboolean(L, is_empty);
    return 1;
}

static int lft_row_count(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    size_t row_count = ft_row_count(*table);

    lua_pushinteger(L, row_count);
    return 1;
}

static int lft_col_count(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    size_t col_count = ft_col_count(*table);

    lua_pushinteger(L, col_count);
    return 1;
}

static int lft_erase_range(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    size_t top_left_row = luaL_checknumber(L, 2) - 1;
    size_t top_left_col = luaL_checknumber(L, 3) - 1;
    size_t bottom_right_row = luaL_checknumber(L, 4) - 1;
    size_t bottom_right_col = luaL_checknumber(L, 5) - 1;

    luaL_argcheck(L, lua_tonumber(L, 2) > 0, 2, "Invalid row index");
    luaL_argcheck(L, lua_tonumber(L, 3) > 0, 3, "Invalid column index");
    luaL_argcheck(L, lua_tonumber(L, 4) > 0, 4, "Invalid row index");
    luaL_argcheck(L, lua_tonumber(L, 5) > 0, 5, "Invalid column index");

    ERR_CHECK(ft_erase_range(*table, top_left_row, top_left_col, bottom_right_row, bottom_right_col));

    return 0;
}

static const char **generate_row_cells(lua_State *L, int arg_num) {
    size_t cols = lua_tbl_len(L, 2);
    const char **row_cells = malloc(cols * sizeof(char *));
    if (row_cells == NULL) {
        return NULL;
    }
    size_t cols_populated = 0;

    lua_pushnil(L);
    while (lua_next(L, arg_num) != 0) {
        // ensure we dont go over bounds
        if (cols_populated >= cols) {
            lua_pop(L, 1);
            break;
        }
        row_cells[cols_populated] = lua_tostring(L, -1);
        cols_populated += 1;
        lua_pop(L, 1);
    }
    return row_cells;
}

static int lft_row_write(lua_State *L) {
    if (!lua_istable(L, 2)) {
        return luaL_error(L, "Expected string[] for argument 2");
    }

    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);

    size_t cols = lua_tbl_len(L, 2);
    const char **row_cells = generate_row_cells(L, 2);
    if (row_cells == NULL) {
        return luaL_error(L, "Unable to create row cells");
    }

    int error_code = ft_u8nrow_write(*table, cols, row_cells);
    free(row_cells);
    ERR_CHECK(error_code);
    return 0;
}

static int lft_row_write_ln(lua_State *L) {
    if (!lua_istable(L, 2)) {
        return luaL_error(L, "Expected string[] for argument 2");
    }

    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);

    size_t cols = lua_tbl_len(L, 2);
    const char **row_cells = generate_row_cells(L, 2);
    if (row_cells == NULL) {
        return luaL_error(L, "Unable to create row cells");
    }

    int error_code = ft_u8nrow_write_ln(*table, cols, row_cells);
    free(row_cells);
    ERR_CHECK(error_code);
    return 0;
}

static int lft_add_separator(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);

    ERR_CHECK(ft_add_separator(*table));
    return 0;
}

static int lft_to_string(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    const char *table_string = (const char *)ft_to_u8string(*table);
    lua_pushstring(L, table_string);
    return 1;
}

static int lft_set_default_border_style(lua_State *L) {
    const struct ft_border_style **style = luaL_checkudata(L, 1, FBORDERSTYLE);

    ERR_CHECK(ft_set_default_border_style(*style));
    return 0;
}
static int lft_set_border_style(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    const struct ft_border_style **style = luaL_checkudata(L, 2, FBORDERSTYLE);

    ERR_CHECK(ft_set_border_style(*table, *style));
    return 0;
}

static int lft_set_default_cell_prop(lua_State *L) {
    uint32_t property = luaL_checknumber(L, 1);
    int value = luaL_checknumber(L, 2);

    luaL_argcheck(L, lua_tonumber(L, 1) > 0, 1, "property must be > 0");

    ERR_CHECK(ft_set_default_cell_prop(property, value));
    return 0;
}
static int lft_set_cell_prop(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    size_t row = luaL_checknumber(L, 2);
    row -= (row > FT_MAX_ROW_INDEX) ? 0 : 1;
    size_t col = luaL_checknumber(L, 3);
    col -= (col > FT_MAX_COL_INDEX) ? 0 : 1;
    uint32_t property = luaL_checknumber(L, 4);
    int value = luaL_checknumber(L, 5);

    luaL_argcheck(L, lua_tonumber(L, 2) > 0, 2, "Invalid row index");
    luaL_argcheck(L, lua_tonumber(L, 3) > 0, 3, "Invalid column index");
    luaL_argcheck(L, lua_tonumber(L, 4) > 0, 4, "property > 0");

    ERR_CHECK(ft_set_cell_prop(*table, row, col, property, value));
    return 0;
}

static int lft_set_default_tbl_prop(lua_State *L) {
    uint32_t property = luaL_checknumber(L, 1);
    int value = luaL_checknumber(L, 2);

    luaL_argcheck(L, lua_tonumber(L, 1) > 0, 1, "property must be > 0");

    ERR_CHECK(ft_set_default_tbl_prop(property, value));
    return 0;
}
static int lft_set_tbl_prop(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    uint32_t property = luaL_checknumber(L, 2);
    int value = luaL_checknumber(L, 3);

    luaL_argcheck(L, lua_tonumber(L, 2) > 0, 2, "property must be > 0");

    ERR_CHECK(ft_set_tbl_prop(*table, property, value));
    return 0;
}
static int lft_set_cell_span(lua_State *L) {
    ft_table_t **table = luaL_checkudata(L, 1, FTABLEMETA);
    size_t row = luaL_checknumber(L, 2) - 1;
    size_t col = luaL_checknumber(L, 3) - 1;
    size_t hor_span = luaL_checknumber(L, 4);

    luaL_argcheck(L, lua_tonumber(L, 2) > 0, 2, "Invalid row index");
    luaL_argcheck(L, lua_tonumber(L, 3) > 0, 3, "Invalid column index");
    luaL_argcheck(L, lua_tonumber(L, 4) > 0, 4, "property must be > 0");

    ERR_CHECK(ft_set_cell_span(*table, row, col, hor_span));
    return 0;
}

static const struct luaL_Reg fort_functions[] = {
    {"create_table", lft_create_table},
    {"copy_table", lft_copy_table},
    {"ln", lft_ln},
    {"cur_row", lft_cur_row},
    {"cur_col", lft_cur_col},
    {"set_cur_cell", lft_set_cur_cell},
    {"is_empty", lft_is_empty},
    {"row_count", lft_row_count},
    {"col_count", lft_col_count},
    {"erase_range", lft_erase_range},
    {"row_write", lft_row_write},
    {"row_write_ln", lft_row_write_ln},
    {"add_separator", lft_add_separator},
    {"to_string", lft_to_string},
    {"set_default_border_style", lft_set_default_border_style},
    {"set_border_style", lft_set_border_style},
    {"set_default_cell_prop", lft_set_default_cell_prop},
    {"set_cell_prop", lft_set_cell_prop},
    {"set_default_tbl_prop", lft_set_default_tbl_prop},
    {"set_tbl_prop", lft_set_tbl_prop},
    {"set_cell_span", lft_set_cell_span},
    {NULL, NULL},
};

static const struct luaL_Reg ftable_meta[] = {
    {"__gc", ftable_destroy},
    {"__tostring", lft_to_string},
    {NULL, NULL},
};

static void register_fort_style(lua_State *L, const struct ft_border_style *style) {
    const struct ft_border_style **border_style = lua_newuserdata(L, sizeof(struct ft_border_style **));
    *border_style = style;
    luaL_getmetatable(L, FBORDERSTYLE);
    lua_setmetatable(L, -2);
}

#define REGISTER_FORT_STYLE(name, style) \
    do {                                 \
        register_fort_style(L, style);   \
        lua_setfield(L, -2, name);       \
    } while (0)

int luaopen_cfort(lua_State *L) {
    new_lib(L, fort_functions);

    luaL_newmetatable(L, FTABLEMETA);
#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(L, ftable_meta, 0);
#else
    luaL_openlib(L, 0, ftable_meta, 0);
#endif

    lua_setfield(L, -2, "ftable_mt");

    luaL_newmetatable(L, FBORDERSTYLE);
    lua_pop(L, 1);

    // styles
    REGISTER_FORT_STYLE("BASIC_STYLE", FT_BASIC_STYLE);
    REGISTER_FORT_STYLE("BASIC2_STYLE", FT_BASIC2_STYLE);
    REGISTER_FORT_STYLE("SIMPLE_STYLE", FT_SIMPLE_STYLE);
    REGISTER_FORT_STYLE("PLAIN_STYLE", FT_PLAIN_STYLE);
    REGISTER_FORT_STYLE("DOT_STYLE", FT_DOT_STYLE);
    REGISTER_FORT_STYLE("EMPTY_STYLE", FT_EMPTY_STYLE);
    REGISTER_FORT_STYLE("EMPTY2_STYLE", FT_EMPTY2_STYLE);
    REGISTER_FORT_STYLE("SOLID_STYLE", FT_SOLID_STYLE);
    REGISTER_FORT_STYLE("SOLID_ROUND_STYLE", FT_SOLID_ROUND_STYLE);
    REGISTER_FORT_STYLE("NICE_STYLE", FT_NICE_STYLE);
    REGISTER_FORT_STYLE("DOUBLE_STYLE", FT_DOUBLE_STYLE);
    REGISTER_FORT_STYLE("DOUBLE2_STYLE", FT_DOUBLE2_STYLE);
    REGISTER_FORT_STYLE("BOLD_STYLE", FT_BOLD_STYLE);
    REGISTER_FORT_STYLE("BOLD2_STYLE", FT_BOLD2_STYLE);
    REGISTER_FORT_STYLE("FRAME_STYLE", FT_FRAME_STYLE);

    // special macros
    lua_pushinteger(L, FT_ANY_COLUMN);
    lua_setfield(L, -2, "ANY_COLUMN");
    lua_pushinteger(L, FT_CUR_COLUMN);
    lua_setfield(L, -2, "CUR_COLUMN");
    lua_pushinteger(L, FT_ANY_ROW);
    lua_setfield(L, -2, "ANY_ROW");
    lua_pushinteger(L, FT_CUR_ROW);
    lua_setfield(L, -2, "CUR_ROW");

    // cell properties
    lua_pushinteger(L, FT_CPROP_MIN_WIDTH);
    lua_setfield(L, -2, "CPROP_MIN_WIDTH");
    lua_pushinteger(L, FT_CPROP_TEXT_ALIGN);
    lua_setfield(L, -2, "CPROP_TEXT_ALIGN");
    lua_pushinteger(L, FT_CPROP_TOP_PADDING);
    lua_setfield(L, -2, "CPROP_TOP_PADDING");
    lua_pushinteger(L, FT_CPROP_BOTTOM_PADDING);
    lua_setfield(L, -2, "CPROP_BOTTOM_PADDING");
    lua_pushinteger(L, FT_CPROP_LEFT_PADDING);
    lua_setfield(L, -2, "CPROP_LEFT_PADDING");
    lua_pushinteger(L, FT_CPROP_RIGHT_PADDING);
    lua_setfield(L, -2, "CPROP_RIGHT_PADDING");
    lua_pushinteger(L, FT_CPROP_EMPTY_STR_HEIGHT);
    lua_setfield(L, -2, "CPROP_EMPTY_STR_HEIGHT");
    lua_pushinteger(L, FT_CPROP_ROW_TYPE);
    lua_setfield(L, -2, "CPROP_ROW_TYPE");
    lua_pushinteger(L, FT_CPROP_CONT_FG_COLOR);
    lua_setfield(L, -2, "CPROP_CONT_FG_COLOR");
    lua_pushinteger(L, FT_CPROP_CELL_BG_COLOR);
    lua_setfield(L, -2, "CPROP_CELL_BG_COLOR");
    lua_pushinteger(L, FT_CPROP_CONT_BG_COLOR);
    lua_setfield(L, -2, "CPROP_CONT_BG_COLOR");
    lua_pushinteger(L, FT_CPROP_CELL_TEXT_STYLE);
    lua_setfield(L, -2, "CPROP_CELL_TEXT_STYLE");
    lua_pushinteger(L, FT_CPROP_CONT_TEXT_STYLE);
    lua_setfield(L, -2, "CPROP_CONT_TEXT_STYLE");

    // colors
    lua_pushinteger(L, FT_COLOR_DEFAULT);
    lua_setfield(L, -2, "COLOR_DEFAULT");
    lua_pushinteger(L, FT_COLOR_BLACK);
    lua_setfield(L, -2, "COLOR_BLACK");
    lua_pushinteger(L, FT_COLOR_RED);
    lua_setfield(L, -2, "COLOR_RED");
    lua_pushinteger(L, FT_COLOR_GREEN);
    lua_setfield(L, -2, "COLOR_GREEN");
    lua_pushinteger(L, FT_COLOR_YELLOW);
    lua_setfield(L, -2, "COLOR_YELLOW");
    lua_pushinteger(L, FT_COLOR_BLUE);
    lua_setfield(L, -2, "COLOR_BLUE");
    lua_pushinteger(L, FT_COLOR_MAGENTA);
    lua_setfield(L, -2, "COLOR_MAGENTA");
    lua_pushinteger(L, FT_COLOR_CYAN);
    lua_setfield(L, -2, "COLOR_CYAN");
    lua_pushinteger(L, FT_COLOR_LIGHT_GRAY);
    lua_setfield(L, -2, "COLOR_LIGHT_GRAY");
    lua_pushinteger(L, FT_COLOR_DARK_GRAY);
    lua_setfield(L, -2, "COLOR_DARK_GRAY");
    lua_pushinteger(L, FT_COLOR_LIGHT_RED);
    lua_setfield(L, -2, "COLOR_LIGHT_RED");
    lua_pushinteger(L, FT_COLOR_LIGHT_GREEN);
    lua_setfield(L, -2, "COLOR_LIGHT_GREEN");
    lua_pushinteger(L, FT_COLOR_LIGHT_YELLOW);
    lua_setfield(L, -2, "COLOR_LIGHT_YELLOW");
    lua_pushinteger(L, FT_COLOR_LIGHT_BLUE);
    lua_setfield(L, -2, "COLOR_LIGHT_BLUE");
    lua_pushinteger(L, FT_COLOR_LIGHT_MAGENTA);
    lua_setfield(L, -2, "COLOR_LIGHT_MAGENTA");
    lua_pushinteger(L, FT_COLOR_LIGHT_CYAN);
    lua_setfield(L, -2, "COLOR_LIGHT_CYAN");
    lua_pushinteger(L, FT_COLOR_LIGHT_WHYTE);
    lua_setfield(L, -2, "COLOR_LIGHT_WHYTE");

    // text style
    lua_pushinteger(L, FT_TSTYLE_DEFAULT);
    lua_setfield(L, -2, "TSTYLE_DEFAULT");
    lua_pushinteger(L, FT_TSTYLE_BOLD);
    lua_setfield(L, -2, "TSTYLE_BOLD");
    lua_pushinteger(L, FT_TSTYLE_DIM);
    lua_setfield(L, -2, "TSTYLE_DIM");
    lua_pushinteger(L, FT_TSTYLE_ITALIC);
    lua_setfield(L, -2, "TSTYLE_ITALIC");
    lua_pushinteger(L, FT_TSTYLE_UNDERLINED);
    lua_setfield(L, -2, "TSTYLE_UNDERLINED");
    lua_pushinteger(L, FT_TSTYLE_BLINK);
    lua_setfield(L, -2, "TSTYLE_BLINK");
    lua_pushinteger(L, FT_TSTYLE_INVERTED);
    lua_setfield(L, -2, "TSTYLE_INVERTED");
    lua_pushinteger(L, FT_TSTYLE_HIDDEN);
    lua_setfield(L, -2, "TSTYLE_HIDDEN");

    // text alignment
    lua_pushinteger(L, FT_ALIGNED_LEFT);
    lua_setfield(L, -2, "ALIGNED_LEFT");
    lua_pushinteger(L, FT_ALIGNED_CENTER);
    lua_setfield(L, -2, "ALIGNED_CENTER");
    lua_pushinteger(L, FT_ALIGNED_RIGHT);
    lua_setfield(L, -2, "ALIGNED_RIGHT");

    // row type
    lua_pushinteger(L, FT_ROW_COMMON);
    lua_setfield(L, -2, "ROW_COMMON");
    lua_pushinteger(L, FT_ROW_HEADER);
    lua_setfield(L, -2, "ROW_HEADER");

    // text prop
    lua_pushinteger(L, FT_TPROP_LEFT_MARGIN);
    lua_setfield(L, -2, "TPROP_LEFT_MARGIN");
    lua_pushinteger(L, FT_TPROP_TOP_MARGIN);
    lua_setfield(L, -2, "TPROP_TOP_MARGIN");
    lua_pushinteger(L, FT_TPROP_RIGHT_MARGIN);
    lua_setfield(L, -2, "TPROP_RIGHT_MARGIN");
    lua_pushinteger(L, FT_TPROP_BOTTOM_MARGIN);
    lua_setfield(L, -2, "TPROP_BOTTOM_MARGIN");
    lua_pushinteger(L, FT_TPROP_ADDING_STRATEGY);
    lua_setfield(L, -2, "TPROP_ADDING_STRATEGY");

    // adding strategy
    lua_pushinteger(L, FT_STRATEGY_REPLACE);
    lua_setfield(L, -2, "STRATEGY_REPLACE");
    lua_pushinteger(L, FT_STRATEGY_INSERT);
    lua_setfield(L, -2, "STRATEGY_INSERT");

    return 1;
}
