/*
 * $Id: plp_aperta2_peregrine5_pc_ucode.h $
 * $Copyright: (c) 2013 Broadcom Corporation All Rights Reserved.$
 * All Rights Reserved.$
 */
#ifndef peregrine5_pc_ucode_H_
#define peregrine5_pc_ucode_H_ 

#include <phymod/phymod.h>


#define PEREGRINE5_PC_UCODE_IMAGE_VERSION "E001_06"  /* matches the version number from microcode */
#define PEREGRINE5_PC_UCODE_IMAGE_SIZE    82012
#define PEREGRINE5_PC_UCODE_STACK_SIZE    0xEF2
#define PEREGRINE5_PC_UCODE_IMAGE_CRC     0x83AC
#define PEREGRINE5_PC_UCODE_DR_SIZE       0x0

extern unsigned char*  plp_aperta2_peregrine5_pc_ucode_get(void);


#endif
