/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#ifndef INCLUDE_APML_ERR_H_
#define INCLUDE_APML_ERR_H_

/** \file apml_err.h
 *  Header file for the APML library error/return codes.
 *
 *  @details  This header file has error/return codes for the API.
 */

/* These error bases are used to identify FW returned error codes */
#define OOB_CPUID_MSR_ERR_BASE	0x800 //!< CPUID MSR FW error code
#define OOB_MAILBOX_ERR_BASE	0x900 //!< MAILBOX FW error code

/**
 * @brief Error codes retured by APML_ERR functions
 */
typedef enum {
	OOB_SUCCESS = 0,			//!< Operation was successful
	OOB_NOT_FOUND,				//!< An item was searched for but not found
	OOB_PERMISSION,				//!< Permission denied/EACCESS file error.
						//!< many functions require root access to run.
	OOB_NOT_SUPPORTED,			//!< The requested information or
						//!< action is not available for the
						//!< given input, on the given system
	OOB_FILE_ERROR,				//!< Problem accessing a file. This
						//!< may because the operation is not
						//!< supported by the Linux kernel
						//!< version running on the executing
						//!< machine
	OOB_INTERRUPTED,			//!< An interrupt occurred during
						//!< execution of function
	OOB_UNEXPECTED_SIZE,			//!< An unexpected amount of data
						//!< was read
	OOB_UNKNOWN_ERROR,			//!< An unknown error occurred
	OOB_ARG_PTR_NULL,			//!< Parsed argument ptr null
	OOB_NO_MEMORY,				//!< Not enough memory to allocate
	OOB_NOT_INITIALIZED,			//!< APML object not initialized
	OOB_TRY_AGAIN,				//!< No match Try again
	OOB_INVALID_INPUT,			//!< Input value is invalid
	OOB_CMD_TIMEOUT,			//!< Command timed out
	OOB_INVALID_MSGSIZE,			//!< Mesg size too long
	OOB_CPUID_MSR_ERR_START =
		OOB_CPUID_MSR_ERR_BASE,
	OOB_CPUID_MSR_CMD_TIMEOUT =
		OOB_CPUID_MSR_ERR_BASE + 0x11,	//!< RMI cmd timeout
	OOB_CPUID_MSR_CMD_WARM_RESET =
		OOB_CPUID_MSR_ERR_BASE + 0x22,	//!< Warm reset during RMI cmd
	OOB_CPUID_MSR_CMD_UNKNOWN_FMT =
		OOB_CPUID_MSR_ERR_BASE + 0x40,	//!< Cmd fmt field not recongnised
	OOB_CPUID_MSR_CMD_INVAL_RD_LEN =
		OOB_CPUID_MSR_ERR_BASE + 0x41,	//!< RMI cmd invalid read len
	OOB_CPUID_MSR_CMD_EXCESS_DATA_LEN =
		OOB_CPUID_MSR_ERR_BASE + 0x42,	//!< excess data
	OOB_CPUID_MSR_CMD_INVAL_THREAD =
		OOB_CPUID_MSR_ERR_BASE + 0x44,	//!< Invalid thread selected
	OOB_CPUID_MSR_CMD_UNSUPP =
		OOB_CPUID_MSR_ERR_BASE + 0x45,	//!< Cmd not supported
	OOB_CPUID_MSR_CMD_ABORTED =
		OOB_CPUID_MSR_ERR_BASE + 0x81,	//!< Cmd aborted
	OOB_CPUID_MSR_ERR_END =
		OOB_CPUID_MSR_ERR_BASE + 0xFF,
	OOB_MAILBOX_ERR_START =
		OOB_MAILBOX_ERR_BASE,
	OOB_MAILBOX_CMD_ABORTED =
		OOB_MAILBOX_ERR_BASE + 0x1,	//!< Mailbox cmd aborted
	OOB_MAILBOX_CMD_UNKNOWN =
		OOB_MAILBOX_ERR_BASE + 0x2,	//!< Unknown mailbox cmd
	OOB_MAILBOX_CMD_INVAL_CORE =
		OOB_MAILBOX_ERR_BASE + 0x3,	//!< Invalid core
	OOB_MAILBOX_ADD_ERR_DATA =
		OOB_MAILBOX_ERR_BASE + 0x5,	//!< Additional error data with error
	OOB_MAILBOX_INVALID_INPUT_ARGS =
		OOB_MAILBOX_ERR_BASE + 0x9,	//!< Invalid Input Arguments
	OOB_MAILBOX_INVALID_OOBRAS_CONFIG =
		OOB_MAILBOX_ERR_BASE +0xA,	//!< Invalid OOB RAS config
	OOB_MAILBOX_NOT_READY =
		OOB_MAILBOX_ERR_BASE + 0xB,	//!< Data not ready
	OOB_MAILBOX_ERR_END =
		OOB_MAILBOX_ERR_BASE + 0xFF,
} oob_status_t;

/*****************************************************************************/
/** @defgroup AuxilQuer Auxiliary functions
 *  Below functions provide interfaces to get the total number of cores,
 *  sockets and threads per core in the system.
 *  @{
 */

/**
 *  @brief convert linux error to esmi error.
 *
 *  @details Get the appropriate esmi error for linux error.
 *
 *  @param[in] err a linux error number
 *
 *  @retval oob_status_t is returned upon particular esmi error
 *
 */
oob_status_t errno_to_oob_status(int err);
/**
 * @brief Get the error string message for esmi oob errors.
 *
 *  @details Get the error message for the esmi oob error numbers
 *
 *  @param[in] oob_err is a esmi oob error number
 *
 *  @retval char* value returned upon successful call.
 */
char *esmi_get_err_msg(oob_status_t oob_err);

/** @} */  // end of AuxilQuer
/*****************************************************************************/

#endif  // INCLUDE_APML_ERR_H_
