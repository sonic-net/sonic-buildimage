// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * See fpm_mac.h. This file deliberately depends only on the kernel netlink
 * headers, not on zebra, so the decode can be exercised directly by
 * tests/zebra/fpm_mac_test.c.
 */
#include <string.h>
#include <sys/socket.h>
#include <linux/neighbour.h>
#include <linux/nexthop.h>

#include "fpm_mac.h"

bool fpm_mac_decode(const struct nlmsghdr *hdr, struct fpm_local_mac *out)
{
	const struct ndmsg *ndm;
	struct rtattr *rta;
	struct fpm_local_mac decoded;
	bool have_mac = false;
	int len;

	if (hdr == NULL || out == NULL)
		return false;

	if (hdr->nlmsg_type != RTM_NEWNEIGH && hdr->nlmsg_type != RTM_DELNEIGH)
		return false;

	if (hdr->nlmsg_len < NLMSG_LENGTH(sizeof(struct ndmsg)))
		return false;

	ndm = (const struct ndmsg *)NLMSG_DATA(hdr);
	if (ndm->ndm_family != AF_BRIDGE)
		return false;

	memset(&decoded, 0, sizeof(decoded));
	decoded.ifindex = ndm->ndm_ifindex;
	decoded.generation = hdr->nlmsg_seq;
	decoded.del = (hdr->nlmsg_type == RTM_DELNEIGH);
	decoded.protocol = RTPROT_UNSPEC;
	/* Stickiness rides in ndm_flags as NTF_STICKY, the same field zebra's
	 * kernel path reads. It must not be taken from ndm_state: NUD_NOARP
	 * means the entry does not age, which is a different property, and
	 * conflating the two advertises every non-ageing MAC to BGP with the
	 * EVPN sticky bit and so blocks MAC mobility. */
	decoded.sticky = !!(ndm->ndm_flags & NTF_STICKY);

	len = (int)(hdr->nlmsg_len - NLMSG_LENGTH(sizeof(struct ndmsg)));
	rta = (struct rtattr *)((char *)ndm + NLMSG_ALIGN(sizeof(struct ndmsg)));

	for (; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
		switch (rta->rta_type) {
		case NDA_LLADDR:
			if (RTA_PAYLOAD(rta) != ETH_ALEN)
				break;
			memcpy(decoded.mac, RTA_DATA(rta), ETH_ALEN);
			have_mac = true;
			break;
		case NDA_VLAN:
			if (RTA_PAYLOAD(rta) < sizeof(uint16_t))
				break;
			decoded.vid = *(uint16_t *)RTA_DATA(rta);
			break;
		case NDA_PROTOCOL:
			if (RTA_PAYLOAD(rta) < sizeof(uint8_t))
				break;
			decoded.protocol = *(uint8_t *)RTA_DATA(rta);
			break;
		default:
			break;
		}
	}

	/* Without a MAC there is nothing to install, and a caller acting on a
	 * partially filled struct would install a zero MAC. */
	if (!have_mac)
		return false;

	/* Loop guard, mirroring zebra's kernel path: an entry zebra itself
	 * originated must never be fed back in as a locally learnt MAC. */
	if (decoded.protocol == RTPROT_ZEBRA)
		return false;

	*out = decoded;
	return true;
}

bool fpm_mac_is_stale(bool fpm_learned, uint32_t mac_generation,
		      uint32_t sweep_generation)
{
	if (!fpm_learned)
		return false;

	return mac_generation != sweep_generation;
}

static bool fpm_attr_put(struct nlmsghdr *n, size_t maxlen, int type,
			 const void *data, size_t alen)
{
	size_t len = RTA_LENGTH(alen);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen)
		return false;

	rta = (struct rtattr *)(((char *)n) + NLMSG_ALIGN(n->nlmsg_len));
	rta->rta_type = (unsigned short)type;
	rta->rta_len = (unsigned short)len;
	if (alen)
		memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = (uint32_t)(NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len));

	return true;
}

size_t fpm_fdb_nh_encode(const struct fpm_fdb_nh *nh, void *buf, size_t buflen)
{
	struct nlmsghdr *n = buf;
	struct nhmsg *nhm;

	if (nh == NULL || buf == NULL)
		return 0;

	if (buflen < NLMSG_LENGTH(sizeof(struct nhmsg)))
		return 0;

	memset(buf, 0, NLMSG_LENGTH(sizeof(struct nhmsg)));
	n->nlmsg_len = NLMSG_LENGTH(sizeof(struct nhmsg));
	n->nlmsg_flags = NLM_F_REQUEST;
	if (!nh->del)
		n->nlmsg_flags |= NLM_F_CREATE | NLM_F_REPLACE;
	n->nlmsg_type = nh->del ? RTM_DELNEXTHOP : RTM_NEWNEXTHOP;

	nhm = (struct nhmsg *)NLMSG_DATA(n);
	nhm->nh_family = AF_UNSPEC;

	if (!fpm_attr_put(n, buflen, NHA_ID, &nh->id, sizeof(nh->id)))
		return 0;

	/* Without this the receiver cannot tell the message from an ordinary L3
	 * nexthop and would publish it as a real nexthop group. */
	if (!fpm_attr_put(n, buflen, NHA_FDB, NULL, 0))
		return 0;

	if (nh->del)
		return n->nlmsg_len;

	if (nh->family == AF_INET) {
		nhm->nh_family = AF_INET;
		if (!fpm_attr_put(n, buflen, NHA_GATEWAY, nh->addr, 4))
			return 0;
	} else if (nh->family == AF_INET6) {
		nhm->nh_family = AF_INET6;
		if (!fpm_attr_put(n, buflen, NHA_GATEWAY, nh->addr, 16))
			return 0;
	} else if (nh->member_cnt > 0 && nh->members != NULL) {
		struct nexthop_grp grp[FPM_FDB_NH_MAX_MEMBERS];
		uint32_t i;

		if (nh->member_cnt > FPM_FDB_NH_MAX_MEMBERS)
			return 0;

		memset(grp, 0, sizeof(grp));
		for (i = 0; i < nh->member_cnt; i++)
			grp[i].id = nh->members[i];

		if (!fpm_attr_put(n, buflen, NHA_GROUP, grp,
				  nh->member_cnt * sizeof(struct nexthop_grp)))
			return 0;
	} else {
		/* Neither a VTEP nor a group: nothing the receiver can resolve. */
		return 0;
	}

	return n->nlmsg_len;
}
