/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */
#ifndef INCLUDE_APML_CPUID_MSR_H_
#define INCLUDE_APML_CPUID_MSR_H_

#include "apml_err.h"

/** \file esmi_cpuid_msr.h
 *  Header file for the APML library cpuid and msr read functions.
 *  All required function, structure, enum and protocol specific data etc.
 *  definitions should be defined in this header.
 *
 *  @details  This header file contains the following:
 *  APIs prototype of the APIs exported by the APML library.
 *  Description of the API, arguments and return values.
 *  The Error codes returned by the API.
 *
 */

/**
 * CPUID register indexes 0 for EAX, 1 for EBX, 2 ECX and 3 for EDX
 *
 */
typedef enum {
	EAX = 0,
	EBX,
	ECX,
	EDX
} cpuid_reg;

/**
 * @brief Read Proccessor Info
 */
struct processor_info {
	uint32_t family;  //!< Processor Family in hexa
	uint32_t model;	  //!< Processor Model in hexa
	uint32_t step_id; //!< Stepping Identifier in hexa
};

/**
 * @brief Platform Info instance
 */
extern struct processor_info plat_info[1];

/** @defgroup PROCESSOR_INFO using CPUID Register Access
 *  Below function provide interface to read the processor info using
 *  CPUID register.
 *  output from commmand will be written into the buffer.
 *  @{
 */

/**
 *  @brief Get the number of logical cores per socket
 *
 *  @details Get the processor vendor
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] vendor_id to get the processor vendor, 12 byte RO value
 *
 *  @retval uint32_t is returned upon successful call.
 *
 */
oob_status_t esmi_get_vendor_id(uint8_t soc_num, char *vendor_id);

/**
 *  @brief Get the number of logical cores per socket
 *
 *  @details Get the effective family, model and step_id of the processor.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] proc_info to get family, model & stepping identifier
 *
 *  @retval uint32_t is returned upon successful call.
 *
 */
oob_status_t esmi_get_processor_info(uint8_t soc_num,
				     struct processor_info *proc_info);

/**
 *  @brief Get the number of logical cores per socket
 *
 *  @details Get the total number of logical cores in a socket.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] logical_cores_per_socket is returned
 *
 *  @retval logical_cores_per_socket is returned upon successful call.
 *
 */
oob_status_t
esmi_get_logical_cores_per_socket(uint8_t soc_num,
				  uint32_t *logical_cores_per_socket);

/**
 *  @brief Get number of threads per core.
 *
 *  @details Get the number of threads per core.
 *  Supported platforms: \ref Fam-19h_Mod-00h-0Fh, \ref Fam-19h_Mod-10h-1Fh,
 *  \ref Fam-19h_Mod-90h-9Fh and \ref Fam-1Ah_Mod-00h-0Fh.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[inout] threads_per_core is returned
 *
 *  @retval threads_per_core is returned upon successful call.
 */
oob_status_t esmi_get_threads_per_core(uint8_t soc_num,
				       uint32_t *threads_per_core);


/** @} */  // end of PROCESSOR_INFO

/*****************************************************************************/
/** @defgroup ProcessorAccess SB_RMI Read Processor Register Access
 *  Below function provide interface to read the SB-RMI MCA MSR register.
 *  output from MCA MSR commmand will be written into the buffer.
 *  @{
 */

/**
 *  @brief Read the MCA MSR register for a given thread.
 *
 *  @details Given a @p thread and SB-RMI register command, this function reads
 *  msr value.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] thread is a particular thread in the system.
 *
 *  @param[in] msraddr MCA MSR register to read
 *
 *  @param[out] buffer is to hold the return output of msr value.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 */
oob_status_t esmi_oob_read_msr(uint8_t soc_num,
			       uint32_t thread, uint32_t msraddr,
			       uint64_t *buffer);

/** @} */  // end of ProcessorAccess

/*****************************************************************************/
/** @defgroup cpuidAccess SB-RMI CPUID Register Access
 *  Below function provide interface to get the CPUID access via the SBRMI.
 *
 *  Output from CPUID commmand will be written into registers eax, ebx, ecx
 *  and edx.
 *  @{
 */

/**
 *  @brief Read CPUID functionality for a particular thread in a system.
 *
 *  @details Given a @p thread , @p eax as function input and @p ecx as
 *  extended function input. this function will get the cpuid details
 *  for a particular thread in a pointer to @p eax, @p ebx, @p ecx, @p edx
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] thread is a particular thread in the system.
 *
 *  @param[inout] eax a pointer uint32_t to get eax value
 *
 *  @param[out] ebx a pointer uint32_t to get ebx value
 *
 *  @param[inout] ecx a pointer uint32_t to get ecx value
 *
 *  @param[out] edx a pointer uint32_t to get edx value
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 */
oob_status_t esmi_oob_cpuid(uint8_t soc_num,
			    uint32_t thread, uint32_t *eax, uint32_t *ebx,
			    uint32_t *ecx, uint32_t *edx);

/**
 *  @brief Read eax register on CPUID functionality.
 *
 *  @details Given a @p thread, @p fn_eax as function and @p fn_ecx as
 *  extended function input, this function will get the cpuid
 *  details for a particular thread at @p eax.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] thread is a particular thread in the system.
 *
 *  @param[in] fn_eax cpuid function
 *
 *  @param[in] fn_ecx cpuid extended function
 *
 *  @param[out] eax is to read eax from cpuid functionality.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 */
oob_status_t esmi_oob_cpuid_eax(uint8_t soc_num,
				uint32_t thread, uint32_t fn_eax,
				uint32_t fn_ecx, uint32_t *eax);

/**
 *  @brief Read ebx register on CPUID functionality.
 *
 *  @details Given a @p thread, @p fn_eax as function and @p fn_ecx as
 *  extended function input, this function will get the cpuid
 *  details for a particular thread at @p ebx.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] thread is a particular thread in the system.
 *
 *  @param[in] fn_eax cpuid function
 *
 *  @param[in] fn_ecx cpuid extended function
 *
 *  @param[out] ebx is to read ebx from cpuid functionality.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 */
oob_status_t esmi_oob_cpuid_ebx(uint8_t soc_num,
				uint32_t thread, uint32_t fn_eax,
				uint32_t fn_ecx, uint32_t *ebx);

/**
 *  @brief Read ecx register on CPUID functionality.
 *
 *  @details Given a @p thread, @p fn_eax as function and @p fn_ecx as
 *  extended function input, this function will get the cpuid
 *  details for a particular thread at @p ecx.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] thread is a particular thread in the system.
 *
 *  @param[in] fn_eax cpuid function
 *
 *  @param[in] fn_ecx cpuid extended function
 *
 *  @param[out] ecx is to read ecx from cpuid functionality.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 */
oob_status_t esmi_oob_cpuid_ecx(uint8_t soc_num,
				uint32_t thread, uint32_t fn_eax,
				uint32_t fn_ecx, uint32_t *ecx);

/**
 *  @brief Read edx register on CPUID functionality.
 *
 *  @details Given a @p thread, @p fn_eax as function and @p fn_ecx as
 *  extended function input, this function will get the cpuid
 *  details for a particular thread at @p edx.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[in] thread is a particular thread in the system.
 *
 *  @param[in] fn_eax cpuid function
 *
 *  @param[in] fn_ecx cpuid extended function
 *
 *  @param[out] edx is to read edx from cpuid functionality.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 */
oob_status_t esmi_oob_cpuid_edx(uint8_t soc_num,
				uint32_t thread, uint32_t fn_eax,
				uint32_t fn_ecx, uint32_t *edx);

/**
 *  @brief Read max threads per L3 cache.
 *
 *  @details Reads max threads per L3 cache.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @param[out] threads_l3 threads per L3 cache.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 */
oob_status_t read_max_threads_per_l3(uint8_t soc_num, uint32_t *threads_l3);

/** @} */  // end of cpuidAccess

/*****************************************************************************/
#endif  // INCLUDE_APML_CPUID_MSR_H_

