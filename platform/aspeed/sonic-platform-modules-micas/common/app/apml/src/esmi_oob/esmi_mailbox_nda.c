/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <esmi_oob/apml.h>
#include <esmi_oob/esmi_mailbox.h>
#include <esmi_oob/esmi_mailbox_nda.h>

#define REPAIR_ENTRY_NUM	21
#define RAS_ACTION_ID		27
#define EOM			31

oob_status_t set_bmc_ras_action_status(uint8_t soc_num,
				       struct set_ras_action_data_in data_in,
				       uint32_t *status)
{
	uint32_t input = 0;

	input = (uint32_t)data_in.eom_flag << EOM
		| (uint32_t)data_in.ras_act_id << RAS_ACTION_ID
		| (uint32_t)data_in.payload.repair_entry_num << REPAIR_ENTRY_NUM
		| (uint32_t)data_in.payload.offset << WORD_BITS
		| (uint32_t)data_in.payload.pay_load;

	return esmi_oob_read_mailbox(soc_num, SET_BMC_RAS_ACTION, input, status);
}

oob_status_t get_bmc_ras_action_status(uint8_t soc_num,
				       struct get_ras_action_data_in data_in,
				       struct ras_action_status *status)
{
	uint32_t input = 0, buffer = 0;
	oob_status_t ret;

	if (!status)
		return OOB_ARG_PTR_NULL;

	input = (uint32_t)data_in.ras_action_id << RAS_ACTION_ID
		| (uint32_t)data_in.pay_load.repair_entry_num << REPAIR_ENTRY_NUM;

	ret = esmi_oob_read_mailbox(soc_num, GET_BMC_RAS_ACTION_STATUS,
				    input, &buffer);
	if (!ret) {
		status->repair_result = buffer >> BYTE_BITS;
		status->repair_entry_num = buffer >> REPAIR_ENTRY_NUM;
		status->ras_action_id = buffer >> RAS_ACTION_ID;
	}

	return ret;
}

oob_status_t set_bmc_ras_action_mdc_sdu(uint8_t soc_num,
					struct set_ras_action_mdc_data_in data_in,
					uint32_t *status)
{
	uint32_t input = 0;

	input = (uint32_t)data_in.eom_flag << EOM
		| (uint32_t)data_in.ras_act_id << RAS_ACTION_ID
		| (uint32_t)data_in.payload.payload_size << REPAIR_ENTRY_NUM
		| (uint32_t)data_in.payload.offset << WORD_BITS
		| (uint32_t)data_in.payload.pay_load;

	return esmi_oob_read_mailbox(soc_num, SET_BMC_RAS_ACTION, input, status);
}

oob_status_t get_bmc_ras_action_mdc_sdu(uint8_t soc_num,
					struct get_ras_action_mdc_sdu *status)
{
	uint32_t input = 0, buffer = 0;
	oob_status_t ret;

	if (!status)
		return OOB_ARG_PTR_NULL;

	input = 1 << RAS_ACTION_ID;

	ret = esmi_oob_read_mailbox(soc_num, GET_BMC_RAS_ACTION_STATUS,
				    input, &buffer);
	if (!ret) {
		status->unlock_time = buffer;
		status->utl_valid = buffer >> 16;
		status->state = buffer >> 26;
		status->ras_action_id = buffer >> RAS_ACTION_ID;
	} else if (ret == OOB_MAILBOX_ADD_ERR_DATA) {
		/* Additional error data */
		status->add_err_data = buffer;
	}

	return ret;
}

oob_status_t get_public_serial_number(uint8_t soc_num,
				      struct serial_num_d_in  data_in,
				      uint64_t *serial_num)
{
	uint32_t input = 0, buffer = 0;
	uint8_t offset = 0;
	oob_status_t ret = 0;

	if (!serial_num)
		return OOB_ARG_PTR_NULL;

	*serial_num = 0;
	/* read LoDword of public serial number */
	input = (uint32_t)offset << WORD_BITS |
		(uint32_t)data_in.chiplet_num << BYTE_BITS |
		(uint32_t)data_in.category;

	ret = esmi_oob_read_mailbox(soc_num, GET_PUBLIC_SERIAL_NUMBER,
				    input, &buffer);
	if (!ret) {
		*serial_num = buffer;

		/* read HiDword of public serial number */
		input = input & 0x1FFFF;	// Setting the offset bit to 1
		buffer = 0;
		ret = esmi_oob_read_mailbox(soc_num, GET_PUBLIC_SERIAL_NUMBER,
					    input, &buffer);
		if (!ret)
			*serial_num |= ((uint64_t)buffer << D_WORD_BITS);
		else
			*serial_num = 0;
	}

	return ret;
}
