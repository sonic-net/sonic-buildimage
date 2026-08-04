// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "intrutils.h"
#include "serial_state.h"
#include "serial.h"
#include "uart.h"

typedef struct serial {
    int inited;                 /* state has been initialized */
    serial_state_t *state;      /* serial state */
    uart_state_t *uart;         /* uart state */
    memq_t *txq;                /* txq transfer from device to memq */
    memq_t *rxq;                /* rxq transfer from memq to device */
} serial_t;

static void serial_update_msl(serial_t *s);

static int
memq_putc(volatile memq_t *q, const u_int8_t c)
{
    const unsigned int pidx = q->pidx;
    const unsigned int cidx = q->cidx;
    const unsigned int newpidx = (pidx + 1) % MEMQ_BUFSZ;

    /* check for full q */
    if (newpidx == cidx) return 0;

    q->buf[pidx] = c;
    q->pidx = newpidx;
    return 1;
}

static int
memq_getc(volatile memq_t *q, u_int8_t *cp)
{
    /* check for empty q */
    if (q->cidx == q->pidx) return 0;

    *cp = q->buf[q->cidx];
    q->cidx = (q->cidx + 1) % MEMQ_BUFSZ;
    return 1;
}

static int
memq_full(volatile memq_t *q)
{
    const unsigned int pidx = q->pidx;
    const unsigned int cidx = q->cidx;
    const unsigned int newpidx = (pidx + 1) % MEMQ_BUFSZ;

    return newpidx == cidx;
}

static u_int8_t
serial_rd_rbr(serial_t *s)
{
    u_int8_t c;

    if (memq_getc(s->rxq, &c)) {
        return c;
    }
    return 0;
}

static int
serial_wr_thr(serial_t *s, const u_int8_t c)
{
    if (!memq_putc(s->txq, c)) {
        pciesvc_logerror("wr_thr: memq_putc failed\n");
        return 0;
    }
    return 1;
}

static void
tx_fifo_reset(serial_t *s)
{
    serial_state_t *st = s->state;

    /* seriald detects generation change and resets cidx */
    st->gen++;
}

static void
rx_fifo_reset(serial_t *s)
{
    serial_state_t *st = s->state;

    st->rxq.cidx = st->rxq.pidx;
}

/**
 * serial_rxq_empty:
 * @s: serial struct
 *
 * Returns: true if the receive queue is empty otherwise false.
 */
static int
serial_rxq_empty(serial_t *s)
{
    volatile serial_state_t *st = s->state;

    return st->rxq.cidx == st->rxq.pidx;
}

/**
 * uart_reset:
 * @s: serial struct
 *
 * Set uart state to power on default settings
 */
static void
uart_reset(serial_t *s)
{
    uart_state_t *uart = s->uart;

    uart->rbr = 0;
    uart->ier = 0;
    uart->iir = UART_IIR_NO_INT;
    uart->lcr = 0;
    uart->lsr = UART_LSR_TEMT | UART_LSR_THRE;
    uart->msr = UART_MSR_DCD | UART_MSR_DSR | UART_MSR_CTS;
    uart->scr = 0;
    uart->divider = 0x0c;    /* default 9600 baud 8-N-1 */
    uart->mcr = UART_MCR_RTS | UART_MCR_DTR;
    uart->thr_ipending = ((uart->iir & UART_IIR_ID) == UART_IIR_THRI);
    uart->flags = CHR_TIOCM_CAR;

    serial_update_msl(s);
    uart->msr &= ~UART_MSR_ANY_DELTA;
}

/**
 * uart_write_fcr:
 * @s: serial struct
 * @val: register value
 *
 * Write fifo control register and interrupt identification
 * register receive byte interrupt threshold.
 */
static void
uart_write_fcr(serial_t *s, uint8_t val)
{
    uart_state_t *uart = s->uart;

    uart->fcr = val & 0xc9;

    if (uart->fcr & UART_FCR_FE) {
        uart->iir |= UART_IIR_FE;
        /* Set recv_fifo trigger Level */
        switch (val & 0xc0) {
        case UART_FCR_ITL_1:
            uart->recv_fifo_itl = 1;
            break;
        case UART_FCR_ITL_2:
            uart->recv_fifo_itl = 4;
            break;
        case UART_FCR_ITL_3:
            uart->recv_fifo_itl = 8;
            break;
        case UART_FCR_ITL_4:
            uart->recv_fifo_itl = 14;
            break;
        }
    } else {
        uart->iir &= ~UART_IIR_FE;
    }
}

/**
 * uart_update_parameters:
 * @s: serial struct
 *
 * Set uart settings based on line control register.
 */
static void
uart_update_parameters(serial_t *s)
{
    uart_state_t *uart = s->uart;
    int parity, data_bits, stop_bits;

    /* Start bit */
    if (uart->lcr & 0x08) {
        /* Parity bit. */
        if (uart->lcr & 0x10)
            parity = 'E';
        else
            parity = 'O';
    } else {
        parity = 'N';
    }
    if (uart->lcr & 0x04) {
        stop_bits = 2;
    } else {
        stop_bits = 1;
    }

    data_bits = (uart->lcr & 0x03) + 5;
    uart->parity = parity;
    uart->data_bits = data_bits;
    uart->stop_bits = stop_bits;
}

/**
 * uart_update_irq:
 * @s: serial struct
 *
 * Emulate interrupt identification register.
 */
static void
uart_update_irq(serial_t *s)
{
    uart_state_t *uart = s->uart;
    volatile serial_state_t *st = s->state;
    uint8_t tmp_iir = UART_IIR_NO_INT;

    if ((uart->ier & UART_IER_RLSI) && (uart->lsr & UART_LSR_INT_ANY)) {
        tmp_iir = UART_IIR_RLSI;
    } else if ((uart->ier & UART_IER_RDI) && (uart->lsr & UART_LSR_DR) &&
               (!(uart->fcr & UART_FCR_FE) || !serial_rxq_empty(s))) {
        tmp_iir = UART_IIR_RDI;
    } else if ((uart->ier & UART_IER_THRI) && uart->thr_ipending) {
        tmp_iir = UART_IIR_THRI;
    } else if ((uart->ier & UART_IER_MSI) && (uart->msr & UART_MSR_ANY_DELTA)) {
        tmp_iir = UART_IIR_MSI;
    }

    uart->iir = tmp_iir | (uart->iir & 0xf0);

    if (tmp_iir != UART_IIR_NO_INT) {
        intr_assert(st->intrb);                 /* raise interrupt */
    } else {
        intr_deassert(st->intrb);               /* lower interrupt */
    }
}

static void
serial_update_tiocm(serial_t *s)
{
    uart_state_t *uart = s->uart;

    /* Clear flags and set to match modem control */
    uart->flags &= ~(CHR_TIOCM_RTS | CHR_TIOCM_DTR);

    if (uart->mcr & UART_MCR_RTS) {
        uart->flags |= CHR_TIOCM_RTS;
    }
    if (uart->mcr & UART_MCR_DTR) {
        uart->flags |= CHR_TIOCM_DTR;
    }
}

static void
serial_update_msl(serial_t *s)
{
    uart_state_t *uart = s->uart;
    uint8_t omsr = uart->msr;
    int flags = uart->flags;

    uart->msr = (flags & CHR_TIOCM_CTS) ?
                uart->msr | UART_MSR_CTS : uart->msr & ~UART_MSR_CTS;
    uart->msr = (flags & CHR_TIOCM_DSR) ?
                uart->msr | UART_MSR_DSR : uart->msr & ~UART_MSR_DSR;
    uart->msr = (flags & CHR_TIOCM_CAR) ?
                uart->msr | UART_MSR_DCD : uart->msr & ~UART_MSR_DCD;
    uart->msr = (flags & CHR_TIOCM_RI) ?
                uart->msr | UART_MSR_RI : uart->msr & ~UART_MSR_RI;

    if (uart->msr != omsr) {
         /* Set delta bits */
         uart->msr = uart->msr | ((uart->msr >> 4) ^ (omsr >> 4));
         /* UART_MSR_TERI only if change was from 1 -> 0 */
         if ((uart->msr & UART_MSR_TERI) && !(omsr & UART_MSR_RI))
             uart->msr &= ~UART_MSR_TERI;
         uart_update_irq(s);
    }
}

/**
 * uart_xmit:
 * @st: serial struct
 *
 * Transmit bytes to memq
 */
static void
uart_xmit(serial_t *s)
{
    uart_state_t *uart = s->uart;

    if (uart->mcr & UART_MCR_LOOP) {
        /* Loopback mode, copy holding reg thr to receive reg rbr */
        uart->rbr = uart->thr;
        uart->lsr |= UART_LSR_THRE;    /* tx holding empty */
        uart->lsr |= UART_LSR_DR;      /* rx data ready */

        /* Add to rx queue in loopback */
        memq_putc(s->rxq, uart->thr);
        uart_update_irq(s);
    } else {
        if (!memq_full(s->txq)) {
            serial_wr_thr(s, uart->thr);
        }
    }

    if ((uart->lsr & UART_LSR_THRE) && !uart->thr_ipending) {
        uart->thr_ipending = 1;
        uart_update_irq(s);
    }

    uart->lsr |= UART_LSR_TEMT;
    uart->thr_ipending = 0;
}

/**
 * extract32:
 * @value: the value to extract the bit field from
 * @start: the lowest bit in the bit field (numbered from 0)
 * @length: the length of the bit field
 *
 * Extract from the 32 bit input @value the bit field specified by the
 * @start and @length parameters, and return it. The bit field must
 * lie entirely within the 32 bit word. It is valid to request that
 * all 32 bits are returned (ie @length 32 and @start 0).
 *
 * Returns: the value of the bit field extracted from the input value.
 */
static inline uint32_t extract32(uint32_t value, int start, int length)
{
    pciesvc_assert(start >= 0 && length > 0 && length <= 32 - start);
    return (value >> start) & (~0U >> (32 - length));
}

/**
 * extract16:
 * @value: the value to extract the bit field from
 * @start: the lowest bit in the bit field (numbered from 0)
 * @length: the length of the bit field
 *
 * Extract from the 16 bit input @value the bit field specified by the
 * @start and @length parameters, and return it. The bit field must
 * lie entirely within the 16 bit word. It is valid to request that
 * all 16 bits are returned (ie @length 16 and @start 0).
 *
 * Returns: the value of the bit field extracted from the input value.
 */
static inline uint16_t extract16(uint16_t value, int start, int length)
{
    pciesvc_assert(start >= 0 && length > 0 && length <= 16 - start);
    return extract32(value, start, length);
}

/**
 * deposit32:
 * @value: initial value to insert bit field into
 * @start: the lowest bit in the bit field (numbered from 0)
 * @length: the length of the bit field
 * @fieldval: the value to insert into the bit field
 *
 * Deposit @fieldval into the 32 bit @value at the bit field specified
 * by the @start and @length parameters, and return the modified
 * @value. Bits of @value outside the bit field are not modified.
 * Bits of @fieldval above the least significant @length bits are
 * ignored. The bit field must lie entirely within the 32 bit word.
 * It is valid to request that all 32 bits are modified (ie @length
 * 32 and @start 0).
 *
 * Returns: the modified @value.
 */
static inline uint32_t deposit32(uint32_t value, int start, int length,
                                 uint32_t fieldval)
{
    uint32_t mask;
    pciesvc_assert(start >= 0 && length > 0 && length <= 32 - start);
    mask = (~0U >> (32 - length)) << start;
    return (value & ~mask) | ((fieldval << start) & mask);
}

static serial_t *
serial_get(pciehwdev_t *phwdev)
{
    static serial_t serial;

    if (!serial.inited) {
        pciehw_shmem_t *pshmem = pciesvc_shmem_get();
        serial_uart_state_t *su =
            (serial_uart_state_t *)PSHMEM_DATA_FIELD(pshmem, serial[0]);
        serial_state_t *st = &su->serial_state;

        serial.state = st;
        serial.uart = &su->uart_state;
        serial.txq = &st->txq;
        serial.rxq = &st->rxq;

        if (st->gen == 0) {
            st->intrb = phwdev->intrb;
            st->intrc = phwdev->intrc;

            uart_reset(&serial);
            rx_fifo_reset(&serial);
            tx_fifo_reset(&serial);
        }
        serial.inited = 1;
    }
    return &serial;
}

uint64_t
serial_barrd(pciehwdev_t *phwdev,
             const u_int64_t baroff, const size_t size)
{
    serial_t *s = serial_get(phwdev);
    uart_state_t *uart = s->uart;
    uint32_t r;

    /* only byte access */
    if (size != 1) return 0;
    if (baroff >= 8) return 0;

    switch (baroff) {
    case UART_RX_BUF:
        if (uart->lcr & UART_LCR_DLAB) {
            r = extract16(uart->divider, 8 * (int)baroff, 8);
        } else {
            r = 0;
            if (uart->mcr & UART_MCR_LOOP) {
                if (!serial_rxq_empty(s)) {
                    r = serial_rd_rbr(s);
                    uart->lsr |= UART_LSR_DR;
                } else {
                    uart->lsr &= ~(UART_LSR_DR | UART_LSR_BI);
                }
            } else {
                r = serial_rd_rbr(s);
                if (uart->fcr & UART_FCR_FE) {
                    if (serial_rxq_empty(s)) {
                        uart->lsr &= ~(UART_LSR_DR | UART_LSR_BI);
                    }
                } else {
                    uart->lsr &= ~(UART_LSR_DR | UART_LSR_BI);
                }
                uart_update_irq(s);
            }
        }
        break;
    case UART_INTERRUPT_ENABLE:
        if (uart->lcr & UART_LCR_DLAB) {
            r = extract16(uart->divider, 8 * (int)baroff, 8);
        } else {
            r = uart->ier;
        }
        break;
    case UART_INTERRUPT_ID:
        if (!serial_rxq_empty(s)) {
            uart->lsr |= UART_LSR_DR;
        }
        uart_update_irq(s);

        if ((uart->iir & UART_IIR_ID) == UART_IIR_THRI) {
            /* transmit hold register is empty */
            uart->thr_ipending = 0;
            uart_update_irq(s);
        }
        r = uart->iir;
        break;
    case UART_LINE_CONTROL:
        r = uart->lcr;
        break;
    case UART_MODEM_CONTROL:
        uart->mcr_read = 1;
        if (uart->mcr_write == 0) {
            /* linux */
            uart->flags = CHR_TIOCM_CTS | CHR_TIOCM_DSR | CHR_TIOCM_CAR;
            serial_update_msl(s);
        }
        r = uart->mcr;
        break;
    case UART_LINE_STATUS:
        if (!serial_rxq_empty(s)) {
            uart->lsr |= UART_LSR_DR;
        }
        if (memq_full(s->txq)) {
            uart->lsr &= ~UART_LSR_THRE;   /* clear thr empty */
            uart->lsr &= ~UART_LSR_TEMT;   /* clear transmitter empty */
        } else {
            uart->lsr |= UART_LSR_THRE;    /* thr empty */
            uart->lsr |= UART_LSR_TEMT;    /* transmitter empty */
        }
        /* Clear break and overrun interrupts */
        if (uart->lsr & (UART_LSR_BI | UART_LSR_OE)) {
            uart->lsr &= ~(UART_LSR_BI | UART_LSR_OE);
            uart_update_irq(s);
        }
        r = uart->lsr;
        break;
    case UART_MODEM_STATUS:
        if (uart->mcr & UART_MCR_LOOP) {
            /* In loopback modem output pins are connected to the inputs */
            r = (uart->mcr & 0x0c) << 4;
            r |= (uart->mcr & 0x02) << 3;
            r |= (uart->mcr & 0x01) << 5;
        } else {
            serial_update_msl(s);
            r = uart->msr;
            /* Clear delta bits & msr int after read, if they were set */
            if (uart->msr & UART_MSR_ANY_DELTA) {
                uart->msr &= 0xf0;
                uart_update_irq(s);
            }
        }
        break;
    case UART_SCRATCH:
        r = uart->scr;
        break;
    default:
        break;
    }

    return r;
}

void
serial_barwr(pciehwdev_t *phwdev,
             const u_int64_t baroff, const size_t size, const u_int64_t val)
{
    serial_t *s = serial_get(phwdev);
    volatile serial_state_t *st = s->state;
    uart_state_t *uart = s->uart;
    uint8_t changed;
    uint8_t temp;

    /* only byte access */
    if (size != 1) return;
    if (baroff >= 8) return;

    switch (baroff) {
    case UART_TX_BUF:
        if (uart->lcr & UART_LCR_DLAB) {
            uart->divider = deposit32(uart->divider, 8 * (int)baroff, 8,
                                      (int)val);
            uart_update_parameters(s);
        } else {
            uart->thr = (uint8_t)val;
            uart->thr_ipending = 0;
            uart->lsr &= ~UART_LSR_THRE;    /* clear thr empty */
            uart->lsr &= ~UART_LSR_TEMT;    /* clear transmitter empty */
            uart_update_irq(s);
            uart_xmit(s);
        }
        break;
    case UART_INTERRUPT_ENABLE:
        if (uart->lcr & UART_LCR_DLAB) {
            uart->divider = deposit32(uart->divider, 8 * (int)baroff, 8, (int)val);
            uart_update_parameters(s);
        } else {
            changed = (uart->ier ^ val) & 0x0f;
            uart->ier = val & 0x0f;

            if (changed & UART_IER_MSI) {
                if (uart->ier & UART_IER_MSI) {
                     /*
                      * Carry over mcr RTS/DTR to msr and let
                      * serial_update_msl set the delta bits.
                      */
                     if (uart->mcr == 0xb) {
                         uart->msr = 0xb0;
                     } else if (uart->mcr == 0x3) {
                         uart->msr = 0xb0;
                     } else if (uart->mcr == 0x8) {
                         uart->msr = 0x80;
                     } else if (uart->mcr == 0x0) {
                         uart->msr = 0x80;
                     }
                     serial_update_msl(s);
                }
            }

            /* Turning on the THRE interrupt on IER can trigger the interrupt
             * if LSR.THRE=1, even if it had been masked before by reading IIR.
             * This is not in the datasheet, but Windows relies on it.  It is
             * unclear if THRE has to be resampled every time THRI becomes
             * 1, or only on the rising edge.  Bochs does the latter, and
             * Windows always toggles IER to all zeroes and back to all ones,
             * so do the same.
             *
             * If IER.THRI is zero, thr_ipending is not used.  Set it to zero
             * so that the thr_ipending subsection is not migrated.
             */
            if (changed & UART_IER_THRI) {
                if ((uart->ier & UART_IER_THRI) &&
                    (uart->lsr & UART_LSR_THRE)) {
                    uart->thr_ipending = 1;
                } else {
                    uart->thr_ipending = 0;
                }
            }

            if (changed) {
                uart_update_irq(s);
            }

            if ((uart->ier & 0xf) == 0) {
                intr_drvcfg_mask(st->intrb, 1);    /* mask */
            } else {
                intr_drvcfg_mask(st->intrb, 0);    /* unmask */
            }
        }
        break;
    case UART_FIFO_CONTROL:
        /* Flush FIFOs if enable/disable flag changed */
        temp = (uint8_t)val;
        if ((temp ^ uart->fcr) & UART_FCR_FE) {
            temp |= UART_FCR_XFR | UART_FCR_RFR;
        }

        if (temp & UART_FCR_RFR) {
            /* Reset the receive fifo */
            uart->lsr &= ~(UART_LSR_DR | UART_LSR_BI);
            rx_fifo_reset(s);
        }

        if (temp & UART_FCR_XFR) {
            /* Reset the transmit fifo */
            uart->lsr |= UART_LSR_THRE;
            uart->thr_ipending = 1;
            tx_fifo_reset(s);
        }
        uart_write_fcr(s, val);
        uart_update_irq(s);
        break;
    case UART_LINE_CONTROL:
        uart->lcr = (uint8_t)val;
        if (uart->lcr & UART_LCR_BRK) st->breakreq++;
        uart_update_parameters(s);
        break;
    case UART_MODEM_CONTROL:
        {
            int old_mcr = uart->mcr;

            uart->mcr_write = 1;
            uart->mcr = val & 0x1f;
            if (uart->mcr & UART_MCR_LOOP) {
                break;
            }

            if (old_mcr != uart->mcr) {
                serial_update_tiocm(s);
            }
        }
        break;
    case UART_LINE_STATUS:
        break;
    case UART_MODEM_STATUS:
        break;
    case UART_SCRATCH:
        uart->scr = (uint8_t)val;
        break;
    default:
        break;
    }
}

void
serial_reset(pciehwdev_t *phwdev, const pciesvc_rsttype_t rsttype)
{
    serial_t *s = serial_get(phwdev);

    /*
     * It makes some sense to do this here:
     *     uart_reset(s);
     * but it looks like the linux serial driver doesn't
     * expect the uart registers to change during FLR so doing
     * the uart_reset() will cause the serial driver not to recover.
     */
    rx_fifo_reset(s);
    tx_fifo_reset(s);
    s->state->breakreq++;
}
