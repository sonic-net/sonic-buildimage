/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#ifndef INCLUDE_APML_RMI_H_
#define INCLUDE_APML_RMI_H_

#include "apml_err.h"

/** \file esmi_rmi.h
 *  Header file for the APML library for SB-RMI functionality access.
 *  All required function, structure, enum, etc. definitions should be defined
 *  in this file for SB-RMI Register accessing.
 *
 *  @details  This header file contains the following:
 *  APIs prototype of the APIs exported by the APML library.
 *  Description of the API, arguments and return values.
 *  The Error codes returned by the API.
 */
#define MAX_ALERT_REG_V21_DENSE		48	//!< Max alert registers for V21 dense platform //
#define MAX_ALERT_REG           	32      //!< Max alert register //
#define MAX_THREAD_REG_V20      	32      //!< Max thread register for rev 20 //
#define MAX_THREAD_REG_V10      	16      //!< Max thread register for rev 10 //
#define MAX_THREAD_REG_V21_DENSE	48      //!< Max thread register for rev 21 dense platform //

/**
 * @brief Error codes retured by APML mailbox functions
 */
typedef enum {
	SBRMI_SUCCESS = 0x0,
	SBRMI_CMD_TIMEOUT = 0x11,
	SBRMI_WARM_RESET = 0x22,
	SBRMI_UNKNOWN_CMD_FORMAT = 0x40,
	SBRMI_INVALID_READ_LENGTH = 0x41,
	SBRMI_EXCESSIVE_DATA_LENGTH = 0x42,
	SBRMI_INVALID_THREAD = 0x44,
	SBRMI_UNSUPPORTED_CMD = 0x45,
	SBRMI_CMD_ABORTED = 0x81
} sbrmi_status_code;

/**
 * @brief SB-RMI(Side-Band Remote Management Interface) features
 * register access
 */
typedef enum {
	SBRMI_REVISION = 0x0,
	SBRMI_CONTROL,
	SBRMI_STATUS,
	SBRMI_READSIZE,
	SBRMI_THREADENABLESTATUS0,
	SBRMI_ALERTSTATUS0 = 0x10,
	SBRMI_ALERTSTATUS15 = 0x1F,
	SBRMI_ALERTMASK0 = 0x20,
	SBRMI_ALERTMASK15 = 0x2F,
	SBRMI_SOFTWAREINTERRUPT = 0x40,
	SBRMI_THREADNUMBER,
	SBRMI_THREAD128CS = 0x4B,
	SBRMI_RASSTATUS,
	SBRMI_THREADNUMBERLOW = 0x4E,
	SBRMI_THREADNUMBERHIGH = 0x4F,
	SBRMI_ALERTSTATUS16 = 0x50,
	SBRMI_ALERTSTATUS31 = 0x5F,
	SBRMI_ALERTSTATUS32 = 0x220,
	SBRMI_ALERTSTATUS47 = 0x22F,
	SBRMI_MP0OUTBNDMSG0 = 0x80,
	SBRMI_MP0OUTBNDMSG7 = 0x87,
	SBRMI_ALERTMASK16 = 0xC0,
	SBRMI_ALERTMASK31 = 0xCF,
	SBRMI_ALERTMASK32 = 0x1C0,
	SBRMI_ALERTMASK47 = 0x1CF,
} sbrmi_registers;

/* SBRMI registers Revision 0x10 */
/**
 * @brief thread enable register revision 0x10
 */
extern const uint8_t thread_en_reg_v10[MAX_THREAD_REG_V10];

/* SBRMI registers Revision 0x20 */
/**
 * @brief thread enable register revision 0x20
 */
extern const uint8_t thread_en_reg_v20[MAX_THREAD_REG_V20];

/**
 * @brief alert status register
 */
extern const uint8_t alert_status[MAX_ALERT_REG];

/**
 * @brief alert mask
 */
extern const uint8_t alert_mask[MAX_ALERT_REG];

/*****************************************************************************/
/** @defgroup SB-RMIRegisterAccess SB-RMI Register Read Byte Protocol
 *  The SB-RMI registers can be read or written from the SMBus interface using
 *  the SMBus defined PEC-optional Read Byte and Write Byte protocols with the
 *  SB-RMI register number in the command byte.
 *  @{
 */

/**
 *  @brief Read one byte from a given SB_RMI register number
 *  provided socket index and buffer to get the read data for a particular
 *  SB-RMI command register.
 *
 *  @details Given a socket index @p socket_ind and a pointer to hold the
 *  output at uint8_t @p buffer, this function will get the value from a
 *  particular command of SB_RMI register.
 *
 *  @param[in] soc_num Socket uindex.
 *
 *  @param[inout] buffer a pointer to a uint8_t that indicates value to hold
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
/**
 *  @brief This value specifies the APML specification revision that the
 *  product is compliant to. 0x10 = 1.0x Revision.
 */
/* SBRMI registers Revision 0x10 */
/**
 * @brief thread enable register revision 0x10
 */
extern const uint8_t thread_en_reg_v10[MAX_THREAD_REG_V10];

/* SBRMI registers Revision 0x20 */
/**
 * @brief thread enable register revision 0x20
 */
extern const uint8_t thread_en_reg_v20[MAX_THREAD_REG_V20];

/* SBRMI registers Revision 0x21 for Dense Platform */
/**
 * @brief thread enable register revision 0x21 for dense plaform
 */
extern const uint16_t thread_en_reg_v21_dense[MAX_THREAD_REG_V21_DENSE];
/**
 * @brief alert status register
 */
extern const uint8_t alert_status[MAX_ALERT_REG];

/**
 * @brief alert status register for V21 dense platform
 */
extern const uint16_t alert_status_v21_dense[MAX_ALERT_REG_V21_DENSE];

/**
 * @brief alert mask
 */
extern const uint8_t alert_mask[MAX_ALERT_REG];

/**
 * @brief alert mask
 */
extern const uint16_t alert_mask_v21_dense[MAX_ALERT_REG_V21_DENSE];

/**
 *  @brief Read revision from SB_RMI register command.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_revision(uint8_t soc_num,
				 uint8_t *buffer);
/**
 *  @brief Read Control byte from SB_RMI register command.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_control(uint8_t soc_num,
				uint8_t *buffer);
/**
 *  @brief Read one byte of Status value from SB_RMI register command.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_status(uint8_t soc_num,
			       uint8_t *buffer);
/**
 *  @brief This register specifies the number of bytes to return when using
 *  the block read protocol to read SBRMI_x[4F:10].
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_readsize(uint8_t soc_num,
				 uint8_t *buffer);
/**
 *  @brief Read one byte of Thread Status from SB_RMI register command.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_threadenablestatus(uint8_t soc_num,
					   uint8_t *buffer);
/**
 *  @brief Read one byte of Thread Status from SB_RMI register command.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_multithreadenablestatus(uint8_t soc_num,
						uint8_t *buffer);
/**
 *  @brief This register is used by the SMBus master to generate an
 *  interrupt to the processor to indicate that a message is
 *  available..
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_swinterrupt(uint8_t soc_num,
				    uint8_t *buffer);
/**
 *  @brief This register indicates the maximum number of threads present.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_threadnumber(uint8_t soc_num,
				     uint8_t *buffer);

/**
 *  @brief This register will read the message running on the MP0.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_mp0_msg(uint8_t soc_num,
				uint8_t *buffer);

/**
 *  @brief This function will read bit vector for all the threads.
 *  Value of 1 indicates MCE occured for the thread and is set by
 *  hardware.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] num_of_alert_status_reg number of alert status
 *  registers.
 *
 *  @param[inout] buffer a pointer to read all "num_of_alert_status_reg"
 *  of alert status registers. Buffer length should be
 *  equal to "num_of_alert_status_reg" value.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval None-zero is returned upon failure.
 */
oob_status_t read_sbrmi_alert_status(uint8_t soc_num,
				     uint8_t num_of_alert_status_reg,
				     uint8_t **buffer);

/**
 *  @brief This function will read bit vector for all the threads.
 *  Value of 1 indicates alert signaling disabled for corresponding
 *  SBRMI::AlertStatus[MceStat] for the thread.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] num_of_alert_mask_reg number of alert mask
 *  registers.
 *
 *  @param[inout] buffer a pointer to read all "num_of_alert_mask_reg"
 *  of alert mask registers. Buffer length should be equal to
 *  "num_of_alert_mask_reg" value.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval None-zero is returned upon failure.
 */
oob_status_t read_sbrmi_alert_mask(uint8_t soc_num,
				   uint8_t num_of_alert_mask_reg,
				   uint8_t **buffer);

/**
 *  @brief This register will read the inbound message.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_inbound_msg(uint8_t soc_num,
				    uint8_t *buffer);

/**
 *  @brief This register will read the outbound message.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_outbound_msg(uint8_t soc_num,
				     uint8_t *buffer);

/**
 *  @brief This register indicates the low part of maximum number of threads.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_threadnumberlow(uint8_t soc_num,
					uint8_t *buffer);

/**
 *  @brief This register indicates the upper part of maximum number of threads.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_threadnumberhi(uint8_t soc_num,
				       uint8_t *buffer);

/**
 *  @brief This register is used to read the thread cs.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_thread_cs(uint8_t soc_num,
				  uint8_t *buffer);

/**
 *  @brief This register will read the ras status.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 */
oob_status_t read_sbrmi_ras_status(uint8_t soc_num,
				   uint8_t *buffer);

/**
 *  @brief This API will clear ras status register.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] buffer bit mask to clear ras status bits
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t clear_sbrmi_ras_status(uint8_t soc_num, uint8_t buffer);

/**
 *  @brief Get the number of threads per socket
 *
 *  @details Get the total number of threads in a socket.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] threads_per_socket is returned
 *
 *  @retval threads_per_socket is returned upon successful call.
 *
 */
oob_status_t esmi_get_threads_per_socket(uint8_t soc_num,
					 uint32_t *threads_per_socket);
/** @} */  // end of SB-RMI Register access
/*****************************************************************************/

#endif  // INCLUDE_APML_RMI_H_
