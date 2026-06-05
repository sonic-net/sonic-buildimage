/*
 * $Id$
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

#include <phymod/phymod_symbols.h>

static size_t
phymod_strlcpy(char *dest, const char *src, size_t cnt)
{
    char *ptr = dest;
    size_t copied = 0;

    while (*src && (cnt > 1)) {
	*ptr++ = *src++;
	cnt--;
	copied++;
    }
    *ptr = '\0';

    return copied;
}

/*
 * Function:
 *	plp_barchetta_phymod_symbol_field_filter
 * Purpose:
 *	Callback for Filtering fields based on current data view.
 * Parameters:
 *	symbol - symbol information
 *	fnames - list of all field names for this device
 *	encoding - key for decoding overlay
 *	cookie - context data
 * Returns:
 *      Non-zero if field should be filtered out (not displayed)
 * Notes:
 *      The filter key has the following syntax:
 *
 *        {[<keysrc>]:<keyfield>:<keyval>[|<keyval> ... ]}
 *
 *      Ideally the keysrc is the same data entry which is
 *      being decoded, and in this case it can left out, e.g.:
 *
 *        {:KEY_TYPEf:1}
 *
 *      This example encoding means that if KEY_TYPEf=1, then
 *      this field is valid for this view.
 *
 *      Note that a filed can be for multiple views, e.g.:
 *
 *        {:KEY_TYPEf:1|3}
 *
 *      This example encoding means that this field is valid
 *      if KEY_TYPEf=1 or KEY_TYPEf=3.
 *
 *      The special <keyval>=-1 means that this field is valid
 *      even if there is no context (cookie=NULL).
 *
 *      Note that this filter code does NOT support a <keysrc>
 *      which is different from the current data entry.
 */
int 
plp_barchetta_phymod_symbol_field_filter(const plp_barchetta_phymod_symbol_t *symbol, const char **fnames,
                              const char *encoding, void *cookie)
{
#if PHYMOD_CONFIG_INCLUDE_FIELD_NAMES == 1
    uint32_t *data = (uint32_t *)cookie;
    uint32_t val[PHYMOD_MAX_REG_WSIZE];
    plp_barchetta_phymod_field_info_t finfo; 
    char tstr[128];
    char *keyfield, *keyvals;
    char *ptr;
    int kval = -1;

    /* Do not filter if no (or unknown) encoding */
    if (encoding == NULL || *encoding != '{') {
        return 0;
    }

    /* Do not filter if encoding cannot be parsed */
    phymod_strlcpy(tstr, encoding, sizeof(tstr));
    ptr = tstr;
    if ((ptr = PHYMOD_STRCHR(ptr, ':')) == NULL) {
        return 0;
    }
    *ptr++ = 0;
    keyfield = ptr;
    if ((ptr = PHYMOD_STRCHR(ptr, ':')) == NULL) {
        return 0;
    }
    *ptr++ = 0;
    keyvals = ptr;

    /* Only show default view if no context */
    if (data == NULL) {
        return (PHYMOD_STRSTR(keyvals, "-1") == NULL) ? 1 : 0;
    }

    /* Look for <keyfield> in data entry */
    PHYMOD_SYMBOL_FIELDS_ITER_BEGIN(symbol->fields, finfo, fnames) {

        if (finfo.name && PHYMOD_STRCMP(finfo.name, keyfield) == 0) {
            /* Get normalized field value */
            PHYMOD_MEMSET(val, 0, sizeof(val));
            plp_barchetta_phymod_field_get(data, finfo.minbit, finfo.maxbit, val);
            kval = val[0];
            break;
        }

    } PHYMOD_SYMBOL_FIELDS_ITER_END(); 

    /* Check if current key matches any <keyval> in encoding */
    ptr = keyvals;
    while (ptr) {
        if (PHYMOD_STRTOUL(ptr, NULL, 0) == kval) {
            return 0;
        }
        if ((ptr = PHYMOD_STRCHR(ptr, '|')) != NULL) {
            ptr++;
        }
    }
#endif

    /* No match - filter this field */
    return 1; 
}
