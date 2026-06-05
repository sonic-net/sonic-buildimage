/*
*
* $Id: aperta2_msg_tasks.c,  $
*
*  *
*  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
*  *
*  *
*
*/

#include <tier1/aperta2_msg_task.h>
#include <tier1/bcm_aperta2_direct_defs.h>
#include <tier1/aperta2_reg_access.h>

/*!
 *  Function to put a byte (8-bits) to uC memory (Little-endian).
 *
 *  Puts a byte to the given location and increases passed pointer by 1.
 *
 *  @param ptr    Pointer to pointer to uC memory location
 *  @param value  Value (16-bit) to copy
 */
void
plp_aperta2_put_byte (uint8_t **ptr, uint8_t value)
{
    *(*ptr) = value;
    (*ptr)++;
}

/*!
 *  Function to get a byte (8-bits) from uC memory (Little-endian)
 *
 *  Get a byte from the given location and increase passed pointer by 1.
 *
 *  @param ptr    Pointer to pointer to uC memory location
 *
 *  @return  Value (16-bit) read from the memory
 */
uint8_t
plp_aperta2_get_byte (uint8_t **ptr)
{
    uint8_t value;

    value = **ptr;
    (*ptr)++;

    return (value);
}

/*!
 *  Function to put a half word (16-bits) to uC memory (Little-endian)
 *
 *  Put a half-word at the given location and increase passed pointer by 2.
 *
 *  @param ptr    Pointer to pointer to uC memory location
 *  @param value  Value (16-bit) to copy
 */
void
plp_aperta2_put_half_word (uint8_t **ptr, uint16_t value)
{
    **ptr = (value & 0xFF);
    (*ptr)++;
    **ptr = (value >> 8);
    (*ptr)++;
}

/*!
 *  Function to get a half word (16-bits) from uC memory (Little-endian)
 *
 *  Get a half-word at the given location and increase passed pointer by 2.
 *
 *  @param ptr    Pointer to pointer to uC memory location
 *
 *  @return  Value (16-bit) read from the memory
 */
uint16_t
plp_aperta2_get_half_word (uint8_t **ptr)
{
    uint16_t value;

    value = **ptr;
    (*ptr)++;
    value += ((**ptr) << 8);
    (*ptr)++;

    return (value);
}
/*!
 *  Function to put 24bits of a word to memory 
 *
 *  Put a word at the given location and increase passed pointer by 3.
 *
 *  @param ptr    Pointer to pointer to uC memory location
 *  @param value  Value (24-bit) to copy
 */

void
plp_aperta2_put_24bits (uint8_t **ptr, uint32_t value)
{
    **ptr = (value & 0xFF);
    (*ptr)++;
    value >>= 8;

    **ptr = (value & 0xFF);
    (*ptr)++;
    value >>= 8;

    **ptr = (value & 0xFF);
    (*ptr)++;
}

/*!
 *  Function to put a word (32-bits) to uC memory (Little-endian)
 *
 *  Put a word at the given location and increase passed pointer by 4.
 *
 *  @param ptr    Pointer to pointer to uC memory location
 *  @param value  Value (32-bit) to copy
 */
void
plp_aperta2_put_word (uint8_t **ptr, uint32_t value)
{
    **ptr = (uint8_t) (value & 0xFF);
    (*ptr)++;
    value >>= 8;

    **ptr = (uint8_t) (value & 0xFF);
    (*ptr)++;
    value >>= 8;

    **ptr = (uint8_t) (value & 0xFF);
    (*ptr)++;
    value >>= 8;

    **ptr = (uint8_t) (value & 0xFF);
    (*ptr)++;
}

/*!
 *  Function to get a word (16-bits) from  memory
 *
 *  Get a half-word at the given location and increase passed pointer by 4.
 *
 *  @param ptr    Pointer to pointer to uC memory location
 *
 *  @return  Value (32-bit) read from the memory
 */
uint32_t
plp_aperta2_get_word (uint8_t **ptr)
{
    uint32_t value;

    value = **ptr;
    (*ptr)++;

    value += ((**ptr) << 8);
    (*ptr)++;

    value += ((**ptr) << 16);
    (*ptr)++;

    value += ((**ptr) << 24);
    (*ptr)++;

    return (value);
}

/*!
 *  Function to send a message to chip-level firmware
 *
 *  This function sends a message, waits for the firmware to process the message
 *  and finally reads back the response from firmware.
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int
plp_aperta2_msg_send (const plp_aperta2_phymod_phy_access_t *phy,  uint8_t function, uint8_t operation,
                  uint8_t *tx_msg, uint8_t *rx_msg, uint8_t *result)
{
    uint32_t msgout = 0, msgin = 0, length = 0, value = 0;
    uint32_t addr = APERTA2_MSG_INTF0_IN_BUFFER_ADDR, retry_cnt = APERTA2_FW_MSG_RETRY_CNT;

    length = plp_aperta2_get_half_word (&tx_msg);
    PHYMOD_IF_ERR_RETURN(
          PHYMOD_BUS_WRITE(&phy->access, addr, length));
    addr++;
    length = (length + 1) / 2;
    while (length) {
        value = plp_aperta2_get_half_word (&tx_msg);
          PHYMOD_IF_ERR_RETURN(
               PHYMOD_BUS_WRITE(&phy->access, addr, value));
        addr++;
        length--;
    }
    /* Trigger the message processing by writing to MSGIN register*/
    if (APERTA2_FW_FUN_SUPPORT_START_RESULT(function) && 
            (operation != APERTA2_OP_START_RESULT)) {
        msgin = (function << 8) | (operation << 4) | (APERTA2_STS_SENT << 0);
        PHYMOD_IF_ERR_RETURN(
           PHYMOD_BUS_WRITE(&phy->access, APERTA2_MSG_INTF0_MSGIN_ADR, msgin));
    }
    /* Adding another IF because START_RESULT and READ operations hold the same value,
     * handling in the same IF is not possible*/
    if (!APERTA2_FW_FUN_SUPPORT_START_RESULT(function)) {
        msgin = (function << 8) | (operation << 4) | (APERTA2_STS_SENT << 0);
        PHYMOD_IF_ERR_RETURN(
           PHYMOD_BUS_WRITE(&phy->access, APERTA2_MSG_INTF0_MSGIN_ADR, msgin));
    }

    /* Wait for the APERTA2_STS_PROCD response from firmware*/
    do {
        if (APERTA2_FW_FUN_SUPPORT_START_RESULT(function) &&
             (operation == APERTA2_OP_START_RESULT)) {
            msgin = (function << 8) | (operation << 4) | (APERTA2_STS_SENT << 0);
            PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_WRITE(&phy->access, APERTA2_MSG_INTF0_MSGIN_ADR, msgin));
         }
         PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access, APERTA2_MSG_INTF0_MSGOUT_ADR, &msgout));
#ifdef WIN32
        PHYMOD_USLEEP(2);
#else
        /* PHYMOD_USLEEP(100); */
#endif
    } while (((msgout & 0xFF0F) != ((function << 8) | (APERTA2_STS_PROCD << 0))) && (--retry_cnt));

    if (retry_cnt == 0) {
         PHYMOD_DEBUG_ERROR(("Timeout in getting response from FW\n"));
         return PHYMOD_E_TIMEOUT;
    }
    /*PHYMOD_DEBUG_INFO(("MSGout:%x rt:%d\n", msgout, retry_cnt));*/
    *result = (msgout >> 4) & 0x000F;

    /* Skip reading the response when op=APERTA2_OP_WRITE and result=APERTA2_OP_SUCCESS*/
    if ((APERTA2_OP_WRITE == operation) && (APERTA2_OP_SUCCESS == *result)) {
        plp_aperta2_put_half_word (&rx_msg, 0x0000);
        return (PHYMOD_E_NONE);
    }

    /* Adding else if because below code needs to be executed for all  
     * error cases and it needs to be executed for PASS case only for read and 
     * read EXT operations.
     * Since read and start result operations hold the same value, we are using 
     * function and operation together as a checkpoint */
    if (APERTA2_OP_ERROR == *result ) {
        APERTA2_OUTPUT_BUFFER_READ
    } else if (!APERTA2_FW_FUN_SUPPORT_START_RESULT(function) && (operation == APERTA2_OP_READ ||
            operation == APERTA2_OP_READ_EXT)) {
        APERTA2_OUTPUT_BUFFER_READ
    }

    return (PHYMOD_E_NONE);
}

/*!
 *  Function to perform FW write operation
 *
 *  This function construct write buffer based on register block type and 
 *  Write in to FW by calling plp_aperta2_msg_send API
 *
 *  @param   phy         phymod access structure 
 *  @param   block       Register block
 *  @param   data        input structure based on block
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_fw_write_register (const plp_aperta2_phymod_phy_access_t *phy, int block, void* data)
{
    uint8_t tx_buf[50], rx_buf[50];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0;
    uint8_t data_ptr[8];
    aperta2_pm_reg_t *pm_reg = NULL;
    aperta2_tsc_reg_t *tsc_reg = NULL;
    aperta2_ind_reg_t *ind_reg = NULL;
    aperta2_macsec_reg_t *macsec_reg = NULL;
    aperta2_ahb_reg_t *ahb_reg = NULL;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));
    PHYMOD_MEMSET(data_ptr, 0, sizeof(uint8_t)*8);

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    if (block == APERTA2_FUNC_PM_REGS) {
        pm_reg = (aperta2_pm_reg_t*)data;
        /* Prepare the Message*/
        /* Length, PMSel, AccessType, DataLen, Mask, Cnt, Addr, WrData*/
        for (index =0; index < pm_reg->datalen; index++) {
            data_ptr[index] = (*pm_reg->data >> (index*8) & 0xFF);
        }
        plp_aperta2_put_half_word (&tx_msg, (0x0007 + pm_reg->datalen));            /* Length*/     
        plp_aperta2_put_byte (&tx_msg, pm_reg->pm_sel);         /* PMSel*/
        plp_aperta2_put_byte (&tx_msg, pm_reg->access_type);    /* AccessType*/
        plp_aperta2_put_byte (&tx_msg, pm_reg->datalen);        /* DataLen*/
        plp_aperta2_put_byte (&tx_msg, 0x00);                   /* Mask*/
        plp_aperta2_put_byte (&tx_msg, 0x01);                   /* Cnt*/
        plp_aperta2_put_half_word (&tx_msg, pm_reg->addr);      /* Addr*/
        index = 0;
        while (pm_reg->datalen--) {
            /* WrData*/
            plp_aperta2_put_byte (&tx_msg, data_ptr[index]);
            index++;
        }
    } else if (block == APERTA2_FUNC_TSC_REGS) {
        tsc_reg = (aperta2_tsc_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 9);                          /* Length*/
        plp_aperta2_put_byte (&tx_msg, tsc_reg->pm_sel);                 /* SideSel*/
        plp_aperta2_put_byte (&tx_msg, 0x00);                            /* Mask*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                             /* Cnt*/
        plp_aperta2_put_word (&tx_msg, tsc_reg->addr);                   /* Addr*/
        plp_aperta2_put_half_word (&tx_msg, (*tsc_reg->data & 0xFFFF));  /* data*/
    } else if (block == APERTA2_FUNC_CHIP_IND_REGS) {
        ind_reg = (aperta2_ind_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 7);                          /* Length*/
        plp_aperta2_put_byte (&tx_msg, ind_reg->oct_sel);                /* octal select*/
        plp_aperta2_put_byte (&tx_msg, 0x00);                            /* Mask*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                             /* Cnt*/
        plp_aperta2_put_half_word (&tx_msg, ind_reg->addr);              /* Addr*/
        plp_aperta2_put_half_word (&tx_msg, (*ind_reg->data & 0xFFFF));  /* data*/
    } else if (block == APERTA2_FUNC_MACSEC_REGS) {
        macsec_reg = (aperta2_macsec_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 10);                         /* Length*/
        plp_aperta2_put_byte (&tx_msg, macsec_reg->oct_sel);             /* octal select*/
        plp_aperta2_put_byte (&tx_msg, 0x00);                            /* Mask*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                             /* Cnt*/
        plp_aperta2_put_24bits (&tx_msg, macsec_reg->addr);              /* Addr - 24 bits*/
        plp_aperta2_put_word (&tx_msg, *macsec_reg->data);               /* data*/
    } else if (block == APERTA2_FUNC_AHB_REGS) {
        ahb_reg = (aperta2_ahb_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 10);                         /* Length*/
        plp_aperta2_put_byte (&tx_msg, 0x00);                            /* Mask*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                             /* Cnt*/
        plp_aperta2_put_word (&tx_msg, ahb_reg->addr);                   /* Addr - 32 bits*/
        plp_aperta2_put_word (&tx_msg, *ahb_reg->data);                  /* data*/
    } else if (block == APERTA2_FUNC_CHIP_DIR_REGS) {
        ahb_reg = (aperta2_ahb_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 6);                          /* Length*/
        plp_aperta2_put_byte (&tx_msg, 0x00);                            /* Mask*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                             /* Cnt*/
        plp_aperta2_put_half_word (&tx_msg, ahb_reg->addr);              /* Addr - 16 bits*/
        plp_aperta2_put_half_word (&tx_msg, *ahb_reg->data);             /* data*/
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid Block in write\n"));
        return PHYMOD_E_PARAM;
    }

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
          plp_aperta2_msg_send (phy, block, APERTA2_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("Write failed :%d for Block:0x%x\n", result, block));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }

    (void) rx_msg;

    return (0);
} 

/*!
 *  Function to perform FW read operation
 *
 *  This function construct read buffer based on register block type and 
 *  read from FW by calling plp_aperta2_msg_send API
 *
 *  @param   phy         phymod access structure 
 *  @param   block       Register block
 *  @param   data        input/output structure based on block
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_fw_read_register (const plp_aperta2_phymod_phy_access_t *phy, int block, void *data)
{
    uint8_t tx_buf[50], rx_buf[50], data_ptr[8];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0;
    aperta2_pm_reg_t *pm_reg = NULL;
    aperta2_tsc_reg_t *tsc_reg = NULL;
    aperta2_ind_reg_t *ind_reg = NULL;
    aperta2_macsec_reg_t *macsec_reg = NULL;
    aperta2_ahb_reg_t *ahb_reg = NULL;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    /* Prepare the Message*/
    /* Length, PMSel, AccessType, DataLen, Cnt, Addr*/
    if (block == APERTA2_FUNC_PM_REGS) {
        pm_reg = (aperta2_pm_reg_t*)data;
        COMPILER_64_ZERO(*pm_reg->data);
        plp_aperta2_put_half_word (&tx_msg, 0x6);                 /* Length*/
        plp_aperta2_put_byte (&tx_msg, pm_reg->pm_sel);           /* PMSel*/
        plp_aperta2_put_byte (&tx_msg, pm_reg->access_type);      /* AccessType*/
        plp_aperta2_put_byte (&tx_msg, pm_reg->datalen);          /* DataLen*/
        plp_aperta2_put_byte (&tx_msg, 0x01);                     /* Cnt*/
        plp_aperta2_put_half_word (&tx_msg, pm_reg->addr);        /* Addr*/
    } else if (block == APERTA2_FUNC_TSC_REGS) {
        tsc_reg = (aperta2_tsc_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 6);                   /* Length*/
        plp_aperta2_put_byte (&tx_msg, tsc_reg->pm_sel);          /* PMSel*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                      /* Cnt*/
        plp_aperta2_put_word (&tx_msg, tsc_reg->addr);            /* Addr*/
    } else if (block == APERTA2_FUNC_CHIP_IND_REGS) {
        ind_reg = (aperta2_ind_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 4);                   /* Length*/
        plp_aperta2_put_byte (&tx_msg, ind_reg->oct_sel);         /* octal Sel*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                      /* Cnt*/
        plp_aperta2_put_half_word (&tx_msg, ind_reg->addr);       /* Addr*/
    } else if (block == APERTA2_FUNC_MACSEC_REGS) {
        macsec_reg = (aperta2_macsec_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 5);                   /* Length*/
        plp_aperta2_put_byte (&tx_msg, macsec_reg->oct_sel);      /* octal sel*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                      /* Cnt*/
        plp_aperta2_put_24bits (&tx_msg, macsec_reg->addr);       /* Addr*/
    } else if (block == APERTA2_FUNC_AHB_REGS) {
        ahb_reg = (aperta2_ahb_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 5);                   /* Length*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                      /* Cnt*/
        plp_aperta2_put_word (&tx_msg, ahb_reg->addr);            /* Addr*/
    } else if (block == APERTA2_FUNC_CHIP_DIR_REGS) {
        ahb_reg = (aperta2_ahb_reg_t*)data;
        plp_aperta2_put_half_word (&tx_msg, 3);                   /* Length*/
        plp_aperta2_put_byte (&tx_msg, 0x1);                      /* Cnt*/
        plp_aperta2_put_half_word (&tx_msg, ahb_reg->addr);       /* Addr - 16 bits*/
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid Block\n"));
        return PHYMOD_E_PARAM;
    }

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
          plp_aperta2_msg_send (phy, block, APERTA2_OP_READ, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW READ failed :%d Block:0x%x\n", result, block));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    /* Read the response*/
    if (block == APERTA2_FUNC_PM_REGS) {
        index = 0;
        if (pm_reg) {
            COMPILER_64_ZERO(*pm_reg->data);
            if (pm_reg->datalen == plp_aperta2_get_half_word (&rx_msg)) {
                while (pm_reg->datalen--) {
                    data_ptr[index] = plp_aperta2_get_byte (&rx_msg);
                    *pm_reg->data |= ((uint64_t)data_ptr[index] << (index*8));
                    index++;
                }
                if ((*pm_reg->data  & 0xBEEFDEAD) == 0xBEEFDEAD) {
                    PHYMOD_DEBUG_ERROR(("PM READ failed for addr:%x accesstyp:%x phy:%x \n", pm_reg->addr, pm_reg->access_type, phy->access.addr));
                    return PHYMOD_E_INTERNAL;
                }
                return PHYMOD_E_NONE;
            } else {
                PHYMOD_DEBUG_ERROR(("PM READ failed with data len(%d)  read len:%d not matching\n", pm_reg->datalen, rx_buf[0]));
                return PHYMOD_E_INTERNAL;
            }
        }
    } else {
        length = plp_aperta2_get_half_word (&rx_msg);
        if (length == 2) {
            if (block == APERTA2_FUNC_TSC_REGS) {
                if (tsc_reg) {
                    *tsc_reg->data = plp_aperta2_get_half_word (&rx_msg);
                }
            } else if (block == APERTA2_FUNC_CHIP_DIR_REGS) {
                if (ahb_reg) {
                    *ahb_reg->data = plp_aperta2_get_half_word (&rx_msg);
                }
            } else {
                if (ind_reg) {
                    *ind_reg->data = plp_aperta2_get_half_word (&rx_msg);
                }
            }
            return PHYMOD_E_NONE;
        } else if (length == 4) {
            if (block == APERTA2_FUNC_MACSEC_REGS) {
                if (macsec_reg) {
                    *macsec_reg->data = plp_aperta2_get_half_word (&rx_msg);
                    *macsec_reg->data |= (plp_aperta2_get_half_word (&rx_msg) << 16);
                }
            } 
            if (block == APERTA2_FUNC_AHB_REGS) {
                if (ahb_reg) {
                    *ahb_reg->data = plp_aperta2_get_half_word (&rx_msg);
                    *ahb_reg->data |= (plp_aperta2_get_half_word (&rx_msg) << 16);
                }
            }
            return PHYMOD_E_NONE;
        } else {
            PHYMOD_DEBUG_ERROR(("READ failed with data lesser than expected::%d-Block:%d\n", length, block));
            return PHYMOD_E_INTERNAL;
        }
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to perform FW memory operation
 *
 *  This function construct read buffer based on register block type and 
 *  read from FW by calling plp_aperta2_msg_send API
 *
 *  @param   phy         phymod access structure 
 *  @param   block       Register block
 *  @param   data        input/output structure based on block
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_fw_reg_access(const plp_aperta2_phymod_phy_access_t* phy, int write_en, int block, void *data) 
{

    /* Get data size for LMI Transaction*/
    if (write_en) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_fw_write_register (phy, block, data));
    } else {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_fw_read_register(phy, block, data));
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to read/write in to MEMORY.
 *
 *  @param phy       phy Configuration details
 *  @param mem       memory detail i.e. PM sel, Mem type, index etc
 *  @param write_en  1 represents write, read otherwise
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_fw_mem_access(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_reg_t *mem, int write_en) 
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t residual = 0, res_index = 0;
    uint8_t *tx_msg, *rx_msg, result = 0;
    int index = 0, temp = 0;
    /* Get data size for LMI Transaction*/
    /* Prepare the Message*/
    /* Length, PMSel, AccessType, DataLen, Mask, Cnt, Addr, WrData*/
    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    if (write_en) {
        plp_aperta2_put_half_word (&tx_msg, (0x0007 + mem->datalen));            /* Length*/     
    } else {
        plp_aperta2_put_half_word (&tx_msg, 0x6);                 /* Length*/
    }
    plp_aperta2_put_byte (&tx_msg, mem->pm_sel);         /* PMSel*/
    plp_aperta2_put_byte (&tx_msg, mem->access_type);    /* AccessType*/
    plp_aperta2_put_byte (&tx_msg, mem->datalen);        /* DataLen*/
    if (write_en) {
        plp_aperta2_put_byte (&tx_msg, 0x00);                   /* Mask*/
    }
    plp_aperta2_put_byte (&tx_msg, 0x01);                   /* Cnt*/
    plp_aperta2_put_half_word (&tx_msg, mem->addr);      /* Addr*/
    if (write_en) {
        for (index = 0; index < (mem->datalen/4); index++) {  /* Copying 4 bytes*/
            plp_aperta2_put_word(&tx_msg, mem->mem_data[index]); /* 32 - data*/
        }
        residual = (mem->datalen % 4);
        while (residual) { /* Copying residual*/
            plp_aperta2_put_byte(&tx_msg, ((mem->mem_data[index] >> (8*res_index)) & 0xFF)); /* 8 bit - data*/
            residual --;
            res_index++;
        }
    }
    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
      plp_aperta2_msg_send (phy, APERTA2_FUNC_PM_REGS,  
          (write_en == 0) ? APERTA2_OP_READ : APERTA2_OP_WRITE, 
          &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW MEM %s failed :%d\n", (write_en == 0) ? "Read":"Write", result));
        temp = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+temp; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    if (write_en == 1) {
        return PHYMOD_E_NONE;
    }

    temp = plp_aperta2_get_half_word (&rx_msg);
    if (temp == mem->datalen) {
        for (index = 0; (index < temp/4); index++) {
            mem->mem_data[index] = rx_msg[index*4] | (rx_msg[(index*4)+1] << 8) | (rx_msg[(index*4)+2] << 16) | (rx_msg[(index*4)+3] << 24);
        }
        mem->mem_data[index] = 0;
        residual = temp %4;
        temp = 0;
        while (residual) {
           mem->mem_data[index] |=  ((rx_msg[(index*4)+temp]) << (temp *8));
           residual --;
           temp++;
        }
    } else {
        PHYMOD_DEBUG_ERROR(("PM READ failed with data len(%d) not matching\n", temp));
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to configure a Phy (CONFIG_PHY.WRITE)
 *
 *  @param phy       phy Configuration
 *  @param phy_cfg   Pointer to phy configuration structure
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int
plp_aperta2_fw_config_phy(const plp_aperta2_phymod_phy_access_t *phy,  aperta2_config_phy_t *phy_cfg)
{
    uint8_t tx_buf[50], rx_buf[50];
     uint8_t *tx_msg, *rx_msg, result = 0;
     uint16_t length = 0, index = 0;

     PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
     PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    /* use the global buffers for tx/rx message*/
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    length = 0;

    if (APERTA2_OP_WRITE == phy_cfg->port_operation) {
        /* Prepare the Message
           Length, PortNum, PortType, PortData*/
        plp_aperta2_put_half_word (&tx_msg, 0x0000);           /* Length (real length inserted at the end*/
        plp_aperta2_put_byte (&tx_msg, phy_cfg->macsec_option);    /* MACsecOpt*/
        plp_aperta2_put_byte (&tx_msg, phy_cfg->ptp_option);        /* ptp option*/
        length += 2;
    }

    /* Length (at the first location)*/
    tx_msg = &tx_buf[0];
    plp_aperta2_put_half_word (&tx_msg, length);

    /* Send Message*/
     PHYMOD_IF_ERR_RETURN(
         plp_aperta2_msg_send (phy, APERTA2_FUNC_CONFIG_PHY, phy_cfg->port_operation, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in Config PHY:0x%x\n", result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return (PHYMOD_E_INTERNAL);
    }
    if (APERTA2_OP_WRITE == phy_cfg->port_operation) {
        return (PHYMOD_E_NONE);
    }
    /* Read the response*/
    length = plp_aperta2_get_half_word (&rx_msg);
    if (length >= 2) {
        phy_cfg->macsec_option = plp_aperta2_get_byte (&rx_msg);
        phy_cfg->ptp_option = plp_aperta2_get_byte (&rx_msg);
        return (PHYMOD_E_NONE);
    } else {
        PHYMOD_DEBUG_ERROR(("FW : Error in Config getting config PHY:0x%x\n", plp_aperta2_get_half_word (&rx_msg)));
        return (PHYMOD_E_INTERNAL);
    }
    return (0);
}

/*!
 *  Function to configure a Port (CONFIG_PORT.WRITE/READ)
 *
 *  @param phy       PHY Configuration
 *  @param port_cfg  Pointer to port configuration structure
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int
plp_aperta2_fw_config_port (const plp_aperta2_phymod_phy_access_t *phy,  aperta2_config_port_t *port_cfg)
{
    uint8_t tx_buf[50], rx_buf[50];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length =0, index = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    length = 0;

    /* Prepare the Message
       Length, PortNum, PortType, PortData*/
    plp_aperta2_put_half_word (&tx_msg, 0x0000);            /* update lenght at lasT*/
    plp_aperta2_put_byte (&tx_msg, port_cfg->PortNum);      /* PortNum      */
    length += 1;
    if (APERTA2_OP_WRITE == port_cfg->port_operation) {
        plp_aperta2_put_byte (&tx_msg, port_cfg->PortType);     /* PortType      */
        plp_aperta2_put_byte (&tx_msg, port_cfg->PortMode);     /* PortMode      */
        plp_aperta2_put_byte (&tx_msg, port_cfg->SysPortSpeed);  /* Sys PortSpeed      */
        plp_aperta2_put_byte (&tx_msg, port_cfg->LinePortSpeed); /* Line PortSpeed      */
        plp_aperta2_put_byte (&tx_msg, port_cfg->SPMPortID);    /* SPMPortID      */
        plp_aperta2_put_byte (&tx_msg, port_cfg->LPMPortID);    /* LPMPortID      */
        plp_aperta2_put_half_word (&tx_msg, port_cfg->PortOptions);  /* PortOptions*/
        length += 8;
        if (port_cfg->PortOptions & 0x10)  {
            plp_aperta2_put_half_word (&tx_msg, port_cfg->IngFixedLatency);
            length += 2;
        }
        if (port_cfg->PortOptions & 0x40)  {
            plp_aperta2_put_half_word (&tx_msg, port_cfg->EgrFixedLatency);
            length += 2;
        }
        if (port_cfg->PortMode) {
            /* Failover MUX Port*/
            plp_aperta2_put_byte (&tx_msg, port_cfg->FOOptions);      /* FOOptions*/
            plp_aperta2_put_byte (&tx_msg, port_cfg->FOPortID);       /* FOPortID*/
            length += 2;
        }
    }    
    /* Length (at the first location)*/
    tx_msg = &tx_buf[0];
    plp_aperta2_put_half_word (&tx_msg, length);
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_msg_send (phy, APERTA2_FUNC_CONFIG_PORT, port_cfg->port_operation , &tx_buf[0], &rx_buf[0], &result));
    /* Send Message*/
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in Config setting config port:0x%x\n", result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return (PHYMOD_E_INTERNAL);
    }
    if (APERTA2_OP_WRITE == port_cfg->port_operation) {
        return (PHYMOD_E_NONE);
    }
    /*Read the response for READ operation*/
    length = plp_aperta2_get_half_word (&rx_msg);

    /* length should at least be 9 bytes or more*/
    if (length >= 9) {
        port_cfg->PortNum      = plp_aperta2_get_byte (&rx_msg);         /* PortNum*/
        port_cfg->PortType     = plp_aperta2_get_byte (&rx_msg);        /* PortType*/
        port_cfg->PortMode     = plp_aperta2_get_byte (&rx_msg);        /* PortMode*/
        port_cfg->SysPortSpeed = plp_aperta2_get_byte (&rx_msg);        /* SysPortSpeed*/
        port_cfg->LinePortSpeed= plp_aperta2_get_byte (&rx_msg);        /* Line PortSpeed*/
        port_cfg->SPMPortID    = plp_aperta2_get_byte (&rx_msg);        /* SPMPortID*/
        port_cfg->LPMPortID    = plp_aperta2_get_byte (&rx_msg);        /* LPMPortID*/
        port_cfg->PortOptions  = plp_aperta2_get_half_word (&rx_msg); /* PortOptions*/

        if (port_cfg->PortOptions & 0x0010) {/* Ingress Fixed-Latency Enabled*/
            port_cfg->IngFixedLatency = plp_aperta2_get_half_word (&rx_msg);
        }
        if (port_cfg->PortOptions & 0x0040) { /* Egress Fixed-Latency Enabled*/
            port_cfg->EgrFixedLatency = plp_aperta2_get_half_word (&rx_msg);
        }
        if (port_cfg->PortMode) {
            /* Failover MUX Port*/
            port_cfg->FOOptions = plp_aperta2_get_byte (&rx_msg);      /* FOOptions*/
            port_cfg->FOPortID  = plp_aperta2_get_byte (&rx_msg);      /* FOPortID*/
        }
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid length for config port get:%d\n", length));
        return (PHYMOD_E_INTERNAL);
    }
    
    return (PHYMOD_E_NONE);
}

/* Port operation such as Enable, Disable, flush, pause, resume port
 *
 *  @param phy       phy Configuration
 *  @param port_num  Port number
 *  @param port_function port operations to be performed
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
static int
plp_aperta2_fw_port_op_start (const plp_aperta2_phymod_phy_access_t *phy,  uint8_t port_num, int port_function)
{
    uint8_t tx_buf[50], rx_buf[50];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint32_t length=0, index = 0, op = 0, op_check = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    if ((port_function != APERTA2_FUNC_ENABLE_PORT) &&(port_function != APERTA2_FUNC_DISABLE_PORT) &&
            (port_function != APERTA2_FUNC_FLUSH_PORT) && (port_function != APERTA2_FUNC_PAUSE_PORT)
            && (port_function != APERTA2_FUNC_RESUME_PORT)) {
        PHYMOD_DEBUG_ERROR(("Invalid FW message port_function(plp_aperta2_fw_port_op_start)\n"));
        return PHYMOD_E_PARAM;
    }
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /*Prepare the Message
      Length, PortNum*/
    if (port_function == APERTA2_FUNC_FLUSH_PORT) {
        plp_aperta2_put_half_word (&tx_msg, 0x0001);           /* Length*/
        plp_aperta2_put_byte (&tx_msg, port_num);              /* PortNum*/
    } else {
        if (port_function == APERTA2_FUNC_PAUSE_PORT) {
            plp_aperta2_put_half_word (&tx_msg, 0x0002);           /* Length*/
            plp_aperta2_put_byte (&tx_msg, port_num);              /* PortNum*/
            plp_aperta2_put_byte (&tx_msg, 0);                     /* pause state*/
        } else {
            plp_aperta2_put_half_word (&tx_msg, 0x0001);           /* Length*/
            plp_aperta2_put_byte (&tx_msg, port_num);              /* PortNum*/

        }
    }
    (void) rx_msg;
    if ((port_function != APERTA2_FUNC_PAUSE_PORT) &&
            (port_function != APERTA2_FUNC_RESUME_PORT)) {
        op = APERTA2_OP_START; 
        op_check = APERTA2_OP_PROCESSING;
    } else {
        op = APERTA2_OP_WRITE; 
        op_check = APERTA2_OP_SUCCESS;
    }
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_msg_send (phy, port_function, op, &tx_buf[0], &rx_buf[0], &result));
    
    if (op_check != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in port port_function:0x%x start:0x%x\n", port_function, result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to wait until the process of enabling a Port is complete
 *  @param phy       phy Configuration
 *  @param port_num  Port number
 *  @param port_function port function to be performed
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
static int
plp_aperta2_fw_port_op_result (const plp_aperta2_phymod_phy_access_t *phy,  uint8_t port_num, int port_function)
{
    uint8_t tx_buf[50], rx_buf[50];
    uint8_t *tx_msg, *rx_msg;
    uint8_t result =0;
    unsigned int retry_cnt = APERTA2_FW_MSG_RETRY_CNT, length = 0, index = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    if ((port_function != APERTA2_FUNC_ENABLE_PORT) &&(port_function != APERTA2_FUNC_DISABLE_PORT) &&
            (port_function != APERTA2_FUNC_FLUSH_PORT)) {
        PHYMOD_DEBUG_ERROR(("Invalid FW message function(plp_aperta2_fw_port_op_result)\n"));
        return PHYMOD_E_PARAM;
    }
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, PortNum*/
    plp_aperta2_put_half_word (&tx_msg, 0x0001);   /* Length     */
    plp_aperta2_put_byte (&tx_msg, port_num);      /* PortNum*/
    do {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_msg_send (phy, port_function, APERTA2_OP_START_RESULT, &tx_buf[0], &rx_buf[0], &result));
        PHYMOD_USLEEP(100);
    } while ((APERTA2_OP_PROCESSING == result) && (retry_cnt --));
    if (retry_cnt == 0) {
        PHYMOD_DEBUG_ERROR(("Port Operation:%x is not getting completed:%d\n", port_function, result));
        return PHYMOD_E_INTERNAL;
    }
    if (APERTA2_OP_SUCCESS == result) {
        return PHYMOD_E_NONE;
    } else {
        PHYMOD_DEBUG_ERROR(("Port Func:0x%x result:%d\n", port_function, result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :0x%x\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to perform Port functions and wait until the process is complete
 *
 *  @param phy       phy Configuration
 *  @param port_num  Port number
 *  @param port_function  Port port_function
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int
plp_aperta2_fw_port_op (const plp_aperta2_phymod_phy_access_t *phy,  uint8_t port_num, int port_function)
{
    PHYMOD_IF_ERR_RETURN(plp_aperta2_fw_port_op_start (phy, port_num, port_function));
    if ((port_function != APERTA2_FUNC_PAUSE_PORT) &&
            (port_function != APERTA2_FUNC_RESUME_PORT)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_fw_port_op_result (phy, port_num, port_function));
    }

    return PHYMOD_E_NONE;
}

/*!
 *  Function to switch datapath clock
 *
 *  @param phy       phy Configuration
 *  @param operation  Start/Start result for switching datapath clock
 *  @param switch_dp_clk octal and clock select for datapath 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_switch_dp_clock(const plp_aperta2_phymod_phy_access_t *phy, int operation, aperta2_switch_dpclk_t switch_dp_clk)
{
    uint8_t tx_buf[50], rx_buf[50];
    uint8_t *tx_msg, *rx_msg;
    uint8_t result =0, op_check = 0, length = 0;
    unsigned int retry_cnt = APERTA2_FW_MSG_RETRY_CNT, index = 0;
 
    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    /* use the global buffers for tx/rx message*/
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, OctalSel, ClkSel*/
    if (operation == APERTA2_OP_START) {
        plp_aperta2_put_half_word(&tx_msg, 0x0002);                  /* Length   */
        op_check = APERTA2_OP_PROCESSING;
    } else {
        plp_aperta2_put_half_word(&tx_msg, 0x0001);                  /* Length   */
        op_check = APERTA2_OP_SUCCESS;
    }
    plp_aperta2_put_byte(&tx_msg, switch_dp_clk.OctalSel);       /* OctalSel*/
    plp_aperta2_put_byte(&tx_msg, switch_dp_clk.ClkSel);         /* ClkSel - Dont care for result as length is 1*/

    (void) rx_msg;
    
    /* Send Message*/
    do {
        PHYMOD_IF_ERR_RETURN(
              plp_aperta2_msg_send (phy, APERTA2_FUNC_SWITCH_DPCLK, operation, &tx_buf[0], &rx_buf[0], &result));
        if (result == APERTA2_OP_ERROR) {
             PHYMOD_DEBUG_ERROR(("FW : Error in dp clock switch:0x%x \n", operation));
             length = plp_aperta2_get_half_word (&rx_msg);
             for(index = 2; index < 2+length; index++) {
                PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
             }
            return PHYMOD_E_INTERNAL;
        }
        PHYMOD_USLEEP(100);
    } while ((op_check != result) && (retry_cnt--));
    if (retry_cnt == 0) {
        PHYMOD_DEBUG_ERROR(("FW : Error in dp clock switch:Retry count expired \n"));
        return PHYMOD_E_NONE;
    }

    return PHYMOD_E_NONE;
}

/*!
 *  Function to configure PTP
 *
 *  @param phy         phy Configuration
 *  @param ptp_cfg     PTP configuration such as latency value and port options 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_fw_config_ptp_write (const plp_aperta2_phymod_phy_access_t* phy, aperta2_ptp_config_t *ptp_cfg)
{
    uint8_t tx_buf[8], rx_buf[8];
    uint8_t *tx_msg = NULL;
    uint8_t *rx_msg = NULL;
    uint8_t result  = 0;
    uint16_t length = 0;
    uint32_t index  = 0;

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    length = 0;

    /* Length (real length inserted at the end */
    plp_aperta2_put_half_word (&tx_msg, 0x0000);

    plp_aperta2_put_byte (&tx_msg, ptp_cfg->PortNum);      /* PortNum */
    plp_aperta2_put_byte (&tx_msg, ptp_cfg->PortOptions);  /* PortOptions */
    length += 2;

    /* Ingress Fixed-Latency Enabled */
    if (ptp_cfg->PortOptions & 0x01) {
        plp_aperta2_put_half_word (&tx_msg, ptp_cfg->IngFixedLatency);
        length += 2;
    }
    /* Egress Fixed-Latency Enabled */
    if (ptp_cfg->PortOptions & 0x04) {
        plp_aperta2_put_half_word (&tx_msg, ptp_cfg->EgrFixedLatency);
        length += 2;
    }
    /* Length (at the first location) */
    tx_msg = &tx_buf[0];
    plp_aperta2_put_half_word (&tx_msg, length);

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(plp_aperta2_msg_send (phy, APERTA2_FUNC_PTP_CONFIG, APERTA2_OP_WRITE , &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in PTP_CONFIG.Write():0x%x\n", result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return (PHYMOD_E_INTERNAL);
    }
    return (PHYMOD_E_NONE);
}

/*!
 *  Function to retrive PTP configurations
 *
 *  @param phy         phy Configuration
 *  @param ptp_cfg     OUT, PTP configuration such as latency value and port options 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_fw_config_ptp_read (const plp_aperta2_phymod_phy_access_t* phy, aperta2_ptp_config_t *ptp_cfg)
{
    uint8_t tx_buf[8], rx_buf[8];
    uint8_t *tx_msg = NULL;
    uint8_t *rx_msg = NULL;
    uint8_t result  = 0;
    uint16_t length = 0;
    uint32_t index  = 0;

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    plp_aperta2_put_half_word (&tx_msg, 0x0001);
    plp_aperta2_put_byte (&tx_msg, ptp_cfg->PortNum);

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(plp_aperta2_msg_send (phy, APERTA2_FUNC_PTP_CONFIG, APERTA2_OP_READ, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in PTP_CONFIG.Read():0x%x\n", result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }

    /* Read the response */
    length = plp_aperta2_get_half_word(&rx_msg);

    /* length should atleast be 2 bytes or more */
    if (length >= 2) {
        ptp_cfg->PortNum = plp_aperta2_get_byte (&rx_msg);      /* PortNum */
        ptp_cfg->PortOptions = plp_aperta2_get_byte (&rx_msg);  /* PortOptions */

        /* Ingress Fixed-Latency Enabled */
        if (ptp_cfg->PortOptions & 0x01) {
            ptp_cfg->IngFixedLatency = plp_aperta2_get_half_word (&rx_msg);
        }

        /* Egress Fixed-Latency Enabled */
        if (ptp_cfg->PortOptions & 0x04) {
            ptp_cfg->EgrFixedLatency = plp_aperta2_get_half_word (&rx_msg);
        }
        return (PHYMOD_E_NONE);
    } else {
        PHYMOD_DEBUG_ERROR(("PTP config READ failed with data len(%d) not matching\n", length));
        return (PHYMOD_E_INTERNAL);
    }
    return (PHYMOD_E_NONE);
}

/*!
 *  Function to configure lane swap
 *
 *  @param phy            phy Configuration
 *  @param config_lanes   lane swap  
 *  @param operation      Read/Write  
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_fw_config_lanes (const plp_aperta2_phymod_phy_access_t* phy, aperta2_config_lane_t *lane_cfg, int operation)
{
    uint8_t tx_buf[50], rx_buf[50];
    uint8_t *tx_msg, *rx_msg;
    uint8_t result =0, length = 0;
    unsigned int index = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    /* use the buffers for tx/rx message*/
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    
    if(operation == APERTA2_OP_WRITE ) {
        /* Prepare the Message
           num_sys_lane, num_line_lane, sys_lane_list,line_lane_list*/
        plp_aperta2_put_half_word(&tx_msg, 34);                       /* Length   */
        plp_aperta2_put_byte(&tx_msg, lane_cfg->num_sys_lane);        /* Number of system lanes*/
        plp_aperta2_put_byte(&tx_msg, lane_cfg->num_line_lane);       /* Number of line lanes*/
        for (index = 0; index < lane_cfg->num_sys_lane; index++) {
            plp_aperta2_put_byte(&tx_msg, lane_cfg->sys_lane_list[index]);     
        }
        for (index = 0; index < lane_cfg->num_line_lane; index++) {
            plp_aperta2_put_byte(&tx_msg, lane_cfg->line_lane_list[index]);     
        }

        (void) rx_msg;
    } else {
        plp_aperta2_put_half_word(&tx_msg, 0);                       /* Length   */
    }

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_msg_send (phy, APERTA2_FUNC_CONFIG_LANES, operation, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in CONFIG_LANES:0x%x\n", result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return (PHYMOD_E_INTERNAL);
    }
    if (operation == APERTA2_OP_READ) {
        /* Read the response */
        length = plp_aperta2_get_half_word(&rx_msg);

        /* length should atleast be 2 bytes or more */
        if (length >= 2) {
            lane_cfg->num_sys_lane = plp_aperta2_get_byte (&rx_msg);      /* Number of System lanes*/
            lane_cfg->num_line_lane = plp_aperta2_get_byte (&rx_msg);     /* Number of line lanes*/
            for (index = 0; index < lane_cfg->num_sys_lane; index++) {
                lane_cfg->sys_lane_list[index] = plp_aperta2_get_byte (&rx_msg);      /*System lanes*/
            }
            for (index = 0; index < lane_cfg->num_line_lane; index++) {
                lane_cfg->line_lane_list[index] = plp_aperta2_get_byte (&rx_msg);      /* Line lanes*/
            }
            return (PHYMOD_E_NONE);
        } else {
            PHYMOD_DEBUG_ERROR(("Lane config READ failed with data len(%d) not matching\n", length));
            return (PHYMOD_E_INTERNAL);
        }
 
    }

    return (PHYMOD_E_NONE);
}

/*!
 *  Function to configure failover port
 *
 *  @param phy            phy Configuration
 *  @param port_num       primary port number
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_fw_switch_mux (const plp_aperta2_phymod_phy_access_t* phy, int port_num)
{
    uint8_t tx_buf[56], rx_buf[56];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0, retry_cnt = APERTA2_FW_MSG_RETRY_CNT;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, PortNum*/
    plp_aperta2_put_half_word (&tx_msg, 0x1);             /* Length */
    plp_aperta2_put_byte (&tx_msg, port_num);             /* portNum*/
    (void) rx_msg;

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
          plp_aperta2_msg_send (phy, APERTA2_FUNC_SWITCH_MUX, APERTA2_OP_START, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_PROCESSING != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in Switch mux start:0x%x\n", result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length+2; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    do {
        PHYMOD_IF_ERR_RETURN(
               plp_aperta2_msg_send (phy, APERTA2_FUNC_SWITCH_MUX, APERTA2_OP_START_RESULT, &tx_buf[0], &rx_buf[0], &result));
    } while ((APERTA2_OP_PROCESSING == result) && (retry_cnt --));
    if (retry_cnt == 0) {
         PHYMOD_DEBUG_ERROR(("Switching Mux:%d is not getting completed:%d\n", port_num, result));
         return PHYMOD_E_INTERNAL;
    }
    (void) rx_msg;
    if (APERTA2_OP_SUCCESS == result) {
        return PHYMOD_E_NONE;
    } else {
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length+2; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to Read/Write Clock Gen 
 *
 *  @param phy            phy Configuration
 *  @param op             Firmware Read/Write
 *  @param clk_cfg        Pointer to clock configuration structure
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int plp_aperta2_fw_clock_gen (const plp_aperta2_phymod_phy_access_t *phy,  int op, aperta2_clock_gen_t *clk_cfg)
{
    uint8_t tx_buf[50], rx_buf[50];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, PortNum, PortType, PortData*/
    plp_aperta2_put_half_word (&tx_msg, 0x0000);    /* Length (real length inserted at the end*/
    plp_aperta2_put_byte (&tx_msg, clk_cfg->RClkNum);       /* RClkNum*/
    length += 1;
    if (APERTA2_OP_WRITE == op) {
        plp_aperta2_put_byte (&tx_msg, clk_cfg->ClkGenEn);      /* ClkGenE*/
        length += 1;
        if (clk_cfg->ClkGenEn) {
            plp_aperta2_put_byte (&tx_msg, clk_cfg->ClkPortNum);   /* Clk port number*/
            plp_aperta2_put_byte (&tx_msg, clk_cfg->ClkSide);   /* ClkSide*/
            plp_aperta2_put_byte (&tx_msg, clk_cfg->ClkLane);   /* ClkLane*/
            plp_aperta2_put_byte (&tx_msg, clk_cfg->Divider);   /* Divider*/
            plp_aperta2_put_byte (&tx_msg, clk_cfg->SquelchMode); /* SquelchMode*/
            length += 5;
        }
    }
    /* Length (at the first location)*/
    tx_msg = &tx_buf[0];
    plp_aperta2_put_half_word (&tx_msg, length);
    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_msg_send (phy, APERTA2_FUNC_CLOCK_GEN, op , &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("Clock gene Write failed :%d\n", result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    if (APERTA2_OP_READ == op) {
        /* Read the response */
        length = plp_aperta2_get_half_word(&rx_msg);

        /* length should atleast be 2 bytes or more */
        if (length >= 2) {
            clk_cfg->RClkNum = plp_aperta2_get_byte (&rx_msg);      /* RClkNum*/
            clk_cfg->ClkGenEn = plp_aperta2_get_byte (&rx_msg);      /* ClkGenEn*/
            if (clk_cfg->ClkGenEn) {
                clk_cfg->ClkPortNum = plp_aperta2_get_byte(&rx_msg);   /* Clk port number*/
                clk_cfg->ClkSide =  plp_aperta2_get_byte(&rx_msg);   /* ClkSide*/
                clk_cfg->ClkLane =  plp_aperta2_get_byte(&rx_msg);   /* ClkLane*/
                clk_cfg->Divider =  plp_aperta2_get_byte(&rx_msg);   /* Divider*/
                clk_cfg->SquelchMode = plp_aperta2_get_byte(&rx_msg); /* SquelchMode*/
            }
            return (PHYMOD_E_NONE);
        } else {
            PHYMOD_DEBUG_ERROR(("Lane config READ failed with data len(%d) not matching\n", length));
            return (PHYMOD_E_INTERNAL);
        }
    }
    (void) rx_msg;
    return (PHYMOD_E_NONE);
}

/*!
 *  Function to configure package polarity
 *
 *  @param phy            phy Configuration
 *  @param pm_sel         port macro select
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_fw_package_polarity_set (const plp_aperta2_phymod_phy_access_t* phy, int pm_sel)
{
    uint8_t tx_buf[56], rx_buf[56];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, PortNum*/
    plp_aperta2_put_half_word (&tx_msg, 0x1);             /* Length */
    plp_aperta2_put_byte (&tx_msg, pm_sel);               /* PM select*/
    (void) rx_msg;

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
          plp_aperta2_msg_send (phy, APERTA2_FUNC_CONFIG_POLSWAPS, APERTA2_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA2_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in PKG POLSWAP :0x%x\n", result));
        length = plp_aperta2_get_half_word (&rx_msg);
        for(index = 2; index < 2+length+2; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}
