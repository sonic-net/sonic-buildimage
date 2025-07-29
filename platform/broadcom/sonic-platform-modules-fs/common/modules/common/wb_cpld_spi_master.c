/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2011, 2012 Cavium, Inc.
 */

#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/nmi.h>
#include <linux/version.h>
#include "wb_cpld_spi_master.h"
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define SPI_CPLD_MAX_TRANSFER_SIZE     (512)

static u32 lpc_read(void __iomem * addr)
{
    u32 data;

    data = readl(addr);
    DEBUG_VERBOSE(KERN_INFO "Read (%llx) %08x\n", (u64)addr, data);
    return data;
}

static void lpc_write(u32 val, void __iomem *addr)
{
    writel(val, addr);
    DEBUG_VERBOSE(KERN_INFO "Write (%llx) %08x\n", (u64)addr, val);
    return;
}

static int spi_cpld_wait_ready(struct spi_cpld *p)
{
    union spi_cpld_cfg_s mpi_cfg;
    unsigned int loops = 0;

    do {
        loops++;
        if ((loops % (SPI_CPLD_BUSY_WATCHDOG_TIMES/SPI_CPLD_BUSY_DELAY)) == 0) {
            touch_nmi_watchdog();
        }
        if (loops > (SPI_CPLD_BUSY_TIMEOUT / SPI_CPLD_BUSY_DELAY)) {
            return -EBUSY;
        }

        __delay(SPI_CPLD_BUSY_DELAY);
        mpi_cfg.u32 = lpc_read(p->register_base + SPI_CPLD_CFG(p));
    } while (mpi_cfg.s.busy);

    return 0;
}

static int spi_cpld_do_one_transfer(struct spi_cpld *p,
                                    struct spi_transfer *xfer,
                                    u32 init_cfg,
                                    const u8 *tx_buf,
                                    u8 *rx_buf,
                                    int len,
                                    bool is_end,
                                    bool last_xfer)
{
    int i;
    union spi_cpld_cfg_s mpi_cfg;
    u32 data;
    int ret;

    data = 0;
    mpi_cfg.u32 = init_cfg;
    if (tx_buf) {
        for (i = 0; i < len; i += 4) {
            if (i < len) {
                data |= tx_buf[i];
            }
            if (i + 1 < len) {
                data |= tx_buf[i + 1 ] << 8;
            }

            if (i + 2 < len) {
                data |= tx_buf[i + 2 ] << 16;
            }

            if (i + 3 < len) {
                data |= tx_buf[i + 3 ] << 24;
            }

            lpc_write(data, p->register_base + SPI_CPLD_DAT0(p) + i);
            data = 0;
        }
    }

    if (is_end) {
        if (last_xfer)
            mpi_cfg.s.leavecs = xfer->cs_change;
        else
            mpi_cfg.s.leavecs = !xfer->cs_change;
    } else {
        mpi_cfg.s.leavecs = 1;
    }

    mpi_cfg.s.txnum = tx_buf ? len : 0;
    mpi_cfg.s.totnum = len;
    mpi_cfg.s.go_start = 1;
    lpc_write(mpi_cfg.u32, p->register_base + SPI_CPLD_CFG(p));

    ret = spi_cpld_wait_ready(p);
    if (ret < 0) {
        DEBUG_VERBOSE(KERN_INFO "Wait ready timeout.\n");
        return ret;
    }

    if (rx_buf) {
        for (i = 0; i < len; i += 4) {
            u32 v = lpc_read(p->register_base + SPI_CPLD_DAT0(p) + i);
            rx_buf[i] = v  & 0xff;
            if (i + 1 < len) {
                rx_buf[i + 1] = v >> 8 & 0xff;
            }
            if (i + 2 < len) {
                rx_buf[i + 2] = v >> 16 & 0xff;
            }
            if (i + 3 < len) {
                rx_buf[i + 3] = v >> 24 & 0xff;
            }
        }
    }

    return len;
}

static int spi_cpld_do_transfer(struct spi_cpld *p,
                  struct spi_message *msg,
                  struct spi_transfer *xfer,
                  int chip_select,
                  bool last_xfer)
{
    struct spi_device *spi = msg->spi;
    union spi_cpld_cfg_s mpi_cfg;
    unsigned int clkdiv;
    int mode;
    bool cpha, cpol;
    const u8 *tx_buf;
    u8 *rx_buf;
    int len;
    int count;
    int ret;

    mode = spi->mode;
    cpha = mode & SPI_CPHA;
    cpol = mode & SPI_CPOL;

    if (xfer->speed_hz != 0 && xfer->speed_hz < SPI_CPLD_MAX_CLOCK_HZ) {
        clkdiv = p->sys_freq / (2 * xfer->speed_hz) - 2;
    } else {
        clkdiv = p->sys_freq / (2 * SPI_CPLD_MAX_CLOCK_HZ) - 2;
        DEBUG_VERBOSE(KERN_INFO "Use default ");
    }
    DEBUG_VERBOSE(KERN_INFO "clkdiv %d, xfer->speed_hz %d, xfer->cs_change %d\n",
        clkdiv, xfer->speed_hz, xfer->cs_change);

    mpi_cfg.u32 = 0;
    mpi_cfg.s.divider = clkdiv;
    mpi_cfg.s.lsbfirst = (mode & SPI_LSB_FIRST) ? 1 : 0;
    mpi_cfg.s.idlelo = cpha != cpol;
    if (chip_select == 1) {
        /* CPLD */
        mpi_cfg.s.csid = SPI_CPLD_FOR_CPLD;
    } else if (chip_select == 2){
        /* FPGA */
        mpi_cfg.s.csid = SPI_CPLD_FOR_FPGA;
    } else if (chip_select == 3){
        /* SPI FLASH */
        mpi_cfg.s.csid = SPI_CPLD_FOR_SPI_FLASH;
    } else {
        mpi_cfg.s.csid = SPI_CPLD_FOR_CPLD;
    }

    tx_buf = xfer->tx_buf;
    rx_buf = xfer->rx_buf;
    /* tx buf length or rx buf length */
    len = xfer->len;
    count = 0;
    DEBUG_VERBOSE(KERN_INFO "tx_buf %p, rx_buf %p, len %d, mpi_cfg.u32 %08x (xfer->cs_change %d)\n",
        tx_buf, rx_buf, len, mpi_cfg.u32, xfer->cs_change);
    while (len > SPI_CPLD_MAX_BYTES) {
        ret = 0;
        if (tx_buf) {
            ret = spi_cpld_do_one_transfer(p, xfer, mpi_cfg.u32,
                tx_buf + count, rx_buf, SPI_CPLD_MAX_BYTES, false, last_xfer);
        } else if (rx_buf) {
            ret = spi_cpld_do_one_transfer(p, xfer, mpi_cfg.u32,
                tx_buf, rx_buf + count, SPI_CPLD_MAX_BYTES, false, last_xfer);
        }
        if (ret < 0) {
            return ret;
        }

        count += ret;
        if ((((count + SPI_CPLD_MAX_BYTES) / SPI_CPLD_MAX_BYTES) % SPI_CPLD_WATCHDOG_TIMES) == 0) {
            touch_nmi_watchdog();
        }

        len -= SPI_CPLD_MAX_BYTES;
    }

    ret = 0;
    if (tx_buf) {
        ret = spi_cpld_do_one_transfer(p, xfer, mpi_cfg.u32,
            tx_buf + count, rx_buf, len, true, last_xfer);
    } else if (rx_buf) {
        ret = spi_cpld_do_one_transfer(p, xfer, mpi_cfg.u32,
            tx_buf, rx_buf + count, len, true, last_xfer);
    }
    if (ret < 0) {
        return ret;
    }

    return xfer->len;
}

int spi_cpld_transfer(struct spi_device *spi,
                    struct spi_message *msg)
{
    struct spi_master *master = spi->master;
    struct spi_cpld *p = spi_master_get_devdata(master);
    unsigned int total_len = 0;
    int status = 0;
    struct spi_transfer *xfer;
    int i = 0;

    list_for_each_entry(xfer, &msg->transfers, transfer_list) {
        bool last_xfer = list_is_last(&xfer->transfer_list,
                          &msg->transfers);
        int r = spi_cpld_do_transfer(p, msg, xfer, spi->chip_select, last_xfer);
        i++;
        pr_debug("i:%d, len:%d, len1:%d\n", i, r, xfer->len);
        if (r < 0) {
            status = r;
            goto err;
        }
        total_len += r;
        pr_debug("total_len:%u\n", total_len);
    }

err:
    msg->status = status;
    msg->actual_length = total_len;

    if (msg->complete) {
        msg->complete(msg->context);
    }

    return status;
}

static size_t cpld_spi_max_transfer_size(struct spi_device *spi)
{
    return SPI_CPLD_MAX_TRANSFER_SIZE;
}


static int spi_cpld_probe(struct platform_device *pdev)
{
    struct resource *res_mem;
    void __iomem *reg_base;
    struct spi_master *master;
    struct spi_cpld *p;
    int err = -ENOENT;
    union spi_cpld_cfg_s mpi_cfg;

    master = spi_alloc_master(&pdev->dev, sizeof(struct spi_cpld));
    if (!master) {
        return -ENOMEM;
    }

    p = spi_master_get_devdata(master);
    platform_set_drvdata(pdev, master);
    res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    reg_base = devm_ioremap_resource(&pdev->dev, res_mem);
    if (IS_ERR(reg_base)) {
        dev_err(&pdev->dev, "IS_ERR(reg_base\n");
        err = PTR_ERR(reg_base);
        goto fail;
    }

    DEBUG_VERBOSE(KERN_INFO "Base addr: %llx\n", (u64)reg_base);

    mpi_cfg.u32 = 0;
    mpi_cfg.s.lsbfirst = 1;
    p->register_base = reg_base;
    p->sys_freq = CPLD_BASE_CLOCK_HZ;

    p->regs.config = SPI_CPLD_BASE_ADDR + 0;
    p->regs.data = SPI_CPLD_BASE_ADDR + 0x4;

    master->num_chipselect = 4;
    master->mode_bits = SPI_CPHA |
                SPI_CPOL |
                SPI_CS_HIGH |
                SPI_LSB_FIRST |
                SPI_3WIRE;

    master->transfer = spi_cpld_transfer;

    master->max_transfer_size = cpld_spi_max_transfer_size;

    master->bits_per_word_mask = BIT(8 - 1);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,12,0)
    master->max_speed_hz = SPI_CPLD_MAX_CLOCK_HZ;
#endif

    master->dev.of_node = pdev->dev.of_node;
    err = spi_register_master(master);
    if (err) {
        dev_err(&pdev->dev, "register master failed: %d\n", err);
        goto fail;
    }

    dev_info(&pdev->dev, "CPLD SPI bus driver\n");

    return 0;
fail:
    spi_master_put(master);
    return err;
}

static int spi_cpld_remove(struct platform_device *pdev)
{
    struct spi_master *master = platform_get_drvdata(pdev);
    struct spi_cpld *p = spi_master_get_devdata(master);

    lpc_write(0, p->register_base + SPI_CPLD_CFG(p));
    spi_unregister_master(master);

    return 0;
}

static const struct of_device_id spi_cpld_match[] = {
    { .compatible = "spi-cpld", },
    {},
};
MODULE_DEVICE_TABLE(of, spi_cpld_match);

static struct platform_driver spi_cpld_driver = {
    .driver = {
        .name           = "spi-cpld",
        .of_match_table = spi_cpld_match,
    },
    .probe              = spi_cpld_probe,
    .remove             = spi_cpld_remove,
};

static int __init spi_cpld_init(void)
{
    int rv;

    rv = platform_driver_register(&spi_cpld_driver);
    return rv;
}

static void __exit spi_cpld_exit(void)
{
    platform_driver_unregister(&spi_cpld_driver);
}

module_init(spi_cpld_init);
module_exit(spi_cpld_exit);

MODULE_LICENSE("Dual BSD/GPL");
