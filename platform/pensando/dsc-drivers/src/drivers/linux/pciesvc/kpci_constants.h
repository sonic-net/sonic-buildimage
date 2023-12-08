/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
 */
/*
 * Layout of non-Linux Memory:
 *  (base address provided in device tree and may change)
 *  C500 0000  SHMEM segment (pciehw_shmem_t) [0x942440 bytes ~9.25Mb]
 *  C5F0 0000  kpcimgr state (kstate_t)       [3 * 64k]
 *  C5F3 0000  relocated code                 [Allow 256k]
 *  C5F7 0000  available for stack when in nommu mode (64k)
 *  C5F8 0000  top of stack
 *  C5FF FFFF  end of 1M allotted range
 */
#define SHMEM_KSTATE_OFFSET       0xF00000
#define SHMEM_KSTATE_SIZE          0x30000
#define KSTATE_STACK_OFFSET        0x80000
#define KSTATE_CODE_OFFSET      (SHMEM_KSTATE_OFFSET + SHMEM_KSTATE_SIZE)
#define KSTATE_CODE_SIZE        (256 * 1024)
#define KSTATE_MAGIC            0x1743BA1F

/* size of trace data arrays */
#define DATA_SIZE 100
#define MSG_BUF_SIZE 32768

/* uart and time related constants */
#define PEN_UART 0x4800
#define UART_THR 0
#define UART_LSR 0x14
#define DATA_READY 1
#define OK_TO_WRITE 0x20
#define UART_THRE_BIT 5

/* phases */
#define NOMMU 0
#define NORMAL 1
#define NUM_PHASES 2

#define MSI_INDIRECT_IDX	0	/* indirect vector */
#define MSI_NOTIFY_IDX		1	/* notify vector */
#define MSI_NVECTORS		2

