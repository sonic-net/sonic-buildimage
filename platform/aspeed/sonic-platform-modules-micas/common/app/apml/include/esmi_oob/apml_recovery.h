/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#ifndef INCLUDE_APML_RECOVER_H_
#define INCLUDE_APML_RECOVER_H_

/** \file apml_recovery.h
 *  Header file for the APML recovery flow
 */

/**
 * @brief APML Client Devices
 */
typedef enum {
	DEV_SBRMI = 0x0,
	DEV_SBTSI,
} apml_client;


/**
 *  @brief Recover the APML client device for the given socket.
 *
 *  @details This function will recover the APML client device and
 *  returns successful on recovery or error if recovery is unsuccessful
 *
 *  NOTE: This fix is a software workaround for the erratum,
 *  Erratum:1444, "Advanced Platform Management Link (APML)
 *  May Cease to Function After Incomplete Read Transaction"
 *  Further details can be found in mentioned tech doc
 *  https://www.amd.com/system/files/TechDocs/57095-PUB_1.00.pdf
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] client DEV_SBRMI[0]/DEV_SBTSI[1] enum: apml_client
 *
 *  @retval ::OOB_SUCCESS is returned upon successful recovery
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t apml_recover_dev(uint8_t soc_num, uint8_t client);
#endif	//INCLUDE_APML_RECOVERY_H_
