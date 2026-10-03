/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <esmi_oob/apml.h>
#include <esmi_oob/apml_common.h>
#include <esmi_oob/esmi_tsi.h>
#include <esmi_oob/tsi_mi300.h>

/* Decimal portion bits */
#define DEC_PORTION_BITS	5	//!< Decimal portion bits
/* Delay time or wait time */
#define WAIT_TIME		1000	//!< Delay time
/* Min temperature */
#define MIN_TEMP		0	//!< Min Temp
/* Max temperature */
#define MAX_TEMP		255	//!< Max Temp

/* sb-tsi register access for MI*/

oob_status_t read_sbtsi_hbm_hi_temp_int_th(uint8_t soc_num, uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_HBM_HITEMPINT_LIMIT,
				      buffer);
}

oob_status_t read_sbtsi_hbm_hi_temp_dec_th(uint8_t soc_num, float *buffer)
{
	uint8_t d_out;
	oob_status_t ret;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_HBM_HITEMPDEC_LIMIT,
				     &d_out);
	if (!ret)
		*buffer = ((d_out >> DEC_PORTION_BITS) * TEMP_INC);

	return ret;
}

oob_status_t write_sbtsi_hbm_hi_temp_th(uint8_t soc_num, float hi_temp_th)
{
	uint8_t int_temp, cur_temp_dec, byte_dec;
	float temp_dec;
	oob_status_t ret;

	if (hi_temp_th < MIN_TEMP || hi_temp_th > MAX_TEMP)
		return OOB_INVALID_INPUT;

	int_temp = hi_temp_th;
	temp_dec = hi_temp_th - int_temp;

	/* Write integer part of high temp threshold */
	ret = esmi_oob_tsi_write_byte(soc_num, SBTSI_HBM_HITEMPINT_LIMIT,
				      int_temp);
	if (ret)
		return ret;

	/* Read decimal part of current high temp threshold */
	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_HBM_HITEMPDEC_LIMIT,
				     &cur_temp_dec);
	if (ret)
		return ret;

	/* get number of steps increment */
	byte_dec = temp_dec / TEMP_INC;
	/* [7:5] HiTempDec and [4:0] Reserved */
	byte_dec = ((byte_dec << 5) | (cur_temp_dec & 0x1F));

	return esmi_oob_tsi_write_byte(soc_num, SBTSI_HBM_HITEMPDEC_LIMIT,
				       byte_dec);
}

oob_status_t read_sbtsi_hbm_hi_temp_th(uint8_t soc_num, float *buffer)
{
	float dec_temp;
	uint8_t int_temp;
	oob_status_t ret;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = read_sbtsi_hbm_hi_temp_int_th(soc_num, &int_temp);
	if (ret)
		return ret;

	usleep(WAIT_TIME);
	ret = read_sbtsi_hbm_hi_temp_dec_th(soc_num, &dec_temp);
	if (!ret)
		*buffer = int_temp + dec_temp;

	return ret;
}

oob_status_t read_sbtsi_hbm_lo_temp_int_th(uint8_t soc_num, uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_HBM_LOTEMPINT_LIMIT,
				      buffer);
}

oob_status_t read_sbtsi_hbm_lo_temp_dec_th(uint8_t soc_num, float *buffer)
{
	uint8_t d_out;
	oob_status_t ret;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret =  esmi_oob_tsi_read_byte(soc_num, SBTSI_HBM_LOTEMPDEC_LIMIT,
				      &d_out);
	if (!ret)
		*buffer = ((d_out >> DEC_PORTION_BITS) * TEMP_INC);

	return ret;
}

oob_status_t write_sbtsi_hbm_lo_temp_th(uint8_t soc_num, float temp_th)
{
	uint8_t int_temp, cur_temp_dec, byte_dec;
	float temp_dec;
	oob_status_t ret;

	if (temp_th < MIN_TEMP || temp_th > MAX_TEMP)
		return OOB_INVALID_INPUT;

	int_temp = temp_th;
	temp_dec = temp_th - int_temp;

	/* Write integer part of low temp threshold */
	ret = esmi_oob_tsi_write_byte(soc_num, SBTSI_HBM_LOTEMPINT_LIMIT,
				      int_temp);
	if (ret)
		return ret;

	/* Read decimal part of current low temp threshold */
	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_HBM_LOTEMPDEC_LIMIT,
				     &cur_temp_dec);
	if (ret)
		return ret;

	/* get number of steps increment */
	byte_dec = temp_dec / TEMP_INC;
	/* [7:5] HiTempDec and [4:0] Reserved */
	byte_dec = ((byte_dec << 5) | (cur_temp_dec & 0x1F));

	return esmi_oob_tsi_write_byte(soc_num, SBTSI_HBM_LOTEMPDEC_LIMIT,
				       byte_dec);
}

oob_status_t read_sbtsi_hbm_lo_temp_th(uint8_t soc_num, float *buffer)
{
	float dec_temp;
	uint8_t int_temp;
	oob_status_t ret;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = read_sbtsi_hbm_lo_temp_int_th(soc_num, &int_temp);
	if (ret)
		return ret;

	usleep(WAIT_TIME);
	ret = read_sbtsi_hbm_lo_temp_dec_th(soc_num, &dec_temp);
	if (!ret)
		*buffer = int_temp + dec_temp;

	return ret;
}

oob_status_t read_sbtsi_max_hbm_temp_int(uint8_t soc_num, uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_MAX_HBMTEMPINT,
				      buffer);
}

oob_status_t read_sbtsi_max_hbm_temp_dec(uint8_t soc_num, float *buffer)
{
        uint8_t d_out;
        oob_status_t ret;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_MAX_HBMTEMPDEC,
				     &d_out);
	if (!ret)
		*buffer = ((d_out >> DEC_PORTION_BITS) * TEMP_INC);

	return ret;
}

oob_status_t read_sbtsi_max_hbm_temp(uint8_t soc_num, float *buffer)
{
	float dec_temp;
	uint8_t int_temp;
	oob_status_t ret;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = read_sbtsi_max_hbm_temp_int(soc_num, &int_temp);
	if (ret)
		return ret;

	usleep(WAIT_TIME);
	ret = read_sbtsi_max_hbm_temp_dec(soc_num, &dec_temp);
	if (!ret)
		*buffer = int_temp + dec_temp;

	return ret;
}

oob_status_t read_sbtsi_hbm_temp_int(uint8_t soc_num, uint8_t *buffer)
{
	return esmi_oob_tsi_read_byte(soc_num, SBTSI_HBMTEMPINT,
				      buffer);
}

oob_status_t read_sbtsi_hbm_temp_dec(uint8_t soc_num, float *buffer)
{
	uint8_t d_out;
	oob_status_t ret;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_HBMTEMPDEC,
				     &d_out);
	if (!ret)
		*buffer = ((d_out >> DEC_PORTION_BITS) * TEMP_INC);

	return ret;
}

oob_status_t read_sbtsi_hbm_temp(uint8_t soc_num, float *buffer)
{
	float dec_temp;
	uint8_t int_temp;
	oob_status_t ret;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = read_sbtsi_hbm_temp_int(soc_num, &int_temp);
	if (ret)
		return ret;

	usleep(WAIT_TIME);
	ret = read_sbtsi_hbm_temp_dec(soc_num, &dec_temp);
	if (!ret)
		*buffer = int_temp + dec_temp;

	return ret;
}

oob_status_t read_sbtsi_hbm_alertthreshold(uint8_t soc_num, uint8_t *samples)
{
	oob_status_t ret;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_ALERTTHRESHOLD,
				     samples);
	if (ret)
		return ret;

	/*
	 *  Bits 3 - 5 will provide hbm alert threshold
	 *  ex value : samples
	 *  0h: 1 sample
	 *  6h-1h: (value + 1) sample
	 *  7h: 8 samples
	 */
	*samples = ((*samples & 0x38) >> 3) + 1;

	return ret;
}

oob_status_t sbtsi_set_hbm_alert_threshold(uint8_t soc_num,
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
	* [7:6] reserved and [5:3] HBM AlertThr value
	* ex value : samples
	* 0h: 1 sample
	* 6h-1h: (value + 1) sample
	* 7h: 8 samples
	*/

	new = (prev & 0xC7) | ((samples - 1) << 3);
	return esmi_oob_tsi_write_byte(soc_num, SBTSI_ALERTTHRESHOLD, new);
}

oob_status_t get_sbtsi_hbm_alertconfig(uint8_t soc_num, uint8_t *mode)
{
	oob_status_t ret;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_ALERTCONFIG, mode);
	if (ret)
		return ret;

	/**
	 * [7:2] are reserved, Bit [1] HBM Alert config (1 indicates and
	 * 0 indicates disable).
	 */
	*mode = (*mode >> 1) & BIT(0);

	return ret;
}

oob_status_t set_sbtsi_hbm_alertconfig(uint8_t soc_num, uint8_t mode)
{
	oob_status_t ret;
	uint8_t prev, new;

	/* single bit validation */
	if (mode > 1)
		return OOB_INVALID_INPUT;

	ret = esmi_oob_tsi_read_byte(soc_num, SBTSI_ALERTCONFIG, &prev);
	if (ret)
		return ret;
	/* If the bit-1 of previous value is same as mode value */
	if (mode == ((prev >> 1) & BIT(0)))
		return OOB_SUCCESS;
	/* [7:2] reserved, [1] HBM Alert config bit */
	new = (prev & 0xFD) | (mode << 1);
	return esmi_oob_tsi_write_byte(soc_num, SBTSI_ALERTCONFIG, new);
}
