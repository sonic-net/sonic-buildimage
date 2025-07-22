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


#include "fuxi-gmac.h"
#include "fuxi-gmac-reg.h"
#ifdef HAVE_FXGMAC_DEBUG_FS
#include <linux/debugfs.h>
#endif
#include <linux/module.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fcntl.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h> 
#include <linux/etherdevice.h>
#include <linux/netdevice.h> 
#include <linux/etherdevice.h>
#include <linux/ip.h> 
#include <linux/udp.h>

#define TEST_MAC_HEAD               14
#define TEST_TCP_HEAD_LEN_OFFSET    12
#define TEST_TCP_OFFLOAD_LEN_OFFSET 48
#define TEST_TCP_FIX_HEAD_LEN       24
#define TEST_TCP_MSS_OFFSET         56

#define DF_MAX_NIC_NUM      16

#ifdef HAVE_FXGMAC_DEBUG_FS

/**
 * fxgmac_dbg_netdev_ops_read - read for netdev_ops datum
 * @filp: the opened file
 * @buffer: where to write the data for the user to read
 * @count: the size of the user's buffer
 * @ppos: file position offset
 **/
static ssize_t fxgmac_dbg_netdev_ops_read(struct file *filp,
					 char __user *buffer,
					 size_t count, loff_t *ppos)
{
    struct fxgmac_pdata *pdata = filp->private_data;
    char *buf;
    int len;

    /* don't allow partial reads */
    if (*ppos != 0)
    	return 0;

    buf = kasprintf(GFP_KERNEL, "%s: %s\n",
    		pdata->netdev->name,
    		pdata->expansion.fxgmac_dbg_netdev_ops_buf);
    if (!buf)
    	return -ENOMEM;

    if (count < strlen(buf)) {
    	kfree(buf);
    	return -ENOSPC;
    }

    len = simple_read_from_buffer(buffer, count, ppos, buf, strlen(buf));

    kfree(buf);
    return len;
}

/**
 * fxgmac_dbg_netdev_ops_write - write into netdev_ops datum
 * @filp: the opened file
 * @buffer: where to find the user's data
 * @count: the length of the user's data
 * @ppos: file position offset
 **/
static ssize_t fxgmac_dbg_netdev_ops_write(struct file *filp,
					  const char __user *buffer,
					  size_t count, loff_t *ppos)
{
    struct fxgmac_pdata *pdata = filp->private_data;
    int len;

    /* don't allow partial writes */
    if (*ppos != 0)
    	return 0;
    if (count >= sizeof(pdata->expansion.fxgmac_dbg_netdev_ops_buf))
    	return -ENOSPC;

    len = simple_write_to_buffer(pdata->expansion.fxgmac_dbg_netdev_ops_buf,
    			     sizeof(pdata->expansion.fxgmac_dbg_netdev_ops_buf)-1,
    			     ppos,
    			     buffer,
    			     count);
    if (len < 0)
    	return len;

    pdata->expansion.fxgmac_dbg_netdev_ops_buf[len] = '\0';

    if (strncmp(pdata->expansion.fxgmac_dbg_netdev_ops_buf, "tx_timeout", 10) == 0) {
    	DPRINTK("tx_timeout called\n");
    } else {
    	FXGMAC_PR("Unknown command: %s\n", pdata->expansion.fxgmac_dbg_netdev_ops_buf);
    	FXGMAC_PR("Available commands:\n");
    	FXGMAC_PR("    tx_timeout\n");
    }
    return count;
}
#endif

static void fxgmac_dbg_tx_pkt(struct fxgmac_pdata *pdata, u8 *pcmd_data)
{
    unsigned int pktLen = 0;
    struct sk_buff *skb;
    pfxgmac_test_packet pPkt;
    u8 * pTx_data = NULL;
    u8 * pSkb_data = NULL;
    u32 offload_len = 0;
    u8 ipHeadLen, tcpHeadLen, headTotalLen;
    static u32 lastGsoSize = 806;//initial default value
    //int i = 0;

    /* get fxgmac_test_packet */
    pPkt = (pfxgmac_test_packet)(pcmd_data + sizeof(struct ext_ioctl_data));
    pktLen = pPkt->length;
    
    /* get pkt data */
    pTx_data = (u8 *)pPkt + sizeof(fxgmac_test_packet);

#if 0 //for debug
    DPRINTK("Send packet len is %d, data:\n", pktLen);
#if 1
    for(i = 0; i < 70 ; i++)
    {
        DPRINTK("%2x ", pTx_data[i]);
    }
#endif
#endif
 
    /* alloc sk_buff */
    skb = alloc_skb(pktLen, GFP_ATOMIC);
    if (!skb){
        DPRINTK("alloc skb fail\n");
        return;
    }

    /* copy data to skb */
    pSkb_data = skb_put(skb, pktLen);
    memset(pSkb_data, 0, pktLen);
    memcpy(pSkb_data, pTx_data, pktLen);
   
    /* set skb parameters */
    skb->dev = pdata->netdev;
    skb->pkt_type = PACKET_OUTGOING;
    skb->protocol = ntohs(ETH_P_IP);
    skb->no_fcs = 1;
    skb->ip_summed = CHECKSUM_PARTIAL; 
    if(skb->len > 1514){
        /* TSO packet */
        /* set tso test flag */
        pdata->expansion.fxgmac_test_tso_flag = true;

        /* get protocol head length */
        ipHeadLen = (pSkb_data[TEST_MAC_HEAD] & 0xF) * 4;
        tcpHeadLen = (pSkb_data[TEST_MAC_HEAD + ipHeadLen + TEST_TCP_HEAD_LEN_OFFSET] >> 4 & 0xF) * 4;
        headTotalLen = TEST_MAC_HEAD + ipHeadLen + tcpHeadLen;
        offload_len = (pSkb_data[TEST_TCP_OFFLOAD_LEN_OFFSET] << 8 | 
                        pSkb_data[TEST_TCP_OFFLOAD_LEN_OFFSET + 1]) & 0xFFFF;
        /* set tso skb parameters */
        //skb->ip_summed = CHECKSUM_PARTIAL; 
        skb->transport_header = ipHeadLen + TEST_MAC_HEAD;
        skb->network_header = TEST_MAC_HEAD;
        skb->inner_network_header = TEST_MAC_HEAD;
        skb->mac_len = TEST_MAC_HEAD;

        /* set skb_shinfo parameters */
        if(tcpHeadLen > TEST_TCP_FIX_HEAD_LEN){
            skb_shinfo(skb)->gso_size = (pSkb_data[TEST_TCP_MSS_OFFSET] << 8 | 
                pSkb_data[TEST_TCP_MSS_OFFSET + 1]) & 0xFFFF;
        }else{
            skb_shinfo(skb)->gso_size = 0;
        }
        if(skb_shinfo(skb)->gso_size != 0){
            lastGsoSize = skb_shinfo(skb)->gso_size;
        }else{
            skb_shinfo(skb)->gso_size = lastGsoSize;
        }
        //DPRINTK("offload_len is %d, skb_shinfo(skb)->gso_size is %d", offload_len, skb_shinfo(skb)->gso_size);
        /* get segment size */
        if(offload_len % skb_shinfo(skb)->gso_size == 0){
            skb_shinfo(skb)->gso_segs = offload_len / skb_shinfo(skb)->gso_size;
            pdata->expansion.fxgmac_test_last_tso_len = skb_shinfo(skb)->gso_size + headTotalLen;
        }else{
            skb_shinfo(skb)->gso_segs = offload_len / skb_shinfo(skb)->gso_size + 1;
            pdata->expansion.fxgmac_test_last_tso_len = offload_len % skb_shinfo(skb)->gso_size + headTotalLen;
        }
        pdata->expansion.fxgmac_test_tso_seg_num = skb_shinfo(skb)->gso_segs;

        skb_shinfo(skb)->gso_type = SKB_GSO_TCPV4 ;
        skb_shinfo(skb)->frag_list = NULL;
        skb->csum_start = skb_headroom(skb) + TEST_MAC_HEAD + ipHeadLen;
        skb->csum_offset = skb->len - TEST_MAC_HEAD - ipHeadLen;

        pdata->expansion.fxgmac_test_packet_len = skb_shinfo(skb)->gso_size + headTotalLen;
    }else{
        /* set non-TSO packet parameters */
        pdata->expansion.fxgmac_test_packet_len = skb->len;
    }

    /* send data */
    if(dev_queue_xmit(skb) != NET_XMIT_SUCCESS){
        DPRINTK("xmit data fail \n");
    }
}

static void fxgmac_dbg_rx_pkt(struct fxgmac_pdata *pdata, u8 *pcmd_data)
{
    unsigned int totalLen = 0;
    struct sk_buff *rx_skb;
    struct ext_ioctl_data *pcmd;
    fxgmac_test_packet pkt;
    void* addr = 0;
    u8 *rx_data = (u8*)kzalloc(FXGMAC_MAX_DBG_RX_DATA, GFP_KERNEL);
    if (!rx_data)
        return;
    //int i;

    /* initial dest data region */
    pcmd = (struct ext_ioctl_data *)pcmd_data;
    addr = pcmd->cmd_buf.buf;
    while(pdata->expansion.fxgmac_test_skb_arr_in_index != pdata->expansion.fxgmac_test_skb_arr_out_index){
        /* get received skb data */
        rx_skb = pdata->expansion.fxgmac_test_skb_array[pdata->expansion.fxgmac_test_skb_arr_out_index];

        if(rx_skb->len + sizeof(fxgmac_test_packet) + totalLen < 64000){
            pkt.length = rx_skb->len;
            pkt.type = 0x80;
            pkt.buf[0].offset = totalLen + sizeof(fxgmac_test_packet);
            pkt.buf[0].length = rx_skb->len;

            /* get data from skb */
            //DPRINTK("FXG:rx_skb->len=%d", rx_skb->len);
            memcpy(rx_data, rx_skb->data, rx_skb->len);

	        /* update next pointer */
	        if((pdata->expansion.fxgmac_test_skb_arr_out_index + 1) % FXGMAC_MAX_DBG_TEST_PKT == pdata->expansion.fxgmac_test_skb_arr_in_index)
	        {
		        pkt.next = NULL;
	        }
	        else
	        {
            	pkt.next = (pfxgmac_test_packet)(addr + totalLen + sizeof(fxgmac_test_packet) + pkt.length);
	        }

            /* copy data to user space */
            if(copy_to_user((void *)(addr + totalLen), (void*)(&pkt), sizeof(fxgmac_test_packet)))
            {
                DPRINTK("cppy pkt data to user fail...");
            }
            //FXGMAC_PR("FXG:rx_skb->len=%d", rx_skb->len);
            if(copy_to_user((void *)(addr + totalLen + sizeof(fxgmac_test_packet)), (void*)rx_data, rx_skb->len))
            {
                DPRINTK("cppy data to user fail...");
            }

            /* update total length */
            totalLen += (sizeof(fxgmac_test_packet) + rx_skb->len);

            /* free skb */
            kfree_skb(rx_skb);
            pdata->expansion.fxgmac_test_skb_array[pdata->expansion.fxgmac_test_skb_arr_out_index] = NULL;

            /* update gCurSkbOutIndex */
            pdata->expansion.fxgmac_test_skb_arr_out_index = (pdata->expansion.fxgmac_test_skb_arr_out_index + 1) % FXGMAC_MAX_DBG_TEST_PKT;
       }else{
            DPRINTK("receive data more receive buffer... \n");
            break;
       }
    }

    if (rx_data)
        kfree(rx_data);
#if 0
    pPkt = (pfxgmac_test_packet)buf;
    DPRINTK("FXG: pPkt->Length is %d", pPkt->length);
    DPRINTK("FXG: pPkt->Length is %d", pPkt->length);
    DPRINTK("pPkt: %p, buf is %lx",pPkt, pcmd->cmd_buf.buf);
    for(i = 0; i < 30; i++){
        DPRINTK("%x",*(((u8*)pPkt + sizeof(fxgmac_test_packet)) + i));
    }
#endif
}

// Based on the current application scenario,we only use CMD_DATA for data.
// if you use other struct, you should recalculate in_total_size 
long fxgmac_dbg_netdev_ops_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    bool ret = true;
    int regval = 0;
    struct fxgmac_pdata *pdata = file->private_data;
    struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
    FXGMAC_PDATA_OF_PLATFORM *ex = &pdata->expansion;
    CMD_DATA ex_data;
    struct ext_ioctl_data pcmd;
    u8* data = NULL;
    u8* buf = NULL;
    int in_total_size, in_data_size, out_total_size;
    int ioctl_cmd_size = sizeof(struct ext_ioctl_data);
    u8 mac[ETH_ALEN] = {0};
    struct sk_buff *tmpskb;

    if (!arg) {
        DPRINTK("[%s] command arg is %lx !\n", __func__, arg);
        goto err; 
    }

    /* check device type */
    if (_IOC_TYPE(cmd) != IOC_MAGIC) {
        DPRINTK("[%s] command type [%c] error!\n", __func__, _IOC_TYPE(cmd));
        goto err; 
    }

    /* check command number*/
    if (_IOC_NR(cmd) > IOC_MAXNR) { 
        DPRINTK("[%s] command numer [%d] exceeded!\n", __func__, _IOC_NR(cmd));
        goto err; 
    }

    //buf = (u8*)kzalloc(FXGMAC_MAX_DBG_BUF_LEN, GFP_KERNEL);
    if(copy_from_user(&pcmd, (void*)arg, ioctl_cmd_size)) {
        DPRINTK("copy data from user fail... \n");
        goto err; 
    }

    in_total_size = pcmd.cmd_buf.size_in;
    in_data_size = in_total_size - ioctl_cmd_size;
    out_total_size = pcmd.cmd_buf.size_out;

    buf = (u8*)kzalloc(in_total_size, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    if(copy_from_user(buf, (void*)arg, in_total_size)) {
        DPRINTK("copy data from user fail... \n");
        goto err; 
    }
    data = buf + ioctl_cmd_size;

    if(arg != 0) {
        switch(pcmd.cmd_type) {
        /* ioctl diag begin */
        case FUXI_DFS_IOCTL_DIAG_BEGIN:
            DPRINTK("Debugfs received diag begin command.\n");
            if (netif_running(pdata->netdev)){
                fxgmac_restart_dev(pdata);
            }

            /* release last loopback test abnormal exit buffer */
            while(ex->fxgmac_test_skb_arr_in_index != 
                                            ex->fxgmac_test_skb_arr_out_index)
            {
                tmpskb = ex->fxgmac_test_skb_array[ex->fxgmac_test_skb_arr_out_index];
                if(tmpskb)
                {
                    kfree_skb(tmpskb);
                    ex->fxgmac_test_skb_array[ex->fxgmac_test_skb_arr_out_index] = NULL;
                }

                ex->fxgmac_test_skb_arr_out_index = (ex->fxgmac_test_skb_arr_out_index + 1) % FXGMAC_MAX_DBG_TEST_PKT;
            }

            /* init loopback test parameters */
            ex->fxgmac_test_skb_arr_in_index = 0;
            ex->fxgmac_test_skb_arr_out_index = 0;
            ex->fxgmac_test_tso_flag = false;
            ex->fxgmac_test_tso_seg_num = 0;
            ex->fxgmac_test_last_tso_len = 0;
            ex->fxgmac_test_packet_len = 0;
           break;

        /* ioctl diag end */
        case FUXI_DFS_IOCTL_DIAG_END:
            DPRINTK("Debugfs received diag end command.\n");
            if (netif_running(pdata->netdev)){
                fxgmac_restart_dev(pdata);
            }
            break;

        /* ioctl diag tx pkt */
        case FUXI_DFS_IOCTL_DIAG_TX_PKT:
            fxgmac_dbg_tx_pkt(pdata, buf);
            break;

        /* ioctl diag rx pkt */
        case FUXI_DFS_IOCTL_DIAG_RX_PKT:
            fxgmac_dbg_rx_pkt(pdata, buf);
            break;

        /* ioctl device reset */
        case FUXI_DFS_IOCTL_DEVICE_RESET:
            DPRINTK("Debugfs received device reset command.\n");
            if (netif_running(pdata->netdev)){
                fxgmac_restart_dev(pdata);
            }
            break;

        case FXGMAC_EFUSE_LED_TEST:
            DPRINTK("Debugfs received device led test command.\n");
            memcpy(&pdata->led, data, sizeof(struct led_setting));
            fxgmac_restart_dev(pdata);
            break;

        case FXGMAC_EFUSE_UPDATE_LED_CFG:
            DPRINTK("Debugfs received device led update command.\n");
            memcpy(&pdata->ledconfig, data, sizeof(struct led_setting));
            ret = hw_ops->write_led_config(pdata);
            hw_ops->read_led_config(pdata);
            hw_ops->led_under_active(pdata);
            break;

        case FXGMAC_EFUSE_WRITE_LED:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            DPRINTK("FXGMAC_EFUSE_WRITE_LED, val = 0x%x\n", ex_data.val0);
            ret = hw_ops->write_led(pdata, ex_data.val0);
            break;

        case FXGMAC_EFUSE_WRITE_OOB:
            DPRINTK("FXGMAC_EFUSE_WRITE_OOB.\n");
            ret = hw_ops->write_oob(pdata);
            break;

        case FXGMAC_EFUSE_READ_REGIONABC:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            ret = hw_ops->read_efuse_data(pdata, ex_data.val0, &ex_data.val1);
            /*
             * DPRINTK("FXGMAC_EFUSE_READ_REGIONABC, address = 0x%x, val = 0x%x\n", 
             *    ex_data.val0, 
             *    ex_data.val1);
             */
            if (ret) {
                memcpy(data, &ex_data, sizeof(CMD_DATA));
                out_total_size = ioctl_cmd_size + sizeof(CMD_DATA);
                if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                    goto err;
            }
            break;

        case FXGMAC_EFUSE_WRITE_PATCH_REG:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            /*
             * DPRINTK("FXGMAC_EFUSE_WRITE_PATCH_REG, address = 0x%x, val = 0x%x\n", 
             *    ex_data.val0, 
             *    ex_data.val1);
             */
            ret = hw_ops->write_patch_to_efuse(pdata, ex_data.val0, ex_data.val1);
            break;

        case FXGMAC_EFUSE_READ_PATCH_REG:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            ret = hw_ops->read_patch_from_efuse(pdata, ex_data.val0, &ex_data.val1);
            /*
             * DPRINTK("FXGMAC_EFUSE_READ_PATCH_REG, address = 0x%x, val = 0x%x\n", 
             *    ex_data.val0, ex_data.val1);
             */
            if (ret) {
                memcpy(data, &ex_data, sizeof(CMD_DATA));
                out_total_size = ioctl_cmd_size + sizeof(CMD_DATA);
                if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                    goto err;
            }
            break;

        case FXGMAC_EFUSE_WRITE_PATCH_PER_INDEX:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            ret = hw_ops->write_patch_to_efuse_per_index(pdata, ex_data.val0,
                                                            ex_data.val1,
                                                            ex_data.val2);
            /*
             * DPRINTK("FXGMAC_EFUSE_WRITE_PATCH_PER_INDEX, index = %d, address = 0x%x, val = 0x%x\n", 
             *            ex_data.val0, ex_data.val1, ex_data.val2);
             */
            break;

        case FXGMAC_EFUSE_READ_PATCH_PER_INDEX:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            ret = hw_ops->read_patch_from_efuse_per_index(pdata,ex_data.val0,
                                                            &ex_data.val1,
                                                            &ex_data.val2);
            /*
             * DPRINTK("FXGMAC_EFUSE_READ_PATCH_PER_INDEX, address = 0x%x, val = 0x%x\n", 
             *    ex_data.val1, ex_data.val2);
             */
            if (ret) {
                memcpy(data, &ex_data, sizeof(CMD_DATA));
                out_total_size = ioctl_cmd_size + sizeof(CMD_DATA);
                if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                    goto err;
            }
            break;

        case FXGMAC_EFUSE_LOAD:
            DPRINTK("FXGMAC_EFUSE_LOAD.\n");
            ret = hw_ops->efuse_load(pdata);
            break;

        case FXGMAC_GET_MAC_DATA:
            ret = hw_ops->read_mac_subsys_from_efuse(pdata, mac, NULL, NULL);
            if (ret) {
                memcpy(data, mac, ETH_ALEN);
                out_total_size = ioctl_cmd_size + ETH_ALEN;
                if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                    goto err;
            }
            break;

        case FXGMAC_SET_MAC_DATA:
            if (in_data_size != ETH_ALEN)
                goto err;
            memcpy(mac, data, ETH_ALEN);
            ret = hw_ops->write_mac_subsys_to_efuse(pdata, mac, NULL, NULL);
            if (ret) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0))
                eth_hw_addr_set(pdata->netdev, mac);
#else
                memcpy(pdata->netdev->dev_addr, mac, ETH_ALEN);
#endif

                memcpy(pdata->mac_addr, mac, ETH_ALEN);

                hw_ops->set_mac_address(pdata, mac);
                hw_ops->set_mac_hash(pdata);
            }
            break;

        case FXGMAC_GET_SUBSYS_ID:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            ret = hw_ops->read_mac_subsys_from_efuse(pdata,
                                                    NULL,
                                                    &ex_data.val0,
                                                    NULL);
            if (ret) {
                ex_data.val1 = 0xFFFF; // invalid value
                memcpy(data, &ex_data, sizeof(CMD_DATA));
                out_total_size = ioctl_cmd_size + sizeof(CMD_DATA);
                if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                    goto err;
            }
            break;

        case FXGMAC_SET_SUBSYS_ID:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            ret = hw_ops->write_mac_subsys_to_efuse(pdata,
                                                    NULL,
                                                    &ex_data.val0,
                                                    NULL);
            break;

        case FXGMAC_GET_REG:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            ex_data.val1 = hw_ops->get_gmac_register(pdata,
                                        (u8*)(pdata->base_mem + ex_data.val0));
            memcpy(data, &ex_data, sizeof(CMD_DATA));
            out_total_size = ioctl_cmd_size + sizeof(CMD_DATA);
            if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                goto err;
            break;

        case FXGMAC_SET_REG:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            regval = hw_ops->set_gmac_register(pdata,
                                        (u8*)(pdata->base_mem + ex_data.val0),
                                        ex_data.val1);
            ret = (regval == 0 ? true : false);
            break;

        case FXGMAC_GET_PHY_REG:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            regval = hw_ops->read_ephy_reg(pdata, ex_data.val0, &ex_data.val1);
            if (regval != -1) {
                memcpy(data, &ex_data, sizeof(CMD_DATA));
                out_total_size = ioctl_cmd_size + sizeof(CMD_DATA);
                if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                    goto err;
            }
            ret = (regval == -1 ? false : true);
            break;

        case FXGMAC_SET_PHY_REG:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            regval = hw_ops->write_ephy_reg(pdata, ex_data.val0, ex_data.val1);
            ret = (regval == 0 ? true : false);
            break;

        case FXGMAC_GET_PCIE_LOCATION:
            ex_data.val0 = pdata->pdev->bus->number;
            ex_data.val1 = PCI_SLOT(pdata->pdev->devfn);
            ex_data.val2 = PCI_FUNC(pdata->pdev->devfn);
            memcpy(data, &ex_data, sizeof(CMD_DATA));
            out_total_size = ioctl_cmd_size + sizeof(CMD_DATA);
            if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                goto err;
            break;

        case FXGMAC_GET_GSO_SIZE:
            ex_data.val0 = pdata->netdev->gso_max_size;
            memcpy(data, &ex_data, sizeof(CMD_DATA));
            out_total_size = ioctl_cmd_size + sizeof(CMD_DATA);
            if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                goto err;
            break;

        case FXGMAC_SET_GSO_SIZE:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            pdata->netdev->gso_max_size = ex_data.val0;
            break;

        case FXGMAC_SET_RX_MODERATION:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            regval = readreg(pdata->pAdapter, pdata->base_mem + INT_MOD);
            regval = FXGMAC_SET_REG_BITS(regval, INT_MOD_RX_POS, INT_MOD_RX_LEN, ex_data.val0);
            writereg(pdata->pAdapter, regval, pdata->base_mem + INT_MOD);
            break;

        case FXGMAC_SET_TX_MODERATION:
            memcpy(&ex_data, data, sizeof(CMD_DATA));
            regval = readreg(pdata->pAdapter, pdata->base_mem + INT_MOD);
            regval = FXGMAC_SET_REG_BITS(regval, INT_MOD_TX_POS, INT_MOD_TX_LEN, ex_data.val0);
            writereg(pdata->pAdapter, regval, pdata->base_mem + INT_MOD);
            break;

         case FXGMAC_GET_TXRX_MODERATION:
            regval = readreg(pdata->pAdapter, pdata->base_mem + INT_MOD);
            ex_data.val0 = FXGMAC_GET_REG_BITS(regval, INT_MOD_RX_POS, INT_MOD_RX_LEN);
            ex_data.val1 = FXGMAC_GET_REG_BITS(regval, INT_MOD_TX_POS, INT_MOD_TX_LEN);
            memcpy(data, &ex_data, sizeof(CMD_DATA));
            out_total_size = ioctl_cmd_size + sizeof(CMD_DATA);
            if (copy_to_user((void*)arg, (void*)buf, out_total_size))
                goto err;
            break;

        default:
            DPRINTK("Debugfs received invalid command: %x.\n", pcmd.cmd_type);
            ret = false;
            break;
        }   
    }

    if (buf)
        kfree(buf);
    return ret ? FXGMAC_SUCCESS : FXGMAC_FAIL;

err:
    if (buf)
        kfree(buf);
    return FXGMAC_FAIL;
}

#ifdef HAVE_FXGMAC_DEBUG_FS

static struct file_operations fxgmac_dbg_netdev_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = fxgmac_dbg_netdev_ops_read,
	.write = fxgmac_dbg_netdev_ops_write,
    .unlocked_ioctl = fxgmac_dbg_netdev_ops_ioctl,
};

/**
 * fxgmac_dbg_adapter_init - setup the debugfs directory for the adapter
 * @pdata: board private structure
 **/
void fxgmac_dbg_adapter_init(struct fxgmac_pdata *pdata)
{
    const char *name = pdata->drv_name;
    struct dentry *pfile;

    pdata->expansion.dbg_adapter = debugfs_create_dir(name, pdata->expansion.fxgmac_dbg_root);
    if (pdata->expansion.dbg_adapter) {
        pfile = debugfs_create_file("netdev_ops", 0600,
    	    		    pdata->expansion.dbg_adapter, pdata,
    	    		    &fxgmac_dbg_netdev_ops_fops);
        if (!pfile)
        	DPRINTK("debugfs netdev_ops for %s failed\n", name);
    } else {
        DPRINTK("debugfs entry for %s failed\n", name);
    }
}

/**
 * fxgmac_dbg_adapter_exit - clear out the adapter's debugfs entries
 * @pdata: board private structure
 **/
void fxgmac_dbg_adapter_exit(struct fxgmac_pdata *pdata)
{
    if (pdata->expansion.dbg_adapter)
    	debugfs_remove_recursive(pdata->expansion.dbg_adapter);
    pdata->expansion.dbg_adapter = NULL;
}

/**
 * fxgmac_dbg_init - start up debugfs for the driver
 * @pdata: board private structure
 **/
void fxgmac_dbg_init(struct fxgmac_pdata *pdata)
{
    unsigned int i;
    char num[3];
    const char debug_path[] = "/sys/kernel/debug/";
    const char file_prefix[] = "fuxi_";
    char file_path[50];
    char file_name[8];
    
    /* init file_path */
    memset(file_path, '\0', sizeof(file_path));
    memcpy(file_path, debug_path, sizeof(debug_path));

	for(i = 0; i < DF_MAX_NIC_NUM; i++)
	{
        /* init num and filename */
        memset(num, '\0', sizeof(num));
        memset(file_name, '\0', sizeof(file_name));

        /* int to string */
        sprintf(num, "%d", i);

        /* file name */
        memcpy(file_name, file_prefix, sizeof(file_prefix));
        memcpy(file_name + strlen(file_prefix), num, sizeof(num));

        /* file path */
        memcpy(file_path + sizeof(debug_path) - 1, file_name, sizeof(file_name));
        //DPRINTK("FXG: file_path is %s", file_path);

        /* whether file exist */
        pdata->expansion.fxgmac_dbg_root = debugfs_lookup(file_name, NULL);
        if (!pdata->expansion.fxgmac_dbg_root)
        {
            /* create file */
	        pdata->expansion.fxgmac_dbg_root = debugfs_create_dir(file_name, NULL);
            if (IS_ERR(pdata->expansion.fxgmac_dbg_root))
                DPRINTK("fxgmac init of debugfs failed\n");

            break;
        }
	}
}

/**
 * fxgmac_dbg_exit - clean out the driver's debugfs entries
 * @pdata: board private structure
 **/
void fxgmac_dbg_exit(struct fxgmac_pdata *pdata)
{
    if (pdata->expansion.fxgmac_dbg_root)
        debugfs_remove_recursive(pdata->expansion.fxgmac_dbg_root);
}

#endif /* HAVE_XLGMAC_DEBUG_FS */
