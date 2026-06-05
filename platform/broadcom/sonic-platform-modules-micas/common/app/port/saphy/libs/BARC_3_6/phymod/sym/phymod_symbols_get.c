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
plp_barchetta_phymod_symbols_get(const plp_barchetta_phymod_symbols_t* symbols, uint32_t sindex, plp_barchetta_phymod_symbol_t* rsym)
{
    if (symbols) {
	if (sindex < symbols->size) {
	    /* Index is within the symbol table */
	    *rsym = symbols->symbols[sindex]; 
	    return 0;
	}
    }
    return -1; 
}

