/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#ifndef INCLUDE_APML_TSI_H_
#define INCLUDE_APML_TSI_H_

#include "apml_err.h"

/** \file esmi_tsi.h
 *  Header file for the APML library for SB-TSI functionality access.
 *  All required function, structure, enum, etc. definitions should be defined
 *  in this file for SB-TSI Register accessing.
 *
 *  @details  This header file contains the following:
 *  APIs prototype of the APIs exported by the APML library.
 *  Description of the API, arguments and return values.
 *  The Error codes returned by the API.
 */

/**
 * @brief SB-TSI(Side-Band Temperature Sensor Interface) commands register
 * access.
 * The below registers mentioned as per Genessis PPR
 */
typedef enum {
	SBTSI_CPUTEMPINT = 0x1,
	SBTSI_STATUS,
	SBTSI_CONFIGURATION,
	SBTSI_UPDATERATE,
	SBTSI_HITEMPINT = 0x7,
	SBTSI_LOTEMPINT,
	SBTSI_CONFIGWR,
	SBTSI_CPUTEMPDEC = 0x10,
	SBTSI_CPUTEMPOFFINT,
	SBTSI_CPUTEMPOFFDEC,
	SBTSI_HITEMPDEC,
	SBTSI_LOTEMPDEC,
	SBTSI_TIMEOUTCONFIG = 0x22,
	SBTSI_ALERTTHRESHOLD = 0x32,
	SBTSI_ALERTCONFIG = 0xBF,
	SBTSI_MANUFID = 0xFE,
	SBTSI_REVISION = 0xFF
} sbtsi_registers;

/**
 * @brief Register encode the temperature to increase in 0.125
 * In decimal portion one increase in byte is equivalent to 0.125
 */
#define TEMP_INC 0.125

/**
 * @brief Bitfield values to be set for SBTSI confirwr register
 * [7] Alert mask
 * [6] RunStop
 * [5] ReadOrder
 * [1] AraDis
 */
typedef enum {
	ARA_MASK = 0x2,
	READORDER_MASK = 0x20,
	RUNSTOP_MASK = 0x40,
	ALERTMASK_MASK = 0x80
} sbtsi_config_write;

/*****************************************************************************/
/** @defgroup SB-TSIRegisterAccess SBTSI Register Read Byte Protocol
 *  Below functions provide interface to read one byte from the SB-TSI register
 *  and output is from a given SB_TSI register command.
 *  @{
 */

/**
 *  @brief integer CPU temperature value
 *  The CPU temperature is calculated by adding the CPU temperature
 *  offset(SBTSI::CpuTempOffInt, SBTSI::CpuTempOffDec) to the processor control
 *  temperature (Tctl). SBTSI::CpuTempInt and SBTSI::CpuTempDec
 *  combine to return the CPU temperature.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  This field returns the integer portion of the CPU temperature
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to hold the cpu temperature
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_cpuinttemp(uint8_t soc_num, uint8_t *buffer);

/**
 *  @brief Status register is Read-only, volatile field
 *  If SBTSI::AlertConfig[AlertCompEn] == 0 , the temperature alert is latched
 *  high until the alert is read. If SBTSI::AlertConfig[AlertCompEn] == 1,
 *  the alert is cleared when the temperature does not meet the threshold
 *  conditions for temperature and number of samples.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to hold the cpu temperature
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_status(uint8_t soc_num, uint8_t *buffer);

/**
 *  @brief The bits in this register are Read-only and can be written
 *  by Writing to the corresponding bits in SBTSI::ConfigWr
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to hold the cpu temperature
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_config(uint8_t soc_num, uint8_t *buffer);

/**
 *  @brief This register value specifies the rate at which CPU temperature
 *  is compared against the temperature thresholds to determine if an alert
 *  event has occurred.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to hold the cpu temperature
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_updaterate(uint8_t soc_num, float *buffer);

/**
 *  @brief This register value specifies the rate at which CPU temperature
 *  is compared against the temperature thresholds to determine if an alert
 *  event has occurred.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] uprate value to write in raw format
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t write_sbtsi_updaterate(uint8_t soc_num, float uprate);

/**
 *  @brief This value specifies the integer  portion of the high temperature
 *  threshold.
 *  The high temperature threshold specifies the CPU temperature that
 *  causes ALERT_L to assert if the CPU temperature is greater than or
 *  equal to the threshold.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to hold the integer part of high cpu temp
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_hitempint(uint8_t soc_num, uint8_t *buffer);

/**
 *  @brief This value specifies the integer portion of the low temperature
 *  threshold.
 *  The low temperature threshold specifies the CPU temperature that causes
 *  ALERT_L to assert if the CPU temperature is less than or equal to the
 *  threshold
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to hold the integer part of low cpu temp
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_lotempint(uint8_t soc_num, uint8_t *buffer);

/**
 *  @brief This register provides write access to SBTSI::Config
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to hold the configuraion
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_configwrite(uint8_t soc_num, uint8_t *buffer);

/**
 *  @brief The value returns the decimal portion of the CPU temperature
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] buffer a pointer to hold the cpu temperature decimal
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_cputempdecimal(uint8_t soc_num, float *buffer);

/**
 *  @brief SBTSI::CpuTempOffInt and SBTSI::CpuTempOffDec combine to specify
 *  the CPU temperature offset
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] temp_int a pointer to hold the cpu offset interger
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_cputempoffint(uint8_t soc_num, uint8_t *temp_int);

/**
 *  @brief This value specifies the decimal/fractional portion of the
 *  CPU temperature offset added to Tctl to calculate the CPU temperature.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] temp_dec a pointer to hold the cpu offset decimal
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_cputempoffdec(uint8_t soc_num, float *temp_dec);

/**
 *  @brief This value specifies the decimal portion of the high temperature
 *  threshold.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] temp_dec a pointer to hold the decimal part of cpu high temp
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_hitempdecimal(uint8_t soc_num, float *temp_dec);

/**
 *  @brief value specifies the decimal portion of the low temperature
 *  threshold.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] temp_dec a pointer to hold the decimal part of cpu low temperature
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_lotempdecimal(uint8_t soc_num, float *temp_dec);

/**
 *  @brief value specifies 0=SMBus defined timeout support disabled.
 *  1=SMBus defined timeout support enabled. SMBus timeout enable.
 *  If SB-RMI is in use, SMBus timeouts should
 *  be enabled or disabled in a consistent manner on both interfaces.
 *  SMBus defined timeouts are not disabled for SB-RMI when this bit is set
 *  to 0.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] timeout a pointer to hold the cpu timeout configuration
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_timeoutconfig(uint8_t soc_num, uint8_t *timeout);

/**
 *  @brief Specifies the number of consecutive CPU temperature
 *  samples for which a temperature alert condition needs to remain valid
 *  before the corresponding alert bit is set.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] samples a pointer to hold the cpu temperature alert threshold
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_alertthreshold(uint8_t soc_num, uint8_t *samples);

/**
 *  @brief Status register is Read-only, volatile field
 *  If SBTSI::AlertConfig[AlertCompEn] == 0 , the temperature alert is latched
 *  high until the alert is read. If SBTSI::AlertConfig[AlertCompEn] == 1,
 *  the alert is cleared when the temperature does not meet the threshold
 *  conditions for temperature and number of samples.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] mode a pointer to hold the cpu temperature alert configuration
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_alertconfig(uint8_t soc_num, uint8_t *mode);

/**
 *  @brief Returns the AMD manufacture ID
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] man_id a pointer to hold the manufacture id
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_manufid(uint8_t soc_num, uint8_t *man_id);

/**
 *  @brief Specifies the SBI temperature sensor interface revision
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] rivision a pointer to hold the cpu temperature revision
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_revision(uint8_t soc_num, uint8_t *rivision);

/* Extra API's for bit parsing */
/**
 *  @brief CPU temperature value
 *  The CPU temperature is calculated by adding SBTSI::CpuTempInt
 *  and SBTSI::CpuTempDec combine to return the CPU temperature.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] cpu_temp a pointer to get temperature of the CPU
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_get_cputemp(uint8_t soc_num, float *cpu_temp);

/**
 *  @brief Status register is Read-only, volatile field
 *  If SBTSI::AlertConfig[AlertCompEn] == 0 , the temperature alert is latched
 *  high until the alert is read. If SBTSI::AlertConfig[AlertCompEn] == 1,
 *  the alert is cleared when the temperature does not meet the threshold
 *  conditions for temperature and number of samples.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] loalert: 1=> CPU temp is less than or equal to low temperature
 *  threshold for consecutive samples
 *
 *  @param[inout] hialert: 1=> CPU temp is greater than or equal to high
 *  temperature threshold for consecutive samples
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_get_temp_status(uint8_t soc_num,
				   uint8_t *loalert, uint8_t *hialert);

/**
 *  @brief The bits in this register are Read-only and can be written
 *  by Writing to the corresponding bits in SBTSI::ConfigWr
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] al_mask: 0=> ALERT_L pin enabled. 1=> ALERT_L pin disabled
 *  and does not assert.
 *
 *  @param[inout] run_stop: 0=> Updates to CpuTempInt and CpuTempDec and
 *  alert comparisons are enabled. 1=> Updates are disabled and
 *  alert comparisons are disabled.
 *
 *  @param[inout] read_ord: 0=> Reading CpuTempInt causes the satate of
 *  CpuTempDec to be latched. 1=> Reading CpuTempInt causes
 *  the satate of CpuTempDec to be latched.
 *
 *  @param[inout] ara: 1=> ARA response disabled.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_get_config(uint8_t soc_num,
			      uint8_t *al_mask, uint8_t *run_stop,
			      uint8_t *read_ord, uint8_t *ara);

/**
 *  @brief The bits in this register are defined sbtsi_config_write and can
 *  be written by writing to the corresponding bits in SBTSI::ConfigWr
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  NOTE: Currently testing is not done for this API.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] mode: value to update 0 or 1
 *
 *  @param[in] config_mask: which bit need to update
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_set_configwr(uint8_t soc_num,
				uint8_t mode, uint8_t config_mask);

/**
 *  @brief To verify if timeout support enabled or disabled
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] timeout_en: 0=>SMBus defined timeout support disabled.
 *
 *  1=SMBus defined timeout support enabled. SMBus timeout enable.
 *  If SB-RMI is in use, SMBus timeouts should
 *  be enabled or disabled in a consistent manner on both interfaces.
 *  SMBus defined timeouts are not disabled for SB-RMI when this bit
 *  is set to 0.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_get_timeout(uint8_t soc_num,
			       uint8_t *timeout_en);

/**
 *  @brief To enable/disable timeout support
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] mode:  0=>SMBus defined timeout support disabled.
 *
 *  1=>SMBus defined timeout support enabled. SMBus timeout enable.
 *  If SB-RMI is in use, SMBus timeouts should
 *  be enabled or disabled in a consistent manner on both interfaces.
 *  SMBus defined timeouts are not disabled for SB-RMI when
 *  this bit is set to 0.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_set_timeout_config(uint8_t soc_num,
				      uint8_t mode);

/**
 *  @brief This value set the high temperature threshold.
 *  The high temperature threshold specifies the CPU temperature that
 *  causes ALERT_L to assert if the CPU temperature is greater than or
 *  equal to the threshold.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] hitemp_thr: Specifies the high temperature threshold
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_set_hitemp_threshold(uint8_t soc_num,
					float hitemp_thr);

/**
 *  @brief This value set the low temperature threshold.
 *  The low temperature threshold specifies the CPU temperature that
 *  causes ALERT_L to assert if the CPU temperature is less than or
 *  equal to the threshold.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] lotemp_thr: Specifies the low temperature threshold
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_set_lotemp_threshold(uint8_t soc_num,
					float lotemp_thr);

/**
 *  @brief This value specifies the high temperature threshold.
 *  The high temperature threshold specifies the CPU temperature that
 *  causes ALERT_L to assert if the CPU temperature is greater than or
 *  equal to the threshold.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] hitemp_thr: Specifies the high temperature threshold
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_get_hitemp_threshold(uint8_t soc_num,
					float *hitemp_thr);

/**
 *  @brief This value specifies the low temperature threshold.
 *  The low temperature threshold specifies the CPU temperature that
 *  causes ALERT_L to assert if the CPU temperature is less than or
 *  equal to the threshold.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] lotemp_thr: Get the low temperature threshold
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_get_lotemp_threshold(uint8_t soc_num,
					float *lotemp_thr);

/**
 *  @brief SBTSI::CpuTempOffInt and SBTSI::CpuTempOffDec combine to specify
 *  the CPU temperature offset
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] temp_offset to get the offset value for temperature
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t read_sbtsi_cputempoffset(uint8_t soc_num,
				      float *temp_offset);

/**
 *  @brief SBTSI::CpuTempOffInt and SBTSI::CpuTempOffDec combine to set
 *  the CPU temperature offset
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] temp_offset to set the offset value for temperature
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t write_sbtsi_cputempoffset(uint8_t soc_num,
				       float temp_offset);

/**
 *  @brief Specifies the number of consecutive CPU temperature
 *  samples for which a temperature alert condition needs to remain valid
 *  before the corresponding alert bit is set.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] samples: Number of samples
 *  0h: 1 sample
 *  6h-1h: (value + 1) sample
 *  7h: 8 sample
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_set_alert_threshold(uint8_t soc_num,
				       uint8_t samples);

/**
 *  @brief Alert comparator mode enable
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] mode 0=> SBTSI::Status[TempHighAlert] &
 *  SBTSI::Status[TempLowAlert] are read-clear.
 *  1=>  SBTSI::Status[TempHighAlert] &
 *  SBTSI::Status[TempLowAlert] are read-only. ARA response
 *  disabled.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *
 *  @retval Non-zero is returned upon failure.
 */
oob_status_t sbtsi_set_alert_config(uint8_t soc_num,
				    uint8_t mode);
/** @} */  // end of SB-TSI Register access
/*****************************************************************************/

#endif  // INCLUDE_APML_TSI_H_
