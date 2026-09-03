/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <esmi_oob/apml.h>
#include <esmi_oob/esmi_mailbox.h>
#include <esmi_oob/esmi_mailbox_nda.h>
#include "apml_nda_tool.h"

static int flag;

void show_module_nda_commands(char *exe_name, char *command, uint8_t p_type)
{
	if (!strcmp(command, "mailbox") || !strcmp(command, "1")) {
		if (p_type == FAM_1A_MOD_00 || p_type == FAM_1A_MOD_10) {
			printf("  --setbmcrasaction\t\t\t  [PAYLOAD(HEX)]"
			       "[OFFSET]\n\t\t\t\t\t  [REPAIRENTRYNUMBER]"
			       "[RASACTIONID][EOM]\t Set BMC RAS action\n"
			       "  --getbmcrasactionstatus\t\t"
			       "  [REPAIRENTRYNUMBER][RASACTIONID]\t"
			       " Get BMC RAS action status\n"
			       "  --setbmcrasmdcsduaction\t\t  [PAYLOAD(HEX)]"
			       "[OFFSET]\n\t\t\t\t\t  [PAYLOADSIZE]"
			       "[RASACTIONID][EOM]\t Set BMC RAS MDC SDU action\n"
			       "  --getbmcrasmdcsduactionstatus\t\t\t\t\t\t\t"
			       " Get BMC RAS MDC SDU action status\n"
			       "  --getpublicserialnumber\t\t  [CATEGORY(0,1)][CHIPLET_NUM(HEX)\t"
			       " Get Public Serial number of die.Valid category 0: CCD, 1: IOD \n"
			       );
		} else if (p_type == FAM_19_MOD_10 || p_type == FAM_19_MOD_A0) {
			printf("  --getpublicserialnumber\t\t  [CATEGORY(0,1)][CHIPLET_NUM(HEX)\t"
			       " Get Public Serial number of die.Valid category 0: CCD, 1: IOD \n");
		}
	}
}

static void apml_get_ras_action_status(uint8_t soc_num,
				       struct get_ras_action_data_in data_in)
{
	struct ras_action_status status = {0};
	oob_status_t ret;

	ret = get_bmc_ras_action_status(soc_num, data_in, &status);
	if (ret) {
		printf("Failed to get ras action status, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("---------------------------------------------\n");
	printf("| Repair result \t | %-16u |\n", status.repair_result);
	printf("| Repair entry num \t | %-16u |\n",
	       status.repair_entry_num);
	printf("| RAS action ID \t | %-16u |\n", status.ras_action_id);
	printf("---------------------------------------------\n");
}

static void apml_set_ras_action(uint8_t soc_num,
				struct set_ras_action_data_in data_in)
{
	uint32_t resp = 0;
	oob_status_t ret;

	ret = set_bmc_ras_action_status(soc_num, data_in, &resp);
	if (ret) {
		printf("Failed to set ras action, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("---------------------------------------------\n");
	printf("| RAS action response\t | %-16u |\n", resp);
	printf("---------------------------------------------\n");
}

static void apml_get_ras_mdc_sdu_action_status(uint8_t soc_num)
{
	struct get_ras_action_mdc_sdu status = {0};
	oob_status_t ret;

	ret = get_bmc_ras_action_mdc_sdu(soc_num, &status);
	if (ret) {
		if (ret == OOB_MAILBOX_ADD_ERR_DATA)
			printf("| MB error:0x%x additional error data | 0x%x|\n",
			       ret, status.add_err_data);
		else
			printf("Failed to get ras mdc sdu action, Err[%d]:%s\n",
			       ret, esmi_get_err_msg(ret));
		return;
	}

	if (status.utl_valid) {
		printf("---------------------------------------------\n");
		printf("| UNLOCK TIME\t\t | %-16u |\n", status.unlock_time);
		printf("| MDC SDU STATE\t\t | %-16u |\n", status.state);
		printf("| RAS ACTION ID\t\t | %-16u |\n", status.ras_action_id);
		printf("---------------------------------------------\n");
	} else {
		printf("---------------------------------------------\n");
		printf("| MDC SDU STATE\t\t | %-16u |\n", status.state);
		printf("| MDC SDU UTL VALID\t\t | %-16u |\n", status.utl_valid);
		printf("| RAS ACTION ID\t\t | %-16u |\n", status.ras_action_id);
		printf("---------------------------------------------\n");
	}
}

static void apml_set_ras_mdc_action_status(uint8_t soc_num,
					   struct set_ras_action_mdc_data_in d_in)
{
	uint32_t resp = 0;
	oob_status_t ret;

	ret = set_bmc_ras_action_mdc_sdu(soc_num, d_in, &resp);
	if (ret) {
		printf("Failed to set ras mdc sdu action, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
	}
	printf("-----------------------------------------------\n");
	printf("| RAS action response\t | 0x%-16x |\n", resp);
	printf("-----------------------------------------------\n");
}

static void apml_get_public_serial_number(uint8_t soc_num, struct serial_num_d_in d_in)
{
	uint64_t serial_num = 0;
	uint32_t buffer = 0;
	oob_status_t ret;

	/* read public serial number */
	ret = get_public_serial_number(soc_num, d_in, &serial_num);
	if (ret) {
		printf("Failed to read public serial number, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-----------------------------------------------\n");
	printf("| Public Serial Number\t | 0x%-16llx |\n", serial_num);
	printf("-----------------------------------------------\n");
}

/*
 * returns 0 if the given string is a number for the given base, else 1.
 * Base will be 16 for hexdecimal value and 10 for decimal value.
 */
static oob_status_t validate_number(char *str, uint8_t base)
{
	uint64_t buffer_number = 0;
	char *endptr;

	if (base == 10 || base == 0) {
		if (str[0] < '0' || str[0] > '9' )
			return OOB_INVALID_INPUT;
	}

	buffer_number = strtol(str, &endptr, base);
	if (*endptr != '\0')
		return OOB_INVALID_INPUT;

	return OOB_SUCCESS;
}

/*
 * Parse command line parameters and set data for program.
 * @param argc number of command line parameters
 * @param argv list of command line parameters
 */
oob_status_t parseesb_nda_args(int argc, char **argv, uint8_t soc_num)
{
	//Specifying the expected options
	static struct option long_options[] = {
		{"setbmcrasaction",	      required_argument,  &flag,  405},
		{"getbmcrasactionstatus",     required_argument,  &flag,  406},
		{"getbmcrasmdcsduactionstatus",	no_argument,	  &flag,  407},
		{"setbmcrasmdcsduactionstatus",	required_argument,&flag,  408},
		{"getpublicserialnumber",	required_argument,&flag,  409},
		{0,                     0,                      0,      0},
	};

	struct set_ras_action_data_in d_in = {0};
	struct set_ras_action_mdc_data_in mdc_d_in = {0};
	struct get_ras_action_data_in ras_din = {0};
	struct serial_num_d_in ser_num_d_in = {0};
	uint32_t val1;
	uint32_t val2;
	int opt = 0; /* option character */
	int long_index = 0;
	char *end;
        char *helperstring = "";
        oob_status_t ret = OOB_NOT_FOUND;

	optind = 2;
	opterr = 0;

	while ((opt = getopt_long(argc, argv, helperstring,
		long_options, &long_index)) != -1) {
		if (opt && optopt) {
			printf("Option '%s' require an argument\n", argv[ optind - 1]);
			return OOB_SUCCESS;
		}

		if (opt == 0 && (*long_options[long_index].flag == 405
		    || *long_options[long_index].flag == 408)) {
			if ((optind + 3) >= argc) {
				printf("Option '--%s' require FIVE"
				       " arguments\n",
				       long_options[long_index].name);
				return OOB_SUCCESS;
			}

			if (validate_number(argv[optind - 1], 0)) {
				printf("Option '--%s' require argument as"
				       "valid hex value\n\n",
				       long_options[long_index].name);
				return OOB_SUCCESS;
			}
			if (validate_number(argv[optind], 10)
			    || validate_number(argv[optind + 1], 10)
			    || validate_number(argv[optind + 2], 10)
			    || validate_number(argv[optind + 3], 10)) {
				printf("Option '--%s' require argument as"
				       " valid numeric value\n",
				       long_options[long_index].name);
				return OOB_SUCCESS;
			}
		}

		if (opt == 0 && (*long_options[long_index].flag == 406
		    || *long_options[long_index].flag == 409)) {
			if ((optind) >= argc) {
				printf("Option '--%s' require TWO"
				       " arguments\n",
				       long_options[long_index].name);
				return OOB_SUCCESS;
			}

			if (*long_options[long_index].flag == 406 &&
			    (validate_number(argv[optind - 1], 10)
			    || validate_number(argv[optind], 10))) {
				printf("Option '--%s' require argument as"
				       " valid numeric value\n",
				       long_options[long_index].name);
				return OOB_SUCCESS;
			}
			if (*long_options[long_index].flag == 409) {
				if (validate_number(argv[optind - 1], 10)) {
					printf("Option '--%s' require 1st argument"
					       " as valid numeric value\n",
					       long_options[long_index].name);
					return OOB_SUCCESS;
				}
				if (validate_number(argv[optind], 0)) {
					printf("Option '--%s' require 2nd argument as"
					       " valid hex value\n\n",
					       long_options[long_index].name);
					return OOB_SUCCESS;
				}
			}
		}

		switch (opt) {
		case 0:
			if (*(long_options[long_index].flag) == 405) {
				/* Set bmc ras action */
				/* pay load */
				d_in.payload.pay_load = strtol(argv[optind - 1], &end, 0);
				/* offset */
				d_in.payload.offset = atoi(argv[optind++]);
				/* repair entry number */
				d_in.payload.repair_entry_num = atoi(argv[optind++]);
				/* ras action ID */
				d_in.ras_act_id = atoi(argv[optind++]);
				/* EOM flag */
				d_in.eom_flag = atoi(argv[optind++]);
				apml_set_ras_action(soc_num, d_in);
			} else if (*(long_options[long_index].flag) == 406) {
				/* Get bmc ras action status */
				/* Repair entry number */
				ras_din.pay_load.repair_entry_num = atoi(argv[optind - 1]);
				/* ras action id */
				ras_din.ras_action_id = atoi(argv[optind++]);
				apml_get_ras_action_status(soc_num, ras_din);
			} else if (*(long_options[long_index].flag) == 407) {
				/* Get bmc ras mdc sdu status */
				apml_get_ras_mdc_sdu_action_status(soc_num);
			} else if (*(long_options[long_index].flag) == 408) {
				/* Set bmc ras mdc sdu status */
				/* pay load */
				mdc_d_in.payload.pay_load = strtol(argv[optind - 1], &end, 0);
				/* offset */
				mdc_d_in.payload.offset = atoi(argv[optind++]);
				/* payload size */
				mdc_d_in.payload.payload_size = atoi(argv[optind++]);
				/* ras action id */
				mdc_d_in.ras_act_id = atoi(argv[optind++]);
				/* EOM flag */
				mdc_d_in.eom_flag = atoi(argv[optind++]);

				apml_set_ras_mdc_action_status(soc_num, mdc_d_in);
			} else if (*(long_options[long_index].flag) == 409) {
				/* Get public serial number */
				/* category */
				ser_num_d_in.category = atoi(argv[optind - 1]);
				ser_num_d_in.chiplet_num = strtol(argv[optind++], &end, 0);
				apml_get_public_serial_number(soc_num, ser_num_d_in);
			}

			ret = OOB_SUCCESS;
			break;
		default:
			break;
		}
	}

	return ret;
}
