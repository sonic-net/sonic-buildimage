// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Stubs for the zebra-daemon symbols fpm_nhg.c references, so the unit tests
 * can link the module against libfrr alone (no zebra daemon, no dataplane).
 */
#include "config.h"

#include "lib/zebra.h"
#include "lib/memory.h"
#include "lib/prefix.h"

#include "zebra/debug.h"
#include "zebra/zebra_vrf.h"
#include "zebra/zebra_srv6.h"

/*
 * fpm_nhg.c registers its MTYPEs in zebra's group; the group itself is
 * defined by the zebra daemon (zebra_rib.c), which is not linked here.
 */
DEFINE_MGROUP(ZEBRA, "zebra (unit test stub)");

/* FPM debug logging stays off in tests. */
unsigned long zebra_debug_fpm;

/*
 * fpm_nhg.c reads encap_src_addr when it encodes an SRv6 nexthop; the tests
 * carry no SRv6 nexthops, and the zeroed context is the truthful answer.
 */
static struct zebra_srv6 srv6_stub;

struct zebra_srv6 *zebra_srv6_get_default(void)
{
	return &srv6_stub;
}

/*
 * Only the `show fpm nhg-fib route` CLI helper calls these; the tests never
 * enter that path and no VRF exists here, so NULL is truthful.
 */
struct zebra_vrf *zebra_vrf_lookup_by_name(const char *name)
{
	return NULL;
}

struct zebra_vrf *zebra_vrf_lookup_by_id(vrf_id_t vrf_id)
{
	return NULL;
}
