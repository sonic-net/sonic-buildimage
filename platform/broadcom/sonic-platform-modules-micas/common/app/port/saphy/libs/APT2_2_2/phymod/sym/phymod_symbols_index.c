/*
 * $Id$
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

/*******************************************************************************
 *
 * PHYMOD Symbol Routines
 *
 *
 ******************************************************************************/

#include <phymod/phymod_symbols.h>

int
plp_aperta2_phymod_symbols_index(const plp_aperta2_phymod_symbols_t *symbols, const plp_aperta2_phymod_symbol_t *symbol)
{
    int size, i; 

    if (symbols == NULL || symbols->symbols == NULL) {
        return -1;
    }

    size = symbols->size;
    for (i = 0; i < size; i++) {
        /* Sufficient to compare just the pointers */
        if (symbol->name == symbols->symbols[i].name) {
            return i;
        }
    }
    return -1;
}
