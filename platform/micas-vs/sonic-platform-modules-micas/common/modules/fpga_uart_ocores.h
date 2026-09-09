#ifndef _FPGA_UART_OCORES_H
#define _FPGA_UART_OCORES_H

#include <linux/serial_core.h>
#include <linux/platform_device.h>

#define MC_UART_TYPE_NAME      "mc-serial"
#define BUF_SIZE         (256)
#define MC_UART_MAJOR    (211)
#define MC_UART_MINOR    (9)
#define FIFO_SIZE        (16)

#define DIVISOR_MASK     (0xFF)

#define BYTE_BIT         (8)

#define FROMRX           (1)

#define TTYMC_NUM        (48)

/* IER */
#define ETBEI            (0x02)
#define ERBFI            (0x01)
#define ELSI             (0x04)
#define EDSSI            (0x08)
#define PTIME            (0x80)
#define CLEAR            (0x00)

#define RXFLAG           (ERBFI)
#define TXFLAG           (ETBEI)

/* IIR */
#define IDT              (0x0e)
#define TXEMPTY          (0x02)
#define RECEIVEIRQ       (0x04)

/* LSR */
#define THRE             (0x20)
#define TRAN_EMP         (0x40)
#define TX_EMPTY         (THRE | TRAN_EMP)
#define DR               (0x01)

/* LCR */
#define DLAB             (0x80)
#define BIT8             (0x03)
#define BIT7             (0x02)
#define BIT6             (0x01)
#define BIT5             (0x00)
#define STOP2            (0x04)
#define STOP1            (~STOP2)
#define PARITY           (0x08)
#define NOPARITY         (~PARITY)
#define EVEN             (0x10)
#define ODD              (~EVEN)
#define BREAK            (0x40)
#define NOBREAK          (~BREAK)

/* FCR */
#define TXFIFO_RESET     (0x04)
#define RXFIFO_RESET     (0x02)
#define FIFO_EN          (0x01)
#define FIFO_DIS         (0x00)

/* MCR */
#define DIS_LOOPBACK     (0x00)
#define EN_LOOPBACK      (0x10)

/* UART Registers */
#define UART_BYTE_MASK   (0xFF)
#define UART_HBYTE_MASK  (0x0F)
#define UART_RX          (0x00)
#define UART_TX          (0x00)
#define UART_DLL         (0x00)
#define UART_DLM         (0x04)
#define UART_IER         (0x04)
#define UART_IIR         (0x08)

#define UART_IIR_NO_INT         (0x01) /* No interrupts pending */
#define UART_IIR_ID             (0x0e) /* Mask for the interrupt ID */
#define UART_IIR_MSI            (0x00) /* Modem status interrupt */
#define UART_IIR_THRI           (0x02) /* Transmitter holding register empty */
#define UART_IIR_RDI            (0x04) /* Receiver data interrupt */
#define UART_IIR_RLSI           (0x06) /* Receiver line status interrupt */

#define UART_IIR_BUSY           (0x07) /* DesignWare APB Busy Detect */
#define UART_IIR_RX_TIMEOUT     (0x0c) /* OMAP RX Timeout interrupt */
#define UART_IIR_XOFF           (0x10) /* OMAP XOFF/Special Character */
#define UART_IIR_CTS_RTS_DSR    (0x20) /* OMAP CTS/RTS/DSR Change */

#define UART_FCR         (0x08)
#define UART_LCR         (0x0c)
#define UART_MCR         (0x10)
#define UART_LSR         (0x14)
#define UART_MSR         (0x18)
#define UART_SCR         (0x1c)

#define UPF_FOURPORT            ((__force upf_t) ASYNC_FOURPORT             /* 1  */ )
#define UPF_SAK                 ((__force upf_t) ASYNC_SAK                  /* 2  */ )
#define UPF_SPD_HI              ((__force upf_t) ASYNC_SPD_HI               /* 4  */ )
#define UPF_SPD_VHI             ((__force upf_t) ASYNC_SPD_VHI              /* 5  */ )
#define UPF_SPD_CUST            ((__force upf_t) ASYNC_SPD_CUST             /* 0x0030 */ )
#define UPF_SPD_WARP            ((__force upf_t) ASYNC_SPD_WARP             /* 0x1010 */ )
#define UPF_SPD_MASK            ((__force upf_t) ASYNC_SPD_MASK             /* 0x1030 */ )
#define UPF_SKIP_TEST           ((__force upf_t) ASYNC_SKIP_TEST            /* 6  */ )
#define UPF_AUTO_IRQ            ((__force upf_t) ASYNC_AUTO_IRQ             /* 7  */ )
#define UPF_HARDPPS_CD          ((__force upf_t) ASYNC_HARDPPS_CD           /* 11 */ )
#define UPF_SPD_SHI             ((__force upf_t) ASYNC_SPD_SHI              /* 12 */ )
#define UPF_LOW_LATENCY         ((__force upf_t) ASYNC_LOW_LATENCY          /* 13 */ )
#define UPF_BUGGY_UART          ((__force upf_t) ASYNC_BUGGY_UART           /* 14 */ )
#define UPF_NO_TXEN_TEST        ((__force upf_t) (1 << 15))
#define UPF_MAGIC_MULTIPLIER    ((__force upf_t) ASYNC_MAGIC_MULTIPLIER     /* 16 */ )

/* Port has hardware-assisted h/w flow control */
#define UPF_AUTO_CTS        ((__force upf_t) (1 << 20))
#define UPF_AUTO_RTS        ((__force upf_t) (1 << 21))
#define UPF_HARD_FLOW       ((__force upf_t) (UPF_AUTO_CTS | UPF_AUTO_RTS))
/* Port has hardware-assisted s/w flow control */
#define UPF_SOFT_FLOW       ((__force upf_t) (1 << 22))
#define UPF_CONS_FLOW       ((__force upf_t) (1 << 23))
#define UPF_SHARE_IRQ       ((__force upf_t) (1 << 24))
#define UPF_EXAR_EFR        ((__force upf_t) (1 << 25))
#define UPF_BUG_THRE        ((__force upf_t) (1 << 26))
/* The exact UART type is known and should not be probed.  */
#define UPF_FIXED_TYPE      ((__force upf_t) (1 << 27))
#define UPF_BOOT_AUTOCONF   ((__force upf_t) (1 << 28))
#define UPF_FIXED_PORT      ((__force upf_t) (1 << 29))
#define UPF_DEAD            ((__force upf_t) (1 << 30))
#define UPF_IOREMAP         ((__force upf_t) (1 << 31))

/*
 * The Intel XScale on-chip UARTs define these bits
 */
#define UART_IER_DMAE       (0x80) /* DMA Requests Enable */
#define UART_IER_UUE        (0x40) /* UART Unit Enable */
#define UART_IER_NRZE       (0x20) /* NRZ coding Enable */
#define UART_IER_RTOIE      (0x10) /* Receiver Time Out Interrupt Enable */

/*
 * Note: if the word length is 5 bits (UART_LCR_WLEN5), then setting
 * UART_LCR_STOP will select 1.5 stop bits, not 2 stop bits.
 */
#define UART_LCR_DLAB        (0x80) /* Divisor latch access bit */
#define UART_LCR_SBC         (0x40) /* Set break control */
#define UART_LCR_SPAR        (0x20) /* Stick parity (?) */
#define UART_LCR_EPAR        (0x10) /* Even parity select */
#define UART_LCR_PARITY      (0x08) /* Parity Enable */
#define UART_LCR_STOP        (0x04) /* Stop bits: 0=1 bit, 1=2 bits */
#define UART_LCR_WLEN5       (0x00) /* Wordlength: 5 bits */
#define UART_LCR_WLEN6       (0x01) /* Wordlength: 6 bits */
#define UART_LCR_WLEN7       (0x02) /* Wordlength: 7 bits */
#define UART_LCR_WLEN8       (0x03) /* Wordlength: 8 bits */

#define UART_LSR_BRK_ERROR_BITS    (0x1E)
#define UART_LSR_BAIL_OUT          (0xFF) /* likely no UART here */

#define LSR_SAVE_FLAGS UART_LSR_BRK_ERROR_BITS

#define UART_FCR_ENABLE_FIFO        (0x01) /* Enable the FIFO */
#define UART_FCR_DISABLE_FIFO       (0x00) /* Disable the FIFO */
#define UART_FCR_CLEAR_RCVR         (0x02) /* Clear the RCVR FIFO */
#define UART_FCR_CLEAR_XMIT         (0x04) /* Clear the XMIT FIFO */

#define UART_LSR_FIFOE              (0x80) /* Fifo error */
#define UART_LSR_TEMT               (0x40) /* Transmitter empty */
#define UART_LSR_THRE               (0x20) /* Transmit-hold-register empty */
#define UART_LSR_BI                 (0x10) /* Break interrupt indicator */
#define UART_LSR_FE                 (0x08) /* Frame error indicator */
#define UART_LSR_PE                 (0x04) /* Parity error indicator */
#define UART_LSR_OE                 (0x02) /* Overrun error indicator */
#define UART_LSR_DR                 (0x01) /* Receiver data ready */
#define UART_LSR_BRK_ERROR_BITS     (0x1E) /* BI, FE, PE, OE bits */

#define UART_LSR_THRE           (0x20) /* Transmit-hold-register empty */
#define UART_LSR_BI             (0x10) /* Break interrupt indicator */
#define UART_LSR_FE             (0x08) /* Frame error indicator */
#define UART_LSR_PE             (0x04) /* Parity error indicator */
#define UART_LSR_OE             (0x02) /* Overrun error indicator */

#define UART_MSR_DCD            (0x80) /* Data Carrier Detect */
#define UART_MSR_RI             (0x40) /* Ring Indicator */
#define UART_MSR_DSR            (0x20) /* Data Set Ready */
#define UART_MSR_CTS            (0x10) /* Clear to Send */
#define UART_MSR_DDCD           (0x08) /* Delta DCD */
#define UART_MSR_TERI           (0x04) /* Trailing edge ring indicator */
#define UART_MSR_DDSR           (0x02) /* Delta DSR */
#define UART_MSR_DCTS           (0x01) /* Delta CTS */
#define UART_MSR_ANY_DELTA      (0x0F) /* Any of the delta bits! */

#define UART_SCR_A5             (0xA5)
#define UART_SCR_5A             (0x5A)
#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)
/*
 * Note: The FIFO trigger levels are chip specific:
 *    RX:76 = 00  01  10  11    TX:54 = 00  01  10  11
 * PC16550D:     1   4   8  14        xx  xx  xx  xx
 * TI16C550A:    1   4   8  14        xx  xx  xx  xx
 * TI16C550C:    1   4   8  14        xx  xx  xx  xx
 * ST16C550:     1   4   8  14        xx  xx  xx  xx
 * ST16C650:     8  16  24  28        16   8  24  30    PORT_16650V2
 * NS16C552:     1   4   8  14        xx  xx  xx  xx
 * ST16C654:     8  16  56  60         8  16  32  56    PORT_16654
 * TI16C750:     1  16  32  56        xx  xx  xx  xx    PORT_16750
 * TI16C752:     8  16  56  60         8  16  32  56
 * Tegra:        1   4   8  14        16   8   4   1    PORT_TEGRA
 */
#define UART_FCR_R_TRIG_00    (0x00)
#define UART_FCR_R_TRIG_01    (0x40)
#define UART_FCR_R_TRIG_10    (0x80)
#define UART_FCR_R_TRIG_11    (0xc0)
#define UART_FCR_T_TRIG_00    (0x00)
#define UART_FCR_T_TRIG_01    (0x10)
#define UART_FCR_T_TRIG_10    (0x20)
#define UART_FCR_T_TRIG_11    (0x30)

#define UART_FCR_TRIGGER_MASK       (0xC0) /* Mask for the FIFO trigger range */
#define UART_FCR_TRIGGER_1          (0x00) /* Mask for trigger set at 1 */
#define UART_FCR_TRIGGER_4          (0x40) /* Mask for trigger set at 4 */
#define UART_FCR_TRIGGER_8          (0x80) /* Mask for trigger set at 8 */
#define UART_FCR_TRIGGER_16         (0xC0) /* Mask for trigger set at 16 */
#define UART_FIFO_TRIGGER_1         (1)    /* FIFO trigger at 1 byte */
#define UART_FIFO_TRIGGER_4         (4)    /* FIFO trigger at 4 byte */
#define UART_FIFO_TRIGGER_8         (8)    /* FIFO trigger at 8 byte */
#define UART_FIFO_TRIGGER_16        (16)   /* FIFO trigger at 16 byte */
#define UART_FIFO_MAX_COUNT         (256)  /* FIFO mac count */
/* 16650 definitions */
#define UART_FCR6_R_TRIGGER_8       (0x00) /* Mask for receive trigger set at 1 */
#define UART_FCR6_R_TRIGGER_16      (0x40) /* Mask for receive trigger set at 4 */
#define UART_FCR6_R_TRIGGER_24      (0x80) /* Mask for receive trigger set at 8 */
#define UART_FCR6_R_TRIGGER_28      (0xC0) /* Mask for receive trigger set at 14 */
#define UART_FCR6_T_TRIGGER_16      (0x00) /* Mask for transmit trigger set at 16 */
#define UART_FCR6_T_TRIGGER_8       (0x10) /* Mask for transmit trigger set at 8 */
#define UART_FCR6_T_TRIGGER_24      (0x20) /* Mask for transmit trigger set at 24 */
#define UART_FCR6_T_TRIGGER_30      (0x30) /* Mask for transmit trigger set at 30 */
#define UART_FCR7_64BYTE            (0x20) /* Go into 64 byte mode (TI16C750 and Freescale UARTs) */

#define UART_FCR_R_TRIG_SHIFT        (6)
#define UART_FCR_R_TRIG_BITS(x)        \
    (((x) & UART_FCR_TRIGGER_MASK) >> UART_FCR_R_TRIG_SHIFT)
#define UART_FCR_R_TRIG_MAX_STATE    (4)

/*
 * LCR=0xBF (or DLAB=1 for 16C660)
 */
#define UART_EFR                (2)    /* I/O: Extended Features Register */
#define UART_XR_EFR             (9)    /* I/O: Extended Features Register (XR17D15x) */
#define UART_EFR_CTS            (0x80) /* CTS flow control */
#define UART_EFR_RTS            (0x40) /* RTS flow control */
#define UART_EFR_SCD            (0x20) /* Special character detect */
#define UART_EFR_ECB            (0x10) /* Enhanced control bit */

/*
 * Access to some registers depends on register access / configuration
 * mode.
 */
#define UART_LCR_CONF_MODE_A    UART_LCR_DLAB /* Configutation mode A */
#define UART_LCR_CONF_MODE_B    (0xBF) /* Configutation mode B */

#define UART_MCR_CLKSEL         (0x80) /* Divide clock by 4 (TI16C752, EFR[4]=1) */
#define UART_MCR_TCRTLR         (0x40) /* Access TCR/TLR (TI16C752, EFR[4]=1) */
#define UART_MCR_XONANY         (0x20) /* Enable Xon Any (TI16C752, EFR[4]=1) */
#define UART_MCR_AFE            (0x20) /* Enable auto-RTS/CTS (TI16C550C/TI16C750) */
#define UART_MCR_LOOP           (0x10) /* Enable loopback test mode */
#define UART_MCR_OUT2           (0x08) /* Out2 complement */
#define UART_MCR_OUT1           (0x04) /* Out1 complement */
#define UART_MCR_RTS            (0x02) /* RTS complement */
#define UART_MCR_DTR            (0x01) /* DTR complement */

#define UART_IER_MSI            (0x08) /* Enable Modem status interrupt */
#define UART_IER_RLSI           (0x04) /* Enable receiver line status interrupt */
#define UART_IER_THRI           (0x02) /* Enable Transmitter holding register int. */
#define UART_IER_RDI            (0x01) /* Enable receiver data interrupt */

#define UART_CAP_FIFO           (1 << 8)  /* UART has FIFO */
#define UART_CAP_EFR            (1 << 9)  /* UART has EFR */
#define UART_CAP_SLEEP          (1 << 10) /* UART has IER sleep */
#define UART_CAP_AFE            (1 << 11) /* MCR-based hw flow control */
#define UART_CAP_UUE            (1 << 12) /* UART needs IER bit 6 set (Xscale) */
#define UART_CAP_RTOIE          (1 << 13) /* UART needs IER bit 4 set (Xscale, Tegra) */
#define UART_CAP_HFIFO          (1 << 14) /* UART has a "hidden" FIFO */
#define UART_CAP_RPM            (1 << 15) /* Runtime PM is active while idle */

#define UART_BUG_QUOT           (1 << 0) /* UART has buggy quot LSB */
#define UART_BUG_TXEN           (1 << 1) /* UART has buggy TX IIR status */
#define UART_BUG_NOMSR          (1 << 2) /* UART has buggy MSR status bits (Au1x00) */
#define UART_BUG_THRE           (1 << 3) /* UART has buggy THRE reassertion */
#define UART_BUG_PARITY         (1 << 4) /* UART mishandles parity if FIFO enabled */

#define BUFFER_SIZE             (128 * 1024) /* UART default buffer size */
#define MC_UART_START           (1)          /* UART start status flag */
#define MC_UART_CLOSE           (0)          /* UART stop status flag */
#define MC_UART_BUFFER_ON       (1)          /* UART buffer switch flag */
#define MC_UART_BUFFER_OFF      (0)          /* UART buffer switch flag */
#define MC_UART_BUFFER_MIN      (1)          /* Minimum buffer size */
#define MIN_BAUDRATE_SUPPORTED  (110)
#define MAX_BAUDRATE_SUPPORTED  (921600)

#define EXCR1                   (0x02)
#define SERIAL_BUFFER_NOT_ALLOCCED                (0)
typedef enum {
    BUFFER_EMPTY = 0,
    BUFFER_BUFFERING,
    BUFFER_FULL,
} buffer_status_t;

struct mc_uart_platform_data {
    int baud_rate;
    int id;
    int div;
    int clk;
};

struct mc_circ_buf {
    unsigned char *buf;
    int head;
    int tail;
    int status;
};

struct ocores_uart {
    struct uart_port port;
    unsigned short capabilities;    /* port capabilities */
    int baud_rate;
    int div;
    int clk;
    int xmit_size;
    unsigned char last_iir;
    bool fifo_bug;                  /* min RX trigger if enabled */
    unsigned int tx_loadsz;         /* transmit fifo load size */
    unsigned char acr;
    unsigned char fcr;
    unsigned char ier;
    unsigned char lcr;
    unsigned char mcr;
    unsigned char mcr_mask;         /* mask of user bits */
    unsigned char mcr_force;        /* mask of forced bits */
    unsigned char cur_iotype;       /* Running I/O type */

    unsigned int rpm_tx_active;

    spinlock_t tx_lock;
    spinlock_t rx_lock;
    spinlock_t uart_irq_lock;
    unsigned short bugs;            /* port bugs */

    unsigned char lsr_saved_flags;
    unsigned char msr_saved_flags;
    int mc_uart_status;             /* Uart current status */
    int mc_uart_buffer_switch;      /* Uart buffer switch */
    int config_buffer_size;         /* Uart buffer control */
    struct mc_circ_buf rx_ring;
    struct mutex mc_uart_buffer_lock;
};

static inline int serial_in(struct ocores_uart *up, int offset)
{
    return up->port.serial_in(&up->port, offset);
}

static inline void serial_out(struct ocores_uart *up, int offset, int value)
{
    up->port.serial_out(&up->port, offset, value);
}

static inline void mc_uart_out_mcr(struct ocores_uart *up, int value)
{
    serial_out(up, UART_MCR, value);
}

static inline int mc_uart_in_mcr(struct ocores_uart *up)
{
    return serial_in(up, UART_MCR);
}

static inline void mc_uart_out_fcr(struct ocores_uart *up, int value)
{
    serial_out(up, UART_LCR, serial_in(up, UART_LCR) | DLAB);
    serial_out(up, UART_FCR, value);
    serial_out(up, UART_LCR, serial_in(up,  UART_LCR) & ~DLAB);
}

static inline int mc_uart_in_fcr(struct ocores_uart *up)
{
    int tmp;

    serial_out(up, UART_LCR, serial_in(up, UART_LCR) | DLAB);
    tmp = serial_in(up, UART_FCR);
    serial_out(up, UART_LCR, serial_in(up,  UART_LCR) & ~DLAB);
    return tmp;
}

extern int serial_buffer_alloc(int index, int buffer_size);
extern int serial_buffer_realloc(int index, int new_buffer_size);
extern int serial_buffer_write(int index, unsigned char ch);
extern int serial_buffer_read(int index, unsigned char *ch);
extern int serial_get_buffer_status(int index);
extern int serial_get_buffer_size(int index);
extern int serial_buffer_if_allocced(int index);

#endif /* _FPGA_UART_OCORES_H */
