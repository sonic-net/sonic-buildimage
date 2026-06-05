/*
*
* $Id: aperta_msg_tasks.c,  $
*
*  *
*  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
*  *
*  *
*
*/

#include <tier1/aperta_msg_tasks.h>

/*!
 *  Function to put a byte (8-bits) to uC memory (Little-endian).
 *
 *  Puts a byte to the given location and increases passed pointer by 1.
 *
 *  @param ptr    Pointer to pointer to uC memory location
 *  @param value  Value (16-bit) to copy
 */
void
plp_aperta_put_byte (uint8_t **ptr, uint8_t value)
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
plp_aperta_get_byte (uint8_t **ptr)
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
plp_aperta_put_half_word (uint8_t **ptr, uint16_t value)
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
plp_aperta_get_half_word (uint8_t **ptr)
{
    uint16_t value;

    value = **ptr;
    (*ptr)++;
    value += ((**ptr) << 8);
    (*ptr)++;

    return (value);
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
plp_aperta_put_word (uint8_t **ptr, uint32_t value)
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
 *  Function to send a message to chip-level firmware
 *
 *  This functions sends a message, waits for the firmware to process the message
 *  and finally reads bakc the response from firmware.
 *
 * @retval APERTA_OP_PROCESSING  Firmware is processing the message
 * @retval APERTA_OP_SUCCESS     Firmware successfully processed the message
 * @retval APERTA_OP_ERROR       Firmware encountered an error while processing the message
 */
int
plp_aperta_msg_send (const plp_aperta_phymod_phy_access_t *phy,  uint8_t function, uint8_t operation, uint8_t *tx_msg, uint8_t *rx_msg,
                 uint8_t *result)
{
    uint32_t msgout = 0, msgin = 0, length = 0, value = 0;
    uint32_t addr = MSG_IN_BUFFER_ADDR, retry_cnt = 500;

    length = plp_aperta_get_half_word (&tx_msg);
    PHYMOD_IF_ERR_RETURN(
          PHYMOD_BUS_WRITE(&phy->access, addr, length));
    addr++;
    length = (length + 1) / 2;
    while (length) {
        value = plp_aperta_get_half_word (&tx_msg);
          PHYMOD_IF_ERR_RETURN(
               PHYMOD_BUS_WRITE(&phy->access, addr, value));
        addr++;
        length--;
    }
    /* Trigger the message processing by writing to MSGIN register*/
    if ( APERTA_FW_FUN_SUPPORT_START_RESULT(function) && 
            (operation != APERTA_OP_START_RESULT)) {
        msgin = (function << 8) | (operation << 4) | (STS_SENT << 0);
        PHYMOD_IF_ERR_RETURN(
           PHYMOD_BUS_WRITE(&phy->access, BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr, msgin));
    }
    /* Adding another IF because START_RESULT and READ operations hold the same value,
     * handling in the same IF is not possible*/
    if (!APERTA_FW_FUN_SUPPORT_START_RESULT(function)) {
        msgin = (function << 8) | (operation << 4) | (STS_SENT << 0);
        PHYMOD_IF_ERR_RETURN(
           PHYMOD_BUS_WRITE(&phy->access, BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr, msgin));
    }

    /* Wait for the STS_PROCD response from firmware*/
    do {
        if (APERTA_FW_FUN_SUPPORT_START_RESULT(function) &&
             (operation == APERTA_OP_START_RESULT)) {
            msgin = (function << 8) | (operation << 4) | (STS_SENT << 0);
            PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_WRITE(&phy->access, BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr, msgin));
         }
         PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(&phy->access,  BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr, &msgout));
#ifdef WIN32
        PHYMOD_USLEEP(2);
#else
        /* PHYMOD_USLEEP(100); */
#endif
    } while (((msgout & 0xFF0F) != ((function << 8) | (STS_PROCD << 0))) && (--retry_cnt));

    if (retry_cnt == 0) {
         PHYMOD_DEBUG_ERROR(("Timeout in getting response from FW\n"));
         return PHYMOD_E_TIMEOUT;
    }
    /*PHYMOD_DEBUG_INFO(("MSGout:%x rt:%d\n", msgout, retry_cnt));*/
    *result = (msgout >> 4) & 0x000F;

    /* Skip reading the response when op=APERTA_OP_WRITE and result=APERTA_OP_SUCCESS*/
    if ((APERTA_OP_WRITE == operation) && (APERTA_OP_SUCCESS == *result)) {
        plp_aperta_put_half_word (&rx_msg, 0x0000);
        return (PHYMOD_E_NONE);
    }

    /* Adding else if because below code needs to be executed for all  
     * error cases and it needs to be executed for PASS case only for read and 
     * read EXT operations.
     * Since read and start result operations hold the same value, we are using 
     * function and operation together as a checkpoint */
    if (APERTA_OP_ERROR == *result ) {
        APERTA_OUTPUT_BUFFER_READ
    } else if (!APERTA_FW_FUN_SUPPORT_START_RESULT(function) && (operation == APERTA_OP_READ ||
            operation == APERTA_OP_READ_EXT)) {
        APERTA_OUTPUT_BUFFER_READ
    }

    return (PHYMOD_E_NONE);
}

/*!
 *  Function to configure package polarity based on package type and die number
 *
 *  @param chip_cfg  Chip Configuration
 *  @param die       Die Number
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int
plp_aperta_fw_dft_polarity_set (const plp_aperta_phymod_phy_access_t *phy)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0;
    unsigned int length = 0, index = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    /* use the global buffers for tx/rx message*/
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    plp_aperta_put_half_word (&tx_msg, 0x0000);   /* Length*/
    /* Send Message*/
     PHYMOD_IF_ERR_RETURN(
          plp_aperta_msg_send (phy, APERTA_FUNC_PKG_CFG, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result)  {
        PHYMOD_DEBUG_ERROR(("FW : Setting defult polarity FAiled:0x%x\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }

        return (PHYMOD_E_INTERNAL);
    }
    (void)rx_msg;

    return (PHYMOD_E_NONE);
}

/*!
 *  Function to configure a Phy (CONFIG_PHY.WRITE)
 *
 *  @param chip_cfg  Chip Configuration
 *  @param die       Die Number
 *  @param phy_cfg   Pointer to phy configuration structure
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int
plp_aperta_fw_config_phy_set(const plp_aperta_phymod_phy_access_t *phy,  aperta_config_phy_t *phy_cfg)
{
    uint8_t tx_buf[256], rx_buf[256];
     uint8_t *tx_msg, *rx_msg, result = 0;
     uint16_t length = 0, index = 0;

     PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
     PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    /* use the global buffers for tx/rx message*/
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    length = 0;

    /* Prepare the Message
       Length, PortNum, PortType, PortData*/
    plp_aperta_put_half_word (&tx_msg, 0x0000);           /* Length (real length inserted at the end*/
    plp_aperta_put_byte (&tx_msg, phy_cfg->MACsecOpt);    /* MACsecOpt*/
    plp_aperta_put_byte (&tx_msg, phy_cfg->IOOpt);        /* IOOpt*/
    length += 2;

    /* Length (at the first location)*/
    tx_msg = &tx_buf[0];
    plp_aperta_put_half_word (&tx_msg, length);

    /* Send Message*/
     PHYMOD_IF_ERR_RETURN(
         plp_aperta_msg_send (phy, APERTA_FUNC_CONFIG_PHY, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in Config PHY:0x%x\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }

        return (PHYMOD_E_INTERNAL);
    }
    (void) rx_msg;
    return (0);
}

/*!
 *  Function to read a configuration of phy (CONFIG_PHY.READ)
 *
 *  @param chip_cfg  Chip Configuration
 *  @param die       Die Number
 *  @param phy_cfg   Pointer to phy configuration structure
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int
plp_aperta_fw_config_phy_get(const plp_aperta_phymod_phy_access_t *phy,  aperta_config_phy_t *phy_cfg)
{
    uint8_t tx_buf[256], rx_buf[256];
     uint8_t *tx_msg, *rx_msg, result = 0;
     uint16_t length = 0, index = 0;

     PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
     PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    /* use the global buffers for tx/rx message*/
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, PortNum*/
    plp_aperta_put_half_word (&tx_msg, 0x0000);

    /* Send Message*/
     result = plp_aperta_msg_send (phy, APERTA_FUNC_CONFIG_PHY, APERTA_OP_READ, &tx_buf[0], &rx_buf[0], &result);
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in Config PHY get:0x%x\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }

        return (PHYMOD_E_INTERNAL);
    }
    /* Read the response*/
    length = plp_aperta_get_half_word (&rx_msg);
    if (length >= 2) {
        phy_cfg->MACsecOpt = plp_aperta_get_byte (&rx_msg);
        phy_cfg->IOOpt = plp_aperta_get_byte (&rx_msg);
        return (PHYMOD_E_NONE);
    } else {
        PHYMOD_DEBUG_ERROR(("FW : Error in Config getting config PHY:0x%x\n", plp_aperta_get_half_word (&rx_msg)));
        return (PHYMOD_E_INTERNAL);
    }
    return (PHYMOD_E_NONE);
}

/*!
 *  Function to configure a Port (CONFIG_PORT.WRITE)
 *
 *  @param chip_cfg  Chip Configuration
 *  @param die       Die Number
 *  @param port_cfg  Pointer to port configuration structure
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int
plp_aperta_fw_config_port_set (const plp_aperta_phymod_phy_access_t *phy,  aperta_config_port_t *port_cfg)
{
    uint8_t tx_buf[256], rx_buf[256];
     uint8_t *tx_msg, *rx_msg, result = 0;
     uint16_t length =0, index = 0;
     uint32_t rev_id = 0;

     PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
     PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    length = 0;

    /* Prepare the Message
       Length, PortNum, PortType, PortData*/
    plp_aperta_put_half_word (&tx_msg, 0x0000);
    plp_aperta_put_byte (&tx_msg, port_cfg->PortNum);      /* PortNum      */
    plp_aperta_put_byte (&tx_msg, port_cfg->PortType);     /* PortType      */
    plp_aperta_put_byte (&tx_msg, port_cfg->PortMode);     /* PortMode      */
    plp_aperta_put_byte (&tx_msg, port_cfg->PortSpeed);    /* PortSpeed      */
    plp_aperta_put_byte (&tx_msg, port_cfg->SPMPortID);    /* SPMPortID      */
    plp_aperta_put_byte (&tx_msg, port_cfg->LPMPortID);    /* LPMPortID      */
    plp_aperta_put_byte (&tx_msg, port_cfg->PortOptions);  /* PortOptions*/
    length += 7;
    if (port_cfg->PortOptions & 0x80)  {
        plp_aperta_put_half_word (&tx_msg, port_cfg->IngFixedLatency);
        plp_aperta_put_half_word (&tx_msg, port_cfg->EgrFixedLatency);
        length += 4;
    }
    if (port_cfg->PortMode) {
        /* Failover MUX Port*/
        plp_aperta_put_byte (&tx_msg, port_cfg->FOOptions);      /* FOOptions*/
        plp_aperta_put_byte (&tx_msg, port_cfg->FOPortNum);      /* FOPortNum*/
        plp_aperta_put_byte (&tx_msg, port_cfg->FOPortID);       /* FOPortID*/
        length += 3;
    }
    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));
    if (rev_id == APERTA_REV_B0) {
        if (port_cfg->PortOptions & 0x08) { /* B0 PTP Mode (PTP Option[3] = 1)*/
            /* EgrPTPFixedLatency*/
            plp_aperta_put_half_word (&tx_msg, port_cfg->EgrptpFixedLatency);
            length += 2;
        }
    } else { /* A0 */
        if (port_cfg->PortOptions & 0x08) { /* Return error*/
            PHYMOD_DEBUG_ERROR(("Unsupported Port option(0x%x) for rev :0x%x\n",port_cfg->PortOptions, rev_id));
            return PHYMOD_E_PARAM;
        }
    }

    /* Length (at the first location)*/
    tx_msg = &tx_buf[0];
    plp_aperta_put_half_word (&tx_msg, length);
     PHYMOD_IF_ERR_RETURN(
            plp_aperta_msg_send (phy, APERTA_FUNC_CONFIG_PORT, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    /* Send Message*/
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in Config setting config port:0x%x\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }

        return (PHYMOD_E_INTERNAL);
    }
    (void) rx_msg;
    return (PHYMOD_E_NONE);
}

/*!
 *  Function to read a configuration of port (CONFIG_PORT.READ)
 *
 *  @param chip_cfg  Chip Configuration
 *  @param die       Die Number
 *  @param port_cfg  Pointer to port configuration structure
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int
plp_aperta_fw_config_port_get (const plp_aperta_phymod_phy_access_t *phy,  aperta_config_port_t *port_cfg)
{
     uint8_t tx_buf[256], rx_buf[256];
     uint8_t *tx_msg, *rx_msg, result = 0;
     uint16_t length = 0, index = 0;
     uint32_t rev_id = 0;

     PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
     PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
     Length, PortNum*/
    plp_aperta_put_half_word (&tx_msg, 0x0001);
    plp_aperta_put_byte (&tx_msg, port_cfg->PortNum);
    PHYMOD_IF_ERR_RETURN(
          plp_aperta_msg_send(phy, APERTA_FUNC_CONFIG_PORT, APERTA_OP_READ, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in Config getting config port:0x%x\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
 
        return PHYMOD_E_INTERNAL;
    }
    length = plp_aperta_get_half_word (&rx_msg);
    if (length >= 7) {
        port_cfg->PortNum = plp_aperta_get_byte (&rx_msg);      /* PortNum       */
        port_cfg->PortType = plp_aperta_get_byte (&rx_msg);     /* PortType       */
        port_cfg->PortMode = plp_aperta_get_byte (&rx_msg);     /* PortMode       */
        port_cfg->PortSpeed = plp_aperta_get_byte (&rx_msg);    /* PortSpeed  */
        port_cfg->SPMPortID = plp_aperta_get_byte (&rx_msg);    /* SPMPortID  */
        port_cfg->LPMPortID = plp_aperta_get_byte (&rx_msg);    /* LPMPortID  */
        port_cfg->PortOptions = plp_aperta_get_byte (&rx_msg);  /* PortOptions*/
        if (port_cfg->PortOptions & 0x80) {
            port_cfg->IngFixedLatency = plp_aperta_get_half_word (&rx_msg);
            port_cfg->EgrFixedLatency = plp_aperta_get_half_word (&rx_msg);
        }
        if (port_cfg->PortMode) {
            /* Failover MUX Port*/
            port_cfg->FOOptions = plp_aperta_get_byte (&rx_msg);      /* FOOptions*/
            port_cfg->FOPortNum = plp_aperta_get_byte (&rx_msg);      /* FOPortNum*/
            port_cfg->FOPortID  = plp_aperta_get_byte (&rx_msg);      /* FOPortID */
        }
        PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));
        if (rev_id == APERTA_REV_B0) {
            if (port_cfg->PortOptions & 0x08) { /* B0 PTP Mode (PTP Option[3] = 1)*/
                /* EgrPTPFixedLatency*/
                port_cfg->EgrptpFixedLatency = plp_aperta_get_half_word (&rx_msg);
            }
        }
        return (PHYMOD_E_NONE);
    } else {
          PHYMOD_DEBUG_ERROR(("FW : Error in getting config port value:0x%x\n", length));
        return (PHYMOD_E_INTERNAL);
    }
    return (PHYMOD_E_NONE);
}

/*the process of enabling a Port
 *
 *  @param chip_cfg  Chip Configuration
 *  @param die       Die Number
 *  @param port_num  Port number
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int
plp_aperta_fw_port_op_start (const plp_aperta_phymod_phy_access_t *phy,  uint8_t port_num, int operation)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0, func = 0;
    uint32_t length=0, index = 0;


     PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
     PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

     if (operation == APERTA_FW_PORT_OP_ENABLE) {
          func = APERTA_FUNC_ENABLE_PORT;
     } else if (operation == APERTA_FW_PORT_OP_DISABLE) {
          func = APERTA_FUNC_DISABLE_PORT;
     } else if (operation == APERTA_FW_PORT_OP_FLUSH) {
          func = APERTA_FUNC_FLUSH_PORT;
     } else if (operation == APERTA_FW_PORT_OP_PAUSE) {
          func = APERTA_FUNC_PAUSE_PORT;
     } else if (operation == APERTA_FW_PORT_OP_RESUME) {
          func = APERTA_FUNC_RESUME_PORT;
     } else {
          PHYMOD_DEBUG_ERROR(("Invalid FW message operation\n"));
          return PHYMOD_E_PARAM;
     }
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /*Prepare the Message
      Length, PortNum*/
    if (operation == APERTA_FW_PORT_OP_FLUSH) {
        plp_aperta_put_half_word (&tx_msg, 0x0002);           /* Length*/
        plp_aperta_put_byte (&tx_msg, port_num);              /* PortNum*/
        plp_aperta_put_byte (&tx_msg, 1);                     /* Reset credits*/

    } else {
        plp_aperta_put_half_word (&tx_msg, 0x0001);           /* Length*/
        plp_aperta_put_byte (&tx_msg, port_num);              /* PortNum*/
    }

    (void) rx_msg;
     if ((operation != APERTA_FW_PORT_OP_PAUSE) &&
               (operation != APERTA_FW_PORT_OP_RESUME)) {
         PHYMOD_IF_ERR_RETURN(
              plp_aperta_msg_send (phy, func, APERTA_OP_START, &tx_buf[0], &rx_buf[0], &result));
        if (APERTA_OP_PROCESSING != result) {
            PHYMOD_DEBUG_ERROR(("FW : Error in port operation:%x start:0x%x\n", func, result));
            length = plp_aperta_get_half_word (&rx_msg);
            for(index = 2; index < 2+length; index++) {
                PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
            }
            return PHYMOD_E_INTERNAL;
        }
     } else {
          PHYMOD_IF_ERR_RETURN(
              plp_aperta_msg_send (phy, func, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
        if (APERTA_OP_SUCCESS != result) {
            PHYMOD_DEBUG_ERROR(("FW : Error in port operation:%x start:0x%x\n", func, result));
            length = plp_aperta_get_half_word (&rx_msg);
            for(index = 2; index < 2+length; index++) {
                PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
            }
            return PHYMOD_E_INTERNAL;
        }
     }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to wait until the process of enabling a Port is complete
 *
 *  @param chip_cfg  Chip Configuration
 *  @param die       Die Number
 *  @param port_num  Port number
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int
plp_aperta_fw_port_op_result (const plp_aperta_phymod_phy_access_t *phy,  uint8_t port_num, int operation)
{
     uint8_t tx_buf[256], rx_buf[256];
     uint8_t *tx_msg, *rx_msg;
     uint8_t result =0, func = 0;
     unsigned int retry_cnt = APERTA_FW_MSG_RETRY_CNT, length = 0, index = 0;

     PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
     PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));
     if (operation == APERTA_FW_PORT_OP_ENABLE) {
          func = APERTA_FUNC_ENABLE_PORT;
     } else if (operation == APERTA_FW_PORT_OP_DISABLE) {
          func = APERTA_FUNC_DISABLE_PORT;
     } else if (operation == APERTA_FW_PORT_OP_FLUSH) {
          func = APERTA_FUNC_FLUSH_PORT;
     } else {
          return PHYMOD_E_PARAM;
     }
     tx_msg = &tx_buf[0];
     rx_msg = &rx_buf[0];

    /* Prepare the Message
      Length, PortNum*/
    plp_aperta_put_half_word (&tx_msg, 0x0001);   /* Length     */
    plp_aperta_put_byte (&tx_msg, port_num);      /* PortNum*/
    do {
        PHYMOD_IF_ERR_RETURN(
               plp_aperta_msg_send (phy, func, APERTA_OP_START_RESULT, &tx_buf[0], &rx_buf[0], &result));
          PHYMOD_USLEEP(100);
    } while ((APERTA_OP_PROCESSING == result) && (retry_cnt --));
     if (retry_cnt == 0) {
          PHYMOD_DEBUG_ERROR(("Port Operation:%x is not getting completed:%d\n", func, result));
          return PHYMOD_E_INTERNAL;
     }
     (void) rx_msg;
    if (APERTA_OP_SUCCESS == result) {
        return PHYMOD_E_NONE;
    } else {
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
     }
     return PHYMOD_E_NONE;
}

/*!
 *  Function to Start enabling a Port and wait until the process is complete
 *
 *  @param chip_cfg  Chip Configuration
 *  @param die       Die Number
 *  @param port_num  Port number
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int
plp_aperta_fw_port_op (const plp_aperta_phymod_phy_access_t *phy,  uint8_t port_num, int operation)
{

     PHYMOD_IF_ERR_RETURN(plp_aperta_fw_port_op_start (phy, port_num, operation));
     if ((operation != APERTA_FW_PORT_OP_PAUSE) &&
               (operation != APERTA_FW_PORT_OP_RESUME)) {
        PHYMOD_IF_ERR_RETURN(plp_aperta_fw_port_op_result (phy, port_num, operation));
     }

    return PHYMOD_E_NONE;
}

/*!
 *  Function to configure a Clock Gen (CLOCK_GEN.WRITE)
 *
 *  @param chip_cfg  Chip Configuration
 *  @param die       Die Number
 *  @param clk_cfg  Pointer to clk configuration structure
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int plp_aperta_fw_configure_clock_gen (const plp_aperta_phymod_phy_access_t *phy,  aperta_clock_gen_t *clk_cfg)
{
     uint8_t tx_buf[256], rx_buf[256];
     uint8_t *tx_msg, *rx_msg, result = 0;
     uint16_t length = 0, index = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
     PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, PortNum, PortType, PortData*/
    plp_aperta_put_half_word (&tx_msg, 0x0000);    /* Length (real length inserted at the end*/
    plp_aperta_put_byte (&tx_msg, clk_cfg->RClkNum);       /* RClkNum*/
    plp_aperta_put_byte (&tx_msg, clk_cfg->ClkGenEn);      /* ClkGenE*/
    length += 2;
    if (clk_cfg->ClkGenEn)
    {
        plp_aperta_put_byte (&tx_msg, clk_cfg->ClkSide);   /* ClkSide*/
        plp_aperta_put_byte (&tx_msg, clk_cfg->ClkLane);   /* ClkLane*/
        plp_aperta_put_byte (&tx_msg, clk_cfg->Divider);   /* Divider*/
        plp_aperta_put_byte (&tx_msg, clk_cfg->SquelchMode); /* SquelchMode*/
        plp_aperta_put_byte (&tx_msg, clk_cfg->PortLanes);   /* SquelchMode*/
        length += 5;
    }
    /* Length (at the first location)*/
    tx_msg = &tx_buf[0];
    plp_aperta_put_half_word (&tx_msg, length);

    /* Send Message*/
     PHYMOD_IF_ERR_RETURN(
          plp_aperta_msg_send (phy, APERTA_FUNC_CLOCK_GEN, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("Clock gene Write failed :%d\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
 
        return PHYMOD_E_INTERNAL;
    }
    (void) rx_msg;
    return (PHYMOD_E_NONE);
}

int plp_aperta_nse_ptp_sopmem_read (const plp_aperta_phymod_phy_access_t *phy, aperta_ptp_sopmem_t *ptp_sopmem)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message Length, PortNum, Addr*/
    plp_aperta_put_half_word (&tx_msg, 0x0002);             
    plp_aperta_put_byte (&tx_msg, ptp_sopmem->port);     
    plp_aperta_put_byte (&tx_msg, ptp_sopmem->addr);        

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_msg_send (phy, APERTA_FUNC_PTP_SOPMEM, APERTA_OP_READ, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("PTP SOP MEM READ failed :%d\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }

    /* Read the response*/
    length = plp_aperta_get_half_word (&rx_msg);

    /* length should be exactly 28 bytes*/
    if (length == 28) {
        for (index=0; index<14; index++) {
            /* MemData in half-words (total of 28 bytes)*/
            ptp_sopmem->data [index] = plp_aperta_get_half_word (&rx_msg);
        }
        return PHYMOD_E_NONE;
    } else {
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_fw_clock_gen_read (const plp_aperta_phymod_phy_access_t *phy,  aperta_clock_gen_t *clk_cfg)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, PortNum*/
    plp_aperta_put_half_word (&tx_msg, 0x0001);             /* Length */
    plp_aperta_put_byte (&tx_msg, clk_cfg->RClkNum);        /* RClkNum*/

     PHYMOD_IF_ERR_RETURN(
          plp_aperta_msg_send (phy, APERTA_FUNC_CLOCK_GEN, APERTA_OP_READ, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("Clock gene read failed :%d\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    length = plp_aperta_get_half_word (&rx_msg);
    if (length >= 2) {
        clk_cfg->RClkNum  = plp_aperta_get_byte (&rx_msg);     /* RClkNum*/
        clk_cfg->ClkGenEn = plp_aperta_get_byte (&rx_msg);     /* ClkGenE*/
        if (clk_cfg->ClkGenEn) {
            clk_cfg->ClkSide = plp_aperta_get_byte (&rx_msg);  /* ClkSide*/
            clk_cfg->ClkLane = plp_aperta_get_byte (&rx_msg);  /* ClkLane*/
            clk_cfg->Divider = plp_aperta_get_byte (&rx_msg);  /* Divider*/
            clk_cfg->SquelchMode = plp_aperta_get_byte (&rx_msg);  /* SquelchMode*/
            clk_cfg->PortLanes = plp_aperta_get_byte (&rx_msg);    /* PortLanes      */
        } else {
            clk_cfg->ClkSide = 0x00;     /* ClkSide        */
            clk_cfg->ClkLane = 0x00;     /* ClkLane        */
            clk_cfg->Divider = 0x00;     /* Divider        */
            clk_cfg->SquelchMode = 0x00; /* SquelchMode*/
            clk_cfg->PortLanes = 0x00;   /* PortLanes*/
        }
        return (PHYMOD_E_NONE);
    } else {
          PHYMOD_DEBUG_ERROR(("Clock gen read result fail:%d\n", length));
        return (PHYMOD_E_INTERNAL);
    }
    return (PHYMOD_E_NONE);
}

int plp_aperta_fw_write_register (const plp_aperta_phymod_phy_access_t *phy, int block, void* data)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0, side = 0;
    uint8_t data_ptr[8];
    aperta_pm_reg_t *pm_reg;
    aperta_tsc_reg_t *tsc_reg;
    aperta_ind_reg_t *ind_reg;
    aperta_macsec_reg_t *macsec_reg;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));
    PHYMOD_MEMSET(data_ptr, 0, sizeof(uint8_t)*8);

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    side = (phy->port_loc == phymodPortLocSys) ? 0 : 1;
    if (block == APERTA_FUNC_PM_REGS) {
        pm_reg = (aperta_pm_reg_t*)data;
        /* Prepare the Message*/
        /* Length, SideSel, AccessType, DataLen, Mask, Cnt, Addr, WrData*/
        for (index =0; index < pm_reg->datalen; index++) {
            data_ptr[index] = (*pm_reg->data >> (index*8) & 0xFF);
        }
        plp_aperta_put_half_word (&tx_msg, (0x0007 + pm_reg->datalen));            /* Length*/     
        plp_aperta_put_byte (&tx_msg, side);                   /* SideSel*/
        plp_aperta_put_byte (&tx_msg, pm_reg->access_type);    /* AccessType*/
        plp_aperta_put_byte (&tx_msg, pm_reg->datalen);        /* DataLen*/
        plp_aperta_put_byte (&tx_msg, 0x00);                   /* Mask*/
        plp_aperta_put_byte (&tx_msg, 0x01);                   /* Cnt*/
        plp_aperta_put_half_word (&tx_msg, pm_reg->addr);      /* Addr*/
        index = 0;
        while (pm_reg->datalen--) {
            /* WrData*/
            plp_aperta_put_byte (&tx_msg, data_ptr[index]);
            index++;
        }
    } else if (block == APERTA_FUNC_TSC_REGS) {
        tsc_reg = (aperta_tsc_reg_t*)data;
        plp_aperta_put_half_word (&tx_msg, 9);                          /* Length*/
        plp_aperta_put_byte (&tx_msg, side);                            /* SideSel*/
        plp_aperta_put_byte (&tx_msg, 0x00);                            /* Mask*/
        plp_aperta_put_byte (&tx_msg, 0x1);                             /* Cnt*/
        plp_aperta_put_word (&tx_msg, tsc_reg->addr);                   /* Addr*/
        plp_aperta_put_half_word (&tx_msg, (*tsc_reg->data & 0xFFFF));  /* data*/
    } else if (block == APERTA_FUNC_CHIP_IND_REGS) {
        ind_reg = (aperta_ind_reg_t*)data;
        plp_aperta_put_half_word (&tx_msg, 6);                          /* Length*/
        plp_aperta_put_byte (&tx_msg, 0x00);                            /* Mask*/
        plp_aperta_put_byte (&tx_msg, 0x1);                             /* Cnt*/
        plp_aperta_put_half_word (&tx_msg, ind_reg->addr);              /* Addr*/
        plp_aperta_put_half_word (&tx_msg, (*ind_reg->data & 0xFFFF));  /* data*/
    } else if (block == APERTA_FUNC_MACSEC_REGS) {
        macsec_reg = (aperta_macsec_reg_t*)data;
        plp_aperta_put_half_word (&tx_msg, 11);                         /* Length*/
        plp_aperta_put_byte (&tx_msg, macsec_reg->direction);           /* Dir*/
        plp_aperta_put_byte (&tx_msg, macsec_reg->block);               /* block*/
        plp_aperta_put_byte (&tx_msg, 0x00);                            /* Mask*/
        plp_aperta_put_byte (&tx_msg, 0x1);                             /* Cnt*/
        plp_aperta_put_half_word (&tx_msg, macsec_reg->addr);           /* Addr - 16 bits*/
        plp_aperta_put_byte (&tx_msg,  (macsec_reg->addr >> 16));               /* Addr - 8 bits*/
        plp_aperta_put_word (&tx_msg, *macsec_reg->data);               /* data*/
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid Block in write\n"));
        return PHYMOD_E_PARAM;
    }
    
    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
          plp_aperta_msg_send (phy, block, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("PM write failed :%d\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }

    (void) rx_msg;

    return (0);
} 

int plp_aperta_fw_read_register (const plp_aperta_phymod_phy_access_t *phy, int block, void *data)
{
    uint8_t tx_buf[256], rx_buf[256], data_ptr[8];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0, side = 0;
    aperta_pm_reg_t *pm_reg = NULL;
    aperta_tsc_reg_t *tsc_reg = NULL;
    aperta_ind_reg_t *ind_reg = NULL;
    aperta_macsec_reg_t *macsec_reg = NULL;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    /* Prepare the Message*/
    /* Length, SideSel, AccessType, DataLen, Cnt, Addr*/
    side = (phy->port_loc == phymodPortLocSys) ? 0 : 1;
    if (block == APERTA_FUNC_PM_REGS) {
        pm_reg = (aperta_pm_reg_t*)data;
        COMPILER_64_ZERO(*pm_reg->data);
        plp_aperta_put_half_word (&tx_msg, 0x6);                 /* Length*/
        plp_aperta_put_byte (&tx_msg, side);                     /* SideSel*/
        plp_aperta_put_byte (&tx_msg, pm_reg->access_type);      /* AccessType*/
        plp_aperta_put_byte (&tx_msg, pm_reg->datalen);          /* DataLen*/
        plp_aperta_put_byte (&tx_msg, 0x01);                     /* Cnt*/
        plp_aperta_put_half_word (&tx_msg, pm_reg->addr);        /* Addr*/
    } else if (block == APERTA_FUNC_TSC_REGS) {
        tsc_reg = (aperta_tsc_reg_t*)data;
        plp_aperta_put_half_word (&tx_msg, 6);                   /* Length*/
        plp_aperta_put_byte (&tx_msg, side);                     /* SideSel*/
        plp_aperta_put_byte (&tx_msg, 0x1);                      /* Cnt*/
        plp_aperta_put_word (&tx_msg, tsc_reg->addr);            /* Addr*/
    } else if (block == APERTA_FUNC_CHIP_IND_REGS) {
        ind_reg = (aperta_ind_reg_t*)data;
        plp_aperta_put_half_word (&tx_msg, 3);                   /* Length*/
        plp_aperta_put_byte (&tx_msg, 0x1);                      /* Cnt*/
        plp_aperta_put_half_word (&tx_msg, ind_reg->addr);       /* Addr*/
    } else if (block == APERTA_FUNC_MACSEC_REGS) {
        macsec_reg = (aperta_macsec_reg_t*)data;
        plp_aperta_put_half_word (&tx_msg, 6);                   /* Length*/
        plp_aperta_put_byte (&tx_msg, macsec_reg->direction);    /* dir*/
        plp_aperta_put_byte (&tx_msg, macsec_reg->block);        /* block*/
        plp_aperta_put_byte (&tx_msg, 0x1);                      /* Cnt*/
        plp_aperta_put_half_word (&tx_msg, macsec_reg->addr);    /* Addr*/
        plp_aperta_put_byte (&tx_msg, (macsec_reg->addr >> 16)); /* Addr*/
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid Block\n"));
        return PHYMOD_E_PARAM;
    }

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
          plp_aperta_msg_send (phy, block, APERTA_OP_READ, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW READ failed :%d\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    /* Read the response*/
    if (block == APERTA_FUNC_PM_REGS) {
        index = 0;
        if (pm_reg) {
            COMPILER_64_ZERO(*pm_reg->data);
            if (pm_reg->datalen == plp_aperta_get_half_word (&rx_msg)) {
                while (pm_reg->datalen--) {
                    data_ptr[index] = plp_aperta_get_byte (&rx_msg);
                    *pm_reg->data |= ((uint64_t)data_ptr[index] << (index*8));
                    index++;
                }
                if ((*pm_reg->data  & 0xBEEFDEAD) == 0xBEEFDEAD) {
                    PHYMOD_DEBUG_ERROR(("PM READ failed for addr:%x accesstyp:%x phy:%x \n", pm_reg->addr, pm_reg->access_type, phy->access.addr));
                    return PHYMOD_E_INTERNAL;
                }
                return PHYMOD_E_NONE;
            } else {
                PHYMOD_DEBUG_ERROR(("PM READ failed with data len(%d) not matching\n", pm_reg->datalen));
                return PHYMOD_E_INTERNAL;
            }
        }
    } else {
        length = plp_aperta_get_half_word (&rx_msg);
        if (length == 2) {
            if (block == APERTA_FUNC_TSC_REGS) {
                if (tsc_reg) {
                    *tsc_reg->data = plp_aperta_get_half_word (&rx_msg);
                }
            } else {
                if (ind_reg) {
                    *ind_reg->data = plp_aperta_get_half_word (&rx_msg);
                }
            }
            return PHYMOD_E_NONE;
        } else if (length == 4) {
            if (block == APERTA_FUNC_MACSEC_REGS) {
                if (macsec_reg) {
                    *macsec_reg->data = plp_aperta_get_half_word (&rx_msg);
                    *macsec_reg->data |= (plp_aperta_get_half_word (&rx_msg) << 16);
                }
            }
            return PHYMOD_E_NONE;
        } else {
            PHYMOD_DEBUG_ERROR(("READ failed with data lesser than expected::%d\n", length));
            return PHYMOD_E_INTERNAL;
        }
    }
    return PHYMOD_E_NONE;
}

static uint8_t _plp_aperta_log2n(uint32_t n) 
{
    return ((n > 1) ? (1 + _plp_aperta_log2n(n / 2)) : 0);
}

int plp_aperta_fw_reg_access(const plp_aperta_phymod_phy_access_t* phy, uint16_t port_sel, uint16_t lane_sel, 
                         uint32_t reg_addr, uint64_t* data, int write_en, int pcs_pmd_addr) 
{
    uint16_t gen_access = 0, fw_lane = 0;
    APERTA_SLAVE_TYPE_T slave;
    int lmi_data_size = 0;
    aperta_pm_reg_t pm_data;
    aperta_tsc_reg_t tsc_data;
    aperta_ind_reg_t ind_data;
    aperta_macsec_reg_t macsec_data;
    uint32_t reg_data = 0, block = 0;
    void *reg_access = NULL;
    slave = plp_aperta_get_lmi_slave(reg_addr);

    /* Get data size for LMI Transaction*/
    lmi_data_size = plp_aperta_get_lmi_slave_data_length(reg_addr);
    PHYMOD_MEMSET(&pm_data, 0, sizeof(aperta_pm_reg_t));
    
    /* For MAC*/
    if (slave == APERTA_PM_MAC) {
        gen_access = (reg_addr & (1 << 17)) ? 1 : 0;
        fw_lane = _plp_aperta_log2n((lane_sel & ~(lane_sel-1)));
        if (gen_access) {
            if (phy->access.lane_mask & 0xF) {
                pm_data.access_type = 0x10;
            } else {
                pm_data.access_type = 0x20;
            }
        } else {
            if (phy->access.lane_mask & 0xF) {
                pm_data.access_type = 0x90 | fw_lane;
            } else {
                fw_lane -= 4;
                if (fw_lane >= 4) {
                    PHYMOD_DEBUG_ERROR(("plp_aperta_fw_reg_access:: Unexpected lane sel :%x\n", fw_lane));
                    return PHYMOD_E_PARAM;
                }
                pm_data.access_type = 0xA0 | fw_lane;
            }
        }
    } else if (slave == APERTA_PM_PORT) {
        gen_access = (reg_addr & (1 << 17)) ? 1 : 0;
        fw_lane = _plp_aperta_log2n((lane_sel & ~(lane_sel-1)));
        if (gen_access) {
            pm_data.access_type = 0x0;
        } else {
            pm_data.access_type = 0x80 | fw_lane;
        }
    } else if (slave == APERTA_PM_TSC) {
        tsc_data.addr = pcs_pmd_addr;
        reg_data = COMPILER_64_LO(*data);
        tsc_data.data = &reg_data;
        block = APERTA_FUNC_TSC_REGS;
        reg_access = &tsc_data;
    } else if (slave == APERTA_CHIP_RDB) {
        ind_data.addr = reg_addr & 0xFFFF;
        reg_data = COMPILER_64_LO(*data);
        ind_data.data = &reg_data;
        block = APERTA_FUNC_CHIP_IND_REGS;
        reg_access = &ind_data;
    } else if (slave == APERTA_EIP163_INGRESS || slave == APERTA_EIP164_INGRESS || slave == APERTA_EIP218_INGRESS 
              || slave == APERTA_EIP163_EGRESS || slave == APERTA_EIP164_EGRESS ||
                 slave == APERTA_EIP218_EGRESS) {
        if (slave == APERTA_EIP163_INGRESS || slave == APERTA_EIP164_INGRESS || 
                slave == APERTA_EIP218_INGRESS) {
            macsec_data.direction = 0;
        } else {
            macsec_data.direction = 1;
        }
        if (slave == APERTA_EIP163_INGRESS || slave == APERTA_EIP163_EGRESS) { 
            macsec_data.block = 0;
        } else if (slave == APERTA_EIP164_INGRESS || slave == APERTA_EIP164_EGRESS) { 
            macsec_data.block = 1;
        } else {
            macsec_data.block = 2;
        }
        macsec_data.addr = reg_addr & 0xFFFFFF;
        reg_data = COMPILER_64_LO(*data);
        macsec_data.data = &reg_data;
        block = APERTA_FUNC_MACSEC_REGS;
        reg_access = &macsec_data;
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid Slave \n"));
        return PHYMOD_E_PARAM;
    }
    if ((slave == APERTA_PM_PORT) || (slave == APERTA_PM_MAC)) {
        pm_data.datalen = (lmi_data_size/8);
        pm_data.addr = reg_addr & 0xFFFF;
        pm_data.data = data;
        block = APERTA_FUNC_PM_REGS;
        reg_access = &pm_data;
    }
    if (write_en) {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_fw_write_register (phy, block, reg_access));
    } else {
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_fw_read_register(phy, block, reg_access));
        if (block == APERTA_FUNC_TSC_REGS || block == APERTA_FUNC_CHIP_IND_REGS ||
                block == APERTA_FUNC_MACSEC_REGS) {
            COMPILER_64_SET(*data, 0, reg_data);
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_fw_mem_access(const plp_aperta_phymod_phy_access_t* phy, APERTA_PM_MEM_ACCESS_T* mem_access)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t temp = 0, index = 0, residual = 0, fw_mem_type = 0; 

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    APERTA_PHYMOD_FW_MEMTYPE(mem_access->pm_mem_type, fw_mem_type);
    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];
    if (mem_access->pm_mem_rw == 0) {
        PHYMOD_MEMSET(mem_access->pm_mem_data, 0 , sizeof(uint32_t) * 16);
    }

    /* Prepare the Message*/
    /* Length, SideSel, AccessType, DataLen, Cnt, Addr*/
    if (mem_access->pm_mem_rw == 1) {
        plp_aperta_put_half_word (&tx_msg, (0x7+(mem_access->pm_mem_len *2)));               /* Length*/
    } else {
        plp_aperta_put_half_word (&tx_msg, 0x6);                                             /* Length*/
    }
    plp_aperta_put_byte (&tx_msg, (mem_access->pm_mem_sel == APERTA_LINE_SIDE_PM) ? 1 : 0);  /* SideSel*/
    plp_aperta_put_byte (&tx_msg, (0xF0 | fw_mem_type));                                     /* AccessType*/
    plp_aperta_put_byte (&tx_msg, (mem_access->pm_mem_len *2));                              /* DataLen*/
    if (mem_access->pm_mem_rw == 1) {
        plp_aperta_put_byte (&tx_msg, 0x0);                                                  /* Mask */
    }
    plp_aperta_put_byte (&tx_msg, 0x1);                                                      /* Cnt*/
    plp_aperta_put_half_word (&tx_msg, mem_access->pm_mem_addr);                             /* Addr*/
    if (mem_access->pm_mem_rw == 1) {
        for (index = 0; index < (mem_access->pm_mem_len); index++) {
            plp_aperta_put_half_word (&tx_msg, mem_access->pm_mem_data[index]);              /* data*/
            plp_aperta_put_half_word (&tx_msg, (mem_access->pm_mem_data[index] >> 16));      /* data*/
        }
    }
    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
      plp_aperta_msg_send (phy, APERTA_FUNC_PM_REGS, 
          (mem_access->pm_mem_rw == 0) ? APERTA_OP_READ : APERTA_OP_WRITE, 
          &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW READ failed :%d\n", result));
        temp = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+temp; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    if (mem_access->pm_mem_rw == 1) {
        return PHYMOD_E_NONE;
    }
    temp = plp_aperta_get_half_word (&rx_msg);
    if (temp == (mem_access->pm_mem_len *2)) {
        for (index = 0; (index < temp/4); index++) {
            mem_access->pm_mem_data[index] = rx_msg[index*4] | (rx_msg[(index*4)+1] << 8) | (rx_msg[(index*4)+2] << 16) | (rx_msg[(index*4)+3] << 24);
        }
        mem_access->pm_mem_data[index] = 0;
        residual = temp %4;
        temp = 0;
        while (residual) {
           mem_access->pm_mem_data[index] |=  ((rx_msg[(index*4)+temp]) << (temp *8));
           residual --;
           temp++;
        }
    } else {
        PHYMOD_DEBUG_ERROR(("PM READ failed with data len(%d) not matching\n", temp));
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_fw_switch_mux (const plp_aperta_phymod_phy_access_t* phy, int port_num)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0, retry_cnt = APERTA_FW_MSG_RETRY_CNT;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, PortNum*/
    plp_aperta_put_half_word (&tx_msg, 0x1);             /* Length */
    plp_aperta_put_byte (&tx_msg, port_num);        /* portNum*/
    (void) rx_msg;

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
          plp_aperta_msg_send (phy, APERTA_FUNC_SWITCH_MUX, APERTA_OP_START, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_PROCESSING != result) {
        PHYMOD_DEBUG_ERROR(("FW : Error in Switch mux start:0x%x\n", result));
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length+2; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    do {
        PHYMOD_IF_ERR_RETURN(
               plp_aperta_msg_send (phy, APERTA_FUNC_SWITCH_MUX, APERTA_OP_START_RESULT, &tx_buf[0], &rx_buf[0], &result));
        /*PHYMOD_USLEEP(100);*/
    } while ((APERTA_OP_PROCESSING == result) && (retry_cnt --));
    if (retry_cnt == 0) {
         PHYMOD_DEBUG_ERROR(("Switching Mux:%d is not getting completed:%d\n", port_num, result));
         return PHYMOD_E_INTERNAL;
    }
    (void) rx_msg;
    if (APERTA_OP_SUCCESS == result) {
        return PHYMOD_E_NONE;
    } else {
        length = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+length+2; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }
    return PHYMOD_E_NONE;
} 

int plp_aperta_fw_config_ptp_write (const plp_aperta_phymod_phy_access_t* phy, aperta_ptp_config_t *ptp_cfg)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0, temp =0;
    uint32_t rev_id = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    /* Prepare the Message
       Length, PortNum, PortType, PortData*/
    plp_aperta_put_half_word (&tx_msg, 0x0000);    /* Length (real length inserted at the end*/
    plp_aperta_put_byte(&tx_msg, ptp_cfg->PortNum);      /* PortNum*/
    plp_aperta_put_byte(&tx_msg, ptp_cfg->PortOptions);  /* PortOptions*/
    length += 2;

    /* FixedLatency*/
    if (ptp_cfg->PortOptions & 0x80) { /* Fixed-Latency Enabled*/
        plp_aperta_put_half_word (&tx_msg, ptp_cfg->IngFixedLatency);
        plp_aperta_put_half_word (&tx_msg, ptp_cfg->EgrFixedLatency);
        length += 4;
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));
    if (rev_id == APERTA_REV_B0) {
        if (ptp_cfg->PortOptions & 0x08) { /* B0 PTP Mode (PTP Option[3] = 1)*/
            /* EgrPTPFixedLatency*/
            plp_aperta_put_half_word (&tx_msg, ptp_cfg->EgrPTPFixedLatency);
            length += 2;
        }
    } else { /* A0 */
        if (ptp_cfg->PortOptions & 0x08) { /* Return error*/
            PHYMOD_DEBUG_ERROR(("Unsupported Port option(0x%x) for rev :0x%x\n",ptp_cfg->PortOptions, rev_id));
            return PHYMOD_E_PARAM;
        }
    }

    /* Length (at the first location)*/
    tx_msg = &tx_buf[0];
    plp_aperta_put_half_word (&tx_msg, length);

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
      plp_aperta_msg_send (phy, APERTA_FUNC_PTP_CONFIG, APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW Write failed :%d\n", result));
        temp = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+temp; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }

    /* Read the response (nothing to read)*/
    (void) rx_msg;

    return PHYMOD_E_NONE;
} 

int plp_aperta_fw_config_ptp_read (const plp_aperta_phymod_phy_access_t* phy, aperta_ptp_config_t *ptp_cfg)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0, temp=0;
    uint32_t rev_id = 0;

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    PHYMOD_IF_ERR_RETURN(plp_aperta_get_chip_rev(&phy->access, &rev_id));
    /* Prepare the Message
       Length, PortNum, PortType, PortData*/
    plp_aperta_put_half_word (&tx_msg, 0x0001);    /* Length */
    plp_aperta_put_byte(&tx_msg, ptp_cfg->PortNum);      /* PortNum*/

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(
      plp_aperta_msg_send (phy, APERTA_FUNC_PTP_CONFIG, APERTA_OP_READ, &tx_buf[0], &rx_buf[0], &result));
    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW Write failed :%d\n", result));
        temp = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+temp; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }

    /* Read the response*/
    length = plp_aperta_get_half_word (&rx_msg);

    /* length should atleast be 7 bytes or more*/
    if (length >= 2) {
        ptp_cfg->PortNum = plp_aperta_get_byte (&rx_msg);      /* PortNum*/
        ptp_cfg->PortOptions = plp_aperta_get_byte (&rx_msg);  /* PortOptions*/

        if (ptp_cfg->PortOptions & 0x80) {/* Fixed-Latency Enabled*/
            /* FixedLatency*/
            ptp_cfg->IngFixedLatency = plp_aperta_get_half_word (&rx_msg);
            ptp_cfg->EgrFixedLatency = plp_aperta_get_half_word (&rx_msg);
        }
        if (rev_id == APERTA_REV_B0) {
            if (ptp_cfg->PortOptions & 0x08) { /* B0 PTP Mode (PTP Option[3] = 1)*/
                /* EgrPTPFixedLatency*/
                ptp_cfg->EgrPTPFixedLatency = plp_aperta_get_half_word (&rx_msg);
            }
        }

        return (PHYMOD_E_NONE);
    } else {
        PHYMOD_DEBUG_ERROR(("PTP config READ failed with data len(%d) not matching\n", length));
        return (PHYMOD_E_INTERNAL);
    }
    return (PHYMOD_E_NONE);
}

/*!
 *  Function to write PTP TOD48/80 
 *
 *  @param chip_cfg   Chip Configuration
 *  @param die        Die Number
 *  @param ptp_tod48  Pointer to PTP_TOD48 structure
 *
 *  @retval 0 success
 *  @retval 1 failure
 */
int plp_aperta_fw_tod_config(const plp_aperta_phymod_phy_access_t* phy, int tod_type, aperta_ptp_tod_t *ptp_tod)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, *rx_msg, result = 0;
    uint16_t length = 0, index = 0, temp=0;
    uint16_t cnt = 0, max_cnt = ((tod_type == APERTA_PTP_TOD_TYPE_48BIT) ? 6 : 10);

    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));

    tx_msg = &tx_buf[0];
    rx_msg = &rx_buf[0];

    length = 0;

    /* Prepare the Message*/
    /* Length, PortNum, TOD_TS, TOD_DPLL*/
    plp_aperta_put_half_word (&tx_msg, 0x0000);    /* Length (real length inserted at the end)*/

    plp_aperta_put_byte (&tx_msg, ptp_tod->PortNum);      /* PortNum*/
    for (cnt=0; cnt < max_cnt; cnt++) {
        plp_aperta_put_byte (&tx_msg, ptp_tod->TOD_TS[cnt]);     /*TOD_TS bytes*/
    }
    for (cnt=0; cnt < max_cnt; cnt++) {
        plp_aperta_put_byte (&tx_msg, ptp_tod->TOD_DPLL[cnt]);   /* TOD_DPLL bytes*/
    }
    if (tod_type == APERTA_PTP_TOD_TYPE_48BIT) {
        length += 13;
    } else {
        length += 21;
    }

    /* Length (at the first location)*/
    tx_msg = &tx_buf[0];
    plp_aperta_put_half_word (&tx_msg, length);

    /* Send Message*/
    PHYMOD_IF_ERR_RETURN(plp_aperta_msg_send (phy, 
          ((tod_type == APERTA_PTP_TOD_TYPE_48BIT) ? APERTA_FUNC_PTP_TOD48 : APERTA_FUNC_PTP_TOD80),
           APERTA_OP_WRITE, &tx_buf[0], &rx_buf[0], &result));

    if (APERTA_OP_SUCCESS != result) {
        PHYMOD_DEBUG_ERROR(("FW Write failed :%d\n", result));
        temp = plp_aperta_get_half_word (&rx_msg);
        for(index = 2; index < 2+temp; index++) {
            PHYMOD_DEBUG_ERROR(("Output buffer :%d\n", rx_buf[index]));
        }
        return PHYMOD_E_INTERNAL;
    }

    /* Read the response (nothing to read)*/
    (void) rx_msg;

    return (0);
}

