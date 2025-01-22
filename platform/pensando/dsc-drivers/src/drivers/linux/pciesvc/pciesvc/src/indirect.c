// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018,2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "pcietlp.h"
#include "req_int.h"
#include "indirect.h"

#define IND_INFO_BASE   PXB_(STA_TGT_IND_INFO)
#define IND_INFO_NWORDS 1
#define IND_INFO_STRIDE 4

static u_int64_t
ind_info_addr(const int port)
{
    return IND_INFO_BASE + (port * IND_INFO_STRIDE);
}

static u_int64_t
indirect_int_addr(void)
{
    return PXB_(CFG_TGT_REQ_INDIRECT_INT);
}

/*****************************************************************
 * aximst rams
 */
#define AXIMST_BASE     PXB_(DHS_TGT_AXIMST0)
#define AXIMST_STRIDE   \
    (ASIC_(PXB_CSR_DHS_TGT_AXIMST1_BYTE_ADDRESS) - \
     ASIC_(PXB_CSR_DHS_TGT_AXIMST0_BYTE_ADDRESS))

#define AXIMST_NWORDS           4
#define AXIMST_ENTRY_STRIDE     32
#define AXIMST_ENTRIES_PER_PORT 16
#define AXIMST_PORTS_PER_ROW    8
#define AXIMST_PORT_STRIDE      (AXIMST_ENTRY_STRIDE * AXIMST_ENTRIES_PER_PORT)

static u_int64_t
aximst_addr(const unsigned int port,
            const unsigned int idx,
            const unsigned int entry)
{
    return (AXIMST_BASE +
            ((u_int64_t)idx * AXIMST_STRIDE) +
            ((u_int64_t)port * AXIMST_PORT_STRIDE) +
            ((u_int64_t)entry * AXIMST_ENTRY_STRIDE));
}

static void
read_aximst(const unsigned int port,
            const unsigned int idx,
            const unsigned int entry,
            u_int32_t *buf)
{
    const u_int64_t pa = aximst_addr(port, idx, entry);

    pciesvc_reg_rd32w(pa, buf, AXIMST_NWORDS);
}

/*
 * Indirect info tlp format is reversed in srams:
 *
 *    15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * --------------------------------------------------
 * 0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 1: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 2: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 <= tlp[16]
 * 3: 00 00 00 00 60 02 00 3d 0f 00 00 3a 01 00 00 05 <= tlp[0]
 * 4: 24 48 00 00 00 04 c0 bc 05 78 02 00 40 0e 41 c4 <= indirect info
 */
static void
decode_indirect_info(u_int8_t *info, indirect_entry_t *ientry)
{
    u_int8_t *p;
    int i;

    /* copy the raw tlp data */
    p = (u_int8_t *)&ientry->rtlp;
    for (i = 0; i < sizeof(ientry->rtlp); i++) {
        p[i] = info[63 - i];
    }

    /* copy the tlp aux info */
    p = (u_int8_t *)&ientry->info;
    for (i = 0; i < sizeof(ientry->info); i++) {
        p[i] = info[64 + i];
    }
}

static void
read_ind_info(const unsigned int port, int *entryp, int *pendingp)
{
    union {
        struct {
            u_int32_t pending:1;
            u_int32_t entry:4;
            u_int32_t port:3;
        } __attribute__((packed));
        u_int32_t w[IND_INFO_NWORDS];
    } ind_info;

    pciesvc_reg_rd32w(ind_info_addr(port), ind_info.w, IND_INFO_NWORDS);

    /* pciesvc_assert(ind_info.port == port); */

    if (entryp) *entryp = ind_info.entry;
    if (pendingp) *pendingp = ind_info.pending;
}

/*
 * Fill the buffer with the raw indirect info data
 * from the aximst srams.
 */
static void
read_indirect_info(const unsigned int port,
                   const unsigned int entry,
                   u_int8_t *buf)
{
    u_int8_t *bp;
    int i;

    for (bp = buf, i = 0; i < 5; i++, bp += 16) {
        read_aximst(port, i, entry, (u_int32_t *)bp);
    }
}

static void
read_indirect_entry(const unsigned int port,
                    const unsigned int entry,
                    indirect_entry_t *ientry)
{
    u_int8_t buf[80];

    read_indirect_info(port, entry, buf);
    decode_indirect_info(buf, ientry);
    ientry->port = port;
}

static int
read_pending_indirect_entry(const unsigned int port,
                            indirect_entry_t *ientry)
{
    int entry, pending;

    read_ind_info(port, &entry, &pending);
    read_indirect_entry(port, entry, ientry);
    return pending;
}

void
pciehw_indirect_complete(indirect_entry_t *ientry)
{
#define IND_RSP_ADDR    PXB_(DHS_TGT_IND_RSP_ENTRY)
#define IND_RSP_NWORDS  5
    union {
        struct {
            u_int32_t data0;
            u_int32_t data1;
            u_int32_t data2;
            u_int32_t data3;
            u_int32_t cpl_stat:3;
            u_int32_t port_id:3;
            u_int32_t axi_id:7;
            u_int32_t fetch_rsp:1;
        } __attribute__((packed));
        u_int32_t w[IND_RSP_NWORDS];
    } ind_rsp;
    const u_int64_t pa = ientry->info.direct_addr;
    const size_t sz = ientry->info.direct_size;

    if (ientry->completed) return;

    /*
     * This indirect transaction was handled by software.
     * We might have written some memory that will be read
     * by subsequent direct transactions handled in hw.
     * Insert barrier here to be sure all memory writes have
     * landed so hw will always see the data we wrote.
     */
    pciesvc_mem_barrier();

    if (sz < 4 && (pa & 0x3)) {
        /*
         * If sub-dword read, shift return data to the correct
         * byte lanes expected for this transaction.
         *
         *     data0 = data0 << (address-dword-offset * 8);
         */
        ind_rsp.data0 = ientry->data[0] << ((pa & 0x3) << 3);
    } else {
        ind_rsp.data0 = ientry->data[0];
        ind_rsp.data1 = ientry->data[1];
        ind_rsp.data2 = ientry->data[2];
        ind_rsp.data3 = ientry->data[3];
    }
    ind_rsp.cpl_stat = ientry->cpl;
    ind_rsp.port_id = ientry->port;
    ind_rsp.axi_id = ientry->info.context_id;
    ind_rsp.fetch_rsp = 0;

    pciesvc_reg_wr32w(IND_RSP_ADDR, ind_rsp.w, IND_RSP_NWORDS);

    ientry->completed = 1;
}

static void
handle_indirect(const int port, pciehw_port_t *p, indirect_entry_t *ientry)
{
    const u_int32_t pmti = ientry->info.pmti;
    pciehw_spmt_t *spmt = pciesvc_spmt_get(pmti);
    const pcie_tlp_common_hdr_t *hdr = (void *)ientry->rtlp;
    const u_int8_t tlp_type = hdr->type;

    switch (tlp_type) {
    case PCIE_TLP_TYPE_CFGRD0:
    case PCIE_TLP_TYPE_CFGRD1:
        pciehw_cfgrd_indirect(port, ientry);
        spmt->swrd++;
        p->stats.ind_cfgrd++;
        break;
    case PCIE_TLP_TYPE_CFGWR0:
    case PCIE_TLP_TYPE_CFGWR1:
        pciehw_cfgwr_indirect(port, ientry);
        spmt->swwr++;
        p->stats.ind_cfgwr++;
        break;
    case PCIE_TLP_TYPE_MEMRD:
    case PCIE_TLP_TYPE_MEMRD64:
        pciehw_barrd_indirect(port, ientry);
        spmt->swrd++;
        p->stats.ind_memrd++;
        break;
    case PCIE_TLP_TYPE_MEMWR:
    case PCIE_TLP_TYPE_MEMWR64:
        pciehw_barwr_indirect(port, ientry);
        spmt->swwr++;
        p->stats.ind_memwr++;
        break;
    case PCIE_TLP_TYPE_IORD:
        pciehw_barrd_indirect(port, ientry);
        spmt->swrd++;
        p->stats.ind_iord++;
        break;
    case PCIE_TLP_TYPE_IOWR:
        pciehw_barwr_indirect(port, ientry);
        spmt->swwr++;
        p->stats.ind_iowr++;
        break;
    default:
        ientry->cpl = PCIECPL_UR;
        pciehw_indirect_complete(ientry);
        p->stats.ind_unknown++;
        break;
    }

    pciesvc_spmt_put(spmt, DIRTY);
}

/******************************************************************
 * apis
 */

int
pciehw_indirect_intr_init(const int port,
                          const u_int64_t msgaddr, const u_int32_t msgdata)
{
    return req_int_init(indirect_int_addr(), port,
                        msgaddr, msgdata | MSGDATA_ADD_PORT);
}

static int
pciehw_indirect_handle(const int port, const int polled)
{
    pciehw_port_t *p = pciesvc_port_get(port);
    indirect_entry_t ientrybuf, *ientry = &ientrybuf;
    int pending;
    int r = 0;

    pciesvc_memset(ientry, 0, sizeof(*ientry));
    pending = read_pending_indirect_entry(port, ientry);

    p->stats.ind_intr++;
    if (polled) p->stats.ind_polled++;
    if (!pending) {
        p->stats.ind_spurious++;
        goto out;
    }

    ientry->cpl = PCIECPL_SC; /* assume success */
    handle_indirect(port, p, ientry);
    r = 1;

 out:
    pciesvc_port_put(p, DIRTY);
    return r;
}

int
pciehw_indirect_intr(const int port)
{
    return pciehw_indirect_handle(port, 0);
}

/*
 * Arrange to have the notify interrupt written to memory,
 * then we can poll memory locations to see if there is work to do.
 */
int
pciehw_indirect_poll_init(const int port)
{
    const u_int64_t msgaddr = pciesvc_indirect_intr_dest_pa(port);
    const u_int32_t msgdata = 1;

    return req_int_init(indirect_int_addr(), port, msgaddr, msgdata);
}

int
pciehw_indirect_poll(const int port)
{
    int pending;
    int r = 0;

    read_ind_info(port, NULL, &pending);
    if (pending) {
        r = pciehw_indirect_handle(port, 1);
    }
    return r;
}
