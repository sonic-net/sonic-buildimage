#include <linux/stddef.h>
#include "kpcimgr_api.h"

extern char pciesvc_end;
extern void kpcimgr_init_intr(void *);
extern void kpcimgr_init_fn(void *);
extern void kpcimgr_version_fn(char **);
extern void kpcimgr_init_poll(void *);
extern void pciesvc_shut(int);
extern void kpcimgr_poll(kstate_t *, int, int);
extern unsigned long kpcimgr_get_holding_pen(unsigned long, unsigned int);
extern int kpcimgr_ind_intr(void *, int);
extern int kpcimgr_not_intr(void *, int);
extern void kpcimgr_undefined_entry(void);
extern int pciesvc_sysfs_cmd_read(void *, char *, int *);
extern int pciesvc_sysfs_cmd_write(void *, char *, size_t, int *);

/* * Removed extern version ints to avoid ADRP relocation errors.
 * These are now pulled from the kstate context.
 */

struct kpcimgr_entry_points_t ep;

struct kpcimgr_entry_points_t *kpci_get_entry_points(void)
{
    kstate_t *ks = get_kstate();
    int i;

    /* CRITICAL FIX: If the global pointer is NULL, the driver will panic.
     * We must ensure kstate is valid before dereferencing. */
    if (!ks) {
        pr_err("pciesvc: kstate is NULL in kpci_get_entry_points! Aborting.\n");
        return NULL; 
    }

    ep.expected_mgr_version = 3;
    ep.lib_version_major = ks->pciesvc_version_major;
    ep.lib_version_minor = ks->pciesvc_version_minor;
    ep.code_end = &pciesvc_end;

    for (i=0; i<K_NUM_ENTRIES; i++)
        ep.entry_point[i] = (void *)kpcimgr_undefined_entry;

    ep.entry_point[K_ENTRY_INIT_INTR] = (void *)kpcimgr_init_intr;
    ep.entry_point[K_ENTRY_INIT_POLL] = (void *)kpcimgr_init_poll;
    ep.entry_point[K_ENTRY_SHUT] = (void *)pciesvc_shut;
    ep.entry_point[K_ENTRY_POLL] = (void *)kpcimgr_poll;
    ep.entry_point[K_ENTRY_HOLDING_PEN] = (void *)kpcimgr_get_holding_pen;
    ep.entry_point[K_ENTRY_INDIRECT_INTR] = (void *)kpcimgr_ind_intr;
    ep.entry_point[K_ENTRY_NOTIFY_INTR] = (void *)kpcimgr_not_intr;
    ep.entry_point[K_ENTRY_INIT_FN] = (void *)kpcimgr_init_fn;
    ep.entry_point[K_ENTRY_CMD_READ] = (void *)pciesvc_sysfs_cmd_read;
    ep.entry_point[K_ENTRY_CMD_WRITE] = (void *)pciesvc_sysfs_cmd_write;
    ep.entry_point[K_ENTRY_GET_VERSION] = (void *)kpcimgr_version_fn;

    return &ep;
}
