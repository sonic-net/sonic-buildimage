/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Advanced Micro Devices, Inc.
 */

#ifndef __PCIESVC_CMD_H__
#define __PCIESVC_CMD_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

typedef enum pciesvc_cmdcode_e {
    PCIESVC_CMD_NOP                     = 0,
    PCIESVC_CMD_SET_LOG_LEVEL           = 1,
} pciesvc_cmdcode_t;

typedef enum pciesvc_cmdstatus_e {
    PCIESVC_CMDSTATUS_SUCCESS           = 0,
    PCIESVC_CMDSTATUS_UNKNOWN_CMD       = 1,
} pciesvc_cmdstatus_t;

typedef struct pciesvc_cmd_nop_s {
    uint32_t cmd;
} pciesvc_cmd_nop_t;

typedef struct pciesvc_cmdres_nop_s {
    uint32_t status;
} pciesvc_cmdres_nop_t;

typedef struct pciesvc_cmd_set_log_level_s {
    uint32_t cmd;
    uint32_t log_level;
} pciesvc_cmd_set_log_level_t;

typedef struct pciesvc_cmdres_set_log_level_s {
    uint32_t status;
    uint32_t old_level;
} pciesvc_cmdres_set_log_level_t;

typedef union pciesvc_cmd_u {
    uint32_t words[16];
    uint8_t cmd;
    pciesvc_cmd_nop_t nop;
    pciesvc_cmd_set_log_level_t set_log_level;
} pciesvc_cmd_t;

typedef union pciesvc_cmdres_u {
    uint32_t words[16];
    uint8_t status;
    pciesvc_cmdres_nop_t nop;
    pciesvc_cmdres_set_log_level_t set_log_level;
} pciesvc_cmdres_t;

#ifdef __cplusplus
}
#endif

#endif /* __PCIESVC_CMD_H__ */
