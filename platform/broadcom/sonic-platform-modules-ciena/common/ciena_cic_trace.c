#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include "generic_cic_dumper.h"
#define CREATE_TRACE_POINTS
#include "generic_cic_tp.h"

static LIST_HEAD(ciena_cic_dumper_list);
static DEFINE_MUTEX(ciena_cic_dumper_mutex);

/* panic when interrupts are not happening */
bool ciena_cic_panic_enable = false;

void ciena_cic_register_dumper(struct ciena_cic_dumper *ccd)
{
	mutex_lock(&ciena_cic_dumper_mutex);

	list_add(&ccd->lh, &ciena_cic_dumper_list);

	mutex_unlock(&ciena_cic_dumper_mutex);
}

void ciena_cic_deregister_dumper(struct ciena_cic_dumper *ccd)
{
	mutex_lock(&ciena_cic_dumper_mutex);

	list_del(&ccd->lh);

	mutex_unlock(&ciena_cic_dumper_mutex);
}

bool ciena_cic_dump(int irq)
{
	struct ciena_cic_dumper *ccd;
	struct list_head        *lh;
	bool                     do_panic = false;

	mutex_lock(&ciena_cic_dumper_mutex);

	list_for_each(lh, &ciena_cic_dumper_list) {

		ccd = container_of(lh, struct ciena_cic_dumper, lh);
		if (ccd->dump_fn && ccd->data)
			do_panic |= (ccd->dump_fn)(ccd->data, irq);
	}

	mutex_unlock(&ciena_cic_dumper_mutex);

	return do_panic;
}

EXPORT_SYMBOL_GPL(ciena_cic_register_dumper);
EXPORT_SYMBOL_GPL(ciena_cic_deregister_dumper);
EXPORT_SYMBOL_GPL(ciena_cic_dump);
EXPORT_SYMBOL_GPL(ciena_cic_panic_enable);

module_param_named(cic_panic_enable, ciena_cic_panic_enable,
		   bool, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(cic_panic_enable,
		 "missing cic interrupts trigger a kernel panic");

MODULE_DESCRIPTION("Cascading interrupt tracepoints");
MODULE_AUTHOR("Marc St-Amand <mstamand@ciena.com>");
MODULE_LICENSE("GPL v2");
