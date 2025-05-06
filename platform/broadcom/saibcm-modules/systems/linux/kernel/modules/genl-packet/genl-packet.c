/*
 * net/genl-packet/genl-packet.c - Netlink channel for general packetIO
 * Copyright (c) 2021 Google, based on psample.c (originally written by
 * Yotam Gigi <yotamg@mellanox.com>)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <lkm.h>
#include <linux/skbuff.h>
#include <net/genetlink.h>
#include <net/genl-packet.h>

#define GENL_PACKET_MAX_PACKET_SIZE 0xffff

/* multicast groups */
enum genl_packet_multicast_groups {
	GENL_PACKET_MCGRP_PACKET,
};

static const struct genl_multicast_group genl_packet_mcgrps[] = {
	[GENL_PACKET_MCGRP_PACKET] = { .name = GENL_PACKET_MCGRP_NAME },
};

static struct genl_family genl_packet_family = {
	.name		= GENL_PACKET_NAME,
	.version	= GENL_PACKET_VERSION,
	.maxattr	= GENL_PACKET_ATTR_MAX,
	.netnsok	= true,
	.module		= THIS_MODULE,
	.mcgrps		= genl_packet_mcgrps,
	.n_mcgrps	= ARRAY_SIZE(genl_packet_mcgrps),
};

void genl_packet_send_packet(struct net *net, struct sk_buff *skb,
					  int in_ifindex, int out_ifindex, unsigned int context)
{
	struct sk_buff *nl_skb;
	int data_len;
	int meta_len;
	void *data;
	int ret;

    /* Metalength is sum of netlink message sizes of in_ifindex + out_ifindex +
     * context */
	meta_len = nla_total_size(sizeof(s16)) +
             nla_total_size(sizeof(s16)) +
             nla_total_size(sizeof(u32));

	data_len = skb->len;
	if (meta_len + nla_total_size(data_len) > GENL_PACKET_MAX_PACKET_SIZE)
		data_len = GENL_PACKET_MAX_PACKET_SIZE - meta_len - NLA_HDRLEN
			    - NLA_ALIGNTO;
	if (data_len <= 0)
		return;

	nl_skb = genlmsg_new(meta_len + nla_total_size(data_len), GFP_ATOMIC);
	if (unlikely(!nl_skb))
		return;

	data = genlmsg_put(nl_skb, 0, 0, &genl_packet_family, 0,
			   GENL_PACKET_CMD_PACKET);
	if (unlikely(!data))
		goto error;

	ret = nla_put_s16(nl_skb, GENL_PACKET_ATTR_IIFINDEX, in_ifindex);
	if (unlikely(ret < 0))
		goto error;

	ret = nla_put_s16(nl_skb, GENL_PACKET_ATTR_OIFINDEX, out_ifindex);
	if (unlikely(ret < 0))
		goto error;

	ret = nla_put_u32(nl_skb, GENL_PACKET_ATTR_CONTEXT, context);
	if (unlikely(ret < 0))
		goto error;

	if (data_len > 0) {
		int nla_len = nla_total_size(data_len);
		struct nlattr *nla;

		nla = (struct nlattr *)skb_put(nl_skb, nla_len);
		nla->nla_type = GENL_PACKET_ATTR_DATA;
		nla->nla_len = nla_attr_size(data_len);

		if (skb_copy_bits(skb, 0, nla_data(nla), data_len))
			goto error;
	}

	genlmsg_end(nl_skb, data);
	genlmsg_multicast_netns(&genl_packet_family, net, nl_skb, 0,
				GENL_PACKET_MCGRP_PACKET, GFP_ATOMIC);

	return;
error:
	pr_err_ratelimited("Could not create genl_packet message\n");
	nlmsg_free(nl_skb);
}
EXPORT_SYMBOL_GPL(genl_packet_send_packet);

static int __init genl_packet_module_init(void)
{
	return genl_register_family(&genl_packet_family);
}

static void __exit genl_packet_module_exit(void)
{
	genl_unregister_family(&genl_packet_family);
}

module_init(genl_packet_module_init);
module_exit(genl_packet_module_exit);

MODULE_AUTHOR("Google");
MODULE_DESCRIPTION("netlink channel for genl_packet");
MODULE_LICENSE("GPL v2");
