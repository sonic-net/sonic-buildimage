#! /bin/bash
## Check the platform PCIe device presence and status

VERBOSE="no"
RESULTS="PCIe Device Checking All Test"
EXPECTED="PCIe Device Checking All Test ----------->>> PASSED"
MAX_WAIT_SECONDS=15
PCIE_STATUS_TABLE="PCIE_DEVICES|status"

function debug()
{
    /usr/bin/logger "$0 : $1"
    if [[ x"${VERBOSE}" == x"yes" ]]; then
        echo "$(date) $0: $1"
    fi
}

function check_and_rescan_pcie_devices()
{
    PCIE_CHK_CMD='sudo pcieutil check | grep "$RESULTS"'
    PLATFORM=$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' platform)

    if [ ! -f /usr/share/sonic/device/$PLATFORM/pcie*.yaml ]; then
        debug "pcie.yaml does not exist! Can't check PCIe status!"
        exit
    fi

    begin=$SECONDS
    end=$((begin + MAX_WAIT_SECONDS))
    rescan_time=$((MAX_WAIT_SECONDS/2))
    rescan_time=$((begin + rescan_time))

    while true
    do
        now=$SECONDS
        if [[ $now -gt $end ]]; then
            break
        fi

        if [ "$(eval $PCIE_CHK_CMD)" = "$EXPECTED" ]; then
            sonic-db-cli STATE_DB HSET $PCIE_STATUS_TABLE "status" "PASSED"
            debug "PCIe check passed"
            exit
        else
            debug "sleep 0.1 seconds"
            sleep 0.1
        fi

        if [ $now -gt $rescan_time ]; then
            debug "PCIe check failed, try pci bus rescan"
            echo 1 > /sys/bus/pci/rescan
            rescan_time=$end
        fi

     done
     debug "PCIe check failed"
     sonic-db-cli STATE_DB HSET $PCIE_STATUS_TABLE "status" "FAILED"
}

check_and_rescan_pcie_devices
