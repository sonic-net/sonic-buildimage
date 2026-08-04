#!/bin/sh

set -e

#
# Sign efi file with secret key and certificate
# - shim
# - grub
# - vmlinuz
#
print_usage() {
    cat <<EOF

$0: Usage
$0 -p <PRIVATE_KEY_PEM> -c <CERT_PEM> -e <EFI_FILE> -s <EFI_FILE_SIGNED>
Usage example: efi-sign.sh -p priv-key.pem -c pub-key.pem -e shimx64.efi -s shimx64-signed.efi

EOF
}

while getopts 'p:c:e:s:hv' flag; do
  case "${flag}" in
    p) PRIVATE_KEY_PEM="${OPTARG}" ;;
    c) CERT_PEM="${OPTARG}" ;;
    e) EFI_FILE="${OPTARG}" ;;
    s) EFI_FILE_SIGNED="${OPTARG}" ;;
    v) VERBOSE='true' ;;
    h) print_usage
       exit 1 ;;
  esac
done
if [ $OPTIND -eq 1 ]; then echo "no options were pass"; print_usage; exit 1 ;fi

[ -f "$PRIVATE_KEY_PEM" ] || {
    echo "Error: PRIVATE_KEY_PEM file does not exist: $PRIVATE_KEY_PEM"
    print_usage
    exit 1
}

[ -f "$CERT_PEM" ] || {
    echo "Error: CERT_PEM file does not exist: $CERT_PEM"
    print_usage
    exit 1
}

[ -f "$EFI_FILE" ] || {
    echo "Error: File for signing does not exist: $EFI_FILE"
    print_usage
    exit 1
}

if [ -z ${EFI_FILE_SIGNED} ]; then
    echo "ERROR: no arg named <EFI_FILE_SIGNED> supplied"
    print_usage
    exit 1
fi

echo "$0 signing $EFI_FILE with ${PRIVATE_KEY_PEM},  ${CERT_PEM} to create $EFI_FILE_SIGNED"
sbsign --key ${PRIVATE_KEY_PEM} --cert ${CERT_PEM} \
       --output ${EFI_FILE_SIGNED} ${EFI_FILE} || {
    echo "EFI sign error"
    exit 1
}
