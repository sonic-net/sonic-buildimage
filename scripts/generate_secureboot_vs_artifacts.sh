#!/bin/bash

set -euo pipefail

WORKDIR=""
OVMF_VARS="/usr/share/OVMF/OVMF_VARS_4M.fd"
OUTPUT_VARS=""
BUILD_ID="${BUILD_BUILDID:-local}"
OWNER_GUID="8c7f3d10-9b67-4e17-8d8a-73e6f9725b1d"
REUSE_KEYS=false

usage() {
    cat <<EOF
Usage: $0 --workdir DIR --output-vars FILE [options]

Options:
  --ovmf-vars FILE  Clean OVMF variable-store template
  --build-id ID     Identifier included in certificate common names
  --owner-guid GUID Signature owner GUID stored in UEFI signature lists
  --reuse-keys      Reuse PK/KEK/DB certificates already in the work directory
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --workdir)
            WORKDIR="$2"
            shift 2
            ;;
        --ovmf-vars)
            OVMF_VARS="$2"
            shift 2
            ;;
        --output-vars)
            OUTPUT_VARS="$2"
            shift 2
            ;;
        --build-id)
            BUILD_ID="$2"
            shift 2
            ;;
        --owner-guid)
            OWNER_GUID="$2"
            shift 2
            ;;
        --reuse-keys)
            REUSE_KEYS=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ -z "$WORKDIR" || -z "$OUTPUT_VARS" ]]; then
    usage >&2
    exit 2
fi

for tool in openssl cert-to-efi-sig-list sign-efi-sig-list virt-fw-vars; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "Missing required tool: $tool" >&2
        exit 1
    }
done

if [[ ! -f "$OVMF_VARS" ]]; then
    echo "OVMF variable-store template is missing: $OVMF_VARS" >&2
    exit 1
fi

mkdir -p "$WORKDIR" "$(dirname "$OUTPUT_VARS")"
WORKDIR="$(realpath "$WORKDIR")"
OVMF_VARS="$(realpath "$OVMF_VARS")"
OUTPUT_VARS="$(realpath -m "$OUTPUT_VARS")"
cd "$WORKDIR"
umask 077

PK_CN="SONiC CI PK build ${BUILD_ID}"
KEK_CN="SONiC CI KEK build ${BUILD_ID}"
DB_CN="SONiC CI DB build ${BUILD_ID}"

if [[ "$REUSE_KEYS" == false ]]; then
    openssl genrsa -out PK.key 4096
    openssl req -new -x509 -sha256 -days 3650 \
        -key PK.key -out PK.crt \
        -subj "/CN=${PK_CN}/" \
        -addext "basicConstraints=critical,CA:TRUE" \
        -addext "keyUsage=critical,keyCertSign,cRLSign" \
        -addext "subjectKeyIdentifier=hash"

    openssl genrsa -out KEK.key 4096
    cat > KEK.cnf <<EOF
[ req ]
prompt = no
distinguished_name = dn
[ dn ]
CN = ${KEK_CN}
[ v3_ca ]
basicConstraints = critical,CA:TRUE,pathlen:0
keyUsage = critical,keyCertSign,cRLSign
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always,issuer
EOF
    openssl req -new -sha256 -key KEK.key -out KEK.csr -config KEK.cnf
    openssl x509 -req -sha256 -days 3650 \
        -in KEK.csr \
        -CA PK.crt -CAkey PK.key -CAcreateserial \
        -out KEK.crt \
        -extfile KEK.cnf -extensions v3_ca

    openssl genrsa -out DB.key 4096
    cat > DB.cnf <<EOF
[ req ]
prompt = no
distinguished_name = dn
[ dn ]
CN = ${DB_CN}
[ v3_db ]
basicConstraints = critical,CA:FALSE
keyUsage = critical,digitalSignature
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
EOF
    openssl req -new -sha256 -key DB.key -out DB.csr -config DB.cnf
    openssl x509 -req -sha256 -days 365 \
        -in DB.csr \
        -CA KEK.crt -CAkey KEK.key -CAcreateserial \
        -out DB.crt \
        -extfile DB.cnf -extensions v3_db

    cert-to-efi-sig-list PK.crt PK.esl
    sign-efi-sig-list -k PK.key -c PK.crt PK PK.esl PK.auth

    cert-to-efi-sig-list KEK.crt KEK.esl
    sign-efi-sig-list -k PK.key -c PK.crt KEK KEK.esl KEK.auth

    cert-to-efi-sig-list DB.crt DB.esl
    sign-efi-sig-list -k KEK.key -c KEK.crt db DB.esl DB.auth
else
    for certificate in PK.crt KEK.crt DB.crt; do
        if [[ ! -f "$certificate" ]]; then
            echo "Missing certificate for --reuse-keys: $WORKDIR/$certificate" >&2
            exit 1
        fi
    done
fi

openssl verify -CAfile PK.crt KEK.crt
openssl verify -CAfile PK.crt -untrusted KEK.crt DB.crt

virt-fw-vars \
    --input "$OVMF_VARS" \
    --set-pk "$OWNER_GUID" PK.crt \
    --add-kek "$OWNER_GUID" KEK.crt \
    --add-db "$OWNER_GUID" DB.crt \
    --secure-boot \
    --output "$OUTPUT_VARS"

VARS_CONTENT="$(virt-fw-vars --input "$OUTPUT_VARS" --print --verbose)"
grep -Fq "subject CN=${PK_CN}" <<<"$VARS_CONTENT"
grep -Fq "subject CN=${KEK_CN}" <<<"$VARS_CONTENT"
grep -Fq "subject CN=${DB_CN}" <<<"$VARS_CONTENT"
grep -Fq "name=SecureBootEnable" <<<"$VARS_CONTENT"
grep -Fq "bool: ON" <<<"$VARS_CONTENT"

if [[ "$REUSE_KEYS" == false ]]; then
    chmod 600 PK.key KEK.key DB.key
    chmod 644 PK.auth KEK.auth DB.auth
fi
chmod 644 PK.crt KEK.crt DB.crt "$OUTPUT_VARS"

if [[ "$REUSE_KEYS" == false ]]; then
    echo "Secure Boot build inputs generated in $WORKDIR"
else
    echo "Reused Secure Boot certificates from $WORKDIR"
fi
echo "Provisioned OVMF variables written to $OUTPUT_VARS"
