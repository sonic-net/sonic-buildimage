// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017-2019,2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "pcietlp.h"
#include "portcfg.h"
#include "cfgspace.h"
#include "bdf.h"
#include "intr.h"
#include "indirect.h"
#include "notify.h"
#include "hdrt.h"
#include "vpd.h"
#include "reset.h"

typedef struct handler_ctx_s {
    pcie_stlp_t stlp;
    int port;
    pciehwdevh_t hwdevh;
    uint32_t retval;
    indirect_entry_t *ientry;
    notify_entry_t *nentry;
} handler_ctx_t;

/*
 * Detect these overlaps:
 *
 * regaddr        regsize
 * v              v
 * +--------------+
 * +--------------+
 * ^              ^
 * tlpaddr        tpsize
 *
 * regaddr        regsize
 * v              v
 * +--------------+
 *           +--------------+
 *           ^              ^
 *           tlpaddr        tpsize
 *
 *     regaddr        regsize
 *     v              v
 *     +--------------+
 * +--------------+
 * ^              ^
 * tlpaddr        tpsize
 */
static int
stlp_overlap(const pcie_stlp_t *stlp,
             const u_int32_t regaddr, const u_int32_t regsize)
{
    const u_int32_t tlpaddr = stlp->addr;
    const u_int32_t tlpsize = stlp->size;

    return tlpaddr < regaddr + regsize && tlpaddr + tlpsize > regaddr;
}

/*
 * The "info->vfid" parameter is scaled by the vfstride entry
 * to compute the target config space physical address.  We
 * use the hardware target "cfgpa" to determine the target
 * hwdev that is being addressed.  This makes us independent
 * of the vfstride scaling of "info->vfid" to find the target device.
 *
 * We could record the vfid scale factor in the spmt and then
 * shift the "info->vfid" as the hw would do, but using the
 * "info->direct_addr" provided by hw is easier and gives us
 * the same answer.
 *
 * We could also lookup based on stlp->bdf that comes from the
 * decode of the rawtlp so we know it is accurate.  Right now
 * our bdf lookup is not very efficient so "cfgpa" is faster.
 *
 * Sometimes we get called with a "cfgpa" that is outside the
 * device cfgcur region.  This happens when we have indirect_catchall
 * and the cfgpa is in zerospa.  For this case we return 0
 * which is an unused handle so config space is all 0's so we'll
 * end up reading a 0 for all values.
 */
static pciehwdevh_t
cfgpa_to_hwdevh(const u_int64_t cfgpa)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
#define CFGCURSZ PHWMEM_SIZEOF(phwmem, pshmem, cfgcur)
    const u_int64_t cfgcurpa = pciesvc_cfgcur_pa();

    if (cfgpa >= cfgcurpa && cfgpa < cfgcurpa + CFGCURSZ) {
        const u_int64_t cfgoff = cfgpa - cfgcurpa;
        return cfgoff >> PCIEHW_CFGSHIFT;
    }
    return 0;
}

void
pciehw_cfg_load(pciehwdev_t *phwdev)
{
    pciehw_pmt_load_cfg(phwdev);
}

void
pciehw_cfg_unload(pciehwdev_t *phwdev)
{
    pciehw_pmt_unload_cfg(phwdev);
}

/*****************************************************************
 * cfgrd handlers
 */

static void
pciehw_cfgrd_delay(handler_ctx_t *hctx)
{
    pciesvc_debug_cmd(&hctx->retval);
}

/*****************************************************************
 * cfgwr handlers
 */

static u_int32_t
cfg_bar32(cfgspace_t *cs, const u_int32_t cfgoff)
{
    u_int32_t baraddr = cfgspace_readd(cs, cfgoff);
    return baraddr;
}

static u_int64_t
cfg_bar64(cfgspace_t *cs, const u_int32_t cfgoff)
{
    u_int32_t barlo, barhi;

    barlo = cfgspace_readd(cs, cfgoff + 0);
    barhi = cfgspace_readd(cs, cfgoff + 4);

    return ((u_int64_t)barhi << 32) | barlo;
}

static u_int64_t
cfg_baraddr(cfgspace_t *cs, const u_int32_t cfgoff, const u_int32_t barlen)
{
    u_int64_t baraddr;

    if (barlen == 8) {
        baraddr = cfg_bar64(cs, cfgoff);
    } else {
        baraddr = cfg_bar32(cs, cfgoff);
    }
    return baraddr;
}

static void
pciehw_cfg_bars_enable(pciehwdev_t *phwdev, const u_int16_t cmd)
{
    const int io_en = (cmd & PCI_COMMAND_IO) != 0;
    const int mem_en = (cmd & PCI_COMMAND_MEMORY) != 0;
    pciehwbar_t *phwbar;
    int i;

#ifdef PCIEMGR_DEBUG
    if (!phwdev->vf) {
        pciesvc_logdebug("bars_enable: %s mem%c io%c\n",
                         pciehwdev_get_name(phwdev),
                         mem_en ? '+' : '-',
                         io_en ? '+' : '-');
    }
#endif

    for (phwbar = phwdev->bar, i = 0; i < PCIEHW_NBAR; i++, phwbar++) {
        if (!phwbar->valid) continue;

        if ((phwbar->type == PCIEHWBARTYPE_MEM ||
             phwbar->type == PCIEHWBARTYPE_MEM64)) {
            pciehw_bar_enable(phwdev, phwbar, mem_en);
        } else if (phwbar->type == PCIEHWBARTYPE_IO) {
            pciehw_bar_enable(phwdev, phwbar, io_en);
        }
    }
}

/*
 * rombar is enabled iff CMD.memory_space_en && ROMBAR.en.
 */
static void
pciehw_cfg_rombar_enable(pciehwdev_t *phwdev, cfgspace_t *cs)
{
    pciehwbar_t *phwbar = &phwdev->rombar;

    if (phwbar->valid) {
        const int mem_en = (cfgspace_readw(cs, PCI_COMMAND) & 0x2) != 0;
        const int rom_en = (cfgspace_readd(cs, PCI_ROM_ADDRESS) & 0x1) != 0;

        pciehw_bar_enable(phwdev, phwbar, mem_en && rom_en);
    }
}

static void
pciehw_cfg_busmaster_enable(pciehwdev_t *phwdev, const int on)
{
#ifdef PCIEMGR_DEBUG
    if (!phwdev->vf) {
        pciesvc_logdebug("busmaster_enable: %s %s\n",
                         pciehwdev_get_name(phwdev), on ? "on" : "off");
    }
#endif
    if (on) {
        pciehw_hdrt_load(phwdev->lifb, phwdev->lifc, phwdev->bdf);
    } else {
        pciehw_hdrt_unload(phwdev->lifb, phwdev->lifc);
    }
}

static void
pciehw_cfg_cmd(pciehwdev_t *phwdev, cfgspace_t *cs, const u_int16_t cmd)
{
    u_int16_t msixcap, msixctl;

    /*
     * PF check cmd reg for bar enables.
     * VF bar enables come from PF sriov capability (see cfgwr_sriov()).
     */
    if (!phwdev->vf) {
        /* bar control */
        pciehw_cfg_bars_enable(phwdev, cmd);
        /* cmd.mem_enable might have enabled rombar */
        pciehw_cfg_rombar_enable(phwdev, cs);

        msixcap = cfgspace_findcap(cs, PCI_CAP_ID_MSIX);
        if (msixcap) {
            msixctl = cfgspace_readw(cs, msixcap + PCI_MSIX_FLAGS);
        } else {
            msixctl = 0;
        }

        /* intx_disable */
        if ((msixctl & PCI_MSIX_FLAGS_ENABLE) == 0) {
            const int legacy = 1;
            const int fmask = (cmd & PCI_COMMAND_INTX_DISABLE) != 0;
            pciehw_intr_config(phwdev, legacy, fmask);
        }
    }

    pciehw_cfg_busmaster_enable(phwdev, (cmd & PCI_COMMAND_MASTER) != 0);
}

static void
pciehw_cfgwr_cmd(const handler_ctx_t *hctx)
{
    pciehwdev_t *phwdev;
    cfgspace_t cs;
    u_int16_t cmd;

    phwdev = pciehwdev_get(hctx->hwdevh);
    pciesvc_cfgspace_get(hctx->hwdevh, &cs);
    cmd = cfgspace_readw(&cs, PCI_COMMAND);
    pciehw_cfg_cmd(phwdev, &cs, cmd);
    pciesvc_cfgspace_put(&cs, CLEAN);
    pciehwdev_put(phwdev, DIRTY); /* updated bars[].bdf */
}

static void
pciehw_cfgwr_bars(pciehwdev_t *phwdev,
                  const pcie_stlp_t *stlp,
                  cfgspace_t *cs,
                  const int cfgbase)
{
    pciehwbar_t *phwbar;
    int i;

    for (phwbar = phwdev->bar, i = 0; i < PCIEHW_NBAR; i++, phwbar++) {
        if (phwbar->valid) {
            const int cfgoff = cfgbase + phwbar->cfgidx * 4;
            const int barlen = phwbar->type == PCIEHWBARTYPE_MEM64 ? 8 : 4;
            if (stlp_overlap(stlp, cfgoff, barlen)) {
                const u_int64_t vfbaroff = (pciehw_bar_getsize(phwbar) *
                                            phwdev->vfidx);
                u_int64_t baraddr = cfg_baraddr(cs, cfgoff, barlen);
                u_int64_t addr;

                if (phwbar->type == PCIEHWBARTYPE_IO) {
                    baraddr &= ~0x3ULL;
                } else {
                    baraddr &= ~0xfULL;
                }
                addr = baraddr + vfbaroff;
#ifdef PCIEMGR_DEBUG
                pciesvc_logdebug("%s: bar %d pmt %d setaddr 0x%" PRIx64 "\n",
                                 pciehwdev_get_name(phwdev),
                                 phwbar->cfgidx, phwbar->pmtb, addr);
#endif
                pciehw_bar_setaddr(phwbar, addr);
            }
        }
    }
}

static void
pciehw_cfgwr_dev_bars(const handler_ctx_t *hctx)
{
    const int cfgbase = 0x10;
    pciehwdev_t *phwdev;
    cfgspace_t cs;

    phwdev = pciehwdev_get(hctx->hwdevh);
    pciesvc_cfgspace_get(hctx->hwdevh, &cs);
    pciehw_cfgwr_bars(phwdev, &hctx->stlp, &cs, cfgbase);
    pciesvc_cfgspace_put(&cs, CLEAN);
    pciehwdev_put(phwdev, DIRTY); /* updated phwdev->bars[] bdf,addr */

}

static void
pciehw_cfgwr_rom_bar(const handler_ctx_t *hctx)
{
    pciehwdev_t *phwdev;
    pciehwbar_t *phwbar;
    cfgspace_t cs;
    u_int32_t baraddr;

    phwdev = pciehwdev_get(hctx->hwdevh);
    pciesvc_cfgspace_get(hctx->hwdevh, &cs);
    baraddr = cfgspace_readd(&cs, PCI_ROM_ADDRESS);
    baraddr &= ~0x1; /* mask enable bit */
    phwbar = &phwdev->rombar;
    pciehw_bar_setaddr(phwbar, baraddr);
    pciehw_cfg_rombar_enable(phwdev, &cs);
    pciesvc_cfgspace_put(&cs, CLEAN);
    pciehwdev_put(phwdev, DIRTY); /* updated phwdev->bars[] bdf,addr */
}

static void
pciehw_mgmtchg_event(const pciehwdev_t *phwdev)
{
    pciesvc_eventdata_t evd;

    pciesvc_memset(&evd, 0, sizeof(evd));
    evd.evtype = PCIESVC_EV_MGMTCHG;
    evd.port = phwdev->port;
    evd.lif = phwdev->lifb;
    pciesvc_event_handler(&evd, sizeof(evd));
}

/*
 * Set a new device bus identity for this device.
 * This happens when a bridge secondary bus is written.
 * We also load the cfg entries into the pmt tcam if requested.
 */
static void
pciehw_cfg_set_bus(pciehwdev_t *phwdev, const u_int8_t bus, const int load)
{
    u_int8_t busbase, busdelta;
    u_int32_t pmti;

    busbase = 0;
    for (pmti = phwdev->pmtb; pmti < phwdev->pmtb + phwdev->pmtc; pmti++) {
        pciehw_spmt_t *spmt = pciesvc_spmt_get(pmti);
        pmt_t *pmt = &spmt->pmt;
        const pmr_cfg_entry_t *pmr = &pmt->pmre.cfg;

        /*
         * If we have >255 vfs then some pmt entries will have a
         * different bus.  We'll keep track of the (pre-adjusted) bstart
         * from the first entry and apply any delta to the new bus.
         */
        if (pmti == phwdev->pmtb) {
            busbase = pmr->bstart;
        }
        busdelta = pmr->bstart - busbase;

        pmt_cfg_set_bus(pmt, bus + busdelta);
        if (load) {
            pmt_set(pmti, pmt);
            spmt->loaded = 1;
        }
        pciesvc_spmt_put(spmt, DIRTY); /* pmt, loaded */
    }
    if (!phwdev->cfgloaded && load) {
        phwdev->cfgloaded = 1;
    }
}

/*
 * Device captures a new bus.  We do this when our parent bridge
 * gets a new secondary bus number assigned.
 * If new bus == 0 then unload the cfg entries.  Host software
 * will set bus == 0 during the bus walk step as the number of
 * required buses due to ari/sriov is determined, only a single
 * bus at a time gets a bus number assigned.
 *
 * For example, during bios scan
 * (on UCS C220 Bios Version C220M5.3.1.3c.0307181404, and
 *  Dell R6525 BIOS Version 2.2.5) we see:
 * [2000-06-21 09:18:28.829918] bridgedn0: hwbus 0xb4 secbus 0xb5 adjbus 0x01
 * [2000-06-21 09:18:28.833321] bridgedn0: hwbus 0xb4 secbus 0x00 adjbus 0x00
 * [2000-06-21 09:18:28.837797] bridgedn1: hwbus 0xb4 secbus 0xb5 adjbus 0x01
 * [2000-06-21 09:18:28.841914] bridgedn1: hwbus 0xb4 secbus 0x00 adjbus 0x00
 * [2000-06-21 09:18:28.845558] bridgedn2: hwbus 0xb4 secbus 0xb5 adjbus 0x01
 * [2000-06-21 09:18:28.849805] bridgedn2: hwbus 0xb4 secbus 0x00 adjbus 0x00
 *
 * Then, the bus requirements are determined, and the final config is set:
 * [2000-06-21 09:18:31.920035] bridgedn0: hwbus 0xb4 secbus 0xb5 adjbus 0x01
 * [2000-06-21 09:18:31.946122] bridgedn1: hwbus 0xb4 secbus 0xb6 adjbus 0x02
 * [2000-06-21 09:18:31.968468] bridgedn2: hwbus 0xb4 secbus 0xb7 adjbus 0x03
 */
static void
pciehw_capture_bus(pciehwdev_t *phwdev, const u_int8_t bus, const int load)
{
    if (!phwdev->vf) {
        pciesvc_loginfo("capture_bus: %s bdf 0x%04x new bus 0x%02x\n",
                        pciehwdev_get_name(phwdev), phwdev->bdf, bus);
    }
    if (bus) {
        const uint8_t vfbusoff = phwdev->vf ? bdf_to_bus(phwdev->vfidx+1) : 0;
        const uint16_t newbdf = bdf_make(bus + vfbusoff,
                                         bdf_to_dev(phwdev->bdf),
                                         bdf_to_fnc(phwdev->bdf));
        if (phwdev->bdf != newbdf) {
            phwdev->bdf = newbdf;
            /* event for mgmteth bdf change */
            if (phwdev->type == PCIEHDEVICE_MGMTETH) {
                pciehw_mgmtchg_event(phwdev);
            }
        }
        pciehw_cfg_set_bus(phwdev, bus, load);
    } else {
        pciehw_cfg_unload(phwdev);
    }
}

static void
pciehw_assign_bus(pciehwdevh_t hwdevh, const u_int8_t bus, const int load)
{
    while (hwdevh) {
        pciehwdev_t *phwdev = pciehwdev_get(hwdevh);
        const pciehwdevh_t childh = phwdev->childh;
        const pciehwdevh_t peerh = phwdev->peerh;

        pciehw_capture_bus(phwdev, bus, load);
        pciehwdev_put(phwdev, DIRTY); /* bdf */

        /* also assign bus to vfs if any */
        pciehw_assign_bus(childh, bus, load);

        hwdevh = peerh;
    }
}

static void
pciehw_bridge_secbus(pciehwdev_t *phwdev)
{
    cfgspace_t cs;
    u_int8_t hwbus, secbus, adjbus;
    pciehwdevh_t childh;

    /*
     * Note that our bridge PRIMARY_BUS is the same
     * as hwbus, but pribus is optional in pcie and
     * some systems (UCS bios) don't set bridge pribus
     * during the initial bus scan, so we get the
     * secbus of the hwbridge as a reliable bus.
     */
    portcfg_read_bus(phwdev->port, NULL, &hwbus, NULL);

    pciesvc_cfgspace_get(phwdev->hwdevh, &cs);
    secbus = cfgspace_get_secbus(&cs);
    pciesvc_cfgspace_put(&cs, CLEAN);

    /*
     * The bridge secbus is a physical bus number.
     * The hardware usually deals with "adjusted" bus numbers,
     * i.e. bus numbers relative to the secondary bus of the hw bridge.
     * Here we perform the bus adjustment the hw will do to our
     * secondary bus by subtracting the hw bridge secondary bus
     * from the configured secbus to get the
     * adjusted bus to assign to our devices.
     */
    adjbus = (secbus && hwbus != 0xff) ? secbus - hwbus : 0;

    pciesvc_loginfo("%s: hwbus 0x%02x secbus 0x%02x adjbus 0x%02x\n",
                    pciehwdev_get_name(phwdev), hwbus, secbus, adjbus);
    childh = phwdev->childh;

    pciehw_assign_bus(childh, adjbus, 1);
}

static void
pciehw_cfgwr_bridge_bus(const handler_ctx_t *hctx)
{
    if (stlp_overlap(&hctx->stlp, PCI_SECONDARY_BUS, sizeof(uint8_t))) {
        pciehwdev_t *phwdev = pciehwdev_get(hctx->hwdevh);

        pciehw_bridge_secbus(phwdev);

        pciehwdev_put(phwdev, CLEAN);
    }
}

static void
pciehw_cfgwr_bridgectl(const handler_ctx_t *hctx)
{
    cfgspace_t cs;
    u_int16_t brctl;
    u_int8_t secbus;

    pciesvc_cfgspace_get(hctx->hwdevh, &cs);
    brctl = cfgspace_readw(&cs, PCI_BRIDGE_CONTROL);
    secbus = cfgspace_get_secbus(&cs);
    pciesvc_cfgspace_put(&cs, CLEAN);

    if (brctl & PCI_BRIDGE_CTL_BUS_RESET) {
        pciehwdev_t *phwdev = pciehwdev_get(hctx->hwdevh);
        pciehw_reset_bus(phwdev, secbus);
        pciehwdev_put(phwdev, CLEAN);
    }
}

static void
pciehw_cfgwr_msix(const handler_ctx_t *hctx)
{
    const u_int16_t reg = hctx->stlp.addr;
    const u_int16_t regdw = reg >> 2;
    pciehwdev_t *phwdev;
    cfgspace_t cs;
    u_int16_t msixctl, cmd;
    int msix_en, msix_mask, fmask, legacy;

    pciesvc_cfgspace_get(hctx->hwdevh, &cs);
    msixctl = cfgspace_readw(&cs, (regdw << 2) + 2);
    msix_en = (msixctl & PCI_MSIX_FLAGS_ENABLE) != 0;
    msix_mask = (msixctl & PCI_MSIX_FLAGS_MASKALL) != 0;

    phwdev = pciehwdev_get(hctx->hwdevh);
    phwdev->msix_en = msix_en;

    if (msix_en) {
        /* msix mode */
        legacy = 0;
        fmask = msix_mask;
    } else if (phwdev->vf) {
        /* sriov vf disabled */
        legacy = 0;
        fmask = 1;
    } else {
        /* intx mode */
        legacy = 1;
        cmd = cfgspace_readw(&cs, PCI_COMMAND);
        fmask = phwdev->vf || (cmd & PCI_COMMAND_INTX_DISABLE) != 0;
    }
    pciesvc_cfgspace_put(&cs, CLEAN);

    pciehw_intr_config(phwdev, legacy, fmask);
    pciehwdev_put(phwdev, CLEAN);
}

static void
pciehw_cfgwr_vpd(const handler_ctx_t *hctx)
{
    cfgspace_t cs;
    u_int16_t vpdcap, addr, f;
    u_int32_t data;

    pciesvc_cfgspace_get(hctx->hwdevh, &cs);
    vpdcap = cfgspace_findcap(&cs, PCI_CAP_ID_VPD);
    addr = cfgspace_readw(&cs, vpdcap + PCI_VPD_ADDR);
    f = addr & PCI_VPD_ADDR_F;
    addr &= PCI_VPD_ADDR_MASK;

    /*
     * Flag set indicates write data, clear flag when complete.
     * Flag clear indicates read data, set flag when complete.
     */
    if (f) {
        /* vpd write */
        data = cfgspace_readd(&cs, vpdcap + PCI_VPD_DATA);
        pciehw_vpd_write(hctx->hwdevh, addr, data);
        cfgspace_writew(&cs, vpdcap + PCI_VPD_ADDR, addr);
    } else {
        /* vpd read */
        data = pciehw_vpd_read(hctx->hwdevh, addr);
        cfgspace_writed(&cs, vpdcap + PCI_VPD_DATA, data);
        pciesvc_mem_barrier();  /* data lands *before* we set ADDR_F */
        cfgspace_writew(&cs, vpdcap + PCI_VPD_ADDR, addr | PCI_VPD_ADDR_F);
    }
    pciesvc_cfgspace_put(&cs, DIRTY); /* VPD_DATA,VPD_ADDR */
}

static void
pciehw_cfgwr_pcie_devctl(const handler_ctx_t *hctx)
{
    cfgspace_t cs;
    u_int16_t pciecap, devctl;

    pciesvc_cfgspace_get(hctx->hwdevh, &cs);
    pciecap = cfgspace_findcap(&cs, PCI_CAP_ID_EXP);
    devctl = cfgspace_readw(&cs, pciecap + PCI_EXP_DEVCTL);
    pciesvc_cfgspace_put(&cs, CLEAN);

    if (stlp_overlap(&hctx->stlp, pciecap + 0x8, sizeof(u_int16_t))) {
        if (devctl & PCI_EXP_DEVCTL_BCR_FLR) {
            pciehwdev_t *phwdev = pciehwdev_get(hctx->hwdevh);
            pciehw_reset_flr(phwdev);
            pciehwdev_put(phwdev, CLEAN);
        }
    }
}

static void
pciehw_sriov_numvfs_event(pciehwdev_t *phwdev, const u_int16_t numvfs)
{
    pciesvc_eventdata_t evd;
    pciesvc_sriov_numvfs_t *sriov_numvfs;

    pciesvc_memset(&evd, 0, sizeof(evd));
    evd.evtype = PCIESVC_EV_SRIOV_NUMVFS;
    evd.port = phwdev->port;
    evd.lif = phwdev->lifb;
    sriov_numvfs = &evd.sriov_numvfs;
    sriov_numvfs->numvfs = numvfs;
    pciesvc_event_handler(&evd, sizeof(evd));
}

/*
 * Some of the bar entries of vf0 cover the bars for all vfs.
 * Apply the "numvfs" limit to these vf0 bars.
 */
static int
pciehw_sriov_adjust_vf0(pciehwdev_t *vfhwdev, const int numvfs)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    pciehwbar_t *phwbar;
    int i, r, do_log;

    r = 0;
    for (phwbar = vfhwdev->bar, i = 0; i < PCIEHW_NBAR; i++, phwbar++) {
        pciehw_spmt_t *spmt, *spmte;
        if (!phwbar->valid) continue;
        do_log = 1; /* log adjust_vf0 for first pmt of bar */
        spmt = PSHMEM_ADDR_FIELD(pshmem, spmt[phwbar->pmtb]);
        spmte = spmt + phwbar->pmtc;
        for ( ; spmt < spmte; spmt++) {
            if (spmt->vf0) {
                const u_int64_t pmtaddr = phwbar->addr + spmt->baroff;
                r = pciehw_pmt_adjust_vf0(spmt, pmtaddr, numvfs, do_log);
                if (r < 0) goto out;
#ifndef PCIEMGR_DEBUG
                do_log = 0;
#endif
            }
        }
    }
 out:
    return r;
}

/*
 * Enable this VF.  Make it visible on the PCIe bus in cfg space,
 * and enable bars too if Memory Space Enable (mse) is set.
 */
static void
pciehw_sriov_enable_vf(pciehwdev_t *vfhwdev, const int mse)
{
    u_int16_t cmd;

    /* XXX handle vfe load/unload cfg space */
    /* refactor and call pciehw_cfg_load(vfhwdev) */

    /* load/unload the bars */
    cmd = mse ? PCI_COMMAND_MEMORY : 0;
    pciehw_cfg_bars_enable(vfhwdev, cmd);
}

static void
pciehw_sriov_enable_vfs(pciehwdev_t *phwdev, const int numvfs, const int mse)
{
    pciehwdev_t *vfhwdev;
    int vfidx, r;

    vfhwdev = pciehwdev_vfdev_get(phwdev, 0);
    r = pciehw_sriov_adjust_vf0(vfhwdev, numvfs);
    pciehwdev_vfdev_put(vfhwdev, CLEAN);
    if (r < 0) {
        pciesvc_logerror("%s: adjust_vf0 failed\n",
                         pciehwdev_get_name(phwdev));
        return;
    }

    for (vfidx = 0; vfidx < numvfs; vfidx++) {
        vfhwdev = pciehwdev_vfdev_get(phwdev, vfidx);
        pciehw_sriov_enable_vf(vfhwdev, mse);
        pciehwdev_vfdev_put(vfhwdev, DIRTY); /* bdf */
    }
}

static void
pciehw_sriov_disable_vf(pciehwdev_t *vfhwdev)
{
    const u_int16_t cmd = 0;
    pciehw_cfg_bars_enable(vfhwdev, cmd);

    /* XXX handle vfe load/unload cfg space */
    /* refactor and call pciehw_cfg_unload(vfhwdev) */
}

/*
 * Disable VFs.  Unload the bars and clear bus master enable.
 * We'll reset cfg space for the disable VFs which clears bus master enable.
 */
static void
pciehw_sriov_disable_vfs(pciehwdev_t *phwdev, const int vfb, const int vfc)
{
    int vfidx;

    for (vfidx = vfb; vfidx < vfb + vfc; vfidx++) {
        pciehwdev_t *vfhwdev = pciehwdev_vfdev_get(phwdev, vfidx);
        pciehw_sriov_disable_vf(vfhwdev);
        pciehwdev_vfdev_put(vfhwdev, DIRTY);
    }
    /* Park disabled vf's in reset state. */
    pciehw_reset_vfs(phwdev, vfb, vfc);
}

/*
 * If VF Enable (vfe) is set, then enable VFs and possibly enable bars
 * if Memory Space Enable (mse) is also set.
 *
 * If VF Enable (vfe) is clear, then disable VFs (mse is ignored).
 */
static void
pciehw_sriov_ctrl_numvfs(pciehwdev_t *phwdev,
                         const u_int16_t ctrl, const u_int16_t numvfs)
{
    const int vfe = (ctrl & PCI_SRIOV_CTRL_VFE) != 0; /* VF Enable */
    const int mse = (ctrl & PCI_SRIOV_CTRL_MSE) != 0; /* Memory Space Enable */

    if (vfe) {
        /*
         * VF Enable set, first disable any enabled VFs greater than numvfs,
         * then enable [0-numvfs) range.
         */
        if (phwdev->enabledvfs > numvfs) {
            pciehw_sriov_disable_vfs(phwdev,
                                     numvfs, phwdev->enabledvfs - numvfs);
        }
        pciehw_sriov_enable_vfs(phwdev, numvfs, mse);
        phwdev->enabledvfs = numvfs;

    } else {
        /*
         * VF Enable clear, disable all enabled VFs.
         */
        if (phwdev->enabledvfs) {
            pciehw_sriov_disable_vfs(phwdev, 0, phwdev->enabledvfs);
            phwdev->enabledvfs = 0;
        }
    }

    /*
     * Generate an event for numvfs change.
     */
    if (phwdev->numvfs != numvfs) {
        pciehw_sriov_numvfs_event(phwdev, numvfs);
        phwdev->numvfs = numvfs;
    }
}

void
pciehw_sriov_ctrl(pciehwdev_t *phwdev,
                  const u_int16_t ctrl, const u_int16_t numvfs)
{
    if (phwdev->sriovctrl != ctrl) {
#ifdef __aarch64__
        pciesvc_loginfo("%s "
                        "sriov_ctrl 0x%04x vfe%c mse%c ari%c numvfs %d\n",
                        pciehwdev_get_name(phwdev),
                        ctrl,
                        ctrl & PCI_SRIOV_CTRL_VFE ? '+' : '-',
                        ctrl & PCI_SRIOV_CTRL_MSE ? '+' : '-',
                        ctrl & PCI_SRIOV_CTRL_ARI ? '+' : '-',
                        numvfs);
#endif
        pciehw_sriov_ctrl_numvfs(phwdev, ctrl, numvfs);
        phwdev->sriovctrl = ctrl;
    }
}

static void
pciehw_cfgwr_sriov_ctrl(const handler_ctx_t *hctx)
{
    pciehwdev_t *phwdev;
    cfgspace_t cs;
    u_int16_t sriovcap, sriovctrl, numvfs;

    phwdev = pciehwdev_get(hctx->hwdevh);
    pciesvc_cfgspace_get(hctx->hwdevh, &cs);
    sriovcap = cfgspace_findextcap(&cs, PCI_EXT_CAP_ID_SRIOV);
    sriovctrl = cfgspace_readw(&cs, sriovcap + PCI_SRIOV_CTRL);

    numvfs = cfgspace_readw(&cs, sriovcap + PCI_SRIOV_NUM_VF);
    if (numvfs > phwdev->totalvfs) numvfs = phwdev->totalvfs;

    pciesvc_cfgspace_put(&cs, CLEAN);

    /*
     * If we're running as an indirect transaction then we'll have ientry
     * set.  If indirect, complete the transaction now before we go do
     * the potentially long work of resetting a bunch of VFs.
     */
    if (hctx->ientry) {
        pciehw_indirect_complete(hctx->ientry);
    }

    pciehw_sriov_ctrl(phwdev, sriovctrl, numvfs);
    pciehwdev_put(phwdev, DIRTY); /* set sriovctrl,enabledvs */
}

static void
pciehw_cfgwr_sriov_bars(const handler_ctx_t *hctx)
{
    pciehwdev_t *phwdev, *vfhwdev;
    cfgspace_t pfcs;
    int vfidx, sriovcap;

    phwdev = pciehwdev_get(hctx->hwdevh);
    pciesvc_cfgspace_get(hctx->hwdevh, &pfcs);
    sriovcap = cfgspace_findextcap(&pfcs, PCI_EXT_CAP_ID_SRIOV);

    /*
     * Distribute the new bar address to all the VFs.
     * Each VF will compute its own offset within
     * the bar for its VF sliced region.
     */
    for (vfidx = 0; vfidx < phwdev->totalvfs; vfidx++) {
        vfhwdev = pciehwdev_vfdev_get(phwdev, vfidx);
        pciehw_cfgwr_bars(vfhwdev, &hctx->stlp, &pfcs, sriovcap + 0x24);
        pciehwdev_vfdev_put(vfhwdev, DIRTY); /* vfhwdev->bars[] bdf,addr */
    }
    pciesvc_cfgspace_put(&pfcs, CLEAN);
    pciehwdev_put(phwdev, CLEAN);
}

/*****************************************************************
 * cfg handlers
 */

static void
pciehw_cfgrd_handler(handler_ctx_t *hctx)
{
    const u_int16_t reg = hctx->stlp.addr;
    const u_int16_t regdw = reg >> 2;
    pciehw_cfghnd_t hnd = PCIEHW_CFGHND_NONE;

    if (regdw < PCIEHW_CFGHNDSZ) {
        pciehwdev_t *phwdev = pciehwdev_get(hctx->hwdevh);
        hnd = phwdev->cfghnd[regdw];
        pciehwdev_put(phwdev, CLEAN);
    }
    switch (hnd) {
    default:
    case PCIEHW_CFGHND_NONE:
        break;
    case PCIEHW_CFGHND_DBG_DELAY:
        pciehw_cfgrd_delay(hctx);
        break;
    }
}

static void
pciehw_cfgwr_handler(const handler_ctx_t *hctx)
{
    const u_int16_t reg = hctx->stlp.addr;
    const u_int16_t regdw = reg >> 2;
    pciehw_cfghnd_t hnd = PCIEHW_CFGHND_NONE;

    if (regdw < PCIEHW_CFGHNDSZ) {
        pciehwdev_t *phwdev = pciehwdev_get(hctx->hwdevh);
        hnd = phwdev->cfghnd[regdw];
        pciehwdev_put(phwdev, CLEAN);
    }
    switch (hnd) {
    default:
    case PCIEHW_CFGHND_NONE:
        break;
    case PCIEHW_CFGHND_CMD:
        pciehw_cfgwr_cmd(hctx);
        break;
    case PCIEHW_CFGHND_DEV_BARS:
        pciehw_cfgwr_dev_bars(hctx);
        break;
    case PCIEHW_CFGHND_ROM_BAR:
        pciehw_cfgwr_rom_bar(hctx);
        break;
    case PCIEHW_CFGHND_BRIDGE_BUS:
        pciehw_cfgwr_bridge_bus(hctx);
        break;
    case PCIEHW_CFGHND_BRIDGECTL:
        pciehw_cfgwr_bridgectl(hctx);
        break;
    case PCIEHW_CFGHND_MSIX:
        pciehw_cfgwr_msix(hctx);
        break;
    case PCIEHW_CFGHND_VPD:
        pciehw_cfgwr_vpd(hctx);
        break;
    case PCIEHW_CFGHND_PCIE_DEVCTL:
        pciehw_cfgwr_pcie_devctl(hctx);
        break;
    case PCIEHW_CFGHND_SRIOV_CTRL:
        pciehw_cfgwr_sriov_ctrl(hctx);
        break;
    case PCIEHW_CFGHND_SRIOV_BARS:
        pciehw_cfgwr_sriov_bars(hctx);
        break;
    }
}

/*****************************************************************
 * notify handlers
 */

void
pciehw_cfgrd_notify(const int port, notify_entry_t *nentry)
{
    handler_ctx_t hctx;

    pciesvc_memset(&hctx, 0, sizeof(hctx));
    hctx.port = port;
    hctx.nentry = nentry;
    hctx.hwdevh = cfgpa_to_hwdevh(nentry->info.direct_addr);
    pcietlp_decode(&hctx.stlp, nentry->rtlp, sizeof(nentry->rtlp));

    pciehw_cfgrd_handler(&hctx);
}

void
pciehw_cfgwr_notify(const int port, notify_entry_t *nentry)
{
    handler_ctx_t hctx;

    pciesvc_memset(&hctx, 0, sizeof(hctx));
    hctx.port = port;
    hctx.nentry = nentry;
    hctx.hwdevh = cfgpa_to_hwdevh(nentry->info.direct_addr);
    pcietlp_decode(&hctx.stlp, nentry->rtlp, sizeof(nentry->rtlp));

    pciehw_cfgwr_handler(&hctx);
}

/*****************************************************************
 * indirect handlers
 */

void
pciehw_cfgrd_indirect(const int port, indirect_entry_t *ientry)
{
    handler_ctx_t hctx;
    cfgspace_t cs;

    pciesvc_memset(&hctx, 0, sizeof(hctx));
    hctx.port = port;
    hctx.ientry = ientry;
    hctx.hwdevh = cfgpa_to_hwdevh(ientry->info.direct_addr);
    pcietlp_decode(&hctx.stlp, ientry->rtlp, sizeof(ientry->rtlp));

    /*
     * For indirect reads read the current value at target addr
     * and put in retval.  The handler has a chance to modify
     * retval if desired.
     */
    pciesvc_cfgspace_get(hctx.hwdevh, &cs);
    cfgspace_read(&cs, hctx.stlp.addr, hctx.stlp.size, &hctx.retval);
    pciesvc_cfgspace_put(&cs, CLEAN);

    pciehw_cfgrd_handler(&hctx);

    ientry->data[0] = hctx.retval;
    pciehw_indirect_complete(ientry);

#ifdef PCIEMGR_DEBUG
    pciesvc_logdebug("cfgrd_indirect: "
                     "hwdevh %d vfid %d rd 0x%lx sz %d data 0x%x\n",
                     hctx.hwdevh, ientry->info.vfid,
                     hctx.stlp.addr, hctx.stlp.size, ientry->data[0]);
#endif
}

void
pciehw_cfgwr_indirect(const int port, indirect_entry_t *ientry)
{
    handler_ctx_t hctx;
    cfgspace_t cs;
    int r;

    pciesvc_memset(&hctx, 0, sizeof(hctx));
    hctx.port = port;
    hctx.ientry = ientry;
    hctx.hwdevh = cfgpa_to_hwdevh(ientry->info.direct_addr);
    pcietlp_decode(&hctx.stlp, ientry->rtlp, sizeof(ientry->rtlp));

#ifdef PCIEMGR_DEBUG
    pciesvc_logdebug("cfgwr_indirect: "
                     "hwdevh %d vfid %d wr 0x%lx sz %d data 0x%lx\n",
                     hctx.hwdevh, ientry->info.vfid,
                     hctx.stlp.addr, hctx.stlp.size, hctx.stlp.data);
#endif

    /*
     * For indirect writes, write the data first,
     * then let the handler run with the updated data.
     */
    pciesvc_cfgspace_get(hctx.hwdevh, &cs);
    r = cfgspace_write(&cs, hctx.stlp.addr, hctx.stlp.size, hctx.stlp.data);
    pciesvc_cfgspace_put(&cs, DIRTY);

    if (r < 0) {
        ientry->cpl = PCIECPL_CA;
    }
    pciehw_cfgwr_handler(&hctx);
    pciehw_indirect_complete(ientry);
}

/*****************************************************************
 * reset
 */

void
pciehw_cfg_reset(pciehwdev_t *phwdev, const pciesvc_rsttype_t rsttype)
{
    cfgspace_t cs;
    u_int16_t cfgsz, cmd, pciecap, devctl, maxpayload;

    pciesvc_cfgspace_get(pciehwdev_geth(phwdev), &cs);
    cfgsz = cfgspace_size(&cs);

    /* save maxpayload setting before reset */
    pciecap = cfgspace_findcap(&cs, PCI_CAP_ID_EXP);
    if (pciecap) {
        devctl = cfgspace_readw(&cs, pciecap + PCI_EXP_DEVCTL);
        maxpayload = devctl & PCI_EXP_DEVCTL_PAYLOAD;
    }

    /*****************
     * reset cfg space
     */
    pciesvc_memcpy_toio(cs.cur, cs.rst, cfgsz);

    /* maxpayload setting preserved across FLR, restore saved value */
    if (rsttype == PCIESVC_RSTTYPE_FLR && pciecap) {
        devctl = cfgspace_readw(&cs, pciecap + PCI_EXP_DEVCTL);
        devctl &= ~PCI_EXP_DEVCTL_PAYLOAD;
        devctl |= maxpayload;
        cfgspace_writew(&cs, pciecap + PCI_EXP_DEVCTL, devctl);
    }

    /* Read reset value for cmd */
    cmd = cfgspace_readw(&cs, PCI_COMMAND);
    pciehw_cfg_cmd(phwdev, &cs, cmd);
    /* XXX Reset bar addrs? */

    /* bridge just reset secbus to reset value=0 */
    if (cfgspace_get_headertype(&cs) == 0x1) {
        pciehw_bridge_secbus(phwdev);
    }

    if (phwdev->pf) {
        u_int16_t sriovcap, sriovctrl, numvfs;

        /* Read reset values for sriovctrl, numvfs. */
        sriovcap = cfgspace_findextcap(&cs, PCI_EXT_CAP_ID_SRIOV);
        sriovctrl = cfgspace_readw(&cs, sriovcap + PCI_SRIOV_CTRL);
        numvfs = cfgspace_readw(&cs, sriovcap + PCI_SRIOV_NUM_VF);
        if (numvfs > phwdev->totalvfs) numvfs = phwdev->totalvfs;

        /* ARI-Capable bit preserved across FLR reset */
        if (rsttype == PCIESVC_RSTTYPE_FLR) {
            sriovctrl |= (phwdev->sriovctrl & PCI_SRIOV_CTRL_ARI);
            cfgspace_writew(&cs, sriovcap + PCI_SRIOV_CTRL, sriovctrl);
        }

        /* release our cfgspace before resetting vfs */
        pciesvc_cfgspace_put(&cs, DIRTY);

        pciehw_sriov_ctrl(phwdev, sriovctrl, numvfs);
        /* XXX Reset VF bar addrs? */
    } else {
        pciesvc_cfgspace_put(&cs, DIRTY);
    }
}
