/*
 *  COPYRIGHT (c) 2011 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/** @file XO2_api.h */

#ifndef LATTICE_XO2_API_H
#define LATTICE_XO2_API_H

#define XO2ECA_PROGRAM_TRANSPARENT 0x10 // program in Background, user logic runs while doing it
#define XO2ECA_PROGRAM_OFFLINE     0x00 // program in Direct mode, user logic halts
#define XO2ECA_PROGRAM_VERIFY      0x20 // Verify Programming of any of the above modes

#define XO2ECA_ERASE_PROG_UFM      0x08 // Erase/program UFM sector
#define XO2ECA_ERASE_PROG_CFG      0x04 // Erase and program CFG sector
#define XO2ECA_ERASE_PROG_FEATROW  0x02 // Erase/program Feature Row
#define XO2ECA_ERASE_SRAM          0x01 // Erase SRAM (used in Offline mode)

#define NOT_IMPLEMENTED_ERR   (-1000)

int XO2ECA_apiProgram(XO2Handle_t *pXO2dev, XO2_JEDEC_t *pProgJED, int mode);
int XO2ECA_apiClearXO2(XO2Handle_t *pXO2dev);
int XO2ECA_apiEraseFlash(XO2Handle_t *pXO2dev,  int mode);
void XO2ECA_apiJEDECinfo(XO2Handle_t *pXO2dev, XO2_JEDEC_t *pProgJED);
int XO2ECA_apiJEDECverify(XO2Handle_t *pXO2dev, XO2_JEDEC_t *pProgJED);
int XO2ECA_apiReadBackCfg(XO2Handle_t *pXO2dev, unsigned char *pBuf);
int XO2ECA_apiReadBackUFM(XO2Handle_t *pXO2dev, int startPg, int numPgs, unsigned char *pBuf);
int XO2ECA_apiWriteUFM(XO2Handle_t *pXO2dev, int startPg, int numPgs, unsigned char *pBuf, int erase);
int XO2ECA_apiGetHdwStatus(XO2Handle_t *pXO2dev, unsigned int *pVal);
int XO2ECA_apiGetHdwInfo(XO2Handle_t *pXO2dev, XO2RegInfo_t *pInfo);

#endif
