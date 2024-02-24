/*
 * dfd_debug.c
 *
 * Function:Device framework driver debugging interface
 *
 * History
 *  v1.0    support <support> 2013-10-25  Initial version.
 */

#include <stdint.h>
#include <string.h>

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "dfd_fpga_debug.h"

#undef ARRAY_SIZE
#define ARRAY_SIZE(a)       (sizeof(a) /sizeof((a)[0]))
/* Debug switch storage of dfd module */
int g_dfd_fpga_debug = 0x0;

/**
 * dfd_fpga_pkt_debug_set - Debug switch setting interface of dfd module
 * @type: Types of debugging information
 * @enable: enable/Disable debugging information
 *
 * return 0 if success, otherwise reuturn -1.
 */
static int dfd_fpga_pkt_debug_set(int type, int enable)
{

    if (type >= DFD_DBG_CNT || type < 0) {
        DFD_ERROR("unknow dfd debug type=%d\n", type);
        return -1;
    }

    if (enable) {
        g_dfd_fpga_debug |= 1U << type;
    } else {
        g_dfd_fpga_debug &= ~(1U << type);
    }

    return 0;
}

void dfd_fpga_open_debug(int val)
{
    if (val == 1) {
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_ERR, 1);
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_WARN, 1);
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_VBOSE, 1);
    } else if (val == 2) {
        (void)dfd_fpga_pkt_debug_set(DFD_FLOCK_DBG_VBOSE, 1);
    } else if (val == 3) {
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_ERR, 1);
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_WARN, 1);
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_VBOSE, 1);
        (void)dfd_fpga_pkt_debug_set(DFD_FLOCK_DBG_VBOSE, 1);
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_DBG, 1);
    } else if (val == 4) {
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_DBG, 1);
    } else {
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_ERR, 0);
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_WARN, 0);
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_VBOSE, 0);
        (void)dfd_fpga_pkt_debug_set(DFD_FLOCK_DBG_VBOSE, 0);
        (void)dfd_fpga_pkt_debug_set(DFD_DBG_DBG, 0);
    }

    return;
}

void dfd_fpga_debug_init(void)
{
    FILE *fp;
    char buf[10] = {0};

    fp = fopen(DFD_DEBUG_FILE, "r");
    if (fp != NULL) {
        if (fgets(buf, sizeof(buf), fp) != NULL) {
            if (strstr(buf, DFD_DEBUG_SET_NO_WARN) != NULL) {
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_ERR, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_WARN, 0);
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_VBOSE, 0);
                (void)dfd_fpga_pkt_debug_set(DFD_FLOCK_DBG_VBOSE, 0);
            } else if (strstr(buf, DFD_DEBUG_SET_NO_VBOSE) != NULL) {
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_ERR, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_WARN, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_VBOSE, 0);
                (void)dfd_fpga_pkt_debug_set(DFD_FLOCK_DBG_VBOSE, 0);
            } else if (strstr(buf, DFD_DEBUG_SET_NO_FLOCK_VBOSE) != NULL) {
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_ERR, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_WARN, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_VBOSE, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_FLOCK_DBG_VBOSE, 0);
            } else if (strstr(buf, DFD_DEBUG_SET_ALL) != NULL) {
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_ERR, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_WARN, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_VBOSE, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_FLOCK_DBG_VBOSE, 1);
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_DBG, 1);
            } else if (strstr(buf, DFD_DEBUG_SET_DBG) != NULL) {
                (void)dfd_fpga_pkt_debug_set(DFD_DBG_DBG, 1);
            } else if (strstr(buf, DFD_DEBUG_SET_FLOCK) != NULL) {
                (void)dfd_fpga_pkt_debug_set(DFD_FLOCK_DBG_VBOSE, 1);
            }
        }

        fclose(fp);
    }

    return;
}