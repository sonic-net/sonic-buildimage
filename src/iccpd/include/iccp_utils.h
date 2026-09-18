#ifndef ICCP_UTILS_H_
#define ICCP_UTILS_H_

#include <stdbool.h>
#include <stddef.h>

#include "mlacp_tlv.h"

int iccp_exec_command(char *const argv[], bool suppress_output);
bool iccp_is_interface_name_valid(const char *name, size_t name_len);
int iccp_parse_agg_config_tlv(
    const char *msg_buf,
    size_t msg_len,
    mLACPAggConfigTLV *portconf);

#endif /* ICCP_UTILS_H_ */
