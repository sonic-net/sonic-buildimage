/*
 *  COPYRIGHT (c) 2012 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/** @file XO2_cmds.c
 * The commands provided here allow low-level access to the operations of the XO2 configuration
 * logic. The functions are C implementations of the published XO2 configuration commands and their
 * operands and usage.  The commands usually need to be used in groups.  For example, some
 * commands require that the device be in configuration mode in order to issue a command.
 * The XO2 API's provide routines that bundle together the necessary sequence of commands
 * to implement the higher-level function.  @see XO2_api.c
 *
 * <p>
 * All functions take a pointer to an XO2 device structure.  This structure provides the list
 * of access functions and other information, such as I2C slave address.  The XO2 command functions do not
 * assume a particular access method
 * (i.e. they do not care if connected to XO2 by I2C or SPI or other method).
 */

#include <stdio.h>
#include "debug.h"

#include "xo2_cmds.h"
#include "cpld_i2c_port.h"

/**
 * Read the 4 byte Device ID from the XO2 Configuration logic block.
 * This function assembles the command sequence that allows reading the XO2 Device ID
 * from the configuration logic.  The command is first written to the XO2, then a read
 * of 4 bytes is perfromed to return the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 32 bit integer to return the ID value in
 * @return OK if successful, ERROR if failed to read
 *
 */
int XO2ECAcmd_readDevID(XO2Handle_t *pXO2, unsigned int *pVal)
{
    unsigned char cmd[4];
    unsigned char data[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_readDevID()\n");

    /* Read Device ID opcode */
    cmd[0] = 0xE0;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msgs[1].len = 4;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = pI2C->slaveAddr;
    msgs[1].buf = data;

    msg.nmsgs = 2;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d  data=%x %x %x %x\n", status, data[0], data[1], data[2], data[3]);

    if (status == OK) {
        *pVal = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Read the 4 byte USERCODE from the XO2 Configuration logic block.
 * This function assembles the command sequence that allows reading the XO2 USERCODE
 * value from the configuration Flash sector.  The command is first written to the XO2, then a read
 * of 4 bytes is perfromed to return the value.
 *
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 32 bit integer to return the USERCODE value in
 * @return OK if successful, ERROR if failed to read
 * @note
 *
 */
int XO2ECAcmd_readUserCode(XO2Handle_t *pXO2, unsigned int *pVal)
{
    unsigned char cmd[4];
    unsigned char data[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_readUserCode()\n");

    /* Read USERCODE opcode */
    cmd[0] = 0xC0;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msgs[1].len = 4;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = pI2C->slaveAddr;
    msgs[1].buf = data;

    msg.nmsgs = 2;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d  data=%x %x %x %x\n", status, data[0], data[1], data[2], data[3]);

    if (status == OK) {
        *pVal = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Set the 4 byte USERCODE in the XO2 Configuration logic block.
 * This function assembles the command sequence that allows programming the XO2 USERCODE
 * value.  The command is written to the XO2, along with 4 bytes to program into the USERCODE
 * area of flash.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param val new USERCODE value
 * @return OK if successful, ERROR if failed to write
 *
 * @note This command is only useful if the USERCODE contents is previously all 0's.
 * The USERCODE is cleared when the Cfg sector is erased.  So there is no way to
 * individually clear just the USERCODE and reprogram with a new value using this command.
 * Its usefulness is questionable, seeing as the USERCODE is set when programming the
 * Cfg sector anyway.
 */
int XO2ECAcmd_setUserCode(XO2Handle_t *pXO2, unsigned int val)
{
    unsigned char cmd[8];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_setUserCode()\n");

    if (pXO2->cfgEn == FALSE) {
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }

    /* program USERCODE opcode */
    cmd[0] = 0xC2;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;

    cmd[4] = (unsigned char)(val>>24);
    cmd[5] = (unsigned char)(val>>16);
    cmd[6] = (unsigned char)(val>>8);
    cmd[7] = (unsigned char)(val);
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 8;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d\n", status);

    if (status == OK) {
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Read the 8 byte (64 bit) TraceID from the XO2 Feature Row.
 * This function assembles the command sequence that allows reading the XO2 TraceID
 * value from the Feature Row Flash sector.  The command is first written to the XO2, then a read
 * of 8 bytes is perfromed to return the value.
 * The TraceID is set in the Global Settings of Spreadsheet view.
 * The first byte read back (pVal[0]) can be set by the user in Spreadsheet view.
 * The remaining 7 bytes are unique for each silicon die.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 8 byte array to return the TraceID value in
 * @return OK if successful, ERROR if failed to read
 *
 */
int XO2ECAcmd_readTraceID(XO2Handle_t *pXO2, unsigned char *pVal)
{
    unsigned char cmd[4];
    unsigned char data[8];
    int status;
    int i;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_readTraceID()\n");

    /* Read TraceID opcode */
    cmd[0] = 0x19;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msgs[1].len = 8;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = pI2C->slaveAddr;
    msgs[1].buf = data;

    msg.nmsgs = 2;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d  data=", status);
    for (i = 0; i < 7; i++) {
        CPLD_I2C_ISPVME("  %x", data[i]);
    }

    CPLD_I2C_ISPVME("  %x\n", data[7]);

    if (status == OK) {
        for (i = 0; i < 8; i++) {
            pVal[i] = data[i];
        }
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Enable access to Configuration Logic in Transparent mode or Offline mode.
 * This function issues one of the Enable Configuration Interface commands depending on
 * the value of mode.  Transparent mode allows the XO2 to continue operating in user mode
 * while access to the config logic is performed.  Offline mode halts all user logic, and
 * tri-states I/Os, while access is occurring.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param mode specify TRANSPARENT_MODE or OFFLINE_MODE
 * @return OK if successful, ERROR if failed to read
 *
 */
int XO2ECAcmd_openCfgIF(XO2Handle_t *pXO2, XO2CfgMode_t mode)
{
    unsigned char cmd[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    if (mode == TRANSPARENT_MODE) {
        CPLD_I2C_ISPVME("XO2ECAcmd_openCfgIF(Transparent_MODE)\n");
    } else {
        CPLD_I2C_ISPVME("XO2ECAcmd_openCfgIF(Offline_MODE)\n");
    }

    /* Enable Config Interface in Transparent Mode opcode */
    cmd[0] = 0x74;
    cmd[1] = 0x08;
    cmd[2] = 0x00;

    if (mode == OFFLINE_MODE) {
        cmd[0] = 0xC6;  /* Enable Config  Interface in Offline Mode opcode */
    }

    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 3;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    /* Wait till not busy - we have entered Config mode */
    if (status == OK) {
        status = XO2ECAcmd_waitStatusBusy(pXO2);
    }

    CPLD_I2C_ISPVME("\tstatus=%d\n", status);

    if (status == OK) {
        pXO2->cfgEn = TRUE;
        return(OK);
    } else {
        pXO2->cfgEn = FALSE;
        return(ERROR);
    }
}

/**
 * Disable access to Configuration Logic Interface.
 * This function issues the Disable Configuration Interface command and
 * registers that the interface is no longer available for certain commands
 * that require it to be enabled.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed to read
 *
 */
int XO2ECAcmd_closeCfgIF(XO2Handle_t *pXO2)
{
    unsigned char cmd[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_closeCfgIF()\n");

    /* Disable Config  Interface opcode */
    cmd[0] = 0x26;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 3;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d\n", status);

    if (status == OK) {
        pXO2->cfgEn = FALSE;
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Issue the Refresh command that updates the SRAM from Flash and boots the XO2.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR code if failed to write
 *
 */
int XO2ECAcmd_Refresh(XO2Handle_t *pXO2)
{
    unsigned char cmd[4];
    int status;
    unsigned int sr;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_Refresh()\n");

    /* Refresh opcode */
    cmd[0] = 0x79;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 3;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);
    (*pXO2->pDrvrCalls->pfmSecDelay)(XO2DevList[pXO2->devType].Trefresh );

    if (XO2ECAcmd_readStatusReg(pXO2, &sr) != OK) {
        return(ERROR);
    }

    CPLD_I2C_ISPVME("\tstatus=%d   sr=%x\n", status, sr);
    /* Verify that only DONE bit is definitely set and not FAIL or BUSY or ISC_ENABLED */
    if ((sr & 0x3f00) == 0x0100) {
        pXO2->cfgEn = FALSE;
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Issue the Done command that updates the Program DONE bit.
 * Typically used after programming the Cfg Flash and before
 * closing access to the configuration interface.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR code if failed to write
 *
 */
int XO2ECAcmd_setDone(XO2Handle_t *pXO2)
{
    unsigned char cmd[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_setDone()\n");

    if (pXO2->cfgEn == FALSE) {
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }
    /* Program DONE bit opcode */
    cmd[0] = 0x5E;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    /* TODO: This delay time may be excessive */
    if (status == OK) {
        /* Wait 10 msec for Done */
        (*pXO2->pDrvrCalls->pfmSecDelay)(1000);
    } else {
        return(ERROR);
    }
    return(OK);
}

/**
 * Read the 4 byte Status Register from the XO2 Configuration logic block.
 * This function assembles the command sequence that allows reading the XO2 Status Register.
 * The command is first written to the XO2, then a read of 4 bytes is perfromed to return the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 32 bit integer to return the Status Register value in.
 * @return OK if successful, ERROR if failed to read.
 *
 */
int XO2ECAcmd_readStatusReg(XO2Handle_t *pXO2, unsigned int *pVal)
{
    unsigned char cmd[4];
    unsigned char data[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_readStatusReg()\n");

    /* Read Status Register opcode */
    cmd[0] = 0x3C;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msgs[1].len = 4;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = pI2C->slaveAddr;
    msgs[1].buf = data;

    msg.nmsgs = 2;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d  data=%x %x %x %x\n", status, data[0], data[1], data[2], data[3]);

    if (status == OK) {
        *pVal = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Wait for the Status register to report no longer busy.
 * Read the 4 byte Status Register from the XO2 Configuration logic block and check bit 12
 * to see if BUSY.  Also check bit 13 for FAIL indication.  Return error if an error
 * condition is detected.  Also return if exceed polling loop timeout.
 * This function assembles the command sequence that allows reading the XO2 Status Register.
 * The command is first written to the XO2, then a read of 4 bytes is perfromed to return the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if no longer Busy and can proceed, ERROR if failed to read.
 *
 */
int XO2ECAcmd_waitStatusBusy(XO2Handle_t *pXO2)
{
    unsigned char cmd[4];
    unsigned char data[4];
    int status;
    int loop;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_waitStatusBusy()\n");

    cmd[0] = 0x3C;  // Read Status Register opcode
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    loop = XO2ECA_CMD_LOOP_TIMEOUT;
    do
    {
        msgs[0].len = 4;
        msgs[0].addr = pI2C->slaveAddr;
        msgs[0].buf = cmd;
        msgs[0].flags = 0;

        msgs[1].len = 4;
        msgs[1].flags = I2C_M_RD;
        msgs[1].addr = pI2C->slaveAddr;
        msgs[1].buf = data;

        msg.nmsgs = 2;
        msg.msgs = msgs;
        status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

        if (status != OK) {
            return(ERROR);
        }

        if (data[2] & 0x20) { /* FAIL bit set */
            return(ERROR);
        }

        if (data[2] & 0x10) {
            /* Still busy so wait another msec and loop again, if not timed out */
            --loop;
            (*pXO2->pDrvrCalls->pfmSecDelay)(1);   // delay 1 msec
        }
    } while(loop && (data[2] & 0x10));

    if (loop) {
        return(OK);
    } else {
        return(ERROR);   /* timed out waiting for BUSY to clear */
    }
}

/**
 * Read the Busy Flag bit from the XO2 Configuration logic block.
 * This function assembles the command sequence that allows reading the XO2 Busy Flag.
 * The command is first written to the XO2, then a read of 1 byte is perfromed to return the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 8 bit integer to return the Busy Flag bit value in.
 * @return OK if successful, ERROR if failed to read.
 *
 */
int XO2ECAcmd_readBusyFlag(XO2Handle_t *pXO2, unsigned char *pVal)
{
    unsigned char cmd[4];
    unsigned char data;
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_readBusyFlag()\n");

    /* Read Status Register opcode */
    cmd[0] = 0xF0;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;

    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msgs[1].len = 1;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = pI2C->slaveAddr;
    msgs[1].buf = &data;

    msg.nmsgs = 2;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d  data=%x\n", status, data);

    if (status == OK) {
        *pVal = data;
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Wait for the Busy Flag to be cleared.
 * Read the 1 byte from the Busy Flag resgister and check if not 0
 * to see if still BUSY.  Return error if an error
 * condition is detected.  Also return if exceed polling loop timeout.
 * This function assembles the command sequence that allows reading the XO2 Busy Flag Register.
 * The command is first written to the XO2, then a read of 1 byte is perfromed to obtain the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if no longer Busy and can proceed, ERROR if failed to read.
 *
 */
int XO2ECAcmd_waitBusyFlag(XO2Handle_t *pXO2)
{
    unsigned char cmd[4];
    unsigned char data[4];
    int status;
    int loop;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_waitBusyFlag()\n");

    cmd[0] = 0xF0;  // Check Busy Flag Register opcode
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    loop = XO2ECA_CMD_LOOP_TIMEOUT;
    do
    {
        msgs[0].len = 4;
        msgs[0].addr = pI2C->slaveAddr;
        msgs[0].buf = cmd;
        msgs[0].flags = 0;

        msgs[1].len = 1;
        msgs[1].flags = I2C_M_RD;
        msgs[1].addr = pI2C->slaveAddr;
        msgs[1].buf = data;

        msg.nmsgs = 2;
        msg.msgs = msgs;
        status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

        if (status != OK) {
            return(ERROR);
        }

        if (data[0]) {
            /* Still busy so wait another msec */
            --loop;
            (*pXO2->pDrvrCalls->pfmSecDelay)(1);   /* delay 1 msec */
        }

    } while(loop && data[0]);

    if (loop) {
        return(OK);
    } else {
        return(ERROR);   /* timed out waiting for BUSY to clear */
    }
}

/**
 * Send the Bypass command.
 * This function assembles the command sequence that allows writing the XO2 Bypass command.
 * This command is typically called after closing the Configuration Interface.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed to read.
 *
 */
int XO2ECAcmd_Bypass(XO2Handle_t *pXO2)
{
    unsigned char cmd[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_Bypass()\n");

    cmd[0] = 0xFF;  /* Bypass opcode - supposedly does not have arguements, just command byte */
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 1;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d\n", status);

    if (status == OK) {
        return(OK);
    } else {
        return(ERROR);
    }
}
/**
 * Set the current Page Address in either Cfg Flash or UFM.
 * The specific page is set for the next read/write operation.
 * This is probably only useful for the UFM since skipping around in the configuration
 * sector is very unlikely.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param mode which address to update: CFG_SECTOR or UFM_SECTOR
 * @param pageNum the page number to set address pointer to
 * @return OK if successful, ERROR if failed to set address.
 *
 */
int XO2ECAcmd_SetPage(XO2Handle_t *pXO2, XO2SectorMode_t mode, unsigned int pageNum)
{
    unsigned char cmd[8];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_SetPage()\n");

    if (pXO2->cfgEn == FALSE) {
        CPLD_I2C_ISPVME("\tERR_XO2_NOT_IN_CFG_MODE\n");
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }

    if ((mode == UFM_SECTOR) && (pageNum > XO2DevList[pXO2->devType].UFMpages)) {
        CPLD_I2C_ISPVME("\tERR_XO2_EXCEEDS_UFM_SIZE\n");
        return(ERR_XO2_EXCEEDS_UFM_SIZE );
    }

    if ((mode == CFG_SECTOR) && (pageNum > XO2DevList[pXO2->devType].Cfgpages)) {
        CPLD_I2C_ISPVME("\tERR_XO2_EXCEEDS_CFG_SIZE\n");
        return(ERR_XO2_EXCEEDS_CFG_SIZE );
    }

    /* opcode */
    cmd[0] = 0xB4;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;

    if (mode == CFG_SECTOR) {
        cmd[4] = 0x00;  /* page[0] = 0=Config, 1=UFM */
    } else {
        cmd[4] = 0x40;  /* page[0] = 0=Config, 1=UFM */
        cmd[5] = 0x00;  /* page[1] */
        cmd[6] = (unsigned char)(pageNum>>8);  /* page[2] = page number MSB */
        cmd[7] = (unsigned char)pageNum;       /* page[3] = page number LSB */
    }

    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 8;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d\n", status);

    if (status == OK) {
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Erase any/all entire sectors of the XO2 Flash memory.
 * Erase sectors based on the bitmap of parameter mode passed in.
 * <ul>
 * <li> 8 = UFM
 * <li> 4 = CFG
 * <li> 2 = Feature Row
 * <li> 1 = SRAM
 * </ul>
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param mode bit map of what sector contents to erase
 * @return OK if successful, ERROR if failed.
 *
 * @note Erases bits to a value of 0.  Any bit that is a 0 can then be programmed to a 1.
 *
 */
int XO2ECAcmd_EraseFlash(XO2Handle_t *pXO2, unsigned char mode)
{
    unsigned char cmd[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_EraseFlash()\n");

    if (pXO2->cfgEn == FALSE) {
        CPLD_I2C_ISPVME("\tERR_XO2_NOT_IN_CFG_MODE\n");
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }

    mode = mode & 0x0f;

    /* Erase Flash opcode */
    cmd[0] = 0x0E;
    cmd[1] = mode; /*which sectors to clear */
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    if (status == OK) {
        /* Must wait an amount of time, based on device size, for largest flash sector to erase. */
        if (mode & XO2ECA_CMD_ERASE_CFG)
            (*pXO2->pDrvrCalls->pfmSecDelay)(XO2DevList[pXO2->devType].CfgErase);  /* longest */
        else if (mode & XO2ECA_CMD_ERASE_UFM)
            (*pXO2->pDrvrCalls->pfmSecDelay)(XO2DevList[pXO2->devType].UFMErase);  /* medium */
        else
            (*pXO2->pDrvrCalls->pfmSecDelay)(50);/* SRAM & Feature Row = shortest */

        status = XO2ECAcmd_waitStatusBusy(pXO2);
    }

    CPLD_I2C_ISPVME("\tstatus=%d\n", status);

    if (status == OK) {
        return(OK);
    } else {
        return(ERROR);
    }
}

/*===========================================================================================================================
                          C O N F I G     F L A S H      C O M M A N D S
===========================================================================================================================*/
/**
 * Reset the Address Regsiter to point to the first Config Flash page (sector 0, page 0).
 * @param pXO2 pointer to the XO2 device to operate on
 *
 * @return OK is address reset.  Error if failed.
 *
 */
int XO2ECAcmd_CfgResetAddr(XO2Handle_t *pXO2)
{
    unsigned char cmd[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_CfgResetAddr()\n");

    if (pXO2->cfgEn == FALSE) {
        CPLD_I2C_ISPVME("\tERR_XO2_NOT_IN_CFG_MODE\n");
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }

    /* Reset CFG Address pointer to page 0 opcode */
    cmd[0] = 0x46;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d\n", status);

    if (status == OK) {
        return(OK);
    } else {
        return(ERROR);
    }

}

/**
 * Read the next page (16 bytes) from the Config Flash memory.
 * Page address can be set using SetAddress command.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pBuf pointer to the 16 byte array to return the Config page bytes in.
 * @return OK if successful, ERROR if failed to read.
 *
 * @note There is no advantage to reading multiple pages since the interface
 * is most likely so slow.  The I2C only runs at 100kHz.
 *
 * @note The number of pages read is not comparted against the total pages in the
 * device.  Reading too far may have unexpected results.
 */
int XO2ECAcmd_CfgReadPage(XO2Handle_t *pXO2, unsigned char *pBuf)
{
    unsigned char cmd[4];
    unsigned char data[XO2_FLASH_PAGE_SIZE];
    int status;
    int i;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_CfgReadPage()\n");
    if (pXO2->cfgEn == FALSE) {

        CPLD_I2C_ISPVME("\tERR_XO2_NOT_IN_CFG_MODE\n");
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }

    /* Read UFM opcode */
    cmd[0] = 0x73;
    cmd[1] = 0x00; /* pad 4 bytes per page (to get around a readback problem) */
    cmd[2] = 0x00;
    cmd[3] = 0x01; /* 1 page */
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msgs[1].len = XO2_FLASH_PAGE_SIZE;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = pI2C->slaveAddr;
    msgs[1].buf = data;

    msg.nmsgs = 2;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d  data=", status);
    for (i = 0; i < XO2_FLASH_PAGE_SIZE-1; i++) {
        CPLD_I2C_ISPVME("  %x", data[i]);
    }

    CPLD_I2C_ISPVME("  %x\n", data[XO2_FLASH_PAGE_SIZE-1]);

    if (status == OK) {
        for (i = 0; i < XO2_FLASH_PAGE_SIZE; i++) {
            pBuf[i] = data[i];
        }
        return(OK);
    } else {
        return(ERROR);
    }
}
/**
 * Write a page (16 bytes) into the current UFM memory page.
 * Page address can be set using SetAddress command.
 * Page address advances to next page after programming is completed.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pBuf pointer to the 16 byte array to write into the UFM page.
 * @return OK if successful, ERROR if failed to read.
 *
 * @note Programming must be done on a page basis.  Pages must be erased to 0's first.
 * @see XO2ECAcmd_UFMErase
 *
 * @note The number of pages written is not compared against the total pages in the
 * device.  Writing too far may have unexpected results.
 */
int XO2ECAcmd_CfgWritePage(XO2Handle_t *pXO2, unsigned char *pBuf)
{
    unsigned char cmd[4 + 16];
    int status;
    int i;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_CfgWritePage()\n");

    if (pXO2->cfgEn == FALSE) {
        CPLD_I2C_ISPVME("\tERR_XO2_NOT_IN_CFG_MODE\n");
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }

    /* Program Cfg opcode */
    cmd[0] = 0x70;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x01;

    for (i = 0; i < 16; i++) {
        cmd[4 + i] = pBuf[i];
    }

    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4+16;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    if (status == OK) {
        /* Must wait 200 usec for a page to program.  This is a constant for all devices (see XO2 datasheet) */
        (*pXO2->pDrvrCalls->pfuSecDelay)(200);
        status = XO2ECAcmd_waitStatusBusy(pXO2);
    }
    CPLD_I2C_ISPVME("\tstatus=%d\n", status);

    if (status == OK) {
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Erase the entire sector of the Configuration Flash memory.
 * This is a convience function to erase all Config contents to 0.  You can not erase on a page basis.
 * The entire sector is cleared.
 * The erase takes up to a few seconds.  The time to wait is device specific.
 * It is important that the correct device is selected in the pXO2 structure.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed.
 *
 * @note Erases bits to a value of 0.  Any bit that is a 0 can then be programmed to a 1.
 *
 */
int XO2ECAcmd_CfgErase(XO2Handle_t *pXO2)
{
    CPLD_I2C_ISPVME("XO2ECAcmd_CfgErase()\n");
    return(XO2ECAcmd_EraseFlash(pXO2, XO2ECA_CMD_ERASE_CFG));
}

/*===========================================================================================================================
                            U F M      C O M M A N D S
===========================================================================================================================*/
/**
 * Reset the Address Regsiter to point to the first UFM page (sector 1, page 0).
 * @param pXO2 pointer to the XO2 device to operate on
 *
 * @return OK if successful. ERROR if failed.
 *
 */
int XO2ECAcmd_UFMResetAddr(XO2Handle_t *pXO2)
{
    unsigned char cmd[4];
    int status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_UFMResetAddr()\n");
    if (pXO2->cfgEn == FALSE) {
        CPLD_I2C_ISPVME("\tERR_XO2_NOT_IN_CFG_MODE\n");
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }

    if (pXO2->devType == MachXO2_256) {
        CPLD_I2C_ISPVME("\tERR_XO2_NO_UFM\n");
        return(ERR_XO2_NO_UFM);
    }

    /* Reset UFM Address pointer to page 0 opcode */
    cmd[0] = 0x47;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d\n", status);

    if (status == OK) {
        return(OK);
    } else {
        return(ERROR);
    }

}

/**
 * Read the next page (16 bytes) from the UFM memory.
 * Page address can be set using SetAddress command.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pBuf pointer to the 16 byte array to return the UFM page bytes in.
 * @return OK if successful, ERROR if failed to read.
 *
 * @note There is no advantage to reading multiple pages since the interface
 * is most likely so slow.  The I2C only runs at 100kHz.
 *
 */
int XO2ECAcmd_UFMReadPage(XO2Handle_t *pXO2, unsigned char *pBuf)
{
    unsigned char cmd[4];
    unsigned char data[16];
    int status;
    int i;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_UFMReadPage()\n");
    if (pXO2->cfgEn == FALSE) {
        CPLD_I2C_ISPVME("\tERR_XO2_NOT_IN_CFG_MODE\n");
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }

    if (pXO2->devType == MachXO2_256) {
        CPLD_I2C_ISPVME("\tERR_XO2_NO_UFM\n");
        return(ERR_XO2_NO_UFM);
    }

    /* Read UFM opcode */
    cmd[0] = 0xCA;
    cmd[1] = 0x00; /* pad 4 bytes per page (to get around a readback problem) */
    cmd[2] = 0x00;
    cmd[3] = 0x01; /* 1 page */

    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msgs[1].len = 16;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = pI2C->slaveAddr;
    msgs[1].buf = data;

    msg.nmsgs = 2;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    CPLD_I2C_ISPVME("\tstatus=%d  data=", status);
    for (i = 0; i < 15; i++) {
        CPLD_I2C_ISPVME("  %x", data[i]);
    }

    CPLD_I2C_ISPVME("  %x\n", data[15]);

    if (status == OK) {
        for (i = 0; i < 16; i++) {
            pBuf[i] = data[i];
        }
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Write a page (16 bytes) into the current UFM memory page.
 * Page address can be set using SetAddress command.
 * Page address advances to next page after programming is completed.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pBuf pointer to the 16 byte array to write into the UFM page.
 * @return OK if successful, ERROR if failed to read.
 *
 * @note Programming must be done on a page basis.  Pages must be erased to 0's first.
 * @see XO2ECAcmd_UFMErase
 *
 */
int XO2ECAcmd_UFMWritePage(XO2Handle_t *pXO2, unsigned char *pBuf)
{
    unsigned char cmd[4 + 16];
    int status;
    int i;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_UFMWritePage()_1\n");
    if (pXO2->cfgEn == FALSE) {
        CPLD_I2C_ISPVME("\tERR_XO2_NOT_IN_CFG_MODE\n");
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }

    if (pXO2->devType == MachXO2_256) {
        CPLD_I2C_ISPVME("\tERR_XO2_NO_UFM\n");
        return(ERR_XO2_NO_UFM);
    }
    /* Program UFM opcode */
    cmd[0] = 0xC9;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x01;

    for (i = 0; i < 16; i++) {
        cmd[4 + i] = pBuf[i];
    }

    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4+16;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    if (status == OK) {
        /* Must wait 200 usec for a page to program.  This is a constant for all devices (see XO2 datasheet) */
        (*pXO2->pDrvrCalls->pfuSecDelay)(200);
        status = XO2ECAcmd_waitStatusBusy(pXO2);
    }
    CPLD_I2C_ISPVME("\tstatus=%d\r\n", status);

    if (status == OK) {
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Erase the entire sector of the UFM memory.
 * This is a convience function to erase all UFM contents to 0.  You can not erase on a page basis.
 * The entire sector is cleared.  Therefore save any data first, erase, then
 * reprogram, putting the saved data back along with any new data.
 * The erase takes up to a few hundered milliseconds.  The time to wait is device specific.
 * It is important that the correct device is selected in the pXO2 structure.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed.
 *
 * @note Erases bits to a value of 0.  Any bit that is a 0 can then be programmed to a 1.
 * @note The routine does not poll for completion, but rather waits the maximum time
 * for an erase as specified in the data sheet.
 */
int XO2ECAcmd_UFMErase(XO2Handle_t *pXO2)
{
    CPLD_I2C_ISPVME("XO2ECAcmd_UFMErase()\n");
    return(XO2ECAcmd_EraseFlash(pXO2, XO2ECA_CMD_ERASE_UFM));
}

/*===========================================================================================================================
                            F E A T U R E    R O W     C O M M A N D S
===========================================================================================================================*/

/**
 * Erase the Feature Row contents.
 * This is a convience function to erase just the feature row bits to 0.
 * You must reprogram the Feature Row with FeatureWrite() or the
 * XO2 part may be in an unusable state.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed.
 *
 * @note Erases bits to a value of 0.  Any bit that is a 0 can then be programmed to a 1.
 *
 */
int XO2ECAcmd_FeatureRowErase(XO2Handle_t *pXO2)
{
    CPLD_I2C_ISPVME("XO2ECAcmd_FeatureErase()\n");
    return(XO2ECAcmd_EraseFlash(pXO2, XO2ECA_CMD_ERASE_FTROW));
}

/**
 * Set the Feature Row.
 * This function assembles the command sequence that allows programming the XO2 FEATURE
 * bits and the FEABITS in the Feature Row.  The 8 FEATURE bytes and 2 FEABITS bytes
 * must be properly formatted or possible
 * lock-up of the XO2 could occur.  Only the values obtained from properly parsing the JEDEC
 * file should be used.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pFeature pointer to the Feature Row structure containing the encoded data to write
 * into the Feature and FEABITS fields in the Feature Row.
 * @return OK if successful, ERROR if failed to write
 *
 * @note The Feature Row must first be erased
 */
int XO2ECAcmd_FeatureRowWrite(XO2Handle_t *pXO2, XO2FeatureRow_t *pFeature)
{
    unsigned char cmd[16];
    int i, status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_FeatureWrite()\n");

    if (pXO2->cfgEn == FALSE) {
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }
    /* program Feature opcode */
    cmd[0] = 0xE4;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;

    for (i = 0; i < 8; i++) {
        cmd[4 + i] = pFeature->feature[i];
    }
    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 12;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    if (status != OK) {
        return(ERROR);
    }
    /* Must wait 200 usec for a page to program.  This is a constant for all devices (see XO2 datasheet) */
    (*pXO2->pDrvrCalls->pfuSecDelay)(200);
    /* program FEABITS opcode */
    cmd[0] = 0xF8;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;

    cmd[4] = pFeature->feabits[0];
    cmd[5] = pFeature->feabits[1];

    msgs[0].len = 6;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msg.nmsgs = 1;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    if (status == OK) {
        /* Must wait 200 usec for a page to program.  This is a constant for all devices (see XO2 datasheet) */
        (*pXO2->pDrvrCalls->pfuSecDelay)(200);
        status = XO2ECAcmd_waitStatusBusy(pXO2);
    }

    if (status == OK) {
        return(OK);
    } else {
        return(ERROR);
    }
}

/**
 * Read the Feature Row contents.
 * This function assembles the command sequence that allows reading back the Feature Row
 * contents.  The FEATURE bytes and FEABITS fields are returned in a XO2 specific stucture.
 * Uses would be to verify successful XO2ECAcmd_FeatureWrite() programing. Or to read back
 * and preserve during an update.
 * @param pXO2 pointer to the XO2 device to access
 * @param pFeature pointer to the Feature Row structure that will be loaded with the
 * feature row contents.
 * @return OK if successful, ERROR if failed to read
 *
 */
int XO2ECAcmd_FeatureRowRead(XO2Handle_t *pXO2, XO2FeatureRow_t *pFeature)
{
    unsigned char cmd[4];
    unsigned char data[8];

    int i, status;
    OC_I2C_t *pI2C;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msg;

    CPLD_I2C_ISPVME("XO2ECAcmd_FeatureRead()\n");
    if (pXO2->cfgEn == FALSE) {
        return(ERR_XO2_NOT_IN_CFG_MODE);
    }
    /* Read Feature opcode */
    cmd[0] = 0xE7;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;

    pI2C = (OC_I2C_t  *)pXO2->pDrvrParams;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msgs[1].len = 8;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = pI2C->slaveAddr;
    msgs[1].buf = data;

    msg.nmsgs = 2;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    if (status != OK) {
        return(ERROR);
    }

    for (i = 0; i < 8; i++) {
        pFeature->feature[i] = data[i];
    }
    /* Read FEABITS opcode */
    cmd[0] = 0xFB;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;

    msgs[0].len = 4;
    msgs[0].addr = pI2C->slaveAddr;
    msgs[0].buf = cmd;
    msgs[0].flags = 0;

    msgs[1].len = 2;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = pI2C->slaveAddr;
    msgs[1].buf = data;

    msg.nmsgs = 2;
    msg.msgs = msgs;
    status = (*pXO2->pDrvrCalls->pfCmdTransfer)(pXO2->fd, &msg);

    if (status != OK) {
        return(ERROR);
    }

    pFeature->feabits[0] = data[0];
    pFeature->feabits[1] = data[1];

    return(OK);
}

/**
 * Erase the SRAM, clearing the user design.
 * This is a convience function to erase just the SRAM.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed.
 */
int XO2ECAcmd_SRAMErase(XO2Handle_t *pXO2)
{
    CPLD_I2C_ISPVME("XO2ECAcmd_SRAMErase()\n");
    return(XO2ECAcmd_EraseFlash(pXO2, XO2ECA_CMD_ERASE_SRAM));
}
