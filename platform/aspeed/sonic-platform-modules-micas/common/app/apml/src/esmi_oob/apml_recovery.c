/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <esmi_oob/apml.h>
#include <esmi_oob/apml_recovery.h>
#include <esmi_oob/esmi_rmi.h>
#include <esmi_oob/esmi_tsi.h>

#define MAX_RETRY	20
#define CONFIG_MASK	0x1
#define CTRL_MASK	0x2
/* Recovery wait time of 1000 micro seconds */
#define REC_WAIT	1000

static oob_status_t apml_recover_sbrmi(uint8_t soc_num)
{
	int retry = MAX_RETRY;
	oob_status_t ret = 0;
	uint8_t config = 0, rev = 0;

	/* Verify that the SBTSI is working */
	ret = read_sbtsi_revision(soc_num, &rev);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Read the curent configuration before writing to bit 0 */
	ret = read_sbtsi_config(soc_num, &config);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Write 1 to bit 0 of sbtsi config register to notify recovery of sbrmi */
	config |= CONFIG_MASK;
	ret = esmi_oob_tsi_write_byte(soc_num, SBTSI_CONFIGWR, config);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Wait for the sbtsi config reg bit 0 to set to 0 */
	do {
		ret = read_sbtsi_config(soc_num, &config);
		if (ret != OOB_SUCCESS)
			return ret;

		if (!(config & CONFIG_MASK))
			break;

		/* Sleep for 1 millsecond */
		usleep(REC_WAIT);

	} while (retry--);

	/* Verify if sbrmi is recoverd */
	ret = read_sbrmi_revision(soc_num, &rev);
	if (ret != OOB_SUCCESS)
		return ret;
	return ret;
}

static oob_status_t apml_recover_sbtsi(uint8_t soc_num)
{
	int retry = MAX_RETRY;
	oob_status_t ret = 0;
	uint8_t control= 0, rev = 0;

	/* Verify that the SBRMI is working */
	ret = read_sbrmi_revision(soc_num, &rev);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Read the curent configuration before writing to bit 1 */
	ret = read_sbrmi_control(soc_num, &control);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Write 1 to bit 1 of sbrmi control register to notify recovery of sbtsi */
	control |= CTRL_MASK;
	ret = esmi_oob_rmi_write_byte(soc_num, SBRMI_CONTROL, control);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Wait for the control reg bit 1 to set to 0 */
	do {
		ret = read_sbrmi_control(soc_num, &control);
		if (ret != OOB_SUCCESS)
			return ret;

		if (!(control & CTRL_MASK))
			break;
		/* sleep for 1 millisecond */
		usleep(REC_WAIT);

	} while (retry--);

	/* Verify if sbtsi is recoverd */
	ret = read_sbtsi_revision(soc_num, &rev);
	if (ret != OOB_SUCCESS)
		return ret;
	return ret;
}

oob_status_t apml_recover_dev(uint8_t soc_num, uint8_t client)
{
	oob_status_t ret = 0;

	/* Verify recovery require in sbrmi or sbtsi */
	if (client == DEV_SBRMI) {
		ret = apml_recover_sbrmi(soc_num);
		if (ret != OOB_SUCCESS)
			return ret;
	} else if (client == DEV_SBTSI) {
		ret = apml_recover_sbtsi(soc_num);
		if (ret != OOB_SUCCESS)
			return ret;
	} else {
		ret = OOB_INVALID_INPUT;
	}

	return ret;
}
