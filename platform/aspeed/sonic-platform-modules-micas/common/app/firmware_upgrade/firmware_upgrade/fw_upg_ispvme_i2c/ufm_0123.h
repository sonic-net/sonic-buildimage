#ifndef XO2_JEDEC_ufm_0123_H
#define XO2_JEDEC_ufm_0123_H

#include "xo2_dev.h"    // for XO2_JEDEC_t def

#define UFM_CFG_DATA_MAX_LEN                (1024 * 1024)
#define UFM_UFM_DATA_MAX_LEN                (1024 * 1024)

#define CPLD_DESIGN_NUMBER                  (1)
#define CPLD_DESIGN_WORD                    (2)

int ufm_convert_jed(char *jed_file, XO2Devices_t cpld_type, XO2_JEDEC_t *pjedec_t);

#endif
