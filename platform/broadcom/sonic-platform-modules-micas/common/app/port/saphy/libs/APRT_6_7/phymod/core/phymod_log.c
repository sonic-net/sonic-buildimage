/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

#include <phymod/phymod.h>
char plp_aperta_log_level_string [4][20] = {
                               "BCM",
                               "PHYMOD",
                               "DRIVER",
                               "BUS"
                             };
#ifndef phymod_log_formatted_message
int
phymod_log_formatted_message( const char * format_p,
                                       ... )
{
    int rc;
    va_list args;
    va_start (args, format_p);
    rc  = vprintf (format_p, args);
    va_end (args);
    return rc;
}
#endif

