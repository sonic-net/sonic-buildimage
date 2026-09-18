/*
 *  COPYRIGHT (c) 2012 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/** @file oc_i2c_port.c
 * OpenCores I2C Master Controller Access Routines.
 * This implements the Access Layer to the OpenCores I2C controller that is used by
 * the XO2 ECA commands to read/write bytes across the I2C link to the XO2.
 * The specific read/write routines are wrappers around the MSB OpenCores
 * driver functions to map the generic call from a ECA command to the
 * specific parameters needed by the OpenCores I2C routines, such as
 * device context and slave address.
 * <p>
 * All routines will print run-time diagnostic information if compiled with the
 * define DEBUG_I2C.
 * <P>
 * A note on I2C addressing.  The XO2 configuration logic can be accessed in user mode by setting
 * the lower 2 bits of the address to 2'b00.  The Primary I2C Port needs to be enabled in the EFB and
 * the EFB needs to be instantiated in the design.  Using the Config address (7'bxxxxx00) will direct
 * the I2C transfers to the configuration mode and not the Wishbone interface.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "debug.h"
#include "cpld_i2c_port.h"
#include "systypes.h"
#include "xo2_dev.h"
#include "eca_drvr.h"

int cpld_i2c_fd_open(int i2cbus)
{
    int file, ret;
    char filename[I2C_MAX_NAME_SIZE];

    ret = snprintf(filename, I2C_MAX_NAME_SIZE, "/dev/i2c-%d", i2cbus);
    filename[ret] = '\0';
    if ((file = open(filename, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        fprintf(stderr, "Error: Could not open file "
                "`%s': %s\n", filename, strerror(errno));
    }

    return file;
}

void cpld_i2c_fd_close(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}

int cpld_i2c_transfer(int fd, struct i2c_rdwr_ioctl_data *i2cinfo)
{
    int ret;

    ret = ioctl(fd, I2C_RDWR, i2cinfo);
    if (ret < 0) {
        printf("cpld_i2c_transfer ioctl failed ret %d.\n", ret);
        return ERROR;
    }

    return OK;
}

void MicoSleepMilliSecs(unsigned int msec)
{
    usleep(msec * 1000);
}

void MicoSleepMicroSecs(unsigned int usec)
{
    usleep(usec);
}
