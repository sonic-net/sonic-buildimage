#ifndef ICCP_UTILS_H_
#define ICCP_UTILS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mlacp_tlv.h"

int iccp_exec_command(char *const argv[], bool suppress_output);
bool iccp_is_interface_name_valid(const char *name, size_t name_len);
int iccp_get_tlv_type(
    const char *msg_buf,
    size_t msg_len,
    uint16_t *tlv_type);
int iccp_validate_message(
    const char *msg_buf,
    size_t msg_len,
    uint16_t msg_type);
int iccp_validate_tlv(
    const char *msg_buf,
    size_t msg_len,
    size_t minimum_tlv_len,
    uint16_t *tlv_type,
    size_t *tlv_len);
int iccp_parse_agg_config_tlv(
    const char *msg_buf,
    size_t msg_len,
    mLACPAggConfigTLV *portconf);

#endif /* ICCP_UTILS_H_ */
