// SPDX-License-Identifier: GPL-2.0
/*
 * sonic_stel - Minimal genetlink kernel module for SONiC Stream Telemetry
 *
 * Registers genetlink family "sonic_stel" with multicast group "ipfix",
 * enabling countersyncd to subscribe and receive IPFIX data in virtual
 * (vslib) environments.
 *
 * Userspace (vslib) writes IPFIX data via the netlink command interface,
 * and this module multicasts it to all subscribers.
 *
 * Usage from userspace:
 *   1. Open NETLINK_GENERIC socket
 *   2. Resolve family "sonic_stel" via CTRL_CMD_GETFAMILY
 *   3. Send GENL_CMD_SEND_IPFIX command with NLA attr containing IPFIX data
 *   4. Module multicasts the data to "ipfix" group subscribers (countersyncd)
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <net/genetlink.h>

#define SONIC_STEL_FAMILY_NAME  "sonic_stel"
#define SONIC_STEL_VERSION      1
#define SONIC_STEL_MCGRP_NAME   "ipfix"

/* Genetlink commands */
enum {
	SONIC_STEL_CMD_UNSPEC,
	SONIC_STEL_CMD_SEND_IPFIX,  /* Userspace -> kernel -> multicast */
	__SONIC_STEL_CMD_MAX,
};
#define SONIC_STEL_CMD_MAX (__SONIC_STEL_CMD_MAX - 1)

/* Netlink attributes */
enum {
	SONIC_STEL_ATTR_UNSPEC,
	SONIC_STEL_ATTR_IPFIX_DATA,  /* Binary IPFIX payload */
	__SONIC_STEL_ATTR_MAX,
};
#define SONIC_STEL_ATTR_MAX (__SONIC_STEL_ATTR_MAX - 1)

/* Multicast group */
static const struct genl_multicast_group sonic_stel_mcgrps[] = {
	{ .name = SONIC_STEL_MCGRP_NAME },
};

/* Attribute policy */
static const struct nla_policy sonic_stel_policy[SONIC_STEL_ATTR_MAX + 1] = {
	[SONIC_STEL_ATTR_IPFIX_DATA] = { .type = NLA_BINARY },
};

/* Forward declaration */
static int sonic_stel_cmd_send_ipfix(struct sk_buff *skb, struct genl_info *info);

/* Family definition - forward declared for ops */
static struct genl_family sonic_stel_family;

/* Operations */
static const struct genl_ops sonic_stel_ops[] = {
	{
		.cmd    = SONIC_STEL_CMD_SEND_IPFIX,
		.doit   = sonic_stel_cmd_send_ipfix,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
		.policy = sonic_stel_policy,
#endif
	},
};

/*
 * Handle SEND_IPFIX command: extract IPFIX payload from NLA and
 * multicast it to all "ipfix" group subscribers.
 *
 * The multicast message format matches what countersyncd expects:
 *   nlmsghdr (16B) + genlmsghdr (4B) + raw IPFIX payload
 *
 * We use nla_append to add raw IPFIX data without NLA header wrapping,
 * matching the HLD 7.2.4 kernel sample code pattern.
 */
static int sonic_stel_cmd_send_ipfix(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *skb_out;
	void *hdr;
	int ipfix_len;
	void *ipfix_data;

	if (!info->attrs[SONIC_STEL_ATTR_IPFIX_DATA]) {
		pr_err("sonic_stel: missing IPFIX data attribute\n");
		return -EINVAL;
	}

	ipfix_data = nla_data(info->attrs[SONIC_STEL_ATTR_IPFIX_DATA]);
	ipfix_len = nla_len(info->attrs[SONIC_STEL_ATTR_IPFIX_DATA]);

	/* Allocate output skb for multicast */
	skb_out = genlmsg_new(ipfix_len, GFP_KERNEL);
	if (!skb_out) {
		pr_err("sonic_stel: failed to allocate skb\n");
		return -ENOMEM;
	}

	/* Add genetlink header */
	hdr = genlmsg_put(skb_out, 0, 0, &sonic_stel_family,
			  0, SONIC_STEL_CMD_SEND_IPFIX);
	if (!hdr) {
		nlmsg_free(skb_out);
		return -EMSGSIZE;
	}

	/*
	 * Append raw IPFIX data directly (no NLA wrapper).
	 * countersyncd reads: skip nlmsghdr(16) + genlmsghdr(4) = 20 bytes,
	 * then treats the rest as raw IPFIX payload.
	 */
	if (nla_append(skb_out, ipfix_len, ipfix_data) < 0) {
		nlmsg_free(skb_out);
		return -EMSGSIZE;
	}

	genlmsg_end(skb_out, hdr);

	/* Multicast to ipfix group (group index 0) */
	return genlmsg_multicast(&sonic_stel_family, skb_out, 0, 0, GFP_KERNEL);
}

/* Family definition */
static struct genl_family sonic_stel_family = {
	.name       = SONIC_STEL_FAMILY_NAME,
	.version    = SONIC_STEL_VERSION,
	.maxattr    = SONIC_STEL_ATTR_MAX,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
	.policy     = sonic_stel_policy,
#endif
	.module     = THIS_MODULE,
	.ops        = sonic_stel_ops,
	.n_ops      = ARRAY_SIZE(sonic_stel_ops),
	.mcgrps     = sonic_stel_mcgrps,
	.n_mcgrps   = ARRAY_SIZE(sonic_stel_mcgrps),
};

static int __init sonic_stel_init(void)
{
	int ret;

	ret = genl_register_family(&sonic_stel_family);
	if (ret) {
		pr_err("sonic_stel: failed to register genetlink family: %d\n", ret);
		return ret;
	}

	pr_info("sonic_stel: registered genetlink family '%s' with multicast group '%s'\n",
		SONIC_STEL_FAMILY_NAME, SONIC_STEL_MCGRP_NAME);
	return 0;
}

static void __exit sonic_stel_exit(void)
{
	genl_unregister_family(&sonic_stel_family);
	pr_info("sonic_stel: unregistered genetlink family\n");
}

module_init(sonic_stel_init);
module_exit(sonic_stel_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SONiC");
MODULE_DESCRIPTION("Minimal genetlink module for SONiC Stream Telemetry (virtual SAI)");
