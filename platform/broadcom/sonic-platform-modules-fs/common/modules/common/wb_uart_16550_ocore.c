#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/console.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/synclink.h>
#include <linux/pci.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/version.h>

#include "wb_uart_16550_ocore.h"

#define UART_DRV_NAME "wb_uart_16550_ocore"

/*********************start: reg addr *******************/
/* LCR DLAB=1 */
#define WB_UART_DLL    (0x00)    /* R/W: Divisor Latch Low */
#define WB_UART_DLM    (0x04)    /* R/W: Divisor Latch High */
#define WB_UART_FCR_RO (0x08)    /* RO: FIFO Control Register */

/* LCR DLAB=0 */
#define WB_UART_RX     (0x00)    /* RO:  Receive buffer */
#define WB_UART_TX     (0x00)    /* WO: Transmit buffer */
#define WB_UART_IER    (0x04)    /* R/W: Interrupt Enable Register */
#define WB_UART_IIR    (0x08)    /* RO:  Interrupt ID Register */
#define WB_UART_FCR_WO (0x08)    /* WO: FIFO Control Register */

/* LCR DLAB=x */
#define WB_UART_LCR    (0x0c)    /* R/W: Line Control Register */
#define WB_UART_LSR    (0x14)    /* RO:  Line Status Register */

/* FPGA only have RX\TX, Modem reg not use */
#define WB_UART_MCR    (0x10)    /* R/W: Modem Control Register */
#define WB_UART_MSR    (0x18)    /* RO:  Modem Status Register */
#define WB_UART_SCR    (0x1c)    /* R/W: Scratch Register */

#define WB_UART_MCR_CLKSEL     (0x80) /* Divide clock by 4 (TI16C752, EFR[4]=1) */
#define WB_UART_MCR_TCRTLR     (0x40) /* Access TCR/TLR (TI16C752, EFR[4]=1) */
#define WB_UART_MCR_XONANY     (0x20) /* Enable Xon Any (TI16C752, EFR[4]=1) */
#define WB_UART_MCR_AFE        (0x20) /* Enable auto-RTS/CTS (TI16C550C/TI16C750) */
#define WB_UART_MCR_LOOP       (0x10) /* Enable loopback test mode */
#define WB_UART_MCR_OUT2       (0x08) /* Out2 complement */
#define WB_UART_MCR_OUT1       (0x04) /* Out1 complement */
#define WB_UART_MCR_RTS        (0x02) /* RTS complement */
#define WB_UART_MCR_DTR        (0x01) /* DTR complement */

#define WB_UART_MSR_DCD        (0x80) /* Data Carrier Detect */
#define WB_UART_MSR_RI         (0x40) /* Ring Indicator */
#define WB_UART_MSR_DSR        (0x20) /* Data Set Ready */
#define WB_UART_MSR_CTS        (0x10) /* Clear to Send */
#define WB_UART_MSR_DDCD       (0x08) /* Delta DCD */
#define WB_UART_MSR_TERI       (0x04) /* Trailing edge ring indicator */
#define WB_UART_MSR_DDSR       (0x02) /* Delta DSR */
#define WB_UART_MSR_DCTS       (0x01) /* Delta CTS */
#define WB_UART_MSR_ANY_DELTA  (0x0F) /* Any of the delta bits! */
/***********************end: reg addr *******************/

/********************start: Definitions of bits related to each register **********************/
/* Bits related to UART_IER */
#define WB_UART_IER_MSI            (0x08) /* Enable Modem status interrupt */
#define WB_UART_IER_RLSI           (0x04) /* Enable receiver line status interrupt */
#define WB_UART_IER_THRI           (0x02) /* Enable Transmitter holding register empty int. */
#define WB_UART_IER_RDI            (0x01) /* Enable receiver data interrupt */

/* Bits related to UART_IIR */
#define WB_UART_IIR_NO_INT         (0x01) /* No interrupts pending */
#define WB_UART_IIR_ID             (0x0e) /* Mask for the interrupt ID */
#define WB_UART_IIR_MSI            (0x00) /* Modem status interrupt */
#define WB_UART_IIR_THRI           (0x02) /* Transmitter holding register empty */
#define WB_UART_IIR_RDI            (0x04) /* Receiver data interrupt */
#define WB_UART_IIR_RLSI           (0x06) /* Receiver line status interrupt */

/* Bits related to UART_FCR */
#define WB_UART_FCR_ENABLE_FIFO    (0x01) /* Enable the FIFO */
#define WB_UART_FCR_CLEAR_RCVR     (0x02) /* Clear the RCVR FIFO */
#define WB_UART_FCR_CLEAR_XMIT     (0x04) /* Clear the XMIT FIFO */
#define WB_UART_FCR_DMA_SELEC      (0x08) /* For DMA applications */
#define WB_UART_FCR_TRIGGER_MAS    (0xC0) /* Mask for the FIFO trigger range */
#define WB_UART_FCR_TRIGGER_1      (0x00) /* Mask for trigger set at 1 */
#define WB_UART_FCR_TRIGGER_4      (0x40) /* Mask for trigger set at 4 */
#define WB_UART_FCR_TRIGGER_8      (0x80) /* Mask for trigger set at 8 */
#define WB_UART_FCR_TRIGGER_14     (0xC0) /* Mask for trigger set at 14 */

/* Bits related to UART_LCR */
#define WB_UART_LCR_DLAB           (0x80) /* Divisor latch access bit */
#define WB_UART_LCR_SBC            (0x40) /* Set break control */
#define WB_UART_LCR_SPAR           (0x20) /* Stick parity (?) */
#define WB_UART_LCR_EPAR           (0x10) /* Even parity select */
#define WB_UART_LCR_PARITY         (0x08) /* Parity Enable */
#define WB_UART_LCR_STOP           (0x04) /* Stop bits: 0=1 bit, 1=2 bits */
#define WB_UART_LCR_WLEN5          (0x00) /* Wordlength: 5 bits */
#define WB_UART_LCR_WLEN6          (0x01) /* Wordlength: 6 bits */
#define WB_UART_LCR_WLEN7          (0x02) /* Wordlength: 7 bits */
#define WB_UART_LCR_WLEN8          (0x03) /* Wordlength: 8 bits */

/* Bits related to UART_LSR */
#define WB_UART_LSR_FIFOE          (0x80) /* Fifo error */
#define WB_UART_LSR_TEMT           (0x40) /* Transmitter empty */
#define WB_UART_LSR_THRE           (0x20) /* Transmit-hold-register empty */
#define WB_UART_LSR_BI             (0x10) /* Break interrupt indicator */
#define WB_UART_LSR_FE             (0x08) /* Frame error indicator */
#define WB_UART_LSR_PE             (0x04) /* Parity error indicator */
#define WB_UART_LSR_OE             (0x02) /* Overrun error indicator */
#define WB_UART_LSR_DR             (0x01) /* Receiver data ready */
#define WB_UART_LSR_BRK_ERROR_BITS (0x1E) /* BI, FE, PE, OE bits */

#define WB_BOTH_EMPTY             (WB_UART_LSR_TEMT | WB_UART_LSR_THRE)

/********************end: Definitions of bits related to each register ***********************/

#define WB_UART_CLK_SOURCE_FREQ_DEF   (62500000)
#define WB_UART_MAX_RX_FIFO_SIZE      (1024)
#define WB_UART_TX_FIFO_SIZE_DEF      (1)

/* All serial ports shared registers */
#define WB_UART_REG_SPD_CFG_VAL   (0x400)
#define WB_UART_REG_SPD_DETECT    (0x404)
#define WB_UART_REG_SPD_RELIABLE  (0x408)
#define WB_UART_REG_IMPLICIT0     (0x40c)
#define WB_UART_REG_IMPLICIT1     (0x410)
#define WB_UART_REG_IMPLICIT2     (0x414)

/* Be consistent with the number of msi interrupts applied for by the driver */
#define WB_FPGA_MAX_MSI_IRQ_NUM    (14)
#define WB_FPGA_PORT_UART_16550     "wb-fpga-uart-16550"

#define UART_MAJOR                 (211)
#define UART_MINOR                 (9)
#define MAX_TTYRG_NUM              (26)

/* uart_lite only supports two baud rates */
#define BAUD_9600                  (9600)
#define BAUD_115200                (115200)

/* Check baud rates every 5 seconds */
#define TIMING_HANDLE_TIME         (5 * HZ)

/* uart_lite bit width */
enum reg_bit_width_e {
    REG_WIDTH_1B  = 1,
    REG_WIDTH_2B  = 2,
    REG_WIDTH_4B  = 4,
    REG_WIDTH_END = 8,
};

/* uart structure */
struct wb_uart_ocore_s {
    struct uart_port port;
    /* uart port rate adaptive, default 0 */
    u32 is_spd_adapt;
    u32 is_active;
    u32 current_baud;
    struct list_head list;
    char name[32];
};

/* uart list */
static LIST_HEAD(wb_uart_ocore_list);

/* uart list lock */
static DEFINE_SPINLOCK(wb_uart_ocore_list_lock);

/* uart port rate adaptive check timer */
static struct timer_list g_uart_speed_adapt_timer;

#if 0
static void wb_uart_set_mctrl(struct uart_port *port, unsigned int mctrl);
#endif

/********************************** start: debug ***********************************/
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "control print info level. 0: none; 1: err;" \
                 "2: err + warn; 3: err + warn +info; >3: all");
#define DEBUG_ERROR(fmt, args...)                                                           \
    do {                                                                                    \
        if (debug > 0) {                                                                    \
            printk(KERN_ERR "[func:%s line:%d]  "fmt, __func__, __LINE__, ## args);         \
        } else {                                                                            \
            pr_debug(fmt, ## args);                                                         \
        }                                                                                   \
    } while (0)

#define DEBUG_WARN(fmt, args...)                                                            \
        do {                                                                                \
            if (debug > 1) {                                                                \
                printk(KERN_WARNING "[func:%s line:%d]  "fmt, __func__, __LINE__, ## args); \
            } else {                                                                        \
                pr_debug(fmt, ## args);                                                     \
            }                                                                               \
        } while (0)

#define DEBUG_INFO(fmt, args...)                                                            \
    do {                                                                                    \
        if (debug > 2) {                                                                    \
            printk(KERN_INFO "[func:%s line:%d]  "fmt, __func__, __LINE__, ## args);        \
        } else {                                                                            \
            pr_debug(fmt, ## args);                                                         \
        }                                                                                   \
    } while (0)

#define DEBUG_VERBOSE(fmt, args...)                                                         \
    do {                                                                                    \
        if (debug > 3) {                                                                    \
            printk(KERN_DEBUG "[func:%s line:%d]  "fmt, __func__, __LINE__, ## args);       \
        } else {                                                                            \
            pr_debug(fmt, ## args);                                                         \
        }                                                                                   \
    } while (0)
/********************************** end: debug ***********************************/

static inline struct wb_uart_ocore_s *up_to_wb_uart_ocore(struct uart_port* up)
{
    return container_of(up, struct wb_uart_ocore_s, port);
}

static u32 uart_probe_baud(struct uart_port *port)
{
    u32 lcr, dll, dlm;
    u32 quot;
    u32 baud;

    /* Calculating the baud rate */
    lcr = port->serial_in(port, WB_UART_LCR);
    port->serial_out(port, WB_UART_LCR, lcr | WB_UART_LCR_DLAB);
    dll = port->serial_in(port, WB_UART_DLL);
    dlm = port->serial_in(port, WB_UART_DLM);
    port->serial_out(port, WB_UART_LCR, lcr);

    quot = ((dlm & 0xFF) << 8) + (dll & 0xFF);
    if (quot != 0) {
        baud = (port->uartclk / 16) / quot;
    } else {
        baud = BAUD_115200;
        DEBUG_ERROR("quot is 0! uart%d, dlm 0x%x, dll 0x%x, baud 0x%x\n", port->line, dlm, dll, baud);
    }
    DEBUG_INFO("uart%d, dlm 0x%x, dll 0x%x, baud 0x%x\n", port->line, dlm, dll, baud);

    return baud;
}

static void uart_set_baud_rate(struct uart_port *port, u32 baud)
{
    int dll_reg_val;
    int dlm_reg_val;
    u32 lcr_reg_val;
    int tmp;

    /* 1. invalid baud rate, default 9600 */
    if ((baud != BAUD_115200) && (baud != BAUD_9600)) {
        DEBUG_WARN("invalid uart baud rate: %u, use default 9600", baud);
        baud = BAUD_9600;
    }

    /* 2. Calculating dll dlm value and set */
    tmp = uart_get_divisor(port, baud);
    /* baud rate must bigger than target baud */
    dll_reg_val = (tmp & 0xFF) - 1;
    dlm_reg_val = (tmp >> 8) & 0xFF;

    DEBUG_INFO("uart%d: \n", port->line);
    DEBUG_INFO("w UART_DLL, addr: 0x%x, val: 0x%x\n", WB_UART_DLL, dll_reg_val);
    DEBUG_INFO("w UART_DLM, addr: 0x%x, val: 0x%x\n", WB_UART_DLM, dlm_reg_val);

    lcr_reg_val = port->serial_in(port, WB_UART_LCR);
    port->serial_out(port, WB_UART_LCR, (int)(lcr_reg_val | WB_UART_LCR_DLAB));
    port->serial_out(port, WB_UART_DLL, dll_reg_val);
    port->serial_out(port, WB_UART_DLM, dlm_reg_val);
    port->serial_out(port, WB_UART_LCR, (int)lcr_reg_val);

    return;
}

static void uart_get_detected_baud(struct uart_port *port, u32 *pbaud)
{
    u32 tmp;
    int offset;
    u32 detected_baud;

    /* 1. Calculating UART_REG_SPD_DETECT offset, read reg vaule */
    offset = WB_UART_REG_SPD_DETECT - ((unsigned long)port->membase & (~PAGE_MASK));
    tmp = port->serial_in(port, offset);
    DEBUG_INFO("offset: 0x%x, tmp = 0x%x\n", offset, tmp);

    /* 2. Confirm the baud rate of the uart of the peer device */
    if (tmp & (1 << port->line)) {
        detected_baud = BAUD_115200;
    } else {
        detected_baud = BAUD_9600;
    }
    DEBUG_INFO("detected_baud: %u\n", detected_baud);

    *pbaud = detected_baud;

    return;
}

static void uart_hw_init(struct uart_port *port)
{
    int fcr_reg_val;
    int lcr_reg_val;
    int ier_reg_val;
    u32 tmp1, tmp2;

    DEBUG_INFO("uart%d: \n", port->line);

    /* 1. set FCR: enable FIFO */
    fcr_reg_val = WB_UART_FCR_ENABLE_FIFO;
    port->serial_out(port, WB_UART_FCR_WO, fcr_reg_val);
    DEBUG_INFO("w UART_FCR, addr: 0x%x, val:  0x%x\n", WB_UART_FCR_WO, fcr_reg_val);

    /* 1.1 set FCR: clear RX TX FIFO */
    fcr_reg_val = WB_UART_FCR_ENABLE_FIFO | WB_UART_FCR_CLEAR_RCVR | WB_UART_FCR_CLEAR_XMIT;
    port->serial_out(port, WB_UART_FCR_WO, fcr_reg_val);
    DEBUG_INFO("w UART_FCR, addr: 0x%x, val:  0x%x\n", WB_UART_FCR_WO, fcr_reg_val);

    /* 1.2 set FCR: enable FIFO, and set RXFIFO trigger level */
    fcr_reg_val = WB_UART_FCR_ENABLE_FIFO | WB_UART_FCR_TRIGGER_14;
    port->serial_out(port, WB_UART_FCR_WO, fcr_reg_val);
    DEBUG_INFO("w UART_FCR, addr: 0x%x, val:  0x%x\n", WB_UART_FCR_WO, fcr_reg_val);

    /* 2. set LCR: date 8bit, 1bit stop, no parity, close break condition, DLAB = 0 */
    lcr_reg_val = WB_UART_LCR_WLEN8;
    port->serial_out(port, WB_UART_LCR, lcr_reg_val);
    DEBUG_INFO("w UART_LCR, addr: 0x%x, val:  0x%x\n", WB_UART_LCR, lcr_reg_val);

#if 0
    port->mctrl |= TIOCM_OUT2;
    wb_uart_set_mctrl(port, port->mctrl);
#endif

    /* 3. clear status reg */
    tmp1 = port->serial_in(port, WB_UART_LSR);
    (void)port->serial_in(port, WB_UART_RX);
    tmp2 = port->serial_in(port, WB_UART_IIR);
    (void)port->serial_in(port, WB_UART_MSR);

    DEBUG_INFO("r UART_LSR, addr: 0x%x, val: 0x%x, tmp1: 0x%x\n", \
               WB_UART_LSR, port->serial_in(port, WB_UART_LSR), tmp1);
    DEBUG_INFO("r UART_IIR, addr: 0x%x, val: 0x%x, tmp2: 0x%x\n", \
               WB_UART_IIR, port->serial_in(port, WB_UART_IIR), tmp2);

    /* 4. set IER: enable irq */
    ier_reg_val = WB_UART_IER_RDI | WB_UART_IER_RLSI;
    port->serial_out(port, WB_UART_IER, ier_reg_val);
    DEBUG_INFO("w UART_IER, addr: 0x%x, val: 0x%x\n", WB_UART_IER, ier_reg_val);

    return;
}

static void uart_hw_deinit(struct uart_port *port)
{
    /* 1. disable irq */
    port->serial_out(port, WB_UART_IER, 0);
    DEBUG_INFO("w UART_IER, addr: 0x%x, val: 0\n", WB_UART_IER);

    /* 2. close FIFO */
    port->serial_out(port, WB_UART_FCR_WO, 0);
    DEBUG_INFO("w UART_FCR_WO, addr: 0x%x, val: 0\n", WB_UART_FCR_WO);

    return;
}

/* disable tx irq */
static void uart_disable_tx_int(struct uart_port *port)
{
    u32 ier;

    ier = port->serial_in(port, WB_UART_IER);
    DEBUG_INFO("r UART_IER, addr: 0x%x, val: 0x%x\n", WB_UART_IER, ier);

    if (ier & WB_UART_IER_THRI) {
        ier &= ~WB_UART_IER_THRI;
        port->serial_out(port, WB_UART_IER, ier);
        DEBUG_INFO("w UART_IER, addr: 0x%x, val:  0x%x\n", WB_UART_IER, ier);
    }

    return;
}

static int uart_cfg_speed_adapt(struct uart_port *port)
{
    struct wb_uart_ocore_s *p_uart_ocore;
    u32 detected_baud;

    /* 1. get p_uart_ocore */
    p_uart_ocore = up_to_wb_uart_ocore(port);
    DEBUG_INFO("uart %d: 0x%p\n", port->line, p_uart_ocore);

    /* 2. is_spd_adapt is 0, return */
    if (p_uart_ocore->is_spd_adapt == 0) {
        DEBUG_INFO("is_spd_adapt: 0\n");
        return 0;
    }

    /* 3. get detected baud */
    detected_baud = 0;
    uart_get_detected_baud(port, &detected_baud);

    /* 4. config to detected baud */
    DEBUG_INFO("current_baud: %u\n", p_uart_ocore->current_baud);
    if (detected_baud != p_uart_ocore->current_baud) {
        p_uart_ocore->current_baud = detected_baud;
        uart_set_baud_rate(port, detected_baud);
    }

    return 1;
}

/****************************start: uart ops ***********************************/
/* get uart type */
static const char *wb_uart_type(struct uart_port *port)
{
    DEBUG_VERBOSE("start\n");
    return port->type == PORT_16550 ? WB_FPGA_PORT_UART_16550 : NULL;
}

/* request mem */
static int wb_uart_request_port(struct uart_port *port)
{
    char *name;
    struct wb_uart_ocore_s *p_uart_ocore;
    p_uart_ocore = up_to_wb_uart_ocore(port);

    DEBUG_VERBOSE("start\n");
    DEBUG_INFO("port%u: iotype = 0x%x, mapbase = 0x%llx, mapsize = 0x%llx\n", \
               port->line, port->iotype, port->mapbase, port->mapsize);

    name = p_uart_ocore->name;
    DEBUG_INFO("name: %s\n", name);

    switch (port->iotype) {
        case UPIO_MEM:
            if (!request_mem_region(port->mapbase, port->mapsize, name)) {
                DEBUG_ERROR("Memory region busy, uport%u, 0x%llx, 0x%llx \n", \
                            port->line, port->mapbase, port->mapsize);
                return -EBUSY;
            }

            port->membase = ioremap(port->mapbase, port->mapsize);
            if (port->membase == NULL) {
                DEBUG_ERROR("Unable to map registers, uport%u, 0x%llx, 0x%llx \n", \
                            port->line, port->mapbase, port->mapsize);
                return -EBUSY;
            }
            DEBUG_INFO("port%u: port->membase: 0x%p\n", port->line, port->membase);

            break;

        case UPIO_PORT:
            if (!request_region(port->mapbase, port->mapsize, name)) {
                DEBUG_ERROR("io region busy, uport%u, 0x%llx, 0x%llx \n", \
                            port->line, port->mapbase, port->mapsize);
                return -EBUSY;
            }

            break;

        default:
            DEBUG_ERROR("port%u unsupport iotype: 0x%x\n", port->line, port->iotype);
            break;
    }

    DEBUG_VERBOSE("end\n");
    return 0;
}

/* release uart mem */
static void wb_uart_release_port(struct uart_port *port)
{
    DEBUG_VERBOSE("start\n");

    switch (port->iotype) {
        case UPIO_MEM:
            iounmap(port->membase);
            port->membase = NULL;
            release_mem_region(port->mapbase, port->mapsize);
            break;

        case UPIO_PORT:
            release_region(port->iobase, port->mapsize);
            port->iobase = 0;
            break;

        default:
            DEBUG_ERROR("port%u unsupport iotype: 0x%x\n", port->line, port->iotype);
            break;
    }

    DEBUG_VERBOSE("end\n");
    return;
}

/* config uart */
static void wb_uart_config_port(struct uart_port *port, int flags)
{
    DEBUG_VERBOSE("start\n");

    if (!wb_uart_request_port(port)) {
        port->type = PORT_16550;
    }

    return;
}

/* verify uart config */
static int wb_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
    /* we don't want the core code to modify any port params */
    DEBUG_VERBOSE("start\n");
    return -EINVAL;
}

/* Receive one character, and according to read_status_mask and ignore_status_mask decide whether to upload the received character */
/* return 0: no receive data; return 1, receive one character */
static int uart_rx_one_char(struct uart_port *port)
{
    unsigned char ch;
    u32 flag, lsr;

    /* 1. UART_LSR have no rx status, return */
    lsr = port->serial_in(port, WB_UART_LSR);
    DEBUG_INFO("r UART_LSR, addr: 0x%x, val: 0x%x\n", WB_UART_LSR, lsr);
    if ((lsr & (WB_UART_LSR_DR | WB_UART_LSR_BRK_ERROR_BITS)) == 0) {
        DEBUG_INFO("lsr val: 0x%x\n", lsr);
        return 0;
    }

    /* 2. Receive one character */
    ch = 0;
    flag = TTY_NORMAL;
    if (lsr & WB_UART_LSR_DR) {
        port->icount.rx++;
        ch = port->serial_in(port, WB_UART_RX);
        DEBUG_INFO("rx char: 0x%x(%c), rx count: %u\n", ch, ch, port->icount.rx);
    }

    /* 3. Receive one character, but an error occurred with this character */
    if (unlikely(lsr & WB_UART_LSR_BRK_ERROR_BITS)) {
        /* 3.1 Receive the break signal sent by the peer */
        if (lsr & WB_UART_LSR_BI) {
            lsr &= ~(WB_UART_LSR_FE | WB_UART_LSR_PE);
            port->icount.brk++;
            DEBUG_WARN("rx break, brk count: %u\n", port->icount.brk);
#if 0
            if (uart_handle_break(port)) {
                DEBUG_INFO("handle break\n");
                return 1;
            }
#endif

            /* 3.2 parity error */
        } else if (lsr & WB_UART_LSR_PE) {
            port->icount.parity++;
            DEBUG_WARN("parity err, err count: %u\n", port->icount.parity);

            /* 3.3 frame error */
        } else if (lsr & WB_UART_LSR_FE) {
            port->icount.frame++;
            DEBUG_WARN("frame err, err count: %u\n", port->icount.parity);
        }

        /* 3.4 overrun error */
        if (lsr & WB_UART_LSR_OE) {
            port->icount.overrun++;
            DEBUG_WARN("overrun err, err count: %u\n", port->icount.overrun);
        }

        /* 3.5 set flag */
        lsr &= port->read_status_mask;
        if (lsr & WB_UART_LSR_BI) {
            flag = TTY_BREAK;
        } else if (lsr & WB_UART_LSR_PE) {
            flag = TTY_PARITY;
        } else if (lsr & WB_UART_LSR_FE) {
            flag = TTY_FRAME;
        }
    }

#if 0
    if (magic_key_identify(port, ch)) {
        return 1;
    }
#endif

    /* 4. set character to TTY buffer */
    DEBUG_INFO("uart%d, lsr: 0x%x, ch: 0x%x(%c), flag: %u\n", \
               port->line, lsr, ch, ch, flag);
    uart_insert_char(port, lsr, WB_UART_LSR_OE, ch, flag);

    return 1;
}

static void uart_rx_chars(struct uart_port *port)
{
    int i;
    int ret;
    int rx_fifo_size;

    rx_fifo_size = WB_UART_MAX_RX_FIFO_SIZE;
    for (i = 0; i < rx_fifo_size; i++) {
        DEBUG_INFO("uart%d: i = %d\n", port->line, i);
        ret = uart_rx_one_char(port);
        if (ret == 0) {
            break;
        }
    }

    /* push to buffer */
    tty_flip_buffer_push(&port->state->port);

    return;
}

static void uart_tx_chars(struct uart_port *port)
{
    struct circ_buf *xmit;
    int i;
#if 0
    u32 lsr;
#endif

    xmit = &port->state->xmit;

    /* 1. if have x_char, then send it */
    if (port->x_char) {
        DEBUG_INFO("uart%u, tx x_char %d\n", port->line, port->x_char);
        port->serial_out(port, WB_UART_TX, port->x_char);
        port->icount.tx++;
        port->x_char = 0;
        return;
    }

    /* 2. ring buf is empty or tx stopped */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
        uart_disable_tx_int(port);
        DEBUG_INFO("uart%u, circ empty: %d\n", port->line, uart_circ_empty(xmit));
        return;
    }

    /* 3. write tx data to TX FIFO */
    for (i = 0; i < port->fifosize; i++) {
        /* 3.1 Send one character */
        DEBUG_INFO("i = %d, tx char = 0x%x(%c)\n", i, xmit->buf[xmit->tail], xmit->buf[xmit->tail]);
        port->serial_out(port, WB_UART_TX, xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;

        /* 3.2 ring buf is empty, all data transmit, break */
        if (uart_circ_empty(xmit)) {
            break;
        }

#if 0
        /* 3.3 LSR reg, TX FIFO have data transmit */
        lsr = port->serial_in(port, UART_LSR);
        DEBUG_INFO("r UART_LSR, addr: 0x%x, val: 0x%x\n", UART_LSR, lsr);
        if (lsr & BOTH_EMPTY != BOTH_EMPTY) {
            break;
        }
#endif
    }

    /* 4. xmit number less than WAKEUP_CHARS, wake up xmit thread */
    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
        DEBUG_INFO("uart_write_wakeup\n");
        uart_write_wakeup(port);
    }

    /* 5. ring buf is empty, disable tx irq */
    if (uart_circ_empty(xmit)) {
        uart_disable_tx_int(port);
    }

    return;
}

#if 0
static void dump_reg_info(struct uart_port *port)
{
    u32 reg_val, lcr_reg_val;
    int offset;
    int i;
    int port_regs_0[] = {UART_RX, UART_IER, UART_IIR, UART_LCR, UART_MCR, UART_LSR, UART_MSR};
    int port_regs_1[] = {UART_DLL, UART_DLM, UART_FCR_RO};
    int port_common_regs[] = {UART_REG_SPD_CFG_VAL, UART_REG_SPD_DETECT, \
                              UART_REG_SPD_RELIABLE, UART_REG_IMPLICIT0, \
                              UART_REG_IMPLICIT1, UART_REG_IMPLICIT2
                             };

    /* dump uart reg info @DLAB = 0 */
    for (i = 0; i < ARRAY_SIZE(port_regs_0); i++) {
        reg_val = port->serial_in(port, port_regs_0[i]);
        DEBUG_INFO("r addr: 0x%x, val: 0x%x\n", port_regs_0[i], reg_val);
    }

    /* dump uart reg info @DLAB = 1 */
    lcr_reg_val = port->serial_in(port, UART_LCR);
    port->serial_out(port, UART_LCR, (int)(lcr_reg_val | UART_LCR_DLAB));
    for (i = 0; i < ARRAY_SIZE(port_regs_1); i++) {
        reg_val = port->serial_in(port, port_regs_1[i]);
        DEBUG_INFO("r addr: 0x%x, val: 0x%x\n", port_regs_1[i], reg_val);
    }
    port->serial_out(port, UART_LCR, (int)lcr_reg_val);

    /* dump uart common reg info */
    for (i = 0; i < ARRAY_SIZE(port_common_regs); i++) {
        offset = port_common_regs[i] - ((unsigned long)port->membase & (~PAGE_MASK));
        reg_val = port->serial_in(port, offset);
        DEBUG_INFO("r addr: 0x%x, val: 0x%x, offset: 0x%x\n", port_common_regs[i], reg_val, offset);
    }

    return;
}
#endif

/* tx rx irq */
static irqreturn_t wb_uart_isr(int irq, void *dev_id)
{
    struct uart_port *port = dev_id;
    unsigned long flags;
    u32 iir, lsr, ier;

    DEBUG_VERBOSE("start\n");

    /* 1. get UART_IIR */
    iir = port->serial_in(port, WB_UART_IIR);
    DEBUG_INFO("r UART_IIR, addr: 0x%x, val: 0x%x\n", WB_UART_IIR, iir);

    /* 2. no irq, return */
    if (iir & WB_UART_IIR_NO_INT) {
        DEBUG_INFO("no int, direct exit\n");
#if 0
        dump_reg_info(port);
#endif
        return IRQ_NONE;
    }

    /* 3. Off interrupt The interrupt processing is performed */
    spin_lock_irqsave(&port->lock, flags);
    lsr = port->serial_in(port, WB_UART_LSR);
    DEBUG_INFO("r UART_LSR, addr: 0x%x, val: 0x%x\n", WB_UART_LSR, lsr);

    ier = port->serial_in(port, WB_UART_IER);
    DEBUG_INFO("r UART_IER, addr: 0x%x, val: 0x%x\n", WB_UART_IER, ier);

    /* 3.1 Receive interrupt processing */
    if (lsr & (WB_UART_LSR_DR | WB_UART_LSR_BRK_ERROR_BITS)) {
        uart_rx_chars(port);
    }

    /* 3.2 Send interrupt processing. Entry condition: FIFO is empty + Send interrupt is enabled */
    if ((lsr & WB_BOTH_EMPTY) && (ier & WB_UART_IER_THRI)) {
        uart_tx_chars(port);
    }
    spin_unlock_irqrestore(&port->lock, flags);

    return IRQ_HANDLED;
}

/* Start the serial port: initializes the serial port */
static int wb_uart_startup(struct uart_port *port)
{
    struct wb_uart_ocore_s *p_uart_ocore;
    unsigned long flags;

    DEBUG_VERBOSE("start\n");

    /* 1. Indicates that the serial port is enabled */
    p_uart_ocore = up_to_wb_uart_ocore(port);
    p_uart_ocore->is_active = 1;

    /* 2.clear ring buf */
    if (!uart_circ_empty(&port->state->xmit)) {
        uart_circ_clear(&port->state->xmit);
    }

    /* 3. Port rate adaptive configuration */
    spin_lock_irqsave(&port->lock, flags);
    (void)uart_cfg_speed_adapt(port);

    /* 4. Initialize hardware */
    uart_hw_init(port);

    spin_unlock_irqrestore(&port->lock, flags);

    DEBUG_VERBOSE("end\n");
    return 0;
}

/* Close the serial port: releases the interrupt and performs deinitialization */
static void wb_uart_shutdown(struct uart_port *port)
{
    struct wb_uart_ocore_s *p_uart_ocore;

    DEBUG_VERBOSE("start\n");
    uart_hw_deinit(port);

    p_uart_ocore = up_to_wb_uart_ocore(port);
    p_uart_ocore->is_active = 0;
    DEBUG_VERBOSE("end\n");

    return;
}

static void wb_uart_stop_tx(struct uart_port *port)
{
    DEBUG_VERBOSE("start\n");
    uart_disable_tx_int(port);
    return;
}

/* Used to send characters */
static void  wb_uart_start_tx(struct uart_port *port)
{
    u32 ier;
    u32 lsr;

    /* 1. enable tx buf empty irq */
    ier = port->serial_in(port, WB_UART_IER);
    DEBUG_INFO("r UART_IER, addr: 0x%x, val: 0x%x\n", WB_UART_IER, ier);
    if (!(ier & WB_UART_IER_THRI)) {
        ier |= WB_UART_IER_THRI;
        port->serial_out(port, WB_UART_IER, ier);
    }

    /* 2. transmit ring buf characters */
    lsr = port->serial_in(port, WB_UART_LSR);
    DEBUG_INFO("r UART_LSR, addr: 0x%x, val: 0x%x\n", WB_UART_LSR, lsr);
    if (lsr & WB_UART_LSR_THRE) {
        uart_tx_chars(port);
    }

    return;
}

/* Indicates whether the send queue is empty */
static unsigned int wb_uart_tx_empty(struct uart_port *port)
{
    unsigned long flags;
    u32 stat;

    DEBUG_VERBOSE("start\n");

    spin_lock_irqsave(&port->lock, flags);
    stat = port->serial_in(port, WB_UART_LSR);
    spin_unlock_irqrestore(&port->lock, flags);

    DEBUG_INFO("stat: 0x%x\n", stat);

    return (stat & WB_BOTH_EMPTY) == WB_BOTH_EMPTY ? TIOCSER_TEMT : 0;
}

static unsigned int wb_uart_get_mctrl(struct uart_port *port)
{
    /* FPGA only RX and TX signals are used, and no other signals of the modem are generated. Therefore, this interface is not needed */
    /* The terminal device is allowed to send data, the modem is ready, and the carrier is detected on the line */
    DEBUG_VERBOSE("start\n");
    return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void wb_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    /* FPGA uses only RX and TX signals, and the other signals of the modem are not extracted, and this interface is not possible */
    DEBUG_VERBOSE("start\n");

#if 0
    u32 mcr;
    DEBUG_VERBOSE("start\n");

    DEBUG_INFO("uart%d, mctrl = 0x%x\n", port->line, mctrl);
    mcr = 0;
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

    port->serial_out(port, UART_MCR, mcr);
    DEBUG_INFO("w UART_MCR, addr: 0x%x, val:  0x%x\n", UART_MCR, mcr);
#endif

    return;
}

static void wb_uart_stop_rx(struct uart_port *port)
{
#if 0
    u32 ier;

    /* 1. Close receive interrupt */
    ier = port->serial_in(port, UART_IER);
    ier &= ~(UART_IER_RLSI | UART_IER_RDI);
    port->serial_out(port, UART_IER, ier);
#endif
    DEBUG_VERBOSE("start\n");

    /* 2. Set the status mask so that interrupted received characters are not sent */
    port->read_status_mask &= ~WB_UART_LSR_DR;

    return;
}

#if 0
/* break signal control: that is, set the TX signal to force 0 to generate the break condition */
static void wb_uart_break_ctl(struct uart_port *port, int ctl)
{
    unsigned long flags;
    u32 lcr;

    DEBUG_INFO("uart%d: \n", port->line);
    spin_lock_irqsave(&port->lock, flags);

    /* read --- modify --- write */
    lcr = port->serial_in(port, UART_LCR);
    DEBUG_INFO("r UART_LCR, addr: 0x%x, val: 0x%x\n", UART_LCR, lcr);

    if (ctl == -1) {
        lcr |= UART_LCR_SBC;
    } else {
        lcr &= ~UART_LCR_SBC;
    }

    port->serial_out(port, UART_LCR, lcr);
    DEBUG_INFO("w UART_LCR, addr: 0x%x, val: 0x%x\n", UART_LCR, lcr);

    spin_unlock_irqrestore(&port->lock, flags);

    return;
}
#endif

/* The value of the LCR register is configured according to the value of the terminal control flag */
static void uart_cfg_lcr(struct uart_port *port, tcflag_t c_cflag)
{
    int cval;

    DEBUG_INFO("uart%d: c_cflag 0x%x\n", port->line, c_cflag);

    /* 1. Determine the number of bits of a single data piece */
    switch (c_cflag & CSIZE) {
        case CS5:
            cval = WB_UART_LCR_WLEN5;
            break;
        case CS6:
            cval = WB_UART_LCR_WLEN6;
            break;
        case CS7:
            cval = WB_UART_LCR_WLEN7;
            break;
        default:
        case CS8:
            cval = WB_UART_LCR_WLEN8;
            break;
    }

    /* 2. Check the number of stop bits */
    if (c_cflag & CSTOPB) {
        cval |= WB_UART_LCR_STOP;
    }

    /* 3. Check whether parity is enabled */
    if (c_cflag & PARENB) {
        cval |= WB_UART_LCR_PARITY;
    }

    /* 4. Verify whether to use odd or even check */
    if (!(c_cflag & PARODD)) {
        cval |= WB_UART_LCR_EPAR;
    }

    /* 5. Confirm whether to use Stick Parity */
    if (c_cflag & CMSPAR) {
        cval |= WB_UART_LCR_SPAR;
    }

    port->serial_out(port, WB_UART_LCR, cval);
    DEBUG_INFO("w UART_LCR, addr: 0x%x, val:  0x%x\n", WB_UART_LCR, cval);

    return;
}

/* This interface is used to set serial port parameters such as eg: baud rate of the serial port and number of data bits of the serial port */
static void wb_uart_set_termios(struct uart_port *port, struct ktermios *termios,
                                const struct ktermios *old)
{
    unsigned long flags;
    unsigned int baud;
    u32 cur_baud;
    u32 ier;

    DEBUG_VERBOSE("start\n");

    DEBUG_INFO("c_iflag: 0x%x, c_oflag: 0x%x, c_cflag: 0x%x\n", \
               termios->c_iflag, termios->c_oflag, termios->c_cflag);

    spin_lock_irqsave(&port->lock, flags);

    /* 1. Configure the data bit width, number of stop bits, and parity check */
    uart_cfg_lcr(port, termios->c_cflag);

    /* 2. Set the read status mask, which is used when data is received. That is, the software only cares about the configured status bit */
    port->read_status_mask = WB_UART_LSR_DR | WB_UART_LSR_OE | WB_UART_LSR_THRE;

    /* 2.1 If this flag is set, parity is allowed*/
    if (termios->c_iflag & INPCK) {
        port->read_status_mask |= WB_UART_LSR_PE | WB_UART_LSR_FE;
    }

    /* 2.2 With this flag set, you need to pay attention to the Break signal */
    if (termios->c_iflag & (IGNBRK | BRKINT | PARMRK)) {
        port->read_status_mask |= WB_UART_LSR_BI;
    }

    DEBUG_INFO("uart%d: read_status_mask = 0x%x\n", port->line, port->read_status_mask);

    /* 3. Ignore state mask */
    port->ignore_status_mask = 0;

    /* 3.1 If this flag is set, parity errors are ignored. That is, do not upload the received characters for which parity has occurred*/
    if (termios->c_iflag & IGNPAR) {
        port->ignore_status_mask |= WB_UART_LSR_PE | WB_UART_LSR_FE;
    }

    /* 3.2 If this flag is set, the Break error is ignored. That is, do not upload the received Break character */
    if (termios->c_iflag & IGNBRK) {
        port->ignore_status_mask |= WB_UART_LSR_BI;
        /*
         * If we're ignoring parity and break indicators,
         * ignore overruns too (for real raw support).
         */
        if (termios->c_iflag & IGNPAR) {
            port->ignore_status_mask |= WB_UART_LSR_OE;
        }
    }

    /* 3.3. If this flag is not set, none of the received characters will be uploaded */
    if ((termios->c_cflag & CREAD) == 0) {
        port->ignore_status_mask |= WB_UART_LSR_DR;
    }

    DEBUG_INFO("uart%d: ignore_status_mask = 0x%x\n", port->line, port->ignore_status_mask);

    /* 4. Configure the baud rate of the serial port */
    /* 4.1 If the rate is adaptive, the system ignores the configuration delivered by the upper layer */
    if (uart_cfg_speed_adapt(port)) {
        baud = 0;
        uart_get_detected_baud(port, &baud);
        DEBUG_INFO("spd adapt have cfged, current baud: %u\n", baud);
        goto out;
    }

    /* 4.2 Obtain the baud rate to be set */
    baud = uart_get_baud_rate(port, termios, old, BAUD_9600, BAUD_115200);
    if ((baud != BAUD_9600) && (baud != BAUD_115200)) {
        DEBUG_WARN("baud is not 9600 or 115200, but is %u\n", baud);
        baud = BAUD_9600;
        goto out;
    }

    /* 4.3 According to the configuration of DLM & DLL frequency division register, the current baud rate is calculated */
    cur_baud = uart_probe_baud(port);
    DEBUG_INFO("cur_baud: %u, will set baud = %u\n", cur_baud, baud);

    /* 4.4 The baud rate is set at the bottom layer */
    uart_set_baud_rate(port, baud);

    /* 5. The receiving interrupt was enabled */
    ier = port->serial_in(port, WB_UART_IER);
    DEBUG_INFO("r UART_IER, addr: 0x%x, val: 0x%x\n", WB_UART_IER, ier);
    ier |= WB_UART_IER_RLSI | WB_UART_IER_RDI;
    port->serial_out(port, WB_UART_IER, ier);
    DEBUG_INFO("w UART_IER, addr: 0x%x, val: 0x%x\n", WB_UART_IER, ier);

    /* 6. Update timeout parameter */
out:
    uart_update_timeout(port, termios->c_cflag, baud);
    spin_unlock_irqrestore(&port->lock, flags);

    DEBUG_VERBOSE("end\n");
    return;
}

/* Description The modem status is interrupted */
#if 0
static void wb_uart_enable_ms(struct uart_port *port)
{
    DEBUG_VERBOSE("start\n");
    return;
}
#endif

static const struct uart_ops wb_uart_16550_ops = {
    .tx_empty        = wb_uart_tx_empty,
    .set_mctrl       = wb_uart_set_mctrl,
    .get_mctrl       = wb_uart_get_mctrl,
    .stop_tx         = wb_uart_stop_tx,
    .start_tx        = wb_uart_start_tx,
    .stop_rx         = wb_uart_stop_rx,
#if 0
    .break_ctl       = wb_uart_break_ctl,
#endif
    .set_termios     = wb_uart_set_termios,
    .startup         = wb_uart_startup,
    .shutdown        = wb_uart_shutdown,
    .request_port    = wb_uart_request_port,
    .release_port    = wb_uart_release_port,
    .config_port     = wb_uart_config_port,
    .verify_port     = wb_uart_verify_port,
#if 0
    .enable_ms       = wb_uart_enable_ms,
#endif
    .type            = wb_uart_type,
};

/****************************end:Serial port operation interface ***********************************/

/****************************start:sysfs debugging interface ***********************************/
static struct wb_uart_ocore_s* up_to_wb_uart_ocore_by_tport(struct tty_port *tport)
{
    struct uart_state     *state;
    struct uart_port      *uport;
    struct wb_uart_ocore_s *p_uart_ocore;

    state = container_of(tport, struct uart_state, port);
    uport = state->uart_port;
    p_uart_ocore = container_of(uport, struct wb_uart_ocore_s, port);

    return p_uart_ocore;
}

/* dev refers to the tty device whose driver data points to struct tty_port */
static ssize_t wb_uart_show_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct tty_port *tport;
    struct wb_uart_ocore_s *p_uart_ocore;
    struct uart_port *port;
    ssize_t size;
    u32 reg_val, lcr_reg_val;
    int offset;
    int i;
    int port_regs_0[] = {WB_UART_RX, WB_UART_IER, WB_UART_IIR, WB_UART_LCR, WB_UART_MCR, WB_UART_LSR, WB_UART_MSR};
    int port_regs_1[] = {WB_UART_DLL, WB_UART_DLM, WB_UART_FCR_RO};
    int port_common_regs[] = {WB_UART_REG_SPD_CFG_VAL, WB_UART_REG_SPD_DETECT, \
                              WB_UART_REG_SPD_RELIABLE, WB_UART_REG_IMPLICIT0, \
                              WB_UART_REG_IMPLICIT1, WB_UART_REG_IMPLICIT2
                             };

    DEBUG_VERBOSE("start\n");

    tport = dev_get_drvdata(dev);
    p_uart_ocore = up_to_wb_uart_ocore_by_tport(tport);
    port = &p_uart_ocore->port;
    DEBUG_INFO("p_uart%u: 0x%p\n", p_uart_ocore->port.line, p_uart_ocore);

    size = snprintf(buf, PAGE_SIZE, "uart%u:\n", p_uart_ocore->port.line);
    size += snprintf(buf + size, PAGE_SIZE - size, "is speed adapt: %u\n", \
                     p_uart_ocore->is_spd_adapt);

    /* dump uart reg info @DLAB = 0 */
    for (i = 0; i < ARRAY_SIZE(port_regs_0); i++) {
        reg_val = port->serial_in(port, port_regs_0[i]);
        size += snprintf(buf + size, PAGE_SIZE - size, "reg: 0x%x, val: 0x%x\n", \
                         port_regs_0[i], reg_val);
    }

    /* dump uart reg info @DLAB = 1 */
    lcr_reg_val = port->serial_in(port, WB_UART_LCR);
    port->serial_out(port, WB_UART_LCR, (int)(lcr_reg_val | WB_UART_LCR_DLAB));
    for (i = 0; i < ARRAY_SIZE(port_regs_1); i++) {
        reg_val = port->serial_in(port, port_regs_1[i]);
        size += snprintf(buf + size, PAGE_SIZE - size, "reg: 0x%x, val: 0x%x\n", \
                         port_regs_1[i], reg_val);
    }
    port->serial_out(port, WB_UART_LCR, (int)lcr_reg_val);

    /* dump uart common reg info */
    for (i = 0; i < ARRAY_SIZE(port_common_regs); i++) {
        offset = port_common_regs[i] - ((unsigned long)p_uart_ocore->port.membase & (~PAGE_MASK));
        reg_val = port->serial_in(port, offset);
        size += snprintf(buf + size, PAGE_SIZE - size, "reg: 0x%x, val: 0x%x, offset: 0x%x\n", \
                         port_common_regs[i], reg_val, offset);
    }

    DEBUG_VERBOSE("end\n");
    return size;
}

static ssize_t wb_uart_rw_reg(struct device *dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct tty_port *tport;
    struct wb_uart_ocore_s *p_uart_ocore;
    struct uart_port *port;
    int reg_addr, reg_val;
    char rw_flag;
    int ret;
    unsigned long flags;

    DEBUG_VERBOSE("start\n");

    if (count == 0) {
        DEBUG_ERROR("input is null\n");
        return -EINVAL;
    }

    rw_flag  = 0;
    reg_addr = 0;
    reg_val  = 0;
    ret = sscanf(buf, "%c %x %x", &rw_flag, &reg_addr, &reg_val);
    if (ret != 3) {
        printk(KERN_ERR "para err, usage: echo r/w addr val > /sys/xxx \n");
        return count;
    }

    printk(KERN_ERR "\n intput para: %c 0x%x 0x%x\n", rw_flag, reg_addr, reg_val);

    tport = dev_get_drvdata(dev);
    p_uart_ocore = up_to_wb_uart_ocore_by_tport(tport);
    port = &p_uart_ocore->port;
    printk(KERN_ERR "uart%u: \n", port->line);

    spin_lock_irqsave(&port->lock, flags);

    if (rw_flag == 'w') {
        port->serial_out(port, reg_addr, reg_val);
        printk(KERN_ERR "w addr: 0x%x, val: 0x%x\n", reg_addr, reg_val);
    } else {
        reg_val = (int)port->serial_in(port, reg_addr);
        printk(KERN_ERR "r addr: 0x%x, val: 0x%x\n", reg_addr, reg_val);
    }
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static ssize_t get_all_uart_spd_adapt_info(char *buf, ssize_t size)
{
    struct list_head *n, *pos;
    struct wb_uart_ocore_s *p_uart_ocore;
    ssize_t size_tmp;

    spin_lock(&wb_uart_ocore_list_lock);
    size_tmp = size;
    list_for_each_safe(pos, n, &wb_uart_ocore_list) {
        p_uart_ocore = list_entry(pos, struct wb_uart_ocore_s, list);
        size_tmp += snprintf(buf + size_tmp, PAGE_SIZE - size_tmp, "uart%u, is speed adapt: %u\n", \
                             p_uart_ocore->port.line, p_uart_ocore->is_spd_adapt);
    }
    spin_unlock(&wb_uart_ocore_list_lock);

    return size_tmp;
}

static ssize_t wb_uart_show_spd_adapt(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct tty_port *tport;
    struct wb_uart_ocore_s *p_uart_ocore;
    ssize_t size;

    DEBUG_VERBOSE("start\n");

    tport = dev_get_drvdata(dev);
    p_uart_ocore = up_to_wb_uart_ocore_by_tport(tport);
    DEBUG_INFO("uart%u: 0x%p\n", p_uart_ocore->port.line, p_uart_ocore);

    /* dump current uart spd adapt info */
    size = snprintf(buf, PAGE_SIZE, "current uart: uart%u, is speed adapt: %u\n", \
                    p_uart_ocore->port.line, p_uart_ocore->is_spd_adapt);

    /* dump all registered uart spd adapt info */
    size = get_all_uart_spd_adapt_info(buf, size);

    DEBUG_VERBOSE("end\n");

    return size;
}

static ssize_t wb_uart_store_spd_adapt(struct device *dev, struct device_attribute *attr,
                                       const char *buf, size_t count)
{
    struct tty_port *tport;
    struct wb_uart_ocore_s *p_uart_ocore;
    unsigned long val;
    unsigned long flags;

    DEBUG_VERBOSE("start\n");

    if (count == 0) {
        DEBUG_ERROR("input is null\n");
        return -EINVAL;
    }

    tport = dev_get_drvdata(dev);
    p_uart_ocore = up_to_wb_uart_ocore_by_tport(tport);
    DEBUG_INFO("uart%u: 0x%p\n", p_uart_ocore->port.line, p_uart_ocore);

    val = simple_strtoul(buf, NULL, 0);
    if (val == 0) {
        p_uart_ocore->is_spd_adapt = 0;
    } else {
        p_uart_ocore->is_spd_adapt = 1;
    }

    spin_lock_irqsave(&p_uart_ocore->port.lock, flags);
    (void)uart_cfg_speed_adapt(&p_uart_ocore->port);
    spin_unlock_irqrestore(&p_uart_ocore->port.lock, flags);

    DEBUG_VERBOSE("end\n");

    return count;
}

static struct device_attribute dev_attr_ulite_info;
static struct device_attribute dev_attr_ulite_spd_adapt;

static DEVICE_ATTR(ulite_info, S_IWUSR | S_IRUSR | S_IRGRP, \
                   wb_uart_show_info, wb_uart_rw_reg);
static DEVICE_ATTR(ulite_spd_adapt, S_IWUSR | S_IRUSR | S_IRGRP, \
                   wb_uart_show_spd_adapt, wb_uart_store_spd_adapt);

static struct attribute *wb_uart_dev_attrs[] = {
    &dev_attr_ulite_info.attr,
    &dev_attr_ulite_spd_adapt.attr,
    NULL,
};

/* uart port Characteristic attribute*/
static struct attribute_group wb_uart_dev_attr_group = {
    .attrs = wb_uart_dev_attrs,
};
/****************************end:sysfs debugging interface ***********************************/

static struct uart_driver wb_uart_driver = {
    .owner       = THIS_MODULE,
    .driver_name = "wb-uart-ocore",     /* Driver name, that is, tty_driver driver name */
    .dev_name    = "ttyCS",             /* Character device name prefixes for all tty ports attached to a tty controller: ttyCS, CS: Card Serial */
    .major       = UART_MAJOR,          /* Master device number */
    .minor       = UART_MINOR,          /* Secondary device number */
    .nr          = MAX_TTYRG_NUM,           /* Number of tty devices to be applied for */
};

/* Memory mapping mode, small endian format 32/16/8 bit register operation interface */
static u32 wb_uart_read32(struct uart_port *port, int offset)
{
    return ioread32(port->membase + offset);
}

static void wb_uart_write32(struct uart_port *port, int offset, int value)
{
    iowrite32(value, (port->membase + offset));
    return;
}

static u32 wb_uart_read16(struct uart_port *port, int offset)
{
    return (u32)ioread16(port->membase + offset);
}

static void wb_uart_write16(struct uart_port *port, int offset, int value)
{
    iowrite16((u16)value, (port->membase + offset));
    return;
}

static u32 wb_uart_read32be(struct uart_port *port, int offset)
{
    return ioread32be(port->membase + offset);
}

static void wb_uart_write32be(struct uart_port *port, int offset, int value)
{
    iowrite32be(value, (port->membase + offset));
    return;
}

static u32 wb_uart_read16be(struct uart_port *port, int offset)
{
    return (u32)ioread16be(port->membase + offset);
}

static void wb_uart_write16be(struct uart_port *port, int offset, int value)
{
    iowrite16be((u16)value, (port->membase + offset));
    return;
}

static u32 wb_uart_read8(struct uart_port *port, int offset)
{
    return (u32)ioread8(port->membase + offset);
}

static void wb_uart_write8(struct uart_port *port, int offset, int value)
{
    iowrite8((u8)value, (port->membase + offset));
    return;
}

/* In I/O mapping mode, 32-bit 16/8 bit register operation interface in small-endian format */
static u32 wb_uart_in32(struct uart_port *port, int offset)
{
    return inl((unsigned long)port->iobase + offset);
}

static void wb_uart_out32(struct uart_port *port, int offset, int value)
{
    outl(value, (unsigned long)(port->iobase + offset));
    return;
}

static u32 wb_uart_in16(struct uart_port *port, int offset)
{
    return (u32)inw((unsigned long)port->iobase + offset);
}

static void wb_uart_out16(struct uart_port *port, int offset, int value)
{
    outw((u16)value, (unsigned long)(port->iobase + offset));
    return;
}

static u32 wb_uart_in8(struct uart_port *port, int offset)
{
    return (u32)inb((unsigned long)port->iobase + offset);
}

static void wb_uart_out8(struct uart_port *port, int offset, int value)
{
    outb((u8)value, (unsigned long)(port->iobase + offset));
    return;
}

static void init_port_reg_ops(struct uart_port *port, unsigned long flags, u32 reg_width, u32 is_le)
{
    DEBUG_VERBOSE("start\n");
    DEBUG_INFO("flags: 0x%lx, reg_width: %u, is_le: %u\n", flags, reg_width, is_le);

    /* Is memory mapped and the device is in small-end format */
    if ((flags == IORESOURCE_MEM) && is_le) {
        if (reg_width == REG_WIDTH_4B) {
            port->serial_in  = wb_uart_read32;
            port->serial_out = wb_uart_write32;
        } else if (reg_width == REG_WIDTH_2B) {
            port->serial_in  = wb_uart_read16;
            port->serial_out = wb_uart_write16;
        } else {
            port->serial_in  = wb_uart_read8;
            port->serial_out = wb_uart_write8;
        }
    } else if ((flags == IORESOURCE_MEM) && (!is_le)) {
        if (reg_width == REG_WIDTH_4B) {
            port->serial_in  = wb_uart_read32be;
            port->serial_out = wb_uart_write32be;
        } else if (reg_width == REG_WIDTH_2B) {
            port->serial_in  = wb_uart_read16be;
            port->serial_out = wb_uart_write16be;
        } else {
            port->serial_in  = wb_uart_read8;
            port->serial_out = wb_uart_write8;
        }
    } else {
        if (reg_width == REG_WIDTH_4B) {
            port->serial_in  = wb_uart_in32;
            port->serial_out = wb_uart_out32;
        } else if (reg_width == REG_WIDTH_2B) {
            port->serial_in  = wb_uart_in16;
            port->serial_out = wb_uart_out16;
        } else {
            port->serial_in  = wb_uart_in8;
            port->serial_out = wb_uart_out8;
        }
    }

    DEBUG_VERBOSE("end\n");
    return;
}

static int wb_uart_probe(struct platform_device *pdev)
{
    int ret;
    u32 port_index, irq_offset, irq, bar_index, reg_width, reg_offset, map_size, is_le;
    u32 domain, bus_num, devfn, vid, did;
    u32 clk_freq, tx_fifo_size;
    unsigned long flags;
    struct pci_dev *pcidev;
    resource_size_t start;
    struct wb_uart_ocore_s *p_uart_ocore, *p_uart_ocore_temp;
    uart_16550_ocore_device_t *uart_16550_ocore_device;

    DEBUG_VERBOSE("start\n");

    /* 1. param init */
    port_index = 0;
    irq_offset = 0;
    domain     = 0;
    bus_num    = 0;
    devfn      = 0;
    vid        = 0;
    did        = 0;
    bar_index  = 0;
    reg_width  = 0;
    reg_offset = 0;
    map_size   = 0;
    is_le      = 0;
    clk_freq   = 0;
    tx_fifo_size = 0;

    /* 2. Obtain device param information and check the validity of the data. The parameters include serial port index and interrupt offset */
    /* 2.1 Data information acquisition */
    if (pdev->dev.of_node) {
        ret  = of_property_read_u32(pdev->dev.of_node, "port_index",   &port_index);
        ret |= of_property_read_u32(pdev->dev.of_node, "irq_offset",   &irq_offset);
        ret |= of_property_read_u32(pdev->dev.of_node, "domain",       &domain);
        ret |= of_property_read_u32(pdev->dev.of_node, "bus_num",      &bus_num);
        ret |= of_property_read_u32(pdev->dev.of_node, "devfn",        &devfn);
        ret |= of_property_read_u32(pdev->dev.of_node, "vid",          &vid);
        ret |= of_property_read_u32(pdev->dev.of_node, "did",          &did);
        ret |= of_property_read_u32(pdev->dev.of_node, "bar_index",    &bar_index);
        ret |= of_property_read_u32(pdev->dev.of_node, "reg_width",    &reg_width);
        ret |= of_property_read_u32(pdev->dev.of_node, "reg_offset",   &reg_offset);
        ret |= of_property_read_u32(pdev->dev.of_node, "map_size",     &map_size);
        ret |= of_property_read_u32(pdev->dev.of_node, "is_le",        &is_le);
        ret |= of_property_read_u32(pdev->dev.of_node, "clk_freq",     &clk_freq);
        ret |= of_property_read_u32(pdev->dev.of_node, "tx_fifo_size", &tx_fifo_size);

        if (ret != 0) {
            DEBUG_ERROR("read dts propert fail, ret = %d\n", ret);
            return -EINVAL;
        }
    } else {
        if (pdev->dev.platform_data == NULL) {
            dev_err(&pdev->dev, "Failed to get platform data config.\n");
            return -ENXIO;
        }
        uart_16550_ocore_device = pdev->dev.platform_data;
        port_index = uart_16550_ocore_device->port_index;
        irq_offset = uart_16550_ocore_device->irq_offset;
        domain = uart_16550_ocore_device->domain;
        bus_num = uart_16550_ocore_device->bus_num;
        devfn = uart_16550_ocore_device->devfn;
        vid = uart_16550_ocore_device->vid;
        did = uart_16550_ocore_device->did;
        bar_index = uart_16550_ocore_device->bar_index;
        reg_width = uart_16550_ocore_device->reg_width;
        reg_offset = uart_16550_ocore_device->reg_offset;
        map_size = uart_16550_ocore_device->map_size;
        is_le = uart_16550_ocore_device->is_le;
        clk_freq = uart_16550_ocore_device->clk_freq;
        tx_fifo_size = uart_16550_ocore_device->tx_fifo_size;
    }

    DEBUG_INFO("port_index: %u, irq_offset: %u, domain = %u, bus_num = %u, devfn = 0x%x, bar_index: %u,  " \
               "reg_width: %u, reg_offset: 0x%x, map_size: 0x%x, is_le: %u, clk_freq: %u, ret = %d, " \
               "tx_fifo_size: %u, vid: 0x%x, did: 0x%x\n",
               port_index, irq_offset, domain, bus_num, devfn, bar_index, reg_width, reg_offset, \
               map_size, is_le, clk_freq, ret, tx_fifo_size, vid, did);

    /* 2.2 param judgment */
    if ((port_index >= MAX_TTYRG_NUM) || (irq_offset >= WB_FPGA_MAX_MSI_IRQ_NUM)) {
        DEBUG_ERROR("port index %u or irq offset %u is out of bound\n", port_index, irq_offset);
        return -EINVAL;
    }

    /* 2.3 Iterate over the linked list to see if uart_port with index number port_index has been added */
    spin_lock(&wb_uart_ocore_list_lock);
    list_for_each_entry(p_uart_ocore_temp, &wb_uart_ocore_list, list) {
        if (p_uart_ocore_temp->port.line == port_index) {
            DEBUG_ERROR("uart port%u has already added\n", port_index);
            spin_unlock(&wb_uart_ocore_list_lock);
            return -EINVAL;
        }
    }
    spin_unlock(&wb_uart_ocore_list_lock);

    /* 3. Run the bus num: devid. funcid command to obtain the PCIe device and then obtain the BASE irq number of the PCIe device */
    pcidev = pci_get_domain_bus_and_slot(domain, bus_num, devfn);
    if (pcidev == NULL) {
        DEBUG_ERROR("get pci dev fail, domain %u, bus_num %u, devfn 0x%x\n", \
                    domain, bus_num, devfn);
        return -ENODEV;
    }

    /* If the vendor ID and device ID of the pcie device are different from those specified in the dts, exit */
    if ((pcidev->vendor != vid) || (pcidev->device != did)) {
        DEBUG_ERROR("vid or did not match: 0x%x, 0x%x, 0x%x, 0x%x\n", \
                    pcidev->vendor, vid, pcidev->device, did);
        return -ENODEV;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,8,0)
    irq = pci_irq_vector(pcidev, irq_offset); /* pcidev->irq + irq_offset; */
#else
    irq = pcidev->irq + irq_offset;
#endif
    DEBUG_INFO("base_irq: %u, irq: %u, %u\n", pcidev->irq, irq_offset, pcidev->irq + irq_offset);

    /* 4. Gets the base address to map and the mapped space type tag from the BAR space of the pci device */
    start = pci_resource_start(pcidev, bar_index);
    flags = pci_resource_flags(pcidev, bar_index);
    DEBUG_INFO("start: 0x%llx, flags: 0x%lx", start, flags);

    /* 5. Apply for a wb_uart_ocore_s structure to manage a single serial port */
    p_uart_ocore = devm_kzalloc(&pdev->dev, sizeof(struct wb_uart_ocore_s), GFP_KERNEL);
    if (p_uart_ocore == NULL) {
        DEBUG_ERROR("devm_kzalloc failed.\n");
        return -ENOMEM;
    }
    DEBUG_INFO("p_uart_ocore%u: 0x%p", port_index, p_uart_ocore);

    /* 6. Initialize port */
    spin_lock_init(&p_uart_ocore->port.lock);     /* Prevents concurrent access to the port*/
    p_uart_ocore->port.line     = port_index;
    p_uart_ocore->port.ops      = &wb_uart_16550_ops;
    /* p_uart_ocore->name must be assigned before uart_add_one_port, otherwise the name in wb_uart_request_port may be empty */
    (void)snprintf(p_uart_ocore->name, sizeof(p_uart_ocore->name), "wb_uart%u", p_uart_ocore->port.line);

    /* 6.1 The current FPGA Uart scheme is that two serial ports share an interrupt */
    p_uart_ocore->port.irq      = irq;
    p_uart_ocore->port.irqflags = IRQF_SHARED;

    /* 6.2
     * UPF_BOOT_AUTOCONF: Whether it is automatically configured during serial port registration(use port->ops->config_port())
     * UPF_IOREMAP: The IOREMAP operation is required for serial port access to resources
     */
    p_uart_ocore->port.flags = UPF_BOOT_AUTOCONF | UPF_IOREMAP;

    /* 6.3 Initializes io and the interface for port resources based on the type of BAR */
    if ((flags & IORESOURCE_MEM) == IORESOURCE_MEM) {
        p_uart_ocore->port.iotype     = UPIO_MEM;
        p_uart_ocore->port.mapbase    = start + reg_offset;    /* Base + offset address */

        init_port_reg_ops(&p_uart_ocore->port, IORESOURCE_MEM, reg_width, is_le);
    } else {
        p_uart_ocore->port.iotype     = UPIO_PORT;
        p_uart_ocore->port.mapbase    = start + reg_offset;
        p_uart_ocore->port.iobase     = start + reg_offset;

        init_port_reg_ops(&p_uart_ocore->port, IORESOURCE_IO, reg_width, is_le);
    }
    p_uart_ocore->port.mapsize = map_size;

    /* 6.4 In addition to the default attributes, one more is registered here to set and retrieve port information through sysfs */
    p_uart_ocore->port.attr_group = &wb_uart_dev_attr_group;

    /* 6.5 Other initialization */
    p_uart_ocore->port.membase  = NULL;
    p_uart_ocore->port.fifosize = tx_fifo_size ? tx_fifo_size : WB_UART_TX_FIFO_SIZE_DEF;
    p_uart_ocore->port.uartclk  = clk_freq ? clk_freq : WB_UART_CLK_SOURCE_FREQ_DEF;
    p_uart_ocore->port.regshift = 2;
    p_uart_ocore->port.type     = PORT_UNKNOWN;
    p_uart_ocore->port.dev      = &pdev->dev;
    p_uart_ocore->is_active     = 0;
    p_uart_ocore->current_baud  = 0;

    /* 6.6 Port rate auto-adaptation is disabled by default */
    p_uart_ocore->is_spd_adapt  = 0;

    /* 7. Adding a port to the uart controller driver completes the association between the driver and uart_port */
    ret = uart_add_one_port(&wb_uart_driver, &p_uart_ocore->port);
    if (ret != 0) {
        DEBUG_ERROR("uart_add_one_port failed, ret = %d\n", ret);
        return -EPERM;
    }

    /* 8. Set platform driver data */
    platform_set_drvdata(pdev, p_uart_ocore);

    /* 9. Interrupt request: The request should be made when the serial port is opened, but because the interrupt of base irq is disabled,
     * other MSI interrupts cannot be reported. Therefore, the request interrupt is put into probe, and the bottom interrupt enable
     * function is disabled before the request interrupt, so as to prevent the interrupt from being triggered incorrectly because the
     * value of the bottom interrupt enable register is unknown
     */
    uart_hw_deinit(&p_uart_ocore->port);
    DEBUG_INFO("isr name: %s\n", p_uart_ocore->name);
    ret = request_irq(p_uart_ocore->port.irq, wb_uart_isr,
                      p_uart_ocore->port.irqflags, p_uart_ocore->name, &p_uart_ocore->port);
    if (ret != 0) {
        DEBUG_ERROR("request irq fail, irq = %u, index = %u, ret = %d\n", \
                    p_uart_ocore->port.irq, p_uart_ocore->port.line, ret);
        ret = uart_remove_one_port(&wb_uart_driver, &p_uart_ocore->port);
        if (ret != 0) {
            DEBUG_ERROR("uart_remove_one_port failed, ret = %d\n", ret);
        }

        return -ENODEV;
    }

    /* a. Add to linked list */
    spin_lock(&wb_uart_ocore_list_lock);
    list_add_tail(&p_uart_ocore->list, &wb_uart_ocore_list);
    spin_unlock(&wb_uart_ocore_list_lock);

    DEBUG_VERBOSE("end\n");
    return 0;
}

static int wb_uart_remove(struct platform_device *pdev)
{
    struct wb_uart_ocore_s *p_uart_ocore;
    int ret;

    DEBUG_VERBOSE("start\n");

    p_uart_ocore = platform_get_drvdata(pdev);

    /* 1. Deletes the entry from the linked list */
    spin_lock(&wb_uart_ocore_list_lock);
    list_del(&p_uart_ocore->list);
    spin_unlock(&wb_uart_ocore_list_lock);

    /* 2. The driver and port are disassociated */
    ret = uart_remove_one_port(&wb_uart_driver, &p_uart_ocore->port);
    if (ret != 0) {
        DEBUG_ERROR("uart_remove_one_port failed, ret = %d\n", ret);
        return -ENOMEM;
    }

    free_irq(p_uart_ocore->port.irq, &p_uart_ocore->port);

    DEBUG_VERBOSE("end\n");
    return 0;
}

static const struct of_device_id wb_uart_of_match[] = {
    { .compatible = "wb_uart_16550_ocore", },
    { /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, wb_uart_of_match);

static struct platform_driver wb_uart_plat_drv = {
    .probe  = wb_uart_probe,
    .remove = wb_uart_remove,
    .driver = {
        .name  = UART_DRV_NAME,
        .of_match_table = of_match_ptr(wb_uart_of_match),
    },
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
static void wb_uart_speed_adapt_do_timer(ulong arg)
#else
static void wb_uart_speed_adapt_do_timer(struct timer_list *timer)
#endif
{
    struct list_head *n, *pos;
    struct wb_uart_ocore_s *p_uart_ocore;
    struct uart_port *port;
    ulong flags;

    DEBUG_VERBOSE("start\n");

    /* 1. Traverse the linked list, detect the baud rate of the peer end, and set it */
    spin_lock(&wb_uart_ocore_list_lock);
    list_for_each_safe(pos, n, &wb_uart_ocore_list) {
        p_uart_ocore = list_entry(pos, struct wb_uart_ocore_s, list);
        port = &p_uart_ocore->port;
        DEBUG_INFO("uart%u, is_active: %u\n", port->line, p_uart_ocore->is_active);

        if (p_uart_ocore->is_active) {
            spin_lock_irqsave(&port->lock, flags);
            (void)uart_cfg_speed_adapt(port);
            spin_unlock_irqrestore(&port->lock, flags);
        }

        DEBUG_INFO("uart%u, is_spd_adapt: %u, current_baud: %u\n",
                   port->line, p_uart_ocore->is_spd_adapt, p_uart_ocore->current_baud);
    }
    spin_unlock(&wb_uart_ocore_list_lock);

    /* 2. Prepare for the next dispatch timer */
    g_uart_speed_adapt_timer.expires  = jiffies + TIMING_HANDLE_TIME;
    add_timer(&g_uart_speed_adapt_timer);
    DEBUG_VERBOSE("end\n");

    return;
}

static int __init wb_uart_ocore_init(void)
{
    int ret;

    DEBUG_VERBOSE("start\n");

    ret = uart_register_driver(&wb_uart_driver);
    if (ret != 0) {
        DEBUG_ERROR("uart driver register fail, ret = %d\n", ret);
        return ret;
    }

    ret = platform_driver_register(&wb_uart_plat_drv);
    if (ret != 0) {
        DEBUG_ERROR("uart lite plat driver register fail, ret = %d\n", ret);
        uart_unregister_driver(&wb_uart_driver);
        return ret;
    }

    /* Timer initialization */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
    init_timer(&g_uart_speed_adapt_timer);
    g_uart_speed_adapt_timer.function = wb_uart_speed_adapt_do_timer;
    g_uart_speed_adapt_timer.data     = 0;
#else
    timer_setup(&g_uart_speed_adapt_timer, wb_uart_speed_adapt_do_timer, 0);
#endif
    g_uart_speed_adapt_timer.expires  = jiffies + TIMING_HANDLE_TIME;

    /* Register timers and join the kernel dynamic timer list */
    add_timer(&g_uart_speed_adapt_timer);

    DEBUG_VERBOSE("end\n");
    return 0;
}

static void __exit wb_uart_ocore_exit(void)
{
    (void)del_timer_sync(&g_uart_speed_adapt_timer);
    platform_driver_unregister(&wb_uart_plat_drv);
    uart_unregister_driver(&wb_uart_driver);
    return;
}

module_init(wb_uart_ocore_init);
module_exit(wb_uart_ocore_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("uart ocore serial controller driver");
MODULE_LICENSE("GPL");
