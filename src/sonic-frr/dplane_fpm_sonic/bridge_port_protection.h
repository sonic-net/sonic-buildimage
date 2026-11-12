/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SONiC protection FPM adapter. See https://github.com/sonic-net/SONiC/pull/2556. */
#ifndef SONIC_BRIDGE_PORT_PROTECTION_H
#define SONIC_BRIDGE_PORT_PROTECTION_H
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Wire definitions and ABI guards live with the existing SONiC FPM
 * extensions in zebra/kernel_netlink.h. Only encoding helpers live here. */
#include "zebra/kernel_netlink.h"

/* Per-port Binding ABI: signed interface index, complete unsigned Group ID. */
static inline ssize_t sonic_bpp_binding(uint32_t ifindex, uint32_t group_id,
					bool deleting, uint8_t *buffer,
					size_t capacity)
{
	struct evpn_backup_nhg_msg binding = {0};
	struct nlmsghdr header = {0};
	size_t length = NLMSG_LENGTH(sizeof(binding));
	if (!ifindex || ifindex > INT_MAX || capacity < length)
		return -1;
	binding.ebnm_ifindex = ifindex;
	binding.ebnm_backup_nhg_id = deleting ? 0 : group_id;
	header.nlmsg_len = length;
	header.nlmsg_type = binding.ebnm_backup_nhg_id
				? RTM_FPM_ADD_EVPN_ES_BACKUP_NHG
				: RTM_FPM_DEL_EVPN_ES_BACKUP_NHG;
	header.nlmsg_flags = NLM_F_CREATE | NLM_F_REQUEST;
	memset(buffer, 0, length);
	memcpy(buffer, &header, sizeof(header));
	memcpy(buffer + NLMSG_HDRLEN, &binding, sizeof(binding));
	return NLMSG_ALIGN(length);
}

/* FRR #24071 provides these operations/getters. Older FRR keeps its existing
 * plugin behavior until the companion control-plane support is integrated. */
#ifdef EVPN_MH_NHG_MAX_MEMBERS
static inline bool sonic_bpp_id(uint32_t id, unsigned type)
{
	return (id >> 28) == type;
}

static inline int sonic_bpp_member(struct l2_nhg_member_msg *out, uint32_t id,
				   uint8_t type, const struct ipaddr *vtep,
				   const struct ethaddr *rmac)
{
	if (!sonic_bpp_id(id, 4) || type != L2_NHG_MEMBER_TYPE_L3 || !vtep ||
	    !rmac)
		return -1;
	memset(out, 0, sizeof(*out));
	out->l2nm_nh_id = id;
	out->l2nm_type = type;
	if (IS_IPADDR_V4(vtep)) {
		out->l2nm_family = AF_INET;
		out->l2nm_remote_vtep.ip4 = vtep->ipaddr_v4;
	} else if (IS_IPADDR_V6(vtep)) {
		out->l2nm_family = AF_INET6;
		out->l2nm_remote_vtep.ip6 = vtep->ipaddr_v6;
	} else
		return -1;
	memcpy(out->l2nm_router_mac, rmac, sizeof(out->l2nm_router_mac));
	if ((out->l2nm_router_mac[0] & 1) ||
	    !memcmp(out->l2nm_router_mac, (uint8_t[6]){0}, 6))
		return -1;
	return 0;
}

static inline int sonic_bpp_member_compare(const void *a, const void *b)
{
	const struct l2_nhg_member_msg *x = a, *y = b;
	return (x->l2nm_nh_id > y->l2nm_nh_id) -
	       (x->l2nm_nh_id < y->l2nm_nh_id);
}

/* Returns an aligned netlink message, or -1 without publishing partial data.
 * The caller supplies the ordinary FPM framing/output-buffer handling. */
static inline ssize_t sonic_bpp_encode(const struct zebra_dplane_ctx *ctx,
				       uint8_t *buffer, size_t capacity)
{
	enum dplane_op_e op = dplane_ctx_get_op(ctx);
	struct nlmsghdr header = {0};
	bool deleting;
	uint32_t id, count;
	size_t length;
	if (op == DPLANE_OP_EVPN_MH_REPLAY_DONE) {
		length = NLMSG_LENGTH(0);
		if (capacity < length)
			return -1;
		memset(buffer, 0, length);
		header.nlmsg_type = RTM_FPM_EVPN_MH_REPLAY_DONE;
	} else if (op == DPLANE_OP_EVPN_MH_NH_UPDATE) {
		struct l2_nhg_member_msg member = {0};
		deleting = dplane_ctx_get_evpn_mh_is_del(ctx);
		id = dplane_ctx_get_evpn_mh_nh_id(ctx);
		length = NLMSG_LENGTH(sizeof(member));
		if (!sonic_bpp_id(id, 4) || capacity < length)
			return -1;
		if (!deleting &&
		    sonic_bpp_member(&member, id,
				     dplane_ctx_get_evpn_mh_nh_type(ctx),
				     dplane_ctx_get_evpn_mh_vtep(ctx),
				     dplane_ctx_get_evpn_mh_rmac(ctx)) < 0)
			return -1;
		member.l2nm_nh_id = id;
		memset(buffer, 0, length);
		memcpy(buffer + NLMSG_HDRLEN, &member, sizeof(member));
		header.nlmsg_type =
		    deleting ? RTM_FPM_DEL_L2_NEXTHOP : RTM_FPM_ADD_L2_NEXTHOP;
	} else if (op == DPLANE_OP_EVPN_MH_NHG_UPDATE) {
		struct l2_nhg_group_msg *group;
		const struct dplane_evpn_mh_member *members;
		const esi_t *esi;
		deleting = dplane_ctx_get_evpn_mh_is_del(ctx);
		id = dplane_ctx_get_evpn_mh_nhg_id(ctx);
		count = deleting ? 0 : dplane_ctx_get_evpn_mh_member_count(ctx);
		if (!sonic_bpp_id(id, 8) || count > L2_NHG_MAX_MEMBERS)
			return -1;
		length = NLMSG_LENGTH(L2_NHG_GROUP_MSG_LEN(count));
		if (capacity < length)
			return -1;
		members = dplane_ctx_get_evpn_mh_members(ctx);
		esi = dplane_ctx_get_evpn_mh_esi(ctx);
		if (!deleting && ((!members && count) || !esi))
			return -1;
		memset(buffer, 0, length);
		group = (void *)(buffer + NLMSG_HDRLEN);
		group->l2ng_nhg_id = id;
		group->l2ng_count = count;
		if (!deleting)
			memcpy(group->l2ng_esi, esi, L2_NHG_ESI_LEN);
		for (uint32_t i = 0; i < count; i++)
			if (sonic_bpp_member(&group->l2ng_members[i],
					     members[i].nh_id, members[i].type,
					     &members[i].vtep,
					     &members[i].rmac) < 0)
				return -1;
		qsort(group->l2ng_members, count,
		      sizeof(group->l2ng_members[0]), sonic_bpp_member_compare);
		for (uint32_t i = 1; i < count; i++)
			if (group->l2ng_members[i - 1].l2nm_nh_id ==
			    group->l2ng_members[i].l2nm_nh_id)
				return -1;
		header.nlmsg_type = deleting ? RTM_FPM_DEL_L2_NEXTHOP_GROUP
					     : RTM_FPM_ADD_L2_NEXTHOP_GROUP;
	} else
		return -1;
	header.nlmsg_len = length;
	header.nlmsg_flags = NLM_F_CREATE | NLM_F_REQUEST;
	memcpy(buffer, &header, sizeof(header));
	return NLMSG_ALIGN(length);
}
#endif
#endif
