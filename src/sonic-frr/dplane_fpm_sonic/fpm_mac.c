// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * See fpm_mac.h. This file deliberately depends only on the kernel netlink
 * headers, not on zebra, so both directions can be exercised directly by
 * tests/zebra/fpm_mac_test.c.
 */
#include <string.h>
#include <sys/socket.h>
#include <linux/neighbour.h>

#include "fpm_mac.h"

bool fpm_mac_decode(const struct nlmsghdr *hdr, struct fpm_hw_mac *out)
{
	const struct ndmsg *ndm;
	struct rtattr *rta;
	struct fpm_hw_mac decoded;
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
	decoded.del = (hdr->nlmsg_type == RTM_DELNEIGH);
	/* fpmsyncd sends NTF_MASTER | NTF_EXT_LEARNED with NUD_REACHABLE, and adds
	 * NTF_STICKY with NUD_NOARP for a static MAC, as zebra's own encoding does.
	 * Stickiness is read from NTF_STICKY, the field zebra's kernel path reads:
	 * it is the EVPN sticky bit, the MAC may not move. NUD_NOARP only means the
	 * entry does not age; reading it as sticky would advertise every non-ageing
	 * MAC as sticky and block MAC mobility. */
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
		default:
			break;
		}
	}

	/* Without a MAC there is nothing to install, and a caller acting on a
	 * partially filled struct would install a zero MAC. */
	if (!have_mac)
		return false;

	*out = decoded;
	return true;
}

static bool fpm_mac_put_attr(struct nlmsghdr *n, size_t buflen, int type,
			     const void *data, size_t len)
{
	size_t off = NLMSG_ALIGN(n->nlmsg_len);
	struct rtattr *rta;

	if (off + RTA_SPACE(len) > buflen)
		return false;

	rta = (struct rtattr *)((char *)n + off);
	memset(rta, 0, RTA_SPACE(len));
	rta->rta_type = (unsigned short)type;
	rta->rta_len = (unsigned short)RTA_LENGTH(len);
	memcpy(RTA_DATA(rta), data, len);
	n->nlmsg_len = (uint32_t)(off + RTA_ALIGN(rta->rta_len));
	return true;
}

size_t fpm_mac_encode(const struct fpm_remote_mac *in, void *buf, size_t buflen)
{
	struct nlmsghdr *n = buf;
	size_t vtep_len = 0;
	struct ndmsg *ndm;

	if (in == NULL || buf == NULL ||
	    buflen < NLMSG_SPACE(sizeof(struct ndmsg)))
		return 0;

	memset(buf, 0, NLMSG_SPACE(sizeof(struct ndmsg)));
	n->nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
	n->nlmsg_type = in->del ? RTM_DELNEIGH : RTM_NEWNEIGH;
	n->nlmsg_flags = NLM_F_REQUEST;
	if (!in->del)
		n->nlmsg_flags |= NLM_F_CREATE | NLM_F_REPLACE;

	ndm = NLMSG_DATA(n);
	ndm->ndm_family = AF_BRIDGE;
	ndm->ndm_ifindex = in->ifindex;
	ndm->ndm_flags = NTF_MASTER | NTF_SELF | NTF_EXT_LEARNED;
	ndm->ndm_state = NUD_REACHABLE;
	if (in->sticky) {
		ndm->ndm_flags |= NTF_STICKY;
		ndm->ndm_state |= NUD_NOARP;
	}

	if (in->vtep_family == AF_INET)
		vtep_len = 4;
	else if (in->vtep_family == AF_INET6)
		vtep_len = 16;

	if (!fpm_mac_put_attr(n, buflen, NDA_LLADDR, in->mac, ETH_ALEN))
		return 0;
	if (in->nhg_id) {
		if (!fpm_mac_put_attr(n, buflen, NDA_NH_ID, &in->nhg_id,
				      sizeof(in->nhg_id)))
			return 0;
	} else if (vtep_len) {
		if (!fpm_mac_put_attr(n, buflen, NDA_DST, in->vtep, vtep_len))
			return 0;
	}
	if (in->vid &&
	    !fpm_mac_put_attr(n, buflen, NDA_VLAN, &in->vid, sizeof(in->vid)))
		return 0;
	if (in->vni &&
	    !fpm_mac_put_attr(n, buflen, NDA_SRC_VNI, &in->vni, sizeof(in->vni)))
		return 0;

	return NLMSG_ALIGN(n->nlmsg_len);
}
