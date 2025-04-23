#!/bin/bash
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
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld1_build))
    echo "CPLD1: ${cpld1_ver}" >> $FIRMWARE_VERSION_FILE

    cpld2_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_build))
    echo "CPLD2: ${cpld2_ver}" >> $FIRMWARE_VERSION_FILE

    cpld3_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld3_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld3_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld3_build))
    echo "CPLD3: ${cpld3_ver}" >> $FIRMWARE_VERSION_FILE

    fpga_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/fpga_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/fpga_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/fpga_build))
    echo "FPGA: ${fpga_ver}" >> $FIRMWARE_VERSION_FILE

    bios_ver=$(cat /sys/class/dmi/id/bios_version)
    echo "BIOS: ${bios_ver}" >> $FIRMWARE_VERSION_FILE
    
    VERARR=(`ipmitool raw 0x6 0x1 | cut -d ' ' -f 4,5,16,15,14`)
    echo "BMC: ${VERARR[0]}.${VERARR[1]}.${VERARR[4]}.${VERARR[3]}${VERARR[2]}" >> ${FIRMWARE_VERSION_FILE}
}

function enable_i2c_realy {
    local items=(
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_i2c_ctrl"
        "/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld3_i2c_ctrl"
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

function diable_bmc_watchdog {
    echo "Disable BMC watchdog"
    timeout 3 ipmitool mc watchdog off
}

function set_mac_rov {
    echo "Set MAC rov"

    local bus_id="1"
    local rov_i2c_bus="13"
    local rov_i2c_addr=("0x64")
    local rov_config_reg="0x21"
    local rov_sysfs="/sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld_mac_rov"
    local mac_rov_done="/tmp/mac_rov_done"

    local avs_array=(
        0x84 0x86 0x88 0x8A 0x8C 0x8E
        0x90 0x92 0x94 0x96 0x98
    )

    local vdd_val=(
        '0.7875V'  '0.775V' '0.7625V' '0.75V' '0.7375V' '0.725V'
        '0.7125V'  '0.7V'   '0.6875V' '0.675V''0.6625V'
    )

    local vout_cmd=(
        '0x064F' '0x063B' '0x0622' '0x060E' '0x05F5' '0x05DC'
        '0x05C3' '0x05AA' '0x0591' '0x0578' '0x055F'
    )
    # Set MAC ROV Status
    if [ -f "$rov_sysfs" ]  ;then
        rov_reg=$(eval "cat $rov_sysfs")
        avs=$(( $rov_reg ))

        if [ -c "/dev/i2c-${rov_i2c_bus}" ]; then
            for (( i=0; i<${#rov_i2c_addr[@]}; i++ ))
            do
                for (( j=0; j<${#avs_array[@]}; j++ ))
                do
                    if [ $avs -eq $((${avs_array[j]})) ];then
                        bus=${rov_i2c_bus}
                        addr=${rov_i2c_addr[i]}
                        reg=${rov_config_reg}
                        val=${vout_cmd[j]}
                        echo "Set MAC($i) rov bus($bus) addr($addr) reg($reg) avs($(printf "0x%X" $avs)) val($val)"
                        if [ ! -f "$mac_rov_done" ]; then
                            $(i2cset -y $bus $addr $reg $val w)
                            $(touch "$mac_rov_done")
                        else
                            echo "  * MAC ROV has been set, skipping it."
                        fi
                        break;
                    fi
                done
            done
        fi
    fi
}


diable_bmc_watchdog
enable_i2c_realy
set_mac_rov
enable_event_control
set_led_default_val
platform_firmware_versions
echo "PDDF device post-create completed"
