/*
 * This file is part of Cienaâs i2c adapter
 *
 * Copyright (C) 2021 Ciena Corporation
 * Author: Marc St-Amand <mstamand@ciena.com>
 *
 * i2c adapter is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 2 of the License.
 *
 * i2c adapter is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "i2c-ciena.h"
#include "i2c-ciena-err.h"
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/ratelimit.h>
#include <linux/regmap.h>
#include <linux/rtmutex.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/of_irq.h>
#ifdef CONFIG_CIENA_MCEE
#include <linux/ciena_mcee.h>
#endif

#include "generic_cic_dumper.h"

#define CREATE_TRACE_POINTS
#include "i2c-ciena-tp.h"

static unsigned int max_latency_us;
module_param(max_latency_us, uint, S_IRUGO);

/*----------------------------------------------------------------------------*/
#define CIENA_I2C_RESET_DELAY_MS       100 /* 80ms required by specification */
#define CIENA_I2C_FAST_RESET_DELAY_MS   10 /* not sure where 80ms came from  */
#define CIENA_I2C_POLL_DELAY_US      35000 /* based on SMBus max timeout     */
#define CIENA_I2C_IRQ_MIN_LATENCY_US 10000 /* based on our fussiest optics   */
#define CIENA_I2C_POLL_PERIOD_US        50 /* 20 kHz polling is good enough  */
#define CIENA_I2C_BUS_FREE_MIN_US       20 /* SFF8636 gap before a START     */
#define CIENA_I2C_T_WR_MS               40 /* Complete Single or Seq Write   */

#define CIENA_I2C_STATUS_STAT        (1 << 0)
#define CIENA_I2C_STATUS_ACKR        (1 << 1)
#define CIENA_I2C_STATUS_SCL         (1 << 2)
#define CIENA_I2C_STATUS_SDA         (1 << 3)
#define CIENA_I2C_STATUS_STATE_SHIFT 4
#define CIENA_I2C_STATUS_STATE_MASK  (0xf << CIENA_I2C_STATUS_STATE_SHIFT)

#define ciena_i2c_err(__dev, priv, rc, __fmt, __args...)		\
	{								\
		ciena_i2c_reg_t __stat;					\
		__stat = ciena_i2c_reg_rd(priv, STATUS);		\
		CIENA_I2C_ERR_REPORT(__dev, rc, *priv->err_state,	\
				     "[STAT=0x%x] "__fmt, __stat,	\
				     ## __args);			\
	}

#define ciena_i2c_dbg(__dev, __fmt, __args...)				\
	pr_devel_ratelimited("%s %s: " __fmt,				\
			     dev_driver_string(__dev),			\
			     dev_name(__dev), ## __args)


/*----------------------------------------------------------------------------*/
enum ciena_i2c_op {
	ciena_i2c_op_RESET    = (1 << 0),
	ciena_i2c_op_START    = (1 << 1),
	ciena_i2c_op_STOP     = (1 << 2),
	ciena_i2c_op_WRITE    = (1 << 3),
	ciena_i2c_op_READNACK = (1 << 4),
	ciena_i2c_op_READACK  = (1 << 5),
	ciena_i2c_op_MASK     = (1 << 6) - 1,
	ciena_i2c_op_WRITE_IGNORE_NAK = (1 << 8) | ciena_i2c_op_WRITE
};

/*----------------------------------------------------------------------------*/
enum ciena_i2c_irq {
	ciena_i2c_irq_COMPLETE,
	ciena_i2c_irq_MAX
};

/*----------------------------------------------------------------------------*/
enum ciena_i2c_register {
	ciena_i2c_register_CTRL,
	ciena_i2c_register_STATUS,
	ciena_i2c_register_WDATA,
	ciena_i2c_register_RDATA,
	ciena_i2c_register_ISTAT,
	ciena_i2c_register_IMASK,
	ciena_i2c_register_IF_SEL,
	ciena_i2c_register_HALF_PERIOD,
	ciena_i2c_register_MAX,
	ciena_i2c_register_INVALID,
};

/*----------------------------------------------------------------------------*/
static const unsigned int ciena_i2c_regmap_default[ciena_i2c_register_MAX] = {
	[ciena_i2c_register_CTRL]   	 = ciena_i2c_register_CTRL,
	[ciena_i2c_register_STATUS] 	 = ciena_i2c_register_STATUS,
	[ciena_i2c_register_WDATA]  	 = ciena_i2c_register_WDATA,
	[ciena_i2c_register_RDATA]  	 = ciena_i2c_register_RDATA,
	[ciena_i2c_register_ISTAT]  	 = ciena_i2c_register_ISTAT,
	[ciena_i2c_register_IMASK]  	 = ciena_i2c_register_IMASK,
	[ciena_i2c_register_IF_SEL]      = ciena_i2c_register_INVALID,
	[ciena_i2c_register_HALF_PERIOD] = ciena_i2c_register_INVALID,
};

/*----------------------------------------------------------------------------*/
static const unsigned int ciena_i2c_regmap_sw_sel[ciena_i2c_register_MAX] = {
	[ciena_i2c_register_CTRL]   	 = 2,
	[ciena_i2c_register_STATUS] 	 = 3,
	[ciena_i2c_register_WDATA]  	 = 4,
	[ciena_i2c_register_RDATA]  	 = 5,
	[ciena_i2c_register_ISTAT]  	 = 6,
	[ciena_i2c_register_IMASK]  	 = 7,
	[ciena_i2c_register_IF_SEL]      = 0,
	[ciena_i2c_register_HALF_PERIOD] = 1,
};

/*----------------------------------------------------------------------------*/
static const unsigned int ciena_i2c_regmap_sw_sel2[ciena_i2c_register_MAX] = {
	[ciena_i2c_register_CTRL]        = ciena_i2c_register_CTRL,
	[ciena_i2c_register_STATUS]      = ciena_i2c_register_STATUS,
	[ciena_i2c_register_WDATA]       = ciena_i2c_register_WDATA,
	[ciena_i2c_register_RDATA]       = ciena_i2c_register_RDATA,
	[ciena_i2c_register_ISTAT]       = ciena_i2c_register_ISTAT,
	[ciena_i2c_register_IMASK]       = ciena_i2c_register_INVALID,
	[ciena_i2c_register_IF_SEL]      = 5,
	[ciena_i2c_register_HALF_PERIOD] = 6,
};

/*----------------------------------------------------------------------------*/
enum ciena_i2c_wait_state {
	CIENA_I2C_WAIT_INIT,
	CIENA_I2C_WAITING,
	CIENA_I2C_POLLING,
	CIENA_I2C_WAIT_DONE,
};

/*----------------------------------------------------------------------------*/
struct ciena_i2c_private;
struct ciena_i2c_op_watch {
	struct ciena_i2c_private *priv;
	atomic_t                 wait_done;
	struct hrtimer           op_timer;
	struct kthread_work      op_work;
	struct rt_mutex          op_mutex;
};

/*----------------------------------------------------------------------------*/
typedef u32 ciena_i2c_reg_t;

struct ciena_i2c_private {
	void __iomem       *regs;
	struct regmap      *parent_regmap;
	unsigned int       reg_width;
	unsigned int       reg_offset;
	unsigned int       reg_gap;
	unsigned int       sw_if_sel;
	unsigned int       clock_freq;
	const unsigned int *reg_map;
	struct device      *dev;
	struct completion  op_complete;
	struct i2c_adapter adapter;
	bool               shared_io;
	bool               little_endian;
	bool               no_watch;
	bool               failed_stop;
	int                irq;
	unsigned int       i2c_timeout;
	unsigned int       missed_irq_count;
	int                pre_start_gap;
	ktime_t            pre_start_tick;
	ciena_i2c_reg_t   (*ior)(struct ciena_i2c_private *priv,
				 const void __iomem *addr);
	void              (*iow)(struct ciena_i2c_private *priv,
				 ciena_i2c_reg_t data, void __iomem *addr);

	struct ciena_i2c_op_watch  *prev_watch;
	struct ciena_i2c_op_watch  *next_watch;
	struct ciena_i2c_op_watch  op_watch[2];
	struct work_struct         nowatch_poll_work;
	struct ciena_i2c_err_state **err_state;
	struct ciena_i2c_err_state *local_err_state;
	struct ciena_i2c_err_state adap_err_state;
	struct work_struct         children_work;
#ifdef CONFIG_CIENA_MCEE
	struct ciena_mcee_dev       mceed;
#endif
};

/*----------------------------------------------------------------------------*/
static struct kthread_worker *ciena_i2c_watch;
static struct kthread_work    ciena_i2c_watch_init;

/*----------------------------------------------------------------------------*/
#define __ciena_i2c_ior(_w, _e)						\
	static								\
	ciena_i2c_reg_t __ciena_i2c_ior ## _w ## _e			\
	(struct ciena_i2c_private *priv,				\
	 const void __iomem *addr)					\
	{								\
		return (ciena_i2c_reg_t) ioread ## _w ## _e (addr);	\
	}

__ciena_i2c_ior(8,)
__ciena_i2c_ior(16,)
__ciena_i2c_ior(16, be)
__ciena_i2c_ior(32,)
__ciena_i2c_ior(32, be)

/*----------------------------------------------------------------------------*/
#define __ciena_i2c_iow(_w, _e)						\
	static								\
	void __ciena_i2c_iow ## _w ## _e				\
	(struct ciena_i2c_private *priv,				\
	 ciena_i2c_reg_t data,						\
	 void __iomem *addr)						\
	{								\
		iowrite ## _w ## _e ((u ## _w) data, addr);		\
	}

__ciena_i2c_iow(8,)
__ciena_i2c_iow(16,)
__ciena_i2c_iow(16, be)
__ciena_i2c_iow(32,)
__ciena_i2c_iow(32, be)

/*----------------------------------------------------------------------------*/
static ciena_i2c_reg_t __ciena_i2c_rd_regmap(struct ciena_i2c_private *priv,
					     const void __iomem *addr)
{
	unsigned int val = ~0;
	int rc = regmap_read(priv->parent_regmap, addr - (void *) NULL, &val);

	if (rc) dev_dbg(priv->dev, "%s failed addr=%px rc=%d\n",
			__func__, addr, rc);

	return (ciena_i2c_reg_t) val;
}

/*----------------------------------------------------------------------------*/
static void __ciena_i2c_wr_regmap(struct ciena_i2c_private *priv,
				  ciena_i2c_reg_t data, void __iomem *addr)
{
	int rc = regmap_write(priv->parent_regmap, addr - (void *) NULL, data);

	if (rc) dev_dbg(priv->dev, "%s failed data=0x%x addr=%px rc=%d\n",
			__func__, data, addr, rc);
}

/*----------------------------------------------------------------------------*/
static inline int ciena_i2c_id(struct ciena_i2c_private *priv)
{
	if (priv->err_state && *priv->err_state)
		return (*priv->err_state)->adapter_id;

	return i2c_adapter_id(&priv->adapter);
}

/*----------------------------------------------------------------------------*/
static ciena_i2c_reg_t __ciena_i2c_reg_rd(struct ciena_i2c_private *priv,
					  enum ciena_i2c_register reg)
{
	unsigned int off = priv->reg_map[reg];
	void __iomem *addr = priv->regs + off * priv->reg_width * priv->reg_gap;
	ciena_i2c_reg_t data = 0;

	BUG_ON(ciena_i2c_register_INVALID == off);

	data = (*priv->ior)(priv, addr);

	trace_ciena_i2c_reg_rd(ciena_i2c_id(priv), reg, data);

	return data;
}

#define ciena_i2c_reg_rd(_priv, _reg)\
	__ciena_i2c_reg_rd(_priv, ciena_i2c_register_##_reg)

/*----------------------------------------------------------------------------*/
static void __ciena_i2c_reg_wr(struct ciena_i2c_private *priv,
			       enum ciena_i2c_register reg,
			       ciena_i2c_reg_t data)
{
	unsigned int off = priv->reg_map[reg];
	void __iomem *addr = priv->regs + off * priv->reg_width * priv->reg_gap;

	BUG_ON(ciena_i2c_register_INVALID == off);

	(*priv->iow)(priv, data, addr);

	trace_ciena_i2c_reg_wr(ciena_i2c_id(priv), reg, data);
}

#define ciena_i2c_reg_wr(_priv, _reg, data)\
	__ciena_i2c_reg_wr(_priv, ciena_i2c_register_##_reg, data)

/*----------------------------------------------------------------------------*/
static inline bool ciena_i2c_status_complete(struct ciena_i2c_private *priv)
{
	bool complete = ((ciena_i2c_reg_rd(priv, ISTAT) &
			  (1 << ciena_i2c_irq_COMPLETE)) != 0);

	if (complete)
		ciena_i2c_reg_wr(priv, ISTAT, 1 << ciena_i2c_irq_COMPLETE);

	return complete;
}

/*----------------------------------------------------------------------------*/
static inline bool ciena_i2c_status_ack(struct ciena_i2c_private *priv)
{
	return (ciena_i2c_reg_rd(priv, STATUS) & CIENA_I2C_STATUS_ACKR) != 0;
}

/*----------------------------------------------------------------------------*/
static inline bool ciena_i2c_status_ready(struct ciena_i2c_private *priv)
{
	return (ciena_i2c_reg_rd(priv, STATUS) & CIENA_I2C_STATUS_STAT) == 0;
}

/*----------------------------------------------------------------------------*/
static inline bool ciena_i2c_status_idle(struct ciena_i2c_private *priv)
{
	/* For happy transfers, the controller state must be 'IDLE'
	 * (i.e. 0), with both SCL and SDA high.
	 */
	ciena_i2c_reg_t status = ciena_i2c_reg_rd(priv, STATUS);

	return (((status & CIENA_I2C_STATUS_STATE_MASK) == 0) &&
		((status & CIENA_I2C_STATUS_SDA) == CIENA_I2C_STATUS_SDA) &&
		((status & CIENA_I2C_STATUS_SCL) == CIENA_I2C_STATUS_SCL));
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_start_watch_timer(struct ciena_i2c_private *priv,
					struct ciena_i2c_op_watch *watch,
					unsigned int timeout_us)
{
	ktime_t ktime_to_wait = ktime_set(0, timeout_us * 1000);

	hrtimer_start(&watch->op_timer, ktime_to_wait, HRTIMER_MODE_REL);
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_restart_watch(struct ciena_i2c_private *priv)
{
	struct ciena_i2c_op_watch *next_watch = &priv->op_watch[0];

	if (priv->no_watch) return;

	priv->prev_watch = NULL;
	priv->next_watch = next_watch;

	atomic_set(&next_watch->wait_done, CIENA_I2C_WAIT_INIT);

	rt_mutex_lock(&next_watch->op_mutex);
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_stop_watch(struct ciena_i2c_private *priv,
				 struct ciena_i2c_op_watch *watch)
{
	enum ciena_i2c_wait_state wait_done;

	if (priv->no_watch) return;

	if (watch) {
		wait_done = atomic_xchg(&watch->wait_done,
					CIENA_I2C_WAIT_INIT);

		rt_mutex_unlock(&watch->op_mutex);

		switch (wait_done) {
		case CIENA_I2C_POLLING:
			hrtimer_cancel(&watch->op_timer);
			break;
		case CIENA_I2C_WAIT_DONE:
			hrtimer_cancel(&watch->op_timer);
			kthread_cancel_work_sync(&watch->op_work);
			break;
		default:
			break;
		}
	}
}

/*----------------------------------------------------------------------------*/
static irqreturn_t ciena_i2c_irq_handler(int irq, void *data)
{
	unsigned int tmo_us = CIENA_I2C_IRQ_MIN_LATENCY_US / 2;
	struct ciena_i2c_private *priv = data;
	struct ciena_i2c_op_watch *op_watch;
	enum ciena_i2c_wait_state wait_done;

	op_watch = priv->next_watch;

	if (ciena_i2c_status_complete(priv) && op_watch) {
		wait_done = atomic_cmpxchg(&op_watch->wait_done,
					   CIENA_I2C_WAITING,
					   CIENA_I2C_WAIT_DONE);

		if (CIENA_I2C_WAITING == wait_done) {
			ciena_i2c_start_watch_timer(priv, op_watch, tmo_us);
			complete(&priv->op_complete);
		}

		trace_ciena_i2c_irq(ciena_i2c_id(priv), wait_done);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

/*----------------------------------------------------------------------------*/
static irqreturn_t ciena_i2c_irq_handler_nowatch(int irq, void *data)
{
	struct ciena_i2c_private *priv = data;

	if (ciena_i2c_status_complete(priv)) {
		complete(&priv->op_complete);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

/*----------------------------------------------------------------------------*/
#define CIENA_I2C_MAX_MISS 10
static int ciena_i2c_wait_irq(struct ciena_i2c_private *priv)
{
	/* This is a coarse timer, add a one-tick wiggle room. */
	unsigned long timeout = 1 + usecs_to_jiffies(priv->i2c_timeout);
	int rc;

	/* the transfer is now out of reach from the previous
	 * interrupt: stop the previous watch */
	ciena_i2c_stop_watch(priv, priv->prev_watch);

	rc = wait_for_completion_timeout(&priv->op_complete, timeout);

	if (0 == rc) {
		if (ciena_i2c_status_complete(priv)) {
			/* the FPGA indicates that the interrupt
			 * should have fired, yet it never did */
			ciena_i2c_err(priv->dev, priv, -ETIMEDOUT,
				      "missed completion interrupt\n");

			priv->missed_irq_count++;
			/* dump the cic state on the first interrupt miss */
			if (1 == priv->missed_irq_count)
				ciena_cic_dump(priv->irq);
			/* ten strikes: we are out, something is broken */
			else if (CIENA_I2C_MAX_MISS == priv->missed_irq_count)
				ciena_cic_panic("%s: missed %u completion "
						"interrupts in a row\n",
						dev_name(priv->dev),
						priv->missed_irq_count);
		}
		else {
			/* everything on the FPGA side agrees that the
			 * interrupt did not happen: the cic driver
			 * cannot be blamed */
			priv->missed_irq_count = 0;
			dev_dbg(priv->dev, "IRQ wait timed out\n");
			return -ETIMEDOUT;
		}
	}
	else priv->missed_irq_count = 0;

	return 0;
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_wait(struct ciena_i2c_private *priv)
{
	s64 usecs_since_irq;
	ktime_t irq_tick;
	int rc;

	irq_tick = ktime_get_raw();

	if (priv->no_watch && !priv->irq)
		queue_work(system_long_wq, &priv->nowatch_poll_work);

	rc = ciena_i2c_wait_irq(priv);

	if (rc != 0) {
		if (priv->no_watch && !priv->irq)
			cancel_work_sync(&priv->nowatch_poll_work);
		return rc;
	}

	if (!ciena_i2c_status_ready(priv))
		return -EIO;

	/* Latency measurements are useless on easy-going controllers. */
	if (priv->no_watch) return 0;

	usecs_since_irq = ktime_us_delta(ktime_get_raw(), irq_tick);

	if (max_latency_us < usecs_since_irq)
		max_latency_us = usecs_since_irq;

	if (CIENA_I2C_IRQ_MIN_LATENCY_US < usecs_since_irq) {
		trace_ciena_i2c_latency(ciena_i2c_id(priv), usecs_since_irq);
		dev_dbg(priv->dev, "IRQ latency: %lld microseconds\n",
			usecs_since_irq);
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_watch_work_expire(struct kthread_work *work)
{
	struct ciena_i2c_op_watch *op_watch;
	struct ciena_i2c_private *priv;

	op_watch = container_of(work, struct ciena_i2c_op_watch, op_work);
	priv = op_watch->priv;

	trace_ciena_i2c_prio_bump(ciena_i2c_id(priv),
				  atomic_read(&op_watch->wait_done));

	/* grab and release the lock: this will bump the i2c transfer
	 * to RT priority and expedite the next operation */
	rt_mutex_lock(&op_watch->op_mutex);
	rt_mutex_unlock(&op_watch->op_mutex);

	trace_ciena_i2c_prio_bump(ciena_i2c_id(priv), CIENA_I2C_WAIT_INIT);
}

/*----------------------------------------------------------------------------*/
static bool ciena_i2c_watch_timer_poll(struct ciena_i2c_private *priv,
				       struct ciena_i2c_op_watch *watch)
{
	unsigned int watch_us = CIENA_I2C_IRQ_MIN_LATENCY_US / 2;
	unsigned int poll_us = CIENA_I2C_POLL_PERIOD_US;
	enum ciena_i2c_wait_state wait_done;
	ktime_t kttw = ktime_set(0, poll_us * 1000);

	if (ciena_i2c_status_complete(priv)) {
		wait_done = atomic_cmpxchg(&watch->wait_done,
					   CIENA_I2C_POLLING,
					   CIENA_I2C_WAIT_DONE);
		if (CIENA_I2C_POLLING == wait_done) {
			kttw = ktime_set(0, watch_us * 1000);
			hrtimer_forward_now(&watch->op_timer, kttw);
			complete(&priv->op_complete);
			return true;
		}
		else return false;
	}

	hrtimer_forward_now(&watch->op_timer, kttw);

	return true;
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_nowatch_poll(struct work_struct *work)
{
	struct ciena_i2c_private *priv;

	priv = container_of(work, struct ciena_i2c_private, nowatch_poll_work);

	usleep_range(CIENA_I2C_POLL_PERIOD_US, CIENA_I2C_POLL_PERIOD_US * 2);

	if (ciena_i2c_status_complete(priv)) {
		complete(&priv->op_complete);
		return;
	}
	queue_work(system_long_wq, &priv->nowatch_poll_work);
}

/*----------------------------------------------------------------------------*/
static enum hrtimer_restart ciena_i2c_watch_timer_expire(struct hrtimer *timer)
{
	struct ciena_i2c_private *priv;
	struct ciena_i2c_op_watch *op_watch;

	op_watch = container_of(timer, struct ciena_i2c_op_watch, op_timer);
	priv = op_watch->priv;

	switch (atomic_read(&op_watch->wait_done)) {
	case CIENA_I2C_POLLING:
		if (ciena_i2c_watch_timer_poll(priv, op_watch))
			return HRTIMER_RESTART;
		break;
	case CIENA_I2C_WAIT_DONE:
		kthread_queue_work(ciena_i2c_watch, &op_watch->op_work);
		break;
	default:
		break;
	}

	return HRTIMER_NORESTART;
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_prepare_to_wait(struct ciena_i2c_private *priv)
{
	struct ciena_i2c_op_watch *next_watch;
	struct ciena_i2c_op_watch *prev_watch;
	int next_watch_index = 0;

	ciena_i2c_reg_wr(priv, ISTAT, 1 << ciena_i2c_irq_COMPLETE);

	reinit_completion(&priv->op_complete);

	if (priv->no_watch) return;

	if (priv->next_watch == &priv->op_watch[0]) next_watch_index = 1;

	prev_watch = priv->prev_watch = priv->next_watch;
	next_watch = priv->next_watch = &priv->op_watch[next_watch_index];

	kthread_init_work(&next_watch->op_work, ciena_i2c_watch_work_expire);

	hrtimer_init(&next_watch->op_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	next_watch->op_timer.function = ciena_i2c_watch_timer_expire;

	if (!priv->irq) {
		atomic_set(&next_watch->wait_done, CIENA_I2C_POLLING);
		/* Watch the interrupt status every 50 microseconds. */
		ciena_i2c_start_watch_timer(priv, next_watch,
					    CIENA_I2C_POLL_PERIOD_US);
	}
	else atomic_set(&next_watch->wait_done, CIENA_I2C_WAITING);

	rt_mutex_lock(&next_watch->op_mutex);
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_init(struct ciena_i2c_private *priv)
{
	if (priv->sw_if_sel) {
		ciena_i2c_reg_wr(priv, IF_SEL, (0 != priv->sw_if_sel));

		dev_dbg(priv->dev, "IF_SEL is 0x%x (should be 0x%x)\n",
			ciena_i2c_reg_rd(priv, IF_SEL), priv->sw_if_sel);
	}

	if (priv->clock_freq) {
		uint32_t half_period_nsecs;
		uint32_t cycles_of_100MHz;

		half_period_nsecs = 500000000 / priv->clock_freq;
		cycles_of_100MHz = half_period_nsecs / 10;

		ciena_i2c_reg_wr(priv, HALF_PERIOD, cycles_of_100MHz);

		dev_dbg(priv->dev, "HALF_PERIOD is 0x%x (should be 0x%x)\n",
			ciena_i2c_reg_rd(priv, HALF_PERIOD), cycles_of_100MHz);
	}
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_reset(struct ciena_i2c_private *priv, bool slow)
{
	ciena_i2c_reg_wr(priv, CTRL, ciena_i2c_op_RESET);
	if (slow) mdelay(CIENA_I2C_RESET_DELAY_MS);
	else mdelay(CIENA_I2C_FAST_RESET_DELAY_MS);
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_poll_error(struct ciena_i2c_private *priv, int rc)
{
	switch (rc) {
	case -ECOMM:
		ciena_i2c_dbg(priv->dev, "NACK on write\n");
		break;
	case -EIO:
		ciena_i2c_err(priv->dev, priv, rc, "not ready after wait\n");
		break;
	case -ETIMEDOUT:
		ciena_i2c_err(priv->dev, priv, rc, "wait timeout on complete\n");
		break;
	case -ERESTARTSYS:
		ciena_i2c_err(priv->dev, priv, rc, "interrupted\n");
		break;
#ifdef CONFIG_CIENA_MCEE
	case -ENXIO:
		ciena_i2c_err(priv->dev, priv, rc, "controller disappeared\n");
		break;
#endif
	default:
		break;
	}
}


/*----------------------------------------------------------------------------*/
static int ciena_i2c_op(struct ciena_i2c_private *priv,
			enum ciena_i2c_op op,
			unsigned int retry)
{
	int rc = 0;

	if (op == ciena_i2c_op_RESET) {
		rc = -EINVAL;
		goto done;
	}

	while (1) {
		ciena_i2c_prepare_to_wait(priv);

		ciena_i2c_reg_wr(priv, CTRL, (op & ciena_i2c_op_MASK));

		rc = ciena_i2c_wait(priv);
		if (rc != 0) goto retry_op;

		if (op == ciena_i2c_op_WRITE && !ciena_i2c_status_ack(priv))
			rc = -ECOMM;

		if (rc == 0)
			goto done;

retry_op:
		/* the i2c beat is lost: stop watching */
		ciena_i2c_stop_watch(priv, priv->next_watch);
		ciena_i2c_restart_watch(priv);

#ifdef CONFIG_CIENA_MCEE
		if (ciena_mcee_dev_is_stale(&priv->mceed)) {
			rc = -ENXIO;
			break;
		}
#endif
		if (retry == 0)
			break;

		ciena_i2c_reset(priv, true);
		--retry;
	}

	if (rc) ciena_i2c_poll_error(priv, rc);

done:
	trace_ciena_i2c_op(ciena_i2c_id(priv), op, retry, rc);

	return rc;
}

/*----------------------------------------------------------------------------*/
static inline int ciena_i2c_start(struct ciena_i2c_private *priv)
{
	s64 usecs_since_stop = ktime_us_delta(ktime_get_raw(),
					      priv->pre_start_tick);
	s64 gap = priv->pre_start_gap - usecs_since_stop;

	if (0 < gap) {
		usleep_range(gap, priv->pre_start_gap);
		trace_ciena_i2c_pre_start(ciena_i2c_id(priv), gap);
	}

	return ciena_i2c_op(priv, ciena_i2c_op_START, 1);
}

/*----------------------------------------------------------------------------*/
static inline int ciena_i2c_stop(struct ciena_i2c_private *priv)
{
	int rc = ciena_i2c_op(priv, ciena_i2c_op_STOP, 0);
	priv->pre_start_tick = ktime_get_raw();

	if (rc) priv->failed_stop = true;

	return rc;
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_rd(struct ciena_i2c_private *priv, struct i2c_msg *msg)
{
	enum ciena_i2c_op op;
	u8               *data = msg->buf;
	int               rc = 0;

	if (msg->len < 1) {
		rc = -EINVAL;
		goto done;
	}

	/* Read data bytes. */
	for (; data < &msg->buf[msg->len]; ++data) {
		if (data + 1 == &msg->buf[msg->len])
		{
			if ((data == msg->buf) &&
			    (msg->flags & I2C_M_RECV_LEN) != 0)
				op = ciena_i2c_op_READACK;
			else
				op = ciena_i2c_op_READNACK;
		}
		else
			op = ciena_i2c_op_READACK;

		rc = ciena_i2c_op(priv, op, 0);
		if (rc != 0) {
			ciena_i2c_err(priv->dev, priv, rc,
				      "failed to read byte %zu from "
				      "device 0x%x\n",
				      (data - msg->buf), msg->addr);
			goto done;
		}
		*data = ciena_i2c_reg_rd(priv, RDATA);

		if (data == msg->buf && (msg->flags & I2C_M_RECV_LEN) != 0) {
			if (unlikely(msg->flags & I2C_M_DMA_SAFE &&
				*data > I2C_SMBUS_BLOCK_MAX)) {
				rc = -EPROTO;
				ciena_i2c_err(priv->dev, priv, rc,
					"device 0x%x recv length (0x%x) would overflow buffer.\n",
					msg->addr, *data);
				goto done;
			}
			msg->len += *data;
		}

		dev_dbg(priv->dev, "read %zu: 0x%02x\n",
			(data - msg->buf), *data);
	}

done:
	trace_ciena_i2c_msg_rd(ciena_i2c_id(priv), msg, rc, (data - msg->buf));

	return rc;
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_wr(struct ciena_i2c_private *priv, struct i2c_msg *msg)
{
	const u8 *data = msg->buf;
	int      rc = 0;

	enum ciena_i2c_op op = (msg->flags & I2C_M_IGNORE_NAK)
	             ? ciena_i2c_op_WRITE_IGNORE_NAK : ciena_i2c_op_WRITE;

	if (msg->len < 1) {
		rc = -EINVAL;
		goto done;
	}

	/* Write data bytes. */
	for (; data < &msg->buf[msg->len]; ++data) {
		ciena_i2c_reg_wr(priv, WDATA, *data);

		rc = ciena_i2c_op(priv, op, 0);
		if (rc != 0) {
			ciena_i2c_err(priv->dev, priv, rc,
				      "failed to write byte %zu(0x%x) "
				      "to device 0x%x [op=%x] [flags=%x]\n",
				      (data - msg->buf), *data, msg->addr, op,
				      msg->flags);
			goto done;
		}

		dev_dbg(priv->dev, "wrote %zu: 0x%02x\n",
			(data - msg->buf), *data);
	}

done:
	trace_ciena_i2c_msg_wr(ciena_i2c_id(priv), msg, rc, (data - msg->buf));

	return rc;
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_acknowledge_polling(struct ciena_i2c_private *priv,
			unsigned int dev_addr)
{
	int             rc = 0;
	unsigned int    count;

	for (count = 0; count < CIENA_I2C_T_WR_MS; ++count) {
		if (signal_pending_state(TASK_INTERRUPTIBLE, current)) {
			rc = -ERESTARTSYS;
			break;
		}

		do {
			rc = ciena_i2c_start(priv);
			if (rc != 0) {
				ciena_i2c_err(priv->dev, priv, rc,
					      "failed to start\n");
				break;  /* exit do..while 0 loop */
			}

			/* Write the device address byte */
			ciena_i2c_reg_wr(priv, WDATA, dev_addr);

			ciena_i2c_prepare_to_wait(priv);

			ciena_i2c_reg_wr(priv, CTRL, ciena_i2c_op_WRITE);

			rc = ciena_i2c_wait(priv);
			if (rc != 0)
				break;  /* exit do..while 0 loop */

			if (!ciena_i2c_status_ack(priv)) {
				rc = -ECOMM;
				break;  /* exit do..while 0 loop */
			}
		} while (0);

		if (rc == 0)
			goto done;

		if (ciena_i2c_stop(priv))
			break;

		/* the STOP does not need to be watched */
		ciena_i2c_stop_watch(priv, priv->next_watch);
		msleep(1);
		ciena_i2c_restart_watch(priv);
	}

	/* the i2c beat is lost: stop watching */
	ciena_i2c_stop_watch(priv, priv->next_watch);
	ciena_i2c_restart_watch(priv);

	ciena_i2c_poll_error(priv, rc);

	ciena_i2c_dbg(priv->dev, "failed acknowledge polling [%d][0x%x]\n",
		      rc, ciena_i2c_reg_rd(priv, STATUS));
done:
	trace_ciena_i2c_ack_poll(ciena_i2c_id(priv), dev_addr, rc);

	return rc;
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_master_xfer(struct i2c_adapter *adap,
				 struct i2c_msg *msgs,
				 int num)
{
	struct ciena_i2c_private *priv = i2c_get_adapdata(adap);
	struct i2c_msg           *msg;
	uint32_t                  rd;
	int                       rc = 0;
	unsigned int              dev_addr;

	if (NULL == priv) {
		pr_err("%s: transfer attempt on deactivated adapter %s\n",
		       __func__, adap->name);
		return -EIO;
	}

	trace_ciena_i2c_xfer_entry(ciena_i2c_id(priv), msgs, num);

	ciena_i2c_err_set_state(priv->err_state, &priv->adap_err_state);

	if (priv->failed_stop || !ciena_i2c_status_idle(priv)) {
		priv->failed_stop = false;
		ciena_i2c_reset(priv, false);
		if (!ciena_i2c_status_idle(priv)) {
			ciena_i2c_err(priv->dev, priv, -EBUSY,
				      "controller unready after reset\n");
			rc = -EBUSY;
			goto done;
		}
	}

	ciena_i2c_restart_watch(priv);

	for (msg = msgs; msg < &msgs[num]; ++msg) {

		ciena_i2c_err_set_devaddr(*(priv->err_state), msg->addr);

		rd = (msg->flags & I2C_M_RD) ? 1 : 0 ;

		if (msg->flags & I2C_M_REV_DIR_ADDR)
			dev_addr = (msg->addr << 1) | !rd;
		else
			dev_addr = (msg->addr << 1) | rd;

		dev_dbg(priv->dev,
			"%s %u bytes to/from 0x%02x [flags:%x] - %zu of %d messages\n",
			rd ? "read" : "write", msg->len, msg->addr, msg->flags,
			(msg - msgs) + 1, num);

		if (!(msg->flags & I2C_M_NOSTART)) {
			rc = ciena_i2c_acknowledge_polling(priv, dev_addr);
			if (rc != 0)
				goto done;
		}

		if (rd)
			rc = ciena_i2c_rd(priv, msg);
		else
			rc = ciena_i2c_wr(priv, msg);

		if (rc != 0) {
			ciena_i2c_stop(priv);
			goto done;
		}

		if (msg != &msgs[num-1]) {
			if (msg->flags & I2C_M_STOP) {
				rc = ciena_i2c_stop(priv);
				if (rc != 0)
					goto done;
			}
			else priv->pre_start_tick = ktime_get_raw();
		}
	}

	rc = ciena_i2c_stop(priv);
done:
	ciena_i2c_stop_watch(priv, priv->next_watch);

	trace_ciena_i2c_xfer_exit(ciena_i2c_id(priv), msgs, num, rc);

	ciena_i2c_err_reset_rc(*priv->err_state, rc);
	ciena_i2c_err_clr_state(priv->err_state, &priv->adap_err_state);

	if (rc != 0)
		return rc;
	return num;
}

/*----------------------------------------------------------------------------*/
static u32 ciena_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C |
		I2C_FUNC_SMBUS_EMUL |

		/* Following need support for the I2C_M_RECV_LEN msg flag. */
		I2C_FUNC_SMBUS_READ_BLOCK_DATA |
		I2C_FUNC_SMBUS_BLOCK_PROC_CALL |

		/* Following need support for I2C_M_NO_START and I2C_M_IGNORE_NAK. */
		I2C_FUNC_NOSTART |
		I2C_FUNC_PROTOCOL_MANGLING;
}

/*----------------------------------------------------------------------------*/
static const struct i2c_algorithm ciena_i2c_algorithm = {
	.master_xfer   = ciena_i2c_master_xfer,
	.functionality = ciena_i2c_functionality,
};

/*----------------------------------------------------------------------------*/
static int ciena_i2c_get_irq(struct platform_device      *pd,
			     struct ciena_i2c_private    *priv,
			     const struct ciena_i2c_info *info)
{
	struct resource  *res = platform_get_resource(pd, IORESOURCE_IRQ, 0);
	struct device    *dev = &pd->dev;
	irq_handler_t     irqh;
	int               irq;
	int               rc;

	if (res) {
		irq = res->start;
	}
	else if (dev->of_node &&
		 of_find_property(dev->of_node, "interrupts", NULL)) {
		irq = of_irq_get(dev->of_node, 0);
		if (0 < irq) dev_info(dev, "using OF interrupt %d\n", irq);
		else return irq ? irq : -EPROBE_DEFER;
	} else return 0;

	if (priv->no_watch) irqh = ciena_i2c_irq_handler_nowatch;
	else irqh = ciena_i2c_irq_handler;

	if (priv->parent_regmap)
		rc = request_threaded_irq(irq, NULL, irqh, IRQF_ONESHOT,
					  dev_name(dev), priv);
	else rc = request_irq(irq, irqh, IRQF_SHARED, dev_name(dev), priv);

	if (rc != 0) {
		dev_err(dev, "failed to request IRQ %d [%d]\n",
			(int)irq, rc);
		return rc;
	}

	priv->irq = irq;
	priv->pre_start_gap = CIENA_I2C_BUS_FREE_MIN_US;

	return 0;
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_parse_of(struct device *dev,
			      struct ciena_i2c_private *priv)
{
	struct device_node *node = dev->of_node;
	const u32          *prop;
	int                len;

	if (!node)
		return 0;

	prop = of_get_property(node, "ciena,shared-io", NULL);
	if (prop)
		priv->shared_io = true;

	prop = of_get_property(node, "ciena,little-endian", NULL);
	if (prop)
		priv->little_endian = true;

	prop = of_get_property(node, "ciena,no-watch", NULL);
	if (prop)
		priv->no_watch = true;

	prop = of_get_property(node, "cell-index", &len);
	if (prop) {
		if (len != sizeof(*prop)) {
			dev_err(dev, "invalid cell-index for '%s'\n",
				of_node_full_name(node));
			return -EINVAL;
		}
		priv->adapter.nr = be32_to_cpup(prop);
	}

	prop = of_get_property(node, "clock-frequency", &len);
	if (prop)
		priv->clock_freq = be32_to_cpup(prop);

	prop = of_get_property(node, "ciena,sw-if-sel", &len);
	if (prop)
		priv->sw_if_sel = 1;

	prop = of_get_property(node, "ciena,sw-if-sel2", &len);
	if (prop)
		priv->sw_if_sel = 2;

	if (priv->reg_width != 0)
		return 0;

	prop = of_get_property(node, "ciena,8-bit", NULL);
	if (prop) {
		priv->reg_width = sizeof(u8);
		return 0;
	}

	prop = of_get_property(node, "ciena,16-bit", NULL);
	if (prop) {
		priv->reg_width = sizeof(u16);
		return 0;
	}

	prop = of_get_property(node, "ciena,32-bit", NULL);
	if (prop) {
		priv->reg_width = sizeof(u32);
		return 0;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_setup_regmap(struct ciena_i2c_private *priv)
{
	struct device          *dev  = priv->dev;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource        *regs;
	struct resource        adj_regs;

	priv->ior = __ciena_i2c_rd_regmap;
	priv->iow = __ciena_i2c_wr_regmap;

	regs = platform_get_resource(pdev, IORESOURCE_REG, 0);
	if (!regs) {
		dev_err(dev, "missing register resource\n");
		return -ENXIO;
	}

	/* adjust the start of the resource based on the actual
	 * register offset */
	adj_regs = *regs;
	adj_regs.start += priv->reg_width * priv->reg_offset;

	dev_dbg(dev, "%pR from parent\n%pR adjusted\n", regs, &adj_regs);

	priv->regs = ((void *) NULL) + adj_regs.start;

	/* SCL deadlines require register access at interrupt
	 * level. This is not compatible with the regmap API. */
	if (!priv->no_watch)
		dev_warn(dev, "regmap with high-res watch: expect deadlocks");

	return 0;
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_setup_mmio(struct ciena_i2c_private *priv)
{
	struct device          *dev  = priv->dev;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource        *mem;
	struct resource        adj_mem;
	void                   *region;

	/* Assign the register IO routines. */
	switch (priv->reg_width) {
	case sizeof(u8):
		priv->ior = __ciena_i2c_ior8;
		priv->iow = __ciena_i2c_iow8;
		break;
	case sizeof(u16):
		if (priv->little_endian) {
			priv->ior = __ciena_i2c_ior16;
			priv->iow = __ciena_i2c_iow16;
		}
		else {
			priv->ior = __ciena_i2c_ior16be;
			priv->iow = __ciena_i2c_iow16be;
		}
		break;
	case sizeof(u32):
		if (priv->little_endian) {
			priv->ior = __ciena_i2c_ior32;
			priv->iow = __ciena_i2c_iow32;
		}
		else {
			priv->ior = __ciena_i2c_ior32be;
			priv->iow = __ciena_i2c_iow32be;
		}
		break;
	default:
		dev_err(priv->dev, "invalid reg_width: %u\n",
			priv->reg_width);
		BUG();
	}

	/* Get memory mapped resource. */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(dev, "missing memory resource\n");
		return -ENXIO;
	}

	/* adjust the start of the resource based on the actual
	 * register offset */
	adj_mem = *mem;
	adj_mem.start += priv->reg_width * priv->reg_offset;

	dev_dbg(dev, "%pR from parent\n%pR adjusted\n", mem, &adj_mem);

	if (!priv->shared_io) {
		region = devm_request_mem_region(dev, adj_mem.start,
						 resource_size(&adj_mem),
						 CIENA_I2C_DRIVER_NAME);
		if (!region) {
			dev_err(dev,"failed to request region %pR\n", &adj_mem);
			return -ENXIO;
		}
	}

	priv->regs = devm_ioremap(dev, adj_mem.start, resource_size(&adj_mem));
	if (!priv->regs) {
		dev_err(dev, "failed to ioremap registers\n");
		return -EFAULT;
	}
#ifdef CONFIG_CIENA_MCEE
	ciena_mcee_dev_init(to_platform_device(dev), priv->regs,
			    resource_size(&adj_mem), &priv->mceed);
#endif

	return 0;
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_create_children(struct ciena_i2c_private    *priv,
				     const struct i2c_board_info *info,
				     unsigned                     count)
{
	const struct i2c_board_info *end = &info[count];
	struct device               *dev = priv->dev;
	struct i2c_client           *client;

	for (; info < end; ++info) {
		request_module("%s", info->type);

		client = i2c_new_client_device(&priv->adapter, info);
		if (!client) {
			dev_err(dev, "failed to create device %s @0x%02x\n",
				info->type, info->addr);
			return -1;
		}
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_child_work(struct work_struct *work)
{
	struct ciena_i2c_private    *priv;
	const struct ciena_i2c_info *info;

	priv = container_of(work, struct ciena_i2c_private, children_work);
	info = (const struct ciena_i2c_info *) dev_get_platdata(priv->dev);

	(void) ciena_i2c_create_children(priv, info->deferred_info,
					 info->num_deferred_info);
}

/*----------------------------------------------------------------------------*/
static int ciena_i2c_probe(struct platform_device *pdev)
{
	struct device               *dev        = &pdev->dev;
	struct device_node          *node       = dev->of_node;
	const struct ciena_i2c_info *info       = dev->platform_data;
	struct ciena_i2c_private    *priv;
	const char                  *name       = NULL;
	int                          bus_number = 0;
	int                          rc;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(dev, "failed to alloc private data\n");
		return -ENOMEM;
	}

	init_completion(&priv->op_complete);

	for (rc = 0; rc < ARRAY_SIZE(priv->op_watch); rc++) {
		rt_mutex_init(&priv->op_watch[rc].op_mutex);
		priv->op_watch[rc].priv = priv;
		atomic_set(&priv->op_watch[rc].wait_done,
			   CIENA_I2C_WAIT_INIT);
	}

	priv->dev           = dev;
	priv->irq           = 0;
	priv->reg_map       = ciena_i2c_regmap_default;
	priv->i2c_timeout   = CIENA_I2C_POLL_DELAY_US;
	priv->pre_start_gap = (CIENA_I2C_BUS_FREE_MIN_US * 5) / 2;
	priv->err_state     = &priv->local_err_state;

	/* Parse platform data. */
	if (info) {
		priv->shared_io     = info->shared_io;
		priv->little_endian = info->little_endian;
		priv->no_watch      = info->no_watch;
		priv->reg_width     = info->reg_width;
		priv->reg_offset    = info->reg_offset;
		priv->reg_gap       = info->reg_gap;
		priv->sw_if_sel     = info->sw_if_sel;
		priv->clock_freq    = info->clock_freq;
		priv->parent_regmap = info->parent_regmap;
		bus_number          = info->bus_number;
		name                = info->name;

		if (info->err_state) priv->err_state = info->err_state;
	}

	/* Parse device-tree properties. */
	rc = ciena_i2c_parse_of(dev, priv);
	if (rc != 0)
		goto free_priv;

	if (priv->no_watch)
		INIT_WORK(&priv->nowatch_poll_work, ciena_i2c_nowatch_poll);

	/* Special HALF_PERIOD and IF_SEL registers require a special
	 * register map. */
	if (priv->clock_freq || priv->sw_if_sel)
		priv->reg_map = ciena_i2c_regmap_sw_sel;

	if (priv->sw_if_sel == 2) {
		priv->reg_map = ciena_i2c_regmap_sw_sel2;
	}
	/* Default register width to 32-bit for backwards compatibility. */
	if (priv->reg_width == 0)
		priv->reg_width = sizeof(u32);
	/* Default space between registers to 1 for backwards compatibility. */
	if (priv->reg_gap == 0)
		priv->reg_gap = 1;

	if ((priv->parent_regmap && ciena_i2c_setup_regmap(priv)) ||
	    (!priv->parent_regmap && ciena_i2c_setup_mmio(priv)))
		goto free_priv;

	dev_set_drvdata(dev, priv);

	/* Get IRQ resource. */
	rc = ciena_i2c_get_irq(pdev, priv, info);
	if (rc) goto unmap_mem;

	/* If the user wants a specific pre-start gap, oblige. */
	if (info && info->pre_start_gap)
               priv->pre_start_gap = info->pre_start_gap;

	/* Create the I2C adapter. */
	ciena_i2c_init(priv);
	ciena_i2c_reset(priv, true);

	priv->adapter.dev.parent  = dev;
	priv->adapter.dev.of_node = of_node_get(node);
	priv->adapter.owner       = THIS_MODULE;
	priv->adapter.algo        = &ciena_i2c_algorithm;
	priv->adapter.timeout     = HZ;
	/* default to dynamic bus number */
	priv->adapter.nr          = bus_number ? bus_number : -1;

	i2c_set_adapdata(&priv->adapter, priv);
	strncpy(priv->adapter.name,
		name ? name : CIENA_I2C_DRIVER_NAME "-adapter",
		sizeof(priv->adapter.name) - 1);

	rc = i2c_add_numbered_adapter(&priv->adapter);
	if (rc != 0) {
		dev_err(dev, "failed to add adapter [%d]\n", rc);
		goto free_irq;
	}

	priv->adap_err_state.adapter_id = i2c_adapter_id(&priv->adapter);

	dev_info(dev, "regs=%p irq=%d shio=%d le=%d width=%u off=%u gap=%u\n",
		 priv->regs, priv->irq, priv->shared_io, priv->little_endian,
		 priv->reg_width, priv->reg_offset, priv->reg_gap);

	/* Register child I2C devices from platform data. */
	if (info) {
		if (info->board_info)
			if (ciena_i2c_create_children(priv, info->board_info,
						      info->num_board_info))
				goto del_adapter;

		if (info->deferred_info) {
			INIT_WORK(&priv->children_work, ciena_i2c_child_work);
			if (!schedule_work(&priv->children_work)) {
				dev_err(dev, "failed deferred child work\n");
				goto del_adapter;
			}
		}
	}

	return 0;

del_adapter:
	i2c_del_adapter(&priv->adapter);
free_irq:
	of_node_put(node);
	if (priv->irq != 0)
		free_irq(priv->irq, priv);
	dev_set_drvdata(dev, NULL);
unmap_mem:
#ifdef CONFIG_CIENA_MCEE
	ciena_mcee_dev_deinit(&priv->mceed);
#endif
free_priv:
	kfree(priv);
	return rc;
}

/*----------------------------------------------------------------------------*/
static void ciena_i2c_remove(struct platform_device *pdev)
{
	struct ciena_i2c_private *priv = dev_get_drvdata(&pdev->dev);

	dev_dbg(&pdev->dev, "%s\n", __func__);

	i2c_del_adapter(&priv->adapter);
	of_node_put(pdev->dev.of_node);

	if (priv->irq != 0)
		free_irq(priv->irq, priv);
	dev_set_drvdata(&pdev->dev, NULL);

#ifdef CONFIG_CIENA_MCEE
	ciena_mcee_dev_deinit(&priv->mceed);
#endif

	kfree(priv);
}

/*----------------------------------------------------------------------------*/
#ifdef CONFIG_OF
static const struct of_device_id ciena_i2c_of_ids[] = {
	{.compatible = "ciena,i2c"},
	{}
};
MODULE_DEVICE_TABLE(of, ciena_i2c_of_ids);
#endif

static const struct platform_device_id ciena_i2c_platform_ids[] = {
	{.name = CIENA_I2C_DRIVER_NAME},
	{.name = "ciena-i2c"}, /* for backwards-compatibility */
	{}
};
MODULE_DEVICE_TABLE(platform, ciena_i2c_platform_ids);

/*----------------------------------------------------------------------------*/
static struct platform_driver ciena_i2c_driver = {
	.driver = {
		.name           = CIENA_I2C_DRIVER_NAME,
		.owner          = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= ciena_i2c_of_ids,
#endif
	},
	.probe    = ciena_i2c_probe,
	.remove   = ciena_i2c_remove,
	.id_table = ciena_i2c_platform_ids,
};

/*----------------------------------------------------------------------------*/
static void __init ciena_i2c_watch_setprio(struct kthread_work *work)
{
	sched_set_fifo_low(current);
}

/*----------------------------------------------------------------------------*/
static int __init ciena_i2c_driver_init(void)
{
	int rc;

	ciena_i2c_watch = kthread_create_worker(0, "ciena_i2c_watch");
	if (IS_ERR(ciena_i2c_watch))
		return PTR_ERR(ciena_i2c_watch);

	kthread_init_work(&ciena_i2c_watch_init,
			  ciena_i2c_watch_setprio);

	kthread_queue_work(ciena_i2c_watch, &ciena_i2c_watch_init);

	rc = platform_driver_register(&ciena_i2c_driver);

	if (rc) kthread_destroy_worker(ciena_i2c_watch);

	return rc;
}

static void __exit ciena_i2c_driver_exit(void)
{
	kthread_destroy_worker(ciena_i2c_watch);
	platform_driver_unregister(&ciena_i2c_driver);
}

module_init(ciena_i2c_driver_init);
module_exit(ciena_i2c_driver_exit);

/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Tao Wang <tawang@ciena.com>");
MODULE_DESCRIPTION("Ciena I2C Bus Adapter");
MODULE_LICENSE("GPL");

/*
 * Local variables:
 *  c-indent-level: 8
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
