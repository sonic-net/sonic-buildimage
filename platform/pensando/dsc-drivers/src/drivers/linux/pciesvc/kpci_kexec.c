/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
 */

/*
 * Kernel PCIE Manager - kexec related code
 *
 * Author: rob.gardner@oracle.com
 */

#include "kpcimgr_api.h"
#include "pciesvc_impl.h"
#include "pciesvc.h"
#include "pciesvc_system.h"

#define TICKS_PER_US 200
#define TICKS_PER_MS  (1000*TICKS_PER_US)
#define TICKS_PER_SEC (1000*TICKS_PER_MS)

int holding_pen_idx;
unsigned long kstate_paddr;
kstate_t *kstate = NULL;

void set_kstate(kstate_t *ks)
{
	kstate = ks;
}

int virtual(void)
{
	return ((long)kstate) < 0 ;
}

/* called in physical mode */
void kpcimgr_nommu_poll(kstate_t *ks)
{
	kpcimgr_poll(ks, 0, NOMMU);
	ks->trace_data[NOMMU][LAST_CALL_TIME] = read_sysreg(cntvct_el0);

}

void kpcimgr_cpu_holding_pen(kstate_t *ks)
{
	long npolls = 0;
	int i;

	set_kstate(ks);
	ks->uart_addr = (void *) PEN_UART;
	if (ks->debug)
		_uart_write((void *) PEN_UART, 'C');
	kpcimgr_init_poll(ks);

	kpr_err("%s with EL%ld on cpu%d\n", __func__, read_el(), cpuid());

	holding_pen_idx = 0;
	kpr_err("going into poll loop...\n");

	while (1) {
		if (ks->debug)
			_uart_write((void *) PEN_UART, 'S');

		kpcimgr_nommu_poll(ks);
		npolls++;

		for (i=0; i<10; i++) {
			if (release()) {
				kpcimgr_nommu_poll(ks);
				kpr_err("polling did %ld polls.\n", npolls);
				return;
			}
			kp_udelay(1*1000); /* 1ms */
		}
	}
}

void serial_help(void)
{
	kpr_err("Commands:\n");
	kpr_err(" c      Cpu id\n");
	kpr_err(" e      Event queue\n");
	kpr_err(" f      Show/set cfgval\n");
	kpr_err(" h      Show help message\n");
	kpr_err(" m      Memory ranges\n");
	kpr_err(" q      Quit serial thread\n");
	kpr_err(" r      Reboot\n");
	kpr_err(" s      Serror trigger\n");
	kpr_err(" t      Report Stats\n");
}

void set_cfgval(kstate_t *ks)
{
	int cfgval = 0, modify = 0;
	char c;

	kpr_err("New cfgval: ");
	while (1) {
		while (uart_read(ks, &c) == 0);
		uart_write(ks, c);
		if (c >= '0' && c <= '9')
			cfgval = (cfgval << 4) + (c - '0');
		else if (c >= 'a' && c <= 'f')
			cfgval = (cfgval << 4) + (10 + c - 'a');
		else if (c >= 'A' && c <= 'F')
			cfgval = (cfgval << 4) + (10 + c - 'A');
		else
			break;
		modify = 1;
	}
	if (modify) {
		kpr_err("\r\ncfgval set to %x\n", cfgval);
		ks->cfgval = cfgval;
	}
	else
		kpr_err("\r\ncfgval not modified\n");
}

#define WDOG_REGS (void *)0x1400
#define WDOG_CONTROL_REG_OFFSET             0x00
#define WDOG_CONTROL_REG_WDT_EN_MASK        0x01
#define WDOG_CONTROL_REG_RESP_MODE_MASK     0x02
void watchdog_reboot(void)
{
        u32 val = readl(WDOG_REGS + WDOG_CONTROL_REG_OFFSET);

	kpr_err("Rebooting...\n");
        /* Disable interrupt mode; always perform system reset. */
        val &= ~WDOG_CONTROL_REG_RESP_MODE_MASK;
        /* Enable watchdog. */
        val |= WDOG_CONTROL_REG_WDT_EN_MASK;
        writel(val, WDOG_REGS + WDOG_CONTROL_REG_OFFSET);
}


void serial_input(char c)
{
	kstate_t *ks = get_kstate();
	int n;

	switch (c) {
	case 'c': case 'C':
		kpr_err("serial thread running on cpu#%d\n", cpuid());
		break;
	case 'e': case 'E':
		n = ks->evq_head - ks->evq_tail;
		if (n < 0)
			n += EVENT_QUEUE_LENGTH;
		kpr_err("event queue contains %d records\n", n);
		break;
	case 'f': case 'F':
		kpr_err("cfgval = %x\n", ks->cfgval);
		set_cfgval(ks);
		break;
	case '?':
	case 'h':
	case 'H':
		serial_help();
		break;
	case 'm': case 'M':
		for (n=0; n<ks->nranges; n++) {
			struct mem_range_t *mr = &ks->mem_ranges[n];
			kpr_err("range [%lx..%lx] mapped at %lx\n",
				mr->base, mr->end, mr->vaddr);
		}
		break;
	case 'q': case 'Q':
		__asm__("hvc #0;" ::);
		break;
	case 'r': case 'R':
		watchdog_reboot();
		break;
	case 's':
	case 'S':
		trigger_serr(0x100);
		break;
	case 't':
	case 'T':
		kpcimgr_report_stats(ks, NOMMU, 1, 1);
		break;
	default:
		kpr_err("'%c' unknown command\n", c);
		break;
	}
}


void kpcimgr_serial_thread(kstate_t *ks)
{
	unsigned long start = read_sysreg(cntvct_el0);
	int warning_printed = 0;

	ks->uart_addr = (void *) PEN_UART;
	set_kstate(ks);

	kpr_err("%s el%d on cpu%d\n", __func__, read_el(), cpuid());
	while (!release()) {
		char c;
		if (uart_read(ks, &c))
			serial_input(c);
		if (!warning_printed && time_elapsed(start, 2*TICKS_PER_SEC)) {
			kpr_err("Serial thread running for >2s, 'H' for help\n");
			warning_printed = 1;
		}
	}
	kpr_err("%s done\n", __func__);
}


/*
 * Called from kpcimgr when the secondary CPUs are being taken
 * offline.  We return a physical address which the secondary CPU will
 * jump to.  The global 'holding_pen_idx' keeps a count of how many
 * times we've been called so that we can return the appropriate
 * function pointer for a given cpu. It would seem that there are some
 * very dangerous race conditions here:
 *
 * 1. Can't this function be called concurrently on multiple CPUs?
 *    No, it cannot, because we are called by kpcimgr_get_entry(),
 *    which protects against this with a spinlock.
 *
 * 2. holding_pen_idx is reset to zero in kpcimgr_cpu_holding_pen(),
 *    and can't that execute on CPU1 while this function executes
 *    concurrently on CPU2?
 *    Good question! The answer is yes, they can execute
 *    simultaneously, but it is not a race because they will operate
 *    on different memory.  When this function is called, it is in
 *    virtual mode, with the code and data in normal module_alloc'ed
 *    memory. But when kpcimgr_cpu_holding_pen() executes, it is
 *    running in physical mode from a copy of the code and data that
 *    has been relocated to persistent memory. Thus, references to
 *    'holding_pen_idx' in these two functions refer to different
 *    memory locations.
 */

unsigned long kpcimgr_get_holding_pen(unsigned long old_entry,
				      unsigned int cpu, unsigned long ks_paddr)
{
	kstate_t *ks = get_kstate();
	unsigned long offset, entry;
	extern void __kpcimgr_cpu_holding_pen(void);
	extern void __kpcimgr_serial_thread(void);

	if (ks == NULL || ks->valid != KSTATE_MAGIC || !ks->running || !ks->have_persistent_mem)
		return old_entry;

	if (cpu == 0)
		return old_entry;

	switch (holding_pen_idx) {
	case 0:
		offset = (unsigned long) __kpcimgr_cpu_holding_pen - (unsigned long) ks->code_base;
		break;
	case 1:
		offset = (unsigned long) __kpcimgr_serial_thread - (unsigned long) ks->code_base;
		break;
	default:
		return old_entry;
	}
	holding_pen_idx++;

#if 1
	/* temp: stay compatible with old kernel */
	if ((ks_paddr >> 24) != 0xc5)
		ks_paddr = ks->shmembase + COMPAT_SHMEM_KSTATE_OFFSET;
#endif
	entry = ks_paddr + KSTATE_CODE_OFFSET + offset;
	kpr_err("%s(cpu%d) entry = %lx\n", __func__, cpu, entry);

	/* propagate value of ks_paddr to persistent memory */
	offset = ((unsigned long) &kstate_paddr) - (unsigned long) ks->code_base;
	*(unsigned long *)(ks->persistent_base + offset) = ks_paddr;
	return entry;
}
