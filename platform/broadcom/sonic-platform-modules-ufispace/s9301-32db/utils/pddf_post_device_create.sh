#!/bin/bash
#disable bmc watchdog
timeout 3 ipmitool mc watchdog off

#set bmc sel time
echo "Set BMC SEL time to system time"
timeout 5 ipmitool sel time set now > /dev/null 2>&1

echo 1 > /sys/kernel/pddf/devices/sysstatus/sysstatus_data/port_led_clr_ctrl

function execute_post_device_init {
    local pddf_py_script="/usr/local/bin/pddf_post_device_create.py"

    if [ -f "$pddf_py_script" ]; then
        echo "Executing post device create Python script..."
        if ! python3 "$pddf_py_script"; then
            echo "[WARNING] Python script execution failed"
        fi
    fi
}

execute_post_device_init
echo "PDDF device post-create completed"
