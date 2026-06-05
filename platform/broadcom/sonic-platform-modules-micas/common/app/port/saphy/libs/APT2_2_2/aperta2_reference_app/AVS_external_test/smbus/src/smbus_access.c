/*
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/* Includes */
#include <smbus_access.h>

#define SMBUS_ADDR_MODE_16BIT         0x0
#define SMBUS_ADDR_MODE_32BIT         0x1
#define SMBUS_ADDR_MODE_32BIT_PROP    0x2
#define SMBUS_ADDR_MODE_RSVD          0x3

#define INTEL_SMBUS_WR32_2ADDR        0x07
#define INTEL_SMBUS_WR32_4ADDR        0x0F
#define INTEL_SMBUS_WR32_2ADDR_PEC    0x87
#define INTEL_SMBUS_WR32_4ADDR_PEC    0x8F

#define INTEL_SMBUS_RD32_2ADDR        0x02
#define INTEL_SMBUS_RD32_2ADDR_D      0x01
#define INTEL_SMBUS_RD32_2ADDR_PEC    0x82
#define INTEL_SMBUS_RD32_2ADDR_D_PEC  0x81

#define INTEL_SMBUS_RD32_4ADDR        0x0A
#define INTEL_SMBUS_RD32_4ADDR_D      0x09
#define INTEL_SMBUS_RD32_4ADDR_PEC    0x8A
#define INTEL_SMBUS_RD32_4ADDR_D_PEC  0x89

#define SMBUS_WR_BLOCK_CMD            0xA7
#define SMBUS_WR_BLOCK_CMD_PEC        0xB7

#define SMBUS_PROCESS_BLOCK_CMD       0xA9
#define SMBUS_PROCESS_BLOCK_CMD_PEC   0xB9

#define SMBUS_GET_STATUS              0xA5
#define SMBUS_GET_STATUS_PEC          0xB5

#define ADDR_FPGA_I2C_CTRL            (0x08)
#define ADDR_FPGA_I2C_DEV_ADDR        (0x09)
#define ADDR_FPGA_I2C_REG_TYPE        (0x0A)
#define ADDR_FPGA_I2C_REG_DATA        (0x0B)
#define ADDR_FPGA_I2C_DATA            (0x0C)
#define ADDR_FPGA_I2C_STATUS          (0x0D)

#define I2C_MODE_GENERIC              (0xC) /* GENERIC: slave addr + WR(0) + ACK + WR I2C FIFO Data + ACK + .... + ACK + Re-start, slave addr+RD(1), data from I2C RD FIFO */

#define I2C_WR_LEN_LSB                (0x4)
#define I2C_WR_LEN_MSB                (0x5)
#define I2C_RD_LEN_LSB                (0x6)
#define I2C_RD_LEN_MSB                (0x7)

typedef enum {
    RETIMER_IAL_SUCCESS            = 0x0,
    RETIMER_IAL_FAILED,
    RETIMER_IAL_DEVICE_OPEN_FAILED,
    RETIMER_IAL_INVALID_MEM,
    RETIMER_IAL_SMBUS_RD_TIMEOUT,
    RETIMER_IAL_SMBUS_HW_ERROR,
    RETIMER_IAL_PEC_MISMATCH,
    RETIMER_IAL_INVALID_ADDR,
    RETIMER_IAL_ECHOED_ADDR_MISMATCH,
    RETIMER_IAL_INVALID_PARAMETER,
    RETIMER_IAL_RD_LEN_MISMATCH
} RETIMER_IAL_STATUS;

int rtmWriteReg(char *chip_name, bcm_plp_access_t plp_info, uint32_t addr, uint8_t data)
{
    bcm_plp_reg_value_set(chip_name, plp_info, 30, addr, data);
    return 0 ;
}

uint8_t rtmReadReg(char *chip_name, bcm_plp_access_t plp_info, uint32_t addr)
{
    uint32_t data_u32 = 0;
    bcm_plp_reg_value_get(chip_name, plp_info, 30, addr, &data_u32);
    return(data_u32 & 0xFF);
}

static void updatePEC(U8 data, U8 *crc)
{
    static U8 table[] = {0x07, 0x0E, 0x1C, 0x38,  0x70, 0xE0, 0xC7, 0x89};
    U8 xx, kk, dd = *crc ^data;
    *crc = 0;
    for (xx=1,kk=0; xx; xx <<=1, ++kk) {
        if (xx & dd) {
            *crc ^= table[kk];
        }
    }
}

static U8 calculatePEC(U8 pecInit, const U8* data, U8 L)
{
    U8 crc = pecInit;
    while (L--) {
      updatePEC(*data++, &crc);
    }
    return crc;
}

static int rtmIsI2cDone(char *chip_name, bcm_plp_access_t plp_info)
{
    int rv = 0;
    U8 i2cOpStatus;
    U16 timeoutCnt = 0;
    U16 timeoutVal = 65000;
    do {
        i2cOpStatus = rtmReadReg(chip_name, plp_info, ADDR_FPGA_I2C_STATUS);
        if (++timeoutCnt > timeoutVal) break;
        usleep(10*1000);
    } while(0 == (i2cOpStatus & 0x80));  /* I2C operation done */

    LOGX(LOG_TRACE, "I2C HW ready after %d checks ", timeoutCnt);
    if (timeoutCnt > timeoutVal) {
       LOGX(LOG_ERROR, "HOST I2C operation is time out after %d polls.", timeoutCnt);
       rv = RETIMER_IAL_SMBUS_RD_TIMEOUT;
    } else {
       LOGX(LOG_DBG1,  "HOST I2C operation completes   after %d polls.", timeoutCnt);
       rv = RETIMER_IAL_SUCCESS;
    }
    return rv ;
}

static int i2cRwOperation(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config,
                          const U8* wrBuf, U8 wrLen, U8* rdBuf, U8 rdLen)
{
    int rv = 0 ;
    int kk;
    U8 I2cAddr, PecCfg;

    if ((wrLen == 0) && (rdLen == 0)) {
       LOGX(LOG_ERROR, "Both write and read lengths are 0.");
       return RETIMER_IAL_FAILED;
    }

    I2cAddr = ial_config->connection.Bits.SmbusI2cAddr;
    PecCfg  = ial_config->connection.Bits.PecEnabled;


    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_DEV_ADDR, I2cAddr);  /* I2C 7-bit device address */
    /* Set I2C write and read length */
    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_REG_TYPE, I2C_WR_LEN_LSB);
    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_REG_DATA, wrLen + ((rdLen == 0) && (PecCfg)));
    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_REG_TYPE, I2C_WR_LEN_MSB);
    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_REG_DATA, 0);
    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_REG_TYPE, I2C_RD_LEN_LSB);
    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_REG_DATA, rdLen + ((rdLen != 0) && (PecCfg)));
    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_REG_TYPE, I2C_RD_LEN_MSB);
    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_REG_DATA, 0);

    if (0 == rdLen) {
        /* I2C write only */
        for (kk = 0; kk<wrLen; ++kk) {
            rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_DATA, wrBuf[kk]);
        }

        if (PecCfg) {
            U8 pec = 0;
            updatePEC((I2cAddr)<<1, &pec);
            pec = calculatePEC(pec, wrBuf, wrLen);
            rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_DATA, pec);
            LOGX(LOG_TRACE, "PEC{%d|%02X, %02X,..., %02X} = %02X inserted.", \
                wrLen, ((I2cAddr)<<1), wrBuf[0], wrBuf[wrLen-1], pec);
        }
        /* Start I2C write */
        rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_CTRL, I2C_MODE_GENERIC*16+2);
    } else if (0 == wrLen) {
        /* I2C read only */
        /* Start I2C read */
        rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_CTRL, I2C_MODE_GENERIC*16+2);
        /* Check if i2C read is done */
        rtmIsI2cDone(chip_name, plp_info);

        for (kk=0; kk<rdLen; ++kk) {
            rdBuf[kk] = rtmReadReg(chip_name, plp_info, ADDR_FPGA_I2C_DATA);
            LOGX(LOG_DBG2, "i2c_rd_data[%d] = 0x%02X", kk, rdBuf[kk]);
         }
    } else {
        /* I2C write, followed by I2C read */
        U8 pec_c = 0, pec_r;
        for (kk=0; kk<wrLen; ++kk) {
            rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_DATA, wrBuf[kk]);
        }

        if (PecCfg) {
            updatePEC((I2cAddr)<<1, &pec_c);
            pec_c = calculatePEC(pec_c, wrBuf, wrLen);
        }
        /* Start I2C write/read */
        rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_CTRL, I2C_MODE_GENERIC*16+2);
        rv = rtmIsI2cDone(chip_name, plp_info);

        if (rv == RETIMER_IAL_SMBUS_RD_TIMEOUT) {
            return RETIMER_IAL_SMBUS_RD_TIMEOUT;
        }

        for (kk= 0; kk<rdLen; ++kk) {
            rdBuf[kk] = rtmReadReg(chip_name, plp_info, ADDR_FPGA_I2C_DATA);
            LOGX(LOG_DBG2, "i2c_rd_data[%d] = 0x%02X", kk, rdBuf[kk]);
        }
        if (PecCfg) {
            updatePEC(1 | ((I2cAddr)<<1), &pec_c);
            pec_c = calculatePEC(pec_c, rdBuf, rdLen);
            pec_r = rtmReadReg(chip_name, plp_info, ADDR_FPGA_I2C_DATA);

            if (pec_c != pec_r) {
                rv = RETIMER_IAL_PEC_MISMATCH;
                LOGX(LOG_INFO, "PEC{%d+%d|%02X, %02X,..., %02X, %02X,...,%02X} = %02X does not match locally calculated PEC %02X.", \
                    wrLen, rdLen, (I2cAddr<<1), wrBuf[0], (1|(I2cAddr<<1)), rdBuf[0], rdBuf[rdLen-1], pec_r, pec_c);
            } else {
                LOGX(LOG_TRACE, "PEC{%d+%d|%02X, %02X,..., %02X, %02X,...,%02X} = %02X        matches locally calculated PEC %02X.", \
                    wrLen, rdLen, (I2cAddr<<1), wrBuf[0], (1|(I2cAddr<<1)), rdBuf[0], rdBuf[rdLen-1], pec_r, pec_c);
            }
        }
    }

    rtmWriteReg(chip_name, plp_info, ADDR_FPGA_I2C_CTRL, I2C_MODE_GENERIC*16+0);
    return rv;
}

int configSmbusParam(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, uint8_t i2cAddr, uint8_t pecEn, uint8_t addrMode)
{
    if (i2cAddr  != 0xFF) {
        ial_config->connection.Bits.SmbusI2cAddr  = i2cAddr;
    }
    if (pecEn    != 0xFF) {
        ial_config->connection.Bits.PecEnabled    = pecEn;
    }
    if (addrMode != 0xFF) {
        ial_config->connection.Bits.SmbusAddrMode = addrMode;
    }
    return 0;
}

uint8_t smbusReadByte(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, uint8_t cmd)
{
    U8 rbuf[2];
    i2cRwOperation(chip_name, plp_info, ial_config, &cmd, 1, rbuf, 1);
    return rbuf[0];
}

uint16_t smbusReadWord (char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, uint8_t cmd)
{
    U16 rv;
    U8 rbuf[3];
    i2cRwOperation(chip_name, plp_info, ial_config, &cmd, 1, rbuf, 2);
    rv = rbuf[1];
    return (rv << 8) | rbuf[0];
}

void smbusWriteByte(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, uint8_t cmd, uint8_t data)
{
    U8 wbuf[2] = {cmd, data};
    i2cRwOperation(chip_name, plp_info, ial_config, wbuf, 2, NULL, 0);
}

void smbusWriteWord(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, uint8_t cmd, uint16_t data)
{
    U8 wbuf[3] = {cmd, data&0xFF, data>>8};
    i2cRwOperation(chip_name, plp_info, ial_config, wbuf, 3, NULL, 0);
}

void smbusWrRdBlock(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, const uint8_t* wrBuf, uint8_t wrLen, uint8_t* rdBuf, uint8_t rdLen)
{
    i2cRwOperation(chip_name, plp_info, ial_config, wrBuf, wrLen, rdBuf, rdLen);
}
