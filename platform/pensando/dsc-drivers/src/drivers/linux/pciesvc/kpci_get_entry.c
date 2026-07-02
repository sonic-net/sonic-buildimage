#include "kpcimgr_api.h"

/*
 * For unity builds, all these functions are defined in other .c files
 * that are included before this file. We only need extern declarations
 * for symbols that are truly external (pciesvc_start/end markers and
 * version variables).
 *
 * For separate compilation, we include pciesvc_system_extern.h which
 * provides the correct prototypes.
 */
#include "pciesvc_system_extern.h"

/* Code boundary markers */
extern void pciesvc_start(void);
extern void pciesvc_end(void);

/* Version variables */
extern int pciesvc_version_major;
extern int pciesvc_version_minor;

static struct kpcimgr_entry_points_t ep;

struct kpcimgr_entry_points_t *kpci_get_entry_points(void)
{
	int i;

	/* initialize entry_points struct via executable code so that
	 * PC relative relocations are generated */
	ep.expected_mgr_version = 3;
	ep.lib_version_major = pciesvc_version_major;
	ep.lib_version_minor = pciesvc_version_minor;
	ep.code_start = (void *)pciesvc_start;
	ep.code_end = (void *)pciesvc_end;

	for (i=0; i<K_NUM_ENTRIES; i++)
		ep.entry_point[i] = kpcimgr_undefined_entry;

	ep.entry_point[K_ENTRY_INIT_INTR] = kpcimgr_init_intr;
	ep.entry_point[K_ENTRY_INIT_POLL] = kpcimgr_init_poll;
	ep.entry_point[K_ENTRY_SHUT] = pciesvc_shut;
	ep.entry_point[K_ENTRY_POLL] = kpcimgr_poll;
	ep.entry_point[K_ENTRY_HOLDING_PEN] = kpcimgr_get_holding_pen;
	ep.entry_point[K_ENTRY_INDIRECT_INTR] = kpcimgr_ind_intr;
	ep.entry_point[K_ENTRY_NOTIFY_INTR] = kpcimgr_not_intr;
	ep.entry_point[K_ENTRY_INIT_FN] = kpcimgr_init_fn;
	ep.entry_point[K_ENTRY_CMD_READ] = pciesvc_sysfs_cmd_read;
	ep.entry_point[K_ENTRY_CMD_WRITE] = pciesvc_sysfs_cmd_write;
	ep.entry_point[K_ENTRY_GET_VERSION] = kpcimgr_version_fn;

	return &ep;
}
