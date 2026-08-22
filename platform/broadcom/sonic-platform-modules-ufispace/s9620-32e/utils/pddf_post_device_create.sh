#!/bin/bash

# import sonic env
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment

TRUE=0
FALSE=1

PLATFORM=${PLATFORM:-x86_64-ufispace_s9620_32e-r0}

platform_firmware_versions() {
    FIRMWARE_VERSION_FILE=/var/log/firmware_versions
    rm -rf ${FIRMWARE_VERSION_FILE}

    major_minor=$(xxd -s 0x600 -p -l 1 /dev/port)
    major=$(((16#$major_minor & 2#11000000) >> 6))
    minor=$((16#$major_minor & 2#00111111))
    build=$((16#$(xxd -s 0x6E0 -p -l 1 /dev/port)))
    cpu_cpld_ver=$(printf '%d.%02d.%03d' $major $minor $build)
    echo "CPU CPLD: ${cpu_cpld_ver}" >> $FIRMWARE_VERSION_FILE

    cpld1_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld1_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld1_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld1_build_ver))
    echo "CPLD1: ${cpld1_ver}" >> $FIRMWARE_VERSION_FILE

    cpld2_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_build_ver))
    echo "CPLD2: ${cpld2_ver}" >> $FIRMWARE_VERSION_FILE

    cpld3_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld3_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld3_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld3_build_ver))
    echo "CPLD3: ${cpld3_ver}" >> $FIRMWARE_VERSION_FILE

    cpld4_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld4_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld4_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld4_build_ver))
    echo "CPLD4: ${cpld4_ver}" >> $FIRMWARE_VERSION_FILE

    fpga_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/fpga_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/fpga_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/fpga_build_ver))
    echo "FPGA: ${fpga_ver}" >> $FIRMWARE_VERSION_FILE

    bios_ver=$(cat /sys/class/dmi/id/bios_version)
    echo "BIOS: ${bios_ver}" >> $FIRMWARE_VERSION_FILE
    
    VERARR=(`ipmitool raw 0x6 0x1 2> /dev/null | cut -d ' ' -f 4,5,16,15,14`)
    echo "BMC: ${VERARR[0]}.${VERARR[1]}.${VERARR[4]}.${VERARR[3]}${VERARR[2]}" >> ${FIRMWARE_VERSION_FILE}
}

function check_filepath {
    local filepath="$1"
    local silent="${2:-0}"

    if [[ -z "$filepath" ]]; then
        (( silent )) || echo "[ERROR] The input string is empty!"
        return $FALSE
    fi

    if [[ ! -f "$filepath" ]]; then
        (( silent )) || echo "[ERROR] No such file: $filepath"
        return $FALSE
    fi

    return $TRUE
}

function init_bcm82399 {
    local epdm_cli_path="/usr/share/sonic/device/$PLATFORM/epdm_cli"

    if [[ ! -x "$epdm_cli_path" ]]; then
        echo "[ERROR] epdm_cli not found at $epdm_cli_path. Aborting."
        return $FALSE
    fi

    echo "Executing: $epdm_cli_path init -s 10G"
    
    if ! "$epdm_cli_path" init -s 10G; then
        local retcode=$?
        echo "[WARNING] Command failed: retcode=$retcode"
        return $retcode
    fi

    echo "[INFO] init_bcm82399 completed successfully."
    return $TRUE
}

function enable_i2c_relay {
    local items=(
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_i2c_ctrl"
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld3_i2c_ctrl"
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld4_i2c_ctrl"
    )
    local i=""

    echo "Set i2c control enable"
    for i in "${items[@]}"
    do
        reg=$(cat ${i})
        set_reg=$(( $reg | 2#10000000 ))
        echo $set_reg > ${i}
    done
}

function enable_event_control {
    local items=(
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld1_evt_ctrl"
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_evt_ctrl"
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld3_evt_ctrl"
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld4_evt_ctrl"
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/fpga_evt_ctrl"
    )

    local i=""

    echo "Set event control enable"
    for i in "${items[@]}"
    do
        reg=$(cat ${i})
        set_reg=$(( $reg | 2#00000001 ))
        echo $set_reg > ${i}
    done
}

function set_led_default_val {
    pddf_ledutil setstatusled LOC_LED off > /dev/null
    pddf_ledutil setstatusled SYNC_LED off > /dev/null
}

function disable_bmc_watchdog {
    echo "Disable BMC watchdog"
    timeout 3 ipmitool mc watchdog off > /dev/null 2>&1
}

function set_bmc_sel_time {
    echo "Set BMC sel time"
    timeout 3 ipmitool sel time set now > /dev/null 2>&1
}

function set_mac_rov {
    echo "Set MAC rov"

    local rov_i2c_bus="56"
    local rov_i2c_addr=("0x64" "0x68")
    local rov_config_reg="0x21"
    local rov_sysfs_paths=(
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld_mac_0_rov"
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld_mac_1_rov"
    )
    local mac_rov_done="/tmp/mac_rov_done"

    local avs_array=(
        0x7A 0x7C 0x7E 0x80 0x82 0x84 0x86 0x88 0x8A 
        0x8C 0x8E 0x90 0x92 0x94 0x96 0x98 0x9A
    )

    local vdd_val=(
        '0.85V' '0.8375V' '0.825V' '0.8125V' '0.8V' '0.7875V' '0.775V' '0.7625V' '0.75V' 
        '0.7375V' '0.725V' '0.7125V' '0.7V' '0.6875V' '0.675V' '0.6625V' '0.65V'
    )

    local vout_cmd=(
        '0x0366' '0x035A' '0x034E' '0x0342' '0x0336' '0x032A' '0x031A' '0x030E' '0x0300'
        '0x02F4' '0x02E8' '0x02DC' '0x02D0' '0x02C4' '0x02B8' '0x02A7' '0x029A'
    )
    # Set MAC ROV Status
     if [ ! -c "/dev/i2c-${rov_i2c_bus}" ]; then
        echo "I2C bus /dev/i2c-${rov_i2c_bus} not found"
        return
    fi

    for ((i = 0; i < ${#rov_sysfs_paths[@]}; i++)); do
        local sysfs_path="${rov_sysfs_paths[i]}"
        local addr="${rov_i2c_addr[i]}"
        local done_flag="/tmp/mac_rov_done_${i}"

        if [ ! -f "$sysfs_path" ]; then
            echo "Sysfs file not found: $sysfs_path, skipping MAC $i"
            continue
        fi

        local rov_reg=$(cat "$sysfs_path")
        local avs=$(( rov_reg ))

        for ((j = 0; j < ${#avs_array[@]}; j++)); do
            if [ $avs -eq $((${avs_array[j]})) ]; then
                local val=${vout_cmd[j]}
                local voltage=${vdd_val[j]}

                echo "Setting MAC $i: bus=$rov_i2c_bus addr=$addr reg=$rov_config_reg avs=0x$(printf "%X" $avs) ($voltage) -> vout_cmd=$val"

                if [ ! -f "$done_flag" ]; then
                    i2cset -y $rov_i2c_bus $addr $rov_config_reg $val w
                    touch "$done_flag"
                    echo "MAC $i configured"
                else
                    echo "MAC $i already configured, skipping"
                fi
                break
            fi
        done
    done
}

function execute_post_device_init {
    local pddf_py_script="/usr/local/bin/pddf_post_device_create.py"

    if [ -f "$pddf_py_script" ]; then
        echo "Executing post device create Python script..."
        if ! python3 "$pddf_py_script"; then
            echo "[WARNING] Python script execution failed"
        fi
    fi
}


disable_bmc_watchdog
set_bmc_sel_time
enable_i2c_relay
enable_event_control
set_mac_rov
init_bcm82399
set_led_default_val
platform_firmware_versions
execute_post_device_init

echo "PDDF device post-create completed"
