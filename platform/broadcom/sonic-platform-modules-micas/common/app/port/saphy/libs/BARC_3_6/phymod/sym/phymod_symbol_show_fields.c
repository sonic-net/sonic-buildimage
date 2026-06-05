/*
 * $Id$
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

#include <phymod/phymod_system.h>
#include <phymod/phymod_symbols.h>

#if PHYMOD_CONFIG_INCLUDE_FIELD_INFO == 1

static int
phymod_util_bit_range(char *buf, int size, int minbit, int maxbit)
{
    if (minbit == maxbit) {
        PHYMOD_SNPRINTF(buf, size, "<%d>", minbit);
    } else {
        PHYMOD_SNPRINTF(buf, size, "<%d:%d>", maxbit, minbit);
    }
    return 0;
}

/*
 * Function:
 *	plp_barchetta_phymod_symbol_show_fields
 * Purpose:
 *	Display all fields of a register/memory
 * Parameters:
 *	symbol - symbol information
 *	fnames - list of all field names
 *	data - symbol data (one or more 32-bit words)
 *	nz - skip fields with value of zero
 *	fcb - filter call-back function
 *	cookie - filter call-back context
 * Returns:
 *      Always 0
 */
int 
plp_barchetta_phymod_symbol_show_fields(const plp_barchetta_phymod_symbol_t *symbol,
                          const char **fnames, uint32_t *data, int nz,
                          int (*print_str)(const char *str),
                          phymod_symbol_filter_cb_t fcb, void *cookie)
{
    int mval; 
    plp_barchetta_phymod_field_info_t finfo; 
    char *ptr;
    char vstr[8 * PHYMOD_MAX_REG_WSIZE + 32];
    char fname_str[16];
    const char *fname;
    uint32_t val[PHYMOD_MAX_REG_WSIZE];
    int idx = 0;

    PHYMOD_SYMBOL_FIELDS_ITER_BEGIN(symbol->fields, finfo, fnames) {

        /* Create default field name */
        PHYMOD_SPRINTF(fname_str, "field%d", idx++);
        fname = fname_str;

#if PHYMOD_CONFIG_INCLUDE_FIELD_NAMES == 1
        /* Use real field name when available */
        if (finfo.name) {
            if (fcb && fcb(symbol, fnames, finfo.name, cookie) != 0) {
                continue;
            }
            /* Skip encoding part of field name if present */
            if ((fname = PHYMOD_STRCHR(finfo.name, '}')) != NULL) {
                fname++;
            } else {
                fname = finfo.name;
            }
        }
#endif

        /* Create bit range string */
        phymod_util_bit_range(vstr, sizeof(vstr),
                              finfo.minbit, 
                              finfo.maxbit);

        if (data) {
            /* Get normalized field value */
            PHYMOD_MEMSET(val, 0, sizeof(val));
            plp_barchetta_phymod_field_get(data, finfo.minbit, finfo.maxbit, val);

            /* Create field value string */
            mval = COUNTOF(val) - 1;
            while (mval && val[mval] == 0) {
                mval--;
            }

            /* Optionally skip fields with all zeros */
            if (nz && mval == 0 && val[mval] == 0) {
                continue;
            }

            ptr = vstr + PHYMOD_STRLEN(vstr);
            PHYMOD_SPRINTF(ptr, "=0x%"PRIx32"", val[mval]);
            while (mval--) {
                ptr += PHYMOD_STRLEN(ptr);
                PHYMOD_SPRINTF(ptr, "%08"PRIx32"", val[mval]);
            }
        }

        print_str("\t");
        print_str(fname);
        print_str(vstr);
        print_str("\n");

    } PHYMOD_SYMBOL_FIELDS_ITER_END(); 

    return 0; 
}

#endif /* PHYMOD_CONFIG_INCLUDE_FIELD_INFO */
