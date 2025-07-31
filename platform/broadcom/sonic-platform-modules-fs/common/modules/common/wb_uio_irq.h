#ifndef __WB_UIO_IRQ_H__
#define __WB_UIO_IRQ_H__

typedef enum {
    MODE_PIRQ_LINE,
    MODE_GPIO,
} uio_irq_mode_t;

typedef struct uio_irq_device_s {
    u32 irq_info_mode;  /* 0: pirq_line mode; 1: gpio mode*/
    u32 pirq_line;
    u32 gpio;
    u32 irq_type;
    int device_flag;    /* Device generates tag, 0: success, -1: failure */
} uio_irq_device_t;

#endif  /* __WB_UIO_IRQ_H__ */