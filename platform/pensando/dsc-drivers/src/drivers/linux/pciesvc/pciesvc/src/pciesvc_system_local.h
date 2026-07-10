/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2022, Pensando Systems Inc.
 */

#ifndef __PCIESVC_SYSTEM_LOCAL_H__
#define __PCIESVC_SYSTEM_LOCAL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#define __USE_GNU
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <endian.h>
#include <sys/param.h>
#include <linux/pci_regs.h>

#include "platform/pal/include/pal.h"
#include "platform/pciemgr/include/pciemgr.h"
#include "platform/pciesvc/include/pciesvc.h"

#define pciesvc_shmem_get       pciehw_get_shmem
#define pciesvc_hwmem_get       pciehw_get_hwmem
#define pciesvc_vtop            pal_mem_vtop
#define pciesvc_reg_rd32        pal_reg_rd32
#define pciesvc_reg_wr32        pal_reg_wr32
#define pciesvc_pciepreg_rd32   pal_pciepreg_rd32
#define pciesvc_pciepreg_wr32   pal_pciepreg_wr32
#define pciesvc_mem_barrier     PAL_barrier
#define pciesvc_memset          memset
#define pciesvc_memcpy          memcpy
#define pciesvc_memcpy_toio     memcpy
#define pciesvc_assert          assert
#define pciesvc_usleep          usleep
#define pciesvc_ffs             ffs
#define pciesvc_ffsll           ffsll
#define pciesvc_snprintf        snprintf
#define pciesvc_vsnprintf       vsnprintf

#define pciesvc_htobe32         htobe32
#define pciesvc_be32toh         be32toh
#define pciesvc_htobe16         htobe16
#define pciesvc_be16toh         be16toh
#define pciesvc_htole32         htole32
#define pciesvc_le32toh         le32toh

#define pciesvc_loglocal        pciesys_loginfo

#define pciesvc_logdebug(args...) do { \
    if (PCIESVC_LOGPRI_DEBUG >= pciesvc_log_level) { \
        pciesys_logdebug(args); \
    } } while(0)

#define pciesvc_loginfo(args...) do { \
    if (PCIESVC_LOGPRI_INFO >= pciesvc_log_level) { \
        pciesys_loginfo(args); \
    } } while(0)

#define pciesvc_logwarn(args...) do { \
    if (PCIESVC_LOGPRI_WARN >= pciesvc_log_level) { \
        pciesys_logwarn(args); \
    } } while(0)

#define pciesvc_logerror(args...) do { \
    if (PCIESVC_LOGPRI_ERROR >= pciesvc_log_level) { \
        pciesys_logerror(args); \
    } } while(0)

typedef union {
    u_int32_t l;
    u_int16_t h[2];
    u_int8_t  b[4];
} iodata_t;

static inline int
pciesvc_mem_rd(const uint64_t pa, void *buf, const size_t sz)
{
    uint64_t pa_aligned;
    uint8_t idx;
    iodata_t v;

    switch (sz) {
    case 1:
        pa_aligned = pa & ~0x3;
        idx = pa & 0x3;
        v.l = pal_reg_rd32(pa_aligned);
        *(uint8_t *)buf = v.b[idx];
        break;
    case 2:
        pa_aligned = pa & ~0x3;
        idx = (pa & 0x3) >> 1;
        v.l = pal_reg_rd32(pa_aligned);
        *(uint16_t *)buf = v.h[idx];
        break;
    case 4:
    case 8:
        pal_reg_rd32w(pa, (uint32_t *)buf, sz >> 2);
        break;
    default:
        return -1;
    }
    return 0;
}

static inline void
pciesvc_mem_wr(const uint64_t pa, const void *buf, const size_t sz)
{
    uint64_t pa_aligned;
    uint8_t idx;
    iodata_t v;

    switch (sz) {
    case 1:
        pa_aligned = pa & ~0x3;
        idx = pa & 0x3;
        v.l = pal_reg_rd32(pa_aligned);
        v.b[idx] = *(uint8_t *)buf;
        pal_reg_wr32(pa_aligned, v.l);
        break;
    case 2:
        pa_aligned = pa & ~0x3;
        idx = (pa & 0x3) >> 1;
        v.l = pal_reg_rd32(pa_aligned);
        v.h[idx] = *(uint16_t *)buf;
        pal_reg_wr32(pa_aligned, v.l);
        break;
    case 4:
    case 8:
        pal_reg_wr32w(pa, (uint32_t *)buf, sz >> 2);
        break;
    default:
        break;
    }
}

static inline int
pciesvc_event_handler(const void *evdata, const size_t evsize)
{
    pciehdev_event(evdata);
    return 0;
}

static inline void
pciesvc_debug_cmd(uint32_t *valp)
{
    uint32_t delayus = *valp;

    if (delayus) {
        pciesvc_logdebug("cfgrd delay %uus\n", delayus);
        pciesvc_usleep(delayus);
    }
}

#endif /* __PCIESVC_SYSTEM_LOCAL_H__ */
