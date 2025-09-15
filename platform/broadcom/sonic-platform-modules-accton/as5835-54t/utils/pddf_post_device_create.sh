#!/bin/bash

set_qsfp_reset_to_normal_state()
{
    i2cset -y -f 3 0x62 0x15 0x3f &>/dev/null
}

set_system_led_diag_loc_to_off()
{
    i2cset -y -f 3 0x60 0xa 0x1c &>/dev/null
}

set_qsfp_reset_to_normal_state
set_system_led_diag_loc_to_off
