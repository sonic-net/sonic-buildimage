/*! \file ngknetcb_main.c
 *
 * NGKNET Callback module entry.
 */
/*
 * $Copyright: (c) 2022 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

#include <lkm/lkm.h>
#include <ngknet_callback.h>
#include "psample-cb.h"
#include "bcmcnet/bcmcnet_core.h"
/*! \cond */
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("NGKNET Callback Module");
MODULE_LICENSE("GPL");
/*! \endcond */

/*! \cond */
int debug = 0;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug,
"Debug level (default 0)");
/*! \endcond */

/*! Module information */
#define NGKNETCB_MODULE_NAME    "linux_ngknetcb"
#define NGKNETCB_MODULE_MAJOR   122

/* set KNET_CB_DEBUG for debug info */
#define KNET_CB_DEBUG

/* These below need to match incoming enum values */
#define FILTER_TAG_STRIP 0
#define FILTER_TAG_KEEP  1
#define FILTER_TAG_ORIGINAL 2

/* Maintain tag strip statistics */
struct strip_stats_s {
    unsigned long stripped;     /* Number of packets that have been stripped */
    unsigned long checked;
    unsigned long skipped;
};

static struct strip_stats_s strip_stats;
static unsigned int rx_count = 0;

/* Local function prototypes */
static void strip_vlan_tag(struct sk_buff *skb);

/* Remove VLAN tag for select TPIDs */
static void
strip_vlan_tag(struct sk_buff *skb)
{
    uint16_t    vlan_proto;
    uint8_t     *pkt = skb->data;

    vlan_proto = (uint16_t) ((pkt[12] << 8) | pkt[13]);
    if ((vlan_proto == 0x8100) || (vlan_proto == 0x88a8) || (vlan_proto == 0x9100)) {
        /* Move first 12 bytes of packet back by 4 */
        memmove(&skb->data[4], skb->data, 12);
        skb_pull(skb, 4);       /* Remove 4 bytes from start of buffer */
    }
}

static uint32_t
dev_id_get(char* dev_type)
{
    uint32_t dev_id = 0xb880;

    if (0 == strcmp(dev_type, "bcm56880_a0"))
    {
        dev_id = 0xb880;
    }
    else if (0 == strcmp(dev_type, "bcm56780_a0"))
    {
        dev_id = 0xb780;
    }
    else if ((0 == strcmp(dev_type, "bcm56990_a0")) ||
            (0 == strcmp(dev_type, "bcm56990_b0")))
    {
        dev_id = 0xb990;
    }
    else if ((0 == strcmp(dev_type, "bcm56996_a0")) ||
            (0 == strcmp(dev_type, "bcm56996_b0")))
    {
        dev_id = 0xb996;
    }
    else if ((0== strcmp(dev_type, "bcm56995_a0")) ||
            (0== strcmp(dev_type, "bcm56999_a0")))
    {
        dev_id = 0xb999;
    }
    else if ((0== strcmp(dev_type, "bcm56993_b0")) ||
            (0== strcmp(dev_type, "bcm56998_a0")))
    {
        dev_id = 0xb993;
    }
    return dev_id;
}

/*
 * The function get_tag_status() returns the tag status.
 * 0 = Untagged
 * 1 = Single inner-tag
 * 2 = Single outer-tag
 * 3 = Double tagged.
 * -1 = Unsupported type
 */
static int
get_tag_status(char* dev_type, char* dev_var, void *meta)
{
    uint32_t  *valptr;
    uint32_t  fd_index;
    uint32_t  outer_l2_hdr;
    int tag_status = -1;
    uint32_t  match_id_minbit = 1;
    uint32_t  outer_tag_match = 0x10;
    uint32_t dev_id = 0xb880;

    dev_id = dev_id_get(dev_type);
#ifdef KNET_CB_DEBUG
    if (debug & 0x1) {
        printk("dev_type %s dev_var %s\n", dev_type, dev_var);
    }
#endif

    if ((0xb880 == dev_id ) || (0xb780 == dev_id))
    {
        /* Field BCM_PKTIO_RXPMD_MATCH_ID_LO has tag status in RX PMD */
        fd_index = 2;
        valptr = (uint32_t *)meta;
        match_id_minbit = (dev_id == 0xb780) ? 2 : 1;        
        outer_l2_hdr = (valptr[fd_index] >> match_id_minbit & 0xFF);
        outer_tag_match = (((dev_id == 0xb780) &&
                           ((strncmp(dev_var, "DNA_", 4)) == 0)) ? 0x8 : 0x10);
        if (outer_l2_hdr & 0x1) {
#ifdef KNET_CB_DEBUG
            if (debug & 0x1) {
                printk("  L2 Header Present\n");
            }
#endif
            tag_status = 0;
            if (outer_l2_hdr & 0x4) {
#ifdef KNET_CB_DEBUG
                if (debug & 0x1) {
                    printk("  SNAP/LLC\n");
                }
#endif
                tag_status = 0;
            }
            if (outer_l2_hdr & outer_tag_match) {
#ifdef KNET_CB_DEBUG
                if (debug & 0x1) {
                    printk("  Outer Tagged\n");
                }
#endif
                tag_status = 2;    
                if (outer_l2_hdr & 0x20) {
#ifdef KNET_CB_DEBUG
                    if (debug & 0x1) {
                        printk("  Double Tagged\n");
                    }
#endif
                    tag_status = 3;
                }
            }
            else if (outer_l2_hdr & 0x20) {
#ifdef KNET_CB_DEBUG
                if (debug & 0x1) {
                    printk("  Inner Tagged\n");
        }
#endif
                tag_status = 1;
            }
        }
    }
    else if ((dev_id == 0xb990)|| (dev_id == 0xb996) || 
             (dev_id == 0xb999)|| (dev_id == 0xb993))
    {
        fd_index = 9;
        valptr = (uint32_t *)meta;
        /* On TH4, outer_l2_header means INCOMING_TAG_STATUS.
         * TH4 only supports single tagging, so if TAG_STATUS
         * says there's a tag, then we don't want to strip.
         * Otherwise, we do.
         */
        outer_l2_hdr = (valptr[fd_index] >> 13) & 3;

        if (outer_l2_hdr)
        {
            tag_status = 2;
#ifdef KNET_CB_DEBUG
            if (debug & 0x1)
            {
                printk("  Incoming frame tagged\n");
            }
#endif
        }
        else
        {
            tag_status = 0;
#ifdef KNET_CB_DEBUG
            if (debug & 0x1)
            {
                printk("  Incoming frame untagged\n");
            }
#endif
        }
    }
#ifdef KNET_CB_DEBUG
    if (debug & 0x1) {
        printk("%s; Device Type: %s; tag status: %d\n", __func__, dev_type, tag_status);
    }
#endif
    return tag_status;
}

#ifdef KNET_CB_DEBUG
static void
dump_buffer(uint8_t * data, int size)
{
    const char         *const to_hex = "0123456789ABCDEF";
    int                 i;
    char                buffer[128];
    char               *buffer_ptr;
    int                 addr = 0;

    buffer_ptr = buffer;
    for (i = 0; i < size; i++) {
        *buffer_ptr++ = ' ';
        *buffer_ptr++ = to_hex[(data[i] >> 4) & 0xF];
        *buffer_ptr++ = to_hex[data[i] & 0xF];
        if (((i % 16) == 15) || (i == size - 1)) {
            *buffer_ptr = '\0';
            buffer_ptr = buffer;
            printk(KERN_INFO "%04X  %s\n", addr, buffer);
            addr = i + 1;
    }
    }
}

static void
show_pmd(uint8_t *pmd, int len)
{
    if (debug & 0x1) {
        printk("PMD (%d bytes):\n", len);
        dump_buffer(pmd, len);
    }
}

static void
show_mac(uint8_t *pkt)
{
    if (debug & 0x1) {
    printk("DMAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
           pkt[0], pkt[1], pkt[2], pkt[3], pkt[4], pkt[5]);
}
}
#endif

static struct sk_buff *
strip_tag_rx_cb(struct sk_buff *skb)
{
    const struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    int rcpu_mode = 0;
    int tag_status;

    rcpu_mode = (cbd->netif->flags & NGKNET_NETIF_F_RCPU_ENCAP)? 1 : 0;
#ifdef KNET_CB_DEBUG
    if (debug & 0x1)
    {
        printk(KERN_INFO
               "\n%4u --------------------------------------------------------------------------------\n",
               rx_count);
        printk(KERN_INFO
               "RX KNET callback: dev_no=%1d; dev_id=:%6s; type_str=%4s; RCPU: %3s \n",
               cbd->dinfo->dev_no, cbd->dinfo->var_str, cbd->dinfo->type_str, rcpu_mode ? "yes" : "no");
        printk(KERN_INFO "                  pkt_len=%4d; pmd_len=%2d; SKB len: %4d\n",
               cbd->pkt_len, cbd->pmd_len, skb->len);
    if (cbd->filt) {
            printk(KERN_INFO "Filter user data: 0x%08x\n",
                   *(uint32_t *) cbd->filt->user_data);
    }
        printk(KERN_INFO "Before SKB (%d bytes):\n", skb->len);
        dump_buffer(skb->data, skb->len);
        printk("rx_cb for dev %d: id %s, %s\n", cbd->dinfo->dev_no, cbd->dinfo->var_str, cbd->dinfo->type_str);
        printk("netif user data: 0x%08x\n", *(uint32_t *)cbd->netif->user_data);
        show_pmd(cbd->pmd, cbd->pmd_len);
        if (rcpu_mode) {
            const int           RCPU_header_len = PKT_HDR_SIZE + cbd->pmd_len;
            const int           payload_len = skb->len - RCPU_header_len;
            unsigned char      *payload_start = skb->data + payload_len;

            printk(KERN_INFO "Packet Payload (%d bytes):\n", payload_len);
            dump_buffer(payload_start, payload_len);
        } else {
            printk(KERN_INFO "Packet (%d bytes):\n", cbd->pkt_len);
            dump_buffer(skb->data, cbd->pkt_len);
        }
    }
#endif

    if ((!rcpu_mode) && (cbd->filt)) {
        if (FILTER_TAG_ORIGINAL == cbd->filt->user_data[0]) {
            tag_status = get_tag_status(cbd->dinfo->type_str,
                                        cbd->dinfo->var_str,
                                        (void *)cbd->pmd);
            if (tag_status < 0) {
                strip_stats.skipped++;
                goto _strip_tag_rx_cb_exit;
            }
            strip_stats.checked++;
            if (tag_status < 2) {
                strip_stats.stripped++;
                strip_vlan_tag(skb);
            }
        }
        if (FILTER_TAG_STRIP == cbd->filt->user_data[0]) {
            strip_stats.stripped++;
            strip_vlan_tag(skb);
        }
    }
_strip_tag_rx_cb_exit:
#ifdef KNET_CB_DEBUG
    if (debug & 0x1) {
        printk(KERN_INFO "After SKB (%d bytes):\n", skb->len);
        dump_buffer(skb->data, skb->len);
        printk(KERN_INFO
               "\n%4u --------------------------------------------------------------------------------\n",
               rx_count++);
    }
#endif
    return skb;
}

static struct sk_buff *
strip_tag_tx_cb(struct sk_buff *skb)
{
#ifdef KNET_CB_DEBUG
    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);

    if (debug & 0x1) {
    printk("tx_cb for dev %d: %s\n", cbd->dinfo->dev_no, cbd->dinfo->type_str);
    }
    show_pmd(cbd->pmd, cbd->pmd_len);
    show_mac(cbd->pmd + cbd->pmd_len);
#endif
    return skb;
}

static struct sk_buff *
ngknet_rx_cb(struct net_device *dev, struct sk_buff *skb)
{
    skb = strip_tag_rx_cb(skb);
#ifdef PSAMPLE_SUPPORT
    skb = psample_rx_cb(dev, skb); 
#endif
    return skb;
}

static struct sk_buff *
ngknet_tx_cb(struct sk_buff *skb)
{
    skb = strip_tag_tx_cb(skb);
    return skb;
}

static int
ngknet_netif_create_cb(struct net_device *dev)
{
    int retv = 0;
#ifdef PSAMPLE_SUPPORT
    retv = psample_netif_create_cb(dev); 
#endif
    return retv;
}

static int
ngknet_netif_destroy_cb(struct net_device *dev)
{
    int retv = 0;
#ifdef PSAMPLE_SUPPORT
    retv = psample_netif_destroy_cb(dev); 
#endif
    return retv;
}

/*!
 * Generic module functions
 */
static int
ngknetcb_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Broadcom Linux NGKNET Callback: Untagged VLAN Stripper\n");
    seq_printf(m, "    %lu stripped packets\n", strip_stats.stripped);
    seq_printf(m, "    %lu packets checked\n", strip_stats.checked);
    seq_printf(m, "    %lu packets skipped\n", strip_stats.skipped);
    return 0;
}

static int
ngknetcb_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, ngknetcb_show, NULL);
}

static int
ngknetcb_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t
ngknetcb_write(struct file *file, const char *buf,
               size_t count, loff_t *loff)
{
    memset(&strip_stats, 0, sizeof(strip_stats));
    printk("Cleared NGKNET callback stats\n");
    return count;
}

static long
ngknetcb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int
ngknetcb_mmap(struct file *filp, struct vm_area_struct *vma)
{
    return 0;
}

static struct file_operations ngknetcb_fops = {
    PROC_OWNER(THIS_MODULE)
    .open = ngknetcb_open,
    .read = seq_read,
    .write = ngknetcb_write,
    .llseek = seq_lseek,
    .release = ngknetcb_release,
    .unlocked_ioctl = ngknetcb_ioctl,
    .compat_ioctl = ngknetcb_ioctl,
    .mmap = ngknetcb_mmap,
};

/* Added this for PROC_CREATE */
static struct proc_ops ngknetcb_proc_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open = ngknetcb_open,
    .proc_read = seq_read,
    .proc_write = ngknetcb_write,
    .proc_lseek = seq_lseek,
    .proc_release = ngknetcb_release,
    .proc_ioctl = ngknetcb_ioctl,
    .proc_compat_ioctl = ngknetcb_ioctl,
    .proc_mmap = ngknetcb_mmap,
};

static int __init
ngknetcb_init_module(void)
{
    int rv;
    struct proc_dir_entry *entry = NULL; 

    rv = register_chrdev(NGKNETCB_MODULE_MAJOR, NGKNETCB_MODULE_NAME, &ngknetcb_fops);
    if (rv < 0) {
        printk(KERN_WARNING "%s: can't get major %d\n",
               NGKNETCB_MODULE_NAME, NGKNETCB_MODULE_MAJOR);
        return rv;
    }

    PROC_CREATE(entry, NGKNETCB_MODULE_NAME, 0666, NULL, &ngknetcb_proc_ops);
    if (entry == NULL) {
        printk(KERN_ERR "%s: proc_mkdir failed\n",
                NGKNETCB_MODULE_NAME);
    }

    ngknet_rx_cb_register(ngknet_rx_cb);
    ngknet_tx_cb_register(ngknet_tx_cb);

#ifdef PSAMPLE_SUPPORT
    psample_init();
#endif

    ngknet_netif_create_cb_register(ngknet_netif_create_cb);
    ngknet_netif_destroy_cb_register(ngknet_netif_destroy_cb);
    return 0;
}

static void __exit
ngknetcb_exit_module(void)
{
    ngknet_netif_create_cb_unregister(ngknet_netif_create_cb);
    ngknet_netif_destroy_cb_unregister(ngknet_netif_destroy_cb);

#ifdef PSAMPLE_SUPPORT
    psample_cleanup();
#endif

    ngknet_rx_cb_unregister(ngknet_rx_cb);
    ngknet_tx_cb_unregister(ngknet_tx_cb);

    remove_proc_entry(NGKNETCB_MODULE_NAME, NULL);

    unregister_chrdev(NGKNETCB_MODULE_MAJOR, NGKNETCB_MODULE_NAME);
}

module_init(ngknetcb_init_module);
module_exit(ngknetcb_exit_module);
