/* -------------------------------------------------------------------- 

	 * A hwmon driver for the CIG cs5435-54P
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

/* --------------------------------------------------------------------	*/

#ifndef I2C_LPC_H
#define I2C_LPC_H 1

/* ----- Control register bits ----------------------------------------	*/
#define I2C_LPC_PIN	0x80
#define I2C_LPC_ESO	0x40
#define I2C_LPC_ES1	0x20
#define I2C_LPC_ES2	0x10
#define I2C_LPC_ENI	0x08

#define I2C_LPC_STO	0x40
#define I2C_LPC_ACK	0x01

/*command register*/
#define I2C_LPC_STA	0x80
#define I2C_LPC_ABT	0x40

/*status register*/
#define I2C_LPC_TBE 0x02
#define I2C_LPC_IBB 0x80
#define I2C_LPC_RBF 0x01
#define I2C_LPC_TD 0x08

#define I2C_LPC_START    I2C_LPC_STA
#define I2C_LPC_STOP     I2C_LPC_STO
#define I2C_LPC_REPSTART I2C_LPC_STA
#define I2C_LPC_IDLE     

/* ----- Status register bits -----------------------------------------	*/
/*#define I2C_LPC_PIN  0x80    as above*/

#define I2C_LPC_INI 0x40   /* 1 if not initialized */
#define I2C_LPC_STS 0x20
#define I2C_LPC_BER 0x10
#define I2C_LPC_AD0 0x08
#define I2C_LPC_LRB 0x08
#define I2C_LPC_AAS 0x04
#define I2C_LPC_LAB 0x02
#define I2C_LPC_BB  0x80

/* ----- Chip clock frequencies ---------------------------------------	*/
#define I2C_LPC_CLK3	0x00
#define I2C_LPC_CLK443	0x10
#define I2C_LPC_CLK6	0x14
#define I2C_LPC_CLK	    0x18
#define I2C_LPC_CLK12	0x1c

/* ----- transmission frequencies -------------------------------------	*/
#define I2C_LPC_TRNS90 0x00	/*  90 kHz */
#define I2C_LPC_TRNS45 0x01	/*  45 kHz */
#define I2C_LPC_TRNS11 0x02	/*  11 kHz */
#define I2C_LPC_TRNS15 0x03	/* 1.5 kHz */


#define I2C_LPC_OWNADR	0
#define I2C_LPC_INTREG	I2C_LPC_ES2
#define I2C_LPC_CLKREG	I2C_LPC_ES1

#define I2C_LPC_REG_TEST 0x01
#define I2C_LPC_REG_BUS_SEL 0x80
#define I2C_LPC_REG_DEVICE_ADDR 0x81
#define I2C_LPC_REG_BYTE_COUNT 0x83
#define I2C_LPC_REG_COMMAND 0x84
#define I2C_LPC_REG_STATUS 0x85
#define I2C_LPC_REG_DATA_RX1 0x86
#define I2C_LPC_REG_DATA_RX2 0x87
#define I2C_LPC_REG_DATA_RX3 0x88
#define I2C_LPC_REG_DATA_RX4 0x89
#define I2C_LPC_REG_DATA_TX1 0x8a
#define I2C_LPC_REG_DATA_TX2 0x8b
#define I2C_LPC_REG_DATA_TX3 0x8c
#define I2C_LPC_REG_DATA_TX4 0x8d


#define ADDR_REG_SFP_STATUS_ADDR 0X62 //reg addr +R/W#   //1031
#define ADDR_REG_SFP_STATUS_TX 0X63  // write data
#define ADDR_REG_SFP_STATUS_RX 0X64 //read data
#define ADDR_REG_SFP_STATUS_COMMAND 0X65 //cmd bit7=1,go
#define ADDR_REG_SFP_STATUS_STATUS 0X66 //status 

#define CPLD_MASTER_INTERRUPT_STATUS_REG 0x20
#define CPLD_MASTER_INTERRUPT_MASK_REG 0x21
#define CPLD_MASTER_INTERRUPT_ALL 0x3f
#define CPLD_MASTER_INTERRUPT_CPLD2 0x20 
#define CPLD_MASTER_INTERRUPT_CPLD1 0x10 
#define CPLD_MASTER_INTERRUPT_PSU2 0x08 
#define CPLD_MASTER_INTERRUPT_PSU1 0x04 
#define CPLD_MASTER_INTERRUPT_6320 0x02 
#define CPLD_MASTER_INTERRUPT_LSW 0x01 



#define CPLD_SLAVE1_INTERRUPT_STATUS_L_REG 0x20
#define CPLD_SLAVE1_INTERRUPT_STATUS_H_REG 0x21
#define CPLD_SLAVE2_INTERRUPT_STATUS_L_REG 0x22
#define CPLD_SLAVE2_INTERRUPT_STATUS_H_REG 0x23
#define CPLD_SLAVE1_INTERRUPT_MASK_REG 	   0x24
#define CPLD_SLAVE2_INTERRUPT_MASK_REG     0x25


#define CPLD_SLAVE1_PRESENT08_REG 0x01
#define CPLD_SLAVE1_PRESENT16_REG 0x02
#define CPLD_SLAVE1_PRESENT24_REG 0x03
#define CPLD_SLAVE2_PRESENT32_REG 0x04
#define CPLD_SLAVE2_PRESENT40_REG 0x05
#define CPLD_SLAVE2_PRESENT48_REG 0x06

#define CPLD_SLAVE1_RX_LOST08_REG 0x07
#define CPLD_SLAVE1_RX_LOST16_REG 0x08
#define CPLD_SLAVE1_RX_LOST24_REG 0x09
#define CPLD_SLAVE2_RX_LOST32_REG 0x0a
#define CPLD_SLAVE2_RX_LOST40_REG 0x0b
#define CPLD_SLAVE2_RX_LOST48_REG 0x0c

#define CPLD_SLAVE1_TX_FAULT08_REG 0x0d
#define CPLD_SLAVE1_TX_FAULT16_REG 0x0e
#define CPLD_SLAVE1_TX_FAULT24_REG 0x0f
#define CPLD_SLAVE2_TX_FAULT32_REG 0x10
#define CPLD_SLAVE2_TX_FAULT40_REG 0x11
#define CPLD_SLAVE2_TX_FAULT48_REG 0x12

#define CPLD_SLAVE2_PRESENT56_REG 0x19
#define CPLD_SLAVE2_QSFP_CR56_REG 0x1a


#define CPLD_SLAVE1_INTERRUPT_PRESENT08 0x0001
#define CPLD_SLAVE1_INTERRUPT_PRESENT16 0x0002
#define CPLD_SLAVE1_INTERRUPT_PRESENT24 0x0004
#define CPLD_SLAVE2_INTERRUPT_PRESENT32 0x0001
#define CPLD_SLAVE2_INTERRUPT_PRESENT40 0x0002
#define CPLD_SLAVE2_INTERRUPT_PRESENT48 0x0004

#define CPLD_SLAVE2_INTERRUPT_QSFP_CR56 0x0200
#define CPLD_SLAVE2_INTERRUPT_PRESENT56 0x0400

#define CPLD_SLAVE1_INTERRUPT_RX_LOST08 0x0008
#define CPLD_SLAVE1_INTERRUPT_RX_LOST16 0x0010
#define CPLD_SLAVE1_INTERRUPT_RX_LOST24 0x0020
#define CPLD_SLAVE2_INTERRUPT_RX_LOST32 0x0008
#define CPLD_SLAVE2_INTERRUPT_RX_LOST40 0x0010
#define CPLD_SLAVE2_INTERRUPT_RX_LOST48 0x0020

#define CPLD_SLAVE1_INTERRUPT_TX_FAULT08 0x0040
#define CPLD_SLAVE1_INTERRUPT_TX_FAULT16 0x0080
#define CPLD_SLAVE1_INTERRUPT_TX_FAULT24 0x0100
#define CPLD_SLAVE2_INTERRUPT_TX_FAULT32 0x0040
#define CPLD_SLAVE2_INTERRUPT_TX_FAULT40 0x0080
#define CPLD_SLAVE2_INTERRUPT_TX_FAULT48 0x0100







#define WAIT_TIME_OUT_COUNT 100


struct i2c_algo_lpc_data {
	void *data;		/* private data for lolevel routines	*/
	void (*setlpc) (void *data, int ctl, int val);
	int  (*getlpc) (void *data, int ctl);
	int  (*getown) (void *data);
	int  (*getclock) (void *data);
	void (*waitforpin) (void *data);

	int (*xfer_begin) (void *data);
	int (*xfer_end) (void *data);

	/* Multi-master lost arbitration back-off delay (msecs)
	 * This should be set by the bus adapter or knowledgable client
	 * if bus is multi-mastered, else zero
	 */
	unsigned long lab_mdelay;
};


struct subsys_private {
	struct kset subsys;
	struct kset *devices_kset;
	struct list_head interfaces;
	struct mutex mutex;

	struct kset *drivers_kset;
	struct klist klist_devices;
	struct klist klist_drivers;
	struct blocking_notifier_head bus_notifier;
	unsigned int drivers_autoprobe:1;
	struct bus_type *bus;

	struct kset glue_dirs;
	struct class *class;
};

void cs5435_54p_sysfs_add_client(struct i2c_client *client);
void cs5435_54p_sysfs_remove_client(struct i2c_client *client);


#endif /* I2C_LPC8584_H */
