#ifndef __WB_UART_16550_OCORE_H__
#define __WB_UART_16550_OCORE_H__

typedef struct uart_16550_ocore_device_s {
    u32 port_index;
    u32 irq_offset;
    u32 domain;
    u32 bus_num;
    u32 devfn;
    u32 vid;
    u32 did;
    u32 bar_index;
    u32 reg_width;
    u32 reg_offset;
    u32 map_size;
    u32 is_le;
    u32 clk_freq;
    u32 tx_fifo_size;
    int device_flag;
} uart_16550_ocore_device_t;

#endif
