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
