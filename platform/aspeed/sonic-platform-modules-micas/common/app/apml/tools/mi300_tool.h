/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc. All rights reserved.
 */

#include <stdint.h>

/**
 *  @brief Displays the list of MI300 commands available for the module.
 *
 *  @details This function will print the list of MI300 commands for the module.
 *
 *  @param[in] exe_name input.
 *
 */
void get_mi300_tsi_commands(char *exec_name);

/**
 *  @brief Get the mi300 arguments.
 *
 *  @details This function will get the mi300 arguments.
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
oob_status_t parseesb_mi300_args(int argc, char **argv, uint8_t soc_num);

/**
 *  @brief Get the mi300 mailbox commands.
 *
 *  @details This function will get the mi300 mailbox commands.
 *
 *  @param[in] exe_name tool name.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
void get_mi300_mailbox_commands(char *exe_name);

/**
 *  @brief Get the mi300 specific tsi register descriptions.
 *
 *  @details This function will print mi300 specific tsi register
 *  descriptions.
 *
 *  @param[in] soc_num socket number.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
oob_status_t get_apml_mi300_tsi_register_descriptions(uint8_t soc_num);

/**
 *  @brief Get the mi300 mailbox commands summary.
 *
 *  @details This function will print mi300 specific mailbox
 *  commands summary.
 *
 *  @param[in] soc_num socket number.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
void get_mi_300_mailbox_cmds_summary(uint8_t soc_num);
/**
 *  @brief Get the HBM temperature status.
 *
 *  @details This function will HBM temperature status.
 *
 *  @param[in] soc_num socket number.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
oob_status_t get_hbm_temp_status(uint8_t soc_num);

void get_mi300c_mailbox_commands(char *exe_name);
