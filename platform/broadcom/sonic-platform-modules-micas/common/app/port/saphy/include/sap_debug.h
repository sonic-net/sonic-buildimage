/********************************************************************************
 * Copyright(C) 2020 Micas Network. All rights reserved.
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期        | 作者     |  修改记录
**===============================================================================
**| 2024/08/23  | zhoutenghui  |  创建该文件
**
*********************************************************************************/
#ifndef _SAP_DEBUG_H_
#define _SAP_DEBUG_H_

#include <stdio.h>
#include "sap_common.h"

typedef int (*sap_diag_func_t)(int unit, int port, char** args, int n_arg);

/* 由MACSDK来执行时，需要使用这个接口 */
int sap_debug_cmd_exec(int unit, int port, char** args, int n_arg);
/* 直接执行命令行时，需要使用这个接口 */
int sap_debug_line_exec(char *cmd, uint32_t cmd_len);
int sapcmd_service_init(void);

#endif /* _SAP_DEBUG_H_ */
