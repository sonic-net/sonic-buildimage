#ifndef GENERIC_CIC_DUMPER_H
#define GENERIC_CIC_DUMPER_H

#include <linux/kernel.h>
#include <linux/types.h>

struct ciena_cic_dumper {
	struct list_head lh;
	bool (*dump_fn)(void *data, int irq);
	void *data;
};

extern bool ciena_cic_panic_enable;

void ciena_cic_register_dumper(struct ciena_cic_dumper *ccd);
void ciena_cic_deregister_dumper(struct ciena_cic_dumper *ccd);
bool ciena_cic_dump(int irq);

#define ciena_cic_panic(_fmt, _args...)				\
	do {							\
		if (ciena_cic_dump(0)) panic(_fmt, ##_args);	\
		else pr_info(_fmt, ##_args);			\
	} while (0);

#endif /* GENERIC_CIC_DUMPER_H */
