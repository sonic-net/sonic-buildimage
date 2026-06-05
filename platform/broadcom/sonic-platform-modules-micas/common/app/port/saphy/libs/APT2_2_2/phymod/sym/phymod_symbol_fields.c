/*
 * $Id$
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

/*******************************************************************************
 *
 * PHYMOD Symbol Field Routines
 *
 *
 ******************************************************************************/

#include <phymod/phymod_symbols.h>

/*
 * Function:
 *	plp_aperta2_phymod_field_get
 * Purpose:
 *	Extract multi-word field value from multi-word register/memory.
 * Parameters:
 *	entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 *      fbuf - buffer where to store extracted field value
 * Returns:
 *      Pointer to extracted field value.
 */
uint32_t *
plp_aperta2_phymod_field_get(const uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf)
{
    int i, wp, bp, len;

    bp = sbit;
    len = ebit - sbit + 1;

    wp = bp / 32;
    bp = bp & (32 - 1);
    i = 0;

    for (; len > 0; len -= 32, i++) {
        if (bp) {
            fbuf[i] = (entbuf[wp++] >> bp) & ((1 << (32 - bp)) - 1);
            if (len > (32 - bp)) {
                fbuf[i] |= entbuf[wp] << (32 - bp);
            }
        } else {
            fbuf[i] = entbuf[wp++];
        }
        if (len < 32) {
            fbuf[i] &= ((1 << len) - 1);
        }
    }

    return fbuf;
}

/*
 * Function:
 *	plp_aperta2_phymod_field_set
 * Purpose:
 *	Assign multi-word field value in multi-word register/memory.
 * Parameters:
 *	entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 *      fbuf - buffer with new field value
 * Returns:
 *      Nothing.
 */
int
plp_aperta2_phymod_field_set(uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf)
{
    uint32_t mask;
    int i, wp, bp, len;

    if (fbuf == NULL) {
        return -1;
    }

    bp = sbit;
    len = ebit - sbit + 1;
    if (len <= 0) {
        return -1;
    }

    wp = bp / 32;
    bp = bp & (32 - 1);
    i = 0;

    for (; len > 0; len -= 32, i++) {
        if (bp) {
            if (len < 32) {
                mask = (1 << len) - 1;
            } else {
                mask = ~0;
            }
            entbuf[wp] &= ~(mask << bp);
            entbuf[wp++] |= fbuf[i] << bp;
            if (len > (32 - bp)) {
                entbuf[wp] &= ~(mask >> (32 - bp));
                entbuf[wp] |= fbuf[i] >> (32 - bp) & ((1 << bp) - 1);
            }
        } else {
            if (len < 32) {
                mask = (1 << len) - 1;
                entbuf[wp] &= ~mask;
                entbuf[wp++] |= (fbuf[i] & mask) << bp;
            } else {
                entbuf[wp++] = fbuf[i];
            }
        }
    }
    return 0;
}

uint32_t *
plp_aperta2_phymod_field_info_decode(uint32_t* fp, plp_aperta2_phymod_field_info_t* finfo, const char** fnames)
{
    if(!fp) {
        return NULL; 
    }

    if(finfo) {
        /*
         * Single or Double Word Descriptor?
         */
        if(PHYMOD_SYMBOL_FIELD_EXT(*fp)) {
            /* Double Word */
            finfo->fid = PHYMOD_SYMBOL_FIELD_EXT_ID_GET(*fp); 
            finfo->maxbit = PHYMOD_SYMBOL_FIELD_EXT_MAX_GET(*(fp+1)); 
            finfo->minbit = PHYMOD_SYMBOL_FIELD_EXT_MIN_GET(*(fp+1)); 
        }       
        else {
            /* Single Word */
            finfo->fid = PHYMOD_SYMBOL_FIELD_ID_GET(*fp); 
            finfo->maxbit = PHYMOD_SYMBOL_FIELD_MAX_GET(*fp); 
            finfo->minbit = PHYMOD_SYMBOL_FIELD_MIN_GET(*fp); 
        }       

        if(fnames) {
            finfo->name = fnames[finfo->fid]; 
        }
        else {
            finfo->name = NULL; 
        }       
    }

    if(PHYMOD_SYMBOL_FIELD_LAST(*fp)) {
        return NULL; 
    }

    if(PHYMOD_SYMBOL_FIELD_EXT(*fp)) {
        return fp+2; 
    }

    return fp+1; 
}

uint32_t
plp_aperta2_phymod_field_info_count(uint32_t* fp)
{    
    int count = 0; 
    while(fp) {
        fp = plp_aperta2_phymod_field_info_decode(fp, NULL, NULL); 
        count++; 
    }   
    return count; 
}
