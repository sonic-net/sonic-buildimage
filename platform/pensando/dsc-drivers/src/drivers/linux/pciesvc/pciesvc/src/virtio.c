// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "virtio.h"

#include "virtio_spec.h"

#define FMT64X  "0x%" PRIx64
#define FMT64U "%" PRIu64
#define FMT64S "%lu"

#define VIRTIO_DEV_REG_NOTIFY(fld)                                      \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        *do_notify = 1;                                                 \
        break;

#define VIRTIO_DEV_REG_RD(fld)                                          \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_mem_rd(addr, &val, VIRTIO_DEV_REG_SZ(fld));             \
        pciesvc_logdebug("%s: read %s addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X"", \
            pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val); \
        break;

#define VIRTIO_DEV_REG_RD_ARR(fld, arr_fld, idx_fld, idx_count)         \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, idx_fld),              \
                       &idx, VIRTIO_DEV_REG_SZ(idx_fld));               \
        if (idx < idx_count) {                                          \
            pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, arr_fld),          \
                           &val, VIRTIO_DEV_REG_SZ(arr_fld));           \
            pciesvc_logdebug("%s: read %s["FMT64U"] addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X"",\
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
        } else {                                                        \
            pciesvc_logerror("%s: read %s["FMT64U"] addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X" (out of bounds)",\
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
        }                                                               \
        break;

#define VIRTIO_DEV_REG_WR(fld)                               \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_logdebug("%s: write %s addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X"",\
            pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val); \
        pciesvc_mem_wr(addr, &val, VIRTIO_DEV_REG_SZ(fld));             \
        break;

#define VIRTIO_DEV_REG_WR_COND(fld, cond)                               \
        case VIRTIO_DEV_REG_OFF(fld):                                              \
            pciesvc_logdebug("%s: write %s addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X" cond %u", \
                pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val, cond);  \
            if (cond) {                                                            \
                pciesvc_mem_wr(addr, &val, VIRTIO_DEV_REG_SZ(fld));                \
            }                                                                      \
            break;

#define VIRTIO_DEV_REG_WR_PROC(fld, proc)                               \
        case VIRTIO_DEV_REG_OFF(fld):                                              \
            pciesvc_logdebug("%s: write %s addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X" proc %s", \
                pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val, #proc);  \
            proc(phwdev, addr, baroff, size, val);                                 \
            break;

#define VIRTIO_DEV_REG_WR_IGN(fld)                           \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_logdebug("%s: write %s addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X" ignore",\
            pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val); \
        break;

#define VIRTIO_DEV_REG_WR_ARR(fld, arr_fld, idx_fld, idx_count)         \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, idx_fld),              \
                       &idx, VIRTIO_DEV_REG_SZ(idx_fld));               \
        if (idx < idx_count) {                                          \
            pciesvc_logdebug("%s: write %s["FMT64U"] addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X"",\
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
            pciesvc_mem_wr(VIRTIO_DEV_REG_ADDR(base, arr_fld),          \
                           &val, VIRTIO_DEV_REG_SZ(arr_fld));           \
        } else {                                                        \
            pciesvc_logerror("%s: write %s["FMT64U"] addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X" (out of bounds)",\
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
        }                                                               \
        break;

#define VIRTIO_DEV_REG_WR_ARR_IGN(fld, arr_fld, idx_fld, idx_count)     \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, idx_fld),              \
                       &idx, VIRTIO_DEV_REG_SZ(idx_fld));               \
        if (idx < idx_count) {                                          \
            pciesvc_logdebug("%s: write %s["FMT64U"] addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X" ignore",\
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
            pciesvc_mem_wr(VIRTIO_DEV_REG_ADDR(base, arr_fld),          \
                           &val, VIRTIO_DEV_REG_SZ(arr_fld));           \
        } else {                                                        \
            pciesvc_logerror("%s: write %s["FMT64U"] addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X" ignore (out of bounds)",\
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
        }                                                               \
        break;

#define VIRTIO_DEV_REG_INSIDE(_fld, _offs, _sz)                         \
        (_offs >= VIRTIO_DEV_REG_OFF(_fld) &&                           \
         (_offs + _sz) <= VIRTIO_DEV_REG_OFF(_fld) + VIRTIO_DEV_REG_SZ(_fld))

u_int64_t
virtio_barrd(pciehwdev_t *phwdev, u_int64_t addr,
             const u_int64_t baroff, const size_t size,
             u_int8_t *do_notify)
{
    u_int64_t base = addr - baroff;
    u_int64_t val = 0;
    u_int64_t idx = 0;

    /* net_cfg */
    if (VIRTIO_DEV_REG_INSIDE(part1, baroff, size)) {
        pciesvc_mem_rd(addr, &val, size);
        pciesvc_logdebug("%s: read part1 addr "FMT64X" "
                         "off "FMT64U" size "FMT64S" val "FMT64X"",
                         pciehwdev_get_name(phwdev), addr, baroff, size, val);
        return val;
    }

    switch (baroff) {
    VIRTIO_DEV_REG_RD(cmn_cfg.device_feature_select);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.device_feature,
                          cmn_cfg.device_feature_cfg[idx],
                          cmn_cfg.device_feature_select,
                          VIRTIO_PCI_FEATURE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD(cmn_cfg.driver_feature_select);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.driver_feature,
                          cmn_cfg.driver_feature_cfg[idx],
                          cmn_cfg.driver_feature_select,
                          VIRTIO_PCI_FEATURE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD(cmn_cfg.config_msix_vector);
    VIRTIO_DEV_REG_RD(cmn_cfg.num_queues);
    VIRTIO_DEV_REG_RD(cmn_cfg.device_status);
    VIRTIO_DEV_REG_RD(cmn_cfg.config_generation);
    VIRTIO_DEV_REG_RD(cmn_cfg.queue_select);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_size,
                          queue_cfg[idx].queue_size,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_msix_vector,
                          queue_cfg[idx].queue_msix_vector,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_enable,
                          queue_cfg[idx].queue_enable,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_notify_off,
                          queue_cfg[idx].queue_notify_off,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_desc_lo,
                          queue_cfg[idx].queue_desc_lo,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_desc_hi,
                          queue_cfg[idx].queue_desc_hi,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_avail_lo,
                          queue_cfg[idx].queue_avail_lo,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_avail_hi,
                          queue_cfg[idx].queue_avail_hi,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_used_lo,
                          queue_cfg[idx].queue_used_lo,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(cmn_cfg.queue_cfg.queue_used_hi,
                          queue_cfg[idx].queue_used_hi,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    default:
        val = 0;
        pciesvc_logerror("%s: read addr "FMT64X" off "FMT64U" size "FMT64S" default ignore",
            pciehwdev_get_name(phwdev), addr, baroff, size);
        break;
    }

    return val;
}

static void
virtio_barwr_device_status(pciehwdev_t *phwdev, u_int64_t addr,
                           const u_int64_t baroff, const size_t size,
                           const u_int64_t val)
{
    u_int64_t base = addr - baroff;
    u_int8_t old = 0;

    pciesvc_mem_rd(addr, &old, VIRTIO_DEV_REG_SZ(cmn_cfg.device_status));

    if (!val) {
        // If pciemgr sees the transition nonzero -> zero, then nicmgr needs to
        // reset the device before device_status actually changes to zero.
        //
        // If the device status was already zero, and is written zero, there is
        // a race!  After writing zero, the driver would read zero and proceed.
        //
        // Nicmgr, when it handles the event, cannot depend on the current
        // value being the old value.  The driver may have written a new
        // nonzero value after proceeding.  If nicmgr assumes that the now
        // current nonzero value is the old value, it will falsely observe a
        // transition from nonzero to zero, which did not actually occur, and
        // reset the device at the same time as the driver is initializing.
        //
        // To avoid this, when pciemgr sees the transition from nonzero ->
        // zero, then pciemgr indicates so in need_reset.
        //
        // If nicmgr receives an event for the device_status register, it
        // should check need_reset.  If reset is needed, then clear need_reset,
        // reset the device, and then finally clear device_status.  If reset is
        // not needed, nicmgr should not reset the device, to avoid the race.
        //
        if (old) {
            old = 1;
            pciesvc_mem_wr(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.need_reset),
                           &old, sizeof(old));
        }

        // Eventually, nicmgr will update device_status.  Not here.
        return;
    }

    if ((val & VIRTIO_S_FEATURES_OK) && !(old & VIRTIO_S_FEATURES_OK)) {
        u_int32_t feature_lo = 0;
        u_int32_t feature_hi = 0;
        u_int64_t feature = 0;

        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.driver_feature_cfg[0]),
                       &feature_lo, sizeof(feature_lo));

        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.driver_feature_cfg[1]),
                       &feature_hi, sizeof(feature_hi));

        feature = (u_int64_t)feature_lo | ((u_int64_t)feature_hi << 32);

        pciesvc_loginfo("proc: features_ok "FMT64X"", feature);

        if (feature & VIRTIO_F_NOTIFICATION_DATA) {
            // Nicmgr initialized the queue configs with notify offsets in
            // the incr_pi_dbell range.  If this feature is selected,
            // modify the queue configs to ring the same doorbell via the
            // set_pi_dbell range.
            //
            // This is done here in pciesvc, so that the driver can read
            // the notify offset of queues _immediately_ after setting
            // features ok.

            const uint16_t notify_offset =
                offsetof(struct virtio_pci_notify_reg, set_pi_dbell)
                / VIRTIO_NOTIFY_MULTIPLIER;

            u_int16_t vq_i = 0, vq_count = 0;

            pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.num_queues),
                           &vq_count, sizeof(vq_count));

            pciesvc_logdebug("proc: vq_count %u notify_offset %u",
                             vq_count, notify_offset);

            for (; vq_i < vq_count; ++vq_i) {
                u_int64_t off_addr =
                    VIRTIO_DEV_REG_ADDR(base, queue_cfg[vq_i].queue_notify_off);

                u_int16_t off = 0;

                pciesvc_mem_rd(off_addr, &off, sizeof(off));
                off += notify_offset;
                pciesvc_mem_wr(off_addr, &off, sizeof(off));
            }
        }
    }

    pciesvc_mem_wr(addr, &val, VIRTIO_DEV_REG_SZ(cmn_cfg.device_status));
}

void
virtio_barwr(pciehwdev_t *phwdev, u_int64_t addr,
             const u_int64_t baroff, const size_t size, const u_int64_t val,
             u_int8_t *do_notify)
{
    u_int64_t base = addr - baroff;
    u_int64_t idx = 0;

    switch (baroff) {
    VIRTIO_DEV_REG_WR(cmn_cfg.device_feature_select);

    VIRTIO_DEV_REG_WR_ARR_IGN(cmn_cfg.device_feature,
                              cmn_cfg.device_feature_cfg[idx],
                              cmn_cfg.device_feature_select,
                              VIRTIO_PCI_FEATURE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR(cmn_cfg.driver_feature_select);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.driver_feature,
                          cmn_cfg.driver_feature_cfg[idx],
                          cmn_cfg.driver_feature_select,
                          VIRTIO_PCI_FEATURE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR(cmn_cfg.config_msix_vector);
    VIRTIO_DEV_REG_WR_IGN(cmn_cfg.num_queues);
    VIRTIO_DEV_REG_WR_PROC(cmn_cfg.device_status, virtio_barwr_device_status);
    VIRTIO_DEV_REG_WR_IGN(cmn_cfg.config_generation);
    VIRTIO_DEV_REG_WR(cmn_cfg.queue_select);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_size,
                          queue_cfg[idx].queue_size,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_msix_vector,
                          queue_cfg[idx].queue_msix_vector,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_enable,
                          queue_cfg[idx].queue_enable,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_notify_off,
                          queue_cfg[idx].queue_notify_off,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_desc_lo,
                          queue_cfg[idx].queue_desc_lo,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_desc_hi,
                          queue_cfg[idx].queue_desc_hi,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_avail_lo,
                          queue_cfg[idx].queue_avail_lo,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_avail_hi,
                          queue_cfg[idx].queue_avail_hi,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_used_lo,
                          queue_cfg[idx].queue_used_lo,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_WR_ARR(cmn_cfg.queue_cfg.queue_used_hi,
                          queue_cfg[idx].queue_used_hi,
                          cmn_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    default:
        pciesvc_logerror("%s: write addr "FMT64X" off "FMT64U" size "FMT64S" val "FMT64X" default ignore",
            pciehwdev_get_name(phwdev), addr, baroff, size, val);
        break;
    }

    switch (baroff) {
    VIRTIO_DEV_REG_NOTIFY(cmn_cfg.device_status);
    VIRTIO_DEV_REG_NOTIFY(cmn_cfg.queue_select);
    VIRTIO_DEV_REG_NOTIFY(cmn_cfg.queue_cfg.queue_enable);
    }
}
