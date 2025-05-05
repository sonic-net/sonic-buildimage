// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "virtio.h"

#include "virtio_spec.h"

#define VIRTIO_LOG_FMT "addr 0x%"PRIx64" off %"PRIu64" size %lu val 0x%"PRIx64

#define VIRTIO_DEV_REG_NOTIFY(fld)                                      \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        *do_notify = 1;                                                 \
        break;

#define VIRTIO_DEV_REG_RD(fld)                                          \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_mem_rd(addr, &val, VIRTIO_DEV_REG_SZ(fld));             \
        pciesvc_logdebug("%s: read %s "VIRTIO_LOG_FMT"",                \
            pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val); \
        break;

#define VIRTIO_DEV_REG_RD_PROC(fld, proc)                               \
        case VIRTIO_DEV_REG_OFF(fld):                                   \
            proc(phwdev, addr - baroff, size, &val);                    \
            pciesvc_logdebug("%s: write %s "VIRTIO_LOG_FMT" proc %s",   \
                pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val, #proc);  \
            break;

#define VIRTIO_DEV_REG_RD_ARR(fld, arr_fld, idx_fld, idx_count)         \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, idx_fld),              \
                       &idx, VIRTIO_DEV_REG_SZ(idx_fld));               \
        if (idx < idx_count) {                                          \
            pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, arr_fld),          \
                           &val, VIRTIO_DEV_REG_SZ(arr_fld));           \
            pciesvc_logdebug("%s: read %s[%"PRIu64"] "VIRTIO_LOG_FMT"", \
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
        } else {                                                        \
            pciesvc_logdebug("%s: read %s[%"PRIu64"] "VIRTIO_LOG_FMT" (out of bounds)",\
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
        }                                                               \
        break;

#define VIRTIO_DEV_REG_WR(fld)                                          \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_logdebug("%s: write %s "VIRTIO_LOG_FMT"",               \
            pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val); \
        pciesvc_mem_wr(addr, &val, VIRTIO_DEV_REG_SZ(fld));             \
        break;

#define VIRTIO_DEV_REG_WR_PROC(fld, proc)                               \
        case VIRTIO_DEV_REG_OFF(fld):                                   \
            pciesvc_logdebug("%s: write %s "VIRTIO_LOG_FMT" proc %s",   \
                pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val, #proc);  \
            proc(phwdev, addr - baroff, size, val);                     \
            break;

#define VIRTIO_DEV_REG_WR_IGN(fld)                                      \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_logdebug("%s: write %s "VIRTIO_LOG_FMT" ignore",        \
            pciehwdev_get_name(phwdev), #fld, addr, baroff, size, val); \
        break;

#define VIRTIO_DEV_REG_WR_ARR(fld, arr_fld, idx_fld, idx_count)         \
    case VIRTIO_DEV_REG_OFF(fld):                                       \
        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, idx_fld),              \
                       &idx, VIRTIO_DEV_REG_SZ(idx_fld));               \
        if (idx < idx_count) {                                          \
            pciesvc_logdebug("%s: write %s[%"PRIu64"] "VIRTIO_LOG_FMT"",\
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
            pciesvc_mem_wr(VIRTIO_DEV_REG_ADDR(base, arr_fld),          \
                           &val, VIRTIO_DEV_REG_SZ(arr_fld));           \
        } else {                                                        \
            pciesvc_logdebug("%s: write %s[%"PRIu64"] "VIRTIO_LOG_FMT" (out of bounds)",\
                             pciehwdev_get_name(phwdev), #fld, idx,     \
                             VIRTIO_DEV_REG_ADDR(base, arr_fld),        \
                             baroff, size, val);                        \
        }                                                               \
        break;

#define VIRTIO_DEV_REG_INSIDE(_fld, _offs, _sz)                         \
        (_offs >= VIRTIO_DEV_REG_OFF(_fld) &&                           \
         (_offs + _sz) <= VIRTIO_DEV_REG_OFF(_fld) + VIRTIO_DEV_REG_SZ(_fld))


static void
virtio_legacy_barrd_isr_status(pciehwdev_t *phwdev, const u_int64_t base,
                               const size_t size, u_int64_t *val)
{
    u_int8_t isr_reset = 0;

    // TODO - first entry in isr_cfg region? Or somewhere else?
    pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, isr_cfg), val, 1);

    // clear-on-read
    if (*val) {
        pciesvc_mem_wr(VIRTIO_DEV_REG_ADDR(base, isr_cfg), &isr_reset, 1);
    }
}

static u_int64_t
virtio_legacy_barrd(pciehwdev_t *phwdev, u_int64_t addr,
                    const u_int64_t baroff, const size_t size,
                    u_int8_t *do_notify)
{
    u_int64_t base = addr - baroff;
    u_int64_t val = 0;
    u_int64_t idx = 0;
    u_int64_t offset;

    /* net_cfg */
    if (phwdev->msix_en) {
        if (VIRTIO_DEV_REG_INSIDE(legacy_cfg.net_cfg_msix, baroff, size)) {
            offset = baroff - VIRTIO_DEV_REG_OFF(legacy_cfg.net_cfg_msix);
            pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, net_cfg) + offset, &val, size);
            return val;
        }
    } else {
        if (VIRTIO_DEV_REG_INSIDE(legacy_cfg.net_cfg, baroff, size)) {
            offset = baroff - VIRTIO_DEV_REG_OFF(legacy_cfg.net_cfg);
            pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, net_cfg) + offset, &val, size);
            return val;
        }
    }

    /* msi-x */
    if (phwdev->msix_en &&
        VIRTIO_DEV_REG_INSIDE(legacy_cfg.msix_cfg, baroff, size)) {
        switch (baroff) {
        // Indirect scalar
        case VIRTIO_DEV_REG_OFF(legacy_cfg.msix_cfg.config_msix_vector):
            pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.config_msix_vector),
                           &val, VIRTIO_DEV_REG_SZ(cmn_cfg.config_msix_vector));
            break;

        VIRTIO_DEV_REG_RD_ARR(legacy_cfg.msix_cfg.queue_msix_vector,
                              queue_cfg[idx].queue_msix_vector,
                              legacy_cfg.queue_select,
                              VIRTIO_PCI_QUEUE_SELECT_COUNT);
        default:
                break;
        }

        return val;
    }

    switch (baroff) {
    // NB - select is always 0 in legacy case
    VIRTIO_DEV_REG_RD_ARR(legacy_cfg.device_feature,
                          cmn_cfg.device_feature_cfg[idx],
                          cmn_cfg.device_feature_select, 1);

    // NB - select is always 0 in legacy case
    VIRTIO_DEV_REG_RD_ARR(legacy_cfg.driver_feature,
                          cmn_cfg.driver_feature_cfg[idx],
                          cmn_cfg.driver_feature_select, 1);

    VIRTIO_DEV_REG_RD_ARR(legacy_cfg.queue_address,
                          queue_cfg[idx].queue_desc_lo,
                          legacy_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD_ARR(legacy_cfg.queue_size,
                          queue_cfg[idx].queue_size,
                          legacy_cfg.queue_select,
                          VIRTIO_PCI_QUEUE_SELECT_COUNT);

    VIRTIO_DEV_REG_RD(legacy_cfg.queue_select);

    VIRTIO_DEV_REG_RD(legacy_cfg.queue_notify);

    // Indirect scalar
    case VIRTIO_DEV_REG_OFF(legacy_cfg.device_status):
        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.device_status),
                       &val, VIRTIO_DEV_REG_SZ(cmn_cfg.device_status));
        break;

    VIRTIO_DEV_REG_RD_PROC(legacy_cfg.isr_status,
                           virtio_legacy_barrd_isr_status);
    default:
            break;
    }

    return val;
}

u_int64_t
virtio_barrd(pciehwdev_t *phwdev, u_int64_t addr,
             const u_int64_t baroff, const size_t size,
             u_int8_t *do_notify)
{
    u_int64_t base = addr - baroff;
    u_int64_t val = 0;
    u_int64_t idx = 0;

    /* legacy_cfg */
    if (VIRTIO_DEV_REG_INSIDE(part0, baroff, size)) {
        val = virtio_legacy_barrd(phwdev, addr, baroff, size, do_notify);
        pciesvc_logerror("%s: read part0 "VIRTIO_LOG_FMT"",
                         pciehwdev_get_name(phwdev), addr, baroff, size, val);
        return val;
    }

    /* net_cfg */
    if (VIRTIO_DEV_REG_INSIDE(part2, baroff, size)) {
        pciesvc_mem_rd(addr, &val, size);
        pciesvc_logdebug("%s: read part2 "VIRTIO_LOG_FMT"",
                         pciehwdev_get_name(phwdev), addr, baroff, size, val);
        return val;
    }

    /* isr_cfg */
    if (VIRTIO_DEV_REG_INSIDE(isr_cfg, baroff, size)) {
        val = 0;
        pciesvc_logdebug("%s: read isr_cfg "VIRTIO_LOG_FMT"",
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
        pciesvc_logdebug("%s: read "VIRTIO_LOG_FMT" default ignore",
                         pciehwdev_get_name(phwdev), addr, baroff, size, val);
        break;
    }

    return val;
}

// Nicmgr initialized the queue configs with notify offsets in the incr_pi_dbell range.
// Here we modify the offsets depending on which features are selected, to change the
// doorbell behavior.
//
// Notification Data wants to use SET_PI instead of INC_PI.  Split VQ wants to use
// SCHED_SET instead of SCHED_NONE for RX.
//
// This is done here in pciesvc, so that the driver can read the notify offset of queues
// _immediately_ after setting features ok.
static void
virtio_barwr_config_notif_data(pciehwdev_t *phwdev,
                               const u_int64_t base,
                               const u_int64_t features)
{
    const uint16_t notify_offset = virtio_features_notify_offset(features);
    u_int16_t vq_i, vq_count;

    pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.num_queues),
                   &vq_count, sizeof(vq_count));

    pciesvc_logdebug("proc: vq_count %u notify_offset %u",
                     vq_count, notify_offset);

    for (vq_i = 0; vq_i < vq_count; ++vq_i) {
        u_int64_t off_addr =
            VIRTIO_DEV_REG_ADDR(base, queue_cfg[vq_i].queue_notify_off);

        u_int16_t off =
            notify_offset + VIRTIO_VQID_NOTIFY_OFF(vq_i);

        pciesvc_mem_wr(off_addr, &off, sizeof(off));
    }
}

static void
virtio_barwr_device_status(pciehwdev_t *phwdev, const u_int64_t base,
                           const size_t size, const u_int64_t val)
{
    u_int8_t old = 0;

    pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.device_status),
                   &old, VIRTIO_DEV_REG_SZ(cmn_cfg.device_status));

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

    // FEATURES_OK is only defined for version 1, so this is not entered
    // for legacy devices. Legacy devices react to feature bit changes when
    // the register is written - see virtio_legacy_barwr_driver_feature().
    if ((val & VIRTIO_S_FEATURES_OK) && !(old & VIRTIO_S_FEATURES_OK)) {
        u_int64_t features = 0;
        u_int64_t hw_features = 0;
        u_int64_t unsupp_features;

        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.active_features),
                       &features, sizeof(features));

        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, ident.hw_features),
                       &hw_features, sizeof(hw_features));

        unsupp_features = features & ~hw_features;
        if (unsupp_features) {
            pciesvc_logdebug("proc: request for unsupported features %"PRIx64,
                             unsupp_features);
            // Don't update the status. Driver will see that the
            // FEATURES_OK bit was not acknowledged by the device.
            return;
        }

        pciesvc_loginfo("proc: features_ok 0x%"PRIx64"", features);

        // Now react to any feature bit changes, as needed

        virtio_barwr_config_notif_data(phwdev, base, features);
    }

    pciesvc_mem_wr(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.device_status), &val,
                   VIRTIO_DEV_REG_SZ(cmn_cfg.device_status));
}

static void
virtio_barwr_driver_feature(pciehwdev_t *phwdev, const u_int64_t base,
                            const size_t size, const u_int64_t val)
{
    u_int64_t reg_addr, reg_size;
    u_int64_t idx = 0;
    u_int8_t status = 0;

    // Verify that device is in the correct state
    pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.device_status), &status,
                   VIRTIO_DEV_REG_SZ(cmn_cfg.device_status));

    if (status & VIRTIO_S_FEATURES_OK) {
        pciesvc_logdebug("proc: ignoring late features write");
        return;
    }

    pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.driver_feature_select),
                   &idx,
                   VIRTIO_DEV_REG_SZ(cmn_cfg.driver_feature_select));

    reg_addr = VIRTIO_DEV_REG_ADDR(base, cmn_cfg.driver_feature_cfg[idx]);
    reg_size = VIRTIO_DEV_REG_SZ(cmn_cfg.driver_feature_cfg[idx]);

    if (idx < VIRTIO_PCI_FEATURE_SELECT_COUNT) {
        pciesvc_logdebug("%s: write cmn_cfg.driver_feature[%"PRIu64"] "VIRTIO_LOG_FMT"",
                         pciehwdev_get_name(phwdev), idx, reg_addr,
                         reg_addr - base, size, val);
        // Actually perform the requested write
        pciesvc_mem_wr(reg_addr, &val, reg_size);

        // If the driver negotiates F_MRG_RXBUF, switch to the larger mtu,
        // this is required /before/ the driver sends features ok.
        if (idx == 0 && (val & VIRTIO_NET_F_MRG_RXBUF)) {
            u_int16_t mtu = 0;
            pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.mtu_mrg_rxbuf),
                           &mtu, sizeof(mtu));
            pciesvc_mem_wr(VIRTIO_DEV_REG_ADDR(base, net_cfg.mtu),
                           &mtu, sizeof(mtu));
        }
    } else {
        pciesvc_logdebug("%s: write cmn_cfg.driver_feature[%"PRIu64"] "VIRTIO_LOG_FMT" (out of bounds)",
                         pciehwdev_get_name(phwdev), idx, reg_addr,
                         reg_addr - base, size, val);
    }
}

static void
virtio_legacy_barwr_driver_feature(pciehwdev_t *phwdev, const u_int64_t base,
                                   const size_t size, const u_int64_t val)
{
    u_int64_t reg_addr, reg_size;
    u_int64_t hw_features = 0;
    u_int64_t unsupp_features;
    u_int8_t status = 0;

    // Verify that device is in the correct state
    pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.device_status),
                   &status, sizeof(status));

    // NB: There is no FEATURES_OK bit in the legacy interface
    if (status & VIRTIO_S_DRIVER_OK) {
        pciesvc_logdebug("proc: ignoring late features write");
        return;
    }

    pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, ident.hw_features),
                   &hw_features, sizeof(hw_features));

    unsupp_features = val & ~hw_features;
    if (unsupp_features) {
        pciesvc_logdebug("proc: request for unsupported features %"PRIx64"",
                         unsupp_features);
        // NB: There is no NEEDS_RESET bit in the legacy interface
        return;
    }

    // Now react to any feature bit changes, as needed

    // If the driver negotiates F_MRG_RXBUF, switch to the larger mtu
    if (val & VIRTIO_NET_F_MRG_RXBUF) {
        u_int16_t mtu = 0;
        pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.mtu_mrg_rxbuf),
                       &mtu, sizeof(mtu));
        pciesvc_mem_wr(VIRTIO_DEV_REG_ADDR(base, net_cfg.mtu),
                       &mtu, sizeof(mtu));
    }

    // NB: driver_feature_select is always 0 in legacy case
    reg_addr = VIRTIO_DEV_REG_ADDR(base, cmn_cfg.driver_feature_cfg[0]);
    reg_size = VIRTIO_DEV_REG_SZ(cmn_cfg.driver_feature_cfg[0]);

    pciesvc_logdebug("%s: write cmn_cfg.driver_feature[%"PRIu64"] "VIRTIO_LOG_FMT"",
                     pciehwdev_get_name(phwdev), (u_int64_t)0, reg_addr,
                     reg_addr - base, size, val);
    // Actually perform the requested write
    pciesvc_mem_wr(reg_addr, &val, reg_size);
}

// 2.7.2 The driver writes a single address pointing to the beginning of
// struct vring; from there the device calculates the rest of the addresses.
// NB: The queue_address is only 44 bits (32 bits * 4096)
static void
virtio_legacy_barwr_queue_address(pciehwdev_t *phwdev, const u_int64_t base,
                                  const size_t size, const u_int64_t val)
{
    u_int64_t idx = 0;
    u_int64_t reg_addr = VIRTIO_DEV_REG_ADDR(base, legacy_cfg.queue_address);
    size_t reg_size = VIRTIO_DEV_REG_SZ(queue_cfg[idx].queue_desc_lo);
    u_int64_t vq_addr = val << 12;
    u_int64_t vq_addr_hi;
    u_int64_t vq_size = 0;

    pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, legacy_cfg.queue_select),
                   &idx, VIRTIO_DEV_REG_SZ(legacy_cfg.queue_select));

    if (idx >= VIRTIO_PCI_QUEUE_SELECT_COUNT) {
        pciesvc_logdebug("%s: write legacy_cfg.queue_address[%"PRIu64"] "VIRTIO_LOG_FMT" (out of bounds)",
                         pciehwdev_get_name(phwdev), idx, reg_addr,
                         reg_addr - base, reg_size, val);
        return;
    }

    pciesvc_mem_rd(VIRTIO_DEV_REG_ADDR(base, queue_cfg[idx].queue_size),
                   &vq_size, VIRTIO_DEV_REG_SZ(queue_cfg[idx].queue_size));

    reg_addr = VIRTIO_DEV_REG_ADDR(base, queue_cfg[idx].queue_desc_lo);
    pciesvc_mem_wr(reg_addr, &vq_addr, reg_size);

    vq_addr_hi = vq_addr >> 32;
    reg_addr = VIRTIO_DEV_REG_ADDR(base, queue_cfg[idx].queue_desc_hi);
    pciesvc_mem_wr(reg_addr, &vq_addr_hi, reg_size);

    vq_addr += (16 * vq_size); /* sizeof(struct vring_desc) */
    reg_addr = VIRTIO_DEV_REG_ADDR(base, queue_cfg[idx].queue_avail_lo);
    pciesvc_mem_wr(reg_addr, &vq_addr, reg_size);

    vq_addr_hi = vq_addr >> 32;
    reg_addr = VIRTIO_DEV_REG_ADDR(base, queue_cfg[idx].queue_avail_hi);
    pciesvc_mem_wr(reg_addr, &vq_addr_hi, reg_size);

    vq_addr += (4 + 2 * vq_size + 2); /* sizeof(struct vring_avail) */
    vq_addr = align_to(vq_addr, 4096);
    reg_addr = VIRTIO_DEV_REG_ADDR(base, queue_cfg[idx].queue_used_lo);
    pciesvc_mem_wr(reg_addr, &vq_addr, reg_size);

    vq_addr_hi = vq_addr >> 32;
    reg_addr = VIRTIO_DEV_REG_ADDR(base, queue_cfg[idx].queue_used_hi);
    pciesvc_mem_wr(reg_addr, &vq_addr_hi, reg_size);
}

static void
virtio_legacy_barwr(pciehwdev_t *phwdev, u_int64_t addr,
                    const u_int64_t baroff, const size_t size,
                    const u_int64_t val, u_int8_t *do_notify)
{
    u_int64_t base = addr - baroff;
    u_int64_t idx = 0;

    /* msi-x */
    if (phwdev->msix_en &&
        VIRTIO_DEV_REG_INSIDE(legacy_cfg.msix_cfg, baroff, size)) {
        switch (baroff) {
        // Indirect scalar
        case VIRTIO_DEV_REG_OFF(legacy_cfg.msix_cfg.config_msix_vector):
            pciesvc_mem_wr(VIRTIO_DEV_REG_ADDR(base, cmn_cfg.config_msix_vector),
                           &val, VIRTIO_DEV_REG_SZ(cmn_cfg.config_msix_vector));
            break;

        VIRTIO_DEV_REG_WR_ARR(legacy_cfg.msix_cfg.queue_msix_vector,
                              queue_cfg[idx].queue_msix_vector,
                              legacy_cfg.queue_select,
                              VIRTIO_PCI_QUEUE_SELECT_COUNT);
        default:
            break;
        }

        return;
    }

    switch (baroff) {
    VIRTIO_DEV_REG_WR_IGN(legacy_cfg.device_feature);

    VIRTIO_DEV_REG_WR_PROC(legacy_cfg.driver_feature,
                           virtio_legacy_barwr_driver_feature);

    VIRTIO_DEV_REG_WR_PROC(legacy_cfg.queue_address,
                           virtio_legacy_barwr_queue_address);

    VIRTIO_DEV_REG_WR_IGN(legacy_cfg.queue_size);

    VIRTIO_DEV_REG_WR(legacy_cfg.queue_select);

    VIRTIO_DEV_REG_WR(legacy_cfg.queue_notify);

    VIRTIO_DEV_REG_WR_PROC(legacy_cfg.device_status,
                           virtio_barwr_device_status);

    VIRTIO_DEV_REG_WR_IGN(legacy_cfg.isr_status);

    default:
        pciesvc_logdebug("%s: write "VIRTIO_LOG_FMT" default ignore",
            pciehwdev_get_name(phwdev), addr, baroff, size, val);
        break;
    }

    switch (baroff) {
    VIRTIO_DEV_REG_NOTIFY(legacy_cfg.device_status);
    VIRTIO_DEV_REG_NOTIFY(legacy_cfg.queue_select);
    VIRTIO_DEV_REG_NOTIFY(legacy_cfg.queue_address); // enable
    VIRTIO_DEV_REG_NOTIFY(legacy_cfg.queue_notify);
    }
}

void
virtio_barwr(pciehwdev_t *phwdev, u_int64_t addr,
             const u_int64_t baroff, const size_t size, const u_int64_t val,
             u_int8_t *do_notify)
{
    u_int64_t base = addr - baroff;
    u_int64_t idx = 0;

    /* legacy_cfg */
    if (VIRTIO_DEV_REG_INSIDE(part0, baroff, size)) {
        virtio_legacy_barwr(phwdev, addr, baroff, size, val, do_notify);
        pciesvc_logerror("%s: write part0 "VIRTIO_LOG_FMT"",
                         pciehwdev_get_name(phwdev), addr, baroff, size, val);
        return;
    }

    switch (baroff) {
    VIRTIO_DEV_REG_WR(cmn_cfg.device_feature_select);

    VIRTIO_DEV_REG_WR_IGN(cmn_cfg.device_feature);

    VIRTIO_DEV_REG_WR(cmn_cfg.driver_feature_select);

    VIRTIO_DEV_REG_WR_PROC(cmn_cfg.driver_feature,
                           virtio_barwr_driver_feature);

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
        pciesvc_logdebug("%s: write "VIRTIO_LOG_FMT" default ignore",
            pciehwdev_get_name(phwdev), addr, baroff, size, val);
        break;
    }

    switch (baroff) {
    VIRTIO_DEV_REG_NOTIFY(cmn_cfg.device_status);
    VIRTIO_DEV_REG_NOTIFY(cmn_cfg.queue_select);
    VIRTIO_DEV_REG_NOTIFY(cmn_cfg.queue_cfg.queue_enable);
    }
}
