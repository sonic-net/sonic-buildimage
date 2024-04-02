/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
 */

/*
 * Kernel PCIE Manager "glue" code
 *
 * Author: rob.gardner@oracle.com
 */

#include "pciesvc_impl.h"
#include "version.h"

/*
 * This file contains only functions essential to the
 * operation of the pciesvc library code.
 */

void kpcimgr_init_fn(kstate_t *ks)
{
	set_kstate(ks);
}

void kpcimgr_version_fn(char **version)
{
	if (version)
		*version = PCIESVC_VERSION;
}

/*
 * Dummy function called for undefined entry points
 */
void kpcimgr_undefined_entry(void)
{
	pciesvc_log(KERN_INFO "undefined entry called\n");
}

/*
 * Initialize pciesvc for interrupt based operation
 */
void kpcimgr_init_intr(kstate_t *ks)
{
	pciesvc_params_t p;
	volatile struct msi_info *msi;

	set_kstate(ks);
	memset(&p, 0, sizeof(pciesvc_params_t));

	p.version = 0;
	p.params_v0.port = ks->active_port;

	msi = &ks->msi[MSI_INDIRECT_IDX];
	p.params_v0.ind_intr = 1;
	p.params_v0.ind_msgaddr = msi->msgaddr;
	p.params_v0.ind_msgdata = msi->msgdata;

	msi = &ks->msi[MSI_NOTIFY_IDX];
	p.params_v0.not_intr = 1;
	p.params_v0.not_msgaddr = msi->msgaddr;
	p.params_v0.not_msgdata = msi->msgdata;

	if (pciesvc_init(&p))
		kpr_err("%s: pciesvc_init failed\n", __func__);

	/* clear out any pending transactions */
	kpcimgr_poll(ks, 0, NORMAL);
}

/*
 * Initialize pciesvc for polling based operation
 */
void kpcimgr_init_poll(kstate_t *ks)
{
	pciesvc_params_t p;

	set_kstate(ks);
	memset(&p, 0, sizeof(pciesvc_params_t));

	p.version = 0;
	p.params_v0.port = ks->active_port;

	p.params_v0.ind_poll = 1;
	p.params_v0.not_poll = 1;

	pciesvc_init(&p);
}

/*
 * Main poll function
 *
 * Essentially a wrapper for pciesvc_poll() that
 * updates statistics, does some error checking,
 * and outputs some debugging information.
 */
void kpcimgr_poll(kstate_t *ks, int index, int phase)
{
	int i, result;
	long ts = read_sysreg(cntvct_el0);

	set_kstate(ks);
	ks->ncalls++;

	if (ks->trace_data[phase][FIRST_CALL_TIME] == 0) {
		uart_write_debug(ks, 'F');
		ks->trace_data[phase][FIRST_CALL_TIME] = ts;

		if (phase == NOMMU)
			kpcimgr_report_stats(ks, NORMAL, 1, 0);
		else
			kpcimgr_report_stats(ks, NOMMU, 1, 0);
	}

	ks->trace_data[phase][NUM_CALLS]++;

	if (phase == NOMMU)
		uart_write_debug(ks, 'M');

	if (ks->valid != KSTATE_MAGIC) {
		uart_write_debug(ks, 'V');
		return;
	}

	if (!ks->running) {
		uart_write_debug(ks, 'P');
		return;
	}

	ks->trace_data[phase][LAST_CALL_TIME] = ts;
	ks->trace_data[phase][NUM_CHECKS]++;

	if (ks->debug & 0x300) {
		trigger_serr(ks->debug & 0x300);
		ks->debug &= ~0x300;
	}

	for (i=0; i<10; i++) {

		result = pciesvc_poll(0);
		/*
		 * return value:
		 *  1: valid pending and handled
		 *  0: nothing pending
		 */

		if (result == 0)
			break;
		if (result == -1) {
			uart_write_debug(ks, '?');
			break;
		}

		uart_write_debug(ks, 'h');

		ks->trace_data[phase][NUM_PENDINGS]++;
	}
	kpcimgr_report_stats(ks, phase, 0, 0);
}

/*
 * ISR for Indirect Interrupt
 */
int kpcimgr_ind_intr(kstate_t *ks, int port)
{
	int ret;

	set_kstate(ks);
	ret = pciesvc_indirect_intr(port);
	if (ks->debug & 0x300) {
		trigger_serr(ks->debug & 0x300);
		ks->debug &= ~0x300;
	}

	return ret;
}

/*
 * ISR for Notify Interrupt
 */
int kpcimgr_not_intr(kstate_t *ks, int port)
{
	set_kstate(ks);
	return pciesvc_notify_intr(port);
}

/*
 * Return a VA from one of our known ranges
 *
 * If we're running with the MMU turned off, then just return the
 * physical address.
 *
 */
void *kpcimgr_va_get(unsigned long pa, unsigned long sz)
{
	kstate_t *ks = get_kstate();
	int i;

	if (!virtual())
		return (void *) pa;

	for (i=0; i<ks->nranges; i++) {
		struct mem_range_t *mr = &ks->mem_ranges[i];
		if (pa >= mr->base && pa < mr->end)
			return mr->vaddr + (pa - mr->base);
	}

	kpr_err("%s: bad pa 0x%lx\n", __func__, pa);
    	pciesvc_assert(0);
	return NULL;
}

/*
 * Reverse translation: return a physical address
 * corresponding to some virtual address.
 */
u64 pciesvc_vtop(const void *hwmemva)
{
	kstate_t *ks = get_kstate();
	u64 hwptr = (u64) hwmemva;
	int i;

	for (i=0; i<ks->nranges; i++) {
		struct mem_range_t *mr = &ks->mem_ranges[i];
		u64 size;

		/* was a physical address passed in to us? */
		if (hwptr >= mr->base && hwptr < mr->end)
			return hwptr;
		size = mr->end - mr->base;
		if (hwmemva >= mr->vaddr &&
		    hwmemva <  mr->vaddr + size)
			return mr->base + (hwmemva - mr->vaddr);
	}
	return 0;
}

/*
 * Up calls from pciesvc
 */
uint32_t
pciesvc_reg_rd32(const uint64_t pa)
{
    u_int32_t val, *va = kpcimgr_va_get(pa, 4);

    pciesvc_assert((pa & 0x3) == 0);
    val = readl(va);
    __asm__ __volatile__("isb; dsb sy;" ::);

    return val;
}

void
pciesvc_pciepreg_rd32(const uint64_t pa, uint32_t *dest)
{
	u_int32_t val, (*upcall)(int req, unsigned long pa);
	kstate_t *ks = get_kstate();

	pciesvc_assert((pa & 0x3) == 0);
	upcall = ks->upcall;
	if (upcall && virtual())
		val = upcall(PREG_READ, pa);
	else
		val = pciesvc_reg_rd32(pa);

	*dest = val;
}

void
pciesvc_reg_wr32(const uint64_t pa, const uint32_t val)
{
    u_int32_t *va = kpcimgr_va_get(pa, 4);

    pciesvc_assert((pa & 0x3) == 0);
    writel(val, va);
}

/*
 * Similar calls implemented in terms of rd32/wr32.
 */
typedef union {
    u_int32_t l;
    u_int16_t h[2];
    u_int8_t  b[4];
} iodata_t;

int
pciesvc_mem_rd(const uint64_t pa, void *buf, const size_t sz)
{
    uint64_t pa_aligned;
    uint8_t idx;
    iodata_t v;

    switch (sz) {
    case 1:
        pa_aligned = pa & ~0x3;
        idx = pa & 0x3;
        v.l = pciesvc_reg_rd32(pa_aligned);
        *(uint8_t *)buf = v.b[idx];
        break;
    case 2:
        pa_aligned = pa & ~0x3;
        idx = (pa & 0x3) >> 1;
        v.l = pciesvc_reg_rd32(pa_aligned);
        *(uint16_t *)buf = v.h[idx];
        break;
    case 4:
    case 8:
        pciesvc_reg_rd32w(pa, (uint32_t *)buf, sz >> 2);
        break;
    default:
        return -1;
    }
    return 0;
}

void
pciesvc_mem_wr(const uint64_t pa, const void *buf, const size_t sz)
{
    uint64_t pa_aligned;
    uint8_t idx;
    iodata_t v;

    switch (sz) {
    case 1:
        pa_aligned = pa & ~0x3;
        idx = pa & 0x3;
        v.l = pciesvc_reg_rd32(pa_aligned);
        v.b[idx] = *(uint8_t *)buf;
        pciesvc_reg_wr32(pa_aligned, v.l);
        break;
    case 2:
        pa_aligned = pa & ~0x3;
        idx = (pa & 0x3) >> 1;
        v.l = pciesvc_reg_rd32(pa_aligned);
        v.h[idx] = *(uint16_t *)buf;
        pciesvc_reg_wr32(pa_aligned, v.l);
        break;
    case 4:
    case 8:
        pciesvc_reg_wr32w(pa, (uint32_t *)buf, sz >> 2);
        break;
    default:
        break;
    }
}

void
pciesvc_mem_barrier(void)
{
	mb();
}

/*
 * We need our own memset/memcpy functions because we
 * cannot call any kernel functions. And even if we could,
 * we need to avoid cache operations since "non-linux" memory
 * is non-cached.
 */
void *
pciesvc_memset(void *s, int c, size_t n)
{
	if (((uintptr_t)s & 0x3) == 0 && (n & 0x3) == 0) {
		volatile u_int32_t *p;
		int i;

		c &= 0xff;
		c = ((c << 0) |
		     (c << 8) |
		     (c << 16) |
		     (c << 24));
		for (p = s, i = 0; i < n >> 2; i++, p++) {
			*p = c;
		}
	} else {
		volatile u_int8_t *p;
		int i;

		for (p = s, i = 0; i < n; i++, p++) {
			*p = c;
		}
	}

	return s;
}

#undef memset
void * __weak
memset(void *s, int c, size_t n)
{
    return pciesvc_memset(s, c, n);
}

void *
pciesvc_memcpy(void *dst, const void *src, size_t n)
{
	volatile u_int8_t *d = dst;
	const u_int8_t *s = src;
	int i;

	for (i = 0; i < n; i++) {
		*d++ = *s++;
	}
	return dst;
}

#undef memcpy
void * __weak
memcpy(void *dst, const void *src, size_t n)
{
    return pciesvc_memcpy(dst, src, n);
}
void *

pciesvc_memcpy_toio(void *dsthw, const void *src, size_t n)
{
    return pciesvc_memcpy(dsthw, src, n);
}

void *
pciesvc_shmem_get(void)
{
	kstate_t *ks = get_kstate();

	if (virtual())
		return ks->shmemva;
	else
		return (void *) ks->shmembase;
}

void *pciesvc_hwmem_get(void)
{
	kstate_t *ks = get_kstate();

	if (virtual())
		return ks->mem_ranges[ks->hwmem_idx].vaddr;
	else
		return (void *) ks->mem_ranges[ks->hwmem_idx].base;
}

void
pciesvc_log(const char *msg)
{
	kstate_t *ks = get_kstate();
	u64 (*upcall)(int req, char *msg);

	upcall = ks->upcall;
	if (upcall && virtual())
		upcall(PRINT_LOG_MSG, (char *)msg);
	else
		kdbg_puts((char *)msg);
}

void wakeup_event_queue(void)
{
	kstate_t *ks = get_kstate();
	u64 (*upcall)(int req);

	upcall = ks->upcall;
	if (upcall && virtual())
		upcall(WAKE_UP_EVENT_QUEUE);
}

/*
 * Event Queue Handler
 *
 * Event queue semantics:
 *  evq_head = index of slot used for next insertion
 *  evq_tail = index of slot used for next removal
 *  queue is empty when head == tail
 *  queue is full when (head + 1) % queue_size == tail
 *  queue is nearly full when (head + 2) % queue_size == tail
 *
 * Only head is modified here, and the read() function only
 * modifies tail, so theoretically no race can exist. It is
 * possible for the reader to see an empty queue momentarily
 * or the handler to see a full queue momentarily, but these
 * situations do not justify adding locks.
 */
int pciesvc_event_handler(pciesvc_eventdata_t *evdata, const size_t evsize)
{
	kstate_t *ks = get_kstate();
	int ret = 0;
	static int was_full = 0;

	if (evsize != sizeof(pciesvc_eventdata_t)) {
		kpr_err("%s: evsize != sizeof(pciesvc_eventdata_t))\n", __func__);
		return -1;
	}

	if ((ks->evq_head + 1) % EVENT_QUEUE_LENGTH == ks->evq_tail) {
		if (!was_full)
			pciesvc_log(KERN_INFO "pciesvc_event_handler: event queue full\n");
		was_full = 1;
		return -1;
	}
	was_full = 0;

	if ((ks->evq_head + 2) % EVENT_QUEUE_LENGTH == ks->evq_tail) {
		pciesvc_log(KERN_INFO "pciesvc_event_handler: event queue almost full\n");
		evdata->evtype = PCIESVC_EV_QFULL;
		ret = -1;
	}

	pciesvc_memcpy_toio((void *)ks->evq[ks->evq_head], evdata, sizeof(pciesvc_eventdata_t));

	ks->evq_head = (ks->evq_head + 1) % EVENT_QUEUE_LENGTH;
	wakeup_event_queue();
	return ret;
}

void pciesvc_debug_cmd(uint32_t *cmd)
{
	kstate_t *ks = get_kstate();
	uint32_t delayus;

	switch (*cmd) {
	case 0x17:
		*cmd = virtual();
		return;
	case 0x19:
		*cmd = ks->cfgval;
		return;
	case 0x100:
	case 0x200:
		ks->debug |= *cmd;
		return;
	default:
		delayus = *cmd;
		if (delayus) {
			pciesvc_usleep(delayus);
		}
		break;
	}
}

/*
 * cmd read/write
 */
int pciesvc_sysfs_cmd_read(kstate_t *ks, char *buf, loff_t off, size_t count, int *exists)
{
	int ret;

	if (exists)
		*exists = 1;

	ret = pciesvc_cmd_read(buf, off, count);
	return ret < 0 ? -EINVAL : ret;
}

int pciesvc_sysfs_cmd_write(kstate_t *ks, char *buf, loff_t off, size_t count, int *exists)
{
	int ret;

	if (exists)
		*exists = 1;

	ret = pciesvc_cmd_write(buf, off, count);
	return ret < 0 ? -EINVAL : ret;
}
