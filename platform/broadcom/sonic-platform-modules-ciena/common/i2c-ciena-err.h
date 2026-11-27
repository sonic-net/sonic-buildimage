#ifndef _I2C_CIENA_ERR_H
#define _I2C_CIENA_ERR_H

#include <linux/ktime.h>
#include <linux/timekeeping.h>

struct ciena_i2c_err_state {
	time64_t since;
	time64_t last_ok;
	time64_t last_err;
	int      adapter_id;
	int      endpt_addr;
	int      prev_error;
	unsigned ok_count;
	unsigned err_count;
};

#define CIENA_I2C_ERR_REPORT(dev, rc, state, fmt, args...)		\
	if (state) {							\
		(state)->last_err = ktime_get_seconds();		\
		(state)->err_count++;					\
		if (rc != (state)->prev_error) {			\
			if (!(state)->prev_error)			\
				(state)->since = (state)->last_err;	\
			(state)->prev_error = rc;			\
			if (0 <= (state)->adapter_id)			\
				dev_err(dev, "[%d-%04x rc=%d] " fmt,	\
					(state)->adapter_id,		\
					(state)->endpt_addr, rc,	\
					##args);			\
			else						\
				dev_err(dev, "[%04x rc=%d] " fmt,	\
					(state)->endpt_addr, rc,	\
					##args);			\
		}							\
	}

static inline void ciena_i2c_err_reset_rc(struct ciena_i2c_err_state *err,
					  int                         rc)
{
	if (err && !rc) {
		err->last_ok = ktime_get_seconds();
		err->ok_count++;
		if (err->prev_error || !err->since)
			err->since = err->last_ok;
		err->prev_error = rc;
	}
}

static inline void ciena_i2c_err_set_devaddr(struct ciena_i2c_err_state *err,
					     int                         addr)
{
	if (err) err->endpt_addr = addr;
}

static inline void ciena_i2c_err_set_state(struct ciena_i2c_err_state **parent,
					   struct ciena_i2c_err_state  *child)
{
	if (parent && (NULL == *parent))
		*parent = child;
}

static inline bool ciena_i2c_err_clr_state(struct ciena_i2c_err_state **parent,
					   struct ciena_i2c_err_state  *child)
{
	if (parent && (child == *parent)) {
		*parent = NULL;
		return true;
	}
	return false;
}
#endif
