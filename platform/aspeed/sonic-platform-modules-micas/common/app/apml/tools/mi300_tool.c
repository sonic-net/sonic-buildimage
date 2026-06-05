/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <esmi_oob/apml.h>
#include <esmi_oob/apml_common.h>
#include <esmi_oob/esmi_cpuid_msr.h>
#include <esmi_oob/esmi_tsi.h>
#include <esmi_oob/rmi_mailbox_mi300.h>
#include <esmi_oob/esmi_mailbox.h>
#include <esmi_oob/tsi_mi300.h>

static int flag;
#define APML_SLEEP 10000

oob_status_t get_hbm_temp_status(uint8_t soc_num)
{
	uint8_t reg_val;
	oob_status_t ret;

	ret = read_sbtsi_status(soc_num, &reg_val);
	if (ret)
		return ret;
	printf("\tMem Temp Alert  |\t\t|\t\t|");
	if (reg_val >> 6 & 1)
		printf(" HBM High Temp Alert\n");
	else if (reg_val >> 5 & 1)
		printf(" HBM Low Temp Alert\n");
	else if (!(reg_val >> 6 & 1) && !(reg_val >> 5 & 1))
		printf(" No HBM Temp Alert\n");
	else
		printf("HBM High and Low Temp Alert\n");

	return ret;
}

static void apml_get_hbm_throttle(uint8_t soc_num)
{
	uint32_t buffer;
	oob_status_t ret;

	ret = get_hbm_throttle(soc_num, &buffer);
	if (ret) {
		printf("Failed to get the HBM throttle, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("------------------------------------\n");
	printf("| HBM THROTTLE (%%)  | \t%-10u |\n", buffer);
	printf("------------------------------------\n");
}

static void apml_set_hbm_throttle(uint8_t soc_num, uint32_t mem_thr)
{
	uint32_t limit = 0;
	oob_status_t ret;

	ret = set_hbm_throttle(soc_num, mem_thr);
	if (ret) {
		printf("Failed: to set HBM throttle, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("Hbm throttle set Successfully\n");
	return;
}

static void apml_get_hbm_bandwidth(uint8_t soc_num)
{
	struct max_mem_bw bw;
	oob_status_t ret;

	ret = get_max_mem_bw_util(soc_num, &bw);
	if (ret) {
		printf("Failed: to get hbm bandwidth, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------------------------");
	printf("\n| HBM Max BW (GB/s)\t        | %-17d|", bw.max_bw);
	printf("\n| HBM Utilized BW (GB/s) \t| %-17d|\n", bw.utilized_bw);
	printf("---------------------------------------------------");
}

static void apml_set_gfx_core_clock(uint8_t soc_num, enum range_type type,
				    uint32_t freq)
{
	char *status;
	oob_status_t ret;

	status = type ? "Max" : "Min";
	ret = set_gfx_core_clock(soc_num, type, freq);
	if (ret != OOB_SUCCESS) {
		printf("Failed to set %s gfx core clock freq, "
		       "Err[%d]:%s\n", status, ret, esmi_get_err_msg(ret));
		return;
	}

	printf("%s GFX core clk freq set successfully\n", status);
}

static void apml_display_alarms_status(enum alarms_type type, uint32_t buffer)
{
	uint16_t alarms_len = 0;
	uint8_t index = 0, size = 0;
	char **alarm_status = {NULL};
	char *separator = ", ";
	char data[100] = " ";
	bool status = false;

	switch(type) {
	case PM:
		alarm_status = pm_alarm_status;
		size = ARRAY_SIZE(pm_alarm_status);
		break;
	default:
		break;
	}

	for (index = 0; index < size; index++) {
		switch(buffer & BIT(index)) {
		case 1:
		case 2:
		case 4:
		case 8:
			alarms_len = (status ? strlen(alarm_status[index]) + strlen(separator)
				      : strlen(alarm_status[index]));
			if ((strlen(data) + alarms_len) > 100)
				break;
			if (status)
				strncat(strncat(data, separator, strlen(separator)),
					alarm_status[index], alarms_len);
			else
				strncat(data, alarm_status[index], alarms_len);
			status = true;
			break;
		default:
			break;
		}
	}
	if (status)
		printf("%-51s", data);
	else
		printf("%-51s", " No Alarm");
}

static void apml_get_alarms(uint8_t soc_num, enum alarms_type type)
{
	uint32_t buffer;
	char *status;
	oob_status_t ret;

	ret = get_alarms(soc_num, type, &buffer);
	if (ret) {
		printf("Failed to read alarms status, "
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("------------------------------------------------------------"
	       "----------\n");
	printf("| %-3s Status \t: ", "PM");
	apml_display_alarms_status(type, buffer);
	printf("|\n------------------------------------------------------------"
	       "----------\n");
}

static void apml_get_psn(uint8_t soc_num, uint32_t die_index)
{
	uint64_t buffer;
	oob_status_t ret;

	ret = get_psn(soc_num, die_index, &buffer);
	if (ret) {
		printf("Failed to read PSN, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("----------------------------------------------\n");
	printf("| PSN \t| 0x%-32llx |\n", buffer);
	printf("----------------------------------------------\n");
}

static void apml_get_link_info(uint8_t soc_num)
{
	uint8_t link_config, mod_id;
	oob_status_t ret;

	ret = get_link_info(soc_num, &link_config, &mod_id);
	if (ret) {
		printf("Failed to read link info, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("--------------------------------------------\n");
	printf("| Link Config \t\t| %17u|\n", link_config);
	printf("| Module ID \t\t| %17u|\n", mod_id);
	printf("--------------------------------------------\n");
}

static void apml_die_hotspot_info(uint8_t soc_num)
{
	uint16_t temp;
	uint8_t die_id;
	oob_status_t ret;

	ret = get_die_hotspot_info(soc_num, &die_id, &temp);
	if (ret) {
		printf("Failed to read die hot spot info, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("--------------------------------------------\n");
	printf("| Die ID \t\t| %14u   |\n", die_id);
	printf("| Temperature\t\t| %14u °C|\n", temp);
	printf("--------------------------------------------\n");
}

static void apml_mem_hotspot_info(uint8_t soc_num)
{
	uint16_t temp;
	uint8_t stack_id;
	oob_status_t ret;

	ret = get_mem_hotspot_info(soc_num, &stack_id, &temp);
	if (ret) {
		printf("Failed to read mem hot spot info, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("--------------------------------------------\n");
	printf("| HBM ID \t\t| %14u   |\n", stack_id);
	printf("| Temperature\t\t| %14u °C|\n", temp);
	printf("--------------------------------------------\n");
}

static void apml_get_host_status(uint8_t soc_num)
{
	struct host_status status = {0};
	oob_status_t ret;

	ret = get_host_status(soc_num, &status);
	if (ret) {
		printf("Failed to read PM status, Err[%d]:"
		       "%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-----------------------------------------\n");
	printf("| PM Controller Status\t | %12s |\n",
	status.controller_status ? "Running" : "Not Running");
	printf("| Driver Status\t\t | %12s |\n",
	status.driver_status ? "Running" : "Not Running");
	printf("-----------------------------------------\n");
}

static void apml_get_max_min_gfx_freq(uint8_t soc_num)
{
	uint16_t max_freq = 0, min_freq = 0;
	oob_status_t ret = OOB_SUCCESS;

	ret = get_max_min_gfx_freq(soc_num, &max_freq, &min_freq);
	if (ret) {
		printf("Failed to read absolute max, min freq, Err[0x%x]:"
		       "%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------\n");
	printf("| %-25s | %16u|\n", "Max Freq (HHz)", max_freq);
	printf("| %-25s | %16u|\n", "Min Freq (HHz)", min_freq);
	printf("-----------------------------------------------\n");
}

static void apml_get_act_gfx_freq(uint8_t soc_num)
{
	uint16_t freq = 0;
	oob_status_t ret = OOB_SUCCESS;

	ret = get_act_gfx_freq_cap(soc_num, &freq);
	if (ret) {
		printf("Failed to actual gfx freq cap selected, Err[0x%x]:"
		       "%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------\n");
	printf("| %-25s | %16u|\n", "Act Freq (HHz)", freq);
	printf("-----------------------------------------------\n");
}

static void apml_get_clk_freq_limit(uint8_t soc_num, uint32_t clk_type)
{
	struct freq_limits limit = {0};
	char *clk_status;
	oob_status_t ret;

	clk_status = clk_type ? "F_CLK" : "GFX_CLK";
	ret = get_clk_freq_limits(soc_num, clk_type, &limit);
	if (ret) {
		printf("Failed to read %s freq limit,"
		       "Err[%d]:%s\n", clk_status, ret, esmi_get_err_msg(ret));
		return;
	}

	printf("--------------------------------------------\n");
	printf("| %7s Max Freq (MHZ) | %16u|\n", clk_status, limit.max);
	printf("| %7s Min Freq (MHz) | %16u|\n", clk_status, limit.min);
	printf("--------------------------------------------\n");
}

static void apml_get_hbm_stack_temp(uint8_t soc_num, uint32_t stack_index)
{
	uint16_t temp = 0;
	oob_status_t ret;

	ret = get_hbm_temperature(soc_num, stack_index, &temp);
	if (ret) {
		printf("Failed to read hbm temperature,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-------------------------------\n");
	printf("| Temp (°C) | %16u|\n", temp);
	printf("-------------------------------\n");
}

static void get_xgmi_link_width(uint8_t xgmi_ln_width, uint8_t *width)
{
	/* Get XGMI link width string */
	switch (xgmi_ln_width) {
	case 1:
	        *width = 2;
	        break;
	case 2:
	        *width = 4;
	        break;
	case 4:
	        *width = 8;
	        break;
	case 8:
	        *width = 16;
	        break;
	default:
		*width = 0;
	}
}

static void apml_get_xgmi_pstates(uint8_t soc_num, uint32_t pstate_ind)
{
	struct xgmi_speed_rate_n_width xgmi_pstate;
	uint8_t ln_width = 0;
	oob_status_t ret;


	ret = get_xgmi_pstates(soc_num, pstate_ind, &xgmi_pstate);
	if (ret) {
		printf("Failed to read XGMI pstates,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	get_xgmi_link_width(xgmi_pstate.link_width, &ln_width);

	printf("-----------------------------------------------"
	       "-------------\n");
	printf("| XGMI speed rate (GHz) | %-32u |\n", xgmi_pstate.speed_rate);
	printf("| XGMI Link Width  	| x%-31u |\n", ln_width);
	printf("-----------------------------------------------"
	       "-------------\n");
}

static void apml_set_xgmi_pstate(uint8_t soc_num, uint32_t pstate)
{
	oob_status_t ret;

	ret = set_xgmi_pstate(soc_num, pstate);
	if (ret) {
		printf("Failed to write XGMI pstate,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("XGMI pstate set successfully\n");
}

static void apml_unset_xgmi_pstate(uint8_t soc_num)
{
	oob_status_t ret;

	ret = unset_xgmi_pstate(soc_num);
	if (ret) {
		printf("Failed to unset the XGMI pstate,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("XGMI pstate unset successfully\n");
}

static void apml_get_mclk_fclk_pstates(uint8_t soc_num, uint32_t pstate_index)
{
	struct mclk_fclk_pstates pstate = {0};
	oob_status_t ret;

	ret = get_mclk_fclk_pstates(soc_num, pstate_index, &pstate);
	if (ret) {
		printf("Failed to get mem clk and fclk pstates,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("--------------------------------------------\n");
	printf("| Mem clk (MHz)\t\t | %16u|\n", pstate.mem_clk);
	printf("| Fclk (MHz)\t\t | %16u|\n", pstate.f_clk);
	printf("--------------------------------------------\n");
}

static void apml_set_max_mclk_fclk_pstate(uint8_t soc_num, uint32_t pstate)
{
	oob_status_t ret;

	ret = set_mclk_fclk_max_pstate(soc_num, pstate);
	if (ret) {
		printf("Failed to set max mem pstate,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("Max memory pstate set successfully\n");
}

static void apml_show_bist_results(uint8_t soc_num, uint32_t die_id)
{
	uint32_t bist_result = 0;
	oob_status_t ret;

	ret = get_bist_results(soc_num, die_id, &bist_result);
	if (ret) {
		printf("Failed to get bist result,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-----------------------------------\n");
	printf("| BIST RESULT\t  | \t%s |\n",
	       bist_result == 0 ? "BIST PASS" : "BIST FAIL");
	printf("-----------------------------------\n");
}

static void apml_get_svi_telemetry_by_rail(uint8_t soc_num,
					   struct svi_port_domain port)
{
	uint32_t pow = 0;
	oob_status_t ret;

	ret = get_svi_rail_telemetry(soc_num, port, &pow);
	if (ret) {
		printf("Failed to get SVI based telemetry for individual"
		       " rails,Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------------------\n");
	printf("| Power (W) \t\t| %-17.03f |\n", (float)pow / 1000);
	printf("---------------------------------------------\n");
}

static void apml_get_energy_accumulator_with_timestamp(uint8_t soc_num)
{
	uint64_t energy_acc = 0;
	uint64_t time_stamp = 0;
	oob_status_t ret;

	ret =  get_energy_accum_with_timestamp(soc_num, &energy_acc,
					       &time_stamp);
	if (ret) {
		printf("Failed to get the energy accumulator with time stamp,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-----------------------------------------------"
	"--------------\n");
	printf("| Energy Accumulator (MJ) | %-32.6f |\n",
	       (double)energy_acc / 1000000);
	printf("| %-23s | %-32.6f |\n", "Timestamp (s) ",
	       (double)time_stamp / 1000000000);
	printf("-----------------------------------------------"
	"--------------\n");
}

static void apml_get_xcc_idle_residency(uint8_t soc_num)
{
	uint32_t xcc_res = 0;
	oob_status_t ret;

	ret = get_xcc_idle_residency(soc_num, &xcc_res);
	if (ret) {
		printf("Failed to read XCC idle residency,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------"
	"---\n");
	printf("| XCC Res (%%) | %32u |\n", xcc_res);
	printf("-----------------------------------------------"
	"---\n");
}

static void apml_get_number_of_soc(uint8_t soc_num)
{
	uint32_t sockets = 0;
	oob_status_t ret;

	ret = get_sockets_in_system(soc_num, &sockets);
	if (ret) {
		printf("Failed to read number of sockets,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------"
	       "-----\n");
	printf("| Sockets Count  | %32u |\n", sockets);
	printf("-----------------------------------------------"
	       "-----\n");
}

static void apml_query_statistics(uint8_t soc_num, struct statistics stat)
{
	uint32_t param_value;
	char unit[BYTE_BITS] = "";
	oob_status_t ret;

	ret = get_statistics(soc_num, stat, &param_value);
	if (ret) {
		printf("Failed to query statistics for a given parameter,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	if(!stat.output_control)  {
		switch (stat.stat_param) {
		case 0:
			printf("Stop data Collection\n");
			return;
		case 1:
			strncpy(unit, "mW", NIBBLE_BITS);
			break;
		case 2:
			strncpy(unit,"Celsius", BYTE_BITS);
			break;
		case 4:
		case 6:
			strncpy(unit,"mA", NIBBLE_BITS);
			break;
		case 3:
		case 5:
		case 7:
		case 8:
			printf("Operation not supported\n");
			return;
		default:
			break;
		}
	}
	else {
		switch(stat.stat_param) {
		case 0:
		case 1:
		case 2:
		case 4:
		case 6:
			strncpy(unit, "ms", NIBBLE_BITS);
			break;
		case 3:
		case 5:
		case 7:
		case 8:
			printf("Operation not supported\n");
			return;
		default:
			break;
		}
	}
	printf("-----------------------------------------------"
	       "----------------\n");
	printf("| Parameter's Value  | %32u %-7s|\n", param_value, unit);
	printf("-----------------------------------------------"
	       "----------------\n");
}

static void apml_clear_statistics(uint8_t soc_num)
{
	oob_status_t ret;

	ret = clear_statistics(soc_num);
	if (ret) {
		printf("Failed to clear statistics,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------"
	       "---\n");
	printf("Clear all stored query statistics successful\n");
	printf("-----------------------------------------------"
	       "---\n");
}

static void apml_set_hbm_high_threshold_temp(uint8_t soc_num, float temp_th)
{
	oob_status_t ret;

	ret = write_sbtsi_hbm_hi_temp_th(soc_num, temp_th);
	if (ret) {
		printf("Failed to set hbm high temperature threshold,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("HBM high temperature threshold set successfully\n");
}

static void apml_set_hbm_low_threshold_temp(uint8_t soc_num, float temp_th)
{
	oob_status_t ret;

	ret = write_sbtsi_hbm_lo_temp_th(soc_num, temp_th);
	if (ret) {
		printf("Failed to set hbm low temperature threshold,"
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("HBM low temperature threshold set successfully\n");
}

static oob_status_t apml_set_hbm_alert_config(uint8_t soc_num, int value)
{
	oob_status_t ret;

	ret = set_sbtsi_hbm_alertconfig(soc_num, value);
	if (ret) {
		printf("Failed: to set hbm alert config, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}

	printf("Set HBM alert config success\n");
	return OOB_SUCCESS;
}

static oob_status_t apml_get_maximum_die_id(uint8_t soc_num)
{
	uint32_t max_die_id;
	oob_status_t ret;


	ret = get_die_type(soc_num, BIT_LEN, &max_die_id);
	if (ret) {
		printf("Failed: to get maximum die-id, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("--------------------------------------\n");
	printf("| Maximum Die-ID  | %-16u |\n", max_die_id & ONE_BYTE_MASK);
	printf("--------------------------------------\n");
	return OOB_SUCCESS;
}

static oob_status_t apml_get_die_type(uint8_t soc_num, uint32_t die_id)
{
	uint32_t buffer = 0;
	char *die_type;
	oob_status_t ret;

	die_id = die_id << 1;

	ret = get_die_type(soc_num, die_id, &buffer);
	if (ret) {
		printf("Failed: to get die-type, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}

	switch (buffer & ONE_BYTE_MASK) {
	case 0:
		die_type = "NOT DEFINED";
		break;
	case 1:
		die_type = "AID";
		break;
	case 2:
		die_type = "XCD";
		break;
	case 3:
		die_type = "CCD";
		break;
	case 4:
		die_type = "HBM STACK";
		break;
	default:
		die_type = "RESERVED";
		break;
	}
	printf("-----------------------------------------------------\n");
	printf("| Die Type\t\t\t | %-16s |\n", die_type);
	printf("| Max count of current die-type  | %-16u |\n",
	       (buffer >> BYTE_BITS) & ONE_BYTE_MASK);
	printf("| AID associated with Die-IDi\t | %-16u |\n",
	       (buffer >> WORD_BITS) & NIBBLE_MASK);
	printf("-----------------------------------------------------\n");
	return OOB_SUCCESS;
}

static void apml_get_curr_xgmi_pstate(uint8_t soc_num)
{
	uint8_t xgmi_pstate = 0;
	oob_status_t ret = OOB_SUCCESS;

	ret = get_curr_xgmi_pstate(soc_num, &xgmi_pstate);
	if (ret) {
		printf("Failed to get current xgmi pstate "
		       "Err[0x%x]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------------\n");
	printf("| Curr XGMI pstate\t\t | %-17u|\n", xgmi_pstate);
	printf("-----------------------------------------------------\n");
}

static void apml_get_max_operating_temp(uint8_t soc_num, uint32_t ctf_type)
{
	uint16_t temp = 0;
	oob_status_t ret = OOB_SUCCESS;

	ret = get_max_operating_temp(soc_num, ctf_type, &temp);
	if (ret) {
		printf("Failed to get max operating temperature "
		       "Err[0x%x]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-------------------------------------------------"
	       "-----------------------\n");
	printf("| Max Operating Temp ctf type[%u](ºC)\t\t | %-17u\t|\n",
	       ctf_type, temp);
	printf("-------------------------------------------------"
	       "-----------------------\n");
}

static void apml_get_slow_down_temp(uint8_t soc_num, uint32_t ctf_type)
{
	uint16_t temp = 0;
	oob_status_t ret = OOB_SUCCESS;

	ret = get_slow_down_temp(soc_num, ctf_type, &temp);
	if (ret) {
		printf("Failed to get slow down temperature "
		       "Err[0x%x]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------------"
	       "---------------------------\n");
	printf("| Slow Down Temperature ctf type[%u]%-8s\t\t | %-17u\t|\n",
	       ctf_type, "(ºC)", temp);
	printf("-----------------------------------------------------"
	       "---------------------------\n");
}

static void apml_get_hbm_dev_info(uint8_t soc_num)
{
	struct hbm_device_info dev_info = {0};
	oob_status_t ret = OOB_SUCCESS;

	ret = get_hbm_dev_info(soc_num, &dev_info);
	if (ret) {
		printf("Failed to get hbm device info "
		       "Err[0x%x]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------------\n");
	printf("|  Device Vendor\t\t | 0x%-15x|\n", dev_info.dev_vendor);
	printf("|  Part Number\t\t\t | 0x%-15x|\n", dev_info.part_num);
	printf("|  Total Memory (GBs)\t\t | 0x%-15x|\n", dev_info.total_mem);
	printf("-----------------------------------------------------\n");
}

static void apml_get_pcie_stats(uint8_t soc_num, uint32_t pcie_select)
{
	uint32_t pcie_stat = 0;
	char *stat;
	oob_status_t ret = OOB_SUCCESS;

	ret = get_pciestats(soc_num, pcie_select, &pcie_stat);
	if (ret) {
		printf("Failed to get PCI statistics "
		       "Err[0x%x]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------------\n");
	switch(pcie_select) {
	case 0:
		/* L0 to recovery count */
		stat = "L0 to Recovery Count";
		break;
	case 1:
		/* Replay count */
		stat = "Replay Count";
		break;
	case 2: /* Replay rollover count */
		stat = "Replay Rollover Count";
		break;
	case 3:
		/* NAK sent count */
		stat = "NAK sent Count";
		break;
	case 4:
		/* NAK received count */
		stat = "NAK Received Count";
		break;
	default:
		printf("| Invalid Input\t\t\t\t\t|\n");
		printf("--------------------------------------------------"
		       "---\n");
		return;
	}

	printf("|  %-29s | 0x%-15x|\n", stat, pcie_stat);
	printf("-----------------------------------------------------\n");
}

void get_mi_300_mailbox_cmds_summary(uint8_t soc_num)
{
	struct mclk_fclk_pstates pstate = {0};
	struct xgmi_speed_rate_n_width xgmi_pstate = {0};
	struct max_mem_bw bw = {0};
	struct freq_limits limit = {0};
	struct processor_info plat_info = {0};
	uint64_t energy_acc = 0, time_stamp = 0, psn = 0;
	uint32_t d_out = 0;
	uint16_t max_freq = 0, min_freq = 0, freq = 0, temp = 0;
	uint8_t pstate_index = 0, die_id = 0, link_config = 0, module_id = 0;
	uint8_t hbm_id = 0, ln_width = 0;
	oob_status_t ret;
	struct host_status h_status = {0};
	bool is_mi300c = false;

	ret = esmi_get_processor_info(soc_num, &plat_info);
	if (plat_info.family == 0x19 && (plat_info.model >= 0x80 && plat_info.model <= 0x8F))
		is_mi300c = true;

	printf("\n| MemClk/FClk_Pstate [0x%x] \t\t |", pstate_index);
	ret = get_mclk_fclk_pstates(soc_num, pstate_index, &pstate);
	if (ret) {
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	} else {
		printf("\n| \tMem_CLK  (MHz)\t\t\t | %-16u", pstate.mem_clk);
		printf("\n| \tF_CLK (MHz) \t\t\t | %-16u", pstate.f_clk);
	}

	printf("\n| XGMI power state Mappings [0x%x] \t |", pstate_index);
	ret = get_xgmi_pstates(soc_num, pstate_index, &xgmi_pstate);
	if (ret) {
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	} else {
		get_xgmi_link_width(xgmi_pstate.link_width, &ln_width);

		printf("\n| \tXGMI speed rate (GHz) \t\t | %-16u",
		       xgmi_pstate.speed_rate);
		printf("\n| \tXGMI link width\t\t\t | x%-16u",
		       ln_width);
	}
	if (!is_mi300c) {
		printf("\n| XCC IDLE RESIDENCY (%%)\t\t |");
		ret = get_xcc_idle_residency(soc_num, &d_out);
		if (ret)
			printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
		else
			printf(" %-17u", d_out);
	}
	printf("\n| Energy Accumulator \t\t\t |");
	ret = get_energy_accum_with_timestamp(soc_num, &energy_acc,
					      &time_stamp);
	if (ret) {
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	} else {
		printf("\n| \tEnergy Acc (MJ) \t\t | %-32.6f",
		       (double)energy_acc / 1000000);
		printf("\n| \tTime stamp (s) \t\t\t | %-32.6f",
		       (double)time_stamp / 1000000000);
	}

	printf("\n| PM Status \t\t\t\t |");
	ret = get_alarms(soc_num, PM, &d_out);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		apml_display_alarms_status(PM, d_out);

	printf("\n| PSN (0x%x)\t\t\t\t |", pstate_index);
	ret = get_psn(soc_num, pstate_index, &psn);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" 0x%-llx", psn);
	printf("\n| Link Info \t\t\t\t |");
	ret = get_link_info(soc_num, &link_config, &module_id);
	if (ret) {
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	} else {
		printf("\n| \tLink Config\t\t\t | %-16u", link_config);
		printf("\n| \tModule ID\t\t\t | %-16u", module_id);
	}

	if (!is_mi300c) {
		printf("\n| Abs Gfx Freq (MHz) \t\t\t |");
		ret = get_max_min_gfx_freq(soc_num, &max_freq, &min_freq);
		if (ret) {
			printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
		} else {
			printf("\n| \tMax Freq \t\t\t |");
			printf(" %-16u", max_freq);
			printf("\n| \tMin Freq \t\t\t |");
			printf(" %-16u", min_freq);
		}

		printf("\n| Act Max Gfx Freq (MHz) \t\t |");
		ret = get_act_gfx_freq_cap(soc_num, &freq);
		if (ret)
			printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
		else
			printf(" %-16u", freq);
	}

	printf("\n| Die Hot Spot Info \t\t\t |");
	ret = get_die_hotspot_info(soc_num, &die_id, &temp);
	if (ret) {
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	} else {
		printf("\n| \tDie ID\t\t\t\t | %-16u", die_id);
		printf("\n| \tTemperature (ºC)\t\t | %-16u", temp);
	}

	printf("\n| Mem Hot Spot Info \t\t\t |");
	ret = get_mem_hotspot_info(soc_num, &hbm_id, &temp);
	if (ret) {
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	} else {
		printf("\n| \tHBM ID\t\t\t\t | %-16u", hbm_id);
		printf("\n| \tTemperature (ºC)\t\t | %-16u", temp);
	}

	printf("\n| Host Status \t\t\t\t |");
	ret = get_host_status(soc_num, &h_status);
	if (ret) {
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	} else {
		printf("\n| \t PM Status\t\t\t | %-25s",
		       h_status.controller_status ? "Running" : "Not Running");
		printf("\n| \t Driver status\t\t\t | %-25s",
		       h_status.driver_status ? "Running" : "Not Running");
	}


	if (!is_mi300c) {
		printf("\n| Max Mem BW and utilization \t\t |");
		ret = get_max_mem_bw_util(soc_num, &bw);
		if (ret) {
			printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
		} else {
			printf("\n| \tHBM Max BW (GB/s)\t\t | %-17u", bw.max_bw);
			printf("\n| \tHBM Utilized BW (GB/s)\t\t | %-17u",
			       bw.utilized_bw);
		}
	}
	printf("\n| HBM Throttle (%%)\t\t\t |");
	ret = get_hbm_throttle(soc_num, &d_out);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17u", d_out);
	printf("\n| HBM Stack Temp [0x%x](ºC)\t\t |", pstate_index);
	ret = get_hbm_temperature(soc_num, pstate_index, &temp);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-16u", temp);
	if (!is_mi300c) {
		printf("\n| GFX CLK Freq Limit (MHz) \t\t |");
		ret = get_clk_freq_limits(soc_num, GFX_CLK, &limit);
		if (ret) {
			printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
		} else {
			printf("\n| \tMax Freq\t\t\t | %-16u", limit.max);
			printf("\n| \tMin Freq\t\t\t | %-16u", limit.min);
		}
	}
	printf("\n| F_CLK Freq Limit (MHz) \t\t |");
	ret = get_clk_freq_limits(soc_num, F_CLK, &limit);
	if (ret) {
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	} else {
		printf("\n| \tMax Freq\t\t\t | %-16u", limit.max);
		printf("\n| \tMin Freq\t\t\t | %-16u", limit.min);
	}
	printf("\n| Number of Sockets \t\t\t |");
	ret = get_sockets_in_system(soc_num, &d_out);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-16u", d_out);
}

void get_mi300_mailbox_commands(char *exe_name)
{
	printf("Usage: %s  [SOC_NUM] [Option]"
	       "\nOption:\n"
	       "\n< MAILBOX COMMANDS [params] >:\n"
	       "  --showmailboxsummary\t\t\t\t\t\t\t\t "
	       "Get summary of the mailbox commands\n"
	       "  -p, (--showpower)\t\t\t\t\t\t\t\t "
	       "Get Power for a given socket in Watts\n"
	       "  -t, (--showtdp)\t\t\t\t\t\t\t\t "
	       "Get TDP for a given socket in Watts\n"
	       "  -s, (--setpowerlimit)\t\t\t  [POWER]\t\t\t\t "
	       "Set powerlimit for a given socket in mWatts\n"
	       "  -b, (--showboostlimit)\t\t  [THREAD]\t\t\t\t "
	       "Get APML and BIOS boostlimit for a given core index "
	       "in MHz\n"
	       "  -d, (--setapmlboostlimit)\t\t  [THREAD]"
	       "[BOOSTLIMIT]\t\t\t Set APML boostlimit for a given "
	       "core in MHz\n"
	       "  -a, (--setapmlsocketboostlimit)\t  [BOOSTLIMIT]"
	       "\t\t\t\t Set APML boostlimit for all cores in a "
	       "socket in MHz\n"
	       "  --showPCIeconfigspacedata\t\t  [SEGMENT][OFFSET]\n"
	       "\t\t\t\t\t  [BUS(HEX)][DEVICE(HEX)][FUNC]\t\t Show "
	       "32 bit data from extended PCI config space\n"
	       "  --showvalidmcabanks\t\t\t\t\t\t\t\t "
	       "Show number of MCA banks & bytes/bank with valid "
	       "status after a fatal error\n"
	       "  --showrasmcamsr\t\t\t  [MCA_BANK_INDEX][OFFSET]"
	       "\t\t Show 32 bit data from specified MCA bank and "
	       "offset\n"
	       "  --showfchresetreason\t\t\t  [FCHID(0 or 1)]\t\t"
	       "\t Show previous reset reason from FCH register\n"
	       "  --showsktfreqlimit\t\t\t\t\t\t\t\t "
	       "Show per socket current active freq limit\n"
	       "  --showcclklimit\t\t\t  [THREAD]\t\t\t\t "
	       "Show core clock limit\n"
	       "  --showsvitelemetryallrails\t\t\t\t\t\t\t "
	       "Show svi based pwr telemetry for all rails\n"
	       "  --showsktfreqrange\t\t\t\t\t\t\t\t "
	       "Show per socket fmax fmin\n"
	       "  --showiobandwidth\t\t\t  "
	       "[LINKID(P2-P3,G0-G7)][BW(AGG_BW)]"
	       "\t Show IO bandwidth\n"
	       "  --showxGMIbandwidth\t\t\t  [LINKID(P2-P3,G0-G7)]\n"
	       "\t\t\t\t          [BW(AGG_BW,RD_BW,WR_BW)]"
	       "\t\t Show current xGMI bandwidth\n"
	       "  --showfclkmclkuclk\t\t\t  \t\t\t\t\t "
	       "Show df clock, memory clock and umc clock frequencies\n"
	       "  --setlclkdpmlevel\t\t\t  [NBIOID(0-3)][MAXDPM]"
	       "[MINDPM]\t\t Set dpm level range, valid dpm "
	       "values from 0 - 3, max value >= min value\n"
	       "  --showprocbasefreq\t\t\t  \t\t\t\t\t "
	       "Show processor base frequency\n"
	       "  --setpwrefficiencymode\t\t  [MODE(0,1,2)]\t\t\t\t "
	       "Set power efficiency profile policy\n"
	       "  --showraplcore\t\t\t  [THREAD]\t\t\t\t "
	       "Show runnng average power on specified core\n"
	       "  --showraplpkg\t\t\t\t  \t\t\t\t\t "
	       "Show running average power on pkg\n"
	       "  --showiodbist\t\t\t\t  \t\t\t\t\t "
	       "Show IOD/AID bist status\n"
	       "  --showccdbist\t\t\t\t  [CCDINSTANCE]\t\t\t\t "
	       "Show CCD/XCD bist status\n"
	       "  --showccxbist\t\t\t\t  [CCXINSTANCE]\t\t\t\t "
	       "Show CCX bist status\n"
	       "  --shownbioerrorloggingregister\t  "
	       "[AID_INDEX(HEX)][OFFSET(HEX)]\t\t Show nbio error "
	       "logging register\n"
	       "  --showprochotstatus\t\t\t  \t\t\t\t\t "
	       "Show prochot status\n"
	       "  --showprochotresidency\t\t  \t\t\t\t\t "
	       "Show prochot residency\n"
	       "  --showlclkdpmlevelrange\t\t  [NBIOID(0~3)]\t\t\t\t "
	       "Show LCLK DPM level range\n"
	       "  --showucoderevision\t\t\t  \t\t\t\t\t "
	       "Show micro code revision number\n"
	       "  --rasresetonsyncflood\t\t\t \t\t\t\t\t "
	       "Request warm reset after sync flood\n"
	       "  --getpostcode\t\t\t\t  [POST_CODE_OFFSET(0 - 7 or s"
	       "/summary)] Get post code for the given offset or"
	       " recent 8 offsets\n"
	       "  --showpowerconsumed\t\t\t  \t\t\t\t\t "
	       "Show consumed power\n"
	       "  --showppinfuse\t\t\t\t\t\t\t\t Show 64bit PPIN"
	       " fuse data\n"
	       "  --showrasdferrvaliditycheck\t\t  [DF_BLOCK_ID]\t\t\t\t "
	       "Show RAS DF error validity check for a given blockID\n"
	       "  --showrasdferrdump\t\t\t  [OFFSET][BLK_ID][BLK_INST]\t\t "
	       "Show RAS DF error dump\n"
	       "  --showcclkfreqlimit\t\t\t\t\t\t\t\t Get "
	       "cclk freqlimit for a given socket in MHz\n"
	       "  --showc0residency\t\t\t\t\t\t\t\t Show "
	       "c0_residency for a given socket\n"
	       "  --showhbmbandwidth\t\t\t\t\t\t\t\t Show "
	       "max, utilized HBM Bandwidth of the system\n"
	       "  --sethbmthrottle\t\t\t  [0 to 80%%]"
	       "\t\t\t\t Set HBM Throttle\n"
	       "  --showhbmthrottle\t\t\t  \t\t\t\t\t "
	       "Show hbm throttle value\n"
	       "  --setmaxgfxcoreclock\t\t\t  [FREQ]\t\t\t\t "
	       "Set max gfx core clock frequency in MHZ\n"
	       "  --setmingfxcoreclock\t\t\t  [FREQ]\t\t\t\t "
	       "Set min gfx core clock frequency in MHZ\n"
	       "  --showpmstatus\t\t\t  \t\t\t\t\t "
	       "Show PM status\n"
	       "  --showpsn\t\t\t\t  [CORE/DIE_INDEX]\t\t\t "
	       "Show 64 bit PSN\n"
	       "  --showlinkinfo\t\t\t  \t\t\t\t\t "
	       "Show module id and link config reflecting"
	       " strapping pins\n"
	       "  --showdiehotspotinfo\t\t\t  \t\t\t\t\t "
	       "Show die hot spot info\n"
	       "  --showmemhotspotinfo\t\t\t  \t\t\t\t\t "
	       "Show memory hot spot info\n"
	       "  --showhoststatus\t\t\t  \t\t\t\t\t "
	       "Show power management controller and driver running status\n"
	       "  --showabsmaxmingfxfreq\t\t  \t\t\t\t\t "
	       "Show abs max and min gfx frequency in MHz\n"
	       "  --showactfreqcapselected\t\t  \t\t\t\t\t "
	       "Show actual freq cap selected in MHz\n"
	       "  --showgfxclkfreqlimit\t\t  \t\t\t\t\t\t "
	       "Show gfx clock freq limit in MHz\n"
	       "  --showflckfreqlimit\t\t\t  \t\t\t\t\t "
	       "Show fclk freq limit in MHz\n"
	       "  --showhbmstacktemp\t\t\t  [INDEX(0-7)]\t\t\t\t "
	       "Show hbm stack temperature in °C\n"
	       "  --showxgmipstates\t\t\t  [PSTATE_INDEX]\t\t\t "
	       "Show XGMI power state mappings\n"
	       "  --setxgmipstate\t\t\t  [PSTATE]\t\t\t\t "
	       "Set XGMI pstate.Valid values are 0 - 1\n"
	       "  --unsetxgmipstate\t\t\t  \t\t\t\t\t "
	       "Unset XGMI pstate\n"
	       "  --showpstates\t\t\t\t  [PSTATE_INDEX]\t\t\t "
               "Show memclk and fclk frequency\n"
	       "  --setmaxpstate\t\t\t  [PSTATE]\t\t\t\t "
	       "Set max memory and fabric clock pstate\n"
	       "  --showbistresult\t\t\t  [DIE_ID]\t\t\t\t "
	       "Show die level bist result from package\n"
	       "  --showsvibasedtelemetryforindvrails\t  [PORT]"
	       "[SLAVE_ADDR]\t\t\t Show svi based telemetry for "
	       "individual rails\n"
	       "  --showenergyacctimestamp\t\t\t  \t\t\t\t "
	       "Show energy accumulator with time stamp\n"
	       "  --showxccidleres\t\t\t  \t\t\t\t\t "
	       "Show socket GFX  idle residency\n"
	       "  --shownumberofsockets\t\t\t  \t\t\t\t\t "
	       "Show number of sockets in system\n"
	       "  --querystatistics\t\t\t  [STAT_PARAM][OUTPUT_CONTROL]"
	       "\t\t Query statistics for a given parameter\n"
	       "  --clearstatistics\t\t\t  \t\t\t\t\t "
	       "clear statistics\n"
	       "  --showmaxdieid\t\t\t  \t\t\t\t\t "
	       "Show maximum die-id\n"
	       "  --showdietype\t\t\t\t  [die_id]\t\t\t\t"
	       " Show dietype. Valid die_id is 0 - (maxdie-id - 1)\n"
	       "  --getcurrxgmipstate\t\t\t  \t\t\t\t\t "
	       "Get current xgmi pstate\n"
	       "  --getoperatingtemp\t\t\t  [CTF_TYPE(0 - 3)]\t\t\t "
	       "Get max operating temperature(°C)\n"
	       "  --getslowdowntemp\t\t\t  [CTF_TYPE(0 - 3)]\t\t\t "
	       "Get slow down temperature(°C)\n"
	       "  --gethbmdeviceinfo\t\t\t  \t\t\t\t\t "
	       "Get HBM device vendor, part number and memory size(GBs)\n"
	       "  --getpciestats\t\t\t  [STAT_SELECTOR]\t\t\t "
	       "Get PCIe statistics including L0 to recovery count, replay count,"
	       "NAK sent count, NAK received count\n", exe_name);
}

void get_mi300c_mailbox_commands(char *exe_name)
{
	printf("Usage: %s  [SOC_NUM] [Option]"
	       "\nOption:\n"
	       "\n< MAILBOX COMMANDS [params] >:\n"
	       "  --showmailboxsummary\t\t\t\t\t\t\t\t "
	       "Get summary of the mailbox commands\n"
	       "  -p, (--showpower)\t\t\t\t\t\t\t\t "
	       "Get Power for a given socket in Watts\n"
	       "  -t, (--showtdp)\t\t\t\t\t\t\t\t "
	       "Get TDP for a given socket in Watts\n"
	       "  -s, (--setpowerlimit)\t\t\t  [POWER]\t\t\t\t "
	       "Set powerlimit for a given socket in mWatts\n"
	       "  -b, (--showboostlimit)\t\t  [THREAD]\t\t\t\t "
	       "Get APML and BIOS boostlimit for a given core index "
	       "in MHz\n"
	       "  -d, (--setapmlboostlimit)\t\t  [THREAD]"
	       "[BOOSTLIMIT]\t\t\t Set APML boostlimit for a given "
	       "core in MHz\n"
	       "  -a, (--setapmlsocketboostlimit)\t  [BOOSTLIMIT]"
	       "\t\t\t\t Set APML boostlimit for all cores in a "
	       "socket in MHz\n"
	       "  --showPCIeconfigspacedata\t\t  [SEGMENT][OFFSET]\n"
	       "\t\t\t\t\t  [BUS(HEX)][DEVICE(HEX)][FUNC]\t\t Show "
	       "32 bit data from extended PCI config space\n"
	       "  --showvalidmcabanks\t\t\t\t\t\t\t\t "
	       "Show number of MCA banks & bytes/bank with valid "
	       "status after a fatal error\n"
	       "  --showrasmcamsr\t\t\t  [MCA_BANK_INDEX][OFFSET]"
	       "\t\t Show 32 bit data from specified MCA bank and "
	       "offset\n"
	       "  --showfchresetreason\t\t\t  [FCHID(0 or 1)]\t\t"
	       "\t Show previous reset reason from FCH register\n"
	       "  --showsktfreqlimit\t\t\t\t\t\t\t\t "
	       "Show per socket current active freq limit\n"
	       "  --showcclklimit\t\t\t  [THREAD]\t\t\t\t "
	       "Show core clock limit\n"
	       "  --showsvitelemetryallrails\t\t\t\t\t\t\t "
	       "Show svi based pwr telemetry for all rails\n"
	       "  --showsktfreqrange\t\t\t\t\t\t\t\t "
	       "Show per socket fmax fmin\n"
	       "  --showiobandwidth\t\t\t  "
	       "[LINKID(P2-P3,G0-G7)][BW(AGG_BW)]"
	       "\t Show IO bandwidth\n"
	       "  --showxGMIbandwidth\t\t\t  [LINKID(P2-P3,G0-G7)]\n"
	       "\t\t\t\t          [BW(AGG_BW,RD_BW,WR_BW)]"
	       "\t\t Show current xGMI bandwidth\n"
	       "  --showfclkmclkuclk\t\t\t  \t\t\t\t\t "
	       "Show df clock, memory clock and umc clock frequencies\n"
	       "  --setlclkdpmlevel\t\t\t  [NBIOID(0-3)][MAXDPM]"
	       "[MINDPM]\t\t Set dpm level range, valid dpm "
	       "values from 0 - 3, max value >= min value\n"
	       "  --showprocbasefreq\t\t\t  \t\t\t\t\t "
	       "Show processor base frequency\n"
	       "  --setpwrefficiencymode\t\t  [MODE(0,1,2)]\t\t\t\t "
	       "Set power efficiency profile policy\n"
	       "  --showraplcore\t\t\t  [THREAD]\t\t\t\t "
	       "Show runnng average power on specified core\n"
	       "  --showraplpkg\t\t\t\t  \t\t\t\t\t "
	       "Show running average power on pkg\n"
	       "  --showiodbist\t\t\t\t  \t\t\t\t\t "
	       "Show IOD/AID bist status\n"
	       "  --showccdbist\t\t\t\t  [CCDINSTANCE]\t\t\t\t "
	       "Show CCD/XCD bist status\n"
	       "  --showccxbist\t\t\t\t  [CCXINSTANCE]\t\t\t\t "
	       "Show CCX bist status\n"
	       "  --shownbioerrorloggingregister\t  "
	       "[AID_INDEX(HEX)][OFFSET(HEX)]\t\t Show nbio error "
	       "logging register\n"
	       "  --showprochotstatus\t\t\t  \t\t\t\t\t "
	       "Show prochot status\n"
	       "  --showprochotresidency\t\t  \t\t\t\t\t "
	       "Show prochot residency\n"
	       "  --showlclkdpmlevelrange\t\t  [NBIOID(0~3)]\t\t\t\t "
	       "Show LCLK DPM level range\n"
	       "  --showucoderevision\t\t\t  \t\t\t\t\t "
	       "Show micro code revision number\n"
	       "  --rasresetonsyncflood\t\t\t \t\t\t\t\t "
	       "Request warm reset after sync flood\n"
	       "  --getpostcode\t\t\t\t  [POST_CODE_OFFSET(0 - 7 or s"
	       "/summary)] Get post code for the given offset or"
	       " recent 8 offsets\n"
	       "  --showpowerconsumed\t\t\t  \t\t\t\t\t "
	       "Show consumed power\n"
	       "  --showppinfuse\t\t\t\t\t\t\t\t Show 64bit PPIN"
	       " fuse data\n"
	       "  --showrasdferrvaliditycheck\t\t  [DF_BLOCK_ID]\t\t\t\t "
	       "Show RAS DF error validity check for a given blockID\n"
	       "  --showrasdferrdump\t\t\t  [OFFSET][BLK_ID][BLK_INST]\t\t "
	       "Show RAS DF error dump\n"
	       "  --showrasrterrvalidityck\t\t  [ERR_CATERGORY(0-2)][REQ_TYPE(0-1)]\t "
	       "BMC RAS runtime error validity check\n"
	       "  --showrasrterrinfo\t\t\t  [OFFSET][CATEGORY][VALID_INST]\t "
	       "BMC RAS runtime error Info\n"
	       "  --showcclkfreqlimit\t\t\t\t\t\t\t\t Get "
	       "cclk freqlimit for a given socket in MHz\n"
	       "  --showc0residency\t\t\t\t\t\t\t\t Show "
	       "c0_residency for a given socket\n"
	       "  --sethbmthrottle\t\t\t  [0 to 80%%]"
	       "\t\t\t\t Set HBM Throttle\n"
	       "  --showhbmthrottle\t\t\t  \t\t\t\t\t "
	       "Show hbm throttle value\n"
	       "  --showpmstatus\t\t\t  \t\t\t\t\t "
	       "Show PM status\n"
	       "  --showpsn\t\t\t\t  [CORE/DIE_INDEX]\t\t\t "
	       "Show 64 bit PSN\n"
	       "  --showlinkinfo\t\t\t  \t\t\t\t\t "
	       "Show module id and link config reflecting"
	       " strapping pins\n"
	       "  --showdiehotspotinfo\t\t\t  \t\t\t\t\t "
	       "Show die hot spot info\n"
	       "  --showmemhotspotinfo\t\t\t  \t\t\t\t\t "
	       "Show memory hot spot info\n"
	       "  --showhoststatus\t\t\t  \t\t\t\t\t "
	       "Show power management controller and driver running status\n"
	       "  --showflckfreqlimit\t\t\t  \t\t\t\t\t "
	       "Show fclk freq limit in MHz\n"
	       "  --showhbmstacktemp\t\t\t  [INDEX(0-7)]\t\t\t\t "
	       "Show hbm stack temperature in °C\n"
	       "  --showxgmipstates\t\t\t  [PSTATE_INDEX]\t\t\t "
	       "Show XGMI power state mappings\n"
	       "  --setxgmipstate\t\t\t  [PSTATE]\t\t\t\t "
	       "Set XGMI pstate.Valid values are 0 - 1\n"
	       "  --unsetxgmipstate\t\t\t  \t\t\t\t\t "
	       "Unset XGMI pstate\n"
	       "  --showpstates\t\t\t\t  [PSTATE_INDEX]\t\t\t "
               "Show memclk and fclk frequency\n"
	       "  --setmaxpstate\t\t\t  [PSTATE]\t\t\t\t "
	       "Set max memory and fabric clock pstate\n"
	       "  --showbistresult\t\t\t  [DIE_ID]\t\t\t\t "
	       "Show die level bist result from package\n"
	       "  --showsvibasedtelemetryforindvrails\t  [PORT]"
	       "[SLAVE_ADDR]\t\t\t Show svi based telemetry for "
	       "individual rails\n"
	       "  --showenergyacctimestamp\t\t\t  \t\t\t\t "
	       "Show energy accumulator with time stamp\n"
	       "  --shownumberofsockets\t\t\t  \t\t\t\t\t "
	       "Show number of sockets in system\n"
	       "  --querystatistics\t\t\t  [STAT_PARAM][OUTPUT_CONTROL]"
	       "\t\t Query statistics for a given parameter\n"
	       "  --clearstatistics\t\t\t  \t\t\t\t\t "
	       "clear statistics\n"
	       "  --showmaxdieid\t\t\t  \t\t\t\t\t "
	       "Show maximum die-id\n"
	       "  --showdietype\t\t\t\t  [die_id]\t\t\t\t"
	       " Show dietype. Valid die_id is 0 - (maxdie-id - 1)\n"
	       "  --getcurrxgmipstate\t\t\t  \t\t\t\t\t "
	       "Get current xgmi pstate\n"
	       "  --getoperatingtemp\t\t\t  [CTF_TYPE(0 - 3)]\t\t\t "
	       "Get max operating temperature(°C)\n"
	       "  --getslowdowntemp\t\t\t  [CTF_TYPE(0 - 3)]\t\t\t "
	       "Get slow down temperature(°C)\n"
	       "  --gethbmdeviceinfo\t\t\t  \t\t\t\t\t "
	       "Get HBM device vendor, part number and memory size(GBs)\n"
	       "  --getpciestats\t\t\t  [STAT_SELECTOR]\t\t\t "
	       "Get PCIe statistics including L0 to recovery count, replay count,"
	       "NAK sent count, NAK received count\n", exe_name);
}

void get_mi300_tsi_commands(char *exec_name)
{
	printf("Usage: %s [SOC_NUM] [Option]"
	       "\nOption:\n"
	       "\n< SB-TSI COMMANDS [params] >:\n"
	       "  --showtsiregisters\t\t\t  \t\t\t\t\t Get "
	       "values of SB-TSI reg commands for a given socket\n"
	       "  --set_verify_updaterate\t       [UPDATERATE]"
	       "\t\t\t\t Set APML Freq Update rate."
	       "Valid values are 2^i, i=[-4,6]\n"
	       "  --sethightempthreshold\t       [TEMP(°C)]\t\t"
	       "\t\t Set APML High Temp Threshold\n"
	       "  --sethbmhightempthreshold\t       [TEMP(°C)]\t\t"
	       "\t\t Set HBM high Temp Threshold\n"
	       "  --sethbmlowtempthreshold\t       [TEMP(°C)]\t\t"
	       "\t\t Set HBM low Temp Threshold\n"
	       "  --setlowtempthreshold\t\t       [TEMP(°C)]\t\t"
	       "\t\t Set APML Low Temp Threshold\n"
	       "  --settempoffset\t\t       [VALUE]\t\t\t\t\t Set "
	       "APML processor Temp Offset, VALUE = [-CPU_TEMP(°C), 127 "
	       "°C]\n"
	       "  --setalertthreshold\t\t       [VALUE]\t\t\t\t\t "
	       "Set APML processor alert threshold sample, VALUE = 1 to 8\n"
	       "  --sethbmalertthreshold\t       [VALUE]\t\t\t\t\t "
	       "Set APML HBM alert threshold sample, VALUE = 1 to 8\n"
	       "  --setalertconfig\t\t       [VALUE]\t\t\t\t\t "
	       "Set/Reset APML processor alert config, VALUE = 0 or 1\n"
	       "  --sethbmalertconfig\t\t       [VALUE]\t\t\t\t\t "
	       "Set/Reset APML HBM alert config, VALUE = 0 or 1\n"
	       "  --setalertmask\t\t       [VALUE]\t\t\t\t\t "
	       "Set/Reset APML processor alert mask, VALUE = 0 or 1\n"
	       "  --setrunstop\t\t\t       [VALUE]\t\t\t\t\t "
	       "Set/Reset APML processor runstop, VALUE = 0 or 1\n"
	       "  --setreadorder\t\t       [VALUE]\t\t\t\t\t "
	       "Set/Reset APML processor read order, VALUE = 0 or 1\n", exec_name);
}

oob_status_t get_apml_mi300_tsi_register_descriptions(uint8_t soc_num) {
	float temp_value = 0;
	oob_status_t ret;

	ret = read_sbtsi_hbm_hi_temp_th(soc_num, &temp_value);
	if (ret)
		return ret;

	printf("_HBM_HIGH_THRESHOLD_TEMP|\t\t|\t\t| %.3f °C\n",
	       temp_value);
	printf("\tHIGH_INT  \t| 0x%x \t\t| 0x%-5x\t| %u °C\n",
	       SBTSI_HBM_HITEMPINT_LIMIT,
	       (int)(temp_value), (int)(temp_value));
	printf("\tHIGH_DEC \t| 0x%x \t\t| 0x%-5x\t| %.3f °C\n",
	       SBTSI_HBM_HITEMPDEC_LIMIT,
	       (uint8_t)((temp_value - floor(temp_value)) / TEMP_INC),
	       temp_value - floor(temp_value));

	temp_value = 0;
	ret = read_sbtsi_hbm_lo_temp_th(soc_num, &temp_value);
	if (ret)
		return ret;

	printf("_HBM_LOW_THRESHOLD_TEMP |\t\t|\t\t| %.3f °C\n", temp_value);
	printf("\tLOW_INT  \t| 0x%x \t\t| 0x%-5x\t| %u °C\n",
	       SBTSI_HBM_LOTEMPINT_LIMIT, (int)(temp_value),
	       (int)(temp_value));
	printf("\tLOW_DEC \t| 0x%x \t\t| 0x%-5x\t| %.3f °C\n",
	       SBTSI_HBM_LOTEMPDEC_LIMIT,
	       (uint8_t)((temp_value - floor(temp_value)) / TEMP_INC),
	       temp_value - floor(temp_value));
	usleep(APML_SLEEP);

	temp_value = 0;
	ret = read_sbtsi_max_hbm_temp(soc_num, &temp_value);
	if (ret)
		return ret;

	printf("_HBM_MAX_TEMP \t\t|\t\t|\t\t| %.3f °C\n", temp_value);
	printf("\tMAX_INT  \t| 0x%x \t\t| 0x%-5x\t| %u °C\n",
	       SBTSI_MAX_HBMTEMPINT, (int)(temp_value),
	       (int)(temp_value));
	printf("\tMAX_DEC \t| 0x%x \t\t| 0x%-5x\t| %.3f °C\n",
	       SBTSI_MAX_HBMTEMPDEC,
	       (uint8_t)((temp_value - floor(temp_value)) / TEMP_INC),
	       temp_value - floor(temp_value));
	usleep(APML_SLEEP);

	temp_value = 0;
	ret = read_sbtsi_hbm_temp(soc_num, &temp_value);
	if (ret)
		return ret;

	printf("_HBM_TEMP \t\t|\t\t|\t\t| %.3f °C\n", temp_value);
	printf("\tHBM_INT  \t| 0x%x \t\t| 0x%-5x\t| %u °C\n",
	       SBTSI_HBMTEMPINT, (int)(temp_value),
	       (int)(temp_value));
	printf("\tHBM_DEC \t| 0x%x \t\t| 0x%-5x\t| %.3f °C\n",
	       SBTSI_HBMTEMPDEC,
	       (uint8_t)((temp_value - floor(temp_value)) / TEMP_INC),
	       temp_value - floor(temp_value));
	usleep(APML_SLEEP);

	return ret;
}

static oob_status_t apml_set_hbm_alert_threshold(uint8_t soc_num, uint8_t value)
{
	oob_status_t ret;

	ret = sbtsi_set_hbm_alert_threshold(soc_num, value);
	if (ret != OOB_SUCCESS) {
	printf("Failed: to set hbm alert threshold sample, Err[%d]: %s\n",
	       ret, esmi_get_err_msg(ret));
	return ret;
	}
	printf("Set hbm alert threshold success\n");
	return OOB_SUCCESS;
}

/*
 * returns 0 if the given string is a number for the given base, else 1.
 * Base will be 16 for hexadecimal value and 10 for decimal value.
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
oob_status_t parseesb_mi300_args(int argc, char **argv, uint8_t soc_num)
{
	//Specifying the expected options
	static struct option long_options[] = {
		{"showhbmthrottle",             no_argument,            &flag,  800},
		{"showhbmbandwidth",            no_argument,            &flag,  801},
		{"sethbmthrottle",  		required_argument,      &flag,  802},
		{"setmaxgfxcoreclock",          required_argument,      &flag,  803},
		{"setmingfxcoreclock",          required_argument,      &flag,  804},
		{"showpmstatus",		no_argument,            &flag,  806},
		{"showpsn",                     required_argument,      &flag,  807},
		{"showlinkinfo",                no_argument,            &flag,  808},
		{"showdiehotspotinfo",          no_argument,            &flag,  809},
		{"showmemhotspotinfo",          no_argument,            &flag,  810},
		{"showhoststatus",		no_argument,            &flag,  811},
		{"showabsmaxmingfxfreq",        no_argument,            &flag,  812},
		{"showactfreqcapselected",      no_argument,            &flag,  813},
		{"showgfxclkfreqlimit",         no_argument,            &flag,  814},
		{"showflckfreqlimit",           no_argument,            &flag,  815},
		{"showhbmstacktemp",            required_argument,      &flag,  816},
		{"showxgmipstates",             required_argument,      &flag,  817},
		{"setxgmipstate",               required_argument,      &flag,  818},
		{"unsetxgmipstate",             no_argument,            &flag,  819},
		{"showpstates",                 required_argument,      &flag,  820},
		{"setmaxpstate",                required_argument,      &flag,  821},
		{"showbistresult",              required_argument,      &flag,  822},
		{"showsvibasedtelemetryforindvrails",    required_argument,      &flag,  823},
		{"showenergyacctimestamp",      no_argument,            &flag,  824},
		{"showxccidleres",              no_argument,            &flag,  825},
		{"shownumberofsockets",         no_argument,            &flag,  826},
		{"querystatistics",             required_argument,      &flag,  827},
		{"clearstatistics",             no_argument,            &flag,  828},
		{"sethbmhightempthreshold",     required_argument,      &flag,  829},
		{"sethbmlowtempthreshold",      required_argument,      &flag,  830},
		{"sethbmalertthreshold",	required_argument,	&flag,	831},
		{"sethbmalertconfig",		required_argument,	&flag,	832},
		{"showmaxdieid",		no_argument,		&flag,	833},
		{"showdietype",			required_argument,	&flag,  834},
		{"getcurrxgmipstate",           no_argument,            &flag,  835},
		{"getoperatingtemp",            required_argument,	&flag,  836},
		{"getslowdowntemp",             required_argument,	&flag,  837},
		{"gethbmdeviceinfo",            no_argument,            &flag,  838},
		{"getpciestats",                required_argument,      &flag,  839},
		{0,                     0,                      0,      0},
	};

	struct statistics stat = {0};
	struct svi_port_domain port = {0};
	float temp;
	uint32_t val1;
	uint32_t val2;
	int opt = 0; /* option character */
	int long_index = 0;
	uint8_t type = 0;
	char *end;
	char *helperstring = "";
	oob_status_t ret = OOB_NOT_FOUND;

	optind = 2;
	opterr = 0;


	while ((opt = getopt_long(argc, argv, helperstring,
		long_options, &long_index)) != -1) {
		if (opt && optopt) {
			printf("Option '%s' require an argument\n",
			       argv[ optind - 1]);
			return OOB_SUCCESS;
		}

		if (opt == 0 && ((*long_options[long_index].flag) == 802 ||
				(*long_options[long_index].flag) == 803 ||
				(*long_options[long_index].flag) == 804 ||
				(*long_options[long_index].flag) == 807 ||
				(*long_options[long_index].flag) == 816 ||
				(*long_options[long_index].flag) == 817 ||
				(*long_options[long_index].flag) == 818 ||
				(*long_options[long_index].flag) == 820 ||
				(*long_options[long_index].flag) == 821 ||
				(*long_options[long_index].flag) == 822 ||
				(*long_options[long_index].flag) == 829 ||
				(*long_options[long_index].flag) == 830 ||
				(*long_options[long_index].flag) == 831 ||
				(*long_options[long_index].flag) == 832 ||
				(*long_options[long_index].flag) == 834 ||
				(*long_options[long_index].flag) == 836 ||
				(*long_options[long_index].flag) == 837)) {
			if (*long_options[long_index].flag == 829 ||
			    *long_options[long_index].flag == 830) {
				strtof(argv[optind - 1], &end);
				if (*end != '\0') {
					printf("\nOption '--%s' require"
					       " argument as valid decimal "
					       "value\n",
					       long_options[long_index].name);
					return OOB_SUCCESS;
				}
			} else {
				if (validate_number(argv[optind - 1], 10)) {
					printf("\nOption '--%s' require argument"
					       " as valid numeric value\n",
					       long_options[long_index].name);
					return OOB_SUCCESS;
				}
			}
		}

		if (*long_options[long_index].flag == 823 ||
		    *long_options[long_index].flag == 827) {
			if (optind >= argc || *argv[optind] == '-') {
				printf("\nOption '--%s' require TWO arguments\n",
				       long_options[long_index].name);
				return OOB_SUCCESS;
			}
			if (validate_number(argv[optind - 1], 10)) {
				printf("Option '--%s' require 1st argument "
				       "as valid numeric value\n",
				       long_options[long_index].name);
				return OOB_SUCCESS;
			}
			if (validate_number(argv[optind], 10)) {
				printf("Option '--%s' require 2nd argument as "
				       " valid numeric value\n",
				       long_options[long_index].name);
				return OOB_SUCCESS;
			}
		}

		switch(*long_options[long_index].flag) {
		case 800:
			/* Show hbm throttle */
			apml_get_hbm_throttle(soc_num);
			break;
		case 801:
			/* Show hbm memory bandwidth */
			apml_get_hbm_bandwidth(soc_num);
			break;
		case 802:
			/* Set and verify hbm throttle */
			val1 = atoi(argv[optind - 1]);
			apml_set_hbm_throttle(soc_num, val1);
			break;
		case 803:
			/* Set max gfx core clock frequency */
			val1 = atoi(argv[optind - 1]);
			apml_set_gfx_core_clock(soc_num, MAX, val1);
			break;
		case 804:
			/* Set min gfx core clock frequency */
			val1 = atoi(argv[optind - 1]);
			apml_set_gfx_core_clock(soc_num, MIN, val1);
			break;
		case 806:
			/* Get PM ALARMS status */
			apml_get_alarms(soc_num, PM);
			break;
		case 807:
			/* Get PSN */
			val1 = atoi(argv[optind - 1]);
			apml_get_psn(soc_num, val1);
			break;
		case 808:
			/* Get Link Info */
			apml_get_link_info(soc_num);
			break;
		case 809:
			/* Get die hotspot Info */
			apml_die_hotspot_info(soc_num);
			break;
		case 810:
			/* Get Memory hotspot Info */
			apml_mem_hotspot_info(soc_num);
			break;
		case 811:
			/* Get Status */
			apml_get_host_status(soc_num);
			break;
		case 812:
			/* Get absolute max gfx frequnecy */
			apml_get_max_min_gfx_freq(soc_num);
			break;
		case 813:
			 /* Get act freq cap selected */
			apml_get_act_gfx_freq(soc_num);
			break;
		case 814:
			/* Get gfx freq limit */
			apml_get_clk_freq_limit(soc_num, GFX_CLK);
			break;
		case 815:
			/* Get fclk freq limit */
			apml_get_clk_freq_limit(soc_num, F_CLK);
			break;
		case 816:
			/* Get hbm stack index */
			val1 = atoi(argv[optind - 1]);
			/* Get hbm stack temperature */
			apml_get_hbm_stack_temp(soc_num, val1);
			break;
		case 817:
			/* Get xgmi pstates */
			/* pstate index */
			type = atoi(argv[optind - 1]);
			apml_get_xgmi_pstates(soc_num, type);
			break;
		case 818:
			/* Set XGMI PState */
			/* Pstate value */
			val1 = atoi(argv[optind - 1]);
			apml_set_xgmi_pstate(soc_num, val1);
			break;
		case 819:
			/* Unset XGMI Pstate */
			apml_unset_xgmi_pstate(soc_num);
			break;
		case 820:
			/* pstateindex */
			val1 = atoi(argv[optind - 1]);
			/* Get pstates */
			apml_get_mclk_fclk_pstates(soc_num, val1);
			break;
		case 821:
			/* pstate */
			val1 = atoi(argv[optind - 1]);
			apml_set_max_mclk_fclk_pstate(soc_num, val1);
			break;
		case 822:
			/* Die ID */
			val1 = atoi(argv[optind - 1]);
			/* Get die level bist result */
			apml_show_bist_results(soc_num, val1);
			break;
		case 823:
			/* port */
			port.port = atoi(argv[optind - 1]);
			port.slave_addr = atoi(argv[optind++]);
			/* Get SVI based telemetry for individual rails */
			apml_get_svi_telemetry_by_rail(soc_num, port);
			break;
		case 824:
			/* Get Energy accumulator with time stamp */
			apml_get_energy_accumulator_with_timestamp(soc_num);
			break;
		case 825:
			/* Get XCC Idle residency */
			apml_get_xcc_idle_residency(soc_num);
			break;
		case 826:
			/* Get number of sockets */
			apml_get_number_of_soc(soc_num);
			break;
		case 827:
			/* Get query statistics */
			stat.stat_param = atoi(argv[optind - 1]);
			stat.output_control = atoi(argv[optind++]);
			apml_query_statistics(soc_num, stat);
			break;
		case 828:
			/* Get Clear Statistics */
			apml_clear_statistics(soc_num);
			break;
		case 829:
			/* Hbm high temperature threshold */
			temp = atof(argv[optind - 1]);
			/* Set hbm high temperature threshold */
			apml_set_hbm_high_threshold_temp(soc_num, temp);
			break;
		case 830:
			 /* Hbm low temperature threshold */
			temp = atof(argv[optind - 1]);
			/* Set hbm low temperature threshold */
			apml_set_hbm_low_threshold_temp(soc_num, temp);
			break;
		case 831:
			/* Set HBM alert threshold samples */
			val1 = atoi(argv[optind - 1]);
			apml_set_hbm_alert_threshold(soc_num, val1);
			break;
		case 832:
			/* Set HBM Alert config */
			val1 = atoi(argv[optind - 1]);
			apml_set_hbm_alert_config(soc_num, val1);
			break;
		case 833:
			/* show max die id */
			apml_get_maximum_die_id(soc_num);
			break;
		case 834:
			/* Show die-type info */
			val1 = atoi(argv[optind - 1]);
			apml_get_die_type(soc_num, val1);
			break;
		case 835:
			/* Get current XGMI pstate */
			apml_get_curr_xgmi_pstate(soc_num);
			break;
		case 836:
			/* Get Max operating temperature */
			val1 = atoi(argv[optind - 1]);
			apml_get_max_operating_temp(soc_num, val1);
			break;
		case 837:
			/* Get slow down temperature */
			val1 = atoi(argv[optind - 1]);
			apml_get_slow_down_temp(soc_num, val1);
			break;
		case 838:
			/* Get hbm device info */
			apml_get_hbm_dev_info(soc_num);
			break;
		case 839:
			/* Get PCIe statistics */
			val1 = atoi(argv[optind - 1]);
			apml_get_pcie_stats(soc_num, val1);
			break;
		default:
			return ret;
		}
	}
	return OOB_SUCCESS;
}
