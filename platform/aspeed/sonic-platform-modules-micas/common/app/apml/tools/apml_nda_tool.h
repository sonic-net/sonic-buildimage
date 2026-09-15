/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */

#include <stdint.h>
#include <esmi_oob/esmi_cpuid_msr.h>

/**
 *  @brief Displays the list of NDA commands available for the module.
 *
 *  @details This function will print the list of NDA commands for the module.
 *
 *  @param[in] exe_name input.
 *
 *  @param[in] command input command.
 *
 *  @param[in] p_type platform type.
 *
 */
void show_module_nda_commands(char *exe_name, char *command,
			      uint8_t p_type);

/**
 *  @brief Get the nda arguments.
 *
 *  @details This function will get the nda arguments.
 *
 *  @param[in] argc argument count.
 *
 *  @param[in] argv argument values.
 *
 *  @param[in] soc_num Socket index.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
oob_status_t parseesb_nda_args(int argc, char **argv, uint8_t soc_num);

