#!/bin/bash
# This script is signing boot components: shim, mmx, grub, kernel and kernel modules in development env.

## Enable debug output for script & exit code when failing occurs
set -x -e

print_usage() {
    cat <<EOF

$0: Usage
$0 -r <FS_ROOT> -l <LINUX_KERNEL_VERSION> -c <PEM_CERT> -p <PEM_PRIV_KEY>

EOF
}

clean_file() {
    if [ -f $1 ]; then
        echo "clean old file named: $1"
        echo "rm -f $1"
        rm -f $1
    fi
}

while getopts 'a:r:l:c:p:hv' flag; do
  case "${flag}" in
    a) CONFIGURED_ARCH="${OPTARG}" ;;
    r) FS_ROOT="${OPTARG}" ;;
    l) LINUX_KERNEL_VERSION="${OPTARG}" ;;
    c) PEM_CERT="${OPTARG}" ;;
    p) PEM_PRIV_KEY="${OPTARG}" ;;
    v) VERBOSE='true' ;;
    h) print_usage
       exit 1 ;;
  esac
done
if [ $OPTIND -eq 1 ]; then echo "no options were pass"; print_usage; exit 1 ;fi

echo "$0 signing & verifying EFI files and Kernel Modules start ..."

if [ -z ${CONFIGURED_ARCH} ]; then
    echo "ERROR: CONFIGURED_ARCH=${CONFIGURED_ARCH} is empty"
    print_usage
    exit 1
fi

if [ -z ${FS_ROOT} ]; then
    echo "ERROR: FS_ROOT=${FS_ROOT} is empty"
    print_usage
    exit 1
fi

if [ -z ${LINUX_KERNEL_VERSION} ]; then
    echo "ERROR: LINUX_KERNEL_VERSION=${LINUX_KERNEL_VERSION} is empty"
    print_usage
    exit 1
fi

if [ ! -f "${PEM_CERT}" ]; then
    echo "ERROR: PEM_CERT=${PEM_CERT} file does not exist"
    print_usage
    exit 1
fi

if [ ! -f "${PEM_PRIV_KEY}" ]; then
    echo "ERROR: PEM_PRIV_KEY=${PEM_PRIV_KEY} file does not exist"
    print_usage
    exit 1
fi

# efi-sign.sh is used to sign: shim, mmx, grub, and kernel (vmlinuz)
EFI_SIGNING=scripts/efi-sign.sh

# ######################################
# Signing EFI files: mm, shim, grub
# #####################################
efi_file_list=$(sudo find ${KERNEL_MODULES_DIR} -name "*.efi")

for efi in $efi_file_list
do
    # grep filename from full path
    efi_filename=$(echo $efi | grep -o '[^/]*$')

    if echo $efi_filename | grep -e "shim" -e "grub" -e "mm"; then

        clean_file ${efi}-signed

        echo "signing efi file - full path: ${efi} filename: ${efi_filename}"
        echo "sudo ${EFI_SIGNING} -p $PEM_PRIV_KEY -c $PEM_CERT -e ${efi} -s ${efi}-signed"
        ${EFI_SIGNING} -p $PEM_PRIV_KEY -c $PEM_CERT -e ${efi} -s ${efi}-signed

        # cp shim & mmx signed files to boot directory in the fs.
        cp ${efi}-signed $FS_ROOT/boot/${efi_filename}

        # verifying signature of mm & shim efi files.
        ./scripts/secure_boot_signature_verification.sh -c $PEM_CERT -e $FS_ROOT/boot/${efi_filename}
    fi
done

######################
## vmlinuz signing
######################

CURR_VMLINUZ=$FS_ROOT/boot/vmlinuz-${LINUX_KERNEL_VERSION}-sonic-${CONFIGURED_ARCH}

# clean old files
clean_file ${CURR_VMLINUZ}-signed

echo "signing ${CURR_VMLINUZ} .."
${EFI_SIGNING} -p $PEM_PRIV_KEY -c $PEM_CERT -e ${CURR_VMLINUZ} -s ${CURR_VMLINUZ}-signed

# rename signed vmlinuz with the name vmlinuz without signed suffix
mv ${CURR_VMLINUZ}-signed ${CURR_VMLINUZ}

./scripts/secure_boot_signature_verification.sh -c $PEM_CERT -e ${CURR_VMLINUZ}

#########################
# Kernel Modules signing
#########################
./scripts/signing_kernel_modules.sh -l ${LINUX_KERNEL_VERSION} -c ${PEM_CERT} -p ${PEM_PRIV_KEY} -k ${FS_ROOT}/usr/lib/modules

echo "$0 signing & verifying EFI files and Kernel Modules DONE"
