/*
 * Sirilx UART driver
 *
 * Copyright (C) 2019  Ciena Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/workqueue.h>

#include "sirilx_gnss.h"
#define CREATE_TRACE_POINTS
#include "sirilx_gnss_tp.h"

#define SIRILx_GNSS_NAME "ttyGNSS"

struct uart_gnss_priv {
	struct device      *dev;
	struct uart_port    port;
	struct gpio_desc   *gpiod;
	spinlock_t          ugp_lock;
	struct delayed_work tx_work;
	unsigned            rx_highwm;
	u64                 tx_count;
	u64                 rx_trigger;
	u64                 tx_trigger;
	u64                 rx_underflow;
};

enum uart_register {
    UART_DATA = 0,
    PAD,
    UART_CTRL,
    UART_WRITE_FIFO_DEPTH,
    UART_READ_FIFO_DEPTH,
};

#define UART_CTRL_LOOPBACK___MASK            UINT32_C(0x10000)
#define UART_CTRL_RESET_UART___MASK          UINT32_C(0x8000)
#define UART_CTRL_RX_STOP_ERR___MASK         UINT32_C(0x4000)
#define UART_CTRL_RX_START_ERR___MASK        UINT32_C(0x2000)
#define UART_CTRL_RX_FIFO_OVERFLOW___MASK    UINT32_C(0x1000)
#define UART_CTRL_TX_FIFO_OVERFLOW___MASK    UINT32_C(0x800)
#define UART_CTRL_PARITY_ERROR___MASK        UINT32_C(0x400)
#define UART_CTRL_WRITE_FIFO_EMPTY___MASK    UINT32_C(0x200)
#define UART_CTRL_READ_FIFO_EMPTY___MASK     UINT32_C(0x100)
#define UART_CTRL_PARITY___MASK              UINT32_C(0xc0)
#define UART_CTRL_TX_XTRA_STOP___MASK        UINT32_C(0x20)
#define UART_CTRL_RX_XTRA_STOP___MASK        UINT32_C(0x10)
#define UART_CTRL_SEL_SW___MASK              UINT32_C(0x8)
#define UART_CTRL_BAUD_SEL___MASK            UINT32_C(0x7)

#define UART_WRITE_FIFO_DEPTH_fill_level___MASK  UINT32_C(0x3f)
#define UART_READ_FIFO_DEPTH_fill_level___MASK   UINT32_C(0x3f)

#define SIRIL_UART_CS8_PARNO            0x00
#define SIRIL_UART_CS7_PARODD           0x40
#define SIRIL_UART_CS7_PAREVE           0x80
#define SIRIL_UART_CS8_PARODD           0xc0
#define SIRIL_UART_1STOP_BIT            0x00
#define SIRIL_UART_RX_2STOP_BIT         0x10
#define SIRIL_UART_TX_2STOP_BIT         0x20

#define UART_RX_VALID_MASK              0xff
#define UART_RX_PARERR_SHIFT            8
#define UART_RX_PARERR_MASK             (1 << UART_RX_PARERR_SHIFT)
#define UART_RX_STOPERR_SHIFT           11
#define UART_RX_STOPERR_MASK            (1 << UART_RX_STOPERR_SHIFT)

#define UART_RX_ANYERR_MASK             (UART_RX_PARERR_MASK | \
					 UART_RX_STOPERR_MASK)

typedef uint32_t reg_t;
#define UINT32_C(c)  __UINT32_C(c)

static inline reg_t gnss_reg_read(struct uart_gnss_priv *priv,
				  enum uart_register reg)
{
	void __iomem *addr = priv->port.membase + reg * sizeof(uint32_t);
	return ioread32(addr);
}

static inline void gnss_reg_write(struct uart_gnss_priv *priv,
				  enum uart_register reg, reg_t val)
{
	void __iomem *addr = priv->port.membase + reg * sizeof(uint32_t);
	iowrite32(val, addr);
}

static inline reg_t read_uart_ctrl(struct uart_gnss_priv *priv)
{
	return gnss_reg_read(priv, UART_CTRL);
}

static inline void write_uart_ctrl(struct uart_gnss_priv *priv, reg_t val)
{
	gnss_reg_write(priv, UART_CTRL, val);
}

static inline void clr_write_fifo_overflow(struct uart_gnss_priv *priv)
{
	unsigned long flags = 0;
	reg_t ctrl;

	spin_lock_irqsave(&priv->ugp_lock, flags);

	ctrl = read_uart_ctrl(priv);
	ctrl &= ~(UART_CTRL_TX_FIFO_OVERFLOW___MASK);

	write_uart_ctrl(priv, ctrl);

	spin_unlock_irqrestore(&priv->ugp_lock, flags);
}

static inline void clr_read_fifo_overflow(struct uart_gnss_priv *priv)
{
	unsigned long flags = 0;
	reg_t ctrl;

	spin_lock_irqsave(&priv->ugp_lock, flags);

	ctrl = read_uart_ctrl(priv);
	ctrl &= ~(UART_CTRL_RX_FIFO_OVERFLOW___MASK);

	write_uart_ctrl(priv, ctrl);

	spin_unlock_irqrestore(&priv->ugp_lock, flags);
}

static inline reg_t read_uart_data(struct uart_gnss_priv *priv)
{
	return gnss_reg_read(priv, UART_DATA);
}

static inline void write_uart_data(struct uart_gnss_priv *priv, reg_t val)
{
	gnss_reg_write(priv, UART_DATA, val);
}

static inline bool is_write_fifo_overflow(struct uart_gnss_priv *priv)
{
	return (0 != (read_uart_ctrl(priv)
		      & (UART_CTRL_TX_FIFO_OVERFLOW___MASK)));
}

static inline bool is_read_fifo_overflow(struct uart_gnss_priv *priv)
{
	return (0 != (read_uart_ctrl(priv)
		      & (UART_CTRL_RX_FIFO_OVERFLOW___MASK)));
}

static inline bool is_write_fifo_empty(struct uart_gnss_priv *priv)
{
	return (0 != (read_uart_ctrl(priv)
		      & (UART_CTRL_WRITE_FIFO_EMPTY___MASK)));
}

static inline bool is_read_fifo_empty(struct uart_gnss_priv *priv)
{
	return (0 != (read_uart_ctrl(priv)
		      & (UART_CTRL_READ_FIFO_EMPTY___MASK)));
}

static inline reg_t depth_write_fifo(struct uart_gnss_priv *priv)
{
	return gnss_reg_read(priv, UART_WRITE_FIFO_DEPTH)
		& (UART_WRITE_FIFO_DEPTH_fill_level___MASK);
}

static inline reg_t depth_read_fifo(struct uart_gnss_priv *priv)
{
	return gnss_reg_read(priv, UART_READ_FIFO_DEPTH)
		& (UART_READ_FIFO_DEPTH_fill_level___MASK);
}

static void reset_uart(struct uart_gnss_priv *priv)
{
	unsigned long flags        = 0;
	unsigned      remaining    = 2 * priv->port.fifosize;
	ktime_t       tstamp;
	reg_t         ctrl;
	s64           reset_max_us = 10; /* FPGA people say 5 */
	s64           reset_wait;
	int           buzz         = 0;

	spin_lock_irqsave(&priv->ugp_lock, flags);

	ctrl = (read_uart_ctrl(priv) |
	        (UART_CTRL_RESET_UART___MASK));

	ctrl &= ~(UART_CTRL_RX_FIFO_OVERFLOW___MASK);
	ctrl &= ~(UART_CTRL_TX_FIFO_OVERFLOW___MASK);
	ctrl &= ~(UART_CTRL_RX_STOP_ERR___MASK);
	ctrl &= ~(UART_CTRL_RX_START_ERR___MASK);
	ctrl &= ~(UART_CTRL_PARITY_ERROR___MASK);

	write_uart_ctrl(priv, ctrl);

	tstamp = ktime_get_raw();

	do {
		buzz++;

		ctrl       = read_uart_ctrl(priv);
		reset_wait = ktime_us_delta(ktime_get_raw(), tstamp);

		if (reset_max_us < reset_wait)
			break;
	} while (0 != (ctrl & UART_CTRL_RESET_UART___MASK));

	spin_unlock_irqrestore(&priv->ugp_lock, flags);

	if (ctrl & UART_CTRL_RESET_UART___MASK)
		dev_warn(priv->dev, "no reset after %lld usecs, ctrl=0x%x\n",
			 reset_wait, ctrl);

	trace_sirilx_gnss_reset_uart(dev_name(priv->dev), buzz,
				     (int) reset_wait, ctrl);

	while (!is_read_fifo_empty(priv)) {
		/* A UART reset is supposed to empty the RX FIFO. But
		 * it does not always happen. If/when UART resets
		 * become reliable, this loop will not execute. */
		read_uart_data(priv);
		/* insane draining safeguard */
		if (!remaining--) {
			dev_err(priv->dev, "drained more than %u chars!\n",
				priv->port.fifosize);
			break;
		}
	}
}

static void sirilx_gnss_overflow_irq(struct uart_gnss_priv *priv,
				     reg_t                  depth)
{
	struct sirilx_gnss_platform_data *pdata = dev_get_platdata(priv->dev);
	const unsigned char              *oflow = pdata->oflow_pattern;
	struct uart_port                 *port  = &priv->port;
	struct tty_port                  *tport = &port->state->port;
	int                               oflsz = pdata->oflow_size;
	int                               ovbuf = 0;
	int                               rc;

	port->icount.overrun++;

	if (C_CREAD(tport->tty)) {
		/* if the user provided an overflow pattern,
		 * push it out now */
		if (oflow) rc = tty_insert_flip_string_fixed_flag(tport,
								  oflow,
								  TTY_OVERRUN,
								  oflsz);
		else rc = tty_insert_flip_char(tport, 0, TTY_OVERRUN);

		ovbuf = (0 == rc);
	}

	if (ovbuf) port->icount.buf_overrun++;

	trace_sirilx_gnss_rx_overflow(dev_name(priv->dev), depth, ovbuf);
}

static irqreturn_t sirilx_gnss_rx_irq(int irq, void *data)
{
	struct uart_gnss_priv *priv  = data;
	struct uart_port      *port  = &priv->port;
	struct tty_port       *tport = &port->state->port;
	reg_t                  statc = 0;
	reg_t                  depth;

	priv->rx_trigger++;
	trace_sirilx_gnss_irq(dev_name(priv->dev));

	if ((0 == (depth = depth_read_fifo(priv))) &&
	    is_read_fifo_empty(priv)) {
		priv->rx_underflow++;
		return IRQ_HANDLED;
	}

	do {
		unsigned int rx, ch;
		char         flag;
		int          overbuf = 0;

		if (depth > priv->rx_highwm) priv->rx_highwm = depth;

		if (unlikely(is_read_fifo_overflow(priv))) {
			sirilx_gnss_overflow_irq(priv, depth);
			/* RT thread will do the UART reset */
			return IRQ_WAKE_THREAD;
		}

		rx = ch = read_uart_data(priv);
		flag = TTY_NORMAL;
		port->icount.rx++;
		ch &= 0xff;

		if (unlikely(rx & UART_RX_ANYERR_MASK)) {
			/* record stats first */
			if (rx & UART_RX_PARERR_MASK) {
				port->icount.parity++;
				statc |= UART_CTRL_PARITY_ERROR___MASK;
			}
			else if (rx & UART_RX_STOPERR_MASK) {
				port->icount.frame++;
				statc |= UART_CTRL_RX_STOP_ERR___MASK;
			}

			rx &= port->read_status_mask;

			/* update flag wrt read_status_mask */
			if (rx & UART_RX_PARERR_MASK)
				flag = TTY_PARITY;
			else if (rx & UART_RX_STOPERR_MASK)
				flag = TTY_FRAME;
		}

		if (C_CREAD(tport->tty) &&
		    /* If this bit is set, input can be read from the
		     * terminal. Otherwise, input is discarded when it
		     * arrives.
		     */
		    ((rx & port->ignore_status_mask) == 0))
			if (!tty_insert_flip_char(tport, ch, flag)) {
				overbuf = 1;
				port->icount.buf_overrun++;
			}

		trace_sirilx_gnss_rx(dev_name(priv->dev), depth, overbuf, rx);

		/* always check emptiness *and* depth */
	} while ((0 != (depth = depth_read_fifo(priv))) ||
		 !is_read_fifo_empty(priv));

	tty_flip_buffer_push(tport);

	/* clear the control register bits if needed */
	if (statc) write_uart_ctrl(priv, ~statc & read_uart_ctrl(priv));

	return IRQ_HANDLED;
}

/*
 * Implementation sirilx_gnss_pops
 */

static unsigned int sirilx_tx_empty(struct uart_port *port)
{
	struct uart_gnss_priv *priv =
		container_of(port, struct uart_gnss_priv, port);

	return is_write_fifo_empty(priv) ? TIOCSER_TEMT : 0;
}

static void sirilx_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	/* N/A */
}

static unsigned int sirilx_get_mctrl(struct uart_port *port)
{
	/* As per documentation:
	 * If the port does not support CTS, DCD or DSR, the driver
	 * should indicate that the signal is permanently active.  If
	 * RI is not available, the signal should not be indicated as
	 * active.
	 */
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void sirilx_disable_uart(struct uart_port *port)
{
	struct uart_gnss_priv *priv =
		container_of(port, struct uart_gnss_priv, port);

	trace_sirilx_gnss_disable_uart(dev_name(priv->dev));

	disable_irq_nosync(port->irq);

	dev_dbg(port->dev, "%s\n", __func__);
}

static void sirilx_enable_uart(struct uart_port *port)
{
	struct uart_gnss_priv *priv =
		container_of(port, struct uart_gnss_priv, port);

	trace_sirilx_gnss_enable_uart(dev_name(priv->dev));

	enable_irq(port->irq);

	dev_dbg(port->dev, "%s\n", __func__);
}

#define TX_FIFO_SZ 32
static void sirilx_gnss_tx_work(struct work_struct *work)
{
	struct uart_gnss_priv *priv;
	struct uart_port      *port;
	struct tty_port       *tport;
	unsigned long          flags;
	reg_t                  depth;
	reg_t                  tx;

	priv = container_of(work, struct uart_gnss_priv, tx_work.work);
	port = &priv->port;
	tport = &port->state->port;

	if ((TX_FIFO_SZ >> 1) <= (depth = depth_write_fifo(priv))) {
		schedule_work(work);
		return;
	}

	spin_lock_irqsave(&port->lock, flags);

	if (kfifo_is_empty(&tport->xmit_fifo)) {
		spin_unlock_irqrestore(&port->lock, flags);
		uart_write_wakeup(port);
		return;
	}

	uart_fifo_out(port, (unsigned char*)(&tx), 1);
	//tx = xmit->buf[xmit->tail];
	write_uart_data(priv, tx);
	//uart_xmit_advance(port, 1);

	priv->tx_count++;
	trace_sirilx_gnss_tx(dev_name(priv->dev), depth, tx);

	spin_unlock_irqrestore(&port->lock, flags);

	schedule_work(work);
}

static void sirilx_stop_tx(struct uart_port *port)
{
	struct uart_gnss_priv *priv;

	priv = container_of(port, struct uart_gnss_priv, port);

	dev_dbg(port->dev, "%s\n", __func__);

	/* This function runs while holding the port spinlock.
	 * Transmission must be stopped 'as soon as possible'. */
	cancel_delayed_work(&priv->tx_work);
}

static void sirilx_start_tx(struct uart_port *port)
{
	struct uart_gnss_priv *priv;

	priv = container_of(port, struct uart_gnss_priv, port);

	priv->tx_trigger++;

	/* This delay is required between writes, otherwise Trimble cannot
	 * handle the data being pushed through the tx FIFO.
	 * There is no indication via FPGA ctrl registers that allows us to
	 * anticipate this condition.
	 * Unlock during delay as not to block processing of rx FIFO via IRQ,
	 * as that WILL result in rx FIFO overflow!
	 */
	schedule_delayed_work(&priv->tx_work, msecs_to_jiffies(2));
}

static void sirilx_stop_rx(struct uart_port *port)
{
	struct uart_gnss_priv *priv =
		container_of(port, struct uart_gnss_priv, port);

	trace_sirilx_gnss_stop_rx(dev_name(priv->dev));
	dev_dbg(port->dev, "%s\n", __func__);

	/* This function runs while holding the port spinlock.
	 * There cannot be any waiting for interrupts to stop. */
	disable_irq_nosync(port->irq);
}

static irqreturn_t sirilx_gnss_irq_thread(int irq, void *data);
static int sirilx_startup(struct uart_port *port)
{
	int                    ret;
	struct uart_gnss_priv *priv =
		container_of(port, struct uart_gnss_priv, port);

	reset_uart(priv);

	if (!port->irq) {
		dev_err(priv->dev, "driver requires an IRQ\n");
		return -EINVAL;
	}

	ret = request_threaded_irq(port->irq, sirilx_gnss_rx_irq,
				   sirilx_gnss_irq_thread,
				   IRQF_ONESHOT,
				   dev_name(priv->dev), priv);
	if (ret) {
		dev_err(priv->dev, "no irq (%d)\n", ret);
		return ret;
	}

	dev_info(port->dev, "%s: UART_CTRL(0x%x)\n",
		 __func__, read_uart_ctrl(priv));

	return 0;
}

static void sirilx_shutdown(struct uart_port *port)
{
	struct uart_gnss_priv *priv =
		container_of(port, struct uart_gnss_priv, port);

	free_irq(port->irq, priv);

	/* This function does not hold the port spinlock. It can
	 * safely wait for the the rx and tx workers to finish. */
	cancel_delayed_work_sync(&priv->tx_work);

	reset_uart(priv);

	dev_info(port->dev, "%s (rx_trigger=%llu, tx_trigger=%llu, "
		 "rx_underflow=%llu, rx_overflow=%d)\n",
		 __func__, priv->rx_trigger, priv->tx_trigger,
		 priv->rx_underflow, priv->port.icount.overrun);

	priv->rx_highwm = priv->tx_count = 0;
	priv->rx_trigger = priv->tx_trigger = 0;
	priv->rx_underflow = 0;
}

static void sirilx_set_termios(struct uart_port      *port,
			       struct ktermios       *new,
			       const struct ktermios *old)
{
	struct uart_gnss_priv *priv =
		container_of(port, struct uart_gnss_priv, port);
	unsigned long flags = 0;
	unsigned int cflag;
	reg_t ctrl;
	unsigned int baud;
	unsigned int old_csize = old ? old->c_cflag & CSIZE : CS8;

	/*
	 * We only support CS7 with odd/even parity and CS8 with Odd/No parity
	 */

	while ((new->c_cflag & CSIZE) != CS7 &&
	       (new->c_cflag & CSIZE) != CS8) {
		new->c_cflag &= ~CSIZE;
		new->c_cflag |= old_csize;
		old_csize = CS8;
	}

	cflag = new->c_cflag;

	baud = uart_get_baud_rate(port, new, old, 0, port->uartclk / 16);
	if (!baud) {
		dev_warn(port->dev, "baud rate zero: default to 115200\n");
		baud = 115200;
	}

	spin_lock_irqsave(&priv->port.lock, flags);
	sirilx_disable_uart(port);

	ctrl = read_uart_ctrl(priv);
	ctrl &= ~(UART_CTRL_PARITY___MASK);
	ctrl &= ~(UART_CTRL_RX_XTRA_STOP___MASK);
	ctrl &= ~(UART_CTRL_TX_XTRA_STOP___MASK);
	ctrl &= ~(UART_CTRL_BAUD_SEL___MASK);


	switch (cflag & CSIZE) {
	case CS7:
		if ((cflag & PARENB) && (cflag & PARODD)) {
			ctrl |= SIRIL_UART_CS7_PARODD;
		}
		else {
			ctrl |= SIRIL_UART_CS7_PAREVE;
		}
		break;
	case CS8:
	default:
		if ((cflag & PARENB) && (cflag & PARODD)) {
			ctrl |= SIRIL_UART_CS8_PARODD;
		}
		else {
			ctrl |= SIRIL_UART_CS8_PARNO;
		}
	}

	if (cflag & CSTOPB) {
		ctrl |= SIRIL_UART_TX_2STOP_BIT;
		ctrl |= SIRIL_UART_RX_2STOP_BIT;
	}

	switch (baud) {
	case 9600:
		  ctrl |= 0x4;
		  break;
	case 19200:
		  ctrl |= 0x0;
		  break;
	case 38400:
		  ctrl |= 0x1;
		  break;
	case 57600:
		  ctrl |= 0x2;
		  break;
	case 115200:
	default:
		  ctrl |= 0x3;
	}

	/* Update read/ignore mask */
	priv->port.read_status_mask   = 0;
	priv->port.ignore_status_mask = 0;
	if (new->c_iflag & INPCK) {
		/* Description from the glibc manual:
		 *
		 * If this bit is set, input parity checking is
		 * enabled. If it is not set, no checking at all is
		 * done for parity errors on input; the characters are
		 * simply passed through to the application.
		 *
		 * Parity checking on input processing is independent
		 * of whether parity detection and generation on the
		 * underlying terminal hardware is enabled; see
		 * Control Modes. For example, you could clear the
		 * INPCK input mode flag and set the PARENB control
		 * mode flag to ignore parity errors on input, but
		 * still generate parity on output.
		 *
		 * If this bit is set, what happens when a parity
		 * error is detected depends on whether the IGNPAR or
		 * PARMRK bits are set. If neither of these bits are
		 * set, a byte with a parity error is passed to the
		 * application as a '\0' character.
		 *
		 * NOTE: the kernel 8250 driver watches parity and
		 * framing errors when INPCK is set. This driver
		 * monkeys the behaviour.
		 */
		priv->port.read_status_mask |= UART_RX_PARERR_MASK;
		priv->port.read_status_mask |= UART_RX_STOPERR_MASK;

		if (new->c_iflag & IGNPAR) {
			/* If this bit is set, any byte with a framing
			 * or parity error is ignored. This is only
			 * useful if INPCK is also set.
			 */
			priv->port.ignore_status_mask |= UART_RX_PARERR_MASK;
			priv->port.ignore_status_mask |= UART_RX_STOPERR_MASK;
		}
		else if (new->c_iflag & PARMRK) {
			/* If this bit is set, input bytes with parity
			 * or framing errors are marked when passed to
			 * the program. This bit is meaningful only
			 * when INPCK is set and IGNPAR is not set.
			 *
			 * The way erroneous bytes are marked is with
			 * two preceding bytes, 377 and 0. Thus, the
			 * program actually reads three bytes for one
			 * erroneous byte received from the terminal.
			 *
			 * If a valid byte has the value 0377, and
			 * ISTRIP (see below) is not set, the program
			 * might confuse it with the prefix that marks
			 * a parity error. So a valid byte 0377 is
			 * passed to the program as two bytes, 0377
			 * 0377, in this case.
			 *
			 * NOTE: there is nothing to do here, the 0xff
			 * 0x00 marking is performed by the core UART
			 * driver.
			 */
		}
	}

	/* update the per-port timeout */
	uart_update_timeout(port, cflag, baud);

	write_uart_ctrl(priv, ctrl);

	dev_info(port->dev, "%s: baud:%d UART_CTRL(0x%x:0x%x)\n",
			__func__, baud, read_uart_ctrl(priv), ctrl);

	reset_uart(priv);
	sirilx_enable_uart(port);
	spin_unlock_irqrestore(&priv->port.lock, flags);
}

static const char *sirilx_type(struct uart_port *port)
{
	return "Sirilx GNSS UART iface";
}

static irqreturn_t sirilx_gnss_irq_thread(int irq, void *data)
{
	struct uart_gnss_priv *priv = data;

	reset_uart(priv);

	return IRQ_HANDLED;
}

static void sirilx_config_port(struct uart_port *port, int flags)
{
	struct uart_gnss_priv *priv =
		container_of(port, struct uart_gnss_priv, port);
	reg_t ctrl;

	port->type = PORT_8250; /* Avoid PORT_UNKNOWN */

	ctrl = read_uart_ctrl(priv);
	switch (ctrl & UART_CTRL_BAUD_SEL___MASK) {
	case 0:
		port->uartclk = 19200 * 16;
		break;
	case 1:
		port->uartclk = 38400 * 16;
		break;
	case 2:
		port->uartclk = 57600 * 16;
		break;
	case 3:
		port->uartclk = 115200 * 16;
		break;
	case 4:
		port->uartclk = 9600 * 16;
		break;
	default:
		dev_info(port->dev, "invalid baud: 0x%x, back to 115200\n",
			 ctrl & UART_CTRL_BAUD_SEL___MASK);
		port->uartclk = 115200 * 16;
		break;
	}

	dev_info(port->dev, "%s: UART_CTRL(0x%x)\n",
		 __func__, read_uart_ctrl(priv));
}

static const struct uart_ops sirilx_gnss_pops = {
	.tx_empty       = sirilx_tx_empty,
	.set_mctrl      = sirilx_set_mctrl,
	.get_mctrl      = sirilx_get_mctrl,
	.stop_tx        = sirilx_stop_tx,
	.start_tx       = sirilx_start_tx,
	.stop_rx        = sirilx_stop_rx,
	.startup        = sirilx_startup,
	.shutdown       = sirilx_shutdown,
	.set_termios    = sirilx_set_termios,
	.type           = sirilx_type,
	.config_port    = sirilx_config_port,
};

static struct uart_driver sirilx_gnss_reg = {
	.owner          = THIS_MODULE,
	.driver_name    = GNSS_NAME "-drv",
	.dev_name       = SIRILx_GNSS_NAME,
	.nr             = 2,    /* support 2 port.                 */
	.cons           = 0,    /* Not supporting console on this UART. */
};

static ssize_t sirilx_gnss_stats_show(struct device           *dev,
				      struct device_attribute *attr,
				      char                    *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct uart_gnss_priv  *priv = platform_get_drvdata(pdev);

	return sprintf(buf,
		       "rx_count      %d\n"
		       "rx_trigger    %llu\n"
		       "rx_underflow  %llu\n"
		       "rx_overflow   %u\n"
		       "rx_highwm     %u\n"
		       "rx_frame_err  %d\n"
		       "rx_parity_err %d\n"
		       "tx_count      %llu\n"
		       "tx_trigger    %llu\n"
		       "buf_overrun   %d\n",
		       priv->port.icount.rx,
		       priv->rx_trigger,
		       priv->rx_underflow,
		       priv->port.icount.overrun,
		       priv->rx_highwm,
		       priv->port.icount.frame,
		       priv->port.icount.parity,
		       priv->tx_count,
		       priv->tx_trigger,
		       priv->port.icount.buf_overrun);
}
static DEVICE_ATTR(gnss_stats, 0444, sirilx_gnss_stats_show, NULL);

static int sirilx_gnss_probe(struct platform_device *pdev)
{
	struct resource *res = 0;
	struct device   *dev = &pdev->dev;
	int              irq = 0;
	int              ret = 0;

	struct sirilx_gnss_platform_data *pdata;
	struct uart_gnss_priv            *priv;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (! priv) {
		dev_err(&pdev->dev, "Unable to allocate priv\n");
		return -ENOMEM;
	}

	pdata = dev_get_platdata(dev);
	BUG_ON(! pdata);

	priv->dev = &pdev->dev;
	priv->port = pdata->port;
	spin_lock_init(&priv->ugp_lock);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res) irq = res->start;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	BUG_ON(!res);
	priv->port.membase = devm_ioremap(&pdev->dev, res->start,
					  resource_size(res));
	BUG_ON(! priv->port.membase);
	dev_dbg(&pdev->dev, "%pR, iobase=%p\n", res, priv->port.membase);

	if (0 < irq) priv->port.irq = irq;

	priv->port.ops      = &sirilx_gnss_pops;
	priv->port.dev      = &pdev->dev;
	priv->port.iotype   = UPIO_MEM;
	priv->port.flags    = UPF_BOOT_AUTOCONF;  /* Run uart_ops.config_port */
	priv->port.line     = pdev->id;
	priv->port.fifosize = 32;

	INIT_DELAYED_WORK(&priv->tx_work, sirilx_gnss_tx_work);

	platform_set_drvdata(pdev, priv);
	ret = uart_add_one_port(&sirilx_gnss_reg, &priv->port);
	if (ret) {
		dev_err(&pdev->dev, "uart_add_one_port failed:%d\n",ret);
		return ret;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_gnss_stats);
	if (ret) {
		dev_err(&pdev->dev, "cannot create gnss_stats file:%d\n", ret);
		uart_remove_one_port(&sirilx_gnss_reg, &priv->port);
		return ret;
	}

	dev_info(&pdev->dev, "%s \n", __func__);

	return 0;
}

static void sirilx_gnss_remove(struct platform_device *pdev)
{
	struct uart_gnss_priv *priv = platform_get_drvdata(pdev);

	device_remove_file(&pdev->dev, &dev_attr_gnss_stats);
	uart_remove_one_port(&sirilx_gnss_reg, &priv->port);
	if (priv->gpiod) gpiod_put(priv->gpiod);
	devm_kfree(&pdev->dev, priv);
}

static struct platform_driver sirilx_gnss_driver = {
	.probe  = sirilx_gnss_probe,
	.remove = sirilx_gnss_remove,
	.driver = {
		.name = GNSS_NAME,
	},
};

static int __init sirilx_gnss_init(void)
{
	int ret = -1;

	ret = uart_register_driver(&sirilx_gnss_reg);
	if (0 != ret)
		return ret;
	ret = platform_driver_register(&sirilx_gnss_driver);
	if (0 != ret)
		uart_unregister_driver(&sirilx_gnss_reg);
	return ret;
}

static void __exit sirilx_gnss_exit(void)
{
	/* Order is important here: unregistering uart driver before platform
	 * driver will crash system. */
	platform_driver_unregister(&sirilx_gnss_driver);
	uart_unregister_driver(&sirilx_gnss_reg);
}

module_init(sirilx_gnss_init);
module_exit(sirilx_gnss_exit);

MODULE_ALIAS("platform:" GNSS_NAME);
MODULE_DESCRIPTION("Sirilx GNSS UART Driver");
MODULE_AUTHOR("Currie Reid <creid@ciena.com>");
MODULE_LICENSE("GPL v2");
