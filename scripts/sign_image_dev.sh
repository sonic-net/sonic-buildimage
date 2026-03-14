cert_file=$1
key_file=$2
image_to_sign=$3
cms_sig_out=$4
openssl cms -sign -nosmimecap -signer ${cert_file} -inkey ${key_file} -binary -in $image_to_sign -outform pem -out ${cms_sig_out} || {
    echo "$?: CMS sign error"
    sudo rm -rf ${cms_sig_out}
    exit 1
}
echo "CMS sign OK"
exit 0
