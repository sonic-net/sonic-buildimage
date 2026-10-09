#ifndef _RESET_SYSFS_INCLUDED
#define _RESET_SYSFS_INCLUDED

#include <linux/of.h>
#include <linux/printk.h>

#define RESET_SYSFS_DRIVER_NAME "reset-sysfs"
#define RESET_SYSFS_MAX_NAMES   64

struct ciena_sysfs_reset {
	const char *reset_name;
	unsigned bit_offset;
	unsigned bit_flags;
	unsigned value;
};

struct ciena_sysfs_reset_pdata {
	const char *controller_name;
	/* list ends with a NULL resets->reset_name */
	const struct ciena_sysfs_reset *resets;
};

struct ciena_sysfs_reset_names {
	const char *nm[RESET_SYSFS_MAX_NAMES];
};

static inline
uint32_t ciena_sysfs_of_reset_names(struct device_node             *np,
				    struct ciena_sysfs_reset_names *names)
{
	struct ciena_sysfs_reset_names retnm = {};
	struct device_node            *dnode = NULL;
	struct of_phandle_args         args  = {};
	uint32_t                      *rnum  = &args.args[0];
	uint32_t                       max   = 0;
	int                            nrst;
	int                            rc;

	for_each_node_with_property(dnode, "reset-names") {
		nrst = of_count_phandle_with_args(dnode, "resets",
						  "#reset-cells");
		while (nrst--) {
			rc = of_parse_phandle_with_args(dnode, "resets",
							"#reset-cells",
							nrst, &args);

			/* Device tree properties called "reset-names"
			 * may not all relate to a reset controller.
			 * Be inclusive here, and quietly exclude.
			 */
			if (rc || (np != args.np) || (!args.args_count))
				continue;

			if (RESET_SYSFS_MAX_NAMES <= *rnum) {
				/* A jumbo reset controller out there
				 * expects a reset pin larger than 64.
				 * If/when this warning shows up, it
				 * means RESET_SYSFS_MAX_NAMES must be
				 * incremented.
				 */
				pr_warn("%s expects reset %s #%u\n",
					dnode->name, np->name, *rnum);
				continue;
			}

			rc = of_property_read_string_index(dnode,
							   "reset-names",
							   nrst,
							   &retnm.nm[*rnum]);

			if (!rc && (max <= *rnum)) max = 1 + *rnum;
		}
	}

	*names = retnm;
	return max;
};
#endif
