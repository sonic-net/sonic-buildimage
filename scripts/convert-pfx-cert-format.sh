#!/bin/bash -ex

usage()
{
    echo "Usage:  $0 -p <pfx_cert> -k <signing_key> -c <signing_cert> -a <ca_cert>"
    exit 1
}

while getopts "p:k:c:a:" opt; do
    case $opt in
        p)
            PFX_FILE=$OPTARG
            ;;
        k)
            SIGNING_KEY=$OPTARG
            ;;
        c)
            SIGNING_CERT=$OPTARG
            ;;
        a)
            CA_CERT=$OPTARG
            ;;
        *)
            usage
            ;;
    esac
done


( [ -z $PFX_FILE ] || [ -z $SIGNING_KEY ] || [ -z $SIGNING_CERT ] || [ -z $CA_CERT ] ) && exit 1

openssl pkcs12 -in "${PFX_FILE}" -clcerts -nokeys -nodes -passin pass: -out ${SIGNING_CERT}
openssl pkcs12 -in "${PFX_FILE}" -nocerts -nodes -passin pass: -out ${SIGNING_KEY}

# Export the last intermediate CA ceritficate
openssl pkcs12 -in "${PFX_FILE}" -cacerts -nokeys -nodes -passin pass: | sed -z -e "s/.*\(-----BEGIN CERTIFICATE\)/\1/" > ${CA_CERT}
