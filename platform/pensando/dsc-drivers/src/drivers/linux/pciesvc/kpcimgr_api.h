/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
 */
#ifndef __KPCIMGR_API_H__
#define __KPCIMGR_API_H__

#ifdef __KERNEL__
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/reboot.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>
#include <linux/msi.h>
#include <linux/mm.h>
#include <linux/kallsyms.h>
#include <linux/moduleloader.h>
#include <linux/set_memory.h>
#include <asm/insn.h>
#include <asm/cacheflush.h>
#endif

#include "kpci_constants.h"

#define K_ENTRY_INIT_INTR 0
#define K_ENTRY_INIT_POLL 1
#define K_ENTRY_SHUT 2
#define K_ENTRY_POLL 3
#define K_ENTRY_HOLDING_PEN 4
#define K_ENTRY_INDIRECT_INTR 5
#define K_ENTRY_NOTIFY_INTR 6
#define K_ENTRY_INIT_FN 7
#define K_ENTRY_CMD_READ 8
#define K_ENTRY_CMD_WRITE 9
#define K_ENTRY_GET_VERSION 10
#define K_NUM_ENTRIES 16

struct kpcimgr_entry_points_t {
	int expected_mgr_version;
	int lib_version_major;
	int lib_version_minor;
	void *code_start;
	void *code_end;
	void *entry_point[K_NUM_ENTRIES];
};

/* upcalls */
#define WAKE_UP_EVENT_QUEUE 1
#define PRINT_LOG_MSG 2
#define PREG_READ 3

/* event queue sizing */
#define EVENT_QUEUE_LENGTH 1024
#define EVENT_SIZE 128

/* max command size for sysfs cmd node */
#define CMD_SIZE 4096

/* max number of memory ranges from device tree */
#define NUM_MEMRANGES 32

struct kpcimgr_state_t {
	/* essential state */
	int valid;
	int debug;
	int running;
	int active_port;
	int have_persistent_mem;
	int lib_version_major;
	int lib_version_minor;

	/* timestamps and general trace data */
	long kexec_time;
	long driver_start_time;
	unsigned long trace_data[NUM_PHASES][DATA_SIZE];

	/* virtual addresses */
	void *uart_addr;
	void *code_base;
	void *persistent_base;
	void *upcall;
	void *pfdev;
	void *shmemva;

	unsigned long shmembase, shmem_size, code_size;
	struct mem_range_t {
		unsigned long base, end;
		void *vaddr;
	} mem_ranges[NUM_MEMRANGES];
	int nranges;
	int hwmem_idx;

	/* interrupt vectors */
	struct msi_info {
		unsigned long msgaddr;
		unsigned int msgdata;
	} msi[MSI_NVECTORS];

	/* stats for work done */
	int ind_cfgrd, ind_cfgwr;
	int ind_memrd, ind_memwr;
	int ncalls;
	int ind_intr, not_intr, event_intr;

	int unused1[7];	/* was version=2 code_offsets[], keep evq* compat */

	/* Event queue handling */
	int evq_head, evq_tail;
	char evq[EVENT_QUEUE_LENGTH][EVENT_SIZE];

	/* debugging */
	void *mod;
	int msg_idx;
	int cfgval;

	/* offsets into relocated library code */
	int code_offsets[K_NUM_ENTRIES];
};

typedef struct kpcimgr_state_t kstate_t;
_Static_assert(sizeof(kstate_t) < SHMEM_KSTATE_SIZE,
	       "kstate size insufficient");

/* trace_data[] elements */
#define FIRST_CALL_TIME 0
#define FIRST_SEQNUM 1
#define LAST_SEQNUM 2
#define TAG 3
#define PA_BAD_CNT 4
#define NUM_CHECKS 5
#define NUM_CALLS 6
#define NUM_PENDINGS 7
#define LAST_CALL_TIME 8
#define EARLY_POLL 9
#define MAX_DATA 10

#define KPCIMGR_DEV "/dev/kpcimgr"
#define KPCIMGR_NAME "kpcimgr"
#define PFX KPCIMGR_NAME ": "
#define KPCIMGR_KERNEL_VERSION 3

#ifdef __KERNEL__
int kpcimgr_module_register(struct module *mod,
			    struct kpcimgr_entry_points_t *ep, int relocate);
void kpcimgr_start_running(void);
void kpcimgr_stop_running(void);
void kpcimgr_sysfs_setup(struct platform_device *pfdev);
void *kpci_memcpy(void *dst, const void *src, size_t n);
void wake_up_event_queue(void);
int aarch64_insn_read(void *addr, u32 *insnp);
extern spinlock_t kpcimgr_lock;

#define reset_stats(k) \
	kpci_memset((void *)&(k)->trace_data[0][0], 0, sizeof((k)->trace_data))

static inline void set_init_state(kstate_t *k)
{
	k->trace_data[NORMAL][FIRST_CALL_TIME] = 0;
	k->ncalls = 0;
}

static inline kstate_t *get_kstate(void)
{
	extern kstate_t *kstate;
	return kstate;
}
#endif

#endif
