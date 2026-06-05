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

#if PHYMOD_CONFIG_INCLUDE_CHIP_SYMBOLS == 1

static const void *
__phymod_symbol_find(const char *name, const void *table, int size, int entry_size)
{
    int i; 
    plp_aperta2_phymod_symbol_t *sym;
    unsigned char *ptr = (unsigned char*)table; 

    if (table == NULL) {
        return NULL;
    }
    
    for (i = 0; (sym = (plp_aperta2_phymod_symbol_t*)(ptr)) && (i < size); i++) {
	if (PHYMOD_STRCMP(sym->name, name) == 0) {
	    return (void*) sym; 
	}
#if PHYMOD_CONFIG_INCLUDE_ALIAS_NAMES == 1
	if (sym->ufname && PHYMOD_STRCMP(sym->ufname, name) == 0) {
	    return (void*) sym; 
	}
	if (sym->alias && PHYMOD_STRCMP(sym->alias, name) == 0) {
	    return (void*) sym; 
	}
#endif
	ptr += entry_size; 
    }

    return NULL; 
}
	

const plp_aperta2_phymod_symbol_t *
phymod_symbol_find(const char *name, const plp_aperta2_phymod_symbol_t *table, int size)
{
    return (plp_aperta2_phymod_symbol_t*) __phymod_symbol_find(name, table, size, sizeof(plp_aperta2_phymod_symbol_t)); 
}


int 
plp_aperta2_phymod_symbols_find(const char *name, const plp_aperta2_phymod_symbols_t *symbols, plp_aperta2_phymod_symbol_t *rsym)
{
    const plp_aperta2_phymod_symbol_t *s = NULL; 

    if (rsym == NULL) return -1; 

    if ((symbols->symbols) && (s = phymod_symbol_find(name, symbols->symbols, symbols->size))) {
	*rsym = *s;
	return 0; 
    }
    return -1;
}

#endif
