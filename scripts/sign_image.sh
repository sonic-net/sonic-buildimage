#!/bin/bash -ex


IMAGE=""
SIGNING_KEY=""
SIGNING_CERT=""
CA_CERT=""

usage()
{
    echo "Usage:  $0 -i <image_path> [-k <signing_key> -c <signing_cert> -a <ca_cert>]"
    exit 1
}

generate_signing_key()
{
    TMP_CERT_PATH=$(mktemp -d)
    SIGNING_KEY="${TMP_CERT_PATH}/signing.key"
    SIGNING_CERT="${TMP_CERT_PATH}/signing.crt"
    SIGNING_CSR="${TMP_CERT_PATH}/signing.csr"
    CA_KEY="${TMP_CERT_PATH}/ca.key"

    # Generate the CA key and certificate
    openssl genrsa -out $CA_KEY 4096
    openssl req -x509 -new -nodes -key $CA_KEY -sha256 -days 3650 -subj "/C=US/ST=Test/L=Test/O=Test/CN=Test" -out $CA_CERT

    # Generate the signing key, certificate request and certificate
    openssl genrsa -out $SIGNING_KEY 4096
    openssl req -new -key $SIGNING_KEY -subj "/C=US/ST=Test/L=Test/O=Test/CN=Test" -out $SIGNING_CSR
    openssl x509 -req -in $SIGNING_CSR -CA $CA_CERT -CAkey $CA_KEY -CAcreateserial -out $SIGNING_CERT -days 1825 -sha256
}

while getopts "i:k:c:a:t:" opt; do
    case $opt in
        i)
            IMAGE=$OPTARG
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

[ -z $CA_CERT ] && echo "Not to sign the image since the CA certificate not provided" 1>&2 && exit 1

# Generate the self signed cert if not provided by input
[ ! -f $CA_CERT ] && generate_signing_key

# Verify the required files existing
[ ! -f $SIGNING_KEY ] && echo "$SIGNING_KEY not exist" && exit 1
[ ! -f $SIGNING_CERT ] && echo "$SIGNING_CERT not exist" && exit 1
[ ! -f $CA_CERT ] && echo "$CA_CERT not exist" && exit 1

# Prepare the image
swi-signature prepare $IMAGE

# Sign the image
swi-signature sign $IMAGE $SIGNING_CERT $CA_CERT --key $SIGNING_KEY

exit 0
