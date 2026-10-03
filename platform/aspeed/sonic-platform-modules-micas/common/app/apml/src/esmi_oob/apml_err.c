/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <esmi_oob/apml_err.h>
/*
 * Get appropriate error strings for the esmi oob error numbers
 */
char *esmi_get_err_msg(oob_status_t oob_err)
{
	switch (oob_err) {
	case OOB_SUCCESS:
		return "Success";
	case OOB_NOT_FOUND:
		return "APML device not found";
	case OOB_PERMISSION:
		return "Permission denied to access I2C dev";
	case OOB_NOT_SUPPORTED:
		return "Not supported";
	case OOB_FILE_ERROR:
		return "File error";
	case OOB_INTERRUPTED:
		return "Task Interrupted";
	case OOB_UNEXPECTED_SIZE:
		return "I/O Error";
	case OOB_ARG_PTR_NULL:
		return "Invalid pointer";
	case OOB_NO_MEMORY:
		return "Memory Error";
	case OOB_NOT_INITIALIZED:
		return "APML Library not initialized";
	case OOB_TRY_AGAIN:
		return "Does not match. Try Again";
	case OOB_INVALID_INPUT:
		return "Input value is invalid";
	case OOB_CMD_TIMEOUT:
		return "Command is timed out";
	case OOB_CPUID_MSR_ERR_START...OOB_CPUID_MSR_ERR_END:
		switch (oob_err) {
		case OOB_CPUID_MSR_CMD_TIMEOUT:
			return "Command did not complete before an SMBus timeout occurred";
		case OOB_CPUID_MSR_CMD_WARM_RESET:
			return "A warm reset occurred during the transaction.";
		case OOB_CPUID_MSR_CMD_UNKNOWN_FMT:
			return "The value in Command Format field is not recognized.";
		case OOB_CPUID_MSR_CMD_INVAL_RD_LEN:
			return "The value in RdDataLen is less than 1 or greater than 32.";
		case OOB_CPUID_MSR_CMD_EXCESS_DATA_LEN:
			return "The sum of the RdDataLen and WrDataLen is greater than 32 and"
				"RdDataLen is greater than or equal to 1 and less than or equal"
				"to 32.";
		case OOB_CPUID_MSR_CMD_INVAL_THREAD:
			return "Invalid thread selected";
		case OOB_CPUID_MSR_CMD_UNSUPP:
			return "Command not supported";
		case OOB_CPUID_MSR_CMD_ABORTED:
			return "Command Aborted";
		default:
			return "Unknown error in status";
		}
		break;
	case OOB_MAILBOX_ERR_START...OOB_MAILBOX_ERR_END:
		switch (oob_err) {
		case OOB_MAILBOX_CMD_ABORTED:
			return "Mailbox command aborted.";
		case OOB_MAILBOX_CMD_UNKNOWN:
			return "Unknown mailbox message";
		case OOB_MAILBOX_CMD_INVAL_CORE:
			return "Invalid core.";
		case OOB_MAILBOX_ADD_ERR_DATA:
			return "Additional error data.";
		case OOB_MAILBOX_INVALID_INPUT_ARGS:
			return "Mailbox message command failed due to invalid input arguments";
		case OOB_MAILBOX_INVALID_OOBRAS_CONFIG:
			return "Mailbox message command failed due to improper or invalid settings"
			       " of OOB Ras Config fields";
		case OOB_MAILBOX_NOT_READY:
			return "Data not ready";
		default:
			return "Unknown error in status";
		}
		break;
	default:
		return strerror(oob_err);
	}
}

/*
 * Map linux errors to esmi oob errors.
 */
oob_status_t errno_to_oob_status(int err)
{
	switch (err) {
	case 0:
		return OOB_SUCCESS;
	case ESRCH:
		return OOB_NOT_FOUND;
	case EACCES:
		return OOB_PERMISSION;
	case EPERM:
	case ENOENT:
		return OOB_NOT_SUPPORTED;
	case EBADF:
	case EOF:
	case EISDIR:
		return OOB_FILE_ERROR;
	case EINTR:
		return OOB_INTERRUPTED;
	case EREMOTEIO:
	case EIO:
		return OOB_UNEXPECTED_SIZE;
	case ENOMEM:
		return OOB_NO_MEMORY;
	case EAGAIN:
		return OOB_TRY_AGAIN;
	case EMSGSIZE:
		return OOB_INVALID_MSGSIZE;
	case OOB_CPUID_MSR_ERR_START...OOB_CPUID_MSR_ERR_END:
	case OOB_MAILBOX_ERR_START...OOB_MAILBOX_ERR_END:
		return err;
	default:
		return err;
	}
}

