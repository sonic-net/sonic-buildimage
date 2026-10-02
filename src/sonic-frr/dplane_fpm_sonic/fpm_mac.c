// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * See fpm_mac.h. This file deliberately depends only on the kernel netlink
 * headers, not on zebra, so the decode can be exercised directly by
 * tests/zebra/fpm_mac_test.c.
 */
#include <string.h>
#include <sys/socket.h>
#include <linux/neighbour.h>

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
