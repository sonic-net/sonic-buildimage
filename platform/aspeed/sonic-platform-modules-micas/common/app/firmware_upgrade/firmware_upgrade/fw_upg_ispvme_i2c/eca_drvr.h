/*
 *  COPYRIGHT (c) 2012 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/** @file ECA_drvr.h
 * The Embedded Configuration Access APIs and Commands invoke user
 * supplied driver functions in order to perform the actual read
 * and write bytes over the hardware interface.  The ECADrvrCalls_t
 * structure, defined here, holds the 6 access layer driver functions
 * that the user needs to supply.
 * <p>
 * <ul> User Supplied Driver Functions
 * <li> CmdStart - do anything necessary to start the transfer of a byte(s) on
 *      the interface.  For I2C this is asserting the START sequence.
 *      This will be called before starting a CmdRead/CmdWrite.
 * <li> CmdStop - do anything necessary to end the transfer of a byte(s) on
 *      the interface.  For I2C this is asserting the STOP sequence.
 *      This will be called after a CmdRead/CmdWrite ends.
 * <li> CmdRead - read a number of bytes, on the interface, from the XO2
 * <li> CmdWrite - read a number of bytes, on the interface, from the XO2
 * <li> mSecDelay - implement a milliSec delay function on this platform.
 *      Used for timing delays in operations, like waiting for erase to complete.
 * <li> uSecDelay- implement a microSec delay function on this platform.
 *      Used for timing delays in operations, like waiting for page write to complete.
 * </ul>
  * <p>
 * While the intention is to have the XO2 Embedded Config Access (ECA)
 * routines independent of hardware interface to the XO2 config logic,
 * there is a bias towards I2C access since this is the first interface
 * implemented and tested.
 * <p>
 * An example of implementing and populating the ECADrvrCalls_t structure.
 * @code
    ECADrvrCalls_t i2c_drvr;

    i2c_drvr.pfCmdStart = &oc_i2c_start;
    i2c_drvr.pfCmdStop = &oc_i2c_stop;
    i2c_drvr.pfCmdRead = &oc_i2c_read;
    i2c_drvr.pfCmdWrite = &oc_i2c_write;
    i2c_drvr.pfmSecDelay = &MicoSleepMilliSecs;
    i2c_drvr.pfuSecDelay = &MicoSleepMicroSecs;

    @endcode
 *
 *
 */

#ifndef LATTICE_ECA_DRVR_H
#define LATTICE_ECA_DRVR_H
#include <linux/types.h>

/**
 * Definition of driver function signatures that must be provided and registered with
 * an instance of XO2Handle_t for access to a specific XO2 device.
 * The user initializes an instance of this structure with their specific driver function
 * addresses.
 * <p>
 * pDrvrParams is a pointer to any additional information (user defined structure) that
 * may be required for their driver to implement the specific call.  For example, the
 * XO2 ECA commands are not written to support a specific communication link, such as I2C.
 * They are generic, and independent of the link (SPI, I2C, JTAG, etc.) used to communicate
 * to the XO2.  Therefore pDrvrParams would be used to hold link specific information such
 * as an I2C slave address or other specifics not expected/supported by the generic ECA command routines.
 */

struct i2c_msg {
	__u16 addr;	/* slave address			*/
	__u16 flags;
#define I2C_M_TEN		0x0010	/* this is a ten bit chip address */
#define I2C_M_RD		0x0001	/* read data, from slave to master */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
	__u16 len;		/* msg length				*/
	__u8 *buf;		/* pointer to msg data			*/
};

struct i2c_rdwr_ioctl_data {
    struct i2c_msg *msgs;    /* pointers to i2c_msgs */
    __u32 nmsgs;                    /* number of i2c_msgs */
};

typedef struct
{
    int     fd;
    int     (*pfCmdOpenfile)(int i2cbus);
    void    (*pfCmdClosefile)(int fd);
    int     (*pfCmdTransfer)(int fd, struct i2c_rdwr_ioctl_data *info);

    /** Pointer to function to implement a milliSec delay function on this platform. */
    void    (*pfmSecDelay)(unsigned int msec);

    /** Pointer to function to implement a microSec delay function on this platform. */
    void    (*pfuSecDelay)(unsigned int usec);
} ECADrvrCalls_t;

#endif
