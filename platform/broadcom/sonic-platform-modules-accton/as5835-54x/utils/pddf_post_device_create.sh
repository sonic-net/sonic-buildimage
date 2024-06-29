#!/bin/bash

set_qsfp_reset_to_normal_state()
{
    i2cset -y -f 3 0x62 0x15 0x3f &>/dev/null
}


set_sfp_tx_disable_to_disable()
{
    i2cset -y -f 3 0x61 0x12 0x0 &>/dev/null
    i2cset -y -f 3 0x61 0x13 0x0 &>/dev/null
    i2cset -y -f 3 0x61 0x14 0x0 &>/dev/null
    i2cset -y -f 3 0x61 0x15 0x0 &>/dev/null
    i2cset -y -f 3 0x61 0x16 0x0 &>/dev/null
    i2cset -y -f 3 0x62 0xc 0x0  &>/dev/null
    i2cset -y -f 3 0x62 0xd 0x0  &>/dev/null
}

set_system_led_diag_loc_to_off()
{
    i2cset -y -f 3 0x60 0xa 0x1c &>/dev/null
}

set_qsfp_reset_to_normal_state
set_sfp_tx_disable_to_disable
set_system_led_diag_loc_to_off
