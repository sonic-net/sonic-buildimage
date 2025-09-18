/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SPI_CPLD_H
#define __SPI_CPLD_H

#include <linux/clk.h>
#include <asm/byteorder.h>


#define SPI_CPLD_BUSY_DELAY (50)
#define SPI_CPLD_BUSY_TIMEOUT (300000)
#define SPI_CPLD_BUSY_WATCHDOG_TIMES (50000)

#define SPI_CPLD_MAX_BYTES 8
/* 8 * 1024 once watchdog */
#define SPI_CPLD_WATCHDOG_TIMES ((8 * 1024) / SPI_CPLD_MAX_BYTES)

#define CPLD_BASE_CLOCK_HZ  25000000
/* f = f_b / ((div + 2) * 2) */
#define SPI_CPLD_MAX_CLOCK_HZ (CPLD_BASE_CLOCK_HZ / (2 * 2))

#define SPI_CPLD_FOR_CPLD (0x1)
#define SPI_CPLD_FOR_FPGA (0x2)
#define SPI_CPLD_FOR_SPI_FLASH (0x3)

#define SPI_CPLD_FOR_SPI_FLASH_ENABLE_OFFSET  (0xa)

struct spi_cpld_regs {
    int config;
    int status;
    int tx0;
    int tx1;
    int data;
};

struct spi_cpld {
    void __iomem *register_base;
    u64 last_cfg;
    u64 cs_enax;
    int sys_freq;
    struct spi_cpld_regs regs;
    struct clk *clk;
};
#define SPI_CPLD_BASE_ADDR (0xb0)
#define SPI_CPLD_CFG(x)    (x->regs.config)
#define SPI_CPLD_DAT0(x)   (x->regs.data)

int spi_cpld_transfer_one_message(struct spi_master *master,
                    struct spi_message *msg);

union spi_cpld_cfg_s {
    uint32_t u32;

    struct spi_cfg_s {
#ifdef __BIG_ENDIAN_BITFIELD
        uint32_t csid:3;
        uint32_t reserved3_2_3:3;
        uint32_t go_start:1;
        uint32_t leavecs:1;
        uint32_t totnum:4;
        uint32_t txnum:4;
        uint32_t rxnum:4;
        uint32_t reserved1_2_3:2;
        uint32_t busy:1;
        uint32_t intr:1;
        uint32_t divider:4;
        uint32_t reserved0_3:1;
        uint32_t intr_en:1;
        uint32_t idlelo:1;
        uint32_t lsbfirst:1;
#else
        uint32_t lsbfirst:1;
        uint32_t idlelo:1;
        uint32_t intr_en:1;
        uint32_t reserved0_3:1;
        uint32_t divider:4;
        uint32_t intr:1;
        uint32_t busy:1;
        uint32_t reserved1_2_3:2;
        uint32_t rxnum:4;
        uint32_t txnum:4;
        uint32_t totnum:4;
        uint32_t leavecs:1;
        uint32_t go_start:1;
        uint32_t reserved3_2_3:3;
        uint32_t csid:3;
#endif
    } s;
};

#endif /* __SPI_CPLD_H */
