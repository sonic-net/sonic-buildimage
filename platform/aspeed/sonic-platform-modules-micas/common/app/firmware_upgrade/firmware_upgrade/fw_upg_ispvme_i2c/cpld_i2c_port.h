/*
 *  COPYRIGHT (c) 2012 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/** @file oc_i2c_port.h
 * OpenCores I2C Master Controller Access Routines.
 * This file provides function prototypes for the OpenCores I2C wrapper routines,
 * and a definition of the structure to be passed to the routines that holds
 * specific parameters needed by the OpenCores I2C routines, such as
 * device context and slave address.  @see OC_I2C_t
 */

#ifndef LATTICE_OC_I2C_PORT_H
#define LATTICE_OC_I2C_PORT_H

#include "eca_drvr.h"
#include "xo2_dev.h"
/*#include "OpenCoresI2CMaster.h"*/
#define I2C_MAX_NAME_SIZE           (128)
#define I2C_RDWR                    (0x0707)
/**
 * Structure to hold device access specific information when using
 * the MSB OpenCores I2C drivers.  An instance of this structure needs
 * to exist for each XO2 I2C device that will be accessed.  The 7 bit I2C address
 * of the specific XO2 is stored here, along with the reference to the specific OpenCores
 * I2C controller to use for the access.
 * The reference to this structure is held in the pDrvrParams in XO2Handle_t.
 *
 */
typedef struct
{
    unsigned int        masterBus;
    unsigned int        slaveAddr;   /**< 7 bit I2C address of XO2 device */
    XO2Devices_t        cpld_type;
    int                 ProgramMode;
    int                 VerifyMode;
    XO2_JEDEC_t         *pCfgBits;  // pointer to the JEDEC confgiruation data to load into XO2
    int                 designNum;
    char                *file_path;

    /*OpenCoresI2CMasterCtx_t *i2c_ctx;*/  /**< specific I2C controller instance */
} OC_I2C_t;

////////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
////////////////////////////////////////////////////////////////////////////////
int cpld_i2c_transfer(int fd, struct i2c_rdwr_ioctl_data *i2cinfo);
int cpld_i2c_fd_open(int i2cbus);
void cpld_i2c_fd_close(int fd);

void MicoSleepMilliSecs(unsigned int msec);
void MicoSleepMicroSecs(unsigned int usec);

#endif
