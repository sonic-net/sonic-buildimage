// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017-2018,2020-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "pcietlp.h"
#include "indirect.h"
#include "notify.h"
#include "serial.h"
#include "virtio.h"
#include "pmt.h"

static pciehwbar_t *
pciehw_bar_get(pciehwdev_t *phwdev, const int idx)
{
    if (idx < 0 || idx > 7) return NULL;
    if (idx == 7) return &phwdev->rombar;
    return &phwdev->bar[idx];
}

u_int64_t
pciehw_bar_getsize(pciehwbar_t *phwbar)
{
    if (!phwbar->valid) return 0;
    return phwbar->size;
}

void
pciehw_bar_setaddr(pciehwbar_t *phwbar, const u_int64_t addr)
{
    if (phwbar->addr != addr) {
        phwbar->addr = addr;
        pciehw_pmt_setaddr(phwbar, addr);
    }
}

void
pciehw_bar_load(pciehwdev_t *phwdev, pciehwbar_t *phwbar)
{
    if (!phwbar->loaded) {
#ifdef PCIEMGR_DEBUG
        const pciehwdev_t *phwdev = pciehwdev_get(owner);
        pciesvc_logdebug("%s: bar %d pmt %d loaded\n",
                         pciehwdev_get_name(phwdev),
                         phwbar->cfgidx, phwbar->pmtb);
        pciehwdev_put(phwdev, CLEAN);
#endif
        phwbar->bdf = pciehwdev_get_hostbdf(phwdev);
        pciehw_bar_load_pmts(phwbar);
        phwbar->loaded = 1;
    }
}

void
pciehw_bar_unload(pciehwdev_t *phwdev, pciehwbar_t *phwbar)
{
    if (phwbar->loaded) {
#ifdef PCIEMGR_DEBUG
        const pciehwdev_t *phwdev = pciehwdev_get(owner);
        pciesvc_logdebug("%s: bar %d pmt %d unloaded\n",
                         pciehwdev_get_name(phwdev),
                         phwbar->cfgidx, phwbar->pmtb);
        pciehwdev_put(phwdev, CLEAN);
#endif
        pciehw_bar_unload_pmts(phwbar);
        phwbar->loaded = 0;
    }
}

void
pciehw_bar_enable(pciehwdev_t *phwdev, pciehwbar_t *phwbar, const int on)
{
    if (on) {
        pciehw_bar_load(phwdev, phwbar);
    } else {
        pciehw_bar_unload(phwdev, phwbar);
    }
}

static void
pciehw_barrw_notify(const pciesvc_event_t evtype,
                    const int port,
                    pciehwdev_t *phwdev,
                    const pcie_stlp_t *stlp,
                    const tlpauxinfo_t *info,
                    const pciehw_spmt_t *spmt)
{
    const pciehwbar_t *phwbar = pciehw_bar_get(phwdev, spmt->cfgidx);
    pciesvc_eventdata_t evd;
    pciesvc_memrw_notify_t *memrw;

    pciesvc_memset(&evd, 0, sizeof(evd));
    evd.evtype = evtype;
    evd.port = port;
    evd.lif = phwdev->lifb;
    memrw = &evd.memrw_notify;
    memrw->baraddr = stlp->addr;
    memrw->cfgidx = spmt->cfgidx;
    memrw->baroffset = stlp->addr - phwbar->addr;
    memrw->size = stlp->size;
    memrw->localpa = info->direct_addr;
    memrw->data = stlp->data; /* data, if write or hacked in */
    pciesvc_event_handler(&evd, sizeof(evd));
}

void
pciehw_barrd_notify(const int port, notify_entry_t *nentry)
{
    const tlpauxinfo_t *info = &nentry->info;
    const pciehw_spmt_t *spmt = pciesvc_spmt_get(info->pmti);
    pciehwdev_t *phwdev = pciehwdev_get(spmt->owner + info->vfid);
    pcie_stlp_t stlpbuf, *stlp = &stlpbuf;

    pcietlp_decode(stlp, nentry->rtlp, sizeof(nentry->rtlp));

    pciehw_barrw_notify(PCIESVC_EV_MEMRD_NOTIFY,
                        port, phwdev, stlp, info, spmt);

    pciehwdev_put(phwdev, CLEAN);
    pciesvc_spmt_put(spmt, CLEAN);
}

void
pciehw_barwr_notify(const int port, notify_entry_t *nentry)
{
    const tlpauxinfo_t *info = &nentry->info;
    const pciehw_spmt_t *spmt = pciesvc_spmt_get(info->pmti);
    pciehwdev_t *phwdev = pciehwdev_get(spmt->owner + info->vfid);
    pcie_stlp_t stlpbuf, *stlp = &stlpbuf;

    pcietlp_decode(stlp, nentry->rtlp, sizeof(nentry->rtlp));

    pciehw_barrw_notify(PCIESVC_EV_MEMWR_NOTIFY,
                        port, phwdev, stlp, info, spmt);

    pciehwdev_put(phwdev, CLEAN);
    pciesvc_spmt_put(spmt, CLEAN);
}

void
pciehw_barrd_indirect(const int port, indirect_entry_t *ientry)
{
    const tlpauxinfo_t *info = &ientry->info;
    const pciehw_spmt_t *spmt = pciesvc_spmt_get(info->pmti);
    pciehwdev_t *phwdev = pciehwdev_get(spmt->owner + info->vfid);
    const pciehwbar_t *phwbar = pciehw_bar_get(phwdev, spmt->cfgidx);

    switch (phwbar->hnd) {

    case PCIEHW_BARHND_SERIAL: {
        pcie_stlp_t stlpbuf, *stlp = &stlpbuf;
        u_int64_t baroff;

        pcietlp_decode(stlp, ientry->rtlp, sizeof(ientry->rtlp));
        baroff = stlp->addr - phwbar->addr;
        ientry->data[0] = serial_barrd(phwdev, baroff, info->direct_size);
        break;
    }

    case PCIEHW_BARHND_VIRTIO: {
        pcie_stlp_t stlpbuf, *stlp = &stlpbuf;
        u_int64_t baroff;
        u_int8_t do_notify = 0;

        pcietlp_decode(stlp, ientry->rtlp, sizeof(ientry->rtlp));
        baroff = stlp->addr - phwbar->addr;
        ientry->data[0] = virtio_barrd(phwdev, info->direct_addr, baroff,
                            info->direct_size, &do_notify);

        stlp->data = ientry->data[0]; // HACK so logging shows real value

        if (do_notify) {
            pciehw_barrw_notify(PCIESVC_EV_MEMRD_NOTIFY,
                       port, phwdev, stlp, info, spmt);
        }

        break;
    }

    default: {
        u_int64_t pa = info->direct_addr;
        size_t sz = info->direct_size;

        pciesvc_mem_rd(pa, ientry->data, sz);
        break;
    }
    }
    pciehwdev_put(phwdev, CLEAN);
    pciesvc_spmt_put(spmt, CLEAN);

    pciehw_indirect_complete(ientry);
}

void
pciehw_barwr_indirect(const int port, indirect_entry_t *ientry)
{
    const tlpauxinfo_t *info = &ientry->info;
    const pciehw_spmt_t *spmt = pciesvc_spmt_get(info->pmti);
    pciehwdev_t *phwdev = pciehwdev_get(spmt->owner + info->vfid);
    const pciehwbar_t *phwbar = pciehw_bar_get(phwdev, spmt->cfgidx);
    pcie_stlp_t stlpbuf, *stlp = &stlpbuf;

    pcietlp_decode(stlp, ientry->rtlp, sizeof(ientry->rtlp));

    switch (phwbar->hnd) {

    case PCIEHW_BARHND_SERIAL: {
        const u_int64_t baroff = stlp->addr - phwbar->addr;
        const u_int32_t size = info->direct_size;

        serial_barwr(phwdev, baroff, size, stlp->data);
        break;
    }

    case PCIEHW_BARHND_VIRTIO: {
        const u_int64_t baroff = stlp->addr - phwbar->addr;
        const u_int32_t size = info->direct_size;
        u_int8_t do_notify = 0;

        virtio_barwr(phwdev, info->direct_addr, baroff, size, stlp->data,
                     &do_notify);

        if (do_notify) {
            pciehw_barrw_notify(PCIESVC_EV_MEMWR_NOTIFY,
                                port, phwdev, stlp, info, spmt);
        }

        break;
    }

    default: {
        u_int64_t pa = info->direct_addr;
        size_t sz = info->direct_size;

        pciesvc_mem_wr(pa, &stlp->data, sz);
        break;
    }
    }
    pciehwdev_put(phwdev, CLEAN);
    pciesvc_spmt_put(spmt, CLEAN);

    pciehw_indirect_complete(ientry);
}
