/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <esmi_oob/esmi_tsi.h>
#include <esmi_oob/apml.h>

/* sb-tsi register access */
oob_status_t read_sbtsi_cpuinttemp(uint8_t soc_num,
				   uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPINT,
				      buffer);
}

oob_status_t read_sbtsi_status(uint8_t soc_num,
			       uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num,
				  SBTSI_STATUS, buffer);
}

oob_status_t read_sbtsi_config(uint8_t soc_num,
			       uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_CONFIGURATION, buffer);
}

oob_status_t read_sbtsi_updaterate(uint8_t soc_num,
				   float *buffer)
{
	/* as per the ssp document valid rates from 0 - 10 are as below */
	float valid_rate[] = {0.0625, 0.125, 0.25, 0.5, 1, 2, 4, 8, 16, 32, 64};
	int items = 11; /* total number of uprates in above valid_rate[] */
	oob_status_t ret;
	uint8_t rdbyte;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_UPDATERATE, &rdbyte);
	if (ret != OOB_SUCCESS)
		return ret;
	if (rdbyte >= items)
		return OOB_UNKNOWN_ERROR;
	*buffer = valid_rate[rdbyte];

	return OOB_SUCCESS;
}

oob_status_t write_sbtsi_updaterate(uint8_t soc_num,
				    float uprate)
{
	/* as per the ssp document valid rates from 0 - 10 are as below */
	float valid_rate[] = {0.0625, 0.125, 0.25, 0.5, 1, 2, 4, 8, 16, 32, 64};
	int items = 11; /* total number of uprates in above valid_rate[] */
	uint8_t wrbyte;

	for (wrbyte = 0; wrbyte < items; wrbyte++) {
		if (uprate == valid_rate[wrbyte])
			break;
	}
	if (wrbyte >= items)
		return OOB_INVALID_INPUT;

	return esmi_oob_tsi_write_byte(soc_num, SBTSI_UPDATERATE, wrbyte);
}

oob_status_t sbtsi_set_hitemp_threshold(uint8_t soc_num,
					float hitemp_thr)
{
	oob_status_t ret;
	uint8_t prev, current, byte_int, byte_dec;
	float temp_dec;

	if (hitemp_thr < 0 || hitemp_thr >= 256)
		return OOB_INVALID_INPUT;

	byte_int = hitemp_thr;
	temp_dec = hitemp_thr - byte_int;

	ret = esmi_oob_tsi_write_byte(soc_num, SBTSI_HITEMPINT, byte_int);
	if (ret != OOB_SUCCESS)
		return ret;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_HITEMPDEC, &prev);
	if (ret != OOB_SUCCESS)
		return ret;

	/* get number of steps increment */
	byte_dec = temp_dec / TEMP_INC;

	/* [7:5] HiTempDec and [4:0] Reserved */
	current = ((byte_dec << 5) | (prev & 0x1F));
	return esmi_oob_tsi_write_byte(soc_num, SBTSI_HITEMPDEC, current);
}

oob_status_t sbtsi_set_lotemp_threshold(uint8_t soc_num,
					float lotemp_thr)
{
	oob_status_t ret;
	uint8_t prev, current, byte_int, byte_dec;
	float temp_dec;

	if (lotemp_thr < 0 || lotemp_thr >= 256)
		return OOB_INVALID_INPUT;

	byte_int = lotemp_thr;
	temp_dec = lotemp_thr - byte_int;

	ret = esmi_oob_tsi_write_byte(soc_num, SBTSI_LOTEMPINT, byte_int);
	if (ret != OOB_SUCCESS)
		return ret;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_LOTEMPDEC, &prev);
	if (ret != OOB_SUCCESS)
		return ret;

	/* get number of steps increment */
	byte_dec = temp_dec / TEMP_INC;

	/* [7:5] LoTempDec and [4:0] Reserved */
	current = ((byte_dec << 5) | (prev & 0x1F));
	return esmi_oob_tsi_write_byte(soc_num, SBTSI_LOTEMPDEC, current);
}

oob_status_t sbtsi_set_timeout_config(uint8_t soc_num,
				      uint8_t mode)
{
	oob_status_t ret;
	uint8_t prev, new;

	/* 1 : Enabled and 0 Disbaled */
	if (mode != 1 && mode != 0)
		return OOB_INVALID_INPUT;
	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_TIMEOUTCONFIG, &prev);
	if (ret != OOB_SUCCESS)
		return ret;

	/* [7] TimeoutEn and [6:0] Reserved */
	new = ((mode << 7) | (prev & 0x7F));
	return esmi_oob_tsi_write_byte(soc_num, SBTSI_TIMEOUTCONFIG, new);
}

oob_status_t sbtsi_set_alert_threshold(uint8_t soc_num,
				       uint8_t samples)
{
	oob_status_t ret;
	uint8_t prev, new;

	/* Alert threshold valid range from 1 to 8 samples. */
	if (samples < 1 || samples > 8)
		return OOB_INVALID_INPUT;
	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_ALERTTHRESHOLD, &prev);
	if (ret != OOB_SUCCESS)
		return ret;
	/**
	 * [7:3] reserved and [2:0] AlertThr value
	 * ex value : samples
	 * 0h: 1 sample
	 * 6h-1h: (value + 1) sample
	 * 7h: 8 samples
	 */
	new = (prev & 0xF8) | (samples - 1);
	return esmi_oob_tsi_write_byte(soc_num, SBTSI_ALERTTHRESHOLD, new);
}

oob_status_t sbtsi_set_alert_config(uint8_t soc_num,
				    uint8_t mode)
{
	oob_status_t ret;
	uint8_t prev, new;

	/* single bit validation */
	if (mode != 1 && mode != 0)
		return OOB_INVALID_INPUT;
	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_ALERTCONFIG, &prev);
	if (ret != OOB_SUCCESS)
		return ret;
	/* [7:1] reserved, [0] Alert Comparator mode enable */
	new = (prev & 0xFE) | mode;
	return esmi_oob_tsi_write_byte(soc_num, SBTSI_ALERTCONFIG, new);
}

oob_status_t sbtsi_set_configwr(uint8_t soc_num,
				uint8_t mode, uint8_t config_mask)
{
	oob_status_t ret;
	uint8_t prev, new;

	/* single bit validation */
	if (mode != 1 && mode != 0)
		return OOB_INVALID_INPUT;
	if (config_mask != ALERTMASK_MASK &&
	    config_mask != RUNSTOP_MASK &&
	    config_mask != READORDER_MASK &&
	    config_mask != ARA_MASK)
		return OOB_INVALID_INPUT;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CONFIGURATION, &prev);
	if (ret != OOB_SUCCESS)
		return ret;

	new = mode ? prev | config_mask : prev & (~config_mask);
	return esmi_oob_tsi_write_byte(soc_num, SBTSI_CONFIGWR, new);
}

oob_status_t read_sbtsi_hitempint(uint8_t soc_num,
				  uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_HITEMPINT, buffer);
}

oob_status_t read_sbtsi_lotempint(uint8_t soc_num,
				  uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_LOTEMPINT, buffer);
}

oob_status_t read_sbtsi_configwrite(uint8_t soc_num,
				    uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_CONFIGWR, buffer);
}

oob_status_t read_sbtsi_cputempdecimal(uint8_t soc_num,
				       float *buffer)
{
	uint8_t rd_byte;
	oob_status_t ret;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPDEC, &rd_byte);
	if (ret)
		return ret;
	*buffer = ((rd_byte >> 5) * TEMP_INC);

	return OOB_SUCCESS;
}

oob_status_t read_sbtsi_cputempoffint(uint8_t soc_num,
				      uint8_t *temp_int)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPOFFINT, temp_int);
}

oob_status_t read_sbtsi_cputempoffdec(uint8_t soc_num,
				      float *temp_dec)
{
	uint8_t rd_byte;
	oob_status_t ret;

	if (!temp_dec)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPOFFDEC, &rd_byte);
	if (ret)
		return ret;
	*temp_dec = ((rd_byte >> 5) * TEMP_INC);

	return OOB_SUCCESS;
}

oob_status_t read_sbtsi_hitempdecimal(uint8_t soc_num,
				      float *temp_dec)
{
	uint8_t rd_byte;
	oob_status_t ret;

	if (!temp_dec)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_HITEMPDEC, &rd_byte);
	if (ret)
		return ret;
	*temp_dec = ((rd_byte >> 5) * TEMP_INC);

	return OOB_SUCCESS;
}

oob_status_t read_sbtsi_lotempdecimal(uint8_t soc_num,
				      float *temp_dec)
{
	uint8_t rd_byte;
	oob_status_t ret;

	if (!temp_dec)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_LOTEMPDEC, &rd_byte);
	if (ret)
		return ret;
	*temp_dec = ((rd_byte >> 5) * TEMP_INC);

	return OOB_SUCCESS;
}

oob_status_t read_sbtsi_timeoutconfig(uint8_t soc_num,
				      uint8_t *timeout)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_TIMEOUTCONFIG, timeout);
}

oob_status_t read_sbtsi_cputempoffset(uint8_t soc_num,
				      float *temp_offset)
{
	oob_status_t ret;
	int8_t byte_int;
	uint8_t byte_dec;

	if (!temp_offset)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPOFFINT, &byte_int);
	if (ret != OOB_SUCCESS)
		return ret;
	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPOFFDEC, &byte_dec);
	if (ret != OOB_SUCCESS)
		return ret;
	/* combining integer and decimal part to make float value
	 * [7:5] decimal value in byte
	 */
	*temp_offset = byte_int + (byte_dec >> 5) * TEMP_INC;

	return OOB_SUCCESS;
}

oob_status_t write_sbtsi_cputempoffset(uint8_t soc_num,
				       float temp_offset)
{
	oob_status_t ret;
	int8_t byte_int, byte_dec;
	uint8_t prev, current;

	if (temp_offset < -128 || temp_offset >= 128)
		return OOB_INVALID_INPUT;
	/* extracting integer and decimal part from float value */
	byte_int = temp_offset;
	if (temp_offset < 0 && (temp_offset - byte_int != 0))
		byte_int--;

	byte_dec = (temp_offset - byte_int) / TEMP_INC;

	ret = esmi_oob_tsi_write_byte(soc_num, SBTSI_CPUTEMPOFFINT, byte_int);
	if (ret != OOB_SUCCESS)
		return ret;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPOFFDEC, &prev);
	if (ret != OOB_SUCCESS)
		return ret;
	current = ((prev & 0x1F) | (byte_dec << 5));
	return esmi_oob_tsi_write_byte(soc_num, SBTSI_CPUTEMPOFFDEC, current);
}

oob_status_t read_sbtsi_alertthreshold(uint8_t soc_num,
				       uint8_t *samples)
{
	oob_status_t ret;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_ALERTTHRESHOLD, samples);
	if (ret != OOB_SUCCESS)
		return ret;
	/**
	 * [7:3] reserved and [2:0] AlertThr value
	 * ex value : samples
	 * 0h: 1 sample
	 * 6h-1h: (value + 1) sample
	 * 7h: 8 samples
	 */
	*samples = (*samples & 0x07) + 1;

	return OOB_SUCCESS;
}

oob_status_t read_sbtsi_alertconfig(uint8_t soc_num,
				    uint8_t *mode)
{
	oob_status_t ret;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_ALERTCONFIG, mode);
	if (ret != OOB_SUCCESS)
		return ret;
	/* [7:1] reserved, [0] Alert Comparator mode enable */
	*mode = (*mode & 1);

	return OOB_SUCCESS;
}

oob_status_t read_sbtsi_manufid(uint8_t soc_num,
				uint8_t *man_id)
{
	oob_status_t ret;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_MANUFID, man_id);
	if (ret != OOB_SUCCESS)
		return ret;
	/* [7:1] reserved, [0] Manufacture ID */
	*man_id = (*man_id & 1);

	return OOB_SUCCESS;
}

oob_status_t read_sbtsi_revision(uint8_t soc_num,
				 uint8_t *rivision)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_REVISION, rivision);
}

oob_status_t sbtsi_get_cputemp(uint8_t soc_num,
			       float *cpu_temp)
{
	oob_status_t ret;
	uint8_t byte_int, byte_dec;
	uint8_t rd_order;

	if (!cpu_temp)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CONFIGURATION, &rd_order);
	if (ret != OOB_SUCCESS)
		return ret;
	rd_order &= READORDER_MASK;
	if (rd_order) {
		ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPDEC,
					     &byte_dec);
		if (ret != OOB_SUCCESS)
			return ret;
		usleep(1000);
		ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPINT,
					     &byte_int);
		if (ret != OOB_SUCCESS)
			return ret;
	} else {
		ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPINT,
					     &byte_int);
		if (ret != OOB_SUCCESS)
			return ret;
		usleep(1000);
		ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CPUTEMPDEC,
					     &byte_dec);
		if (ret != OOB_SUCCESS)
			return ret;
	}
	*cpu_temp = byte_int + ((byte_dec >> 5) * TEMP_INC);

	return OOB_SUCCESS;
}

oob_status_t sbtsi_get_hitemp_threshold(uint8_t soc_num,
					float *hitemp_thr)
{
	oob_status_t ret;
	uint8_t byte_int, byte_dec;

	if (!hitemp_thr)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_HITEMPINT, &byte_int);
	if (ret != OOB_SUCCESS)
		return ret;
	usleep(1000);
	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_HITEMPDEC, &byte_dec);
	if (ret != OOB_SUCCESS)
		return ret;
	/* combining integer and decimal part to make float value
	 * [7:5] decimal value in byte
	 */
	*hitemp_thr = byte_int + ((byte_dec >> 5) * TEMP_INC);

	return OOB_SUCCESS;
}

oob_status_t sbtsi_get_lotemp_threshold(uint8_t soc_num,
					float *lotemp_thr)
{
	oob_status_t ret;
	uint8_t byte_int, byte_dec;

	if (!lotemp_thr)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_LOTEMPINT, &byte_int);
	if (ret != OOB_SUCCESS)
		return ret;
	usleep(1000);
	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_LOTEMPDEC, &byte_dec);
	if (ret != OOB_SUCCESS)
		return ret;
	/* combining integer and decimal part to make float value
	 * [7:5] decimal value in byte
	 */
	*lotemp_thr = byte_int + ((byte_dec >> 5) * TEMP_INC);

	return OOB_SUCCESS;
}

oob_status_t sbtsi_get_temp_status(uint8_t soc_num,
				   uint8_t *loalert, uint8_t *hialert)
{
	oob_status_t ret;
	uint8_t rdbyte;

	if ((!loalert) || (!hialert))
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_STATUS, &rdbyte);
	if (ret != OOB_SUCCESS)
		return ret;
	/* [4] temperature high alert, [3] temperature low alerti */
	*loalert = rdbyte & (1 << 3);
	*hialert = rdbyte & (1 << 4);

	return OOB_SUCCESS;
}

oob_status_t sbtsi_get_config(uint8_t soc_num,
			      uint8_t *al_mask, uint8_t *run_stop,
			      uint8_t *read_ord, uint8_t *ara)
{
	oob_status_t ret;
	uint8_t rdbytes;

	if ((!al_mask) || (!run_stop) || (!read_ord) || (!ara))
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_CONFIGURATION, &rdbytes);
	if (ret != OOB_SUCCESS)
		return ret;
	*al_mask = rdbytes & ALERTMASK_MASK;
	*run_stop = rdbytes & RUNSTOP_MASK;
	*read_ord = rdbytes & READORDER_MASK;
	*ara = rdbytes & ARA_MASK;

	return OOB_SUCCESS;
}

oob_status_t sbtsi_get_timeout(uint8_t soc_num,
			       uint8_t *timeout_en)
{
	oob_status_t ret;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_TIMEOUTCONFIG, timeout_en);
	if (ret != OOB_SUCCESS)
		return ret;
	/* [7] TimeoutEn and [6:0] Reserved */
	*timeout_en = *timeout_en & (1 << 7);

	return OOB_SUCCESS;
}
