/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef INCLUDE_APML_H_
#define INCLUDE_APML_H_

#include <stdbool.h>

#include "amd-apml.h"
#include "apml_err.h"

/** \file apml.h
 *  Main header file for the APML library.
 *  All required function, structure, enum, etc. definitions should be defined
 *  in this file.
 *
 *  @details  This header file contains the following:
 *  APIs prototype of the APIs exported by the APML library.
 *  Description of the API, arguments and return values.
 *  The Error codes returned by the API.
 */

 /**
 * @brief SBRMI outbound messages defined in the APML library
 */
typedef enum {
	SBRMI_OUTBNDMSG0 = 0x30,
	SBRMI_OUTBNDMSG1,
	SBRMI_OUTBNDMSG2,
	SBRMI_OUTBNDMSG3,
	SBRMI_OUTBNDMSG4,
	SBRMI_OUTBNDMSG5,
	SBRMI_OUTBNDMSG6,
	SBRMI_OUTBNDMSG7,
} sbrmi_outbnd_msg;

 /**
 * @brief SBRMI inbound messages defined in the APML library
 *
 * Usage convention is:
 • SBRMI::InBndMsg_inst0 is command
 • SBRMI::InBndMsg_inst[4:1] are 32 bit data
 • SBRMI::InBndMsg_inst[6:5] are reserved.
 • SBRMI::InBndMsg_inst7: [7] Must be 1'b1 to send message to firmware
 */
typedef enum {
	SBRMI_INBNDMSG0 = 0x38,
	SBRMI_INBNDMSG1,
	SBRMI_INBNDMSG2,
	SBRMI_INBNDMSG3,
	SBRMI_INBNDMSG4,
	SBRMI_INBNDMSG5,
	SBRMI_INBNDMSG6,
	SBRMI_INBNDMSG7,
} sbrmi_inbnd_msg;

/**
 * @brief processor family and model details
 * LEGACY_PLATFORMS: Family:0x17, Model: 30h~3Fh (Rome),
 *		     Family:0x19, MOdel: 00h~0Fh (Milan)
 * FAM_19_MOD_10:    Family:0x19, MOdel: 10h~1Fh (Genoa)
 * FAM_19_MOD_90:    Family:0x19, MOdel: 90h~9Fh (MI300A)
 * FAM_1A_MOD_00:    Family:0x1A, MOdel: 00h~0Fh (Turin)
 * FAM_1A_MOD_10:    Family:0x1A, MOdel: 10h~1Fh (Turin-D)
 * FAM_19_MOD_80:    Family:0x19, MOdel: 80h~8Fh (MI300C)
 * FAM_19_MOD_A0:    Family:0x19, Model: A0~AFh  (Genoa Dense)
 */
typedef enum {
	NOT_SUPPORTED,
	LEGACY_PLATFORMS,
	FAM_19_MOD_10,
	FAM_19_MOD_90,
	FAM_1A_MOD_00,
	FAM_1A_MOD_10,
	FAM_19_MOD_80,
	FAM_19_MOD_A0,
} PROC_DETAILS;

#define SBRMI		"sbrmi"		//!< SBRMI module //
#define SBTSI		"sbtsi"		//!< SBTSI module //
#define MAX_DEV_COUNT	8		//!< APML ADDRESSES count

extern const uint16_t sbrmi_addr[MAX_DEV_COUNT];	//!< SBRMI addresses //
extern const uint16_t sbtsi_addr[MAX_DEV_COUNT];	//!< SBTSI addresses //

/**
 *  @brief Reads data for the given register.
 *
 *  @details This function will read the data for the given register.
 *  NOTE: In future, this API will be deprecated, as individual APIs for
 *  read SBRMI and read SBTSI support is available.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] reg_offset Register offset for RMI/TSI I/F.
 *
 *  @param[in] file_name Character device file name for RMI/TSI I/F.
 *
 *  @param[out] buffer output value for the register.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t esmi_oob_read_byte(uint8_t soc_num, uint16_t reg_offset,
				char *file_name, uint8_t *buffer);

/**
 *  @brief Writes data to the specified register.
 *
 *  @details This function will write the data to the specified register.
 *  NOTE: In future, this API will be deprecated, as individual APIs for
 *  write SBRMI and write SBTSI support is available.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] file_name Character device file name for RMI/TSI I/F.
 *
 *  @param[in] reg_offset Register offset for RMI/TSI I/F.
 *
 *  @param[in] value data to write to the register.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t esmi_oob_write_byte(uint8_t soc_num, uint16_t reg_offset,
				 char *file_name, uint8_t value);

/**
 *  @brief Reads data for the given RMI register.
 *
 *  @details This function will read the data for the given RMI register.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] reg_offset RMI register offset.
 *
 *  @param[out] buffer output value for the register.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t esmi_oob_rmi_read_byte(uint8_t soc_num, uint16_t reg_offset,
				    uint8_t *buffer);
/**
 *  @brief Reads data for the given TSI register.
 *
 *  @details This function will read the data for the given TSI register.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] reg_offset TSI register offset.
 *
 *  @param[out] buffer output value for the register.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t esmi_oob_tsi_read_byte(uint8_t soc_num, uint8_t reg_offset,
				    uint8_t *buffer);

/**
 *  @brief Writes data to the specified RMI register.
 *
 *  @details This function will write the data to the specified RMI register.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] reg_offset RMI register offset.
 *
 *  @param[in] value data to write to the TSI register.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t esmi_oob_rmi_write_byte(uint8_t soc_num, uint16_t reg_offset,
				     uint8_t value);

/**
 *  @brief Writes data to the specified TSI register.
 *
 *  @details This function will write the data to the specified TSI register.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] reg_offset TSI register offset.
 *
 *  @param[in] value data to write to the TSI register.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t esmi_oob_tsi_write_byte(uint8_t soc_num, uint8_t reg_offset,
				     uint8_t value);

/**
 *  @brief Reads mailbox command data.
 *
 *  @details This function will read mailbox command data.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] cmd mailbox command.
 *
 *  @param[in] input data.
 *
 *  @param[out] buffer output data for the given mailbox command.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t esmi_oob_read_mailbox(uint8_t soc_num, uint32_t cmd,
				   uint32_t input, uint32_t *buffer);

/**
 *  @brief Writes data to the given  mailbox command.
 *
 *  @details This function will writes data to mailbox command.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] cmd mailbox command.
 *
 *  @param[in] data input data.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t esmi_oob_write_mailbox(uint8_t soc_num,
				    uint32_t cmd, uint32_t data);
/**
 *  @brief Writes data to RMI device file
 *
 *  @details This function will write data to RMI device file,
 *  through ioctl.
 *
 *  @param[in] soc_num  Socket index.
 *
 *  @param[in] msg struct apml_message which contains information about the protocol,
 *  input/output data etc.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t sbrmi_xfer_msg(uint8_t soc_num, struct apml_message *msg);

/**
 *  @brief Writes data to TSI device file
 *
 *  @details This function will write data to TSI device file,
 *  through ioctl.
 *
 *  @param[in] soc_num  Socket index.
 *
 *  @param[in] msg struct apml_message which contains information about the protocol,
 *  input/output data etc.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t sbtsi_xfer_msg(uint8_t soc_num, struct apml_message *msg);

/**
 *  @brief Validates sbtsi module is present for the given socket
 *
 *  @details This function will validate sbtsi module is present
 *  for the specified socket.
 *
 *  @param[in] soc_num  Socket index.
 *
 *  @param[out] is_sbtsi returns true if the sbtsi is present else false
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t validate_sbtsi_module(uint8_t soc_num, bool *is_sbtsi);

/**
 *  @brief Validates sbrmi module is present for the given socket
 *
 *  @details This function will validate sbrmi module is present
 *  for the specified socket.
 *
 *  @param[in] soc_num  Socket index.
 *
 *  @param[out] is_sbrmi returns true if the sbrmi is present else false
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t validate_sbrmi_module(uint8_t soc_num, bool *is_sbrmi);

/**
 *  @brief Validates sbrmi and sbtsi modules are present for the given socket
 *
 *  @details This function will validate sbrmi and sbtsi modules are present
 *  for the specified socket.
 *
 *  @param[in] soc_num  Socket index.
 *
 *  @param[out] is_sbrmi returns true if the sbrmi is present else false
 *
 *  @param[out] is_sbtsi returns true if the sbtsi is present else false
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t validate_apml_dependency(uint8_t soc_num, bool *is_sbrmi,
				      bool *is_sbtsi);

#endif  // INCLUDE_APML_H_
