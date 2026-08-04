/*
 * A hwmon driver for the CIG cs6436-54P CPLD
 *
 * Copyright (C) 2018 Cambridge, Inc.
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
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/wait.h>
#include <linux/isa.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <asm/irq.h>
#include "i2c-algo-lpc.h"
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/i2c-mux.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/dma-mapping.h>

#ifndef CPLD_USER
# include <linux/ioctl.h>
#else
# include <sys/ioctl.h>
#endif

#include <linux/input.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/rtc.h>





/**********************************************   Start  ********************************************************/

/*
 * ISA bus.
 */

static void platform_isa_bus_release(struct device * dev)
{
    return ;
}


static struct device isa_bus = {
	.init_name	= "lpc-isa",
    .release = platform_isa_bus_release,
};

struct isa_dev {
	struct device dev;
	struct device *next;
	unsigned int id;
};

#define to_isa_dev(x) container_of((x), struct isa_dev, dev)

static int isa_bus_match(struct device *dev, struct device_driver *driver)
{
	struct isa_driver *isa_driver = to_isa_driver(driver);

	if (dev->platform_data == isa_driver) {
		if (!isa_driver->match ||
			isa_driver->match(dev, to_isa_dev(dev)->id))
			return 1;
		dev->platform_data = NULL;
	}
	return 0;
}

static int isa_bus_probe(struct device *dev)
{
	struct isa_driver *isa_driver = dev->platform_data;

	if (isa_driver->probe)
		return isa_driver->probe(dev, to_isa_dev(dev)->id);

	return 0;
}

static int isa_bus_remove(struct device *dev)
{
	struct isa_driver *isa_driver = dev->platform_data;

	if (isa_driver->remove)
		return isa_driver->remove(dev, to_isa_dev(dev)->id);

	return 0;
}

static void isa_bus_shutdown(struct device *dev)
{
	struct isa_driver *isa_driver = dev->platform_data;

	if (isa_driver->shutdown)
		isa_driver->shutdown(dev, to_isa_dev(dev)->id);
}

static int isa_bus_suspend(struct device *dev, pm_message_t state)
{
	struct isa_driver *isa_driver = dev->platform_data;

	if (isa_driver->suspend)
		return isa_driver->suspend(dev, to_isa_dev(dev)->id, state);

	return 0;
}

static int isa_bus_resume(struct device *dev)
{
	struct isa_driver *isa_driver = dev->platform_data;

	if (isa_driver->resume)
		return isa_driver->resume(dev, to_isa_dev(dev)->id);

	return 0;
}

static struct bus_type isa_bus_type = {
	.name		= "lpc-isa",
	.match		= isa_bus_match,
	.probe		= isa_bus_probe,
	.remove 	= isa_bus_remove,
	.shutdown	= isa_bus_shutdown,
	.suspend	= isa_bus_suspend,
	.resume 	= isa_bus_resume
};

static void isa_dev_release(struct device *dev)
{
	kfree(to_isa_dev(dev));
}

void lpc_unregister_driver(struct isa_driver *isa_driver)
{
	struct device *dev = isa_driver->devices;

	while (dev) {
		struct device *tmp = to_isa_dev(dev)->next;
		device_unregister(dev);
		dev = tmp;
	}
	driver_unregister(&isa_driver->driver);
}


int lpc_register_driver(struct isa_driver *isa_driver, unsigned int ndev)
{
	int error;
	unsigned int id;

	isa_driver->driver.bus	= &isa_bus_type;
	isa_driver->devices = NULL;

	error = driver_register(&isa_driver->driver);
	if (error)
		return error;

	for (id = 0; id < ndev; id++) {
		struct isa_dev *isa_dev;

		isa_dev = kzalloc(sizeof *isa_dev, GFP_KERNEL);
		if (!isa_dev) {
			error = -ENOMEM;
			break;
		}

		isa_dev->dev.parent = &isa_bus;
		isa_dev->dev.bus	= &isa_bus_type;

		dev_set_name(&isa_dev->dev, "%s.%u",
				 isa_driver->driver.name, id);
		isa_dev->dev.platform_data	= isa_driver;
		isa_dev->dev.release		= isa_dev_release;
		isa_dev->id 		= id;

		isa_dev->dev.coherent_dma_mask = DMA_BIT_MASK(24);
		isa_dev->dev.dma_mask = &isa_dev->dev.coherent_dma_mask;

		error = device_register(&isa_dev->dev);
		if (error) {
			put_device(&isa_dev->dev);
			break;
		}

		if (isa_dev->dev.platform_data) {
			isa_dev->next = isa_driver->devices;
			isa_driver->devices = &isa_dev->dev;
		} else
			device_unregister(&isa_dev->dev);
	}

	if (!error && !isa_driver->devices)
		error = -ENODEV;

	if (error)
		isa_unregister_driver(isa_driver);

	return error;
}


int lpc_bus_init(void)
{
	int error;

	error = bus_register(&isa_bus_type);
	if (!error) {
		error = device_register(&isa_bus);
		if (error)
			bus_unregister(&isa_bus_type);
	}
	return error;
}

void lpc_bus_exit(void)
{

   device_unregister(&isa_bus);

   bus_unregister(&isa_bus_type);
}


/**********************************************   End  ********************************************************/






/**********************************************   Start  ********************************************************/
/*
 * module parameters:
 */
static int i2c_debug = 0;
static struct mutex	lpc_lock;


#define DEB2(x) if (i2c_debug == 2) x
#define DEB3(x) if (i2c_debug == 3) x
    /* print several statistical values */
#define DEBPROTO(x) if (i2c_debug == 9) x;
	/* debug the protocol by showing transferred bits */
#define DEF_TIMEOUT 160



/* setting states on the bus with the right timing: */

#define set_lpc(adap, ctl, val) adap->setlpc(adap->data, ctl, val)
#define get_lpc(adap, ctl) adap->getlpc(adap->data, ctl)
#define get_own(adap) adap->getown(adap->data)
#define get_clock(adap) adap->getclock(adap->data)
#define i2c_outaddr(adap, val) adap->setlpc(adap->data, I2C_LPC_REG_DEVICE_ADDR, val)

#define i2c_outbyte1(adap, val) adap->setlpc(adap->data, I2C_LPC_REG_DATA_TX1, val)
#define i2c_outbyte2(adap, val) adap->setlpc(adap->data, I2C_LPC_REG_DATA_TX2, val)
#define i2c_outbyte3(adap, val) adap->setlpc(adap->data, I2C_LPC_REG_DATA_TX3, val)
#define i2c_outbyte4(adap, val) adap->setlpc(adap->data, I2C_LPC_REG_DATA_TX4, val)
#define i2c_inbyte1(adap) adap->getlpc(adap->data, I2C_LPC_REG_DATA_RX1)
#define i2c_inbyte2(adap) adap->getlpc(adap->data, I2C_LPC_REG_DATA_RX2)
#define i2c_inbyte3(adap) adap->getlpc(adap->data, I2C_LPC_REG_DATA_RX3)
#define i2c_inbyte4(adap) adap->getlpc(adap->data, I2C_LPC_REG_DATA_RX4)



#define LPC_FPRINTF_LOG_PATH "/tmp/file.log"
struct file *lpc_fprintf_file = NULL;

static int lpc_fprintf_debug(const char *fmt, ...)
{
	char lpc_fprintf_buf[256]={0};
	struct va_format vaf;
	va_list args;
	int r;
	mm_segment_t old_fs;
	struct timeval tv;
	struct rtc_time tm;

    do_gettimeofday(&tv);

	rtc_time_to_tm(tv.tv_sec,&tm);

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	r=snprintf(lpc_fprintf_buf,sizeof(lpc_fprintf_buf),"[%04d.%08d] %pV\n",tm.tm_sec, (int)tv.tv_usec, &vaf);
	va_end(args);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_write(lpc_fprintf_file, (char *)&lpc_fprintf_buf, strlen(lpc_fprintf_buf), &lpc_fprintf_file->f_pos);
	set_fs(old_fs);
	memset(lpc_fprintf_buf,0x0,sizeof(lpc_fprintf_buf));
	return r;

}



static int lpc_fprintf_init(void)
{
	printk("lpc_fprintf_init.\n");

	if(lpc_fprintf_file == NULL)
		lpc_fprintf_file = filp_open(LPC_FPRINTF_LOG_PATH, O_RDWR | O_APPEND | O_CREAT, 0644);

	if (IS_ERR(lpc_fprintf_file)) {
		printk("Error occured while opening file %s, exiting...\n", LPC_FPRINTF_LOG_PATH);
		return -1;
	}

	return 0;
}

static int lpc_fprintf_exit(void)
{
	printk("lpc_fprintf_exit.\n");

	if(lpc_fprintf_file != NULL)
		filp_close(lpc_fprintf_file, NULL);

	return 0;
}


/* other auxiliary functions */


void print_reg(struct i2c_algo_lpc_data *adap)
{
	unsigned char status;
	DEBPROTO(lpc_fprintf_debug("================================================\n");)
	status = get_lpc(adap, I2C_LPC_REG_BUS_SEL);
	DEBPROTO(lpc_fprintf_debug("%s select reg %x : %x\n",__func__,I2C_LPC_REG_BUS_SEL, status);)
	status = get_lpc(adap, I2C_LPC_REG_BYTE_COUNT);
	DEBPROTO(lpc_fprintf_debug("%s count reg %x : %x\n",__func__,I2C_LPC_REG_BYTE_COUNT, status);)

	status = get_lpc(adap, I2C_LPC_REG_COMMAND);
	DEBPROTO(lpc_fprintf_debug("%s command reg %x : %x\n",__func__,I2C_LPC_REG_COMMAND, status);)
	status = get_lpc(adap, I2C_LPC_REG_DEVICE_ADDR);
	DEBPROTO(lpc_fprintf_debug("%s address reg %x : %x\n",__func__,I2C_LPC_REG_DEVICE_ADDR, status);)

	status = get_lpc(adap, I2C_LPC_REG_STATUS);
	DEBPROTO(lpc_fprintf_debug("%s status reg %x : %x\n",__func__,I2C_LPC_REG_STATUS, status);)
}



static void i2c_repstart(struct i2c_algo_lpc_data *adap)
{
	DEBPROTO(lpc_fprintf_debug("%s :\n",__func__);)
	set_lpc(adap, I2C_LPC_REG_COMMAND, I2C_LPC_REPSTART);
}

static void i2c_stop(struct i2c_algo_lpc_data *adap)
{
	DEBPROTO(lpc_fprintf_debug("%s :\n",__func__);)
	set_lpc(adap, I2C_LPC_REG_COMMAND, I2C_LPC_STOP);
	udelay(60);
	set_lpc(adap, I2C_LPC_REG_COMMAND, 0x00);
}




static void i2c_start(struct i2c_algo_lpc_data *adap)
{
	print_reg(adap);

	set_lpc(adap, I2C_LPC_REG_COMMAND, I2C_LPC_START);

	print_reg(adap);
}




static int wait_for_bb(struct i2c_algo_lpc_data *adap)
{

	int timeout = DEF_TIMEOUT;
	int status;

	while (--timeout) {
		status = get_lpc(adap, I2C_LPC_REG_STATUS);

		DEBPROTO(lpc_fprintf_debug("%s : Waiting for bus free status : %x\n",__func__,status);)

		if(status == I2C_LPC_TD)
		{
			DEBPROTO(lpc_fprintf_debug("%s : Bus is free status : %x\n",__func__,status);)
			break;
		}
	}

	if (timeout == 0) {
		DEBPROTO(lpc_fprintf_debug("%s : Timeout for free busy status : %x\n",__func__,status);)
		return -ETIMEDOUT;
	}



	return 0;
}


static int wait_for_be(int mode,struct i2c_algo_lpc_data *adap)
{

	int timeout = DEF_TIMEOUT;
	unsigned char status;


	while (--timeout) {

		status = get_lpc(adap, I2C_LPC_REG_STATUS);

		DEBPROTO(lpc_fprintf_debug("%s : Waiting for bus empty status : %x\n",__func__,status);)

		if(mode == 1)
		{
			if((status & I2C_LPC_IBB) && (status & I2C_LPC_TBE))
			{
				DEBPROTO(lpc_fprintf_debug("%s : Bus is empty status : %x\n",__func__,status);)
				break;
			}
		}
		else
		{
			if(status & I2C_LPC_TD)
			{
				DEBPROTO(lpc_fprintf_debug("%s : Bus is empty status : %x\n",__func__,status);)
				break;
			}
		}

		status = get_lpc(adap, I2C_LPC_REG_TEST);

		DEBPROTO(lpc_fprintf_debug("%s : The test register data : %x\n",__func__,status);)
		udelay(1); /* wait for 100 us */
	}

	if (timeout == 0) {
		DEBPROTO(lpc_fprintf_debug("%s : Timeout waiting for Bus Empty\n",__func__);)
		return -ETIMEDOUT;
	}

	return 0;
}


static int wait_for_bf(struct i2c_algo_lpc_data *adap)
{

	int timeout = DEF_TIMEOUT;
	int status;


	while (--timeout) {
		status = get_lpc(adap, I2C_LPC_REG_STATUS);

		DEBPROTO(lpc_fprintf_debug("%s : Waiting for bus full status : %x\n",__func__,status);)

		if(status & I2C_LPC_RBF)
		{
			DEBPROTO(lpc_fprintf_debug("%s : Bus is full status : %x\n",__func__,status);)
			break;
		}

		status = get_lpc(adap, I2C_LPC_REG_TEST);

		DEBPROTO(lpc_fprintf_debug("%s : The test register data : %x\n",__func__,status);)
		udelay(1); /* wait for 100 us */
	}

	if (timeout == 0) {
		DEBPROTO(lpc_fprintf_debug("%s : Timeout waiting for Bus Full\n",__func__);)
		return -ETIMEDOUT;
	}

	return 0;
}

static int wait_for_td(struct i2c_algo_lpc_data *adap)
{

	int timeout = DEF_TIMEOUT;
	int status=0;

	while (--timeout) {
		udelay(4);
		status = get_lpc(adap, I2C_LPC_REG_STATUS);

		DEBPROTO(lpc_fprintf_debug("%s : Waiting for bus done status : %x\n",__func__,status);)

		if(status == I2C_LPC_TD)
		{
			DEBPROTO(lpc_fprintf_debug("%s : Bus is done status : %x\n",__func__,status);)
			break;
		}
	}

	if (timeout == 0) {
		DEBPROTO(lpc_fprintf_debug("%s : Timeout waiting for Bus Done\n",__func__);)
		return -ETIMEDOUT;
	}

	return 0;
}



static int wait_for_pin(struct i2c_algo_lpc_data *adap, int *status)
{
	int timeout = DEF_TIMEOUT;
	*status = get_lpc(adap, I2C_LPC_REG_STATUS);

	while ((*status & I2C_LPC_TBE) && --timeout) {
		*status = get_lpc(adap, I2C_LPC_REG_STATUS);
	}

	if (timeout == 0)
		return -ETIMEDOUT;


	return 0;
}


static int lpc_doAddress(struct i2c_algo_lpc_data *adap,struct i2c_msg *msg)
{
	unsigned short flags = msg->flags;
	unsigned char addr;

	addr = msg->addr << 1;
	if (flags & I2C_M_RD)
	{
		addr |= 1;
		DEBPROTO(lpc_fprintf_debug("step 7 : read mode then write device address 0x%x\n",addr);)
	}
	else
	{
		DEBPROTO(lpc_fprintf_debug("step 2 : write mode then write device address 0x%x\n",addr);)
	}

	if (flags & I2C_M_REV_DIR_ADDR)
	{
		addr ^= 1;

	}
	i2c_outaddr(adap, addr);
	return 0;

}


static int lpc_sendbytes(struct i2c_adapter *i2c_adap, struct i2c_msg *msg)
{
	struct i2c_algo_lpc_data *adap = i2c_adap->algo_data;
	int i = 0,timeout=0;

	unsigned int count = msg->len;
	unsigned char *buf = msg->buf;

	do{
		lpc_doAddress(adap,msg);
		set_lpc(adap, I2C_LPC_REG_BYTE_COUNT, (count-i) >= 4 ? 4:(count - i));
		DEBPROTO(lpc_fprintf_debug("step 3 : write register count %x\n",count);)

		if((count -i) >= 4)
		{
			i2c_outbyte1(adap, buf[i+0] & 0xff);
			i2c_outbyte2(adap, buf[i+1] & 0xff);
			i2c_outbyte3(adap, buf[i+2] & 0xff);
			i2c_outbyte4(adap, buf[i+3] & 0xff);

			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+0,buf[i+0]);)
			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+1,buf[i+1]);)
			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+2,buf[i+2]);)
			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+3,buf[i+3]);)
			i += 4;
		}
		else if((count -i) == 3)
		{
			i2c_outbyte1(adap, buf[i+0] & 0xff);
			i2c_outbyte2(adap, buf[i+1] & 0xff);
			i2c_outbyte3(adap, buf[i+2] & 0xff);

			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+0,buf[i+0]);)
			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+1,buf[i+1]);)
			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+2,buf[i+2]);)

			i += 3;
		}
		else if((count -i) == 2)
		{
			i2c_outbyte1(adap, buf[i+0] & 0xff);
			i2c_outbyte2(adap, buf[i+1] & 0xff);
			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+0,buf[i+0]);)
			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+1,buf[i+1]);)
			i += 2;
		}
		else if((count -i) == 1)
		{
			i2c_outbyte1(adap, buf[i+0] & 0xff);
			DEBPROTO(lpc_fprintf_debug("step 4 : Send data[%d] = %x\n",i+0,buf[i+0]);)
			i += 1;
		}

		/* Send START */
		DEBPROTO(lpc_fprintf_debug("step 5-1 : Delay 6mS \n");)
		udelay(6000);
		DEBPROTO(lpc_fprintf_debug("step 5-2 : Start to transfrom \n");)
		i2c_stop(adap);
		i2c_start(adap);
		DEBPROTO(lpc_fprintf_debug("step 5-3 : Start done\n");)

		udelay(400);
		DEBPROTO(lpc_fprintf_debug("step 6 : Waiting for BE\n");)
		timeout = wait_for_td(adap);
		if (timeout) {
			DEBPROTO(lpc_fprintf_debug("step 6 : Timeout waiting for BE \n");)
			return -EREMOTEIO;
		}
	}while (i < count);

	if(i == count)
	{
		DEBPROTO(lpc_fprintf_debug("Writen %d bytes successd !\n",count);)
		return i;
	}
	else
	{
		DEBPROTO(lpc_fprintf_debug("Writen %d bytes failed \n",count);)
		return -EIO;
	}
}

static int lpc_readbytes(struct i2c_adapter *i2c_adap, struct i2c_msg *msg)
{
	int i=0,timeout=0;
	struct i2c_algo_lpc_data *adap = i2c_adap->algo_data;

	unsigned int count = msg->len;
	unsigned char *buf = msg->buf;

	do{
		lpc_doAddress(adap,msg);
		set_lpc(adap, I2C_LPC_REG_BYTE_COUNT, (count-i) >= 4 ? 4:(count - i));
		DEBPROTO(lpc_fprintf_debug("step 8 : write register count %d\n",count);)

		/* Send START */
		DEBPROTO(lpc_fprintf_debug("step 9-1 : Delay 6mS\n");)
		udelay(6000);
		DEBPROTO(lpc_fprintf_debug("step 9-2 : Start to receive data\n");)
		i2c_stop(adap);
		i2c_start(adap);
		DEBPROTO(lpc_fprintf_debug("step 9-3 : Start done\n");)

		udelay(400);
		DEBPROTO(lpc_fprintf_debug("step 10 : Waiting for TD\n");)
		timeout = wait_for_td(adap);
		if (timeout) {
			DEBPROTO(lpc_fprintf_debug("step 10 : Timeout waiting for TD \n");)
			return -EREMOTEIO;
		}

		if((count -i) >= 4)
		{
			buf[i+0] = 0xff & i2c_inbyte1(adap);
			buf[i+1] = 0xff & i2c_inbyte2(adap);
			buf[i+2] = 0xff & i2c_inbyte3(adap);
			buf[i+3] = 0xff & i2c_inbyte4(adap);

			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+0,buf[i+0]);)
			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+1,buf[i+1]);)
			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+2,buf[i+2]);)
			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+3,buf[i+3]);)

			i += 4;
		}
		else if((count -i) == 3)
		{
			buf[i+0] = 0xff & i2c_inbyte1(adap);
			buf[i+1] = 0xff & i2c_inbyte2(adap);
			buf[i+2] = 0xff & i2c_inbyte3(adap);

			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+0,buf[i+0]);)
			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+1,buf[i+1]);)
			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+2,buf[i+2]);)

			i += 3;
		}
		else if((count -i) == 2)
		{
			buf[i+0] = 0xff & i2c_inbyte1(adap);
			buf[i+1] = 0xff & i2c_inbyte2(adap);
			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+0,buf[i+0]);)
			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+1,buf[i+1]);)
			i += 2;
		}
		else if((count -i) == 1)
		{
			buf[i+0] = 0xff & i2c_inbyte1(adap);
			DEBPROTO(lpc_fprintf_debug("step 11 : Receive data[%d] = %x\n",i+0,buf[i+0]);)
			i += 1;
		}


	}while(i < count);

	if(i == count)
	{
		DEBPROTO(lpc_fprintf_debug("Read %d bytes successd !\n",count);)
		return i;
	}
	else
	{
		DEBPROTO(lpc_fprintf_debug("Read %d bytes failed \n",count);)
		return -EIO;
	}
}


struct cpld_client_node {
	struct i2c_client *client;
	struct list_head   list;
};
#define LPC_I2C_MAX_NCHANS 6


struct lpc_iic {
	struct i2c_adapter *virt_adaps[LPC_I2C_MAX_NCHANS];
	u8 last_chan;		/* last register value */
};


static int lpc_master_xfer(struct i2c_adapter *i2c_adap,
		    struct i2c_msg *msgs,
		    int num)
{
	struct i2c_algo_lpc_data *adap = i2c_adap->algo_data;
	struct i2c_msg *pmsg;
	int i;
	int ret=0;

	mutex_lock(&lpc_lock);

	if (adap->xfer_begin)
		adap->xfer_begin(&i2c_adap->nr);


	for (i = 0;ret >= 0 && i < num; i++) {
		pmsg = &msgs[i];

		DEBPROTO(lpc_fprintf_debug("lpc_xfer.o: Doing %s %d bytes to 0x%02x - %d of %d messages\n",
		     pmsg->flags & I2C_M_RD ? "read" : "write",
		     pmsg->len, pmsg->addr, i + 1, num);)

		DEBPROTO(lpc_fprintf_debug("lpc_xfer.o: Msg %d, addr=0x%x, flags=0x%x, len=%d\n",
			    i, msgs[i].addr, msgs[i].flags, msgs[i].len);)

		if (pmsg->flags & I2C_M_RD) {
			ret = lpc_readbytes(i2c_adap, pmsg);

			if (ret != pmsg->len) {
				DEBPROTO(lpc_fprintf_debug("lpc_xfer.o: fail: "
					    "only read %d bytes.\n",ret));
			} else {
				DEBPROTO(lpc_fprintf_debug("lpc_xfer.o: read %d bytes.\n",ret));
			}
		} else {

			ret = lpc_sendbytes(i2c_adap, pmsg);

			if (ret != pmsg->len) {
				DEBPROTO(lpc_fprintf_debug("lpc_xfer.o: fail: "
					    "only wrote %d bytes.\n",ret));
			} else {
				DEBPROTO(lpc_fprintf_debug("lpc_xfer.o: wrote %d bytes.\n",ret));
			}
		}
	}

	if (adap->xfer_end)
		adap->xfer_end(&i2c_adap->nr);

	mutex_unlock(&lpc_lock);

	DEBPROTO(lpc_fprintf_debug("ret = 0x%x num = 0x%x i = 0x%x.\n",ret,num,i));

	return ret = (ret < 0) ? ret : num;
}


static u32 lpc_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_SMBUS_QUICK;
}

/* exported algorithm data: */
static const struct i2c_algorithm lpc_algo = {
	.master_xfer	= lpc_master_xfer,
	//.smbus_xfer		= lpc_smbus_xfer,
	.functionality	= lpc_func,
};


/**********************************************   End  ********************************************************/






/**********************************************   Start  ********************************************************/
#define DEFAULT_BASE 0x0a00

static int lpc_base= 0x0a00;
static u8 __iomem *lpc_base_iomem;

static int lpc_irq;
static int lpc_clock  = 0x1c;
static int lpc_own    = 0x55;
static int lpc_mmapped;

static unsigned long lpc_base_addr = 0x0a00;
static unsigned int  lpc_io_space_size = 2;

static unsigned long LPC_INDEX_REG;
static unsigned long LPC_DATA_REG;


/* notice : removed static struct i2c_lpc_iic gpi; code -
  this module in real supports only one device, due to missing arguments
  in some functions, called from the algo-lpc module. Sometimes it's
  need to be rewriten - but for now just remove this for simpler reading */

static wait_queue_head_t lpc_wait;
static int lpc_pending;
static spinlock_t lock;
static spinlock_t lpc_slock;

static struct i2c_adapter lpc_iic_ops;

struct cpld_dev_type {
    struct resource *io_resource;
    struct semaphore sem;
    struct cdev cdev;
};

struct cpld_dev_type *cpld_device;


/* ----- local functions ----------------------------------------------	*/

static void lpc_cpld_setbyte(void *data, int ctl, int val)
{
    outb(ctl, LPC_INDEX_REG);
    mb();

    outb(val, LPC_DATA_REG);
    mb();
}

static int lpc_cpld_getbyte(void *data, int ctl)
{
	u8 val = 0;

    outb(ctl, LPC_INDEX_REG);
    mb();

    val = inb(LPC_DATA_REG);
    mb();

    return val;
}

static void lpc_iic_setbyte(void *data, int ctl, int val)
{
    if (!cpld_device)
    {
        return  ;
    }

    if (down_interruptible(&cpld_device->sem))
	{
		return	;
	}


	lpc_cpld_setbyte(data,ctl,val);

	up(&cpld_device->sem);
	DEBPROTO(lpc_fprintf_debug("%s REG[%x] = %x\n",__func__,ctl,val);)
}


static int lpc_iic_getbyte(void *data, int ctl)
{
	u8 val = 0;
    if (!cpld_device)
        return -ENOTTY;

    if (down_interruptible(&cpld_device->sem))
        return -ERESTARTSYS;

	val = lpc_cpld_getbyte(data,ctl);

	up(&cpld_device->sem);
	DEBPROTO(lpc_fprintf_debug("%s REG[%x] = %x\n",__func__,ctl,val);)
    return val;
}

int cig_cpld_read_register(u8 reg_off, u8 *val)
{
    if (!cpld_device)
        return -ENOTTY;

    if (down_interruptible(&cpld_device->sem))
        return -ERESTARTSYS;

    *val = lpc_cpld_getbyte(cpld_device, reg_off);

	up(&cpld_device->sem);

    return 0;
}
EXPORT_SYMBOL(cig_cpld_read_register);

int cig_cpld_write_register(u8 reg_off, u8 val)
{
    if (!cpld_device)
        return -ENOTTY;

    if (down_interruptible(&cpld_device->sem))
        return -ERESTARTSYS;

    lpc_cpld_setbyte(cpld_device, reg_off, val);
	up(&cpld_device->sem);
    return 0;
}
EXPORT_SYMBOL(cig_cpld_write_register);



static int lpc_iic_getown(void *data)
{
	return (lpc_own);
}


static int lpc_iic_getclock(void *data)
{
	return (lpc_clock);
}

static void lpc_iic_waitforpin(void *data)
{
	DEFINE_WAIT(wait);
	int timeout = 2;
	unsigned long flags;

	if (lpc_irq > 0) {
		spin_lock_irqsave(&lock, flags);
		if (lpc_pending == 0) {
			spin_unlock_irqrestore(&lock, flags);
			prepare_to_wait(&lpc_wait, &wait, TASK_INTERRUPTIBLE);
			if (schedule_timeout(timeout*HZ)) {
				spin_lock_irqsave(&lock, flags);
				if (lpc_pending == 1) {
					lpc_pending = 0;
				}
				spin_unlock_irqrestore(&lock, flags);
			}
			finish_wait(&lpc_wait, &wait);
		} else {
			lpc_pending = 0;
			spin_unlock_irqrestore(&lock, flags);
		}
	} else {
		udelay(100);
	}
}


static irqreturn_t lpc_iic_handler(int this_irq, void *dev_id) {
	spin_lock(&lock);
	lpc_pending = 1;
	spin_unlock(&lock);
	wake_up_interruptible(&lpc_wait);
	return IRQ_HANDLED;
}

static int board_id = 0;


static int lpc_iic_select(void *data)
{
	unsigned int chan_id=0;
	chan_id = *(unsigned int *)data;
	chan_id -= 2;
	DEBPROTO(lpc_fprintf_debug("step 1 : selest channel id = %d\n",chan_id);)
	lpc_iic_setbyte(data,I2C_LPC_REG_BUS_SEL,chan_id);

	return 0;
}

static int lpc_iic_deselect(void *data)
{

	unsigned int chan_id=0;
	chan_id = *(unsigned int *)data;
	chan_id -= 2;
	DEBPROTO(lpc_fprintf_debug("step last :deselect channel id = %d\n",chan_id);)

	return 0;
}


/* ------------------------------------------------------------------------
 * Encapsulate the above functions in the correct operations structure.
 * This is only done when more than one hardware adapter is supported.
 */
static struct i2c_algo_lpc_data lpc_iic_data = {
	.setlpc	    = lpc_iic_setbyte,
	.getlpc	    = lpc_iic_getbyte,
	.getown	    = lpc_iic_getown,
	.getclock   = lpc_iic_getclock,
	.waitforpin = lpc_iic_waitforpin,
	.xfer_begin = lpc_iic_select,
	.xfer_end	= lpc_iic_deselect,
};

#include <linux/i2c-algo-bit.h>

static struct i2c_adapter lpc_iic_arr_ops[LPC_I2C_MAX_NCHANS] = {0};

static void dummy_setscl(void *data, int state)
{
	return;
}

static void dummy_setsda(void *data, int state)
{
	return;

}

static int dummy_getscl(void *data)
{
	return 1;

}

static int dummy_getsda(void *data)
{
	return 1;
}


static struct i2c_algo_bit_data dummy_algo_data = {
	.setsda		= dummy_setsda,
	.setscl		= dummy_setscl,
	.getsda		= dummy_getsda,
	.getscl		= dummy_getscl,
	.udelay		= 50,
	.timeout	= HZ,
};


static int dummy_xfer(struct i2c_adapter *i2c_adap,
		    struct i2c_msg *msgs,
		    int num)
{
	return 1;
}

static u32 dummy_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_SMBUS_QUICK;
}


static const struct i2c_algorithm dummy_algo = {
	.master_xfer	= dummy_xfer,
	.functionality	= dummy_func,
};



static struct i2c_adapter i2c_dummy = {
	.owner		= THIS_MODULE,
	.class		= I2C_CLASS_HWMON,
	.algo_data	= &dummy_algo_data,
	.algo		= &dummy_algo,
	.name		= "i2c_dummy",
};


static int lpc_iic_match(struct device *dev, unsigned int id)
{
	/* sanity checks for lpc_mmapped I/O */

	DEB2(printk("lpc_iic_match\n");)


	if (lpc_base < DEFAULT_BASE) {
		dev_err(dev, "incorrect lpc_base address (%#x) specified "
		       "for lpc_mmapped I/O\n", lpc_base);
		return 0;
	}

	if (lpc_base == 0) {
		lpc_base = DEFAULT_BASE;
	}
	return 1;
}

static int lpc_iic_probe(struct device *dev, unsigned int id)
{
	int rval,num;

	lpc_fprintf_init();

	DEB2(printk("lpc_iic_probe\n");)

	mutex_init(&lpc_lock);

	for(num = 0; num < LPC_I2C_MAX_NCHANS;num++)
	{
		lpc_iic_arr_ops[num].dev.parent = dev;
		lpc_iic_arr_ops[num].owner = THIS_MODULE;
		lpc_iic_arr_ops[num].class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
		lpc_iic_arr_ops[num].algo = &lpc_algo;
		lpc_iic_arr_ops[num].algo_data = &lpc_iic_data,
		lpc_iic_arr_ops[num].nr=num;
		snprintf(lpc_iic_arr_ops[num].name, sizeof(lpc_iic_arr_ops[num].name), "i2c-%d-lpc", i2c_adapter_id(&lpc_iic_arr_ops[num]));
		rval |= i2c_add_adapter(&lpc_iic_arr_ops[num]);
		DEB2(printk("%s\n",lpc_iic_arr_ops[num].name);)
	}

	return 0;
}

static int lpc_iic_remove(struct device *dev, unsigned int id)
{
	int num;
	DEB2(printk("lpc_iic_remove\n"));

	lpc_fprintf_exit();
	for(num = LPC_I2C_MAX_NCHANS - 1; num >= 0 ;num--)
		i2c_del_adapter(&lpc_iic_arr_ops[num]);


	return 0;
}

static struct isa_driver i2c_lpc_driver = {
	.match		= lpc_iic_match,
	.probe		= lpc_iic_probe,
	.remove		= lpc_iic_remove,
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "lpc-iic",
	},
};

/**********************************************   End  ********************************************************/






/**********************************************   Start  ********************************************************/

static int cpld_major = 0;
static int cpld_minor = 0;

struct cpld_rw_msg {
    unsigned char addr;
    unsigned char data;
};


static struct cpld_rw_msg param_read = {-1};
static struct cpld_rw_msg param_write = {-1};
static struct cpld_rw_msg param_reads = {-1};
static struct cpld_rw_msg param_writes = {-1};

void cpld_sysfs_kobj_release(struct kobject *kobj)
{
    return;
}

int cpld_sysfs_add_attr(struct kobject* kobj, char* attr_name)
{

	struct attribute *attr;

    attr = kmalloc(sizeof(struct attribute), GFP_KERNEL);
    attr->name = attr_name;
    attr->mode = 0644;

    return sysfs_create_file(kobj, attr);
}

static int cig_cpld_write_slave_cpld_register(u8 reg_addr, u8 reg_data);
static int cig_cpld_read_slave_cpld_register(u8 reg_addr, u8 *reg_data);


static ssize_t cpld_sysfs_show(struct kobject *kobj, struct attribute *attr, char *buffer)
{
	u8 val=0,ret=0,year=0,month=0,day=0,cpld_m=0,cpld_1=0,cpld_2=0;

    if (0 == strcmp(attr->name, "read"))
    {
		val = lpc_iic_getbyte(NULL,param_read.addr);
		ret = sprintf(buffer,"read : addr = 0x%x val = 0x%x\n",param_read.addr, val);

    }
    else if (0 == strcmp(attr->name, "write"))
    {
		lpc_iic_setbyte(NULL, param_write.addr,param_write.data);
		ret = sprintf(buffer,"write : addr = 0x%x val = 0x%x\n",param_write.addr, param_write.data);
    }
	else if (0 == strcmp(attr->name, "version"))
    {
		cpld_m = lpc_iic_getbyte(NULL, 0x02);
		year = lpc_iic_getbyte(NULL, 0x03);
		month = lpc_iic_getbyte(NULL, 0x04);
		day = lpc_iic_getbyte(NULL, 0x05);

		cig_cpld_read_slave_cpld_register(0x1d,&cpld_1);
		cig_cpld_read_slave_cpld_register(0x1e,&cpld_2);

		ret = sprintf(buffer,"Main CPLD version : V%02x\n"\
							  "Main CPLD date : 20%02x-%02x-%02x\n"\
							  "Slave 1 CPLD version : V%02x\n"\
							  "Slave 2 CPLD version : V%02x\n",cpld_m,year,month,day,cpld_1,cpld_2);
    }
    if (0 == strcmp(attr->name, "reads"))
    {
		ret = cig_cpld_read_slave_cpld_register(param_reads.addr,&val);
		if (ret < 0)
		    printk("ERROR:Failed to read slave cpld.\n");
		ret = sprintf(buffer,"reads : addr = 0x%x val = 0x%x\n",param_reads.addr, val);

    }
    else if (0 == strcmp(attr->name, "writes"))
    {
		ret = cig_cpld_write_slave_cpld_register(param_writes.addr,param_writes.data);
		if (ret < 0)
		    printk("ERROR:Failed to read slave cpld.\n");
		ret = sprintf(buffer,"writes : addr = 0x%x val = 0x%x\n",param_writes.addr, param_writes.data);
    }


    return ret;

}

static ssize_t cpld_sysfs_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t count)
{
    int param[3];

    if (0 == strcmp(attr->name, "read"))
    {
        sscanf(buffer, "0x%02x", &param[0]);
        param_read.addr = param[0];
    }
    else if (0 == strcmp(attr->name, "write"))
    {
        sscanf(buffer, "0x%2x 0x%02x", &param[0], &param[1]);
        param_write.addr = param[0];
        param_write.data = param[1];
    }
    if (0 == strcmp(attr->name, "reads"))
    {
        sscanf(buffer, "0x%02x", &param[0]);
        param_reads.addr = param[0];
    }
    else if (0 == strcmp(attr->name, "writes"))
    {
        sscanf(buffer, "0x%2x 0x%02x", &param[0], &param[1]);
        param_writes.addr = param[0];
        param_writes.data = param[1];
    }
	return count;
}



static struct sysfs_ops cpld_sysfs_ops =
{
    .show   = cpld_sysfs_show,
    .store  = cpld_sysfs_store,
};

static struct kobj_type cpld_kobj_type =
{
    .release        = cpld_sysfs_kobj_release,
    .sysfs_ops      = &cpld_sysfs_ops,
    .default_attrs  = NULL,
};


static const char driver_name[] = "cpld_drv";
static atomic_t cpld_available = ATOMIC_INIT(1);
static struct class *cpld_class;
static struct device *cpld_dev;



#define CPLD_IOC_MAGIC  '['

#define CPLD_IOC_RDREG  _IOR(CPLD_IOC_MAGIC, 0, struct cpld_rw_msg)
#define CPLD_IOC_WRREG  _IOW(CPLD_IOC_MAGIC, 1, struct cpld_rw_msg)

#define CPLD_IOC_MAXNR  2


int cpld_open(struct inode *inode, struct file *filp)
{
    struct cpld_dev_type *dev;

    if (! atomic_dec_and_test(&cpld_available)) {
        atomic_inc(&cpld_available);
        return -EBUSY;
    }

    dev = container_of(inode->i_cdev, struct cpld_dev_type, cdev);
    filp->private_data = dev;

    return 0;
}

int cpld_release(struct inode *inode, struct file *flip)
{
    atomic_inc(&cpld_available);
    return 0;
}


long cpld_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int rc = 0;
	int err = 0;
    struct cpld_dev_type *dev = (struct cpld_dev_type *)filp->private_data;
    struct cpld_rw_msg msg;

    if (_IOC_TYPE(cmd) != CPLD_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > CPLD_IOC_MAXNR)
        return -ENOTTY;

    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    switch(cmd){
        case CPLD_IOC_RDREG:
            rc = copy_from_user(&msg, (void __user *)arg, sizeof(msg));
            if (!rc) {
                msg.data = lpc_cpld_getbyte(dev, msg.addr);
                rc = copy_to_user((void __user *)arg, &msg, sizeof(msg));
            }
            break;

        case CPLD_IOC_WRREG:
            rc = copy_from_user(&msg, (void __user *)arg, sizeof(msg));
            if (!rc) {
                lpc_cpld_setbyte(dev, msg.addr, msg.data);
            }
            break;
        default:
            rc = -ENOTTY;
            break;
    }
    up(&dev->sem);

    return rc;
}

struct file_operations cpld_fops = {
    .owner          = THIS_MODULE,
    .open           = cpld_open,
    .unlocked_ioctl = cpld_ioctl,
    .release        = cpld_release,
};


static void cpld_setup_cdev(struct cpld_dev_type *dev)
{
    int err, devno = MKDEV(cpld_major, cpld_minor);

    cdev_init(&dev->cdev, &cpld_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &cpld_fops;
    err = cdev_add(&dev->cdev, devno, 1);

    if (err)
        DEB2(printk(KERN_NOTICE "Error %d adding cpld", err);)
}
/**********************************************    End   ********************************************************/




/**********************************************   Start  ********************************************************/
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/irq.h>

static spinlock_t		irq_inter_lock;
static struct delayed_work irq_inter_work;
static unsigned long irq_inter_delay;


static int cig_cpld_write_slave_cpld_register(u8 reg_addr, u8 reg_data)
{
	u8 read_status = 0;
	u8 wait_time_out = WAIT_TIME_OUT_COUNT;
	DEB2(printk("<=======write=========>"));
	cig_cpld_write_register(ADDR_REG_SFP_STATUS_ADDR, reg_addr << 1);
	DEB2(printk("[62]=%x\n",reg_addr << 1));
	cig_cpld_write_register(ADDR_REG_SFP_STATUS_TX, reg_data);
	DEB2(printk("[63]=%x\n",reg_data));
	cig_cpld_write_register(ADDR_REG_SFP_STATUS_COMMAND, 0x80);
	DEB2(printk("[65]=%x\n",0x80));
	do{
		cig_cpld_read_register(ADDR_REG_SFP_STATUS_STATUS, &read_status);
		DEB2(printk("[66]=%x\n",read_status));
		udelay(60);
		wait_time_out--;
		if(wait_time_out == 0)
			break;
	}while(read_status != 0x02);
	DEB2(printk("<=======write=========>"));


	if(wait_time_out == 0)
		return -1;

	return 1;
}


static int cig_cpld_read_slave_cpld_register(u8 reg_addr, u8 *reg_data)
{
	u8 read_status = 0;
	u8 wait_time_out = WAIT_TIME_OUT_COUNT;
	DEB2(printk("<========read=========>"));
	cig_cpld_write_register(ADDR_REG_SFP_STATUS_ADDR, reg_addr << 1 | 1);
	DEB2(printk("[62]=%x\n",reg_addr << 1 | 1));
	cig_cpld_write_register(ADDR_REG_SFP_STATUS_COMMAND, 0x80);
	DEB2(printk("[65]=%x\n",0x80));
	do{
		udelay(60);
		cig_cpld_read_register(ADDR_REG_SFP_STATUS_STATUS, &read_status);
		DEB2(printk("[66]=%x\n",read_status));
		wait_time_out--;
		if(wait_time_out == 0)
			break;
	}while(read_status != 0x01);

	cig_cpld_read_register(ADDR_REG_SFP_STATUS_RX,reg_data);
	DEB2(printk("[64]=%x\n",*reg_data));
	DEB2(printk("<========read=========>"));

	if(wait_time_out == 0)
		return -1;

	return 1;
}



struct sock *nlsk = NULL;
extern struct net init_net;
#define NETLINK_TEST     26
#define MSG_LEN            125
#define USER_PORT        100
static u32 irq_present_status_low_current,irq_present_status_low_next;
static u32 irq_present_status_high_current,irq_present_status_high_next;
static u32 irq_tx_fault_status_low_current,irq_tx_fault_status_low_next;
static u32 irq_tx_fault_status_high_current,irq_tx_fault_status_high_next;
static u32 irq_rx_lost_status_low_current,irq_rx_lost_status_low_next;
static u32 irq_rx_lost_status_high_current,irq_rx_lost_status_high_next;

static u8 irq_present_qsfp_current,irq_present_qsfp_next;
static u8 irq_interrupt_qsfp_current,irq_interrupt_qsfp_next;

struct input_dev *cpld_input_dev;



int send_usrmsg(char *pbuf, uint16_t len)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;

    int ret;


    nl_skb = nlmsg_new(len, GFP_ATOMIC);
    if(!nl_skb)
    {
        printk("netlink alloc failure\n");
        return -1;
    }


    nlh = nlmsg_put(nl_skb, 0, 0, NETLINK_TEST, len, 0);
    if(nlh == NULL)
    {
        printk("nlmsg_put failaure \n");
        nlmsg_free(nl_skb);
        return -1;
    }

    memcpy(nlmsg_data(nlh), pbuf, len);
    ret = netlink_unicast(nlsk, nl_skb, USER_PORT, MSG_DONTWAIT);

    return ret;
}

static void netlink_rcv_msg(struct sk_buff *skb)
{

    struct nlmsghdr *nlh = NULL;
    char *umsg = NULL;
    char kmsg[1024] = {0};
	char kmsg_tmp[16] = {0};
	u8 i = 0;
	u8 tmp[3]={0};

    if(skb->len >= nlmsg_total_size(0))
    {
        nlh = nlmsg_hdr(skb);
        umsg = NLMSG_DATA(nlh);
        if(umsg)
		{
			for(i = 0;i < 24;i++)
			{
				if(!(irq_present_status_low_current & (0x1 << i)))
				{
					tmp[0] = 1;
				}
				else
				{
					tmp[0] = 0;
				}

				if(!(irq_rx_lost_status_low_current & (0x1 << i)))
				{
					tmp[1] = 1;
				}
				else
				{
					tmp[1] = 0;
				}

				if(!(irq_tx_fault_status_low_current & (0x1 << i)))
				{
					tmp[2] = 1;
				}
				else
				{
					tmp[2] = 0;
				}
				memset(kmsg_tmp,0xff,sizeof(kmsg_tmp));
				sprintf(kmsg_tmp,"sfp%02d:%1d:%1d:%1d ",i+1,tmp[0],tmp[1],tmp[2]);
				strcat(kmsg,kmsg_tmp);
			}

			for(i = 0;i < 24;i++)
			{
				if(!(irq_present_status_high_current & (0x1 << i)))
				{
					tmp[0] = 1;
				}
				else
				{
					tmp[0] = 0;
				}

				if(!(irq_rx_lost_status_high_current & (0x1 << i)))
				{
					tmp[1] = 1;
				}
				else
				{
					tmp[1] = 0;
				}

				if(!(irq_tx_fault_status_high_current & (0x1 << i)))
				{
					tmp[2] = 1;
				}
				else
				{
					tmp[2] = 0;
				}
				memset(kmsg_tmp,0xff,sizeof(kmsg_tmp));
				sprintf(kmsg_tmp,"sfp%02d:%1d:%1d:%1d ",i+25,tmp[0],tmp[1],tmp[2]);
				strcat(kmsg,kmsg_tmp);
			}


			for(i = 0;i < 8;i++)
			{
				if(!(irq_present_qsfp_current & (0x1 << i)))
				{
					tmp[0] = 1;
				}
				else
				{
					tmp[0] = 0;
				}

				if(!(irq_interrupt_qsfp_current & (0x1 << i)))
				{
					tmp[1] = 1;
				}
				else
				{
					tmp[1] = 0;
				}

				memset(kmsg_tmp,0xff,sizeof(kmsg_tmp));
				sprintf(kmsg_tmp,"qsfp%02d:%1d:%1d:%1d ",i+49,tmp[0],tmp[1],0);
				strcat(kmsg,kmsg_tmp);
			}

            printk("kernel recv from user: %s\n", umsg);
            send_usrmsg(kmsg, strlen(kmsg));
        }
    }

	return ;
}



struct netlink_kernel_cfg cfg = {
        .input  = netlink_rcv_msg, /* set recv callback */
};



#define RANGE_OF_BYTE_SHIFT(to_arg,shift,from_arg)  {to_arg &= ~(0xff << shift); to_arg |= from_arg << shift;}


static void irq_inter_wapper(struct work_struct * work)
{

	u8 m_data = 0;
	u8 data_high8 = 0,data_low8 = 0;
	u16 data_16 = 0;
	u8 status = 0;
	u8 i = 0;
	char kmsg[64]={0};
	u8 tmp[3] = {0};

	DEB2(printk("CPLD_MASTER_INTERRUPT\r\n"));

	m_data = lpc_iic_getbyte(NULL,CPLD_MASTER_INTERRUPT_STATUS_REG);
	lpc_iic_setbyte(NULL,CPLD_MASTER_INTERRUPT_STATUS_REG,0xff);

	cig_cpld_write_slave_cpld_register(CPLD_SLAVE1_INTERRUPT_MASK_REG,0xff);
	cig_cpld_write_slave_cpld_register(CPLD_SLAVE2_INTERRUPT_MASK_REG,0xff);
	if(!(m_data & CPLD_MASTER_INTERRUPT_CPLD1))
	{
		cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_INTERRUPT_STATUS_H_REG,&data_high8);
		cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_INTERRUPT_STATUS_L_REG,&data_low8);
		data_16 = data_low8 | data_high8 << 8;
		if(
			!(data_16 & CPLD_SLAVE1_INTERRUPT_PRESENT08) ||
			!(data_16 & CPLD_SLAVE1_INTERRUPT_PRESENT16) ||
			!(data_16 & CPLD_SLAVE1_INTERRUPT_PRESENT24)
		)
		{
			if(!(data_16 & CPLD_SLAVE1_INTERRUPT_PRESENT08))
			{
				DEB2(printk("CPLD_SLAVE1_INTERRUPT_PRESENT08\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_PRESENT08_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_present_status_low_current,0,status);

			}
			else if(!(data_16 & CPLD_SLAVE1_INTERRUPT_PRESENT16))
			{
				DEB2(printk("CPLD_SLAVE1_INTERRUPT_PRESENT16\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_PRESENT16_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_present_status_low_current,8,status);
			}
			else if(!(data_16 & CPLD_SLAVE1_INTERRUPT_PRESENT24))
			{
				DEB2(printk("CPLD_SLAVE1_INTERRUPT_PRESENT24\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_PRESENT24_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_present_status_low_current,16,status);
			}
			DEB2(printk("irq_present_status_low_next = %08x irq_present_status_low_current = %08x \n",irq_present_status_low_next,irq_present_status_low_current));
		}

		if(
			!(data_16 & CPLD_SLAVE1_INTERRUPT_RX_LOST08) ||
			!(data_16 & CPLD_SLAVE1_INTERRUPT_RX_LOST16) ||
			!(data_16 & CPLD_SLAVE1_INTERRUPT_RX_LOST24)
		)
		{
			if(!(data_16 & CPLD_SLAVE1_INTERRUPT_RX_LOST08))
			{
				DEB2(printk("CPLD_SLAVE1_INTERRUPT_RX_LOST08\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_RX_LOST08_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_rx_lost_status_low_current,0,status);

			}
			else if(!(data_16 & CPLD_SLAVE1_INTERRUPT_RX_LOST16))
			{
				DEB2(printk("CPLD_SLAVE1_INTERRUPT_PRESENT16\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_RX_LOST16_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_rx_lost_status_low_current,8,status);
			}
			else if(!(data_16 & CPLD_SLAVE1_INTERRUPT_RX_LOST24))
			{
				DEB2(printk("CPLD_SLAVE1_INTERRUPT_PRESENT24\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_RX_LOST24_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_rx_lost_status_low_current,16,status);
			}
			DEB2(printk("irq_rx_lost_status_low_next = %08x irq_rx_lost_status_low_current = %08x \n",irq_rx_lost_status_low_next,irq_rx_lost_status_low_current));
		}

		if(
			!(data_16 & CPLD_SLAVE1_INTERRUPT_RX_LOST08) ||
			!(data_16 & CPLD_SLAVE1_INTERRUPT_RX_LOST16) ||
			!(data_16 & CPLD_SLAVE1_INTERRUPT_RX_LOST24)
		)
		{
			if(!(data_16 & CPLD_SLAVE1_INTERRUPT_TX_FAULT08))
			{
				DEB2(printk("CPLD_SLAVE1_INTERRUPT_TX_FAULT08\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_TX_FAULT08_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_tx_fault_status_low_current,0,status);

			}
			else if(!(data_16 & CPLD_SLAVE1_INTERRUPT_TX_FAULT16))
			{
				DEB2(printk("CPLD_SLAVE1_INTERRUPT_TX_FAULT16\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_TX_FAULT16_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_tx_fault_status_low_current,8,status);
			}
			else if(!(data_16 & CPLD_SLAVE1_INTERRUPT_TX_FAULT24))
			{
				DEB2(printk("CPLD_SLAVE1_INTERRUPT_TX_FAULT24\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE1_TX_FAULT24_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_tx_fault_status_low_current,16,status);
			}
			DEB2(printk("irq_tx_fault_status_low_next = %08x irq_tx_fault_status_low_current = %08x \n",irq_tx_fault_status_low_next,irq_tx_fault_status_low_current));

		}
	}
	else if(!(m_data & CPLD_MASTER_INTERRUPT_CPLD2))
	{
		cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_INTERRUPT_STATUS_H_REG,&data_high8);
		cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_INTERRUPT_STATUS_L_REG,&data_low8);
		data_16 = data_low8 | data_high8 << 8;
		if(
			!(data_16 & CPLD_SLAVE2_INTERRUPT_PRESENT32) ||
			!(data_16 & CPLD_SLAVE2_INTERRUPT_PRESENT40) ||
			!(data_16 & CPLD_SLAVE2_INTERRUPT_PRESENT48)
		)
		{
			if(!(data_16 & CPLD_SLAVE2_INTERRUPT_PRESENT32))
			{
				DEB2(printk("CPLD_SLAVE2_PRESENT32_REG\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_PRESENT32_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_present_status_high_current,0,status);
			}
			else if(!(data_16 & CPLD_SLAVE2_INTERRUPT_PRESENT40))
			{
				DEB2(printk("CPLD_SLAVE2_PRESENT40_REG\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_PRESENT40_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_present_status_high_current,8,status);
			}
			else if(!(data_16 & CPLD_SLAVE2_INTERRUPT_PRESENT48))
			{
				DEB2(printk("CPLD_SLAVE2_INTERRUPT_PRESENT48\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_PRESENT48_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_present_status_high_current,16,status);
			}
		}

		if(
			!(data_16 & CPLD_SLAVE2_INTERRUPT_RX_LOST32) ||
			!(data_16 & CPLD_SLAVE2_INTERRUPT_RX_LOST40) ||
			!(data_16 & CPLD_SLAVE2_INTERRUPT_RX_LOST48)
		)
		{
			if(!(data_16 & CPLD_SLAVE2_INTERRUPT_RX_LOST32))
			{
				DEB2(printk("CPLD_SLAVE2_INTERRUPT_RX_LOST32\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_RX_LOST32_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_rx_lost_status_high_current,0,status);
			}
			else if(!(data_16 & CPLD_SLAVE2_INTERRUPT_RX_LOST40))
			{
				DEB2(printk("CPLD_SLAVE2_INTERRUPT_PRESENT40\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_RX_LOST40_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_rx_lost_status_high_current,8,status);
			}
			else if(!(data_16 & CPLD_SLAVE2_INTERRUPT_RX_LOST48))
			{
				DEB2(printk("CPLD_SLAVE2_INTERRUPT_PRESENT48\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_RX_LOST48_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_rx_lost_status_high_current,16,status);
			}

		}

		if(
			!(data_16 & CPLD_SLAVE2_INTERRUPT_TX_FAULT32) ||
			!(data_16 & CPLD_SLAVE2_INTERRUPT_TX_FAULT40) ||
			!(data_16 & CPLD_SLAVE2_INTERRUPT_TX_FAULT48)
		)
		{
			if(!(data_16 & CPLD_SLAVE2_INTERRUPT_TX_FAULT32))
			{
				DEB2(printk("CPLD_SLAVE2_INTERRUPT_RX_LOST32\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_TX_FAULT32_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_tx_fault_status_high_current,0,status);
			}
			else if(!(data_16 & CPLD_SLAVE2_INTERRUPT_TX_FAULT40))
			{
				DEB2(printk("CPLD_SLAVE2_INTERRUPT_PRESENT40\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_TX_FAULT40_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_tx_fault_status_high_current,8,status);
			}
			else if(!(data_16 & CPLD_SLAVE2_INTERRUPT_TX_FAULT48))
			{
				DEB2(printk("CPLD_SLAVE2_INTERRUPT_PRESENT48\r\n"));
				cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_TX_FAULT48_REG,&status);
				RANGE_OF_BYTE_SHIFT(irq_tx_fault_status_high_current,16,status);
			}
		}

		if(!(data_16 & CPLD_SLAVE2_INTERRUPT_PRESENT56))
		{
			DEB2(printk("CPLD_SLAVE2_PRESENT56_REG\r\n"));
			cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_PRESENT56_REG,&status);
			irq_present_qsfp_current = status;
		}

		if(!(data_16 & CPLD_SLAVE2_INTERRUPT_QSFP_CR56))
		{
			DEB2(printk("CPLD_SLAVE2_QSFP_CR56_REG\r\n"));
			cig_cpld_read_slave_cpld_register(CPLD_SLAVE2_QSFP_CR56_REG,&status);
			irq_interrupt_qsfp_current = status;
		}
	}
	else if(!(m_data & CPLD_MASTER_INTERRUPT_LSW))
	{
		DEB2(printk("CPLD_MASTER_INTERRUPT_LSW\r\n"));
	}
	else if(!(m_data & CPLD_MASTER_INTERRUPT_PSU1))
	{
		DEB2(printk("CPLD_MASTER_INTERRUPT_PSU1\r\n"));
	}
	else if(!(m_data & CPLD_MASTER_INTERRUPT_PSU2))
	{
		DEB2(printk("CPLD_MASTER_INTERRUPT_PSU2\r\n"));
	}
	else if(!(m_data & CPLD_MASTER_INTERRUPT_6320))
	{
		DEB2(printk("CPLD_MASTER_INTERRUPT_6320\r\n"));
	}
	cig_cpld_write_slave_cpld_register(CPLD_SLAVE1_INTERRUPT_MASK_REG,0x0);
	cig_cpld_write_slave_cpld_register(CPLD_SLAVE2_INTERRUPT_MASK_REG,0x0);

	memset(tmp,0xff,sizeof(tmp));

	for(i = 0;i < 24;i++)
	{
		if(!(irq_present_status_low_current & (0x1 << i)) && (irq_present_status_low_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d is present\r\n",i+1));
			tmp[0] = 1;
		}
		else if((irq_present_status_low_current & (0x1 << i)) && !(irq_present_status_low_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d is absent\r\n",i+1));
			tmp[0] = 0;
		}

		if(!(irq_tx_fault_status_low_current & (0x1 << i)) && (irq_tx_fault_status_low_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d transmission is right\r\n",i+1));
			tmp[1] = 1;
		}
		else if((irq_tx_fault_status_low_current & (0x1 << i)) && !(irq_tx_fault_status_low_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d transmission is fault\r\n",i+1));
			tmp[1] = 0;
		}

		if(!(irq_rx_lost_status_low_current & (0x1 << i)) && (irq_rx_lost_status_low_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d optical is meet\r\n",i+1));
			tmp[2] = 1;
		}
		else if((irq_rx_lost_status_low_current & (0x1 << i)) && !(irq_rx_lost_status_low_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d optical is lost\r\n",i+1));
			tmp[2] = 0;
		}

		if((tmp[0] != 0xff) || (tmp[1] != 0xff) || (tmp[2] != 0xff))
		{
			memset(kmsg,0xff,sizeof(kmsg));
			snprintf(kmsg,sizeof(kmsg),"sfp%02d:%1d:%1d:%1d ",i+1,(tmp[0] == 0xff) ? 0:tmp[0],(tmp[1] == 0xff) ? 0:tmp[1],(tmp[2] == 0xff) ? 0:tmp[2]);
			break;
		}
	}

	memset(tmp,0xff,sizeof(tmp));
	for(i = 0;i < 24;i++)
	{
		if(!(irq_present_status_high_current & (0x1 << i)) && (irq_present_status_high_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d is present\r\n",i+25));
			tmp[0] = 1;
		}
		else if((irq_present_status_high_current & (0x1 << i)) && !(irq_present_status_high_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d is absent\r\n",i+25));
			tmp[0] = 0;

		}

		if(!(irq_rx_lost_status_high_current & (0x1 << i)) && (irq_rx_lost_status_high_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d optical is meet\r\n",i+25));
			tmp[1] = 1;
		}
		else if((irq_rx_lost_status_high_current & (0x1 << i)) && !(irq_rx_lost_status_high_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d optical is lost\r\n",i+25));
			tmp[1] = 0;
		}

		if(!(irq_tx_fault_status_high_current & (0x1 << i)) && (irq_tx_fault_status_high_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d transmission is right\r\n",i+25));
			tmp[2] = 1;
		}
		else if((irq_tx_fault_status_high_current & (0x1 << i)) && !(irq_tx_fault_status_high_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d transmission is fault\r\n",i+25));
			tmp[2] = 0;
		}

		if((tmp[0] != 0xff) || (tmp[1] != 0xff) || (tmp[2] != 0xff))
		{
			memset(kmsg,0xff,sizeof(kmsg));
			snprintf(kmsg,sizeof(kmsg),"sfp%02d:%1d:%1d:%1d ",i+25,(tmp[0] == 0xff) ? 0:tmp[0],(tmp[1] == 0xff) ? 0:tmp[1],(tmp[2] == 0xff) ? 0:tmp[2]);
			break;
		}
	}

	memset(tmp,0xff,sizeof(tmp));
	for(i = 0 ; i < 8; i++)
	{
		if(!(irq_present_qsfp_current & (0x1 << i)) && (irq_present_qsfp_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d is present\r\n",i+49));
			tmp[0] = 1;
		}
		else if((irq_present_qsfp_current & (0x1 << i)) && !(irq_present_qsfp_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d is absent\r\n",i+49));
			tmp[0] = 0;
		}

		if(!(irq_interrupt_qsfp_current & (0x1 << i)) && (irq_interrupt_qsfp_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d interrupt is occured \r\n",i+49));
			tmp[1] = 1;
		}
		else if((irq_interrupt_qsfp_current & (0x1 << i)) && !(irq_interrupt_qsfp_next & (0x1 << i)))
		{
			DEB2(printk("SFP%d interrupt is cleaned\r\n",i+49));
			tmp[1] = 0;
		}

		if((tmp[0] != 0xff) || (tmp[1] != 0xff))
		{
			memset(kmsg,0xff,sizeof(kmsg));
			snprintf(kmsg,sizeof(kmsg),"qsfp%02d:%1d:%1d:%1d ",i+49,(tmp[0] == 0xff) ? 0:tmp[0],(tmp[1] == 0xff) ? 0:tmp[1],0);
			break;
		}
	}


	irq_present_status_low_next = irq_present_status_low_current;
	irq_rx_lost_status_low_next = irq_rx_lost_status_low_current;
	irq_tx_fault_status_low_next = irq_tx_fault_status_low_current;
	irq_present_status_high_next = irq_present_status_high_current;
	irq_rx_lost_status_high_next = irq_rx_lost_status_high_current;
	irq_tx_fault_status_high_next = irq_tx_fault_status_high_current;
	irq_present_qsfp_next =  irq_present_qsfp_current;
	irq_interrupt_qsfp_next = irq_interrupt_qsfp_current;

    send_usrmsg(kmsg, strlen(kmsg));
}

static void disableIrq(unsigned short maskReg, unsigned short mask)
{
    u8 data = 0;

    data = lpc_iic_getbyte(NULL,maskReg);
    data |= mask;
    lpc_iic_setbyte(NULL,maskReg, data);
}

static void enableIrq(unsigned short maskReg, unsigned short mask)
{
    unsigned short data;

    data = lpc_iic_getbyte(NULL,maskReg);
    data &= ~mask;
    lpc_iic_setbyte(NULL,maskReg, data);
}


static irqreturn_t irq_inter_isr(int irq, void *handle)
{

	/*
	 * use keventd context to read the event fifo registers
	 * Schedule readout at least 25ms after notification for
	 * REVID < 4
	 */

	schedule_delayed_work(&irq_inter_work, irq_inter_delay);

	return IRQ_HANDLED;
}


#define CIG_CPLD_CHR_NAME "cpld"


static int __init cpld_init(void)
{
	int rval,rc=0;
	dev_t dev;
	u8 s_data;

	DEB2(printk("cpld_init\n");)

/**************************************************************************************/

	LPC_INDEX_REG = lpc_base_addr;
    LPC_DATA_REG  = lpc_base_addr + 1;

    cpld_device = kzalloc(sizeof(struct cpld_dev_type), GFP_KERNEL);
    if (!cpld_device)
        goto error3;
    cpld_device->io_resource = request_region(lpc_base_addr,
                                            lpc_io_space_size, "lpc-i2c");
    if (!cpld_device->io_resource) {
        printk("lpc: claim I/O resource fail\n");
        goto error2;
    }
    sema_init(&cpld_device->sem, 1);

    if (cpld_major) {
        dev = MKDEV(cpld_major, cpld_minor);
        rc = register_chrdev_region(dev, 1, CIG_CPLD_CHR_NAME);
    } else {
        rc = alloc_chrdev_region(&dev, cpld_major, 1, CIG_CPLD_CHR_NAME);
        cpld_major = MAJOR(dev);
    }

    cpld_setup_cdev(cpld_device);

    cpld_class = class_create(THIS_MODULE,CIG_CPLD_CHR_NAME);
    if (!cpld_class) {
        DEB2(printk("failed to create class\n");)
        goto error1;
    }

	cpld_class->p->subsys.kobj.ktype= &cpld_kobj_type;
	cpld_sysfs_add_attr(&cpld_class->p->subsys.kobj, "read");
	cpld_sysfs_add_attr(&cpld_class->p->subsys.kobj, "write");
	cpld_sysfs_add_attr(&cpld_class->p->subsys.kobj, "reads");
	cpld_sysfs_add_attr(&cpld_class->p->subsys.kobj, "writes");
	cpld_sysfs_add_attr(&cpld_class->p->subsys.kobj, "version");

    cpld_dev = device_create(cpld_class, NULL, dev, NULL, CIG_CPLD_CHR_NAME);

/**************************************************************************************/

	rval = lpc_bus_init();
	rval = lpc_register_driver(&i2c_lpc_driver, 1);

/**************************************************************************************/
	return 0;
error1:
    cdev_del(&cpld_device->cdev);
    unregister_chrdev_region(dev, 1);
error2:
    release_resource(cpld_device->io_resource);
error3:
    kfree(cpld_device);

	return rc;
}

static void __exit cpld_exit(void)
{
    DEB2(printk("cpld_exit\n"));

    lpc_unregister_driver(&i2c_lpc_driver);
    lpc_bus_exit();
    dev_t devno = MKDEV(cpld_major, cpld_minor);

    cdev_del(&cpld_device->cdev);
    if (cpld_class) {
        device_destroy(cpld_class, devno);
        class_destroy(cpld_class);
    }

    if (cpld_device) {
        if (cpld_device->io_resource)
            release_resource(cpld_device->io_resource);

        kfree(cpld_device);
    }
	unregister_chrdev_region(devno, 1);


}

module_param(cpld_major, int, S_IRUGO);
module_param(cpld_minor, int, S_IRUGO);
module_param(i2c_debug, int, S_IRUGO);
module_param(board_id, int, S_IRUGO);

module_init(cpld_init);
module_exit(cpld_exit);

MODULE_AUTHOR("Zhang Peng <zhangpeng@cigtech.com>");
MODULE_DESCRIPTION("cs6436-54p-cpld driver");
MODULE_LICENSE("GPL");


/**********************************************   End  ********************************************************/



