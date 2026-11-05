// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018-2019,2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "intrutils.h"
#include "serial.h"
#include "reset.h"

static void
pciehw_reset_lifs_event(pciehwdev_t *phwdev,
                        const int lifb, const int lifc,
                        const pciesvc_rsttype_t rsttype)
{
    pciesvc_eventdata_t evd;
    pciesvc_reset_t *reset;

    pciesvc_memset(&evd, 0, sizeof(evd));
    evd.evtype = PCIESVC_EV_RESET;
    evd.port = phwdev->port;
    evd.lif = phwdev->lifb;
    reset = &evd.reset;
    reset->rsttype = rsttype;
    reset->lifb = lifb;
    reset->lifc = lifc;
    pciesvc_event_handler(&evd, sizeof(evd));
}

static void
pciehw_reset_event(pciehwdev_t *phwdev, const pciesvc_rsttype_t rsttype)
{
    /* skip bridges, no lif so no reset event */
    if (phwdev->lifc) {
        pciehw_reset_lifs_event(phwdev, phwdev->lifb, phwdev->lifc, rsttype);
    }
}

static void
pciehw_reset_device_intrs(pciehwdev_t *phwdev, const pciesvc_rsttype_t rsttype)
{
    const int dmask = phwdev->intrdmask;
    int i;

    if (phwdev->novrdintr) {
        for (i = 0; i < phwdev->novrdintr; i++) {
            const u_int32_t intrb = phwdev->ovrdintr[i].intrb;
            const u_int32_t intrc = phwdev->ovrdintr[i].intrc;

            intr_reset_pci(intrb, intrc, dmask);
        }
    } else {
        intr_reset_pci(phwdev->intrb, phwdev->intrc, dmask);
    }
}

static void
pciehw_reset_device(pciehwdev_t *phwdev, const pciesvc_rsttype_t rsttype)
{
#ifdef PCIEMGR_DEBUG
    pciesvc_logdebug("%s: dev reset\n", pciehwdev_get_name(phwdev));
#endif

    if (rsttype != PCIESVC_RSTTYPE_NONE) {
        pciehw_reset_event(phwdev, rsttype);
    }
    pciehw_reset_device_intrs(phwdev, rsttype);
    pciehw_cfg_reset(phwdev, rsttype);

    switch (phwdev->type) {
    case PCIEHDEVICE_SERIAL:
        serial_reset(phwdev, rsttype);
        break;
    default:
        break;
    }
}

static void
pciehw_reset_descendents(pciehwdevh_t hwdevh, const pciesvc_rsttype_t rsttype)
{
    while (hwdevh) {
        pciehwdev_t *phwdev = pciehwdev_get(hwdevh);
        const int is_pf = phwdev->pf;
        const pciehwdevh_t childh = phwdev->childh;
        const pciehwdevh_t peerh = phwdev->peerh;

        pciehw_reset_device(phwdev, rsttype);
        pciehwdev_put(phwdev, DIRTY);

        /*
         * If we are a PF then resetting our cfg space will disable and
         * reset all active VFs so no need to reset them again.  If this
         * is a bridge with child devices, go reset those children here.
         */
        if (!is_pf) {
            pciehw_reset_descendents(childh, rsttype);
        }

        hwdevh = peerh;
    }
}

/*
 * A "bus" reset originates on a bridge device with a request
 * for a secondary bus reset.  We're called with the phwdev of
 * the bridge, but the bridge doesn't get reset.  We reset all
 * the descendents of the bridge device.
 */
void
pciehw_reset_bus(pciehwdev_t *phwdev, const u_int8_t bus)
{
    pciesvc_loginfo("%s: bus reset 0x%02x\n", pciehwdev_get_name(phwdev), bus);
    pciehw_reset_descendents(phwdev->childh, PCIESVC_RSTTYPE_BUS);
}

/*
 * Function Level Reset (FLR) is issued on a device endpoint to reset
 * the device.  If issued on a PF then all the VFs get reset too.
 */
void
pciehw_reset_flr(pciehwdev_t *phwdev)
{
    pciesvc_loginfo("%s: flr reset\n", pciehwdev_get_name(phwdev));
    pciehw_reset_device(phwdev, PCIESVC_RSTTYPE_FLR);
}

/*
 * A PF controls enabling of VFs.  If some enabled VFs get disabled
 * by the PF then we want to reset the VFs.
 *
 * In order to reduce the number of msgs generated for this reset event
 * we compress all the VF reset msgs into a single reset msg spanning
 * all the lifs that were affected.
 */
void
pciehw_reset_vfs(pciehwdev_t *phwdev, const int vfb, const int vfc)
{
    pciehwdev_t *vfhwdev;
    int vfidx, vflifb, vflifc;

    pciesvc_loginfo("%s: vfs reset %d-%d\n",
                    pciehwdev_get_name(phwdev), vfb, vfb + vfc - 1);
    vflifb = 0;
    vflifc = 0;
    for (vfidx = vfb; vfidx < vfb + vfc; vfidx++) {
        vfhwdev = pciehwdev_vfdev_get(phwdev, vfidx);
        if (vfidx == vfb) {
            /* save these from first reset vf for event */
            vflifb = vfhwdev->lifb;
            vflifc = vfhwdev->lifc;
        }
        pciehw_reset_device(vfhwdev, PCIESVC_RSTTYPE_NONE);
        pciehwdev_vfdev_put(vfhwdev, DIRTY);
    }
    pciehw_reset_lifs_event(phwdev, vflifb, vflifc * vfc, PCIESVC_RSTTYPE_VF);
}
