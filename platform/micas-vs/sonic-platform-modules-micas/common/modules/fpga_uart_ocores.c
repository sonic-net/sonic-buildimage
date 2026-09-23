/*
 * fpga_uart_ocores.c
 * Original Author: Rd@micas.com, 2020-09-10
 * FPGA 16550 UART driver
 *
 *  Copyright (C) 2020 Micas.
 *
 * History
* v1.4 Rd@micas.com    2020-12-07
 * Fix uart buffer switch, fifo size switch, buffer size switch interface
 * v1.3 Rd@micas.com    2020-10-30
 * Fix packet losing bug, add fifo trigger interface
 * Add uart buffer function
 * v1.2 Rd@micas.com    2020-10-14
 * Code standardization
 * v1.1 Rd@micas.com    2020-09-23
 * Fix this file support baudrate 9600
 * v1.0 Rd@micas.com    2020-09-10
 * Create this file
 */

#include <linux/console.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/nmi.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/ratelimit.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/version.h>
#include <linux/serial_core.h>
#include "fpga_uart_ocores.h"

static inline struct ocores_uart* up_to_mc_uart(struct uart_port* up)
{
    return container_of(up, struct ocores_uart, port);
}

static void mem32_serial_out(struct uart_port* p, int offset, int value)
{
    offset = offset << p->regshift;
    writel(value, p->membase + offset);
}

static unsigned int mem32_serial_in(struct uart_port* p, int offset)
{
    offset = offset << p->regshift;
    return readl(p->membase + offset);
}

int g_fpga_uart_debug = 0;
int g_fpga_uart_irq = 0;
int g_fpga_uart_error = 0;
int g_irq_dump_debug = 0;
int g_debug_tx = 0;
int g_debug_irq = 0;
int g_debug_init = 0;

module_param(g_fpga_uart_debug, int, S_IRUGO | S_IWUSR);
module_param(g_fpga_uart_error, int, S_IRUGO | S_IWUSR);
module_param(g_fpga_uart_irq, int, S_IRUGO | S_IWUSR);
module_param(g_debug_tx, int, S_IRUGO | S_IWUSR);
module_param(g_irq_dump_debug, int, S_IRUGO | S_IWUSR);
module_param(g_debug_irq, int, S_IRUGO | S_IWUSR);
module_param(g_debug_init, int, S_IRUGO | S_IWUSR);

#define FPGA_UART_DEBUG_INIT(fmt, args...)                          \
    do {                                                        \
        if (g_debug_init) {                                                 \
            printk("[func:%s line:%d] "fmt, __func__,     \
                        __LINE__, ##args);                                         \
        }                                                                   \
    } while (0)

#define FPGA_UART_DEBUG_DUMP(fmt, args...)              \
    do {                                            \
        if (g_irq_dump_debug) {                             \
            printk(fmt, ##args);                \
        }                                                 \
    } while (0)

#define FPGA_DEBUG_IRQ_VERBOSE(fmt, args...)                        \
    do {                                                          \
        if (g_debug_irq) {                                  \
            printk("[FPGA_UART][VER][func:%s line:%d] "fmt, __func__,     \
                        __LINE__, ##args);                                         \
        }                                  \
    } while (0)

#define FPGA_DEBUG_TX_VERBOSE(fmt, args...)                                    \
    do {                                                     \
        if (g_debug_tx) {                                                            \
            printk("[FPGA_UART][VER][func:%s line:%d] "fmt, __func__,     \
                        __LINE__, ##args);                                         \
        }                          \
    } while (0)

#define FPGA_UART_DEBUG_VERBOSE(fmt, args...)                                 \
    do {                                                    \
        if (g_fpga_uart_debug) {                                                         \
            printk("[FPGA_UART][VER][func:%s line:%d] "fmt, __func__,     \
                        __LINE__, ##args);                                         \
        }                                      \
    } while (0)

#define FPGA_UART_DEBUG_ERROR(fmt, args...)       \
    do {                                                             \
        if (g_fpga_uart_error) {                                                                 \
            printk("[FPGA_UART][ERR][func:%s line:%d] "fmt, __func__,     \
                        __LINE__, ##args);                                         \
        }                                 \
    } while (0)

/*
 * Software RTS debounce fallback for RX flow-control:
 * - Trigger block on RX data / RX timeout IRQ.
 * - Release immediately once RX FIFO is observed empty.
 */
struct mc_sw_rts_state {
    bool enabled;
    bool blocked;
    struct ocores_uart *uart;
};

static struct mc_sw_rts_state g_mc_sw_rts[TTYMC_NUM];
static void oc_debug_dump_reg(struct ocores_uart* uart);
static void mc_uart_register_dev_spec_attr_grp(struct ocores_uart *up);
static void mc_uart_set_mctrl(struct uart_port* port, unsigned int mctrl);

static struct mc_sw_rts_state *mc_sw_rts_get_state(struct ocores_uart *uart)
{
    int line = uart->port.line;

    if (line < 0 || line >= TTYMC_NUM) {
        return NULL;
    }

    return &g_mc_sw_rts[line];
}

/* Caller must hold port->lock */
static void mc_sw_rts_apply_locked(struct ocores_uart *uart, bool allow_send)
{
    unsigned int old_mctrl = uart->port.mctrl;
    unsigned int mctrl = old_mctrl;

    if (allow_send) {
        mctrl |= TIOCM_RTS;
    } else {
        mctrl &= ~TIOCM_RTS;
    }

    if (mctrl != old_mctrl) {
        uart->port.mctrl = mctrl;
        mc_uart_set_mctrl(&uart->port, uart->port.mctrl);
    }
}

/* Caller must hold port->lock */
static void mc_sw_rts_try_release_locked(struct ocores_uart *uart,
                                        struct mc_sw_rts_state *state)
{
    unsigned char lsr;

    if (!state || !state->enabled || !state->blocked) {
        return;
    }

    lsr = serial_in(uart, UART_LSR);
    if (lsr & UART_LSR_DR) {
        return;
    }

    mc_sw_rts_apply_locked(uart, true);
    state->blocked = false;
}

/* Caller must hold port->lock */
static void mc_sw_rts_block_locked(struct ocores_uart *uart,
                                        struct mc_sw_rts_state *state)
{
    if (!state || !state->enabled) {
        return;
    }

    if (!state->blocked) {
        state->blocked = true;
        mc_sw_rts_apply_locked(uart, false);
    }
}

/* Caller must hold port->lock */
static void mc_sw_rts_set_mode_locked(struct ocores_uart *uart, bool enable)
{
    struct mc_sw_rts_state *state = mc_sw_rts_get_state(uart);

    if (!state) {
        return;
    }

    state->enabled = enable;
    state->blocked = false;

    mc_sw_rts_apply_locked(uart, true);
}

static void mc_sw_rts_init(struct ocores_uart *uart)
{
    struct mc_sw_rts_state *state = mc_sw_rts_get_state(uart);

    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->uart = uart;
}

static void mc_sw_rts_cleanup(struct ocores_uart *uart)
{
    struct mc_sw_rts_state *state = mc_sw_rts_get_state(uart);

    if (!state) {
        return;
    }

    state->enabled = false;
    state->blocked = false;
    state->uart = NULL;
}

/* Buffer Status
 * 1. Empty
 * 2. Buffered but not full
 * 3. Full
 * While the Buffer is full or empty, the head and tail printor of
 * Buffer would equals to each other.
 * Then the status of Buffer could change to full while buffering data only,
 * and it could change to empty while xmit data only as well.
 */

static void mc_uart_tx_buffer_chars(struct ocores_uart *uart)
{
    int count;
    unsigned char ch;
    struct uart_port *port;
    char flag;

    count = 1;
    flag = TTY_NORMAL;
    port = &uart->port;

    while (serial_get_buffer_status(uart->port.line) != BUFFER_EMPTY) {
        serial_buffer_read(uart->port.line, &ch);
        tty_insert_flip_char(&uart->port.state->port, ch, flag);
        FPGA_DEBUG_TX_VERBOSE( "BUFFER TX: %x\n", ch);
        count++;
        if (count % UART_FIFO_MAX_COUNT == 0) {
            tty_flip_buffer_push(&uart->port.state->port);
        }
    }
}

static void mc_buffer_send_all(struct ocores_uart *uart)
{
    mc_uart_tx_buffer_chars(uart);
}

/*
 * These two wrappers ensure that enable_runtime_pm_tx() can be called more than
 * once and disable_runtime_pm_tx() will still disable RPM because the fifo is
 * empty and the HW can idle again.
 */
static void mc_uart_rpm_get_tx(struct ocores_uart* p)
{
    unsigned char rpm_active;

    if (!(p->capabilities & UART_CAP_RPM)) {
        return;
    }

    rpm_active = xchg(&p->rpm_tx_active, 1);
    if (rpm_active) {
        return;
    }
    pm_runtime_get_sync(p->port.dev);
}

static void mc_uart_rpm_get(struct ocores_uart* p)
{
    if (!(p->capabilities & UART_CAP_RPM)) {
        return;
    }
    pm_runtime_get_sync(p->port.dev);
}

static void mc_uart_rpm_put(struct ocores_uart* p)
{
    if (!(p->capabilities & UART_CAP_RPM)) {
        return;
    }
    pm_runtime_mark_last_busy(p->port.dev);
    pm_runtime_put_autosuspend(p->port.dev);
}

static void mc_uart_rpm_put_tx(struct ocores_uart* p)
{
    unsigned char rpm_active;

    if (!(p->capabilities & UART_CAP_RPM)) {
        return;
    }

    rpm_active = xchg(&p->rpm_tx_active, 0);
    if (!rpm_active) {
        return;
    }
    pm_runtime_mark_last_busy(p->port.dev);
    pm_runtime_put_autosuspend(p->port.dev);
}

static inline void mc_stop_tx(struct ocores_uart* uart)
{
    if (uart->ier & UART_IER_THRI) {
        uart->ier &= ~UART_IER_THRI;
        serial_out(uart, UART_IER, uart->ier);
        mc_uart_rpm_put_tx(uart);
    }
}

static void mc_uart_stop_tx(struct uart_port* port)
{
    struct ocores_uart* uart;
    FPGA_UART_DEBUG_INIT("mc_uart_stop_tx\n");

    uart = up_to_mc_uart(port);
    mc_uart_rpm_get(uart);
    mc_stop_tx(uart);
    mc_uart_rpm_put(uart);
}

/* Caller holds uart port lock */
unsigned int mc_uart_modem_status(struct ocores_uart* up)
{
    struct uart_port* port;
    unsigned int status;

    port = &up->port;
    status = serial_in(up, UART_MSR);
    status |= up->msr_saved_flags;
    up->msr_saved_flags = 0;
    if ((status & UART_MSR_ANY_DELTA) && (up->ier & UART_IER_MSI) && (port->state != NULL)) {
        if (status & UART_MSR_TERI) {
            port->icount.rng++;
        }
        if (status & UART_MSR_DDSR) {
            port->icount.dsr++;
        }
        if (status & UART_MSR_DDCD){
            uart_handle_dcd_change(port, status & UART_MSR_DCD);
        }
        if (status & UART_MSR_DCTS) {
            uart_handle_cts_change(port, status & UART_MSR_CTS);
        }
        wake_up_interruptible(&port->state->port.delta_msr_wait);
    }

    return status;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static void mc_uart_tx_chars(struct ocores_uart* up)
{
    struct uart_port* port;
    struct circ_buf* xmit;
    int count;
    int leftsize;

    port = &up->port;
    xmit = &port->state->xmit;
    leftsize = 0;
    if (port->x_char) {
        serial_out(up, UART_TX, port->x_char);
        port->icount.tx++;
        port->x_char = 0;
        return;
    }
    if (uart_tx_stopped(port)) {
        mc_uart_stop_tx(port);
        return;
    }
    if (uart_circ_empty(xmit)) {
        mc_stop_tx(up);
        return;
    }
    count = up->tx_loadsz;
    do {
        FPGA_DEBUG_TX_VERBOSE("TTYMC%d TX: %x\n", up->port.line, xmit->buf[xmit->tail]);
        serial_out(up, UART_TX, xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;
        if (uart_circ_empty(xmit)) {
            break;
        }
    } while (--count > 0);
    leftsize = uart_circ_chars_pending(xmit);
    FPGA_DEBUG_TX_VERBOSE("TTYMC%d leftsize: %d\n", up->port.line, leftsize);
    if (leftsize < WAKEUP_CHARS) {
        uart_write_wakeup(&up->port);
    }
    if (uart_circ_empty(xmit) && !(up->capabilities & UART_CAP_RPM)) {
        mc_stop_tx(up);
    }
}
#else
static void mc_uart_tx_chars(struct ocores_uart *up)
{
    struct uart_port *port = &up->port;
    struct tty_port *tp;
    int count;
    int leftsize;
    u8 ch;

    if (unlikely(!port->state)) {
        mc_stop_tx(up);
        return;
    }

    tp = &port->state->port;

    if (port->x_char) {
        serial_out(up, UART_TX, port->x_char);
        port->icount.tx++;
        port->x_char = 0;
        return;
    }

    if (uart_tx_stopped(port)) {
        mc_uart_stop_tx(port);
        return;
    }

    count = up->tx_loadsz;
    do {
        if (!kfifo_get(&tp->xmit_fifo, &ch)) {
            break;
        }

        serial_out(up, UART_TX, ch);
        port->icount.tx++;
    } while (--count > 0);

    leftsize = kfifo_len(&tp->xmit_fifo);
    FPGA_DEBUG_TX_VERBOSE("TTYMC%d leftsize: %d\n", up->port.line, leftsize);
    if (leftsize < WAKEUP_CHARS) {
        uart_write_wakeup(port);
    }

    if (kfifo_is_empty(&tp->xmit_fifo) && !(up->capabilities & UART_CAP_RPM)) {
        mc_stop_tx(up);
    }
}
#endif

static void mc_uart_read_char(struct ocores_uart* uart, unsigned char lsr)
{
    struct uart_port* port;
    unsigned char ch;
    char flag;
    int ret;

    port = &uart->port;
    flag = TTY_NORMAL;
    if (likely(lsr & UART_LSR_DR)) {
        ch = serial_in(uart, UART_RX);
    } else {
        ch = 0;
    }

    port->icount.rx++;

    lsr |= uart->lsr_saved_flags;
    uart->lsr_saved_flags = 0;

    if (unlikely(lsr & UART_LSR_BRK_ERROR_BITS)) {
        if (lsr & UART_LSR_BI) {
            lsr &= ~(UART_LSR_FE | UART_LSR_PE);
            port->icount.brk++;

            if (uart_handle_break(port)) {
                return;
            }
        } else if (lsr & UART_LSR_PE) {
            port->icount.parity++;
        } else if (lsr & UART_LSR_FE) {
            port->icount.frame++;
        }

        if (lsr & UART_LSR_OE) {
            port->icount.overrun++;
        }
        lsr &= uart->port.read_status_mask;
        if (lsr & UART_LSR_BI) {
            pr_debug("%s: handling break\n", __func__);
            flag = TTY_BREAK;
        } else if (lsr & UART_LSR_PE) {
            flag = TTY_PARITY;
        } else if (lsr & UART_LSR_FE) {
            flag = TTY_FRAME;
        }
    }
    if (uart_handle_sysrq_char(port, ch)) {
        return;
    }
    if (uart->mc_uart_buffer_switch == MC_UART_BUFFER_ON
            && uart->mc_uart_status == MC_UART_CLOSE) {
        ret = serial_buffer_write(uart->port.line, ch);
        if (ret) {
            FPGA_DEBUG_IRQ_VERBOSE("line %d buffer error.\n", uart->port.line);
        }
    } else {
        uart_insert_char(&uart->port, lsr, UART_LSR_OE, ch, flag);
    }
}

static unsigned char mc_uart_rx_chars(struct ocores_uart* uart, unsigned char lsr)
{
    int max_count;

    max_count = UART_FIFO_MAX_COUNT;
    FPGA_UART_DEBUG_INIT("mc_uart_rx_chars \n");
    do {
        mc_uart_read_char(uart, lsr);
        if (--max_count == 0) {
            break;
        }
        lsr = serial_in(uart, UART_LSR);
    } while (lsr & (UART_LSR_DR | UART_LSR_BI));

    FPGA_DEBUG_IRQ_VERBOSE("rx num %d.\n", (UART_FIFO_MAX_COUNT - max_count));
    tty_flip_buffer_push(&uart->port.state->port);
    return lsr;
}

static int mc_uart_handle_irq(struct uart_port* port, unsigned int iir)
{
    unsigned char status;
    unsigned int iir_id;
    unsigned long flags;
    bool rx_flow_irq;
    struct ocores_uart* uart;
    struct mc_sw_rts_state *sw_rts;

    uart = up_to_mc_uart(port);
    if (iir & 0x01) {
        return 0;
    }

    iir_id = iir & UART_IIR_ID;
    rx_flow_irq = (iir_id == UART_IIR_RDI || iir_id == UART_IIR_RX_TIMEOUT);
    sw_rts = mc_sw_rts_get_state(uart);

    spin_lock_irqsave(&uart->port.lock, flags);
    status = serial_in(uart, UART_LSR);
    if ((status & (UART_LSR_DR | UART_LSR_BI))) {
        status = mc_uart_rx_chars(uart, status);
    }

    /*
     * Trigger flow control only when RX pressure remains after the read pass.
     * This avoids unnecessary RTS toggles when FIFO is drained in time.
     */
    if (sw_rts && sw_rts->enabled && rx_flow_irq && (status & UART_LSR_DR)) {
        mc_sw_rts_block_locked(uart, sw_rts);
    }

    mc_uart_modem_status(uart);
    if (status & UART_LSR_THRE) {
        mc_uart_tx_chars(uart);
    }
    if (sw_rts && sw_rts->enabled && sw_rts->blocked) {
        mc_sw_rts_try_release_locked(uart, sw_rts);
    }

    spin_unlock_irqrestore(&uart->port.lock, flags);
    FPGA_DEBUG_IRQ_VERBOSE("tty:%d    ISR DONE LSR:%x.\n", uart->port.line, status);
    return 1;
}

static irqreturn_t ocores_isr(int irq, void* dev_id)
{
    struct ocores_uart* uart;
    unsigned int iir;
    int ret;

    uart = (struct ocores_uart*)dev_id;

    mc_uart_rpm_get(uart);
    iir = serial_in(uart, UART_IIR);
    ret = mc_uart_handle_irq(&uart->port, iir);

    mc_uart_rpm_put(uart);

    return ret;
}

/* Dump UART info & regs */
static void oc_debug_dump_reg(struct ocores_uart* uart)
{
    if (uart) {
        FPGA_UART_DEBUG_DUMP("TTYMC%d:\n", uart->port.line);
        FPGA_UART_DEBUG_DUMP("base: %p.\n", uart->port.membase);
        FPGA_UART_DEBUG_DUMP("baud_rate: %d.\n", uart->baud_rate);
        FPGA_UART_DEBUG_DUMP("clk: %d.\n", uart->clk);
        FPGA_UART_DEBUG_DUMP("xmit_size: %d.\n", uart->xmit_size);

        FPGA_UART_DEBUG_DUMP("RX: 0x%x.\n", serial_in(uart, UART_RX));
        FPGA_UART_DEBUG_DUMP("IER: 0x%x.\n", serial_in(uart, UART_IER));
        FPGA_UART_DEBUG_DUMP("IIR: 0x%x.\n", serial_in(uart, UART_IIR));
        FPGA_UART_DEBUG_DUMP("LCR: 0x%x.\n", serial_in(uart, UART_LCR));
        FPGA_UART_DEBUG_DUMP("MCR: 0x%x.\n", serial_in(uart, UART_MCR));
        FPGA_UART_DEBUG_DUMP("LSR: 0x%x.\n", serial_in(uart, UART_LSR));
        serial_out(uart, UART_LCR, serial_in(uart, UART_LCR) | DLAB);
        FPGA_UART_DEBUG_DUMP("DLL: 0x%x.\n", serial_in(uart, UART_DLL));
        FPGA_UART_DEBUG_DUMP("DLH: 0x%x.\n", serial_in(uart, UART_DLM));
        FPGA_UART_DEBUG_DUMP("FCR: 0x%x.\n", serial_in(uart, UART_FCR));
        serial_out(uart, UART_LCR, serial_in(uart,  UART_LCR) & ~DLAB);
    } else {
        FPGA_UART_DEBUG_DUMP("uart %p is null.\n", uart);
    }
} /* End of Dump */

static unsigned int mc_uart_tx_empty(struct uart_port* port)
{
    struct ocores_uart* uart;
    unsigned long flags;
    unsigned int lsr;

    uart = up_to_mc_uart(port);
    FPGA_UART_DEBUG_INIT("mc_uart_tx_empty \n");
    mc_uart_rpm_get(uart);

    spin_lock_irqsave(&port->lock, flags);
    lsr = serial_in(uart, UART_LSR);
    uart->lsr_saved_flags |= lsr & LSR_SAVE_FLAGS;
    spin_unlock_irqrestore(&port->lock, flags);

    mc_uart_rpm_put(uart);

    return (lsr & BOTH_EMPTY) == BOTH_EMPTY ? TIOCSER_TEMT : 0;
}

static void mc_uart_flush_buffer(struct uart_port* port)
{
    FPGA_UART_DEBUG_INIT("mc_uart_flush_buffer \n");
    return;
}

static void mc_uart_set_mctrl(struct uart_port* port, unsigned int mctrl)
{
    struct ocores_uart* uart;
    unsigned char mcr;

    mcr = 0;
    uart = up_to_mc_uart(port);
    FPGA_UART_DEBUG_INIT("before mc_uart_set_mctrl: %x \n", mctrl);
    if (mctrl & TIOCM_RTS) {
        mcr |= UART_MCR_RTS;
    }
    if (mctrl & TIOCM_DTR) {
        mcr |= UART_MCR_DTR;
    }
    if (mctrl & TIOCM_OUT1) {
        mcr |= UART_MCR_OUT1;
    }
    if (mctrl & TIOCM_OUT2) {
        mcr |= UART_MCR_OUT2;
    }
    if (mctrl & TIOCM_LOOP) {
        mcr |= UART_MCR_LOOP;
    }

    mcr = (mcr & uart->mcr_mask) | uart->mcr_force | uart->mcr;
    FPGA_UART_DEBUG_INIT("after mc_uart_set_mctrl %x \n", mcr);
    mc_uart_out_mcr(uart, mcr);
    return (void)0;
}

static unsigned int mc_uart_get_mctrl(struct uart_port* port)
{
    struct ocores_uart* up;
    unsigned int status;
    unsigned int ret;

    up = up_to_mc_uart(port);
    mc_uart_rpm_get(up);
    status = mc_uart_modem_status(up);
    mc_uart_rpm_put(up);

    ret = 0;
    if (status & UART_MSR_DCD) {
        ret |= TIOCM_CAR;
    }
    if (status & UART_MSR_RI) {
        ret |= TIOCM_RNG;
    }
    if (status & UART_MSR_DSR) {
        ret |= TIOCM_DSR;
    }
    if (status & UART_MSR_CTS) {
        ret |= TIOCM_CTS;
    }
    return ret;
}

static void mc_uart_start_tx(struct uart_port* port)
{
    struct ocores_uart* up;

    up = up_to_mc_uart(port);
    FPGA_UART_DEBUG_INIT("mc_uart_start_tx \n");
    mc_uart_rpm_get_tx(up);
    FPGA_DEBUG_TX_VERBOSE("up->ier:%d\n", up->ier);

    if (!(up->ier & UART_IER_THRI)) {
        up->ier |= UART_IER_THRI;
        FPGA_DEBUG_TX_VERBOSE("after set up->ier:%d\n", up->ier);
        serial_out(up, UART_IER, up->ier);
        if (up->bugs & UART_BUG_TXEN) {
                unsigned char lsr;
                lsr = serial_in(up, UART_LSR);
                FPGA_DEBUG_TX_VERBOSE("lsr:%x\n", lsr);
                up->lsr_saved_flags |= lsr & LSR_SAVE_FLAGS;
                if (lsr & UART_LSR_THRE) {
                    mc_uart_tx_chars(up);
                }
        }
    }
}

static void mc_uart_stop_rx(struct uart_port* port)
{
    struct ocores_uart* uart;

    uart = up_to_mc_uart(port);
    FPGA_UART_DEBUG_VERBOSE("TTYMC%d stop_rx\n", port->line);
    FPGA_UART_DEBUG_INIT("mc_uart_stop_rx \n");
    mc_uart_rpm_get(uart);
    if (uart->mc_uart_buffer_switch == MC_UART_BUFFER_OFF) {
        uart->ier &= ~(UART_IER_RLSI | UART_IER_RDI);
        uart->port.read_status_mask &= ~UART_LSR_DR;
        serial_out(uart, UART_IER, uart->ier);
    }
    mc_uart_rpm_put(uart);
}

static void mc_uart_enable_ms(struct uart_port* port)
{
    FPGA_UART_DEBUG_INIT("mc_uart_enable_ms \n");
    return;
}

static void mc_uart_break_ctl(struct uart_port* port, int ctl)
{
    FPGA_UART_DEBUG_INIT("mc_uart_break_ctl \n");
    return;
}

/*
 * FIFO support.
 */
static void mc_uart_clear_fifos(struct ocores_uart* uart)
{
    if (uart->capabilities & UART_CAP_FIFO) {
        mc_uart_out_fcr(uart, UART_FCR_ENABLE_FIFO);
        mc_uart_out_fcr(uart, UART_FCR_ENABLE_FIFO | UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
        mc_uart_out_fcr(uart, 0);
    }
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int mc_uart_startup(struct uart_port* port)
{
    struct ocores_uart* uart;
    unsigned long flags;
    unsigned char lsr, iir;
    struct circ_buf* xmit;
    int retval;

    uart = up_to_mc_uart(port);
    xmit = &port->state->xmit;
    FPGA_UART_DEBUG_INIT("start:%d \n", port->line);

    if (!uart_circ_empty(xmit)) {
        uart_circ_clear(xmit);
    }
    uart->mc_uart_status = MC_UART_START;
    uart->mcr = 0;
    mc_uart_clear_fifos(uart);

    serial_in(uart, UART_LSR);
    serial_in(uart, UART_RX);
    serial_in(uart, UART_IIR);
    serial_in(uart, UART_MSR);

    /*
     * At this point, there's no way the LSR could still be 0xff;
     * if it is, then bail out, because there's likely no UART
     * here.
     */
    if (!(port->flags & UPF_BUGGY_UART) && (serial_port_in(port, UART_LSR) == UART_LSR_BAIL_OUT)) {
        printk_ratelimited("ttyS: LSR safety check engaged!\n");
        retval = -ENODEV;
        goto out;
    }
    /* disable loopback, set uart data format, reset registers */
    serial_out(uart, UART_MCR, DIS_LOOPBACK);

    spin_lock_irqsave(&port->lock, flags);
    serial_out(uart, UART_LCR, UART_LCR_WLEN8);
    uart->port.mctrl |= TIOCM_OUT2;
    mc_uart_set_mctrl(port, port->mctrl);

    serial_out(uart, UART_IER, UART_IER_THRI);
    lsr = serial_in(uart, UART_LSR);
    iir = serial_in(uart, UART_IIR);
    serial_out(uart, UART_IER, 0);

    if (lsr & UART_LSR_TEMT && iir & UART_IIR_NO_INT) {
        if (!(uart->bugs & UART_BUG_TXEN)) {
            uart->bugs |= UART_BUG_TXEN;
            dev_dbg(port->dev, "ttyS - enabling bad tx status workarounds\n");
        }
    } else {
        uart->bugs &= ~UART_BUG_TXEN;
    }

    spin_unlock_irqrestore(&port->lock, flags);

    /* clear interrupts */

    serial_in(uart, UART_LSR);
    serial_in(uart, UART_RX);
    serial_in(uart, UART_IIR);
    serial_in(uart, UART_MSR);
    uart->lsr_saved_flags = 0;
    uart->msr_saved_flags = 0;

    uart->ier = UART_IER_RLSI | UART_IER_RDI;
    retval = 0;
out:
    mc_uart_rpm_put(uart);
    mc_buffer_send_all(uart);
    oc_debug_dump_reg(uart);
    return retval;
}
#else
static int mc_uart_startup(struct uart_port* port)
{
    struct ocores_uart* uart;
    unsigned long flags;
    unsigned char lsr, iir;
    int retval;

    uart = up_to_mc_uart(port);
    FPGA_UART_DEBUG_INIT("start:%d \n", port->line);

    if (!kfifo_is_empty(&port->state->port.xmit_fifo)) {
        kfifo_reset(&port->state->port.xmit_fifo);
    }
    uart->mc_uart_status = MC_UART_START;
    uart->mcr = 0;
    mc_uart_clear_fifos(uart);

    serial_in(uart, UART_LSR);
    serial_in(uart, UART_RX);
    serial_in(uart, UART_IIR);
    serial_in(uart, UART_MSR);

    /*
     * At this point, there's no way the LSR could still be 0xff;
     * if it is, then bail out, because there's likely no UART
     * here.
     */
    if (!(port->flags & UPF_BUGGY_UART) && (serial_port_in(port, UART_LSR) == UART_LSR_BAIL_OUT)) {
        printk_ratelimited("ttyS: LSR safety check engaged!\n");
        retval = -ENODEV;
        goto out;
    }
    /* disable loopback, set uart data format, reset registers */
    serial_out(uart, UART_MCR, DIS_LOOPBACK);

    spin_lock_irqsave(&port->lock, flags);
    serial_out(uart, UART_LCR, UART_LCR_WLEN8);
    uart->port.mctrl |= TIOCM_OUT2;
    mc_uart_set_mctrl(port, port->mctrl);

    serial_out(uart, UART_IER, UART_IER_THRI);
    lsr = serial_in(uart, UART_LSR);
    iir = serial_in(uart, UART_IIR);
    serial_out(uart, UART_IER, 0);

    if (lsr & UART_LSR_TEMT && iir & UART_IIR_NO_INT) {
        if (!(uart->bugs & UART_BUG_TXEN)) {
            uart->bugs |= UART_BUG_TXEN;
            dev_dbg(port->dev, "ttyS - enabling bad tx status workarounds\n");
        }
    } else {
        uart->bugs &= ~UART_BUG_TXEN;
    }

    spin_unlock_irqrestore(&port->lock, flags);

    /* clear interrupts */

    serial_in(uart, UART_LSR);
    serial_in(uart, UART_RX);
    serial_in(uart, UART_IIR);
    serial_in(uart, UART_MSR);
    uart->lsr_saved_flags = 0;
    uart->msr_saved_flags = 0;

    uart->ier = UART_IER_RLSI | UART_IER_RDI;
    retval = 0;
out:
    mc_uart_rpm_put(uart);
    mc_buffer_send_all(uart);
    oc_debug_dump_reg(uart);
    return retval;
}
#endif

static void mc_uart_shutdown(struct uart_port* port)
{
    unsigned long flags;
    struct ocores_uart* uart;

    uart = up_to_mc_uart(port);
    FPGA_UART_DEBUG_VERBOSE("TTY%d shutdown\n", port->line);

    spin_lock_irqsave(&port->lock, flags);
    mc_sw_rts_set_mode_locked(uart, false);
    spin_unlock_irqrestore(&port->lock, flags);

    if (uart->mc_uart_buffer_switch == MC_UART_BUFFER_OFF) {
        spin_lock_irqsave(&port->lock, flags);
        uart->ier = 0;
        serial_out(uart, UART_IER, uart->ier);
        spin_unlock_irqrestore(&port->lock, flags);

        synchronize_irq(port->irq);

        spin_lock_irqsave(&port->lock, flags);
        port->mctrl &= ~TIOCM_OUT2;
        mc_uart_set_mctrl(port, port->mctrl);
        spin_unlock_irqrestore(&port->lock, flags);

        /*
        * Disable break condition and FIFOs
        */
        serial_out(uart, UART_LCR, serial_in(uart, UART_LCR) & ~UART_LCR_SBC);
        mc_uart_clear_fifos(uart);

        serial_in(uart, UART_RX);
        mc_uart_rpm_put(uart);
    }

    uart->mc_uart_status = MC_UART_CLOSE;
}

static void mc_uart_release_port(struct uart_port* port)
{
    struct platform_device *pdev;
    int size;

    pdev = to_platform_device(port->dev);
    FPGA_UART_DEBUG_INIT("TTYMC%d release port\n", port->line);
    size = resource_size(platform_get_resource(pdev, IORESOURCE_MEM, 0));
    if (port->flags & UPF_IOREMAP) {
        iounmap(port->membase);
        port->membase = NULL;
    }
    return;
}

static int mc_uart_request_port(struct uart_port* port)
{
    return 0;
}

static unsigned char mc_uart_compute_lcr(struct ocores_uart* up, tcflag_t c_cflag)
{
    unsigned char cval;

    switch (c_cflag & CSIZE) {
        case CS5:
            cval = UART_LCR_WLEN5;
            break;
        case CS6:
            cval = UART_LCR_WLEN6;
            break;
        case CS7:
            cval = UART_LCR_WLEN7;
            break;
        case CS8:
        default:
            cval = UART_LCR_WLEN8;
            break;
    }

    if (c_cflag & CSTOPB) {
        cval |= UART_LCR_STOP;
    }
    if (c_cflag & PARENB) {
        cval |= UART_LCR_PARITY;
        if (up->bugs & UART_BUG_PARITY) {
            up->fifo_bug = true;
        }
    }
    if (!(c_cflag & PARODD)) {
        cval |= UART_LCR_EPAR;
    }
#ifdef CMSPAR
    if (c_cflag & CMSPAR) {
        cval |= UART_LCR_SPAR;
    }
#endif

    return cval;
}

/* Uart divisor latch write */
static void default_serial_dl_write(struct ocores_uart* up, int value)
{
    serial_out(up, UART_DLL, value & UART_BYTE_MASK);
    serial_out(up, UART_DLM, value >> 8 & UART_BYTE_MASK);
}

static void mc_uart_set_divisor(struct uart_port* port, unsigned int baud, unsigned int quot,
                                        unsigned int quot_frac)
{
    struct ocores_uart* up;

    up = up_to_mc_uart(port);
    /*
     * For NatSemi, switch to bank 2 not bank 1, to avoid resetting EXCR2,
     * otherwise just set DLAB
     */
    if (up->capabilities & UART_NATSEMI) {
        serial_out(up, UART_LCR, 0xe0);
    } else {
        serial_out(up, UART_LCR, up->lcr | UART_LCR_DLAB);
    }
    FPGA_UART_DEBUG_INIT("mc_uart_set_divisor. 0x%x.\n", quot);
    default_serial_dl_write(up, quot);
}

static unsigned int mc_uart_get_baud_rate(struct uart_port* port, struct ktermios* termios,
                                                                    const struct ktermios* old)
{
    /*
     * Ask the core to calculate the divisor for us.
     * Allow 1% tolerance at the upper limit so uart clks marginally
     * slower than nominal still match standard baud rates without
     * causing transmission errors.
     */
    return uart_get_baud_rate(port, termios, old, MIN_BAUDRATE_SUPPORTED, MAX_BAUDRATE_SUPPORTED);
}

static void mc_uart_set_termios(struct uart_port* port, struct ktermios* termios,
                                                            const struct ktermios* old)
{
    struct ocores_uart* up;
    unsigned char cval;
    unsigned long flags;
    unsigned int baud, quot, frac;

    up = up_to_mc_uart(port);
    frac = 0;
    FPGA_UART_DEBUG_INIT("mc_uart_set_termios \n");
    cval = mc_uart_compute_lcr(up, termios->c_cflag);
    baud = mc_uart_get_baud_rate(port, termios, old);
    quot = DIV_ROUND_CLOSEST(up->clk, up->div * baud); /* round up */

    mc_uart_rpm_get(up);
    spin_lock_irqsave(&port->lock, flags);
    up->lcr = cval; /* Save computed LCR */

    if (up->capabilities & UART_CAP_FIFO && port->fifosize > 1) {
        /* NOTE: If fifo_bug is not set, a user can set RX_trigger. */
        if ((baud < 2400) || up->fifo_bug) {
            up->fcr &= ~UART_FCR_TRIGGER_MASK;
            up->fcr |= UART_FCR_TRIGGER_1;
        }
    }
    mc_sw_rts_set_mode_locked(up, !!(termios->c_cflag & CRTSCTS));

    /*
     * Update the per-port timeout.
     */
    uart_update_timeout(port, termios->c_cflag, baud);

    port->read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
    if (termios->c_iflag & INPCK) {
        port->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
    }
    if (termios->c_iflag & (IGNBRK | BRKINT | PARMRK)) {
        port->read_status_mask |= UART_LSR_BI;
    }

    /*
     * Characteres to ignore
     */
    port->ignore_status_mask = 0;
    if (termios->c_iflag & IGNPAR) {
        port->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
    }

    if (termios->c_iflag & IGNBRK) {
        port->ignore_status_mask |= UART_LSR_BI;
        /*
         * If we're ignoring parity and break indicators,
         * ignore overruns too (for real raw support).
         */
        if (termios->c_iflag & IGNPAR) {
            port->ignore_status_mask |= UART_LSR_OE;
        }
    }

    /*
     * ignore all characters if CREAD is not set
     */
    if ((termios->c_cflag & CREAD) == 0) {
        port->ignore_status_mask |= UART_LSR_DR;
    }

    /*
     * CTS flow control flag and modem status interrupts
     */
    up->ier &= ~UART_IER_MSI;
    if (!(up->bugs & UART_BUG_NOMSR) && UART_ENABLE_MS(&up->port, termios->c_cflag)) {
        up->ier |= UART_IER_MSI;
    }
    if (up->capabilities & UART_CAP_UUE) {
        up->ier |= UART_IER_UUE;
    }
    if (up->capabilities & UART_CAP_RTOIE) {
        up->ier |= UART_IER_RTOIE;
    }

    serial_out(up, UART_IER, up->ier);

    if (up->capabilities & UART_CAP_EFR) {
        unsigned char efr;

        efr = 0;
        /*
         * TI16C752/Startech hardware flow control.    FIXME:
         * - TI16C752 requires control thresholds to be set.
         * - UART_MCR_RTS is ineffective if auto-RTS mode is enabled.
         */
        if (termios->c_cflag & CRTSCTS) {
            efr |= UART_EFR_CTS;
        }

        serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
        if (port->flags & UPF_EXAR_EFR) {
            serial_out(up, UART_XR_EFR, efr);
        } else {
            serial_out(up, UART_EFR, efr);
        }
    }

    mc_uart_set_divisor(port, baud, quot, frac);

    serial_out(up, UART_LCR, up->lcr); /* reset DLAB */
    if (port->type != PORT_16750) {
        /* emulated UARTs (Lucent Venus 167x) need two steps */
        if (up->fcr & UART_FCR_ENABLE_FIFO) {
            serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO);
        }
        serial_out(up, UART_FCR, up->fcr); /* set fcr */
    }
    mc_uart_set_mctrl(port, port->mctrl);
    spin_unlock_irqrestore(&port->lock, flags);
    mc_uart_rpm_put(up);

    /* Don't rewrite B0 */
    if (tty_termios_baud_rate(termios)) {
        tty_termios_encode_baud_rate(termios, baud, baud);
    }
}

static const char* mc_type(struct uart_port* port)
{
    return MC_UART_TYPE_NAME;
}

static void autoconfig(struct ocores_uart* up)
{
    unsigned char status1, scratch, scratch2, scratch3;
    unsigned char save_mcr;
    struct uart_port* port;
    unsigned long flags;

    port = &up->port;
    if (!port->iobase && !port->mapbase && !port->membase) {
        return;
    }

    spin_lock_irqsave(&port->lock, flags);

    up->capabilities = 0;
    up->bugs = 0;

    if (!(port->flags & UPF_BUGGY_UART)) {
        scratch = serial_in(up, UART_IER);
        serial_out(up, UART_IER, 0);
        scratch2 = serial_in(up, UART_IER) & UART_HBYTE_MASK;
        serial_out(up, UART_IER, UART_HBYTE_MASK);
        scratch3 = serial_in(up, UART_IER) & UART_HBYTE_MASK;
        serial_out(up, UART_IER, scratch);
        if (scratch2 != 0 || scratch3 != UART_HBYTE_MASK) {
            FPGA_UART_DEBUG_ERROR("scratch2:%x scratch3:%x error:\n", scratch2, scratch3);
            spin_unlock_irqrestore(&port->lock, flags);
            goto out;
        }
    }

    save_mcr = mc_uart_in_mcr(up);

    if (!(port->flags & UPF_SKIP_TEST)) {
        mc_uart_out_mcr(up, UART_MCR_LOOP | 0x0A);
        status1 = serial_in(up, UART_MSR) & 0xF0;
        mc_uart_out_mcr(up, save_mcr);
        if (status1 != 0x90) {
            spin_unlock_irqrestore(&port->lock, flags);
            goto out;
        }
    }
    FPGA_UART_DEBUG_INIT("autoconfig\n");

    up->port.type = PORT_16550;
    up->capabilities |= UART_CAP_FIFO;

    port->fifosize = FIFO_SIZE;
    up->tx_loadsz = FIFO_SIZE;

    mc_uart_out_mcr(up, save_mcr);
    mc_uart_clear_fifos(up);
    serial_in(up, UART_RX);
    serial_out(up, UART_IER, 0);

    spin_unlock_irqrestore(&port->lock, flags);
out:
    return;
}

static void mc_uart_config_port(struct uart_port* port, int flags)
{
    struct ocores_uart* up;

    FPGA_UART_DEBUG_INIT("init flag:%x\n", flags);
    up = up_to_mc_uart(port);
    if (flags & UART_CONFIG_TYPE) {
        autoconfig(up);
    }
    serial_out(up, UART_IER, UART_IER_RLSI | UART_IER_RDI);
    up->fcr = UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_11 | UART_FCR_CLEAR_RCVR |
                        UART_FCR_CLEAR_XMIT;
    mc_uart_register_dev_spec_attr_grp(up);
}

static void mc_uart_throttle(struct uart_port* port)
{
    FPGA_UART_DEBUG_INIT("mc_uart_throttle \n");
    port->throttle(port);
}

static void mc_uart_unthrottle(struct uart_port* port)
{
    FPGA_UART_DEBUG_INIT("mc_uart_unthrottle \n");
    port->unthrottle(port);
}

static void mc_uart_set_ldisc(struct uart_port* port, struct ktermios* termios)
{
    FPGA_UART_DEBUG_INIT("mc_uart_set_ldisc \n");
    if (termios->c_line == N_PPS) {
        port->flags |= UPF_HARDPPS_CD;
        spin_lock_irq(&port->lock);
        mc_uart_enable_ms(port);
        spin_unlock_irq(&port->lock);
    } else {
        port->flags &= ~UPF_HARDPPS_CD;
        if (!UART_ENABLE_MS(port, termios->c_cflag)) {
            spin_lock_irq(&port->lock);
            mc_uart_enable_ms(port);
            spin_unlock_irq(&port->lock);
        }
    }
}

static int mc_uart_verify_port(struct uart_port* port, struct serial_struct* ser)
{
    FPGA_UART_DEBUG_INIT("mc_uart_verify_port \n");
    return 0;
}

static const struct uart_ops mc_uart_ops = {
        .tx_empty = mc_uart_tx_empty,
        .set_mctrl = mc_uart_set_mctrl,
        .get_mctrl = mc_uart_get_mctrl,
        .stop_tx = mc_uart_stop_tx,
        .start_tx = mc_uart_start_tx,
        .throttle = mc_uart_throttle,
        .unthrottle = mc_uart_unthrottle,
        .flush_buffer = mc_uart_flush_buffer,
        .stop_rx = mc_uart_stop_rx,
        .enable_ms = mc_uart_enable_ms,
        .break_ctl = mc_uart_break_ctl,
        .startup = mc_uart_startup,
        .shutdown = mc_uart_shutdown,
        .set_termios = mc_uart_set_termios,
        .set_ldisc = mc_uart_set_ldisc,
        .release_port = mc_uart_release_port,
        .request_port = mc_uart_request_port,
        .config_port = mc_uart_config_port,
        .verify_port = mc_uart_verify_port,
        .type = mc_type,
};

static struct uart_driver mc_uart_driver = {
        .owner = THIS_MODULE,
        .driver_name = "mc-serial",
        .dev_name = "ttyMC",
        .major = MC_UART_MAJOR,
        .minor = MC_UART_MINOR,
        .nr = TTYMC_NUM, /* number of tty */
};

static struct ocores_uart* mc_uart_get_uart_via_port(struct tty_port *port){
    struct uart_state *state;
    struct uart_port *uport;
    struct ocores_uart *up;

    state = container_of(port, struct uart_state, port);
    uport = state->uart_port;
    up = container_of(uport, struct ocores_uart, port);
    return up;
}

/* Interface to adjust FIFO trigger level */
static int do_set_rxtrig(struct tty_port *port, unsigned char bytes)
{
    struct ocores_uart *up;
    int rxtrig;

    up = mc_uart_get_uart_via_port(port);

    switch (bytes) {
        case UART_FIFO_TRIGGER_1:
            rxtrig = UART_FCR_TRIGGER_1;
            break;
        case UART_FIFO_TRIGGER_4:
            rxtrig = UART_FCR_TRIGGER_4;
            break;
        case UART_FIFO_TRIGGER_8:
            rxtrig = UART_FCR_TRIGGER_8;
            break;
        case UART_FIFO_TRIGGER_16:
            rxtrig = UART_FCR_TRIGGER_16;
            break;
        default:
            rxtrig = UART_FCR_TRIGGER_1;
            break;
    }

    mc_uart_out_fcr(up, UART_FCR_ENABLE_FIFO);
    up->fcr &= ~UART_FCR_TRIGGER_MASK;
    up->fcr |= (unsigned char)rxtrig;
    mc_uart_out_fcr(up, up->fcr);
    return 0;
}

static int mc_uart_set_rx_bytes(struct tty_port *port, int bytes) {
    int ret;

    mutex_lock(&port->mutex);
    ret = do_set_rxtrig(port, bytes);
    mutex_unlock(&port->mutex);

    return ret;
}

static ssize_t mc_uart_store_fifo_trigger(struct device *dev, struct device_attribute *attr,
                                            const char *buf, size_t count)
{
    struct tty_port *port = dev_get_drvdata(dev);
    int ret;
    if (!count) {
        return -EINVAL;
    }

    ret = mc_uart_set_rx_bytes(port, simple_strtoul(buf, NULL, 0));
    if (ret < 0) {
        return ret;
    }

    return count;
}

static int fcr_get_rxtrig_bytes(struct ocores_uart *up)
{
    unsigned char bytes;

    switch (mc_uart_in_fcr(up) & UART_FCR_TRIGGER_MASK) {
        case UART_FCR_TRIGGER_1:
            bytes = UART_FIFO_TRIGGER_1;
            break;
        case UART_FCR_TRIGGER_4:
            bytes = UART_FIFO_TRIGGER_4;
            break;
        case UART_FCR_TRIGGER_8:
            bytes = UART_FIFO_TRIGGER_8;
            break;
        case UART_FCR_TRIGGER_16:
            bytes = UART_FIFO_TRIGGER_16;
            break;
        default:
            bytes = UART_FIFO_TRIGGER_1;
            break;
    }
    return bytes;
}

static int do_get_rxtrig(struct tty_port *port)
{
    struct ocores_uart *up;

    up = mc_uart_get_uart_via_port(port);
    return fcr_get_rxtrig_bytes(up);
}

static int mc_uart_get_rx_bytes(struct tty_port *port){
    int rxtrig_bytes;

    mutex_lock(&port->mutex);
    rxtrig_bytes = do_get_rxtrig(port);
    mutex_unlock(&port->mutex);

    return rxtrig_bytes;
}

static ssize_t mc_uart_show_fifo_trigger(struct device *dev, struct device_attribute *attr,
                                            char *buf)
{
    struct tty_port *port;
    int rx_bytes;

    port = dev_get_drvdata(dev);
    rx_bytes = mc_uart_get_rx_bytes(port);
    return snprintf(buf, BUF_SIZE, "fifo tigger level: %d bytes\n",
               rx_bytes);
}

static ssize_t mc_uart_show_buffer_switch(struct device *dev, struct device_attribute *attr,
                                                char *buf)
{
    struct tty_port *port;
    struct ocores_uart *up;
    char* buffer_status;

    port = dev_get_drvdata(dev);
    up = mc_uart_get_uart_via_port(port);

    buffer_status = (up->mc_uart_buffer_switch == MC_UART_BUFFER_OFF) ? "off" : "on";
    return snprintf(buf, BUF_SIZE, "buffer status: %s\n",
               buffer_status);
}

static ssize_t mc_uart_store_buffer_switch(struct device *dev, struct device_attribute *attr,
                                                const char *buf, size_t count)
{
    struct tty_port *port;
    struct ocores_uart *up;
    int new_status;
    unsigned long flags;

    port = dev_get_drvdata(dev);
    up = mc_uart_get_uart_via_port(port);

    if (!count) {
        return -EINVAL;
    }

    new_status = simple_strtoul(buf, NULL, 0);
    if (new_status < MC_UART_BUFFER_OFF || new_status > MC_UART_BUFFER_ON) {
        return -EINVAL;
    }
    spin_lock_irqsave(&up->port.lock, flags);
    up->mc_uart_buffer_switch = new_status;
    if (up->mc_uart_buffer_switch == MC_UART_BUFFER_ON) {
        up->ier |= UART_IER_RLSI | UART_IER_RDI;
        serial_out(up, UART_IER, up->ier);
    } else if (up->mc_uart_buffer_switch == MC_UART_BUFFER_OFF
                    && up->mc_uart_status == MC_UART_CLOSE) {
        up->ier &= ~(UART_IER_RLSI | UART_IER_RDI);
        serial_out(up, UART_IER, up->ier);
    }
    spin_unlock_irqrestore(&up->port.lock, flags);
    return count;
}

static ssize_t mc_uart_show_buffer_size(struct device *dev, struct device_attribute *attr,
                                                char *buf)
{
    struct tty_port *port;
    struct ocores_uart *up;
    int buffer_size;

    port = dev_get_drvdata(dev);
    up = mc_uart_get_uart_via_port(port);

    buffer_size = up->config_buffer_size;
    return snprintf(buf, BUF_SIZE, "buffer size: %d byte\n",
               buffer_size);
}

static ssize_t mc_uart_store_buffer_size(struct device *dev, struct device_attribute *attr,
                                                const char *buf, size_t count)
{
    struct tty_port *port;
    struct ocores_uart *up;
    int new_buffer_size, ret;
    unsigned long flags;

    port = dev_get_drvdata(dev);
    up = mc_uart_get_uart_via_port(port);

    if (!count) {
        return -EINVAL;
    }

    new_buffer_size = simple_strtoul(buf, NULL, 0);
    if (new_buffer_size < MC_UART_BUFFER_MIN) {
        return -EINVAL;
    }

    up->config_buffer_size = new_buffer_size;
    spin_lock_irqsave(&up->port.lock, flags);
    ret = serial_buffer_realloc(up->port.line, up->config_buffer_size);
    if (ret < 0) {
        count = -ENOMEM;
    }
    spin_unlock_irqrestore(&up->port.lock, flags);
    return count;
}

static struct device_attribute dev_attr_fifo_trigger;
static struct device_attribute dev_attr_buffer_switch;
static struct device_attribute dev_attr_buffer_size;

static DEVICE_ATTR(fifo_trigger, S_IWUSR | S_IRUSR | S_IRGRP,
        mc_uart_show_fifo_trigger, mc_uart_store_fifo_trigger);

static DEVICE_ATTR(buffer_switch, S_IWUSR | S_IRUSR | S_IRGRP,
        mc_uart_show_buffer_switch, mc_uart_store_buffer_switch);

static DEVICE_ATTR(buffer_size, S_IWUSR | S_IRUSR | S_IRGRP,
        mc_uart_show_buffer_size, mc_uart_store_buffer_size);

static struct attribute *mc_uart_dev_attrs[] = {
    &dev_attr_fifo_trigger.attr,
    &dev_attr_buffer_switch.attr,
    &dev_attr_buffer_size.attr,
    NULL,
    };

static struct attribute_group mc_uart_dev_attr_group = {
    .attrs = mc_uart_dev_attrs,
    };

static void mc_uart_register_dev_spec_attr_grp(struct ocores_uart *up)
{
    up->port.attr_group = &mc_uart_dev_attr_group;
}

static int mc_ocores_uart_probe(struct platform_device* pdev)
{
    struct ocores_uart* uart;
    struct mc_uart_platform_data* pdata;
    struct resource* res;
    int irq;
    int ret;

    uart = devm_kzalloc(&pdev->dev, sizeof(*uart), GFP_KERNEL);
    if (!uart) {
        FPGA_UART_DEBUG_ERROR("devm_kzalloc failed.\n");
        ret = -ENOMEM;
        goto err_nomem;
    }
    pdata = dev_get_platdata(&pdev->dev);
    spin_lock_init(&uart->port.lock);

    /* Init uart */
    uart->div = pdata->div;
    uart->baud_rate = pdata->baud_rate;
    uart->port.uartclk = pdata->baud_rate * uart->div;
    uart->clk = pdata->clk;
    uart->port.line = pdata->id;
    uart->port.dev = &pdev->dev;
    uart->port.iotype = UPIO_MEM;
    uart->port.fifosize = FIFO_SIZE;
    uart->port.ops = &mc_uart_ops;
    uart->port.flags = UPF_IOREMAP | UPF_BOOT_AUTOCONF;
    uart->mcr_mask = 0xFF;
    uart->mcr_force = 0x00;
    uart->xmit_size = uart->port.fifosize;
    uart->port.irqflags = IRQF_SHARED;
    uart->mc_uart_status = MC_UART_CLOSE;
    uart->mc_uart_buffer_switch = MC_UART_BUFFER_ON;
    uart->config_buffer_size = BUFFER_SIZE;
    if (serial_buffer_if_allocced(uart->port.line) == SERIAL_BUFFER_NOT_ALLOCCED) {
        ret = serial_buffer_alloc(uart->port.line, uart->config_buffer_size);
        if (ret < 0) {
            goto err_mem_ring;
        }
    } else {
        uart->config_buffer_size = serial_get_buffer_size(uart->port.line);
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        ret = -ENOMEM;
        goto err_mem;
    }

    uart->port.mapbase = res->start;
    uart->port.mapsize = resource_size(res);

    uart->port.membase = ioremap(uart->port.mapbase, uart->port.mapsize);
    if (IS_ERR(uart->port.membase)) {
        FPGA_UART_DEBUG_ERROR("ioremap_resource failed.\n");
        ret = PTR_ERR(uart->port.membase);
        goto err_iomap;
    }

    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        FPGA_UART_DEBUG_ERROR("platform_get_irq failed irq %d.\n", irq);
        ret = -EINVAL;
        goto err_irq;
    }

    uart->port.irq = irq;
    uart->port.serial_in = mem32_serial_in;
    uart->port.serial_out = mem32_serial_out;

    ret = devm_request_irq(&pdev->dev, uart->port.irq, ocores_isr, IRQF_SHARED, "mc-serial", uart);
    if (irq < 0) {
        FPGA_UART_DEBUG_ERROR("mc_uart failed irq %d.\n", irq);
        ret = -EINVAL;
        goto err_irq;
    }

    ret = uart_add_one_port(&mc_uart_driver, &uart->port);
    if (ret) {
        goto err_add_port;
    }

    mc_sw_rts_init(uart);
    platform_set_drvdata(pdev, uart);
    return 0;

err_add_port:
err_irq:
    iounmap(uart->port.membase);
err_iomap:
err_mem:
err_mem_ring:
err_nomem:
    return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int mc_ocores_uart_remove(struct platform_device* pdev)
#else
static void mc_ocores_uart_remove(struct platform_device* pdev)
#endif
{
    struct ocores_uart* uart;

    uart = platform_get_drvdata(pdev);
    mc_sw_rts_cleanup(uart);
    uart_remove_one_port(&mc_uart_driver, &uart->port);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
    return 0;
#else
    return;
#endif
}

static const struct platform_device_id fpga_uart_id_table[] = {
	{
		.name		= "mc-uart",
		.driver_data	= 0,
	},
    {
		.name		= "fpga-uart",
		.driver_data	= 0,
	},
	{ }
};
MODULE_DEVICE_TABLE(platform, fpga_uart_id_table);

static struct platform_driver ocores_uart_driver =
{
        .probe = mc_ocores_uart_probe,
        .remove = mc_ocores_uart_remove,
        .driver =
                {
                   .name = "mc-uart",
                },
        .id_table = fpga_uart_id_table,
};
static int __init uart_init(void)
{
    int ret;

    ret = uart_register_driver(&mc_uart_driver);
    if (ret) {
        return ret;
    }

    ret = platform_driver_register(&ocores_uart_driver);
    if (ret) {
        uart_unregister_driver(&mc_uart_driver);
        return ret;
    }
    return ret;
}

static void __exit uart_exit(void)
{
    platform_driver_unregister(&ocores_uart_driver);
    uart_unregister_driver(&mc_uart_driver);
}

module_init(uart_init);
module_exit(uart_exit);

MODULE_AUTHOR("micas <rd@micas.com>");
MODULE_DESCRIPTION("OpenCores UART bus driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:ocores-uart");
