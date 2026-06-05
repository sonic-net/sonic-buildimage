/**
 * An improved version of stringify.h that follows cleaner conventions. It converts data to a string.
 */
#ifndef STRIFY_H
#define STRIFY_H

#include "sdk.h"

const char* strify_timestamp_now(char buffer[64]);
const char* strify_timedelta(double seconds, bool show_ms, char buffer[64]);
const char* strify_time_iso8601(double unix_time, char buffer[64]);
const char* strify_fec_error_type(CredoFecErrorType_t error_type);
const char* strify_port_connection_mode(CredoPortConnectionMode_t mode);
const char* strify_port_mode(CredoPortMode_t mode);
const char* strify_prbs_training_rx_status(CredoPrbsTrainingStatus_t status);
const char* strify_fec_type(CredoFecType_t fec_type);

#define STRIFY_TIMEDELTA(seconds, show_ms) strify_timedelta(seconds, show_ms, (char[64]){0})
#define STRIFY_TIMESTAMP_NOW()             strify_timestamp_now((char[64]){0})
#define STRIFY_TIME_ISO8601(unix_time)     strify_time_iso8601(unix_time, (char[64]){0})

#endif
