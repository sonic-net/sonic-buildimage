#!/bin/bash

# Store new SED password in TPM banks A/B and in the SED.
# Algorithm to store in TPM and SED
# There are 2 Banks where to store new password.
# 0. Read TPM Banks 1&2
# 1. The code will first validate Banks.
# 2. The code will store the new password in the bank that succeeded first in decrypting the SED
# 3. The code will store the new password in SED.
# 4. The code will store the secondary Bank with the new password.

sed_pw_bank_1=
sed_pw_bank_2=
disk_name=
tpm_reg_next=

usage() {
    echo "Usage: $0 -a <tpm_bank_a> -b <tpm_bank_b> -p <new_sed_password>"
    exit 1
}

TPM_BANK_A=""
TPM_BANK_B=""
SED_NEW_PW=""
while getopts "a:b:p:" opt; do
    case $opt in
        a) TPM_BANK_A=$OPTARG ;;
        b) TPM_BANK_B=$OPTARG ;;
        p) SED_NEW_PW=$OPTARG ;;
        *) usage ;;
    esac
done

if [ -z "$TPM_BANK_A" ] || [ -z "$TPM_BANK_B" ] || [ -z "$SED_NEW_PW" ]; then
    usage
fi

tpm_reg=$TPM_BANK_A
tpm_reg_2=$TPM_BANK_B
source /usr/local/bin/sed_pw_utils.sh

log_info "SED new password start."

find_disk_name
if [ $? -ne 0 ]; then
    log_warn "Block device cannot be determined"
    exit 1
fi

if ! check_sed_ready; then
    log_warn "SED is not ready for operations"
    exit 1
fi

read_sed_pwd

validate_sed_pw $sed_pw_bank_1
res_val_1=$?
validate_sed_pw $sed_pw_bank_2
res_val_2=$?

log_info "Old Password Validation - Bank 1: $res_val_1, Bank 2: $res_val_2"

if [ $res_val_1 -eq 0 ] && [ $res_val_2 -eq 0 ]; then
    old_good_pw=$sed_pw_bank_1
    curr_tpm_reg=$tpm_reg
    FLAG_NEXT_BANK_TO_STORE='B'
elif [ $res_val_1 -ne 0 ] && [ $res_val_2 -eq 0 ]; then
    old_good_pw=$sed_pw_bank_2
    curr_tpm_reg=$tpm_reg
    FLAG_NEXT_BANK_TO_STORE='B'
elif [ $res_val_1 -eq 0 ] && [ $res_val_2 -ne 0 ]; then
    old_good_pw=$sed_pw_bank_1
    curr_tpm_reg=$tpm_reg_2
    FLAG_NEXT_BANK_TO_STORE='A'
else
    log_error "Validation of old password in both SED Banks failed."
    exit 1
fi

store_sed_pwd_in_tpm $curr_tpm_reg $SED_NEW_PW
res_store_sed=$?

if [ $res_store_sed -eq 0 ]; then
    log_info "sedutil-cli --setadmin1pwd <old_good_pw> <SED_NEW_PW> ${disk_name}"
    sedutil-cli --setadmin1pwd $old_good_pw $SED_NEW_PW ${disk_name}
    if [ $? -ne 0 ]; then
        log_error "sedutil-cli --setadmin1pwd failed"
        exit 1
    fi
else
    log_error "Failed when storing new password in TPM"
    exit 1
fi

if [ "$FLAG_NEXT_BANK_TO_STORE" = "A" ]; then
    log_info "Storing Bank A"
    tpm_reg_next=$tpm_reg
elif [ "$FLAG_NEXT_BANK_TO_STORE" = "B" ]; then
    log_info "Storing Bank B"
    tpm_reg_next=$tpm_reg_2
else
    log_error "No secondary Bank was updated."
    exit 1
fi

store_sed_pwd_in_tpm $tpm_reg_next $SED_NEW_PW
if [ $? -ne 0 ]; then
    log_error "Store new password in the secondary Bank failed."
    exit 1
fi
log_info "SED new password done"
