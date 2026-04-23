// SPDX-License-Identifier: GPL-2.0+
/*
 * mule_uart.c -- Mule UART driver
 *
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kfifo.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/sysfs.h>
#include <asm/io.h>
#include "mule.h"

#define DRV_NAME "mule"
#define MULE_MAXPORTS 48
#define MULE_PORTS_REAL 48
#define PORT_MULE_UART PORT_16550A
#define MULE_FIFO_LEN 8
#define MULE_FPGA_MIN_REV 0x0f
#define MULE_FPGA_MIN_THERMAL_REV 0x11

typedef __u8 u8;
typedef __u32 u32;
typedef __u64 u64;

#define C2H_INTERVAL 600
#define C2H_TIMEOUT_NS (1 * C2H_INTERVAL * 1000)
#define H2C_TIMEOUT_NS (1 * C2H_INTERVAL * 1000)
unsigned int poll_time = (C2H_INTERVAL / 2);

#pragma pack(push, 1)
struct c2h_port
{
    unsigned char data[8];
    unsigned char valid;
    unsigned char par_err;
    unsigned char frm_err;
    unsigned char brk_err;
    unsigned char iir;
    unsigned char lsr;
    unsigned char msr;
};
struct c2h_frame
{
    struct c2h_port ports[MULE_PORTS_REAL];
};

struct h2c_port
{
    unsigned char data[8];
    unsigned char valid;
};
struct h2c_frame
{
    struct h2c_port ports[MULE_PORTS_REAL];
};
#pragma pack(pop)

static_assert(sizeof(struct h2c_frame) == 432);
static_assert(sizeof(struct c2h_frame) == 720);

struct counters {
    u64 h2c_frames_sent;
    u64 h2c_frames_abs;
    u64 h2c_framing_err;
    u64 h2c_overflow_err;
    u64 h2c_dma_avg;
    u64 h2c_dma_time_max;
    u64 c2h_frames_rcvd;
    u64 c2h_dma_avg;
    u64 c2h_dma_time_max;
    u64 c2h_framing_err;
    u64 c2h_overflow_err;
    u64 c2h_desc_err;
    u64 timer_h2c_cnt;
    u64 timer_c2h_cnt;
    u64 timer_loops;
    u64 h2c_timeout;
    u64 c2h_timeout;
};

struct mule_dev
{
    unsigned char __iomem *membase;
    unsigned char __iomem *membase_xdma;
    struct pci_dev *xdma;
    struct xdma_desc *h2c_desc;
    struct xdma_desc *c2h_desc;
    struct h2c_frame *h2c_frame;
    struct c2h_frame *c2h_frame;
    dma_addr_t h2c_frame_phys;
    dma_addr_t c2h_frame_phys;
    dma_addr_t h2c_desc_phys;
    dma_addr_t c2h_desc_phys;
    struct work_struct mule_bh_work;
    struct work_struct c2h_bh_work;
    struct device *dev;
    int h2c_irq;
    int c2h_irq;
    spinlock_t        lock_ap;
    int active_ports;
    struct {
        struct uart_port port;
        struct device *port_dev;
        int enabled;
        int stop_rx;
        int start_tx;
        u8 mcr;
        u8 msr;
        u8 x_char;
    } p[MULE_MAXPORTS];
    atomic_t mule_intr_status;
    atomic_t c2h_xdma_status;
    u64 c2h_busy;
    int c2h_busy_cnt;
    u32 c2h_frc0;
    u64 h2c_busy;
    int h2c_busy_cnt;
    u32 h2c_frc1;
    struct hrtimer c2h_timer;
    spinlock_t     lock_timer;
    u32 version;
    struct counters counters;
    struct sliding_avg h2c_dma_savg;
    struct sliding_avg c2h_dma_savg;
};

struct xdma_desc {
    u32 control;
    u32 len;
    u32 src_addr_lo;
    u32 src_addr_hi;
    u32 dst_addr_lo;
    u32 dst_addr_hi;
    u32 nxt_addr_lo;
    u32 nxt_addr_hi;
} __packed;

#define DESC_MAGIC 0xAD4B0000UL
#define XDMA_DESC_STOPPED    (1UL << 0)
#define XDMA_DESC_COMPLETED    (1UL << 1)
#define XDMA_DESC_EOP        (1UL << 4)


static u32 mule_reg_read(unsigned char __iomem *base, unsigned offset)
{
    u32 val;
    volatile void __iomem *addr = (base + offset);
    val = readl(addr);
    return val;
}

static void mule_reg_write(unsigned char __iomem *base, unsigned offset, u32 value)
{
    u8 *addr = base + offset;
    writel(value, addr);
}

static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct mule_dev *mdev = dev_get_drvdata(dev);
    u32 val = mule_reg_read(mdev->membase,MULE_VERSION_STATUS);
    scnprintf(buf, PAGE_SIZE, "0x%02x\n", val & 0xff);
    return strlen(buf);
}

static int convert_temp(u32 val) {
    u64 mc = (u64)((val & 0xffff) >> 4) * 503975;
    return ((((mc / 4096) - 273150) + 50 ) / 100);
}

static ssize_t temp_cur_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct mule_dev *mdev = dev_get_drvdata(dev);
    if (mdev->version < MULE_FPGA_MIN_THERMAL_REV)
        return -ENODEV;
    u32 val = mule_reg_read(mdev->membase,MULE_UART_CURTEMP);
    int temp = convert_temp(val);
    scnprintf(buf, PAGE_SIZE, "%d.%01d\n", temp / 10, temp % 10);
    return strlen(buf);
}

static ssize_t temp_max_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct mule_dev *mdev = dev_get_drvdata(dev);
    if (mdev->version < MULE_FPGA_MIN_THERMAL_REV)
        return -ENODEV;
    u32 val = mule_reg_read(mdev->membase,MULE_UART_MAXTEMP);
    int temp = convert_temp(val);
    scnprintf(buf, PAGE_SIZE, "%d.%01d\n", temp / 10, temp % 10);
    return strlen(buf);
}

static ssize_t temp_min_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct mule_dev *mdev = dev_get_drvdata(dev);
    if (mdev->version < MULE_FPGA_MIN_THERMAL_REV)
        return -ENODEV;
    u32 val = mule_reg_read(mdev->membase,MULE_UART_MINTEMP);
    int temp = convert_temp(val);
    scnprintf(buf, PAGE_SIZE, "%d.%01d\n", temp / 10, temp % 10);
    return strlen(buf);
}

#define PCOUNTER(_name) p += scnprintf(p, (PAGE_SIZE - (p-buf)), "%s %llu\n", #_name, cnt._name);

static ssize_t counters_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct mule_dev *mdev = dev_get_drvdata(dev);

    if (!mdev)
        return -ENODEV;

    struct counters cnt;
    char* p = buf;

    spin_lock_bh(&mdev->lock_timer);
    cnt = mdev->counters;
    spin_unlock_bh(&mdev->lock_timer);

    PCOUNTER(h2c_frames_sent);
    PCOUNTER(h2c_frames_abs);
    PCOUNTER(h2c_framing_err);
    PCOUNTER(h2c_overflow_err);
    PCOUNTER(h2c_dma_avg);
    PCOUNTER(h2c_dma_time_max);
    PCOUNTER(h2c_timeout);
    PCOUNTER(c2h_frames_rcvd);
    PCOUNTER(c2h_framing_err);
    PCOUNTER(c2h_dma_avg);
    PCOUNTER(c2h_dma_time_max);
    PCOUNTER(c2h_overflow_err);
    PCOUNTER(c2h_desc_err);
    PCOUNTER(c2h_timeout);
    PCOUNTER(timer_h2c_cnt);
    PCOUNTER(timer_c2h_cnt);
    PCOUNTER(timer_loops);

    return (p - buf);
}

static DEVICE_ATTR_RO(version);
static DEVICE_ATTR_RO(temp_cur);
static DEVICE_ATTR_RO(temp_max);
static DEVICE_ATTR_RO(temp_min);
static DEVICE_ATTR_RO(counters);

static struct attribute *counters_attrs[] = {
    &dev_attr_version.attr,
    &dev_attr_temp_cur.attr,
    &dev_attr_temp_max.attr,
    &dev_attr_temp_min.attr,
    &dev_attr_counters.attr,
    NULL,
};

static const struct attribute_group counters_group = {
    .attrs = counters_attrs,
    NULL,
};

static unsigned int op_tx_empty(struct uart_port *port)
{
    unsigned int rc=0;
    u32 val;

    val = mule_reg_read(port->membase,MULE_UART_LSR_RW);
    if (val & MULE_LSR_XMIT_EMPTY)
        rc = TIOCSER_TEMT;

    return rc;
}

static unsigned int op_get_mctrl(struct uart_port *port)
{
    u32 val;
    unsigned int mctrl = 0;
    val = mule_reg_read(port->membase, MULE_UART_MSR_RW);
    if (val & MULE_MSR_CTS)
        mctrl |= TIOCM_CTS;
    if (val & MULE_MSR_DSR)
        mctrl |= TIOCM_DSR;
    if (val & MULE_MSR_RI)
        mctrl |= TIOCM_RI;
    if (val & MULE_MSR_DCD)
        mctrl |= TIOCM_CAR;

    return mctrl;
}

static void op_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    struct mule_dev *mdev = port->private_data;
    u32 mcr = mdev->p[port->line].mcr;

    if (mctrl & TIOCM_RTS)
        mcr |= MULE_MCR_RTS;
    else
        mcr &= ~MULE_MCR_RTS;
    if (mctrl & TIOCM_DTR)
        mcr |= MULE_MCR_DTR;
    else
        mcr &= ~MULE_MCR_DTR;

#ifdef MULE_OUT_LOOP_SUPPORT
    if (mctrl & TIOCM_OUT1)
        mcr |= MULE_MCR_OUT0;
    if (mctrl & TIOCM_OUT2)
        mcr |= MULE_MCR_OUT1;
    if (mctrl & TIOCM_LOOP)
        mcr |= MULE_MCR_LOOP;
#endif

    mule_reg_write(port->membase, MULE_UART_MCR_RW, mcr);

}

static void op_start_tx(struct uart_port *port)
{
    struct mule_dev *mdev = port->private_data;
    mdev->p[port->line].start_tx = 1;
}

static void op_stop_tx(struct uart_port *port)
{
    struct mule_dev *mdev = port->private_data;
    mdev->p[port->line].start_tx = 0;
}

static void op_start_rx(struct uart_port *port)
{
    struct mule_dev *mdev = port->private_data;
    mdev->p[port->line].stop_rx = 0;
}

static void op_stop_rx(struct uart_port *port)
{
    struct mule_dev *mdev = port->private_data;
    mdev->p[port->line].stop_rx = 1;
}

static void op_break_ctl(struct uart_port *port, int ctl)
{
    u32 val;
    val = mule_reg_read(port->membase, MULE_UART_LCR_RW);
    if (ctl)
        val |= 0x40;
    else
        val &= ~0x40;
    mule_reg_write(port->membase, MULE_UART_LCR_RW, val);
}

static void mule_set_baud(struct uart_port *port, unsigned int baud, u8 lcr)
{
    unsigned int div;

    switch (baud)
    {
    case 115200:
        div = 109;
        break;
    case 57600:
        div = 217;
        break;
    case 38400:
        div = 326;
        break;
    case 19200:
        div = 651;
        break;
    case 9600:
        div = 1302;
        break;
    default:
        dev_warn(port->dev, "p=%d unsupported baud %d\n", port->line, baud);
        return;
    }

    lcr |= MULE_UART_LCR_dlab;
    mule_reg_write(port->membase,MULE_UART_LCR_RW,lcr);

    mule_reg_write(port->membase,MULE_UART_DIV_MSB_RW,(div >> 8) & 0xff);
    mule_reg_write(port->membase,MULE_UART_DIV_LSB_RW,div & 0xff);

    lcr &= ~MULE_UART_LCR_dlab;
    mule_reg_write(port->membase,MULE_UART_LCR_RW,lcr);

}

static void op_set_termios(struct uart_port *port,
                           struct ktermios *termios,
                           const struct ktermios *old)
{
    unsigned int baud;
    unsigned char lcr;
    struct mule_dev *mdev = port->private_data;
    u32 val;

    lcr = UART_LCR_WLEN(tty_get_char_size(termios->c_cflag));

    if (termios->c_cflag & CSTOPB)
        lcr |= MULE_UART_LCR_stopbit;
    if (termios->c_cflag & PARENB)
        lcr |= MULE_UART_LCR_parity;
    if (!(termios->c_cflag & PARODD))
        lcr |= MULE_UART_LCR_evenpar;
    if (termios->c_cflag & CMSPAR)
        lcr |= MULE_UART_LCR_stickpar;
    if ((termios->c_cflag & CRTSCTS) && (port->flags & UPF_HARD_FLOW)) {
        port->status |= (UPSTAT_AUTOCTS | UPSTAT_AUTORTS );
        mdev->p[port->line].mcr |= MULE_MCR_AFCE;
    }
    else {
        termios->c_cflag &= ~CRTSCTS;
        port->status &= ~(UPSTAT_AUTOCTS | UPSTAT_AUTORTS );
        mdev->p[port->line].mcr &= ~MULE_MCR_AFCE;
    }
    val = mule_reg_read(port->membase, MULE_UART_MCR_RW);
    val &= ~MULE_MCR_AFCE;
    val |= mdev->p[port->line].mcr;
    mule_reg_write(port->membase, MULE_UART_MCR_RW, val);

    baud = uart_get_baud_rate(port, termios, old, 9600, 115200);
    tty_termios_encode_baud_rate(termios, baud, baud);
    uart_update_timeout(port, termios->c_cflag, baud);
    mule_set_baud(port, baud, lcr);

}

static void mule_uart_mode_set(struct mule_dev *mdev, struct uart_port *port, int enable)
{
    unsigned reg, val, uart;

    if ((port->line < 32)) {
        reg = MULE_UART_CONTROL0;
        uart = (1 << port->line);
    }
    else {
        reg = MULE_UART_CONTROL1;
        uart = (1 << (port->line - 32));
    }
    val = mule_reg_read(mdev->membase,reg);
    if (!enable)
        val &= ~uart;
    else
        val |= uart;
    mule_reg_write(mdev->membase, reg, val);
    mdev->p[port->line].enabled = enable;
    mdev->p[port->line].stop_rx = !enable;

}

static void mule_uart_rx_chars(struct uart_port *port)
{
    u8 ch;
    unsigned long valid, lsr;
    int i, j;
    struct mule_dev *mdev = port->private_data;
    struct c2h_frame *frame = (struct c2h_frame *)mdev->c2h_frame;
    struct c2h_port *c2h = &frame->ports[port->line];

    lsr = c2h->lsr;
    if (lsr & MULE_LSR_OVERRUN) {
        port->icount.overrun++;
    }

    if ((c2h->msr & MULE_MSR_CTS) != (mdev->p[port->line].msr & MULE_MSR_CTS)) {
        mdev->p[port->line].msr = c2h->msr;
        uart_port_lock(port);
        uart_handle_cts_change(port, (c2h->msr & MULE_MSR_CTS) ? 1 : 0);
        uart_port_unlock(port);
    }

    valid = c2h->valid;
    if (valid) {
        for (i=0,j=1;i<8;i++,j<<=1)
        {
            if (valid & j)
            {
                u8 flag = TTY_NORMAL;
                if (lsr & MULE_LSR_FIFO_ERR)
                {
                    if (c2h->brk_err & j)
                        flag = TTY_BREAK;
                    else if (c2h->par_err & j)
                        flag = TTY_PARITY;
                    else if (c2h->frm_err & j)
                        flag = TTY_FRAME;
                }
                ch = c2h->data[i];
                port->icount.rx++;
                uart_insert_char(port, lsr, MULE_LSR_OVERRUN, ch, flag);
            }
        }

        tty_flip_buffer_push(&port->state->port);
    }
}

static void mule_irq_count_err(struct mule_dev *mdev, u32 val)
{
    if (unlikely(val & XDMA_INTR_ALL_ERR_MASK ))
    {
        if (val & MULE_INT_H2C_FRAMING_ERR)
            mdev->counters.h2c_framing_err++;
        if (val & MULE_INT_H2C_OVERFLOW_ERR)
            mdev->counters.h2c_overflow_err++;
        if (val & MULE_INT_C2H_UNDERRUN_ERR)
            mdev->counters.c2h_framing_err++;
        if (val & MULE_INT_C2H_OVERFLOW_ERR)
            mdev->counters.c2h_overflow_err++;
    }
}

static void mule_dma_free(struct mule_dev *mdev)
{
    if( mdev->c2h_desc )
        dma_free_coherent(mdev->dev, sizeof(struct xdma_desc), mdev->c2h_desc, mdev->c2h_desc_phys);
    if( mdev->h2c_desc )
        dma_free_coherent(mdev->dev, sizeof(struct xdma_desc), mdev->h2c_desc, mdev->h2c_desc_phys);
    if( mdev->c2h_frame )
        dma_free_coherent(mdev->dev, sizeof(struct c2h_frame), mdev->c2h_frame, mdev->c2h_frame_phys);
    if( mdev->h2c_frame )
        dma_free_coherent(mdev->dev, sizeof(struct h2c_frame), mdev->h2c_frame, mdev->h2c_frame_phys);
    mdev->c2h_desc = NULL;
    mdev->h2c_desc = NULL;
    mdev->c2h_frame = NULL;
    mdev->h2c_frame = NULL;

}

static int mule_dma_alloc(struct mule_dev *mdev)
{

    int rc = 0;

    mdev->c2h_desc = dma_alloc_coherent(mdev->dev, sizeof(struct xdma_desc), &mdev->c2h_desc_phys, GFP_KERNEL);
    if (!mdev->c2h_desc) {
        goto err_alloc;
    }

    mdev->h2c_desc = dma_alloc_coherent(mdev->dev, sizeof(struct xdma_desc), &mdev->h2c_desc_phys, GFP_KERNEL);
    if (!mdev->h2c_desc) {
        goto err_alloc;
    }

    mdev->c2h_frame = dma_alloc_coherent(mdev->dev, sizeof(struct c2h_frame), &mdev->c2h_frame_phys, GFP_KERNEL);
    if (!mdev->c2h_frame) {
        goto err_alloc;
    }

    mdev->h2c_frame = dma_alloc_coherent(mdev->dev, sizeof(struct h2c_frame), &mdev->h2c_frame_phys, GFP_KERNEL);
    if (!mdev->h2c_frame) {
        goto err_alloc;
    }

    return rc;

err_alloc:
    dev_err(mdev->dev, "Failed to allocate DMA buffers\n");
    mule_dma_free(mdev);
    return -ENOMEM;
}

static void mule_c2h_dma_start(struct mule_dev *mdev)
{

    mdev->c2h_desc->control = cpu_to_le32(DESC_MAGIC | XDMA_DESC_STOPPED | XDMA_DESC_EOP |
                       XDMA_DESC_COMPLETED);
    mdev->c2h_desc->len = cpu_to_le32(sizeof(struct c2h_frame));
    mdev->c2h_desc->src_addr_lo = 0;
    mdev->c2h_desc->src_addr_hi = 0;
    mdev->c2h_desc->dst_addr_lo = cpu_to_le32((u32)(mdev->c2h_frame_phys & 0xffffffff));
    mdev->c2h_desc->dst_addr_hi = cpu_to_le32((u32)(mdev->c2h_frame_phys >> 32));
    mdev->c2h_desc->nxt_addr_lo = 0;
    mdev->c2h_desc->nxt_addr_hi = 0;

    mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x04,0);
    memset(mdev->c2h_frame,0,sizeof(struct c2h_frame));

    mule_reg_write(mdev->membase_xdma, C2H_SGDMA + 0x80, (u32)(mdev->c2h_desc_phys & 0xffffffff));
    mule_reg_write(mdev->membase_xdma, C2H_SGDMA + 0x84, (u32)(mdev->c2h_desc_phys >> 32));
    mule_reg_write(mdev->membase_xdma, C2H_SGDMA + 0x88, (u32)(0));
    mule_reg_write(mdev->membase_xdma, C2H_SGDMA + 0x8C, (u32)(0));

    mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x90, 0x06);

    mdev->c2h_frc0 = mule_reg_read(mdev->membase, MULE_C2H_FRC0);
    do {
        mdev->c2h_busy = ktime_get_ns();
    } while (mdev->c2h_busy == 0);
    mdev->c2h_busy_cnt=0;

    mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x04,
        XDMA_CTRL_RUN_STOP | XDMA_CTRL_IE_DESC_STOPPED | XDMA_CTRL_IE_DESC_COMPLETED |
        XDMA_CTRL_IE_DESC_ALIGN_MISMATCH | XDMA_CTRL_IE_MAGIC_STOPPED | XDMA_CTRL_IE_INVALID_LEN |
        XDMA_CTRL_IE_IDLE_STOPPED | XDMA_CTRL_IE_READ_ERROR | XDMA_CTRL_IE_DESC_ERROR |
        XDMA_CTRL_STM_MODE_WB );

}

static int mule_fill_h2c_frame(struct mule_dev *mdev)
{
    struct h2c_frame *h2cf = (struct h2c_frame *)mdev->h2c_frame;
    int i, j, ready=0;
    u64 txfifo;

    memset(h2cf,0,sizeof(struct h2c_frame));

    for (i = 0; i < MULE_MAXPORTS; i++)
    {
        struct uart_port *port = &mdev->p[i].port;
        struct tty_port *tport = &port->state->port;
        int kflen;
        int len;
        if (!mdev->p[i].start_tx)
            continue;
        h2cf->ports[i].valid = 0;

        if (unlikely(mdev->p[i].x_char)) {
            h2cf->ports[i].data[0] = mdev->p[i].x_char;
            h2cf->ports[i].valid = 1;
            mdev->p[i].x_char = 0;
            ready = 1;
            continue;
        }

        kflen = kfifo_len(&tport->xmit_fifo);
        len = MIN(kflen, MULE_FIFO_LEN);
        if (len) {
            txfifo = ((u64)mule_reg_read(mdev->membase, MULE_UART_TX_FIFO1) << 32) |
                mule_reg_read(mdev->membase, MULE_UART_TX_FIFO0);

            if (txfifo & (u64)(1UL << i)) {
                unsigned char *px;
                unsigned blen;

                uart_port_lock(port);
                blen = kfifo_out_linear_ptr(&tport->xmit_fifo,&px,len);
                BUG_ON(blen > MULE_FIFO_LEN);
                for (j = 0; j < blen; j++) {
                    h2cf->ports[i].data[j] = px[j];
                    h2cf->ports[i].valid |= (1 << j);
                }
                uart_xmit_advance(port, blen);
                uart_port_unlock(port);

                ready = 1;
                if ((kflen - len) < WAKEUP_CHARS)
                    uart_write_wakeup(port);
            }
        }
    }
    return ready;
}

static int mule_h2c_dma_start(struct mule_dev *mdev)
{

    if (0 == mule_fill_h2c_frame(mdev)) {
        return -EAGAIN;
    }

    mdev->h2c_desc->control = cpu_to_le32(DESC_MAGIC | XDMA_DESC_STOPPED | XDMA_DESC_EOP |
                       XDMA_DESC_COMPLETED);
    mdev->h2c_desc->len = cpu_to_le32(sizeof(struct h2c_frame));
    mdev->h2c_desc->src_addr_lo = cpu_to_le32((u32)(mdev->h2c_frame_phys & 0xffffffff));
    mdev->h2c_desc->src_addr_hi = cpu_to_le32((u32)(mdev->h2c_frame_phys >> 32));
    mdev->h2c_desc->dst_addr_lo = 0;
    mdev->h2c_desc->dst_addr_hi = 0;
    mdev->h2c_desc->nxt_addr_lo = 0;
    mdev->h2c_desc->nxt_addr_hi = 0;

    mule_reg_write(mdev->membase_xdma, H2C_SGDMA + 0x80, (u32)(mdev->h2c_desc_phys & 0xffffffff));
    mule_reg_write(mdev->membase_xdma, H2C_SGDMA + 0x84, (u32)(mdev->h2c_desc_phys >> 32));

    mule_reg_write(mdev->membase_xdma, H2C_CHAN + 0x40,0xffffffff);

    mdev->h2c_frc1 = mule_reg_read(mdev->membase, MULE_H2C_FRC1);

    do {
        mdev->h2c_busy = ktime_get_ns();
    } while (mdev->h2c_busy == 0);
    mdev->h2c_busy_cnt = 0;

    mule_reg_write(mdev->membase_xdma, H2C_CHAN + 0x4,
        XDMA_CTRL_RUN_STOP | XDMA_CTRL_IE_DESC_STOPPED | XDMA_CTRL_IE_DESC_COMPLETED |
        XDMA_CTRL_IE_DESC_ALIGN_MISMATCH | XDMA_CTRL_IE_MAGIC_STOPPED );
    mdev->counters.h2c_frames_sent++;

    return 0;
}

static void __mule_int_enable(struct mule_dev *mdev)
{
    mule_reg_write(mdev->membase, MULE_INT_ENABLE0,
        (XDMA_INTR_ALL_ERR_MASK | MULE_INT_H2C_FRAME_ABSORBED ));

    mule_reg_write(mdev->membase, MULE_INT_STATUS, 0);
}

static void __mule_int_disable(struct mule_dev *mdev)
{
    mule_reg_write(mdev->membase, MULE_INT_ENABLE0, 0);
    mule_reg_write(mdev->membase, MULE_INT_ENABLE1, 0);
}

static enum hrtimer_restart c2h_timer_cb(struct hrtimer *timer) {

    struct mule_dev *mdev = container_of(timer, struct mule_dev, c2h_timer);
    u64 now,val;

    spin_lock(&mdev->lock_timer);
    const u32 c2h_mask = ( MULE_INT_C2H_FRAME_RDY | MULE_INT_C2H_BUFFER_RDY);
    u32 c2h_rdy = mule_reg_read(mdev->membase, MULE_INT_STATUS);
    c2h_rdy &= c2h_mask;

    if (mdev->c2h_busy)
        mdev->c2h_busy_cnt++;
    if (mdev->h2c_busy)
        mdev->h2c_busy_cnt++;
    now = ktime_get_ns();

    val = mdev->c2h_busy;

    if (unlikely(val && ((now - val) > C2H_TIMEOUT_NS))) {
        u64 tcnt = mdev->counters.timer_c2h_cnt;
        u64 frc = mdev->counters.c2h_frames_rcvd;
        u32 bhstatus = atomic_read(&mdev->c2h_xdma_status);
        u32 status = mule_reg_read(mdev->membase_xdma, C2H_CHAN + 0x40);
        u32 frc0 = mule_reg_read(mdev->membase, MULE_C2H_FRC0);
        dev_warn(mdev->dev,"%s c2h timeout status 0x%x busy_cnt %d "\
            "now %lld busy %lld  tcnt %lld frc %lld bhstatus 0x%x "\
            "frc0 0x%x -> 0x%x\n",
            __FUNCTION__,status,mdev->c2h_busy_cnt,
            now,val,tcnt,frc,bhstatus,mdev->c2h_frc0,frc0);
        mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x40, status);
        mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x04, 0);
        mdev->c2h_busy = 0;
        mdev->counters.c2h_timeout++;
    }

    if (c2h_rdy && !mdev->c2h_busy && !mdev->h2c_busy) {
        mule_reg_write(mdev->membase,MULE_INT_STATUS,~c2h_rdy);
        mule_c2h_dma_start(mdev);
        mdev->counters.timer_c2h_cnt++;
        goto out;
    }

    val = mdev->h2c_busy;

    if (val && ((now - val) > H2C_TIMEOUT_NS)) {
        u64 tcnt =  mdev->counters.timer_h2c_cnt;
        u64 fabs = mdev->counters.h2c_frames_abs;
        u32 bhstatus = atomic_read(&mdev->mule_intr_status);
        u32 status = mule_reg_read(mdev->membase_xdma, H2C_CHAN + 0x40);
        u32 mstatus = mule_reg_read(mdev->membase, MULE_INT_STATUS);
        u32 menable = mule_reg_read(mdev->membase, MULE_INT_ENABLE0);
        u32 frc1 = mule_reg_read(mdev->membase, MULE_H2C_FRC1);
        u32 frc0 = mule_reg_read(mdev->membase, MULE_H2C_FRC0);
        u32 frc2 = mule_reg_read(mdev->membase, MULE_H2C_FRC2);
        u32 frc3 = mule_reg_read(mdev->membase, MULE_H2C_FRC3);
        dev_warn(mdev->dev,"%s h2c timeout status 0x%x busy_cnt %d "\
            "now %lld busy %lld tcnt %lld abs %lld bhstatus 0x%x "\
            "frc1 0x%x -> 0x%x mstatus 0x%x menable 0x%x "\
            "frc0 0x%x frc2 0x%x frc3 0x%x\n",
            __FUNCTION__,status,mdev->h2c_busy_cnt,\
            now,val,tcnt,fabs,bhstatus,mdev->h2c_frc1,frc1,mstatus,menable,
            frc0,frc2,frc3);
        mule_reg_write(mdev->membase_xdma, H2C_CHAN + 0x40, status);
        mule_reg_write(mdev->membase_xdma, H2C_CHAN + 0x04, 0x0);
        mdev->h2c_busy = 0;
        mdev->counters.h2c_timeout++;
    }

    if (!mdev->c2h_busy && !mdev->h2c_busy) {
        int rc = mule_h2c_dma_start(mdev);
        if (rc == 0) {
            mdev->counters.timer_h2c_cnt++;
        }
    }

out:
    hrtimer_forward_now(timer, ns_to_ktime(poll_time * 1000));
    mdev->counters.timer_loops++;
    spin_unlock(&mdev->lock_timer);
    return HRTIMER_RESTART;
}

#define TARGET_CPU 2

static void __c2h_timer_start(struct mule_dev *mdev) {
    ktime_t ktime;
    if (poll_time < 300)
        poll_time = 300;
    ktime = ktime_set(0, poll_time * 1000);
    hrtimer_init(&mdev->c2h_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED_SOFT);
    mdev->c2h_timer.function = &c2h_timer_cb;
    hrtimer_start(&mdev->c2h_timer, ktime, HRTIMER_MODE_REL_PINNED_SOFT);
}

static void c2h_timer_start(struct mule_dev *mdev) {
    smp_call_function_single(TARGET_CPU,
        (__force smp_call_func_t)__c2h_timer_start, mdev, 1);
}

static void mule_bh_task(struct work_struct *work)
{
    struct mule_dev *mdev = container_of(work, struct mule_dev, mule_bh_work);
    u32 status;

    status = atomic_xchg(&mdev->mule_intr_status,0);

    mule_irq_count_err(mdev, status);
    if (likely(status & (MULE_INT_H2C_FRAME_ABSORBED | MULE_INT_H2C_FRAMING_ERR | MULE_INT_H2C_OVERFLOW_ERR))) {
        u64 now = ktime_get_ns();
        mule_reg_write(mdev->membase_xdma, H2C_CHAN + 0x04, 0x0);
        if (likely(status & MULE_INT_H2C_FRAME_ABSORBED)){
            mdev->counters.h2c_dma_avg = avg_update(&mdev->h2c_dma_savg, now - mdev->h2c_busy);
            mdev->counters.h2c_dma_time_max = MAX(mdev->counters.h2c_dma_time_max, mdev->counters.h2c_dma_avg);
            mdev->counters.h2c_frames_abs++;
            mdev->h2c_busy = 0;
        }
    }
}

static irqreturn_t mule_interrupt(int irq, void *data)
{
    struct mule_dev *mdev = data;
    u32 status,enable0,val;

    status = mule_reg_read(mdev->membase, MULE_INT_STATUS);
    enable0 = mule_reg_read(mdev->membase, MULE_INT_ENABLE0);
    val = status & enable0;

    if (likely(val)) {
        atomic_or(val, &mdev->mule_intr_status);
        mule_reg_write(mdev->membase, MULE_INT_STATUS, ~val);
        queue_work_on(smp_processor_id(),system_bh_highpri_wq,&mdev->mule_bh_work);
    }

    return IRQ_RETVAL(val);
}

static void c2h_xdma_task(struct work_struct *work)
{
    struct mule_dev *mdev = container_of(work, struct mule_dev, c2h_bh_work);
    u32 status;
    int i;

    status = atomic_xchg(&mdev->c2h_xdma_status,0);

    if (likely(status & XDMA_STAT_DESC_COMPLETED)) {
        u64 now = ktime_get_ns();
        mdev->counters.c2h_dma_avg = avg_update(&mdev->c2h_dma_savg, now - mdev->c2h_busy);
        mdev->counters.c2h_dma_time_max = MAX(mdev->counters.c2h_dma_time_max, mdev->counters.c2h_dma_avg);
        mdev->c2h_busy = 0;
        mdev->counters.c2h_frames_rcvd++;
        for (i=0;i<MULE_MAXPORTS;i++) {
            if (mdev->p[i].enabled && !mdev->p[i].stop_rx){
                mule_uart_rx_chars(&mdev->p[i].port);
            }
        }
    }
    else if (status & XDMA_STAT_COMMON_ERR_MASK) {
        dev_err(mdev->dev,"%s c2h desc error 0x%08x\n",__FUNCTION__,status);
        mdev->counters.c2h_desc_err++;
        mdev->c2h_busy = 0;
    }
    else if (status) {
        dev_err(mdev->dev,"%s c2h desc unknown status 0x%08x\n",__FUNCTION__,status);
    }

}

static irqreturn_t c2h_xdma_interrupt(int irq, void *data)
{
    struct mule_dev *mdev = data;
    u32 status;

    status = mule_reg_read(mdev->membase_xdma, C2H_CHAN + 0x40);

    if (likely(status)) {
        mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x40, status);
        mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x04, 0);
        atomic_or(status, &mdev->c2h_xdma_status);
        queue_work_on(smp_processor_id(),system_bh_highpri_wq,&mdev->c2h_bh_work);
    }

    return IRQ_RETVAL(status);
}

static int op_startup(struct uart_port *port)
{
    struct mule_dev *mdev = port->private_data;

    spin_lock(&mdev->lock_ap);
    if (mdev->active_ports == 0)
    {
        avg_init(&mdev->c2h_dma_savg);
        mdev->counters.c2h_dma_time_max = 0;
        avg_init(&mdev->h2c_dma_savg);
        mdev->counters.h2c_dma_time_max = 0;
        c2h_timer_start(mdev);
    }
    mdev->active_ports++;
    spin_unlock(&mdev->lock_ap);

    mule_reg_write(port->membase, MULE_UART_FIFO_CTRL_WO,
        MULE_FCR_RXWM(3) | MULE_FCR_RXRST | MULE_FCR_TXRST);
    mule_uart_mode_set(mdev, port, 1);

    return 0;
}

static void op_shutdown(struct uart_port *port)
{
    struct mule_dev *mdev = port->private_data;
    u32 val;

    spin_lock(&mdev->lock_ap);
    mdev->active_ports--;
    if (mdev->active_ports == 0) {
        hrtimer_cancel(&mdev->c2h_timer);
    }
    spin_unlock(&mdev->lock_ap);

    mule_uart_mode_set(mdev, port, 0);

    val = mule_reg_read(port->membase,MULE_UART_LCR_RW);
    if (MULE_UART_LCR_brkctrl & val)
        mule_reg_write(port->membase, MULE_UART_LCR_RW, (val & ~MULE_UART_LCR_brkctrl));

}

static const char *op_type(struct uart_port *port)
{
    return (port->type == PORT_MULE_UART) ? "Mule UART" : NULL;
}

static int op_request_port(struct uart_port *port)
{
    return 0;
}

static void op_release_port(struct uart_port *port)
{

}

static void op_throttle(struct uart_port *port)
{
    port->mctrl &= ~TIOCM_RTS;
    port->ops->set_mctrl(port, port->mctrl);
}

static void op_unthrottle(struct uart_port *port)
{
    port->mctrl |= TIOCM_RTS;
    port->ops->set_mctrl(port, port->mctrl);
}

static void op_send_xchar(struct uart_port *port, char ch)
{
    struct mule_dev *mdev = port->private_data;
    mdev->p[port->line].x_char = ch;

}

static void op_config_port(struct uart_port *port, int type)
{
    if (type & UART_CONFIG_TYPE)
        port->type = PORT_MULE_UART;

    if (port->line >= MULE_MAXPORTS) {
        port->type = PORT_UNKNOWN;
        return;
    }

    mule_reg_write(port->membase, MULE_UART_LSR_RW, 0);
}

static int op_verify_port(struct uart_port *port, struct serial_struct *ser)
{
    if ((ser->type != PORT_UNKNOWN) && (ser->type != PORT_MULE_UART))
        return -EINVAL;
    return 0;
}

static const struct uart_ops mule_fpga_ops = {
    .tx_empty = op_tx_empty,
    .get_mctrl = op_get_mctrl,
    .set_mctrl = op_set_mctrl,
    .start_tx = op_start_tx,
    .stop_tx = op_stop_tx,
    .start_rx = op_start_rx,
    .stop_rx = op_stop_rx,
    .break_ctl = op_break_ctl,
    .startup = op_startup,
    .shutdown = op_shutdown,
    .set_termios = op_set_termios,
    .type = op_type,
    .throttle = op_throttle,
    .unthrottle = op_unthrottle,
    .send_xchar = op_send_xchar,
    .request_port = op_request_port,
    .release_port = op_release_port,
    .config_port = op_config_port,
    .verify_port = op_verify_port,
};

static struct uart_driver mule_fpga_driver = {
    .owner = THIS_MODULE,
    .driver_name = DRV_NAME,
    .dev_name = "ttyCO",
    .nr = MULE_MAXPORTS,
    .cons = NULL,
};

static void mule_h2c_start(struct mule_dev *mdev)
{
    u32 control = MULE_XDMA_CONTROL_h2c_enable_b | MULE_XDMA_CONTROL_c2h_enable |
        MULE_XDMA_CONTROL_c2h_interval_us(C2H_INTERVAL);

    mule_reg_write(mdev->membase, MULE_XDMA_CONTROL, control);

    __mule_int_enable(mdev);

}

static int mule_interrupt_alloc(struct mule_dev *mdev)
{
    u16 val;
    int rc=0;
    struct cpumask mask;

    pci_read_config_word(mdev->xdma, PCI_COMMAND, &val);
    val |= PCI_COMMAND_INTX_DISABLE;
    pci_write_config_word(mdev->xdma, PCI_COMMAND, val);

    cpumask_clear(&mask);
    cpumask_set_cpu(TARGET_CPU, &mask);

    rc = pci_alloc_irq_vectors(mdev->xdma, 4, 4, PCI_IRQ_MSI | PCI_IRQ_AFFINITY);
    if (rc < 4) {
        dev_err(mdev->dev, "unable to allocate IRQ vectors rc=%d\n", rc);
        goto err_out;
    }

    __mule_int_disable(mdev);
    mule_reg_write(mdev->membase_xdma, IRQ_BLOCK + 0xA0, 0);
    mule_reg_write(mdev->membase_xdma, IRQ_BLOCK + 0x10, 0);

    mdev->h2c_irq = pci_irq_vector(mdev->xdma, 0);
    irq_set_affinity_hint(mdev->h2c_irq, &mask);
    INIT_WORK(&mdev->mule_bh_work, mule_bh_task);
    if (request_irq(mdev->h2c_irq, mule_interrupt, 0, DRV_NAME, mdev)) {
        dev_err(mdev->dev, "unable to request IRQ %d\n", mdev->h2c_irq);
        rc = -EBUSY;
        goto err_out;
    }

    mdev->c2h_irq = pci_irq_vector(mdev->xdma, 1);
    irq_set_affinity_hint(mdev->c2h_irq, &mask);
    INIT_WORK(&mdev->c2h_bh_work, c2h_xdma_task);
    if (request_irq(mdev->c2h_irq, c2h_xdma_interrupt, 0, DRV_NAME, mdev)) {
        dev_err(mdev->dev, "unable to request IRQ %d\n", mdev->c2h_irq);
        rc = -EBUSY;
        goto err_out;
    }

    mule_reg_write(mdev->membase_xdma, H2C_CHAN + 0x04, 0x0);
    mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x04, 0x0);
    mule_reg_write(mdev->membase_xdma, H2C_CHAN + 0x40,0xffffffff);
    mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x40,0xffffffff);

    mule_reg_write(mdev->membase_xdma, IRQ_BLOCK + 0xA0, 0x0100);
    mule_reg_write(mdev->membase_xdma, IRQ_BLOCK + 0x10, 0x02);

    mule_reg_write(mdev->membase_xdma, IRQ_BLOCK + 0x80, 0x0000);
    mule_reg_write(mdev->membase_xdma, IRQ_BLOCK + 0x04, 0x03);

    return 0;

err_out:
    pci_free_irq_vectors(mdev->xdma);
    return rc;
}

static int mule_fpga_probe(struct platform_device *pdev)
{
    struct mule_dev *mdev;
    int i;
    u32 val;
    int rc=0;

    mdev = devm_kzalloc(&pdev->dev, sizeof(struct mule_dev), GFP_KERNEL);
    if (!mdev){
        return -ENOMEM;
    }

    mdev->xdma = pci_get_device(0x1064, 0x7022, NULL);
    if(mdev->xdma == NULL) {
        dev_err(&pdev->dev, "pci_get_device failed\n");
        devm_kfree(&pdev->dev,mdev);
        return -ENODEV;
    }

    rc = pci_enable_device(mdev->xdma);
    if (rc) {
        dev_err(mdev->dev, "pci_enable_device failed rc=%d\n", rc);
        return rc;
    }
    pcie_capability_set_word(mdev->xdma, PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_RELAX_EN);
    pcie_capability_set_word(mdev->xdma, PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_EXT_TAG);

    rc = pcie_set_readrq(mdev->xdma, 512);
    if (rc) {
        dev_err(mdev->dev, "pcie_set_readrq failed rc=%d\n", rc);
        return rc;
    }

    pci_set_master(mdev->xdma);

    rc = pci_request_regions(mdev->xdma, DRV_NAME);
    if (rc) {
        dev_err(mdev->dev, "pci_request_regions failed rc=%d\n", rc);
        return rc;
    }

    mdev->membase = pci_iomap(mdev->xdma, 0, 0);
    mdev->membase_xdma = pci_iomap(mdev->xdma, 1, 0);
    val = mule_reg_read(mdev->membase,MULE_VERSION_STATUS);
    mdev->version = val & 0xff;
    dev_info(&pdev->dev, "%s Version 0x%02x (%08x)\n", __FUNCTION__,
        mdev->version, val);

    if (mdev->version < MULE_FPGA_MIN_REV) {
        dev_err(&pdev->dev, "unsupported firmware version 0x%02x\n", mdev->version);
        pci_iounmap(mdev->xdma, mdev->membase);
        pci_iounmap(mdev->xdma, mdev->membase_xdma);
        pci_release_regions(mdev->xdma);
        devm_kfree(&pdev->dev,mdev);
        return -ENODEV;
    }

    spin_lock_init(&mdev->lock_ap);
    spin_lock_init(&mdev->lock_timer);
    mdev->active_ports = 0;
    mdev->h2c_busy = 0;
    mdev->h2c_busy_cnt = 0;
    mdev->c2h_busy_cnt = 0;
    mdev->c2h_busy = 0;
    atomic_set(&mdev->mule_intr_status,0);
    atomic_set(&mdev->c2h_xdma_status,0);

    mdev->dev = &pdev->dev;
    mule_reg_write(mdev->membase, MULE_XDMA_CONTROL, 0);

    rc = mule_dma_alloc(mdev);
    if (rc) {
        dev_err(&pdev->dev, "%s mule_dma_alloc rc %d\n", __FUNCTION__,rc);
        devm_kfree(&pdev->dev,mdev);
        return rc;
    }

    rc = mule_interrupt_alloc(mdev);
    if (rc) {
        pci_release_regions(mdev->xdma);
        mule_dma_free(mdev);
        devm_kfree(&pdev->dev,mdev);
        return rc;
    }

    mule_reg_write(mdev->membase, MULE_UART_CONTROL0, 0);
    mule_reg_write(mdev->membase, MULE_UART_CONTROL1, 0);

    for (i = 0; i < MULE_MAXPORTS; i++)
    {
        struct uart_port *port;
        port = &mdev->p[i].port;
        port->line = i;
        port->membase = mdev->membase + MULE_PER_UART_OFFSET + (i * 0x20);
        port->type = PORT_MULE_UART;
        port->fifosize = MULE_FIFO_LEN;
        port->uartclk = 9600 * 16;
        port->iotype = SERIAL_IO_MEM;
        port->ops = &mule_fpga_ops;
        port->flags = UPF_BOOT_AUTOCONF | UPF_HARD_FLOW;
        port->dev = &pdev->dev;
        port->private_data = (void*)mdev;
        port->ignore_status_mask = 0;
        rc = uart_add_one_port(&mule_fpga_driver, port);

        if (rc) {
            dev_err(&pdev->dev, "%s uart_add_one_port p=%d rc %d\n", __FUNCTION__,i,rc);
            pci_free_irq_vectors(mdev->xdma);
            devm_kfree(&pdev->dev,mdev);
            return rc;
        }
    }

    platform_set_drvdata(pdev, mdev);
    pci_set_drvdata(mdev->xdma, mdev);

    rc = sysfs_create_group(&mdev->xdma->dev.kobj, &counters_group);
    if (rc)
        dev_err(&pdev->dev, "%s sysfs create failed rc %d\n", __FUNCTION__,rc);

    mule_h2c_start(mdev);

    return rc;
}

static void mule_fpga_remove(struct platform_device *pdev)
{
    int i;
    struct mule_dev *mdev = platform_get_drvdata(pdev);

    __mule_int_disable(mdev);
    mule_reg_write(mdev->membase, MULE_XDMA_CONTROL, 0);
    mule_reg_write(mdev->membase_xdma, H2C_CHAN + 0x04, 0x0);
    mule_reg_write(mdev->membase_xdma, C2H_CHAN + 0x04, 0x0);

    for (i = 0; i < MULE_MAXPORTS; i++)
    {
        struct uart_port *port;
        port = &mdev->p[i].port;
        uart_remove_one_port(&mule_fpga_driver, port);
    }
    irq_set_affinity_hint(mdev->h2c_irq, NULL);
    irq_set_affinity_hint(mdev->c2h_irq, NULL);
    free_irq(mdev->h2c_irq, mdev);
    free_irq(mdev->c2h_irq, mdev);
    pci_free_irq_vectors(mdev->xdma);
    pci_release_regions(mdev->xdma);
    mule_dma_free(mdev);
    sysfs_remove_group(&mdev->xdma->dev.kobj, &counters_group);
    pci_iounmap(mdev->xdma, mdev->membase);
    pci_iounmap(mdev->xdma, mdev->membase_xdma);
    devm_kfree(&pdev->dev,mdev);
}

static struct of_device_id mule_of_match[] = {
    {
        .name = DRV_NAME,
        .compatible = DRV_NAME,
    },
    { },
};

MODULE_DEVICE_TABLE(of, mule_of_match);

static struct platform_driver mule_fpga_platform_driver = {
    .probe = mule_fpga_probe,
    .remove = mule_fpga_remove,
    .driver = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = mule_of_match,
    },
};

static struct platform_device *pdev_simple;

static int __init mule_fpga_init(void)
{
    int rc;

    rc = uart_register_driver(&mule_fpga_driver);
    if (rc < 0) {
        printk(KERN_WARNING "%s uart_register_driver rc %d\n", __FUNCTION__,rc);
        return rc;
    }
    rc = platform_driver_register(&mule_fpga_platform_driver);
    if (rc < 0) {
        printk(KERN_WARNING "%s platform_driver_register rc %d\n", __FUNCTION__,rc);
        uart_unregister_driver(&mule_fpga_driver);
    }
    pdev_simple = platform_device_register_simple(DRV_NAME,0,NULL,0);
    return rc;
}

static void __exit mule_fpga_exit(void)
{
    platform_driver_unregister(&mule_fpga_platform_driver);
    uart_unregister_driver(&mule_fpga_driver);
    platform_device_unregister(pdev_simple);
}

module_init(mule_fpga_init);
module_exit(mule_fpga_exit);

MODULE_DESCRIPTION("Mule UART driver");
MODULE_AUTHOR("jon.goldberg@nokia.com");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
