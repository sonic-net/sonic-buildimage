#!/bin/sh
# This is a standalone test file to test read_conf_file function for
# some types of machine.conf file.
# The read_conf_file function is copy from the install.sh

MACHINE_CONF="sample_machine.conf"

read_conf_file() {
    local conf_file=$1
    while IFS='=' read -r var value || [ -n "$var" ]
    do
        # remove newline character
        var=$(echo $var | tr -d '\r\n')
        value=$(echo $value | tr -d '\r\n')
        # remove comment string
        var=${var%#*}
        value=${value%#*}
        # skip blank line
        [ -z "$var" ] && continue
        # remove double quote in the beginning
        tmp_val=${value#\"}
        # remove double quote in the end
        value=${tmp_val%\"}
        eval "$var=\"$value\""
    done < "$conf_file"
}

TEST_CONF() {
    input_value=$1
    exp_value=$2
    if [ "$input_value" != "$exp_value" ]; then
        echo "[ERR] Expect value($exp_value) is not equal to input value($input_value)"
        exit 1
    fi
}

# define the expected variable value
exp_onie_machine_rev="0"
exp_onie_arch="x86_64"
exp_onie_build_date="2021-02-03T01:50+0800"
exp_onie_partition_type="gpt"
exp_onie_disco_ntpsrv="192.168.0.1 192.168.0.2"
exp_onie_firmware="auto"
exp_onie_skip_ethmgmt_macs="no"
exp_onie_grub_image_name="grubx64.efi"

# read the sample conf file
read_conf_file $MACHINE_CONF

# check each variable and its expected value
TEST_CONF "$onie_machine_rev" "$exp_onie_machine_rev"
TEST_CONF "$onie_arch" "$exp_onie_arch"
TEST_CONF "$onie_build_date" "$exp_onie_build_date"
TEST_CONF "$onie_partition_type" "$exp_onie_partition_type"
TEST_CONF "$onie_disco_ntpsrv" "$exp_onie_disco_ntpsrv"
TEST_CONF "$onie_firmware" "$exp_onie_firmware"
TEST_CONF "$onie_skip_ethmgmt_macs" "$exp_onie_skip_ethmgmt_macs"
TEST_CONF "$onie_grub_image_name" "$exp_onie_grub_image_name"

echo "PASS!!"
exit 0
