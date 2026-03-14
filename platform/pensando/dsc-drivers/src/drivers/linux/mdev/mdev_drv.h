#ifndef _MDEV_DRV_H
#define _MDEV_DRV_H

#include <linux/ioctl.h>

#define DRV_VERSION         "0.1"
#define DRV_DESCRIPTION     "Pensando mdev Driver"

/* XXX There is a bug in older versions of the mnet driver - it fails to call
 * cdev_del() on removal, leaving a landmine in the kobj_map. We can work around
 * the issue by making sure this module loads at the same point in the map.
 * Hence leaving the DRV_NAME as "mnet" and creating only a single device.
 *
 * Ideally this can be removed when we no longer support NDU from affected versions.
 */
#define MDEV_HACK

#ifdef MDEV_HACK
#define DRV_NAME            "mnet"
#define MDEV_CHAR_DEV_NAME  "pen-mnet"
#define NUM_MDEV_DEVICES    1		/* The parent device(s) */
#else
#define DRV_NAME            "mdev"
#define DRV_NAME_ALT        "mnet"
#define MDEV_CHAR_DEV_NAME  "pen-mdev"
#define NUM_MDEV_DEVICES    2		/* The parent device(s) */
#endif

#define MAX_MNET_DEVICES    32
#define MAX_MCRYPT_DEVICES  32
#define MDEV_NAME_LEN       32

struct mdev_create_req {
	uint64_t regs_pa;
	uint64_t drvcfg_pa;
	uint64_t msixcfg_pa;
	uint64_t doorbell_pa;
	uint64_t tstamp_pa;
	int is_uio_dev;
	char name[MDEV_NAME_LEN];
};

#define MDEV_CREATE_MNET 	_IOWR('Q', 11, struct mdev_create_req)
#define MDEV_DESTROY		_IOW('Q',  12, const char*)
#define MDEV_CREATE_MCRYPT 	_IOWR('Q', 13, struct mdev_create_req)

#endif /* _MDEV_DRV_H */
