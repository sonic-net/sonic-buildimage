/*
 * $Id: $
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 *
 */

#include <shbde_iproc.h>
#include <shbde_mdio.h>
#include <shbde_pci.h>

/* PAXB register offsets within PCI BAR0 window */
#define BAR0_PAXB_ENDIANESS                     0x2030
#define BAR0_PAXB_PCIE_EP_AXI_CONFIG            0x2104
#define BAR0_PAXB_CONFIG_IND_ADDR               0x2120
#define BAR0_PAXB_CONFIG_IND_DATA               0x2124

#define PAXB_0_CMICD_TO_PCIE_INTR_EN            0x2380

#define BAR0_PAXB_IMAP0_0                       (0x2c00)
#define BAR0_PAXB_IMAP0_1                       (0x2c04)
#define BAR0_PAXB_IMAP0_2                       (0x2c08)
#define BAR0_PAXB_IMAP0_3                       (0x2c0c)
#define BAR0_PAXB_IMAP0_4                       (0x2c10)
#define BAR0_PAXB_IMAP0_5                       (0x2c14)
#define BAR0_PAXB_IMAP0_6                       (0x2c18)
#define BAR0_PAXB_IMAP0_7                       (0x2c1c)

#define BAR0_PAXB_OARR_FUNC0_MSI_PAGE           0x2d34
#define BAR0_PAXB_OARR_2                        0x2d60
#define BAR0_PAXB_OARR_2_UPPER                  0x2d64
#define BAR0_DMU_PCU_PCIE_SLAVE_RESET_MODE      0x7024

#define PAXB_0_FUNC0_IMAP1_3                    0x2d88

/* Force byte pointer for offset adjustments */
#define ROFFS(_ptr, _offset) ((unsigned char*)(_ptr) + (_offset))

#define PAXB_CONFIG_IND_ADDRr_PROTOCOL_LAYERf_SHFT      11
#define PAXB_CONFIG_IND_ADDRr_PROTOCOL_LAYERf_MASK      0x3
#define PAXB_CONFIG_IND_ADDRr_ADDRESSf_SHFT             0
#define PAXB_CONFIG_IND_ADDRr_ADDRESSf_MASK             0x7ff
#define PAXB_0_FUNC0_IMAP1_3_ADDR_SHIFT                 20

/* Register value set/get by field */
#define REG_FIELD_SET(_r, _f, _r_val, _f_val) \
        _r_val = ((_r_val) & ~(_r##_##_f##_MASK << _r##_##_f##_SHFT)) | \
                 (((_f_val) & _r##_##_f##_MASK) << _r##_##_f##_SHFT)
#define REG_FIELD_GET(_r, _f, _r_val) \
        (((_r_val) >> _r##_##_f##_SHFT) & _r##_##_f##_MASK)

/* PCIe capabilities definition */
#ifndef PCI_EXP_LNKSTA
#define PCI_EXP_LNKSTA                          0x12
#endif
/* Current Link Speed 5.0GT/s */
#ifndef PCI_EXP_LNKSTA_CLS_5_0GB
#define PCI_EXP_LNKSTA_CLS_5_0GB                2
#endif
#ifndef PCI_EXP_LNKSTA2
#define PCI_EXP_LNKSTA2                         0x32
#endif
/* Current Deemphasis Level -3.5 dB */
#ifndef PCI_EXP_LNKSTA2_CDL_3_5DB
#define PCI_EXP_LNKSTA2_CDL_3_5DB               0x1
#endif

#define LOG_OUT(_shbde, _lvl, _str, _prm)             \
    if ((_shbde)->log_func) {                         \
        (_shbde)->log_func(_lvl, _str, _prm);         \
    }
#define LOG_ERR(_shbde, _str, _prm)     LOG_OUT(_shbde, SHBDE_ERR, _str, _prm)
#define LOG_WARN(_shbde, _str, _prm)    LOG_OUT(_shbde, SHBDE_WARN, _str, _prm)
#define LOG_DBG(_shbde, _str, _prm)     LOG_OUT(_shbde, SHBDE_DBG, _str, _prm)

static unsigned int
iproc32_read(shbde_hal_t *shbde, void *addr)
{
    if (!shbde || !shbde->io32_read) {
        return 0;
    }
    return shbde->io32_read(addr);
}

static void
iproc32_write(shbde_hal_t *shbde, void *addr, unsigned int data)
{
    if (!shbde || !shbde->io32_write) {
        return;
    }
    shbde->io32_write(addr, data);
}

static void
wait_usec(shbde_hal_t *shbde, int usec)
{
    if (shbde && shbde->usleep) {
        shbde->usleep(usec);
    } else {
        int idx;
        volatile int count;
        for (idx = 0; idx < usec; idx++) {
            for (count = 0; count < 100; count++);
        }
    }
}

static void
subwin_cache_init(shbde_hal_t *shbde, void *iproc_regs,
                  shbde_iproc_config_t *icfg)
{
    unsigned int idx;
    void *reg;

    /* Cache iProc sub-windows */
    for (idx = 0; idx < SHBDE_NUM_IPROC_SUBWIN; idx++) {
        reg = ROFFS(iproc_regs, BAR0_PAXB_IMAP0_0 + (4 * idx));
        icfg->subwin_base[idx] = iproc32_read(shbde, reg) & ~0xfff;
        LOG_DBG(shbde, "subwin:", icfg->subwin_base[idx]);
    }
    icfg->subwin_valid = 1;
}

/*
 * Function:
 *      shbde_iproc_config_init
 * Purpose:
 *      Initialize iProc configuration parameters
 * Parameters:
 *      icfg - pointer to empty iProc configuration structure
 * Returns:
 *      -1 if error, otherwise 0
 */
int
shbde_iproc_config_init(shbde_iproc_config_t *icfg,
                        unsigned int dev_id, unsigned int dev_rev)
{
    if (!icfg) {
        return -1;
    }

    /* Save device ID and revision */
    icfg->dev_id = dev_id;
    icfg->dev_rev = dev_rev;

    /* Check device families first */
    switch (icfg->dev_id & 0xfff0) {
    case 0x8400: /* Greyhound Lite */
    case 0x8410: /* Greyhound */
    case 0x8420: /* Bloodhound */
    case 0x8450: /* Elkhound */
    case 0xb060: /* Ranger2(Greyhound) */
    case 0x8360: /* Greyhound 53365 & 53369 */
    case 0xb260: /* saber2 */
    case 0xb460: /* saber2+ */
    case 0xb170: /* Hurricane3-MG */
    case 0x8570: /* Greyhound2 */
    case 0xb070: /* Greyhound2(emulation) */
    case 0x8580: /* Greyhound2(emulation) */
    case 0xb230: /* Dagger2 */
        icfg->iproc_ver = 7;
        icfg->dma_hi_bits = 0x2;
        break;
    case 0xb670: /* MO */
        icfg->iproc_ver = 0xB;
        break;
    case 0xb160: /* Hurricane3 */
    case 0x8440: /* Buckhound2 */
    case 0x8430: /* Foxhound2 */
    case 0x8540: /* Wolfhound2 */
        icfg->iproc_ver = 10;
        icfg->dma_hi_bits = 0x2;
        break;
    default:
        break;
    }

    /* Check for exceptions */
    switch (icfg->dev_id) {
    case 0xb168: /* Ranger3+ */
    case 0xb169:
        icfg->iproc_ver = 0;
        icfg->dma_hi_bits = 0;
        break;
    default:
        break;
    }
    /* Check for PCIe PHY address that needs PCIe preemphasis and
     * assign the MDIO base address
     */
    switch (icfg->dev_id & 0xfff0) {
    case 0xb150: /* Hurricane2 */
    case 0x8340: /* Wolfhound */
    case 0x8330: /* Foxhound */
    case 0x8390: /* Dearhound */
        icfg->mdio_base_addr = 0x18032000;
        icfg->pcie_phy_addr = 0x2;
        break;
    case 0xb340: /* Helilx4 */
    case 0xb540: /* FireScout */
    case 0xb040: /* Spiral, Ranger */
        icfg->mdio_base_addr = 0x18032000;
        icfg->pcie_phy_addr = 0x5;
        icfg->adjust_pcie_preemphasis = 1;
        break;
    case 0xb240: /* Saber */
        icfg->mdio_base_addr = 0x18032000;
        icfg->pcie_phy_addr = 0x5;
        icfg->adjust_pcie_preemphasis = 1;
        break;
    default:
        break;
    }

    /* Check for exceptions */
    switch (icfg->dev_id) {
    default:
        break;
    }

    return 0;
}

/*
 * Function:
 *      shbde_iproc_paxb_init
 * Purpose:
 *      Initialize iProc PCI-AXI bridge for CMIC access
 * Parameters:
 *      shbde - pointer to initialized hardware abstraction module
 *      iproc_regs - memory mapped iProc registers in PCI BAR
 *      icfg - iProc configuration parameters
 * Returns:
 *      -1 if error, otherwise 0
 */
int
shbde_iproc_paxb_init(shbde_hal_t *shbde, void *iproc_regs,
                      shbde_iproc_config_t *icfg)
{
    void *reg;
    unsigned int data;
    int pci_num;

    if (!iproc_regs || !icfg) {
        return -1;
    }

    LOG_DBG(shbde, "iProc version:", icfg->iproc_ver);

    /*
     * The following code attempts to auto-detect the correct
     * iProc PCI endianess configuration by reading a well-known
     * register (the endianess configuration register itself).
     * Note that the PCI endianess may be different for different
     * big endian host processors.
     */
    reg = ROFFS(iproc_regs, BAR0_PAXB_ENDIANESS);
    /* Select big endian */
    iproc32_write(shbde, reg, 0x01010101);
    /* Check if endianess register itself is correct endian */
    if (iproc32_read(shbde, reg) != 1) {
        /* If not, then assume little endian */
        iproc32_write(shbde, reg, 0x0);
    }

    /* Select which PCI core to use */
    pci_num = 0;
    reg = ROFFS(iproc_regs, BAR0_PAXB_IMAP0_2);
    data = iproc32_read(shbde, reg);
    if (data  & 0x1000) {
        /* PAXB_1 is mapped to sub-window 2 */
        pci_num = 1;
    }

    /* Default DMA mapping if uninitialized */
    if (icfg->dma_hi_bits == 0) {
        icfg->dma_hi_bits = 0x1;
        if (pci_num == 1) {
            icfg->dma_hi_bits = 0x2;
        }
    }

    /* Enable iProc DMA to external host memory */
    reg = ROFFS(iproc_regs, BAR0_PAXB_PCIE_EP_AXI_CONFIG);
    iproc32_write(shbde, reg, 0x0);
    if(icfg->cmic_ver < 4) { /* Non-CMICX */
        reg = ROFFS(iproc_regs, BAR0_PAXB_OARR_2);
        iproc32_write(shbde, reg, 0x1);
        reg = ROFFS(iproc_regs, BAR0_PAXB_OARR_2_UPPER);
        iproc32_write(shbde, reg, icfg->dma_hi_bits);
        /* Configure MSI interrupt page */
        if (icfg->use_msi) {
            reg = ROFFS(iproc_regs, BAR0_PAXB_OARR_FUNC0_MSI_PAGE);
            data = iproc32_read(shbde, reg);
            iproc32_write(shbde, reg, data | 0x1);
        }
    }

    /* Configure MSIX interrupt page, need for iproc ver 16, 17 and 18 */
    if (icfg->use_msi == 2 &&
        (icfg->iproc_ver >= 16 && icfg->iproc_ver <= 18)) {
        unsigned int mask = (0x1 << PAXB_0_FUNC0_IMAP1_3_ADDR_SHIFT) - 1;
        reg = ROFFS(iproc_regs, PAXB_0_FUNC0_IMAP1_3);
        data = iproc32_read(shbde, reg);
        data &= mask;
        if (icfg->iproc_ver == 0x11) {
            data |= 0x400 << PAXB_0_FUNC0_IMAP1_3_ADDR_SHIFT;
        } else {
            data |= 0x410 << PAXB_0_FUNC0_IMAP1_3_ADDR_SHIFT;
        }

        iproc32_write(shbde, reg, data);
    }

    /* Disable INTx interrupt if MSI/MSIX is selected */
    reg = ROFFS(iproc_regs, PAXB_0_CMICD_TO_PCIE_INTR_EN);
    data = iproc32_read(shbde, reg);
    if (icfg->use_msi) {
        data &= ~0x1;
    } else {
        data |= 0x1;
    }
    iproc32_write(shbde, reg, data);

    /* Cache iProc sub-windows */
    subwin_cache_init(shbde, iproc_regs, icfg);

    return pci_num;
}

/*
 * Function:
 *      shbde_iproc_pci_read
 * Purpose:
 *      Read iProc register through PCI BAR 0
 * Parameters:
 *      shbde - pointer to initialized hardware abstraction module
 *      iproc_regs - memory mapped iProc registers in PCI BAR
 *      addr - iProc register address in AXI memory space
 * Returns:
 *      Register value
 */
unsigned int
shbde_iproc_pci_read(shbde_hal_t *shbde, void *iproc_regs,
                     unsigned int addr)
{
    unsigned int subwin_base;
    unsigned int idx;
    void *reg = 0;
    shbde_iproc_config_t *icfg = &shbde->icfg;

    if (!iproc_regs) {
        return -1;
    }

    if (icfg->subwin_valid == 0) {
        LOG_WARN(shbde, "Re-initializing PCI sub-windows",
                 shbde->icfg.iproc_ver);
        subwin_cache_init(shbde, iproc_regs, icfg);
    }

    /* Sub-window size is 0x1000 (4K) */
    subwin_base = (addr & ~0xfff);

    /* Look for matching sub-window */
    for (idx = 0; idx < SHBDE_NUM_IPROC_SUBWIN; idx++) {
        if (idx == 7 && icfg->no_subwin_remap) {
            /*
             * If sub-window remapping is not permitted, issue a
             * warning if none of the fixed sub-windows are
             * matching. We still allow the remapping to take place in
             * order to avoid breaking existing (unsafe) code.
             */
            LOG_WARN(shbde, "No matching PCI sub-window for", addr);
            break;
        }
        if (icfg->subwin_base[idx] == subwin_base) {
            reg = ROFFS(iproc_regs, idx * 0x1000 + (addr & 0xfff));
            break;
        }
    }

    /* No matching sub-window, reuse the sub-window 7 */
    if (reg == 0) {
        /* Update base address for sub-window 7 */
        subwin_base |= 1; /* Valid bit */
        reg = ROFFS(iproc_regs, BAR0_PAXB_IMAP0_7);
        iproc32_write(shbde, reg, subwin_base);
        /* Read it to make sure the write actually goes through */
        subwin_base = iproc32_read(shbde, reg);
        /* Update cache */
        icfg->subwin_base[7] = subwin_base;

        /* Read register through sub-window 7 */
        reg = ROFFS(iproc_regs, 0x7000 + (addr & 0xfff));
    }

    return iproc32_read(shbde, reg);
}

/*
 * Function:
 *      shbde_iproc_pci_write
 * Purpose:
 *      Write iProc register through PCI BAR 0
 * Parameters:
 *      shbde - pointer to initialized hardware abstraction module
 *      iproc_regs - memory mapped iProc registers in PCI BAR
 *      addr - iProc register address in AXI memory space
 *      data - data to write to iProc register
 * Returns:
 *      Register value
 */
void
shbde_iproc_pci_write(shbde_hal_t *shbde, void *iproc_regs,
                      unsigned int addr, unsigned int data)
{
    unsigned int subwin_base;
    unsigned int idx;
    void *reg = 0;
    shbde_iproc_config_t *icfg = &shbde->icfg;

    if (!iproc_regs) {
        return;
    }

    if (icfg->subwin_valid == 0) {
        LOG_WARN(shbde, "Re-initializing PCI sub-windows",
                 shbde->icfg.iproc_ver);
        subwin_cache_init(shbde, iproc_regs, icfg);
    }

    /* Sub-window size is 0x1000 (4K) */
    subwin_base = (addr & ~0xfff);

    /* Look for matching sub-window */
    for (idx = 0; idx < SHBDE_NUM_IPROC_SUBWIN; idx++) {
        if (idx == 7 && icfg->no_subwin_remap) {
            /*
             * If sub-window remapping is not permitted, issue a
             * warning if none of the fixed sub-windows are
             * matching. We still allow the remapping to take place in
             * order to avoid breaking existing (unsafe) code.
             */
            LOG_WARN(shbde, "No matching PCI sub-window for", addr);
            break;
        }
        if (icfg->subwin_base[idx] == subwin_base) {
            reg = ROFFS(iproc_regs, idx * 0x1000 + (addr & 0xfff));
            break;
        }
    }

    /* No matching sub-window, reuse the sub-window 7 */
    if (reg == 0) {
        /* Update base address for sub-window 7 */
        subwin_base |= 1; /* Valid bit */
        reg = ROFFS(iproc_regs, BAR0_PAXB_IMAP0_7);
        iproc32_write(shbde, reg, subwin_base);
        /* Read it to make sure the write actually goes through */
        subwin_base = iproc32_read(shbde, reg);
        /* Update cache */
        icfg->subwin_base[7] = subwin_base;

        /* Read register through sub-window 7 */
        reg = ROFFS(iproc_regs, 0x7000 + (addr & 0xfff));
    }

    iproc32_write(shbde, reg, data);
}

int
shbde_iproc_pcie_preemphasis_set(shbde_hal_t *shbde, void *iproc_regs,
                                 shbde_iproc_config_t *icfg, void *pci_dev)
{
    shbde_mdio_ctrl_t mdio_ctrl, *smc = &mdio_ctrl;
    unsigned int phy_addr, data;
    void *reg;
    unsigned int pcie_cap_base;
    unsigned short link_stat, link_stat2;

    if (!icfg) {
        return -1;
    }

    /* PHY address for PCIe link */
    phy_addr = icfg->pcie_phy_addr;
    if (phy_addr == 0 || icfg->mdio_base_addr == 0) {
        return 0;
    }

    /* Initialize MDIO control */
    smc->shbde = shbde;
    smc->regs = iproc_regs;
    smc->base_addr = icfg->mdio_base_addr;
    smc->io32_read = shbde_iproc_pci_read;
    smc->io32_write = shbde_iproc_pci_write;
    shbde_iproc_mdio_init(smc);

    /* PCIe SerDes Gen1/Gen2 CDR Track Bandwidth Adjustment
     * for Better Jitter Tolerance
     */
    shbde_iproc_mdio_write(smc, phy_addr, 0x1f, 0x8630);
    shbde_iproc_mdio_write(smc, phy_addr, 0x13, 0x190);
    shbde_iproc_mdio_write(smc, phy_addr, 0x19, 0x191);

    if (!icfg->adjust_pcie_preemphasis) {
        return 0;
    }

    /* Check to see if the PCIe SerDes deemphasis needs to be changed
     * based on the advertisement from the root complex
     */
    /* Find PCIe capability base */
    if (!shbde || !shbde->pcic16_read || !pci_dev) {
        return -1;
    }
    pcie_cap_base = shbde_pci_pcie_cap(shbde, pci_dev);
    if (pcie_cap_base) {
        link_stat = shbde->pcic16_read(pci_dev,
                                       pcie_cap_base + PCI_EXP_LNKSTA);
        link_stat2 = shbde->pcic16_read(pci_dev,
                                        pcie_cap_base + PCI_EXP_LNKSTA2);
        if (((link_stat & 0xf) == PCI_EXP_LNKSTA_CLS_5_0GB) &&
            (link_stat2 & PCI_EXP_LNKSTA2_CDL_3_5DB)) {
            /* Device is operating at Gen2 speeds and RC requested -3.5dB */
            /* Change the transmitter setting */
            shbde_iproc_mdio_write(smc, phy_addr, 0x1f, 0x8610);
            shbde_iproc_mdio_read(smc, phy_addr, 0x17, &data);
            data &= ~0xf00;
            data |= 0x700;
            shbde_iproc_mdio_write(smc, phy_addr, 0x17, data);

            /* Force the PCIe link to retrain */
            data = 0;
            REG_FIELD_SET(PAXB_CONFIG_IND_ADDRr, PROTOCOL_LAYERf, data, 0x2);
            REG_FIELD_SET(PAXB_CONFIG_IND_ADDRr, ADDRESSf, data, 0x4);
            reg = ROFFS(iproc_regs, BAR0_PAXB_CONFIG_IND_ADDR);
            iproc32_write(shbde, reg, data);

            reg = ROFFS(iproc_regs, BAR0_PAXB_CONFIG_IND_DATA);
            data = iproc32_read(shbde, reg);
            data &= ~0x4000;
            iproc32_write(shbde, reg, data);
            data |= 0x4000;
            iproc32_write(shbde, reg, data);
            data &= ~0x4000;
            iproc32_write(shbde, reg, data);

            /* Wait a short while for the retraining to complete */
            wait_usec(shbde, 1000);
        }
    }

    return 0;
}

