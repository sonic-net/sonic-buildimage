/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>

#include <esmi_oob/apml.h>
#include <esmi_oob/apml_common.h>

#define SBRMI_CTRL	0x1
#define SBRMI_STATUS	0x2
#define SW_ALERT_MASK	0x2
/* SBRMI/SBTSI DEVICE FILE PATH */
#define DEV	"/dev/"
/* READ MODE */
#define READ_MODE		1
/*WRITE MODE */
#define WRITE_MODE		0
/* DEVICE FILE LENGTH */
#define DEV_SIZE		14

/* Static address inforamtion is from the PPR */
const uint16_t sbrmi_addr[MAX_DEV_COUNT] = {0x3c, 0x38, 0x3e, 0x3f,
					    0x34, 0x35, 0x36, 0x37};	//!< SBRMI Addresses
const uint16_t sbtsi_addr[MAX_DEV_COUNT] = {0x4c, 0x48, 0x4e, 0x4f,
					    0x44, 0x45, 0x46, 0x47};	//!< SBTSI Addresses

oob_status_t sbrmi_xfer_msg(uint8_t soc_num, struct apml_message *msg)
{
	int fd = 0, ret = 0;
	char dev_file[DEV_SIZE] = "";
	uint16_t soc_addr = 0;

	if (soc_num >= ARRAY_SIZE(sbrmi_addr))
		return OOB_FILE_ERROR;
	else
		soc_addr = sbrmi_addr[soc_num];

	sprintf(dev_file, "%s%s-%hx", DEV, SBRMI, soc_addr);
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		sprintf(dev_file, "%s%s%d", DEV, SBRMI, soc_num);
		fd = open(dev_file, O_RDWR);
		if (fd < 0)
			return OOB_FILE_ERROR;
	}

	if (ioctl(fd, SBRMI_IOCTL_CMD, msg) < 0)
		ret = errno;

	close(fd);

	if (ret == EPROTOTYPE) {
		if (msg->cmd == APML_CPUID || msg->cmd == APML_MCA_MSR)
			ret = OOB_CPUID_MSR_ERR_BASE + msg->fw_ret_code;
		else
			ret = OOB_MAILBOX_ERR_BASE + msg->fw_ret_code;
	}

	return errno_to_oob_status(ret);
}

oob_status_t sbtsi_xfer_msg(uint8_t soc_num, struct apml_message *msg)
{
	int fd = 0, ret = 0;
	char dev_file[DEV_SIZE] = "";
	uint16_t soc_addr = 0;

	if (soc_num >= ARRAY_SIZE(sbtsi_addr))
		return OOB_FILE_ERROR;
	else
		soc_addr = sbtsi_addr[soc_num];

	sprintf(dev_file, "%s%s-%hx", DEV, SBTSI, soc_addr);
	fd = open(dev_file, O_RDWR);
	if (fd < 0) {
		sprintf(dev_file, "%s%s%d", DEV, SBTSI, soc_num);
		fd = open(dev_file, O_RDWR);
		if (fd < 0)
			return OOB_FILE_ERROR;
	}

	if (ioctl(fd, SBRMI_IOCTL_CMD, msg) < 0)
		ret = errno;

	close(fd);

	return errno_to_oob_status(ret);
}

oob_status_t esmi_oob_rmi_read_byte(uint8_t soc_num, uint16_t reg_offset,
				    uint8_t *buffer)
{
	struct apml_message msg = {0};
	oob_status_t ret;

	/* NULL Pointer check */
	if (!buffer)
		return OOB_ARG_PTR_NULL;
	/* Readi/write register command is 0x1002 */
	msg.cmd = 0x1002;
	/* Assign register_offset to msg.data_in[0] */
	msg.data_in.mb_in[0] = reg_offset;
	/* Assign 1  to the msg.data_in[7] for the read operation */
	msg.data_in.reg_in[7] = 1;

	ret = sbrmi_xfer_msg(soc_num, &msg);
	if (!ret)
		*buffer = msg.data_out.reg_out[0];

	return ret;
}

oob_status_t esmi_oob_tsi_read_byte(uint8_t soc_num, uint8_t reg_offset,
				    uint8_t *buffer)
{
	struct apml_message msg = {0};
	oob_status_t ret;

	/* NULL Pointer check */
	if (!buffer)
		return OOB_ARG_PTR_NULL;
	/* Readi/write register command is 0x1002 */
	msg.cmd = 0x1002;
	/* Assign register_offset to msg.data_in[0] */
	msg.data_in.reg_in[0] = reg_offset;
	/* Assign 1  to the msg.data_in[7] for the read operation */
	msg.data_in.reg_in[7] = 1;

	ret = sbtsi_xfer_msg(soc_num, &msg);
	if (!ret)
		*buffer = msg.data_out.reg_out[0];

	return ret;
}

oob_status_t esmi_oob_rmi_write_byte(uint8_t soc_num, uint16_t reg_offset,
				     uint8_t value)
{
	struct apml_message msg = {0};

	/* Read/Write register command is 0x1002 */
	msg.cmd = 0x1002;
	/* Assign register_offset to msg.data_in[0] */
	msg.data_in.mb_in[0] = reg_offset;

	/* Assign value to write to the data_in[4] */
	msg.data_in.reg_in[4] = value;
	/* Assign 0 to the msg.data_in[7] */
	msg.data_in.reg_in[7] = 0;

	return sbrmi_xfer_msg(soc_num, &msg);
}

oob_status_t esmi_oob_tsi_write_byte(uint8_t soc_num, uint8_t reg_offset,
				     uint8_t value)
{
	struct apml_message msg = {0};

	/* Read/Write register command is 0x1002 */
	msg.cmd = 0x1002;
	/* Assign register_offset to msg.data_in[0] */
	msg.data_in.reg_in[0] = reg_offset;

	/* Assign value to write to the data_in[4] */
	msg.data_in.reg_in[4] = value;
	/* Assign 0 to the msg.data_in[7] */
	msg.data_in.reg_in[7] = 0;

	return sbtsi_xfer_msg(soc_num, &msg);
}

oob_status_t esmi_oob_read_byte(uint8_t soc_num,
				uint16_t reg_offset,
				char *file_name,
				uint8_t *buffer)
{
	/* NULL Pointer check */
	if (!buffer)
		return OOB_ARG_PTR_NULL;

	if (!strcmp(file_name, SBRMI))
		return esmi_oob_rmi_read_byte(soc_num, reg_offset, buffer);
	else if (!strcmp(file_name, SBTSI))
		return esmi_oob_tsi_read_byte(soc_num, reg_offset, buffer);
	else
		return OOB_FILE_ERROR;
}

oob_status_t esmi_oob_write_byte(uint8_t soc_num,
				 uint16_t reg_offset,
				 char *file_name,
				 uint8_t value)
{
	if (!strcmp(file_name, SBRMI))
		return esmi_oob_rmi_write_byte(soc_num,	reg_offset, value);
	else if (!strcmp(file_name, SBTSI))
		return esmi_oob_tsi_write_byte(soc_num,	reg_offset, value);
	else
		return OOB_FILE_ERROR;
}
/*
 * For a Mailbox write:
 *
 * Write 0x3f to an 0x80, Send the command to 0x38 and write the desired
 * value to 0x39 through 3C. The arguments of logical core numbers,
 * the % of DRAM throttle, NBIO Quadrant, CCD and CCX instances are all
 * written to 0x39.
 * The answer for our mailbox request is placed on registers 0x31-0x34.
 */
oob_status_t esmi_oob_write_mailbox(uint8_t soc_num,
                                    uint32_t cmd, uint32_t data)
{
	struct apml_message msg = {0};

	msg.cmd = cmd;
	msg.data_in.mb_in[0] = data;

	msg.data_in.mb_in[1] = (uint32_t)WRITE_MODE << 24;

	return sbrmi_xfer_msg(soc_num, &msg);
}

/*
 * The answer for our mailbox request is placed on registers 0x31-0x34
 */
oob_status_t esmi_oob_read_mailbox(uint8_t soc_num,
                                   uint32_t cmd, uint32_t input, uint32_t *buffer)
{
	struct apml_message msg = {0};
	oob_status_t ret = 0;

	/* NULL pointer check */
	if (!buffer)
		return OOB_ARG_PTR_NULL;

	msg.cmd = cmd;
	msg.data_in.mb_in[0] = input;

	msg.data_in.mb_in[1] = (uint32_t)READ_MODE << 24;
	ret = sbrmi_xfer_msg(soc_num, &msg);
	if (ret && ret != OOB_MAILBOX_ADD_ERR_DATA)
		return ret;

	*buffer = msg.data_out.mb_out[0];
	return ret;
}

oob_status_t validate_sbtsi_module(uint8_t soc_num, bool *is_sbtsi)
{
	char dev_file[DEV_SIZE] = "";

	*is_sbtsi = false;
	if (soc_num >= ARRAY_SIZE(sbtsi_addr))
		return OOB_FILE_ERROR;

	/* check if the sbtsi module is present for the given socket */
	sprintf(dev_file, "%s%s-%hx", DEV, SBTSI, sbtsi_addr[soc_num]);
        if (access(dev_file, F_OK) != 0) {
                sprintf(dev_file, "%s%s%d", DEV, SBTSI, soc_num);
		if (access(dev_file, F_OK) != 0)
			return OOB_FILE_ERROR;
        }

	*is_sbtsi = true;
	return OOB_SUCCESS;
}

oob_status_t validate_sbrmi_module(uint8_t soc_num, bool *is_sbrmi)
{
	char dev_file[DEV_SIZE] = "";

	*is_sbrmi = false;
	if (soc_num >= ARRAY_SIZE(sbrmi_addr))
		return OOB_FILE_ERROR;

	/* check if the sbrmi module is present for the given socket*/
	sprintf(dev_file, "%s%s-%hx", DEV, SBRMI, sbrmi_addr[soc_num]);
	if (access(dev_file, F_OK) != 0) {
		sprintf(dev_file, "%s%s%d", DEV, SBRMI, soc_num);
		if (access(dev_file, F_OK) != 0)
			return OOB_FILE_ERROR;
	}

	*is_sbrmi = true;
	return OOB_SUCCESS;
}

oob_status_t validate_apml_dependency(uint8_t soc_num, bool *is_sbrmi,
				      bool *is_sbtsi)
{
	char dev_file[DEV_SIZE] = "";
	uint16_t addr = 0;
	oob_status_t ret = OOB_SUCCESS;

	/* validate sbrmi module */
	ret = validate_sbrmi_module(soc_num, is_sbrmi);
	/* validate sbtsi module */
	ret = validate_sbtsi_module(soc_num, is_sbtsi);

	if (!*is_sbrmi || !*is_sbtsi)
		return OOB_FILE_ERROR;
	return ret;
}
