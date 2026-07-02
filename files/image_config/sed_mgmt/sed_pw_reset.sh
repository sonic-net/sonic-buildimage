#!/bin/bash

# Reset SED password to the provided default password (e.g. from platform get_default_sed_password).
# Usage: sed_pw_reset.sh -a <tpm_bank_a> -b <tpm_bank_b> -p <default_password>

usage() {
    echo "Usage: $0 -a <tpm_bank_a> -b <tpm_bank_b> -p <default_password>"
    exit 1
}

TPM_BANK_A=""
TPM_BANK_B=""
DEFAULT_PW=""
while getopts "a:b:p:" opt; do
    case $opt in
        a) TPM_BANK_A=$OPTARG ;;
        b) TPM_BANK_B=$OPTARG ;;
        p) DEFAULT_PW=$OPTARG ;;
        *) usage ;;
    esac
done

if [ -z "$TPM_BANK_A" ] || [ -z "$TPM_BANK_B" ] || [ -z "$DEFAULT_PW" ]; then
    usage
fi

exec /usr/local/bin/sed_pw_change.sh -a "$TPM_BANK_A" -b "$TPM_BANK_B" -p "$DEFAULT_PW"
