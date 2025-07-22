// SPDX-License-Identifier: GPL-2.0+
/* Copyright (c) 2021 Motor-comm Corporation.
 * Confidential and Proprietary. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */


#include <linux/kernel.h>
#include <linux/module.h>

#include "fuxi-os.h"
#include "fuxi-gmac.h"
#include "fuxi-gmac-reg.h"

/* proc fs */
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/fs.h>

MODULE_LICENSE("Dual BSD/GPL");

static int debug = 0;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "FUXI ethernet debug level (0=none,...,16=all)");

#define FAUX_DEBUG_LEVEL_INFO     (0x1 << 0)
#define FAUX_DEBUG_LEVEL_ERROR    (0x1 << 1)


#define FAUX_DEBUG_INFO(fmt,  args...) \
    if (debug & FAUX_DEBUG_LEVEL_INFO) { \
        printk(KERN_INFO "[FUXI-GMAC][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    }

#define FAUX_DEBUG_ERROR(fmt,  args...) \
    if (debug & FAUX_DEBUG_LEVEL_ERROR) { \
        printk(KERN_ERR "[FUXI-GMAC][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    }


static unsigned char dev_addr[6] = {0, 0x55, 0x7b, 0xb5, 0x7d, 0xf7};

#define YT6801_NETDEV_NAME_SIZE   (32)

/*
 * For debugging scenarios, 
 * it can ensure that there are no mutual exclusion issues 
 * and does not use any locks for protection. 
 */
static LIST_HEAD(proc_list_head);

static void yt6801_debug_init(struct fxgmac_pdata *pdata)
{
    INIT_LIST_HEAD(&(pdata->proc_debug_list));
    list_add_tail(&(pdata->proc_debug_list), &proc_list_head);

    return;
}

static void yt6801_debug_deinit(void)
{
    struct fxgmac_pdata *pos, *n;

    list_for_each_entry_safe(pos, n, &proc_list_head, proc_debug_list) {
        list_del(&(pos->proc_debug_list));
        INIT_LIST_HEAD(&(pos->proc_debug_list));
    }

    return;
}

static struct net_device *yt6801_get_netdev_by_name(unsigned char *name)
{
    struct fxgmac_pdata *pos;
    size_t len;

    len = strlen((const char *)name);
    list_for_each_entry(pos, &proc_list_head, proc_debug_list) {
        if ((len == strlen((const char *)(pos->netdev->name))) 
                && (strncmp((const char *)name, (const char *)(pos->netdev->name), len) == 0)) {
            return pos->netdev;
        }
    }

    return NULL;
}

static int phy_info_show(struct seq_file *m, void *v)
{
    seq_printf(m, "no support \n");
    return 0;
}

static int phy_info_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, phy_info_show, NULL);
}

static ssize_t yt6801_phy_proc_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
    struct fxgmac_pdata *pdata;
    char buf[64];
    size_t len = min(sizeof(buf) - 1, count);
    u32 reg_addr = 0, reg_val = 0;
    int ret;
    char rw_flag = 0;
    struct net_device *dev;
    unsigned char netdev_name[YT6801_NETDEV_NAME_SIZE];
    struct fxgmac_hw_ops* hw_ops;

    memset(buf, 0, sizeof(buf));

    if (copy_from_user(buf, buffer, len)) {
        FAUX_DEBUG_ERROR("copy_from_user error\n");
        return count;
    }

    ret = sscanf(buf, "%s %c %x %x", &netdev_name, &rw_flag, &reg_addr, &reg_val);
    if (ret != 4) {
         printk(KERN_INFO "para err, usage: echo p1 p2 p3 p4 > /proc/yt6801_phy_rw\n"
                "p1: netdev name\n"
                "p2: rw_flag  r/w   read or write PCIe NIC internal phy reg\n"
                "p3: reg_addr\n"
                "p4: reg_val\n");
         return count;
    }

    FAUX_DEBUG_INFO("%s %c %x %x.\n", netdev_name, rw_flag, reg_addr, reg_val);

    dev = yt6801_get_netdev_by_name(netdev_name);
    if (dev == NULL) {
        FAUX_DEBUG_ERROR("Can not find netdev: %s.\n", netdev_name);
        return count;
    }

    pdata  = netdev_priv(dev);
    hw_ops = &pdata->hw_ops;

    /* Read and write the registers of the external PHY of the PCIe network card. */
    if (rw_flag == 'w') {
        (void)hw_ops->write_ephy_reg(pdata, reg_addr, reg_val);
        printk(KERN_INFO "\n write internal phy --- reg_addr: 0x%x val: 0x%x\n", reg_addr, reg_val);
    } else if (rw_flag == 'r') {
        (void)hw_ops->read_ephy_reg(pdata, reg_addr, &reg_val);
        printk(KERN_INFO "\n read internal phy --- reg_addr: 0x%x val: 0x%x\n", reg_addr, reg_val);
    }

    return count;
}

static const struct proc_ops  yt6801_phy_rw_proc_fops = {
	.proc_open	= phy_info_proc_open,
	.proc_read	= seq_read,
	.proc_write	= yt6801_phy_proc_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

void yt6801_proc_init(void)
{
    (void)proc_create("yt6801_phy_rw", 0, NULL, &yt6801_phy_rw_proc_fops);
    return;
}

void yt6801_proc_deinit(void)
{
    remove_proc_entry("yt6801_phy_rw", NULL);
    return;
}

static void fxgmac_read_mac_addr(struct fxgmac_pdata *pdata)
{
    struct net_device *netdev = pdata->netdev;
    struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
    /* DPRINTK("read mac from eFuse\n");  */

    /* if efuse have mac addr,use it.if not,use static mac address. */
    hw_ops->read_mac_subsys_from_efuse(pdata, pdata->mac_addr, NULL, NULL);
    if (ETH_IS_ZEROADDRESS(pdata->mac_addr)) {
        /* Currently it uses a static mac address for test */
        memcpy(pdata->mac_addr, dev_addr, netdev->addr_len);
    }
}

static void fxgmac_default_config(struct fxgmac_pdata *pdata)
{
    pdata->tx_osp_mode = DMA_OSP_ENABLE;
    pdata->tx_sf_mode = MTL_TSF_ENABLE;
    pdata->rx_sf_mode = MTL_RSF_ENABLE;//MTL_RSF_DISABLE 20210514
    pdata->pblx8 = DMA_PBL_X8_ENABLE;//DMA_PBL_X8_ENABLE 20210514
    pdata->tx_pbl = DMA_PBL_16;
    pdata->rx_pbl = DMA_PBL_4;//DMA_PBL_32 20210514
    pdata->tx_threshold = MTL_TX_THRESHOLD_128;
    pdata->rx_threshold = MTL_RX_THRESHOLD_128;
#if 1
    pdata->tx_pause = 1;
    pdata->rx_pause = 1;
#else
    pdata->tx_pause = 0;
    pdata->rx_pause = 0;
#endif
#if FXGMAC_RSS_FEATURE_ENABLED
    pdata->rss = 1;
#else
    pdata->rss = 0;
#endif
    // open interrupt moderation default
    pdata->intr_mod = 1;
    pdata->crc_check = 1;

    //yzhang, set based on phy status. pdata->phy_speed = SPEED_1000;
    pdata->sysclk_rate = FXGMAC_SYSCLOCK;
    pdata->phy_autoeng = AUTONEG_ENABLE; // default to autoneg
    pdata->phy_duplex = DUPLEX_FULL;
    pdata->expansion.phy_link = false;
    pdata->phy_speed = SPEED_1000;

    // default to magic
    pdata->expansion.wol = WAKE_MAGIC;

    strscpy(pdata->drv_name, FXGMAC_DRV_NAME, sizeof(pdata->drv_name));
    strscpy(pdata->drv_ver, FXGMAC_DRV_VERSION, sizeof(pdata->drv_ver));
    dev_info(pdata->dev, "FXGMAC_DRV_NAME:%s, FXGMAC_DRV_VERSION:%s\n",
                                        FXGMAC_DRV_NAME, FXGMAC_DRV_VERSION);
}

static void fxgmac_init_all_ops(struct fxgmac_pdata *pdata)
{
    fxgmac_init_desc_ops(&pdata->desc_ops);
    fxgmac_init_hw_ops(&pdata->hw_ops);

    /* DPRINTK("register desc_ops and hw ops\n"); */
}

int fxgmac_init(struct fxgmac_pdata *pdata, bool save_private_reg)
{
    struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
    struct net_device *netdev = pdata->netdev;
    unsigned int i,dma_width;
    int ret;

    /* Set all the function pointers */
    fxgmac_init_all_ops(pdata);

    /* Set default configuration data */
    fxgmac_default_config(pdata);

    /* Set irq, base_addr, MAC address, */
    netdev->irq = pdata->dev_irq;
    netdev->base_addr = (unsigned long)pdata->base_mem;
    fxgmac_read_mac_addr(pdata);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0))
    eth_hw_addr_set(netdev, pdata->mac_addr);
#else
    memcpy(netdev->dev_addr, pdata->mac_addr, netdev->addr_len);
#endif

    if (save_private_reg) {
        hw_ops->save_nonstick_reg(pdata);
    }

    // reset here to get hw features correctly
    hw_ops->exit(pdata);

    /* Populate the hardware features */
    fxgmac_get_all_hw_features(pdata);
    fxgmac_print_all_hw_features(pdata);

    /* TODO: Set the PHY mode to XLGMII */

    /* Set the DMA mask */
#ifdef CONFIG_ARM64
    dma_width = FUXI_DMA_BIT_MASK;
#else
    dma_width = pdata->hw_feat.dma_width;
#endif
    ret = dma_set_mask_and_coherent(pdata->dev,
        DMA_BIT_MASK(dma_width));
    if (ret) {
        dev_err(pdata->dev, "dma_set_mask_and_coherent failed\n");
        return ret;
    }

    /* Channel and ring params initializtion
         *  pdata->channel_count;
         *  pdata->tx_ring_count;
         *  pdata->rx_ring_count;
         *  pdata->tx_desc_count;
         *  pdata->rx_desc_count;
         */
    BUILD_BUG_ON_NOT_POWER_OF_2(FXGMAC_TX_DESC_CNT);
    pdata->tx_desc_count = FXGMAC_TX_DESC_CNT;
    if (pdata->tx_desc_count & (pdata->tx_desc_count - 1)) {
        dev_err(pdata->dev, "tx descriptor count (%d) is not valid\n",
            pdata->tx_desc_count);
        ret = -EINVAL;
        return ret;
    }
    BUILD_BUG_ON_NOT_POWER_OF_2(FXGMAC_RX_DESC_CNT);
    pdata->rx_desc_count = FXGMAC_RX_DESC_CNT;
    if (pdata->rx_desc_count & (pdata->rx_desc_count - 1)) {
        dev_err(pdata->dev, "rx descriptor count (%d) is not valid\n",
            pdata->rx_desc_count);
        ret = -EINVAL;
        return ret;
    }

    pdata->tx_ring_count = min_t(unsigned int, num_online_cpus(),
        pdata->hw_feat.tx_ch_cnt);
    pdata->tx_ring_count = min_t(unsigned int, pdata->tx_ring_count,
        pdata->hw_feat.tx_q_cnt);
    pdata->tx_q_count = pdata->tx_ring_count;

    ret = netif_set_real_num_tx_queues(netdev, pdata->tx_q_count);
    DPRINTK("num_online_cpus:%u, tx_ch_cnt:%u, tx_q_cnt:%u, tx_ring_count:%u\n",
        num_online_cpus(), pdata->hw_feat.tx_ch_cnt, pdata->hw_feat.tx_q_cnt,
        pdata->tx_ring_count);
    if (ret) {
        dev_err(pdata->dev, "error setting real tx queue count\n");
        return ret;
    }

    pdata->rx_ring_count = min_t(unsigned int,
        netif_get_num_default_rss_queues(),
        pdata->hw_feat.rx_ch_cnt);
#ifdef FXGMAC_ONE_CHANNEL
    pdata->rx_ring_count = 1;
    pdata->hw_feat.rx_q_cnt = pdata->rx_ring_count;
#else
    pdata->rx_ring_count = min_t(unsigned int, pdata->rx_ring_count,
        pdata->hw_feat.rx_q_cnt);
#endif
    pdata->rx_q_count = pdata->rx_ring_count;
    ret = netif_set_real_num_rx_queues(netdev, pdata->rx_q_count);
    if (ret) {
        dev_err(pdata->dev, "error setting real rx queue count\n");
        return ret;
    }

    pdata->channel_count =
        max_t(unsigned int, pdata->tx_ring_count, pdata->rx_ring_count);

    DPRINTK("default rss queues:%u, rx_ch_cnt:%u, rx_q_cnt:%u, rx_ring_count:%u\n",
        netif_get_num_default_rss_queues(), pdata->hw_feat.rx_ch_cnt, pdata->hw_feat.rx_q_cnt,
        pdata->rx_ring_count);
    DPRINTK("channel_count:%u, netdev tx channel_num=%u\n", pdata->channel_count, netdev->real_num_tx_queues);

    /* Initialize RSS hash key and lookup table */
#if FXGMAC_RSS_HASH_KEY_LINUX
    netdev_rss_key_fill(pdata->rss_key, sizeof(pdata->rss_key));
#else
    //this is for test only. HW does not want to change Hash key, 20210617
    hw_ops->get_rss_hash_key(pdata, (u8 *)pdata->rss_key);
#endif

#if FXGMAC_MSIX_CH0RXDIS_EN
    for (i = 0; i < FXGMAC_RSS_MAX_TABLE_SIZE; i++) {
        pdata->rss_table[i] = FXGMAC_SET_REG_BITS(
            pdata->rss_table[i],
            MAC_RSSDR_DMCH_POS,
            MAC_RSSDR_DMCH_LEN,
            (i % 3) + 1); //eliminate ch0
    }
#else
    for (i = 0; i < FXGMAC_RSS_MAX_TABLE_SIZE; i++) {
        pdata->rss_table[i] = FXGMAC_SET_REG_BITS(
            pdata->rss_table[i],
            MAC_RSSDR_DMCH_POS,
            MAC_RSSDR_DMCH_LEN,
            i % pdata->rx_ring_count); //note, rx_ring_count should be equal to IRQ requsted for MSIx, 4
    }
#endif

    pdata->rss_options = FXGMAC_SET_REG_BITS(
        pdata->rss_options,
        MAC_RSSCR_IP4TE_POS,
        MAC_RSSCR_IP4TE_LEN, 1);
    pdata->rss_options = FXGMAC_SET_REG_BITS(
        pdata->rss_options,
        MAC_RSSCR_TCP4TE_POS,
        MAC_RSSCR_TCP4TE_LEN, 1);
    pdata->rss_options = FXGMAC_SET_REG_BITS(
        pdata->rss_options,
        MAC_RSSCR_UDP4TE_POS,
        MAC_RSSCR_UDP4TE_LEN, 1);

    /* config MTU supported, 20210726 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
    netdev->min_mtu = ETH_MIN_MTU;
    netdev->max_mtu = FXGMAC_JUMBO_PACKET_MTU + (ETH_HLEN + VLAN_HLEN + ETH_FCS_LEN);
#endif
    /*
    netdev->extended->min_mtu = netdev->min_mtu;
    netdev->extended->max_mtu = netdev->max_mtu;
    */

    DPRINTK("rss_options:0x%x\n", pdata->rss_options);

    /* Set device operations */
    netdev->netdev_ops = fxgmac_get_netdev_ops();
    netdev->ethtool_ops = fxgmac_get_ethtool_ops();

    /* Set device features */
    if (pdata->hw_feat.tso) {
        netdev->hw_features = NETIF_F_TSO;
        netdev->hw_features |= NETIF_F_TSO6;
        netdev->hw_features |= NETIF_F_SG;
        netdev->hw_features |= NETIF_F_IP_CSUM;
        netdev->hw_features |= NETIF_F_IPV6_CSUM;
    } else if (pdata->hw_feat.tx_coe) {
        netdev->hw_features = NETIF_F_IP_CSUM;
        netdev->hw_features |= NETIF_F_IPV6_CSUM;
    }

    if (pdata->hw_feat.rx_coe) {
        netdev->hw_features |= NETIF_F_RXCSUM;
        netdev->hw_features |= NETIF_F_GRO;
    }

    if (pdata->hw_feat.rss) {
        netdev->hw_features |= NETIF_F_RXHASH; //it is NETIF_F_RXHASH_BIT finally
    }

    netdev->vlan_features |= netdev->hw_features;

    netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
    pdata->vlan_strip = 1;
    if (pdata->hw_feat.sa_vlan_ins) {
        netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
    }
#if FXGMAC_FILTER_SINGLE_VLAN_ENABLED
    /* only can filter one vlan id */
    pdata->hw_feat.vlhash = 1 ;
#else
    pdata->hw_feat.vlhash = 0 ;
#endif

    if (pdata->hw_feat.vlhash) {
        netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
        pdata->vlan_filter = 1;
    }

    netdev->features |= netdev->hw_features;
    pdata->expansion.netdev_features = netdev->features;

    netdev->priv_flags |= IFF_UNICAST_FLT;

    /* Use default watchdog timeout */
    netdev->watchdog_timeo = msecs_to_jiffies(5000);//refer to sunxi-gmac, 5s
    netdev->gso_max_size = NIC_MAX_TCP_OFFLOAD_SIZE;

    /* Tx coalesce parameters initialization */
    pdata->tx_usecs = FXGMAC_INIT_DMA_TX_USECS;
    pdata->tx_frames = FXGMAC_INIT_DMA_TX_FRAMES;

    /* Rx coalesce parameters initialization */
    pdata->rx_riwt = hw_ops->usec_to_riwt(pdata, FXGMAC_INIT_DMA_RX_USECS);

    pdata->rx_usecs = FXGMAC_INIT_DMA_RX_USECS;
    pdata->rx_frames = FXGMAC_INIT_DMA_RX_FRAMES;

    mutex_init(&pdata->expansion.mutex);

    DPRINTK("fxgmac_init callout,ok.\n");

    return 0;
}

static void fxgmac_init_interrupt_scheme(struct fxgmac_pdata *pdata)
{
#ifdef CONFIG_PCI_MSI
    int vectors, rc, i, req_vectors;
    /* check cpu core number.
     * since we have 4 channels, we must ensure the number of cpu core > 4
     * otherwise, just roll back to legacy
     */
    vectors = num_online_cpus();
    if(vectors >= FXGMAC_MAX_DMA_CHANNELS) {
        // 0-3 for rx, 4 for tx, 5 for phy
        req_vectors = FXGMAC_MSIX_INT_NUMS;
        pdata->expansion.msix_entries = kcalloc(req_vectors,
                                            sizeof(struct msix_entry),
                                            GFP_KERNEL);
        if (!pdata->expansion.msix_entries) {
            dev_err(pdata->dev, "MSIx, kcalloc err for msix entries, \
                rollback to MSI\n");
            goto enable_msi_interrupt;
        }else {
            for (i = 0; i < req_vectors; i++)
                pdata->expansion.msix_entries[i].entry = i;

            rc = pci_enable_msix_range(pdata->pdev,
                                        pdata->expansion.msix_entries,
                                        req_vectors,
                                        req_vectors);
            if (rc < 0) {
                    dev_err(pdata->dev, "enable MSIx failed,%d.\n", rc);
                    req_vectors = 0; //indicate failure
            } else {
                    req_vectors = rc;
            }

            if(req_vectors >= FXGMAC_MAX_DMA_CHANNELS_PLUS_1TX) {
                dev_info(pdata->dev, "enable MSIx ok, cpu=%d,vectors=%d.\n",
                        vectors, req_vectors);
                pdata->expansion.int_flags = FXGMAC_SET_REG_BITS(pdata->expansion.int_flags,
                                                        FXGMAC_FLAG_INTERRUPT_POS,
                                                        FXGMAC_FLAG_INTERRUPT_LEN,
                                                        FXGMAC_FLAG_MSIX_ENABLED);
                pdata->per_channel_irq = 1;
                pdata->expansion.misc_irq = 
                    pdata->expansion.msix_entries[MSI_ID_PHY_OTHER].vector;
                return;
            }else if (req_vectors){
                dev_err(pdata->dev, "enable MSIx with only %d vector, \
                    while we need %d, rollback to MSI.\n",
                                                req_vectors, vectors);
                //roll back to msi
                pci_disable_msix(pdata->pdev);
                kfree(pdata->expansion.msix_entries);
                pdata->expansion.msix_entries = NULL;
                req_vectors = 0;
            }else {
                dev_err(pdata->dev, "enable MSIx failure and clear msix entries.\n");
                //roll back to msi
                kfree(pdata->expansion.msix_entries);
                pdata->expansion.msix_entries = NULL;
                req_vectors = 0;
            }
        }
    }

enable_msi_interrupt:
    rc = pci_enable_msi(pdata->pdev);
    if (rc < 0) {
        pdata->expansion.int_flags = FXGMAC_SET_REG_BITS(pdata->expansion.int_flags,
                                                    FXGMAC_FLAG_INTERRUPT_POS,
                                                    FXGMAC_FLAG_INTERRUPT_LEN,
                                                    FXGMAC_FLAG_LEGACY_ENABLED);
        dev_err(pdata->dev, "dev_err MSI failure, rollback to LEGACY.\n");
    } else {
        pdata->expansion.int_flags = FXGMAC_SET_REG_BITS(pdata->expansion.int_flags,
                                                    FXGMAC_FLAG_INTERRUPT_POS,
                                                    FXGMAC_FLAG_INTERRUPT_LEN,
                                                    FXGMAC_FLAG_MSI_ENABLED);
        pdata->dev_irq = pdata->pdev->irq;
        dev_info(pdata->dev, "enable MSI ok, cpu=%d, irq=%d.\n", vectors, pdata->pdev->irq);
    }
#else
    pdata = pdata;
#endif
}

int fxgmac_drv_probe(struct device *dev, struct fxgmac_resources *res)
{
    struct fxgmac_pdata *pdata;
    struct net_device *netdev;
    int ret;

    netdev = alloc_etherdev_mq(sizeof(struct fxgmac_pdata),
        FXGMAC_MAX_DMA_CHANNELS);

    if (!netdev) {
        dev_err(dev, "alloc_etherdev failed\n");
        return -ENOMEM;
    }

    SET_NETDEV_DEV(netdev, dev);
    dev_set_drvdata(dev, netdev);
    pdata = netdev_priv(netdev);
    pdata->dev = dev;
    pdata->pdev = to_pci_dev(dev);
    pdata->netdev = netdev;

    pdata->dev_irq = res->irq;
    pdata->msg_enable = NETIF_MSG_DRV;
    pdata->expansion.dev_state = FXGMAC_DEV_PROBE;
    /* default to legacy interrupt */
    pdata->expansion.int_flags = FXGMAC_SET_REG_BITS(pdata->expansion.int_flags,
                                                FXGMAC_FLAG_INTERRUPT_POS,
                                                FXGMAC_FLAG_INTERRUPT_LEN,
                                                FXGMAC_FLAG_LEGACY_ENABLED);
    pdata->expansion.misc_irq = pdata->dev_irq;

    fxgmac_init_interrupt_scheme(pdata);

    pdata->mac_regs = res->addr;
    pdata->base_mem = res->addr;
    pdata->mac_regs = pdata->mac_regs + FUXI_MAC_REGS_OFFSET;

    ret = fxgmac_init(pdata, true);
    if (ret) {
        dev_err(dev, "fxgmac init failed\n");
        goto err_free_netdev;
    }

    pdata->hw_ops.read_led_config(pdata);

    netif_carrier_off(netdev);
    ret = register_netdev(netdev);
    if (ret) {
        dev_err(dev, "net device registration failed\n");
        goto err_free_netdev;
    }
    if(netif_msg_drv(pdata))
        DPRINTK("fxgamc_drv_prob callout, netdev num_tx_q=%u\n",
                                    netdev->real_num_tx_queues);

#ifdef HAVE_FXGMAC_DEBUG_FS
    fxgmac_dbg_init(pdata);
    fxgmac_dbg_adapter_init(pdata);
#endif /* HAVE_FXGMAC_DEBUG_FS */

    yt6801_debug_init(pdata);

    return 0;

err_free_netdev:
    free_netdev(netdev);
    DPRINTK("fxgamc_drv_prob callout with err \n");

    return ret;
}

int fxgmac_drv_remove(struct device *dev)
{
    struct net_device *netdev = dev_get_drvdata(dev);
    struct fxgmac_pdata * pdata = netdev_priv(netdev);
    struct fxgmac_hw_ops* hw_ops = &pdata->hw_ops;

    yt6801_debug_deinit();

#ifdef HAVE_FXGMAC_DEBUG_FS
    fxgmac_dbg_adapter_exit(pdata);
#endif /*HAVE_FXGMAC_DEBUG_FS */
    hw_ops->led_under_shutdown(pdata);

    unregister_netdev(netdev);
    free_netdev(netdev);

    return 0;
}

void fxgmac_dump_tx_desc(struct fxgmac_pdata *pdata,
            struct fxgmac_ring *ring,
            unsigned int idx,
            unsigned int count,
            unsigned int flag)
{
    struct fxgmac_desc_data *desc_data;
    struct fxgmac_dma_desc *dma_desc;

    while (count--) {
        desc_data = FXGMAC_GET_DESC_DATA(ring, idx);
        dma_desc = desc_data->dma_desc;

        netdev_dbg(pdata->netdev, "TX: dma_desc=%p, dma_desc_addr=%pad\n",
            desc_data->dma_desc, &desc_data->dma_desc_addr);
        netdev_dbg(pdata->netdev,
                        "TX_NORMAL_DESC[%d %s] = %08x:%08x:%08x:%08x\n", idx,
            (flag == 1) ? "QUEUED FOR TX" : "TX BY DEVICE",
            le32_to_cpu(dma_desc->desc0),
            le32_to_cpu(dma_desc->desc1),
            le32_to_cpu(dma_desc->desc2),
            le32_to_cpu(dma_desc->desc3));

        idx++;
    }
}

void fxgmac_dump_rx_desc(struct fxgmac_pdata *pdata,
            struct fxgmac_ring *ring,
            unsigned int idx)
{
    struct fxgmac_desc_data *desc_data;
    struct fxgmac_dma_desc *dma_desc;

    desc_data = FXGMAC_GET_DESC_DATA(ring, idx);
    dma_desc = desc_data->dma_desc;

    netdev_dbg(pdata->netdev, "RX: dma_desc=%p, dma_desc_addr=%pad\n",
            desc_data->dma_desc, &desc_data->dma_desc_addr);
    netdev_dbg(pdata->netdev,
                        "RX_NORMAL_DESC[%d RX BY DEVICE] = %08x:%08x:%08x:%08x\n",
            idx,
            le32_to_cpu(dma_desc->desc0),
            le32_to_cpu(dma_desc->desc1),
            le32_to_cpu(dma_desc->desc2),
            le32_to_cpu(dma_desc->desc3));
}

void fxgmac_dbg_pkt(struct net_device *netdev,
              struct sk_buff *skb, bool tx_rx)
{
    struct ethhdr *eth = (struct ethhdr *)skb->data;
    unsigned char buffer[128];
    unsigned int i;

    netdev_dbg(netdev, "\n************** SKB dump ****************\n");

    netdev_dbg(netdev, "%s packet of %d bytes\n",
           (tx_rx ? "TX" : "RX"), skb->len);

    netdev_dbg(netdev, "Dst MAC addr: %pM\n", eth->h_dest);
    netdev_dbg(netdev, "Src MAC addr: %pM\n", eth->h_source);
    netdev_dbg(netdev, "Protocol: %#06hx\n", ntohs(eth->h_proto));

    for (i = 0; i < skb->len; i += 32) {
        unsigned int len = min(skb->len - i, 32U);

        hex_dump_to_buffer(&skb->data[i], len, 32, 1,
                buffer, sizeof(buffer), false);
        netdev_dbg(netdev, "  %#06x: %s\n", i, buffer);
    }

    netdev_dbg(netdev, "\n************** SKB dump ****************\n");
}

void fxgmac_print_pkt(struct net_device *netdev,
              struct sk_buff *skb, bool tx_rx)
{
#ifdef FXGMAC_DEBUG
    struct ethhdr *eth = (struct ethhdr *)skb->data;
#endif
    unsigned char buffer[128];
    unsigned int i;

    DPRINTK("\n************** SKB dump ****************\n");
    DPRINTK("%s packet of %d bytes\n",
           (tx_rx ? "TX" : "RX"), skb->len);

#ifdef FXGMAC_DEBUG
    DPRINTK("Dst MAC addr: %pM\n", eth->h_dest);
    DPRINTK("Src MAC addr: %pM\n", eth->h_source);
    DPRINTK("Protocol: %#06hx\n", ntohs(eth->h_proto));
#endif
    for (i = 0; i < skb->len; i += 32) {
        unsigned int len = min(skb->len - i, 32U);

        hex_dump_to_buffer(&skb->data[i], len, 32, 1,
                    buffer, sizeof(buffer), false);
        DPRINTK("  %#06x: %s\n", i, buffer);
    }

    DPRINTK("\n************** SKB dump ****************\n");
}

void fxgmac_get_all_hw_features(struct fxgmac_pdata *pdata)
{
    struct fxgmac_hw_features *hw_feat = &pdata->hw_feat;
    unsigned int mac_hfr0, mac_hfr1, mac_hfr2, mac_hfr3;

    mac_hfr0 = readl(pdata->mac_regs + MAC_HWF0R);
    mac_hfr1 = readl(pdata->mac_regs + MAC_HWF1R);
    mac_hfr2 = readl(pdata->mac_regs + MAC_HWF2R);
    mac_hfr3 = readl(pdata->mac_regs + MAC_HWF3R);

    memset(hw_feat, 0, sizeof(*hw_feat));

    hw_feat->version = readl(pdata->mac_regs + MAC_VR);
    if(netif_msg_drv(pdata)) DPRINTK ("get offset 0x110,ver=%#x\n", readl(pdata->mac_regs + 0x110));

    /* Hardware feature register 0 */
    hw_feat->phyifsel    = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_ACTPHYIFSEL_POS,
                        MAC_HWF0R_ACTPHYIFSEL_LEN);
    hw_feat->vlhash      = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_VLHASH_POS,
                        MAC_HWF0R_VLHASH_LEN);
    hw_feat->sma         = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_SMASEL_POS,
                        MAC_HWF0R_SMASEL_LEN);
    hw_feat->rwk         = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_RWKSEL_POS,
                        MAC_HWF0R_RWKSEL_LEN);
    hw_feat->mgk         = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_MGKSEL_POS,
                        MAC_HWF0R_MGKSEL_LEN);
    hw_feat->mmc         = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_MMCSEL_POS,
                        MAC_HWF0R_MMCSEL_LEN);
    hw_feat->aoe         = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_ARPOFFSEL_POS,
                        MAC_HWF0R_ARPOFFSEL_LEN);
    hw_feat->ts          = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_TSSEL_POS,
                        MAC_HWF0R_TSSEL_LEN);
    hw_feat->eee         = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_EEESEL_POS,
                        MAC_HWF0R_EEESEL_LEN);
    hw_feat->tx_coe      = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_TXCOESEL_POS,
                        MAC_HWF0R_TXCOESEL_LEN);
    hw_feat->rx_coe      = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_RXCOESEL_POS,
                        MAC_HWF0R_RXCOESEL_LEN);
    hw_feat->addn_mac    = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_ADDMACADRSEL_POS,
                        MAC_HWF0R_ADDMACADRSEL_LEN);
    hw_feat->ts_src      = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_TSSTSSEL_POS,
                        MAC_HWF0R_TSSTSSEL_LEN);
    hw_feat->sa_vlan_ins = FXGMAC_GET_REG_BITS(mac_hfr0,
                        MAC_HWF0R_SAVLANINS_POS,
                        MAC_HWF0R_SAVLANINS_LEN);

    /* Hardware feature register 1 */
    hw_feat->rx_fifo_size   = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_RXFIFOSIZE_POS,
                        MAC_HWF1R_RXFIFOSIZE_LEN);
    hw_feat->tx_fifo_size   = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_TXFIFOSIZE_POS,
                        MAC_HWF1R_TXFIFOSIZE_LEN);
    hw_feat->adv_ts_hi     = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_ADVTHWORD_POS,
                        MAC_HWF1R_ADVTHWORD_LEN);
    hw_feat->dma_width     = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_ADDR64_POS,
                        MAC_HWF1R_ADDR64_LEN);
    hw_feat->dcb           = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_DCBEN_POS,
                        MAC_HWF1R_DCBEN_LEN);
    hw_feat->sph           = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_SPHEN_POS,
                        MAC_HWF1R_SPHEN_LEN);
#if (0 /*LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)*/)
    hw_feat->tso           = 0; //bypass tso
#else
    hw_feat->tso           = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_TSOEN_POS,
                        MAC_HWF1R_TSOEN_LEN);
#endif
    hw_feat->dma_debug     = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_DBGMEMA_POS,
                        MAC_HWF1R_DBGMEMA_LEN);
#if (FXGMAC_RSS_FEATURE_ENABLED)
    hw_feat->rss           = 1;
#else
    hw_feat->rss           = 0; /* = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_RSSEN_POS,
                        MAC_HWF1R_RSSEN_LEN);*/
#endif
    hw_feat->tc_cnt	       = 3/*1, yzhang try,0412*/; /* FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_NUMTC_POS,
                        MAC_HWF1R_NUMTC_LEN); */
    hw_feat->avsel         = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_AVSEL_POS,
                        MAC_HWF1R_AVSEL_LEN);
    hw_feat->ravsel	       = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_RAVSEL_POS,
                        MAC_HWF1R_RAVSEL_LEN);
    hw_feat->hash_table_size = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_HASHTBLSZ_POS,
                        MAC_HWF1R_HASHTBLSZ_LEN);
    hw_feat->l3l4_filter_num = FXGMAC_GET_REG_BITS(mac_hfr1,
                        MAC_HWF1R_L3L4FNUM_POS,
                        MAC_HWF1R_L3L4FNUM_LEN);

    /* Hardware feature register 2 */
#if 1 /* hw implement only 1 Q, but from software we see 4 logical Qs. hardcode to 4 Qs. */
    hw_feat->rx_q_cnt     = 3/*FXGMAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_RXQCNT_POS,
                        MAC_HWF2R_RXQCNT_LEN)*/;
#else
    hw_feat->rx_q_cnt     = FXGMAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_RXQCNT_POS,
                        MAC_HWF2R_RXQCNT_LEN);
#endif
    hw_feat->tx_q_cnt     = FXGMAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_TXQCNT_POS,
                        MAC_HWF2R_TXQCNT_LEN);
#if 0 /*yzhang for debug. only 1 rx channl and q*/
    hw_feat->rx_ch_cnt    = 0 /*FXGMAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_RXCHCNT_POS,
                        MAC_HWF2R_RXCHCNT_LEN)*/;
#else
    hw_feat->rx_ch_cnt    = FXGMAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_RXCHCNT_POS,
                        MAC_HWF2R_RXCHCNT_LEN);
#endif
    hw_feat->tx_ch_cnt    = FXGMAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_TXCHCNT_POS,
                        MAC_HWF2R_TXCHCNT_LEN);
    hw_feat->pps_out_num  = FXGMAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_PPSOUTNUM_POS,
                        MAC_HWF2R_PPSOUTNUM_LEN);
    hw_feat->aux_snap_num = FXGMAC_GET_REG_BITS(mac_hfr2,
                        MAC_HWF2R_AUXSNAPNUM_POS,
                        MAC_HWF2R_AUXSNAPNUM_LEN);

    /* Translate the Hash Table size into actual number */
    switch (hw_feat->hash_table_size) {
    case 0:
        break;
    case 1:
        hw_feat->hash_table_size = 64;
        break;
    case 2:
        hw_feat->hash_table_size = 128;
        break;
    case 3:
        hw_feat->hash_table_size = 256;
        break;
    }

    /* Translate the address width setting into actual number */
    switch (hw_feat->dma_width) {
    case 0:
        hw_feat->dma_width = 32;
        break;
    case 1:
        hw_feat->dma_width = 40;
        break;
    case 2:
        hw_feat->dma_width = 48;
        break;
    default:
        hw_feat->dma_width = 32;
    }

    /* The Queue, Channel and TC counts are zero based so increment them
     * to get the actual number
     */
    hw_feat->rx_q_cnt++;
    hw_feat->tx_q_cnt++;
    hw_feat->rx_ch_cnt++;
    hw_feat->tx_ch_cnt++;
    hw_feat->tc_cnt++;

    hw_feat->hwfr3 = mac_hfr3;
    /* DPRINTK("HWFR3: %u\n", mac_hfr3); */
}

void fxgmac_print_all_hw_features(struct fxgmac_pdata *pdata)
{
    char *str = NULL;

    DPRINTK("\n");
    DPRINTK("=====================================================\n");
    DPRINTK("\n");
    DPRINTK("HW support following features,ver=%#x\n",pdata->hw_feat.version);
    DPRINTK("\n");
    /* HW Feature Register0 */
    DPRINTK("VLAN Hash Filter Selected                   : %s\n",
            pdata->hw_feat.vlhash ? "YES" : "NO");
    DPRINTK("SMA (MDIO) Interface                        : %s\n",
            pdata->hw_feat.sma ? "YES" : "NO");
    DPRINTK("PMT Remote Wake-up Packet Enable            : %s\n",
            pdata->hw_feat.rwk ? "YES" : "NO");
    DPRINTK("PMT Magic Packet Enable                     : %s\n",
            pdata->hw_feat.mgk ? "YES" : "NO");
    DPRINTK("RMON/MMC Module Enable                      : %s\n",
            pdata->hw_feat.mmc ? "YES" : "NO");
    DPRINTK("ARP Offload Enabled                         : %s\n",
            pdata->hw_feat.aoe ? "YES" : "NO");
    DPRINTK("IEEE 1588-2008 Timestamp Enabled            : %s\n",
            pdata->hw_feat.ts ? "YES" : "NO");
    DPRINTK("Energy Efficient Ethernet Enabled           : %s\n",
            pdata->hw_feat.eee ? "YES" : "NO");
    DPRINTK("Transmit Checksum Offload Enabled           : %s\n",
            pdata->hw_feat.tx_coe ? "YES" : "NO");
    DPRINTK("Receive Checksum Offload Enabled            : %s\n",
            pdata->hw_feat.rx_coe ? "YES" : "NO");
    DPRINTK("Additional MAC Addresses 1-31 Selected      : %s\n",
            pdata->hw_feat.addn_mac ? "YES" : "NO");

    switch (pdata->hw_feat.ts_src) {
    case 0:
        str = "RESERVED";
        break;
    case 1:
        str = "INTERNAL";
        break;
    case 2:
        str = "EXTERNAL";
        break;
    case 3:
        str = "BOTH";
        break;
    }
    DPRINTK("Timestamp System Time Source                : %s\n", str);

    DPRINTK("Source Address or VLAN Insertion Enable     : %s\n",
            pdata->hw_feat.sa_vlan_ins ? "YES" : "NO");

    /* HW Feature Register1 */
    switch (pdata->hw_feat.rx_fifo_size) {
    case 0:
        str = "128 bytes";
        break;
    case 1:
        str = "256 bytes";
        break;
    case 2:
        str = "512 bytes";
        break;
    case 3:
        str = "1 KBytes";
        break;
    case 4:
        str = "2 KBytes";
        break;
    case 5:
        str = "4 KBytes";
        break;
    case 6:
        str = "8 KBytes";
        break;
    case 7:
        str = "16 KBytes";
        break;
    case 8:
        str = "32 kBytes";
        break;
    case 9:
        str = "64 KBytes";
        break;
    case 10:
        str = "128 KBytes";
        break;
    case 11:
        str = "256 KBytes";
        break;
    default:
        str = "RESERVED";
    }
    DPRINTK("MTL Receive FIFO Size                       : %s\n", str);

    switch (pdata->hw_feat.tx_fifo_size) {
    case 0:
        str = "128 bytes";
        break;
    case 1:
        str = "256 bytes";
        break;
    case 2:
        str = "512 bytes";
        break;
    case 3:
        str = "1 KBytes";
        break;
    case 4:
        str = "2 KBytes";
        break;
    case 5:
        str = "4 KBytes";
        break;
    case 6:
        str = "8 KBytes";
        break;
    case 7:
        str = "16 KBytes";
        break;
    case 8:
        str = "32 kBytes";
        break;
    case 9:
        str = "64 KBytes";
        break;
    case 10:
        str = "128 KBytes";
        break;
    case 11:
        str = "256 KBytes";
        break;
    default:
        str = "RESERVED";
    }
    DPRINTK("MTL Transmit FIFO Size                      : %s\n", str);

    DPRINTK("IEEE 1588 High Word Register Enable         : %s\n",
            pdata->hw_feat.adv_ts_hi ? "YES" : "NO");
    DPRINTK("Address width                               : %u\n",
            pdata->hw_feat.dma_width);
    DPRINTK("DCB Feature Enable                          : %s\n",
            pdata->hw_feat.dcb ? "YES" : "NO");
    DPRINTK("Split Header Feature Enable                 : %s\n",
            pdata->hw_feat.sph ? "YES" : "NO");
    DPRINTK("TCP Segmentation Offload Enable             : %s\n",
            pdata->hw_feat.tso ? "YES" : "NO");
    DPRINTK("DMA Debug Registers Enabled                 : %s\n",
            pdata->hw_feat.dma_debug ? "YES" : "NO");
    DPRINTK("RSS Feature Enabled                   : %s\n",
            pdata->hw_feat.rss ? "YES" : "NO");
    DPRINTK("*TODO*Number of Traffic classes             : %u\n",
            (pdata->hw_feat.tc_cnt));
    DPRINTK("AV Feature Enabled                          : %s\n",
            pdata->hw_feat.avsel ? "YES" : "NO");
    DPRINTK("Rx Side Only AV Feature Enabled             : %s\n",
            (pdata->hw_feat.ravsel ? "YES": "NO"));
    DPRINTK("Hash Table Size                             : %u\n",
            pdata->hw_feat.hash_table_size);
    DPRINTK("Total number of L3 or L4 Filters            : %u\n",
            pdata->hw_feat.l3l4_filter_num);

    /* HW Feature Register2 */
    DPRINTK("Number of MTL Receive Queues                : %u\n",
            pdata->hw_feat.rx_q_cnt);
    DPRINTK("Number of MTL Transmit Queues               : %u\n",
            pdata->hw_feat.tx_q_cnt);
    DPRINTK("Number of DMA Receive Channels              : %u\n",
            pdata->hw_feat.rx_ch_cnt);
    DPRINTK("Number of DMA Transmit Channels             : %u\n",
            pdata->hw_feat.tx_ch_cnt);

    switch (pdata->hw_feat.pps_out_num) {
    case 0:
        str = "No PPS output";
        break;
    case 1:
        str = "1 PPS output";
        break;
    case 2:
        str = "2 PPS output";
        break;
    case 3:
        str = "3 PPS output";
        break;
    case 4:
        str = "4 PPS output";
        break;
    default:
        str = "RESERVED";
    }
    DPRINTK("Number of PPS Outputs                       : %s\n", str);

    switch (pdata->hw_feat.aux_snap_num) {
    case 0:
        str = "No auxiliary input";
        break;
    case 1:
        str = "1 auxiliary input";
        break;
    case 2:
        str = "2 auxiliary input";
        break;
    case 3:
        str = "3 auxiliary input";
        break;
    case 4:
        str = "4 auxiliary input";
        break;
    default:
        str = "RESERVED";
    }
    DPRINTK("Number of Auxiliary Snapshot Inputs         : %s", str);

    DPRINTK("\n");
    DPRINTK("=====================================================\n");
    DPRINTK("\n");
}
