/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Pensando Systems Inc.
 */

#ifndef __UART_H__
#define __UART_H__

/*
 * UART definitions
 */
typedef struct uart_state {
    uint8_t rbr;                 /* reg0: receive register (read) */
    uint8_t thr;                 /* reg0: transmit holding register (write) */
    uint8_t ier;                 /* reg1: interrupt enable register */
    uint8_t iir;                 /* reg2: interrupt id register (read) */
    uint8_t fcr;                 /* reg2: fifo control register (write) */
    uint8_t lcr;                 /* reg3: line control register */
    uint8_t mcr;                 /* reg4: modem control register */
    uint8_t lsr;                 /* reg5: line status register */
    uint8_t msr;                 /* reg6: modem status register */
    uint8_t scr;                 /* reg7: scratch register */
    int thr_ipending;
    uint16_t divider;
    int parity;
    int data_bits;
    int stop_bits;
    uint8_t recv_fifo_itl;       /* interrupt trigger level */
    uint8_t mcr_read;
    uint8_t mcr_write;
    int flags;
} uart_state_t;


enum uart_regs {
    UART_RX_BUF = 0,
    UART_TX_BUF = 0,
    UART_INTERRUPT_ENABLE = 1,
    UART_INTERRUPT_ID = 2,
    UART_FIFO_CONTROL = 2,
    UART_LINE_CONTROL = 3,
    UART_MODEM_CONTROL = 4,
    UART_LINE_STATUS = 5,
    UART_MODEM_STATUS = 6,
    UART_SCRATCH = 7,
};

#define UART_LCR_BRK    0x40    /* Set Break Enable */
#define UART_LCR_DLAB   0x80    /* Divisor latch access bit */

/*
 * Interrupt Enable Register (offset 0x1)
 */
#define UART_IER_MSI    0x08    /* Enable Modem status interrupt */
#define UART_IER_RLSI   0x04    /* Enable receiver line status interrupt */
#define UART_IER_THRI   0x02    /* Enable Transmitter holding register int. */
#define UART_IER_RDI    0x01    /* Enable receiver data interrupt */

/*
 * Interrupt Identification Register (offset 0x2) - read
 */
#define UART_IIR_NO_INT 0x01    /* No interrupts pending */
#define UART_IIR_ID     0x06    /* Mask for the interrupt ID */

#define UART_IIR_MSI    0x00    /* Modem status interrupt */
#define UART_IIR_THRI   0x02    /* Transmitter holding register empty */
#define UART_IIR_RDI    0x04    /* Receiver data interrupt */
#define UART_IIR_RLSI   0x06    /* Receiver line status interrupt */
#define UART_IIR_CTI    0x0C    /* Character Timeout Indication */

#define UART_IIR_FENF   0x80    /* Fifo enabled, but not functionning */
#define UART_IIR_FE     0xC0    /* Fifo enabled */

/*
 * FIFO Control Register (offset 0x2) - write
 */
#define UART_FCR_ITL_1  0x00    /* 1 byte ITL */
#define UART_FCR_ITL_2  0x40    /* 4 bytes ITL */
#define UART_FCR_ITL_3  0x80    /* 8 bytes ITL */
#define UART_FCR_ITL_4  0xC0    /* 14 bytes ITL */
#define UART_FCR_DMS    0x08    /* DMA Mode Select */
#define UART_FCR_XFR    0x04    /* XMIT Fifo Reset */
#define UART_FCR_RFR    0x02    /* RCVR Fifo Reset */
#define UART_FCR_FE     0x01    /* FIFO Enable */

/*
 * Modem Control Register (offset 0x4)
 */
#define UART_MCR_LOOP   0x10    /* Enable loopback test mode */
#define UART_MCR_OUT2   0x08    /* Out2 complement */
#define UART_MCR_OUT1   0x04    /* Out1 complement */
#define UART_MCR_RTS    0x02    /* RTS complement */
#define UART_MCR_DTR    0x01    /* DTR complement */

/*
 * Line Status Register (offset 0x5)
 */
#define UART_LSR_TEMT   0x40    /* Transmitter empty */
#define UART_LSR_THRE   0x20    /* Transmit-hold-register empty */
#define UART_LSR_BI     0x10    /* Break interrupt indicator */
#define UART_LSR_FE     0x08    /* Frame error indicator */
#define UART_LSR_PE     0x04    /* Parity error indicator */
#define UART_LSR_OE     0x02    /* Overrun error indicator */
#define UART_LSR_DR     0x01    /* Receiver data ready */
#define UART_LSR_INT_ANY 0x1E   /* Any of the lsr-interrupt-triggering bits */

/*
 * Modem Status Register (offset 0x6)
 */
#define UART_MSR_DCD    0x80    /* Data Carrier Detect */
#define UART_MSR_RI     0x40    /* Ring Indicator */
#define UART_MSR_DSR    0x20    /* Data Set Ready */
#define UART_MSR_CTS    0x10    /* Clear to Send */
#define UART_MSR_DDCD   0x08    /* Delta DCD */
#define UART_MSR_TERI   0x04    /* Trailing edge ring indicator */
#define UART_MSR_DDSR   0x02    /* Delta DSR */
#define UART_MSR_DCTS   0x01    /* Delta CTS */
#define UART_MSR_ANY_DELTA 0x0F /* Any of the delta bits! */

#define CHR_TIOCM_CTS   0x020
#define CHR_TIOCM_CAR   0x040
#define CHR_TIOCM_DSR   0x100
#define CHR_TIOCM_RI    0x080
#define CHR_TIOCM_DTR   0x002
#define CHR_TIOCM_RTS   0x004

#endif /* __UART_H__ */
