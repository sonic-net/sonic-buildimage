#!/bin/bash

reboot_dir_found=false
reboot_file_found=false
smf_dir_missing=0
nvram_missing=0

REBOOT_REASON_FILE=/host/reboot-cause/platform/reboot_reason
BIOS_VERSION_FILE=/host/reboot-cause/platform/bios_minor_version
SMF_MSS_VERSION_FILE=/sys/devices/platform/SMF.512/hwmon/*/smf_firmware_ver
SMF_POWERON_REASON=/sys/devices/platform/SMF.512/hwmon/*/smf_poweron_reason
SMF_RESET_REASON=/sys/devices/platform/SMF.512/hwmon/*/smf_reset_reason
MAILBOX_POWERON_REASON=/sys/devices/platform/SMF.512/hwmon/*/mb_poweron_reason
NVRAM_DEVICE_FILE=/dev/nvram
RESET_REASON_FILE=/host/reboot-cause/platform/reset_reason
SMF_DIR=/sys/devices/platform/SMF.512/hwmon/
TCO_RESET_NVRAM_OFFSET=0x59

while [[ ! -d $SMF_DIR ]]
do
    sleep 0.5
    let smf_dir_missing=$smf_dir_missing+1
    if [[ "$smf_dir_missing" = "5" ]]; then
        echo "SMF is not initialized"
        smf_dir_missing=0
    fi
done

SMF_RESET=$(cat $SMF_RESET_REASON)
TCO_WD_RESET=0

if [[ -d /host/reboot-cause/platform ]]; then
    reboot_dir_found=true
    if [[ -f $REBOOT_REASON_FILE ]]; then
        reboot_file_found=true
    fi
fi

SMF_FPGA_VERSION=$((16#$(io_rd_wr.py --get --offset 0x200 | cut -d " " -f 3)))
SMF_BIOS_REG=$(io_rd_wr.py --get --offset 0x203 | cut -d " " -f 3)
SMF_BIOS_REG=$((16#$SMF_BIOS_REG))
bios_secondary_boot=$(($SMF_BIOS_REG & 1))

_get_smf_reset_register(){
    BIOS_VERSION=$(/usr/sbin/dmidecode -s system-version)
    BIOS_VERSION_MINOR=$(echo $BIOS_VERSION | cut -d'-' -f 2)

    if [[ $BIOS_VERSION_MINOR -gt 7 ]]; then
        echo $BIOS_VERSION > $BIOS_VERSION_FILE
    elif [[ "$bios_secondary_boot" = "0" ]]; then
        # For Primary BIOS with older version
        if [[ -e $BIOS_VERSION_FILE ]]; then
            rm $BIOS_VERSION_FILE
        fi
    fi

    if [[ -e $BIOS_VERSION_FILE ]]; then
        while [[ ! -e $NVRAM_DEVICE_FILE ]]
        do
            sleep 1
            let nvram_missing=$nvram_missing+1
            if [[ "$nvram_missing" = "5" ]]; then
                echo "NVRAM is not initialized"
                nvram_missing=0
            fi
        done
        first_reset=$(nvram_rd_wr.py --get --offset 0x5c | cut -d " " -f 2)
        second_reset=$(nvram_rd_wr.py --get --offset 0x5d | cut -d " " -f 2)
        third_reset=$(nvram_rd_wr.py --get --offset 0x5e | cut -d " " -f 2)
        fourth_reset=$(nvram_rd_wr.py --get --offset 0x5f | cut -d " " -f 2)

        if [[ "$first_reset" != "ee" ]]; then
            SMF_RESET=$first_reset
        fi

        # Saving NVRAM values for future debugging
        if [[ $reboot_dir_found = true ]]; then
            echo "First reset - $first_reset" > $RESET_REASON_FILE
            echo "Second reset - $second_reset" >> $RESET_REASON_FILE
            echo "Third reset - $third_reset" >> $RESET_REASON_FILE
            echo "Fourth reset - $fourth_reset" >> $RESET_REASON_FILE
        fi
        logger -p user.info -t DELL_Z9100_REBOOT_CAUSE "RST value in NVRAM: $first_reset, $second_reset, $third_reset, $fourth_reset"

        if [[ $BIOS_VERSION_MINOR -gt 8 ]]; then
            # Retrieve TCO reset status
            tco_nvram=$((16#$(nvram_rd_wr.py --get --offset $TCO_RESET_NVRAM_OFFSET | cut -d " " -f 2)))
            TCO_WD_RESET=$(($tco_nvram & 1))
            logger -p user.info -t DELL_Z9100_REBOOT_CAUSE "TCO status value in NVRAM: $TCO_WD_RESET"

            # Clear TCO reset status in NVRAM
            tco_nvram=$(printf "%x" $(($tco_nvram & 0xfe)))
            nvram_rd_wr.py --set --val $tco_nvram --offset $TCO_RESET_NVRAM_OFFSET
        fi

        # Clearing NVRAM values to holding next reset values
        nvram_rd_wr.py --set --val 0xee --offset 0x58
        nvram_rd_wr.py --set --val 0xee --offset 0x5c
        nvram_rd_wr.py --set --val 0xee --offset 0x5d
        nvram_rd_wr.py --set --val 0xee --offset 0x5e
        nvram_rd_wr.py --set --val 0xee --offset 0x5f
    fi
}

_is_thermal_reset() {
    prev_thermal=$(cat $REBOOT_REASON_FILE)
    curr_poweron_reason=$(cat $SMF_POWERON_REASON)
    if [[ $curr_poweron_reason = "11" ]]; then
        echo 0
        return
    fi
    if [[ $prev_thermal = $curr_poweron_reason ]]; then
        echo 2
        return
    else
        echo "$curr_poweron_reason" > $REBOOT_REASON_FILE
        echo 1
        return
    fi

    echo 0
    return
}

_is_watchdog_reset(){
    curr_reset_reason=$SMF_RESET
    if [[ $curr_reset_reason = "33" ]]; then
        echo 1
        return
    fi

    echo 0
    return
}

_is_software_reboot(){
    SMF_STATUS=`io_rd_wr.py --set --val 06 --offset 210; io_rd_wr.py --set --val 0B --offset 211; io_rd_wr.py --get --offset 212`
    SMF_STATUS=$(echo "$SMF_STATUS" | awk '{print $NF}')

    CPLD_STATUS=`io_rd_wr.py --get --offset 0x131`
    CPLD_STATUS=$(echo "$CPLD_STATUS" | awk '{print $NF}')

    if [[ $SMF_STATUS == "aa" ]]; then
        if [[ $CPLD_STATUS != "40" ]]; then
            # Cold reset happened; Resetting the registers
            io_rd_wr.py --set --val ff --offset 0x131
            io_rd_wr.py --set --val 06 --offset 210; io_rd_wr.py --set --val 0B --offset 211; io_rd_wr.py --set --val ff --offset 213
            echo 0x88 > $MAILBOX_POWERON_REASON
        else
            io_rd_wr.py --set --val 06 --offset 210; io_rd_wr.py --set --val 0B --offset 211; io_rd_wr.py --set --val ff --offset 213
            echo 0xaa > $MAILBOX_POWERON_REASON
        fi
    else
        echo 0xaa > $MAILBOX_POWERON_REASON
    fi
}

update_mailbox_register(){
    if [[ "$bios_secondary_boot" = "1" ]]; then
        echo "Secondary BIOS booted"
    fi

    if [[ $reboot_file_found = false ]]; then
        echo "None" > $REBOOT_REASON_FILE
    fi

    _get_smf_reset_register
    if [[ -d /sys/devices/platform/SMF.512/hwmon/ ]]; then
        por=$(cat $SMF_POWERON_REASON)
        rst=$(cat $SMF_RESET_REASON)
        mbr=$(cat $MAILBOX_POWERON_REASON)
        reason=$(echo $mbr | cut -d 'x' -f2)
        logger -p user.info -t DELL_Z9100_REBOOT_CAUSE "POR: $por, RST: $rst, MBR: $mbr"

        SMF_MSS_VERSION=$(cat $SMF_MSS_VERSION_FILE)
        SMF_MSS_VERSION_MAJOR=$(echo $SMF_MSS_VERSION | cut -d '.' -f1)
        SMF_MSS_VERSION_MINOR=$(echo $SMF_MSS_VERSION | cut -d '.' -f2)
        SMF_FPGA_VERSION_MAJOR=$(( (SMF_FPGA_VERSION & 0xF0) >> 4 ))
        SMF_FPGA_VERSION_MINOR=$(( SMF_FPGA_VERSION & 0x0F ))

        if [[ -e $BIOS_VERSION_FILE ]] \
           && [[ $SMF_MSS_VERSION_MAJOR -ge 2 ]] && [[ $SMF_MSS_VERSION_MINOR -ge 7 ]] \
           && [[ $SMF_FPGA_VERSION_MAJOR -ge 1 ]] && [[ $SMF_FPGA_VERSION_MINOR -ge 4 ]]; then

            if [[ $TCO_WD_RESET = 1 ]]; then
                echo 0xdd > $MAILBOX_POWERON_REASON
            elif [[ $reason = "cc" ]]; then
                _is_software_reboot
            elif [[ $SMF_RESET = "11" ]]; then
                echo 0xee > $MAILBOX_POWERON_REASON
            elif [[ $SMF_RESET = "33" ]]; then
                echo 0xdd > $MAILBOX_POWERON_REASON
            else
                echo 0x99 > $MAILBOX_POWERON_REASON
            fi

        else
            is_thermal_reboot=$(_is_thermal_reset)

            is_wd_reboot=$(_is_watchdog_reset)

            if [[ $reason = "ff" ]]; then
                echo "None" > $REBOOT_REASON_FILE
                echo 0xbb > $MAILBOX_POWERON_REASON
            elif [[ $is_thermal_reboot = 1 ]]; then
                echo 0xee > $MAILBOX_POWERON_REASON
            elif [[ $is_wd_reboot = 1 ]] && [[ $reason != "cc" ]]; then
                echo 0xdd > $MAILBOX_POWERON_REASON
            elif [[ $TCO_WD_RESET = 1 ]]; then
                echo 0xdd > $MAILBOX_POWERON_REASON
            elif [[ $reason = "cc" ]]; then
                _is_software_reboot
            else
                echo 0x99 > $MAILBOX_POWERON_REASON
            fi
        fi
    fi
}

update_mailbox_register
