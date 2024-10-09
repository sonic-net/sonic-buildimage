#ifndef _LINUX_SFF_8436_H
#define _LINUX_SFF_8436_H

#include <linux/types.h>
#include <linux/memory.h>

/*
 * As seen through Linux I2C, differences between the most common types of I2C
 * memory include:
 * - How much memory is available (usually specified in bit)?
 * - What write page size does it support?
 * - Special flags (read_only, world readable...)?
 *
 * If you set up a custom eeprom type, please double-check the parameters.
 * Especially page_size needs extra care, as you risk data loss if your value
 * is bigger than what the chip actually supports!
 */

struct sff_8436_platform_data {
	u32		byte_len;		/* size (sum of all addr) */
	u16		page_size;		/* for writes */
	u8		flags;
#define SFF_8436_FLAG_READONLY	0x40	/* sysfs-entry will be read-only */
#define SFF_8436_FLAG_IRUGO		0x20	/* sysfs-entry will be world-readable */
#define SFF_8436_FLAG_TAKE8ADDR	0x10	/* take always 8 addresses (24c00) */

	//void		(*setup)(struct memory_accessor *, void *context);
	//void		*context;
};

#endif /* _LINUX_SFF_8436_H */
