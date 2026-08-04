#!/bin/bash
# This Script is verifying the efi file signature by using sbverify.
# In addition, is verifying that kernel modules a directory contained a signature.
# Note: Kernel Module verification is not checking that the signature is correct, but its checking that the Kernel Modules have one.

EFI_FILE=''
KERNEL_MODULES_DIR=''
CERT_PEM=''
VERBOSE='false'

print_usage() {
    cat <<EOF

$0: Usage
$0 -e <EFI_FILE/EFI_DIR> -c <CERT_PEM> -k <KERNEL_MODULES_DIR>
Run Example: secure_boot_signature_verification.sh -e shimx64.efi -c pub-key.pem -k fsroot-mellanox
Run Example: secure_boot_signature_verification.sh -e /boot/efi_dir -c pub-key.pem -k fsroot-mellanox

EOF
}

verify_efi(){
    cert_pem=$1
    efi_file=$2
    echo "sbverify --cert $cert_pem $efi_file"
    sbverify --cert $cert_pem $efi_file || {
        echo "sbverify error with $efi_file"
        exit 1
    }
    echo "$efi_file signed OK."
}

while getopts 'e:k:c:hv' flag; do
  case "${flag}" in
    e) EFI_FILE="${OPTARG}" ;;
    k) KERNEL_MODULES_DIR="${OPTARG}" ;;
    c) CERT_PEM="${OPTARG}" ;;
    v) VERBOSE='true' ;;
    h) print_usage
       exit 1 ;;
  esac
done
if [ $OPTIND -eq 1 ]; then echo "no options were pass"; print_usage; exit 1 ;fi

if [ -d "$EFI_FILE" ];then
    [ -f "$CERT_PEM" ] || {
        echo "Error: option '-c' incorrect, file: certificate=$CERT_PEM does not exist"
        print_usage
        exit 1
    }

    # find all efi files.
    efi_file_list=$(sudo find ${EFI_FILE} -name "*.efi")
    for efi_file in $efi_file_list
    do
        echo "verifying efi_file named: ${efi_file} .."
        verify_efi $CERT_PEM ${efi_file}
    done
    echo "$0: All EFI files SIGNED OK."
fi

if [ -f "$EFI_FILE" ]; then
    [ -f "$CERT_PEM" ] || {
        echo "Error: option '-c' incorrect, file: certificate=$CERT_PEM does not exist"
        print_usage
        exit 1
    }
    verify_efi $CERT_PEM $EFI_FILE
fi

if [ ! -z "$KERNEL_MODULES_DIR" -a -d "$KERNEL_MODULES_DIR" ]; then
    # Condition checking that all the kernel modules in the KERNEL_MODULES_DIR contain a signature if specified.

    # find all the kernel modules.
    modules_list=$(sudo find ${KERNEL_MODULES_DIR} -name "*.ko")

    # Do sign for each found module
    kernel_modules_cnt=0
    for mod in $modules_list
    do
        # check Kernel module is signed.
        if ! grep -q "~Module signature appended~" "${mod}"; then
            echo "Error: Kernel module=${mod} have no signature appened."
            exit 1
        fi

        if [ $VERBOSE = 'true' ]; then
            echo "kernel module named=${mod} have signature appended."
        fi

        kernel_modules_cnt=$((kernel_modules_cnt+1))
    done
    echo "Num of kernel modules signed: kernel_modules_cnt=$kernel_modules_cnt"
    echo "$0: All Kernel Modules SIGNED OK."
fi

