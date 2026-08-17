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

#endif /* _FPM_MAC_H */
