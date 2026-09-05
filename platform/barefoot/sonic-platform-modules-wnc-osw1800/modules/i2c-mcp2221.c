/*
 * i2c bus driver for MCP2221
 *
 * Derived from:
 *  i2c-tiny-usb.c
 *  i2c-diolan-u2c.c
 *  usb-serial.c
 *  onetouch.c
 *  usb-skeleton.c
 *
 * Copyright (C) 2014 Microchip Technology Inc.
 *
 * Author: Bogdan Bolocan http://www.microchip.com/support
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/usb.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#define DRIVER_NAME		"i2c-mcp2221"

#define USB_VENDOR_ID_MCP2221		0x04d8
#define USB_DEVICE_ID_MCP2221		0x00dd

#define MCP2221_OUTBUF_LEN	64	/* USB write packet length */
#define MCP2221_INBUF_LEN	64	/* USB read packet length */

#define MCP2221_MAX_I2C_DATA_LEN	60

//#define MCP2221_FREQ_STD		100000
#define MCP2221_FREQ_STD		400000
//#define MCP2221_FREQ_STD		50000
#define MCP2221_FREQ_MAX		500000

#define MCP2221_RETRY_MAX		50
#define MCP2221_STD_DELAY_MS	1
//#define MCP2221_STD_DELAY_MS	2

#define RESP_ERR_NOERR			0x00
#define RESP_ADDR_NACK			0x25
#define RESP_READ_ERR			0x7F
#define RESP_READ_COMPL			0x55
#define RESP_I2C_IDLE			0x00
#define RESP_I2C_START_TOUT		0x12
#define RESP_I2C_RSTART_TOUT	0x17
#define RESP_I2C_WRADDRL_TOUT	0x23
#define RESP_I2C_WRADDRL_WSEND	0x21
#define RESP_I2C_WRADDRL_NACK	0x25
#define RESP_I2C_WRDATA_TOUT	0x44
#define RESP_I2C_RDDATA_TOUT	0x52
#define RESP_I2C_STOP_TOUT		0x62

#define CMD_MCP2221_STATUS		0x10
#define SUBCMD_STATUS_CANCEL	0x10
#define SUBCMD_STATUS_SPEED		0x20
#define MASK_ADDR_NACK			0x40

#define CMD_MCP2221_RDDATA7		0x91
#define CMD_MCP2221_GET_RDDATA	0x40

#define CMD_MCP2221_WRDATA7		0x90

/* Structure to hold all of our device specific stuff */
struct i2c_mcp2221 {
	u8 obuffer[MCP2221_OUTBUF_LEN];	/* USB write buffer */
	u8 ibuffer[MCP2221_INBUF_LEN];	/* USB read buffer */
	/* I2C/SMBus data buffer */
	u8 user_data_buffer[MCP2221_MAX_I2C_DATA_LEN];
	int ep_in, ep_out;              /* Endpoints    */
	struct usb_device *usb_dev;	/* the usb device for this device */
	struct usb_interface *interface;/* the interface for this device */
	struct i2c_adapter adapter;	/* i2c related things */
	uint frequency;			/* I2C/SMBus communication frequency */
	/* Mutex for low-level USB transactions */
	struct mutex mcp2221_usb_op_lock;
	/* wq to wait for an ongoing read/write */
	wait_queue_head_t usb_urb_completion_wait;
	bool ongoing_usb_ll_op;		/* a ll is in progress */

	struct urb *interrupt_in_urb;
	struct urb *interrupt_out_urb;
};

static uint frequency = MCP2221_FREQ_STD;	/* I2C clock frequency in Hz */

module_param(frequency, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(frequency, "I2C clock frequency in hertz");

/* usb layer */


/*
 * Return list of supported functionality.
 */
static u32 mcp2221_usb_func(struct i2c_adapter *a)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL |
	       I2C_FUNC_SMBUS_READ_BLOCK_DATA | I2C_FUNC_SMBUS_BLOCK_PROC_CALL;
}

static void mcp2221_usb_cmpl_cbk(struct urb *urb)
{
	struct i2c_mcp2221 *dev = urb->context;
	int status = urb->status;
	int retval;

	switch (status) {
	case 0:			/* success */
		break;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	/* -EPIPE:  should clear the halt */
	default:		/* error */
		goto resubmit;
	}

	/* wake up the waitting function
	modify the flag indicating the ll status */
	dev->ongoing_usb_ll_op = 0;
	wake_up_interruptible(&dev->usb_urb_completion_wait);
	return;

resubmit:
	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval) {
		dev_err(&dev->interface->dev,
			"mcp2221(irq): can't resubmit intrerrupt urb, retval %d\n",
			retval);
	}
}

static int mcp2221_ll_cmd(struct i2c_mcp2221 *dev)
{
	int rv;

	/* tell everybody to leave the URB alone */
	dev->ongoing_usb_ll_op = 1;

	/* submit the interrupt out ep packet */
	if (usb_submit_urb(dev->interrupt_out_urb, GFP_KERNEL)) {
		dev_err(&dev->interface->dev,
				"mcp2221(ll): usb_submit_urb intr out failed\n");
		dev->ongoing_usb_ll_op = 0;
		return -EIO;
	}

	/* wait for its completion */
	rv = wait_event_interruptible(dev->usb_urb_completion_wait,
			(!dev->ongoing_usb_ll_op));
	if (rv < 0) {
		dev_err(&dev->interface->dev, "mcp2221(ll): wait interrupted\n");
		goto ll_exit_clear_flag;
	}

	/* tell everybody to leave the URB alone */
	dev->ongoing_usb_ll_op = 1;

	/* submit the interrupt in ep packet */
	if (usb_submit_urb(dev->interrupt_in_urb, GFP_KERNEL)) {
		dev_err(&dev->interface->dev, "mcp2221(ll): usb_submit_urb intr in failed\n");
		dev->ongoing_usb_ll_op = 0;
		return -EIO;
	}

	/* wait for its completion */
	rv = wait_event_interruptible(dev->usb_urb_completion_wait,
			(!dev->ongoing_usb_ll_op));
	if (rv < 0) {
		dev_err(&dev->interface->dev, "mcp2221(ll): wait interrupted\n");
		goto ll_exit_clear_flag;
	}

ll_exit_clear_flag:
	dev->ongoing_usb_ll_op = 0;
	return rv;
}

static int mcp2221_init(struct i2c_mcp2221 *dev)
{
	int ret;

	ret = 0;
	if (frequency > MCP2221_FREQ_MAX)
		frequency = MCP2221_FREQ_MAX;

	/* initialize the MCP2221 and bring it to "idle/ready" state */
	dev_info(&dev->interface->dev,
		 "MCP2221 at USB bus %03d address %03d Freq=%dKhz-- mcp2221_init()\n", 
		 dev->usb_dev->bus->busnum, dev->usb_dev->devnum, frequency/1000);

	/* initialize unlocked mutex */
	mutex_init(&dev->mcp2221_usb_op_lock);

	dev->interrupt_out_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->interrupt_out_urb)
		goto init_error;

	usb_fill_int_urb(dev->interrupt_out_urb, dev->usb_dev,
				usb_sndintpipe(dev->usb_dev,
						  dev->ep_out),
				(void *)&dev->obuffer, MCP2221_OUTBUF_LEN,
				mcp2221_usb_cmpl_cbk, dev,
				1);

	dev->interrupt_in_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->interrupt_in_urb)
		goto init_error;

	usb_fill_int_urb(dev->interrupt_in_urb, dev->usb_dev,
				usb_rcvintpipe(dev->usb_dev,
						dev->ep_in),
				(void *)&dev->ibuffer, MCP2221_INBUF_LEN,
				mcp2221_usb_cmpl_cbk, dev,
				1);
	ret = 0;
	goto init_no_error;

init_error:
	dev_err(&dev->interface->dev, "mcp2221_init: Error = %d\n", ret);
	ret = -ENODEV;

init_no_error:
	dev_info(&dev->interface->dev, "mcp2221_init: Success\n");
	return ret;
}

static int mcp2221_i2c_readwrite(struct i2c_mcp2221 *dev,
					struct i2c_msg *pmsg)
{
	u8 ucI2cDiv, ucCancelXfer, ucXferLen;
	int rv, retries;
	unsigned int sleepCmd;
	u8 *pSrc, *pDst, usbCmdStatus;

	retries = 0;
	ucCancelXfer = 0;
	/* clock divider for I2C operations */
	ucI2cDiv = (u8)((12000000/frequency) - 3);

	/* determine the best delay value here */
	/* (MCP2221_STD_DELAY_MS * MCP2221_FREQ_MAX)/frequency; */
	sleepCmd = MCP2221_STD_DELAY_MS;

	if (pmsg->len > MCP2221_MAX_I2C_DATA_LEN)
		return -EINVAL;

readwrite_reinit:
	dev->obuffer[0] = CMD_MCP2221_STATUS; /* code for STATUS cmd */
	dev->obuffer[1] = 0x00;
	dev->obuffer[2] = ucCancelXfer; /* cancel subcmd */
	dev->obuffer[3] = SUBCMD_STATUS_SPEED; /* set the xfer speed */
	dev->obuffer[4] = ucI2cDiv;
	dev->obuffer[5] = 0x00;
	dev->obuffer[6] = 0x00;
	dev->obuffer[7] = 0x00;

	rv = mcp2221_ll_cmd(dev);
	if (rv < 0)
		return -EFAULT;

	if (dev->ibuffer[1] != RESP_ERR_NOERR)
		return -EFAULT;

	if (dev->ibuffer[3] != SUBCMD_STATUS_SPEED) {
		/* the speed could not be set - wait a while and retry */
		if (retries < MCP2221_RETRY_MAX) {
			/* wait a while and retry the operation */
			retries++;
			msleep(MCP2221_STD_DELAY_MS);
			ucCancelXfer = SUBCMD_STATUS_CANCEL;
			goto readwrite_reinit;
		} else {
			/* max number of retries was reached - return error */
			dev_err(&dev->interface->dev,
				"mcp2221 CANCEL ERROR:retries = %d\n", retries);
			return -EFAULT;
		}
	}

	if (pmsg->flags & I2C_M_RD) {
		/* I2C read */
		ucXferLen = (u8)pmsg->len;
		dev->obuffer[0] = CMD_MCP2221_RDDATA7;
		dev->obuffer[1] = ucXferLen; /* LSB of the xfer length */
		dev->obuffer[2] = 0; /* no MSB for the xfer length */
		/* address in 8-bit format */
		dev->obuffer[3] = (u8)((pmsg->addr) << 1);

		rv = mcp2221_ll_cmd(dev);
		if (rv < 0)
			return -EFAULT;

		if (dev->ibuffer[1] != RESP_ERR_NOERR)
			return -EFAULT;

		retries = 0;
		dev->obuffer[0] = CMD_MCP2221_GET_RDDATA;
		dev->obuffer[1] = 0x00;
		dev->obuffer[2] = 0x00;
		dev->obuffer[3] = 0x00;

		while (retries < MCP2221_RETRY_MAX) {
			msleep(sleepCmd);

			rv = mcp2221_ll_cmd(dev);
			if (rv < 0)
				return -EFAULT;

			if (dev->ibuffer[1] != RESP_ERR_NOERR)
				return -EFAULT;

			if (dev->ibuffer[2] == RESP_ADDR_NACK)
				return -EFAULT;

			/* break the loop - cmd ended ok - used for bus scan */
			if ((dev->ibuffer[3] == 0x00) &&
				(dev->ibuffer[2] == 0x00))
				break;

			if (dev->ibuffer[3] == RESP_READ_ERR) {
				retries++;
				continue;
			}

			if ((dev->ibuffer[2] == RESP_READ_COMPL) &&
				(dev->ibuffer[3] == ucXferLen)) {
				/* we got the data - copy it */
				pSrc = (u8 *)&dev->ibuffer[4];
				pDst = (u8 *)&pmsg->buf[0];
				memcpy(pDst, pSrc, ucXferLen);

				if (pmsg->flags & I2C_M_RECV_LEN)
					pmsg->len = ucXferLen;

				break;
			}

		}
		if (retries >= MCP2221_RETRY_MAX)
			return -EFAULT;
	} else {
		/* I2C write */
		ucXferLen = (u8)pmsg->len;
		dev->obuffer[0] = CMD_MCP2221_WRDATA7;
		dev->obuffer[1] = ucXferLen; /* LSB of the xfer length */
		dev->obuffer[2] = 0; /* no MSB for the xfer length */
		/* address in 8-bit format */
		dev->obuffer[3] = (u8)((pmsg->addr) << 1);
		/* copy the data we've read back */
		pSrc = (u8 *)&pmsg->buf[0];
		pDst = (u8 *)&dev->obuffer[4];
		memcpy(pDst, pSrc, ucXferLen);

		retries = 0;

		while (retries < MCP2221_RETRY_MAX) {
			rv = mcp2221_ll_cmd(dev);
			if (rv < 0)
				return -EFAULT;

			if (dev->ibuffer[1] != RESP_ERR_NOERR) {
				usbCmdStatus = dev->ibuffer[2];
				if (usbCmdStatus == RESP_I2C_START_TOUT)
						return -EFAULT;

				if (usbCmdStatus == RESP_I2C_WRADDRL_TOUT)
						return -EFAULT;

				if (usbCmdStatus == RESP_I2C_WRADDRL_NACK)
						return -EFAULT;

				if (usbCmdStatus == RESP_I2C_WRDATA_TOUT)
						return -EFAULT;

				if (usbCmdStatus == RESP_I2C_STOP_TOUT)
						return -EFAULT;

				msleep(sleepCmd);
				retries++;
				continue;
			} else { /* command completed successfully */
				break;
			}
		}
		if (retries >= MCP2221_RETRY_MAX)
			return -EFAULT;

		/* now, prepare for the STATUS stage */
		retries = 0;
		dev->obuffer[0] = CMD_MCP2221_STATUS; /* code for STATUS cmd */
		dev->obuffer[1] = 0x00;
		dev->obuffer[2] = 0x00;
		dev->obuffer[3] = 0x00;
		dev->obuffer[4] = 0x00;
		dev->obuffer[5] = 0x00;
		dev->obuffer[6] = 0x00;
		dev->obuffer[7] = 0x00;

		while (retries < MCP2221_RETRY_MAX) {
			rv = mcp2221_ll_cmd(dev);
			if (rv < 0)
				return -EFAULT;

			if (dev->ibuffer[1] != RESP_ERR_NOERR)
				return -EFAULT;

			/* i2c slave address was nack-ed */
			if (dev->ibuffer[20] & MASK_ADDR_NACK)
				return -EFAULT;

			usbCmdStatus = dev->ibuffer[8];
			if (usbCmdStatus == RESP_I2C_IDLE)
				break;

			if (usbCmdStatus == RESP_I2C_START_TOUT)
				return -EFAULT;

			if (usbCmdStatus == RESP_I2C_WRADDRL_TOUT)
				return -EFAULT;

			if (usbCmdStatus == RESP_I2C_WRADDRL_WSEND)
				return -EFAULT;

			if (usbCmdStatus == RESP_I2C_WRADDRL_NACK)
				return -EFAULT;

			if (usbCmdStatus == RESP_I2C_WRDATA_TOUT)
				return -EFAULT;

			if (usbCmdStatus == RESP_I2C_STOP_TOUT)
				return -EFAULT;

			msleep(sleepCmd);
			retries++;
		}
		if (retries >= MCP2221_RETRY_MAX)
			return -EFAULT;
	}

	return 0;
}

/* device layer */
static int mcp2221_usb_i2c_xfer(struct i2c_adapter *adap,
		struct i2c_msg *msgs, int num)
{
	struct i2c_mcp2221 *dev = i2c_get_adapdata(adap);
	struct i2c_msg *pmsg;
	int ret, count;

	for (count = 0; count < num; count++) {
		pmsg = &msgs[count];
		/* no concurrent users of the mcp2221 i2c xfer */
		ret = mutex_lock_interruptible(&dev->mcp2221_usb_op_lock);
		if (ret < 0)
			goto abort;

		ret = mcp2221_i2c_readwrite(dev, pmsg);
		mutex_unlock(&dev->mcp2221_usb_op_lock);
		if (ret < 0)
			goto abort;
	}

	/* if all the messages were transferred ok, return "num" */
	ret = num;

abort:
	return ret;
}

static const struct i2c_algorithm mcp2221_usb_algorithm = {
	.master_xfer = mcp2221_usb_i2c_xfer,
	.functionality = mcp2221_usb_func,
};

static const struct usb_device_id mcp2221_table[] = {
	{ USB_DEVICE(USB_VENDOR_ID_MCP2221, USB_DEVICE_ID_MCP2221) },
	{ }
};

MODULE_DEVICE_TABLE(usb, mcp2221_table);

static void mcp2221_free(struct i2c_mcp2221 *dev)
{
	usb_put_dev(dev->usb_dev);
	kfree(dev);
}

static int mcp2221_probe(struct usb_interface *interface,
			    const struct usb_device_id *id)
{
	struct usb_host_interface *hostif = interface->cur_altsetting;
	struct i2c_mcp2221 *dev;
	int ret;

	if ((hostif->desc.bInterfaceNumber != 2)
	    || (hostif->desc.bInterfaceClass != 3)) {
		pr_info("i2c-mcp2221(probe): Interface doesn't match the MCP2221 HID\n");
		return -ENODEV;
	}

	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		pr_info("i2c-mcp2221(probe): no memory for device state\n");
		ret = -ENOMEM;
		goto error;
	}

	dev->ep_in = hostif->endpoint[0].desc.bEndpointAddress;
	dev->ep_out = hostif->endpoint[1].desc.bEndpointAddress;

	dev->usb_dev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	init_waitqueue_head(&dev->usb_urb_completion_wait);

	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);

	/* setup i2c adapter description */
	dev->adapter.owner = THIS_MODULE;
	dev->adapter.class = I2C_CLASS_HWMON;
	dev->adapter.algo = &mcp2221_usb_algorithm;
	i2c_set_adapdata(&dev->adapter, dev);

	snprintf(dev->adapter.name, sizeof(dev->adapter.name),
		 DRIVER_NAME " at bus %03d device %03d",
		 dev->usb_dev->bus->busnum, dev->usb_dev->devnum);

	dev->adapter.dev.parent = &dev->interface->dev;

	/* initialize mcp2221 i2c interface */
	ret = mcp2221_init(dev);
	if (ret < 0) {
		dev_err(&interface->dev, "failed to initialize adapter\n");
		goto error_free;
	}

	/* and finally attach to i2c layer */
	ret = i2c_add_adapter(&dev->adapter);
	if (ret < 0) {
		dev_err(&interface->dev, "failed to add I2C adapter\n");
		goto error_free;
	}

	dev_info(&dev->interface->dev,
			"mcp2221_probe() -> chip connected -> Success\n");
	return 0;

error_free:
	usb_set_intfdata(interface, NULL);
	mcp2221_free(dev);
error:
	return ret;
}

static void mcp2221_disconnect(struct usb_interface *interface)
{
	struct i2c_mcp2221 *dev = usb_get_intfdata(interface);

	i2c_del_adapter(&dev->adapter);

	usb_kill_urb(dev->interrupt_in_urb);
	usb_kill_urb(dev->interrupt_out_urb);
	usb_free_urb(dev->interrupt_in_urb);
	usb_free_urb(dev->interrupt_out_urb);

	usb_set_intfdata(interface, NULL);
	mcp2221_free(dev);

	pr_info("i2c-mcp2221(disconnect) -> chip disconnected");
}

static struct usb_driver mcp2221_driver = {
	.name = DRIVER_NAME,
	.probe = mcp2221_probe,
	.disconnect = mcp2221_disconnect,
	.id_table = mcp2221_table,
};

module_usb_driver(mcp2221_driver);

MODULE_AUTHOR("Bogdan Bolocan");
MODULE_DESCRIPTION(DRIVER_NAME "I2C MCP2221");
MODULE_LICENSE("GPL v2");
