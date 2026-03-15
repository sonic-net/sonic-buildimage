#!/bin/bash

# Verify SED password in TPM banks A&B and recover the wrong bank.
# Reads TPM bank addresses from /etc/sonic/sed_config.conf (created at build).

export PATH=/run:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

SED_CONFIG="${SED_CONFIG:-/etc/sonic/sed_config.conf}"

source /usr/local/bin/sed_pw_utils.sh

if [ ! -f "$SED_CONFIG" ]; then
    log_info "$SED_CONFIG not found, skipping SED TPM Bank recovery"
    exit 0
fi

tpm_reg=$(grep '^tpm_bank_a=' "$SED_CONFIG" 2>/dev/null | cut -d= -f2- | tr -d ' \t')
tpm_reg_2=$(grep '^tpm_bank_b=' "$SED_CONFIG" 2>/dev/null | cut -d= -f2- | tr -d ' \t')

if [ -z "$tpm_reg" ] || [ -z "$tpm_reg_2" ]; then
    log_info "TPM Bank addresses not found in $SED_CONFIG, skipping SED TPM Bank recovery"
    exit 0
fi

sed_pw_bank_1=
sed_pw_bank_2=

recover_bank_logic() {
    validate_sed_pw $sed_pw_bank_1
    rc_sed_1=$?
    log_info "Validation SED TPM Bank 1 res: $rc_sed_1"
    if [ "$sed_pw_bank_1" = "$sed_pw_bank_2" ] && [ $rc_sed_1 -eq 0 ]; then
        log_info "SED TPM Banks are aligned and passed authentication"
        exit 0
    fi
    validate_sed_pw $sed_pw_bank_2
    rc_sed_2=$?
    log_info "Validation SED TPM Bank 2 res: $rc_sed_2"
    if [ "$rc_sed_1" -ne 0 ] && [ "$rc_sed_2" -ne 0 ]; then
        log_error "Both TPM Banks 1&2 not passing authentication"
        exit 1
    elif [ $rc_sed_1 -ne 0 ] && [ $rc_sed_2 -eq 0 ]; then
        log_warn "Authentication Bank 2 passed and Bank 1 failed."
        log_info "Storing PW from Bank 2 to Bank 1"
        store_sed_pwd_in_tpm $tpm_reg $sed_pw_bank_2
        log_info "Stored PW from Bank 2 to Bank 1 succeed"
    elif [ "$rc_sed_1" -eq 0 ] && [ "$rc_sed_2" -ne 0 ]; then
        log_warn "Authentication Bank 1 passed and Bank 2 failed."
        log_info "Storing PW from Bank 1 to Bank 2"
        store_sed_pwd_in_tpm $tpm_reg_2 $sed_pw_bank_1
        log_info "Stored PW from Bank 1 to Bank 2 succeed"
    fi
}

log_info "SED password TPM Banks validation start."

find_disk_name
if [ $? -ne 0 ]; then
    log_warn "Block device cannot be determined"
    exit 0
fi

if ! check_sed_ready; then
    log_warn "SED is not ready for operations"
    exit 0
fi

read_sed_pwd

if [ -z "$sed_pw_bank_1" ] && [ -z "$sed_pw_bank_2" ]; then
    log_warn "SED TPM Banks 1&2 are empty"
    exit 0
fi

recover_bank_logic

log_info "SED password Banks validation done."
exit 0
