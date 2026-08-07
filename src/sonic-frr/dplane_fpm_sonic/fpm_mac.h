// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Decoding of FPM MAC (AF_BRIDGE neighbour) messages.
 *
 * Kept free of zebra state so it can be unit tested on its own, and so the
 * decode runs on the FPM thread while the caller applies the result on
 * zebra's main thread.
 */
#ifndef _FPM_MAC_H
#define _FPM_MAC_H

#include <stdbool.h>
#include <stdint.h>
#include <linux/rtnetlink.h>

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

/* Present only on newer kernel headers. */
#ifndef NTF_STICKY
#define NTF_STICKY 0x40
#endif

struct fpm_local_mac {
	int ifindex;		/* bridge port the ASIC learnt the MAC on */
	uint8_t mac[ETH_ALEN];
	uint16_t vid;		/* VLAN on that port, 0 when untagged */
	bool del;		/* aged out, or no longer on the port */
	bool sticky;		/* static MAC: EVPN sticky, it may not move */
};

/*
 * Decode the AF_BRIDGE RTM_NEWNEIGH/RTM_DELNEIGH that fpmsyncd sends for each
 * entry fdborch writes to or removes from STATE_DB FDB_TABLE. Returns false and
 * leaves *out untouched when the message is not a usable local MAC.
 *
 * fpmsyncd follows zebra's own FDB encoding (netlink_macfdb_update_ctx()):
 *
 *   field        dynamic MAC                   static MAC
 *   ndm_ifindex  bridge port                   bridge port
 *   ndm_flags    NTF_MASTER | NTF_EXT_LEARNED  NTF_MASTER | NTF_EXT_LEARNED |
 *                                              NTF_STICKY
 *   ndm_state    NUD_REACHABLE                 NUD_REACHABLE | NUD_NOARP
 *   NDA_LLADDR   MAC                           MAC
 *   NDA_VLAN     VLAN                          VLAN
 *
 * NTF_STICKY is the EVPN sticky bit: the MAC may not move. NUD_NOARP only means
 * the entry does not age.
 */
extern bool fpm_mac_decode(const struct nlmsghdr *hdr, struct fpm_local_mac *out);

#endif /* _FPM_MAC_H */
