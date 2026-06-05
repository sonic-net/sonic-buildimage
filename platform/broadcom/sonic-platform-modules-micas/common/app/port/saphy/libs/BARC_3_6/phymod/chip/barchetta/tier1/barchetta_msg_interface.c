/*
 *
 * $Id: barchetta_msg_interface.c,  $
 *
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

/*
 * Includes
 */

#include "barchetta_msg_interface.h"

/*****************************************************************************
 \brief   Function to put a byte (8-bits) to uC memory (Little-endian).

 \details Puts a byte to the given location and increases passed pointer by 1.

 \param   ptr     [In] Pointer to pointer to uC memory location
 \param   value   [In] Value (8-bit) to copy

 \return  void
 *******************************************************************************/
static void __plp_barchetta_put_byte(uint8_t **ptr, uint8_t value) {
    *(*ptr) = value;
    (*ptr)++;
}

/*****************************************************************************
 \brief   Function to get a byte (8-bits) from uC memory (Little-endian)

 \details Get a byte from the given location and increase passed pointer by 1.

 \param   ptr     [In] Pointer to pointer to uC memory location

 \return  Value   (8-bit) read from the memory
 *******************************************************************************/
static uint8_t __plp_barchetta_get_byte(uint8_t **ptr) {
    uint8_t value;
    value = **ptr;
    (*ptr)++;
    return (value);
}

/*****************************************************************************
 \brief   Function to put a half word (16-bits) to uC memory (Little-endian).

 \details Puts a byte to the given location and increases passed pointer by 2.

 \param   ptr     [In] Pointer to pointer to uC memory location
 \param   value   [In] Value (16-bit) to copy

 \return  void
 *******************************************************************************/
static void __plp_barchetta_put_half_word(uint8_t **ptr, uint16_t value) {
    **ptr = (value & 0xFF);
    (*ptr)++;
    **ptr = (value >> 8);
    (*ptr)++;
}

/*****************************************************************************
 \brief   Function to get a half word (16-bits) from uC memory (Little-endian)

 \details Get a half-word at the given location and increase passed pointer by 2.

 \param   ptr     [In] Pointer to pointer to uC memory location

 \return  Value   (16-bit) read from the memory
 *******************************************************************************/
static uint16_t __plp_barchetta_get_half_word(uint8_t **ptr) {
    uint16_t value;
    value = **ptr;
    (*ptr)++;
    value += ((**ptr) << 8);
    (*ptr)++;
    return (value);
}

/*******************************************************************************
 PURPOSE:  Function to send a message to chip-level firmware

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_send(const plp_barchetta_phymod_access_t *pa, uint8_t function,
        uint8_t operation, uint8_t *tx_msg, uint8_t *rx_msg, uint32_t rx_msg_size) {
    uint32_t msgout;
    uint16_t msgin;
    uint32_t length;
    uint32_t value;
    uint32_t addr;
    uint8_t result;
    uint16_t retry_count = BARCHETTA_MSG_IF_RETRY_COUNT;

    /* Write the Message into the MSG_IN_BUFFER */
    addr = m_BARCHETTA_GET_MSG_IN_BUF_BASE_ADDR();

    length = __plp_barchetta_get_half_word(&tx_msg);
    PHYMOD_IF_ERR_RETURN(
        PHYMOD_BUS_WRITE(pa, addr, (uint32_t )length));
    addr++;
    length = (length + 1) / 2;
    while (length) {
        value = __plp_barchetta_get_half_word(&tx_msg);
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_WRITE(pa, addr, (uint32_t )value));
        addr++;
        length--;
    }

    /*############################################################
     MSGIN/MSGOUT Bit Description:
     BIT[15:8]:Function  (7:0)
     BIT[ 7:4]:Operation (3:0)
     BIT[ 3:0]:Msg_status(3:0)[1:MSG_SENT,2:MSG_RECD,3:MSG_PROCD]
     ##############################################################*/

    /* Trigger the message processing by writing to MSGIN register */
    msgin = (function << 8) | (operation << 4)| (BARCHETTA_MSG_STATUS_SENT << 0);
    PHYMOD_IF_ERR_RETURN(
        PHYMOD_BUS_WRITE(pa, m_BARCHETTA_GET_MSG_IN_REG_ADDR(),(uint32_t )msgin));

    /* Wait for the MSG_STATUS_PROCD response from firmware */
    do {
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(pa, m_BARCHETTA_GET_MSG_OUT_REG_ADDR(), &msgout));
#ifdef VIRTUAL_SIM_VAL
        PHYMOD_USLEEP(5000);
#else
        PHYMOD_USLEEP(5);
#endif
    } while (((msgout & 0xFF0F)!= ((function << 8) | (BARCHETTA_MSG_STATUS_PROCD << 0))) &&
            (--retry_count));

    result = (msgout >> 4) & 0x000F; /* Returned operation */

    /* Skip reading the response when op=OP_WRITE and result=OP_SUCCESS */
    if ((operation == BARCHETTA_OP_WRITE) && (result == BARCHETTA_OP_SUCCESS)) {
        __plp_barchetta_put_half_word(&rx_msg, 0x0000);
        return (result);
    }

    /* Read the Response from the MSG_OUT_BUFFER */
    addr = m_BARCHETTA_GET_MSG_OUT_BUF_BASE_ADDR();

    /* Here we extracted the length of bytes to be read */
    PHYMOD_IF_ERR_RETURN(
        PHYMOD_BUS_READ(pa, addr, &length));

    if((length & 0xFFFF) > rx_msg_size){
        PHYMOD_DEBUG_ERROR(("ERROR: rx msg_length is greater than expected length of %d Bytes\n",rx_msg_size));
        return BARCHETTA_OP_ERROR;
    }

    __plp_barchetta_put_half_word(&rx_msg, (length & 0xFFFF));
    addr++;
    length = (length + 1) / 2;
    while (length) {
        PHYMOD_IF_ERR_RETURN(
            PHYMOD_BUS_READ(pa, addr, &value));
        __plp_barchetta_put_half_word(&rx_msg, (value & 0xFFFF));
        addr++;
        length--;
    }
    return (result);
}

/*******************************************************************************
 PURPOSE:  Function to configure logical lane numbers based on user input

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_lanes_wr(const plp_barchetta_phymod_access_t *pa,
        barchetta_config_lanes_t *lanes_cfg) {
    uint8_t barchetta_tx_buf[64] = { 0 }; /* 64 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4]  = { 0 }; /* 4 bytes are sufficient for response operation         */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint16_t length;
    uint8_t config_type;
    uint8_t msg_send_status = 0;
    uint8_t lane;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];
    length = 0;

    /* Prepare the Message            */
    /* Length, ConfigType, ConfigData */
    __plp_barchetta_put_half_word(&tx_msg, 0x0000); /* Length (real length inserted at the end */
    __plp_barchetta_put_byte(&tx_msg, lanes_cfg->config_type); /* ConfigType */
    length += 1;

    /* Retrieve the config_type */
    config_type = lanes_cfg->config_type;

    if (config_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        /* Retrieve Duplex lane configuration */
        barchetta_duplex_lanes_t *p_duplex_lanes = &(lanes_cfg->lanes.duplex);

        /* NumSysLanes, NumLineLanes */
        __plp_barchetta_put_byte(&tx_msg, p_duplex_lanes->num_sys_lanes);
        __plp_barchetta_put_byte(&tx_msg, p_duplex_lanes->num_line_lanes);
        length += 2;

        /* SRLaneList */
        for (lane = 0; lane < p_duplex_lanes->num_sys_lanes; lane++) {
            __plp_barchetta_put_byte(&tx_msg, p_duplex_lanes->sr_lane_list[lane]);
        }
        length += p_duplex_lanes->num_sys_lanes;

        /* STLaneList */
        for (lane = 0; lane < p_duplex_lanes->num_sys_lanes; lane++) {
            __plp_barchetta_put_byte(&tx_msg, p_duplex_lanes->st_lane_list[lane]);
        }
        length += p_duplex_lanes->num_sys_lanes;

        /* LRLaneList */
        for (lane = 0; lane < p_duplex_lanes->num_line_lanes; lane++) {
            __plp_barchetta_put_byte(&tx_msg, p_duplex_lanes->lr_lane_list[lane]);
        }
        length += p_duplex_lanes->num_line_lanes;

        /* LTLaneList */
        for (lane = 0; lane < p_duplex_lanes->num_line_lanes; lane++) {
            __plp_barchetta_put_byte(&tx_msg, p_duplex_lanes->lt_lane_list[lane]);
        }
        length += p_duplex_lanes->num_line_lanes;
    } else if (config_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        
    } else if (config_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        
    }

    /* Length (at the first location) */
    tx_msg = &barchetta_tx_buf[0];
    __plp_barchetta_put_half_word(&tx_msg, length);

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send( pa, BARCHETTA_FUNC_CONFIG_LANES, BARCHETTA_OP_WRITE, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0])) ;
    if (msg_send_status != BARCHETTA_OP_SUCCESS) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_config_lanes_wr failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
    /* Read the response (nothing to read) */
    (void) rx_msg;

    return (BARCHETTA_MSG_IF_RET_SUCCESS);
}

/*******************************************************************************
 PURPOSE:  Function to read the lane configuration

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_lanes_rd(const plp_barchetta_phymod_access_t *pa,
        barchetta_config_lanes_t *lanes_cfg) {
    uint8_t barchetta_tx_buf[4]  = { 0 }; /* 4 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[64] = { 0 }; /* 64 bytes are sufficient for response operation       */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t config_type;
    uint8_t lane;
    uint8_t msg_send_status = 0;
    uint16_t length;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length */
    __plp_barchetta_put_half_word(&tx_msg, 0x0000); /* Length */

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_CONFIG_LANES, BARCHETTA_OP_READ, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0])) ;
    if (msg_send_status != BARCHETTA_OP_SUCCESS) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_config_lanes_rd failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }

    /* Read the response */
    length = __plp_barchetta_get_half_word(&rx_msg);

    /* length should atleast be 35 bytes or more */
    
    if (length >= 35) {
        lanes_cfg->config_type = __plp_barchetta_get_byte(&rx_msg); /* ConfigType */

        /* Retrieve the port_type */
        config_type = lanes_cfg->config_type;

        if (config_type == BARCHETTA_PORT_TYPE_DUPLEX) {
            /* Retrieve Duplex lane configuration */
            barchetta_duplex_lanes_t *p_duplex_lanes = &(lanes_cfg->lanes.duplex);

            /* NumXXLanes */
            p_duplex_lanes->num_sys_lanes = __plp_barchetta_get_byte(&rx_msg);
            p_duplex_lanes->num_line_lanes = __plp_barchetta_get_byte(&rx_msg);

            /* SRLaneList */
            for (lane = 0; lane < p_duplex_lanes->num_sys_lanes; lane++) {
                p_duplex_lanes->sr_lane_list[lane] = __plp_barchetta_get_byte(&rx_msg);
            }

            /* STLaneList */
            for (lane = 0; lane < p_duplex_lanes->num_sys_lanes; lane++) {
                p_duplex_lanes->st_lane_list[lane] = __plp_barchetta_get_byte(&rx_msg);
            }

            /* LRLaneList */
            for (lane = 0; lane < p_duplex_lanes->num_line_lanes; lane++) {
                p_duplex_lanes->lr_lane_list[lane] = __plp_barchetta_get_byte(&rx_msg);
            }

            /* LTLaneList */
            for (lane = 0; lane < p_duplex_lanes->num_line_lanes; lane++) {
                p_duplex_lanes->lt_lane_list[lane] = __plp_barchetta_get_byte(&rx_msg);
            }
        } else if (config_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
            
        } else if (config_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
            
        }
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    } else {
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
}

/*******************************************************************************
 PURPOSE:  Function to configure polarity swaps based on user input

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_polarity_swap_wr(const plp_barchetta_phymod_access_t *pa,
        barchetta_config_polarity_swap_t *polarity_cfg) {
    uint8_t barchetta_tx_buf[12] = { 0 }; /* 12 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4] = { 0 }; /* 4 bytes are sufficient for response operation       */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint16_t length;
    uint8_t config_type;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];
    length = 0;

    /* Prepare the Message */
    /* Length, ConfigType, ConfigData */
    __plp_barchetta_put_half_word(&tx_msg, 0x0000); /* Length (real length inserted at the end */

    __plp_barchetta_put_byte(&tx_msg, polarity_cfg->config_type); /* ConfigType */
    length += 1;

    /* Retrieve the config_type */
    config_type = polarity_cfg->config_type;

    if (config_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        barchetta_duplex_polarity_swap_t *polarity_swap;
        /* Retrieve Duplex Pol Swaps configuration */
        polarity_swap = &(polarity_cfg->polarity_swap.duplex);

        /* NumSysLanes, NumLineLanes */
        __plp_barchetta_put_byte(&tx_msg, polarity_swap->num_sys_lanes);
        __plp_barchetta_put_byte(&tx_msg, polarity_swap->num_line_lanes);
        length += 2;

        /* [S|L][R|T]Polarity Swaps */
        __plp_barchetta_put_byte(&tx_msg, polarity_swap->sr_polarity_swap);
        __plp_barchetta_put_byte(&tx_msg, polarity_swap->st_polarity_swap);
        __plp_barchetta_put_byte(&tx_msg, polarity_swap->lr_polarity_swap);
        __plp_barchetta_put_byte(&tx_msg, polarity_swap->lt_polarity_swap);
        length += 4;
    } else if (config_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        
    } else if (config_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        
    }

    /* Length (at the first location) */
    tx_msg = &barchetta_tx_buf[0];
    __plp_barchetta_put_half_word(&tx_msg, length);

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_CONFIG_POL_SWAP, BARCHETTA_OP_WRITE, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0])) ;
    if (msg_send_status != BARCHETTA_OP_SUCCESS) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_config_polarity_swap_wr failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }

    /* Read the response (nothing to read) */
    (void) rx_msg;

    return (BARCHETTA_MSG_IF_RET_SUCCESS);
}

/*******************************************************************************
 PURPOSE:  Function to configure polarity swaps based on user input

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_polarity_swap_rd(const plp_barchetta_phymod_access_t *pa,
        barchetta_config_polarity_swap_t *polarity_cfg) {
    uint8_t barchetta_tx_buf[4] = { 0 };  /* 4 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[12] = { 0 }; /* 12 bytes are sufficient for response operation       */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t config_type;
    uint16_t length;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length */
    __plp_barchetta_put_half_word(&tx_msg, 0x0000); /* Length */

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_CONFIG_POL_SWAP, BARCHETTA_OP_READ, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]));
    if (msg_send_status != BARCHETTA_OP_SUCCESS) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_config_polarity_swap_rd failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }

    /* Read the response */
    length = __plp_barchetta_get_half_word(&rx_msg);

    /* length should at least be 7 bytes or more */
    if (length >= 7) {   
        polarity_cfg->config_type = __plp_barchetta_get_byte(&rx_msg); /* ConfigType */
        /* Retrieve the config_type */
        config_type = polarity_cfg->config_type;
        if (config_type == BARCHETTA_PORT_TYPE_DUPLEX) {
            /* Retrieve Duplex Pol Swaps configuration */
            barchetta_duplex_polarity_swap_t *polarity_swap;
            polarity_swap = &(polarity_cfg->polarity_swap.duplex);

            /* NumXXLanes*/
            polarity_swap->num_sys_lanes = __plp_barchetta_get_byte(&rx_msg);
            polarity_swap->num_line_lanes = __plp_barchetta_get_byte(&rx_msg);

            /* [S|L][R|T]PolSwaps*/
            polarity_swap->sr_polarity_swap = __plp_barchetta_get_byte(&rx_msg);
            polarity_swap->st_polarity_swap = __plp_barchetta_get_byte(&rx_msg);
            polarity_swap->lr_polarity_swap = __plp_barchetta_get_byte(&rx_msg);
            polarity_swap->lt_polarity_swap = __plp_barchetta_get_byte(&rx_msg);
        } else if (config_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
            
        } else if (config_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
            
        }
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    } else {
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
}

/*******************************************************************************
 PURPOSE:  Function to configure a Port (CONFIG_PORT.WRITE)

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_port_wr(const plp_barchetta_phymod_access_t *pa,
        barchetta_port_config_t *port_cfg) {
    uint8_t barchetta_tx_buf[32] = { 0 }; /* 32 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4]  = { 0 }; /* 4 bytes are sufficient for response operation         */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint16_t length;
    uint8_t port_type;
    uint8_t lane_cnt;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];
    length = 0;

    /* Prepare the Message */
    /* Length, PortNum, PortType, PortMode, PortData */
    __plp_barchetta_put_half_word(&tx_msg, 0x0000); /* Length (real length inserted at the end */

    __plp_barchetta_put_byte(&tx_msg, port_cfg->port_num);  /* PortNum  */
    __plp_barchetta_put_byte(&tx_msg, port_cfg->port_type); /* PortType */
    __plp_barchetta_put_byte(&tx_msg, port_cfg->port_mode); /* PortMode */
    length += 3;

    /* Retrieve port direction from PortType */
    port_type = (port_cfg->port_type);

    if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
        /* Retrieve duplex port lane configuration */
        barchetta_duplex_port_lanes_t *p_duplex_port_lane = &(port_cfg->port_lanes.duplex);

        /* NumXXLanes */
        __plp_barchetta_put_byte(&tx_msg, p_duplex_port_lane->num_sys_lanes);
        __plp_barchetta_put_byte(&tx_msg, p_duplex_port_lane->num_line_lanes);
        length += 2;

        /* SysLaneList */
        for (lane_cnt = 0; lane_cnt < p_duplex_port_lane->num_sys_lanes; lane_cnt++) {
            __plp_barchetta_put_byte(&tx_msg, p_duplex_port_lane->sys_lane_list[lane_cnt]);
        }
        length += p_duplex_port_lane->num_sys_lanes;

        /* LineLaneList */
        for (lane_cnt = 0; lane_cnt < p_duplex_port_lane->num_line_lanes; lane_cnt++) {
            __plp_barchetta_put_byte(&tx_msg, p_duplex_port_lane->line_lane_list[lane_cnt]);
        }
        length += p_duplex_port_lane->num_line_lanes;
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
        
    } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
        
    }

    /* Length (at the first location) */
    tx_msg = &barchetta_tx_buf[0];
    __plp_barchetta_put_half_word(&tx_msg, length);

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_CONFIG_PORT, BARCHETTA_OP_WRITE, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]) );
    if (msg_send_status != BARCHETTA_OP_SUCCESS) {
		PHYMOD_DEBUG_ERROR(("ERROR: msg_config_port_wr failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
    /* Read the response (nothing to read) */
    (void) rx_msg;
    return (BARCHETTA_MSG_IF_RET_SUCCESS);
}

/*******************************************************************************
 PURPOSE:  Function to read a configuration of port (CONFIG_PORT.READ)

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_port_rd(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num, barchetta_port_config_t *port_cfg) {
    uint8_t barchetta_tx_buf[4]  = { 0 };  /* 4 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[64] = { 0 }; /* 64 bytes are sufficient for response operation       */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t port_type;
    uint8_t lane_cnt;
    uint16_t length;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0001); /* Length  */
    __plp_barchetta_put_byte(&tx_msg, port_num);    /* PortNum */

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_CONFIG_PORT, BARCHETTA_OP_READ, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]));
    if (msg_send_status != BARCHETTA_OP_SUCCESS) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_config_port_rd failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }

    /* Read the response */
    length = __plp_barchetta_get_half_word(&rx_msg);

    /* length should atleast be 10 bytes or more */
    if (length >= 7) { 
        port_cfg->port_num = __plp_barchetta_get_byte(&rx_msg);  /* PortNum  */
        port_cfg->port_type = __plp_barchetta_get_byte(&rx_msg); /* PortType */
        port_cfg->port_mode = __plp_barchetta_get_byte(&rx_msg); /* PortMode */

        /* Retrieve port direction from port_type */
        port_type = (port_cfg->port_type);

        if (port_type == BARCHETTA_PORT_TYPE_DUPLEX) {
            /* Retrieve Duplex port lane configuration */
            barchetta_duplex_port_lanes_t *p_duplex_port_lane = &(port_cfg->port_lanes.duplex);

            /* NumXXLanes */
            p_duplex_port_lane->num_sys_lanes = __plp_barchetta_get_byte(&rx_msg);
            p_duplex_port_lane->num_line_lanes = __plp_barchetta_get_byte(&rx_msg);

            /* SysLaneList */
            for (lane_cnt = 0; lane_cnt < p_duplex_port_lane->num_sys_lanes; lane_cnt++) {
                p_duplex_port_lane->sys_lane_list[lane_cnt] = __plp_barchetta_get_byte(&rx_msg);
            }
            /* LineLaneList */
            for (lane_cnt = 0; lane_cnt < p_duplex_port_lane->num_line_lanes; lane_cnt++) {
                p_duplex_port_lane->line_lane_list[lane_cnt] = __plp_barchetta_get_byte(&rx_msg);
            }
        } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_SR2LT) {
            
        } else if (port_type == BARCHETTA_PORT_TYPE_SIMPLEX_LR2ST) {
            
        }
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    } else {
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
}

/*******************************************************************************
 PURPOSE:  Function to Start the process of enabling a Port

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_enable_port_start(const plp_barchetta_phymod_access_t *pa,
        barchetta_enable_port_t *port_en) {
    uint8_t barchetta_tx_buf[64] = { 0 }; /* 64 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4]  = { 0 }; /* 4 bytes are sufficient for response operation         */
    barchetta_pmd_config_t *pmd_cfg;
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint16_t length;
    uint8_t side;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];
    length = 0;

    /* Prepare the Message       */
    /* Length, PortNum, PortType */
    __plp_barchetta_put_half_word(&tx_msg, 0x0000); /* Length (real length inserted at the end */

    __plp_barchetta_put_byte(&tx_msg, port_en->port_num);     /* PortNum     */
    __plp_barchetta_put_byte(&tx_msg, port_en->traffic_type); /* TrafficType */
    __plp_barchetta_put_byte(&tx_msg, port_en->ll_mode);      /* LLMode      */
    __plp_barchetta_put_byte(&tx_msg, port_en->div2en);       /* Div2En      */

    length += 4;

    /* PMD Configuration (SIDE_SYS, SIDE_LINE) */
    for (side = BARCHETTA_SIDE_SYS; side <= BARCHETTA_SIDE_LINE; side++) {
        pmd_cfg = &(port_en->pmd_cfg[side]);

        /* ClockConfig */
        __plp_barchetta_put_byte(&tx_msg, pmd_cfg->clock_mode);
        __plp_barchetta_put_byte(&tx_msg, pmd_cfg->pll_sel);
        __plp_barchetta_put_byte(&tx_msg, pmd_cfg->osr);

        length += 3;

        /* ANConfig */
        __plp_barchetta_put_byte(&tx_msg, pmd_cfg->an_opt);
        length += 1;

        if (pmd_cfg->an_opt) {
            __plp_barchetta_put_byte(&tx_msg, pmd_cfg->an_use_pcs_mon);
            __plp_barchetta_put_byte(&tx_msg, pmd_cfg->an_master_lane);
            length += 2;
        }
        __plp_barchetta_put_byte(&tx_msg, pmd_cfg->tx_training_opt);       /* TxTrainingOpt  */
        __plp_barchetta_put_byte(&tx_msg, pmd_cfg->fec_opt);               /* FECOpt         */
        __plp_barchetta_put_half_word(&tx_msg, pmd_cfg->lane_config_word); /* LaneConfigWord */
        length += 4;
    }

    /* Length (at the first location) */
    tx_msg = &barchetta_tx_buf[0];
    __plp_barchetta_put_half_word(&tx_msg, length);

    /* Read the response (nothing to read) */
    (void) rx_msg;

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_ENABLE_PORT, BARCHETTA_OP_START, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]) ) ;
    if (msg_send_status != BARCHETTA_OP_PROCESSING) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_enable_port_start failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    } else {
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    }
}

/*******************************************************************************
 PURPOSE:  Function to wait until the process of enabling a Port is complete

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_enable_port_start_result(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num) {
    uint8_t barchetta_tx_buf[4] = { 0 }; /* 4 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4] = { 0 }; /* 4 bytes are sufficient for response operation        */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t result;
    uint16_t retry_count = BARCHETTA_MSG_IF_RETRY_COUNT;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0001); /* Length  */
    __plp_barchetta_put_byte(&tx_msg, port_num);    /* PortNum */

    /* Wait until the background state machine completes enabling the port */
    do {
        result = _plp_barchetta_msg_send( pa, BARCHETTA_FUNC_ENABLE_PORT, BARCHETTA_OP_START_RESULT,
                                      &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]) );

    } while ((result == BARCHETTA_OP_PROCESSING) && (--retry_count));

    /* Read the response (nothing to read) */
    (void) rx_msg;

    if (result == BARCHETTA_OP_SUCCESS) {
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_enable_port_start_result failed:%d\n",result));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
}

/*******************************************************************************
 PURPOSE:  Function to Start enabling a Port and wait until the process is complete

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_enable_port(const plp_barchetta_phymod_access_t *pa,
        barchetta_enable_port_t *port_en) {
    /* ARUN : Combine both the function*/
    /* Start Enable Port */
    if (BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_enable_port_start(pa, port_en)) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_enable_port_start failed\n"));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
    /* Wait until the firmware completes enabling the port */
    if (BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_enable_port_start_result(pa, port_en->port_num)) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_enable_port_start_result failed\n"));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
    return (BARCHETTA_MSG_IF_RET_SUCCESS);
}

/*******************************************************************************
 PURPOSE:  Function to read PMD configuration of specified side of a port (CONFIG_PMD.READ)

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_config_pmd_rd(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num, uint8_t pmd_side, barchetta_config_pmd_t *pmd_cfg) {
    uint8_t barchetta_tx_buf[8]  = { 0 }; /* 8 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[64] = { 0 }; /* 64 bytes are sufficient for response operation       */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint16_t length;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0002); /* Length   */

    __plp_barchetta_put_byte(&tx_msg, port_num);    /* PortNum  */
    __plp_barchetta_put_byte(&tx_msg, pmd_side);    /* PMD Side */

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_CONFIG_PMD, BARCHETTA_OP_READ, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]) ) ;
    if (msg_send_status != BARCHETTA_OP_SUCCESS) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_config_pmd_rd failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }

    /* Read the response */
    length = __plp_barchetta_get_half_word(&rx_msg);

    /* length should atleast be 7 bytes or more */
    if (length >= 7) { 
        pmd_cfg->port_num = __plp_barchetta_get_byte(&rx_msg); /* Port Number  */
        pmd_cfg->pmd_side = __plp_barchetta_get_byte(&rx_msg); /* PMD side     */

        /* Retrieve AN option */
        pmd_cfg->an_opt = __plp_barchetta_get_byte(&rx_msg);   /* AN option    */

        if (pmd_cfg->an_opt) {
            pmd_cfg->an_use_pcs_mon = __plp_barchetta_get_byte(&rx_msg);
            pmd_cfg->an_master_lane = __plp_barchetta_get_byte(&rx_msg);
        }
        pmd_cfg->tx_training_opt = __plp_barchetta_get_byte(&rx_msg);
        pmd_cfg->fec_opt = __plp_barchetta_get_byte(&rx_msg);
        pmd_cfg->lane_config_word = __plp_barchetta_get_half_word(&rx_msg);
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_config_pmd_rd failed\n"));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
}

/*******************************************************************************
 PURPOSE:  Function to Start the process of modifying a Port

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_modify_port_start(const plp_barchetta_phymod_access_t *pa,
        barchetta_modify_port_t *port_mod) {
    uint8_t barchetta_tx_buf[32] = { 0 }; /* 32 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4]  = { 0 }; /* 4 bytes are sufficient for response operation         */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint16_t length = 0;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message      */
    /* Length, PortNum, PMDSide */
    __plp_barchetta_put_half_word(&tx_msg, 0x0000); /* Length (real length inserted at the end */

    __plp_barchetta_put_byte(&tx_msg, port_mod->port_num); /* PortNum */
    __plp_barchetta_put_byte(&tx_msg, port_mod->pmd_side); /* PMDSide */
    length += 2;

    /* ANConfig */
    __plp_barchetta_put_byte(&tx_msg, port_mod->an_opt);
    length += 1;
    if (port_mod->an_opt) {
        __plp_barchetta_put_byte(&tx_msg, port_mod->an_use_pcs_mon);
        __plp_barchetta_put_byte(&tx_msg, port_mod->an_master_lane);
        length += 2;
    }
    __plp_barchetta_put_byte(&tx_msg, port_mod->tx_training_opt);       /* TxTrainingOpt  */
    __plp_barchetta_put_byte(&tx_msg, port_mod->fec_opt);               /* FECOpt         */
    __plp_barchetta_put_half_word(&tx_msg, port_mod->lane_config_word); /* LaneConfigWord */
    length += 4;

    /* Length (at the first location) */
    tx_msg = &barchetta_tx_buf[0];
    __plp_barchetta_put_half_word(&tx_msg, length);

    /* Read the response (nothing to read) */
    (void) rx_msg;

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send( pa, BARCHETTA_FUNC_MODIFY_PORT, BARCHETTA_OP_START, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0])) ;
    if (msg_send_status != BARCHETTA_OP_PROCESSING) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_modify_port_start failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    } else {
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    }
}

/*******************************************************************************
 PURPOSE:  Function to wait until the process of modifying a Port is complete

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_modify_port_start_result(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num) {
    uint8_t barchetta_tx_buf[4] = { 0 }; /* 4 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4] = { 0 }; /* 4 bytes are sufficient for response operation        */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t result;
    uint16_t retry_count = BARCHETTA_MSG_IF_RETRY_COUNT;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0001); /* Length  */
    __plp_barchetta_put_byte(&tx_msg, port_num);    /* PortNum */

    /* Wait until the background state machine completes modifying the port */
    do {
        result = _plp_barchetta_msg_send( pa, BARCHETTA_FUNC_MODIFY_PORT, BARCHETTA_OP_START_RESULT,
                                      &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]) );

    } while ((result == BARCHETTA_OP_PROCESSING) && (--retry_count));

    /* Read the response (nothing to read) */
    (void) rx_msg;

    if (result == BARCHETTA_OP_SUCCESS) {
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_modify_port_start_result failed:%d\n",result));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
}

/*******************************************************************************
 PURPOSE:  Function to Start modifying a Port and wait until the process is complete

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_modify_port(const plp_barchetta_phymod_access_t *pa,
        barchetta_modify_port_t *port_mod) {
    /* Start Modify Port */
    if (BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_modify_port_start(pa, port_mod)) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_modify_port_start failed\n"));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
    /* Wait until the firmware completes modifying the port */
    if (BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_modify_port_start_result(pa, port_mod->port_num)) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_modify_port_start_result failed\n"));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
    return (BARCHETTA_MSG_IF_RET_SUCCESS);
}

/*******************************************************************************
 PURPOSE:  Function to Start the process of disabling a Port

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_disable_port_start(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num) {
    uint8_t barchetta_tx_buf[4] = { 0 }; /* 4 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4] = { 0 }; /* 4 bytes are sufficient for response operation        */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0001); /* Length  */
    __plp_barchetta_put_byte(&tx_msg, port_num);    /* PortNum */

    (void) rx_msg;

    /* Send Message */
	msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_DISABLE_PORT, BARCHETTA_OP_START, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0])) ;
    if (msg_send_status != BARCHETTA_OP_PROCESSING) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_disable_port_start failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    } else {
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    }
}

/*******************************************************************************
 PURPOSE:  Function to wait until the process of disabling a Port is complete

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_disable_port_start_result(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num) {
    uint8_t barchetta_tx_buf[4] = { 0 }; /* 4 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4] = { 0 }; /* 4 bytes are sufficient for response operation        */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t result;
    uint16_t retry_count = BARCHETTA_MSG_IF_RETRY_COUNT;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0001); /* Length  */
    __plp_barchetta_put_byte(&tx_msg, port_num);    /* PortNum */

    /* Wait until the background state machine completes disabling the port */
    do {
        result = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_DISABLE_PORT, BARCHETTA_OP_START_RESULT,
                                     &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]));

    } while ((result == BARCHETTA_OP_PROCESSING) && (--retry_count));

    /* Read the response (nothing to read) */
    (void) rx_msg;

    if (result == BARCHETTA_OP_SUCCESS) {
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_disable_port_start_result failed:%d\n",result));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
}

/*******************************************************************************
 PURPOSE:  Function to Start disabling a Port and wait until the process is complete

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_disable_port(const plp_barchetta_phymod_access_t *pa, uint8_t port_num) {
    /* Start Disable Port */
    if (BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_disable_port_start(pa, port_num)) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_disable port_start failed\n"));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
    /* Wait until the firmware completes disabling the port */
    if (BARCHETTA_MSG_IF_RET_ERROR == _plp_barchetta_msg_disable_port_start_result(pa, port_num)) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_disable port_start_result failed\n"));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
    return (BARCHETTA_MSG_IF_RET_SUCCESS);
}

/*******************************************************************************
 PURPOSE:  Function to pause a Port (PAUSE_PORT.WRITE)

 COMMENT:  With this API firmware relinquishes the control of the specified port
 *******************************************************************************/
uint8_t _plp_barchetta_msg_pause_port_wr(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num) {
    uint8_t barchetta_tx_buf[4] = { 0 }; /* 4 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4] = { 0 }; /* 4 bytes are sufficient for response operation        */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0001);/* Length  */
    __plp_barchetta_put_byte(&tx_msg, port_num);   /* PortNum */

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_PAUSE_PORT, BARCHETTA_OP_WRITE, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0])) ;
    if (msg_send_status != BARCHETTA_OP_SUCCESS) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_pause_port_wr failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }

    /* Read the response (nothing to read) */
    (void) rx_msg;

    return (BARCHETTA_MSG_IF_RET_SUCCESS);
}

/*******************************************************************************
 PURPOSE:  Function to resume a Port (RESUME_PORT.WRITE)

 COMMENT:  With this API firmware takes back the control of the port
 *******************************************************************************/
uint8_t _plp_barchetta_msg_resume_port_wr(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num) {
    uint8_t barchetta_tx_buf[4] = { 0 }; /* 4 bytes are sufficient for transmitting this message */
    uint8_t barchetta_rx_buf[4] = { 0 }; /* 4 bytes are sufficient for response operation        */
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0001); /* Length  */
    __plp_barchetta_put_byte(&tx_msg, port_num);    /* PortNum */

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_RESUME_PORT, BARCHETTA_OP_WRITE, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]));
    if (msg_send_status != BARCHETTA_OP_SUCCESS) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_resume_port_wr failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
    /* Read the response (nothing to read) */
    (void) rx_msg;

    return (BARCHETTA_MSG_IF_RET_SUCCESS);
}

/*******************************************************************************
 PURPOSE:  Function to Start the process of switching to Failover port on a Mux Port

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_switch_mux_port_start(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num) {
    uint8_t barchetta_tx_buf[4] = { 0 };
    uint8_t barchetta_rx_buf[4] = { 0 };
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t msg_send_status = 0;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0001); /* Length  */
    __plp_barchetta_put_byte(&tx_msg, port_num);    /* PortNum */

    (void) rx_msg;

    /* Send Message */
    msg_send_status = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_SWITCH_MUX_PORT, BARCHETTA_OP_START, &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0])) ;
    if (msg_send_status != BARCHETTA_OP_PROCESSING) {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_switch_mux_port_start failed:%d\n",msg_send_status));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    } else {
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    }
}

/*******************************************************************************
 PURPOSE:  Function to wait until the process of switching to Fail over port on a MUX

 COMMENT:
 *******************************************************************************/
uint8_t _plp_barchetta_msg_switch_mux_port_start_result(const plp_barchetta_phymod_access_t *pa,
        uint8_t port_num) {
    uint8_t barchetta_tx_buf[4] = { 0 };
    uint8_t barchetta_rx_buf[4] = { 0 };
    uint8_t *tx_msg;
    uint8_t *rx_msg;
    uint8_t result;
    uint16_t retry_count = BARCHETTA_MSG_IF_RETRY_COUNT;

    /* use the global buffers for tx/rx message */
    tx_msg = &barchetta_tx_buf[0];
    rx_msg = &barchetta_rx_buf[0];

    /* Prepare the Message */
    /* Length, PortNum     */
    __plp_barchetta_put_half_word(&tx_msg, 0x0001); /* Length  */
    __plp_barchetta_put_byte(&tx_msg, port_num);    /* PortNum */

    /* Wait until the background state machine completes disabling the port */
    do {
        result = _plp_barchetta_msg_send(pa, BARCHETTA_FUNC_SWITCH_MUX_PORT, BARCHETTA_OP_START_RESULT,
                                     &barchetta_tx_buf[0], &barchetta_rx_buf[0], sizeof(barchetta_rx_buf)/sizeof(barchetta_rx_buf[0]) );

    } while ((result == BARCHETTA_OP_PROCESSING) && (--retry_count));

    /* Read the response (nothing to read) */
    (void) rx_msg;

    if (result == BARCHETTA_OP_SUCCESS) {
        return (BARCHETTA_MSG_IF_RET_SUCCESS);
    } else {
        PHYMOD_DEBUG_ERROR(("ERROR: msg_switch_mux_port_start_result failed:%d\n",result));
        return (BARCHETTA_MSG_IF_RET_ERROR);
    }
}
