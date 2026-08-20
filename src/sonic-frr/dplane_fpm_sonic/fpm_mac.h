// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * FPM wire-format helpers: decoding of MAC (AF_BRIDGE neighbour) messages and
 * encoding of EVPN FDB nexthops.
 *
 * Kept free of zebra state so it can be unit tested on its own, and so the
 * decode runs on the FPM thread while the caller applies the result on
 * zebra's main thread.
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

/* Hardware-learnt MAC marker, carried in NDA_PROTOCOL. Added by the SONiC
 * rtnetlink patch and repeated here so this module and its tests keep building
 * against unpatched kernel headers. */
#ifndef RTPROT_HW
#define RTPROT_HW 193
#endif

/* Present only on newer kernel headers. */
#ifndef NTF_STICKY
#define NTF_STICKY 0x40
#endif

struct fpm_local_mac {
	int ifindex;
	uint8_t mac[ETH_ALEN];
	uint16_t vid;
	uint32_t generation;
	uint8_t protocol;
	bool del;
	bool sticky;
};

/*
 * Decode an AF_BRIDGE RTM_NEWNEIGH/RTM_DELNEIGH message. Returns false and
 * leaves *out untouched when the message is not a usable local MAC.
 */
extern bool fpm_mac_decode(const struct nlmsghdr *hdr, struct fpm_local_mac *out);

/*
 * A MAC is stale when the FPM learnt it and the replay that just ended did not
 * refresh it. Every message fpmsyncd sends carries the current generation, not
 * only the replayed ones, so a refresh arriving while the sweep is held saves
 * the MAC by stamping it. Anything zebra learnt by other means is not ours to
 * remove.
 */
extern bool fpm_mac_is_stale(bool fpm_learned, uint32_t mac_generation,
			     uint32_t sweep_generation);

/* An Ethernet Segment cannot have more VTEPs than this; matches zebra's own
 * ES_VTEP_MAX_CNT, repeated here to keep this module free of zebra headers. */
#define FPM_FDB_NH_MAX_MEMBERS 10

/*
 * One EVPN FDB nexthop: either a single remote VTEP (family AF_INET/AF_INET6
 * with addr set) or a group naming other FDB nexthop ids (family AF_UNSPEC with
 * members set). A delete carries neither.
 */
struct fpm_fdb_nh {
	uint32_t id;
	uint8_t family;
	uint8_t addr[16];
	uint32_t member_cnt;
	const uint32_t *members;
	bool del;
};

/*
 * Encode into buf the message zebra would have given the kernel, so the
 * receiver decodes over FPM exactly what it decodes from the kernel today.
 * Returns the message length, or 0 if it does not fit or is not a usable
 * nexthop.
 */
extern size_t fpm_fdb_nh_encode(const struct fpm_fdb_nh *nh, void *buf,
				size_t buflen);

#endif /* _FPM_MAC_H */
