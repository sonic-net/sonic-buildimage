#ifndef __FGPA_H__
#define __FGPA_H__

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioport.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/acpi.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/gpio/driver.h>
#include <linux/mutex.h>

#define FPGA_I2C_BUSNUM 5
#define FPGA_I2C_MUX_DIS 0
#define FPGA_I2C_MUX_EN 1

#define FPGA_JTAG_MUX_REG 0x100
#define FPGA_JTAG_CTRL0_REG 0x104
#define FPGA_JTAG_CTRL1_REG 0x108
#define FPGA_JTAG_CTRL2_REG 0x10C

typedef struct fpga_i2c
{
	char name[50];
	int bus;
	int offset;
	int mux_en;
	int mux_addr;
	int num_ch;
} fpga_i2c_s;

typedef struct fpga_gpio
{
	int num;
	char name[50];
	int reg;
	int bit;
} fpga_gpio_s;

struct i2c_bus_dev
{
	struct i2c_adapter adapter;
	int offset;
	int busnum;
	int mux_ch;
	int mux_en;
	void *__iomem bar;
};

struct fpga_gpio_chip
{
	struct gpio_chip gpio_chip;
	struct mutex lock;
	void *__iomem bar;
	u32 registers;
	/*
	 * Since the registers are chained, every byte sent will make
	 * the previous byte shift to the next register in the
	 * chain. Thus, the first byte sent will end up in the last
	 * register at the end of the transfer. So, to have a logical
	 * numbering, store the bytes in reverse order.
	 */
	u8 buffer[0];
};

struct fpga_dev
{
	char name[64];
	struct pci_dev *dev;
	struct i2c_bus_dev *i2c;
	struct fpga_gpio_chip *gpio;
	int pci_base;
	int pci_size;
	struct semaphore sem;
	char *buffer;
};

static const size_t BUF_SIZE = PAGE_SIZE;

#endif /* __FGPA_H__ */