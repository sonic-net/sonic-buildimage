/* mule.h */

#ifndef CPUCTL_MOD_H
#define CPUCTL_MOD_H

#define MULE_VERSION_STATUS 0x0
#define MULE_ERROR_ADDRESS 0x4
#define MULE_SCRATCH_PAD0 0x8
#define MULE_SCRATCH_PAD1 0xC
#define MULE_SCRATCH_PAD2 0x10
#define MULE_GPIO_DIRECTION 0x14
#define MULE_GPIO_VALUE 0x18
#define MULE_GPIO_SCAN 0x1c
#define MULE_XDMA_CONTROL 0x20
#define MULE_INT_STATUS 0x24
#define MULE_INT_ENABLE0 0x28
#define MULE_INT_ENABLE1 0x2C
#define MULE_UART_CONTROL0 0x30
#define MULE_UART_CONTROL1 0x34
#define MULE_UART_TX_FIFO0 0x38
#define MULE_UART_TX_FIFO1 0x3c
#define MULE_C2H_FRC0 0x40
#define MULE_C2H_FRC1 0x44
#define MULE_C2H_FRC2 0x48
#define MULE_C2H_FRC3 0x4c
#define MULE_H2C_FRC0 0x50
#define MULE_H2C_FRC1 0x54
#define MULE_H2C_FRC2 0x58
#define MULE_H2C_FRC3 0x5c

#define MULE_PER_UART_OFFSET 0x100
#define MULE_UART_RBR_RO 0x0
#define MULE_UART_THR_WO 0x0
#define MULE_UART_DIV_LSB_RW 0x0
#define MULE_UART_IER_RW 0x04
#define MULE_UART_DIV_MSB_RW 0x04
#define MULE_UART_IIR_RO 0x08
#define MULE_UART_FIFO_CTRL_WO 0x08
#define MULE_UART_LCR_RW 0x0C
#define MULE_UART_MCR_RW 0x10
#define MULE_UART_LSR_RW 0x14
#define MULE_UART_MSR_RW 0x18
#define MULE_UART_SCRATCH_RW 0x1C
#define MULE_UART_CURTEMP 0xe00
#define MULE_UART_MAXTEMP 0xe80
#define MULE_UART_MINTEMP 0xe90

#define MULE_XDMA_CONTROL_h2c_enable_b (1 << 16)
#define MULE_XDMA_CONTROL_c2h_enable (1 << 0)
#define MULE_XDMA_CONTROL_c2h_interval_us(_x) ((((_x / 150 ) - 1) & 0xff) << 2)

#define MULE_UART_LCR_dlab (1 << 7)
#define MULE_UART_LCR_brkctrl (1 << 6)
#define MULE_UART_LCR_stickpar (1 << 5)
#define MULE_UART_LCR_evenpar (1 << 4)
#define MULE_UART_LCR_parity (1 << 3)
#define MULE_UART_LCR_stopbit (1 << 2)
#define MULE_UART_LCR_WLEN5 0x00
#define MULE_UART_LCR_WLEN6 0x01
#define MULE_UART_LCR_WLEN7 0x02
#define MULE_UART_LCR_WLEN8 0x03

#define MULE_FCR_RXRST 0x02
#define MULE_FCR_TXRST 0x04
#define MULE_FCR_RXWM(_x) ((((_x) & 3) << 6))

#define MULE_INT_H2C_FRAMING_ERR (1 << 21)
#define MULE_INT_H2C_OVERFLOW_ERR (1 << 20)
#define MULE_INT_H2C_BUFFER_RDY (1 << 17)
#define MULE_INT_H2C_FRAME_ABSORBED (1 << 16)
#define MULE_INT_C2H_UNDERRUN_ERR (1 << 5)
#define MULE_INT_C2H_OVERFLOW_ERR (1 << 4)
#define MULE_INT_C2H_BUFFER_RDY (1 << 1)
#define MULE_INT_C2H_FRAME_RDY (1 << 0)

#define XDMA_INTR_ALL_ERR_MASK (MULE_INT_H2C_FRAMING_ERR | MULE_INT_H2C_OVERFLOW_ERR | \
    MULE_INT_C2H_UNDERRUN_ERR | MULE_INT_C2H_OVERFLOW_ERR )

#define MULE_LSR_OVERRUN 0x02
#define MULE_LSR_XMIT_FIFO 0x20
#define MULE_LSR_XMIT_EMPTY 0x40
#define MULE_LSR_FIFO_ERR 0x80

#define MULE_MSR_CTS 0x10
#define MULE_MSR_DSR 0x20
#define MULE_MSR_RI  0x40
#define MULE_MSR_DCD 0x80

#define MULE_MCR_DTR (1 << 0)
#define MULE_MCR_RTS (1 << 1)
#define MULE_MCR_OUT0 (1 << 2)
#define MULE_MCR_OUT1 (1 << 3)
#define MULE_MCR_LOOP (1 << 4)
#define MULE_MCR_AFCE (1 << 5)


#define H2C_CHAN 0x0000
#define C2H_CHAN 0x1000
#define IRQ_BLOCK 0x2000
#define H2C_SGDMA 0x4000
#define C2H_SGDMA 0x5000

#define XDMA_CTRL_RUN_STOP			    (1UL << 0)
#define XDMA_CTRL_IE_DESC_STOPPED		(1UL << 1)
#define XDMA_CTRL_IE_DESC_COMPLETED		(1UL << 2)
#define XDMA_CTRL_IE_DESC_ALIGN_MISMATCH	(1UL << 3)
#define XDMA_CTRL_IE_MAGIC_STOPPED		(1UL << 4)
#define XDMA_CTRL_IE_INVALID_LEN		(1UL << 5)
#define XDMA_CTRL_IE_IDLE_STOPPED		(1UL << 6)
#define XDMA_CTRL_IE_READ_ERROR			(0x1FUL << 9)
#define XDMA_CTRL_IE_DESC_ERROR			(0x1FUL << 19)
#define XDMA_CTRL_NON_INCR_ADDR			(1UL << 25)
#define XDMA_CTRL_POLL_MODE_WB			(1UL << 26)
#define XDMA_CTRL_STM_MODE_WB			(1UL << 27)

#define XDMA_STAT_BUSY			    (1UL << 0)
#define XDMA_STAT_DESC_STOPPED		(1UL << 1)
#define XDMA_STAT_DESC_COMPLETED	(1UL << 2)
#define XDMA_STAT_ALIGN_MISMATCH	(1UL << 3)
#define XDMA_STAT_MAGIC_STOPPED		(1UL << 4)
#define XDMA_STAT_INVALID_LEN		(1UL << 5)
#define XDMA_STAT_IDLE_STOPPED		(1UL << 6)
#define XDMA_STAT_COMMON_ERR_MASK \
    (XDMA_STAT_ALIGN_MISMATCH | XDMA_STAT_MAGIC_STOPPED | \
     XDMA_STAT_INVALID_LEN)


#define WINDOW_SIZE 1024
struct sliding_avg {
    u64 buffer[WINDOW_SIZE];
    u64 sum;
    int index;
    int count;
};

static inline void avg_init(struct sliding_avg *s) {
    s->sum = 0;
    s->index = 0;
    s->count = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) s->buffer[i] = 0;
}

static inline u64 avg_update(struct sliding_avg *s, u64 new_val) {
    s->sum -= s->buffer[s->index];
    s->buffer[s->index] = new_val;
    s->sum += new_val;
    s->index = (s->index + 1) % WINDOW_SIZE;
    if (s->count < WINDOW_SIZE)
        s->count++;
    return s->sum / s->count;
}

#endif
