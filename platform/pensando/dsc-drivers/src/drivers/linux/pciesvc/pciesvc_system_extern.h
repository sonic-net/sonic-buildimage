/*
 * Copyright (c) 2021, Pensando Systems Inc.
 */

#ifndef __PCIESVC_SYSTEM_EXTERN_H__
#define __PCIESVC_SYSTEM_EXTERN_H__

#include "kpcimgr_api.h"
#include "pciesvc.h"
#include "portcfg.h"

#include <linux/pci_regs.h>
#include "notify_entry.h"
#include "cfgspace.h"

#define KPR_LINESZ 512
#define kpr_err(fmt, ...) \
	do {		  \
		char buf[KPR_LINESZ];		\
		if (virtual()) \
			pciesvc_snprintf(buf, KPR_LINESZ, KERN_ERR fmt, ##__VA_ARGS__);	\
		else \
			pciesvc_snprintf(buf, KPR_LINESZ, fmt, ##__VA_ARGS__);	\
		pciesvc_log(buf); \
	} while (0)

#define kdbg_puts_caller() \
	do {		      \
		kstate_t *kstate = get_kstate();	\
		kpr_err("%s called from offset %lx\n", __func__, \
			((unsigned long)__builtin_return_address(0) - (unsigned long)kstate->code_base)); \
	} while (0)

#define pciesvc_assert(expr) \
        if (unlikely(!(expr))) {                                   \
		kpr_err("Assertion failed! %s,%s,%s,line=%d\n", \
			#expr, __FILE__, __func__, __LINE__);	\
        }

#define pciesvc_usleep          kp_udelay
#define pciesvc_ffs             ffs
#define pciesvc_ffsll           __builtin_ffsl

#define MIN(x,y) ((x) < (y) ? x : y)
#define MAX(x,y) ((x) > (y) ? x : y)

#define PRIi64 "lld"

#define PRIx8 "x"
#define PRIx16 "x"
#define PRIx32 "x"
#define PRIx64 "llx"
#define PRIu64 "llu"

#define pciesvc_htobe32(x) __cpu_to_be32(x)
#define pciesvc_be32toh(x) __be32_to_cpu(x)

#define pciesvc_htole32(x) __cpu_to_le32(x)
#define pciesvc_le32toh(x) __le32_to_cpu(x)

#define pciesvc_htobe16(x) __cpu_to_be16(x)
#define pciesvc_be16toh(x) __be16_to_cpu(x)

#define CLEAN                   0
#define DIRTY                   1

int
pciesvc_snprintf(char *buf, size_t len, const char *fmt, ...);

int
pciesvc_vsnprintf(char *buf, size_t len, const char *fmt, va_list ap)
    __attribute__((weak));


u64
pciesvc_vtop(const void *hwmemva);

void
*pciesvc_hwmem_get(void);
uint32_t
pciesvc_reg_rd32(const uint64_t pa);
void
pciesvc_pciepreg_rd32(const uint64_t pa, uint32_t *dest);
void
pciesvc_reg_wr32(const uint64_t pa, const uint32_t val);
#define pciesvc_pciepreg_wr32   pciesvc_reg_wr32

int
pciesvc_mem_rd(const uint64_t pa, void *buf, const size_t sz);
void
pciesvc_mem_wr(const uint64_t pa, const void *buf, const size_t sz);
void
pciesvc_mem_barrier(void);

void *
pciesvc_memset(void *s, int c, size_t n);
void *
pciesvc_memcpy(void *dst, const void *src, size_t n);
void *
pciesvc_memcpy_toio(void *dsthw, const void *src, size_t n);

void
pciesvc_log(const char *msg);

int
pciesvc_event_handler(pciesvc_eventdata_t *evdata, const size_t evsize);

void *
pciesvc_shmem_get(void);

int virtual(void);
int cpuid(void);
unsigned long release(void);
long read_el(void);
void kpcimgr_init_poll(kstate_t *ks);
void pciesvc_debug_cmd(uint32_t *val);
void kpcimgr_poll(kstate_t *ks, int index, int phase);

/* functions in kpci_test.c */
void kp_udelay(unsigned long us);
int time_elapsed(unsigned long start, unsigned long elapsed);
void _uart_write(unsigned char *reg, char c);
void uart_write(kstate_t *ks, char c);
int uart_read(kstate_t *ks, char *c);
void uart_write_debug(kstate_t *ks, char c);
void kdbg_puts(const char *s);
void trigger_serr(int val);
void kpcimgr_report_stats(kstate_t *ks, int phase, int always, int rightnow);

/* functions in kpci_kexec.c */
void set_kstate(kstate_t *ks);

#endif /* __PCIESVC_SYSTEM_EXTERN_H__ */
