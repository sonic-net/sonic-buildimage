#!/bin/bash
# This script is signing kernel modules by using sign-file tool

usage() {
    cat <<EOF
$0: # Display Help
$0 -l <LINUX_KERNEL_VERSION> -c <PEM_CERT> -p <PEM_PRIVATE_KEY> -s <LOCAL_SIGN_FILE> -e <LOCAL_EXTRACT_CERT> -k <KERNEL_MODULES_DIR>
Sign kernel modules in <KERNEL_MODULES_DIR> using private & public keys.

Parameters description:
LINUX_KERNEL_VERSION
PEM_CERT                             public key (pem format)
PEM_PRIVATE_KEY                      private key (pem format)
LOCAL_SIGN_FILE                      path of the sign-file tool for signing Kernel Modules, if the value is empty it will used the sign-file installed in /usr/lib/linux-kbuild-<version>/scripts
LOCAL_EXTRACT_CERT                   path of the extract-cert tool for Extract X.509 certificate, if the value is empty it will used the extract-cert installed in /usr/lib/linux-kbuild-<version>/scripts
KERNEL_MODULES_DIR                   root directory of all the kernel modules to be sign by the script, if the value empty it will use the call script location as root.

Runs examples:
1. ./scripts/signing_kernel_modules.sh -l 5.10.0-8-2 -c cert.pem -p priv-key.pem
2. ./scripts/signing_kernel_modules.sh -l 5.10.0-8-2 -c cert.pem -p priv-key.pem -k fsroot-mellanox -e /usr/lib/linux-kbuild-5.10/scripts/extract-cert -s /usr/lib/linux-kbuild-5.10/scripts/sign-file
EOF
}

while getopts 'l:c:p:k:s:e:hv' flag; do
  case "${flag}" in
    l) LINUX_KERNEL_VERSION="${OPTARG}" ;;
    c) PEM_CERT="${OPTARG}" ;;
    p) PEM_PRIVATE_KEY="${OPTARG}" ;;
    k) KERNEL_MODULES_DIR="${OPTARG}" ;;
    s) LOCAL_SIGN_FILE="${OPTARG}" ;;
    e) LOCAL_EXTRACT_CERT="${OPTARG}" ;;
    v) VERBOSE='true' ;;
    h) usage
       exit 1 ;;
  esac
done
if [ $OPTIND -eq 1 ]; then echo "no options were pass"; usage; exit 1 ;fi

if [ -z ${LINUX_KERNEL_VERSION} ]; then
    echo "ERROR: LINUX_KERNEL_VERSION arg1 is empty"
    usage
    exit 1
fi

if [ ! -f ${PEM_CERT} ]; then
    echo "ERROR: arg2 PEM_CERT=${PEM_CERT} file does not exist"
    usage
    exit 1
fi

if [ ! -f ${PEM_PRIVATE_KEY} ]; then
    echo "ERROR: arg3 PEM_PRIVATE_KEY=${PEM_PRIVATE_KEY} file does not exist"
    usage
    exit 1
fi

if [ -z ${LOCAL_SIGN_FILE} ]; then
    LOCAL_SIGN_FILE="/usr/lib/linux-kbuild-${LINUX_KERNEL_VERSION}/scripts/sign-file"
fi

if [ ! -f ${LOCAL_SIGN_FILE} ]; then
    echo "ERROR: LOCAL_SIGN_FILE=${LOCAL_SIGN_FILE} file does not exist"
    usage
    exit 1
fi

if [ -z ${LOCAL_EXTRACT_CERT} ]; then
    LOCAL_EXTRACT_CERT="/usr/lib/linux-kbuild-${LINUX_KERNEL_VERSION}/certs/extract-cert"
fi

if [ ! -f ${LOCAL_EXTRACT_CERT} ]; then
    echo "ERROR: LOCAL_EXTRACT_CERT=${LOCAL_EXTRACT_CERT} file does not exist"
    usage
    exit 1
fi

if [ ! -d "$KERNEL_MODULES_DIR" ]; then
    # If the user do not provide a KERNEL_MODULES_DIR, the script is going to search in the script call path for Kernel modules.
    KERNEL_MODULES_DIR="./"
    echo "KERNEL_MODULES_DIR set to default path: $KERNEL_MODULES_DIR"
fi

# find all the kernel modules.
modules_list=$(find ${KERNEL_MODULES_DIR} -name "*.ko")

dev_certs_tmp_folder="/tmp/dev_kmod_sign"

# clean env
if [ -d ${dev_certs_tmp_folder} ]; then
    rm -r ${dev_certs_tmp_folder}
fi

mkdir -p ${dev_certs_tmp_folder}
local_sign_key="${dev_certs_tmp_folder}/$(basename $PEM_PRIVATE_KEY)"
local_sign_cert="${dev_certs_tmp_folder}/$(basename $PEM_CERT)"

# Combine cert for module signing
echo "keys concat: cat ${PEM_PRIVATE_KEY} ${PEM_CERT} > ${local_sign_key}"
cat ${PEM_PRIVATE_KEY} ${PEM_CERT} > ${local_sign_key}

# Extract x509 cert in corect format
echo "create x509 cert: ${LOCAL_EXTRACT_CERT} ${local_sign_key} ${local_sign_cert}"
${LOCAL_EXTRACT_CERT} ${local_sign_key} ${local_sign_cert}

# Do sign for each found module
kernel_modules_cnt=0
for mod in $modules_list
do
    echo "signing module named: ${mod} .."
    echo "${LOCAL_SIGN_FILE} sha512 ${local_sign_key} ${local_sign_cert} ${mod}"
    kernel_modules_cnt=$((kernel_modules_cnt+1))
    ${LOCAL_SIGN_FILE} sha512 ${local_sign_key} ${local_sign_cert} ${mod}

    # check Kernel module is signed.
    if ! grep -q "~Module signature appended~" "${mod}"; then
        echo "Error: Kernel module=${mod} have no signature appened."
        exit 1
    fi
done

echo "Num of kernel modules signed: kernel_modules_cnt=$kernel_modules_cnt"
echo "$0: All Kernel Modules SIGNED OK."