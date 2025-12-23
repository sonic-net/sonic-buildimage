/*! \file ngknet_procfs.c
 *
 * <description>
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#include <lkm/lkm.h>
#include <lkm/ngknet_ioctl.h>
#include "ngknet_main.h"
#include "ngknet_extra.h"
#include "ngknet_procfs.h"

extern struct ngknet_dev ngknet_devices[];

static struct proc_dir_entry *proc_root = NULL;

static void
proc_data_show(struct seq_file *m, const unsigned char *buf, size_t len)
{
    uint32_t i;

    if (!buf || !len) {
        seq_printf(m, "\n");
        return;
    }

    for (i = 0; i < len; i++) {
        seq_printf(m, "%02x ", buf[i]);
        if ((i + 1) % 32 == 0 || (i + 1) == len) {
            seq_printf(m, "\n");
            if ((i + 1) < len) {
                seq_printf(m, "                ");
            }
        }
    }
}

static int
proc_debug_level_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Debug level: 0x%x\n", ngknet_debug_level_get());

    return 0;
}

static int
proc_debug_level_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_debug_level_show, NULL);
}

static ssize_t
proc_debug_level_write(struct file *file, const char *buf,
                       size_t count, loff_t *loff)
{
    char level_str[11] = {0};
    int debug_level;

    if (copy_from_user(level_str, buf, sizeof(level_str) - 1)) {
        return -EFAULT;
    }
    debug_level = simple_strtol(level_str, NULL, 16);

    ngknet_debug_level_set(debug_level);
    printk("Debug level set to: 0x%x\n", debug_level);

    return count;
}

static int
proc_debug_level_release(struct inode *inode, struct file *file)
{
    return single_release(inode, file);
}

static struct proc_ops proc_debug_level_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_debug_level_open,
    .proc_read =        seq_read,
    .proc_write =       proc_debug_level_write,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_debug_level_release,
};

static int
proc_device_info_show(struct seq_file *m, void *v)
{
    struct ngknet_dev *dev;
    struct bcmcnet_dev_info *info;
    int di, qi, ai = 0;
    int rv;

    for (di = 0; di < NUM_PDMA_DEV_MAX; di++) {
        dev = &ngknet_devices[di];
        if (!(dev->flags & NGKNET_DEV_ACTIVE)) {
            continue;
        }
        ai++;

        rv = bcmcnet_pdma_dev_info_get(&dev->pdma_dev);
        if (SHR_FAILURE(rv)) {
            printk("ngknet: get device%d info failed\n", di);
            break;
        }

        info = &dev->pdma_dev.info;
        seq_printf(m, "dev_no:         %d\n",   di);
        seq_printf(m, "dev_name:       %s\n",   info->dev_name);
        seq_printf(m, "dev_id:         0x%x\n", info->dev_id);
        seq_printf(m, "dev_type:       %d\n",   info->dev_type);
        seq_printf(m, "max_groups:     %d\n",   info->max_groups);
        seq_printf(m, "max_queues:     %d\n",   info->max_queues);
        seq_printf(m, "bm_groups:      0x%x\n", info->bm_groups);
        seq_printf(m, "bm_rx_queues:   0x%x\n", info->bm_rx_queues);
        seq_printf(m, "bm_tx_queues:   0x%x\n", info->bm_tx_queues);
        seq_printf(m, "nb_groups:      %d\n",   info->nb_groups);
        seq_printf(m, "nb_rx_queues:   %d\n",   info->nb_rx_queues);
        seq_printf(m, "nb_tx_queues:   %d\n",   info->nb_tx_queues);
        seq_printf(m, "rx_desc_size:   %d\n",   info->rx_desc_size);
        seq_printf(m, "tx_desc_size:   %d\n",   info->tx_desc_size);
        seq_printf(m, "rx_ph_size:     %d\n",   info->rx_ph_size);
        seq_printf(m, "tx_ph_size:     %d\n",   info->tx_ph_size);
        for (qi = 0; qi < info->nb_rx_queues; qi++) {
            seq_printf(m, "rx_buf_sz[%d]:   %d\n", qi, info->rx_buf_size[qi]);
        }
        for (qi = 0; qi < info->nb_rx_queues; qi++) {
            seq_printf(m, "nb_rx_desc[%d]:  %d\n", qi, info->nb_rx_desc[qi]);
        }
        for (qi = 0; qi < info->nb_rx_queues; qi++) {
            seq_printf(m, "rxq_state[%d]:   0x%x\n", qi, info->rxq_state[qi]);
        }
        for (qi = 0; qi < info->nb_tx_queues; qi++) {
            seq_printf(m, "nb_tx_desc[%d]:  %d\n", qi, info->nb_tx_desc[qi]);
        }
        for (qi = 0; qi < info->nb_tx_queues; qi++) {
            seq_printf(m, "txq_state[%d]:   0x%x\n", qi, info->txq_state[qi]);
        }
    }

    if (!ai) {
        seq_printf(m, "%s\n", "No active device");
    } else {
        seq_printf(m, "------------------------\n");
        seq_printf(m, "Total %d devices\n", ai);
    }

    return 0;
}

static int
proc_device_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_device_info_show, NULL);
}

static int
proc_device_info_release(struct inode *inode, struct file *file)
{
    return single_release(inode, file);
}

static struct proc_ops proc_device_info_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_device_info_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_device_info_release,
};

static int
proc_filter_info_show(struct seq_file *m, void *v)
{
    struct ngknet_dev *dev;
    struct filt_ctrl *fc;
    ngknet_filter_t filt;
    uint64_t hits;
    unsigned long flags;
    int di, id, dn = 0, fn = 0;

    for (di = 0; di < NUM_PDMA_DEV_MAX; di++) {
        dev = &ngknet_devices[di];
        if (!(dev->flags & NGKNET_DEV_ACTIVE)) {
            continue;
        }
        dn++;

        for (id = 1; id <= NUM_FILTER_MAX; id++) {
            spin_lock_irqsave(&dev->lock, flags);
            fc = (struct filt_ctrl *)dev->fc[id];
            if (!fc) {
                spin_unlock_irqrestore(&dev->lock, flags);
                continue;
            }
            memcpy(&filt, &fc->filt, sizeof(filt));
            hits = fc->hits;
            fn++;
            spin_unlock_irqrestore(&dev->lock, flags);

            seq_printf(m, "\n");
            seq_printf(m, "dev_no:         %d\n",   di);
            seq_printf(m, "id:             %d\n",   filt.id);
            seq_printf(m, "next:           %d\n",   filt.next);
            seq_printf(m, "type:           %d\n",   filt.type);
            seq_printf(m, "flags:          0x%x\n", filt.flags);
            seq_printf(m, "prio:           %d\n",   filt.priority);
            seq_printf(m, "chan:           %d\n",   filt.chan);
            seq_printf(m, "desc:           %s\n",   filt.desc);
            seq_printf(m, "dest_type:      %d\n",   filt.dest_type);
            seq_printf(m, "dest_id:        %d\n",   filt.dest_id);
            seq_printf(m, "dest_proto:     0x%x\n", filt.dest_proto);
            seq_printf(m, "mirror_type:    %d\n",   filt.mirror_type);
            seq_printf(m, "mirror_id:      %d\n",   filt.mirror_id);
            seq_printf(m, "mirror_proto:   0x%x\n", filt.mirror_proto);
            seq_printf(m, "oob_offset:     %d\n",   filt.oob_data_offset);
            seq_printf(m, "oob_size:       %d\n",   filt.oob_data_size);
            seq_printf(m, "pkt_offset:     %d\n",   filt.pkt_data_offset);
            seq_printf(m, "pkt_size:       %d\n",   filt.pkt_data_size);
            seq_printf(m, "filt_data:      ");
            proc_data_show(m, filt.data.b, filt.oob_data_size + filt.pkt_data_size);
            seq_printf(m, "filt_mask:      ");
            proc_data_show(m, filt.mask.b, filt.oob_data_size + filt.pkt_data_size);
            seq_printf(m, "user_data:      ");
            proc_data_show(m, filt.user_data, NGKNET_FILTER_USER_DATA);
            seq_printf(m, "hits:           %llu\n", hits);
        }
    }

    if (!dn) {
        seq_printf(m, "%s\n", "No active device");
    } else {
        seq_printf(m, "--------------------------------\n");
        seq_printf(m, "Total %d devices, %d filters\n", dn, fn);
    }

    return 0;
}

static int
proc_filter_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_filter_info_show, NULL);
}

static int
proc_filter_info_release(struct inode *inode, struct file *file)
{
    return single_release(inode, file);
}

static struct proc_ops proc_filter_info_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_filter_info_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_filter_info_release,
};

static int
proc_netif_info_show(struct seq_file *m, void *v)
{
    struct ngknet_dev *dev;
    struct net_device *ndev;
    struct ngknet_private *priv;
    ngknet_netif_t netif;
    struct net_device_stats stats;
    unsigned long flags;
    int di, ma, id, dn = 0, nn = 0;

    for (di = 0; di < NUM_PDMA_DEV_MAX; di++) {
        dev = &ngknet_devices[di];
        if (!(dev->flags & NGKNET_DEV_ACTIVE)) {
            continue;
        }
        dn++;

        for (id = 0; id <= NUM_VDEV_MAX; id++) {
            spin_lock_irqsave(&dev->lock, flags);
            ndev = id == 0 ? dev->net_dev : dev->vdev[id];
            if (!ndev) {
                spin_unlock_irqrestore(&dev->lock, flags);
                continue;
            }
            priv = netdev_priv(ndev);
            memcpy(&netif, &priv->netif, sizeof(netif));
            memcpy(&stats, &priv->stats, sizeof(stats));
            nn++;
            spin_unlock_irqrestore(&dev->lock, flags);

            seq_printf(m, "\n");
            seq_printf(m, "dev_no:         %d\n",   di);
            seq_printf(m, "id:             %d\n",   netif.id);
            seq_printf(m, "next:           %d\n",   netif.next);
            seq_printf(m, "type:           %d\n",   netif.type);
            seq_printf(m, "flags:          0x%x\n", netif.flags);
            seq_printf(m, "vlan:           %d\n",   netif.vlan);
            seq_printf(m, "mac:            ");
            for (ma = 0; ma < 6; ma++) {
                if (ma == 5) {
                    seq_printf(m, "%02x\n", netif.macaddr[ma]);
                } else {
                    seq_printf(m, "%02x:", netif.macaddr[ma]);
                }
            }
            seq_printf(m, "mtu:            %d\n",   netif.mtu);
            seq_printf(m, "chan:           %d\n",   netif.chan);
            seq_printf(m, "name:           %s\n",   netif.name);
            seq_printf(m, "meta_off:       %d\n",   netif.meta_off);
            seq_printf(m, "meta_len:       %d\n",   netif.meta_len);
            seq_printf(m, "meta_data:      ");
            proc_data_show(m, netif.meta_data, netif.meta_len);
            seq_printf(m, "user_data:      ");
            proc_data_show(m, netif.user_data, NGKNET_NETIF_USER_DATA);
            seq_printf(m, "rx_packets:     %lu\n",  stats.rx_packets);
            seq_printf(m, "rx_bytes:       %lu\n",  stats.rx_bytes);
            seq_printf(m, "rx_dropped:     %lu\n",  stats.rx_dropped);
            seq_printf(m, "rx_errors:      %lu\n",  stats.rx_errors);
            seq_printf(m, "tx_packets:     %lu\n",  stats.tx_packets);
            seq_printf(m, "tx_bytes:       %lu\n",  stats.tx_bytes);
            seq_printf(m, "tx_dropped:     %lu\n",  stats.tx_dropped);
            seq_printf(m, "tx_errors:      %lu\n",  stats.tx_errors);
        }
    }

    if (!dn) {
        seq_printf(m, "%s\n", "No active device");
    } else {
        seq_printf(m, "--------------------------------\n");
        seq_printf(m, "Total %d devices, %d netifs\n", dn, nn);
    }

    return 0;
}

static int
proc_netif_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_netif_info_show, NULL);
}

static int
proc_netif_info_release(struct inode *inode, struct file *file)
{
    return single_release(inode, file);
}

static struct proc_ops proc_netif_info_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_netif_info_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_netif_info_release,
};

static int
proc_pkt_stats_show(struct seq_file *m, void *v)
{
    struct ngknet_dev *dev;
    struct bcmcnet_dev_stats *stats;
    int di, qi, ai = 0;
    int rv;

    for (di = 0; di < NUM_PDMA_DEV_MAX; di++) {
        dev = &ngknet_devices[di];
        if (!(dev->flags & NGKNET_DEV_ACTIVE)) {
            continue;
        }
        ai++;

        rv = bcmcnet_pdma_dev_stats_get(&dev->pdma_dev);
        if (SHR_FAILURE(rv)) {
            printk("ngknet: get device%d stats failed\n", di);
            break;
        }

        stats = &dev->pdma_dev.stats;
        seq_printf(m, "rx_packets:     %llu\n", (unsigned long long)stats->rxqs.packets);
        seq_printf(m, "rx_bytes:       %llu\n", (unsigned long long)stats->rxqs.bytes);
        for (qi = 0; qi < dev->pdma_dev.ctrl.nb_rxq; qi++) {
            seq_printf(m, "rx_packets[%d]:  %llu\n", qi, (unsigned long long)stats->rxq[qi].packets);
            seq_printf(m, "rx_bytes[%d]:    %llu\n", qi, (unsigned long long)stats->rxq[qi].bytes);
        }
        seq_printf(m, "rx_dropped:     %llu\n", (unsigned long long)stats->rxqs.dropped);
        seq_printf(m, "rx_errors:      %llu\n", (unsigned long long)stats->rxqs.errors);
        seq_printf(m, "rx_head_errors: %llu\n", (unsigned long long)stats->rxqs.head_errors);
        seq_printf(m, "rx_data_errors: %llu\n", (unsigned long long)stats->rxqs.data_errors);
        seq_printf(m, "rx_cell_errors: %llu\n", (unsigned long long)stats->rxqs.cell_errors);
        seq_printf(m, "rx_nomems:      %llu\n", (unsigned long long)stats->rxqs.nomems);
        seq_printf(m, "tx_packets:     %llu\n", (unsigned long long)stats->txqs.packets);
        seq_printf(m, "tx_bytes:       %llu\n", (unsigned long long)stats->txqs.bytes);
        for (qi = 0; qi < dev->pdma_dev.ctrl.nb_txq; qi++) {
            seq_printf(m, "tx_packets[%d]:  %llu\n", qi, (unsigned long long)stats->txq[qi].packets);
            seq_printf(m, "tx_bytes[%d]:    %llu\n", qi, (unsigned long long)stats->txq[qi].bytes);
        }
        seq_printf(m, "tx_dropped:     %llu\n", (unsigned long long)stats->txqs.dropped);
        seq_printf(m, "tx_errors:      %llu\n", (unsigned long long)stats->txqs.errors);
        seq_printf(m, "tx_xoffs:       %llu\n", (unsigned long long)stats->txqs.xoffs);
        seq_printf(m, "interrupts:     %llu\n", (unsigned long long)stats->intrs);
    }

    if (!ai) {
        seq_printf(m, "%s\n", "No active device");
    } else {
        seq_printf(m, "------------------------\n");
        seq_printf(m, "Total %d devices\n", ai);
    }

    return 0;
}

static int
proc_pkt_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_pkt_stats_show, NULL);
}

static int
proc_pkt_stats_release(struct inode *inode, struct file *file)
{
    return single_release(inode, file);
}

static struct proc_ops proc_pkt_stats_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_pkt_stats_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_pkt_stats_release,
};

static int
proc_rate_limit_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Rx rate limit: %d pps\n", ngknet_rx_rate_limit_get());

    return 0;
}

static int
proc_rate_limit_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_rate_limit_show, NULL);
}

static ssize_t
proc_rate_limit_write(struct file *file, const char *buf,
                      size_t count, loff_t *loff)
{
    char limit_str[9] = {0};
    int rate_limit;

    if (copy_from_user(limit_str, buf, sizeof(limit_str) - 1)) {
        return -EFAULT;
    }
    rate_limit = simple_strtol(limit_str, NULL, 10);

    ngknet_rx_rate_limit_set(rate_limit);
    printk("Rx rate limit set to: %d pps\n", rate_limit);

    return count;
}

static int
proc_rate_limit_release(struct inode *inode, struct file *file)
{
    return single_release(inode, file);
}

static struct proc_ops proc_rate_limit_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_rate_limit_open,
    .proc_read =        seq_read,
    .proc_write =       proc_rate_limit_write,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_rate_limit_release,
};

static int
proc_reg_status_show(struct seq_file *m, void *v)
{
    struct ngknet_dev *dev;
    int di, qi, ai = 0;

    for (di = 0; di < NUM_PDMA_DEV_MAX; di++) {
        dev = &ngknet_devices[di];
        if (!(dev->flags & NGKNET_DEV_ACTIVE)) {
            continue;
        }
        ai++;
        for (qi = 0; qi < dev->pdma_dev.ctrl.nb_rxq; qi++) {
            bcmcnet_pdma_rx_queue_reg_dump(&dev->pdma_dev, qi);
        }
        for (qi = 0; qi < dev->pdma_dev.ctrl.nb_txq; qi++) {
            bcmcnet_pdma_tx_queue_reg_dump(&dev->pdma_dev, qi);
        }
    }

    if (!ai) {
        seq_printf(m, "%s\n", "No active device");
    } else {
        seq_printf(m, "------------------------\n");
        seq_printf(m, "Total %d devices\n", ai);
    }

    return 0;
}

static int
proc_reg_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_reg_status_show, NULL);
}

static int
proc_reg_status_release(struct inode *inode, struct file *file)
{
    return single_release(inode, file);
}

static struct proc_ops proc_reg_status_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_reg_status_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_reg_status_release,
};

static int
proc_ring_status_show(struct seq_file *m, void *v)
{
    struct ngknet_dev *dev;
    int di, qi, ai = 0;

    for (di = 0; di < NUM_PDMA_DEV_MAX; di++) {
        dev = &ngknet_devices[di];
        if (!(dev->flags & NGKNET_DEV_ACTIVE)) {
            continue;
        }
        ai++;
        seq_printf(m, "%s-%d, ", "Unit", di);
        for (qi = 0; qi < dev->pdma_dev.ctrl.nb_rxq; qi++) {
            bcmcnet_pdma_rx_ring_dump(&dev->pdma_dev, qi);
        }
        seq_printf(m, "%s%d, ", "Rx queues: ", qi);
        for (qi = 0; qi < dev->pdma_dev.ctrl.nb_txq; qi++) {
            bcmcnet_pdma_tx_ring_dump(&dev->pdma_dev, qi);
        }
        seq_printf(m, "%s%d. ", "Tx queues: ", qi);
        seq_printf(m, "\n");
    }

    if (!ai) {
        seq_printf(m, "%s\n", "No active device");
    } else {
        seq_printf(m, "------------------------\n");
        seq_printf(m, "Total %d devices\n", ai);
    }

    return 0;
}

static int
proc_ring_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_ring_status_show, NULL);
}

static int
proc_ring_status_release(struct inode *inode, struct file *file)
{
    return single_release(inode, file);
}

static struct proc_ops proc_ring_status_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_ring_status_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_ring_status_release,
};

int
ngknet_procfs_init(void)
{
    struct proc_dir_entry *entry = NULL;

    proc_root = proc_mkdir(NGKNET_MODULE_NAME, NULL);
    if (proc_root == NULL) {
        printk(KERN_ERR "ngknet: proc_mkdir failed\n");
        return -1;
    }

    PROC_CREATE(entry, "debug_level", 0666, proc_root, &proc_debug_level_fops);
    if (entry == NULL) {
        printk(KERN_ERR "ngknet: proc_create failed\n");
        return -1;
    }

    PROC_CREATE(entry, "device_info", 0444, proc_root, &proc_device_info_fops);
    if (entry == NULL) {
        printk(KERN_ERR "ngknet: proc_create failed\n");
        return -1;
    }

    PROC_CREATE(entry, "filter_info", 0444, proc_root, &proc_filter_info_fops);
    if (entry == NULL) {
        printk(KERN_ERR "ngknet: proc_create failed\n");
        return -1;
    }

    PROC_CREATE(entry, "netif_info", 0444, proc_root, &proc_netif_info_fops);
    if (entry == NULL) {
        printk(KERN_ERR "ngknet: proc_create failed\n");
        return -1;
    }

    PROC_CREATE(entry, "pkt_stats", 0444, proc_root, &proc_pkt_stats_fops);
    if (entry == NULL) {
        printk(KERN_ERR "ngknet: proc_create failed\n");
        return -1;
    }

    PROC_CREATE(entry, "rate_limit", 0666, proc_root, &proc_rate_limit_fops);
    if (entry == NULL) {
        printk(KERN_ERR "ngknet: proc_create failed\n");
        return -1;
    }

    PROC_CREATE(entry, "reg_status", 0444, proc_root, &proc_reg_status_fops);
    if (entry == NULL) {
        printk(KERN_ERR "ngknet: proc_create failed\n");
        return -1;
    }

    PROC_CREATE(entry, "ring_status", 0444, proc_root, &proc_ring_status_fops);
    if (entry == NULL) {
        printk(KERN_ERR "ngknet: proc_create failed\n");
        return -1;
    }

    return 0;
}

int
ngknet_procfs_cleanup(void)
{
    remove_proc_entry("debug_level", proc_root);
    remove_proc_entry("device_info", proc_root);
    remove_proc_entry("filter_info", proc_root);
    remove_proc_entry("netif_info", proc_root);
    remove_proc_entry("pkt_stats", proc_root);
    remove_proc_entry("rate_limit", proc_root);
    remove_proc_entry("reg_status", proc_root);
    remove_proc_entry("ring_status", proc_root);

    remove_proc_entry(NGKNET_MODULE_NAME, NULL);

    return 0;
}

