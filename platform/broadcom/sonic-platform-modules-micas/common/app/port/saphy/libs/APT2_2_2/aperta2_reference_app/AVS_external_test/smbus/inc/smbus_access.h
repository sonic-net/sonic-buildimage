/*
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.
 */
#ifndef __SMBUS_ACCESS_H__
#define __SMBUS_ACCESS_H__

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <epdm.h>
#include "smbus_libtypes.h"
#include "smbus_log.h"

/* Supported SW SMBus clock values */
#define SMBUS_CLOCK_400KHZ 0
#define SMBUS_CLOCK_1MHZ   1

/* Supported HW SMBus clock values */
#define SMBUS_SPEED_400K 0x2
#define SMBUS_SPEED_1M   0x4

typedef struct ial_config_s
{
    union
    {
        uint32_t IalConfigValue;
        struct {
#if IS_BIG_ENDIAN
                uint32_t RsvdM1        :  3;
                uint32_t MdioPortAddr  :  5;
                uint32_t RsvdM0        :  3;
                uint32_t MdioDevAddr   :  5;
                uint32_t RsvdS0        :  6;
                uint32_t SmbusAddrMode :  2;
                uint32_t PecEnabled    :  1;
                uint32_t SmbusI2cAddr  :  7;
#else
                uint32_t SmbusI2cAddr  :  7;
                uint32_t PecEnabled    :  1;
                uint32_t SmbusAddrMode :  2;
                uint32_t RsvdS0        :  6;
                uint32_t MdioDevAddr   :  5;
                uint32_t RsvdM0        :  3;
                uint32_t MdioPortAddr  :  5;
                uint32_t RsvdM1        :  3;
#endif
        } Bits;
    } connection;
} ial_config_t;

int configSmbusParam(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, uint8_t i2cAddr, uint8_t pecEn, uint8_t addrMode);
uint8_t smbusReadByte(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, U8 cmd);
uint16_t smbusReadWord (char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, uint8_t cmd);
void smbusWriteByte(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, uint8_t cmd, uint8_t data);
void smbusWriteWord(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, uint8_t cmd, uint16_t data);
void smbusWrRdBlock(char *chip_name, bcm_plp_access_t plp_info, ial_config_t *ial_config, const uint8_t* wrBuf, uint8_t wrLen, uint8_t* rdBuf, uint8_t rdLen);

#endif /* __SMBUS_ACCESS_H__ */
