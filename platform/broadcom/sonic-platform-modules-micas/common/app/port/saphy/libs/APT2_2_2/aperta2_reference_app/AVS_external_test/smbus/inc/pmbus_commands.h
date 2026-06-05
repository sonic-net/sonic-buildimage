/*
* $Copyright: (c) 2024 Broadcom.
* Broadcom Proprietary and Confidential. All rights reserved.$
*
*/
#ifndef PMBUS_COMMANDS_H_
#define PMBUS_COMMANDS_H_

typedef enum {
    SMBUS_QUICK_WR_CMD,             /* addr+wr only, not cmd or data */
    SMBUS_QUICK_RD_CMD,             /* addr+rd only, not cmd or data */
    SMBUS_SEND_BYTE,                /* addr+wr, byte */
    SMBUS_RECIEVE_BYTE,             /* addr+rd, _byte */
    SMBUS_WRITE_BYTE,               /* addr+wr, cmd, byte */
    SMBUS_READ_BYTE,                /* addr+wr, cmd, Sr, addr+rd, _byte */
    SMBUS_WRITE_WORD,               /* addr+wr, cmd, LSB byte, MSB byte */
    SMBUS_READ_WORD,                /* addr+wr, cmd, Sr, addr+rd, _LSB_byte, _MSB_byte */
    SMBUS_WRITE_BLOCK,              /* addr+wr, cmd, N, bytes */
    SMBUS_READ_BLOCK,               /* addr+wr, cmd, Sr, addr+rd, _N, _bytes */
    SMBUS_WR_RD_BLOCK               /* addr+wr, cmd, N, bytes, Sr, addr+rd, _N, _bytes */
} SMBUS_protocols_e;

typedef enum {
    PMBUS_PAGE                    = 0,
    PMBUS_OPERATION               = 0x1,
    PMBUS_ON_OFF_CONFIG           = 0x2,
    PMBUS_PMBUS_CLEAR_FAULTS      = 0x3,
    PMBUS_PMBUS_PAGE_PLUS_WRITE   = 0x5,
    PMBUS_PMBUS_PAGE_PLUS_READ    = 0x6,
    PMBUS_PMBUS_WRITE_PROTECT     = 0x10,
    PMBUS_PMBUS_STORE_USER_ALL    = 0x15,
    PMBUS_PMBUS_RESTORE_USER_ALL  = 0x16,
    PMBUS_CAPABILITY              = 0x19,
    PMBUS_SMBALERT_MASK           = 0x1B,
    PMBUS_PMBUS_VOUT_MODE         = 0x20,
    PMBUS_VOUT_COMMAND            = 0x21,
    PMBUS_PMBUS_VOUT_MAX          = 0x24,
    PMBUS_VOUT_MARGIN_HIGH        = 0x25,
    PMBUS_VOUT_MARGIN_LOW         = 0x26,
    PMBUS_VOUT_TRANSITION_RATE    = 0x27,
    PMBUS_FREQUENCY_SWITCH        = 0x33,
    PMBUS_VIN_ON                  = 0x35,
    PMBUS_VIN_OFF                 = 0x36,
    PMBUS_VOUT_OV_FAULT_LIMIT     = 0x40,
    PMBUS_VOUT_OV_FAULT_RESPONSE  = 0x41,
    PMBUS_VOUT_OV_WARN_LIMIT      = 0x42,
    PMBUS_VOUT_UV_WARN_LIMIT      = 0x43,
    PMBUS_VOUT_UV_FAULT_LIMIT     = 0x44,
    PMBUS_VOUT_UV_FAULT_RESPONSE  = 0x45,
    PMBUS_IOUT_OC_FAULT_LIMIT     = 0x46,
    PMBUS_IOUT_OC_FAULT_RESPONSE  = 0x47,
    PMBUS_IOUT_OC_WARN_LIMIT      = 0x4A,
    PMBUS_STATUS_BYTE             = 0x78,
    PMBUS_STATUS_WORD             = 0x79,
    PMBUS_STATUS_VOUT             = 0x7A,
    PMBUS_STATUS_IOUT             = 0x7B,
    PMBUS_STATUS_INPUT            = 0x7C,
    PMBUS_STATUS_TEMPERATURE      = 0x7D,
    PMBUS_STATUS_CML              = 0x7E,
    PMBUS_READ_VIN                = 0x88,
    PMBUS_READ_IIN                = 0x89,
    PMBUS_READ_VOUT               = 0x8B,
    PMBUS_READ_IOUT               = 0x8C,
    PMBUS_READ_TEMPERATURE_1      = 0x8D,
    PMBUS_READ_FREQUENCY          = 0x95,
    PMBUS_READ_POUT               = 0x96,
    PMBUS_PMBUS_REVISION          = 0x98,
    PMBUS_MFR_ID                  = 0x99,
    PMBUS_MFR_MODEL               = 0x9A,
    PMBUS_MFR_REVISION            = 0x9B,
    PMBUS_MFR_SERIAL              = 0x9E,
    PMBUS_IC_DEVICE_ID            = 0xAD,
    PMBUS_IC_DEVICE_REV           = 0xAE,
    PMBUS_MFR_RAIL_ADDRESS        = 0xFA,
} PMBus_commands_e;


#endif

