// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * The FPM MAC (AF_BRIDGE neighbour) messages exchanged with fpmsyncd: decoding
 * the local MACs SONiC sends, encoding the remote MACs FRR sends.
 *
 * Kept free of zebra state so both directions can be unit tested on their own,
 * and so the decode runs on the FPM thread while the caller applies the result
 * on zebra's main thread.
 */
#ifndef _FPM_MAC_H
#define _FPM_MAC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <linux/rtnetlink.h>

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

/* Present only on newer kernel headers. */
#ifndef NTF_STICKY
#define NTF_STICKY 0x40
#endif

struct fpm_hw_mac {
	int ifindex;		/* bridge port the hardware learnt the MAC on */
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
extern bool fpm_mac_decode(const struct nlmsghdr *hdr, struct fpm_hw_mac *out);

struct fpm_remote_mac {
	int ifindex;		/* VXLAN device, or the access port of a local ES */
	uint8_t mac[ETH_ALEN];
	uint16_t vid;		/* 0 when the bridge is not VLAN aware */
	uint32_t vni;		/* 0 when not known */
	int vtep_family;	/* AF_INET or AF_INET6, 0 without a VTEP */
	uint8_t vtep[16];
	uint32_t nhg_id;	/* L2 nexthop group of a remote ES, 0 when none */
	bool del;
	bool sticky;		/* EVPN sticky (static) MAC, it may not move */
};

/*
 * Encode the AF_BRIDGE RTM_NEWNEIGH/RTM_DELNEIGH that fpmsyncd turns into an
 * APPL_DB VXLAN_FDB_TABLE entry, for a remote MAC zebra learnt from BGP EVPN.
 * Returns the message length, or 0 when it does not fit in buflen.
 *
 * This is zebra's own encoding (netlink_macfdb_update_ctx()) in mac-ext-learn
 * mode, which fpm mode runs, less NDA_PROTOCOL and NDA_MASTER, which fpmsyncd
 * does not read:
 *
 *   field        dynamic MAC                   sticky MAC
 *   ndm_ifindex  ifindex                       ifindex
 *   ndm_flags    NTF_MASTER | NTF_SELF |       NTF_MASTER | NTF_SELF |
 *                NTF_EXT_LEARNED               NTF_EXT_LEARNED | NTF_STICKY
 *   ndm_state    NUD_REACHABLE                 NUD_REACHABLE | NUD_NOARP
 *   NDA_LLADDR   MAC                           MAC
 *   NDA_NH_ID    nhg_id, when set; then no NDA_DST
 *   NDA_DST      VTEP, 4 or 16 bytes, when there is one
 *   NDA_VLAN     vid, when set
 *   NDA_SRC_VNI  vni, when set
 */
extern size_t fpm_mac_encode(const struct fpm_remote_mac *in, void *buf,
			     size_t buflen);

#endif /* _FPM_MAC_H */
