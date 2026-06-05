/*
 * $Id$
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 * PHYMOD Shell Utility: SYMOPS
 *
 * These utility functions provide all of the symbolic
 * register/memory encoding and decoding. 
 *
 */

#include <phymod/phymod_system.h>
#include <phymod/phymod_symbols.h>

static char *
_strtolower(char *dst, const char *src, int dmax)
{
    const char *s = src; 
    char *d = dst; 
    
    while (*s && --dmax) {
        *d = *s;
        if (*s >= 'A' && *s <= 'Z') {
            *d += ('a' - 'A'); 
        }
        s++;
        d++;
    }
    *d = 0;

    return dst; 
}

static int
_sym_match(plp_aperta_phymod_symbols_iter_t *iter, const char *sym_name)
{
    char search_name[64]; 
    char symbol_name[64]; 

    _strtolower(search_name, iter->name, sizeof(search_name)); 
    _strtolower(symbol_name, sym_name, sizeof(symbol_name));

    switch(iter->matching_mode) {
    case PHYMOD_SYMBOLS_ITER_MODE_EXACT:
        if (PHYMOD_STRCMP(search_name, symbol_name) == 0) {
            /* Name matches */
            return 1;
        }
        break; 
    case PHYMOD_SYMBOLS_ITER_MODE_START:
        if (PHYMOD_STRNCMP(symbol_name, search_name,
                        PHYMOD_STRLEN(search_name)) == 0) {
            /* Name matches */
            return 1;
        }
        break;
    case PHYMOD_SYMBOLS_ITER_MODE_STRSTR:
        if (PHYMOD_STRSTR(symbol_name, search_name) != NULL) {
            /* Name matches */
            return 1;
        }
        break; 
    default:
        break;
    }
    return 0;
}

int
plp_aperta_phymod_symbols_iter(plp_aperta_phymod_symbols_iter_t *iter)
{       
    int count = 0; 
    int rc; 
    int match;
    uint32_t idx; 
    plp_aperta_phymod_symbol_t s; 

    for (idx = 0; plp_aperta_phymod_symbols_get(iter->symbols, idx, &s) >= 0; idx++) {

        if (s.name == 0) {
            /* Last */
            continue;
        }

        /* Check flags which must be present */
        if (iter->pflags && ((s.flags & iter->pflags) != iter->pflags)) {
            /* Symbol does not match */
            continue; 
        }

        /* Check flags which must be absent */
        if (iter->aflags && ((s.flags & iter->aflags) != 0)) {
            /* Symbol does not match */
            continue;
        }

        /* Check the name */
        if (PHYMOD_STRCMP("*", iter->name) != 0) {
            /* Not wildcarded */
            match = 0;
            if (_sym_match(iter, s.name)) {
                match = 1;
            }
#if PHYMOD_CONFIG_INCLUDE_ALIAS_NAMES == 1
            else if (s.ufname && _sym_match(iter, s.ufname)) {
                match = 1;
            }
            else if (s.alias && _sym_match(iter, s.alias)) {
                match = 1;
            }
#endif
            if (!match) {
                continue;
            }
        }

        /* Whew, name is okay */
        count++; 

        if ((rc = iter->function(&s, iter->vptr)) < 0) {
            return rc;
        }
    }
    
    return count; 
}
