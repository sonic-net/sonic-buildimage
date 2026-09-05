/*
 * Copyright (c) 2008-2009 Rodolfo Giometti <giometti@linux.it>
 * Copyright (c) 2008-2009 Eurotech S.p.A. <info@eurotech.it>
 * Copyright (c) 2019  <sonic_rd@ruijie.com.cn>
 *
 * I2C multiplexer
 *
 * This module supports the PCA954x series of I2C multiplexer/switch chips
 * made by Philips Semiconductors.
 * This includes the:
 *	 PCA9540, PCA9542, PCA9543, PCA9544, PCA9545, PCA9546, PCA9547
 *	 and PCA9548.
 *
 * These chips are all controlled via the I2C bus itself, and all have a
 * single 8-bit register. The upstream "parent" bus fans out to two,
 * four, or eight downstream busses or channels; which of these
 * are selected is determined by the chip type and register contents. A
 * mux can select only one sub-bus at a time; a switch can select any
 * combination simultaneously.
 *
 * Based on:
 *	pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *	pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *	i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *	pca9540.c from Jean Delvare <jdelvare@suse.de>.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/platform_data/pca954x.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c-smbus.h>
#include <linux/fs.h>


#define PCA954X_MAX_NCHANS 8

#define PCA954X_IRQ_OFFSET 4

extern int pca9641_setmuxflag(int nr, int flag);

int force_create_bus = 0;
module_param(force_create_bus, int, S_IRUGO | S_IWUSR);

enum pca_type {
	pca_9540,
	pca_9542,
	pca_9543,
	pca_9544,
	pca_9545,
	pca_9546,
	pca_9547,
	pca_9548,
};

struct chip_desc {
	u8 nchans;
	u8 enable;	/* used for muxes only */
	u8 has_irq;
	enum muxtype {
		pca954x_ismux = 0,
		pca954x_isswi
	} muxtype;
};




struct pca954x {
	const struct chip_desc *chip;

	u8 last_chan;		/* last register value */
	u8 deselect;
	struct i2c_client *client;

	struct irq_domain *irq;
	unsigned int irq_mask;
	raw_spinlock_t lock;
};

/* Provide specs for the PCA954x types we know about */
static const struct chip_desc chips[] = {
	[pca_9540] = {
		.nchans = 2,
		.enable = 0x4,
		.muxtype = pca954x_ismux,
	},
	[pca_9542] = {
		.nchans = 2,
		.enable = 0x4,
		.has_irq = 1,
		.muxtype = pca954x_ismux,
	},
	[pca_9543] = {
		.nchans = 2,
		.has_irq = 1,
		.muxtype = pca954x_isswi,
	},
	[pca_9544] = {
		.nchans = 4,
		.enable = 0x4,
		.has_irq = 1,
		.muxtype = pca954x_ismux,
	},
	[pca_9545] = {
		.nchans = 4,
		.has_irq = 1,
		.muxtype = pca954x_isswi,
	},
	[pca_9546] = {
		.nchans = 4,
		.muxtype = pca954x_isswi,
	},
	[pca_9547] = {
		.nchans = 8,
		.enable = 0x8,
		.muxtype = pca954x_ismux,
	},
	[pca_9548] = {
		.nchans = 8,
		.muxtype = pca954x_isswi,
	},
};

static const struct i2c_device_id pca954x_id[] = {
	{ "pca9540", pca_9540 },
	{ "pca9542", pca_9542 },
	{ "pca9543", pca_9543 },
	{ "pca9544", pca_9544 },
	{ "pca9545", pca_9545 },
	{ "pca9546", pca_9546 },
	{ "pca9547", pca_9547 },
	{ "pca9548", pca_9548 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pca954x_id);

#ifdef CONFIG_OF
static const struct of_device_id pca954x_of_match[] = {
	{ .compatible = "nxp,pca9540", .data = &chips[pca_9540] },
	{ .compatible = "nxp,pca9542", .data = &chips[pca_9542] },
	{ .compatible = "nxp,pca9543", .data = &chips[pca_9543] },
	{ .compatible = "nxp,pca9544", .data = &chips[pca_9544] },
	{ .compatible = "nxp,pca9545", .data = &chips[pca_9545] },
	{ .compatible = "nxp,pca9546", .data = &chips[pca_9546] },
	{ .compatible = "nxp,pca9547", .data = &chips[pca_9547] },
	{ .compatible = "nxp,pca9548", .data = &chips[pca_9548] },
	{}
};
MODULE_DEVICE_TABLE(of, pca954x_of_match);
#endif

/* Write to mux register. Don't use i2c_transfer()/i2c_smbus_xfer()
   for this as they will try to lock adapter a second time */
static int pca954x_reg_write(struct i2c_adapter *adap,
			     struct i2c_client *client, u8 val)
{
	int ret = -ENODEV;

	if (adap->algo->master_xfer) {
		struct i2c_msg msg;
		char buf[1];

		msg.addr = client->addr;
		msg.flags = 0;
		msg.len = 1;
		buf[0] = val;
		msg.buf = buf;
		ret = __i2c_transfer(adap, &msg, 1);

		if (ret >= 0 && ret != 1)
			ret = -EREMOTEIO;
	} else {
		union i2c_smbus_data data;
		ret = adap->algo->smbus_xfer(adap, client->addr,
					     client->flags,
					     I2C_SMBUS_WRITE,
					     val, I2C_SMBUS_BYTE, &data);
	}

	return ret;
}

 static int pca954x_setmuxflag(struct i2c_client *client, int flag)
 {	 
	 struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	 pca9641_setmuxflag(adap->nr, flag);
	 return 0;
 }

static int pca954x_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
	struct pca954x *data = i2c_mux_priv(muxc);
	struct i2c_client *client = data->client;
	const struct chip_desc *chip = data->chip;
	u8 regval;
	int ret = 0;

	/* we make switches look like muxes, not sure how to be smarter */
	if (chip->muxtype == pca954x_ismux)
		regval = chan | chip->enable;
	else
		regval = 1 << chan;

	/* Only select the channel if its different from the last channel */
	if (data->last_chan != regval) {
		pca954x_setmuxflag(client, 0);
		ret = pca954x_reg_write(muxc->parent, client, regval);
		data->last_chan = ret < 0 ? 0 : regval;
	}

	return ret;
}


typedef void  (*pca954x_hw_do_reset_func_t)(int busid, int addr);
pca954x_hw_do_reset_func_t g_notify_to_do_reset = NULL;

void pca954x_hw_do_reset_func_register(void* func)
{
    if (func == NULL) {
        return ;
    }
    g_notify_to_do_reset = func;
    return;
}
EXPORT_SYMBOL(pca954x_hw_do_reset_func_register);

static int pca954x_hw_do_reset(int busid, int addr)
{
    if (g_notify_to_do_reset != NULL) {
        (*g_notify_to_do_reset)(busid, addr);
        return 0;
    }

    return 0;
}
/***************************************9548 reset*****************************************/
#define DEV_TYPE                0x4040  /* BT2575 */
#define PCA9548_MAX_CPLD_NUM    (32)    /* PCA9548 max number */
#define PCA9548_MAX_CPLD_LAYER  (8)     /* PCA9548 max layer */
#define DFD_PID_BUF_LEN         (32)
#define DFD_PRODUCT_ID_LENGTH   (8)
#define CPLD_PCA9548_RESET      0x023500b0 /* bus:2, addr:0x35, offset:0xb0 */
#define B6510_32CQ_CPLD_PCA9548_RESET      0x060d0060 /* bus:6, addr:0x0d, offset:0x60 */

#define DFD_PUB_CARDTYPE_FILE   "/sys/module/ruijie_common/parameters/dfd_my_type"
#define DFD_MAX_PRODUCT_NUM     (32)


#define I2C_RETRY_TIMES         5
#define I2C_RETRY_WAIT_TIMES    10      /*delay 10ms*/

#define PCA9548_I2C_GET_BUS(addr)       (((addr) >> 24) & 0x00ff)
#define PCA9548_I2C_GET_CLIENT(addr)    (((addr) >> 16) & 0x00ff)
#define PCA9548_I2C_GET_OFFSET(addr)    (addr & 0xffff)

typedef enum pca9548_reset_type {
    PCA9548_RESET_FUNC = 0,
    PCA9548_RESET_GPIO = 1,
} pca9548_reset_type_t;

typedef void  (*pca954x_hw_do_reset_func_t_new)(int io_port, u8 value);
typedef u8  (*pca954x_get_umask_func_t)(int io_port);

void pca954x_hw_do_reset_by_i2c(int addr, u8 value);
u8 pca954x_get_umask_by_i2c(int addr);
void pca954x_hw_do_reset_by_lpc(int io_port, u8 value);
u8 pca954x_get_umask_by_lpc(int io_port);


typedef struct func_attr_s {
    int cfg_offset[PCA9548_MAX_CPLD_LAYER];
    int umask[PCA9548_MAX_CPLD_LAYER];
    pca954x_hw_do_reset_func_t_new reset_func;        /* 9548 reset function */
    pca954x_get_umask_func_t get_umask_func;        /* get reset mask */
} func_attr_t;

typedef struct gpio_attr_s {
    int gpio;
    int gpio_init;
    u8 reset_on;
    u8 reset_off;
} gpio_attr_t;

typedef struct pca9548_cfg_info_s {
    int pca9548_reset_type;
    int pca9548_bus;
    int pca9548_addr;
    int rst_delay_b; /* delay time before reset(us) */
    int rst_delay; /* reset time(us) */
    int rst_delay_a; /* delay time after reset(us) */
    union {
        func_attr_t func_attr;
        gpio_attr_t gpio_attr;
    } attr;
} pca9548_cfg_info_t;

typedef struct fpga_pcie_card_info_s {
    int dev_type[DFD_MAX_PRODUCT_NUM];      /* dev type */
    pca9548_cfg_info_t pca9548_cfg_info[PCA9548_MAX_CPLD_NUM];
} pca9548_card_info_t;

static pca9548_card_info_t g_pca9548_card_info[] = {
    {
        .dev_type          = {0x4040,0x4061,0x4071}, /*B6510,BT2575,TCS81*/
        .pca9548_cfg_info  = {
            /* psu fan */
            {
                .pca9548_reset_type = PCA9548_RESET_GPIO,
                .pca9548_bus        = 2,
                .pca9548_addr       = 0x70,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .gpio_attr.gpio           = 7,
                    .gpio_attr.gpio_init      = 0,
                    .gpio_attr.reset_on       = 1,
                    .gpio_attr.reset_off      = 0,
                },
            },
            /* sff1 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x70,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(0), -1},
                },
            },
            /* sff2 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x71,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(1), -1},
                },
            },
            /* sff3 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x72,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(2), -1},
                },
            },
            /* sff4 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x73,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(3), -1},
                },
            },
            /* sff5 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x74,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(4), -1},
                },
            },
            /* sff6 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x75,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(5), -1},
                },
            },
            /* sff7 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x76,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(6), -1},
                },
            },
        },
    },
    {
        .dev_type          = {0x4041},   /*B6520*/
        .pca9548_cfg_info  = {
            /* psu fan */
            {
                .pca9548_reset_type = PCA9548_RESET_GPIO,
                .pca9548_bus        = 2,
                .pca9548_addr       = 0x70,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .gpio_attr.gpio           = 7,
                    .gpio_attr.gpio_init      = 0,
                    .gpio_attr.reset_on       = 1,
                    .gpio_attr.reset_off      = 0,
                },
            },
            /* sff1 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x70,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(0), -1},
                },
            },
            /* sff2 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x71,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(1), -1},
                },
            },
            /* sff3 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x72,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(2), -1},
                },
            },
            /* sff4 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x73,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(3), -1},
                },
            },
            /* sff5 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x74,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(4), -1},
                },
            },
            /* sff6 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x75,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(5), -1},
                },
            },
            /* sff7 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x76,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(6), -1},
                },
            },
            /* sff8 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 1,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_i2c,
                    .func_attr.get_umask_func = pca954x_get_umask_by_i2c,
                    .func_attr.cfg_offset     = {CPLD_PCA9548_RESET, -1},
                    .func_attr.umask          = {BIT(7), -1},
                },
            },
        },
    },
    {
        .dev_type          = {0x4044,0x4072,0x4048},    /*B6920,TCS83,BS100R0*/
        .pca9548_cfg_info  = {
            /* һ��9548 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 2,
                .pca9548_addr       = 0x76,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0x936, -1},
                    .func_attr.umask          = {BIT(4), -1},
                },
            },
            /* �װ� */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 8,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0x917, -1},
                    .func_attr.umask          = {BIT(4), -1},
                },
            },
            /* �ӿ�1 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 9,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0x917, -1},
                    .func_attr.umask          = {BIT(0), -1},
                },
            },
            /* �ӿ�2 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 12,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0x917, -1},
                    .func_attr.umask          = {BIT(1), -1},
                },
            },
            /* �ӿ�3 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 11,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0x917, -1},
                    .func_attr.umask          = {BIT(2), -1},
                },
            },
            /* �ӿ�4 */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 7,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0x917, -1},
                    .func_attr.umask          = {BIT(3), -1},
                },
            },
            /* ����A */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 14,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb10, -1},
                    .func_attr.umask          = {BIT(5), -1},
                },
            },
            /* ����B */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 13,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb10, -1},
                    .func_attr.umask          = {BIT(7), -1},
                },
            },
            /* �ӿ�1 ��ģ�� */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 3,
                .pca9548_addr       = 0x70,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(0), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 3,
                .pca9548_addr       = 0x71,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(0), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 3,
                .pca9548_addr       = 0x72,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(0), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 3,
                .pca9548_addr       = 0x73,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(0), -1},
                },
            },
            /* �ӿ�2 ��ģ�� */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 4,
                .pca9548_addr       = 0x70,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(1), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 4,
                .pca9548_addr       = 0x71,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(1), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 4,
                .pca9548_addr       = 0x72,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(1), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 4,
                .pca9548_addr       = 0x73,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(1), -1},
                },
            },
            /* �ӿ�3 ��ģ�� */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 5,
                .pca9548_addr       = 0x70,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(2), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 5,
                .pca9548_addr       = 0x71,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(2), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 5,
                .pca9548_addr       = 0x72,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(2), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 5,
                .pca9548_addr       = 0x73,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(2), -1},
                },
            },
            /* �ӿ�4 ��ģ�� */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 6,
                .pca9548_addr       = 0x70,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(3), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 6,
                .pca9548_addr       = 0x71,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(3), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 6,
                .pca9548_addr       = 0x72,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(3), -1},
                },
            },
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 6,
                .pca9548_addr       = 0x73,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0xb17, -1},
                    .func_attr.umask          = {BIT(3), -1},
                },
            },
        },
    },
    {
        .dev_type          = {0x4058,0x4073}, /* B6510-32CQ, TCS82 */
        .pca9548_cfg_info  = {
            /* psu */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 4,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0x960, -1},
                    .func_attr.umask          = {BIT(0), -1},
                },
            },
            /* fan */
            {
                .pca9548_reset_type = PCA9548_RESET_FUNC,
                .pca9548_bus        = 2,
                .pca9548_addr       = 0x77,
                .rst_delay_b        = 0,
                .rst_delay          = 1000,
                .rst_delay_a        = 1000,
                .attr = {
                    .func_attr.reset_func     = pca954x_hw_do_reset_by_lpc,
                    .func_attr.get_umask_func = pca954x_get_umask_by_lpc,
                    .func_attr.cfg_offset     = {0x960, -1},
                    .func_attr.umask          = {BIT(1), -1},
                },
            },
        },
    },
};
int g_pca954x_debug = 0;
module_param(g_pca954x_debug, int, S_IRUGO | S_IWUSR);

#define PCA954X_DEBUG(fmt, args...) do {                                        \
    if (g_pca954x_debug) { \
        printk(KERN_ERR "[PCA95x][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

/* x86�豸��ȡ�忨���ͷ��� */
static int dfd_get_my_dev_type_by_file(void)
{
    struct file *fp;
    /* mm_segment_t fs;*/
    loff_t pos;
    static int card_type;
    char buf[DFD_PID_BUF_LEN];

    if (card_type != 0) {
        return card_type;
    }

    fp= filp_open(DFD_PUB_CARDTYPE_FILE, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        PCA954X_DEBUG("open file fail!\r\n");
        return -1;
    }
    /* fs = get_fs(); */
    /* set_fs(KERNEL_DS); */
    memset(buf, 0, DFD_PID_BUF_LEN);
    pos = 0;
    kernel_read(fp, pos, buf, DFD_PRODUCT_ID_LENGTH + 1 );
    if (pos < 0) {
        PCA954X_DEBUG("read file fail!\r\n");
        goto exit;
    }

    card_type = simple_strtoul(buf, NULL, 10); 
    PCA954X_DEBUG("card_type 0x%x.\n", card_type);

exit:
    /* set_fs(fs); */
    filp_close(fp, NULL);
    return card_type;
}

static int drv_get_my_dev_type(void)
{
    static int type = -1;

    if (type > 0) {
        return type;
    }
    type = dfd_get_my_dev_type_by_file();
    PCA954X_DEBUG("ko board type %d\r\n", type);

    return type;
}

pca9548_card_info_t* pca9548_get_card_info(int dev_type)
{
    int i, j;
    int size;

    size = ARRAY_SIZE(g_pca9548_card_info);

    PCA954X_DEBUG("Enter dev_type 0x%x size %d.\n", dev_type, size);
    for (i = 0; i < size; i++) {
        for(j = 0; j < DFD_MAX_PRODUCT_NUM; j++) {
            if (g_pca9548_card_info[i].dev_type[j] == dev_type) {
                PCA954X_DEBUG("match dev_type 0x%x.\n", dev_type);
                return &g_pca9548_card_info[i];
            }
        }
    }

    PCA954X_DEBUG("dismatch dev_type 0x%x.\n", dev_type);
    return NULL;
}

pca9548_cfg_info_t* get_pca9548_cfg_info(int bus, int addr)
{
    int dev_type;
    pca9548_card_info_t *info;
    pca9548_cfg_info_t *pca9548_cfg_info;
    int i;
    int size;

    dev_type = drv_get_my_dev_type();
    if (dev_type < 0) {
        PCA954X_DEBUG("drv_get_my_dev_type failed ret %d.\n", dev_type);
        return NULL;
    }

    info = pca9548_get_card_info(dev_type);
    if (info == NULL) {
        PCA954X_DEBUG("fpga_pcie_get_card_info dev_type %d failed.\n", dev_type);
        return NULL;
    }

    size = PCA9548_MAX_CPLD_NUM;
    for (i = 0; i < size; i++) {
        pca9548_cfg_info = &(info->pca9548_cfg_info[i]);
        if ((pca9548_cfg_info->pca9548_bus == bus) && (pca9548_cfg_info->pca9548_addr == addr)) {
            PCA954X_DEBUG("match dev_type 0x%x bus %d addr 0x%x.\n", dev_type, bus, addr);
            return pca9548_cfg_info;
        }
    }

    PCA954X_DEBUG("dismatch dev_type 0x%x bus %d addr 0x%x.\n", dev_type, bus, addr);
    return NULL;
}

static void pca9548_gpio_init(gpio_attr_t *gpio_attr)
{
    if (gpio_attr->gpio_init == 0) {
        PCA954X_DEBUG("gpio%d init.\n", gpio_attr->gpio);
        gpio_request(gpio_attr->gpio, "pca9548_reset");
        gpio_direction_output(gpio_attr->gpio, gpio_attr->reset_off);
        gpio_attr->gpio_init = 1;
    }
}

static void pca9548_gpio_free(gpio_attr_t *gpio_attr)
{
    if (gpio_attr == NULL) {
        PCA954X_DEBUG("pca9548_gpio_free,params error\n");
        return ;
    }
    if (gpio_attr->gpio_init == 1) {
        PCA954X_DEBUG("gpio%d release.\n", gpio_attr->gpio);
        gpio_free(gpio_attr->gpio);
        gpio_attr->gpio_init = 0;
    }
}

static int pca954x_do_gpio_reset(pca9548_cfg_info_t *cfg_info, struct i2c_adapter *adap,
            struct i2c_client *client, u32 chan)
{
    struct pca954x *data = i2c_get_clientdata(client);
    int ret = -1;
    gpio_attr_t *tmp_gpio_attr;
    int timeout;
    int val;
    struct i2c_adapter *adapter;
    int adapter_timeout;

    if (cfg_info == NULL) {
        PCA954X_DEBUG("pca9548 cfg info is null.\n");
        return ret;
    }

    if (cfg_info->pca9548_reset_type == PCA9548_RESET_GPIO) {
        tmp_gpio_attr = &(cfg_info->attr.gpio_attr);
        timeout = cfg_info->rst_delay_a;

        pca9548_gpio_init(tmp_gpio_attr);
        udelay(cfg_info->rst_delay_b);
        /* reset on */
        PCA954X_DEBUG("set gpio%d %d.\n", tmp_gpio_attr->gpio, tmp_gpio_attr->reset_on);
        __gpio_set_value(tmp_gpio_attr->gpio, tmp_gpio_attr->reset_on);
        udelay(cfg_info->rst_delay);
        /* reset off */
        PCA954X_DEBUG("set gpio%d %d.\n", tmp_gpio_attr->gpio, tmp_gpio_attr->reset_off);
        __gpio_set_value(tmp_gpio_attr->gpio, tmp_gpio_attr->reset_off);

        while (timeout > 0) {
            udelay(1);
            val = __gpio_get_value(tmp_gpio_attr->gpio);
            if (val == tmp_gpio_attr->reset_off) {
                adapter = adap;
                /* get bus info */
                while(i2c_parent_is_i2c_adapter(adapter)){
                    adapter = to_i2c_adapter(adapter->dev.parent);
                }

                adapter_timeout = adapter->timeout;
                adapter->timeout = msecs_to_jiffies(50);
                pca954x_reg_write(adap, client, data->last_chan);
                adapter->timeout = adapter_timeout;
                ret = 0;
                PCA954X_DEBUG("pca954x_do_gpio_reset success.\n");
                break;
            }

            if (timeout >= 1000 && (timeout % 1000 == 0)) {
                /* 1MS schedule*/
                schedule();
            }
            timeout--;
        }

        if (ret) {
            PCA954X_DEBUG("pca954x_do_gpio_reset failed.\n");
        }
        pca9548_gpio_free(&(cfg_info->attr.gpio_attr));
    } else {
        PCA954X_DEBUG("pca9548_reset_type invalid, pca954x_do_gpio_reset failed.\n");
    }
    
    return ret;
}

static int pca954x_do_func_reset(pca9548_cfg_info_t *cfg_info, struct i2c_adapter *adap,
            struct i2c_client *client, u32 chan)
{
    struct pca954x *data = i2c_get_clientdata(client);
    int ret = -1;
    func_attr_t *tmp_func_attr;
    int timeout;
    int val;
    struct i2c_adapter *adapter;
    int adapter_timeout;
    int i;
    u8 old_value;

    if (cfg_info == NULL) {
        PCA954X_DEBUG("pca9548 cfg info is null.\n");
        return ret;
    }

    if (cfg_info->pca9548_reset_type == PCA9548_RESET_FUNC) {
        tmp_func_attr = &(cfg_info->attr.func_attr);
        timeout = cfg_info->rst_delay_a;

        if ((tmp_func_attr->reset_func == NULL) || (tmp_func_attr->get_umask_func == NULL)) {
            PCA954X_DEBUG("pca954x hw do reset func or get umask func is null.\n");
            return ret;
        }

        for(i = 0; (i < PCA9548_MAX_CPLD_LAYER) && (tmp_func_attr->cfg_offset[i] != -1)
                    && (tmp_func_attr->umask[i] != -1); i++) {
            old_value = (*tmp_func_attr->get_umask_func)(tmp_func_attr->cfg_offset[i]);
            PCA954X_DEBUG("cfg info: offset:0x%x umask:0x%x, old_value:0x%x\n",
                        tmp_func_attr->cfg_offset[i], tmp_func_attr->umask[i],old_value);
            (*tmp_func_attr->reset_func)(tmp_func_attr->cfg_offset[i], old_value & ~tmp_func_attr->umask[i]);
            udelay(cfg_info->rst_delay);
            (*tmp_func_attr->reset_func)(tmp_func_attr->cfg_offset[i], old_value | tmp_func_attr->umask[i]);
        }

        while (timeout > 0) {
            udelay(1);
            val = (*tmp_func_attr->get_umask_func)(tmp_func_attr->cfg_offset[i - 1]);
            val &= (tmp_func_attr->umask[i - 1]);
            if (val == tmp_func_attr->umask[i - 1]) {
                adapter = adap;
                /* get bus info */
                while(i2c_parent_is_i2c_adapter(adapter)){
                    adapter = to_i2c_adapter(adapter->dev.parent);
                }

                adapter_timeout = adapter->timeout;
                adapter->timeout = msecs_to_jiffies(50);
                pca954x_reg_write(adap, client, data->last_chan);
                adapter->timeout = adapter_timeout;
                ret = 0;
                PCA954X_DEBUG("pca954x_do_func_reset success.\n");
                break;
            }

            if (timeout >= 1000 && (timeout % 1000 == 0)) {
                /* 1MS schedule*/
                schedule();
            }
            timeout--;
        }

        if (ret) {
            PCA954X_DEBUG("pca954x_do_func_reset failed.\n");
        }
    } else {
        PCA954X_DEBUG("pca9548_reset_type invalid, pca954x_do_func_reset failed.\n");
    }

    return ret;
}

static int pca9548_reset_ctrl(pca9548_cfg_info_t *cfg_info, struct i2c_adapter *adap,
            struct i2c_client *client, u32 chan)
{
    int ret = -1;

    if (cfg_info == NULL) {
        PCA954X_DEBUG("pca9548 cfg info is null.\n");
        return ret;
    }

    if (cfg_info->pca9548_reset_type == PCA9548_RESET_FUNC) {
        ret = pca954x_do_func_reset(cfg_info, adap, client, chan);
    } else if (cfg_info->pca9548_reset_type == PCA9548_RESET_GPIO) {
        ret = pca954x_do_gpio_reset(cfg_info, adap, client, chan);
    }

    if (ret < 0) {
        PCA954X_DEBUG("pca9548_reset_ctrl failed.\n");
    }
    return ret;
}

static int pca954x_reset_i2c_read(uint32_t bus, uint32_t addr, uint32_t offset_addr,
            unsigned char *buf, uint32_t size)
{
    struct file *fp;
    /* mm_segment_t fs; */
    struct i2c_client client;
    char i2c_path[32];
    int i ,j ;
    int rv;

    rv = 0;
    memset(i2c_path, 0, 32);
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", bus);
    fp = filp_open(i2c_path, O_RDWR, S_IRUSR | S_IWUSR);
    if (IS_ERR(fp)) {
        PCA954X_DEBUG("i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = addr;
    /* fs = get_fs(); */
    /* set_fs(KERNEL_DS); */
    for (j = 0 ;j < size ;j++){
        for (i = 0; i < I2C_RETRY_TIMES; i++) {
            rv = i2c_smbus_read_byte_data(&client, (offset_addr + j));
            if (rv  < 0) {
                PCA954X_DEBUG("i2c read failed, try again.\n");
                msleep(I2C_RETRY_WAIT_TIMES);
                if (i >= (I2C_RETRY_TIMES - 1)) {
                    goto out;
                }
                continue;
          }
          *(buf + j) = (unsigned char)rv;
          break;
        }
    }
out:
    filp_close(fp, NULL);
    /* set_fs(fs); */
    return rv;
}

static int pca954x_reset_i2c_write(uint32_t bus, uint32_t dev_addr, uint32_t offset_addr,
            uint8_t write_buf)
{
    struct file *fp;
    /* mm_segment_t fs; */
    struct i2c_client client;
    char i2c_path[32];
    int i;
    int rv;

    rv = 0;
    memset(i2c_path, 0, 32);
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", bus);
    fp = filp_open(i2c_path, O_RDWR, S_IRUSR | S_IWUSR);
    if (IS_ERR(fp)) {
        PCA954X_DEBUG("i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = dev_addr;
    /* fs = get_fs(); */
    /* set_fs(KERNEL_DS); */
    for (i = 0; i < I2C_RETRY_TIMES; i++) {
        rv = i2c_smbus_write_byte_data(&client, offset_addr, write_buf);
        if (rv < 0) {
            PCA954X_DEBUG("i2c write failed, try again.\n");
            msleep(I2C_RETRY_WAIT_TIMES);
            if (i >= (I2C_RETRY_TIMES - 1)) {
                goto out;
            }
            continue;
        }
        break;
    }
out:
    filp_close(fp, NULL);
    /* set_fs(fs); */
    return rv;
}

int pca954x_reset_reg_i2c_read_byte(int addr, u8 *value)
{
    int bus;
    int client_addr;
    int offset;
    int ret;

    bus = PCA9548_I2C_GET_BUS(addr);
    client_addr = PCA9548_I2C_GET_CLIENT(addr);
    offset = PCA9548_I2C_GET_OFFSET(addr);

    ret = pca954x_reset_i2c_read(bus, client_addr, offset, value, 1);
    if (ret < 0) {
        PCA954X_DEBUG(" 0x%x read fail\r\n", addr);
        goto end;
    }
end:
    return ret;
}

int pca954x_reset_reg_i2c_write_byte(int addr, u8 value)
{
    int bus;
    int client_addr;
    int offset;
    int ret;

    bus = PCA9548_I2C_GET_BUS(addr);
    client_addr = PCA9548_I2C_GET_CLIENT(addr);
    offset = PCA9548_I2C_GET_OFFSET(addr);

    ret = pca954x_reset_i2c_write(bus, client_addr, offset, value);
    if (ret < 0) {
        PCA954X_DEBUG(" 0x%x write fail\r\n", addr);
        goto end;
    }
end:
    return ret;
}

void pca954x_hw_do_reset_by_i2c(int addr, u8 value)
{
    int ret;

    PCA954X_DEBUG("write i2c cpld[0x%x], value[%d]\n", addr, value);
    ret = pca954x_reset_reg_i2c_write_byte(addr, value);
    if (ret < 0) {
        PCA954X_DEBUG("write cpld pca9548 reset reg failed, ret = %d \n", ret);
    }
}

u8 pca954x_get_umask_by_i2c(int addr)
{
    u8 value = 0xFF;
    int ret;

    ret = pca954x_reset_reg_i2c_read_byte(addr, &value);
    PCA954X_DEBUG("read i2c cpld[0x%x], value[%d], ret = %d\n", addr, value, ret);

    return value;
}

void pca954x_hw_do_reset_by_lpc(int io_port, u8 value)
{
    PCA954X_DEBUG("write lpc offset[0x%x], value[%d]\n", (u16)io_port, value);
     outb(value, (u16)io_port);
}

u8 pca954x_get_umask_by_lpc(int io_port)
{
    u8 value;

    value = inb(io_port);
    PCA954X_DEBUG("read lpc offset[0x%x], value[%d]\n", (u16)io_port, value);

    return value;
}

int pca954x_hw_do_reset_new(struct i2c_adapter *adap,
            struct i2c_client *client, u32 chan)
{
    pca9548_cfg_info_t *cfg_info;
    int ret = -1;

    cfg_info = get_pca9548_cfg_info(adap->nr, client->addr);
    if (cfg_info == NULL && g_notify_to_do_reset == NULL) {
        PCA954X_DEBUG("fpga_do_pca954x_reset_func do nothing.\n");
        return ret;
    }
    if(cfg_info != NULL) {
        ret = pca9548_reset_ctrl(cfg_info, adap, client, chan);
    } else {
        ret = pca954x_hw_do_reset(adap->nr, client->addr);
    }
    if (ret < 0) {
        PCA954X_DEBUG("pca954x_hw_do_reset failed.\n");
    }
    return ret;
}
/******************************end 9548 reset***********************************/

static int pca954x_do_reset(struct i2c_adapter *adap,
            void *client, u32 chan)
{
    struct i2c_client *new_client;
    int ret = -1;

    PCA954X_DEBUG("do pca954x reset x86\n");
    new_client =(struct i2c_client *) client;
    ret = pca954x_hw_do_reset_new(adap, new_client, chan);
    if (ret < 0) {
        PCA954X_DEBUG("pca954x_do_reset failed.\n");
        return ret;
    }

    PCA954X_DEBUG("pca954x_do_reset success.\n");
    ret = 0;
    return ret;
}
static int pca954x_deselect_mux(struct i2c_mux_core *muxc, u32 chan)
{
	struct pca954x *data = i2c_mux_priv(muxc);
	struct i2c_client *client = data->client;
	int ret, rv;
	struct i2c_client * new_client;

	/* Deselect active channel */
	data->last_chan = 0;

	ret = pca954x_reg_write(muxc->parent, client, data->last_chan);
    if (ret < 0) {
        /* ʹ��i2c��ʽ�ر�ͨ��ʧ�ܣ�����ʹ��reset��ʽ */
        new_client =(struct i2c_client *) client;
        dev_warn(&new_client->dev, "pca954x close chn failed, do reset.\n");
        rv = pca954x_do_reset(client->adapter, client, chan);
        if (rv == 0) {
            ret = 0;
        }

    }
    /*9641Ȩ����Ҫ��9548��λ֮���ͷţ����¹�һ��ͨ��ȷ��9641Ȩ�������ͷ�*/
    pca954x_setmuxflag(client, 1);
    (void)pca954x_reg_write(muxc->parent, client, data->last_chan);
    
	return ret;

}

static irqreturn_t pca954x_irq_handler(int irq, void *dev_id)
{
	struct pca954x *data = dev_id;
	unsigned int child_irq;
	int ret, i, handled = 0;

	ret = i2c_smbus_read_byte(data->client);
	if (ret < 0)
		return IRQ_NONE;

	for (i = 0; i < data->chip->nchans; i++) {
		if (ret & BIT(PCA954X_IRQ_OFFSET + i)) {
			child_irq = irq_linear_revmap(data->irq, i);
			handle_nested_irq(child_irq);
			handled++;
		}
	}
	return handled ? IRQ_HANDLED : IRQ_NONE;
}

static void pca954x_irq_mask(struct irq_data *idata)
{
	struct pca954x *data = irq_data_get_irq_chip_data(idata);
	unsigned int pos = idata->hwirq;
	unsigned long flags;

	raw_spin_lock_irqsave(&data->lock, flags);

	data->irq_mask &= ~BIT(pos);
	if (!data->irq_mask)
		disable_irq(data->client->irq);

	raw_spin_unlock_irqrestore(&data->lock, flags);
}

static void pca954x_irq_unmask(struct irq_data *idata)
{
	struct pca954x *data = irq_data_get_irq_chip_data(idata);
	unsigned int pos = idata->hwirq;
	unsigned long flags;

	raw_spin_lock_irqsave(&data->lock, flags);

	if (!data->irq_mask)
		enable_irq(data->client->irq);
	data->irq_mask |= BIT(pos);

	raw_spin_unlock_irqrestore(&data->lock, flags);
}

static int pca954x_irq_set_type(struct irq_data *idata, unsigned int type)
{
	if ((type & IRQ_TYPE_SENSE_MASK) != IRQ_TYPE_LEVEL_LOW)
		return -EINVAL;
	return 0;
}

static struct irq_chip pca954x_irq_chip = {
	.name = "i2c-mux-pca954x",
	.irq_mask = pca954x_irq_mask,
	.irq_unmask = pca954x_irq_unmask,
	.irq_set_type = pca954x_irq_set_type,
};

static int pca954x_irq_setup(struct i2c_mux_core *muxc)
{
	struct pca954x *data = i2c_mux_priv(muxc);
	struct i2c_client *client = data->client;
	int c, err, irq;

	if (!data->chip->has_irq || client->irq <= 0)
		return 0;

	raw_spin_lock_init(&data->lock);

	data->irq = irq_domain_add_linear(client->dev.of_node,
					  data->chip->nchans,
					  &irq_domain_simple_ops, data);
	if (!data->irq)
		return -ENODEV;

	for (c = 0; c < data->chip->nchans; c++) {
		irq = irq_create_mapping(data->irq, c);
		irq_set_chip_data(irq, data);
		irq_set_chip_and_handler(irq, &pca954x_irq_chip,
			handle_simple_irq);
	}

	err = devm_request_threaded_irq(&client->dev, data->client->irq, NULL,
					pca954x_irq_handler,
					IRQF_ONESHOT | IRQF_SHARED,
					"pca954x", data);
	if (err)
		goto err_req_irq;

	disable_irq(data->client->irq);

	return 0;
err_req_irq:
	for (c = 0; c < data->chip->nchans; c++) {
		irq = irq_find_mapping(data->irq, c);
		irq_dispose_mapping(irq);
	}
	irq_domain_remove(data->irq);

	return err;
}

/*
 * I2C init/probing/exit functions
 */
static int pca954x_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	struct pca954x_platform_data *pdata = dev_get_platdata(&client->dev);
	struct device_node *of_node = client->dev.of_node;
	bool idle_disconnect_dt;
	struct gpio_desc *gpio;
	int num, force, class;
	struct i2c_mux_core *muxc;
	struct pca954x *data;
	const struct of_device_id *match;
	int ret;


	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
		return -ENODEV;

	muxc = i2c_mux_alloc(adap, &client->dev,
			     PCA954X_MAX_NCHANS, sizeof(*data), 0,
			     pca954x_select_chan, pca954x_deselect_mux);
	if (!muxc)
		return -ENOMEM;
	data = i2c_mux_priv(muxc);

	i2c_set_clientdata(client, muxc);
	data->client = client;

	/* Get the mux out of reset if a reset GPIO is specified. */
	gpio = devm_gpiod_get_optional(&client->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(gpio))
		return PTR_ERR(gpio);

	/* Write the mux register at addr to verify
	 * that the mux is in fact present. This also
	 * initializes the mux to disconnected state.
	 */
	if ((i2c_smbus_write_byte(client, 0) < 0) && (force_create_bus == 0)) {
		dev_warn(&client->dev, "probe failed\n");
		return -ENODEV;
	}

	match = of_match_device(of_match_ptr(pca954x_of_match), &client->dev);
	if (match)
		data->chip = of_device_get_match_data(&client->dev);
	else
		data->chip = &chips[id->driver_data];

	data->last_chan = 0;		   /* force the first selection */

	idle_disconnect_dt = of_node &&
		of_property_read_bool(of_node, "i2c-mux-idle-disconnect");

	ret = pca954x_irq_setup(muxc);
	if (ret)
		goto fail_del_adapters;

	/* Now create an adapter for each channel */
	for (num = 0; num < data->chip->nchans; num++) {
		bool idle_disconnect_pd = false;

		force = 0;			  /* dynamic adap number */
		class = 0;			  /* no class by default */
		if (pdata) {
			if (num < pdata->num_modes) {
				/* force static number */
				force = pdata->modes[num].adap_id;
				class = pdata->modes[num].class;
			} else
				/* discard unconfigured channels */
				break;
			idle_disconnect_pd = pdata->modes[num].deselect_on_exit;
		}
		data->deselect |= (idle_disconnect_pd ||
				   idle_disconnect_dt) << num;

		ret = i2c_mux_add_adapter(muxc, force, num, class);
		if (ret)
			goto fail_del_adapters;
	}

	dev_info(&client->dev,
		 "registered %d multiplexed busses for I2C %s %s\n",
		 num, data->chip->muxtype == pca954x_ismux
				? "mux" : "switch", client->name);

	return 0;

fail_del_adapters:
	i2c_mux_del_adapters(muxc);
	return ret;
}

static int pca954x_remove(struct i2c_client *client)
{
	struct i2c_mux_core *muxc = i2c_get_clientdata(client);
	struct pca954x *data = i2c_mux_priv(muxc);
	int c, irq;

	if (data->irq) {
		for (c = 0; c < data->chip->nchans; c++) {
			irq = irq_find_mapping(data->irq, c);
			irq_dispose_mapping(irq);
		}
		irq_domain_remove(data->irq);
	}

	i2c_mux_del_adapters(muxc);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pca954x_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_mux_core *muxc = i2c_get_clientdata(client);
	struct pca954x *data = i2c_mux_priv(muxc);

	data->last_chan = 0;
	return i2c_smbus_write_byte(client, 0);
}
#endif

static SIMPLE_DEV_PM_OPS(pca954x_pm, NULL, pca954x_resume);

static struct i2c_driver pca954x_driver = {
	.driver		= {
		.name	= "pca954x",
		.pm	= &pca954x_pm,
		.of_match_table = of_match_ptr(pca954x_of_match),
	},
	.probe		= pca954x_probe,
	.remove		= pca954x_remove,
	.id_table	= pca954x_id,
};

module_i2c_driver(pca954x_driver);

MODULE_AUTHOR("sonic_rd sonic_rd@ruijie.com.cn");
MODULE_DESCRIPTION("PCA954x I2C mux/switch driver");
MODULE_LICENSE("GPL");
