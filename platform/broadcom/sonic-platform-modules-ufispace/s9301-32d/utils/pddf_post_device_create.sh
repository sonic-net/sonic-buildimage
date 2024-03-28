#!/bin/bash
echo 1 > /sys/kernel/pddf/devices/sysstatus/sysstatus_data/port_led_clr_ctrl
echo "PDDF device post-create completed"
