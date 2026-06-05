/*
 * $Id$
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

#include <phymod/phymod_symbols.h>

#if PHYMOD_CONFIG_INCLUDE_CHIP_SYMBOLS == 1

/*
 * Function:
 *	plp_barchetta_phymod_symbol_dump
 * Purpose:
 *	Lookup symbol name and display all fields
 * Parameters:
 *	name - symbol name
 *	symbols - symbol table to search
 *	data - symbol data (one or more 32-bit words)
 * Returns:
 *      -1 if symbol not found
 */
int 
plp_barchetta_phymod_symbol_dump(const char *name, const plp_barchetta_phymod_symbols_t *symbols,
                   uint32_t *data,
                   int (*print_str)(const char *str))
{
    int rv;
    plp_barchetta_phymod_symbol_t symbol;

    if ((rv = plp_barchetta_phymod_symbols_find(name, symbols, &symbol)) == 0) {
        plp_barchetta_phymod_symbol_show_fields(&symbol, symbols->field_names, data, 0,
                                  print_str,
                                  plp_barchetta_phymod_symbol_field_filter, data);
    }

    return rv;
}

#endif /* PHYMOD_CONFIG_INCLUDE_CHIP_SYMBOLS */
