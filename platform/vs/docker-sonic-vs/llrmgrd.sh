#!/usr/bin/env bash

LLR_PROFILE_LOOKUP_FILE="/usr/share/sonic/hwsku/llr_profile_lookup.ini"

if [[ -r "${LLR_PROFILE_LOOKUP_FILE}" ]]; then
    exec /usr/bin/llrmgrd -l "${LLR_PROFILE_LOOKUP_FILE}"
else
    # If no vendor lookup file then profiles can be created through CONFIG_DB
    exec /usr/bin/llrmgrd
fi
