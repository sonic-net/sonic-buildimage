/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
 */

/*
 * Kernel PCIE Manager - test/serial/debug code
 *
 * Author: rob.gardner@oracle.com
 */

#include "pciesvc_impl.h"

#define TICKS_PER_US 200
#define TICKS_PER_MS  (1000*TICKS_PER_US)
#define TICKS_PER_SEC (1000*TICKS_PER_MS)

/*
 * kp_udelay
 *
 * Like kernel udelay(), but avoids an external call.
 */
void kp_udelay(unsigned long us)
{
	unsigned long last = read_sysreg(cntvct_el0);
	unsigned long now, elapsed = 0;
	unsigned long limit = us * TICKS_PER_US;

	while (elapsed < limit) {
		now = read_sysreg(cntvct_el0);
		if (now > last)
			elapsed += now - last;
		last = now;
	}
}

int time_elapsed(unsigned long start, unsigned long elapsed)
{
	unsigned long now = read_sysreg(cntvct_el0);

	if (now > start + elapsed)
		return 1;

	if (now < start && now > elapsed) /* good enough */
		return 1;

	return 0;
}

/*
 * Very simple global spin lock:
 * Not very well throught out or tested since it is
 * not used for any important purpose. It is only
 * used by the serial puts() function.
 */
unsigned long lock_table[16];

void kp_lock(void)
{
	int i, cpu = cpuid();
	unsigned long sum;

	while (1) {
		lock_table[cpu] = 1;
		__asm__ __volatile__("dsb sy;" ::);

		for (sum=0, i=0; i<16; i++)
			sum += lock_table[i];

		if (sum == 1)	/* acquired lock */
			return;

		lock_table[cpu] = 0;
		__asm__ __volatile__("dsb sy;" ::);
		kp_udelay(1000+cpu*1000); /* a few ms */
	}
}

void kp_unlock(void)
{
	lock_table[cpuid()] = 0;
	__asm__ __volatile__("dsb sy;" ::);
}

/*
 * Mini serial output driver
 *
 * We want to avoid a potential infinite loop if something
 * goes wrong with the uart, so let's wait no more than 1ms
 * for the transmitter shift register to become empty. The
 * baud rate is 115200, so theoretically, the shift register
 * should never take longer than 100us to become empty.
 */
void _uart_write(unsigned char *reg, char c)
{
	int i;

	for (i=0; i<10; i++) {
		if (*(reg + UART_LSR) & OK_TO_WRITE)
			break;
		kp_udelay(100);
	}
	*(reg + UART_THR) = c;
	for (i=0; i<10; i++) {
		if (*(reg + UART_LSR) & OK_TO_WRITE)
			break;
		kp_udelay(100);
	}
}

void uart_write(kstate_t *ks, char c)
{
	_uart_write(ks->uart_addr, c);
}

int uart_read(kstate_t *ks, char *c)
{
	volatile unsigned char *reg = ks->uart_addr;
	if (*(reg + UART_LSR) & DATA_READY) {
		*c = *(reg + UART_THR);
		return 1;
	}
	return 0;
}

void uart_write_debug(kstate_t *ks, char c)
{
	if (ks->debug)
		_uart_write(ks->uart_addr, c);
}

void kdbg_puts(const char *s)
{
	kstate_t *ks = get_kstate();

	if (ks->uart_addr == NULL)
		return;

	kp_lock();
	for ( ; *s; s++) {
		uart_write(ks, *s);
		if (*s == '\n')
			uart_write(ks, '\r');
	}
	kp_unlock();
}

/*
 * For testing, this causes an SERR to be generated
 */
void trigger_serr(int val)
{
	const uint64_t good_bad_pa = 0x20141000;
	uint32_t dummy;

	kdbg_puts("kpcimgr: triggering serr\n");
	if (val == 0x100)
		dummy = pciesvc_reg_rd32(good_bad_pa);
	else
		pciesvc_pciepreg_rd32(good_bad_pa, &dummy);
}

void kpcimgr_report_stats(kstate_t *ks, int phase, int always, int rightnow)
{
	pciehw_shmem_t *pshmem = pciesvc_shmem_get();
	unsigned long now = read_sysreg(cntvct_el0);
	uint64_t cfgrd, cfgwr, memrd, memwr;
	static unsigned long last_call = 0;
	pciemgr_stats_t *s;
	pciehw_port_t *p;

	if (!always && (now - last_call) < 5 * TICKS_PER_SEC)
		return;

	p = PSHMEM_ADDR_FIELD(pshmem, port[0]);
	s = &p->stats;
	cfgrd = s->ind_cfgrd - ks->ind_cfgrd;
	cfgwr = s->ind_cfgwr - ks->ind_cfgwr;
	memrd = s->ind_memrd - ks->ind_memrd;
	memwr = s->ind_memwr - ks->ind_memwr;

	if (!always && (cfgrd + cfgwr + memrd + memwr) == 0)
		return;

	if (rightnow || ks->debug) {
		kpr_err("KPCIMGR: called %d times during %s phase: %lld cfgrd, %lld cfgwr, %lld memrd, %lld memwr\n",
			ks->ncalls, (phase == NOMMU) ? "nommu" : "normal",
			cfgrd, cfgwr, memrd, memwr);
		kpr_err("         %d ind_intr, %d not_intr, %d event_intr\n", ks->ind_intr, ks->not_intr, ks->event_intr);
	}

	ks->ind_cfgrd = s->ind_cfgrd;
	ks->ind_cfgwr = s->ind_cfgwr;
	ks->ind_memrd = s->ind_memrd;
	ks->ind_memwr = s->ind_memwr;

	last_call = read_sysreg(cntvct_el0);
}

